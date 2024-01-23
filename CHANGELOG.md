# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.6.9] - 2024-01-23

### Added

- [Nano S/SP/X] Always inform about the blind signing, even if the option is activated in the settings.
- [Nano S/SP/X] Invite users to enable the blind signing setting during the signature request.
- Add MAX_TRANSACTION_PACKETS to Makefile for TARGET_STAX.
- Implement the UI for Ledger Stax.
  - Separate the common UI code for BAGL and NBGL.
  - Put general functions into utils.
  - Refactor BAGL UI (display, menu, settings).
  - Implement NBGL UI (display, menu, settings).
  - Add Aptos icon for Ledger Stax.
- Expand the displayed info:
  - Parse `0x1::aptos_account::transfer_coins` function
  - Now show "Transaction Type" for all entry_function calls
  - Add support for multisig payload variant
- Add new tests.

### Fixed

- Fix typo on Blind Signing warning screen.
- Fix the tests, remove the obsolete ones after an upgrade.
- Use "no_throw" crypto functions instead of deprecated ones.

### Changed

- Upgrade to a newer version of the SDK.
- Rewrite existing tests using the Ragger framework.
- Update workflows for misspellings checks, coding style checks, documentation generation, clang static analyzer, Python client checks, CodeQL checks, build and functional tests.

## [0.4.17] - 2023-04-18

### Fixed

- Reduce the RAM allocated for large transactions.
- Remove the hard-set DEBUG=1 flag in the Makefile.

## [0.4.15] - 2023-04-12

### Added

- Implement blind signing functionality.
- Add the "Allow Blind Signing" option to the "Settings".
- Add status word for fail to display gas fee (0xB009).

### Fixed

- Make sure there is no THROW operator in the CATCH_OTHER block.
- Check for null characters in the middle of the message.
- Make sure the message is a null-terminated string.
- Use blind signing for shortened messages.
- Do not display leading zeros for module addresses, display only meaningful bytes for visual verification.
- Clearer work with the global state.
- Fix the operation of P1 as a sequence number of the chunks on SIGN_TX request.

### Changed

- Refactoring transaction displaying. Do not calculate gas fee when displaying a message.
- Get rid of the use of H as a format specifier using the snprintf function while working with the UI.

### Removed

- Remove unnecessary transaction hash calculations.

## [0.3.4] - 2023-02-28

### Added

- Use the maximum SRAM capacity to store transactions for each Ledger model.
- Create a "Settings" step in the menu.
  - Create data types for the NVM storage.
  - Add the "Show Full Message" option.
  - Move the "Version" info to the "About" submenu.

### Fixed

- Fix a problem with signing short messages.
- Add a string length check defining well-known transaction types.
- CI: Explicitly specify the ledger-app-builder version.

### Removed

- Get rid of the "app-boilerplate" legacy.

## [0.1.0] - 2023-02-10

### Added

- Improve display for well-known transaction types.

### Fixed

- Fix the bug in the CodeQL workflow.
- Fix Fuzzing test.

### Removed

- Remove irrelevant code.

## [0.0.1] - 2022-09-27

### Added

- Initial commit.
