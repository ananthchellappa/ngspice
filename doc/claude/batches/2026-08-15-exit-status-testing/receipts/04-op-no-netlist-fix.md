# Receipt: item 4 — 0072's fix, RED-first, at `dotcards.c:225`

PLAN item **4 — 0072: the fix, RED-first, with a deck in the new directory**,
from `doc/claude/batches/2026-08-15-exit-status-testing/PLAN.md`.

Crew D, 2026-08-15, branch `ver_50`. HEAD at start **`1c07b937b`**, at finish
**`ce48f9630`**.

**Code was touched.** One production file, `src/frontend/dotcards.c`: twelve
lines added, one removed. Everything else is the three decks, their assertion
files, the directory's `Makefile.am`, and `doc/codex/issues/0072`. Nothing
under `doc/claude/upstream/` was opened, edited or referenced, and nothing here
or in the issue states or implies that any upstream submission has been sent —
because none has. `src/frontend/outitf.c` is untouched; `git diff
1c07b937b..HEAD -- src/frontend/outitf.c` is empty. `tests/bin/check.sh` and
`tests/bin/check_status.sh` are untouched.

**Binaries.** Everything was run in place at `build-ver_50/src/ngspice`, never
copied. Three builds were involved, because the before-state was re-measured
rather than carried from receipt 03, and `./autogen.sh` moves the banner's
Creation Date by re-running `configure`:

| build | tree | sha256 `build-ver_50/src/ngspice` | Creation Date |
| --- | --- | --- | --- |
| as found | `1c07b937b`, untouched | `1280b0bece82f2f0…d7ea` | `Sun Aug 16 00:00:02 UTC 2026` |
| **BEFORE** | `1c07b937b`, rebuilt after `autogen.sh` | `0e740008e6b7a177…fe04` | `Sun Aug 16 00:49:22 UTC 2026` |
| **AFTER** | the fix and the three decks | `1fa06748c09be050…7686` | `Sun Aug 16 00:53:25 UTC 2026` |

The `as found` hash is byte-identical to the pristine binary receipt 03
finished on, which is the check that this crew started where that one stopped.
BEFORE and AFTER differ from it only in the Creation Date string; the defect
reproduced identically on all three.

Scratch: `/tmp/claude-1000/-home-qflow-dev-ngspice-test/d775575c-7554-4eec-9dca-17c55bd7d1cf/scratchpad`
(`measure.sh`, `results-before/`, `results-after/`, `mine/`, `check-before.log`,
`check-after2.log`, `build*.log`, `autogen*.log`). Nothing was left in the
repository or the build tree. No `core` file exists anywhere in the tree; this
machine's `/proc/sys/kernel/core_pattern` is `|/wsl-capture-crash …`, so the
aborting runs wrote no core file into any directory — see *What was left*.

## The RED failure, verbatim

The three decks and their `.status`/`.err`/`.files` siblings were written and
registered **before** `src/frontend/dotcards.c` was touched, and
`make -C build-ver_50/tests/regression/exitstatus check` was run against the
BEFORE binary. Each deck fails **three** checks — `signal`, `status` and `err`.
Receipt 01's contract predicted the first two; the `err` failure is the third,
because these decks assert the text as well as the number.

```
../../../../tests/bin/check_status.sh: line 235: 4140624 Aborted                 (core dumped) $SPICE --batch -n "$testdir/$testname.cir" > "$out" 2> "$err"
check_status: op-empty-netlist: FAILED (signal)
  the simulator did not terminate normally: rc=134 is 128+6, SIGABRT.
  A run that cannot proceed has to say so and return a status.
check_status: op-empty-netlist: FAILED (status)
  expected exit status 1, got 134
  stderr was:
    ngspice: ../../../src/frontend/dotcards.c:225: ft_cktcoms: Assertion `plot_cur->pl_dvecs != NULL' failed.
--- op-empty-netlist.stderr.expected	2026-08-15 17:45:30.313724708 -0700
+++ op-empty-netlist.stderr.filtered	2026-08-15 17:45:30.309724695 -0700
@@ -1,3 +1 @@
-Error: incomplete or empty netlist
-       or no node to report an operating point for;
-no operating point printed!
+ngspice: src/frontend/dotcards.c:225: ft_cktcoms: Assertion `plot_cur->pl_dvecs != NULL' failed.
check_status: op-empty-netlist: FAILED (err)
  filtered stderr does not match ../../../../tests/regression/exitstatus/op-empty-netlist.err (- expected, + actual)
check_status: op-empty-netlist: FAILED: signal status err
  The captured output is in op-empty-netlist.stdout and op-empty-netlist.stderr.
FAIL: op-empty-netlist.cir
```

