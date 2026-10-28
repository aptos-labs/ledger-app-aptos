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

//! The compiled-in token table: v1's listed coins and fungible assets.
//!
//! Decimals mirror each asset's on-chain metadata; keep them in sync with the
//! registry when the list changes.

use crate::bcs::ADDRESS_LEN;
use crate::transaction::{CoinType, FRAMEWORK_ADDRESS};

/// How a listed asset is identified on chain.
#[derive(Clone, Copy, Debug)]
pub enum Asset {
    /// A legacy coin, identified by the struct tag of its `Coin<T>` type.
    Coin {
        address: [u8; ADDRESS_LEN],
        module: &'static str,
        name: &'static str,
    },
    /// A fungible asset, identified by its metadata object address.
    FungibleAsset {
        // Matched by `primary_fungible_store::transfer` once #25 ports it.
        #[allow(dead_code)]
        metadata: [u8; ADDRESS_LEN],
    },
}

/// A listed asset and how its amounts are shown.
#[derive(Clone, Copy, Debug)]
pub struct Token {
    pub ticker: &'static str,
    pub decimals: u8,
    pub asset: Asset,
}

/// Decimals assumed for an asset that is not listed.
pub const DEFAULT_DECIMALS: u8 = 8;

const fn hex_digit(digit: u8) -> u8 {
    match digit {
        b'0'..=b'9' => digit - b'0',
        b'a'..=b'f' => digit - b'a' + 10,
        _ => panic!("token table addresses are lowercase hexadecimal"),
    }
}

/// Decode a 64-digit lowercase hexadecimal address at compile time.
const fn address(hex: &str) -> [u8; ADDRESS_LEN] {
    assert!(hex.len() == 2 * ADDRESS_LEN);
    let digits = hex.as_bytes();
    let mut out = [0u8; ADDRESS_LEN];
    let mut i = 0;
    while i < ADDRESS_LEN {
        out[i] = (hex_digit(digits[2 * i]) << 4) | hex_digit(digits[2 * i + 1]);
        i += 1;
    }
    out
}

const fn coin(
    ticker: &'static str,
    decimals: u8,
    address_hex: &str,
    module: &'static str,
    name: &'static str,
) -> Token {
    Token {
        ticker,
        decimals,
        asset: Asset::Coin {
            address: address(address_hex),
            module,
            name,
        },
    }
}

const fn fungible_asset(ticker: &'static str, decimals: u8, metadata_hex: &str) -> Token {
    Token {
        ticker,
        decimals,
        asset: Asset::FungibleAsset {
            metadata: address(metadata_hex),
        },
    }
}

