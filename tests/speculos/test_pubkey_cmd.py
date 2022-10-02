def test_get_public_key(cmd):
    pub_key, chain_code = cmd.get_public_key(
        bip32_path="m/44'/637'/1'/0'/0'",
        display=False
    )  # type: bytes, bytes

    assert len(pub_key) == 33
    assert len(chain_code) == 32
