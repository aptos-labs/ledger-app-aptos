use alloc::vec::Vec;

use crate::AppSW;
use ledger_device_sdk::ecc::Ed25519;
use ledger_device_sdk::hash::{
    sha3::{Keccak256, Sha3_256},
    HashInit,
};

/// BIP32 derivation path stored as a vector of u32 components.
///
/// Aptos paths start at m/44'/637', include at least one additional component,
/// and have only hardened components: Ed25519 SLIP-10 cannot derive normal children.
#[derive(Default)]
pub struct Bip32Path(Vec<u32>);

impl AsRef<[u32]> for Bip32Path {
    fn as_ref(&self) -> &[u32] {
        &self.0
    }
}

impl TryFrom<&[u8]> for Bip32Path {
    type Error = AppSW;

    /// Constructs a [`Bip32Path`] from APDU-encoded bytes.
    ///
    /// # Format
    ///
    /// - First byte: Number of path components (3..=10)
    /// - Remaining bytes: Big-endian u32 components (4 bytes each)
    /// - First two components: m/44'/637'
    /// - All components hardened (Ed25519 SLIP-10)
    ///
    /// A mismatch in encoded length is 0x6A87; unsupported paths are 0xB006.
    fn try_from(data: &[u8]) -> Result<Self, Self::Error> {
        if data.is_empty() || data[0] as usize * 4 != data.len() - 1 {
            return Err(AppSW::WrongDataLength);
        }
        if !(3..=10).contains(&data[0]) {
            return Err(AppSW::GetPubKeyFail);
        }
        if data[1..9] != [0x80, 0, 0, 44, 0x80, 0, 2, 125] {
            return Err(AppSW::GetPubKeyFail);
        }
        if data[1..]
            .chunks_exact(4)
            .any(|component| component[0] & 0x80 == 0)
        {
            return Err(AppSW::GetPubKeyFail);
        }
        Ok(Bip32Path(
            data[1..]
                .chunks(4)
                .map(|chunk| u32::from_be_bytes(chunk.try_into().unwrap()))
                .collect(),
        ))
    }
}

/// Derive the Aptos Ed25519 public key and chain code using SLIP-10.
/// The SDK clears the derived private key and temporary seed on drop.
pub fn derive_aptos_public_key(path: &[u32]) -> Result<([u8; 32], [u8; 32]), AppSW> {
    let (private_key, chain_code) = Ed25519::derive_from_path_slip10_with_chaincode(path);
    let public_key = private_key.public_key().map_err(|_| AppSW::GetPubKeyFail)?;

    // Ledger represents Ed25519 public keys as uncompressed (X, Y). v1 transmits
    // the compressed Y coordinate in little-endian, with X parity in its top bit.
    let mut raw = [0u8; 32];
    for (i, byte) in raw.iter_mut().enumerate() {
        *byte = public_key.pubkey[64 - i];
    }
    if public_key.pubkey[32] & 1 != 0 {
        raw[31] |= 0x80;
    }
    Ok((raw, chain_code.value))
}

/// Aptos account address: SHA3-256(compressed Ed25519 public key || scheme 0).
pub fn aptos_address_from_pubkey(pubkey: &[u8; 32]) -> Result<[u8; 32], AppSW> {
    let mut sha3 = Sha3_256::new();
    let mut address = [0u8; 32];
    sha3.update(pubkey).map_err(|_| AppSW::GetPubKeyFail)?;
    sha3.update(&[0]).map_err(|_| AppSW::GetPubKeyFail)?;
    sha3.finalize(&mut address)
        .map_err(|_| AppSW::GetPubKeyFail)?;
    Ok(address)
}

/// Compute Keccak256 hash of a public key for address derivation.
///
/// This is used for Ethereum-style address computation:
/// 1. Take uncompressed pubkey (65 bytes)
/// 2. Skip first byte (0x04 marker)
/// 3. Hash the remaining 64 bytes with Keccak256
/// 4. Take last 20 bytes as address
///
/// # Used by
///
/// - `handler_get_public_key`: For displaying address to user
/// - `swap::check_address`: For verifying address ownership
///
/// # Arguments
///
/// * `pubkey` - 65-byte uncompressed secp256k1 public key
///
/// # Returns
///
/// 32-byte Keccak256 hash (last 20 bytes are the Ethereum address)
pub fn get_address_hash_from_pubkey(pubkey: &[u8; 65]) -> [u8; 32] {
    let mut keccak256 = Keccak256::new();
    let mut address: [u8; 32] = [0u8; 32];
    // Hash pubkey excluding first byte (0x04 uncompressed marker)
    let _ = keccak256.hash(&pubkey[1..], &mut address);
    address
}
