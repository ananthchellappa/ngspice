# `.op` with no netlist aborts on an assertion in `ft_cktcoms()`

Report accompanying `0001-frontend-report-an-op-card-with-no-netlist.patch`.

**What this is.** A deck consisting of a title line and an `.op` card, with no
netlist, makes ngspice die on `SIGABRT` in the batch-mode dot-card epilogue.
It reproduces on `ngspice-46` as released. This is a mistake-shaped input: no
correct deck produces it, no data is lost that would otherwise have been
written, no wrong answer is computed, and the analysis has already finished
when the abort fires. The patch replaces the assertion with a test, a
diagnostic and a return.

All measurements below were taken on 2026-08-15 on x86-64 Linux (WSL2, kernel
6.6.87.2), on the two binaries in the table near the end.

---

## 1. Reproducer

Six bytes:

```
$ printf '*\n.op\n' > tiny.cir
$ wc -c tiny.cir
6 tiny.cir
```

Run on released `ngspice-46`, output verbatim:

```
$ /usr/local/bin/ngspice --batch -n tiny.cir ; echo rc=$?
ngspice: ../../../src/frontend/dotcards.c:225: ft_cktcoms: Assertion `plot_cur->pl_dvecs != NULL' failed.
Aborted (core dumped)
rc=134
```

The `Aborted (core dumped)` line is the invoking shell's, not ngspice's.

The title line is required. Without one, `.op` becomes the title, the deck is
empty, and ngspice already diagnoses that properly: `Warning: Empty netlist!`,
then the `incomplete or empty netlist` error, `rc=1`. `.end` is not required.

## 2. It reproduces on released `ngspice-46`

Measured on the stock install, not on a development tree:

```
$ /usr/local/bin/ngspice --version
******
** ngspice-46 : Circuit level simulation program
** Compiled with KLU Direct Linear Solver
** The U. C. Berkeley CAD Group
** Copyright 1985-1994, Regents of the University of California.
** Copyright 2001-2025, The ngspice team.
** Please get your ngspice manual from https://ngspice.sourceforge.io/docs.html
** Please file your bug-reports at http://ngspice.sourceforge.net/bugrep.html
** Creation Date: Sun Aug  2 23:29:26 UTC 2026
******
```

`sha256(/usr/local/bin/ngspice)` =
`b127abce0b8d1e67ce99229a769d6a7cd4e6646c2c1002df51a1590766ab105b`.

The assertion is live in that build. It is a release build, and `NDEBUG` is
defined nowhere in `configure.ac`, `src/Makefile.am`, `m4/` or
`compile_linux.sh`, so the assertion is not compiled out of a shipped binary.

## 3. The failure

- Site: `src/frontend/dotcards.c:225`, in `ft_cktcoms()`. The same line number
  in the current tree and in the released binary's message.
- `rc=134` is `128 + 6`; `kill -l 6` is `ABRT`.
- `stderr` is **106 bytes**: the assertion line and nothing else.
- `stdout` is **empty under a redirect**, because `abort()` does not flush
  stdio:

  ```
  $ /usr/local/bin/ngspice --batch -n tiny.cir > so.txt 2> se.txt ; echo rc=$?
  rc=134
  $ wc -c so.txt se.txt
    0 so.txt
  106 se.txt
  ```

  The run does produce that output. Under a pty it is line-buffered and
  appears:

  ```
  $ script -qec '/usr/local/bin/ngspice --batch -n tiny.cir; echo rc=$?' /dev/null

  Note: No compatibility mode selected!


  Circuit: *

  Doing analysis at TEMP = 27.000000 and TNOM = 27.000000

  Using SPARSE 1.3 as Direct Linear Solver

  No. of Data Rows : 1
  ngspice: ../../../src/frontend/dotcards.c:225: ft_cktcoms: Assertion `plot_cur->pl_dvecs != NULL' failed.
  rc=134
  ```

  So the analysis is not what fails. The operating point is computed,
  `No. of Data Rows : 1` is printed, and the dot-card epilogue aborts on the
  way out. A caller that captures `stdout` and reports its tail on failure has
  nothing to report.

