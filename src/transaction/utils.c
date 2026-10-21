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

#include <ctype.h>
#include <stdbool.h>  // bool
#include <stdint.h>   // uint*_t
#include <string.h>   // memcmp

#include "types.h"

bool transaction_utils_check_encoding(const uint8_t* msg, uint64_t msg_len) {
    for (uint64_t i = 0; i < msg_len; i++) {
        if (msg[i] > 0x7F) {
            return false;
        }
    }

    return true;
}

bool bcs_cmp_bytes(const fixed_bytes_t* bcs_bytes, const void* value,
                   size_t len) {
    return bcs_bytes->len == len && memcmp(bcs_bytes->bytes, value, len) == 0;
}

int _strcasecmp(const char* s1, const char* s2) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    int result = 0;
    if (p1 == p2) return 0;
    while ((result = toupper(*p1) - toupper(*p2++)) == 0)
        if (*p1++ == '\0') break;
    return result;
}
