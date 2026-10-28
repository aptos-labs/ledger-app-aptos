"""Aptos SIGN_TX transport and blind-signing functional tests."""

import hashlib

import pytest
from application_client.aptos_command_sender import AptosCommandSender
from application_client.aptos_pubkey import AptosPubkeyClient, unpack_public_key
from application_client.boilerplate_command_sender import Errors
from nacl.exceptions import BadSignatureError
from nacl.signing import VerifyKey
from ragger.error import ExceptionRAPDU
from ragger.navigator import NavIns, NavInsID

PATH = "m/44'/637'/1'/0'/0'"
RAW_TRANSACTION_PREFIX = hashlib.sha3_256(b"APTOS::RawTransaction").digest()
RAW_TRANSACTION_WITH_DATA_PREFIX = hashlib.sha3_256(b"APTOS::RawTransactionWithData").digest()

# Realistic Aptos RawTransaction fixture. The transport signs these exact bytes.
RAW_TRANSACTION_BODY = bytes.fromhex(
    "094c6fc0d3b382a599c37e1aaa7618eff2c96a3586876082c4594c50c50d7dde1b"
    "0000000000000002190d44266241744264b964a37b8f09863167a12d3e70cda39376cfb4e3561e"
    "120a736372697074735f7632047377617003070000000000000000000000000000000000000000000000000000000000000001"
    "0a6170746f735f636f696e094170746f73436f696e000743417434fd869edee76cca2a4d2301e528a1551b1d719b75c350c3c97d15b8b905636f696e7304555344540007190d44266241744264b964a37b8f09863167a12d3e70cda39376cfb4e3561e12066375727665730c556e636f7272656c6174656400020800e1f5050000000008decbb30000000000480000000000000064000000000000008a9ba4640000000002"
)
TRANSACTION = RAW_TRANSACTION_PREFIX + RAW_TRANSACTION_BODY
# A RawTransactionWithData-shaped fixture: the recognized prefix, one variant
# byte, and the RawTransaction fixture body.
TRANSACTION_WITH_DATA = RAW_TRANSACTION_WITH_DATA_PREFIX + b"\x00" + RAW_TRANSACTION_BODY

SHORT_MESSAGE = b"Hello Ledger!"
SHORT_RAW_MESSAGE = bytes.fromhex("01020304ff")
LONG_RAW_MESSAGE = bytes(range(256)) * 3
MALFORMED_TRANSACTION = RAW_TRANSACTION_PREFIX + b"\x00"
MALFORMED_TRANSACTION_WITH_DATA = RAW_TRANSACTION_WITH_DATA_PREFIX + b"\x00"


def _review_blind(firmware, navigator, test_name, default_screenshot_path):
    if firmware.device.startswith("nano"):
        navigator.navigate_and_compare(
            default_screenshot_path,
            f"{test_name}/part0",
            [
                NavInsID.RIGHT_CLICK,
                NavInsID.BOTH_CLICK,
                NavInsID.RIGHT_CLICK,
                NavInsID.BOTH_CLICK,
            ],
        )
        navigator.navigate_until_text_and_compare(
            NavInsID.RIGHT_CLICK,
            [NavInsID.BOTH_CLICK, NavInsID.BOTH_CLICK],
            "Sign",
            default_screenshot_path,
            f"{test_name}/part0",
            screen_change_before_first_instruction=False,
            snap_start_idx=4,
        )
    else:
        navigator.navigate_and_compare(
            default_screenshot_path,
            f"{test_name}/part0",
            [
                NavInsID.USE_CASE_CHOICE_CONFIRM,
                NavInsID.USE_CASE_STATUS_DISMISS,
                NavInsID.USE_CASE_CHOICE_REJECT,
                NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
                NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
                NavInsID.USE_CASE_REVIEW_CONFIRM,
                NavInsID.USE_CASE_STATUS_DISMISS,
            ],
        )