- Batch mode only. `ft_cktcoms()` has exactly two callers, `src/main.c:1573`
  and `:1581`, both inside the `if (ft_batchmode)` block; `grep -rn ft_cktcoms
  src/` finds no third. `--batch`, `-b < deck`, and a plain `ngspice deck.cir`
  with no tty and no `-i` are the same path — `main.c:1175`'s
  `(!iflag && !istty)` sets `ft_batchmode` at `:1177` — and all three
  abort. A tty session, `-p`, and `-r` do not reach it, and `libngspice`
  cannot reach it at all.

## 4. Why `.op` is the only analysis this happens to

Two steps, both in the current upstream tree.

**`.op` is the only analysis whose plot can be empty.** `beginPlot()` adds a
data descriptor for the reference vector when it is given one, and otherwise
sets `run->refIndex = -1` (`src/frontend/outitf.c:291`). `DCop()` passes
`NULL` for `refName` (`src/spicelib/analysis/dcop.c:50`) — an operating point
has no sweep to plot against. `DCtran()` passes `timeUid`
(`src/spicelib/analysis/dctran.c:183`), `.ac` passes a frequency, `.dc` a
sweep. So for every analysis but `.op` the plot holds at least its reference
column before a single node name is considered. That is why the same nodeless
deck with `.tran` is a clean `rc=1`, and why its `-r` rawfile carries
`No. Variables: 1` where the `.op` one carries `No. Variables: 0`.

**`beginPlot()`'s own "nothing to save" guard has the test for this state and
declines to fire.** `src/frontend/outitf.c:461`:

```c
        if (numNames &&
            ((run->numData == 1 && run->refIndex != -1) ||
             (run->numData == 0 && run->refIndex == -1)))
        {
            fprintf(cp_err, "Error: no data saved for %s; analysis not run\n",
                    spice_analysis_get_description(analysisPtr->JOBtype));
            return E_NOTFOUND;
        }
```

The second disjunct is this state exactly: `numData == 0` with
`refIndex == -1`. But `CKTnames()` sets `*numNames = ckt->CKTmaxEqNum - 1`
(`src/spicelib/analysis/cktnames.c:23`), which is `0` for a circuit whose only
node is the implicit ground, so the leading `numNames &&` is false. The guard
declines, `plotInit()` runs, the descriptor loop runs zero times, and
`pl->pl_dvecs` stays as `plot_alloc()` left it: `NULL`.

`ft_cktcoms()` then finds that plot with `setcplot("op")` and asserts on it.
The assertion is standing on a property of the user's deck — "an operating
point has at least one node" — rather than on an invariant of the program.
The line immediately after it already tests a member of the same struct for
`NULL` before dereferencing it.

## 5. Before and after, measured

Three deck shapes, none with a netlist:

| deck | bytes | content |
| --- | --- | --- |
| A | 6 | `*` / `.op` |
| B | 24 | `*` / `.op` / `.tran 1n 10n` / `.end` |
| C | 30 | `*` / `.op` / `.control` / `run` / `.endc` / `.end` |

Run as `ngspice --batch -n <deck> > out 2> err`.

**Before** — released `ngspice-46`, stamp `Sun Aug  2 23:29:26 UTC 2026`:

| deck | rc | stdout bytes | stderr bytes | stderr |
| --- | --- | --- | --- | --- |
| A | 134 | 0 | 106 | the assertion at `dotcards.c:225` |
| B | 134 | 324 | 106 | the same, byte-identical |
| C | 134 | 0 | 106 | the same, byte-identical |

**After** — the patch applied, stamp `Sun Aug 16 00:53:25 UTC 2026`:

| deck | rc | stdout bytes | stderr bytes | stderr |
| --- | --- | --- | --- | --- |
| A | 1 | 173 | 115 | the three-line diagnostic below |
| B | 1 | 347 | 115 | the same, byte-identical |
| C | 1 | 293 | 115 | the same, byte-identical |

The diagnostic, verbatim, identical for all three:

