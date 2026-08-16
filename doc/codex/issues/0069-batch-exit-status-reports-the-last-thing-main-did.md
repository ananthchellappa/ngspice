# Issue: Batch Exit Status Reports the Last Thing `main()` Did, Not Whether the Analysis Ran

## Status

Open, filed 2026-08-14 on branch `ver_50`.

**Decided at filing: the exit status is not going to change.** The repo owner's
answer is `$sim_status`, which is readable from the control language today, on
this build and on stock `ngspice-46`, and which turns the failure into an exit
status the deck itself chooses. The reasoning is under Resolution; the
acceptance criteria are written against that answer rather than against a
change to `sp_shutdown()`.

Pre-existing, upstream and mode independent. Reproduces on
`/usr/local/bin/ngspice` (`ngspice-46`, no `casemode` support) with no `-D` at
all, from a `.save` of a node no netlist has. The measurements were first taken
2026-08-14 against `build-ver_50/src/ngspice` (`ngspice-46+`, build stamp
`Fri Aug 14 20:52:09 UTC 2026`), with stock as the baseline, and every one of
them was re-taken 2026-08-15 against the same path rebuilt (build stamp
`Sat Aug 15 18:18:34 UTC 2026`).

**Corrected 2026-08-15.** The Resolution's guard listing had no netlist lines in
it — as printed it was not a runnable deck, it was a `.control` block with
nothing to simulate — and it named three `.cir` files that exist nowhere in the
tree, which is what every number in its table was attributed to. Every deck in
this file is now printed complete and was run as printed, every number was
re-measured, and no file is named that does not exist. Two byte counts moved,
for a reason recorded under *Resolution*; no rc and no table row moved. The
decision is untouched: the exit status does not change and `$sim_status` is the
answer.

Reported by the client integration (xschem generating decks, reading the raw
file back) as **R1** of
`doc/claude/feedback/reply_from_xschem_session/REPLY.md`, whose decks are in
`repro2/` there and are the ones quoted below. It matters to them because
`doc/claude/feedback/ngspice_upstream/RESPONSE.md` advised clients to keep `rc`
as a defence, and for the deck shape a schematic tool generates that advice is
wrong. RESPONSE.md is corrected at the point of use in its round-2 rewrite.

**Not `doc/codex/issues/0059`.** 0059 is about the *artefact*: `write` emitting
the constants plot when no analysis ran. This is about the *signal*: whether
the process's exit status says so. They are independent — 0059's rc=0 rows are
one shape of this defect and this issue's rc=1 rows still leave 0059's file on
disk — and neither is fixed by fixing the other. 0059 documents rc=0 on the
write path; the exit status itself is filed here for the first time.

## Summary

The client reported it as "the same failure exits 0 inside `.control` and 1
outside it". Measured, that is real but mis-attributed: **the variable is
`-r`, not `.control`.** Their two commands differ in two ways, and it is the
flag rather than the block that decides.

The controlled experiment. One deck spelling its net `MidNode` and its `.save`
card `v(midnode)`, so the save misses under `distinguish`; one deck the same
plus a `.control run` / `write` block, which is how a generated deck names its
own rawfile. Both decks are committed, at
`doc/claude/feedback/reply_from_xschem_session/repro2/plain_fail.cir` and
`.../repro2/ctl_fail.cir`. Four runs, re-measured 2026-08-15 and unchanged:

```
$ ngspice -b -n -D casemode=distinguish -r plain_fail.raw plain_fail.cir
rc=1        no raw written                    <- the client's first command
$ ngspice -b -n -D casemode=distinguish       plain_fail.cir
rc=0                                          <- same deck, -r removed
$ ngspice -b -n -D casemode=distinguish       ctl_fail.cir
rc=0        ctl_fail.raw: Plotname: constants <- the client's second command
$ ngspice -b -n -D casemode=distinguish -r x.raw ctl_fail.cir
rc=1        ctl_fail.raw: Plotname: constants <- same deck, -r added
```

The `.control` block is not what changes the answer. A deck whose analysis
lives *only* in a `.control` block — no analysis dot card at all — exits 1:

```spice
* ctl_only.cir -- no analysis dot card; save, op and write inside .control
Vs In 0 DC 3
Rl In MidNode 1k
Rg MidNode 0 3k
.control
save v(midnode)
op
write ctl_only.raw
.endc
.end
```

