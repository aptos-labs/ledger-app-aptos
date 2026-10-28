"""Aptos SIGN_TX transport, clear-signed transfer and blind-signing functional tests."""

import hashlib

import pytest
from application_client.aptos_command_sender import AptosCommandSender
from application_client.aptos_pubkey import AptosPubkeyClient, unpack_public_key
from application_client.boilerplate_command_sender import MAX_APDU_LEN, Errors
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
# A RawTransactionWithData::MultiAgent fixture: the recognized prefix, the
# variant, the RawTransaction fixture body and one secondary signer.
TRANSACTION_WITH_DATA = RAW_TRANSACTION_WITH_DATA_PREFIX + b"\x00" + RAW_TRANSACTION_BODY + b"\x01" + b"\x55" * 32

# v1's short transaction: `0x1::coin::transfer<0x1::aptos_coin::AptosCoin>` of 42 octas.
SHORT_TRANSACTION = bytes.fromhex(
    "b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b193783135e8b00430253a22ba041d860c373d7a1501ccf7ac2d1ad37a8ed2775aee"
    "000000000000000002000000000000000000000000000000000000000000000000000000000000000104636f696e087472616e73666572010700000000000000000000000000000000000000000000000000000000000000010a6170746f735f636f696e094170746f73436f696e000220094c6fc0d3b382a599c37e1aaa7618eff2c96a3586876082c4594c50c50d7dde082a00000000000000204e0000000000006400000000000000565c51630000000022"
)
# v1's listed and unlisted token fixtures stop 255 bytes in, six bytes short of
# a complete footer. v1 read the footer from the tail and displayed a garbage
# gas fee; v2 parses front to back and rejects them.
V1_TRUNCATED_LISTED_TRANSACTION = bytes.fromhex(
    "b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b1934e5e65d5c7a3191e4310ecd210e8f0ff53823189123b47086d928bd574a573d1"
    "14000000000000000200000000000000000000000000000000000000000000000000000000000000010d6170746f735f6163636f756e740e7472616e736665725f636f696e730107d11107bdf0d6d7040c6c0bfbdecb6545191fdf13e8d8d259952f53e1713f61b50b7374616b65645f636f696e0b5374616b65644170746f73000220a0d8abc262e3321f87d745bd5d687e8f3fb14c87d48f840b6b56867df0026ec808a0936c02000000000b0000000000000064000000000000005459d0"
)
V1_TRUNCATED_UNLISTED_TRANSACTION = bytes.fromhex(
    "b5e97db07fa0bd0e5598aa3643a9bc6f6693bddc1a9fec9e674a461eaa00b1934e5e65d5c7a3191e4310ecd210e8f0ff53823189123b47086d928bd574a573d1"
    "14000000000000000200000000000000000000000000000000000000000000000000000000000000010d6170746f735f6163636f756e740e7472616e736665725f636f696e730107804cef4821e11c55e87f2e9ec7dfc0d31d297cd34d20bfb2ae166e5069b40fe20b6c65646765725f636f696e0b4c65646765724170746f73000220a0d8abc262e3321f87d745bd5d687e8f3fb14c87d48f840b6b56867df0026ec808a0936c02000000000b0000000000000064000000000000005459d0"
)


def _uleb(value):
    out = bytearray()
    while True:
        byte = value & 0x7F
        value >>= 7
        if value:
            out.append(byte | 0x80)
        else:
            out.append(byte)
            return bytes(out)


def _bcs_bytes(data):
    return _uleb(len(data)) + data


def _footer(max_gas=72, gas_unit_price=100):
    return max_gas.to_bytes(8, "little") + gas_unit_price.to_bytes(8, "little") + (1_700_000_000).to_bytes(8, "little") + b"\x02"


SENDER = bytes.fromhex("094c6fc0d3b382a599c37e1aaa7618eff2c96a3586876082c4594c50c50d7dde")


def _raw_body(payload, footer=None):
    return SENDER + (7).to_bytes(8, "little") + payload + (footer or _footer())


def _entry_function_body(module, function, address=b"\x00" * 31 + b"\x01"):
    return address + _bcs_bytes(module) + _bcs_bytes(function) + b"\x00" + b"\x00"