```
Error: incomplete or empty netlist
       or no node to report an operating point for;
no operating point printed!
```

Its first line is byte-identical to `main.c:1590`'s, which is what `.tran`,
`.dc` and `.ac` on the identical netlist have always printed. The last clause
is `no operating point printed!` rather than `no simulations run!` because the
operating point did run, and `No. of Data Rows : 1` is on `stdout` immediately
above it.

Two cells in the before table are worth stating precisely rather than
generalising:

- Deck B keeps 324 bytes of `stdout` before the fix, not zero. The transient
  path flushes on its way past, so the banner, `No. of Data Rows : 1` and an
  empty `Initial Transient Solution` header survive; the transient's own
  results do not. Decks A and C lose everything. "stdout is empty" is exact
  for the reproducer and for the `.control` shape, and partial for B.
- The 106-byte `stderr` and the `dotcards.c:225` line are byte-identical
  across all three shapes, so these are one defect reached three ways and not
  three defects.

For contrast, the same nodeless deck without the `.op` card is already handled,
before and after alike: `.tran 1n 10n` on its own gives `rc=1` with
`Error: incomplete or empty netlist / or no ".plot", ".print", or ".fourier"
lines in batch mode; / no simulations run!`. `.dc` and `.ac` give the same.
`.op` was the exception.

## 6. The wider fix at `beginPlot()` was measured and rejected

The obvious alternative is to fix this where the empty plot is made rather
than where it is found: drop the `numNames &&` conjunct at
`src/frontend/outitf.c:461` so the existing `E_NOTFOUND` path fires. That was
built and measured as a scratch patch and rejected, because **it fails a test
you ship**.

`tests/regression/misc/empty-1.cir`, byte for byte:

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

The title is the specification. It runs `op`, `tran` and `ac` on a zero-node
circuit and asserts that all three survive; its committed `.out` carries a
`No. of Data Rows` line for each. Because the deck is upstream, the rejection
argument below reproduces in your own tree. Measured here on released
`ngspice-46`: `rc=0`, 626 bytes on `stdout`, `stderr` empty. With the patched
binary it is unchanged — `rc=0`, `stderr` empty, and `stdout` differs only in
the version string the deck's own `echo` prints.

Under the `outitf.c` patch all three of its analyses stop:

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

Full `make check` with that patch applied is **327 PASS / 1 FAIL** against a
**328 PASS / 0 FAIL** baseline, and plain `make check` aborts the recursion in
`tests/regression/misc` after 53 tests. The empty `.op` plot is not the
defect; it is a state the suite creates deliberately and expects to survive.

A narrower variant was measured too: keep `numNames &&` on the
reference-vector disjunct and remove it only from the `.op` disjunct. That
scores 328 PASS / 0 FAIL — but that is the harness not seeing the change, not
the change being harmless. `empty-1.cir`'s `op` still stops. It passes because
`tests/bin/check.sh`'s `FILTER` contains `Data`, so the vanished
`No. of Data Rows : 1` is stripped from both sides of the `diff`; because
`check.sh` never captures `stderr`, so the three new error lines are
discarded; and because the deck's own `quit 0` sets the status.

Three further rows move under the wider patch that are nobody's bug report: a
nodeless deck with `.print tran v(1)` + `.tran` goes from `rc=0` to `rc=1`, and
so do the `.print ac` and `.four` variants. `numNames == 0` reaches
`beginPlot()` from every analysis, not just `.op`, and for those the plot
already holds its reference column and is not empty. The `-r` route moves
twice over: the `.op` run stops writing a rawfile, and so does the nodeless
`.tran` run, which today writes a valid `No. Variables: 1` file.

