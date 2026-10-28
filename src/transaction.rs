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

use crate::bcs::{BcsError, Cursor, ADDRESS_LEN, TYPE_TAG_STRUCT};
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

/// Decimal places of APT: one APT is 10^8 octas.
pub const APT_DECIMALS: u8 = 8;

/// The Aptos framework address, `0x1`. Only functions published there are
/// clear-signed as known operations.
pub const FRAMEWORK_ADDRESS: [u8; ADDRESS_LEN] = {
    let mut address = [0u8; ADDRESS_LEN];
    address[ADDRESS_LEN - 1] = 1;
    address
};

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

/// The struct tag of a legacy coin type.
#[derive(Clone, Copy, Debug)]
pub struct CoinType<'a> {
    pub address: &'a [u8; ADDRESS_LEN],
    pub module: &'a str,
    pub name: &'a str,
}

/// The arguments of a transfer: `(receiver: address, amount: u64)`.
#[derive(Clone, Copy, Debug)]
pub struct Transfer<'a> {
    pub receiver: &'a [u8; ADDRESS_LEN],
    pub amount: u64,
}

/// What a framework entry function does, when the app knows it.
#[derive(Clone, Copy, Debug)]
pub enum Operation<'a> {
    /// `0x1::aptos_account::transfer`, which moves APT.
    AptTransfer(Transfer<'a>),
    /// `0x1::coin::transfer<Coin>` or `0x1::aptos_account::transfer_coins<Coin>`.
    CoinTransfer {
        coin: CoinType<'a>,
        transfer: Transfer<'a>,
    },
}

/// A fully qualified entry function.
#[derive(Clone, Copy, Debug)]
pub struct EntryFunction<'a> {
    pub module_address: &'a [u8; ADDRESS_LEN],
    pub module: &'a str,
    pub function: &'a str,
    /// `None` for a function outside the clear-signed set.
    pub operation: Option<Operation<'a>>,
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

    /// The operation this transaction clear-signs, if any: a plain
    /// `RawTransaction` calling a framework function the app understands.
    pub fn clear_signed_operation(&self) -> Option<(&EntryFunction<'a>, Operation<'a>)> {
        match (self.envelope, &self.payload) {
            (Envelope::Raw, Payload::EntryFunction(function)) => {
                function.operation.map(|operation| (function, operation))
            }
            _ => None,
        }
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
        // Only a plain `RawTransaction` can be clear-signed, so only its
        // entry function needs its operation decoded.
        PAYLOAD_ENTRY_FUNCTION => {
            Payload::EntryFunction(parse_entry_function(cursor, envelope == Envelope::Raw)?)
        }
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

/// Parse an entry function.
///
/// With `decode`, a framework operation the app understands has its
/// arguments decoded and their shape checked, as v1 did; otherwise the
/// arguments are skipped, as for any function the app does not know.
fn parse_entry_function<'a>(
    cursor: &mut Cursor<'a>,
    decode: bool,
) -> Result<EntryFunction<'a>, AppSW> {
    let module_address = cursor.address()?;
    let module = cursor.identifier()?;
    let function = cursor.identifier()?;
    let operation = if decode && *module_address == FRAMEWORK_ADDRESS {
        match (module, function) {
            ("aptos_account", "transfer") => Some(parse_apt_transfer(cursor)?),
            ("coin", "transfer") | ("aptos_account", "transfer_coins") => {
                Some(parse_coin_transfer(cursor)?)
            }
            _ => skip_arguments(cursor)?,
        }
    } else {
        skip_arguments(cursor)?
    };
    Ok(EntryFunction {
        module_address,
        module,
        function,
        operation,
    })
}

/// Skip the type arguments and arguments of a function the app does not know.
fn skip_arguments(cursor: &mut Cursor) -> Result<Option<Operation<'static>>, AppSW> {
    cursor.skip_type_tags()?;
    // Arguments: a vector of BCS-encoded byte strings.
    for _ in 0..cursor.uleb128()? {
        cursor.bytes()?;
    }
    Ok(None)
}

/// `transfer(to: address, amount: u64)` with no type argument.
fn parse_apt_transfer<'a>(cursor: &mut Cursor<'a>) -> Result<Operation<'a>, AppSW> {
    if cursor.uleb128()? != 0 {
        return Err(AppSW::TxParsingFail);
    }
    Ok(Operation::AptTransfer(parse_transfer_arguments(cursor)?))
}

/// `transfer<Coin>(to: address, amount: u64)` where `Coin` is a struct with
/// no type arguments of its own.
fn parse_coin_transfer<'a>(cursor: &mut Cursor<'a>) -> Result<Operation<'a>, AppSW> {
    if cursor.uleb128()? != 1 || cursor.uleb128()? != TYPE_TAG_STRUCT {
        return Err(AppSW::TxParsingFail);
    }
    let coin = CoinType {
        address: cursor.address()?,
        module: cursor.identifier()?,
        name: cursor.identifier()?,
    };
    if cursor.uleb128()? != 0 {
        return Err(AppSW::TxParsingFail);
    }
    let transfer = parse_transfer_arguments(cursor)?;
    Ok(Operation::CoinTransfer { coin, transfer })
}

/// Exactly two arguments: a 32-byte receiver address and a `u64` amount.
fn parse_transfer_arguments<'a>(cursor: &mut Cursor<'a>) -> Result<Transfer<'a>, AppSW> {
    if cursor.uleb128()? != 2 {
        return Err(AppSW::TxParsingFail);
    }
    let receiver = cursor.bytes()?;
    let amount = cursor.bytes()?;
    let receiver = receiver.try_into().map_err(|_| AppSW::TxParsingFail)?;
    let amount = amount.try_into().map_err(|_| AppSW::TxParsingFail)?;
    Ok(Transfer {
        receiver,
        amount: u64::from_le_bytes(amount),
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
                parse_entry_function(cursor, false)?;
            }
            MULTISIG_SCRIPT => parse_script(cursor)?,
            _ => return Err(AppSW::TxParsingFail),
        }
    }
    Ok(())
}
