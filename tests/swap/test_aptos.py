"""Exchange library tests for Aptos transfers and strict swap validation."""

import pytest
from application_client.boilerplate_command_sender import (
    BoilerplateCommandSender,
    Errors,
)
from application_client.boilerplate_transaction import Transaction
from ledger_app_clients.exchange.test_runner import (
    ALL_TESTS_EXCEPT_MEMO_THORSWAP_AND_FEES,
    ExchangeTestRunner,
)

from . import cal_helper as cal

OWNED = "0x8f13f355f3af444bd356adeaaaf01235a7817d6a4417f5c9fa3d74a68f7b7afd"
DESTINATION_1 = "0x3835075df1bf469c336eabed8ac87052ee4485f3ec93380a5382fbf76b7a3307"
DESTINATION_2 = "0x7be51d04d3a482fa056bc094bc5eadad005aaf823a95269410f08730f0d03cb4"


class AptosTests(ExchangeTestRunner):
    currency_configuration = cal.APTOS_CURRENCY_CONFIGURATION
    valid_destination_1 = DESTINATION_1
    valid_destination_memo_1 = ""
    valid_destination_2 = DESTINATION_2
    valid_destination_memo_2 = ""
    valid_refund = OWNED
    valid_refund_memo = ""

    valid_send_amount_1 = 1000
    valid_send_amount_2 = 666
    valid_fees_1 = 0
    valid_fees_2 = 0

    fake_refund = "abcdabcd"
    fake_refund_memo = "bla"
    fake_payout = "abcdabcd"
    fake_payout_memo = "bla"
    signature_refusal_error_code = Errors.SW_DENY
    wrong_amount_error_code = Errors.SW_SWAP_CHECKING_FAIL
    wrong_destination_error_code = Errors.SW_SWAP_CHECKING_FAIL

    def perform_final_tx(self, destination, send_amount, fees, memo):
        tx = Transaction(
            nonce=1,
            coin="APT",
            to=destination.removeprefix("0x"),
            value=send_amount,
            memo=memo,
        ).serialize()
        BoilerplateCommandSender(self.backend).sign_tx_sync(path=cal.APTOS_PATH, transaction=tx)


class TestAptosExchange:
    @pytest.mark.parametrize("test_to_run", ALL_TESTS_EXCEPT_MEMO_THORSWAP_AND_FEES)
    def test_aptos_exchange(self, backend, exchange_navigation_helper, test_to_run):
        AptosTests(backend, exchange_navigation_helper).run_test(test_to_run)
