/*****************************************************************************
 *   Ledger App Aptos.
 *   (c) 2020 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#include <stdbool.h>  // bool
#include <string.h>   // memset

#include "../address.h"
#include "../common/user_format.h"
#include "../globals.h"
#include "../sw.h"
#include "../transaction/types.h"
#include "../transaction/utils.h"
#include "action/validate.h"
#include "bip32.h"
#include "constants.h"
#include "display.h"
#include "format.h"
#include "glyphs.h"
#include "io.h"
#include "os.h"
#include "settings.h"
#include "ux.h"

char g_bip32_path[60];
char g_tx_type[60];
char g_address[67];
char g_gas_fee[30];
char g_struct[120];
char g_function[120];
char g_amount[30];
int g_is_token_listed;

#define MAX_COIN_TYPE_LEN 110
#define MAX_TOKEN_LEN 30
#define MAX_TICKER_LENG 16

// Mapping to facilitate identification of Tickers and Token Names via the Coin
// Type id
typedef struct token_info {
    const char ticker[MAX_TICKER_LENG];
    uint8_t decimals;
    const char coin_type[MAX_COIN_TYPE_LEN];
    const char token[MAX_TOKEN_LEN];
} token_info_t;

// decimals mirror each token's on-chain metadata (the coin/fungible-asset
// `decimals`); keep in sync with the registry when the token list changes.
static const token_info_t TOKEN_MAPPING[] = {
    {.ticker = "amAPT",
     .decimals = 8,
     .token = "Amnis Aptos Coin",
     .coin_type = "0x111ae3e5bc816a5e63c2da97d0aa3886519e0cd5e4b046659fa35796bd"
                  "11542a::amapt_token:"
                  ":AmnisApt"},
    {.ticker = "APARTMENT",
     .decimals = 8,
     .token = "Apartment",
     .coin_type = "0x7b7bab2131de3e4f318b4abaa952f7c817b2c3df16c951caca809ac9ca"
                  "9b650e::APARTMENT::"
                  "APARTMENT"},
    {.ticker = "APE",
     .decimals = 8,
     .token = "APETOS",
     .coin_type = "0xada35ada7e43e2ee1c39633ffccec38b76ce702b4efc2e60b50f63fbe4"
                  "f710d8::apetos_"
                  "token::ApetosCoin"},
    {.ticker = "APT",
     .decimals = 8,
     .token = "Aptos Coin",
     .coin_type = "0x1::aptos_coin::AptosCoin"},
    {.ticker = "FOMO",
     .decimals = 6,
     .token = "APTOS FOMO",
     .coin_type = "0xf891d2e004973430cc2bbbee69f3d0f4adb9c7ae03137b4579f7bb9979"
                  "283ee6::APTOS_FOMO::"
                  "APTOS_FOMO"},
    {.ticker = "ALT",
     .decimals = 8,
     .token = "Aptos Launch Token",
     .coin_type = "0xd0b4efb4be7c3508d9a26a9b5405cf9f860d0b9e5fe2f498b90e68b8d2"
                  "cedd3e::aptos_launch_token::"
                  "AptosLaunchToken"},
    {.ticker = "BLT",
     .decimals = 8,
     .token = "Blocto Token",
     .coin_type = "0xfbab9fb68bd2103925317b6a540baa20087b1e7a7a4eb90badee04abb6"
                  "b5a16f::blt::Blt"},
    {.ticker = "MOVE",
     .decimals = 8,
     .token = "BlueMove",
     .coin_type = "0x27fafcc4e39daac97556af8a803dbb52bcb03f0821898dc845ac54225b"
                  "9793eb::move_coin::MoveCoin"},
    {.ticker = "BUBBLES",
     .decimals = 8,
     .token = "BUBBLES",
     .coin_type = "0xd6a49762f6e4f7401ee79be6f5d4111e70db1408966ba1aa204e6e10c9"
                  "d437ca::bubbles::"
                  "BubblesCoin"},
    {.ticker = "doodoo",
     .decimals = 8,
     .token = "DooDoo",
     .coin_type = "0x73eb84966be67e4697fc5ae75173ca6c35089e802650f75422ab49a872"
                  "9704ec::coin::DooDoo"},
    {.ticker = "dstAPT",
     .decimals = 8,
     .token = "dstAPT",
     .coin_type = "0xd11107bdf0d6d7040c6c0bfbdecb6545191fdf13e8d8d259952f53e171"
                  "3f61b5::staked_coin:"
                  ":StakedAptos"},
    {.ticker = "whGARI",
     .decimals = 8,
     .token = "Gari (Wormhole)",
     .coin_type = "0x4def3d3dee27308886f0a3611dd161ce34f977a9a5de4e80b237225923"
                  "492a2a::coin::T"},
    {.ticker = "GUI",
     .decimals = 6,
     .token = "Gui Inu",
     .coin_type = "0xe4ccb6d39136469f376242c31b34d10515c8eaaa38092f804db8e08a8f"
                  "53c5b2::assets_v1::"
                  "EchoCoin002"},
    {.ticker = "HEART",
     .decimals = 8,
     .token = "HEART",
     .coin_type = "0x7de3fea83cd5ca0e1def27c3f3803af619882db51f34abf30dd04ad12e"
                  "e6af31::tapos::Heart"},
    {.ticker = "LSD",
     .decimals = 8,
     .token = "Liquidswap",
     .coin_type = "0x53a30a6e5936c0a4c5140daed34de39d17ca7fcae08f947c02e979cef9"
                  "8a3719::coin::LSD"},
    {.ticker = "MOJO",
     .decimals = 8,
     .token = "Mojito",
     .coin_type = "0x881ac202b1f1e6ad4efcff7a1d0579411533f2502417a19211cfc49751"
                  "ddb5f4::coin::MOJO"},
    {.ticker = "MOOMOO",
     .decimals = 6,
     .token = "MOO MOO",
     .coin_type = "0xc5fbbcc4637aeebb4e732767abee8a21f2b0776f73b73e16ce13e7d31d"
                  "6700da::MOOMOO::MOOMOO"},
    {.ticker = "MOD",
     .decimals = 8,
     .token = "Move Dollar",
     .coin_type = "0x6f986d146e4a90b828d8c12c14b6f4e003fdff11a8eecceceb63744363"
                  "eaac01::mod_coin::MOD"},
    {.ticker = "Cake",
     .decimals = 8,
     .token = "PancakeSwap Token",
     .coin_type = "0x159df6b7689437016108a019fd5bef736bac692b6d4a1f10c941f6fbb9"
                  "a74ca6::oft::CakeOFT"},
    {.ticker = "RETuRD",
     .decimals = 8,
     .token = "Returd",
     .coin_type = "0xdf3d5eb83df80dfde8ceb1edaa24d8dbc46da6a89ae134a858338e1b86"
                  "a29e38::coin::Returd"},
    {.ticker = "SHRIMP",
     .decimals = 2,
     .token = "SHRIMP",
     .coin_type = "0x55987edfab9a57f69bac759674f139ae473b5e09a9283848c1f87faf6f"
                  "c1e789::shrimp::ShrimpCoin"},
    {.ticker = "whSOL",
     .decimals = 8,
     .token = "Solana (Wormhole)",
     .coin_type = "0xdd89c0e695df0692205912fb69fc290418bed0dbe6e4573d744a6d5e6b"
                  "ab6c13::coin::T"},
    {.ticker = "stAPT",
     .decimals = 8,
     .token = "Staked Aptos Coin",
     .coin_type = "0x111ae3e5bc816a5e63c2da97d0aa3886519e0cd5e4b046659fa35796bd"
                  "11542a::stapt_token:"
                  ":StakedApt"},
    {.ticker = "sthAPT",
     .decimals = 8,
     .token = "Staked Thala APT",
     .coin_type = "0xfaf4e633ae9eb31366c9ca24214231760926576c7b625313b3688b5e90"
                  "0731f6::staking::"
                  "StakedThalaAPT"},
    {.ticker = "ceUSDT",
     .decimals = 6,
     .token = "Tether USD (Celer)",
     .coin_type = "0x8d87a65ba30e09357fa2edea2c80dbac296e5dec2b18287113500b9029"
                  "42929d::celer_coin_"
                  "manager::UsdtCoin"},
    {.ticker = "lzUSDT",
     .decimals = 6,
     .token = "Tether USD (LayerZero)",
     .coin_type = "0xf22bede237a07e121b56d91a491eb7bcdfd1f5907926a9e58338f964a0"
                  "1b17fa::asset::USDT"},
    {.ticker = "whUSDT",
     .decimals = 6,
     .token = "Tether USD (Wormhole)",
     .coin_type = "0xa2eda21a58856fda86451436513b867c97eecb4ba099da5775520e0f74"
                  "92e852::coin::T"},
    {.ticker = "thAPT",
     .decimals = 8,
     .token = "Thala APT",
     .coin_type = "0xfaf4e633ae9eb31366c9ca24214231760926576c7b625313b3688b5e90"
                  "0731f6::staking::ThalaAPT"},
    {.ticker = "THL",
     .decimals = 8,
     .token = "Thala Token",
     .coin_type = "0x7fd500c11216f0fe3095d0c4b8aa4d64a4e2e04f83758462f2b1272556"
                  "43615::thl_coin::THL"},
    {.ticker = "LOON",
     .decimals = 6,
     .token = "The Loonies",
     .coin_type = "0x268d4a7a2ad93274edf6116f9f20ad8455223a7ab5fc73154f687e7dbc"
                  "3e3ec6::LOON::LOON"},
    {.ticker = "TIN",
     .decimals = 8,
     .token = "Token \"IN\"",
     .coin_type = "0xc32ba5d293577cbb1df390f35b2bc6369a593b736d0865fedec1a2b085"
                  "65de8e::in_coin::InCoin"},
    {.ticker = "TOMA",
     .decimals = 6,
     .token = "Tomarket",
     .coin_type = "0x9d0595765a31f8d56e1d2aafc4d6c76f283c67a074ef8812d8c31bd825"
                  "2ac2c3::asset::TOMA"},
    {.ticker = "tAPT",
     .decimals = 8,
     .token = "Tortuga Staked APT",
     .coin_type = "0x84d7aeef42d38a5ffc3ccef853e1b82e4958659d16a7de736a29c55fbb"
                  "eb0114::staked_aptos_coin::"
                  "StakedAptosCoin"},
    {.ticker = "UPTOS",
     .decimals = 8,
     .token = "UPTOS",
     .coin_type = "0x4fbed3f8a3fd8a11081c8b6392152a8b0cb14d70d0414586f0c9b858fc"
                  "d2d6a7::UPTOS::UPTOS"},
    {.ticker = "lzUSDC",
     .decimals = 6,
     .token = "USD Coin (LayerZero)",
     .coin_type = "0xf22bede237a07e121b56d91a491eb7bcdfd1f5907926a9e58338f964a0"
                  "1b17fa::asset::USDC"},
    {.ticker = "whUSDC",
     .decimals = 6,
     .token = "USD Coin (Wormhole)",
     .coin_type = "0x5e156f1207d0ebfa19a9eeff00d62a282278fb8719f4fab3a586a0a2c0"
                  "fffbea::coin::T"},
    {.ticker = "ceWBNB",
     .decimals = 8,
     .token = "Wrapped BNB (Celer)",
     .coin_type = "0x8d87a65ba30e09357fa2edea2c80dbac296e5dec2b18287113500b9029"
                  "42929d::celer_coin_"
                  "manager::BnbCoin"},
    {.ticker = "lzWETH",
     .decimals = 6,
     .token = "Wrapped Ether (LayerZero)",
     .coin_type = "0xf22bede237a07e121b56d91a491eb7bcdfd1f5907926a9e58338f964a0"
                  "1b17fa::asset::WETH"},
    {.ticker = "whWETH",
     .decimals = 8,
     .token = "Wrapped Ether (Wormhole)",
     .coin_type = "0xcc8a89c8dce9693d354449f1f73e60e14e347417854f029db5bc8e7454"
                  "008abb::coin::T"},
    {.ticker = "ANI",
     .decimals = 8,
     .token = "AnimeSwap Coin",
     .coin_type = "0x16fe2df00ea7dde4a63409201f7f4e536bde7bb7335526a35d05111e68"
                  "aa322c::AnimeCoin::ANI"},
    {.ticker = "AMA",
     .decimals = 8,
     .token = "Amaterasu",
     .coin_type =
         "0xd0ab8c2f76cd640455db56ca758a9766a966c88f77920347aac1719edab1df5e"},
    {.ticker = "CELL",
     .decimals = 8,
     .token = "CELLANA",
     .coin_type =
         "0x2ebb2ccac5e027a87fa0e2e5f656a3a4238d6a48d93ec9b610d570fc0aa0df12"},
    {.ticker = "MKL",
     .decimals = 6,
     .token = "MKL",
     .coin_type =
         "0x878370592f9129e14b76558689a4b570ad22678111df775befbfcbc9fb3d90ab"},
    {.ticker = "USDT",
     .decimals = 6,
     .token = "Tether USD",
     .coin_type =
         "0x357b0b74bc833e95a115ad22604854d6b0fca151cecd94111770e5d6ffc9dc2b"},
    {.ticker = "TruAPT",
     .decimals = 8,
     .token = "TruAPT coin",
     .coin_type =
         "0xaef6a8c3182e076db72d64324617114cacf9a52f28325edc10b483f7f05da0e7"},
    {.ticker = "USDC",
     .decimals = 6,
     .token = "USDC",
     .coin_type =
         "0xbae207659db88bea0cbead6da0ed00aac12edcdda169e591cd41c94180b46f3b"}};

static size_t count_leading_zeros(const uint8_t* src, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (src[i] != 0) {
            return i;
        }
    }
    return len;
}

int ui_prepare_address() {
    if (G_context.req_type != CONFIRM_ADDRESS) {
        return io_send_sw(SW_BAD_STATE);
    }

    memset(g_bip32_path, 0, sizeof(g_bip32_path));
    if (!bip32_path_format(G_context.bip32_path, G_context.bip32_path_len,
                           g_bip32_path, sizeof(g_bip32_path))) {
        return io_send_sw(SW_DISPLAY_BIP32_PATH_FAIL);
    }

    memset(g_address, 0, sizeof(g_address));
    uint8_t address[ADDRESS_LEN] = {0};
    if (!address_from_pubkey(G_context.pk_info.raw_public_key, address,
                             sizeof(address))) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }
    if (0 > format_prefixed_hex(address, sizeof(address), g_address,
                                sizeof(g_address))) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }

    return UI_PREPARED;
}

int ui_prepare_transaction() {
    if (G_context.req_type != CONFIRM_TRANSACTION ||
        G_context.state != STATE_PARSED) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }

    transaction_t* transaction = &G_context.tx_info.transaction;

    if (transaction->tx_variant == TX_MESSAGE) {
        return ui_display_message();
    } else if (transaction->tx_variant == TX_RAW_MESSAGE) {
        return ui_display_raw_message();
    } else if (transaction->tx_variant != TX_UNDEFINED) {
        uint64_t gas_fee_value =
            transaction->gas_unit_price * transaction->max_gas_amount;
        memset(g_gas_fee, 0, sizeof(g_gas_fee));
        char gas_fee[30] = {0};
        if (!format_fpu64(gas_fee, sizeof(gas_fee), gas_fee_value, 8)) {
            return io_send_sw(SW_DISPLAY_GAS_FEE_FAIL);
        }
        snprintf(g_gas_fee, sizeof(g_gas_fee), "APT %.*s", sizeof(gas_fee),
                 gas_fee);
        PRINTF("Gas Fee: %s\n", g_gas_fee);

        if (transaction->tx_variant == TX_RAW) {
            switch (transaction->payload_variant) {
                case PAYLOAD_ENTRY_FUNCTION:
                    return ui_display_entry_function();
                case PAYLOAD_SCRIPT:
                    memset(g_tx_type, 0, sizeof(g_tx_type));
                    snprintf(g_tx_type, sizeof(g_tx_type),
                             "%s [payload = SCRIPT]", RAW_TRANSACTION_SALT);
                    break;
                case PAYLOAD_MULTISIG:
                    memset(g_tx_type, 0, sizeof(g_tx_type));
                    snprintf(g_tx_type, sizeof(g_tx_type),
                             "%s [payload = MULTISIG]", RAW_TRANSACTION_SALT);
                    break;
                default:
                    memset(g_tx_type, 0, sizeof(g_tx_type));
                    snprintf(g_tx_type, sizeof(g_tx_type),
                             "%s [payload = UNKNOWN]", RAW_TRANSACTION_SALT);
                    break;
            }
        } else if (transaction->tx_variant == TX_RAW_WITH_DATA) {
            memset(g_tx_type, 0, sizeof(g_tx_type));
            snprintf(g_tx_type, sizeof(g_tx_type),
                     RAW_TRANSACTION_WITH_DATA_SALT);
        }
    } else {
        memset(g_tx_type, 0, sizeof(g_tx_type));
        snprintf(g_tx_type, sizeof(g_tx_type), "unknown data type");
    }

    return UI_PREPARED;
}

static int is_coin_type_aptos(type_tag_struct_t* coin_type) {
    return (memcmp(coin_type->name.bytes, "AptosCoin", coin_type->name.len) ==
                0 &&
            memcmp(coin_type->module_name.bytes, "aptos_coin",
                   coin_type->module_name.len) == 0);
}

int ui_prepare_entry_function() {
    entry_function_payload_t* function =
        &G_context.tx_info.transaction.payload.entry_function;
    char function_module_id_address_hex[67] = {0};

    // Be sure to display at least 1 byte, even if it is zero
    size_t leading_zeros =
        count_leading_zeros(function->module_id.address, ADDRESS_LEN - 1);
    if (0 > format_prefixed_hex(function->module_id.address + leading_zeros,
                                ADDRESS_LEN - leading_zeros,
                                function_module_id_address_hex,
                                sizeof(function_module_id_address_hex))) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }
    memset(g_function, 0, sizeof(g_function));
    snprintf(g_function, sizeof(g_function), "%s::%.*s::%.*s",
             function_module_id_address_hex, function->module_id.name.len,
             function->module_id.name.bytes, function->function_name.len,
             function->function_name.bytes);
    PRINTF("Function: %s\n", g_function);

    switch (function->known_type) {
        case FUNC_APTOS_ACCOUNT_TRANSFER:
            return ui_display_tx_aptos_account_transfer();
        case FUNC_COIN_TRANSFER:
        case FUNC_APTOS_ACCOUNT_TRANSFER_COINS:
            return ui_display_tx_coin_transfer();
        case FUNC_FUNGIBLE_STORE_TRANSFER:
            return ui_display_tx_fungible_asset_transfer();
        case FUNC_ADD_STAKE:
        case FUNC_UNLOCK_STAKE:
        case FUNC_REACTIVATE_STAKE:
        case FUNC_WITHDRAW_STAKE:
            return ui_display_delegation_pool_transfer(function->known_type);
        default:
            memset(g_tx_type, 0, sizeof(g_tx_type));
            snprintf(g_tx_type, sizeof(g_tx_type), "Function call");
            PRINTF("Tx Type: %s\n", g_tx_type);
            break;
    }

    return UI_PREPARED;
}

int ui_prepare_tx_aptos_account_transfer() {
    args_aptos_account_transfer_t* transfer =
        &G_context.tx_info.transaction.payload.entry_function.args.transfer;

    // For well-known functions, display the transaction type in human-readable
    // format
    memset(g_tx_type, 0, sizeof(g_tx_type));
    snprintf(g_tx_type, sizeof(g_tx_type), "APT transfer");
    PRINTF("Tx Type: %s\n", g_tx_type);

    memset(g_address, 0, sizeof(g_address));
    if (0 > format_prefixed_hex(transfer->receiver, ADDRESS_LEN, g_address,
                                sizeof(g_address))) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }
    PRINTF("Receiver: %s\n", g_address);

    memset(g_amount, 0, sizeof(g_amount));
    char amount[30] = {0};
    if (!format_fpu64(amount, sizeof(amount), transfer->amount, 8)) {
        return io_send_sw(SW_DISPLAY_AMOUNT_FAIL);
    }
    snprintf(g_amount, sizeof(g_amount), "APT %.*s", sizeof(amount), amount);
    PRINTF("Amount: %s\n", g_amount);

    return UI_PREPARED;
}

/**
 * @brief Get token info by coin type, to display human-readable token info
 * (ticker, token name)
 *
 * @param[in] coin_type Coin type
 *
 * @return Token info
 */
