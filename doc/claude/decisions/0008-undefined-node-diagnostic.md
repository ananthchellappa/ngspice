# Decision 0008 — the generic undefined-node diagnostic, `doc/codex/issues/0028`

## Status

Accepted, 2026-08-11, branch `ver_50`. Closes `doc/codex/issues/0028`, both
gaps. Extends the mechanism of
`doc/claude/decisions/0002-deferred-node-resolution-check.md` rather than
re-deciding it: the bit, its owner, the deferral and the near-miss text are all
unchanged, and what is new is a second report over the same scan and nine more
call sites feeding it.

Commits: `17e429993` gap 1, the report; `7b18264c4` gap 2, the nine reference
sites; `2d57025d4` the false positive its own review found; `bb049ffcf` the
decks the same review broke; this record.

## Context

`0002` decision 2 declined the generic case in one paragraph — **"What
`distinguish`-only costs, stated plainly"** — and named the reason: *a node
mentioned once is a legal floating node in SPICE, so it would fire on correct
decks and needs the full suite and the differential sweep behind it*.

This is the first change in the series that moves the **default** mode on every
deck. `0037` moved `fold` and `preserve`; this moves all three.

Measured at `a3edef17a`, the defect is `0002`'s two shapes with the case
element removed:

```spice
* a misspelt V() reference, with a DC path to the node it manufactures
.OPTIONS noacct rshunt=1e9
V1 mid 0 dc 1.5
R1 mid 0 1k
B1 out 0 V={V(mdi)*2}
R2 out 0 1k
```

- with a DC path to the manufactured node the run completes and prints
  `v(out) = 0.000000e+00` where the deck meant 3, with no diagnostic;
- without one the matrix is singular and the run aborts with
  `Warning: singular matrix:  check node mdi`, naming a node the deck never
  wrote.

---

## Decision 1 — one scan, two reports, and the generic half is mode independent

**Decided: `INPtermCaseCheck()` keeps its single walk of the term table.  For
each unclaimed entry it looks for a defined case variant; with one it prints
the near-miss text, guarded on `distinguish` as before, and with none it prints**

```
Warning: no node named 'mdi'; it is referenced but no card defines it
```

**in every mode.**

The structure was the question `0028` asked, because the existing function
returns early unless `distinguish` and a mode-independent check cannot live
inside that guard. A second scan was the alternative and is rejected: the two
reports are the same predicate — `t_unclaimed` survived the parse — differing
only in whether a twin exists, so a second scan would walk the same table to
ask a sub-question of the first, and the two could drift apart. Written as one
scan the exclusivity is structural: a node gets exactly one of the two
messages, and the more specific one wins.

Why the generic half is mode independent, which is the substantive half of
this decision:

- **Nothing about it is about case.** `0002` decision 2 had to gate the
  near-miss because under `fold` the only entries carrying upper case are the
  ones ngspice constructs for itself — `q1#collCX` — so the near-miss would
  have fired in the default mode on a name the user cannot control. The
  generic report has no such asymmetry: it names one spelling, the one the
  deck wrote, and the deck wrote it in every mode.
- **The defect predates the feature.** `0028`'s Impact says so and the census
  below confirms it: this is a plain undefined node, not a `casemode` effect,
  and a diagnostic that fired only under an experimental mode would leave the
  shipped modes with the silent wrong answer they have had since Spice3.
- **The reachable condition is narrower than "a floating node".** The bit is
  set by a *reference* and cleared by the first *definition*, whichever order
  they arrive in, so a node named by any card — however few — is not a
  candidate. A legal floating node is named by a card. That is the difference
  between the two things `0002` warned might be confused, and it is asserted
  by the FLOAT case of `tests/regression/misc/bsource-undef-node-report.cir`.

**The function keeps its name.** `INPtermCaseCheck()` now reports something
that is not about case, which is an argument for renaming it. It is not
renamed: seven documents cite it by name as the place where the parser's
end-of-parse check lives — `0001` decision 2's prose, `0002` decision 4,
`0003` in four places, `0006` decision 2 and its evidence table, `0028`,
`doc/codex/issues/0036` and `doc/claude/specs/case-sensitive-identifiers.md`
— and a rename would turn every one of those citations into a dangling one to
save a reader one comment. The comment above the function states both reports
instead.

