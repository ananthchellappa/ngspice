# Decision 0006 — deck coverage for the three case near-miss diagnostics, `doc/codex/issues/0036`

## Status

Accepted, 2026-08-11, branch `ver_50`. Closes `doc/codex/issues/0036` for all
three diagnostics: `INPtermCaseCheck()`
(`doc/claude/decisions/0002-deferred-node-resolution-check.md`),
`report_bridge_case_miss()` and `EVTnode_case_check()`
(`doc/claude/decisions/0003-event-node-near-miss.md`). It re-decides nothing
those records settled: every diagnostic keeps its text, its condition and its
mode guard, and no computed value moves in any mode.

## Context

`0001-distinguish.md` decision 2 was corrected on 2026-08-11 to say that a
deck *can* assert on a diagnostic, because `>&` sets `cp_err` as well as
`cp_out` (`src/frontend/streams.c:156`). That correction is right about the
redirect and was wrong about these three, for a reason nobody had checked:
**they did not write to `cp_err`.** All three used a literal
`fprintf(stderr, ...)`, so the redirect could not see them. Measured at
`8557994bb`, a wrapper deck that sources the circuit under `>&` captured the
sourced parse's `cp_out` — the `Circuit:` line, 22 bytes — and nothing else,
while the warning went to the terminal.

That is the finding, and it is why this issue needed a production change after
all. The rest of the work is three decks.

---

## Decision 1 — the three diagnostics are written to `cp_err`

**Decided: `fprintf(stderr, ...)` becomes `fprintf(cp_err, ...)` at
`src/spicelib/parser/inpsymt.c:156`, `src/xspice/evt/evtcheck_nodes.c:934` and
`src/xspice/evt/evttermi.c:239`. Nothing else changes.**

`cp_err` is the frontend's error stream and is `stderr` in every binary that
parses a deck: `src/main.c:916`, `src/sharedspice.c:921`, `src/tclspice.c:2486`,
each set before any netlist is read, and none of them ever reassigns it. So
the change is invisible on every existing deck and in every mode — the same
bytes reach the same file descriptor — and becomes visible only while a
control-language redirect is in force, which is the deck this record adds.

It is also the direction that removes an inconsistency rather than creating
one. `vec_warn_case_near_miss()` (`src/frontend/vectors.c`), which is decision
2's fourth site and the one written first, already uses `cp_err`; `stderr` at
the other three was the odd spelling. `src/spicelib/parser/inpapnam.c` and
`src/spicelib/analysis/cktop.c` are the precedent for `cp_err` outside the
frontend, and all three files already include `ngspice/fteext.h`, which
reaches `cp_err` through `cpdefs.h` → `cpextern.h`, so no include moves and no
line moves: the edit is one six-character token per file, in place.

Rejected: **leaving the diagnostics on `stderr` and capturing them some other
way.** The only other way is a nested `ngspice` process started with `shell`
and a shell-level `2>`, which needs the deck to know a relative path to the
binary under test. That path is a property of the build tree, not of the deck,
and it would be silently wrong in an installed or `distcheck` run — the deck
would then pass by capturing nothing.

Rejected: **`Notice:` on `stdout`**, which `check.sh`'s filter would let
through. `0002` decision 3 already rejected it, and the reason still holds:
ngspice's `stdout` is where the answer goes, and testability is not worth
putting a diagnostic in the data stream.

## Decision 2 — the capture is taken by `source`, from a wrapper deck

**Decided: each new deck writes the circuit it is about with `echo`, sources
it from inside its own `.control` block with `>&`, and scans the capture.**

None of the three diagnostics is a command. `INPtermCaseCheck()` and
`EVTnode_case_check()` are called from `if_inpdeck()`
(`src/frontend/spiceif.c:206`, `:215`) and `Evtcheck_nodes()` from `:185`, all
of them before the first line of any `.control` block runs, so there is
nothing in the deck itself to attach a redirect to.