`ngspice -b -n -D casemode=distinguish ctl_only.cir` exits **1**, with one
analysis, one near-miss warning, and `ctl_only.raw` on disk anyway — 570 bytes
of `Plotname: constants`. Under `fold` and `preserve` the same deck is rc=0 with
a 289-byte `Plotname: Operating Point` file, so the exit status here is tracking
the failure and not the block.

So the client's rule of thumb — "a `.control` deck has no `rc`" — is not the
rule, and the rule it replaces is worse: rc reports whichever of `main()`'s
four batch epilogue arms ran last, and which arm that is depends on `-r` and on
whether the deck carries an analysis dot card. A deck author cannot read it off
the deck.

## Impact

**For the deck shape a schematic tool generates, `rc` is 0 while the analysis
did not run and the rawfile holds the twelve physical constants.** That shape
is: an analysis dot card (`.op`, `.tran`, `.dc`), a `.control` block that
`run`s and `write`s a named rawfile, and no `-r` — because the point of the
control block is to name the file, which is what `-r` would otherwise do.

Every channel a consumer has then agrees the run was fine:

| channel | reports |
| --- | --- |
| exit status | 0 |
| stdout | `binary raw file "ctl_fail.raw"` |
| stderr | the near-miss warning, if the miss was a case near miss; if the name is simply absent, `Error: no data saved …; analysis not run` and `run simulation(s) aborted` and **no line naming the token** (`doc/codex/issues/0057`) |
| the rawfile | well formed, loads without a diagnostic, twelve variables (`doc/codex/issues/0059`) |

Measured on `doc/claude/feedback/reply_from_xschem_session/repro2/absent.cir`
— `.save v(nosuchnode)` in the shape above, which needs no case mode to reach
— and re-measured 2026-08-15:

```
ver_50 -D casemode=fold          rc=0   Plotname: constants  570 bytes  mentions of 'nosuchnode': 0
ver_50 -D casemode=preserve      rc=0   Plotname: constants  570 bytes  mentions of 'nosuchnode': 0
ver_50 -D casemode=distinguish   rc=0   Plotname: constants  570 bytes  mentions of 'nosuchnode': 0
stock ngspice-46, no flag        rc=0   Plotname: constants  569 bytes
```

The count of mentions is over the rawfile, stdout and stderr together: no
channel names the token the deck got wrong.

The rows above are `doc/codex/issues/0059`'s Impact reached through this
issue's rc=0 arm; 0059 records the same deck at rc=1 because it measured the
`-r`-less shape with no analysis dot card. Both are correct and they are the
two arms of this issue.

**A second, quieter consequence.** Because the `-r`-less path re-runs the
deck's analysis dot cards after the control block has ended, a deck with an
analysis dot card *and* a `.control run` runs its analysis **twice**. That is
also why the near-miss warning `doc/codex/issues/0057` emits appears twice for
one mistake in the client's decks — 0057's contract is one line per token per
simulation, and this deck shape is two simulations. The two findings are one
deck shape and are cross-referenced from 0057.

The reverse hazard is worth stating too, because a client that starts trusting
rc=1 will meet it: **rc=1 does not mean nothing was written.** The control-only
deck above exits 1 and leaves a 570-byte constants rawfile on disk, and the `-r`
arm's `ctl_fail.cir` + `-r` run exits 1 having written `ctl_fail.raw` from
inside the control block. Whatever rc says, 0059's content checks are still
owed.

## Root Cause

`src/main.c:1533-1596`, the batch epilogue, is a four-arm chain. `error3` is
initialised to 1 and then overwritten from the control variable:

```c
        int error3 = 1;
        ...
        /* Check if a simulation has run from a .control section */
        cp_getvar("sim_status", CP_NUM, &error3, 0);          /* :1559 */

        if (rflag) {                                          /* :1561 */
            ...
            int error2 = ft_dorun(ft_rawfile);
            if (ft_cktcoms(TRUE) || error2)
                sp_shutdown(EXIT_BAD);
        }
        else if (ft_savedotargs()) {                          /* :1577 */
            int error2 = ft_dorun(NULL);
            if (ft_cktcoms(FALSE) || error2)
                sp_shutdown(EXIT_BAD);
        }
        else if (error3 == 0) {                               /* :1584 */
            fprintf(stdout, "Note: Simulation executed from .control section \n");
            sp_shutdown(EXIT_NORMAL);
        }
        else {                                                /* :1588 */
            fprintf(stderr, "Error: incomplete or empty netlist\n" ...);
            sp_shutdown(EXIT_BAD);
        }
```

