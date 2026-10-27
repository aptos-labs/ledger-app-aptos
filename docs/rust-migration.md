# Rust migration: branching rules and cutover runbook

Why this approach was chosen: [ADR-0001](adr/0001-rust-rewrite-on-unrelated-history-branch.md). Terms are defined in [GLOSSARY.md](../GLOSSARY.md).

## Branches

| Branch | Contents | Accepts | Lifetime |
|---|---|---|---|
| `main` | v1 (C) until cutover, then v2 | Until cutover: bug/security fixes, Ledger-required changes (new devices, SDK/guideline bumps), syncs from LedgerHQ/app-aptos. **No new features.** | Permanent |
| `next` | v2 (Rust), descended from the boilerplate's history | Feature ports, boilerplate syncs | Deleted after cutover |

aptos-labs/ledger-app-aptos is the source of truth for v2. Ledger reviews and deploys from our tags. Get Ledger's agreement on this before cutover.

## Bootstrap (one-time)

```sh
git remote add boilerplate https://github.com/LedgerHQ/app-boilerplate-rust.git
git fetch boilerplate
git switch -c next boilerplate/main
git submodule update --init
# First commit on next: meta files (CLAUDE.md, docs/, GLOSSARY.md, .claude/skills, .agents),
# keeping the boilerplate's `@ledger-app-ai-instructions/CLAUDE.md` import in our CLAUDE.md.
# Add `next` to the `push.branches` filter of every workflow in .github/workflows/.
git push -u origin next
```

Then configure GitHub:

- **Repository settings:** allow both squash merging and merge commits.
- **`next` protection:** PR required, no approval required, the boilerplate's CI checks required (build, ragger standalone tests, swap tests, lint, guidelines enforcer, spell check, Python checks), no force-push, no deletion.
- **Milestone "v2 parity":** one issue per v1 feature to port.

## Rules during the migration

- **Feature PRs into `next`** are **squash-merged**. Each one ports the v1 functional tests for that feature into `tests/standalone/`, and they become required checks right away.
- **Boilerplate syncs** happen on demand, only when an upstream change is needed: an SDK bump, a CI or guideline change, or a Ledger requirement. Branch `sync/boilerplate-<short-sha>` from `next`, run `git merge boilerplate/main`, and open a PR. Always use a **merge commit**, never squash.
- **v1 fixes on `main`** are ported to `next` by the author or the team, informally. The cutover checklist has a backstop.
- **Tags:** no `X.Y.Z` tags on `next`, because `release.yml` publishes a GitHub release on any of them. Pre-releases, if needed, use `2.0.0-alpha.N`.
- Meta files (agent docs, glossary, ADRs) are maintained on `next` only.

## Cutover checklist

- [ ] Every issue in the "v2 parity" milestone is closed.
- [ ] Every v1 functional test file and the swap tests are ported to `next` and pass on nanox, nanos+, stax, flex, and apex_p. Every snapshot difference from v1 is intentional and was reviewed.
- [ ] **v1 fix audit:** every commit on `main` since `next` was created has been reviewed and is either ported to `next` or recorded as not relevant. List them with:
      `git log --oneline --no-merges --since=<date next was created> origin/main`
      (the two histories share no merge base, so filter by date instead of using a range).
- [ ] Ledger has agreed that v2 releases come from aptos-labs tags.
- [ ] `Makefile`/`Cargo.toml` version is `2.0.0`.

## Cutover runbook

```sh
git fetch origin
git tag v1-final origin/main && git push origin v1-final

git switch -c cutover origin/main
git merge --allow-unrelated-histories --no-commit -s ours origin/next   # record next as 2nd parent
git read-tree -u --reset origin/next                                    # tree becomes exactly next's (C removed)
git submodule update --init
git diff --quiet origin/next && echo "tree matches next"
git commit -m "Cutover: replace C v1 with Rust v2 (join next into main)"
git push -u origin cutover
```

1. Update `main` branch protection: change the required checks from the C job names to the Rust job names, and require **at least 1 approval**.
2. Open a PR from `cutover` into `main` and merge it with a **merge commit**.
3. Tag `2.0.0` on the merge commit. `release.yml` publishes the release.
4. Delete `next`.
5. Remove `next` from the workflows' `push.branches` filters.

## After cutover (v2.1+)

- Normal development on `main`: PR required, **at least 1 approval**, Rust CI checks required, no force-push, no deletion.
- Feature PRs are squash-merged.
- Boilerplate syncs target `main` the same way as before: a `sync/boilerplate-<short-sha>` branch, merged with a merge commit.
- The C code stays reachable through history and the `v1-final` tag. Cut a `v1` branch from that tag only for an emergency.