/// v1's token table, in v1's order.
pub static TOKENS: [Token; 46] = [
    coin(
        "amAPT",
        8,
        "111ae3e5bc816a5e63c2da97d0aa3886519e0cd5e4b046659fa35796bd11542a",
        "amapt_token",
        "AmnisApt",
    ),
    coin(
        "APARTMENT",
        8,
        "7b7bab2131de3e4f318b4abaa952f7c817b2c3df16c951caca809ac9ca9b650e",
        "APARTMENT",
        "APARTMENT",
    ),
    coin(
        "APE",
        8,
        "ada35ada7e43e2ee1c39633ffccec38b76ce702b4efc2e60b50f63fbe4f710d8",
        "apetos_token",
        "ApetosCoin",
    ),
    coin(
        "APT",
        8,
        "0000000000000000000000000000000000000000000000000000000000000001",
        "aptos_coin",
        "AptosCoin",
    ),
    coin(
        "FOMO",
        6,
        "f891d2e004973430cc2bbbee69f3d0f4adb9c7ae03137b4579f7bb9979283ee6",
        "APTOS_FOMO",
        "APTOS_FOMO",
    ),
    coin(
        "ALT",
        8,
        "d0b4efb4be7c3508d9a26a9b5405cf9f860d0b9e5fe2f498b90e68b8d2cedd3e",
        "aptos_launch_token",
        "AptosLaunchToken",
    ),
    coin(
        "BLT",
        8,
        "fbab9fb68bd2103925317b6a540baa20087b1e7a7a4eb90badee04abb6b5a16f",
        "blt",
        "Blt",
    ),
    coin(
        "MOVE",
        8,
        "27fafcc4e39daac97556af8a803dbb52bcb03f0821898dc845ac54225b9793eb",
        "move_coin",
        "MoveCoin",
    ),
    coin(
        "BUBBLES",
        8,
        "d6a49762f6e4f7401ee79be6f5d4111e70db1408966ba1aa204e6e10c9d437ca",
        "bubbles",
        "BubblesCoin",
    ),
    coin(
        "doodoo",
        8,
        "73eb84966be67e4697fc5ae75173ca6c35089e802650f75422ab49a8729704ec",
        "coin",
        "DooDoo",
    ),
    coin(
        "dstAPT",
        8,
        "d11107bdf0d6d7040c6c0bfbdecb6545191fdf13e8d8d259952f53e1713f61b5",
        "staked_coin",
        "StakedAptos",
    ),
    coin(
        "whGARI",
        8,
        "4def3d3dee27308886f0a3611dd161ce34f977a9a5de4e80b237225923492a2a",
        "coin",
        "T",
    ),
    coin(
        "GUI",
        6,
        "e4ccb6d39136469f376242c31b34d10515c8eaaa38092f804db8e08a8f53c5b2",
        "assets_v1",
        "EchoCoin002",
    ),
    coin(
        "HEART",
        8,
        "7de3fea83cd5ca0e1def27c3f3803af619882db51f34abf30dd04ad12ee6af31",
        "tapos",
        "Heart",
    ),
    coin(
        "LSD",
        8,
        "53a30a6e5936c0a4c5140daed34de39d17ca7fcae08f947c02e979cef98a3719",
        "coin",
        "LSD",
    ),
    coin(
        "MOJO",
        8,
        "881ac202b1f1e6ad4efcff7a1d0579411533f2502417a19211cfc49751ddb5f4",
        "coin",
        "MOJO",
    ),
    coin(
        "MOOMOO",
        6,
        "c5fbbcc4637aeebb4e732767abee8a21f2b0776f73b73e16ce13e7d31d6700da",
        "MOOMOO",
        "MOOMOO",
    ),
    coin(
        "MOD",
        8,
        "6f986d146e4a90b828d8c12c14b6f4e003fdff11a8eecceceb63744363eaac01",
        "mod_coin",
        "MOD",
    ),
    coin(
        "Cake",
        8,
        "159df6b7689437016108a019fd5bef736bac692b6d4a1f10c941f6fbb9a74ca6",
        "oft",
        "CakeOFT",
    ),
    coin(
        "RETuRD",
        8,
        "df3d5eb83df80dfde8ceb1edaa24d8dbc46da6a89ae134a858338e1b86a29e38",
        "coin",
        "Returd",
    ),
    coin(
        "SHRIMP",
        2,
        "55987edfab9a57f69bac759674f139ae473b5e09a9283848c1f87faf6fc1e789",
        "shrimp",
        "ShrimpCoin",
    ),
    coin(
        "whSOL",
        8,
        "dd89c0e695df0692205912fb69fc290418bed0dbe6e4573d744a6d5e6bab6c13",
        "coin",
        "T",
    ),
    coin(
        "stAPT",
        8,
        "111ae3e5bc816a5e63c2da97d0aa3886519e0cd5e4b046659fa35796bd11542a",
        "stapt_token",
        "StakedApt",
    ),
    coin(
        "sthAPT",
        8,
        "faf4e633ae9eb31366c9ca24214231760926576c7b625313b3688b5e900731f6",
        "staking",
        "StakedThalaAPT",
    ),
    coin(
        "ceUSDT",
        6,
        "8d87a65ba30e09357fa2edea2c80dbac296e5dec2b18287113500b902942929d",
        "celer_coin_manager",
        "UsdtCoin",
    ),
    coin(
        "lzUSDT",
        6,
        "f22bede237a07e121b56d91a491eb7bcdfd1f5907926a9e58338f964a01b17fa",
        "asset",
        "USDT",
    ),
    coin(
        "whUSDT",
        6,
        "a2eda21a58856fda86451436513b867c97eecb4ba099da5775520e0f7492e852",
        "coin",
        "T",
    ),
    coin(
        "thAPT",
        8,
        "faf4e633ae9eb31366c9ca24214231760926576c7b625313b3688b5e900731f6",
        "staking",
        "ThalaAPT",
    ),
    // v1 listed THL without its leading zero and so never matched it.
    coin(
        "THL",
        8,
        "07fd500c11216f0fe3095d0c4b8aa4d64a4e2e04f83758462f2b127255643615",
        "thl_coin",
        "THL",
    ),
    coin(
        "LOON",
        6,
        "268d4a7a2ad93274edf6116f9f20ad8455223a7ab5fc73154f687e7dbc3e3ec6",
        "LOON",
        "LOON",
    ),
    coin(
        "TIN",
        8,
        "c32ba5d293577cbb1df390f35b2bc6369a593b736d0865fedec1a2b08565de8e",
        "in_coin",
        "InCoin",
    ),
    coin(
        "TOMA",
        6,
        "9d0595765a31f8d56e1d2aafc4d6c76f283c67a074ef8812d8c31bd8252ac2c3",
        "asset",
        "TOMA",
    ),
    coin(
        "tAPT",
        8,
        "84d7aeef42d38a5ffc3ccef853e1b82e4958659d16a7de736a29c55fbbeb0114",
        "staked_aptos_coin",
        "StakedAptosCoin",
    ),
    coin(
        "UPTOS",
        8,
        "4fbed3f8a3fd8a11081c8b6392152a8b0cb14d70d0414586f0c9b858fcd2d6a7",
        "UPTOS",
        "UPTOS",
    ),
    coin(
        "lzUSDC",
        6,
        "f22bede237a07e121b56d91a491eb7bcdfd1f5907926a9e58338f964a01b17fa",
        "asset",
        "USDC",
    ),
    coin(
        "whUSDC",
        6,
        "5e156f1207d0ebfa19a9eeff00d62a282278fb8719f4fab3a586a0a2c0fffbea",
        "coin",
        "T",
    ),
    coin(
        "ceWBNB",
        8,
        "8d87a65ba30e09357fa2edea2c80dbac296e5dec2b18287113500b902942929d",
        "celer_coin_manager",
        "BnbCoin",
    ),
    coin(
        "lzWETH",
        6,
        "f22bede237a07e121b56d91a491eb7bcdfd1f5907926a9e58338f964a01b17fa",
        "asset",
        "WETH",
    ),
    coin(
        "whWETH",
        8,
        "cc8a89c8dce9693d354449f1f73e60e14e347417854f029db5bc8e7454008abb",
        "coin",
        "T",
    ),
    coin(
        "ANI",
        8,
        "16fe2df00ea7dde4a63409201f7f4e536bde7bb7335526a35d05111e68aa322c",
        "AnimeCoin",
        "ANI",
    ),
    fungible_asset(
        "AMA",
        8,
        "d0ab8c2f76cd640455db56ca758a9766a966c88f77920347aac1719edab1df5e",
    ),
    fungible_asset(
        "CELL",
        8,
        "2ebb2ccac5e027a87fa0e2e5f656a3a4238d6a48d93ec9b610d570fc0aa0df12",
    ),
    fungible_asset(
        "MKL",
        6,
        "878370592f9129e14b76558689a4b570ad22678111df775befbfcbc9fb3d90ab",
    ),
    fungible_asset(
        "USDT",
        6,
        "357b0b74bc833e95a115ad22604854d6b0fca151cecd94111770e5d6ffc9dc2b",
    ),
    fungible_asset(
        "TruAPT",
        8,
        "aef6a8c3182e076db72d64324617114cacf9a52f28325edc10b483f7f05da0e7",
    ),
    fungible_asset(
        "USDC",
        6,
        "bae207659db88bea0cbead6da0ed00aac12edcdda169e591cd41c94180b46f3b",
    ),
];

/// Find the listed coin with this struct tag.
///
/// The address must match exactly. The identifiers compare without regard
/// to ASCII case, as v1 compared the formatted type, except at the
/// framework address: only the full `0x1::aptos_coin::AptosCoin` is APT.
pub fn find_coin(coin: &CoinType) -> Option<&'static Token> {
    let exact = *coin.address == FRAMEWORK_ADDRESS;
    let same = |listed: &str, actual: &str| {
        if exact {
            listed == actual
        } else {
            listed.eq_ignore_ascii_case(actual)
        }
    };
    TOKENS.iter().find(|token| match &token.asset {
        Asset::Coin {
            address,
            module,
            name,
        } => address == coin.address && same(module, coin.module) && same(name, coin.name),
        Asset::FungibleAsset { .. } => false,
    })
}
