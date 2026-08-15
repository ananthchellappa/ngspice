# Issue: An `.op` Dot Card With No Netlist Aborts on an Assertion

## Status

**Open, filed 2026-08-15** on branch `ver_50`. **Filing only — no fix, and no
code was written for it.** The batch that filed it
(`doc/claude/batches/2026-08-15-xschem-open-items/`, item 4) was scoped to the
write-up, and the decision under **Resolution** — where the guard goes, and
whether it goes here or upstream — is not taken.

**Pre-existing and upstream.** The same six-byte deck aborts identically on
`/usr/local/bin/ngspice` (`ngspice-46` as released, build stamp
`Sun Aug 2 23:29:26 UTC 2026`) and on `build-ver_50/src/ngspice`
(`ngspice-46+`, build stamp `Sat Aug 15 18:18:34 UTC 2026`): same exit status,
same message, the same 106 bytes on `stderr` naming the same file and the same
line. Nothing in the case-mode work caused it, touches it, or is needed to
reproduce it, and it is mode independent — `fold`, `preserve`, `distinguish`
and no `-D` at all are `rc=134`, and stock has no `casemode` support to set.

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

**`stdout` is empty.** Not truncated — empty. The run gets as far as printing
its banner, `Circuit: *`, the solver line and `No. of Data Rows : 1`, and
`abort()` does not flush `stdio`, so when `stdout` is a file or a pipe every
byte of that is discarded:

```
$ ngspice --batch tiny.cir > so.txt 2> se.txt ; echo rc=$?
rc=134
$ wc -c so.txt se.txt
  0 so.txt
106 se.txt
```

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
  `stdout` survive under a redirect, measured above. A harness that captures
  `stdout` and reports the tail of it on failure has nothing to report. This
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

**What in `tests/` reaches this path today: nothing.** A scan of `tests/` for a
bare `.op` dot card (`^\s*\.op(?![a-z])`, which excludes `.options`) finds
eight decks — `tests/jfet/jfet_vds-vgs.cir`, `tests/vbic/diffamp.cir`,
`tests/polezero/filt_bridge_t.cir`, `tests/filters/lowpass.cir`,
`tests/mesa/mesa11.cir`, `tests/resistance/res_partition.cir`,
`tests/resistance/res_array.cir`, and the reference file
`tests/filters/lowpass.out` — and every one of them has a netlist. No deck in
the tree has an analysis card and an empty netlist, so `make check` exercises
`ft_cktcoms()` only with populated plots and would not notice this changing in
either direction.

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
`.op` plot should exist at all, and that decision belongs at `outitf.c:479`
rather than in the epilogue. See criterion 3.

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

**Not started.** Filed for the record; the shape below is a description of the
work, not a decision that it will be done.

**The shape a fix would take.** One conditional, in one of the two places
criterion 3 names, plus its diagnostic. `beginPlot()` is the better place on
the merits — it is where the empty plot is manufactured, its message for this
exact condition is already written and already reachable for a *different*
input (`numNames > 0` with everything filtered out by `.save`), and stopping
the plot from existing fixes consumers this issue has not enumerated. It is
also the place that needs the most measurement, because `numNames == 0` is not
unique to `.op` and the effect on the other analyses' plots has to be taken
rather than reasoned about. `ft_cktcoms()` is the smaller change and is the
right one if the wider measurement comes back awkward. Nothing else moves: no
header, no `Makefile.am`, no new helper.

**It belongs upstream, and that is the main thing to decide.** Every
measurement in this file was taken on `/usr/local/bin/ngspice`, `ngspice-46` as
released, as well as here, and the two are identical to the byte. The defect is
in code this branch has not touched — `src/frontend/dotcards.c:225` and
`src/frontend/outitf.c:479` are both unmodified from upstream — and it has
nothing to do with case modes, with the raw header, or with anything else this
branch exists for. It is not a `--with-ngshared` bug and cannot be reached from
`libngspice`. So the argument that closed `doc/codex/issues/0067` here rather
than upstream — *that branch work made a pre-existing arm reachable from files
this build writes* — does not apply: this branch makes nothing about this
easier or harder to reach.

Which leaves the owner three options, in order of increasing cost:

1. **Report it upstream and carry no patch.** Correct on ownership. Costs
   nothing here and fixes nothing here.
2. **Report it upstream with a patch, and apply the patch here too**, the way
   `doc/claude/upstream/` is already prepared to do for `0067`. Two lines of
   code and criterion 6's harness question. Reasonable if this branch's client
   generates decks, which it does.
3. **Fix here only.** Not recommended: it is a divergence from upstream in a
   file this branch otherwise does not touch, bought for an input no correct
   deck produces.

**Nothing has been sent upstream.** The submission in `doc/claude/upstream/` is
prepared and unsent, and this issue is not in it.

## Related

- `doc/codex/issues/0069` — batch exit status reports the last thing `main()`
  did. Its Resolution listing, before item 3 of this batch corrected it, was a
  deck with no netlist lines, and running it verbatim is how this was found.
  The two issues are the same complaint from opposite ends: 0069 is a real
  failure reported as `rc=0`, this is a user mistake reported as a signal, and
  in both cases the exit status tells the caller something other than what
  happened. 0069's answer — `$sim_status`, chosen by the deck — does not reach
  this one, because the process is dead before any control language runs.
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
