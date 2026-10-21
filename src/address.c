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

#include "address.h"

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // uint*_t
#include <string.h>   // memmove

#include "cx.h"
#include "os.h"
#include "transaction/types.h"

bool address_from_pubkey(const uint8_t public_key[static 32], uint8_t* out,
                         size_t out_len) {
    const uint8_t signature_scheme_id = 0x00;
    uint8_t address[32] = {0};

    if (out_len < ADDRESS_LEN) {
        return false;
    }

    cx_sha3_t sha3;
    cx_err_t error = cx_sha3_init_no_throw(&sha3, 256);
    if (error != CX_OK) {
        return false;
    }
    error = cx_hash_update((cx_hash_t*)&sha3, public_key, 32);
    if (error != CX_OK) {
        return false;
    }
    error = cx_hash_update((cx_hash_t*)&sha3, &signature_scheme_id, 1);
    if (error != CX_OK) {
        return false;
    }
    error = cx_hash_final((cx_hash_t*)&sha3, address);
    if (error != CX_OK) {
        return false;
    }

    memmove(out, address, ADDRESS_LEN);

    return true;
}

bool validate_aptos_bip32_path(const uint32_t* path, size_t path_len) {
    // m/purpose'/coin_type'/account'/change/address_index
    // m/44'     /637'      /0'      /0'    /0'
    const uint32_t aptos_prefix[2] = {0x8000002C, 0x8000027D};

    // A 3-element HD path limit (`m/44'/637'/x'`) is enforced for:
    //  1. BIP44 compliance, ensuring proper structure (`m/44'/637'` - purpose
    //  and coin type).
    //  2. Operational flexibility, allowing basic account-level (`x'`) fund
    //  segregation.
    //  3. A balance between security and usability, preventing the potential
    //  privacy and security
    //     risks associated with using an overly simple path (e.g.,
    //     `m/44'/637'`), while not requiring the full 5-element path that may
    //     be unnecessary for users seeking straightforward wallet
    //     functionality.
    if (path_len < 3) {
        return false;
    }

    return memcmp(path, aptos_prefix, sizeof(aptos_prefix)) == 0;
}