Each arm is correct on its own and the four together have no single subject.

- **Arm 1, `-r`.** Dot cards are deliberately ignored (`:1562`'s own comment
  says so) except `.save`, so the save list is the deck's `.save` card alone.
  The miss kills the run, `ft_dorun()` returns non-zero, `EXIT_BAD`. This is
  the arm that gave the client rc=1 and it is the *only* one that reports the
  save miss.
- **Arm 2, an analysis dot card and no `-r`.** `ft_savedotargs()`
  (`src/frontend/dotcards.c:93`) builds a save list of its own from the `.op`
  / `.print` / `.tf` cards, so the failing `.save` is no longer the whole save
  list and `ft_dorun(NULL)` **succeeds**. `EXIT_NORMAL`. If a `.control run`
  already ran and failed, that failure is now two analyses behind the exit
  status. Measured: `ctl_fail.cir` starts two analyses, the first aborts and
  the second prints a complete operating-point table.
- **Arm 3, no dot card, a `.control` run that set `sim_status` to 0.** Reports
  success, correctly.
- **Arm 4, everything else,** including a `.control`-only deck whose run
  failed: `error3` is 1, so `EXIT_BAD`. This is why the control-only deck of
  *Summary* exits 1 and why "`.control` swallows the status" is the wrong
  description. Measured on the same deck with a `.save` that resolves, this arm
  is also the rc=0 one: `Note: Simulation executed from .control section` on
  stdout, `EXIT_NORMAL`.

`sim_status` itself is set in `src/frontend/runcoms.c`: to `err` before the
analysis at `:329`, and to 1 at `:352` (`simulation(s) aborted`) and `:358`
(`simulation not started`). It is the per-analysis outcome and it is right;
nothing between it and `sp_shutdown()` preserves it past arm 1 or arm 2.

## Acceptance Criteria

Written against the owner's decision, which is that the exit status does not
change. Criteria 1 and 2 are the decision; 3 to 5 are what is owed anyway.

1. **`sp_shutdown()`'s argument is not touched, and no arm of the chain above
   is re-ordered or given a new condition.** A deck that deliberately continues
   past a failed `run` — retry with different options, sweep until one
   converges, run a known-bad case and report on it — exits 0 today and must
   keep doing so. That is the whole of the argument against the client's ask
   and it is not a small class of deck: `run` inside `.control` is the
   scripting interface, and turning its failure into a process failure would
   be a behaviour change to every script that uses it, with no opt-out and no
   way for the deck to say "I meant that".
2. **`$sim_status` is documented as the signal, at the three places a client
   reads.** `doc/claude/casemode-distinguish-guide.md` §9,
   `doc/claude/feedback/ngspice_upstream/RESPONSE.md`, and this issue. The
   documentation has to say all three of: it is per analysis, it is
   last-writer-wins, and it does not exist before the first analysis of the
   session.
3. **A deck asserts the guard shape.** `run`, then `if $sim_status ne 0` /
   `quit 1`, in the client's exact deck shape (analysis dot card plus
   `.control run`), asserting rc=1 **and** that no rawfile was written. The
   negative control is the same deck with a resolvable `.save`, asserting rc=0
   and a `Plotname: Operating Point` raw. `tests/regression/pipe/` is the
   directory that can see an exit status;
   `tests/regression/casedist/save-undef-report.cir` is the nearest deck-side
   precedent for reading a capture back.
4. **The two-simulation shape is documented where it bites.** A deck with an
   analysis dot card *and* a `.control run` runs the analysis twice. That is
   the root of R4's doubled warning and of arm 2 above, and it is not written
   down anywhere in `doc/` today. `doc/codex/issues/0057` carries the
   diagnostic half; the shape itself belongs beside this chain.
