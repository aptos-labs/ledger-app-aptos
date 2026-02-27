"""Tests for user rejection flows.

When the user rejects a transaction or address confirmation, the app
should return SW_DENY (0x6985).
"""

from ragger.backend.interface import BackendInterface, RaisePolicy
from ragger.firmware import Firmware
from ragger.navigator import Navigator, NavInsID

from utils import (
    CLA, INS_GET_PUBLIC_KEY, INS_SIGN_TX, SW_DENY,
    bip32_path_to_bytes, chunkify, MAX_APDU_LEN,
)

BIP32_PATH = "m/44'/637'/1'/0'/0'"

# The same coin transfer message from test_sign_raw_cmd.py
COIN_TRANSFER_TX = bytes.fromhex(
    "b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b193"
    "783135e8b00430253a22ba041d860c373d7a1501ccf7ac2d1ad37a8ed2775aee"
    "000000000000000002000000000000000000000000000000000000000000000000"
    "00000000000000000104636f696e087472616e73666572010700000000000000000"
    "000000000000000000000000000000000000000000000000001"
    "0a6170746f735f636f696e094170746f73436f696e0002"
    "20094c6fc0d3b382a599c37e1aaa7618eff2c96a3586876082c4594c50c50d7dde"
    "082a00000000000000"
    "204e0000000000006400000000000000565c51630000000022"
)


def test_reject_transaction(backend: BackendInterface, firmware: Firmware, navigator: Navigator):
    """User rejects a coin transfer transaction."""
    # Send BIP32 path
    bip_data = bip32_path_to_bytes(BIP32_PATH)
    rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=0, p2=0x80, data=bip_data)
    assert rapdu.status == 0x9000

    # Send transaction data chunks
    for i, (is_last, chunk) in enumerate(chunkify(COIN_TRANSFER_TX, MAX_APDU_LEN)):
        if is_last:
            # Don't raise on non-0x9000 status - we expect SW_DENY
            backend.raise_policy = RaisePolicy.RAISE_NOTHING
            with backend.exchange_async(CLA, INS_SIGN_TX, p1=i + 1, p2=0x00, data=chunk):
                if firmware.is_nano:
                    # Navigate to "Reject" instead of "Approve"
                    navigator.navigate_until_text_and_compare(
                        NavInsID.RIGHT_CLICK,
                        [NavInsID.BOTH_CLICK],
                        "Reject",
                        screen_change_before_first_instruction=True,
                    )
                else:
                    # On Stax/Flex: reject the review
                    navigator.navigate_until_text_and_compare(
                        NavInsID.USE_CASE_REVIEW_TAP,
                        [NavInsID.USE_CASE_REVIEW_REJECT,
                         NavInsID.USE_CASE_CHOICE_CONFIRM,
                         NavInsID.USE_CASE_STATUS_DISMISS],
                        "Reject",
                        screen_change_before_first_instruction=True,
                    )

            rapdu = backend.last_async_response
            assert rapdu.status == SW_DENY
        else:
            rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=i + 1, p2=0x80, data=chunk)
            assert rapdu.status == 0x9000


def test_reject_message(backend: BackendInterface, firmware: Firmware, navigator: Navigator):
    """User rejects a message signing request."""
    message = b"Sign this message please"

    # Send BIP32 path
    bip_data = bip32_path_to_bytes(BIP32_PATH)
    rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=0, p2=0x80, data=bip_data)
    assert rapdu.status == 0x9000

    # Send message as last chunk
    backend.raise_policy = RaisePolicy.RAISE_NOTHING
    with backend.exchange_async(CLA, INS_SIGN_TX, p1=1, p2=0x00, data=message):
        if firmware.is_nano:
            navigator.navigate_until_text_and_compare(
                NavInsID.RIGHT_CLICK,
                [NavInsID.BOTH_CLICK],
                "Reject",
                screen_change_before_first_instruction=True,
            )
        else:
            navigator.navigate_until_text_and_compare(
                NavInsID.USE_CASE_REVIEW_TAP,
                [NavInsID.USE_CASE_REVIEW_REJECT,
                 NavInsID.USE_CASE_CHOICE_CONFIRM,
                 NavInsID.USE_CASE_STATUS_DISMISS],
                "Reject",
                screen_change_before_first_instruction=True,
            )

    rapdu = backend.last_async_response
    assert rapdu.status == SW_DENY