def _entry_function(module, function, address=b"\x00" * 31 + b"\x01"):
    return b"\x02" + _entry_function_body(module, function, address)


def _multisig(inner_payload):
    return b"\x03" + b"\x22" * 32 + inner_payload


FRAMEWORK = b"\x00" * 31 + b"\x01"
LOOKALIKE_FRAMEWORK = b"\x00" * 30 + b"\x01\x01"
# v1's listed-token fixture: sender, sequence number, receiver and amount.
TOKEN_SENDER = bytes.fromhex("4e5e65d5c7a3191e4310ecd210e8f0ff53823189123b47086d928bd574a573d1")
TOKEN_RECEIVER = bytes.fromhex("a0d8abc262e3321f87d745bd5d687e8f3fb14c87d48f840b6b56867df0026ec8")
TOKEN_AMOUNT = 40670112
DSTAPT_ADDRESS = bytes.fromhex("d11107bdf0d6d7040c6c0bfbdecb6545191fdf13e8d8d259952f53e1713f61b5")
SHRIMP_ADDRESS = bytes.fromhex("55987edfab9a57f69bac759674f139ae473b5e09a9283848c1f87faf6fc1e789")
LEDGER_COIN_ADDRESS = bytes.fromhex("804cef4821e11c55e87f2e9ec7dfc0d31d297cd34d20bfb2ae166e5069b40fe2")


def _struct_tag(address, module, name):
    return b"\x07" + address + _bcs_bytes(module) + _bcs_bytes(name) + b"\x00"


def _transfer_arguments(receiver=TOKEN_RECEIVER, amount=TOKEN_AMOUNT):
    return b"\x02" + _bcs_bytes(receiver) + _bcs_bytes(amount.to_bytes(8, "little"))


def _apt_transfer(arguments=None, address=FRAMEWORK):
    return (
        b"\x02"
        + address
        + _bcs_bytes(b"aptos_account")
        + _bcs_bytes(b"transfer")
        + b"\x00"
        + (arguments if arguments is not None else _transfer_arguments())
    )


def _coin_transfer(module, function, coin, arguments=None, address=FRAMEWORK):
    return (
        b"\x02"
        + address
        + _bcs_bytes(module)
        + _bcs_bytes(function)
        + b"\x01"
        + coin
        + (arguments if arguments is not None else _transfer_arguments())
    )


def _token_transaction(payload):
    return RAW_TRANSACTION_PREFIX + TOKEN_SENDER + (20).to_bytes(8, "little") + payload + _footer(max_gas=11, gas_unit_price=100)


DSTAPT = _struct_tag(DSTAPT_ADDRESS, b"staked_coin", b"StakedAptos")
LISTED_TRANSACTIONS = {
    # v1's listed fixture, completed: `aptos_account::transfer_coins<dstAPT>`.
    "dstapt": _token_transaction(_coin_transfer(b"aptos_account", b"transfer_coins", DSTAPT)),
    # The table matches case-insensitively, and SHRIMP has two decimals.
    "shrimp_uppercase": _token_transaction(
        _coin_transfer(b"coin", b"transfer", _struct_tag(SHRIMP_ADDRESS, b"SHRIMP", b"SHRIMPCOIN"))
    ),
    # The largest amount a transfer can carry.
    "max_amount": _token_transaction(
        _coin_transfer(b"aptos_account", b"transfer_coins", DSTAPT, _transfer_arguments(amount=2**64 - 1))
    ),
}
UNLISTED_TRANSACTIONS = {
    # v1's unlisted fixture, completed: `aptos_account::transfer_coins<LedgerAptos>`.
    "ledger_coin": _token_transaction(
        _coin_transfer(b"aptos_account", b"transfer_coins", _struct_tag(LEDGER_COIN_ADDRESS, b"ledger_coin", b"LedgerAptos"))
    ),
    # Only the full `0x1::aptos_coin::AptosCoin` type is APT: not a look-alike
    # address, and not a case variant, since the framework's identifiers are canonical.
    "lookalike_apt": _token_transaction(
        _coin_transfer(b"coin", b"transfer", _struct_tag(LOOKALIKE_FRAMEWORK, b"aptos_coin", b"AptosCoin"))
    ),
    "uppercase_apt": _token_transaction(
        _coin_transfer(b"coin", b"transfer", _struct_tag(FRAMEWORK, b"APTOS_COIN", b"APTOSCOIN"))
    ),
    # Maximum-length identifiers build the longest coin type a review can show.
    "longest_coin_type": _token_transaction(
        _coin_transfer(b"aptos_account", b"transfer_coins", _struct_tag(bytes.fromhex("ab" * 32), b"m" * 255, b"n" * 255))
    ),
}
APT_TRANSFER_TRANSACTION = _token_transaction(_apt_transfer())
ZERO_APT_TRANSFER_TRANSACTION = _token_transaction(_apt_transfer(_transfer_arguments(amount=0)))
# A `coin::transfer` published outside `0x1` is not the framework's.
LOOKALIKE_COIN_TRANSFER_TRANSACTION = _token_transaction(
    _coin_transfer(b"coin", b"transfer", _struct_tag(FRAMEWORK, b"aptos_coin", b"AptosCoin"), address=LOOKALIKE_FRAMEWORK)
)