@pytest.fixture
def disable_blind_signing(navigator, firmware):
    yield
    if firmware.device.startswith("nano"):
        navigator.navigate(
            [
                NavInsID.RIGHT_CLICK,
                NavInsID.BOTH_CLICK,
                NavInsID.BOTH_CLICK,
                NavInsID.RIGHT_CLICK,
                NavInsID.BOTH_CLICK,
                NavInsID.LEFT_CLICK,
            ],
            screen_change_before_first_instruction=False,
        )
    else:
        navigator.navigate(
            [
                NavInsID.USE_CASE_HOME_SETTINGS,
                NavIns(NavInsID.TOUCH, (200, 113)),
                NavInsID.USE_CASE_SETTINGS_MULTI_PAGE_EXIT,
            ],
            screen_change_before_first_instruction=False,
        )


def _sign_and_verify(
    backend,
    firmware,
    navigator,
    test_name,
    default_screenshot_path,
    payload,
):
    public_key, _ = unpack_public_key(AptosPubkeyClient(backend).get_public_key(PATH).data)
    client = AptosCommandSender(backend)
    with client.sign_tx(PATH, payload):
        _review_blind(firmware, navigator, test_name, default_screenshot_path)
    response = client.get_async_response()
    assert response is not None
    assert len(response.data) == 65
    assert response.data[0] == 64
    try:
        VerifyKey(public_key).verify(payload, response.data[1:])
    except BadSignatureError:
        pytest.fail("Device signature does not verify against the exact submitted bytes")


def test_sign_tx_short_msg(backend, firmware, navigator, test_name, default_screenshot_path, disable_blind_signing):
    _sign_and_verify(backend, firmware, navigator, test_name, default_screenshot_path, SHORT_MESSAGE)


def test_sign_short_raw_msg(backend, firmware, navigator, test_name, default_screenshot_path, disable_blind_signing):
    _sign_and_verify(backend, firmware, navigator, test_name, default_screenshot_path, SHORT_RAW_MESSAGE)


def test_sign_long_raw_msg(backend, firmware, navigator, test_name, default_screenshot_path, disable_blind_signing):
    assert len(LONG_RAW_MESSAGE) > 255
    _sign_and_verify(backend, firmware, navigator, test_name, default_screenshot_path, LONG_RAW_MESSAGE)


def test_blind_sign_tx_long_tx(backend, firmware, navigator, test_name, default_screenshot_path, disable_blind_signing):
    assert len(TRANSACTION) > 255
    _sign_and_verify(backend, firmware, navigator, test_name, default_screenshot_path, TRANSACTION)


def test_blind_sign_tx_with_data(backend, firmware, navigator, test_name, default_screenshot_path, disable_blind_signing):
    assert TRANSACTION_WITH_DATA.startswith(RAW_TRANSACTION_WITH_DATA_PREFIX)
    _sign_and_verify(backend, firmware, navigator, test_name, default_screenshot_path, TRANSACTION_WITH_DATA)


def test_sign_tx_refused(backend, firmware, navigator, test_name, default_screenshot_path):
    client = AptosCommandSender(backend)
    if firmware.device.startswith("nano"):
        with pytest.raises(ExceptionRAPDU) as error:
            with client.sign_tx(PATH, SHORT_MESSAGE):
                navigator.navigate_until_text_and_compare(
                    NavInsID.RIGHT_CLICK,
                    [NavInsID.BOTH_CLICK],
                    "Reject",
                    default_screenshot_path,
                    test_name,
                )
        assert error.value.status == Errors.SW_DENY
        assert error.value.data == b""
    else:
        with pytest.raises(ExceptionRAPDU) as error:
            with client.sign_tx(PATH, SHORT_MESSAGE):
                navigator.navigate_and_compare(
                    default_screenshot_path,
                    test_name,
                    [
                        NavInsID.USE_CASE_REVIEW_REJECT,
                        NavInsID.USE_CASE_CHOICE_CONFIRM,
                        NavInsID.USE_CASE_STATUS_DISMISS,
                    ],
                )
        assert error.value.status == Errors.SW_DENY
        assert error.value.data == b""


