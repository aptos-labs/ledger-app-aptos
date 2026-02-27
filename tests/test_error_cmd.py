from ragger.backend.interface import BackendInterface

from utils import CLA, SW_CLA_NOT_SUPPORTED, SW_INS_NOT_SUPPORTED


def test_bad_cla(backend: BackendInterface):
    rapdu = backend.exchange(0xA0, 0x03, p1=0, p2=0, data=b"")
    assert rapdu.status == SW_CLA_NOT_SUPPORTED


def test_bad_ins(backend: BackendInterface):
    rapdu = backend.exchange(CLA, 0xFF, p1=0, p2=0, data=b"")
    assert rapdu.status == SW_INS_NOT_SUPPORTED
