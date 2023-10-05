#pragma once

#include <stdbool.h>  // bool
#include "ux.h"

/**
 * Callback to reuse action with approve/reject in step FLOW.
 */
typedef void (*action_validate_cb)(bool);

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
