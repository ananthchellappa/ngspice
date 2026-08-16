# Issue: An `.op` Dot Card With No Netlist Aborts on an Assertion

## Status

**Fixed 2026-08-15** on branch `ver_50`, in the same day it was filed, at
**`src/frontend/dotcards.c:225`** — the assertion is now a test, a diagnostic
and a return, in the idiom the same function already uses at `:204`. The fix,
the three decks that assert it in `tests/regression/exitstatus/`, and this
rewrite are one commit, summary `fix: report an .op card with no netlist
instead of aborting`; a commit cannot carry its own hash, so the hash is in
`doc/claude/batches/2026-08-15-exit-status-testing/receipts/04-op-no-netlist-fix.md`.
The six-byte reproducer now answers:

```
$ printf '*\n.op\n' > tiny.cir
$ ngspice --batch -n tiny.cir ; echo rc=$?
Error: incomplete or empty netlist
       or no node to report an operating point for;
no operating point printed!
rc=1
```

`beginPlot()` (`src/frontend/outitf.c:479`) was measured and rejected; the
reason is under **Resolution** and the measurement is
`doc/claude/batches/2026-08-15-exit-status-testing/receipts/03-op-guard-site-measurement.md`.
All nine Acceptance Criteria are met. One route within them still rests on a
recorded measurement rather than on a committed deck: criterion 5's `-r` route.
Criterion 1's other two routes (`-b -n < deck` and `-n deck < /dev/null`) were
in that position for the same reason — `tests/bin/check_status.sh` ran exactly
`--batch -n <deck>` and took no per-deck flags — and are **asserted since
2026-08-15**, when the driver learned a per-deck `<base>.invoke` sidecar
(`38717a031`) and two decks were committed to spend it (`op-empty-stdin.cir`,
`op-empty-notty.cir`). The table under **Resolution** says so row by row, and
*The route dimension* below says what those two decks do and do not prove.
Full `make check` from `build-ver_50/` was **331 PASS / 0 FAIL** at the fix,
against a 328 PASS / 0 FAIL baseline at `1c07b937b`, the difference being
exactly the three new decks.

**Filed and fixed by different batches.** The batch that filed it
(`doc/claude/batches/2026-08-15-xschem-open-items/`, item 4) was scoped to the
write-up and took no decision. `doc/claude/batches/2026-08-15-exit-status-testing/`
built the harness criterion 6 says did not exist (item 1), measured both guard
sites (item 3), and landed this (item 4).

**Pre-existing and upstream.** Before the fix the same six-byte deck aborted
identically on `/usr/local/bin/ngspice` (`ngspice-46` as released, build stamp
`Sun Aug 2 23:29:26 UTC 2026`) and on `build-ver_50/src/ngspice`
(`ngspice-46+`, build stamp `Sat Aug 15 18:18:34 UTC 2026`): same exit status,
same message, the same 106 bytes on `stderr` naming the same file and the same
line. Nothing in the case-mode work caused it, touches it, or is needed to
reproduce it, and it is mode independent — `fold`, `preserve`, `distinguish`
and no `-D` at all were `rc=134`, and stock has no `casemode` support to set.
**Everything in the body of this file below is the state before the fix**, and
is left in the present tense as the record of what was measured; the fix and
its own before/after are under **Status** and **Resolution**. Released
`ngspice-46` still aborts, because nothing has been sent upstream.

**Class**, for the taxonomy in `doc/claude/decisions/0001-distinguish.md`: none
of the three. No identifier is compared anywhere on this path.

**How it was found.** The crew writing
`doc/claude/batches/2026-08-15-xschem-followup/receipts/02-durable-answers-in-the-guide.md`
ran `doc/codex/issues/0069`'s Resolution listing verbatim and the process died.
That listing was a `.control` block with an `.op` card above it and **no netlist
lines at all** — which is exactly this deck, arrived at by accident. 0069's
listing has since been corrected (item 3 of this batch, `61f519208`), so the
accident is no longer reproducible from that file; the defect it exposed is
this one. Their note is the starting point for this issue and not its evidence:
every number below was re-measured against both binaries on 2026-08-15.

## Summary

Six bytes, both binaries, three runs of three:

```
$ printf '*\n.op\n' > tiny.cir
$ ngspice --batch -n tiny.cir ; echo rc=$?
ngspice: ../../../src/frontend/dotcards.c:225: ft_cktcoms: Assertion `plot_cur->pl_dvecs != NULL' failed.
rc=134
```

`rc=134` is `128 + SIGABRT`; the shell reports it as `Aborted (core dumped)`.
The assertion line is the whole of `stderr` — 106 bytes, byte-identical from
both binaries, `ngspice-46+` and `ngspice-46` alike. The `.end` card is not
needed and neither is a title with any text in it; a title *line* is, because
without one `.op` becomes the title and the deck is empty, which ngspice
already diagnoses properly (`Warning: Empty netlist!`, then the
`incomplete or empty netlist` error, `rc=1`).

**`stdout` is empty for this deck.** Not truncated — empty. The run gets as far
as printing its banner, `Circuit: *`, the solver line and `No. of Data Rows : 1`,
and `abort()` does not flush `stdio`, so when `stdout` is a file or a pipe every
byte of that is discarded:

```
$ ngspice --batch tiny.cir > so.txt 2> se.txt ; echo rc=$?
rc=134
$ wc -c so.txt se.txt
  0 so.txt
