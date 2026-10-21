#pragma once

#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <stdint.h>   // uint*_t

#include "buffer.h"

#include "../types.h"

/**
 * Handler for GET_PUBLIC_KEY command. If successfully parse BIP32 path,
 * derive public key/chain code and send APDU response.
 *
 * @see G_context.bip32_path, G_context.pk_info.raw_public_key and
 *      G_context.pk_info.chain_code.
 *
 * @param[in,out] cdata
 *   Command data with BIP32 path.
 * @param[in]     display
 *   Whether to display address on screen or not.
 *
 * @return zero or positive integer if success, negative integer otherwise.
 *
 */
int handler_get_public_key(buffer_t *cdata, bool display);

/**
 * Helper function for GET_PUBLIC_KEY and CHECK_ADDRESS command. If successfully parse
 * BIP32 path, derive public key/chain. The public key is stored in output_public_key,
 * and the BIP32 path and its length are stored in output_bip32_path and
 * output_bip32_path_len respectively.
 *
 * @param[in] cdata
 *   Command data with BIP32 path.
 * @param[out]     output_bip32_path_len
 *   Integer pointer to store the length of the BIP32 path.
 * @param[out]     output_bip32_path
 *   Buffer to store the BIP32 path.
 * @param[out]     output_public_key
 *   Buffer to store the public key.
 *
 * @return zero if success, error code otherwise.
 *
 */
int get_public_key(buffer_t *cdata,
                   uint8_t *output_bip32_path_len,
                   uint32_t output_bip32_path[MAX_BIP32_PATH],
                   pubkey_ctx_t *output_public_key);
