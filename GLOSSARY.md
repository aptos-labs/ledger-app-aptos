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
Bringing upstream **Boilerplate** changes into **v2**.
_Avoid_: upstream update, rebase on boilerplate

**Next**:
Where **v2** is built until **Cutover**.
_Avoid_: rust branch, v2 branch

**Parity**:
The state where **v2** does everything **v1** does, over **v1**'s APDU protocol and with the same review semantics, apart from recorded **Divergences**.
_Avoid_: feature complete, done

**Divergence**:
A deliberate difference between **v2**'s behavior and **v1**'s, recorded in the PR that introduces it.
_Avoid_: regression, deviation

**Port unit**:
One independently mergeable slice of **v1** behavior, ported to **Next** together with its **v1** functional tests and tracked as one issue in the "v2 parity" milestone.
_Avoid_: feature PR, chunk

**Cutover**:
The one-time event when **v2** replaces **v1** on `main` and ships as 2.0.0.
_Avoid_: launch, switchover, v2 merge

## Signing

**Clear signing**:
Signing where the user reviews the decoded meaning of the payload (such as recipient, amount or function) before approving.
_Avoid_: verified signing, safe signing

**Blind signing**:
Signing a payload the app cannot fully decode, permitted only when the user has enabled the Blind signing setting.
_Avoid_: unverified signing, raw signing
