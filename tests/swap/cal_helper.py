"""Exchange CAL configuration for Aptos's native coin and SLIP-10 account."""

from ledger_app_clients.exchange.cal_helper import CurrencyConfiguration
from ragger.bip import pack_derivation_path
from ragger.utils import create_currency_config

APTOS_PATH = "m/44'/637'/0'"
APTOS_PACKED_DERIVATION_PATH = pack_derivation_path(APTOS_PATH)
APTOS_CONF = create_currency_config("APT", "Aptos")
APTOS_CURRENCY_CONFIGURATION = CurrencyConfiguration(
    ticker="APT", conf=APTOS_CONF, packed_derivation_path=APTOS_PACKED_DERIVATION_PATH
)