`source` re-enters `inp_spsource()` and therefore `if_inpdeck()`, so the whole
parse — including all three checks — runs again, and `inp_spsource()` saves
and restores `cp_curout`/`cp_curerr` around it (`src/frontend/inp.c:629-634`,
`:1305-1307`) rather than resetting them, so the wrapper's redirect is in
force for the sourced parse. The sourced circuit is left current, so the
wrapper's `op` and `print` afterwards are about the sub-deck: one deck asserts
the warning and the number in one run, which is what `0036` criterion 1 asks
for.

The sub-decks are written with `echo` rather than shipped as support files.
That keeps the circuits being distinguished visible in the deck that asserts
about them, needs no `EXTRA_DIST` entry, and keeps
`doc/claude/scripts/case_differential_sweep.py`'s deck count unchanged — a
shipped sub-deck is a `.cir` under `tests/` and the sweep would run it as a
deck in its own right. `vector-rawfile-scale-case.cir` already writes the file
it loads for the same reasons.

Rejected: **attaching the redirect to `run`, `op` or `tran`.** Read where each
check is called before assuming it can be deferred: all three are finished
before `CKTsetup`, so a redirect on the analysis captures nothing.

## Decision 3 — new sibling decks, and the existing decks are not converted

**Decided: three new decks —
`tests/regression/casedist/bsource-node-case-report.cir`,
`tests/xspice/casedist/auto-bridge-node-case-report.cir`,
`tests/xspice/casedist/event-node-case-report.cir` — and not one line changed
in the five decks that already carry the numbers.**

`0036` criterion 1 prefers extending. Extending is not available here: a deck
that captures its own parse cannot *be* the circuit, it has to source it, so
converting `bsource-node-case.cir` into a wrapper would replace a plain
netlist with a control script and move its `.out`. The five existing decks
keep asserting exactly what they assert today, which is the property that
makes them a guard, and each now names the report deck that covers its
diagnostic instead of claiming the diagnostic cannot be asserted.

The naming follows `vector-unlet-report.cir`, which is the same split for
`0027`: `-case` decks assert the numbers, `-report` decks assert the
diagnostic.

## Decision 4 — each deck asserts its own noun, and the report cases assert the other nouns' absence

**Decided: the phrase scanned for is `no node named`, `no analog node named`
or `no event node named`, never the shared tail `differs only in case`; and
the two XSPICE report cases also assert that the *other* XSPICE noun does not
appear.**

The three texts differ only in the word before `named`, so a deck asserting
the tail would pass on the wrong warning — including on a hypothetical change
that moved one diagnostic's condition into another's site. `no node named` is
not a substring of either XSPICE text, which is checked in both directions
rather than assumed.

The parser and event report cases go further and scan for the *pair of
spellings* as well: `no node named 'in'; 'In'` and
`no event node named 'dig'; 'Dig'`. Decision 2 of `0001-distinguish.md` says
the text names both spellings because the author cannot see the difference,
so which spelling is named first is part of the contract, and
`0003` decision 1 is explicitly about a case where the message points at the
reader rather than at the misspelling.

## Decision 5 — what each deck asserts as silence, and why that is the half that matters

A deck that asserts only the warning covers the half that was never in
danger. Each report deck therefore runs the silent circuits in the same mode
and the same run:

