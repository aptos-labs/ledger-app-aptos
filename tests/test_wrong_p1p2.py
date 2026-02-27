"""Tests for invalid P1/P2 parameter handling across all APDU commands."""

from ragger.backend.interface import BackendInterface

from utils import (
    CLA, INS_GET_VERSION, INS_GET_APP_NAME, INS_GET_PUBLIC_KEY, INS_SIGN_TX,
    SW_WRONG_P1P2, bip32_path_to_bytes,
)


# --- GET_VERSION: requires P1=0, P2=0 ---

def test_get_version_wrong_p1(backend: BackendInterface):
    rapdu = backend.exchange(CLA, INS_GET_VERSION, p1=1, p2=0, data=b"")
    assert rapdu.status == SW_WRONG_P1P2


def test_get_version_wrong_p2(backend: BackendInterface):
    rapdu = backend.exchange(CLA, INS_GET_VERSION, p1=0, p2=1, data=b"")
    assert rapdu.status == SW_WRONG_P1P2


# --- GET_APP_NAME: requires P1=0, P2=0 ---

def test_get_app_name_wrong_p1(backend: BackendInterface):
    rapdu = backend.exchange(CLA, INS_GET_APP_NAME, p1=1, p2=0, data=b"")
    assert rapdu.status == SW_WRONG_P1P2


def test_get_app_name_wrong_p2(backend: BackendInterface):
    rapdu = backend.exchange(CLA, INS_GET_APP_NAME, p1=0, p2=1, data=b"")
    assert rapdu.status == SW_WRONG_P1P2


# --- GET_PUBLIC_KEY: requires P1 in {0,1}, P2=0 ---

def test_get_public_key_p1_too_high(backend: BackendInterface):
    cdata = bip32_path_to_bytes("m/44'/637'/1'/0'/0'")
    rapdu = backend.exchange(CLA, INS_GET_PUBLIC_KEY, p1=2, p2=0, data=cdata)
    assert rapdu.status == SW_WRONG_P1P2


def test_get_public_key_wrong_p2(backend: BackendInterface):
    cdata = bip32_path_to_bytes("m/44'/637'/1'/0'/0'")
    rapdu = backend.exchange(CLA, INS_GET_PUBLIC_KEY, p1=0, p2=1, data=cdata)
    assert rapdu.status == SW_WRONG_P1P2


# --- SIGN_TX: P1 in [0..3], P2 in {0x00, 0x80}, first chunk must have P2=0x80 ---

def test_sign_tx_p1_too_high(backend: BackendInterface):
    """P1 > P1_MAX (3) should be rejected."""
    cdata = bip32_path_to_bytes("m/44'/637'/1'/0'/0'")
    rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=4, p2=0x80, data=cdata)
    assert rapdu.status == SW_WRONG_P1P2


def test_sign_tx_invalid_p2(backend: BackendInterface):
    """P2 must be 0x00 (LAST) or 0x80 (MORE); anything else is invalid."""
    cdata = bip32_path_to_bytes("m/44'/637'/1'/0'/0'")
    rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=0, p2=0x01, data=cdata)
    assert rapdu.status == SW_WRONG_P1P2


def test_sign_tx_first_chunk_p2_last(backend: BackendInterface):
    """First APDU (P1=0) must have P2=MORE (0x80), not P2=LAST (0x00)."""
    cdata = bip32_path_to_bytes("m/44'/637'/1'/0'/0'")
    rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=0, p2=0x00, data=cdata)
    assert rapdu.status == SW_WRONG_P1P2
