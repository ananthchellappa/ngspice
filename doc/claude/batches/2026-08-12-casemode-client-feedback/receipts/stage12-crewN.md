# Stage 12 receipt — crew N

Crew L closed acceptance criterion 5 of `doc/codex/issues/0068` on one sentence.
Its verifier falsified the sentence. This stage reproduces the falsification,
corrects the sentence everywhere it shipped, and closes the criterion for the
mode the sentence had excused.

Nothing about the diagnostic's behaviour changed: the diff under `src/` is one
comment.

## RESULT per item

### 1. The false claim — REPRODUCED and CORRECTED (four places, not three)

The sentence, as crew L wrote it under criterion 5:

> Nothing owed by tests/regression/misc: `t_unclaimed` is set only by
> `mkvnode()` (B source only) and a B card has no `line_case`, so under fold
> both guards are unreachable.

**Reproduced first**, with the verifier's deck, copied to
`<scratch>/repro/verifier.cir` and run there — never in the source tree:

```
.options noacct rshunt=1e9
V1 in 0 dc 0 ac 1
R1 in 0 1k
.noise v(MISS) V1 dec 10 1 100
.tf v(Miss) V1
```

`build-ver_50/src/ngspice --batch`, shipped code, one line in each mode:

| mode | output |
| --- | --- |
| `fold` | `Warning: no node named 'miss'; it is referenced but no card defines it` |
| `preserve` | `Warning: no node named 'MISS'; …` |
| `distinguish` | the same line twice, once for `'Miss'` and once for `'MISS'` |

With **both** pair-report guards deleted from `INPtermCaseCheck()` and the tree
rebuilt, the same three runs each add a second diagnostic about the same node:

```
Warning: no node named 'miss'; it is referenced but no card defines it
Warning: node names 'MISS' and 'Miss' differ only in case and name one node (casemode=fold)
```

and under `distinguish`, `… and name two nodes (casemode=distinguish)` beside
the two undefined-node lines. So the guards **are** reachable under `fold`, and
the claim is false in both of its halves:

- `t_unclaimed` is not set only by `mkvnode()`. `INPtermInsertRef()` has
  **seven** ~~call sites~~ **calling functions** and **ten call sites**
  (*corrected 2026-08-14 by crew Q; the line list below was always right and
  already shows ten — only the noun was wrong, here and at the three later
  mentions of "the seven call sites" in this file*): `mkvnode()`
  (`src/spicelib/parser/inpptree.c:1259`),
  `INPgetValue()` for an `IF_NODE` value (`inpgval.c:94`), and `dot_noise()`,
  `dot_tf()`, `dot_sens()`, `dot_pss()`, `dot_hb()` (`inp2dot.c:58/64`,
  `:377/382`, `:502/508`, `:685`, `:784`) — `dot_noise()`, `dot_tf()` and
  `dot_sens()` each call it twice. Receipt-only: every shipped document says
  "callers" or "calling functions", which is correct, and `0018` decision 3
  now states the ten as well.
- a dot card is a card the deck wrote, so it carries `line_case` and
  `card_spelling()` answers from it normally. The `.noise` and `.tf` cards above
  leave one entry holding `t_spelling = MISS`, `t_casetwin = Miss` **and**
  `t_unclaimed` still set — exactly the state the guard exists for.

Corrected in four places. Crew L's receipt is **not** one of them, because crew
L wrote no receipt file: `receipts/` runs stage1 → stage10 and stage 11's whole
record is the LEDGER's "Stage 11 — crew H's residuals (crew L)" section. There
was nothing to annotate in place; the LEDGER bullet is annotated instead, in the
dated-correction style `stage8-crewE.md` and `stage10-final.md` use.

| file | what changed |
| --- | --- |
| `doc/codex/issues/0068-a-node-name-spelled-two-ways-is-never-reported.md` | criterion 5's closing paragraph replaced by a dated **Correction** that quotes the false sentence, gives the falsifying deck, names the seven call sites, and states what the mutation prints. Criterion 7's last clause fixed: all three decks now carry criterion 5's cases, not two of them. |
| `doc/claude/decisions/0018-node-name-collision-report.md` (decision 2) | the "Under `fold` neither guard is reachable at all" sentence replaced; the `misc` NMPAIR case named and the four-column table pointed to. |
| `doc/claude/decisions/0018-node-name-collision-report.md` (decision 3) | "One consequence of gap 3 worth naming" is a consequence gap 3 does **not** have: the old paragraph is quoted, struck and replaced. Gap 3 itself is untouched and still true — it is about the B card, not about `t_unclaimed`. **Corrected again 2026-08-14 by stage 13**: the replacement sentence said the outer-guard mutation prints the pair line "in `fold` as in the other two modes", which is false for `distinguish` — this receipt's own both-guards measurement above is what the sentence should have quoted. |
| `doc/claude/batches/…/LEDGER.md` stage 11 P2 | the sentence struck through in place with a dated pointer to stage 12. |
| `src/spicelib/parser/inpsymt.c` `term_insert()` header | **the fourth place, not in the brief.** The comment said "Only mkvnode() does that". Replaced with the seven call sites and why the distinction decides where the fold-mode test can live. Comment only; no code changed. |