# Type arguments at every shape the decoder must walk: eight nested vectors
# (aptos-core's nesting limit), a generic struct over i64, and a function type.
SCRIPT_TYPE_ARGS = (
    b"\x03"
    + b"\x06" * 8
    + b"\x01"
    + b"\x07"
    + b"\x00" * 31
    + b"\x01"
    + _bcs_bytes(b"coin")
    + _bcs_bytes(b"Coin")
    + b"\x01\x0f"
    + b"\x0b"
    + b"\x01\x00\x01"
    + b"\x00"
    + b"\x02"
)
SCRIPT_BODY = (
    _bcs_bytes(b"\xa1\x1c")
    + SCRIPT_TYPE_ARGS
    + b"\x04"
    + b"\x00\x05"
    + b"\x0d"
    + (-1).to_bytes(8, "little", signed=True)
    + b"\x04\x01\xff"
    + b"\x0f"
    + (-(2**255)).to_bytes(32, "little", signed=True)
)
SCRIPT_PAYLOAD = b"\x00" + SCRIPT_BODY
MULTISIG_PAYLOAD = _multisig(b"\x00")
SCRIPT_TRANSACTION = RAW_TRANSACTION_PREFIX + _raw_body(SCRIPT_PAYLOAD)
MULTISIG_TRANSACTION = RAW_TRANSACTION_PREFIX + _raw_body(MULTISIG_PAYLOAD)
MULTISIG_INNER_PAYLOADS = {
    "stored_on_chain": b"\x00",
    "entry_function": b"\x01\x00" + _entry_function_body(b"coin", b"transfer"),
    "script": b"\x01\x01" + SCRIPT_BODY,
}
FEE_PAYER_TRANSACTION = (
    RAW_TRANSACTION_WITH_DATA_PREFIX
    + b"\x01"
    + _raw_body(_entry_function(b"coin", b"transfer"))
    + b"\x01"
    + b"\x33" * 32
    + b"\x44" * 32
)
# Maximum-length identifiers build the longest function name a review can show.
LONGEST_FUNCTION_TRANSACTION = RAW_TRANSACTION_PREFIX + _raw_body(
    _entry_function(b"m" * 255, b"f" * 255, address=bytes.fromhex("ab" * 32))
)

