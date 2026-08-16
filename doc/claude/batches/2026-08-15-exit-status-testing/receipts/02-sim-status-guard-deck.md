# Receipt: item 2 — the decks that assert the `$sim_status` guard

PLAN item **2 — 0069 criterion 3: a deck that asserts the `$sim_status` guard**,
from `doc/claude/batches/2026-08-15-exit-status-testing/PLAN.md`.

Crew B, 2026-08-15, branch `ver_50`. HEAD at start `baccfa570` — item 1's
commit, so the driver everything below runs on is the committed one.

**No code was touched.** Nothing under `src/` was opened for editing, and
neither was `tests/bin/check_status.sh`: the four decks are expressible under
the driver's contract exactly as item 1 shipped it. The change is four `.cir`
files with seven assertion siblings, the `Makefile.am` that registers them, and
one paragraph of `doc/codex/issues/0069`. Nothing under `doc/claude/upstream/`
was opened, referenced or written to, and nothing here states or implies that
any upstream submission has been sent.

Binary: `/home/qflow/dev/ngspice_test/build-ver_50/src/ngspice`, `ngspice-46+`.
**The stamp moved during the item, for the same reason it moved during item 1.**
Everything up to and including the four RED corruptions was measured on
**`Sat Aug 15 23:47:16 UTC 2026`**, the binary item 1 finished on. Registering
the decks meant a `Makefile.am` edit and `./autogen.sh`, which regenerated the
tree's build metadata, so the full `make check` recompiled and relinked and the
final binary is **`Sun Aug 16 00:00:02 UTC 2026`**. The first RED corruption was
re-run on the new binary and is byte-identical, including the 570-byte constants
file; the six-deck targeted run is green on both.

Scratch: `/tmp/claude-1000/-home-qflow-dev-ngspice-test/d775575c-7554-4eec-9dca-17c55bd7d1cf/scratchpad/i2`.
Nothing was left in the repository beyond the files listed below.

## The starting measurement, re-run rather than quoted

```
$ grep -rl sim_status tests/
$ echo $?
1
```

Nothing. That is the whole of criterion 3's complaint, confirmed on this tree
at `baccfa570` before a line was written.

## The RED failure, verbatim

These are characterisation decks over behaviour that already works, so RED is
each deck's own assertion removed or inverted. Four corruptions, each run
through the real harness and each reverted.

### 1. `sim-status-guard.cir` with the four guard lines deleted

The control block becomes `run` / `write sim-status-guard.raw`. This is 0069's
UNGUARDED row: rc=0 in every mode.

```
check_status: sim-status-guard: FAILED (status)
  expected exit status 1, got 0
  stderr was:
    Error: no data saved for D.C. Operating point analysis; analysis not run
    doAnalyses: not found

    run simulation(s) aborted
  absent sim-status-guard.raw: the run created it
check_status: sim-status-guard: FAILED (files)
check_status: sim-status-guard: FAILED: status files
  The captured output is in sim-status-guard.stdout and sim-status-guard.stderr.
FAIL: sim-status-guard.cir
```

**Both halves of the criterion fail, and that is the point.** `FAILED: status
files` is one line naming the two checks: the run exited 0 where the deck says
1, *and* it reached the `write`. The file it left is the artefact the guard
exists to prevent, measured on the spot:

```
-rw-r--r-- 1 qflow qflow 570 Aug 15 17:00 sim-status-guard.raw
Title: Constant values
Plotname: constants
No. Variables: 12
```

570 bytes of physical constants — the same file 0069's *Impact* table records,
re-measured here rather than carried over. Guard restored,
`check_status: sim-status-guard: exit status 1 as expected`, PASS.

### 2. `sim-status-properties.cir` with property 1a inverted

`if $sim_status ne 1` changed to `if $sim_status ne 0` after the failing
analysis, so the deck's own fail-fast fires and it quits 1 where its `.status`
says 0.

```
check_status: sim-status-properties: FAILED (status)
  expected exit status 0, got 1
  stderr was:
    Error: no data saved for D.C. Operating point analysis; analysis not run
    doAnalyses: not found

    op simulation(s) aborted
check_status: sim-status-properties: FAILED: status
  The captured output is in sim-status-properties.stdout and sim-status-properties.stderr.
FAIL: sim-status-properties.cir
```

