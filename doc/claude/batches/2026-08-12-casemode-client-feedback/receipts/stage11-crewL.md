# Stage 11 receipt — crew L (RECONSTRUCTED, not written by crew L)

> **Read this line first.** Crew L wrote no receipt file. This one was
> assembled on 2026-08-14 by crew Q, after the fact, from the two records crew
> L actually left: the `LEDGER.md` section "Stage 11 — crew H's residuals (crew
> L)" and the parts of `doc/codex/issues/0068` and
> `doc/claude/decisions/0018` that stage 11 wrote. It exists so that
> `receipts/` has no hole between `stage10-final.md` and `stage12-crewN.md`,
> **not** because anyone recovered crew L's session.
>
> Every other receipt in this directory is first-hand: its crew ran the
> commands it reports. This one is not. Nothing below was re-run by crew Q
> except where a line says so explicitly. Treat an unmarked number here as a
> quotation from the ledger, not as a measurement.

## Provenance of each section

| section | where it comes from |
| --- | --- |
| RESULT per item | `LEDGER.md` § "Stage 11 — crew H's residuals (crew L)", bullets P1–P3 and the owner-question bullet |
| Files changed | inferred from the shipped text of `0018`, `0068` and the two guard decks; marked where it is inference |
| Tests / suite | `LEDGER.md` stage 11 bullets and the stage 11 Log row |
| RED and GREEN | **partly unrecoverable** — see "What could not be reconstructed" |

## RESULT per item

### P1 — decision 0018's gap 3 was wrong about its own cause — CORRECTED

Crew H's `0018` decision 3 item 3 read "a spelling inside an arithmetic
expression" and blamed `card_word_boundary()` for leaving `+`, `-`, `*` and
`/` inside words. Crew L falsified both halves:

- `(` and `)` **are** boundaries, so `V(VFreq)*V(VTime)` is not one word.
  `card_spelling()` and `card_word_boundary()` copied verbatim into a
  standalone harness and handed the cited line of
  `examples/various/FFT_Leakage.cir` return `VFreq` and `VTime`.
- The silence survives with no arithmetic anywhere: `V1 Mid 0 dc 1` /
  `R1 Mid 0 1k` / `B1 mid 0 I = 1m` puts the second spelling in the B card's
  own node field and `fold` is still silent while `preserve` reports.

The real cause is `inp_bsource_compat()` (`src/frontend/inpcom.c`): it comments
every B card out (`*(card->line) = '*'`) and inserts a replacement built by
`insert_new_line()`, which leaves `line_case` NULL. So **no node named anywhere
on a B card** can supply a spelling under `fold`, terminals included.
`inp_compat()` does the same to an `E`, `G`, `R`, `C` or `L` card whose value
is an expression. Crew L measured one deck per shape with a plain linear `E`
card as the control that still reports, and sized the gap at **236 B cards in
53 of the 875 decks** under `tests/` and `examples/`.

A **fourth gap** was found while re-deriving: `[` and `]` are not boundaries
either, so an XSPICE `a` card written `a1 [in1 in2] [out] m` yields no spelling
where `a1 [ in1 in2 ] [ out ] m` yields one. No corpus deck is affected;
recorded in `0018` decision 3 gap 4 rather than fixed.

*Re-measured by crew Q, 2026-08-14:* the six-row `B`/`E`/`G`/`R` table and the
gap-4 spaced/tight pair both reproduce exactly on this tree. See
`receipts/stage13-crewQ.md`.

### P2 — acceptance criterion 5 was Met with nothing testing it — GUARDED

Confirmed by mutation: with both `t_unclaimed` guards deleted from
`INPtermCaseCheck()` and the tree rebuilt, `tests/regression/case` (118) and
`tests/regression/casedist` (30) both passed in full. Three cases added, each
asserting a **count** rather than a presence:

| case | deck | asserts |
| --- | --- | --- |
| NMDEF | `tests/regression/casedist/node-case-collision-report.cir` | `In` defined, `in` created by a later `V()` — near miss ×1, pair ×0 |
| NMREF | same deck | the same two names in the other interning order — near miss ×1, pair ×0 |
| NMPAIR | `tests/regression/case/node-case-collision-report.cir` | two spellings of a node no card defines — undefined-node ×1, pair ×0 |

Each guard was then deleted on its own and rebuilt, which is what shows no case
is redundant: outer-only fails NMDEF and NMPAIR and leaves NMREF passing;
inner-only fails NMREF alone. Two cases under `distinguish` rather than one
because the bucket chain is prepended to, so interning order decides which
guard is asked.

