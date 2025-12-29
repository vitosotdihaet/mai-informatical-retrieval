from typing import Any
from pymongo import MongoClient
from pymongo.errors import BulkWriteError
from common import ParsedScrap

from config.config import MongoDBConfig
from log import log


class MongoSaver:
    def __init__(self, settings: MongoDBConfig) -> None:
        self.client = MongoClient(settings.uri)
        self.db = self.client[settings.db]
        self.collection = self.db[settings.collection]
        self.collection.create_index("source", unique=True)

    def save_parsed_scrap(self, parsed_scrap: set[ParsedScrap]) -> None:
        documents: list[dict[str, Any]] = [
            {
                "source": scrap.source.path,
                "lastmod": scrap.source.datetime,
                "value": scrap.value,
            }
            for scrap in parsed_scrap
        ]

        try:
            self.collection.insert_many(documents, ordered=False)
        except BulkWriteError as e:
            duplicate_errors = [
                err
                for err in e.details.get("writeErrors", [])
                if err.get("code") == 11000
            ]

            skipped = len(duplicate_errors)
            saved = len(documents) - skipped
            log.warning(f"inserted {saved}, skipped {skipped} duplicates.")

            other_errors = [
                err
                for err in e.details.get("writeErrors", [])
                if err.get("code") != 11000
            ]

            if other_errors:
                raise
