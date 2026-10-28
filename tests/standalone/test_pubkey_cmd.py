import pytest
from application_client.aptos_pubkey import AptosPubkeyClient, unpack_public_key
from ragger.bip import CurveChoice, calculate_public_key_and_chaincode
from ragger.error import ExceptionRAPDU
from ragger.navigator import NavIns, NavInsID


def test_get_public_key_no_confirm(backend):
    client = AptosPubkeyClient(backend)
    for path in [
        "m/44'/637'/1'/0'/0'",
        "m/44'/637'/0'/0'/1'",
        "m/44'/637'/255'/255'/255'",
        "m/44'/637'/2147483647'/0'/0'/0'/0'/0'/0'/0'",
    ]:
        public_key, chain_code = unpack_public_key(client.get_public_key(path).data)
        reference_public_key, reference_chain_code = calculate_public_key_and_chaincode(
            CurveChoice.Ed25519Slip, path=path
        )
        assert public_key.hex() == reference_public_key[2:]
        assert chain_code.hex() == reference_chain_code


def test_get_public_key_rejects_nonhardened_slip10_path(backend):
    client = AptosPubkeyClient(backend)
    # Ed25519 SLIP-10 cannot derive a non-hardened child. Reject it before the
    # secure SDK syscall, which otherwise exits the app without a status word.
    for path in ("m/44'/637'/0", "m/44'/637'/1'/0/0"):
        with pytest.raises(ExceptionRAPDU) as error:
            client.get_public_key(path)
        assert error.value.status == 0xB006
        assert error.value.data == b""


@pytest.mark.parametrize(
    "payload",
    [b"", b"\x03", b"\x03" + b"\x80\x00\x00\x2c" * 2, b"\x03" + b"\x80\x00\x00\x2c" * 4],
)
def test_get_public_key_malformed_path(backend, payload):
    with pytest.raises(ExceptionRAPDU) as error:
        backend.exchange(cla=0x5B, ins=0x05, p1=0, p2=0, data=payload)
    assert error.value.status == 0x6A87
    assert error.value.data == b""


@pytest.mark.parametrize(
    "payload",
    [
        b"\x00",
        b"\x02\x80\x00\x00\x2c\x80\x00\x02\x7d",
        b"\x03\x80\x00\x00\x2c\x80\x00\x00\x01\x80\x00\x00\x00",
        b"\x03\x80\x00\x00\x2d\x80\x00\x02\x7d\x80\x00\x00\x00",
        b"\x0b" + b"\x80\x00\x00\x2c" + b"\x80\x00\x02\x7d" + b"\x80\x00\x00\x00" * 9,
    ],
)
def test_get_public_key_unsupported_path(backend, payload):
    with pytest.raises(ExceptionRAPDU) as error:
        backend.exchange(cla=0x5B, ins=0x05, p1=0, p2=0, data=payload)
    assert error.value.status == 0xB006
    assert error.value.data == b""


def test_get_public_key_confirm_accepted(
    firmware, backend, navigator, test_name, default_screenshot_path
):
    client = AptosPubkeyClient(backend)
    path = "m/44'/637'/1'/0'/0'"
    with client.get_public_key_with_confirmation(path):
        if firmware.device.startswith("nano"):
            navigator.navigate_until_text_and_compare(
                NavInsID.RIGHT_CLICK,
                [NavInsID.BOTH_CLICK],
                "Confirm",
                default_screenshot_path,
                test_name,
            )
        else:
            qr_coordinates = {"flex": (80, 430), "stax": (70, 530), "apex_p": (43, 299)}[
                firmware.device
            ]
            navigator.navigate_and_compare(
                default_screenshot_path,
                test_name,
                [
                    NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
                    NavIns(NavInsID.TOUCH, qr_coordinates),
                    NavInsID.USE_CASE_ADDRESS_CONFIRMATION_EXIT_QR,
                    NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CONFIRM,
                    NavInsID.USE_CASE_STATUS_DISMISS,
                ],
            )

    response = client.get_async_response()
    assert response is not None
    public_key, chain_code = unpack_public_key(response.data)
    reference_public_key, reference_chain_code = calculate_public_key_and_chaincode(
        CurveChoice.Ed25519Slip, path=path
    )
    assert public_key.hex() == reference_public_key[2:]
    assert chain_code.hex() == reference_chain_code


def test_get_public_key_confirm_refused(
    firmware, backend, navigator, test_name, default_screenshot_path
):
    client = AptosPubkeyClient(backend)
    path = "m/44'/637'/1'/0'/0'"

    if firmware.device.startswith("nano"):
        with pytest.raises(ExceptionRAPDU) as error:
            with client.get_public_key_with_confirmation(path):
                navigator.navigate_until_text_and_compare(
                    NavInsID.RIGHT_CLICK,
                    [NavInsID.BOTH_CLICK],
                    "Cancel",
                    default_screenshot_path,
                    test_name,
                )
        assert error.value.status == 0x6985
        assert error.value.data == b""
    else:
        for i, instructions in enumerate(
            [
                [NavInsID.USE_CASE_REVIEW_REJECT, NavInsID.USE_CASE_STATUS_DISMISS],
                [
                    NavInsID.USE_CASE_REVIEW_NEXT,
                    NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CANCEL,
                    NavInsID.USE_CASE_STATUS_DISMISS,
                ],
            ]
        ):
            with pytest.raises(ExceptionRAPDU) as error:
                with client.get_public_key_with_confirmation(path):
                    navigator.navigate_and_compare(
                        default_screenshot_path, f"{test_name}/part{i}", instructions
                    )
            assert error.value.status == 0x6985
            assert error.value.data == b""
            navigator.navigate([NavInsID.WAIT_FOR_HOME_SCREEN])