106 se.txt
```

**Scoped 2026-08-15**, because this paragraph read as a property of the defect
rather than of the deck. Empty is exact for this six-byte reproducer and for the
`.op` + a `.control run` shape; it is **false for `.op` + `.tran 1n 10n`**, which
keeps **324 bytes** under the same redirect. The transient path flushes on its
way past, so the banner, `No. of Data Rows : 1` and an empty
`Initial Transient Solution` header survive while the transient's own results do
not. Measured in
`doc/claude/batches/2026-08-15-exit-status-testing/receipts/05-upstream-material.md`.

Under a pty the same output is line-buffered and does appear, which is why the
abort looks different by hand than it does under automation:

```
Circuit: * t
Doing analysis at TEMP = 27.000000 and TNOM = 27.000000
Using SPARSE 1.3 as Direct Linear Solver
No. of Data Rows : 1
ngspice: ../../../src/frontend/dotcards.c:225: ft_cktcoms: Assertion `plot_cur->pl_dvecs != NULL' failed.
```

With `-o <log>` the caller sees nothing on either stream: the assertion text
lands in the log file, after the same output, and `stderr` is empty.

**The analysis is not what fails.** `No. of Data Rows : 1` is printed by the
run itself. The operating point is computed, the plot is made, and then the
dot-card epilogue kills the process on the way out.

**Which analyses.** Only `.op`, and every row below is both binaries:

| deck (title line plus these cards) | rc | what is reported |
| --- | --- | --- |
| `.op` | **134** | the assertion |
| `.op` + `.tran 1n 10n` | **134** | the assertion |
| `.op` + a `.control run` block | **134** | the assertion |
| `.tran 1n 10n` | 1 | `Error: incomplete or empty netlist … no simulations run!` |
| `.dc v1 0 1 .1` | 1 | the same |
| `.ac dec 10 1 1k` | 1 | the same |
| `.print tran v(1)` + `.tran 1n 10n` | 0 | `Error: no such vector 1` |
| `.print ac v(1)` + `.ac dec 10 1 1k` | 0 | the same |
| `.print dc v(1)` + `.dc v1 0 1 .1` | 1 | `Fatal error: DC Transfer Function: … "v1" is not in the circuit` |
| `.four 1k v(1)` + `.tran 1n 10n` | 0 | `Error: no such vector 1` |
| `.tf v(1) v1` | 1 | `Warning: Transfer function source v1 not in circuit` |
| `.options noacct` only | 1 | `Error: incomplete or empty netlist …` |

**That table is the argument.** The same mistake — a deck with no netlist —
already has a diagnostic and a non-zero exit status in ngspice for every
analysis but one. `.op` is the exception, and it is the exception by two
independent steps, both of which are in Root Cause.

**"No netlist" is not literally required; no non-ground node is.** Any deck
whose `.op` run yields zero output columns does it:

| netlist body, with `.op` | rc |
| --- | --- |
| nothing | 134 |
| `r1 0 0 1k` | 134 |
| `.model m1 nmos` | 134 |
| `v1 1 0 1` / `r1 1 0 1k` | 0 |

**Which routes reach it, and which do not.** `ft_cktcoms()` has exactly two
callers, `src/main.c:1573` and `:1581`, both inside the `if (ft_batchmode)`
block. `grep -rn ft_cktcoms src/` finds no third. So:

| route | rc | why |
| --- | --- | --- |
| `ngspice --batch -n tiny.cir` | **134** | `ft_savedotargs()` true, `main.c:1581` |
| `ngspice -b -n < tiny.cir` | **134** | same; the deck on `stdin` is the same path |
| `ngspice -n tiny.cir < /dev/null` | **134** | no tty and no `-i` is batch mode (`main.c:1175`) |
| `ngspice -b -n -r out.raw tiny.cir` | 0 | streamed: `fileInit()`, so no in-memory plot to find |
| `ngspice -n tiny.cir` from a tty | 0 | not batch mode; loads the circuit and prompts |
| `printf 'source tiny.cir\nrun\n' \| ngspice -p -n` | 0 | the epilogue is batch-only |
| `printf 'op\n' \| ngspice -p -n` | 0 | `Error: there aren't any circuits loaded.` |
| a `.control` block whose only card is `op`, no `.op` card | 0 | analysis runs; `Note: Simulation executed from .control section` |

Two rows there are worth reading twice.

- **The third row is the trap.** `ngspice deck.cir` with `stdin` redirected is
  batch mode whether or not anyone typed `-b`, because `main.c:1175` sets
  `ft_batchmode` when there is no tty and no `-i`. That is every invocation
  from a `Makefile`, a CI job, a `subprocess.run()` or a schematic tool.
- **The last row corrects the shape this was first described in.** A
  `.control` block containing `op` with no netlist above it does *not* abort —
  it runs the operating point and exits 0. The card that aborts is the **dot
  card** `.op`. A deck carrying both, which is the shape `0069`'s listing had,
  aborts because of the dot card.

