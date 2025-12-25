import requests
from exception import ParserException


proxies = {
    "http": "socks5h://127.0.0.1:1080",
    "https": "socks5h://127.0.0.1:1080",
}


def get_from_url(url: str) -> str:
    response = requests.get(url, proxies=proxies)
    response.raise_for_status()
    if response.text is None:
        raise ParserException("Response text is None")
    return str(response.text)


def get_response(url: str) -> requests.Response:
    response = requests.get(url, proxies=proxies)
    response.raise_for_status()
    return response
