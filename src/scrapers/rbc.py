from enum import Enum
import time
from typing import Any
import math
import xml.etree.ElementTree as ET

from bs4 import BeautifulSoup

from src.exception import ParserException
from src.interfaces import IGetter, IParser
from src.scrapers.utils import get_from_url, get_response
from src.log import log


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
        from_lastmod: str,
        category: None | RBCCategory = None,
    ) -> None:
        self.crawl_delay_secs = crawl_delay_secs
        self.type = type
        self.from_lastmod = from_lastmod
        self.category = category

    def get_articles_from_sitemap_index(self) -> set[str]:
        articles = set()
        sitemap_index = get_response("https://www.rbc.ru/sitemap_index.xml")
        sitemap_index_text = sitemap_index.text.replace(
            ' xmlns="http://www.sitemaps.org/schemas/sitemap/0.9"', ""
        )
        sitemap_index_root = ET.fromstring(sitemap_index_text)

        sitemap_counter = 0
        article_counter = 0

        for index_entry in sitemap_index_root.findall("sitemap"):
            loc = index_entry.find("loc")
            if loc is None or loc.text is None:
                continue

            loc = loc.text

            lastmod = index_entry.find("lastmod")
            if lastmod is not None:
                lastmod = lastmod.text
                if lastmod is not None and lastmod < self.from_lastmod:
                    continue

            self.sleep()
            sitemap = get_response(loc).text.replace(
                ' xmlns="http://www.sitemaps.org/schemas/sitemap/0.9"', ""
            )
            log.debug(f"got sitemap at {loc}")
            sitemap_root = ET.fromstring(sitemap)

            sitemap_counter += 1
            if sitemap_counter % 100 == 0:
                log.info(f"got {sitemap_counter} sitemaps")

            for article_entry in sitemap_root.findall("url"):
                url = article_entry.find("loc")
                if url is None or url.text is None:
                    continue

                url = url.text

                lastmod = article_entry.find("lastmod")
                if lastmod is not None:
                    lastmod = lastmod.text
                    if lastmod is not None and lastmod < self.from_lastmod:
                        continue

                articles.add(url)
                log.debug(f"got article at {url}: {articles}")

                article_counter += 1
                if article_counter % 100 == 0:
                    log.info(f"got {article_counter} articles")

        return articles

    def get_latest_article_urls_from_category(self, category: RBCCategory) -> set[str]:
        """
        Get latest urls of the articles for the provided category
        """
        source_category = f"https://www.rbc.ru/quote/category/{category}/"
        log.info(f"source category is {source_category}")

        category_content = get_from_url(source_category)
        soup = BeautifulSoup(category_content, "html.parser")

        articles = set()

        for article in soup.find_all("a", class_="q-item__link"):
            log.info(f"fetched link {article} from category {source_category}")
            articles.add(article["href"])  # type: ignore

        return articles

    def fetch_sources(self) -> set[str]:
        if self.type == RBCGetterType.Category:
            if self.category is None:
                self.category = RBCCategory.All
            return self.get_latest_article_urls_from_category(self.category)

        return self.get_articles_from_sitemap_index()

    def fetch_scrap(self, sources: set[str]) -> set[str]:
        scrap = set()

        for article in sources:
            self.sleep()
            scrap.add(get_from_url(article))

        return scrap

    def sleep(self):
        time.sleep(self.crawl_delay_secs)


class RBCParser(IParser):
    def info_scrap(self, scrap: set[str]) -> Any:
        log.info(f"scrap is {len(scrap)} articles length")

    def info_parsed_scrap(self, parsed_scrap: set[str]) -> Any:
        parsed_scrap_len = len(parsed_scrap)
        if parsed_scrap_len == 0:
            log.warning("parsed zero articles")
            return

        avg_letter_count = sum(map(len, parsed_scrap)) / parsed_scrap_len
        avg_word_count = (
            sum(map(lambda t: t.count(" "), parsed_scrap)) / parsed_scrap_len
        )

        log.info(f"scrap parsed into {parsed_scrap_len} articles")
        log.info(f"average letter count is {avg_letter_count:.2f}")
        log.info(f"average word count is {avg_word_count:.2f}")
        log.info(
            f"need at least {math.ceil(8 * 2**20 / avg_letter_count)} documents for the best mark"
        )

    def parse_scrap(
        self,
        scrap: set[str],
    ) -> set[str]:
        """
        Parse an article content into plain text
        """
        parsed = set()
        failed_count = 0
        for article_content in scrap:
            try:
                soup = BeautifulSoup(article_content, "html.parser")

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

                parsed.add(f"{title} - {article_text}")
            except Exception as e:
                log.warning(f"failed to parse article: {e}")
                failed_count += 1

        log.warning(f"failed to parse {failed_count} articles")

        return parsed