static const token_info_t* get_token_info(const char* coin_type) {
    for (size_t i = 0; i < sizeof(TOKEN_MAPPING) / sizeof(TOKEN_MAPPING[0]);
         i++) {
        if (_strcasecmp(coin_type, TOKEN_MAPPING[i].coin_type) == 0) {
            return &TOKEN_MAPPING[i];
        }
    }
    return NULL;
}

int ui_prepare_tx_coin_transfer() {
    args_coin_transfer_t* transfer = &G_context.tx_info.transaction.payload
                                          .entry_function.args.coin_transfer;
    char transfer_ty_coin_address_hex[67] = {0};

    // For well-known functions, display the transaction type in human-readable
    // format
    memset(g_tx_type, 0, sizeof(g_tx_type));
    snprintf(g_tx_type, sizeof(g_tx_type), "Coin transfer");
    PRINTF("Tx Type: %s\n", g_tx_type);

    // Be sure to display at least 1 byte, even if it is zero
    size_t leading_zeros =
        count_leading_zeros(transfer->ty_coin.address, ADDRESS_LEN - 1);
    if (0 > format_prefixed_hex(transfer->ty_coin.address + leading_zeros,
                                ADDRESS_LEN - leading_zeros,
                                transfer_ty_coin_address_hex,
                                sizeof(transfer_ty_coin_address_hex))) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }
    memset(g_struct, 0, sizeof(g_struct));

    // If the coin type is AptosCoin we ought specify snprintf, as the coin
    // address can have an arbitrary number of leading zeros
    if (is_coin_type_aptos(&transfer->ty_coin)) {
        snprintf(g_struct, sizeof(g_struct), "0x1::aptos_coin::AptosCoin");
    } else {
        snprintf(g_struct, sizeof(g_struct), "%s::%.*s::%.*s",
                 transfer_ty_coin_address_hex,
                 transfer->ty_coin.module_name.len,
                 transfer->ty_coin.module_name.bytes,
                 transfer->ty_coin.name.len, transfer->ty_coin.name.bytes);
    }
    PRINTF("Coin Type: %s\n", g_struct);

    memset(g_address, 0, sizeof(g_address));
    if (0 > format_prefixed_hex(transfer->receiver, ADDRESS_LEN, g_address,
                                sizeof(g_address))) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }
    PRINTF("Receiver: %s\n", g_address);

    memset(g_amount, 0, sizeof(g_amount));
    const token_info_t* info = get_token_info(g_struct);
    char amount[30] = {0};
    if (!format_fpu64(amount, sizeof(amount), transfer->amount,
                      info ? info->decimals : 8)) {
        return io_send_sw(SW_DISPLAY_AMOUNT_FAIL);
    }
    if (info) {
        snprintf(g_amount, sizeof(g_amount), "%s %.*s", info->ticker,
                 sizeof(amount), amount);
        g_is_token_listed = 1;
    } else {
        snprintf(g_amount, sizeof(g_amount), "%.*s", sizeof(amount), amount);
        g_is_token_listed = 0;
    }

    PRINTF("Amount: %s\n", g_amount);
    return UI_PREPARED;
}

