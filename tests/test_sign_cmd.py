import pytest

from application_client.aptos_command_sender import AptosCommandSender, Errors
from application_client.aptos_response_unpacker import unpack_get_public_key_response, unpack_sign_tx_response
from ragger.error import ExceptionRAPDU
from ragger.navigator import NavInsID, NavIns
from utils import ROOT_SCREENSHOT_PATH, check_signature_validity

from aptos_sdk.transactions import RawTransaction, TransactionPayload, TransactionArgument, EntryFunction
from aptos_sdk.account import AccountAddress
from aptos_sdk.type_tag import StructTag, TypeTag
from aptos_sdk.bcs import Serializer


# In this tests we check the behavior of the device when asked to sign a transaction

# This fixture is used to disable the blind signing after a test that enabled it
@pytest.fixture
def disable_blind_signing(firmware, backend, navigator):
    yield

    if firmware.device.startswith("nano"):
        backend.right_click()
        backend.both_click()
        backend.right_click()
        backend.both_click()
        backend.left_click()
        backend.both_click()
        backend.right_click()
        backend.both_click()
    else:
        instructions = [
            NavInsID.USE_CASE_HOME_SETTINGS,
            NavIns(NavInsID.TOUCH, (200, 113)),
            NavInsID.USE_CASE_SETTINGS_MULTI_PAGE_EXIT
        ]
        navigator.navigate(instructions, screen_change_before_first_instruction=False)


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
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
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
def test_blind_sign_tx_long_tx(firmware, backend, navigator, test_name, disable_blind_signing):
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
            navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                           test_name + "/part0",
                                           [NavInsID.USE_CASE_CHOICE_CONFIRM,
                                            NavInsID.USE_CASE_STATUS_DISMISS,
                                            NavInsID.USE_CASE_CHOICE_REJECT,
                                            NavInsID.INFO_HEADER_TAP,
                                            NavInsID.NAVIGATION_HEADER_TAP],
                                           screen_change_after_last_instruction=False)
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
                                                      [NavInsID.INFO_HEADER_TAP,
                                                       NavInsID.NAVIGATION_HEADER_TAP,
                                                       NavInsID.USE_CASE_REVIEW_CONFIRM,
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
            instructions = [NavInsID.USE_CASE_VIEW_DETAILS_NEXT] * i
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
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
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
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
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
def test_sign_long_raw_msg(firmware, backend, navigator, test_name, disable_blind_signing):
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
            navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                           test_name + "/part0",
                                           [NavInsID.USE_CASE_CHOICE_CONFIRM,
                                            NavInsID.USE_CASE_STATUS_DISMISS,
                                            NavInsID.USE_CASE_CHOICE_REJECT,
                                            NavInsID.INFO_HEADER_TAP,
                                            NavInsID.NAVIGATION_HEADER_TAP],
                                           screen_change_after_last_instruction=False)
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
                                                      [NavInsID.INFO_HEADER_TAP,
                                                       NavInsID.NAVIGATION_HEADER_TAP,
                                                       NavInsID.USE_CASE_REVIEW_CONFIRM,
                                                       NavInsID.USE_CASE_STATUS_DISMISS],
                                                      "Hold to sign",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name + "/part1")

    # The device as yielded the result, parse it and ensure that the signature is correct
    response = client.get_async_response().data
    _, sig, _ = unpack_sign_tx_response(response)
    assert check_signature_validity(public_key, sig, message)

