# Batch ledger — ver_51 MC follow-up

**Branch** `ver_51`. **HEAD at start** `5d88401af`. **Baseline** 347 PASS /
0 FAIL (the close of `2026-10-24-mc-v51`).
Parent batch: `../2026-10-24-mc-v51/`. Receipts: `receipts/`.

The owner asked the driver to make the technical calls itself. The brief was:
fix every open minor review issue and deliberate gap from the parent batch's
`receipts/06-close.md` that is a real defect, a misleading message, a weak or
flaky test, or a portability break, and decline the rest with a reason. Also
prepare unsent upstream patches for P1–P3.

## Items

| # | item | crew | outcome | commit |
| --- | --- | --- | --- | --- |
| 1 | P1 fixes: warning text, model-list leak, subckt noise deck, whitespace | 01 | landed, review pass, 348 PASS / 0 FAIL | `82d8e7018`, `da9023b75` |
| 2 | P2 fixes: warning wording and spelling, flaky check, "Multiple option seed" once per session | 02 | landed, review pass, 350 PASS / 0 FAIL | `e1db1d3df`, `3db79a2e3` |
| 3 | P3 fixes: glibc-only decks SKIP on other libcs (`.requires`, exit 77) | 03 | landed, review pass, 351 PASS / 0 FAIL | `73733be3f`, `0dc054bb5` |
| 4 | P4 fixes: header tests, `visualc` project entries | 04 | landed, review pass, 353 PASS / 0 FAIL | `efb8a7528`, `f6353040f` |
| 5 | upstream patches P1–P3, prepared and **not sent** | 05 | partial: patches done; the per-item `REPORT.md` files were refused by the harness | `f8ac6e67c`, `b920d3caf` |
| 6 | close | 06 | closed, 353 PASS / 0 FAIL | `8e5d0b90f` |

**Final `make check`**: 353 PASS / 0 FAIL, XFAIL/SKIP/ERROR zero, `make` exit
0. Both lint specs match the baseline. The owner's two uncommitted edits are
untouched.

## Notes

*Declined, with reasons (receipts 01–04).*
- No-model R drops dtemp/temp/tc=: PLAN's byte-for-byte rule for cards with no
  model token.
- Flicker noise: the rewrite's noise branch carries no DC current.
- tnom≠27: owner decision 3.
- Identical duplicate `.model` cards: would need a runtime-vs-runtime
  comparison, which the parent PLAN's standing constraint forbids.
- 32-bit return rounding (4d): untestable here.
- Unsynchronised counter (4e): needs a thread-safe RNG, outside scope.
- Shared-library items 1d (run), 2a, 3c, 4a, 4f: the owner deleted the shared
  build tree.

*Item 5 correction by the driver.* `upstream/README.md` claimed `REPORT.md`
files the harness had refused. The driver corrected it: the commit messages are
the reports. The driver also added the note that the `sharedspice.c` hunks in
P2/P3 have not been run.

*Found, not caused by this batch.* A `.model` with no type, in a deck with
`.if`, segfaults (exit 139), on the pre-P1 binary as well.

## Waiting on the owner

1. Whether and when to send `upstream/p1–p3`. Sending is outward-facing.
   Before P2 or P3 goes out, the driver recommends a one-off library build to run
   the reset probe, then deleting the build again.
2. Push `ver_51`.