`op-empty-tran` and `op-empty-control` fail identically, same three checks,
same assertion line. The directory's verdict:

```
===========================================================
3 of 9 tests failed
Please report to http://ngspice.sourceforge.net/bugrep.html
===========================================================
```

After the fix, on the same command: **All 9 tests passed**, with
`check_status: op-empty-netlist: exit status 1 as expected` and the same for
the other two.

## The production change

One hunk, `src/frontend/dotcards.c`, inside `ft_cktcoms()`:

```diff
     plot_cur = setcplot("op");
     if (plot_cur != NULL) {
-        assert(plot_cur->pl_dvecs != NULL);
+        /* An operating point has no reference vector, so its plot holds one
+           vector per output column and nothing else.  A circuit with no
+           non-ground node yields no columns, and the plot is empty.  That is
+           a property of the deck, not an invariant of this program, so it is
+           reported and returned the way the ft_curckt guard above does --
+           main.c turns the return into the process's exit status. */
+        if (plot_cur->pl_dvecs == NULL) {
+            fprintf(cp_err,
+                    "Error: incomplete or empty netlist\n"
+                    "       or no node to report an operating point for;\n"
+                    "no operating point printed!\n");
+            return 1;
+        }
         if (plot_cur->pl_dvecs->v_realdata != NULL) {
```

That is the whole of it. No header, no `Makefile.am` under `src/`, no helper,
no comparison of any kind — the test is a NULL check on a pointer. It is
candidate B from receipt 03, applied at the same site with the same shape;
`main.c:1581` already turns the `1` into `sp_shutdown(EXIT_BAD)`.

**The one deviation from receipt 03's measured patch text, and why.** That
patch's third line was `no simulations run!`, pasted from `main.c:1590` so the
measurement could answer whether criterion 2's literal sentence is producible
at this site. It is — byte for byte — and receipt 03 flagged that one clause of
it is false here: the operating point *did* run, and `No. of Data Rows : 1` is
on `stdout` immediately above the error. Shipping a sentence that contradicts
the line above it costs more than it buys, so the clause is
`no operating point printed!`, which is true and names what this function was
about to do. Criterion 2's stated requirement is unaffected: line 1 is
byte-identical to the sentence ngspice already uses to send a reader to their
netlist, line 2 names the exact shortfall, and nothing says "internal error".

**`#include <assert.h>` at `dotcards.c:12` is now unused and was left alone.**
It was the only `assert` in the file. Removing the include is a correct tidy-up
and an unrelated one; the item says "the guard, its diagnostic, and nothing
else", so it is left for whoever prepares the upstream diff to decide, and
flagged here rather than done quietly.

**The decks.** `tests/regression/exitstatus/`, driven by item 1's
`check_status.sh`:

| deck | shape | assertions |
| --- | --- | --- |
| `op-empty-netlist.cir` | **six bytes**, `*\n.op\n` — 0072's reproducer byte for byte | `.status` 1, `.err` the three lines, `.files` `absent rawspice.raw` |
| `op-empty-tran.cir` | `.op` + `.tran 1n 10n`, no netlist | `.status` 1, `.err` the three lines |
| `op-empty-control.cir` | `.op` + a `.control run` block, no netlist | `.status` 1, `.err` the three lines |

The first deck carries **no comment block**, deliberately: it is the
reproducer, and anything added to it is no longer the thing that was reported.
Its commentary lives in `op-empty-netlist.status`, whose `#` lines the driver
strips. The other two carry the usual header.

`Makefile.am`: three entries in `TESTS`, seven in `EXTRA_DIST`, twelve capture
files in `CLEANFILES`, plus `core` and `core.*` — see *What was left*. A header
paragraph in the file's existing voice says what the three decks are for.
`./autogen.sh` was re-run after every `Makefile.am` edit, and a bare rebuild
from `build-ver_50` after each.

## The two `make check` counts