**`-r` does not reach it, and that is a coincidence of a different defect's
shape.** Under `-r`, `OUTpBeginPlot()` calls `fileInit()` *instead of*
`plotInit()` (`src/frontend/outitf.c:494`-`:497`), so nothing is added to
`plot_list` and `setcplot("op")` answers NULL. What that writer puts on disk
says why this deck is special:

```
$ ngspice -b -n -r r1.raw tiny.cir ; cat r1.raw
Title: *
Plotname: Operating Point
Flags: real
No. Variables: 0
No. Points: 1
Variables:
Binary:
```

`No. Variables: 0`. The same `-r` run of a `.tran` deck with the same empty
netlist writes `No. Variables: 1` — the `time` column.

## Impact

**This is a mistake-shaped input, and the severity should be read that way.**
Nobody's correct deck hits this. No data is lost that would otherwise have been
written, no wrong answer is produced, nothing is silently corrupted, and the
analysis has already finished when the abort fires. `make check` does not touch
the path (see below), and the shared library cannot reach it at all:
`ft_cktcoms()` is called only from `main.c`, so a `--with-ngshared` host never
executes it. That is a grep and not a measurement, but it is a complete one —
two call sites, both in the batch block.

What it costs is this:

- **A generated deck is one of the ways in, and it is the likeliest one.** A
  tool that emits an analysis dot card and a `.control` block and gets the
  netlist emission wrong — an empty subcircuit expansion, a schematic with
  nothing on the sheet, a template rendered before its body — produces exactly
  this file. That is not hypothetical: it is how this was found. The crew that
  hit it was running a listing from `doc/codex/issues/0069` that had lost its
  netlist lines in an earlier edit, which is the same accident a code generator
  makes.
- **`rc=134` is the signature of a memory bug, and here it is not one.** A
  caller — a person triaging, or a harness classifying failures — cannot tell
  this from `doc/codex/issues/0067`'s double free, which is also `rc=134` with
  a libc message on `stderr` and nothing on `stdout`. In this repository alone
  three issues filed in the last week report `rc=134` or `rc=139`, and two of
  them are genuine memory defects. The cost of this one is that it spends
  somebody's triage budget looking like the other two.
- **The output that would have explained it is destroyed.** Zero bytes of
  `stdout` survive under a redirect for the reproducer, measured above; the
  `.op` + `.tran` shape keeps only the 324 bytes the transient path had already
  flushed, which stop before the analysis that mattered. A harness that captures
  `stdout` and reports the tail of it on failure has nothing to report, or a
  fragment that stops short of the failure. This
  is the same class of unhelpfulness that `doc/codex/issues/0069` documents
  from the other end — there the status is silent about a real failure, here
  the status is loud about a mistake and the output is silent about which.
- **Batch mode is the default for automation**, per the route table: no tty and
  no `-i` is enough. So the population that hits this is precisely the
  population that runs ngspice from a program.
- **The assertion is live in released builds.** `NDEBUG` appears nowhere in
  `configure.ac`, `compile_linux.sh`, `src/Makefile.am` or `m4/`, and the
  configured `build-ver_50/src/Makefile` has `CFLAGS = -O2 -s -Wall …` with no
  `-DNDEBUG`. So this is not a debug-only guard that a shipped binary compiles
  out; `/usr/local/bin/ngspice` is a release build and it aborts. Worth stating
  because "it's just an assert" is otherwise a reasonable thing to assume.

**What in `tests/` reaches this path today: nothing — but not for the reason
first recorded here.** A scan of `tests/` for a bare `.op` dot card
(`^\s*\.op(?![a-z])`, which excludes `.options`) finds eight decks —
`tests/jfet/jfet_vds-vgs.cir`, `tests/vbic/diffamp.cir`,
`tests/polezero/filt_bridge_t.cir`, `tests/filters/lowpass.cir`,
`tests/mesa/mesa11.cir`, `tests/resistance/res_partition.cir`,
`tests/resistance/res_array.cir`, and the reference file
`tests/filters/lowpass.out` — and every one of them has a netlist.

**Corrected 2026-08-15.** This paragraph used to continue *"No deck in the tree
has an analysis card and an empty netlist, so `make check` exercises
`ft_cktcoms()` only with populated plots"*. The first half of that is **false**,
and the scan above is why: it looks for `.op` **dot cards**, and cannot see a
bare `op` inside a `.control` block. `tests/regression/misc/empty-1.cir` is
upstream, is titled *"check that we can survive emptiness"*, and runs `op`,
`tran` and `ac` on a circuit whose only node is the implicit ground, from a
`.control` block. Measured on `build-ver_50/src/ngspice`: `rc=0`, 627 bytes on
`stdout`, `stderr` empty, and the run does create the empty `.op` plot —

```
$ ngspice --batch -n <the same deck, with 'echo $curplot' and 'display' added>
No. of Data Rows : 1
curplot is op1
There are no vectors currently active.
```

— an `op1` plot, current, with `pl_dvecs == NULL`. So `make check` **does**
manufacture the empty plot this assertion forbids. What it does not do is reach
the assertion, and the reason is one branch up in `main.c`: `empty-1.cir`
carries no analysis dot card, so `ft_savedotargs()` returns 0, the batch
epilogue takes its `.control` arm, and `ft_cktcoms()` is never called.

