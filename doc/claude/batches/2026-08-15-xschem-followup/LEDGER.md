# Ledger: the xschem follow-up, after 0070

Driver session, 2026-08-15, branch `ver_50`. Plan is `PLAN.md`; receipts are
under `receipts/`. One crew per item, handed off one at a time. The driver
did no item work.

Starting state: HEAD `7b5884249`, working tree clean of tracked changes,
`make check` green at `eb0b96c8c` (320 PASS, identity lint baseline matched).

| item | crew | outcome | commit |
| --- | --- | --- | --- |
| 1 — correct RESPONSE.md and the guide for 0070 | A | **done** — three stale claims corrected in both documents, key-matching advice kept, derived-plot behaviour added | `4a042f0f4` |
| 2 — move the Q2/Q3/Q4 answers into the guide | B | **done** — three new §9 subsections, all re-measured, decks in the guide re-extracted and re-run | `9a848ff4f` |
| 3 — upstream cp_remvar submission, ready to send | C | **ready, unsent** — re-validated against `2f9c8ad47`, two false claims in the mail corrected, sending is the owner's | `4df3482c4` |
| 4 — file the `-r` writer's missing header line | D | **filed** — `doc/codex/issues/0071`, measured; timing ruled out as the cause, the arm is simply absent | `3a194ad8e` |
| 5 — 0064's scope, measured, for the owner | E | **measured, undecided by design** — 30 deck shapes, two mechanisms separated, three options costed | `76f70dfb7` |

## Notes

Five crews, five items, one at a time. No `src/` or `tests/` change in the
whole batch, so `make check`'s state is the one `eb0b96c8c` left. Each crew
re-measured rather than carrying a number over, and three of them falsified
something they had been handed.

- **Item 1** found that a plot *derived* from a loaded one — `linearize`,
  `cutout`, `fft`, `psd`, `spec` — also records no mode, and put that in both
  documents. It left `doc/codex/issues/0061` alone with its two stale claims,
  on the ground that 0061 is a closed record and 0070 supersedes it. That
  judgement is the driver's to endorse and is recorded here as endorsed.
- **Item 2** found that `doc/codex/issues/0069`'s Resolution prints a guard
  deck with no netlist lines in it, and names three decks that exist nowhere
  in the tree. The guide's block is complete and was re-extracted and re-run.
  Correcting 0069 is not done and wants a crew of its own.
- **Item 2** also turned up a pre-existing abort while running 0069's listing
  verbatim: `.op` with no netlist aborts on an assertion at `dotcards.c:225`,
  rc=134, identically on stock. Unfiled, needs code, details in receipt 02.
- **Item 3** falsified two claims in a mail that had been called ready: the
  per-patch split was never measured and was wrong in both directions, and a
  test-directory count reproduced by no count. It also regenerated `0001`'s
  patch because its `index` line named this branch's blobs, which would have
  broken `git apply -3` for a maintainer. **Nothing was sent.**
- **Item 4** ruled out the explanation everyone would have reached for: the
  `.control` block completes before `fileInit()` runs, so the missing header
  line is not a timing problem, the arm is simply absent.
- **Item 5** falsified 0064's own advice to the client. "Expect n, or n+1 when
  n is 1" is wrong: two explicit names on a two-save `.op` plot write three
  variables, and under `preserve`, `distinguish` and stock the duplicate pair
  carries a byte-identical name — so the filter-by-name defence the client was
  given does not reach it. Corrected in the issue.

## Waiting on the owner

1. **Send the upstream mail** (`doc/claude/upstream/EMAIL-cp_remvar.txt`, both
   patches attached, to `ngspice-devel@lists.sourceforge.net`). It is the only
   thing that unblocks the `casemodewrite` default flip.
2. **0064's scope**: wildcards only (low risk, enough for the client's decks),
   the wider shape (high risk, and its invariant as worded is unachievable),
   or both in sequence. Receipt 05 costs each.
3. **Whether 0064's second mechanism gets its own issue number.** Deliberately
   not filed.
4. **0071's own question**: whether the `-r` writer should carry the line at
   all, given that its header has been byte-stable since spice3.
5. **0069's Resolution** wants correcting, and the `dotcards.c:225` abort
   wants filing.
