"""Tests for bad state transitions, missing data, and transaction parsing failures."""

from ragger.backend.interface import BackendInterface

from utils import (
    CLA, INS_SIGN_TX, SW_BAD_STATE, SW_TX_PARSING_FAIL, SW_WRONG_TX_LENGTH,
    bip32_path_to_bytes,
)


def test_sign_tx_data_without_bip32(backend: BackendInterface):
    """Sending a data chunk (P1>0) without first sending BIP32 path (P1=0)
    should fail because G_context.req_type != CONFIRM_TRANSACTION."""
    raw_data = b"\x00" * 32
    rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=1, p2=0x00, data=raw_data)
    assert rapdu.status == SW_BAD_STATE


def test_sign_tx_malformed_transaction(backend: BackendInterface):
    """Sending garbage data as a transaction should produce SW_TX_PARSING_FAIL."""
    bip32_path = "m/44'/637'/1'/0'/0'"
    cdata = bip32_path_to_bytes(bip32_path)

    # Send BIP32 path (first APDU)
    rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=0, p2=0x80, data=cdata)
    assert rapdu.status == 0x9000

    # Send garbage data as last chunk
    garbage = bytes(range(64))
    rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=1, p2=0x00, data=garbage)
    assert rapdu.status == SW_TX_PARSING_FAIL


def test_sign_tx_truncated_raw_transaction(backend: BackendInterface):
    """Sending a valid prefix but truncated transaction data should fail parsing."""
    bip32_path = "m/44'/637'/1'/0'/0'"
    cdata = bip32_path_to_bytes(bip32_path)

    rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=0, p2=0x80, data=cdata)
    assert rapdu.status == 0x9000

    # Valid RAW_TX prefix but truncated (only prefix + partial sender)
    prefix = bytes.fromhex(
        "b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b193"
    )
    truncated = prefix + b"\x00" * 10
    rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=1, p2=0x00, data=truncated)
    assert rapdu.status == SW_TX_PARSING_FAIL


def test_sign_tx_data_exceeds_max_length(backend: BackendInterface):
    """Sending more data than MAX_TRANSACTION_LEN (510) should fail."""
    bip32_path = "m/44'/637'/1'/0'/0'"
    cdata = bip32_path_to_bytes(bip32_path)

    rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=0, p2=0x80, data=cdata)
    assert rapdu.status == 0x9000

    # Send 255 bytes in first data chunk (more to come)
    chunk1 = b"\x00" * 255
    rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=1, p2=0x80, data=chunk1)
    assert rapdu.status == 0x9000

    # Send 255 more bytes (total = 510, at the limit)
    chunk2 = b"\x00" * 255
    rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=2, p2=0x80, data=chunk2)
    assert rapdu.status == 0x9000

    # Send 1 more byte - should exceed MAX_TRANSACTION_LEN
    chunk3 = b"\x00"
    rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=3, p2=0x00, data=chunk3)
    assert rapdu.status == SW_WRONG_TX_LENGTH
