"""Tests for signing aptos_account::transfer transactions (FUNC_APTOS_ACCOUNT_TRANSFER).

This exercises a different code path than coin::transfer (tested in test_sign_raw_cmd.py).
The aptos_account::transfer function uses module 0x1::aptos_account with no type args,
while coin::transfer uses 0x1::coin with a type tag struct for the coin type.
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


def build_aptos_account_transfer(receiver: bytes, amount: int,
                                 max_gas: int = 20000, gas_price: int = 100,
                                 chain_id: int = 34) -> bytes:
    """Construct a BCS-encoded 0x1::aptos_account::transfer transaction."""
    # SHA3-256("APTOS::RawTransaction") prefix
    prefix = bytes.fromhex(
        "b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b193"
    )
    sender = bytes.fromhex(
        "783135e8b00430253a22ba041d860c373d7a1501ccf7ac2d1ad37a8ed2775aee"
    )
    sequence = struct.pack("<Q", 0)

    # Entry function payload (variant = 2)
    payload_variant = bytes([0x02])

    # Module ID: 0x1::aptos_account
    module_addr = bytes(31) + bytes([0x01])
    module_name = bytes([13]) + b"aptos_account"

    # Function: transfer
    func_name = bytes([8]) + b"transfer"

    # Type args: 0 (aptos_account::transfer takes no type args)
    ty_args_size = bytes([0])

    # Args: 2 (receiver address + amount)
    args_size = bytes([2])

    # Arg 1: receiver address (length-prefixed)
    receiver_len = bytes([32])

    # Arg 2: amount (length-prefixed u64 LE)
    amount_len = bytes([8])
    amount_bytes = struct.pack("<Q", amount)

    # Transaction footer
    max_gas_bytes = struct.pack("<Q", max_gas)
    gas_price_bytes = struct.pack("<Q", gas_price)
    expiration = struct.pack("<Q", 1666432086)
    chain_id_byte = bytes([chain_id])

    return (prefix + sender + sequence +
            payload_variant + module_addr + module_name + func_name +
            ty_args_size + args_size +
            receiver_len + receiver + amount_len + amount_bytes +
            max_gas_bytes + gas_price_bytes + expiration + chain_id_byte)


def test_sign_aptos_account_transfer(backend: BackendInterface, firmware: Firmware,
                                     navigator: Navigator):
    """Sign an aptos_account::transfer and verify the Ed25519 signature."""
    receiver = bytes.fromhex(
        "094c6fc0d3b382a599c37e1aaa7618eff2c96a3586876082c4594c50c50d7dde"
    )
    message = build_aptos_account_transfer(receiver=receiver, amount=42)

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

    # Send transaction data
    for i, (is_last, chunk) in enumerate(chunkify(message, MAX_APDU_LEN)):
        if is_last:
            with backend.exchange_async(CLA, INS_SIGN_TX, p1=i + 1, p2=0x00, data=chunk):
                if firmware.is_nano:
                    # Review Transaction > Function > Receiver > Amount > Gas Fee > Approve
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
        else:
            rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=i + 1, p2=0x80, data=chunk)
            assert rapdu.status == SW_OK

    # Verify Ed25519 signature
    sig_len = rapdu.data[0]
    sig = rapdu.data[1:1 + sig_len]
    pk.verify(signature=sig, smessage=message)
