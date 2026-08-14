# Issue: A Fatal Netlist Read Strands the Reader Guard, and a Shared Host Never Gets It Back

## Status

Closed 2026-08-13 by one line in `shared_exit()` (`src/sharedspice.c:2192`).

A defect in the fix for `doc/codex/issues/0061`, found by that issue's own
verifier and not by a test — no test could have found it, for the reason
acceptance criterion 4 gives. 0061 narrowed `cp_getvar()` so that a loaded
plot's environment does not answer a variable read taken while a netlist is
being turned into cards, and the window is a depth count raised and lowered by
`inp_readall()` (`src/frontend/inpcom.c:1307`-`:1318`). A netlist read that
ends *fatally* does not return through `inp_readall()`, so it never lowers the
count.

In the standalone binary that is harmless: `controlled_exit()`
(`src/frontend/error.c:28`) calls `exit()` and there is no process left to hold
a stale count. In a `--with-ngshared` build it calls `shared_exit()` instead,
which discards the C stack and hands control back to the host — and the count,
a file-static `int`, stays at 1 for the rest of the host process. From that
moment `plot_cur->pl_env` answers no `cp_getvar()` at all, ever again.

This is the class `CLAUDE.md` names: *anything touching global or static
simulator state must survive repeated `ngSpice_Reset` in the shared build*,
with `c5cd68015` and `5ad395d5e` as precedent. 0061 anticipated two of the
three ways out and missed the third.

Measured 2026-08-13 on branch `ver_50` at `720c8743a` plus this batch's
uncommitted work, against `build-shared/`, configured with a bare
`../configure --with-ngshared` and reporting itself as `ngspice-46+ shared
library`. `build-shared/` is ~98 MB and is not committed;
`doc/codex/issues/0062` describes the same tree. Every measurement below is
`doc/claude/feedback/ngspice_upstream/repro/shared/netlist_guard_probe.c`, run
verbatim. That file is **not** committed and nothing in this batch is:
`doc/claude/feedback/` is untracked in its entirety. See criterion 4.

## Summary

Load a plot carrying `Option: nosort`, ask for `display`, feed the library one
deck with a missing `.include`, ask for `display` again. The second answer is
in a different order from the first, and nothing the host did changed the
plot.

```
$ gcc -g -O0 -o /tmp/netlist_guard_probe \
      doc/claude/feedback/ngspice_upstream/repro/shared/netlist_guard_probe.c \
      -I src/include -L build-shared/src/.libs -lngspice
$ cd /tmp && LD_LIBRARY_PATH=.../build-shared/src/.libs \
      SPICE_SCRIPTS=.../build-shared/src /tmp/netlist_guard_probe command
...
==BEFORE==
Here are the vectors currently active:
    v(in)               : voltage, real, 1 long [default scale]
    i(vs)               : current, real, 1 long
Error: Could not find include file ngp_no_such_file.inc
Error: ngspice.dll cannot recover and awaits to be reset or detached
==AFTER==
Here are the vectors currently active:
    i(vs)               : current, real, 1 long
    v(in)               : voltage, real, 1 long [default scale]
MODE command  HELD=1  before=1 after=0  fatal-before-AFTER=1  GUARD-STUCK
```

`nosort` is a plot-environment key: `com_display()`
(`src/frontend/com_display.c:70`) asks `cp_getvar("nosort", CP_BOOL, ...)`, the
loaded header put `nosort` in `plot_cur->pl_env`, and before the bad deck that
read is answered. After it, it is not, so `display` sorts. `nosort` is only the
cheapest thing to photograph — **every** `cp_getvar()` read of **every**
`Option:` key is dead from that point, in a library the host has been told it
may keep using.

`reset` at the ngspice prompt does not clear it: `com_rset()` is not
`totalreset()`. Only the `ngSpice_Reset()` API reaches the
`inp_netlist_read_reset()` that `doc/codex/issues/0061` put in `totalreset()`,
and a host that never resets never recovers.

### Three ways out, not two

The fix `doc/codex/issues/0061` shipped named the two `setjmp()` landing sites
and reset the count at one of them (`totalreset()`) and at the standalone
interrupt's (`ft_sigintr_cleanup()`). There is a third route, and it lands at
neither. All three were measured, one process per route, before and after:

| route | how the read leaves | before | after |
|---|---|---|---|
| `command` | `ngSpice_Command("source bad.cir")` → `longjmp(errbufc)` → `ngSpice_Command()` (`sharedspice.c:2217`) | GUARD-STUCK | GUARD-OK |
| `circ` | `ngSpice_Circ(deck)` → `longjmp(errbufm)` → `ngSpice_Circ()` (`:2215`) | GUARD-STUCK | GUARD-OK |
| `bg` | `ngSpice_Command("bg_source bad.cir")` → `pthread_exit()` (`:2204`) | GUARD-STUCK | GUARD-OK |

The `bg` route is the one that decides where the fix goes. `runc()`
(`sharedspice.c:648`-`:651`) runs *any* command prefixed `bg_` on a worker
thread, `bg_source` included, so a netlist read can be in progress on a thread
that is not the one the host called in on. When that read dies,
`shared_exit()`'s `if (fl_running && !fl_exited)` arm exits the thread and
returns to nobody. There is no landing site to clear the count at.

