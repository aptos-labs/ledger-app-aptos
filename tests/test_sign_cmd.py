import pytest

from application_client.aptos_command_sender import AptosCommandSender, Errors
from application_client.aptos_response_unpacker import unpack_get_public_key_response, unpack_sign_tx_response
from ragger.error import ExceptionRAPDU
from ragger.navigator import NavInsID
from utils import ROOT_SCREENSHOT_PATH, check_signature_validity

# In this tests we check the behavior of the device when asked to sign a transaction


# In this test we send to the device a transaction to sign and validate it on screen
# The transaction is short and will be sent in one chunk
# We will ensure that the displayed information is correct by using screenshots comparison
def test_sign_tx_short_tx(firmware, backend, navigator, test_name):
    # Use the app interface instead of raw interface
    client = AptosCommandSender(backend)
    # The path used for this entire test
    path: str = "m/44'/637'/1'/0'/0'"

    # First we need to get the public key of the device in order to build the transaction
    rapdu = client.get_public_key(path=path)
    _, public_key, _, _ = unpack_get_public_key_response(rapdu.data)

    # Create the transaction that will be sent to the device for signing
    transaction = bytes.fromhex("b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b193783135e8b00430253a22ba041d860c373d7a1501ccf7ac2d1ad37a8ed2775aee000000000000000002000000000000000000000000000000000000000000000000000000000000000104636f696e087472616e73666572010700000000000000000000000000000000000000000000000000000000000000010a6170746f735f636f696e094170746f73436f696e000220094c6fc0d3b382a599c37e1aaa7618eff2c96a3586876082c4594c50c50d7dde082a00000000000000204e0000000000006400000000000000565c51630000000022")

    # Send the sign device instruction.
    # As it requires on-screen validation, the function is asynchronous.
    # It will yield the result when the navigation is done
    with client.sign_tx(path=path, transaction=transaction):
        # Validate the on-screen request by performing the navigation appropriate for this device
        if firmware.device.startswith("nano"):
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                      [NavInsID.BOTH_CLICK],
                                                      "Approve",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
        else:
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_REVIEW_TAP,
                                                      [NavInsID.USE_CASE_REVIEW_CONFIRM,
                                                       NavInsID.USE_CASE_STATUS_DISMISS],
                                                      "Hold to sign",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)

    # The device as yielded the result, parse it and ensure that the signature is correct
    response = client.get_async_response().data
    _, sig, _ = unpack_sign_tx_response(response)
    assert check_signature_validity(public_key, sig, transaction)


# In this test we send to the device a transaction to sign and validate it on screen
# The transaction will be sent in multiple chunks
# Also, this transaction has a request for blind signing activation
def test_blind_sign_tx_long_tx(firmware, backend, navigator, test_name):
    # Use the app interface instead of raw interface
    client = AptosCommandSender(backend)
    path: str = "m/44'/637'/1'/0'/0'"

    rapdu = client.get_public_key(path=path)
    _, public_key, _, _ = unpack_get_public_key_response(rapdu.data)

    transaction = bytes.fromhex("b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b193094c6fc0d3b382a599c37e1aaa7618eff2c96a3586876082c4594c50c50d7dde1b0000000000000002190d44266241744264b964a37b8f09863167a12d3e70cda39376cfb4e3561e120a736372697074735f76320473776170030700000000000000000000000000000000000000000000000000000000000000010a6170746f735f636f696e094170746f73436f696e000743417434fd869edee76cca2a4d2301e528a1551b1d719b75c350c3c97d15b8b905636f696e7304555344540007190d44266241744264b964a37b8f09863167a12d3e70cda39376cfb4e3561e12066375727665730c556e636f7272656c6174656400020800e1f5050000000008decbb30000000000480000000000000064000000000000008a9ba4640000000002")

    with client.sign_tx(path=path, transaction=transaction):
        if firmware.device.startswith("nano"):
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                      [NavInsID.BOTH_CLICK],
                                                      "Allow",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name + "/part0",
                                                      screen_change_after_last_instruction=False)
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                      [NavInsID.BOTH_CLICK],
                                                      "Approve",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name + "/part1")
        else:
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_REVIEW_TAP,
                                                      [NavInsID.USE_CASE_CHOICE_CONFIRM,
                                                       NavInsID.USE_CASE_STATUS_DISMISS],
                                                      "Enable blind signing",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name + "/part0",
                                                      screen_change_after_last_instruction=False)
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_REVIEW_TAP,
                                                      [NavInsID.USE_CASE_REVIEW_CONFIRM,
                                                       NavInsID.USE_CASE_STATUS_DISMISS],
                                                      "Hold to sign",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name + "/part1")
    response = client.get_async_response().data
    _, sig, _ = unpack_sign_tx_response(response)
    assert check_signature_validity(public_key, sig, transaction)


