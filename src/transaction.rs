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

//! Strict parsing of Aptos signing payloads.
//!
//! A `RawTransaction` is read front to back and must consume the whole
//! buffer, so no trailing or malformed bytes can ride along unseen.

use crate::bcs::{BcsError, Cursor, ADDRESS_LEN};
use crate::AppSW;

/// SHA3-256("APTOS::RawTransaction"), the signing prefix.
const RAW_TRANSACTION_PREFIX: [u8; 32] = [
    181, 233, 125, 176, 127, 160, 189, 14, 85, 152, 170, 54, 67, 169, 188, 111, 102, 147, 189, 220,
    26, 159, 236, 158, 103, 74, 70, 30, 170, 0, 177, 147,
];
/// SHA3-256("APTOS::RawTransactionWithData"), the signing prefix.
const RAW_TRANSACTION_WITH_DATA_PREFIX: [u8; 32] = [
    94, 250, 60, 79, 2, 248, 58, 15, 75, 45, 105, 252, 149, 198, 7, 204, 2, 130, 92, 196, 231, 190,
    83, 110, 240, 153, 45, 240, 80, 217, 230, 124,
];

/// Octas in one APT.
pub const OCTAS_PER_APT: u64 = 100_000_000;

// `TransactionPayload` variant indices.
const PAYLOAD_SCRIPT: u32 = 0;
const PAYLOAD_ENTRY_FUNCTION: u32 = 2;
const PAYLOAD_MULTISIG: u32 = 3;
// `MultisigTransactionPayload` variant indices.
const MULTISIG_ENTRY_FUNCTION: u32 = 0;
const MULTISIG_SCRIPT: u32 = 1;
// `RawTransactionWithData` variant indices.
const WITH_DATA_MULTI_AGENT: u32 = 0;
const WITH_DATA_FEE_PAYER: u32 = 1;

impl From<BcsError> for AppSW {
    fn from(_: BcsError) -> Self {
        AppSW::TxParsingFail
    }
}

/// How the signed bytes wrap the `RawTransaction`.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Envelope {
    /// A plain `RawTransaction`.
    Raw,
    /// A `RawTransactionWithData` (multi-agent or fee payer).
    WithData,
}

/// A fully qualified entry function.
#[derive(Clone, Copy, Debug)]
pub struct EntryFunction<'a> {
    pub module_address: &'a [u8; ADDRESS_LEN],
    pub module: &'a str,
    pub function: &'a str,
}

/// The payload of a `RawTransaction`.
#[derive(Clone, Copy, Debug)]
pub enum Payload<'a> {
    Script,
    EntryFunction(EntryFunction<'a>),
    /// A multisig payload, whose inner entry function is not displayed.
    Multisig,
}

/// The fields of a signing payload that the review shows.
#[derive(Clone, Copy, Debug)]
pub struct Transaction<'a> {
    pub envelope: Envelope,
    pub payload: Payload<'a>,
    /// `gas_unit_price * max_gas_amount`, in octas.
    pub gas_fee: u64,
}

impl<'a> Transaction<'a> {
    /// Parse `input` if it carries a transaction signing prefix.
    ///
    /// Returns `None` for any other payload. A prefixed payload that is
    /// malformed, has trailing bytes, or whose gas fee overflows is an error.
    pub fn parse(input: &'a [u8]) -> Option<Result<Self, AppSW>> {
        if let Some(body) = input.strip_prefix(&RAW_TRANSACTION_PREFIX) {
            return Some(parse_raw_transaction_payload(body));
        }
        if let Some(body) = input.strip_prefix(&RAW_TRANSACTION_WITH_DATA_PREFIX) {
            return Some(parse_raw_transaction_with_data_payload(body));
        }
        None
    }
}

fn parse_raw_transaction_payload(body: &[u8]) -> Result<Transaction<'_>, AppSW> {
    let mut cursor = Cursor::new(body);
    let transaction = parse_raw_transaction(&mut cursor, Envelope::Raw)?;
    finish(&cursor)?;
    Ok(transaction)
}

