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

#ifdef HAVE_SWAP
#include <ctype.h>
#include <string.h>

#include "../address.h"
#include "../common/user_format.h"
#include "../handler/get_public_key.h"
#include "../transaction/utils.h"
#include "os.h"
#include "swap.h"

// The address string length is 66, 2 characters for the prefix and 64 for the
// address
#define ADDRESS_STRING_LENGTH 66

/**
 * Handler for CHECK_ADDRESS command. If successfully parse BIP32 path,
 * derive public key and address, and compare it with the address to check.
 *
 *
 * @param[in,out] params
 *   Command data address parameters and address to check.
 *
 */
void swap_handle_check_address(check_address_parameters_t* params) {
    PRINTF("Inside Aptos swap_handle_check_address\n");
    params->result = 0;

    // Checking that parameters are correct
    if (params->address_parameters == NULL ||
        params->address_parameters_length == 0) {
        PRINTF("address_parameters is empty\n");
        return;
    }
    PRINTF("address_parameters_length: %d\n",
           params->address_parameters_length);
    PRINTF("address_parameters: %.*H\n", params->address_parameters_length,
           params->address_parameters);

    if (params->address_to_check == NULL) {
        PRINTF("address_to_check is empty\n");
        return;
    }

    if (strlen(params->address_to_check) != ADDRESS_STRING_LENGTH) {
        PRINTF("address_to_check length should be %d, not %d\n",
               ADDRESS_STRING_LENGTH, strlen(params->address_to_check));
        return;
    }

    // Calculate the public key from the BIP32 path
    buffer_t cdata;
    cdata.ptr = params->address_parameters;
    cdata.size = params->address_parameters_length;
    cdata.offset = 0;

    uint8_t bip32_path_len;
    uint32_t bip32_path[MAX_BIP32_PATH];
    pubkey_ctx_t public_key;
    if (get_public_key(&cdata, &bip32_path_len, bip32_path, &public_key) != 0) {
        PRINTF("get_public_key failed\n");
        return;
    }
    // Calculate the address from the public key, and decode it to readable
    // format
    uint8_t address[ADDRESS_LEN] = {0};
    if (!address_from_pubkey(public_key.raw_public_key, address,
                             sizeof(address))) {
        return;
    }
    char prefixed_address[ADDRESS_STRING_LENGTH + 1];
    if (0 > format_prefixed_hex(address, sizeof(address), prefixed_address,
                                sizeof(prefixed_address))) {
        return;
    }

    PRINTF("address_to_check: %s\n", params->address_to_check);
    PRINTF("prefixed_address: %s\n", prefixed_address);
    // Compare the strings
    if (_strcasecmp(params->address_to_check, prefixed_address) != 0) {
        PRINTF("addresses does not match\n");
    } else {
        PRINTF("addresses match\n");
        params->result = 1;
    }
}
#endif