Both are a full `make check` from `build-ver_50/`, and **both were run on this
crew's own builds** — the BEFORE row is not carried from receipt 03. The
before-state was produced by copying this crew's work to scratch, reverting
`src/` and the `Makefile.am` to `1c07b937b`, deleting the ten new files,
re-running `autogen.sh` and rebuilding; the tree was then restored the same way.

| run | binary | result |
| --- | --- | --- |
| `make check`, **BEFORE** — tree at `1c07b937b` | `0e740008…fe04` | **328 PASS / 0 FAIL**, `make` exit status 0 |
| `make check`, **AFTER** — the fix and the three decks | `1fa06748…7686` | **331 PASS / 0 FAIL**, `make` exit status 0 |

The difference is exactly the three new decks; no existing test moved in either
direction. `make -C build-ver_50/tests/regression/exitstatus check` is
**9 PASS / 0 FAIL**. Both lint specs report `identity lint: 264 comparisons,
baseline matches` and `identity lint: 11 comparisons, baseline matches` in both
runs — the same counts, so the fix adds no comparison and removes none.

## What was measured

0072's nine Acceptance Criteria, one row each, every cell a measurement.

| # | state | how it was measured |
| --- | --- | --- |
| **1** — three routes, normal termination, non-zero, diagnostic on stderr | **met** | 3 deck shapes × 3 routes (`--batch -n deck`, `-b -n < deck`, `-n deck < /dev/null`) run on BEFORE and AFTER. All nine rows: `rc=134` + the assertion line before, `rc=1` + the three-line diagnostic after. No signal in any row after. `stdout` under a redirect goes from 0 B to 173 B for the bare `.op` deck — the banner and `No. of Data Rows : 1` survive now, because nothing calls `abort()`. **Three of the nine rows are asserted by a committed deck**, the `--batch -n` column; the other six are measured and recorded here, because `check_status.sh` takes no per-deck invocation and `main.c:1175` makes all three routes the same `ft_batchmode` path. |
| **2** — the message names the deck's mistake | **met, one clause changed** | Shipped text is `Error: incomplete or empty netlist` / `       or no node to report an operating point for;` / `no operating point printed!`. Line 1 is byte-identical to `main.c:1590`'s, which is criterion 2's named candidate. The final clause is not `no simulations run!`; the reason is under *The production change*. Nothing says "internal error". Asserted by three `.err` files, so the wording cannot drift silently. |
| **3** — the site decided, the other named | **met** | `dotcards.c:225` taken. `outitf.c:479` rejected, and receipt 03's blockquote is now pasted into 0072's Resolution verbatim, with its number: B moves the nine table rows that are the defect and none that are not at 328/0; A moves fifteen, six unreported, at 327 PASS / 1 FAIL, the failure being `tests/regression/misc/empty-1.cir`. The guard returns before `:226`, so nothing dereferences NULL — the `SIGSEGV` trade the criterion warns about does not happen; measured as `rc=1`, not 139, in all nine rows of criterion 1. |
| **4** — nothing with a netlist changes | **met** | The eight witnesses named in 0072's Impact. The seven decks were run from their own directories on BEFORE and AFTER: `stderr` **byte-identical** for all seven (0, 1319, 0, 0, 38, 0, 223 B), and `stdout` identical for all seven once wall-clock timestamps and the resource-usage block are removed — the raw sha256 differs only because the run prints its own clock. Identical again under `check.sh`'s own `FILTER`, which is the comparison `make check` actually performs. The eighth witness, `tests/filters/lowpass.out`, keeps sha256 `fec8b85afb964252…`. `git status --short -- '*.out'` shows **no tracked `.out` modified**, and none was regenerated. Case modes: `tests/filters/lowpass.cir` run under `-D casemode=fold`, `preserve` and `distinguish` — identical before/after in all three, on both streams. `tests/regression/casedist/` (distinguish) and `tests/regression/case/` (preserve) pass inside both `make check` runs. |
| **5** — the `-r` route decided | **met, by not moving** | `ngspice -b -n -r out.raw tiny.cir`: `rc=0`, **193 bytes**, before and after, and `diff` of the two files with only the `Date:` and `Command:` lines removed is empty. Re-confirmed on this crew's builds, not carried: the file is a valid raw file, measured by reading it back through ngspice's own reader — `printf 'load out.raw\ndisplay\nquit 0\n' \| ngspice -p -n` prints `Loading raw data file … done.`, `Name: Operating Point`, `There are no vectors currently active.`, exits 0. The nodeless `.tran` `-r` file (681 B, `No. Variables: 1`) and a real `.tran` one (1658 B) are also unmoved. Not a third thing. **Measured, not asserted by a deck**, because the driver's command line is `--batch -n <deck>` with no `-r`. |
| **6** — a deck asserts it | **met** | Three decks, above. The criterion's own words are that no existing harness can, and that is still true of `check.sh` and of `pipe/`; the answer was to build one first. `tests/bin/check_status.sh` and `tests/regression/exitstatus/` are item 1 of this batch and existed before this item started. Its signal check is what makes the criterion's real property — *terminated normally rather than on a signal* — a named failure rather than an inference from a number, as the RED output above shows. |
| **7** — no new comparison | **met** | The change adds a NULL pointer test and an `fprintf`. `tests/lint/identity.baseline` sha256 `3dbd620e28d0dbca…`, unchanged and unstaged; `git diff` on it is empty. Both lint specs report `baseline matches` with the same counts (264, 11) before and after. |
| **8** — `make check` unchanged | **met** | 328 PASS / 0 FAIL → 331 PASS / 0 FAIL, `make` rc 0 both times, difference exactly the three new decks. The issue's own baseline of 322 is the number at filing; 328 is the same suite after items 1 and 2 of this batch added six decks. Case-mode coverage is inside those runs, plus row 4's three-mode run. |
| **9** — where it lands | **met** | Fixed here, on `ver_50`, commit `ce48f9630`. 0072's Resolution now records that as option 2 of the three it offered, and says plainly that the upstream material is item 5 of this batch, is not in that commit, and is not written yet. |

