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

//! A zero-allocation BCS cursor over borrowed bytes.
//!
//! Every read is bounds-checked and returns slices of the input, so a decoded
//! value can never show content the signed bytes do not contain.

/// Why a BCS read failed.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum BcsError {
    /// The input ended before the value did.
    Eof,
    /// A ULEB128 length used more bytes than its value needs.
    NonCanonical,
    /// A ULEB128 length does not fit in a `u32`.
    Overflow,
    /// A tag, boolean or identifier holds a value BCS does not allow.
    Invalid,
}

pub type Result<T, E = BcsError> = core::result::Result<T, E>;

/// Size in bytes of an account address.
pub const ADDRESS_LEN: usize = 32;

/// Longest Move identifier the bytecode format allows
/// (`IDENTIFIER_SIZE_MAX` in move-binary-format).
pub const MAX_IDENTIFIER_LEN: usize = 255;

/// Deepest type-argument nesting aptos-core deserializes
/// (`MAX_TYPE_TAG_NESTING` in move-core-types).
const MAX_TYPE_TAG_NESTING: u8 = 8;

/// Reads BCS values front to back from a byte slice.
pub struct Cursor<'a> {
    data: &'a [u8],
}

impl<'a> Cursor<'a> {
    pub const fn new(data: &'a [u8]) -> Self {
        Self { data }
    }

    /// Whether every byte has been consumed.
    pub const fn is_empty(&self) -> bool {
        self.data.is_empty()
    }

    /// Take the next `len` bytes.
    pub fn take(&mut self, len: usize) -> Result<&'a [u8]> {
        if len > self.data.len() {
            return Err(BcsError::Eof);
        }
        let (head, tail) = self.data.split_at(len);
        self.data = tail;
        Ok(head)
    }

    pub fn u8(&mut self) -> Result<u8> {
        Ok(self.take(1)?[0])
    }

    pub fn u64(&mut self) -> Result<u64> {
        let mut bytes = [0u8; 8];
        bytes.copy_from_slice(self.take(8)?);
        Ok(u64::from_le_bytes(bytes))
    }

    /// Read a boolean, rejecting any byte other than 0 and 1.
    pub fn bool(&mut self) -> Result<bool> {
        match self.u8()? {
            0 => Ok(false),
            1 => Ok(true),
            _ => Err(BcsError::Invalid),
        }
    }

    /// Read a ULEB128-encoded `u32` in canonical form: no trailing zero
    /// groups and no value above `u32::MAX`.
    pub fn uleb128(&mut self) -> Result<u32> {
        let mut value: u64 = 0;
        for shift in (0..32).step_by(7) {
            let byte = self.u8()?;
            value |= u64::from(byte & 0x7F) << shift;
            if value > u64::from(u32::MAX) {
                return Err(BcsError::Overflow);
            }
            if byte & 0x80 == 0 {
                if shift > 0 && byte == 0 {
                    return Err(BcsError::NonCanonical);
                }
                return Ok(value as u32);
            }
        }
        Err(BcsError::Overflow)
    }

    /// Read a 32-byte account address.
    pub fn address(&mut self) -> Result<&'a [u8; ADDRESS_LEN]> {
        self.take(ADDRESS_LEN)?
            .try_into()
            .map_err(|_| BcsError::Eof)
    }

    /// Read a length-prefixed byte string.
    pub fn bytes(&mut self) -> Result<&'a [u8]> {
        let len = self.uleb128()? as usize;
        self.take(len)
    }

    /// Read a Move identifier: at most [`MAX_IDENTIFIER_LEN`] ASCII letters,
    /// digits, underscores and dollar signs, and at least one of them.
    pub fn identifier(&mut self) -> Result<&'a str> {
        let bytes = self.bytes()?;
        if bytes.is_empty()
            || bytes.len() > MAX_IDENTIFIER_LEN
            || !bytes
                .iter()
                .all(|b| b.is_ascii_alphanumeric() || matches!(b, b'_' | b'$'))
        {
            return Err(BcsError::Invalid);
        }
        core::str::from_utf8(bytes).map_err(|_| BcsError::Invalid)
    }

    /// Skip one type tag, including any type arguments it nests.
    ///
    /// Nesting is capped like aptos-core's deserializer, so the recursion is
    /// at most [`MAX_TYPE_TAG_NESTING`] small frames deep.
    pub fn skip_type_tag(&mut self) -> Result<()> {
        self.skip_type_tag_at(0)
    }

    fn skip_type_tag_at(&mut self, depth: u8) -> Result<()> {
        let tag = self.uleb128()?;
        if matches!(tag, 6 | 7 | 11) && depth >= MAX_TYPE_TAG_NESTING {
            return Err(BcsError::Invalid);
        }
        match tag {
            // bool, u8, u64, u128, address, signer, u16, u32, u256, i8..i256
            0..=5 | 8..=10 | 12..=17 => Ok(()),
            // vector<T>
            6 => self.skip_type_tag_at(depth + 1),
            // struct: address, module, name, then type arguments
            7 => {
                self.address()?;
                self.identifier()?;
                self.identifier()?;
                for _ in 0..self.uleb128()? {
                    self.skip_type_tag_at(depth + 1)?;
                }
                Ok(())
            }
            // function: parameters and results, each `&T`, `&mut T` or `T`,
            // then the ability set
            11 => {
                for _ in 0..2 {
                    for _ in 0..self.uleb128()? {
                        if self.uleb128()? > 2 {
                            return Err(BcsError::Invalid);
                        }
                        self.skip_type_tag_at(depth + 1)?;
                    }
                }
                self.u8().map(drop)
            }
            _ => Err(BcsError::Invalid),
        }
    }

    /// Skip a length-prefixed vector of type tags.
    pub fn skip_type_tags(&mut self) -> Result<()> {
        for _ in 0..self.uleb128()? {
            self.skip_type_tag()?;
        }
        Ok(())
    }
}