5. **No claim that rc=1 implies no output.** Both rc=1 arms can leave a
   constants rawfile on disk. Any documentation of this issue that lets a
   reader infer otherwise is wrong; `doc/codex/issues/0059` is still owed.

### Where the criteria stand, checked 2026-08-15

The criteria themselves still say what is owed and none of them is withdrawn.
All five are met. The third was the last one outstanding and was closed later
the same day, by the decks named under it.

1. **Met.** `src/main.c` has no commit on `ver_50` since this issue was filed
   and the chain is where *Root Cause* quotes it, line for line: `error3 = 1` at
   `:1534`, the `cp_getvar` at `:1559`, `if (rflag)` at `:1561`,
   `else if (ft_savedotargs())` at `:1577`, `else if (error3 == 0)` at `:1584`,
   the final `else` at `:1588`. `runcoms.c`'s three writes are still at
   `:329`, `:352` and `:358`.
2. **Met, at all three places.**
   `doc/claude/casemode-distinguish-guide.md` §9 carries the subsection *Guard
   the run with `$sim_status`, not with the exit status* with the complete deck
   and all three properties; `doc/claude/feedback/ngspice_upstream/RESPONSE.md`
   carries them under *The answer: `$sim_status`, and it needs no ngspice
   change*; this issue's *Resolution* is the third. All three say per analysis,
   last writer wins, and absent before the first analysis.
3. **Met, 2026-08-15,** on `ver_50` in *test: assert the `$sim_status` guard
   shape*, the child of `baccfa570`. Four decks in
   `tests/regression/exitstatus/`, driven by `tests/bin/check_status.sh` — the
   directory and driver `baccfa570` itself added, precisely because no existing
   harness could see a batch run's exit status.
   `grep -rl sim_status tests/` returned nothing when this
   subsection was first written; it now returns six files (`| sort`):

   ```
   tests/regression/exitstatus/Makefile.am
   tests/regression/exitstatus/sim-status-guard-ok.cir
   tests/regression/exitstatus/sim-status-guard.cir
   tests/regression/exitstatus/sim-status-properties.cir
   tests/regression/exitstatus/sim-status-unset.cir
   tests/regression/exitstatus/sim-status-unset.err
   ```

   - `sim-status-guard.cir` is the criterion's deck: the client's shape — an
     analysis dot card *and* a `.control run`, no `-r` — with
     `.save v(nosuchnode)`, which misses in every case mode so the deck needs
     no `-D` flag. `run`, the four guard lines, then a `write` that must never
     be reached. `sim-status-guard.status` asserts rc=1 and
     `sim-status-guard.files` asserts `absent sim-status-guard.raw`, which is
     the criterion's *both*.
   - `sim-status-guard-ok.cir` is the negative control: the same deck with
     `.save v(MidNode)`, asserting rc=0, the rawfile present, and
     `Plotname: Operating Point` in it — the plotname rather than mere
     existence, because the artefact the guard prevents is also a well-formed
     file.
   - `sim-status-properties.cir` and `sim-status-unset.cir` assert the three
     properties of the *Resolution* in the fail-fast idiom of
     `tests/regression/pipe/*.cmd`. Property 2 is split off because half its
     evidence is `Error: sim_status: no such variable.` on stderr, which no
     deck can read; `sim-status-unset.err` carries it.

   The criterion named `tests/regression/pipe/` as the directory that can see
   an exit status. That was true when it was written and is now superseded:
   `pipe/` drives `ngspice -p`, which never enters batch mode and so cannot
   reach the four-arm epilogue this issue is about. `exitstatus/` runs
   `--batch -n` and asserts the status, so the guard is asserted against the
   path it is a guard for.
4. **Met.** The two-simulation shape is in the guide's §9 as *Do not carry both
   an analysis dot card and a `.control run`*, and in this issue's *Impact*.
   Re-measured 2026-08-15 on `repro2/ctl_fail.cir` under `distinguish`: two
   `Doing analysis` lines and two near-miss warnings for one mistake, rc=0.
5. **Met.** Both this issue and the guide state the reverse hazard, and the
   number behind it was re-measured: the control-only deck exits 1 and leaves a
   570-byte constants file. `doc/codex/issues/0059` is still owed.

## Resolution

**Not fixed, by decision, and the replacement is measured.**

