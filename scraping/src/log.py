import logging
import sys

log = logging.getLogger()
log.setLevel(logging.DEBUG)
if not log.hasHandlers():
    handler = logging.StreamHandler(sys.stdout)
    formatter = logging.Formatter("%(asctime)s: %(levelname)s - %(message)s")
    handler.setFormatter(formatter)
    log.addHandler(handler)

log.debug("logger is set up")
