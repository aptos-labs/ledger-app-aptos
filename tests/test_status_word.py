"""Tests that all status words defined in sw.h are accounted for.

This is a meta-test that parses sw.h and ensures the test suite is aware
of all possible error codes the firmware can return.
"""

import re
from pathlib import Path
from typing import List, Tuple


SW_RE = re.compile(r"""(?x)
    \#                                 # character '#'
    define                             # string 'define'
    \s+                                # spaces
    (?P<identifier>SW(?:_[A-Z0-9]+)*)  # identifier (e.g. 'SW_OK')
    \s+                                # spaces
    0x(?P<sw>[a-fA-F0-9]{4})           # 4 bytes status word
""")

# All non-OK status words that the app can return
EXPECTED_ERROR_WORDS = {
    0x6985,  # SW_DENY
    0x6A86,  # SW_WRONG_P1P2
    0x6A87,  # SW_WRONG_DATA_LENGTH
    0x6D00,  # SW_INS_NOT_SUPPORTED
    0x6E00,  # SW_CLA_NOT_SUPPORTED
    0xB000,  # SW_WRONG_RESPONSE_LENGTH
    0xB001,  # SW_DISPLAY_BIP32_PATH_FAIL
    0xB002,  # SW_DISPLAY_ADDRESS_FAIL
    0xB003,  # SW_DISPLAY_AMOUNT_FAIL
    0xB004,  # SW_WRONG_TX_LENGTH
    0xB005,  # SW_TX_PARSING_FAIL
    0xB006,  # SW_TX_HASH_FAIL
    0xB007,  # SW_BAD_STATE
    0xB008,  # SW_SIGNATURE_FAIL
}


def parse_sw(path: Path) -> List[Tuple[str, int]]:
    """Parse status word definitions from sw.h."""
    if not path.is_file():
        raise FileNotFoundError(f"Can't find sw.h: '{path}'")

    sw_h: str = path.read_text()
    return [(identifier, int(sw, base=16))
            for identifier, sw in SW_RE.findall(sw_h)]


def test_status_words_match_header(sw_h_path):
    """Verify all status words in sw.h are known to the test suite."""
    parsed = parse_sw(sw_h_path)

    # Separate OK from error codes
    error_words_from_header = {sw for (name, sw) in parsed if sw != 0x9000}

    assert error_words_from_header == EXPECTED_ERROR_WORDS, (
        f"Mismatch between sw.h and test suite.\n"
        f"In sw.h but not in tests: {error_words_from_header - EXPECTED_ERROR_WORDS}\n"
        f"In tests but not in sw.h: {EXPECTED_ERROR_WORDS - error_words_from_header}"
    )


def test_sw_ok_defined(sw_h_path):
    """Verify SW_OK (0x9000) is defined in sw.h."""
    parsed = parse_sw(sw_h_path)
    ok_entries = [(name, sw) for (name, sw) in parsed if sw == 0x9000]
    assert len(ok_entries) == 1
    assert ok_entries[0][0] == "SW_OK"
