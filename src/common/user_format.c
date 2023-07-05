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

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // int*_t, uint*_t
#include <string.h>   // strncpy, memmove

#include "format.h"

#include "user_format.h"

int format_prefixed_hex(const uint8_t *in, size_t in_len, char *out, size_t out_len) {
    const char prefix[] = "0x";
    const size_t prefix_len = sizeof(prefix) - 1;

    if (out_len < sizeof(prefix)) {
        return -1;
    }
    strncpy(out, prefix, sizeof(prefix));
    return format_hex(in, in_len, out + prefix_len, out_len - prefix_len);
}

bool is_str_interrupted(const char *src, size_t len) {
    bool interrupted = false;
    for (size_t i = 0; i < len; i++) {
        if (!interrupted && src[i] == 0) {
            interrupted = true;
            continue;
        }
        if (interrupted && src[i] != 0) {
            return true;
        }
    }
    return false;
}