That correction points the opposite way from the sentence it replaces. The
uncovered thing is not "an empty circuit"— the suite has one — it is
specifically *an empty circuit plus an analysis dot card*, and `empty-1.cir` is
in-tree evidence that the empty plot itself is a state upstream ships, asserts
and expects to survive. It is what decided this issue's guard site: see
**Resolution**.

## Root Cause

Two independent decisions put an empty plot in front of an assertion that
forbids one.

**Step 1 — `.op` is the only analysis whose plot can be empty.** `beginPlot()`
adds one data descriptor for the reference vector when it is given one:

```c
        if (refName) {                                  /* outitf.c:295 */
            addDataDesc(run, refName, refType, -1, initmem);
            ...
        } else {
            run->refIndex = -1;                         /* :303 */
        }
```

`DCop()` passes `NULL` for `refName` (`src/spicelib/analysis/dcop.c:50`) —
an operating point has no sweep to plot against. `DCtran()` passes `timeUid`
(`src/spicelib/analysis/dctran.c:183`), `.ac` passes a frequency, `.dc` a
sweep. So for every analysis but `.op` the plot has at least one vector before
a single node name is considered, and the empty-netlist deck simply produces a
plot with only its reference column — which is the `No. Variables: 1` in the
`-r` file above, and the reason `.tran` on the identical netlist is a clean
`rc=1` from `main.c:1590` instead of an abort.

**Step 2 — `beginPlot()`'s own guard for "nothing to save" excludes the case
where there was nothing to save from.** Immediately before it commits to a
plot:

```c
        if (numNames &&                                 /* outitf.c:479 */
            ((run->numData == 1 && run->refIndex != -1) ||
             (run->numData == 0 && run->refIndex == -1)))
        {
            fprintf(cp_err, "Error: no data saved for %s; analysis not run\n",
                    spice_analysis_get_description(analysisPtr->JOBtype));
            return E_NOTFOUND;                          /* :485 */
        }
```

The second disjunct is this state exactly: `numData == 0` with `refIndex == -1`
is an `.op` run that collected no columns. But `CKTnames()` returns
`numNames == 0` for a circuit with no non-ground nodes, so the leading
`numNames &&` is false and the guard declines to fire. **This is the caller
that was supposed to have stopped it.** The function that knows the phrase "no
data saved for Operating Point" has the test for this and gates it behind a
count that is zero precisely when the condition is worst.

`plotInit()` then runs unguarded (`outitf.c:497`), allocates the plot, names it
`op<n>`, links it into `plot_list` with `plot_new()` and makes it current, and
loops:

```c
    for (i = 0; i < run->numData; i++) {                /* outitf.c:1228 */
        ...
        vec_new(v);
    }
```

Zero iterations. `pl->pl_dvecs` stays as `plot_alloc()` left it: NULL.

**These `outitf.c` line numbers are this branch's.** That file carries unrelated
branch changes earlier on, so a reader who follows them into an upstream tree
lands in the wrong place: the `numNames &&` guard is **`outitf.c:461`** on
`pre-master-47` where this file says `:479`, and `run->refIndex = -1` above is
`:291` there rather than `:303`. `dotcards.c` needs no such translation —
`assert(plot_cur->pl_dvecs != NULL)` is line **225 in both trees**, which is why
the fix site's number is the same one an upstream maintainer sees.

**Step 3 — the epilogue asserts the invariant instead of testing it.**
`main.c:1577` asks `ft_savedotargs()`, which returns 1 because the deck has an
`.op` card (`dotcards.c:154`-`:156`, `com_save2(&all, "OP")`), runs the
analyses, and calls `ft_cktcoms(FALSE)` at `:1581`. That function opens with
the house guard —

```c
    if (!ft_curckt) {                                   /* dotcards.c:204 */
        return 1;
    }
```

— which passes, because there *is* a circuit; it is merely a circuit with no
nodes. Then:

```c
    plot_cur = setcplot("op");                          /* dotcards.c:223 */
    if (plot_cur != NULL) {
        assert(plot_cur->pl_dvecs != NULL);             /* :225 */
        if (plot_cur->pl_dvecs->v_realdata != NULL) {
```

`setcplot()` (`dotcards.c:38`) matches on `ciprefix("op", pl->pl_typename)` and
finds the plot from step 2. The assertion is standing on a property of the deck
the user wrote — "an operating point has at least one node" — and stating it as
an internal invariant. The very next line already tests a member of the same
struct for NULL before dereferencing it, which is the shape the line above it
should have had.

**Whether an abort is the right answer here: no, and ngspice already says so
three different ways.**

1. *The same mistake with a different analysis is a diagnostic.* `.tran`,
   `.dc` and `.ac` on the byte-identical netlist print
   `Error: incomplete or empty netlist / … / no simulations run!` and exit 1
   (`main.c:1589`-`:1593`). There is no argument that the `.op` case deserves
   worse; it is the same user error one branch away, and the branch that
   handles it is in the same function that calls the one that aborts.
