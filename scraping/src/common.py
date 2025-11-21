from datetime import datetime


class Source:
    def __init__(self, path: str, dt: datetime) -> None:
        self.path: str = path
        self.datetime: datetime = dt

    def __str__(self) -> str:
        return f"{self.path} --- {self.datetime}"


class Scrap:
    def __init__(self, source: Source, value: str) -> None:
        self.source: Source = source
        self.value: str = value


class ParsedScrap:
    def __init__(self, source: Source, value: None | str) -> None:
        self.source: Source = source
        self.value: None | str = value