### The one thing this crew re-measured and found still true

`tests/regression/misc/empty-1.cir` — the deck receipt 03 found and the reason
candidate A was rejected — is **unmoved** by this fix on both streams: `rc=0`,
627 B of `stdout` with sha256 `fed21996502d8cfd…` before and after, `stderr`
empty. Its `op` on a zero-node circuit still runs and still reports
`No. of Data Rows : 1`.

And the fact that corrects 0072's Impact was verified directly rather than
inferred from candidate A's failure. The same deck with `echo $curplot` and
`display` added:

```
No. of Data Rows : 1
curplot is op1
There are no vectors currently active.
```

An `op1` plot, current, with `pl_dvecs == NULL`. So the empty plot the
assertion forbade **is** manufactured by an in-tree test today; what that test
does not do is reach `ft_cktcoms()`, because it carries no analysis dot card
and `main.c` therefore takes its `.control` arm. That is the corrected
sentence in 0072's Impact, and it is the sentence that decides the guard site:
the plot is not the defect, asserting on it in the epilogue was.

### The issue file

`doc/codex/issues/0072` rewritten in four places:

- **Status** — fixed, the site, the branch, the criteria summary, both
  `make check` counts, and which batch did what. The commit is named by its
  summary line with a pointer to this receipt for the hash, because a commit
  cannot contain its own hash and the item asks for one commit.
- **Impact** — the falsified sentence corrected in place, marked as a
  correction, with `empty-1.cir` quoted and the `curplot is op1` measurement.
  The scan is described for what it actually covered (`.op` **dot cards**), and
  the conclusion is replaced by the one the corrected fact supports: what the
  suite lacks is not an empty circuit but an empty circuit **plus an analysis
  dot card**, and `empty-1.cir` is evidence that the empty plot is a state
  upstream ships and asserts.
- **Root Cause** — its closing sentence claimed the decision "belongs at
  `outitf.c:479` rather than in the epilogue". That is the same claim the
  measurement falsified, so it is corrected in place and marked as such. Not
  named in the item's list of three sections; corrected because leaving it
  would have left the file arguing against its own Resolution.
- **Resolution** — the code, the wording decision, receipt 03's rejection
  blockquote pasted whole, the nine-row criteria table, and *Where it lands*.
  The paragraph that preferred `beginPlot()` "on the merits" is replaced and
  the replacement says why it was wrong.

