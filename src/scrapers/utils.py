import requests
from src.exception import ParserException


def get_from_url(url: str) -> str:
    response = requests.get(url)
    response.raise_for_status()
    if response.text is None:
        raise ParserException("Response text is None")
    return str(response.text)


def get_response(url: str) -> requests.Response:
    response = requests.get(url)
    response.raise_for_status()
    return response