SHORT_MESSAGE = b"Hello Ledger!"
SHORT_RAW_MESSAGE = bytes.fromhex("01020304ff")
LONG_ASCII_MESSAGE = b"The quick brown fox jumps over the lazy dog. " * 6
LONG_RAW_MESSAGE = bytes(range(256)) * 3
EMBEDDED_NUL_MESSAGE = b"Hello\x00Ledger"
MALFORMED_TRANSACTION = RAW_TRANSACTION_PREFIX + b"\x00"
MALFORMED_TRANSACTION_WITH_DATA = RAW_TRANSACTION_WITH_DATA_PREFIX + b"\x00"
MALFORMED_TRANSACTIONS = [
    MALFORMED_TRANSACTION,
    MALFORMED_TRANSACTION_WITH_DATA,
    # Trailing bytes after a complete transaction.
    SCRIPT_TRANSACTION + b"\x00",
    MULTISIG_TRANSACTION + b"\x00",
    FEE_PAYER_TRANSACTION + b"\x00",
    # Truncated before the chain id.
    SCRIPT_TRANSACTION[:-1],
    # Fee-payer transaction truncated before the fee payer address.
    FEE_PAYER_TRANSACTION[:-32],
    # Module name length written with a redundant ULEB128 byte.
    RAW_TRANSACTION_PREFIX
    + _raw_body(b"\x02" + b"\x00" * 31 + b"\x01" + b"\x84\x00" + b"coin" + _bcs_bytes(b"transfer") + b"\x00\x00"),
    # Module name length above u32::MAX.
    RAW_TRANSACTION_PREFIX
    + _raw_body(b"\x02" + b"\x00" * 31 + b"\x01" + b"\xff\xff\xff\xff\x7f" + b"coin" + _bcs_bytes(b"transfer") + b"\x00\x00"),
    # Identifiers may only hold ASCII letters, digits, underscores and dollars.
    RAW_TRANSACTION_PREFIX + _raw_body(_entry_function(b"co in", b"transfer")),
    # Identifiers are never empty.
    RAW_TRANSACTION_PREFIX + _raw_body(_entry_function(b"coin", b"")),
    # Identifiers are at most 255 bytes.
    RAW_TRANSACTION_PREFIX + _raw_body(_entry_function(b"coin", b"f" * 256)),
    # Generic argument decoding runs for functions outside the clear-signed set.
    # Unknown type tag.
    RAW_TRANSACTION_PREFIX
    + _raw_body(b"\x02" + b"\x00" * 31 + b"\x01" + _bcs_bytes(b"coin") + _bcs_bytes(b"deposit") + b"\x01\x12\x00"),
    # Type arguments nested deeper than aptos-core deserializes.
    RAW_TRANSACTION_PREFIX
    + _raw_body(
        b"\x02" + b"\x00" * 31 + b"\x01" + _bcs_bytes(b"coin") + _bcs_bytes(b"deposit") + b"\x01" + b"\x06" * 9 + b"\x01\x00"
    ),
    # Function type parameter that is neither a value nor a reference.
    RAW_TRANSACTION_PREFIX
    + _raw_body(
        b"\x02" + b"\x00" * 31 + b"\x01" + _bcs_bytes(b"coin") + _bcs_bytes(b"deposit") + b"\x01\x0b\x01\x03\x01\x00\x02\x00"
    ),
    # Unknown script argument variant.
    RAW_TRANSACTION_PREFIX + _raw_body(b"\x00" + _bcs_bytes(b"\xa1\x1c") + b"\x00" + b"\x01" + b"\x10\x00"),
    # Script argument truncated: a U64 with only two of its eight bytes.
    RAW_TRANSACTION_PREFIX
    + SENDER
    + (7).to_bytes(8, "little")
    + b"\x00"
    + _bcs_bytes(b"\xa1\x1c")
    + b"\x00"
    + b"\x01"
    + b"\x01\x00\x00",
    # ModuleBundle payloads are not supported.
    RAW_TRANSACTION_PREFIX + _raw_body(b"\x01"),
    # Multisig payload option tag other than 0 or 1.
    RAW_TRANSACTION_PREFIX + _raw_body(_multisig(b"\x02")),
    # Unknown multisig payload variant.
    RAW_TRANSACTION_PREFIX + _raw_body(_multisig(b"\x01\x02" + _entry_function_body(b"coin", b"transfer"))),
    # Unknown RawTransactionWithData variant.
    RAW_TRANSACTION_WITH_DATA_PREFIX + b"\x02" + _raw_body(SCRIPT_PAYLOAD) + b"\x00",
    # Multi-agent transaction missing its secondary signer list.
    RAW_TRANSACTION_WITH_DATA_PREFIX + b"\x00" + _raw_body(SCRIPT_PAYLOAD),
    # v1's truncated token fixtures.
    V1_TRUNCATED_LISTED_TRANSACTION,
    V1_TRUNCATED_UNLISTED_TRANSACTION,
    # Known framework operations with an unexpected shape are rejected, as in v1.
    # `aptos_account::transfer` takes no type argument.
    _token_transaction(
        b"\x02" + FRAMEWORK + _bcs_bytes(b"aptos_account") + _bcs_bytes(b"transfer") + b"\x01" + DSTAPT + _transfer_arguments()
    ),
    # `coin::transfer` takes exactly one type argument.
    _token_transaction(b"\x02" + FRAMEWORK + _bcs_bytes(b"coin") + _bcs_bytes(b"transfer") + b"\x00" + _transfer_arguments()),
    # The coin type argument must be a struct.
    _token_transaction(_coin_transfer(b"coin", b"transfer", b"\x02")),
    # The coin struct takes no type arguments.
    _token_transaction(_coin_transfer(b"coin", b"transfer", DSTAPT[:-1] + b"\x01\x02")),
    # Transfers take exactly two arguments.
    _token_transaction(_apt_transfer(b"\x01" + _bcs_bytes(TOKEN_RECEIVER))),
    _token_transaction(_apt_transfer(_transfer_arguments() + _bcs_bytes(b"\x01"))),
    # The receiver is a 32-byte address.
    _token_transaction(_apt_transfer(b"\x02" + _bcs_bytes(TOKEN_RECEIVER[:-1]) + _bcs_bytes(TOKEN_AMOUNT.to_bytes(8, "little")))),
    # The amount is a u64.
    _token_transaction(
        _coin_transfer(
            b"coin", b"transfer", DSTAPT, b"\x02" + _bcs_bytes(TOKEN_RECEIVER) + _bcs_bytes(TOKEN_AMOUNT.to_bytes(9, "little"))
        )
    ),
]