fn parse_raw_transaction_with_data_payload(body: &[u8]) -> Result<Transaction<'_>, AppSW> {
    let mut cursor = Cursor::new(body);
    let variant = cursor.uleb128()?;
    if variant != WITH_DATA_MULTI_AGENT && variant != WITH_DATA_FEE_PAYER {
        return Err(AppSW::TxParsingFail);
    }
    let transaction = parse_raw_transaction(&mut cursor, Envelope::WithData)?;
    // Secondary signer addresses.
    for _ in 0..cursor.uleb128()? {
        cursor.address()?;
    }
    if variant == WITH_DATA_FEE_PAYER {
        cursor.address()?;
    }
    finish(&cursor)?;
    Ok(transaction)
}

fn finish(cursor: &Cursor) -> Result<(), AppSW> {
    if cursor.is_empty() {
        Ok(())
    } else {
        Err(AppSW::TxParsingFail)
    }
}

/// Parse a `RawTransaction` front to back.
fn parse_raw_transaction<'a>(
    cursor: &mut Cursor<'a>,
    envelope: Envelope,
) -> Result<Transaction<'a>, AppSW> {
    let _sender = cursor.address()?;
    let _sequence_number = cursor.u64()?;
    let payload = match cursor.uleb128()? {
        PAYLOAD_SCRIPT => {
            parse_script(cursor)?;
            Payload::Script
        }
        PAYLOAD_ENTRY_FUNCTION => Payload::EntryFunction(parse_entry_function(cursor)?),
        PAYLOAD_MULTISIG => {
            parse_multisig(cursor)?;
            Payload::Multisig
        }
        _ => return Err(AppSW::TxParsingFail),
    };
    let max_gas_amount = cursor.u64()?;
    let gas_unit_price = cursor.u64()?;
    let _expiration_timestamp_secs = cursor.u64()?;
    let _chain_id = cursor.u8()?;

    // A fee that does not fit in 64 bits is rejected, never shown wrapped.
    let gas_fee = gas_unit_price
        .checked_mul(max_gas_amount)
        .ok_or(AppSW::DisplayGasFeeFail)?;
    Ok(Transaction {
        envelope,
        payload,
        gas_fee,
    })
}

fn parse_entry_function<'a>(cursor: &mut Cursor<'a>) -> Result<EntryFunction<'a>, AppSW> {
    let module_address = cursor.address()?;
    let module = cursor.identifier()?;
    let function = cursor.identifier()?;
    cursor.skip_type_tags()?;
    // Arguments: a vector of BCS-encoded byte strings.
    for _ in 0..cursor.uleb128()? {
        cursor.bytes()?;
    }
    Ok(EntryFunction {
        module_address,
        module,
        function,
    })
}

fn parse_script(cursor: &mut Cursor) -> Result<(), AppSW> {
    cursor.bytes()?;
    cursor.skip_type_tags()?;
    for _ in 0..cursor.uleb128()? {
        skip_script_argument(cursor)?;
    }
    Ok(())
}

fn skip_script_argument(cursor: &mut Cursor) -> Result<(), AppSW> {
    let skipped = match cursor.uleb128()? {
        // U8, I8
        0 | 10 => cursor.take(1).map(drop),
        // U64, I64
        1 | 13 => cursor.take(8).map(drop),
        // U128, I128
        2 | 14 => cursor.take(16).map(drop),
        // Address
        3 => cursor.address().map(drop),
        // U8Vector, Serialized
        4 | 9 => cursor.bytes().map(drop),
        // Bool
        5 => cursor.bool().map(drop),
        // U16, I16
        6 | 11 => cursor.take(2).map(drop),
        // U32, I32
        7 | 12 => cursor.take(4).map(drop),
        // U256, I256
        8 | 15 => cursor.take(32).map(drop),
        _ => Err(BcsError::Invalid),
    };
    skipped.map_err(AppSW::from)
}

fn parse_multisig(cursor: &mut Cursor) -> Result<(), AppSW> {
    cursor.address()?;
    if cursor.bool()? {
        match cursor.uleb128()? {
            MULTISIG_ENTRY_FUNCTION => {
                parse_entry_function(cursor)?;
            }
            MULTISIG_SCRIPT => parse_script(cursor)?,
            _ => return Err(AppSW::TxParsingFail),
        }
    }
    Ok(())
}
