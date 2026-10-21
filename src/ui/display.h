#pragma once

#define UI_PREPARED -10

extern char g_bip32_path[60];
extern char g_tx_type[60];
extern char g_address[67];
extern char g_gas_fee[30];
extern char g_struct[120];
extern char g_function[120];
extern char g_amount[30];
extern int g_is_token_listed;

#include "../types.h"

/**
 * Display address on the device and ask confirmation to export.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_display_address(void);
int ui_prepare_address(void);

/**
 * Display transaction information on the device and ask confirmation to sign.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_display_transaction(void);
int ui_prepare_transaction(void);

int ui_display_message(void);
int ui_display_raw_message(void);

int ui_display_entry_function(void);
int ui_prepare_entry_function(void);

int ui_display_tx_aptos_account_transfer(void);
int ui_prepare_tx_aptos_account_transfer(void);

int ui_display_tx_coin_transfer(void);
int ui_prepare_tx_coin_transfer(void);

int ui_display_tx_fungible_asset_transfer(void);
int ui_prepare_tx_fungible_asset_transfer(void);

int ui_display_delegation_pool_transfer(entry_function_known_type_t function_type);
int ui_prepare_delegation_pool_transfer(void);

#if defined(TARGET_STAX) || defined(TARGET_FLEX)
#define ICON_APP_HOME C_aptos_logo_64px
#elif defined(TARGET_APEX_P)
#define ICON_APP_HOME C_aptos_logo_48px
#endif