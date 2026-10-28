/*****************************************************************************
 *   Ledger App Aptos.
 *   (c) 2023 Ledger SAS.
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

//! Aptos Exchange library integration. Address and amount operations run
//! before the SDK resets BSS, so they use only stack storage.

use arrayvec::ArrayString;
use core::fmt::Write;
use ledger_device_sdk::libcall::{
    self,
    string::uint256_to_float,
    swap::{
        self, CheckAddressParams, CreateTxParams, PrintableAmountParams, SwapError,
        SwapErrorCommonCode,
    },
    SwapAppErrorCodeTrait,
};
use serde::Deserialize;
use serde_json_core::from_slice;

use crate::utils::{aptos_address_from_pubkey, derive_aptos_public_key};
use crate::AppSW;

/// Exchange transaction fields compared with the approved swap.
#[derive(Deserialize)]
pub struct SwapTransaction<'a> {
    pub coin: &'a str,
    pub value: u64,
    #[serde(with = "hex::serde")]
    pub to: [u8; 32],
}

/// Parse the Exchange transaction representation.
#[inline(never)]
pub fn parse_swap_transaction(input: &[u8]) -> Result<SwapTransaction<'_>, AppSW> {
    let (transaction, _) = from_slice(input).map_err(|_| AppSW::TxParsingFail)?;
    Ok(transaction)
}

// An Aptos address is `0x` followed by 64 hexadecimal digits. The SDK's
// default 64-byte address buffer truncates it; 67 also fits the C terminator.
pub type AptosCreateTxParams = CreateTxParams<16, 67, 32>;
type AptosCheckAddressParams = CheckAddressParams<16, 67, 32>;
type AptosPrintableAmountParams = PrintableAmountParams<16, 67, 32>;

#[repr(u8)]
#[derive(Clone, Copy)]
pub enum SwapAppErrorCode {
    Default = 0,
    InvalidAmount = 1,
    InvalidAddress = 2,
    InvalidCoinConfig = 3,
}

impl SwapAppErrorCodeTrait for SwapAppErrorCode {
    fn as_u8(self) -> u8 {
        self as u8
    }
}

fn swap_error(code: SwapErrorCommonCode, detail: SwapAppErrorCode) -> SwapError<SwapAppErrorCode> {
    SwapError::without_message(code, detail)
}

/// Decode Exchange's right-aligned, big-endian amount without truncation.
fn exchange_amount(bytes: &[u8; 16], length: usize) -> Option<u64> {
    if length == 0 || length > 16 || bytes[..8].iter().any(|byte| *byte != 0) {
        return None;
    }
    Some(u64::from_be_bytes(bytes[8..].try_into().ok()?))
}

fn hex_digit(byte: u8) -> Option<u8> {
    match byte {
        b'0'..=b'9' => Some(byte - b'0'),
        b'a'..=b'f' => Some(byte - b'a' + 10),
        b'A'..=b'F' => Some(byte - b'A' + 10),
        _ => None,
    }
}

/// Require the full prefixed 32-byte Aptos account address.
fn decode_address(input: &[u8]) -> Option<[u8; 32]> {
    if input.len() != 66 || input[..2] != *b"0x" {
        return None;
    }
    let mut address = [0u8; 32];
    for (out, pair) in address.iter_mut().zip(input[2..].chunks_exact(2)) {
        *out = (hex_digit(pair[0])? << 4) | hex_digit(pair[1])?;
    }
    Some(address)
}

/// The Exchange subconfiguration is `[ticker_length, ticker..., decimals]`.
fn coin_config(config: &[u8]) -> Option<(&str, u8)> {
    let (&ticker_len, rest) = config.split_first()?;
    let ticker_len = ticker_len as usize;
    if ticker_len == 0 || ticker_len > 13 || rest.len() != ticker_len + 1 {
        return None;
    }
    let ticker = core::str::from_utf8(&rest[..ticker_len]).ok()?;
    if !ticker.bytes().all(|byte| byte.is_ascii_graphic()) {
        return None;
    }
    Some((ticker, rest[ticker_len]))
}

/// Strictly compare the JSON transaction with the Exchange-approved details.
pub fn check_swap_params(
    params: &AptosCreateTxParams,
    tx: &SwapTransaction<'_>,
) -> Result<(), SwapError<SwapAppErrorCode>> {
    use SwapErrorCommonCode as Code;

    let config = params
        .coin_config
        .get(..params.coin_config_len)
        .ok_or_else(|| swap_error(Code::ErrorWrongMethod, SwapAppErrorCode::InvalidCoinConfig))?;
    let expected_coin = if config.is_empty() {
        "APT"
    } else {
        coin_config(config)
            .map(|(ticker, _)| ticker)
            .ok_or_else(|| {
                swap_error(Code::ErrorWrongMethod, SwapAppErrorCode::InvalidCoinConfig)
            })?
    };
    if tx.coin != expected_coin {
        return Err(swap_error(
            Code::ErrorWrongMethod,
            SwapAppErrorCode::Default,
        ));
    }

    let swap_amount = exchange_amount(&params.amount, params.amount_len)
        .ok_or_else(|| swap_error(Code::ErrorWrongAmount, SwapAppErrorCode::InvalidAmount))?;
    if tx.value != swap_amount {
        return Err(swap_error(
            Code::ErrorWrongAmount,
            SwapAppErrorCode::Default,
        ));
    }

    if params.dest_address_extra_id_len != 0 {
        return Err(swap_error(
            Code::ErrorWrongDestination,
            SwapAppErrorCode::InvalidAddress,
        ));
    }
    let recipient = params
        .dest_address
        .get(..params.dest_address_len)
        .and_then(decode_address)
        .ok_or_else(|| {
            swap_error(
                Code::ErrorWrongDestination,
                SwapAppErrorCode::InvalidAddress,
            )
        })?;
    if tx.to != recipient {
        return Err(swap_error(
            Code::ErrorWrongDestination,
            SwapAppErrorCode::Default,
        ));
    }
    Ok(())
}

/// Dispatch the three Exchange library calls.
pub fn swap_main(arg0: u32) {
    match libcall::get_command(arg0) {
        libcall::LibCallCommand::SwapCheckAddress => {
            let mut params: AptosCheckAddressParams = swap::get_check_address_params(arg0);
            let result = check_address(&params);
            swap::swap_return(swap::SwapResult::CheckAddressResult(&mut params, result));
        }
        libcall::LibCallCommand::SwapGetPrintableAmount => {
            let mut params: AptosPrintableAmountParams = swap::get_printable_amount_params(arg0);
            let amount = get_printable_amount(&params);
            swap::swap_return(swap::SwapResult::PrintableAmountResult(
                &mut params,
                amount.as_str(),
            ));
        }
        libcall::LibCallCommand::SwapSignTransaction => {
            let mut params: AptosCreateTxParams = swap::sign_tx_params(arg0);
            let signed = crate::normal_main(Some(&params));
            swap::swap_return(swap::SwapResult::CreateTxResult(
                &mut params,
                u8::from(signed),
            ));
        }
    }
}

/// Validate a SLIP-10 Aptos path and compare its Ed25519 account address with
/// the prefixed hexadecimal address approved by Exchange.
fn check_address(params: &AptosCheckAddressParams) -> i32 {
    if !(3..=10).contains(&params.dpath_len)
        || params.dpath_len > params.dpath.len() / 4
        || params.dpath[..8] != [0x80, 0, 0, 44, 0x80, 0, 2, 125]
    {
        return 0;
    }
    let Some(reference) = params
        .ref_address
        .get(..params.ref_address_len)
        .and_then(decode_address)
    else {
        return 0;
    };

    let mut path = [0u32; 10];
    for (component, bytes) in path[..params.dpath_len]
        .iter_mut()
        .zip(params.dpath[..params.dpath_len * 4].chunks_exact(4))
    {
        *component = u32::from_be_bytes([bytes[0], bytes[1], bytes[2], bytes[3]]);
        if *component & 0x8000_0000 == 0 {
            return 0;
        }
    }

    let Ok((public_key, _)) = derive_aptos_public_key(&path[..params.dpath_len]) else {
        return 0;
    };
    match aptos_address_from_pubkey(&public_key) {
        Ok(derived) if derived == reference => 1,
        _ => 0,
    }
}

/// Format an Exchange amount using the approved coin configuration.
fn get_printable_amount(params: &AptosPrintableAmountParams) -> ArrayString<64> {
    let mut printable = ArrayString::<64>::new();
    let Some(config) = params.coin_config.get(..params.coin_config_len) else {
        return printable;
    };
    let (ticker, decimals) = if params.is_fee || config.is_empty() {
        ("APT", 8)
    } else {
        match coin_config(config) {
            Some(token) => token,
            None => return printable,
        }
    };
    let Some(amount) = exchange_amount(&params.amount, params.amount_len) else {
        return printable;
    };
    let mut amount_u256 = [0u8; 32];
    amount_u256[24..].copy_from_slice(&amount.to_be_bytes());
    let formatted = uint256_to_float(&amount_u256, decimals as usize);
    if formatted.as_str().is_empty() {
        return printable;
    }
    if write!(&mut printable, "{} {}", formatted.as_str(), ticker).is_err() {
        printable.clear();
    }
    printable
}