`$sim_status` is the per-analysis outcome, it is readable from the control
language before the `write` that would produce the bogus artefact, and `quit
<n>` propagates a status of the deck's choosing. So a client gets a reliable
signal with no ngspice change and no risk to the scripts of criterion 1.

**Re-measured 2026-08-15** against `build-ver_50/src/ngspice` (`ngspice-46+`,
build stamp `Sat Aug 15 18:18:34 UTC 2026`), with `/usr/local/bin/ngspice`
(`ngspice-46`) as the baseline. **Every deck below is printed complete and runs
as printed.** The 2026-08-14 listing was neither: it had no netlist lines in it
at all, so a reader who copied it got the `dotcards.c` assertion abort rather
than a demonstration, and the three file names it carried
(`guard_both.cir`, `guard_absent.cir`, `ctl_noop.cir`) named nothing in the
tree. Nothing here is committed as a `.cir` either — copy the block. The same
guard, in the same shape and with the same measurements, is
`doc/claude/casemode-distinguish-guide.md` §9, which is where a client reads it.

Measured in the client's own deck shape — analysis dot card, `.control run`,
no `-r`, which is the shape that gives rc=0:

```spice
* guard.cir -- analysis dot card, .control run, no -r: the shape a
* schematic tool generates, and the shape that otherwise exits 0.
Vs In 0 DC 3
Rl In MidNode 1k
Rg MidNode 0 3k
.save v(midnode)
.op
.control
run
if $sim_status ne 0
  echo RUN-FAILED
  quit 1
end
write guard.raw
.endc
.end
```

The `.save` card spells the net `midnode` where the netlist defines it as
`MidNode`, so it resolves under `fold` and `preserve` and misses under
`distinguish`. One run per row, `ngspice -b -n -D casemode=<mode> guard.cir`:

| mode | rc | guard fired | `guard.raw` |
| --- | --- | --- | --- |
| `fold` | 0 | no | written, `Plotname: Operating Point`, 281 bytes, one variable `v(midnode)` |
| `preserve` | 0 | no | written, `Plotname: Operating Point`, 281 bytes, one variable `v(MidNode)` |
| `distinguish` | **1** | **yes** | **absent** |

The `distinguish` row is the failing run: rc is 1 where the unguarded deck gave
0, and the constants artefact is never created, because the guard quits before
the `write`. The `fold` and `preserve` rows are `doc/codex/issues/0056`'s fix
— the folded `.save` card resolves — and are the negative control.

Delete the four guard lines (`if` through `end`) and the same deck exits **0**
in every mode:

```
UNGUARDED fold         rc=0   Plotname: Operating Point   281 bytes
UNGUARDED preserve     rc=0   Plotname: Operating Point   281 bytes
UNGUARDED distinguish  rc=0   Plotname: constants         570 bytes, 12 variables
```

**It works on stock.** Change that deck's one card to `.save v(nosuchnode)`,
which misses in every mode, and run both binaries with no `casemode` flag
anywhere:

```
guarded    stock ngspice-46   rc=1   RUN-FAILED   guard.raw ABSENT
guarded    ver_50 (no flag)   rc=1   RUN-FAILED   guard.raw ABSENT
unguarded  stock ngspice-46   rc=0   Plotname: constants   569 bytes
unguarded  ver_50 (no flag)   rc=0   Plotname: constants   570 bytes
```

So this is not a feature of this branch and a client can adopt the guard
against every ngspice they support.

**Two of the byte counts this issue first recorded have moved, and no run has.**
On 2026-08-14 the unguarded constants file measured 592 bytes here against 569
on stock, and 599 under `distinguish` in the control-only deck. The difference
was the `Option: casemode=…` line `raw_write()` had just gained, which the
20:52 build wrote unconditionally. As committed that line is opt-in behind
`casemodewrite` — `9e341a8b7`, the same evening,
`doc/codex/issues/0070` — so the default file is now 570 bytes and differs from
stock's 569 only by the `+` in the version string. Setting the gate brings both
old numbers back exactly: `set casemodewrite` before the `write` gives 592 under
`fold` and 599 under `distinguish`, those two lines being 22 and 29 bytes. As of
`731c01455` the `-r` writer emits the same gated line
(`doc/codex/issues/0071`). Either way the header line is a record of the mode,
not of the run's health: the bogus file carries it exactly as a good one does.

