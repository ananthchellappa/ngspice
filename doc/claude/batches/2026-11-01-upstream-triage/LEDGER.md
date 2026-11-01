# Batch ledger — triage of upstream pre-master-48 for ver_51

**Upstream tip** `68b0d57b3` (2026-11-01). **Fork** `ver_51` at `e66057427`.
Merge base `72620328a`. Read-only: nothing from upstream is on `ver_51`.
Trigger: the owner asked what is worth pulling after ticket #136 was closed by
upstream's own commit `95d7f2c2b`.

## Items

| # | item | outcome | receipt |
| --- | --- | --- | --- |
| 1 | classify 212 upstream commits | 15 PULL, 42 MAYBE, rest skip, feature or NO | `receipts/01-triage.md` |
| 2 | run `95d7f2c2b` on Monte Carlo loops | agauss loops fine; 2 regressions; keep ours, adapt 2 pieces | `receipts/02-seed.md` |

## Driver checks

- Re-ran both seed decks on `a1148c969` and `68b0d57b3`: results as the receipt says.
- Replayed the 15 PULL commits with `git merge-tree` as a chain on `ver_51`: all clean.
- sky130 cards use `version = 4.5` (456 of 471 version settings) and `inpdomod.c`
  maps that to BSIM4v5, so `f961c98d0` does reach them.
- Not built or run with any PULL commit applied. Effects in receipt 01 are reasoned
  from code, except the bin comparison, which used the existing fork binary.

## Waiting on the owner

1. Whether to pull the 15 commits. Some change results.
2. Whether to post the reply to Holger Vogt on #136.
3. Whether to take on `940bb50fa`, which reopens the xschem log contract.