Also verified: `ngspice --version`'s banner, `configure.ac`, `src/Makefile.am`
— no `-DNDEBUG` anywhere, so the assertion was live in release builds, as the
issue's Impact says. That is unchanged and is why this is not a debug-only fix.

## What was left, deliberately

- **`src/frontend/outitf.c` was not opened.** Candidate A was not implemented
  and is not present in any form. `git diff 1c07b937b..HEAD -- src/frontend/`
  touches one file.
- **`#include <assert.h>` in `dotcards.c` is now dead and was kept.** Named
  above. It is one line, it is harmless, and removing it is a separate change
  from the one this item authorised. Whoever writes item 5's upstream diff
  should decide; a reviewer will ask.
- **The `-b -n < deck` and no-tty routes are measured, not asserted.** Both are
  in the criterion-1 table above with their numbers. Making them assertions
  would mean a per-deck flag file in `check_status.sh`, and the item forbids
  touching that driver — correctly, since receipt 01 already recorded that a
  per-deck invocation is a change to the driver's contract and not a
  configuration of it. The same reasoning covers criterion 5's `-r` row.
- **`core` and `core.*` are in `CLEANFILES` and match nothing on this
  machine.** Receipt 01 left the decision to whoever produced the crash, which
  is this crew. Measured: `/proc/sys/kernel/core_pattern` here is
  `|/wsl-capture-crash %t %E %p %s`, so the RED runs wrote no core file into
  the build directory and `ls core core.*` finds nothing after either. They are
  listed because a machine with the default pattern would have written one, and
  a deck whose subject is an abort should not be able to leave an unlisted
  artefact if it ever regresses. Stated so the entry is not read as measured.
- **`op-empty-tran` and `op-empty-control` have no `.files` sibling.** Only the
  primary deck asserts `absent rawspice.raw`. All three were measured to write
  no rawfile; repeating the line twice more would add a `CLEANFILES` entry and
  no information.
- **No `.out` file was regenerated, weakened or deleted**, and no assertion was
  loosened. Nothing moved that needed repairing: `git status --short -- '*.out'`
  is empty of tracked files.
- **The `-s` stdout path, CIDER, and `--with-ngshared`** were not measured, per
  the batch's Out of scope. The shared library cannot reach `ft_cktcoms()` at
  all — `grep -rn ft_cktcoms src/` still finds exactly two call sites, both in
  `main.c`'s batch block.
- **Nothing upstream.** No file under `doc/claude/upstream/` was opened or
  referenced, no `upstream-0072/` directory was created, and neither this
  receipt nor 0072 says that anything has been sent. Item 5 owns that material.

## For the owner

1. **The diagnostic's last clause is a decision, not an inheritance.** Criterion
   2's literal sentence ends `no simulations run!`; this ships
   `no operating point printed!` because the operating point ran and says so on
   the line above. If you want the literal sentence instead, it is one string
   and three `.err` files; receipt 03 measured that the site can print it byte
   for byte. Flagged because it is the only place this fix does not do exactly
   what the criterion's text says.
2. **0072's Root Cause needed correcting too, and that was not on the list.**
   Its closing sentence said the decision belongs at `outitf.c:479`. Leaving it
   would have left the issue arguing against its own Resolution, so it was
   corrected in place and marked. Say if you would rather that sentence had
   been left alone.
3. **The issue names the commit by summary, not by hash.** One commit was asked
   for, and a commit cannot contain its own hash. The hash is `ce48f9630` and
   it is here, in the file the issue points at. If you would rather 0072 carried
   the hash, that is a one-line follow-up commit.
4. **Criterion 6 is met, and it is worth reading what it cost.** The criterion
   said no existing harness could assert this; three items of this batch went
   into making one that could, and the payoff is visible in the RED block above
   — the failure names `SIGABRT` rather than reporting "expected 1, got 134".
   The six-byte deck is now a permanent test, and a regression to the abort
   fails three named checks in one run.
5. **Item 5 has what it needs.** The production diff is one hunk against
   upstream-identical code, the reproducer is `printf '*\n.op\n'`, and the
   before/after numbers are in this receipt's tables. `tests/regression/`
   does not exist upstream in this shape, so the upstream diff is the
   `dotcards.c` hunk alone.

Nothing was pushed. One commit on `ver_50`, `ce48f9630`, plus this receipt.