| Deck | Case | What it pins |
| --- | --- | --- |
| `bsource-node-case-report.cir` | MISS | `V(in)` against a deck that only wrote `In` is reported, and `v(miss) = 0` |
| | DEFN | two spellings both written by cards are two nets and neither is reported — the rejected "warn on definition" option of `0001` decision 2 |
| | FWDREF | a reference that creates `mid` and a later card that claims it, **with `MID` also defined**, is silent; only `t_unclaimed` can keep it so, and `0002`'s whole design rests on that bit |
| | NOTWIN | a miss with no case variant is silent, which is `doc/codex/issues/0028`'s class and not this one |
| `auto-bridge-node-case-report.cir` | MISS | `no analog node named 'A'; 'a'`, and `v(a) = 0` |
| | HAND | `already_joined()` — the deck the **first** version of this diagnostic warned on, per `0003` decision 2a, and which only a hand review caught |
| | EXACT | the ordinary bridged deck, where the event node matches an analog node exactly, is silent and reads `3.3` |
| `event-node-case-report.cir` | MISS | `no event node named 'dig'; 'Dig'`, and `v(aout) = 0` |
| | NOTWIN | a dangling event node with no case variant is silent |
| | BRIDGED | an event node the auto-bridge drives, **beside a driven event node differing from it only in case**, is silent — which is only true because `0003` decision 3 puts `EVTnode_case_check()` after `Evtcheck_nodes()`. Run it earlier and this deck reports. `v(aout) = 5` says the inserted bridge really drives the node |

Each silence case also echoes a `CAPTURE-READ` token, set by finding the
sub-deck's own `Circuit:` line in the capture. This was added by the
adversarial review of these three commits and it closes the one way an
assertion about an absence can be vacuously true: a `SILENT` token is printed
whenever the phrase is not found, including when the capture was never opened.
Proved RED by pointing one `fopen` per deck at a name that does not exist —
`CAPTURE-READ` becomes `CAPTURE-EMPTY` and the deck fails, where the `SILENT`
token alone still passed. The three decks were restored bit identically
afterwards and checked with `md5sum -c`.

BRIDGED and FWDREF are the two that could not have been written without
reading the records: each is a circuit that satisfies the diagnostic's
*surface* condition — an undriven node beside a driven twin, a reference-made
node beside a defined twin — and must stay silent because of the one design
decision that separates them.

## Decision 6 — the generated file names are lower case, and that is `doc/codex/issues/0038`

**Decided: `bnr_*`, `abr_*`, `enr_*`, all lower case.**

The first version used upper case, to survive the uppercased copy the
differential sweep makes: the sweep leaves a `source` line alone and
uppercases an `echo` line, so a lower-case sub-deck is written under one
spelling and sourced under another, and the sweep's `preserve-UPPER` run of
these decks dies with `fatal error in ngspice, exit(1)`.

Upper case is worse, and this was measured rather than reasoned: in the
**default** `fold` mode the reader lower cases a control line unless its first
word is on a fixed exempt list at `src/frontend/inpcom.c:1954-1968`. `echo`
and `source` are on that list and `fopen` is not, so an upper-case name is
written as typed and then looked for folded, and the deck stops working in the
mode it is not even about. That asymmetry is filed as
`doc/codex/issues/0038`; it is not caused by this work and is wrong in the
shipped default mode, which is why it is filed rather than worked around
twice.

The lower-case names leave the sweep's `preserve-UPPER` copy broken for these
three decks. That is the collateral `vector-rawfile-scale-case.cir` already
has, it is a `DIFF` and not a `NUM-DIFF` or a `PARSE-FAIL`, and the sweep
never runs `distinguish` — so it is not evidence about these decks in either
direction.

## Evidence

Every RED below is a `make check` failure, not a hand run, and every deck was
first run against an **empty** `.out` to show its evidence is output rather
than an absence.

| Deck | Empty-`.out` failure | RED, and against what | GREEN |
| --- | --- | --- | --- |
| `bsource-node-case-report.cir` | 34 added lines | `NODE-MISS-REPORTED` → `NODE-MISS-SILENT`, twice: once against `8557994bb`'s `fprintf(stderr, ...)`, and once against a binary with `INPtermCaseCheck()`'s `fprintf` deleted | PASS |
| `auto-bridge-node-case-report.cir` | 30 added lines | `BRIDGE-MISS-REPORTED` → `BRIDGE-MISS-SILENT`, same two binaries, the second with `report_bridge_case_miss()`'s `fprintf` deleted | PASS |
| `event-node-case-report.cir` | 30 added lines | `EVENT-MISS-REPORTED` → `EVENT-MISS-SILENT`, same two binaries, the second with `EVTnode_case_check()`'s `fprintf` deleted | PASS |

