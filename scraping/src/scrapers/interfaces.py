from typing import Any, Protocol

from common import ParsedScrap, Scrap, Source


class IGetter(Protocol):
    """
    # Usage

    ```
    getter = Getter(config)
    sources = getter.fetch_sources() # the source of scraping can be actually anything - a csv/html/xml/etc
    sources = sources.filter(...) # filter the sources
    scrap = getter.fetch_scrap(sources) # get the files that will be parsed later
    ```

    # Examples

    If we want to create a web-scraper, it should get all the urls from e.g. a sitemap and return them in `fetch_sources`

    After getting the sources we might filter them (e.g. by time modified or the regex of the url)
    """

    def fetch_sources(self) -> set[Source]:
        """
        Get the sources for the scrap
        """
        ...

    def fetch_scrap(self, sources: set[Source]) -> set[Scrap]:
        """
        Get the scrap we want to parse later
        """
        ...


class IParser(Protocol):
    """
    # Usage

    ```
    parser = Parser(config)
    parsed = parser.parse_scrap(scrap) # parse the files
    scrap_info = parser.info_scrap(scrap) # get info about scrap
    parsed_scrap_info = parser.info_parsed_scrap(scrap) # get info about the parsed scrap
    ```
    """

    def parse_scrap(
        self,
        scrap: set[Scrap],
    ) -> set[ParsedScrap]:
        """
        Parse the provided files
        """
        ...

    def info_scrap(self, scrap: set[Scrap]) -> Any:
        """
        Get some information about the scrap
        """
        ...

    def info_parsed_scrap(self, parsed_scrap: set[ParsedScrap]) -> Any:
        """
        Get some information about the parsed scrap
        """
        ...
