import struct
from typing import Tuple

from ledgercomm import Transport

from aptos_client.aptos_cmd_builder import AptosCommandBuilder, InsType
from aptos_client.button import Button
from aptos_client.exception import DeviceException


class AptosCommand:
    def __init__(self,
                 transport: Transport,
                 debug: bool = False) -> None:
        self.transport = transport
        self.builder = AptosCommandBuilder(debug=debug)
        self.debug = debug

    def get_app_and_version(self) -> Tuple[str, str]:
        sw, response = self.transport.exchange_raw(
            self.builder.get_app_and_version()
        )  # type: int, bytes

        if sw != 0x9000:
            raise DeviceException(error_code=sw, ins=0x01)

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
        sw, response = self.transport.exchange_raw(
            self.builder.get_version()
        )  # type: int, bytes

        if sw != 0x9000:
            raise DeviceException(error_code=sw, ins=InsType.INS_GET_VERSION)

        # response = MAJOR (1) || MINOR (1) || PATCH (1)
        assert len(response) == 3

        major, minor, patch = struct.unpack(
            "BBB",
            response
        )  # type: int, int, int

        return major, minor, patch

    def get_app_name(self) -> str:
        sw, response = self.transport.exchange_raw(
            self.builder.get_app_name()
        )  # type: int, bytes

        if sw != 0x9000:
            raise DeviceException(error_code=sw, ins=InsType.INS_GET_APP_NAME)

        return response.decode("ascii")

    def get_public_key(self, bip32_path: str, display: bool = False) -> Tuple[bytes, bytes]:
        sw, response = self.transport.exchange_raw(
            self.builder.get_public_key(bip32_path=bip32_path,
                                        display=display)
        )  # type: int, bytes

        if sw != 0x9000:
            raise DeviceException(error_code=sw, ins=InsType.INS_GET_PUBLIC_KEY)

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

    def sign_raw(self, bip32_path: str, data: bytes, button: Button, model: str) -> Tuple[int, bytes]:
        sw: int
        response: bytes = b""

        for is_last, chunk in self.builder.sign_raw(bip32_path=bip32_path, data=data):
            self.transport.send_raw(chunk)

            if is_last:
                # Review Transaction
                button.right_click()
                # Function
                button.right_click()
                # Coin Type
                # Due to screen size, NanoS needs 1 more screens to display the coin type
                if model == 'nanos':
                    button.right_click()
                button.right_click()
                # Receiver
                # Due to screen size, NanoS needs 2 more screens to display the address
                if model == 'nanos':
                    button.right_click()
                    button.right_click()
                button.right_click()
                button.right_click()
                # Amount
                button.right_click()
                # Gas Fee
                button.right_click()
                # Approve
                button.both_click()

            sw, response = self.transport.recv()  # type: int, bytes

            if sw != 0x9000:
                raise DeviceException(error_code=sw, ins=InsType.INS_SIGN_TX)

        # response = der_sig_len (1) ||
        #            der_sig (var)
        der_sig_len: int = response[0]
        der_sig: bytes = response[1: 1 + der_sig_len]

        assert len(response) == 1 + der_sig_len

        return der_sig
