from datetime import datetime
from enum import Enum
import time
from typing import Any
import math
import xml.etree.ElementTree as ET
from zoneinfo import ZoneInfo

from bs4 import BeautifulSoup

from common import ParsedScrap, Scrap, Source
from exception import ParserException
from scrapers.interfaces import IGetter, IParser
from scrapers.utils import get_from_url, get_response
from log import log


class RBCCategory(Enum):
    All = "all"
    Invest = "Invest"

    def __str__(self) -> str:
        return self.value


class RBCGetterType(Enum):
    Category = 0
    Sitemap = 1


class RBCGetter(IGetter):
    def __init__(
        self,
        crawl_delay_secs: float,
        type: RBCGetterType,
        from_lastmod: datetime,
        to_lastmod: datetime,
        article_limit: None | int = None,
        category: None | RBCCategory = None,
    ) -> None:
        self.crawl_delay_secs = crawl_delay_secs
        self.type = type
        self.from_lastmod = from_lastmod
        self.to_lastmod = to_lastmod
        self.article_limit = article_limit
        self.category = category
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
        articles = set()
        sitemap_index = get_response("https://www.rbc.ru/sitemap_index.xml")
        sitemap_index_text = sitemap_index.text.replace(
            ' xmlns="http://www.sitemaps.org/schemas/sitemap/0.9"', ""
        )
        sitemap_index_root = ET.fromstring(sitemap_index_text)

        sitemap_counter = 0
        article_counter = 0
        stop = False

        for index_entry in sitemap_index_root.findall("sitemap"):
            self.start_timer()

            try:
                loc = index_entry.find("loc")
                if loc is None or loc.text is None:
                    continue

                loc = loc.text

                lastmod = index_entry.find("lastmod")
                if lastmod is not None:
                    lastmod = lastmod.text
                    if lastmod is None:
                        lastmod = self.now
                    else:
                        lastmod = datetime.fromisoformat(lastmod)
                else:
                    lastmod = self.now

                lastmod = lastmod.replace(tzinfo=ZoneInfo("Europe/Moscow"))
                if lastmod < self.from_lastmod:
                    continue

                sitemap = get_response(loc).text.replace(
                    ' xmlns="http://www.sitemaps.org/schemas/sitemap/0.9"', ""
                )
                log.debug(f"got sitemap at {loc}")
                sitemap_root = ET.fromstring(sitemap)

                sitemap_counter += 1
                if sitemap_counter % 10 == 0:
                    log.info(f"got {sitemap_counter} sitemaps")

                for article_entry in sitemap_root.findall("url"):
                    try:
                        url = article_entry.find("loc")
                        if url is None or url.text is None:
                            continue

                        url = url.text

                        lastmod = article_entry.find("lastmod")
                        if lastmod is not None:
                            lastmod = lastmod.text
                            if lastmod is None:
                                lastmod = self.now
                            else:
                                lastmod = datetime.fromisoformat(lastmod)
                        else:
                            lastmod = self.now

                        lastmod = lastmod.replace(tzinfo=ZoneInfo("Europe/Moscow"))
                        if lastmod < self.from_lastmod or self.to_lastmod < lastmod:
                            continue

                        articles.add(Source(url, lastmod))
                        log.debug(f"got article at {url}")

                        article_counter += 1
                        if article_counter % 100 == 0:
                            log.info(f"got {article_counter} articles")
                        if (
                            self.article_limit is not None
                            and article_counter >= self.article_limit
                        ):
                            stop = True
                            break
                    except Exception as e:
                        log.warning(
                            f"could not fetch the source from sitemap entry {e}"
                        )
                        continue
            except Exception as e:
                log.warning(f"could not fetch sources from the sitemap index entry {e}")

            if stop:
                break
            self.end_timer()

        return articles

    def get_latest_article_urls_from_category(
        self, category: RBCCategory
    ) -> set[Source]:
        """
        Get latest urls of the articles for the provided category
        """
        source_category = f"https://www.rbc.ru/quote/category/{category}/"
        log.info(f"source category is {source_category}")

        try:
            category_content = get_from_url(source_category)
        except Exception as e:
            log.error(f"could not get article from category: {e}")
            return set()
        soup = BeautifulSoup(category_content, "html.parser")

        articles = set()

        for article in soup.find_all("a", class_="q-item__link"):
            log.info(f"fetched link {article} from category {source_category}")
            articles.add(Source(article["href"]))  # type: ignore

        return articles

    def fetch_sources(self) -> set[Source]:
        if self.type == RBCGetterType.Category:
            if self.category is None:
                self.category = RBCCategory.All
            return self.get_latest_article_urls_from_category(self.category)

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


class RBCParser(IParser):
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
        Parse an article content into plain text
        """
        parsed = set()
        failed_count = 0
        counter = 0
        for article_content in scrap:
            try:
                soup = BeautifulSoup(article_content.value, "html.parser")

                content_container = soup.find("div", class_="article__text")
                if not content_container:
                    raise ParserException("Could not find article text")

                title_element = soup.find("h1")
                title = (
                    title_element.get_text(strip=True) if title_element else "No title"
                )

                article_text = " ".join(
                    [
                        " ".join(
                            element.get_text()
                            .strip(" \n")
                            .replace("\xa0", " ")
                            .replace("\n", " ")
                            .split()
                        )
                        for element in content_container.find_all(  # type: ignore
                            ["h1", "h2", "h3", "h4", "h5", "h6", "p"]
                        )
                    ]
                )

                parsed.add(
                    ParsedScrap(article_content.source, f"{title} - {article_text}")
                )
            except Exception as e:
                log.debug(f"failed to parse article: {e}")
                failed_count += 1
                parsed.add(ParsedScrap(article_content.source, None))

            counter += 1
            if counter % 100 == 0:
                log.info(f"parsed {counter} articles")

        log.warning(f"failed to parse {failed_count}/{len(scrap)} articles")

        return parsed
