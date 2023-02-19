#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

extern "C" {
#include "bcs/init.h"
#include "common/buffer.h"
#include "common/format.h"
#include "transaction/deserialize.h"
#include "transaction/utils.h"
#include "transaction/types.h"
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    buffer_t buf = {.ptr = data, .size = size, .offset = 0};
    transaction_t tx;
    parser_status_e status;
    char sender[65] = {0};

    transaction_init(&tx);
    status = transaction_deserialize(&buf, &tx);

    if (status == PARSING_OK && tx.tx_variant == TX_RAW &&
        tx.payload_variant == PAYLOAD_ENTRY_FUNCTION) {
        printf("\nTransaction size: %lu\n", size);
        printf("chain_id: %d\n", tx.chain_id);
        printf("sequence: %lu\n", tx.sequence);
        printf("gas_unit_price: %lu\n", tx.gas_unit_price);
        printf("max_gas_amount: %lu\n", tx.max_gas_amount);
        printf("expiration_timestamp_secs: %lu\n", tx.expiration_timestamp_secs);
        format_hex(tx.sender, ADDRESS_LEN, sender, sizeof(sender));
        printf("sender: %s\n", sender);
        printf("payload_variant: %d\n", tx.payload_variant);
        printf("entry_function.known_type: %d\n", tx.payload.entry_function.known_type);
    }

    return 0;
}