The added-line counts are for the decks as they stand after the
`CAPTURE-READ` commit; the three commits that added them measured 31, 28 and
28 for the decks as those commits left them. Counted with
`grep '^+' | grep -vc '^+++'`, because a plain `grep -c '^+'` also counts the
`+++` header — the mistake `18f751b4b` made.

Each single-line diff is the whole diff: the three silence assertions in each
deck are already correct against the reverted binary, which is what says they
assert an absence and not a side effect of the report.

After each revert the file was restored and checked with `md5sum -c`, and
`git diff` over `src/` is exactly the three one-line `stderr` → `cp_err`
hunks and nothing else.

- `make check`: **264 PASS, 0 FAIL**, up from 261 by exactly these three decks.
- `tests/regression/casedist` 14 for 14, `tests/xspice/casedist` 17 for 17.
- `tests/regression/case` 109 and `tests/xspice/case` 21, the `preserve`
  twins: untouched and passing.
- `python3 doc/claude/scripts/case_differential_sweep.py --jobs 12 --timeout 90`
  on a binary built from `8557994bb` and on the binary after this work, both
  snapshotted before the sweeps so a rebuild could not invalidate one:
  **305 decks, DIFF=59, OK=243, SKIP=3, PARSE-FAIL=0, NUM-DIFF=0 in both, and
  the two logs are byte identical.** No deck moves.

  The totals differ from the 302/53 the last session recorded by the three
  decks added here, which are `DIFF` for decision 6's reason, and by three
  more in the set that flaps run to run. That flap was measured rather than
  assumed: three consecutive filtered sweeps of `alter-rebin*` and `ring51`
  **on the same baseline binary** returned six `DIFF`s, then four, then six.
  The per-deck comparison between the two binaries is the evidence; the
  totals against an older log are not.

No number moves under `fold` or `preserve`, and the argument is stronger than
the measurement: all three diagnostics return early unless
`inp_case_mode() == NG_CASE_DISTINGUISH`, so in the two shipped modes the
changed line is not reached at all.

## What this decision does not decide

1. **`doc/codex/issues/0038`**, per decision 6. Found by this work, wrong in
   the default mode, and not fixed here because the fix has to choose a
   direction for `write`/`source`/`load` as well and needs the sweep behind
   it.
2. **`doc/codex/issues/0039`**, the fifth diagnostic of this family — the
   `.param` built-name ambiguity warning at
   `src/frontend/numparam/xpressn.c:511` — which is still on `stderr` and
   therefore still unguardable. It belongs to `doc/codex/issues/0029` and not
   to `0036`, so it is filed rather than carried along; the fix is decision
   1's one-token change plus a deck in the shape of the three added here.
3. **`doc/codex/issues/0028`**, the generic undefined-node diagnostic on both
   sides of the mixed-signal boundary. Still the largest remaining item; the
   NOTWIN case of two of these decks now pins that it is *silent*, which is
   the behaviour `0028` would change.
4. **`doc/codex/issues/0037`**, `diff`'s cross-plot pairing, which is wrong in
   the default mode and is the strongest competing candidate for the next
   session.
5. **`doc/codex/issues/0034`**, `let` warning on a definition; **`0033`**,
   `cp_remkword()`'s spelling; **`0035`**, all three items; **`0031`**, the
   family-less node.
6. **The residual false positive in `0003` decision 2a** — a hand-bridged
   event node reported because some unrelated analog node happens to differ
   from it only in case. The HAND case pins the suppression that exists, not
   the residual.
7. **The build-enforced lint** against a new `strcmp` or `cieq` on a vector or
   node name. It still goes last.
