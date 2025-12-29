import datetime
from pathlib import Path
import sys

if __name__ == "__main__":
    sys.path.append(str(Path().resolve() / "scraping" / "src"))

import yaml

from log import log


def _get_or_log(d: dict, key):
    v = d.get(key, None)
    if v is None:
        log.warning(f"config doesn't have value at a key '{key}'")

    return v


class MongoDBConfig:
    def __init__(self, settings: dict) -> None:
        address = _get_or_log(settings, "address")
        user = _get_or_log(settings, "user")
        password = _get_or_log(settings, "password")
        uri = ""
        if password is None:
            if user is None:
                uri = f"mongodb://{address}"
            else:
                uri = f"mongodb://{user}@{address}"
        else:
            uri = f"mongodb://{user}:{password}@{address}"

        self.uri = uri

        db = _get_or_log(settings, "db")
        if db is None:
            db = "scrapper_default"
        self.db = db

        collection = _get_or_log(settings, "collection")
        if collection is None:
            collection = "scrapper_default"
        self.collection = collection

    def __str__(self) -> str:
        return f"MongoDBConfig {{ uri: {self.uri} }}"


class SiteConfig:
    def __init__(
        self,
        crawl_delay: int,
        get_from: datetime.datetime,
        get_to: datetime.datetime,
        doc_limit: int,
    ) -> None:
        self.get_from = get_from
        self.get_to = get_to
        self.crawl_delay = crawl_delay
        self.doc_limit = doc_limit

    def __str__(self) -> str:
        return f"SiteConfig {{ get_from: {self.get_from}, get_to: {self.get_to}, crawl_delay: {self.crawl_delay}, doc_limit: {self.doc_limit} }}"


class Config:
    def __init__(self, path: Path) -> None:
        if not path.exists():
            log.error(f"could not read config: path {path} doesn't exist")

        values = dict()
        with open(path) as file:
            values = yaml.load(file.read())  # type: ignore

        log.debug(f"loaded config {values}")

        values_site_lvl = _get_or_log(values, "sites")
        self.sites: dict[str, SiteConfig] = dict()
        for site in values_site_lvl:
            site_settings_lvl = _get_or_log(values_site_lvl, site)
            if site_settings_lvl is None:
                log.error("could not parse config")
                exit(1)
            crawl_delay = _get_or_log(site_settings_lvl, "crawl_delay")
            get_from = datetime.datetime.fromisoformat(
                _get_or_log(site_settings_lvl, "get_from")
            )
            get_to = datetime.datetime.fromisoformat(
                _get_or_log(site_settings_lvl, "get_to")
            )
            doc_limit = _get_or_log(site_settings_lvl, "doc_limit")

            self.sites[site] = SiteConfig(crawl_delay, get_from, get_to, doc_limit)

        self.mongo_settings: MongoDBConfig = MongoDBConfig(
            _get_or_log(values, "mongo_settings")
        )

    def __str__(self) -> str:
        return f"Config {{ mongo_settings: {self.mongo_settings}, sites: {dict(map(lambda x: (x[0], str(x[1])), self.sites.items()))} }}"


if __name__ == "__main__":
    conf = Config(Path("scraper.yml"))
    print(conf)
