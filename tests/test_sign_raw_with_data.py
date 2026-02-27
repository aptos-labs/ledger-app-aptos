"""Tests for signing RawTransactionWithData (TX_RAW_WITH_DATA variant).

The TX_RAW_WITH_DATA variant is identified by the SHA3-256 hash of
"APTOS::RawTransactionWithData" as the first 32 bytes. The parser returns
PARSING_OK without further field parsing. The app displays it as a
generic "APTOS::RawTransactionWithData" transaction with gas fee.
"""

import struct

from nacl.signing import VerifyKey

from ragger.backend.interface import BackendInterface
from ragger.firmware import Firmware
from ragger.navigator import Navigator, NavInsID

from utils import (
    CLA, INS_GET_PUBLIC_KEY, INS_SIGN_TX, SW_OK,
    bip32_path_to_bytes, chunkify, MAX_APDU_LEN,
)

BIP32_PATH = "m/44'/637'/1'/0'/0'"


def build_raw_tx_with_data() -> bytes:
    """Construct a minimal RawTransactionWithData payload.

    Since the parser doesn't parse beyond the prefix for this variant,
    we include the prefix followed by some dummy payload data.
    """
    # SHA3-256("APTOS::RawTransactionWithData")
    prefix = bytes.fromhex(
        "5efa3c4f02f83a0f4b2d69fc95c607cc02825cc4e7be536ef0992df050d9e67c"
    )
    # Dummy payload - must be enough bytes for a valid-looking transaction
    # The parser only checks the prefix, so the content after it doesn't matter
    # for parsing success, but UI will try to calculate gas from uninitialized
    # (zeroed) fields, showing gas fee as 0.
    dummy_payload = b"\x00" * 64
    return prefix + dummy_payload


def test_sign_raw_tx_with_data(backend: BackendInterface, firmware: Firmware,
                               navigator: Navigator):
    """Sign a RawTransactionWithData and verify the Ed25519 signature."""
    message = build_raw_tx_with_data()

    # Get public key
    cdata = bip32_path_to_bytes(BIP32_PATH)
    rapdu = backend.exchange(CLA, INS_GET_PUBLIC_KEY, p1=0, p2=0, data=cdata)
    assert rapdu.status == SW_OK
    pub_key = rapdu.data[1:rapdu.data[0] + 1]
    pk = VerifyKey(pub_key[1:])

    # Send BIP32 path
    bip_data = bip32_path_to_bytes(BIP32_PATH)
    rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=0, p2=0x80, data=bip_data)
    assert rapdu.status == SW_OK

    # Send transaction data
    with backend.exchange_async(CLA, INS_SIGN_TX, p1=1, p2=0x00, data=message):
        if firmware.is_nano:
            # Review Transaction > Tx Type ("APTOS::RawTransactionWithData") > Gas Fee > Approve
            navigator.navigate_until_text_and_compare(
                NavInsID.RIGHT_CLICK,
                [NavInsID.BOTH_CLICK],
                "Approve",
                screen_change_before_first_instruction=True,
            )
        else:
            navigator.navigate_until_text_and_compare(
                NavInsID.USE_CASE_REVIEW_TAP,
                [NavInsID.USE_CASE_REVIEW_CONFIRM,
                 NavInsID.USE_CASE_STATUS_DISMISS],
                "Hold to sign",
                screen_change_before_first_instruction=True,
            )

    rapdu = backend.last_async_response
    assert rapdu.status == SW_OK

    sig_len = rapdu.data[0]
    sig = rapdu.data[1:1 + sig_len]
    pk.verify(signature=sig, smessage=message)
