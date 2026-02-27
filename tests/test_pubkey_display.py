"""Tests for GET_PUBLIC_KEY with display=True (address confirmation flow).

When P1=1, the app shows the derived address on-screen and waits for
user confirmation before returning the public key.
"""

from ragger.backend.interface import BackendInterface, RaisePolicy
from ragger.firmware import Firmware
from ragger.navigator import Navigator, NavInsID

from utils import CLA, INS_GET_PUBLIC_KEY, SW_OK, SW_DENY, bip32_path_to_bytes

BIP32_PATH = "m/44'/637'/1'/0'/0'"


def test_get_public_key_confirm(backend: BackendInterface, firmware: Firmware,
                                navigator: Navigator):
    """GET_PUBLIC_KEY with display=True, user approves."""
    cdata = bip32_path_to_bytes(BIP32_PATH)

    with backend.exchange_async(CLA, INS_GET_PUBLIC_KEY, p1=1, p2=0, data=cdata):
        if firmware.is_nano:
            # Confirm Address > Path > Address > Approve
            navigator.navigate_until_text_and_compare(
                NavInsID.RIGHT_CLICK,
                [NavInsID.BOTH_CLICK],
                "Approve",
                screen_change_before_first_instruction=True,
            )
        else:
            # Stax/Flex: address review confirmation
            navigator.navigate_until_text_and_compare(
                NavInsID.USE_CASE_REVIEW_TAP,
                [NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CONFIRM,
                 NavInsID.USE_CASE_STATUS_DISMISS],
                "Confirm",
                screen_change_before_first_instruction=True,
            )

    rapdu = backend.last_async_response
    assert rapdu.status == SW_OK

    # Parse and validate response structure
    offset = 0
    pub_key_len = rapdu.data[offset]
    offset += 1
    pub_key = rapdu.data[offset:offset + pub_key_len]
    offset += pub_key_len
    chain_code_len = rapdu.data[offset]
    offset += 1
    chain_code = rapdu.data[offset:offset + chain_code_len]
    offset += chain_code_len

    assert len(pub_key) == 33
    assert len(chain_code) == 32


def test_get_public_key_reject(backend: BackendInterface, firmware: Firmware,
                               navigator: Navigator):
    """GET_PUBLIC_KEY with display=True, user rejects."""
    cdata = bip32_path_to_bytes(BIP32_PATH)

    backend.raise_policy = RaisePolicy.RAISE_NOTHING
    with backend.exchange_async(CLA, INS_GET_PUBLIC_KEY, p1=1, p2=0, data=cdata):
        if firmware.is_nano:
            # Navigate to "Reject" instead of "Approve"
            navigator.navigate_until_text_and_compare(
                NavInsID.RIGHT_CLICK,
                [NavInsID.BOTH_CLICK],
                "Reject",
                screen_change_before_first_instruction=True,
            )
        else:
            # Stax/Flex: reject address
            navigator.navigate_until_text_and_compare(
                NavInsID.USE_CASE_REVIEW_TAP,
                [NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CANCEL,
                 NavInsID.USE_CASE_STATUS_DISMISS],
                "Cancel",
                screen_change_before_first_instruction=True,
            )

    rapdu = backend.last_async_response
    assert rapdu.status == SW_DENY


def test_get_public_key_confirm_returns_same_as_silent(backend: BackendInterface,
                                                       firmware: Firmware,
                                                       navigator: Navigator):
    """The key returned after display confirmation must match silent mode."""
    cdata = bip32_path_to_bytes(BIP32_PATH)

    # Get key silently (no display)
    rapdu_silent = backend.exchange(CLA, INS_GET_PUBLIC_KEY, p1=0, p2=0, data=cdata)
    assert rapdu_silent.status == SW_OK

    # Get key with display confirmation
    with backend.exchange_async(CLA, INS_GET_PUBLIC_KEY, p1=1, p2=0, data=cdata):
        if firmware.is_nano:
            navigator.navigate_until_text_and_compare(
                NavInsID.RIGHT_CLICK,
                [NavInsID.BOTH_CLICK],
                "Approve",
                screen_change_before_first_instruction=True,
            )
        else:
            navigator.navigate_until_text_and_compare(
                NavInsID.USE_CASE_REVIEW_TAP,
                [NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CONFIRM,
                 NavInsID.USE_CASE_STATUS_DISMISS],
                "Confirm",
                screen_change_before_first_instruction=True,
            )

    rapdu_display = backend.last_async_response
    assert rapdu_display.status == SW_OK

    # Both should return identical data
    assert rapdu_silent.data == rapdu_display.data
