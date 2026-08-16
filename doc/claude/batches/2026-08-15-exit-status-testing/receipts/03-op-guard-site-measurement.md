# Receipt: item 3 — 0072's two guard sites, measured

PLAN item **3 — 0072: measure both guard sites, decide, record**, from
`doc/claude/batches/2026-08-15-exit-status-testing/PLAN.md`.

Crew C, 2026-08-15, branch `ver_50`. HEAD at start and at finish **`2699f9969`**.

**No production code was committed, and none is left applied.** Three scratch
patches were built, measured and reverted. `git status --short src/` is empty
and the pristine rebuild is back at **328 PASS / 0 FAIL** — see *The two
`make check` counts*. Nothing under `doc/claude/upstream/` was opened, and
nothing here states or implies that any upstream submission has been sent.
`doc/codex/issues/0072` was not edited; item 4 owns it.

**Binaries.** Everything was run in place at `build-ver_50/src/ngspice`, never
copied. The banner's `Creation Date` does **not** move between these builds —
`Spice_Build_Date` is regenerated on configure, not on compile — so it is not a
discriminator here and the binaries are identified by content hash instead:

| build | `sha256sum build-ver_50/src/ngspice` | banner |
| --- | --- | --- |
| pristine (start, and again at finish) | `1280b0bece82f2f0…d7ea` (finish) | `ngspice-46+`, Creation Date `Sun Aug 16 00:00:02 UTC 2026` |
| candidate A | `418e55841d31a062…0360` | same banner |
| candidate A′ | `f8b561b616e48038…3627` | same banner |
| candidate B | `65876eac01dcb97b…1ee9` | same banner |
| stock third column | `/usr/local/bin/ngspice` | `ngspice-46`, Creation Date `Sun Aug 2 23:29:26 UTC 2026` |

Scratch: `/tmp/claude-1000/-home-qflow-dev-ngspice-test/d775575c-7554-4eec-9dca-17c55bd7d1cf/scratchpad`
(`mkdecks.sh`, `measure.sh`, `empty1.sh`, `patchA.py`, `patchAr.py`,
`patchB.py`, `results/`). Nothing was left in the repository or the build tree —
`find tests/ -newermt '2026-08-15 17:00'` returns only item 2's committed files,
and no `core` file survives anywhere in the tree.

## The RED failure, verbatim

This item writes no test, so the "RED" is the defect reproducing on today's
pristine binary before anything was patched. Re-measured, not copied from 0072:

```
$ printf '*\n.op\n' > tiny.cir
$ ngspice --batch -n tiny.cir > so.txt 2> se.txt ; echo rc=$?
/bin/bash: line 1: 4025476 Aborted (core dumped) …
rc=134
$ wc -c so.txt se.txt
  0 so.txt
106 se.txt
$ cat se.txt
ngspice: ../../../src/frontend/dotcards.c:225: ft_cktcoms: Assertion `plot_cur->pl_dvecs != NULL' failed.
```

Stock `ngspice-46` gives the byte-identical answer, `rc=134`, same file, same
line. Every row of 0072's three tables was re-measured on the pristine binary
and every one of them reproduced; the "before" columns below are those
measurements, not 0072's.

## The production change

**None committed.** Three scratch patches, quoted here so item 4's crew can
apply whichever the driver picks. Each was applied with a script that asserts
the pristine text is present, so a half-applied patch could not have been
measured.

### Candidate A — `src/frontend/outitf.c:479`, drop the `numNames &&` conjunct

```diff
--- a/src/frontend/outitf.c
+++ b/src/frontend/outitf.c
@@ -476,9 +476,8 @@ beginPlot(JOB *analysisPtr, CKTcircuit *circuitPtr, char *cktName, char *analNam
             tfree(savesused);
         }
 
