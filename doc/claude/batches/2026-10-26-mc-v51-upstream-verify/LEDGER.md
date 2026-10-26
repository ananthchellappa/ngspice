# Batch ledger — verify the upstream patches before submission

**Branch** `ver_51`. **HEAD at start** `87e388de8`. Upstream target:
`pre-master-48` at `a1148c969` (2026-10-26), 262 commits past the
`pre-master-47` base the patches were made on. Receipts: `receipts/`.
Paste-ready texts: `SUBMIT.md`.

## Items

| # | item | outcome | commit |
| --- | --- | --- | --- |
| 1 | apply, build and `make check` the patches on current upstream; libngspice probe | done; **P3 dropped** (fixed upstream, bugs #872/#873); p2 rebased; probe 12/12 | `700031c43` |
| 2 | fix review findings in the patch texts; draft the submission | done | `882ffde4d` |
| 3 | dead-branch typeless `.model` still exited with a converted R present: fixed in the fork (RED first) and in `p4-crash/on-p1` | done; fork 364 PASS / 0 FAIL, upstream + series 63 PASS / 0 FAIL; independent check: **ready** | `3beb0f2a1`, `62c187b48` |

Send order: p1, then p4-crash/on-p1, then p2.

## Notes

- Upstream `a1148c969` doesn't build from git: `tests/xspice/analog/` is
  referenced but missing. The builds used a local revert of two lines, never
  committed. The forum post mentions it.
- The driver's brief for item 2 wrongly said the owner had decided to keep the
  `Co-Authored-By` trailer. The owner hadn't. That decision is still open.

## Waiting on the owner

1. Keep or drop the AI disclosure: the trailer and the forum sentence go
   together.
2. Is `ananth.chellappa@outlook.com` the address to publish?
3. Post and file (see `SUBMIT.md`), then tell the driver the ticket numbers.