# In this test we send to the device a transaction to sign and validate it on screen
# The transaction is Legacy Tokens to be Clear Signed and is listed in the app
def test_sign_listed_legacy_tokens(firmware, backend, navigator, test_name):
    # Use the app interface instead of raw interface
    client = AptosCommandSender(backend)
    # The path used for this entire test
    path: str = "m/44'/637'/1'/0'/0'"

    # First we need to get the public key of the device in order to build the transaction
    rapdu = client.get_public_key(path=path)
    _, public_key, _, _ = unpack_get_public_key_response(rapdu.data)

    # Create the transaction that will be sent to the device for signing
    transaction = bytes.fromhex("b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b1934e5e65d5c7a3191e4310ecd210e8f0ff53823189123b47086d928bd574a573d114000000000000000200000000000000000000000000000000000000000000000000000000000000010d6170746f735f6163636f756e740e7472616e736665725f636f696e730107d11107bdf0d6d7040c6c0bfbdecb6545191fdf13e8d8d259952f53e1713f61b50b7374616b65645f636f696e0b5374616b65644170746f73000220a0d8abc262e3321f87d745bd5d687e8f3fb14c87d48f840b6b56867df0026ec808a0936c02000000000b0000000000000064000000000000005459d0")

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
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
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
# The transaction is Legacy Tokens to be Clear Signed and is whitelisted in the app
def test_sign_unlisted_legacy_tokens(firmware, backend, navigator, test_name):
    # Use the app interface instead of raw interface
    client = AptosCommandSender(backend)
    # The path used for this entire test
    path: str = "m/44'/637'/1'/0'/0'"

    # First we need to get the public key of the device in order to build the transaction
    rapdu = client.get_public_key(path=path)
    _, public_key, _, _ = unpack_get_public_key_response(rapdu.data)

    # Create the transaction that will be sent to the device for signing
    transaction = bytes.fromhex("b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b1934e5e65d5c7a3191e4310ecd210e8f0ff53823189123b47086d928bd574a573d114000000000000000200000000000000000000000000000000000000000000000000000000000000010d6170746f735f6163636f756e740e7472616e736665725f636f696e730107804cef4821e11c55e87f2e9ec7dfc0d31d297cd34d20bfb2ae166e5069b40fe20b6c65646765725f636f696e0b4c65646765724170746f73000220a0d8abc262e3321f87d745bd5d687e8f3fb14c87d48f840b6b56867df0026ec808a0936c02000000000b0000000000000064000000000000005459d0")

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
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
                                                      [NavInsID.USE_CASE_REVIEW_CONFIRM,
                                                       NavInsID.USE_CASE_STATUS_DISMISS],
                                                      "Hold to sign",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)

    # The device as yielded the result, parse it and ensure that the signature is correct
    response = client.get_async_response().data
    _, sig, _ = unpack_sign_tx_response(response)
    assert check_signature_validity(public_key, sig, transaction)

# # In this test we send to the device a transaction to sign and validate it on screen
# # The transaction is Fungible Asset Tokens and should be Clear Signed
def test_sign_fa_tx(firmware, backend, navigator, test_name):
    # Use the app interface instead of raw interface
    client = AptosCommandSender(backend)
    # The path used for this entire test
    path: str = "m/44'/637'/1'/0'/0'"

    # First we need to get the public key of the device in order to build the transaction
    rapdu = client.get_public_key(path=path)
    _, public_key, _, _ = unpack_get_public_key_response(rapdu.data)

    # Create the transaction that will be sent to the device for signing
    transaction = bytes.fromhex("b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b1938f13f355f3af444bd356adeaaaf01235a7817d6a4417f5c9fa3d74a68f7b7afd0000000000000000020000000000000000000000000000000000000000000000000000000000000001167072696d6172795f66756e6769626c655f73746f7265087472616e73666572010700000000000000000000000000000000000000000000000000000000000000010e66756e6769626c655f6173736574084d65746164617461000320357b0b74bc833e95a115ad22604854d6b0fca151cecd94111770e5d6ffc9dc2b207be51d04d3a482fa056bc094bc5eadad005aaf823a95269410f08730f0d03cb40840420f000000000009000000000000006400000000000000000000000000000001")
    
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
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
                                                      [NavInsID.USE_CASE_REVIEW_CONFIRM,
                                                       NavInsID.USE_CASE_STATUS_DISMISS],
                                                      "Hold to sign",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)

    # The device as yielded the result, parse it and ensure that the signature is correct
    response = client.get_async_response().data
    _, sig, _ = unpack_sign_tx_response(response)
    assert check_signature_validity(public_key, sig, transaction)