## Decision 2 — a warning, not an error, re-tested for the default mode

**Decided: `Warning:` on `cp_err`, run continues.**

`0002` decision 3 argued this for the near-miss and this record does not
inherit it, because that argument was made for a diagnostic that fired only
under an experimental mode. Re-tested where it is now weakest — the default
mode, on every deck in the world:

- **An unresolved node is not an error in SPICE.** `0001` decision 2 rejected
  abort-on-miss for exactly this reason, and in the default mode the reason is
  at its strongest: erroring would turn decks that run today into decks that
  do not run, over a construct the language permits.
- **The neighbours warn and continue.** `.nodeset` and `.ic` on a
  non-existent node (`inppas3.c`) print `Warning : Nodeset on non-existent
  node - %s, ignored` and carry on, and they are the closest thing in the tree
  to this diagnostic: a dot card naming a node that is not there.
- **The goal is met by a warning.** The user-facing failure is a wrong number
  printed silently, and a warning is what makes it audible.

Rejected again, for the reasons `0002` decision 3 gives and which do not
weaken here: promoting it under `-strict`, which needs a fourth mode value
rather than a second variable (`0001` decision 1); and emitting it as
`Notice:` on `stdout`, where `check.sh`'s filter would let it through, because
ngspice's `stdout` is where the answer goes.

Written to `cp_err` and not to `stderr`, per `0001` decision 2's second
`Corrected 2026-08-11` paragraph: a diagnostic that uses a literal
`fprintf(stderr, ...)` cannot be reached by the control language's `>&` and
therefore cannot be guarded. It inherits the `cp_err` that `0006` decision 1
gave the surrounding function.

## Decision 3 — the report arrives before `singular matrix`, and that is measured

`0002` decision 3's third reason was that the warning arrives *first* and
explains the misleading message that follows. Confirmed for the generic case
on a real deck — the shape (b) circuit above, run under the same binary
before and after:

```
Warning: no node named 'mdi'; it is referenced but no card defines it
Warning: singular matrix:  check node mdi
```

versus the baseline binary, where the same run opens with
`Warning: singular matrix:  check node mdi` and the node it names appears
nowhere in the deck. The ordering is structural rather than lucky:
`INPtermCaseCheck()` runs inside `if_inpdeck()` (`src/frontend/spiceif.c`),
and the matrix is not solved until an analysis runs.

## Decision 4 — the nine reference sites are their own commit

**Decided: gap 2 lands separately, with its own RED and its own decks.**

`0007` split a two-defect session into two commits and that worked. The two
gaps are separable — gap 1 is a report over a bit that already existed, gap 2
is nine call sites feeding that report — and gap 2 carries a second defect of
its own that gap 1's deck cannot assert: inserting as a definition **cleared**
the bit, so a `.sens` naming the node a `V()` reference had manufactured
suppressed the near-miss. `0028` criterion 4 is that false negative and it was
assertable before anything changed.

Three corrections to `0028`'s table, re-read at `17e429993`:

| `0028` says | Actually |
| --- | --- |
| `inp2dot.c:371`, `:376` = `.SENS` | `dot_tf()` — **`.TF`** |
| `:495`, `:501` = `.TF` | `dot_sens()` — **`.SENS`** |
| `:677`, `:775` = `.PSS` oscnode | `:677` is `.PSS`; `:775` is **`.hb`** |
| `inpgval.c:90` = an `IF_NODE`-typed **device** parameter | no device or model in the tree declares an `IF_NODE` parm; the reachable producer is **`.pz`** |

Two of the nine cannot be exercised by any deck in this build and are changed
mechanically rather than under test: `.pss` is behind `#ifdef WITH_PSS`, which
bare `../configure` leaves undefined (`build-ver_50/src/include/ngspice/config.h`
has `/* #undef WITH_PSS */`), and `.hb` is behind `WITH_HB`, which no
`configure.ac`, `Makefile.am` or header in the tree defines at all. That is
stated in the deck and here rather than left for the next reader to discover.

The remaining seven are covered: `.noise`, `.tf` and `.sens` by their own
cases, and `inpgval.c:90` through `.pz`, whose four node arguments are the
whole of that site's reachable surface.

