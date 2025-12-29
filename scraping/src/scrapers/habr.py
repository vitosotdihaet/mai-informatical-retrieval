from datetime import datetime
import time
from typing import Any
import math
import xml.etree.ElementTree as ET
from zoneinfo import ZoneInfo

from bs4 import BeautifulSoup

from common import ParsedScrap, Scrap, Source
from config.config import SiteConfig
from exception import ParserException
from scrapers.interfaces import IGetter, IParser
from scrapers.utils import get_from_url, get_response, parse_lastmod
from log import log


class HabrGetter(IGetter):
    def __init__(
        self,
        settings: SiteConfig,
    ) -> None:
        self.crawl_delay_secs = settings.crawl_delay
        self.from_lastmod = settings.get_from
        self.to_lastmod = settings.get_to
        self.article_limit = settings.doc_limit
        self.now = datetime.now()
        self._start_time = None

    def start_timer(self):
        self._start_time = time.monotonic()

    def end_timer(self):
        if self._start_time is None:
            return
        elapsed = time.monotonic() - self._start_time
        sleep_time = self.crawl_delay_secs - elapsed
        if sleep_time > 0:
            time.sleep(sleep_time)

    def get_articles_from_sitemap_index(self) -> set[Source]:
        articles: set[Source] = set()

        index_resp = get_response("https://habr.com/sitemap.xml")
        index_xml = index_resp.text.replace(
            ' xmlns="http://www.sitemaps.org/schemas/sitemap/0.9"', ""
        )
        index_root = ET.fromstring(index_xml)

        sitemap_counter = 0
        article_counter = 0

        for sitemap_entry in index_root.findall("sitemap"):
            self.start_timer()

            try:
                loc_elem = sitemap_entry.find("loc")
                if loc_elem is None or not loc_elem.text:
                    continue

                sitemap_url = loc_elem.text

                lastmod = parse_lastmod(
                    sitemap_entry.find("lastmod"), self.now
                ).replace(tzinfo=ZoneInfo("Europe/Moscow"))

                if lastmod < self.from_lastmod:
                    continue

                if "articles" not in sitemap_url:
                    continue

                sitemap_xml = get_response(sitemap_url).text.replace(
                    ' xmlns="http://www.sitemaps.org/schemas/sitemap/0.9"', ""
                )
                log.debug(f"got sitemap at {sitemap_url}")
                sitemap_root = ET.fromstring(sitemap_xml)

                sitemap_counter += 1
                if sitemap_counter % 10 == 0:
                    log.info(f"processed {sitemap_counter} sitemaps")

                for url_entry in sitemap_root.findall("url"):
                    loc = url_entry.find("loc")
                    if loc is None or not loc.text:
                        continue

                    article_url = loc.text

                    article_lastmod = parse_lastmod(
                        url_entry.find("lastmod"), self.now
                    ).replace(tzinfo=ZoneInfo("Europe/Moscow"))

                    if (
                        article_lastmod < self.from_lastmod
                        or self.to_lastmod < article_lastmod
                    ):
                        continue

                    articles.add(Source(article_url, article_lastmod))

                    article_counter += 1
                    if article_counter % 100 == 0:
                        log.info(f"got {article_counter} articles")

                    if self.article_limit and article_counter >= self.article_limit:
                        return articles

            except Exception as e:
                log.warning(f"failed sitemap {e}")

            self.end_timer()

        return articles

    def fetch_sources(self) -> set[Source]:
        return self.get_articles_from_sitemap_index()

    def fetch_scrap(self, sources: set[Source]) -> set[Scrap]:
        scrap = set()

        counter = 0
        for article in sources:
            self.start_timer()

            try:
                scrap.add(Scrap(article, get_from_url(article.path)))
            except Exception as e:
                log.error(f"could not fetch scrap: {e}")
            counter += 1
            if counter % 100 == 0:
                log.info(f"fetched {counter} articles")

            self.end_timer()

        return scrap

    def sleep(self):
        time.sleep(self.crawl_delay_secs)


class HabrParser(IParser):
    def info_scrap(self, scrap: set[Scrap]) -> Any:
        log.info(f"scrap is {len(scrap)} articles long")

    def info_parsed_scrap(self, parsed_scrap: set[ParsedScrap]) -> Any:
        parsed_scrap_len = len(parsed_scrap)
        if parsed_scrap_len == 0:
            log.warning("parsed zero articles")
            return

        values = []
        for e in parsed_scrap:
            if e.value is not None:
                values.append(e.value)

        avg_letter_count = sum(map(len, values)) / parsed_scrap_len
        avg_word_count = (
            sum(
                map(
                    lambda x: x.count(" "),
                    values,
                )
            )
            / parsed_scrap_len
        )

        log.info(f"scrap parsed into {parsed_scrap_len} articles")
        log.info(f"average letter count is {avg_letter_count:.2f}")
        log.info(f"average word count is {avg_word_count:.2f}")
        if avg_letter_count != 0:
            log.info(
                f"need at least {math.ceil(8 * 2**20 / avg_letter_count)} documents for the best mark"
            )

    def parse_scrap(
        self,
        scrap: set[Scrap],
    ) -> set[ParsedScrap]:
        """
        Parse Habr article HTML into plain text
        """
        parsed: set[ParsedScrap] = set()
        failed_count = 0
        counter = 0

        for article_content in scrap:
            try:
                soup = BeautifulSoup(article_content.value, "html.parser")

                content_container = (
                    soup.find("div", class_="article-body")
                    or soup.find("div", attrs={"data-article-content": True})
                    or soup.find("div", id="post-content-body")
                )

                title = "No title"
                title_el = soup.find("h1")
                if title_el:
                    title = title_el.get_text(strip=True)

                if not content_container:
                    raise ParserException("Could not find article content container")

                blocks = content_container.find_all(
                    ["h1", "h2", "h3", "h4", "h5", "h6", "p", "li", "blockquote", "pre"]
                )

                parts: list[str] = []

                for block in blocks:
                    text = (
                        block.get_text(separator=" ")
                        .replace("\xa0", " ")
                        .replace("\n", " ")
                        .strip()
                    )

                    if not text:
                        continue

                    # normalize whitespace
                    text = " ".join(text.split())
                    parts.append(text)

                article_text = " ".join(parts)

                parsed.add(
                    ParsedScrap(article_content.source, f"{title} - {article_text}")
                )

            except Exception as e:
                log.debug(f"failed to parse habr article: {e}")
                failed_count += 1
                parsed.add(ParsedScrap(article_content.source, None))

            counter += 1
            if counter % 100 == 0:
                log.info(f"parsed {counter} articles")

        log.warning(f"failed to parse {failed_count}/{len(scrap)} articles")
        return parsed