def _review_clear(firmware, navigator, test_name, default_screenshot_path, finish_text):
    if firmware.device.startswith("nano"):
        navigator.navigate_until_text_and_compare(
            NavInsID.RIGHT_CLICK,
            [NavInsID.BOTH_CLICK],
            finish_text,
            default_screenshot_path,
            f"{test_name}/part0",
        )
    else:
        navigator.navigate_until_text_and_compare(
            NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
            [
                NavInsID.USE_CASE_REVIEW_CONFIRM,
                NavInsID.USE_CASE_STATUS_DISMISS,
            ],
            "Hold to sign",
            default_screenshot_path,
            f"{test_name}/part0",
        )


def _review_message(firmware, navigator, test_name, default_screenshot_path):
    _review_clear(firmware, navigator, test_name, default_screenshot_path, "Sign message?")


def _review_transaction(firmware, navigator, test_name, default_screenshot_path):
    _review_clear(firmware, navigator, test_name, default_screenshot_path, "Sign transaction")


def _review_blind(firmware, navigator, test_name, default_screenshot_path, finish_text):
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
            screen_change_after_last_instruction=False,
        )
        navigator.navigate_until_text_and_compare(
            NavInsID.RIGHT_CLICK,
            [NavInsID.BOTH_CLICK, NavInsID.BOTH_CLICK],
            finish_text,
            default_screenshot_path,
            f"{test_name}/part1",
        )
    else:
        navigator.navigate_and_compare(
            default_screenshot_path,
            f"{test_name}/part0",
            [
                NavInsID.USE_CASE_CHOICE_CONFIRM,
                NavInsID.USE_CASE_STATUS_DISMISS,
                NavInsID.USE_CASE_CHOICE_REJECT,
                NavInsID.INFO_HEADER_TAP,
                NavInsID.NAVIGATION_HEADER_TAP,
            ],
            screen_change_after_last_instruction=False,
        )
        navigator.navigate_until_text_and_compare(
            NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
            [
                NavInsID.INFO_HEADER_TAP,
                NavInsID.NAVIGATION_HEADER_TAP,
                NavInsID.USE_CASE_REVIEW_CONFIRM,
                NavInsID.USE_CASE_STATUS_DISMISS,
            ],
            "Hold to sign",
            default_screenshot_path,
            f"{test_name}/part1",
        )


