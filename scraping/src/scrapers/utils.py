from datetime import datetime
import requests
from exception import ParserException

HEADERS = {
    "User-Agent": "VitosScrapper/1.0",
    "Accept": "text/html,application/xml;q=0.9,*/*;q=0.8",
    "Accept-Language": "ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7",
}
TIMEOUT = 10


def get_from_url(url: str) -> str:
    response = requests.get(url, headers=HEADERS, timeout=TIMEOUT)
    response.raise_for_status()
    if response.text is None:
        raise ParserException("Response text is None")
    return str(response.text)


def get_response(url: str) -> requests.Response:
    response = requests.get(url, headers=HEADERS, timeout=TIMEOUT)
    response.raise_for_status()
    return response


def parse_lastmod(elem, default):
    if elem is None or elem.text is None:
        return default
    return datetime.fromisoformat(elem.text.replace("Z", "+00:00"))
