---
status: accepted
---

# v2 keeps v1's APDU protocol; changes are additive and wait for after cutover

v2 speaks v1's APDU protocol exactly: CLA `0x5B`, the same INS/P1/P2 rules and SIGN_TX chunking, the same response layouts, and v1's status words (`0xB000`–`0xB00A`). It does not adopt the boilerplate's CLA `0xE0` and status-word scheme. Ledger Live, the Aptos TS SDK, hw-app-aptos and wallets such as Petra already speak v1's protocol, so 2.0.0 has to be a drop-in replacement. Protocol changes are allowed only as additions alongside the existing commands, and none ship before **Cutover**: **Parity** stays a finite target.

## Considered Options

- **Adopt the boilerplate's conventions and ship a matching hw-app-aptos major version.** Rejected: every client would have to release in lockstep with 2.0.0, and users on stale clients would break at the firmware update.
- **Decide keep/extend/break per command.** Rejected: it reopens the protocol for every **Port unit** and makes **Parity** open-ended.

## Consequences

- The boilerplate's CLA, `AppSW` and response formats get replaced. Expect conflicts in `src/main.rs` during **Boilerplate syncs**.
- Behavior can still change inside the protocol: the review UI moves to NBGL on Nano, and parsing becomes stricter. Each such change is a **Divergence**.
