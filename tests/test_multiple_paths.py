"""Tests for key derivation with multiple BIP32 paths.

Verifies that different BIP32 paths produce different keys, and that
the same path always produces the same key (deterministic derivation).
"""

from ragger.backend.interface import BackendInterface

from utils import CLA, INS_GET_PUBLIC_KEY, SW_OK, bip32_path_to_bytes


def _get_pubkey(backend: BackendInterface, path: str) -> bytes:
    """Helper to extract raw public key bytes for a BIP32 path."""
    cdata = bip32_path_to_bytes(path)
    rapdu = backend.exchange(CLA, INS_GET_PUBLIC_KEY, p1=0, p2=0, data=cdata)
    assert rapdu.status == SW_OK
    pub_key_len = rapdu.data[0]
    return rapdu.data[1:1 + pub_key_len]


def test_same_path_returns_same_key(backend: BackendInterface):
    """Calling GET_PUBLIC_KEY twice with the same path returns the same key."""
    path = "m/44'/637'/0'/0'/0'"
    key1 = _get_pubkey(backend, path)
    key2 = _get_pubkey(backend, path)
    assert key1 == key2


def test_different_account_index(backend: BackendInterface):
    """Different account indices produce different keys."""
    key0 = _get_pubkey(backend, "m/44'/637'/0'/0'/0'")
    key1 = _get_pubkey(backend, "m/44'/637'/1'/0'/0'")
    key2 = _get_pubkey(backend, "m/44'/637'/2'/0'/0'")
    assert key0 != key1
    assert key1 != key2
    assert key0 != key2


def test_different_address_index(backend: BackendInterface):
    """Different address indices within the same account produce different keys."""
    key0 = _get_pubkey(backend, "m/44'/637'/0'/0'/0'")
    key1 = _get_pubkey(backend, "m/44'/637'/0'/0'/1'")
    assert key0 != key1


def test_pubkey_length_is_33(backend: BackendInterface):
    """Public key should be 33 bytes (0x04 prefix + 32 raw bytes)."""
    for path in ["m/44'/637'/0'/0'/0'", "m/44'/637'/1'/0'/0'", "m/44'/637'/99'/0'/0'"]:
        key = _get_pubkey(backend, path)
        assert len(key) == 33, f"Expected 33-byte pubkey for {path}, got {len(key)}"
        assert key[0] == 0x04, f"Expected 0x04 prefix for {path}, got {hex(key[0])}"
