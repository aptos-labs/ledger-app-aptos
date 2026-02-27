from pathlib import Path

import pytest

from ragger.conftest import configuration


SCRIPT_DIR = Path(__file__).absolute().parent

# Ragger configuration: set firmware list, application path pattern
DEVICES = ["nanox", "nanosp", "stax", "flex"]

# This tells the Ragger framework where to find the compiled app.
# The `reusable_build.yml` workflow places binaries in build/<device>/bin/app.elf
configuration.OPTIONAL.BACKEND_SCOPE = "function"


def pytest_addoption(parser):
    # Allow specifying --model for backward compatibility
    pass


@pytest.fixture(scope="module")
def sw_h_path():
    sw_h_path = SCRIPT_DIR.parent / "src" / "sw.h"

    if not sw_h_path.is_file():
        raise FileNotFoundError(f"Can't find sw.h: '{sw_h_path}'")

    return sw_h_path
