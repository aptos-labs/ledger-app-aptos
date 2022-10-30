from nacl.signing import VerifyKey
from nacl.exceptions import BadSignatureError

from aptos_client.transaction import Transaction
from aptos_client.exception import *


def test_sign_raw_tx(cmd, button, model):
    message = bytes.fromhex("b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b193783135e8b00430253a22ba041d860c373d7a1501ccf7ac2d1ad37a8ed2775aee000000000000000002000000000000000000000000000000000000000000000000000000000000000104636f696e087472616e73666572010700000000000000000000000000000000000000000000000000000000000000010a6170746f735f636f696e094170746f73436f696e000220094c6fc0d3b382a599c37e1aaa7618eff2c96a3586876082c4594c50c50d7dde082a00000000000000204e0000000000006400000000000000565c51630000000022")
    bip32_path: str = "m/44'/637'/1'/0'/0'"

    pub_key, chain_code = cmd.get_public_key(
        bip32_path=bip32_path,
        display=False
    )  # type: bytes, bytes

    pk = VerifyKey(pub_key[1:])

    der_sig = cmd.sign_raw(bip32_path=bip32_path,
                           data=message,
                           button=button,
                           model=model)

    try:
        pk.verify(signature=der_sig, smessage=message)
    except BadSignatureError as exc:
        assert False, exc