def test_sign_tx_chunk_without_start(backend):
    client = AptosCommandSender(backend)
    with pytest.raises(ExceptionRAPDU) as error:
        client.send_chunk(1, True, SHORT_MESSAGE)
    assert error.value.status == Errors.SW_BAD_STATE
    assert error.value.data == b""


def test_get_public_key_resets_pending_sign_tx(backend):
    client = AptosCommandSender(backend)
    assert client.start_signing(PATH).status == Errors.SW_OK
    assert client.send_chunk(1, True, SHORT_MESSAGE).status == Errors.SW_OK
    assert AptosPubkeyClient(backend).get_public_key(PATH).status == Errors.SW_OK
    with pytest.raises(ExceptionRAPDU) as error:
        client.send_chunk(2, True, b"after reset")
    assert error.value.status == Errors.SW_BAD_STATE
    assert error.value.data == b""


def test_sign_tx_out_of_order_chunk_can_be_resent(
    backend, firmware, navigator, test_name, default_screenshot_path, disable_blind_signing
):
    client = AptosCommandSender(backend)
    public_key, _ = unpack_public_key(AptosPubkeyClient(backend).get_public_key(PATH).data)
    client.start_signing(PATH)
    with pytest.raises(ExceptionRAPDU) as error:
        client.send_chunk(2, True, TRANSACTION[:100])
    assert error.value.status == Errors.SW_WRONG_P1P2
    assert error.value.data == b""
    client.send_chunk(1, True, TRANSACTION[:100])
    with client.finish_chunk(2, TRANSACTION[100:]):
        _review_blind(firmware, navigator, test_name, default_screenshot_path)
    response = client.get_async_response()
    assert response is not None
    assert response.data[0] == 64
    assert len(response.data) == 65
    VerifyKey(public_key).verify(TRANSACTION, response.data[1:])


def test_sign_tx_max_device_payload_and_packet_limit(
    backend, firmware, navigator, test_name, default_screenshot_path, disable_blind_signing
):
    max_chunks = {
        "nanosp": 106,
        "nanox": 90,
        "stax": 70,
        "flex": 70,
        "apex_p": 70,
    }[firmware.device]
    client = AptosCommandSender(backend)
    public_key, _ = unpack_public_key(AptosPubkeyClient(backend).get_public_key(PATH).data)
    message = b"\xff" * (max_chunks * 255)
    client.start_signing(PATH)
    for index in range(1, max_chunks):
        client.send_chunk(index, True, message[(index - 1) * 255 : index * 255])
    with pytest.raises(ExceptionRAPDU) as error:
        client.send_chunk(max_chunks + 1, True, b"x")
    assert error.value.status == Errors.SW_WRONG_P1P2
    assert error.value.data == b""
    with client.finish_chunk(max_chunks, message[(max_chunks - 1) * 255 :]):
        _review_blind(firmware, navigator, test_name, default_screenshot_path)
    response = client.get_async_response()
    assert response is not None
    assert response.data[0] == 64
    assert len(response.data) == 65
    VerifyKey(public_key).verify(message, response.data[1:])


@pytest.mark.parametrize(
    "payload",
    [MALFORMED_TRANSACTION, MALFORMED_TRANSACTION_WITH_DATA],
)
def test_sign_tx_malformed_transaction_returns_parsing_error(backend, payload):
    client = AptosCommandSender(backend)
    client.start_signing(PATH)
    with pytest.raises(ExceptionRAPDU) as error:
        client.send_chunk(1, False, payload)
    assert error.value.status == Errors.SW_TX_PARSING_FAIL
    assert error.value.data == b""
    with pytest.raises(ExceptionRAPDU) as error:
        client.send_chunk(1, True, b"retry")
    assert error.value.status == Errors.SW_BAD_STATE
    assert error.value.data == b""


def test_sign_tx_disallowed_path(backend):
    client = AptosCommandSender(backend)
    with pytest.raises(ExceptionRAPDU) as error:
        client.start_signing("m/44'/1'/0'/0'")
    assert error.value.status == Errors.SW_GET_PUB_KEY_FAIL
    assert error.value.data == b""
