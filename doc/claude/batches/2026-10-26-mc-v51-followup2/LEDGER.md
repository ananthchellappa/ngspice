# Batch ledger — ver_51 MC follow-up 2

**Branch** `ver_51`. **HEAD at start** `f0c8c25cd`. **Baseline** 353 PASS /
0 FAIL. Parent: `../2026-10-26-mc-v51-followup/`. Receipts: `receipts/`.

The driver chose the scope on the owner's standing instruction to make the
technical calls itself.

## Items

| # | item | crew | outcome | commit |
| --- | --- | --- | --- | --- |
| 1 | typeless `.model` segfault, `doc/codex/issues/0074` (two crashes, both fixed at the root) | 01 | landed, review pass, 360 PASS / 0 FAIL | `b70e5123e`, `8a671e1c0` |
| 2 | P1 hardening: identical duplicate models carry the TC; free order; wrong-scope mutation proof (it also strengthened the subckt noise deck) | 02 | landed, review pass, 361 PASS / 0 FAIL | `3a904f87d`, `c2b63479e` |
| 3 | "Cannot convert option seed" once per session; probe `strncmp` fix (not run) | 03 | landed, review pass, 362 PASS / 0 FAIL | `f8f0f60d5`, `eecdc5a97` |
| 4 | upstream patches refreshed; `p4-crash/` added; **not sent** | 04 | landed | `e9e1e83ce` |
| 5 | close | 06 | closed, 362 PASS / 0 FAIL | `711af805e` |

The driver checked the 11 lines `e9e1e83ce` deleted from `upstream/README.md`.
They are rewritten lines, not dropped sections. "Nothing here has been sent" is
still present.

## Notes

*Declined, with reasons:*
- Resetting the sticky `has_if` static is a separate bug of the `ngSpice_Reset`
  kind. It no longer causes a crash, and it skips unused-model pruning for later
  decks in a session.
- Collecting P1's R→B models after `.if` evaluation means changing P1's
  ordering. A typeless card in an untaken `.if` branch is still reported when a
  converted R is present.
- `p4-crash/` ships without the six exit-status decks, because upstream has no
  `check_status.sh`.

## Waiting on the owner

1. Whether to send `../2026-10-26-mc-v51-followup/upstream/` (p1, p2, p3,
   p4-crash).
2. Push `ver_51`; the owner pushes manually.
