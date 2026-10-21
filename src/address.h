#pragma once

#include <stdint.h>   // uint*_t
#include <stddef.h>   // size_t
#include <stdbool.h>  // bool

/**
 * Convert public key to address.
 *
 * address = Keccak256(public_key) (32 bytes)
 *
 * @param[in]  public_key
 *   Pointer to byte buffer with public key.
 *   The public key is represented as 32 bytes
 * @param[out] out
 *   Pointer to output byte buffer for address.
 * @param[in]  out_len
 *   Length of output byte buffer.
 *
 * @return true if success, false otherwise.
 *
 */
bool address_from_pubkey(const uint8_t public_key[static 32], uint8_t *out, size_t out_len);

/**
 * Validates BIP32 derivation path for Aptos.
 *
 * @param[in] path
 *   Pointer to an array of uint32_t representing the BIP32 path.
 * @param[in] path_len
 *   The number of elements in the `path` array. Must be at least 3 for the function to succeed.
 *
 * @return true if success, false otherwise.
 *
 */
bool validate_aptos_bip32_path(const uint32_t *path, size_t path_len);