2. *The established way to refuse a command that needs a circuit is a message
   and a return.* Four sites print `Error: there aren't any circuits loaded.`
   and return — `com_run()` (`src/frontend/runcoms.c:72`), `dosim()`
   (`:255`), `com_resume()` (`src/frontend/runcoms2.c:78`) and
   `com_wr_ic()` (`src/frontend/com_wr_ic.c:38`) — and about fifteen more use
   `Error: no circuit loaded` variants (`com_option.c:22`, `com_dump.c:18`,
   `com_state.c:19`, `where.c:42`, `device.c:395`, and others). Measured, on
   both binaries:

   ```
   $ printf 'op\nquit 0\n' | ngspice -p -n
   Error: there aren't any circuits loaded.        rc=0
   ```

   None of them aborts. `ft_cktcoms()` itself is written in that idiom at
   `:204` — a guard, a return, and `main.c` turning the return into
   `sp_shutdown(EXIT_BAD)`. The function already distinguishes "I cannot do
   this" from "the program is broken"; the assertion is the one place where it
   forgets.
3. *`assert()` is not how this tree reports bad input.* `src/frontend/` has
   five live assertions. Four are in `logicexp.c` (`:520`, `:580`, `:643`,
   `:644`) and guard internal invariants of a generated-logic walker. This one
   is the only assertion in the whole of `src/frontend/` whose truth value is
   decided by the contents of a user's file.

That said, the *first* question is not where to print — it is whether the empty
`.op` plot should exist at all. **Falsified 2026-08-15, and the sentence that
stood here is corrected.** It used to end *"and that decision belongs at
`outitf.c:479` rather than in the epilogue"*. The empty `.op` plot **should**
exist: `tests/regression/misc/empty-1.cir` is an upstream test that creates one
deliberately and asserts that all three of its analyses survive, and a guard at
`outitf.c:479` fails it. The plot is not the defect; asserting on it in the
epilogue was. See criterion 3 and **Resolution**.

## Acceptance Criteria

1. `printf '*\n.op\n' > tiny.cir; ngspice --batch -n tiny.cir` terminates
   normally with a non-zero exit status and a diagnostic on `stderr`. Not
   `rc=134`, not any signal. The same for `ngspice -b -n < tiny.cir` and for
   `ngspice -n tiny.cir < /dev/null`, which are the same path reached three
   ways, and for `.op` combined with `.tran` and with a `.control run` block.
2. The diagnostic names the deck's mistake in the voice ngspice already uses
   for it. `Error: incomplete or empty netlist … no simulations run!` is what
   `.tran` on the same deck prints today and is the obvious candidate; if a
   different message is chosen, the criterion is that a reader of the message
   knows to look at the netlist, and that it does not say "internal error".
3. **The decision is taken explicitly, and it is where the empty plot is made
   rather than where it is found.** Two places can hold the guard and they are
   not equivalent:
   - `beginPlot()` (`outitf.c:479`): drop or relax the `numNames &&` conjunct
     so the existing `Error: no data saved for %s; analysis not run` fires and
     `E_NOTFOUND` propagates. This stops the empty plot from being created at
     all, which fixes every consumer of `plot_list` at once and not just the
     one that asserts. It is also the wider blast radius: `numNames == 0`
     reaches this function from analyses other than `.op`, so the effect on a
     `.tran` of a nodeless deck has to be measured, not assumed.
   - `ft_cktcoms()` (`dotcards.c:225`): replace the assertion with a test.
     Narrow, obviously safe, and leaves an empty plot in `plot_list` for
     everything else to meet.

   Whichever is chosen, the other must be named and the reason recorded. A
   change that only deletes the assertion and lets `:226` dereference NULL
   trades `SIGABRT` for `SIGSEGV` and fails criterion 1.
4. **Nothing with a netlist changes.** A deck with one non-ground node still
   prints the `.op` table — `Node`/`Voltage`, `Source`/`Current`, the
   `com_showmod()`/`com_show()` block — byte for byte as today, in all three
   case modes. The eight `.op` decks named in Impact are the in-tree witnesses
   and none of their committed `.out` files may move.
5. **The `-r` route is decided rather than inherited.** Today
   `ngspice -b -n -r out.raw tiny.cir` exits 0 and writes a header with
   `No. Variables: 0`, because that path never builds a plot. If the guard goes
   in `beginPlot()` this row changes and the new value is asserted; if it goes
   in `ft_cktcoms()` the row must be shown not to have changed. Either way the
   file this run produces is a valid raw file or the run refuses to write one —
   not a third thing.
6. **A deck asserts it, and the honest statement is that no existing harness
   can.** Measured:
   - `tests/regression/pipe/` sees an exit status but drives `ngspice -p`,
     which never reaches `ft_cktcoms()` — the whole `-p` column of the route
     table is `rc=0`.
   - The `check.sh` directories run `--batch`, which does reach it, but
     `tests/bin/check.sh` ignores ngspice's exit status entirely (`:29`-`:39`:
     it redirects `stdout`, filters, and `diff`s against the `.out`). It never
     sees `stderr`, and this deck's `stdout` is empty both before and after a
     fix, since the banner and `Circuit:` lines are inside the script's
     `FILTER`.
   - A pipe deck could in principle `shell` out to a second ngspice and read
     the status — the idiom of writing a `.cir` with `echo` is already in
     `tests/regression/pipe/unset-rawfile-option-key.cmd` and its neighbours —
     but `$program` is `ft_sim->simulator` (`main.c:933`), the compiled-in
     string `ngspice`, not the path of the running binary. Measured: invoked
     as an absolute path, `echo $program` answers `ngspice`. So a deck written
     that way would test whatever is on `PATH`, which in a build tree is the
     wrong binary.

   So this criterion is a small piece of harness work and should be costed as
   such: either the diagnostic is routed somewhere `check.sh` compares (which
   is a decision about the message, not about the test), or the fixing crew
   adds a driver that can assert an exit status on a `--batch` invocation. Say
   which in the fix.