The captured stdout shows which check fired and what it read, which is the
half `check.sh` would have thrown away:

```
AFTER-RUN=0
AFTER-BAD=1
ERROR: a failed analysis left sim_status at 1, not 1
```

(The message reads oddly only because the *comparison* was corrupted and not
the text; the committed deck says "not 1" when the value is not 1.) Reverted,
PASS.

### 3. `sim-status-unset.cir` with the `echo $sim_status` line deleted

This is the corruption that proves property 2's stderr half is really
asserted, since no deck can read its own stderr.

```
--- sim-status-unset.stderr.expected	2026-08-15 17:01:07.082469164 -0700
+++ sim-status-unset.stderr.filtered	2026-08-15 17:01:07.078469348 -0700
@@ -1 +0,0 @@
-Error: sim_status: no such variable.
check_status: sim-status-unset: FAILED (err)
  filtered stderr does not match ../../../../tests/regression/exitstatus/sim-status-unset.err (- expected, + actual)
check_status: sim-status-unset: FAILED: err
  The captured output is in sim-status-unset.stdout and sim-status-unset.stderr.
FAIL: sim-status-unset.cir
```

The deck's own `$?sim_status` check still passed, so it exited 0 and only the
`err` check fired — the two halves of property 2 are independently asserted.
Reverted, PASS.

### 4. `sim-status-guard-ok.cir` with its `.save` made to miss

Not asked for, and worth one run: a negative control that would fail the same
way as the positive one is not controlling anything. `.save v(MidNode)`
changed to `.save v(nosuchnode)`:

```
  expected exit status 0, got 1
  stderr was:
    Error: no data saved for D.C. Operating point analysis; analysis not run
    doAnalyses: not found

    run simulation(s) aborted
  present sim-status-guard-ok.raw: the run did not create it
  grep sim-status-guard-ok.raw: no such file
check_status: sim-status-guard-ok: FAILED (files)
check_status: sim-status-guard-ok: FAILED: status files
FAIL: sim-status-guard-ok.cir
```

So the two guard decks differ in exactly one card and give opposite verdicts on
all three checks. Reverted, PASS.

## The production change

No production change. Four decks, seven assertion files, one `Makefile.am`, one
issue paragraph.

All four are in `tests/regression/exitstatus/` and all four are in the
directory's default case mode — no deck needs a `-D` flag, because
`v(nosuchnode)` misses under `fold`, `preserve` and `distinguish` alike and
`v(MidNode)` spelled as the netlist spells it resolves under all three.

| deck | shape | asserts |
| --- | --- | --- |
| `sim-status-guard.cir` | `.save v(nosuchnode)`, `.op`, `.control run` + guard + `write` | `.status` = **1**; `.files` = `absent sim-status-guard.raw` |
| `sim-status-guard-ok.cir` | identical but `.save v(MidNode)` | `.status` = **0**; `.files` = `present` + `grep … Plotname: Operating Point` |
| `sim-status-properties.cir` | no analysis card; `run`, then a failing `op`, then a good one | `.status` = **0** via `quit 0`; each of the three checks quits 1 with an `ERROR:` echo |
| `sim-status-unset.cir` | no analysis at all; `$?sim_status` then a read | `.status` = **0**; `.err` = `Error: sim_status: no such variable.` |

**The shape is the criterion's shape.** Both guard decks carry an analysis dot
card **and** a `.control run` with no `-r`. That is arm 2 of `main.c`'s
epilogue — `ft_savedotargs()` builds a save list from the `.op` card, so the
deck's failing `.save` is no longer the whole save list, the post-block re-run
succeeds and the process exits 0 with the analysis never having produced data.
It is also why the analysis runs twice, which the deck's header comment says in
as many words. Every claim in that header was measured today, not paraphrased
from the issue.

**Why the `grep` and not just `present`.** A run whose analysis failed still
writes a well-formed rawfile (`doc/codex/issues/0059`); `present` alone would
be satisfied by exactly the file the guard prevents. `Plotname: Operating
Point` is the discriminator, and it is the driver's `grep` verb doing what
item 1 built it for.

