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
all, from a `.save` of a node no netlist has. Every measurement below was taken
2026-08-14 against `build-ver_50/src/ngspice` (`ngspice-46+`, build stamp
`Fri Aug 14 20:52:09 UTC 2026`), with stock as the baseline.

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
own rawfile. Four runs, `repro2/plain_fail.cir` and `repro2/ctl_fail.cir`:

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

```
* ctl_noop.cir -- no analysis dot card; op and write inside .control
.control
save v(midnode)
op
write ctl_noop.raw
.endc

rc=1, one analysis, one near-miss warning, and ctl_noop.raw is
599 bytes of Plotname: constants anyway.
```

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
| stderr | the near-miss warning, if the miss was a case near miss; **nothing at all** if the name is simply absent (`doc/codex/issues/0057`) |
| the rawfile | well formed, loads without a diagnostic, twelve variables (`doc/codex/issues/0059`) |

Measured on `repro2/absent.cir`, `.save v(nosuchnode)`, which needs no case
mode to reach:

```
ver_50 -D casemode=fold          rc=0   Plotname: constants   mentions of 'nosuchnode': 0
ver_50 -D casemode=preserve      rc=0   Plotname: constants   mentions of 'nosuchnode': 0
ver_50 -D casemode=distinguish   rc=0   Plotname: constants   mentions of 'nosuchnode': 0
stock ngspice-46, no flag        rc=0   Plotname: constants
```

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
rc=1 will meet it: **rc=1 does not mean nothing was written.** `ctl_noop.cir`
above exits 1 and leaves a 599-byte constants rawfile on disk, and the `-r`
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
  failed: `error3` is 1, so `EXIT_BAD`. This is why `ctl_noop.cir` exits 1 and
  why "`.control` swallows the status" is the wrong description.

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

## Resolution

**Not fixed, by decision, and the replacement is measured.**

`$sim_status` is the per-analysis outcome, it is readable from the control
language before the `write` that would produce the bogus artefact, and `quit
<n>` propagates a status of the deck's choosing. So a client gets a reliable
signal with no ngspice change and no risk to the scripts of criterion 1.

Measured in the client's own deck shape — analysis dot card, `.control run`,
no `-r`, which is the shape that gives rc=0:

```
* guard_both.cir
.save v(midnode)
.op
.control
run
if $sim_status ne 0
  echo RUN-FAILED
  quit 1
end
write guard_both.raw
.endc
```

| mode | rc | guard fired | rawfile |
| --- | --- | --- | --- |
| `fold` | 0 | no | written, `Plotname: Operating Point` |
| `preserve` | 0 | no | written, `Plotname: Operating Point` |
| `distinguish` | **1** | **yes** | **absent** |

The `distinguish` row is the failing run: rc is 1 where the unguarded deck gave
0, and the constants artefact is never created, because the guard quits before
the `write`. The `fold` and `preserve` rows are `doc/codex/issues/0056`'s fix
— the folded `.save` card resolves — and are the negative control.

**It works on stock.** Same guard, `.save v(nosuchnode)`, no `casemode` flag
anywhere:

```
stock ngspice-46      rc=1   RUN-FAILED   guard_absent.raw ABSENT
ver_50 (no flag)      rc=1   RUN-FAILED   guard_absent.raw ABSENT
```

Unguarded, the same deck is rc=0 with a constants raw on both binaries — 569
bytes on stock, 592 here, the difference being the `Option: casemode=fold` line
`raw_write()` gained on 2026-08-14, which the bogus file carries exactly as a
good one does. So this is not a feature of this branch and a client can adopt
the guard against every ngspice they support; and the new header line is a
record of the mode, not of the run's health.

**Three properties a consumer has to know**, each measured rather than
reasoned:

1. **It is per analysis, and last writer wins.** A deck that fails one analysis
   and then succeeds at another reads 0 at the end:

   ```
   .control
   save v(midnode) / op / echo AFTER-BAD=$sim_status     -> AFTER-BAD=1
   save all        / op / echo AFTER-GOOD=$sim_status    -> AFTER-GOOD=0
   ```

   So it must be read **after each run**, not once at the end of the block.
   This is the same reading `doc/codex/issues/0059`'s `status_probe.cir`
   already took, from the other side.
2. **It does not exist before the first analysis.** `echo $sim_status` in a
   `.control` block that has not run anything answers an empty string and puts
   `Error: sim_status: no such variable.` on stderr. A guard that runs before
   the first `run` therefore has to test `$?sim_status` first, or accept the
   error line.
3. **A `run` that had no analysis to do reads 0.** A deck with no analysis card
   at all, whose `.control` block says only `run`, sets `sim_status` to 0 —
   `runcoms.c:329` writes `err` before dispatching and nothing failed. So
   `sim_status == 0` means "the last analysis did not report a failure", not
   "an analysis produced data". A consumer that needs the second question has
   to ask the rawfile, which is `doc/codex/issues/0059` again.

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
  produce, and which is why a vector-count check on the rawfile has to expect
  n+1 for a single-vector plot.
