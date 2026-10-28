"""APDU client for Aptos SIGN_TX transport tests."""

from collections.abc import Generator
from contextlib import contextmanager

from ragger.backend.interface import RAPDU, BackendInterface
from ragger.bip import pack_derivation_path

from application_client.boilerplate_command_sender import (
    CLA,
    MAX_APDU_LEN,
    P1,
    P2,
    InsType,
)


class AptosCommandSender:
    def __init__(self, backend: BackendInterface) -> None:
        self.backend = backend

    def start_signing(self, path: str) -> RAPDU:
        return self.send_chunk(P1.P1_START, True, pack_derivation_path(path))

    def send_chunk(self, index: int, more: bool, data: bytes) -> RAPDU:
        return self.backend.exchange(
            cla=CLA,
            ins=InsType.SIGN_TX,
            p1=index,
            p2=P2.P2_MORE if more else P2.P2_LAST,
            data=data,
        )

    @contextmanager
    def finish_chunk(self, index: int, data: bytes) -> Generator[None, None, None]:
        with self.backend.exchange_async(
            cla=CLA,
            ins=InsType.SIGN_TX,
            p1=index,
            p2=P2.P2_LAST,
            data=data,
        ):
            yield

    @contextmanager
    def sign_tx(self, path: str, payload: bytes) -> Generator[None, None, None]:
        self.start_signing(path)
        chunks = [payload[i : i + MAX_APDU_LEN] for i in range(0, len(payload), MAX_APDU_LEN)]
        if not chunks:
            chunks = [b""]
        for index, chunk in enumerate(chunks[:-1], start=1):
            self.send_chunk(index, True, chunk)
        with self.finish_chunk(len(chunks), chunks[-1]):
            yield

    def get_async_response(self) -> RAPDU | None:
        return self.backend.last_async_response
