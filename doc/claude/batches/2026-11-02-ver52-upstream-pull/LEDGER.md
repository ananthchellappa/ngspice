# Batch ledger — ver_52, upstream fixes on top of ver_51

**Branch** `ver_52`, off `ver_51` at `df2194696`, worktree `/home/analog/dev/ngspice-ver_52`,
local only. **Upstream** `refs/upstream/pre-master-48` at `ac2798364`.
Plan: `PLAN.md`. Receipts: `receipts/`.

## Items

| # | group | outcome | make -k check | review |
| --- | --- | --- | --- | --- |
| 00 | baseline | build tree, `measure/` suite | 363 PASS, 1 FAIL | — |
| 01 | core, 13 picks | landed; fix `27d02928d` (`.sp` meas segfault from the picks) | 364, 1 | 1 blocking, fixed; pass |
| 02 | binning | landed; fix `0f763b135` (outer max, float pruner) | 365, 1 | pass |
| 03 | measure, 6 picks | landed; fix `c957569c9` (2-point deriv) | 366, 1 | 1 blocking, fixed; pass |
| 04 | BJT, 5 picks | landed | 367, 1 | pass |
| 05 | diode, MOS2, HFET, 9 picks | landed; fix `a291cdc7f` (`DIOunsetup`) | 368, 1 | 1 blocking, fixed; pass |
| 06 | VBIC, VDMOS, 6 picks | landed | 368, 1 | pass |
| 07 | OSDI, narrow, 27 of 29 | landed; 3 fork fixes; `58e4cf26b` declined, `af240ba02` reverted | 370, 1 | pass |
| 08 | seed, fork-written | landed | 375, 1 | pass |
| 09 | close | 91 commits at close | 375, 1 | — |

## Notes

*The one FAIL.* `resource-keyword-case.cmd` fails in a clean checkout because it needs
the owner's uncommitted edit of that test. ver_51's "364 PASS / 0 FAIL" was measured
with that edit in the tree. The driver's PLAN gate of 0 FAIL was wrong on this point.

*Driver's call on the close's list.* R1–R6 and items 6, 30, 39 and 5 of receipt 09 are
fixed in a follow-up (groups 10, 11). `efe72da1e`'s `-x` line names a wrong hash; the
right one is `a1148c9699f58c32680344ab876ac459d8bb126e`. Not rewritten: every receipt
quotes these hashes, and this line records the correction.

*Attribution.* Clean upstream picks keep upstream's message with no Claude trailer, since
they are not co-authored. Hand-resolved and fork-written commits carry it.
