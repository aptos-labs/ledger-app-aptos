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

#include <stdint.h>   // uint*_t
#include <string.h>   // memset, explicit_bzero
#include <stdbool.h>  // bool

#include "crypto.h"

#include "globals.h"

cx_err_t crypto_derive_private_key(cx_ecfp_private_key_t *private_key,
                                   uint8_t chain_code[static 32],
                                   const uint32_t *bip32_path,
                                   uint8_t bip32_path_len) {
    uint8_t raw_private_key[64] = {0};
    cx_err_t error = CX_OK;

    // derive the seed with bip32_path
    error = os_derive_bip32_with_seed_no_throw(HDW_ED25519_SLIP10,
                                               CX_CURVE_Ed25519,
                                               bip32_path,
                                               bip32_path_len,
                                               raw_private_key,
                                               chain_code,
                                               (unsigned char *) "ed25519 seed",
                                               12);
    if (error != CX_OK) {
        explicit_bzero(&raw_private_key, sizeof(raw_private_key));
        return error;
    }

    // new private_key from raw
    error = cx_ecfp_init_private_key_no_throw(CX_CURVE_Ed25519, raw_private_key, 32, private_key);

    explicit_bzero(&raw_private_key, sizeof(raw_private_key));
    return error;
}

cx_err_t crypto_init_public_key(cx_ecfp_private_key_t *private_key,
                                cx_ecfp_public_key_t *public_key,
                                uint8_t raw_public_key[static 32]) {
    // generate corresponding public key
    cx_err_t error = cx_ecfp_generate_pair_no_throw(CX_CURVE_Ed25519, public_key, private_key, 1);

    if (error != CX_OK) {
        return error;
    }

    for (int i = 0; i < 32; i++) {
        raw_public_key[i] = public_key->W[64 - i];
    }
    if (public_key->W[32] & 1) {
        raw_public_key[31] |= 0x80;
    }

    return error;
}

cx_err_t crypto_sign_message() {
    cx_ecfp_private_key_t private_key = {0};
    uint8_t chain_code[32] = {0};

    // derive private key according to BIP32 path
    cx_err_t error = crypto_derive_private_key(&private_key,
                                               chain_code,
                                               G_context.bip32_path,
                                               G_context.bip32_path_len);
    if (error != CX_OK) {
        explicit_bzero(&private_key, sizeof(private_key));
        return error;
    }

    error = cx_eddsa_sign_no_throw(&private_key,
                                   CX_SHA512,
                                   G_context.tx_info.raw_tx,
                                   G_context.tx_info.raw_tx_len,
                                   G_context.tx_info.signature,
                                   sizeof(G_context.tx_info.signature));

    if (error != CX_OK) {
        explicit_bzero(&private_key, sizeof(private_key));
        return error;
    }

    size_t size;
    error = cx_ecdomain_parameters_length(private_key.curve, &size);
    if (error != CX_OK) {
        explicit_bzero(&private_key, sizeof(private_key));
        return error;
    }
    G_context.tx_info.signature_len = 2 * size;

    PRINTF("Signature: %.*H\n", G_context.tx_info.signature_len, G_context.tx_info.signature);

    explicit_bzero(&private_key, sizeof(private_key));
    return error;
}