## Impact

A libngspice host that ever feeds ngspice a deck that fails to read — a
missing `.include`, an unreadable `.lib`, an unset environment variable in a
path — silently loses the whole plot-environment namespace for the rest of its
process, on every plot, including plots loaded afterwards. The failing deck is
the user's ordinary mistake; the consequence outlives it by the length of the
session and is attached to nothing the user can see.

It is worse than it first looks in one specific way, and better in another.

Worse: the reads that stop being answered are the ones a *data file* supplied,
which is the whole of what `doc/codex/issues/0061` decided to keep working
("shape A" — the pair is still filed, still listed, still readable). This
defect quietly reverses that decision at the first bad deck.

Better: the direction of the failure is safe. A stranded count makes the guard
*more* closed, never less, so no file gets to reconfigure a read that it should
not. Nothing is corrupted and nothing crashes; answers just stop coming.

## Root Cause

`inp_readall()` is a wrapper whose contract is that the count is balanced:

```c
struct card *inp_readall(FILE *fp, ...)
{
    struct card *cc;

    inp_netlist_read_depth++;
    cc = inp_readall_cards(fp, ...);
    inp_netlist_read_depth--;

    return cc;
}
```

`inp_readall_cards()` and everything under it can call `controlled_exit()` —
`src/frontend/inpcom.c` alone has some thirty call sites, of which
`inp_pathresolve()` and the `.include` failure at `:1502` are the ones an
ordinary deck reaches. In the shared build `controlled_exit()` does not return,
so the decrement is never executed and the `++` above it is permanent.

That is the whole of it. The mechanism is not subtle; what is subtle is that it
is invisible in the only build anyone tests. The same wrapper in
`build-ver_50` is correct, because there the same call ends the process.

## Acceptance Criteria

1. A fatal netlist read leaves the count as it found it, and the plot
   environment answers `cp_getvar()` afterwards exactly as it did before.
2. All three routes out of a fatal read are covered, including the one that
   ends in `pthread_exit()` and lands at no `setjmp()`. A fix at the two
   landing sites only is not sufficient and the third route must be measured,
   not argued.
3. The guard itself is not weakened to achieve this. The narrowing
   `doc/codex/issues/0061` shipped must still hold: an `Option:` line in a
   loaded plot must still fail to reconfigure the next netlist read. A "fix"
   that deletes the guard passes criterion 1 trivially and must fail the
   evidence.
4. If it cannot be pinned by `make check`, say so plainly and pin it with a
   committed probe instead. Nothing under `tests/` links `libngspice` and
   `build-ver_50` is not a shared build, so `make check` never executes
   `shared_exit()` at all; there is no deck, in any harness this repo has, that
   can reach the defect.
5. `make check` unchanged. The production change is in a file the standalone
   binary does not compile, so the expected result is *identical*, not merely
   passing.

## Resolution

Closed 2026-08-13. One production line plus its comment, in
`shared_exit()` (`src/sharedspice.c:2192`), immediately before the arm that
exits a worker thread:

```c
    inp_netlist_read_reset();

    // if we are in a worker thread, we exit it here
```

**Why there and not at the landing sites.** `shared_exit()` is
`ATTRIBUTE_NORETURN`, and every route out of it below that point discards the
whole C stack of the call it was reached from, `inp_readall()` frames
included. It is the last point common to all three routes: the `pthread_exit()`
eight lines down leaves without reaching either `longjmp()`, so the two
`setjmp()` sites between them cover only two of the three. Placing it after the
`printsend` flush above means no output is lost, and the guard being down
across the `bgtr()`/`ngexit()` callbacks that follow is correct — by then
nothing of ours is reading.

The `inp_netlist_read_reset()` call `doc/codex/issues/0061` put in
`totalreset()` (`:2627`) and the one in `ft_sigintr_cleanup()` are both kept.
The first is now belt and braces for the fatal-read case and still the only
cover for a host that resets mid-read for its own reasons; the second is the
standalone interrupt, a different path entirely.

`src/frontend/inpcom.c`'s comment above `inp_netlist_read_reset()` was rewritten
to enumerate three callers instead of two, and to say why the standalone binary
needs only one of them.

**Criterion by criterion.**

1. *A fatal read leaves the count as it found it.* The `command` row of the
   table above, re-run after the change:

   ```
   ==BEFORE==
       v(in)               : voltage, real, 1 long [default scale]
       i(vs)               : current, real, 1 long
   Error: Could not find include file ngp_no_such_file.inc
   ==AFTER==
       v(in)               : voltage, real, 1 long [default scale]
       i(vs)               : current, real, 1 long
   MODE command  HELD=1  before=1 after=1  fatal-before-AFTER=1  GUARD-OK
   ```

   The probe asserts the fatal error was delivered *before* the second
   `display` (`fatal-before-AFTER=1`) rather than assuming it, because without
   `low_latency` the shared build hands output to a `printsend` thread which
   preserves message order but not its timing relative to the caller. That is
   also why the phases are marked in band with `echo` instead of by a flag
   around the call.

