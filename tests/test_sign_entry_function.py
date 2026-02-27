"""Tests for signing generic entry function transactions (FUNC_UNKNOWN).

When the entry function is not a known type (aptos_account::transfer or
coin::transfer), the app displays just the function name and gas fee without
parsing the arguments.
"""

import struct

from nacl.signing import VerifyKey

from ragger.backend.interface import BackendInterface
from ragger.firmware import Firmware
from ragger.navigator import Navigator, NavInsID

from utils import (
    CLA, INS_GET_PUBLIC_KEY, INS_SIGN_TX, SW_OK,
    bip32_path_to_bytes, chunkify, MAX_APDU_LEN,
)

BIP32_PATH = "m/44'/637'/1'/0'/0'"


def build_generic_entry_function(module_name: bytes, func_name: bytes,
                                 max_gas: int = 20000, gas_price: int = 100,
                                 chain_id: int = 34) -> bytes:
    """Construct a BCS-encoded transaction with an unknown entry function."""
    prefix = bytes.fromhex(
        "b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b193"
    )
    sender = bytes.fromhex(
        "783135e8b00430253a22ba041d860c373d7a1501ccf7ac2d1ad37a8ed2775aee"
    )
    sequence = struct.pack("<Q", 0)
    payload_variant = bytes([0x02])  # ENTRY_FUNCTION

    module_addr = bytes(31) + bytes([0x01])
    mod_name_enc = bytes([len(module_name)]) + module_name
    func_name_enc = bytes([len(func_name)]) + func_name

    # Empty type args and args (parser doesn't read these for unknown functions)
    ty_args_size = bytes([0])
    args_size = bytes([0])

    # Footer
    max_gas_bytes = struct.pack("<Q", max_gas)
    gas_price_bytes = struct.pack("<Q", gas_price)
    expiration = struct.pack("<Q", 1666432086)
    chain_id_byte = bytes([chain_id])

    return (prefix + sender + sequence +
            payload_variant + module_addr + mod_name_enc + func_name_enc +
            ty_args_size + args_size +
            max_gas_bytes + gas_price_bytes + expiration + chain_id_byte)


def test_sign_managed_coin_register(backend: BackendInterface, firmware: Firmware,
                                    navigator: Navigator):
    """Sign a 0x1::managed_coin::register transaction (FUNC_UNKNOWN)."""
    message = build_generic_entry_function(b"managed_coin", b"register")

    # Get public key
    cdata = bip32_path_to_bytes(BIP32_PATH)
    rapdu = backend.exchange(CLA, INS_GET_PUBLIC_KEY, p1=0, p2=0, data=cdata)
    assert rapdu.status == SW_OK
    pub_key = rapdu.data[1:rapdu.data[0] + 1]
    pk = VerifyKey(pub_key[1:])

    # Send BIP32 path
    bip_data = bip32_path_to_bytes(BIP32_PATH)
    rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=0, p2=0x80, data=bip_data)
    assert rapdu.status == SW_OK

    # Send transaction data (fits in one chunk)
    with backend.exchange_async(CLA, INS_SIGN_TX, p1=1, p2=0x00, data=message):
        if firmware.is_nano:
            # Review Transaction > Function > Gas Fee > Approve
            navigator.navigate_until_text_and_compare(
                NavInsID.RIGHT_CLICK,
                [NavInsID.BOTH_CLICK],
                "Approve",
                screen_change_before_first_instruction=True,
            )
        else:
            navigator.navigate_until_text_and_compare(
                NavInsID.USE_CASE_REVIEW_TAP,
                [NavInsID.USE_CASE_REVIEW_CONFIRM,
                 NavInsID.USE_CASE_STATUS_DISMISS],
                "Hold to sign",
                screen_change_before_first_instruction=True,
            )

    rapdu = backend.last_async_response
    assert rapdu.status == SW_OK

    sig_len = rapdu.data[0]
    sig = rapdu.data[1:1 + sig_len]
    pk.verify(signature=sig, smessage=message)
