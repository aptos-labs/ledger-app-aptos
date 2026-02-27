import struct
from typing import Iterator, Tuple

CLA = 0x5B
INS_GET_VERSION = 0x03
INS_GET_APP_NAME = 0x04
INS_GET_PUBLIC_KEY = 0x05
INS_SIGN_TX = 0x06

MAX_APDU_LEN = 255

# Status words
SW_OK = 0x9000
SW_DENY = 0x6985
SW_WRONG_P1P2 = 0x6A86
SW_WRONG_DATA_LENGTH = 0x6A87
SW_INS_NOT_SUPPORTED = 0x6D00
SW_CLA_NOT_SUPPORTED = 0x6E00
SW_WRONG_RESPONSE_LENGTH = 0xB000
SW_DISPLAY_BIP32_PATH_FAIL = 0xB001
SW_DISPLAY_ADDRESS_FAIL = 0xB002
SW_DISPLAY_AMOUNT_FAIL = 0xB003
SW_WRONG_TX_LENGTH = 0xB004
SW_TX_PARSING_FAIL = 0xB005
SW_TX_HASH_FAIL = 0xB006
SW_BAD_STATE = 0xB007
SW_SIGNATURE_FAIL = 0xB008


def bip32_path_to_bytes(path: str) -> bytes:
    """Convert BIP32 path string like "m/44'/637'/1'/0'/0'" to APDU payload bytes."""
    elements = path.replace("m/", "").split("/")
    result = b""
    for elem in elements:
        if elem.endswith("'"):
            value = int(elem[:-1]) | 0x80000000
        else:
            value = int(elem)
        result += struct.pack(">I", value)
    return bytes([len(elements)]) + result


def chunkify(data: bytes, chunk_len: int) -> Iterator[Tuple[bool, bytes]]:
    """Split data into chunks, yielding (is_last, chunk) tuples."""
    size = len(data)
    if size <= chunk_len:
        yield True, data
        return

    offset = 0
    while offset < size:
        end = min(offset + chunk_len, size)
        is_last = (end == size)
        yield is_last, data[offset:end]
        offset = end
