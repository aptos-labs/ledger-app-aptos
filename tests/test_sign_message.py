"""Tests for message signing flow (TX_MESSAGE variant).

The transaction deserializer identifies a payload as TX_MESSAGE when the data
is all ASCII (bytes <= 0x7F) and doesn't start with either the RawTransaction
or RawTransactionWithData SHA3-256 prefix hash.
"""

from nacl.signing import VerifyKey

from ragger.backend.interface import BackendInterface
from ragger.firmware import Firmware
from ragger.navigator import Navigator, NavInsID

from utils import (
    CLA, INS_GET_PUBLIC_KEY, INS_SIGN_TX, SW_OK,
    bip32_path_to_bytes, chunkify, MAX_APDU_LEN,
)

BIP32_PATH = "m/44'/637'/1'/0'/0'"


def test_sign_short_message(backend: BackendInterface, firmware: Firmware, navigator: Navigator):
    """Sign a short ASCII message and verify the Ed25519 signature."""
    message = b"Hello, Aptos!"

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

    # Send message as last chunk (short message fits in one APDU)
    with backend.exchange_async(CLA, INS_SIGN_TX, p1=1, p2=0x00, data=message):
        if firmware.is_nano:
            # Review Message > Message text > Approve
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

    # Verify Ed25519 signature over the raw message bytes
    sig_len = rapdu.data[0]
    sig = rapdu.data[1:1 + sig_len]
    pk.verify(signature=sig, smessage=message)


def test_sign_longer_message(backend: BackendInterface, firmware: Firmware, navigator: Navigator):
    """Sign a longer ASCII message that exercises the full message display."""
    message = b"This is a longer message for testing the Aptos Ledger app message signing flow."

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

    # Send message as last chunk
    with backend.exchange_async(CLA, INS_SIGN_TX, p1=1, p2=0x00, data=message):
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
