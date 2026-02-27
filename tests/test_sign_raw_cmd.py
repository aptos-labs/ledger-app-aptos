from nacl.signing import VerifyKey

from ragger.backend.interface import BackendInterface
from ragger.firmware import Firmware
from ragger.navigator import Navigator, NavInsID

from utils import CLA, INS_GET_PUBLIC_KEY, INS_SIGN_TX, MAX_APDU_LEN, bip32_path_to_bytes, chunkify


def test_sign_raw_tx(backend: BackendInterface, firmware: Firmware, navigator: Navigator):
    message = bytes.fromhex(
        "b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b193"
        "783135e8b00430253a22ba041d860c373d7a1501ccf7ac2d1ad37a8ed2775aee"
        "000000000000000002000000000000000000000000000000000000000000000000"
        "00000000000000000104636f696e087472616e73666572010700000000000000000"
        "000000000000000000000000000000000000000000000000001"
        "0a6170746f735f636f696e094170746f73436f696e0002"
        "20094c6fc0d3b382a599c37e1aaa7618eff2c96a3586876082c4594c50c50d7dde"
        "082a00000000000000"
        "204e0000000000006400000000000000565c51630000000022"
    )
    bip32_path = "m/44'/637'/1'/0'/0'"

    # Get public key first
    cdata = bip32_path_to_bytes(bip32_path)
    rapdu = backend.exchange(CLA, INS_GET_PUBLIC_KEY, p1=0, p2=0, data=cdata)
    assert rapdu.status == 0x9000
    pub_key = rapdu.data[1:rapdu.data[0] + 1]
    pk = VerifyKey(pub_key[1:])

    # Send BIP32 path chunk (first APDU)
    bip_data = bip32_path_to_bytes(bip32_path)
    rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=0, p2=0x80, data=bip_data)
    assert rapdu.status == 0x9000

    # Send transaction data chunks
    for i, (is_last, chunk) in enumerate(chunkify(message, MAX_APDU_LEN)):
        if is_last:
            # Last chunk: expect review screens
            with backend.exchange_async(CLA, INS_SIGN_TX, p1=i + 1, p2=0x00, data=chunk):
                if firmware.is_nano:
                    # Review Transaction > Function > Coin Type > Receiver > Amount > Gas Fee > Approve
                    navigator.navigate_until_text_and_compare(
                        NavInsID.RIGHT_CLICK,
                        [NavInsID.BOTH_CLICK],
                        "Approve",
                        screen_change_before_first_instruction=True,
                    )
                else:
                    # Navigate through review screens on Stax/Flex
                    navigator.navigate_until_text_and_compare(
                        NavInsID.USE_CASE_REVIEW_TAP,
                        [NavInsID.USE_CASE_REVIEW_CONFIRM,
                         NavInsID.USE_CASE_STATUS_DISMISS],
                        "Hold to sign",
                        screen_change_before_first_instruction=True,
                    )

            rapdu = backend.last_async_response
            assert rapdu.status == 0x9000
        else:
            rapdu = backend.exchange(CLA, INS_SIGN_TX, p1=i + 1, p2=0x80, data=chunk)
            assert rapdu.status == 0x9000

    # Verify signature
    der_sig_len = rapdu.data[0]
    der_sig = rapdu.data[1:1 + der_sig_len]

    pk.verify(signature=der_sig, smessage=message)
