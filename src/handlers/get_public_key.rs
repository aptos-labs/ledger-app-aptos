/*****************************************************************************
 *   Ledger App Boilerplate Rust.
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

use crate::app_ui::address::ui_display_pk;
use crate::utils::{aptos_address_from_pubkey, derive_aptos_public_key, Bip32Path};
use crate::AppSW;
use ledger_device_sdk::io::{Command, CommandResponse};

/// Derive an Aptos Ed25519 public key from a validated SLIP-10 path and,
/// optionally, show the corresponding account address for user verification.
pub fn handler_get_public_key(
    command: Command<'_>,
    display: bool,
) -> Result<CommandResponse<'_>, AppSW> {
    let data = command.get_data();
    let path: Bip32Path = data.try_into()?;

    let (pubkey, chain_code) = derive_aptos_public_key(path.as_ref())?;

    let comm = command.into_comm();
    if display {
        let address = aptos_address_from_pubkey(&pubkey)?;
        if !ui_display_pk(comm, &address, path.as_ref())? {
            return Err(AppSW::Deny);
        }
    }

    let mut response = comm.begin_response();
    response.append(&[33, 0x04])?;
    response.append(&pubkey)?;
    response.append(&[32])?;
    response.append(&chain_code)?;

    Ok(response)
}
