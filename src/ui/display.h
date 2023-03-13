#pragma once

#include <stdbool.h>  // bool
#include "ux.h"

void ui_flow_display(const ux_flow_step_t *const *steps);

void ui_flow_verified_display(const ux_flow_step_t *const *steps);

/**
 * Callback to reuse action with approve/reject in step FLOW.
 */
typedef void (*action_validate_cb)(bool);

/**
 * Display address on the device and ask confirmation to export.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_display_address(void);

/**
 * Display transaction information on the device and ask confirmation to sign.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_display_transaction(void);

int ui_display_message(void);

int ui_display_entry_function(void);

int ui_display_tx_aptos_account_transfer(void);

int ui_display_tx_coin_transfer(void);
