#ifdef HAVE_SWAP

#include <string.h>  // memset, explicit_bzero

#include "common/parse.h"
#include "constants.h"
#include "swap.h"

#define MAX_TICKER_LEN 16

/* Set empty printable_amount on error, printable amount otherwise */
void swap_handle_get_printable_amount(
    get_printable_amount_parameters_t* params) {
    uint64_t amount;
    uint8_t decimals;
    char ticker[MAX_TICKER_LEN] = {0};

    PRINTF("Inside Aptos swap_handle_get_printable_amount \n");

    // If the amount is a fee, its value is nominated in APT
    // If there is no coin_configuration, consider that we are doing a APT swap
    if (params->is_fee || params->coin_configuration == NULL) {
        memcpy(ticker, "APT", sizeof("APT"));
        decimals = APT_DECIMAL_PRECISION;
    } else {
        if (!swap_parse_config(params->coin_configuration,
                               params->coin_configuration_length, ticker,
                               sizeof(ticker), &decimals)) {
            PRINTF("Fail to parse coin_configuration\n");
            goto error;
        }
    }

    if (!swap_str_to_u64(params->amount, params->amount_length, &amount)) {
        PRINTF("Amount is too big\n");
        goto error;
    }

    if (print_amount(amount, decimals, params->printable_amount,
                     sizeof(params->printable_amount)) == 0) {
        PRINTF("print_amount failed\n");
        goto error;
    }

    strlcat(params->printable_amount, " ", sizeof(params->printable_amount));
    strlcat(params->printable_amount, ticker, sizeof(params->printable_amount));

    PRINTF("Amount %s\n", params->printable_amount);
    return;

error:
    memset(params->printable_amount, '\0', sizeof(params->printable_amount));
}

#endif