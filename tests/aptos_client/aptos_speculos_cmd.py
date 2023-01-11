import struct
from typing import Tuple

from speculos.client import SpeculosClient, ApduException

from aptos_client.aptos_cmd_builder import AptosCommandBuilder, InsType
from aptos_client.exception import DeviceException


class AptosSpeculosCommand:
    def __init__(self,
                 client: SpeculosClient,
                 debug: bool = False) -> None:
        self.client = client
        self.builder = AptosCommandBuilder(debug=debug)
        self.debug = debug

    def get_app_and_version(self) -> Tuple[str, str]:
        try:
            response = self.client._apdu_exchange(
                self.builder.get_app_and_version()
            )  # type: int, bytes
        except ApduException as error:
            raise DeviceException(error_code=error.sw, ins=0x01)

        # response = format_id (1) ||
        #            app_name_len (1) ||
        #            app_name (var) ||
        #            version_len (1) ||
        #            version (var) ||
        offset: int = 0

        format_id: int = response[offset]
        offset += 1
        app_name_len: int = response[offset]
        offset += 1
        app_name: str = response[offset:offset + app_name_len].decode("ascii")
        offset += app_name_len
        version_len: int = response[offset]
        offset += 1
        version: str = response[offset:offset + version_len].decode("ascii")
        offset += version_len

        return app_name, version

    def get_version(self) -> Tuple[int, int, int]:
        try:
            response = self.client._apdu_exchange(
                self.builder.get_version()
            )  # type: int, bytes
        except ApduException as error:
            raise DeviceException(error_code=error.sw,
                                  ins=InsType.INS_GET_VERSION)

        # response = MAJOR (1) || MINOR (1) || PATCH (1)
        assert len(response) == 3

        major, minor, patch = struct.unpack(
            "BBB",
            response
        )  # type: int, int, int

        return major, minor, patch

    def get_app_name(self) -> str:
        try:
            response = self.client._apdu_exchange(
                self.builder.get_app_name()
            )  # type: int, bytes
        except ApduException as error:
            raise DeviceException(error_code=error.sw,
                                  ins=InsType.INS_GET_APP_NAME)

        return response.decode("ascii")

    def get_public_key(self, bip32_path: str, display: bool = False) -> Tuple[bytes, bytes]:
        try:
            response = self.client._apdu_exchange(
                self.builder.get_public_key(bip32_path=bip32_path,
                                            display=display)
            )  # type: int, bytes
        except ApduException as error:
            raise DeviceException(error_code=error.sw,
                                  ins=InsType.INS_GET_PUBLIC_KEY)

        # response = pub_key_len (1) ||
        #            pub_key (var) ||
        #            chain_code_len (1) ||
        #            chain_code (var)
        offset: int = 0

        pub_key_len: int = response[offset]
        offset += 1
        pub_key: bytes = response[offset:offset + pub_key_len]
        offset += pub_key_len
        chain_code_len: int = response[offset]
        offset += 1
        chain_code: bytes = response[offset:offset + chain_code_len]
        offset += chain_code_len

        assert len(response) == 1 + pub_key_len + 1 + chain_code_len

        return pub_key, chain_code

    def sign_raw(self, bip32_path: str, data: bytes, model: str) -> Tuple[int, bytes]:
        response: bytes = b""

        for is_last, chunk in self.builder.sign_raw(bip32_path=bip32_path, data=data):
            if is_last:
                with self.client.apdu_exchange_nowait(cla=chunk[0], ins=chunk[1],
                                                      p1=chunk[2], p2=chunk[3],
                                                      data=chunk[5:]) as exchange:
                    # Review Transaction
                    self.client.press_and_release('right')
                    # Function
                    self.client.press_and_release('right')
                    # Coin Type
                    # Due to screen size, NanoS needs 1 more screens to display the coin type
                    if model == 'nanos':
                        self.client.press_and_release('right')
                    self.client.press_and_release('right')
                    # Receiver
                    # Due to screen size, NanoS needs 2 more screens to display the address
                    if model == 'nanos':
                        self.client.press_and_release('right')
                        self.client.press_and_release('right')
                    self.client.press_and_release('right')
                    self.client.press_and_release('right')
                    # Amount
                    self.client.press_and_release('right')
                    # Gas Fee
                    self.client.press_and_release('right')
                    # Approve
                    self.client.press_and_release('both')
                    response = exchange.receive()
            else:
                response = self.client._apdu_exchange(chunk)
                print(response)

        # response = der_sig_len (1) ||
        #            der_sig (var)
        der_sig_len: int = response[0]
        der_sig: bytes = response[1: 1 + der_sig_len]

        assert len(response) == 1 + der_sig_len

        return der_sig
