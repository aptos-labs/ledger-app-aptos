---
status: accepted
---

# Rust rewrite on an unrelated-history branch, joined to main at cutover

We are rewriting the C app (v1) in Rust (v2), starting from LedgerHQ/app-boilerplate-rust. The rewrite lives on a long-lived `next` branch that starts from the boilerplate's full history, so it shares no commits with `main`. Starting from the boilerplate's history means upstream boilerplate updates can be taken with plain `git merge`. Keeping it off `main` means v1 keeps shipping during the migration. At cutover, a join commit with parents (`main`, `next`) and exactly `next`'s tree lands on `main` through a PR. That keeps both histories, and `main`'s first-parent line stays the C history.

The full rules and the cutover runbook are in [docs/rust-migration.md](../rust-migration.md).

## Considered Options

- **Rewrite in a new repository.** Rejected: it loses the issue, PR, and tag history, and the continuity of Ledger's deployment from this repo.
- **Seed `next` from a squashed snapshot of the boilerplate.** Rejected: every boilerplate update would turn into a manual diff-and-reapply.
- **Port in place on `main` behind a build switch.** Rejected: C and Rust CI and build systems can't share one tree cleanly.
- **Join by running `git merge -s ours main` on `next`.** Rejected: it makes `next` the first parent, so `git log --first-parent main` would skip the C history.

## Consequences

- PRs that sync the boilerplate, and the cutover PR, **must** use a merge commit. Squashing them cuts the link to the boilerplate's history, and the next sync re-conflicts. Feature PRs into `next` are squashed.
- GitHub can't open a PR between unrelated histories. The join commit has to be created locally on a `cutover` branch first.
- Files that exist only on `main` disappear at cutover. That's why repo meta files (agent docs, glossary, ADRs) live on `next`.
- Fixes merged into `main` during the migration don't flow into `next` automatically. A cutover checklist item covers this.