## Decision 5 — the census is the deliverable, and it is zero

`make check` **cannot see this diagnostic**: `tests/bin/check.sh` captures
stdout only and its `egrep -v` drops every line containing `Warning` from both
sides. The differential sweep cannot measure it either — a mode-independent
diagnostic fires identically in all three of the sweep's runs and so cannot
move a verdict by construction. So the decks under `tests/` were run directly,
with the new text grepped out of stderr, by
`doc/claude/scripts/undefined_node_census.py`, which this work adds beside the
sweep for the same reason the sweep exists: the harness cannot see what is
being changed.

**Every `.cir` file under `tests/` — 309 before this work, 312 after — run
under each of `fold`, `preserve` and `distinguish`, before and after gap 2.
None of them trips the report.  In any mode.  Zero.**

Each deck is run the way its own directory's `TESTS_ENVIRONMENT` runs it: cwd
is the build-tree counterpart of the deck's directory, and `SPICE_SCRIPTS` is
`.` for the three `xspice` directories, which ship their own `spinit`. Without
that every XSPICE deck loads the installed `/usr/local` code models and
segfaults (`doc/codex/issues/0021`) — 26 of them did in the first attempt, and
a census over crashed runs measures nothing. `rc` and whether the run reached
its `Circuit:` line are recorded per deck for that reason.

The zero was checked in the other direction as well, because a census that
cannot see a hit is indistinguishable from one with no hits: the same
`run_one()` was pointed at a deck that does trip and returned the warning, and
at the legal floating-node deck and returned nothing.

**Why the count is zero rather than small**, which is the part worth keeping:
the bit is set by a reference and cleared by any definition, so the candidate
set is not "nodes mentioned once" but "nodes no card ever mentioned". A legal
floating node is named by a card. `0002`'s stated worry — that this would fire
on correct decks — turns out to be answered by the mechanism it built rather
than by narrowing the rule.

One deck in the tree does move, and it is the one that pinned this silence:
the NOTWIN case of `tests/regression/casedist/bsource-node-case-report.cir`,
which `0006` decision 5 recorded as *"a miss with no case variant is silent,
which is `doc/codex/issues/0028`'s class and not this one"*. It now asserts the
undefined-node text by name **and**, in the same capture, that the near-miss
text does not appear, so the two reports cannot pass for one another. That is
a sharpened assertion, not a weakened one; the deck is RED against the binary
before gap 1 for the line that changed.

## Decision 6 — where each deck lives, and the one that decides the rule

`0007` decision 4 is the rule: `tests/regression/casedist` forces
`-D casemode=distinguish` on every deck it holds, so a default-mode assertion
cannot live there. Gap 1's report and gap 2's dot cards are both mode
independent, so both decks are in `tests/regression/misc`, beside
`diff-pairing-case.cir`. Only criterion 4 — a near miss the `.sens` used to
suppress — is about `distinguish`, and only that deck is in `casedist`.

The deck that decides whether the rule is right is the **FLOAT** case of
`bsource-undef-node-report.cir`: one net named by exactly one card, which is
the legal floating node `0002` and `0001` decision 2 both name as the reason
not to warn on a resolution miss in general. It must be silent, and it is
silent for a structural reason rather than a lucky one.

`0006` decision 5's rule is kept: every silence case echoes a `CAPTURE-READ`
token derived from finding the sub-deck's own `Circuit:` line, so an assertion
about an absence cannot pass on a capture that was never opened. The MISS
cases need no such token, because a found phrase proves its own capture was
read.

## Evidence

Every RED is a `make check` failure against the binary built before the
change, not a hand run, and each new deck was run against an **empty** `.out`
first so that its evidence is output rather than an absence.

| Deck | Empty-`.out` | RED, and against what | GREEN |
| --- | --- | --- | --- |
| `misc/bsource-undef-node-report.cir` | 26 added lines | three lines — `UNDEF-TYPO-REPORTED`, `UNDEF-FLOAT-OTHER-REPORTED`, `UNDEF-FWDREF-OTHER-REPORTED` → `-SILENT`, against `a3edef17a` | PASS |
| `casedist/bsource-node-case-report.cir` (existing) | — | `NODE-NOTWIN-REPORTED` → `NODE-NOTWIN-SILENT`, same binary | PASS |
| `misc/dotcard-undef-node-report.cir` | 36 added lines | nine lines — the eight `*-MISS-REPORTED` names and `DOTCARD-FWD-OTHER-REPORTED` → `-SILENT`, against `17e429993` | PASS |
| `casedist/sens-node-case-report.cir` | 32 added lines | two lines — `SENS-NEARMISS-REPORTED` and `SENS-ONLY-NEARMISS-REPORTED` → `-SILENT`, same binary | PASS |