7. No new `strcmp`-family comparison of a deck-written name without the
   annotation `tests/bin/identity_lint.sh` requires; `tests/lint/identity.baseline`
   unchanged. A fix of the expected shape adds no string comparison at all.
8. `make check` unchanged in all three modes. The baseline at filing is the
   322 PASS / 0 FAIL recorded for `731c01455` in this batch's ledger.
9. **Where it lands is decided.** It reproduces on `ngspice-46` as released, so
   fixing it here fixes it for this branch and for nobody who has already
   installed ngspice. See Resolution.

## Resolution

**Fixed 2026-08-15 at `src/frontend/dotcards.c:225`.** The assertion is a test,
a diagnostic and a return:

```c
    plot_cur = setcplot("op");
    if (plot_cur != NULL) {
        /* An operating point has no reference vector, so its plot holds one
           vector per output column and nothing else.  A circuit with no
           non-ground node yields no columns, and the plot is empty.  That is
           a property of the deck, not an invariant of this program, so it is
           reported and returned the way the ft_curckt guard above does --
           main.c turns the return into the process's exit status. */
        if (plot_cur->pl_dvecs == NULL) {
            fprintf(cp_err,
                    "Error: incomplete or empty netlist\n"
                    "       or no node to report an operating point for;\n"
                    "no operating point printed!\n");
            return 1;
        }
        if (plot_cur->pl_dvecs->v_realdata != NULL) {
```

Twelve added lines, five of them comment, in the idiom the same function uses
at `:204`; `main.c:1581` turns the `1` into `sp_shutdown(EXIT_BAD)`. No header,
no `Makefile.am` under `src/`, no new helper, and no string comparison — the
test is a NULL check on a pointer.

**On the wording.** Criterion 2 names `Error: incomplete or empty netlist … no
simulations run!`, which `.tran` on the identical deck prints from
`main.c:1590`. The first two lines above are that message's shape and its first
line is byte-identical; the last clause is **not** `no simulations run!`,
deliberately. It would be false here: the operating point *did* run, and
`No. of Data Rows : 1` is on `stdout` immediately above the error. What did not
happen is the printing, which is this function's whole job, so the message says
so. Criterion 2's stated requirement is that a reader knows to look at the
netlist and that the text does not say "internal error"; line 1 is the same
sentence ngspice already uses to send a reader to their netlist, and line 2
names the exact shortfall.

### `beginPlot()` (`outitf.c:479`) was measured and rejected

Criterion 3 requires the rejected site to be named with its reason. Both were
built as scratch patches and measured against the full suite; the measurement
is `doc/claude/batches/2026-08-15-exit-status-testing/receipts/03-op-guard-site-measurement.md`.

> Dropping the `numNames &&` conjunct fixes the abort but takes six unrelated
> rows with it, and one of them is an in-tree test.
> `tests/regression/misc/empty-1.cir` — upstream, titled *"check that we can
> survive emptiness"*, a `.control` block running `op`, `tran` and `ac` on a
> circuit whose only node is the implicit ground — fails: all three analyses
> stop at `Error: no data saved for …; analysis not run` and the committed
> `.out` loses its `Initial Transient Solution` block. Full `make check` with
> the patch applied is **327 PASS / 1 FAIL** against a 328 PASS / 0 FAIL
> baseline, and plain `make check` aborts the recursion after 53 tests. The
> deck passes on released `ngspice-46`, so this is a behaviour upstream ships
> and asserts, not an artefact of this branch. This issue's Impact statement
> that no in-tree deck has an analysis card with an empty netlist was true only
> of `.op` **dot cards**; the scan `^\s*\.op(?![a-z])` does not see a
> `.control` block's bare `op`. That statement is now corrected in place.
>
> Three further rows move that are nobody's bug report: a nodeless deck with
> `.print tran v(1)` + `.tran` goes from `rc=0` to `rc=1`, and so do the
> `.print ac` and `.four` variants — `numNames == 0` reaches `beginPlot()` from
> every analysis, not just `.op`, and for those the plot already holds its
> reference column and is not empty at all. And criterion 5's `-r` row moves
> twice over: the `.op` run stops writing a rawfile, and so does the nodeless
> `.tran` run, which was writing a perfectly good `No. Variables: 1` file.
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

In one number: `dotcards.c:225` moves exactly the nine rows of this issue's
three tables that **are** the defect and none that are not, at 328 PASS /
0 FAIL; `outitf.c:479` moves fifteen, six of which nobody reported, at 327 PASS
/ 1 FAIL. The paragraph that used to stand here preferred `beginPlot()` "on the
merits", on the ground that stopping the plot from existing would fix consumers
this issue had not enumerated. The measurement contradicts it: the consumers it
had not enumerated include one that *wants* the empty plot, and it is a test.

