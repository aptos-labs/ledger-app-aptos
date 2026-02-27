import struct

from ragger.backend.interface import BackendInterface

from utils import CLA, INS_GET_VERSION


def test_get_version(backend: BackendInterface):
    rapdu = backend.exchange(CLA, INS_GET_VERSION, p1=0, p2=0, data=b"")
    assert rapdu.status == 0x9000
    assert len(rapdu.data) == 3

    major, minor, patch = struct.unpack("BBB", rapdu.data)
    assert (major, minor, patch) == (0, 1, 0)