int ui_prepare_tx_fungible_asset_transfer() {
    args_fungible_asset_transfer_t* transfer =
        &G_context.tx_info.transaction.payload.entry_function.args.fa_transfer;
    char transfer_fa_coin_address_hex[67] = {0};

    // For well-known functions, display the transaction type in human-readable
    // format
    memset(g_tx_type, 0, sizeof(g_tx_type));
    snprintf(g_tx_type, sizeof(g_tx_type), "Fungible asset transfer");
    PRINTF("Tx Type: %s\n", g_tx_type);

    // Be sure to display at least 1 byte, even if it is zero
    size_t leading_zeros =
        count_leading_zeros(transfer->fungible_asset.address, ADDRESS_LEN - 1);
    if (0 > format_prefixed_hex(
                transfer->fungible_asset.address + leading_zeros,
                ADDRESS_LEN - leading_zeros, transfer_fa_coin_address_hex,
                sizeof(transfer_fa_coin_address_hex))) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }
    memset(g_struct, 0, sizeof(g_struct));
    snprintf(g_struct, sizeof(g_struct), "%s", transfer_fa_coin_address_hex);
    PRINTF("Coin Type: %s\n", g_struct);

    memset(g_address, 0, sizeof(g_address));
    if (0 > format_prefixed_hex(transfer->receiver, ADDRESS_LEN, g_address,
                                sizeof(g_address))) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }
    PRINTF("Receiver: %s\n", g_address);

    const token_info_t* info = get_token_info(g_struct);
    char amount[30] = {0};
    if (!format_fpu64(amount, sizeof(amount), transfer->amount,
                      info ? info->decimals : 8)) {
        return io_send_sw(SW_DISPLAY_AMOUNT_FAIL);
    }
    if (info) {
        snprintf(g_amount, sizeof(g_amount), "%s %.*s", info->ticker,
                 sizeof(amount), amount);
        g_is_token_listed = 1;
    } else {
        snprintf(g_amount, sizeof(g_amount), "%.*s", sizeof(amount), amount);
        g_is_token_listed = 0;
    }
    PRINTF("Amount: %s\n", g_amount);

    return UI_PREPARED;
}

int ui_prepare_delegation_pool_transfer() {
    args_delegation_pool_transfer_t* delegation =
        &G_context.tx_info.transaction.payload.entry_function.args.delegation;

    // For well-known functions, display the transaction type in human-readable
    // format
    memset(g_tx_type, 0, sizeof(g_tx_type));
    snprintf(g_tx_type, sizeof(g_tx_type), "Delegation pool transfer");
    PRINTF("Tx Type: %s\n", g_tx_type);

    memset(g_address, 0, sizeof(g_address));
    if (0 > format_prefixed_hex(delegation->pool, ADDRESS_LEN, g_address,
                                sizeof(g_address))) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }
    PRINTF("Pool: %s\n", g_address);

    memset(g_amount, 0, sizeof(g_amount));
    char amount[30] = {0};
    if (!format_fpu64(amount, sizeof(amount), delegation->amount, 8)) {
        return io_send_sw(SW_DISPLAY_AMOUNT_FAIL);
    }

    snprintf(g_amount, sizeof(g_amount), "APT %.*s", sizeof(amount), amount);
    PRINTF("Amount: %s\n", g_amount);

    return UI_PREPARED;
}
