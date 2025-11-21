from typing import Protocol

from common import ParsedScrap


class ISaver(Protocol):
    """
    # Usage

    ```
    saver = Saver(config)
    saver.save_parsed_scrap(scrap)
    ```
    """

    def save_parsed_scrap(self, parsed_scrap: set[ParsedScrap]): ...