# Transaction signature refused test
# The test will ask for a transaction signature that will be refused on screen
def test_sign_tx_refused(firmware, backend, navigator, test_name):
    # Use the app interface instead of raw interface
    client = AptosCommandSender(backend)
    path: str = "m/44'/637'/1'/0'/0'"

    transaction = bytes.fromhex("b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b193094c6fc0d3b382a599c37e1aaa7618eff2c96a3586876082c4594c50c50d7dde1b000000000000000200000000000000000000000000000000000000000000000000000000000000010d6170746f735f6163636f756e74087472616e736665720002203835075df1bf469c336eabed8ac87052ee4485f3ec93380a5382fbf76b7a33070840420f000000000006000000000000006400000000000000c39aa4640000000002")

    if firmware.device.startswith("nano"):
        with pytest.raises(ExceptionRAPDU) as e:
            with client.sign_tx(path=path, transaction=transaction):
                navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                          [NavInsID.BOTH_CLICK],
                                                          "Reject",
                                                          ROOT_SCREENSHOT_PATH,
                                                          test_name)

        # Assert that we have received a refusal
        assert e.value.status == Errors.SW_DENY
        assert len(e.value.data) == 0
    else:
        for i in range(4):
            instructions = [NavInsID.USE_CASE_REVIEW_TAP] * i
            instructions += [NavInsID.USE_CASE_REVIEW_REJECT,
                             NavInsID.USE_CASE_CHOICE_CONFIRM,
                             NavInsID.USE_CASE_STATUS_DISMISS]
            with pytest.raises(ExceptionRAPDU) as e:
                with client.sign_tx(path=path, transaction=transaction):
                    navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                                   test_name + f"/part{i}",
                                                   instructions)
            # Assert that we have received a refusal
            assert e.value.status == Errors.SW_DENY
            assert len(e.value.data) == 0

# In this test we send to the device a message to sign and validate it on screen
# We will ensure that the displayed information is correct by using screenshots comparison
def test_sign_tx_short_msg(firmware, backend, navigator, test_name):
    # Use the app interface instead of raw interface
    client = AptosCommandSender(backend)
    # The path used for this entire test
    path: str = "m/44'/637'/1'/0'/0'"

    # First we need to get the public key of the device in order to build the transaction
    rapdu = client.get_public_key(path=path)
    _, public_key, _, _ = unpack_get_public_key_response(rapdu.data)

    # Create the mes that will be sent to the device for signing
    message = bytes("Hello Ledger!", 'utf-8')

    # Send the sign device instruction.
    # As it requires on-screen validation, the function is asynchronous.
    # It will yield the result when the navigation is done
    with client.sign_tx(path=path, transaction=message):
        # Validate the on-screen request by performing the navigation appropriate for this device
        if firmware.device.startswith("nano"):
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                      [NavInsID.BOTH_CLICK],
                                                      "Approve",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
        else:
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_REVIEW_TAP,
                                                      [NavInsID.USE_CASE_REVIEW_CONFIRM,
                                                       NavInsID.USE_CASE_STATUS_DISMISS],
                                                      "Hold to sign",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)

    # The device as yielded the result, parse it and ensure that the signature is correct
    response = client.get_async_response().data
    _, sig, _ = unpack_sign_tx_response(response)
    assert check_signature_validity(public_key, sig, message)