def _review_blind_message(firmware, navigator, test_name, default_screenshot_path):
    _review_blind(firmware, navigator, test_name, default_screenshot_path, "Sign message?")


def _review_blind_transaction(firmware, navigator, test_name, default_screenshot_path):
    _review_blind(
        firmware,
        navigator,
        test_name,
        default_screenshot_path,
        "Sign transaction?",
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
    review,
):
    public_key, _ = unpack_public_key(AptosPubkeyClient(backend).get_public_key(PATH).data)
    client = AptosCommandSender(backend)
    with client.sign_tx(PATH, payload):
        review(firmware, navigator, test_name, default_screenshot_path)
    response = client.get_async_response()
    assert response is not None
    assert len(response.data) == 65
    assert response.data[0] == 64
    try:
        VerifyKey(public_key).verify(payload, response.data[1:])
    except BadSignatureError:
        pytest.fail("Device signature does not verify against the exact submitted bytes")


def test_sign_tx_short_msg(backend, firmware, navigator, test_name, default_screenshot_path):
    _sign_and_verify(
        backend,
        firmware,
        navigator,
        test_name,
        default_screenshot_path,
        SHORT_MESSAGE,
        _review_message,
    )


def test_sign_short_raw_msg(backend, firmware, navigator, test_name, default_screenshot_path):
    _sign_and_verify(
        backend,
        firmware,
        navigator,
        test_name,
        default_screenshot_path,
        SHORT_RAW_MESSAGE,
        _review_message,
    )


def test_sign_long_ascii_msg(backend, firmware, navigator, test_name, default_screenshot_path):
    assert len(LONG_ASCII_MESSAGE) > 255
    _sign_and_verify(
        backend,
        firmware,
        navigator,
        test_name,
        default_screenshot_path,
        LONG_ASCII_MESSAGE,
        _review_message,
    )


def test_sign_ascii_msg_at_direct_limit(backend, firmware, navigator, test_name, default_screenshot_path):
    message = b"a" * 119
    _sign_and_verify(backend, firmware, navigator, test_name, default_screenshot_path, message, _review_message)


def test_sign_ascii_msg_just_over_direct_limit(backend, firmware, navigator, test_name, default_screenshot_path):
    message = b"a" * 120
    _sign_and_verify(backend, firmware, navigator, test_name, default_screenshot_path, message, _review_message)


def test_sign_raw_msg_at_clear_limit(backend, firmware, navigator, test_name, default_screenshot_path):
    message = b"\xff" * 59
    _sign_and_verify(backend, firmware, navigator, test_name, default_screenshot_path, message, _review_message)


def test_sign_raw_msg_just_over_clear_limit(
    backend, firmware, navigator, test_name, default_screenshot_path, disable_blind_signing
):
    message = b"\xff" * 60
    _sign_and_verify(
        backend,
        firmware,
        navigator,
        test_name,
        default_screenshot_path,
        message,
        _review_blind_message,
    )


def test_sign_long_ascii_msg_with_embedded_nul_is_blind(
    backend, firmware, navigator, test_name, default_screenshot_path, disable_blind_signing
):
    message = (b"The quick brown fox jumps over the lazy dog. " * 3)[:130] + b"\x00B"
    _sign_and_verify(
        backend,
        firmware,
        navigator,
        test_name,
        default_screenshot_path,
        message,
        _review_blind_message,
    )


def test_sign_message_with_embedded_nul(backend, firmware, navigator, test_name, default_screenshot_path, disable_blind_signing):
    assert EMBEDDED_NUL_MESSAGE.rstrip(b"\x00").count(b"\x00") == 1
    _sign_and_verify(
        backend,
        firmware,
        navigator,
        test_name,
        default_screenshot_path,
        EMBEDDED_NUL_MESSAGE,
        _review_blind_message,
    )


def test_sign_long_raw_msg(backend, firmware, navigator, test_name, default_screenshot_path, disable_blind_signing):
    assert len(LONG_RAW_MESSAGE) > 255
    _sign_and_verify(
        backend,
        firmware,
        navigator,
        test_name,
        default_screenshot_path,
        LONG_RAW_MESSAGE,
        _review_blind_message,
    )