# # In this test we send to the device a transaction to sign and validate it on screen
# # The transaction is a Staking transaction and should be Clear Signed
def test_sign_staking_aptos(firmware, backend, navigator, test_name):
    # Use the app interface instead of raw interface
    client = AptosCommandSender(backend)
    # The path used for this entire test
    path: str = "m/44'/637'/1'/0'/0'"

    # First we need to get the public key of the device in order to build the transaction
    rapdu = client.get_public_key(path=path)
    _, public_key, _, _ = unpack_get_public_key_response(rapdu.data)

    # Create the transaction that will be sent to the device for signing
    sender = AccountAddress.from_str("0x8F13f355F3aF444BD356ADEAAAF01235A7817D6A4417F5c9FA3D74A68F7b7AFD")
    pool =  AccountAddress.from_str("0xA651C7C52D64A2014379902BBC92439D196499BCC36D94FF0395AA45837C66DB")
    max_gas_amount = 100
    fees = 39 * max_gas_amount

    # Transaction parameters
    sequence_number = 0
    expiration_timestamp = 0
    payload = EntryFunction.natural(
        "0x1::delegation_pool",
        "add_stake",
        [],
        [
            TransactionArgument(pool, Serializer.struct),
            TransactionArgument(130976311, Serializer.u64),
        ],
    )

    # Create the raw transaction (TX_RAW)
    gas_unit_price = int(fees/max_gas_amount)
    txn = RawTransaction(
        sender=sender,
        sequence_number=sequence_number,
        payload=TransactionPayload(payload),
        max_gas_amount=max_gas_amount,
        gas_unit_price=gas_unit_price,
        expiration_timestamps_secs=expiration_timestamp,
        chain_id=1,
    )
    # Serialize the transaction
    serializer = Serializer()
    txn.serialize(serializer)
    transaction = serializer.output()
    # This is a salt required by the Nano App to make sure that the payload comes from Ledger Live host
    transaction = bytes.fromhex("b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b193" + transaction.hex())

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
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
                                                      [NavInsID.USE_CASE_REVIEW_CONFIRM,
                                                       NavInsID.USE_CASE_STATUS_DISMISS],
                                                      "Hold to sign",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)

    # The device as yielded the result, parse it and ensure that the signature is correct
    response = client.get_async_response().data
    _, sig, _ = unpack_sign_tx_response(response)
    assert check_signature_validity(public_key, sig, transaction)

# # In this test we send to the device a transaction to sign and validate it on screen
# # The transaction is a Unlock stake transaction and should be Clear Signed
def test_sign_unlocking_aptos(firmware, backend, navigator, test_name):
    # Use the app interface instead of raw interface
    client = AptosCommandSender(backend)
    # The path used for this entire test
    path: str = "m/44'/637'/1'/0'/0'"

    # First we need to get the public key of the device in order to build the transaction
    rapdu = client.get_public_key(path=path)
    _, public_key, _, _ = unpack_get_public_key_response(rapdu.data)

    # Create the transaction that will be sent to the device for signing
    sender = AccountAddress.from_str("0x8F13f355F3aF444BD356ADEAAAF01235A7817D6A4417F5c9FA3D74A68F7b7AFD")
    pool =  AccountAddress.from_str("0x8F13f355F3aF444BD356ADEAAAF01235A7817D6A4417F5c9FA3D74A68F7b7AFD")
    max_gas_amount = 100
    fees = 6 * max_gas_amount

    # Transaction parameters
    sequence_number = 0
    expiration_timestamp = 0
    payload = EntryFunction.natural(
        "0x1::delegation_pool",
        "unlock",
        [],
        [
            TransactionArgument(pool, Serializer.struct),
            TransactionArgument(12345, Serializer.u64),
        ],
    )

    # Create the raw transaction (TX_RAW)
    gas_unit_price = int(fees/max_gas_amount)
    txn = RawTransaction(
        sender=sender,
        sequence_number=sequence_number,
        payload=TransactionPayload(payload),
        max_gas_amount=max_gas_amount,
        gas_unit_price=gas_unit_price,
        expiration_timestamps_secs=expiration_timestamp,
        chain_id=1,
    )
    # Serialize the transaction
    serializer = Serializer()
    txn.serialize(serializer)
    transaction = serializer.output()
    # This is a salt required by the Nano App to make sure that the payload comes from Ledger Live host
    transaction = bytes.fromhex("b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b193" + transaction.hex())

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
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
                                                      [NavInsID.USE_CASE_REVIEW_CONFIRM,
                                                       NavInsID.USE_CASE_STATUS_DISMISS],
                                                      "Hold to sign",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)

    # The device as yielded the result, parse it and ensure that the signature is correct
    response = client.get_async_response().data
    _, sig, _ = unpack_sign_tx_response(response)
    assert check_signature_validity(public_key, sig, transaction)