In each case the diff is exactly those lines: the silence cases are already
correct against the reverted binary, which is what says they assert an absence
and not a side effect of the report.

- `make check`: **271 PASS, 0 FAIL**, up from 268 at `a3edef17a` by exactly
  the three new decks. Measured at each step: 269 after `17e429993`, 271 after
  `7b18264c4`, and 271 after each of `2d57025d4` and `bb049ffcf`.
  `tests/regression/misc` 25 → 26 → 27, `tests/regression/casedist` 17 → 18;
  `tests/regression/case` 109 and `tests/xspice/case` 21, the `preserve`
  twins, untouched and passing.
- `python3 doc/claude/scripts/undefined_node_census.py` on the final binary,
  in each of the three modes: **312 decks, 0 tripped**, and the same on the
  binary after gap 1 alone.
- `python3 doc/claude/scripts/case_differential_sweep.py --jobs 8 --timeout 90`
  on a snapshot of the binary before this work and on a snapshot after:
  **309 decks, DIFF=63, OK=243, SKIP=3, PARSE-FAIL=0, NUM-DIFF=0** before and
  **312 decks, DIFF=67, OK=242, SKIP=3, PARSE-FAIL=0, NUM-DIFF=0** after.
  Compared per deck rather than by totals: the non-`OK` lines differ by the
  three decks this work adds — all three permanent `preserve-UPPER` `DIFF`s of
  the class `vector-rawfile-scale-case.cir` already has, because the sweep
  uppercases an `echo` redirect target and not the `source` that reads it —
  and by one `alter-rebin` line. That line is the known flap and was
  re-measured rather than assumed: `--filter alter-rebin`, three runs on each
  binary, gave DIFF = 4, 4, 2 on the before binary and 4, 4, 4 on the after
  one, so it moves on an unchanged binary. An earlier after-sweep also showed
  `bsim3soifd`/`bsim3soidd` `ring51` moving, and `--filter ring51` gave 1 then
  2 on the before binary alone.

One hazard found by running rather than by reading, and recorded because the
next deck of this shape will meet it: a wrapper deck leaves the **last**
sourced sub-deck current, and batch mode then runs *its* analyses. The MISS
sub-deck of `dotcard-undef-node-report.cir` carries four analyses over eight
DC-pathless nodes, so the solve died inside `spSolve` on an unfactored matrix
and the deck produced no stdout at all — which passes against an empty
reference. Sourcing an analysis-free circuit at the end is the fix; `remcirc`
is the obvious one and segfaults under `-r`, which is `doc/codex/issues/0042`.

## Decision 7 — the check needs a complete parse, and one silence has no deck

Found by reviewing the two commits above rather than by a deck, and fixed in
`2d57025d4`. `INP2dot()` returns 1 on a card it cannot parse — a `.dc` with
missing tokens is the reachable case here — and `INPpas2()` then abandons the
deck at that line. Everything below it is unparsed, so a node those cards
define is indistinguishable from a node nothing defines, and the report was a
false one printed *before* the real error:

```
$ ngspice --batch abort.cir
Warning: no node named 'out'; it is referenced but no card defines it
Error on line 5 or its substitute:
  .dc v1
Bad syntax!
```

**Decided: `INPpas2()` returns non-zero when it gave up with cards still
unread, and `if_inpdeck()` skips the check in that case.** The premise the
call site states in its own comment — every card that can define a node has
now been seen — becomes a thing the code tests rather than assumes. Both
reports are skipped, not only the new one: the near miss rests on the same
premise and would name the same wrong node.

Rejected: **wording around it** — "…or the parse stopped before it" — which
keeps a diagnostic that is wrong on the facts and makes it longer.