def test_blind_sign_tx_long_tx(backend, firmware, navigator, test_name, default_screenshot_path, disable_blind_signing):
    assert len(TRANSACTION) > 255
    _sign_and_verify(
        backend,
        firmware,
        navigator,
        test_name,
        default_screenshot_path,
        TRANSACTION,
        _review_blind_transaction,
    )


def test_blind_sign_tx_with_data(backend, firmware, navigator, test_name, default_screenshot_path, disable_blind_signing):
    assert TRANSACTION_WITH_DATA.startswith(RAW_TRANSACTION_WITH_DATA_PREFIX)
    _sign_and_verify(
        backend,
        firmware,
        navigator,
        test_name,
        default_screenshot_path,
        TRANSACTION_WITH_DATA,
        _review_blind_transaction,
    )


def test_sign_tx_short_tx(backend, firmware, navigator, test_name, default_screenshot_path):
    _sign_and_verify(
        backend,
        firmware,
        navigator,
        test_name,
        default_screenshot_path,
        SHORT_TRANSACTION,
        _review_transaction,
    )


def test_sign_apt_transfer(backend, firmware, navigator, test_name, default_screenshot_path):
    _sign_and_verify(
        backend,
        firmware,
        navigator,
        test_name,
        default_screenshot_path,
        APT_TRANSFER_TRANSACTION,
        _review_transaction,
    )


@pytest.mark.parametrize("coin", LISTED_TRANSACTIONS.keys())
def test_sign_listed_legacy_tokens(backend, firmware, navigator, test_name, default_screenshot_path, coin):
    _sign_and_verify(
        backend,
        firmware,
        navigator,
        f"{test_name}_{coin}",
        default_screenshot_path,
        LISTED_TRANSACTIONS[coin],
        _review_transaction,
    )


@pytest.mark.parametrize("coin", UNLISTED_TRANSACTIONS.keys())
def test_sign_unlisted_legacy_tokens(backend, firmware, navigator, test_name, default_screenshot_path, coin):
    _sign_and_verify(
        backend,
        firmware,
        navigator,
        f"{test_name}_{coin}",
        default_screenshot_path,
        UNLISTED_TRANSACTIONS[coin],
        _review_transaction,
    )


def test_blind_sign_tx_coin_transfer_outside_framework(
    backend, firmware, navigator, test_name, default_screenshot_path, disable_blind_signing
):
    _sign_and_verify(
        backend,
        firmware,
        navigator,
        test_name,
        default_screenshot_path,
        LOOKALIKE_COIN_TRANSFER_TRANSACTION,
        _review_blind_transaction,
    )


def test_blind_sign_tx_script(backend, firmware, navigator, test_name, default_screenshot_path, disable_blind_signing):
    _sign_and_verify(
        backend,
        firmware,
        navigator,
        test_name,
        default_screenshot_path,
        SCRIPT_TRANSACTION,
        _review_blind_transaction,
    )


@pytest.mark.parametrize("inner_payload", MULTISIG_INNER_PAYLOADS.keys())
def test_blind_sign_tx_multisig(
    backend, firmware, navigator, test_name, default_screenshot_path, disable_blind_signing, inner_payload
):
    _sign_and_verify(
        backend,
        firmware,
        navigator,
        f"{test_name}_{inner_payload}",
        default_screenshot_path,
        RAW_TRANSACTION_PREFIX + _raw_body(_multisig(MULTISIG_INNER_PAYLOADS[inner_payload])),
        _review_blind_transaction,
    )


def test_blind_sign_tx_fee_payer(backend, firmware, navigator, test_name, default_screenshot_path, disable_blind_signing):
    _sign_and_verify(
        backend,
        firmware,
        navigator,
        test_name,
        default_screenshot_path,
        FEE_PAYER_TRANSACTION,
        _review_blind_transaction,
    )


def test_blind_sign_tx_longest_function_name(
    backend, firmware, navigator, test_name, default_screenshot_path, disable_blind_signing
):
    assert len(LONGEST_FUNCTION_TRANSACTION) > 255
    _sign_and_verify(
        backend,
        firmware,
        navigator,
        test_name,
        default_screenshot_path,
        LONGEST_FUNCTION_TRANSACTION,
        _review_blind_transaction,
    )


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