**Why four decks and not three.** The three properties needed two decks, not
one, and the reason is a driver contract, not a preference: `.err` is an exact
comparison of the whole filtered stderr. Property 2's stderr evidence is one
line, but a deck that also demonstrates property 1 must run a *failing*
analysis, whose `Error: no data saved for D.C. Operating point analysis;
analysis not run` and `run simulation(s) aborted` would then be frozen into the
same `.err` file. That wording is `doc/codex/issues/0057`'s subject. Splitting
property 2 into its own three-line deck keeps this directory from failing the
day 0057 is fixed, and costs one `.cir` and one `.status`.

**Registration.** `tests/regression/exitstatus/Makefile.am`: four names
appended to `TESTS`, seven assertion files to `EXTRA_DIST`, and `CLEANFILES`
extended with each deck's four capture files plus **the two rawfiles the guard
decks write**, `sim-status-guard.raw` and `sim-status-guard-ok.raw`. A
paragraph in the header block says what the four decks are and which criterion
they close. Then `./autogen.sh` from the repo root; `configure.ac` and
`tests/regression/Makefile.am` needed no edit, since item 1 already registered
the directory.

`tests/lint/identity.baseline` is **unchanged** — `git status --short
tests/lint/` is empty. This item adds no C and no string comparison.

## The two `make check` counts

| run | result |
| --- | --- |
| `make -C build-ver_50/tests/regression/exitstatus check` | 6 PASS / 0 FAIL — "All 6 tests passed" |
| `make check` from `build-ver_50` | **328 PASS / 0 FAIL**, `make` exit status 0 |

Previous count was 324 PASS / 0 FAIL at `baccfa570`; the four new decks account
for the whole difference and no existing test moved. Counted as
`grep -c '^PASS:'` and `grep -c '^FAIL:'` over the full log; `XFAIL`, `SKIP`
and automake `ERROR` results are all zero.

One line in that log reads `ERROR: (internal)  tried to destroy non-existent
graph`. It is **not** a test result — it is simulator output inside
`tests/regression/casedist/vector-probe-report.cir`, which PASSes, and it is
pre-existing and unrelated to this item.

`tests/lint/` in the same run: `identity lint: 264 comparisons, baseline
matches` and `identity lint: 11 comparisons, baseline matches`, both PASS.

## What was measured

**The guard shape, by hand, before it was a test** (scratch copies of the two
guard decks, run from a clean directory on the `23:47:16` binary):

```
$ ngspice --batch -n guard-fires.cir  ; echo rc=$?
rc=1     stdout: RUN-FAILED     guard-fires.raw: No such file or directory
$ ngspice --batch -n guard-passes.cir ; echo rc=$?
rc=0     guard-passes.raw 229 bytes, Plotname: Operating Point, 1 variable v(midnode)
```

`guard-fires` stderr:

```
Error: no data saved for D.C. Operating point analysis; analysis not run
doAnalyses: not found

run simulation(s) aborted
```

The committed negative control writes the same file at **287 bytes** on the
`00:00:02` binary — the difference is its longer title line, which a rawfile
header carries verbatim, and nothing else:

```
Title: * the negative control: the same guarded deck with a .save that resolves
Command: ngspice-46+, Build Sun Aug 16 00:00:02 UTC 2026
Plotname: Operating Point
No. Variables: 1
```

Its stdout carries **two** `Doing analysis` blocks and the operating-point
table after the `write` — the two-simulation shape of 0069's criterion 4,
visible in the negative control precisely because the guard does not fire.

**The three properties, each one measured on the committed deck.**
`sim-status-properties.cir` stdout, filtered to its own echoes:

```
AFTER-RUN=0
AFTER-BAD=1
AFTER-GOOD=0
PROPS-OK
```

`sim-status-unset.cir`: stdout `VALUE-BEFORE=` (empty), stderr exactly
`Error: sim_status: no such variable.`, rc=0, no rawfile.

That is 0069's Resolution table reproduced from `tests/`: property 3 is
`AFTER-RUN=0` from a `run` with no analysis card anywhere; property 1 is
`AFTER-BAD=1` then `AFTER-GOOD=0`; property 2 is the `$?sim_status` check that
the unset deck passes plus the stderr line it asserts.

**The unguarded artefact**, from RED 1: 570 bytes, `Title: Constant values`,
`Plotname: constants`, `No. Variables: 12`. Matches what 0069 records,
independently re-measured, and identical on both build stamps.

**`grep -rl sim_status tests/`** now returns six files (`| sort`):

