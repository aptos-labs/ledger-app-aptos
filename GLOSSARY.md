# Ledger Aptos App

The Aptos wallet application for Ledger hardware devices. It is being rewritten from C (v1) to Rust (v2).

## Release lines

**v1**:
The C implementation of the app, released as 0.x versions. It is retired at **Cutover**.
_Avoid_: legacy app, old app

**v2**:
The Rust implementation of the app, released as 2.x versions from **Cutover** onwards.
_Avoid_: new app, Rust app

## Rust migration

**Boilerplate**:
LedgerHQ/app-boilerplate-rust, the upstream template that **v2** descends from.
_Avoid_: template, starter

**Boilerplate sync**:
Bringing upstream **Boilerplate** changes into **v2** with a merge.
_Avoid_: upstream update, rebase on boilerplate

**Next**:
The long-lived branch where **v2** is built until **Cutover**.
_Avoid_: rust branch, v2 branch

**Parity**:
The state where **v2** does everything **v1** does: every item on the parity checklist is closed, and every v1 functional test and swap test passes on all supported devices.
_Avoid_: feature complete, done

**Cutover**:
The one-time event when **v2** replaces **v1** on `main` and ships as 2.0.0.
_Avoid_: launch, switchover, v2 merge
