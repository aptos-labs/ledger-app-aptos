from ragger.backend.interface import BackendInterface

from utils import CLA, INS_GET_PUBLIC_KEY, bip32_path_to_bytes


def test_get_public_key(backend: BackendInterface):
    cdata = bip32_path_to_bytes("m/44'/637'/1'/0'/0'")

    rapdu = backend.exchange(CLA, INS_GET_PUBLIC_KEY, p1=0, p2=0, data=cdata)
    assert rapdu.status == 0x9000

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