Finally the message. `beginPlot()`'s existing text is `Error: no data saved
for %s; analysis not run`, which `tests/resistance/res_array.cir` already
prints for an unrelated cause — an `.ac` whose columns were filtered out — and
which never mentions the netlist. At that site the ambiguity is structural:
the function knows `numNames == 0` but not whether that is an empty netlist or
a one-node circuit whose only column was filtered. `ft_cktcoms()` owns its own
`fprintf` and can name the netlist directly.

The `dotcards.c` guard returns before the old `:226`, so nothing dereferences
`NULL` and the `SIGABRT` is not traded for a `SIGSEGV`: measured `rc=1`, not
139, on all three deck shapes and all three batch routes.

## 7. What the patch does not do

- It does not stop the empty `.op` plot being created. `empty-1.cir` still
  makes one and still passes, measured above.
- It does not change the `-r` route. `ngspice -b -n -r out.raw tiny.cir` is
  `rc=0` writing a `No. Variables: 0` header before and after — 192 bytes on
  released `ngspice-46`, 193 on the patched build, the two identical once the
  `Date:` and `Command:` lines are removed. That file is a valid raw file, not
  a third thing: it loads back through ngspice's own reader
  (`printf 'load out.raw\ndisplay\nquit 0\n' | ngspice -p -n` reports
  `Name: Operating Point`, `There are no vectors currently active.`, exits 0).
- It does not touch `src/frontend/outitf.c`.
- `#include <assert.h>` at `dotcards.c:12` becomes unused; it guarded the only
  assertion in the file. The patch leaves it in place rather than mixing a
  tidy-up into the fix. Removing it is a one-line follow-up if you want it.

## 8. How it was tested

The patch is applied on a downstream branch (`ver_50` of a fork carrying
case-sensitivity work), where the full suite goes from **328 PASS / 0 FAIL** to
**331 PASS / 0 FAIL**, `make` exit status 0 both times, the difference being
exactly three new decks. No existing test moved in either direction. The seven
in-tree decks carrying an `.op` dot card were run before and after from their
own directories: `stderr` byte-identical for all seven, `stdout` identical once
wall-clock timestamps and the resource-usage block are removed, and identical
under `check.sh`'s own `FILTER`. No committed `.out` file changed, including
`tests/filters/lowpass.out`.

**The three decks that assert this are not in the patch, and cannot be.** They
live in `tests/regression/exitstatus/`, driven by `tests/bin/check_status.sh`,
and neither exists upstream — the directory was built downstream because no
existing harness in `tests/` can assert a `--batch` exit status.
`tests/bin/check.sh` runs `--batch` and discards `$?`; `tests/regression/pipe/`
sees a status but drives `ngspice -p`, which never enters batch mode and never
reaches `ft_cktcoms()`. So the patch is offered without its test, and the
material for writing one is section 5.

## Binaries and versions

| role | path | version | Creation Date | sha256 |
| --- | --- | --- | --- | --- |
| before | `/usr/local/bin/ngspice` | `ngspice-46`, as released | `Sun Aug  2 23:29:26 UTC 2026` | `b127abce0b8d1e67…6ab105b` |
| after | `build-ver_50/src/ngspice` | `ngspice-46+` | `Sun Aug 16 00:53:25 UTC 2026` | `1fa06748c09be050…6557686` |

## Applying the patch

Verified against `pre-master-47`, this fork's upstream-tracking branch, whose
`src/frontend/dotcards.c` is blob
`60a3e1ac65131fb28ec48a109ed2334f1e793deb` — the same blob id the patch's
`index` line names. The hunk's region is byte-identical between that branch and
the fork, so the patch is a diff against unmodified upstream code:

```
$ git am 0001-frontend-report-an-op-card-with-no-netlist.patch
Applying: frontend: report an .op card with no netlist instead of aborting

$ patch -p1 < 0001-frontend-report-an-op-card-with-no-netlist.patch
patching file src/frontend/dotcards.c
```

Both were run against a tree holding `pre-master-47`'s `dotcards.c`; both
applied with no offset, no fuzz and no reject, and produced byte-identical
results.

One line-number caveat for a reader following the code: `dotcards.c:225` is the
same line in `pre-master-47` and in the fork, but the `beginPlot()` guard
quoted in section 4 is `outitf.c:461` in `pre-master-47` and `outitf.c:479` in
the fork, which carries unrelated changes earlier in that file. The
`pre-master-47` numbers are the ones used throughout this report.