def test_sign_long_ascii_msg_refused(backend, firmware, navigator, test_name, default_screenshot_path):
    client = AptosCommandSender(backend)
    with pytest.raises(ExceptionRAPDU) as error:
        with client.sign_tx(PATH, LONG_ASCII_MESSAGE):
            if firmware.device.startswith("nano"):
                navigator.navigate_until_text_and_compare(
                    NavInsID.RIGHT_CLICK,
                    [NavInsID.BOTH_CLICK],
                    "Reject",
                    default_screenshot_path,
                    test_name,
                )
            else:
                navigator.navigate_and_compare(
                    default_screenshot_path,
                    test_name,
                    [
                        NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
                        NavInsID.USE_CASE_REVIEW_REJECT,
                        NavInsID.USE_CASE_CHOICE_CONFIRM,
                        NavInsID.USE_CASE_STATUS_DISMISS,
                    ],
                )
    assert error.value.status == Errors.SW_DENY
    assert error.value.data == b""


def test_sign_apt_transfer_refused(backend, firmware, navigator, test_name, default_screenshot_path):
    client = AptosCommandSender(backend)
    with pytest.raises(ExceptionRAPDU) as error:
        with client.sign_tx(PATH, ZERO_APT_TRANSFER_TRANSACTION):
            if firmware.device.startswith("nano"):
                navigator.navigate_until_text_and_compare(
                    NavInsID.RIGHT_CLICK,
                    [NavInsID.BOTH_CLICK],
                    "Reject",
                    default_screenshot_path,
                    test_name,
                )
            else:
                navigator.navigate_and_compare(
                    default_screenshot_path,
                    test_name,
                    [
                        NavInsID.USE_CASE_VIEW_DETAILS_NEXT,
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
        _review_blind_transaction(firmware, navigator, test_name, default_screenshot_path)
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
        _review_blind_message(firmware, navigator, test_name, default_screenshot_path)
    response = client.get_async_response()
    assert response is not None
    assert response.data[0] == 64
    assert len(response.data) == 65
    VerifyKey(public_key).verify(message, response.data[1:])


@pytest.mark.parametrize(
    "payload",
    MALFORMED_TRANSACTIONS,
)
def test_sign_tx_malformed_transaction_returns_parsing_error(backend, payload):
    client = AptosCommandSender(backend)
    client.start_signing(PATH)
    chunks = [payload[i : i + MAX_APDU_LEN] for i in range(0, len(payload), MAX_APDU_LEN)]
    for index, chunk in enumerate(chunks[:-1], start=1):
        assert client.send_chunk(index, True, chunk).status == Errors.SW_OK
    with pytest.raises(ExceptionRAPDU) as error:
        client.send_chunk(len(chunks), False, chunks[-1])
    assert error.value.status == Errors.SW_TX_PARSING_FAIL
    assert error.value.data == b""
    with pytest.raises(ExceptionRAPDU) as error:
        client.send_chunk(1, True, b"retry")
    assert error.value.status == Errors.SW_BAD_STATE
    assert error.value.data == b""


def test_sign_tx_gas_fee_overflow_is_rejected(backend):
    # 2**63 * 2 does not fit in 64 bits.
    payload = RAW_TRANSACTION_PREFIX + _raw_body(SCRIPT_PAYLOAD, _footer(max_gas=2**63, gas_unit_price=2))
    client = AptosCommandSender(backend)
    client.start_signing(PATH)
    with pytest.raises(ExceptionRAPDU) as error:
        client.send_chunk(1, False, payload)
    assert error.value.status == Errors.SW_DISPLAY_GAS_FEE_FAIL
    assert error.value.data == b""


def test_sign_tx_disallowed_path(backend):
    client = AptosCommandSender(backend)
    with pytest.raises(ExceptionRAPDU) as error:
        client.start_signing("m/44'/1'/0'/0'")
    assert error.value.status == Errors.SW_GET_PUB_KEY_FAIL
    assert error.value.data == b""