-        if (numNames &&
-            ((run->numData == 1 && run->refIndex != -1) ||
-             (run->numData == 0 && run->refIndex == -1)))
+        if ((run->numData == 1 && run->refIndex != -1) ||
+            (run->numData == 0 && run->refIndex == -1))
         {
             fprintf(cp_err, "Error: no data saved for %s; analysis not run\n",
                     spice_analysis_get_description(analysisPtr->JOBtype));
```

### Candidate A′ — the same site, *relaxed* rather than dropped

0072 criterion 3 says "drop **or relax**", and the two are not the same change,
so both were measured. A′ keeps `numNames` on the reference-vector disjunct and
removes it only from the `.op` disjunct — i.e. it fires only for the state this
issue is about:

```diff
--- a/src/frontend/outitf.c
+++ b/src/frontend/outitf.c
@@ -476,9 +476,8 @@ beginPlot(JOB *analysisPtr, CKTcircuit *circuitPtr, char *cktName, char *analNam
             tfree(savesused);
         }
 
-        if (numNames &&
-            ((run->numData == 1 && run->refIndex != -1) ||
-             (run->numData == 0 && run->refIndex == -1)))
+        if ((numNames && run->numData == 1 && run->refIndex != -1) ||
+            (run->numData == 0 && run->refIndex == -1))
         {
```

### Candidate B — `src/frontend/dotcards.c:225`, assertion → test, diagnostic, return

In the idiom the same function already uses at `:204`. The wording below is
the criterion-2 shape, chosen so the measurement answers "can this site produce
that sentence"; see *The diagnostic each site can produce* for the one word in
it that is not true.

```diff
--- a/src/frontend/dotcards.c
+++ b/src/frontend/dotcards.c
@@ -222,7 +222,13 @@ ft_cktcoms(bool terse)
     /* If there was a .op line, then we have to do the .op output. */
     plot_cur = setcplot("op");
     if (plot_cur != NULL) {
-        assert(plot_cur->pl_dvecs != NULL);
+        if (plot_cur->pl_dvecs == NULL) {
+            fprintf(cp_err,
+                    "Error: incomplete or empty netlist\n"
+                    "       or no node to report an operating point for;\n"
+                    "no simulations run!\n");
+            return 1;
+        }
         if (plot_cur->pl_dvecs->v_realdata != NULL) {
```

Neither A nor B is the change 0072 warns against — neither deletes the
assertion and lets `:226` dereference NULL. Both terminate normally with a
status; no candidate produced a signal in any row of any table.

## The two `make check` counts

Every count is a full `make check` from `build-ver_50/`, ~63 s per run.

| build | invocation | result |
| --- | --- | --- |
| pristine, at start | `make check` | **328 PASS / 0 FAIL**, `make` rc 0 |
| **candidate A** | `make check` | **53 PASS / 1 FAIL**, `make` rc 2 — the recursion aborts in `tests/regression/misc` and the remaining directories never run |
| **candidate A** | `make -k check` | **327 PASS / 1 FAIL** — `FAIL: empty-1.cir` |
| **candidate A′** | `make -k check` | **328 PASS / 0 FAIL**, `make` rc 0 |
| **candidate B** | `make -k check` | **328 PASS / 0 FAIL**, `make` rc 0 |
| pristine, at finish | `make check` | **328 PASS / 0 FAIL**, `make` rc 0 |

`identity lint: 264 comparisons, baseline matches` and `identity lint: 11
comparisons, baseline matches` in all five runs. `tests/lint/identity.baseline`
is untouched by every candidate; none of them adds a string comparison.

## What was measured

### Table 1 — the analysis table

Deck is a title line plus the cards named, and **no netlist body**. `rc` and
the salient line of what is reported. `A′` is included because it is a
different change from `A`.

| deck | before | A | A′ | B |
| --- | --- | --- | --- | --- |
| `.op` | **134** assertion | 1 `no data saved for D.C. Operating point analysis` | 1 same | 1 `incomplete or empty netlist … ` |
| `.op` + `.tran 1n 10n` | **134** assertion | 1 same, twice (op then tran) | 1 same | 1 same |
| `.op` + a `.control run` block | **134** assertion | 1 same, twice | 1 same, twice | 1 same |
| `.tran 1n 10n` | 1 `incomplete or empty netlist` | 1 unchanged | 1 unchanged | 1 unchanged |
| `.dc v1 0 1 .1` | 1 the same | 1 unchanged | 1 unchanged | 1 unchanged |
| `.ac dec 10 1 1k` | 1 the same | 1 unchanged | 1 unchanged | 1 unchanged |
| `.print tran v(1)` + `.tran` | 0 `Error: no such vector 1` | **1** `no data saved for Transient analysis` | 0 unchanged | 0 unchanged |
| `.print ac v(1)` + `.ac` | 0 the same | **1** `no data saved for A.C. Small signal analysis` | 0 unchanged | 0 unchanged |
| `.print dc v(1)` + `.dc` | 1 `Fatal error: DC Transfer Function: … "v1" is not in the circuit` | 1 unchanged | 1 unchanged | 1 unchanged |
| `.four 1k v(1)` + `.tran` | 0 `Error: no such vector 1` | **1** `no data saved for Transient analysis` + `No transient data available for fourier analysis` | 0 unchanged | 0 unchanged |
| `.tf v(1) v1` | 1 `Warning: Transfer function source v1 not in circuit` | 1 unchanged | 1 unchanged | 1 unchanged |
| `.options noacct` only | 1 `incomplete or empty netlist` | 1 unchanged | 1 unchanged | 1 unchanged |

Every "before" cell agrees with 0072's table. **A changes three rows that are
not the reported defect** (`.print tran`, `.print ac`, `.four`); A′ and B change
none.

### Table 2 — the route table

Deck is `printf '*\n.op\n'` throughout except the last row.

| route | before | A | A′ | B |
| --- | --- | --- | --- | --- |
| `ngspice --batch -n deck` | **134** | 1 | 1 | 1 |
| `ngspice -b -n < deck` | **134** | 1 | 1 | 1 |
| `ngspice -n deck < /dev/null` | **134** | 1 | 1 | 1 |
| `ngspice -b -n -r out.raw deck` | 0, 193-byte rawfile | **1, no file** | **1, no file** | 0, 193-byte rawfile — unchanged |
| tty (`script -qec`, `quit 0` typed) | 0, reaches the prompt | 0 unchanged | 0 unchanged | 0 unchanged |
| `'source deck; run' \| ngspice -p -n` | 0, empty stderr | 0, **but stderr gains** `no data saved …` | 0, same | 0 unchanged, empty stderr |
| `'op' \| ngspice -p -n` | 0 `Error: there aren't any circuits loaded.` | 0 unchanged | 0 unchanged | 0 unchanged |
| a `.control` block whose only card is `op` | 0 `Note: Simulation executed from .control section` | **1** `no data saved …` then `incomplete or empty netlist … no simulations run!` | **1** same | 0 unchanged |

The tty row needed a pty and a typed `quit 0`, because a run that does *not*
abort sits at the prompt for ever; the property being measured is that no
candidate turns that row into a signal, and none does.

**The last row is the one to read twice.** 0072 recorded it as the row that
"corrects the shape this was first described in": a `.control`-only `op` with no
netlist runs and exits 0 today. A and A′ both turn it into a failure. B leaves
it alone. That row is not a hypothetical — it is `tests/regression/misc/empty-1.cir`,
below.

### Table 3 — the netlist-body table

All four decks carry `.op`.

| netlist body | before | A | A′ | B |
| --- | --- | --- | --- | --- |
| nothing | **134** | 1 | 1 | 1 |
| `r1 0 0 1k` | **134** | 1 | 1 | 1 |
| `.model m1 nmos` | **134** | 1 | 1 | 1 |
| `v1 1 0 1` / `r1 1 0 1k` | 0 | 0 unchanged | 0 unchanged | 0 unchanged |

**stdout is no longer destroyed under any candidate**, which is the second half
of 0072's Impact. Bytes on `stdout` for the `.op` deck under a redirect:

| build | stdout | stderr |
| --- | --- | --- |
| before | **0** (`abort()` does not flush stdio) | 149 (106 + the shell's `Aborted` line) |
| A | 516 | 122 |
| A′ | 516 | 122 |
| B | 173 | 107 |

### Table 4 — criterion 5, the `-r` route, at the byte level

Criterion 5: the file is a valid raw file, or the run refuses to write one —
not a third thing.

| `-r` run | before | A | A′ | B |
| --- | --- | --- | --- | --- |
| `.op`, no netlist | rc 0, **193 bytes** written | rc 1, **file ABSENT** | rc 1, **file ABSENT** | rc 0, **193 bytes** — unchanged |
| `.tran`, no netlist | rc 0, 681 bytes, `No. Variables: 1` | rc 1, **file ABSENT** | rc 0, 681 bytes — unchanged | rc 0, 681 bytes — unchanged |
| `.tran`, real netlist | rc 0, 1658 bytes, `No. Variables: 3` | rc 0 unchanged | rc 0 unchanged | rc 0 unchanged |

The 193 bytes, verbatim (`od -c`, decoded):

```
Title: *
Date: Sat Aug 15 17:29:51  2026
Command: ngspice-46+, Build Sun Aug 16 00:00:02 UTC 2026
Plotname: Operating Point
Flags: real
No. Variables: 0
No. Points: 1
Variables:
Binary:
```

**Is that a valid raw file? Measured, by reading it back with ngspice's own
reader** rather than by inspection:

```
$ printf 'load zero.raw\ndisplay\nquit 0\n' | ngspice -p -n
Loading raw data file ("zero.raw") ...
done.
Title:  *
Name: Operating Point
Date: Sat Aug 15 17:29:51  2026

There are no vectors currently active.
ngspice 10002 -> display
There are no vectors currently active.
rc=0
```

So criterion 5 is satisfiable both ways and neither answer is "a third thing":
**under B the row does not move and the file it writes is a valid raw file
ngspice can load**; under A and A′ the run refuses to write one at all and
returns 1. A additionally stops the nodeless-`.tran` `-r` file being written,
which is a second row moving.

### Table 5 — the eight in-tree `.op` decks (criterion 4)

Run by hand from their own directories, `--batch -n`, on every build:

| deck | before | A | A′ | B |
| --- | --- | --- | --- | --- |
| `tests/jfet/jfet_vds-vgs.cir` | rc 0, 6502 B stdout | identical | identical | identical |
| `tests/vbic/diffamp.cir` | rc 1, 2929 B (gmin/source stepping fails — pre-existing) | identical | identical | identical |
| `tests/polezero/filt_bridge_t.cir` | rc 0, 4174 B | identical | identical | identical |
| `tests/filters/lowpass.cir` | rc 0, 5064 B | identical | identical | identical |
| `tests/mesa/mesa11.cir` | rc 0, 11036 B | identical | identical | identical |
| `tests/resistance/res_partition.cir` | rc 0, 4457 B | identical | identical | identical |
| `tests/resistance/res_array.cir` | rc 1, 13166 B, already prints `Error: no data saved for A.C. Small signal analysis; analysis not run` | identical | identical | identical |
| `tests/filters/lowpass.out` (reference file) | sha256 `fec8b85a…` | unchanged | unchanged | unchanged |

All seven committed `.out` files keep their sha256 under every candidate, and
the whole `make check` comparison of them passes in every run. **Criterion 4 is
met by A, A′ and B alike** — for these eight. `res_array.cir` is worth noting
separately: it is the in-tree witness that candidate A's message is *already*
reachable today, for the `numNames > 0` case. It does **not** appear in any
committed `.out` — `grep -rn 'no data saved' tests/ --include='*.out'` returns
nothing, because the message goes to `stderr` and `check.sh` captures only
`stdout`. So the eight decks prove criterion 4 for `stdout` and prove nothing
about `stderr`, which is the gap item 1's driver exists to close.

### The thing 0072 did not anticipate — `tests/regression/misc/empty-1.cir`

**This is the measurement that decides the item.** 0072's Impact says: *"No
deck in the tree has an analysis card and an empty netlist, so `make check`
exercises `ft_cktcoms()` only with populated plots."* That is true of `.op`
**dot cards** — the scan was `^\s*\.op(?![a-z])` — and false of the tree. This
deck has existed upstream all along:

```
check that we can survive emptiness

* (exec-spice "ngspice -b %s")

* Nothingness, No circuit elements at all.
*   We have only the implicit node "0"
* Checks whether we can live with a
*   zero times zero circuit matrix

.control
op
tran 0.1m 1m
ac lin 3 1kHz 2kHz
echo "TEST: done"
quit 0
.endc

.end
```

Its title is the specification. It runs `op`, `tran` and `ac` on a zero-node
circuit from a `.control` block and asserts that all three survive; its
committed `.out` carries `No. of Data Rows : 1`, `59` and `3` for the three.
It is not an artefact of this branch — stock `ngspice-46` runs it at `rc=0`
with empty stderr.

**Candidate A breaks it.** The `make check` failure, verbatim from
`tests/regression/misc`:

```
PASS: dollar-1.cir
Error: no data saved for D.C. Operating point analysis; analysis not run
doAnalyses: not found

op simulation(s) aborted
Error: no data saved for Transient analysis; analysis not run
doAnalyses: not found

tran simulation(s) aborted
Error: no data saved for A.C. Small signal analysis; analysis not run
doAnalyses: not found

ac simulation(s) aborted
--- empty-1.out_tmp	2026-08-15 17:27:47.874766161 -0700
+++ empty-1.test_tmp	2026-08-15 17:27:47.870766164 -0700
@@ -5,13 +5,4 @@
 
 
 
--Initial Transient Solution
--
--Node                                   Voltage
--
--
--
--
--
--
 TEST: done
FAIL: empty-1.cir
```

All three analyses in the deck refuse to run. This is the `1 FAIL` and it is
why plain `make check` under A stops after 53 tests.

**Candidate A′ does not fail it — and that is the more interesting number.**
A′ scores 328 PASS / 0 FAIL, but the deck's behaviour still changes, and it
changes invisibly. The unfiltered run, A′ against pristine:

| | before / B | A′ |
| --- | --- | --- |
| rc | 0 | 0 (`quit 0` inside `.control` forces it) |
| stdout | contains `No. of Data Rows : 1` for the `op` | **that line is gone** |
| stderr | empty | `Error: no data saved for D.C. Operating point analysis; analysis not run` / `doAnalyses: not found` / `op simulation(s) aborted` |

It passes because `tests/bin/check.sh`'s `FILTER` contains `Data`, so
`No. of Data Rows` is stripped from both sides before the `diff`; because
`check.sh` never redirects `stderr`; and because the deck's own `quit 0` sets
the status. Three independent blind spots, and A′ lands in all three at once.
So **A′'s 328 PASS is not evidence that A′ is harmless** — it is evidence that
the harness cannot see what A′ did. The same run under B is byte-identical to
pristine, stdout and stderr both.

### The exit-status decks from items 1 and 2

All six decks in `tests/regression/exitstatus/`, run through the same
`--batch -n` the driver uses:

| deck | before | A | A′ | B |
| --- | --- | --- | --- | --- |
| `tran-empty-netlist` | rc 1, `incomplete or empty netlist …` | identical | identical | identical |
| `selftest-status` | rc 0 | identical | identical | identical |
| `sim-status-guard` | rc 1, `no data saved for D.C. Operating point analysis` | identical | identical | identical |
| `sim-status-guard-ok` | rc 0 | identical | identical | identical |
| `sim-status-properties` | rc 0 | identical | identical | identical |
| `sim-status-unset` | rc 0, `Error: sim_status: no such variable.` | identical | identical | identical |

**No candidate touches item 1's `.tran`-with-empty-netlist deck or item 2's four
`$sim_status` decks.** `sim-status-guard` is worth a note: it already produces
candidate A's message today, from the `numNames > 0` arm, which is the in-tree
proof that `Error: no data saved for %s; analysis not run` on `stderr` with
`rc=1` is a shape the harness can assert.

### The diagnostic each site can produce

**Candidate A's message is the existing one and it cannot be the criterion-2
sentence.** Verbatim on `stderr` for the six-byte deck:

```
Error: no data saved for D.C. Operating point analysis; analysis not run
doAnalyses: not found

run simulation(s) aborted
```

Does a reader of that know to look at the netlist? **No.** The words are
"no data saved", which in ngspice's vocabulary is about output selection —
`.save`, `.print`, `.plot` — and the first thing a reader checks is their
`.save` lines. The sentence is literally the same one `res_array.cir` prints
today for a completely different cause (an `.ac` whose columns were filtered
out), so it cannot distinguish "you saved nothing" from "there was nothing to
save". At `beginPlot()` that ambiguity is structural: the function knows
`numNames == 0` but does not know whether that is an empty netlist or a
one-node circuit whose only column was filtered. Producing the criterion-2
wording at that site would mean adding a *second* message keyed on
`numNames == 0` — which is a larger change than "drop or relax the conjunct",
and is not what criterion 3 costs A at.

It does clear criterion 2's floor: it does not say "internal error".

**Candidate B can produce any wording, because it owns its own `fprintf`.** The
patch above prints, verbatim:

```
Error: incomplete or empty netlist
       or no node to report an operating point for;
no simulations run!
```

Two of those three lines are the criterion-2 sentence byte for byte, and a
reader of line 1 knows exactly where to look. **One word in it is not true:**
`no simulations run!` — the operating point *did* run, and `No. of Data Rows : 1`
is on `stdout` immediately above it. That is a wording decision for item 4, not
a property of the site; the site can print `no operating point to report!` or
anything else. It is flagged here because pasting `main.c:1590`'s three lines
unchanged buys the criterion-2 wording at the price of one false clause.

## The recommendation

**Take candidate B, `src/frontend/dotcards.c:225`.** The number: **B changes
exactly the nine rows of the three tables that *are* the defect and zero rows
that are not, and scores 328 PASS / 0 FAIL; candidate A changes six further
rows that are not the defect and scores 327 PASS / 1 FAIL, the failure being
`tests/regression/misc/empty-1.cir`, an upstream deck whose title is "check
that we can survive emptiness" and whose entire purpose is to assert that the
zero-node circuit A would forbid still runs.** That is not a test that can be
adjusted, because it is the specification of the behaviour A removes: a deck
saying "we have only the implicit node 0, check whether we can live with a zero
times zero circuit matrix", passing on released `ngspice-46`, would have to be
deleted or rewritten for A to land. B additionally leaves 0072 criterion 5
answered by not moving — the `-r` row stays `rc=0` and its 193-byte
`No. Variables: 0` file was measured to load back cleanly through ngspice's own
`load` command, so it is a valid raw file and not a third thing — while A turns
that row into "no file at all" and takes the nodeless-`.tran` `-r` file with it.

## The case against `outitf.c:479`, for 0072 criterion 3

Recorded in the form the issue needs, so item 4 can paste it:

> **`beginPlot()` (`outitf.c:479`) was measured and rejected.** Dropping the
> `numNames &&` conjunct fixes the abort but takes six unrelated rows with it,
> and one of them is an in-tree test. `tests/regression/misc/empty-1.cir` —
> upstream, titled *"check that we can survive emptiness"*, a `.control` block
> running `op`, `tran` and `ac` on a circuit whose only node is the implicit
> ground — fails: all three analyses stop at `Error: no data saved for …;
> analysis not run` and the committed `.out` loses its
> `Initial Transient Solution` block. Full `make check` with the patch applied
> is **327 PASS / 1 FAIL** against a 328 PASS / 0 FAIL baseline, and plain
> `make check` aborts the recursion after 53 tests. The deck passes on released
> `ngspice-46`, so this is a behaviour upstream ships and asserts, not an
> artefact of this branch. 0072's Impact statement that no in-tree deck has an
> analysis card with an empty netlist is true only of `.op` **dot cards**; the
> scan `^\s*\.op(?![a-z])` does not see a `.control` block's bare `op`.
>
> Three further rows move that are nobody's bug report: a nodeless deck with
> `.print tran v(1)` + `.tran` goes from `rc=0` to `rc=1`, and so do the
> `.print ac` and `.four` variants — `numNames == 0` reaches `beginPlot()` from
> every analysis, not just `.op`, and for those the plot already holds its
> reference column and is not empty at all. And 0072 criterion 5's `-r` row
> moves twice over: the `.op` run stops writing a rawfile, and so does the
> nodeless `.tran` run, which was writing a perfectly good `No. Variables: 1`
> file.
>
> A narrower variant was also measured — keeping `numNames &&` on the
> reference-vector disjunct and removing it only from the `.op` disjunct. It
> scores 328 PASS / 0 FAIL, but that is a harness artefact, not safety: it
> still stops `empty-1.cir`'s `op`, and the change is invisible only because
> `check.sh`'s `FILTER` contains `Data` (so the vanished `No. of Data Rows : 1`
> is stripped from both sides), because `check.sh` never captures `stderr` (so
> the three new error lines are discarded), and because the deck's own `quit 0`
> sets the status. It also still turns the `.control`-only `op` route and the
> `.op` `-r` route from `rc=0` into `rc=1`.
>
> Finally, the message. `beginPlot()`'s existing text is `Error: no data saved
> for %s; analysis not run`, which is what `tests/resistance/res_array.cir`
> already prints for an unrelated cause, and it never mentions the netlist —
> so a reader of it checks their `.save` lines, not their circuit. Producing
> criterion 2's wording there would need a second message keyed on
> `numNames == 0`, which is more than the one-conjunct change criterion 3
> costed. `ft_cktcoms()` owns its own `fprintf` and can print the sentence
> directly.

## What was left, deliberately

- **0072 is not fixed and neither candidate is left applied.** `git status
  --short src/` is empty; the pristine rebuild is 328 PASS / 0 FAIL. Item 4
  writes the fix and the deck.
- **`doc/codex/issues/0072` is not edited.** Item 4 owns its Status, its
  Resolution and its nine criteria. The blockquote above is written to be
  pasted into criterion 3 as-is.
- **No wording was chosen for B's diagnostic.** The patch quoted here uses the
  criterion-2 sentence so the measurement could answer "is that sentence
  producible at this site" — it is, byte for byte. The `no simulations run!`
  clause is false for this deck and item 4 should decide whether to keep it.
- **B's `return 1` is an early return** at `:225`, ahead of the `.` line loop
  at `:275`, the `tf` loop at `:262`, the options print and the accounting
  block — exactly like the existing `:204` guard. Nothing is lost for the decks
  that reach it, because a deck whose `.op` plot has no vectors has no netlist
  to print anything about; measured on `*` / `.op` / `.print tran v(1)` /
  `.tran`, which is `rc=134` today, so no output is being traded away. Stated
  because it is the one control-flow consequence of B that the tables do not
  show.
- **The `-s` stdout path and CIDER were not measured**, per the batch's Out of
  scope.
- **No `.out` file was regenerated, weakened or deleted.** The one that moved
  moved because candidate A moved it, and it is reported rather than repaired.

## For the owner

1. **0072's Impact needs one correction and item 4 should carry it.** "No deck
   in the tree has an analysis card and an empty netlist" is false —
   `tests/regression/misc/empty-1.cir` is exactly that, via a `.control` block,
   and it is the deck that decides this item. The claim came from a scan for
   `.op` dot cards, which cannot see a bare `op` inside `.control`.
2. **The decision is not close.** B is 328/0 and moves nine rows, all of them
   the defect. A is 327/1 and moves fifteen, six of which nobody complained
   about. The relaxed variant A′ is 328/0 but only because three separate
   blind spots in `check.sh` hide what it did to `empty-1.cir` — it is the
   option that would have looked safe and was not, and it is the reason this
   item was worth running rather than reasoning about.
3. **0072's own Resolution currently prefers `beginPlot()` on the merits** —
   "it is where the empty plot is manufactured … stopping the plot from existing
   fixes consumers this issue has not enumerated". That paragraph was written
   before the measurement and the measurement contradicts it: the consumers the
   issue had not enumerated include one that *wants* the empty plot. Item 4
   will need to rewrite that paragraph as well as criterion 3.
4. **Criterion 5 is answered by B without a decision being forced.** The `-r`
   row does not move, and the `No. Variables: 0` file it writes was measured to
   round-trip through ngspice's own `load`. Item 4 can assert that row with the
   harness's `present` + `grep` verbs from item 1 rather than arguing about it.
5. **Item 4's RED deck is already characterised.** Receipt 01 §4 has the
   verbatim double failure (`signal` then `status`) for `printf '*\n.op\n'` with
   a `.status` of `1`; this receipt confirms `rc=134` still reproduces on the
   pristine binary at `2699f9969`, and that under B the same deck gives `rc=1`
   with a three-line `stderr` an `.err` file can hold exactly.

Nothing was pushed. One commit on `ver_50`, this receipt only.