**This silence has no deck, and that is a harness limit rather than a
choice.** The sub-deck that produces it kills the control block that sourced
it, so the wrapper shape every other case uses cannot reach its own
assertions afterwards; `tests/regression/misc/bsource-undef-node-report.cir`
says so where the case would have been. The hand check is the deck above,
which prints the two lines quoted here before `2d57025d4` and only the error
after it.

## Corrections to this record's own commits

Found by reviewing the commits after they landed, which is the step `0027`'s
session added to this workflow and `0007` repeated.

**`7b18264c4`'s message cites the nine sites at their parent's line
numbers.** The comments that commit adds push every call down, so at that
commit the calls are `inp2dot.c:58`, `:64`, `:377`, `:382`, `:502`, `:508`,
`:685`, `:784` and `inpgval.c:94`, not the `:53`/`:59`/`:371`/`:376`/`:495`/
`:501`/`:677`/`:775`/`:90` the message lists. The message's numbers are
`0028`'s and are right for the tree it describes; they are wrong for the tree
it produces. This is the same class of stale citation `0007` had to spend a
commit on.

**`bb049ffcf`'s message says the sens deck adds 32 lines against an empty
`.out` as "30".** Re-measured: 26, 36 and 32 for the three decks.

**The first full sweep after gap 2 reported `NUM-DIFF=1`, and no number had
moved.** `sens-node-case-report.cir` wrote a rawfile in the stock run and none
in the `preserve`-UPPER run, which cannot write one at all — it sources a name
its own `echo` line uppercased. `masked_equal()` treats a missing payload as a
differing payload, so the verdict that exists to mean "a number moved" was
raised by a run that never got as far as a number. Fixed in the deck rather
than in the script, by leaving no plot for the rawfile to come from. `0007`
decision 6 is the same near miss from the other end, and twice now the alarm
has been raised by a deck this series added.

**Three of the nine sites were unexercised by the decks as first committed,
and the decks' silences could be broken six ways without going red.** That is
`bb049ffcf` in full; the interesting part is that every one of those defects
was in the half of a deck that asserts an *absence*, which is the half `0006`
decision 5 already identified as the one that matters.

## What this decision does not decide

1. **`mkinode()` and `I()` references**, `inpptree.c:1282`. `0002` decision 4
   item 4 puts them outside this mechanism: they intern an instance name into
   a different table with no node behind it. (`0002` gives `:1279` for it,
   which is `mkvnode()`'s last line; the number was already stale there.)
2. **The two unexercisable sites**, `.pss` and `.hb`. Changed mechanically; a
   deck for `.pss` needs `../configure --enable-pss` and a build directory
   this tree does not have, and `.hb` is dead in every configuration.
3. **`doc/codex/issues/0043`**, the command forms of these same analyses.
   `sens`, `tf`, `noise` and `pz` typed in a `.control` block reach
   `INPpas2()` through `if_run()`, which never calls `INPtermCaseCheck()`, so
   the report this record adds does not fire there. The nine sites mark the
   node on that path too; nothing reads the mark. Filed rather than fixed
   because the fix is a differently scoped check — the nodes this card
   created, against a table that already holds a built circuit — and because
   `sens v(x)` on that path already dies in `CKTunsetup()` before any
   diagnostic could help.
4. **What the analyses do with a manufactured node once it is reported.** A
   deck carrying four analyses over eight DC-pathless manufactured nodes still
   reaches `spSolve` with an unfactored matrix and aborts on an assertion; a
   `.pz` alone stops earlier, with `doAnalyses: The input signal is shorted on
   the way to the output`. The report now precedes both, which is the whole of
   what this record claims.
5. **`doc/codex/issues/0042`**, `remcirc` under `-r`. Found by this work,
   present at `a3edef17a`, and worked around in the deck that met it.
6. **`doc/codex/issues/0038`**, the reader's per-command fold exemption for
   control lines, on which all three decks depend for `echo` and `source`.
7. **`doc/codex/issues/0039`**, `0034`, `0033`, `0035`, `0031`, `0041`, and
   the residual false positive in `0003` decision 2a.
8. **The build-enforced lint.** It still goes last. With this record the
   parser's node table has one reference entry point and one definition entry
   point, which is the state a lint over `INPtermInsert()` would freeze.
