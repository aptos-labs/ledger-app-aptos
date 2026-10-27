from collections.abc import Generator
from contextlib import contextmanager

from ragger.backend.interface import BackendInterface, RAPDU
from ragger.bip import pack_derivation_path


class AptosPubkeyClient:
    def __init__(self, backend: BackendInterface) -> None:
        self.backend = backend

    def get_public_key(self, path: str) -> RAPDU:
        return self.backend.exchange(cla=0x5B, ins=0x05, p1=0, p2=0, data=pack_derivation_path(path))

    @contextmanager
    def get_public_key_with_confirmation(self, path: str) -> Generator[None, None, None]:
        with self.backend.exchange_async(cla=0x5B, ins=0x05, p1=1, p2=0, data=pack_derivation_path(path)):
            yield

    def get_async_response(self) -> RAPDU | None:
        return self.backend.last_async_response


def unpack_public_key(response: bytes) -> tuple[bytes, bytes]:
    assert len(response) == 67
    assert response[:2] == b"\x21\x04"
    assert response[34] == 0x20
    return response[2:34], response[35:67]