**Three properties a consumer has to know**, each measured rather than
reasoned. The first two come out of one deck:

```spice
* props.cir -- $sim_status before any run, after a failed one, after a good one
Vs In 0 DC 3
Rl In MidNode 1k
Rg MidNode 0 3k
.control
echo HAVE-BEFORE=$?sim_status
echo VALUE-BEFORE=$sim_status
save v(midnode)
op
echo HAVE-AFTER=$?sim_status
echo AFTER-BAD=$sim_status
save all
op
echo AFTER-GOOD=$sim_status
.endc
.end
```

`ngspice -b -n -D casemode=distinguish props.cir`, rc=0:

```
HAVE-BEFORE=0                                  <- stdout
VALUE-BEFORE=                                  <- stdout, empty
HAVE-AFTER=1
AFTER-BAD=1
AFTER-GOOD=0
Error: sim_status: no such variable.           <- stderr, from the read before the first run
```

1. **It is per analysis, and last writer wins.** `AFTER-BAD=1` then
   `AFTER-GOOD=0`: a deck that fails one analysis and then succeeds at another
   reads 0 at the end. So it must be read **after each run**, not once at the
   end of the block. This is the same reading `doc/codex/issues/0059`'s
   `status_probe.cir` already took, from the other side.
2. **It does not exist before the first analysis.** `$?sim_status` is `0` before
   the first `op` and `1` after it, `echo $sim_status` answers an empty string,
   and the read puts `Error: sim_status: no such variable.` on stderr. A guard
   that can be reached before the first `run` therefore has to test
   `$?sim_status` first, or accept the error line.
3. **A `run` that had no analysis to do reads 0.** A deck with no analysis card
   at all, whose `.control` block says only `run`:

   ```spice
   * noanalysis.cir -- no analysis card anywhere; the block only runs
   Vs In 0 DC 3
   Rl In MidNode 1k
   Rg MidNode 0 3k
   .control
   run
   echo AFTER-RUN=$sim_status
   .endc
   .end
   ```

   answers `AFTER-RUN=0`, prints `Note: Simulation executed from .control
   section` and exits 0 — arm 3 of the chain. `runcoms.c:329` writes `err`
   before dispatching and nothing failed. So `sim_status == 0` means "the last
   analysis did not report a failure", not "an analysis produced data". A
   consumer that needs the second question has to ask the rawfile, which is
   `doc/codex/issues/0059` again.

**What this does not give the client.** `$sim_status` is in-band: it is
readable by the deck and it reaches a consumer only if the deck is written to
relay it. A tool that runs decks it did not generate cannot use it. That is the
residual, and it is the same residual `0059` records — neither `sim_status` nor
`curplot` reaches the rawfile.

## Related

- `doc/codex/issues/0059` — the artefact this issue is the missing signal for.
  Its "rc is not the defence FINDINGS credits it with" section is the same
  observation from the write path, and its `rc0.cir` is arm 2 of the chain
  reached a different way (a good analysis after a bad one).
- `doc/codex/issues/0057` — the diagnostic half. Its per-simulation contract
  plus this issue's two-simulation deck shape is R4 of the client's reply.
- `doc/codex/issues/0064` — the phantom `v(all)`, which the same client decks
  produced. **Fixed 2026-08-15** (`25e891ec3`), in the narrow scope: a bare
  `write` of a one-vector plot now holds the net's own name and one column,
  which is why the guard table's `fold` and `preserve` rows read *one variable
  `v(midnode)`* today where the same run would have written `v(midnode)` and
  `v(all)` the day before (`tests/regression/misc/wildcard-rename.cir` is the
  assertion). 0064's second mechanism — `com_write()`'s scale prepend, which
  still duplicates the column when the argument is a name rather than a wildcard
  — survives that fix and is being filed on its own number by item 5 of
  `doc/claude/batches/2026-08-15-xschem-open-items/PLAN.md`. Measured
  2026-08-15 on the guard deck under `fold`: bare `write guard.raw` gives one
  column `v(midnode)`, `write guard.raw v(midnode)` gives two columns of that
  same name. A vector-count check on a rawfile still has to know which of the
  two it is looking at.
