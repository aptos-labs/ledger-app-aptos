from ragger.backend.interface import BackendInterface

from utils import CLA, INS_GET_APP_NAME


def test_get_app_name(backend: BackendInterface):
    rapdu = backend.exchange(CLA, INS_GET_APP_NAME, p1=0, p2=0, data=b"")
    assert rapdu.status == 0x9000
    assert rapdu.data == b"Aptos"