**Crew L's closing sentence on this item is false and is struck in the
ledger.** It read, in substance: *nothing is owed by `tests/regression/misc`,
because `t_unclaimed` is set only from a B source expression and a B card has
no `line_case`, so under `fold` the guards are unreachable.* Stage 12 (crew N)
falsified it — `mkvnode()` is one of seven calling functions of
`INPtermInsertRef()`, and a `.tf` card carries `line_case` — and added the
`misc` NMPAIR case that `fold` was missing. The false sentence is reproduced
here only as the thing that was struck; it must not be quoted from this receipt
as a finding.

### P3 — the gitignored `.out` files — RECORDED

`tests/.gitignore` line 3 is `*.out`; the 117 / 28 / 34 references already
tracked in `case` / `casedist` / `misc` were force-added. Crew L's handoff
table named three files needing `git add -f` by name. It is superseded by stage
12's four-file table in the ledger.

### The owner's question — MEASURED, and it amends decision 1

Under `preserve` and `distinguish` the report is once per colliding pair, and
after subcircuit expansion each instantiation carries its own pair. Spelling
the `nand2` body's one internal node `sig3` as `SIG3` on a single card:

| deck | `nand2` instances | warning lines |
| --- | --- | --- |
| `examples/klu/Circuits/85/c1355/c1355.net` | 416 | 416 |
| `examples/klu/Circuits/85/c5315/c5315.net` | 454 | 454 |
| `examples/klu/Circuits/85/c7552/c7552.net` | 1028 | 1028 |

One line per instantiation, exactly. A formal *pin* spelled two ways is silent
in every mode — it is a body's internal node, which the expander scopes rather
than substitutes, that multiplies. Written up as
`doc/codex/issues/0068` § "Open question for the owner" with three options and
their costs; `0018` decision 1 gained a dated correction saying it costed the
wrong thing, and decision 5 gained item 6 so the earlier numbering stays valid
for the four records that cite it.

### The sweep, re-run independently

875 decks × 3 modes, re-measured rather than taken from crew H's receipt:
`fold` 28 lines / 19 files, `preserve` 38 / 25, `distinguish` 29 / 16 — crew
H's table reproduces. `fold` and `distinguish` are each a strict subset of
`preserve`. **Ten** `preserve` reports have no `fold` twin, not the nine the
issue said: four gap 1, three gap 2, three gap 3.

## Files changed

Reconstructed from the shipped text, not from a diff crew L recorded.

| file | why | confidence |
| --- | --- | --- |
| `doc/claude/decisions/0018-node-name-collision-report.md` | decision 3 gap 3 rewritten with the real cause and the six-row table; gap 4 added; decision 1's "Corrected 2026-08-14" cost paragraph; decision 5 item 6 | high — the text names stage 11's findings |
| `doc/codex/issues/0068-…md` | criterion 5's four-case and four-column tables; "Stated gaps"; "Open question for the owner"; the re-run sweep numbers; the nine tracked decks | high |
| `tests/regression/casedist/node-case-collision-report.cir` + `.out` | NMDEF and NMREF | high |
| `tests/regression/case/node-case-collision-report.cir` + `.out` | NMPAIR | high |
| `src/spicelib/parser/inpsymt.c` | the ledger states stage 11's `src/` diff is **comments only**; the `card_word_boundary()` header carries gaps 3 and 4 in crew L's corrected form, so that is the likely site | inference |
| `doc/claude/batches/…/LEDGER.md` | the stage 11 section, the nine-deck list, the handoff table, the log row | high |

## Tests

Per the ledger: `make -C build-ver_50 check` from cleared caches — **315 PASS,
0 FAIL**; identity lint 264 comparisons, baseline matches.

## RED and GREEN

The RED evidence stage 11 left is the mutation matrix quoted under P2 — each
`t_unclaimed` guard deleted separately, rebuilt, and the guard decks re-run,
with outer-only and inner-only failing different cases. Crew Q re-ran that
matrix on 2026-08-14 and it reproduces (`receipts/stage13-crewQ.md`).

No per-run transcript, diff hunk or command line survives from crew L's
session.

## What could not be reconstructed

- Crew L's own commands, timings, and the order it did the work in.
- Its RED/GREEN diff hunks. The `.out` references it wrote are on disk and
  the decks pass, but whether each was hand-written before first green — the
  thing a receipt exists to attest — is **not** evidenced anywhere.
- Its residuals list, if it had one. Stage 12's residuals are crew N's and
  cover only what crew N found.
- Any item crew L worked and did not put in the ledger. If such an item exists
  there is no trace of it, and this receipt cannot say there is none.

## Residuals

- **This file is testimony about a session nobody has.** If crew L's work is
  ever questioned, the ledger section and the shipped text are the evidence;
  this receipt adds no independent confirmation of anything.
- **P2's struck sentence.** Corrected by stage 12 in `0068`, `0018` (twice) and
  the `term_insert()` comment, and corrected again by stage 13 where stage 12's
  replacement over-generalised. Anyone reading stage 11 for the fold-mode story
  must read stage 12 and stage 13 with it.
