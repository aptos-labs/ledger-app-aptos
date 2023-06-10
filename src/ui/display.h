#pragma once

#include <stdbool.h>  // bool
#include "ux.h"

/**
 * A convenient function for displaying the sequence of steps.
 */
void ui_flow_display(const ux_flow_step_t *const *steps);

/**
 * A function that allows you to check the blind signing permission before displaying the sequence
 * of steps.
 *
 * NOTE: This function should always use UX_FLOW containing the blind signing warning on the first
 * step!
 */
void ui_flow_verified_display(const ux_flow_step_t *const *steps);

/**
 * Callback to reuse action with approve/reject in step FLOW.
 */
typedef void (*action_validate_cb)(bool);

/**
 * Callback to extend the active UX_FLOW.
 */
typedef void (*action_extend_cb)(const ux_flow_step_t *const *steps);

/**
 * Context to extend the active UX_FLOW.
 */
typedef struct action_extend_ctx_s {
    action_extend_cb call;
    const ux_flow_step_t *const *steps;
} action_extend_ctx_t;

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
