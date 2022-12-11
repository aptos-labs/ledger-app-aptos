#pragma once

#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t

#include "../bcs/types.h"

#define MAX_TX_LEN 510

typedef enum { PARSING_OK = 1, WRONG_LENGTH_ERROR = -2000 } parser_status_e;

typedef aptos_transaction_t transaction_t;