### 2. Criterion 5 actually closed — NEW CASE, kept "Met"

Measured before writing anything. With both guards deleted and the tree rebuilt:

| directory | result under the mutation |
| --- | --- |
| `tests/regression/case` | **FAIL** on `node-case-collision-report.cir` |
| `tests/regression/casedist` | **FAIL** on `node-case-collision-report.cir` |
| `tests/regression/misc` | **all 35 passed** — `fold` had no guard at all |

The `misc` count is 35 before and after: the case was added inside the deck
`tests/regression/misc/node-case-collision-report.cir`, which was already in
`TESTS`, so no `Makefile.am` change and no `autogen.sh` was needed.

That is the gap. `NMPAIR` added to
`tests/regression/misc/node-case-collision-report.cir`: two `.tf` cards naming
one node no card defines, `MISS` then `Miss`, asserting `undef5 = 1` and
`pair5 = 0` — a count, not a presence, and the undefined-node count being
exactly 1 is what proves the capture was read.

Two deck details, both deliberate:

- **two `.tf` cards, not the falsifier's `.noise` + `.tf`.** A `.noise` whose
  output node does not exist aborts the run (`Error: no data saved for Noise
  analysis`, `run simulation(s) aborted`) inside the sourced sub-deck. The
  mechanism under test — an unclaimed entry carrying a recovered spelling and a
  case twin — is identical either way, and the header says so.
- **`remcirc` after the `source`.** The `.tf` cards are analysis cards and this
  is the last sub-deck sourced, so `--batch` ran them after `.endc` and printed
  two transfer-function tables into the comparison. Dropping the circuit is
  right rather than convenient: the report under test is written at the end of
  the **parse**, which the `source` has already done.

### 3. Litter — DELETED, three files, each checked first

| file | untracked? | written by |
| --- | --- | --- |
| `preserve.txt` (repo root, 19 lines) | `git ls-files --error-unmatch` → no match | raw `preserve`-mode sweep output (`<deck>\tWarning: node names …`); referenced by no deck, no `Makefile.am` and no doc |
| `examples/xspice/table/bsim4p-2d-1.table` | no match | `examples/xspice/table/table-generator-b4-2d.sp:56`, `set outfile = "$inputdir/bsim4p-2d-1.table"` on the `mtype = 2` arm, written with `echo … > $outfile` |
| `examples/xspice/table/qinn-clc409-2d-1.table` | no match | `examples/xspice/table/table-generator-q-2d.sp:31`, same shape |

The sibling `bsim4n-2d-1.table` **is** tracked and is written by the `mtype = 1`
arm of the same generator; it is unmodified and was not touched. `git status`
shows no tracked file deleted.

### 4. Handoff — four `.out` files need `git add -f` by name

`tests/.gitignore` line 3 is `*.out`. The matching `.cir` decks are untracked
but *visible*, so a plain `git add` stages the decks and silently drops their
references — three tests that fail on a fresh clone.

| file | why |
| --- | --- |
| `tests/regression/misc/node-case-collision-report.out` | **updated by this stage** with NMPAIR's two lines |
| `tests/regression/case/node-case-collision-report.out` | stage 11's NMPAIR |
| `tests/regression/casedist/node-case-collision-report.out` | stage 11's NMDEF and NMREF |
| `tests/regression/casedist/rawfile-casemode-header.out` | crew G's, not this stage's; named so the commit does not lose it |

Added to the LEDGER as stage 12's "Handoff" table; stage 11's table is retitled
"as stage 11 left it — superseded".

## Files changed

| file | why |
| --- | --- |
| `src/spicelib/parser/inpsymt.c` | `term_insert()`'s header comment carried the same false "Only mkvnode() does that" claim; replaced with the seven call sites and the `line_case` consequence. Comment only. |
| `tests/regression/misc/node-case-collision-report.cir` | NMPAIR case added — the fold-mode guard criterion 5 never had — plus its header paragraph naming the falsified claim. |
| `tests/regression/misc/node-case-collision-report.out` | NMPAIR's two reference lines. Ignored by `tests/.gitignore`; see handoff. |
| `doc/codex/issues/0068-…md` | criterion 5's justification corrected and the four-case / four-column tables rewritten; criterion 7's clause fixed. |
| `doc/claude/decisions/0018-node-name-collision-report.md` | the same false sentence corrected in both places it appears. |
| `doc/claude/batches/…/LEDGER.md` | stage 11 P2 struck in place; stage 12 section and superseding handoff table added; log row. |
| `doc/claude/batches/…/receipts/stage12-crewN.md` | this receipt. |

## Tests

`make -C build-ver_50/tests/regression/<dir> check`, cached `*.log`/`*.trs`
deleted before each run.

## RED

Mutation, not a weakened assertion: the guard already exists, so the test's
falsifiability had to be shown against the code with the guard removed.

`if (t->t_unclaimed) continue;` → `if (0 && t->t_unclaimed) continue;`, rebuilt,
new deck run with the hand-written reference in place:

```
--- node-case-collision-report.out_tmp
+++ node-case-collision-report.test_tmp
@@ -21,4 +21,4 @@
 undef5 = 1.000000e+00
-pair5 = 0.000000e+00
+pair5 = 1.000000e+00
FAIL: node-case-collision-report.cir
```

One line of diff, and it is the pair count. The reference was written by hand
before the case was ever run green, so it is not a regenerated `.out`.

Each guard deleted on its own, rebuilt each time, all three decks re-run:

| mutation | `misc` | `case` | `casedist` |
| --- | --- | --- | --- |
| shipped | PASS | PASS | PASS |
| outer `if (t->t_unclaimed) continue;` deleted | **FAIL** | **FAIL** | **FAIL** |
| inner `!u->t_unclaimed` deleted | PASS | PASS | **FAIL** |
| both deleted | **FAIL** | **FAIL** | **FAIL** |

The `misc` row tracks the `case` row exactly, and that is expected rather than
redundant: `fold` can only reach the outer guard, because the inner one sits in
the chain scan that `INPtermCaseCheck()` returns from before it starts in any
mode but `distinguish`. What `misc` adds is the *path* — the spelling comes back
through `card_spelling()` and `line_case` rather than through the token, which
is the half of the mechanism crew L's claim said could not exist.

The three MUTANT edits were reverted with `Edit`/a scripted exact-string swap,
never with `cp -p`; `grep -c MUTANT src/spicelib/parser/inpsymt.c` → `0`, and
the file was rebuilt from the reverted text before the green runs.

## GREEN

Shipped code, rebuilt, caches cleared:

```
tests/regression/misc      All 35 tests passed
tests/regression/case      All 118 tests passed
tests/regression/casedist  All 30 tests passed
```

## Suite status

`make -C build-ver_50 check`, every `*.log` and `*.trs` under
`build-ver_50/tests` deleted first: **315 PASS, 0 FAIL**, `EXIT=0`. Identity
lint 264 comparisons + selftest 11, baseline matches. Same 315 crew L reported —
expected, because the new case went into a deck already in `TESTS`, so the test
count did not move.

**A first full run, before that one, reported 1 FAIL and it was not a
regression.** `tests/regression/pipe/unset-rawfile-option-key.cmd` failed on
`ERROR: harness broken, the raw this build wrote carries no readable Option:
key`. The concurrent crew relinked `build-ver_50/src/ngspice` **at 18:20:22,
inside that run's window** (`src/frontend/rawfile.c` written 18:20:09), so the
deck was measured against a binary being replaced under it. Re-run alone against
the settled binary: **PASS**. `tests/regression/{misc,case,casedist}` re-run
against that same settled binary: 35 / 118 / 30, all passing. Recorded rather
than dropped, because the first run's number would otherwise look like this
stage's.

Neither file involved in that failure is this crew's: `rawfile.c`,
`variable.c`, `options.c` and `tests/regression/pipe/` all belong to the crew
working concurrently, and this stage's only `src/` diff is a comment in
`inpsymt.c`.

## Residuals

- **Crew L left no receipt file.** `receipts/` runs `stage1-*` … `stage10-final`
  and `stage9-crewF`; stage 11's entire record is the LEDGER section. Item 1 of
  this stage's brief asked for the receipt to be annotated in place; there is no
  file to annotate, so the LEDGER bullet carries the dated strike instead. If a
  stage 11 receipt is written later, the corrected sentence must not be copied
  into it from `0018`'s or `0068`'s history.
- **Not fixed, and not this crew's to fix: the same class of claim is still
  under-tested elsewhere.** Criterion 5 was closable only because a mutation was
  run. Nothing in the suite reaches the pair report at all outside the three
  guard decks — `tests/bin/check.sh` captures stdout while the report goes to
  `cp_err`, and its `egrep -v` filter then drops every line containing
  `Warning`. That is stage 11's own finding and it still stands; the three decks
  only work because they re-enter the parse under `>&`.
- **The `misc` deck now depends on `remcirc`.** If `remcirc` ever stops dropping
  the current circuit, the deck fails with two transfer-function tables in the
  diff rather than silently — an acceptable failure mode, but named so the next
  reader knows which line to look at.
- **Untracked byproducts left in `examples/`, not this crew's.** 16 files, all
  written 16:11–17:11 today by another crew's sweep, e.g.
  `examples/control_structs/s3046.s2p`, `examples/measure/meas.out`,
  `examples/plot/py*.{py,data}`. Left alone: they are not in this stage's brief
  and the crew that made them may still be reading them. No tracked file under
  `examples/` is modified.
- **One tracked file left the modified set during this stage, by the other
  crew.** `tests/regression/pipe/postcoms-keyword-case.cmd` was ` M` when this
  stage opened and is clean now; the other 30 are unchanged in membership. Not
  touched here — noted so it is not read as this stage's doing.
- **Changes wanted outside this crew's files: none.** The correction touched
  only `inpsymt.c`, `tests/regression/misc/`, `0068`, `0018`, the LEDGER and
  this receipt.