# In this test we send to the device a message to sign and validate it on screen
# We will ensure that the displayed information is correct by using screenshots comparison
def test_sign_short_raw_msg(firmware, backend, navigator, test_name):
    # Use the app interface instead of raw interface
    client = AptosCommandSender(backend)
    # The path used for this entire test
    path: str = "m/44'/637'/1'/0'/0'"

    # First we need to get the public key of the device in order to build the transaction
    rapdu = client.get_public_key(path=path)
    _, public_key, _, _ = unpack_get_public_key_response(rapdu.data)

    # Create the mes that will be sent to the device for signing
    message = bytes.fromhex("01020304ff")

    # Send the sign device instruction.
    # As it requires on-screen validation, the function is asynchronous.
    # It will yield the result when the navigation is done
    with client.sign_tx(path=path, transaction=message):
        # Validate the on-screen request by performing the navigation appropriate for this device
        if firmware.device.startswith("nano"):
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                      [NavInsID.BOTH_CLICK],
                                                      "Approve",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
        else:
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_REVIEW_TAP,
                                                      [NavInsID.USE_CASE_REVIEW_CONFIRM,
                                                       NavInsID.USE_CASE_STATUS_DISMISS],
                                                      "Hold to sign",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)

    # The device as yielded the result, parse it and ensure that the signature is correct
    response = client.get_async_response().data
    _, sig, _ = unpack_sign_tx_response(response)
    assert check_signature_validity(public_key, sig, message)

# In this test we send to the device a message to sign and validate it on screen
# We will ensure that the displayed information is correct by using screenshots comparison
def test_sign_long_raw_msg(firmware, backend, navigator, test_name):
    # Use the app interface instead of raw interface
    client = AptosCommandSender(backend)
    # The path used for this entire test
    path: str = "m/44'/637'/1'/0'/0'"

    # First we need to get the public key of the device in order to build the transaction
    rapdu = client.get_public_key(path=path)
    _, public_key, _, _ = unpack_get_public_key_response(rapdu.data)

    # Create the mes that will be sent to the device for signing
    message = bytes.fromhex("bc6f6693bddc1a9fec9e674a461eaa00b193094c6fc0d3b382a599c37e1aaa7618eff2c96a3586876082c4594c50c50d7dde1b0000000000000002190d44266241744264b964a37b8f09863167a12d3e70cda39376cfb4e3561e120a736372697074735f76320473776170030700000000000000000000000000000000000000000000000000000000000000010a6170746f735f636f696e094170746f73436f696e000743417434fd869edee76cca2a4d2301e528a1551b1d719b75c350c3c97d15b8b905636f696e7304555344540007190d44266241744264b964a37b8f09863167a12d3e70cda39376cfb4e3561e12066375727665730c556e636f7272656c6174656400020800e1f5050000000008decbb30000000000480000000000000064000000000000008a9ba4640000000002")

    with client.sign_tx(path=path, transaction=message):
        if firmware.device.startswith("nano"):
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                        [NavInsID.BOTH_CLICK],
                                                        "Allow",
                                                        ROOT_SCREENSHOT_PATH,
                                                        test_name + "/part0",
                                                        screen_change_after_last_instruction=False)
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                        [NavInsID.BOTH_CLICK],
                                                        "Approve",
                                                        ROOT_SCREENSHOT_PATH,
                                                        test_name + "/part1")
        else:
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_REVIEW_TAP,
                                                        [NavInsID.USE_CASE_CHOICE_CONFIRM,
                                                        NavInsID.USE_CASE_STATUS_DISMISS],
                                                        "Enable blind signing",
                                                        ROOT_SCREENSHOT_PATH,
                                                        test_name + "/part0",
                                                        screen_change_after_last_instruction=False)
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_REVIEW_TAP,
                                                        [NavInsID.USE_CASE_REVIEW_CONFIRM,
                                                        NavInsID.USE_CASE_STATUS_DISMISS],
                                                        "Hold to sign",
                                                        ROOT_SCREENSHOT_PATH,
                                                        test_name + "/part1")

    # The device as yielded the result, parse it and ensure that the signature is correct
    response = client.get_async_response().data
    _, sig, _ = unpack_sign_tx_response(response)
    assert check_signature_validity(public_key, sig, message)
