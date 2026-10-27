---
status: accepted
---

# Rust rewrite on an unrelated-history branch, joined to main at cutover

We are rewriting the C app (v1) in Rust (v2) on a long-lived `next` branch seeded from LedgerHQ/app-boilerplate-rust's full history, unrelated to `main`, so v1 keeps shipping and boilerplate updates stay plain `git merge`s. At cutover, `next` is joined into `main` with a merge commit that keeps `main`'s C history as the first parent, so both histories survive.

The full rules and the cutover runbook are in [docs/rust-migration.md](../rust-migration.md).

## Considered Options

- **Rewrite in a new repository.** Rejected: it loses the issue, PR, and tag history, and the continuity of Ledger's deployment from this repo.
- **Seed `next` from a squashed snapshot of the boilerplate.** Rejected: every boilerplate update would turn into a manual diff-and-reapply.
- **Port in place on `main` behind a build switch.** Rejected: C and Rust CI and build systems can't share one tree cleanly.
- **Join by running `git merge -s ours main` on `next`.** Rejected: it makes `next` the first parent, so `git log --first-parent main` would skip the C history.

## Consequences

- Boilerplate-sync PRs and the cutover PR can never be squashed: squashing cuts the link to the boilerplate's history.
- GitHub can't open a PR between unrelated histories. The join commit has to be created locally on a `cutover` branch first.
- Files that exist only on `main` disappear at cutover. That's why repo meta files (agent docs, glossary, ADRs) live on `next`.
- Fixes merged into `main` during the migration don't reach `next` on their own.