# # In this test we send to the device a transaction to sign and validate it on screen
# # The transaction is a reactivate stake transaction and should be Clear Signed
def test_sign_reactivate_aptos(firmware, backend, navigator, test_name):
    # Use the app interface instead of raw interface
    client = AptosCommandSender(backend)
    # The path used for this entire test
    path: str = "m/44'/637'/1'/0'/0'"

    # First we need to get the public key of the device in order to build the transaction
    rapdu = client.get_public_key(path=path)
    _, public_key, _, _ = unpack_get_public_key_response(rapdu.data)

    # Create the transaction that will be sent to the device for signing
    sender = AccountAddress.from_str("0x8F13f355F3aF444BD356ADEAAAF01235A7817D6A4417F5c9FA3D74A68F7b7AFD")
    pool =  AccountAddress.from_str("0x8F13f355F3aF444BD356ADEAAAF01235A7817D6A4417F5c9FA3D74A68F7b7AFD")
    max_gas_amount = 100
    fees = 6 * max_gas_amount

    # Transaction parameters
    sequence_number = 0
    expiration_timestamp = 0
    payload = EntryFunction.natural(
        "0x1::delegation_pool",
        "reactivate_stake",
        [],
        [
            TransactionArgument(pool, Serializer.struct),
            TransactionArgument(12345, Serializer.u64),
        ],
    )

    # Create the raw transaction (TX_RAW)
    gas_unit_price = int(fees/max_gas_amount)
    txn = RawTransaction(
        sender=sender,
        sequence_number=sequence_number,
        payload=TransactionPayload(payload),
        max_gas_amount=max_gas_amount,
        gas_unit_price=gas_unit_price,
        expiration_timestamps_secs=expiration_timestamp,
        chain_id=1,
    )
    # Serialize the transaction
    serializer = Serializer()
    txn.serialize(serializer)
    transaction = serializer.output()
    # This is a salt required by the Nano App to make sure that the payload comes from Ledger Live host
    transaction = bytes.fromhex("b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b193" + transaction.hex())

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
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
                                                      [NavInsID.USE_CASE_REVIEW_CONFIRM,
                                                       NavInsID.USE_CASE_STATUS_DISMISS],
                                                      "Hold to sign",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)

    # The device as yielded the result, parse it and ensure that the signature is correct
    response = client.get_async_response().data
    _, sig, _ = unpack_sign_tx_response(response)
    assert check_signature_validity(public_key, sig, transaction)

# # In this test we send to the device a transaction to sign and validate it on screen
# # The transaction is a reactivate stake transaction and should be Clear Signed
def test_sign_withdraw_aptos(firmware, backend, navigator, test_name):
    # Use the app interface instead of raw interface
    client = AptosCommandSender(backend)
    # The path used for this entire test
    path: str = "m/44'/637'/1'/0'/0'"

    # First we need to get the public key of the device in order to build the transaction
    rapdu = client.get_public_key(path=path)
    _, public_key, _, _ = unpack_get_public_key_response(rapdu.data)

    # Create the transaction that will be sent to the device for signing
    sender = AccountAddress.from_str("0x8F13f355F3aF444BD356ADEAAAF01235A7817D6A4417F5c9FA3D74A68F7b7AFD")
    pool =  AccountAddress.from_str("0x8F13f355F3aF444BD356ADEAAAF01235A7817D6A4417F5c9FA3D74A68F7b7AFD")
    max_gas_amount = 100
    fees = 6 * max_gas_amount

    # Transaction parameters
    sequence_number = 0
    expiration_timestamp = 0
    payload = EntryFunction.natural(
        "0x1::delegation_pool",
        "withdraw",
        [],
        [
            TransactionArgument(pool, Serializer.struct),
            TransactionArgument(12345, Serializer.u64),
        ],
    )

    # Create the raw transaction (TX_RAW)
    gas_unit_price = int(fees/max_gas_amount)
    txn = RawTransaction(
        sender=sender,
        sequence_number=sequence_number,
        payload=TransactionPayload(payload),
        max_gas_amount=max_gas_amount,
        gas_unit_price=gas_unit_price,
        expiration_timestamps_secs=expiration_timestamp,
        chain_id=1,
    )
    # Serialize the transaction
    serializer = Serializer()
    txn.serialize(serializer)
    transaction = serializer.output()
    # This is a salt required by the Nano App to make sure that the payload comes from Ledger Live host
    transaction = bytes.fromhex("b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b193" + transaction.hex())

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
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
                                                      [NavInsID.USE_CASE_REVIEW_CONFIRM,
                                                       NavInsID.USE_CASE_STATUS_DISMISS],
                                                      "Hold to sign",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)

    # The device as yielded the result, parse it and ensure that the signature is correct
    response = client.get_async_response().data
    _, sig, _ = unpack_sign_tx_response(response)
    assert check_signature_validity(public_key, sig, transaction)