### The state of each Acceptance Criterion

Measured on `build-ver_50/src/ngspice`, `ngspice-46+`, build stamp
`Sun Aug 16 00:53:25 UTC 2026`, sha256 `1fa06748c09be050…7686`.

| # | state | measurement |
| --- | --- | --- |
| 1 | **met**; five of its nine rows asserted by a deck, four measured | All three deck shapes (`.op`; `.op`+`.tran 1n 10n`; `.op`+`.control run`) × all three routes (`--batch -n deck`, `-b -n < deck`, `-n deck < /dev/null`) are `rc=1` with the three-line diagnostic on `stderr`. Nine rows, all `rc=134` before, no signal in any of them. Five are asserted by a committed deck: the `--batch -n` column is the three decks named in row 6, and `op-empty-stdin.cir` and `op-empty-notty.cir` are the reproducer's same six bytes under the other two routes, each declaring its route in a `<base>.invoke` sidecar. The sentence that stood here — *"measured rather than asserted, because the driver takes no per-deck invocation"* — was true when it was written and is **false since `38717a031`**: `tests/bin/check_status.sh` is no longer fixed at `--batch -n <deck>` and reads a per-deck invocation, so nothing about the driver keeps the remaining four rows unasserted. They are the `.tran` and `.control run` shapes under those two routes, and they are left measured by choice: see *The route dimension* below. |
| 2 | **met, with a one-clause departure** | The message names the netlist in its first line, byte-identical to `main.c:1590`'s, and does not say "internal error". `no simulations run!` was replaced by `no operating point printed!` because the analysis did run; see *On the wording*. |
| 3 | **met** | `dotcards.c:225` chosen, `outitf.c:479` named and rejected above with the number that rejected it. Nothing dereferences NULL: the guard returns before `:226`. |
| 4 | **met** | The eight in-tree witnesses named in Impact — seven decks and one reference file — were checked. The seven decks were run before and after from their own directories: `stderr` is byte-identical for all seven; `stdout` is identical once wall-clock timestamps and the resource-usage block are removed, and identical under `check.sh`'s own `FILTER`. `tests/filters/lowpass.out` keeps sha256 `fec8b85a…`. No tracked `.out` in the tree is modified. `tests/filters/lowpass.cir` was additionally run under `-D casemode=fold`, `preserve` and `distinguish`: identical in all three. |
| 5 | **met, by not moving**; measured, not asserted by a deck | The driver's command line is `--batch -n <deck>` with no `-r` and no per-deck flag file, so this row is a recorded measurement. `ngspice -b -n -r out.raw tiny.cir` is `rc=0` writing 193 bytes before and after, and the two files are identical apart from their `Date:` and `Command:` lines. It is a valid raw file, measured by reading it back with ngspice's own reader: `printf 'load out.raw\ndisplay\nquit 0\n' \| ngspice -p -n` loads it, reports `Name: Operating Point`, and exits 0. The nodeless `.tran` `-r` file (681 B, `No. Variables: 1`) and a real one (1658 B) are also unmoved. Not a third thing. |
| 6 | **met** | Three decks in `tests/regression/exitstatus/`: `op-empty-netlist.cir` (this issue's reproducer, six bytes), `op-empty-tran.cir`, `op-empty-control.cir`, each with a `.status` of `1` and an `.err` holding the diagnostic. The criterion's own answer to "no existing harness can" was taken: `tests/bin/check_status.sh` and this directory were built first, as item 1 of `doc/claude/batches/2026-08-15-exit-status-testing/`. Its signal check makes a return to `SIGABRT` a named failure and not just a wrong number. |
| 7 | **met** | The guard adds a NULL pointer test and an `fprintf`, and no comparison of any kind. `tests/lint/identity.baseline` is unchanged (sha256 `3dbd620e…`); both lint specs report `baseline matches`, 264 and 11 comparisons, the same counts as before. |
| 8 | **met** | Full `make check` from `build-ver_50/`: **328 PASS / 0 FAIL** before, **331 PASS / 0 FAIL** after, `make` exit status 0 both times, the difference being exactly the three new decks. The criterion's 322 is the baseline at filing; the 328 is the same suite after this batch added `tests/regression/exitstatus/`. Case modes are covered inside that suite — `tests/regression/casedist/` runs under `-D casemode=distinguish` and `tests/regression/case/` under `preserve` — plus the three-mode run of a real `.op` deck in row 4. |
| 9 | **met** | Decided: fixed here, on `ver_50`, and the upstream material owed alongside it is a named, separate item, written later the same day and **not sent**. See *Where it lands*. |

### The route dimension

Added 2026-08-15, when criterion 1's other two routes stopped being measurements
and became `op-empty-stdin.cir` and `op-empty-notty.cir` in
`tests/regression/exitstatus/`.

**The 3 shapes × 3 routes cross product is deliberately not built.** The route
dimension is the one nothing asserted; the shape dimension is already covered
three ways under the default `batch` route by the decks in row 6. All nine cells
reach the same `ft_cktcoms()` call at `main.c:1581`, and nine committed decks for
one code path reached three ways is not worth the `EXTRA_DIST`. The four
remaining cells stay measurements, and the measurement is the nine-row run
recorded in row 1.

**What the two route decks prove, and what they do not.** They prove the
property criterion 1 asks for — terminates normally, non-zero, with the
diagnostic, no signal — holds when the driver is told to run the reproducer
those two ways. That was demonstrated by removing the guard at `dotcards.c:225`
in a working tree and rebuilding: both decks then failed the driver's `signal`
check at `rc=134`, named `SIGABRT`, with the invocations
`ngspice -b -n < <deck>` and `ngspice -n <deck> < /dev/null` printed beside the
failures.

They do **not** witness their own route from inside, and this was measured
rather than assumed. Each deck is the six-byte reproducer verbatim and returns
before any control language runs, so it cannot read `$?batchmode` or
`$inputdir`, which are the only two cp variables that separate the three routes.
And the diagnostic carries no route information: run three ways, the reproducer
produces `stderr` **and** `stdout` byte-identical across all three, sha256 equal,
115 and 173 bytes. So a corruption test was run on the sidecars themselves —
deleting the `route` line, changing it to the other route, and deleting the whole
`.invoke` file — and **the verdict did not move in any of the five cases**: the
deck passes under the default `batch` route too, because for this deck every
route produces the same answer, which is precisely what the criterion claims.

That is the honest worth of these two decks: they are assertions about the
**simulator's** behaviour on three command lines, not about the harness's
fidelity in producing them. What holds the routes honest is elsewhere and is
asserted — `tests/regression/exitstatus/selftest-invoke.cir` reads `$?batchmode`
from a `.control` block and fails if the sidecar is dropped, and the driver
prints its exact command line beside every failure.

### Where it lands

**Fixed here; upstream material is a separate, later item.** Option 2 of the
three the filing offered: the patch is applied on this branch, and the material
an upstream report would need — the minimal diff against upstream, the six-byte
reproducer, the measurement that it reproduces on released `ngspice-46`, and
the before/after — is item 5 of
`doc/claude/batches/2026-08-15-exit-status-testing/PLAN.md`. It is not in this
commit. It was written later the same day and is
`doc/claude/batches/2026-08-15-exit-status-testing/upstream-0072/`: a patch
against `pre-master-47`, a report, and a README. **Prepared and not sent** —
whether to send it is the owner's call.

The ownership argument is unchanged, and it is the reason upstream material is
owed at all. Every measurement in this file was taken on
`/usr/local/bin/ngspice`, `ngspice-46` as released, as well as here, and the
two were identical to the byte. The defect is in code this branch has not
otherwise touched — `src/frontend/dotcards.c` and `src/frontend/outitf.c` were
both unmodified from upstream before this fix — and it has nothing to do with
case modes, with the raw header, or with anything else this branch exists for.
It is not a `--with-ngshared` bug and cannot be reached from `libngspice`. So
the argument that closed `doc/codex/issues/0067` here rather than upstream —
*that branch work made a pre-existing arm reachable from files this build
writes* — does not apply. What decided it instead is the population in Impact:
batch mode is the default for any invocation without a tty, this branch's
client generates decks, and a generated deck that loses its netlist lines is
exactly this file.

**Nothing has been sent upstream.** No submission from this repository has been
sent — not the material prepared for `0067`, and not the material now prepared
for this issue.

## Related

- `doc/codex/issues/0069` — batch exit status reports the last thing `main()`
  did. Its Resolution listing, before item 3 of this batch corrected it, was a
  deck with no netlist lines, and running it verbatim is how this was found.
  The two issues are the same complaint from opposite ends: 0069 is a real
  failure reported as `rc=0`, this is a user mistake reported as a signal, and
  in both cases the exit status tells the caller something other than what
  happened. 0069's answer — `$sim_status`, chosen by the deck — does not reach
  this one, because the process is dead before any control language runs. 0069
  is **not fixed, by decision**, and its last open criterion — a deck asserting
  the `$sim_status` guard — was closed on 2026-08-15 by four decks in
  `tests/regression/exitstatus/`, the same directory this issue's three live in.
  Neither could be asserted before `tests/bin/check_status.sh` existed: it is
  the first driver in the tree that can see a `--batch` run's exit status, and
  it was built as item 1 of the batch that fixed this.
- `doc/codex/issues/0067` — `cp_remvar()` frees a node it chose not to unlink.
  Also `rc=134` from a released binary, also found from the client's side, and
  the reason `rc=134` is an expensive thing for ngspice to spend on a bad deck.
  Unlike this, 0067 is a memory defect and was fixed here because branch work
  had widened its reach.
- `doc/codex/issues/0071` — the `-r` writer. Its Root Cause explains why
  `OUTpBeginPlot()` calls `fileInit()` instead of `plotInit()` under `-r`,
  which is why the `-r` row of this issue's route table is `rc=0` and why
  criterion 5 has to be decided rather than assumed.
- `doc/codex/issues/0059` — `write` emitting the constants plot when no
  analysis ran. Another consequence of a deck with nothing in it reaching an
  epilogue; independent of this, and neither is fixed by fixing the other.