```
tests/regression/exitstatus/Makefile.am
tests/regression/exitstatus/sim-status-guard-ok.cir
tests/regression/exitstatus/sim-status-guard.cir
tests/regression/exitstatus/sim-status-properties.cir
tests/regression/exitstatus/sim-status-unset.cir
tests/regression/exitstatus/sim-status-unset.err
```

## Which of the three properties could be asserted, and how

All three, but not all three the same way, and one of them not by status alone.

1. **Per analysis, last writer wins** — asserted in full, in-deck. The deck
   reads the variable after each `op` and quits 1 with an `ERROR:` echo if it
   is not 1 then 0. RED 2 shows the check is live.
2. **Absent before the first analysis** — asserted in full, but in **two
   places**, because the deck can see only half of it. `$?sim_status ne 0` is
   in-deck; `Error: sim_status: no such variable.` is on stderr and no control
   block can read its own stderr, so it is `sim-status-unset.err` and the
   driver compares it. RED 3 shows that half is live and independent.
3. **A `run` with no analysis to do reads 0** — asserted in full, in-deck.

**What could not be asserted, and it is none of the three:** the *stdout* of
these decks is not compared to anything. The driver captures stdout only so a
failure can be read afterwards, by item 1's deliberate decision, so
`AFTER-BAD=1` and friends are evidence in this receipt and a debugging aid in a
failure, not assertions. They do not need to be: each echo is followed by an
`if` that turns the same value into the exit status, which is the assertion.
`PROPS-OK` is likewise not asserted; `quit 0` is only reachable through it.

## What was left, deliberately

- **No `.err` on either guard deck.** They would have frozen `Error: no data
  saved for D.C. Operating point analysis; analysis not run` — 0057's wording,
  which is expected to change — into a directory whose subject is the exit
  status. Criterion 3 asks for rc and the artefact, and that is what they
  assert. The text is recorded here instead.
- **No `-D casemode=` variant of the guard decks.** 0069's three-mode table is
  about `v(midnode)` vs `MidNode`; these decks use a name that is missing in
  every mode, which is what the criterion specifies and what makes them
  belong in this default-mode directory. `tests/regression/casedist/` is where
  a mode-dependent version would go, and none is owed.
- **No `-r` deck.** The criterion's shape is explicitly the one *without* `-r`,
  because that is the shape that otherwise exits 0. The `-r` arm already exits
  1 without any guard, so a deck asserting it would assert `main.c` arm 1 and
  not the guard.
- **`tests/bin/check_status.sh` untouched**, as are `tests/bin/check.sh`,
  `src/frontend/dotcards.c`, `src/frontend/outitf.c` and
  `doc/codex/issues/0072`. Item 4 owns those.
- **`doc/claude/casemode-distinguish-guide.md` §9 and
  `doc/claude/feedback/ngspice_upstream/RESPONSE.md` were not edited**, though
  both now have a deck they could point at. PLAN item 6 owns cross-references.
- **Everything in 0069 except the criterion-3 paragraph** — its Resolution was
  rewritten three commits ago and is correct; the only other edit was the one
  sentence introducing the subsection, which counted the met criteria and would
  otherwise have contradicted the paragraph directly under it.

## For the owner

1. **Criterion 3 is closed and 0069 has no open criterion left.** Its Status
   line still says "Open, filed 2026-08-14", and the decision it records — that
   the exit status does not change — is unaffected by this item. Whether an
   issue whose five criteria are all met should still be Open is a call for the
   owner; I did not make it, because the instruction was to touch only the
   criterion-3 paragraph.
2. **These decks assert the guard, not the defect.** If someone ever "fixes"
   the exit status against criterion 1, `sim-status-guard-ok.cir` is the deck
   that would notice: a deck whose analysis succeeded must keep exiting 0. That
   is worth knowing before anyone reads the directory as a regression net for
   the abort in 0072.
3. **The two-simulation shape is now visible in a test.**
   `sim-status-guard-ok.cir`'s stdout carries two `Doing analysis` blocks for
   one deck. Nothing asserts that count — stdout is not compared — so if PLAN
   item 6 wants criterion 4 asserted rather than documented, it is a `.out`-
   style comparison and this driver does not do one.
4. Nothing was pushed. One commit on `ver_50`.
