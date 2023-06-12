#pragma once

#include <stdint.h>

#include "ux.h"

#include "io.h"
#include "types.h"
#include "constants.h"

/**
 * Global context for user requests.
 */
extern global_ctx_t G_context;

/**
 * Global NVM app storage.
 */
extern const app_storage_t N_app_storage;
#define N_storage (*(volatile app_storage_t*) PIC(&N_app_storage))