2. *All three routes, measured.* One process per route, since a fatal read
   leaves the library asking to be reset and the routes must not contaminate
   each other:

   ```
   $ for m in command circ bg; do /tmp/netlist_guard_probe $m >/dev/null; echo "$m rc=$?"; done
   command rc=0
   circ rc=0
   bg rc=0
   ```

   and with only the `inp_netlist_read_reset()` line removed and the library
   rebuilt, the same three:

   ```
   command rc=1   MODE command  HELD=1  before=1 after=0  GUARD-STUCK
   circ    rc=1   MODE circ     HELD=1  before=1 after=0  GUARD-STUCK
   bg      rc=1   MODE bg       HELD=1  before=1 after=0  GUARD-STUCK
   ```

   The `bg` row needed one thing of the probe that is not about this issue:
   `shared_exit()` calls `bgtr()` through an unchecked pointer where it checks
   `ngexit()` (`sharedspice.c:2198` against `:2200`), so a host that passes
   `NULL` for the `BGThreadRunning` callback segfaults inside `shared_exit()`
   before this defect can be observed. The probe supplies one. That is a
   separate upstream nit, noted under Left.

3. *The guard is not weakened.* Every run of the probe carries a positive
   control ahead of the fatal read: a plot carrying `Option: casemode=preserve`
   is made current and a deck whose net is spelled `MidNode` is sourced. If
   the guard were gone the vector would come back `MidNode`; it must come back
   folded. That is `doc/codex/issues/0061` criterion 1, and this is the first
   time it has been measured in a shared build — 0061 could only compile-check
   that side. Reported as `HELD=1` above, and the two assertions are
   independent, measured by breaking each one separately:

   ```
   fix removed, guard intact:   HELD=1  before=1 after=0   (this issue's defect)
   fix intact, guard removed:   HELD=0  before=1 after=1   (0061's defect)
   ```

   So neither half of the probe is passing on the other's mechanism.

4. *No `make check` test is possible, and this is the pin.* Stated plainly:
   **no file under `tests/` links `libngspice`**, and `build-ver_50` is
   configured without `--with-ngshared`, so `shared_exit()` is not in the
   binary `make check` runs. The defect is not reachable from any harness this
   repo has. The pin is
   `doc/claude/feedback/ngspice_upstream/repro/shared/netlist_guard_probe.c`,
   which sits beside `doc/codex/issues/0062`'s three probes, is self-contained
   (it writes its own four input files, and a rawfile carries a wall-clock
   `Date:`, which is what that directory's `.gitignore` excludes `*.raw` for),
   is self-checking (exit 0/1, so a `for` loop over the three modes is the
   whole test), and prints the entire captured session so a failure can be read
   rather than guessed at. Its header comment carries the build and run lines
   and the meaning of each mode.

   **It is not committed, and neither is anything else in this batch.**
   `doc/claude/feedback/` is untracked in its entirety — the probe, 0062's
   three, and the `repro/` decks alike. Calling it "the pin" is therefore a
   statement about what it *can* do and not about what the repository
   currently guarantees: as things stand it is a working file that vanishes
   with the working copy. Whoever lands this issue has to decide where a probe
   that cannot live under `tests/` should live, and put it there on purpose.
   That decision is open and is recorded under Left.

5. *`make check` unchanged.* 305 passed, 0 failed, with every cached `*.log`
   and `*.trs` deleted first — the same 305 this batch has been running.
   `src/sharedspice.c` is compiled only into `libngspice`, so this was the
   expected result and not merely the hoped-for one. `identity.baseline` is
   unchanged: the change adds no string comparison.

**Left.**

- **The pin is not in the tree.** Criterion 4 says no `make check` test can
  reach this defect and names the probe as the substitute, but the probe is an
  untracked file under `doc/claude/feedback/`, as is every other probe and deck
  this batch wrote. So the criterion is met by a file anyone can delete without
  noticing. Either the shared probes get a committed home — `tests/` cannot
  hold them today, since nothing there links `libngspice` — or the issue closes
  knowing its evidence is reproducible but not preserved. Not decided here.
- `shared_exit()` calls `bgtr()` unguarded at `sharedspice.c:2198` while
  guarding `ngexit()` two lines later, so a host that passes `NULL` for the
  `BGThreadRunning` callback — which `ngSpice_Init()` accepts — segfaults
  inside the error path rather than being told anything. Pre-existing,
  upstream, unrelated to the reader guard, and measured only because the probe
  tripped over it. Worth its own issue; it is not this one and the probe works
  around it in one line.
- The window is still `inp_readall()`, and the elaboration that follows it in
  `inp_spsource()` is still outside it, exactly as `doc/codex/issues/0061`
  recorded. Nothing here widens or narrows the window.
- The count is a plain `static int` with no synchronisation, and `bg_` puts a
  netlist read on a second thread. Two threads reading netlists at once would
  race on it. That is not reachable today — `runc()` starts at most one worker
  and the API is documented as one call at a time — and it is a property of
  the counter as `doc/codex/issues/0061` designed it, not of this fix.
