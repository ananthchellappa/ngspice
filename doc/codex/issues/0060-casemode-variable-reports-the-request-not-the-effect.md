# Issue: `$casemode` Reports the Request, Not the Effect

## Status

Closed 2026-08-13 by the `curcasemode` variable described under Resolution.
Found by a client-program integration — xschem adding case-preserving
signal names, reading the raw file back and showing the names to the user —
and reported as finding 8 of
`doc/claude/feedback/ngspice_upstream/FINDINGS.md`. Every claim below was
re-measured 2026-08-13 at `720c8743a` against `build-ver_50/src/ngspice`
(`ngspice-46+`, build stamp `Wed Aug 12 19:28:37 UTC 2026`) with
`/usr/local/bin/ngspice` (`ngspice-46`, no `casemode` support) as the baseline.
Every deck quoted below is present in the working tree under
`doc/claude/feedback/ngspice_upstream/repro/` and is named where it is used:
`report_probe.cir`, `late_set.cir`, `three_way_probe.cir`,
`relatch_source.cir` with `relatch_inner.cir`, `include_count.cir` with
`include_count.inc`, `divider.cir`, and the generated `hdr_option.raw`, which
`repro/run_all.sh` builds.

Not a regression: the variable and the latch have been two objects since
`casemode` existed. Mode independent in the sense that it can diverge in all
three, though only `preserve` and `distinguish` have anything to lose by it.

## Summary

`casemode` is an ordinary writable control variable. The mode it selects is a
`static int` latched once per netlist read. Nothing couples them after the
latch, so `$casemode` answers what was *asked for*, and the run behaves
according to what was *in effect*.

The latch is `ng_case_mode` (`src/frontend/inpcom.c:1034`), written only by
`set_case_mode()` (`:1085`), called only from `inp_readall()` (`:1178`). Its
single read of the variable is one line:

```c
    if (!cp_getvar("casemode", CP_STRING, mode, sizeof(mode) - 1))   /* :1091 */
        return;
```

`casemode` has no arm in `cp_usrset()` (`src/frontend/options.c:276`) — grep
finds no occurrence of the name in that file — so a `set casemode=…` falls
through the `eqc()` chain to the generic tail and is normally `US_OK`:
recorded in the variable list, no side effect. It is not *always* `US_OK`:
`options.c:414-417` returns `US_READONLY` for any name already present in
`plot_cur->pl_env`, so once a loaded rawfile has put `casemode` there the
`set` is refused outright (measured in criterion 6; the mechanism is
`doc/codex/issues/0061`). Neither outcome moves the enum. The two objects meet
at `:1091` and nowhere else.

**Across two binaries.** The same deck, the same flag, `rc=0` and empty stderr
on both. The deck is `repro/report_probe.cir` — `repro/late_set.cir` with its
`set casemode=preserve` line removed, so that it only reports and never writes:

```
* report-only probe -- reads $casemode, never writes it; nets In / MidNode, source Vs
Vs In 0 DC 3
Rl In MidNode 1k
Rg MidNode 0 3k
.control
op
display
echo casemode-is $casemode
.endc
.end
```

Both runs below are from `repro/`; the `display` listing and the `echo` line
are excerpted from the full batch output, which is otherwise identical between
the two binaries except for the title line and the version banner:

```
$ NG=build-ver_50/src/ngspice ; BASE=/usr/local/bin/ngspice

$ $NG   -b -n -D casemode=preserve report_probe.cir
    In                  : voltage, real, 1 long [default scale]
    MidNode             : voltage, real, 1 long
    Vs#branch           : current, real, 1 long
casemode-is preserve

$ $BASE -b -n -D casemode=preserve report_probe.cir
    in                  : voltage, real, 1 long [default scale]
    midnode             : voltage, real, 1 long
    vs#branch           : current, real, 1 long
casemode-is preserve
```

On the featureless build the variable is not merely stale, it is an outright
lie: nothing reads `casemode` there, an unknown `-D name=value` is just a
control variable, and the answer is the mode the user asked for.

**Within one binary, batch.** `repro/late_set.cir`, whose `.control` block sets
the variable after the deck has been read:

```
$ build-ver_50/src/ngspice -b -n doc/claude/feedback/ngspice_upstream/repro/late_set.cir
    in                  : voltage, real, 1 long [default scale]
    midnode             : voltage, real, 1 long
    vs#branch           : current, real, 1 long
casemode-is preserve
```

(The `display` listing and the `echo` line, excerpted from the batch output.)
`rc=0`, stderr empty. The deck's nets are `In` and `MidNode`; they come back
folded because the deck was read before the `set` ran, and `$casemode` answers
`preserve` because the `set` ran.

**Within one binary, `ngspice -p`, where no deck is read at all.** In a session
that sources nothing the latch runs during start-up and never again — a
`source` typed later is a second `inp_readall()` and does move it, which is
criterion 2's subject — so anything typed at the prompt moves only the
variable. Full stdout, `-p` echoing its own input:

```
$ printf 'set casemode=distinguish\necho v $casemode\nlet ABC = 1\nprint abc\nquit\n' \
    | build-ver_50/src/ngspice -p -n
set casemode=distinguish
ngspice 10002 -> echo v $casemode
v distinguish
ngspice 10003 -> let ABC = 1
ngspice 10004 -> print abc
abc = 1.000000e+00
ngspice 10005 -> quit
ngspice-46+ done
```

stderr was empty (0 bytes): no experimental banner. And `print abc` finds
`ABC` — the session is folding while the variable claims to distinguish.

**And in the other direction, where the binary contradicts itself two lines
apart.** Same session, mode latched to `distinguish` by the flag, variable
moved afterwards. The two streams are captured separately and shown separately,
because the warnings are on stderr and the `echo` they contradict is on stdout:

```
$ printf 'let ABC = 1\nset casemode=fold\necho v $casemode\nprint abc\nquit\n' \
    | build-ver_50/src/ngspice -p -n -D casemode=distinguish >o 2>e

$ cat o
let ABC = 1
ngspice 10002 -> set casemode=fold
ngspice 10003 -> echo v $casemode
v fold
ngspice 10004 -> print abc
ngspice 10005 -> quit
ngspice-46+ done

$ cat e
Warning: casemode 'distinguish' is experimental. Identifier identity is case sensitive, and a vector, a B source V() reference or an XSPICE node whose resolution misses by case is reported, but a deck that spells one net two ways becomes a deck with two nets and nothing says so, because both spellings are definitions.
Warning: casemode 'distinguish' is experimental. Identifier identity is case sensitive, and a vector, a B source V() reference or an XSPICE node whose resolution misses by case is reported, but a deck that spells one net two ways becomes a deck with two nets and nothing says so, because both spellings are definitions.
Warning: no vector named 'abc'; 'ABC' differs only in case (casemode=distinguish)
Warning: no vector named 'abc'; 'ABC' differs only in case (casemode=distinguish)
Warning from checkvalid: vector abc is not available or has zero length.
```

Nothing is elided. The experimental banner appears twice because
`set_case_mode()` runs once per input file read (`doc/codex/issues/0058`), and
the near-miss warning appears twice because of `doc/codex/issues/0046`.

`vec_warn_case_near_miss()` (`src/frontend/vectors.c:464`, its `fprintf` at
`:478`) is the one place the
effective mode reaches the user, and only because its `(casemode=distinguish)`
is a literal on a path that runs under no other mode.

**What is NOT an instance.** Finding 5 of `FINDINGS.md` — a `.spiceinit`
beside the deck holding `set casemode=fold`, defeating `-D casemode=preserve` —
looks like a second instance and is not one. There the init file writes the
*variable*, and `:1091` then reads it, so both agree. Measured in
`repro/spiceinit/` (whose `.spiceinit` is the one line `set casemode=fold`)
with `report_probe.cir` copied in beside it — again the `display` listing and
the `echo` line, excerpted from the batch output:

```
$ build-ver_50/src/ngspice -b    -D casemode=preserve report_probe.cir   # no -n
    in                  : voltage, real, 1 long [default scale]
    midnode             : voltage, real, 1 long
    vs#branch           : current, real, 1 long
casemode-is fold                                    <- correct, the flag lost

$ build-ver_50/src/ngspice -b -n -D casemode=preserve report_probe.cir
    In                  : voltage, real, 1 long [default scale]
    MidNode             : voltage, real, 1 long
    Vs#branch           : current, real, 1 long
casemode-is preserve                                <- correct, the flag won
```

That override is its own complaint and belongs to finding 5; the variable
tracks it correctly and this issue does not cover it.

**The default is a third shape.** With no `-D` at all the effective mode is
`fold` and the variable does not exist, so the natural read is an error rather
than an answer. Streams separated again, because the error is on stderr and
the empty answer is on stdout:

```
$ printf 'echo v $casemode\nlet ABC = 1\nprint abc\nquit\n' \
    | build-ver_50/src/ngspice -p -n >o 2>e

$ cat o
echo v $casemode
v
ngspice 10002 -> let ABC = 1
ngspice 10003 -> print abc
abc = 1.000000e+00
ngspice 10004 -> quit
ngspice-46+ done

$ cat e
Error: casemode: no such variable.
```

## Impact

**The tree's own guide already tells users not to trust it.**
`doc/claude/casemode-distinguish-guide.md:47`: *"Do not trust
`echo $casemode` — it reports the variable, which is set in exactly the cases
that fail."* The guide offers two replacements, and the reason this issue is
worth fixing is what the working one costs, not that none exists:

- §2 (`guide:52-62`) is a **three-way deck probe** — `let CaseProbe = 1`,
  `let caseprobe = 2`, `print CaseProbe caseprobe` in a `.control` block — and
  it does separate all three modes, `preserve` included. `repro/three_way_probe.cir`
  is that probe on the `In`/`MidNode` divider. Measured, the `print` lines
  only:

  ```
  $ NG -b -n -D casemode=fold        three_way_probe.cir   -> caseprobe = 2.000000e+00
                                                              caseprobe = 2.000000e+00
  $ NG -b -n -D casemode=preserve    three_way_probe.cir   -> CaseProbe = 2.000000e+00
                                                              caseprobe = 2.000000e+00
  $ NG -b -n -D casemode=distinguish three_way_probe.cir   -> CaseProbe = 1.000000e+00
                                                              caseprobe = 2.000000e+00
  $ NG -b -n                         three_way_probe.cir   -> the fold shape
  $ BASE -b -n -D casemode=preserve  three_way_probe.cir   -> the fold shape
  ```

  So it separates the two binaries as well. Its cost is that it needs a deck,
  a spawn and a parse of the `print` output, and that it cannot be spliced
  into a session that is already running.
- §9 (`guide:194-197`) is the client-facing one, and it is an **identity**
  test run through `ngspice -p`, so it separates `distinguish` from the other
  two and cannot see `preserve` at all — finding 6, and `guide:220-224` says
  so itself: *"It cannot distinguish `fold` from `preserve`, and no pipe-mode
  probe can: … (§2's three-way table works only from inside a deck, where the
  `.control` text has itself been through the reader.)"* — the elision is one
  sentence of explanation.

The gap is therefore narrower than "no probe": there is no probe for a session
that reads no deck, which is exactly where `$casemode` is the thing that looks
like one.

**The tree's own tests read it.** `grep -rn '\$casemode' tests/` returns four
lines, in four committed decks:

- `tests/regression/case/harness-alive.cir:14` echoes it and its reference
  `.out` asserts `CASEMODE-IS preserve`. Measured, that deck's output is byte
  identical from a binary that has the feature and one that does not:

  ```
  $ /usr/local/bin/ngspice        -b -n -D casemode=preserve harness-alive.cir
  v(out) = 7.500000e-01
  CASEMODE-IS preserve
  $ build-ver_50/src/ngspice      -b -n -D casemode=preserve harness-alive.cir
  v(out) = 7.500000e-01
  CASEMODE-IS preserve
  ```

  (`v(out)` and the echo, excerpted from the batch output.) The deck's own
  header is careful — it claims only to prove that
  `tests/regression/case/Makefile.am:65`'s `TESTS_ENVIRONMENT` reached the
  binary — and that is exactly and only what it proves. It is offered as the
  directory's liveness probe and it passes on `ngspice-46`.
- `tests/regression/casedist/harness-alive.cir:19` is the same deck for the
  `distinguish` directory and its `.out` asserts `CASEMODE-IS distinguish`. It
  carries the same weakness, and measured the same way its two excerpted lines
  are again byte identical between the binaries:

  ```
  $ /usr/local/bin/ngspice   -b -n -D casemode=distinguish harness-alive.cir
  v(out) = 7.500000e-01
  CASEMODE-IS distinguish
  $ build-ver_50/src/ngspice -b -n -D casemode=distinguish harness-alive.cir
  v(out) = 7.500000e-01
  CASEMODE-IS distinguish
  ```
- `tests/xspice/casedist/harness-alive.cir:23` is the XSPICE twin of that one,
  asserting `CASEMODE-IS distinguish` beside `v(aout) = 5.000000e+00`. Run
  with cwd `build-ver_50/tests/xspice/casedist/` — `SPICE_SCRIPTS=.` for the
  repo build, so the directory's own `spinit` loads `digital.cm`; the baseline
  needs nothing, its installed `spinit` already does — both binaries produce
  both lines. So the `casemode` half of its liveness claim is as weak as the
  other two. Its extra claim, that the code model loaded, is real and is not
  affected.
- `tests/regression/casedist/vector-pyplot-probe-report.cir:64` gates a whole
  block of assertions on `strcmp cm "$casemode" distinguish`. Correct today,
  because nothing in that deck writes the variable after the read that latched
  the mode. It is correct by circumstance, not by construction.

Three of the four are liveness probes whose stated purpose includes failing
when the `-D casemode=` flag stops arriving, and none of the three would
notice a binary that accepted the flag and ignored it.

**A consumer of ngspice output cannot ask a *live session* how it cased its
names.** A schematic editor that generates the deck has an answer available and
should use it: splice §2's three-way probe into the `.control` block it is
already writing and read the mode for that exact run off stdout, or write one
throwaway raw and look at the `Variables:` names, which is what
`guide:224-226` and `guide:234-248` recommend. What has no answer is the shape
where no deck is involved — a `libngspice` session between `ngSpice_Circ()`
calls, an `ngspice -p` co-process, a wrapper that wants to check the binary
before it commits to generating anything. There `$casemode` is the only
question available and it answers the request. The failure mode is silent and
it is the guide's *"most likely way to get a silently wrong answer"*
(guide:20-25): a wrapper that finds plain `ngspice` on `$PATH`, passes
`-D casemode=preserve`, gets `rc=0`, reads back `preserve`, and is handed
lower-case names it will show the user as the schematic's spelling.

**Reachable from three routes.** A `.control` block after the deck was read
(`late_set.cir` above), the interactive prompt and `ngspice -p`, and
`ngSpice_Command("set casemode=…")` — the last read from
`src/include/ngspice/sharedspice.h:93-97`, which already documents the latch
semantics (*"It takes effect on the next netlist read"*), and not exercised:
no `libngspice` is built in `build-ver_50`. Two of these three are also
`doc/codex/issues/0048`'s and `0033`'s routes, but for a different reason and
with a different membership: those two are about the reader folding a
mixed-case variable *name*, so they exclude the `.control` block, which is this
issue's first route and its headline measurement. Here the name is always
lower case and the reader's fold is irrelevant; what makes the write late is
that the latch has already fired.

No deck asserts the divergence: all four `$casemode` readers above read the
variable, and none compares it against an observed effect.

## Root Cause

A latched enum and a writable string of the same name, coupled at one instant
per netlist read and free to drift on either side of it.

`set_case_mode()`'s comment (`src/frontend/inpcom.c:1079-1083`) states the
coupling precisely and states it as a rule about *writers*: *"the last writer
before the deck is read wins"*. That rule is correct and is not the defect.
The defect is that the variable stayed **readable** after it stopped being
authoritative. A write after the latch is a no-op — which is documented, in
the guide and in `sharedspice.h` — but the *read* after the latch is not a
no-op: it returns, with no diagnostic and in the same syntax as any other
variable, a string that is now about a run that did not happen.

The drift is symmetric, which is why a one-sided fix does not close it. The
write can be too late (the `.control` block, the prompt), or early enough to
latch and then undone (`-D casemode=distinguish` followed by
`set casemode=fold`). And two of the four measured shapes are not writes at
all but the *absence* of one — on a binary that never had the feature, where
the string is the only object there is, and on a binary that has it and is
defaulting to `fold`, where the enum is the only object there is. A read that
returns the string can be wrong in all four.

**Where the taxonomy applies.** This is not a comparison of two names, so no
row of `doc/claude/decisions/0001-distinguish.md` decision 3's Class A / B / C
table covers it. It touches the taxonomy at exactly one point, and that point
is the fix's foundation: the `static int ng_case_mode`
(`src/frontend/inpcom.c:1034`) is the single authority behind every classified
predicate — Class A's `inp_case_exact_ids()` (`:1060`) and `ng_ideq()`
(`:1074`), Class B's `inp_case_folding()` (`:1041`), Class C's
`vec_name_eq()` (`src/frontend/vectors.c:415`) and `Evt_Node_Name_Eq()`
(`src/xspice/evt/evtplot.c:65`). The first three read the static directly
because they live in the same file; the Class C pair, being outside it, go
through `inp_case_mode()` (`:1036`). All four are declared in
`src/include/ngspice/fteext.h:214-221`, but only `inp_case_mode()` returns the
mode itself — the other three answer derived yes/no questions. So there is
already exactly one function in the tree that reports the effective mode. What
is missing is any way for the *user* to call it.

Two adjacent things are right and are worth naming so the fix does not disturb
them. The mode *values* are matched with `cieq()` (`:1094`, `:1096`, `:1098`),
which is correct in all three modes: `fold`, `preserve` and `distinguish` are
words the control language defines, not identifiers a deck chose, so they
belong with the keywords `doc/claude/decisions/0001-distinguish.md` decision 5
puts on its "Not withdrawn" list (`:482`) — that list does not name them, and
this classification is an inference from it, not a quotation of it. The
variable *name* is matched byte-exactly, which is why `-D CaseMode=preserve`
is a silent no-op (finding 6); that is `doc/codex/issues/0048`'s defect, not
this one.

## Acceptance Criteria

1. **A second, read-only variable, not a promotion of `casemode`.** `casemode`
   must stay writable: it is the only selector, and `libngspice` has no argv
   (`sharedspice.h:93-97`). So the effective mode gets a distinct name, whose
   value is exactly one of `fold`, `preserve`, `distinguish` — the three
   spellings `set_case_mode()` accepts and the three the diagnostics print.
2. **It answers `inp_case_mode()` at the moment of the read**, not a cached
   copy taken at the latch. The enum can move under it at any subsequent
   `inp_readall()`, and `set_case_mode()` runs on every one of them — which is
   why finding 7 counts its warning two and three times
   (`doc/codex/issues/0058`, which owns that count and its call-site table).
   A `source` from inside a `.control` block is such a read; an `.include` and
   a `.lib` are **not**, because they recurse inside `inp_read()`
   (`inpcom.c:1741`, `:599`), one level below `inp_readall()` (`:1168`). 0058
   measured that distinction and this is a re-measurement of it, in `repro/`:

   ```
   $ NG -b -n -D casemode=preserve relatch_source.cir     # sources relatch_inner.cir
       In                  : voltage, real, 1 long [default scale]
       MidNode             : voltage, real, 1 long
       Vs#branch           : current, real, 1 long
       aa                  : voltage, real, 1 long [default scale]
       bb                  : voltage, real, 1 long
       vt#branch           : current, real, 1 long
   ```

   The top deck's nets keep their capitals; the sourced deck's `Aa`/`Bb` do
   not, because the `set casemode=fold` on the line before the `source`
   re-latched the enum. And the negative, counting `unknown casemode` under
   `-D casemode=bogus`, where one warning is emitted per `set_case_mode()`
   call:

   ```
   divider.cir       (neither)   : 2      <- spinit + the deck
   include_count.cir (.include)  : 2      <- unchanged, no extra latch
   ```

   So the read must consult the enum, not a copy: a `source` moves it and an
   `.include` does not, and no cache can be invalidated on the right one of
   those two without duplicating this distinction.
3. **Defined at every point the two can diverge**, each of which is measured
   above and each of which must answer the effect:
   - inside a `.control` block after the deck was read, ignoring any
     `set casemode=` on a preceding line of that block (`late_set.cir` →
     `fold`);
   - at the interactive prompt and in `ngspice -p`, where the latch ran during
     start-up and no deck is read at all — including the case where the user
     typed `set casemode=distinguish` at the prompt (→ `fold`) and the case
     where the flag latched `distinguish` and the user then typed
     `set casemode=fold` (→ `distinguish`);
   - after `ngSpice_Command("set casemode=…")` on a live shared-library
     session, before the next `ngSpice_Circ()`;
   - with no `casemode` variable set at all, where the effective mode is the
     static initialiser `NG_CASE_FOLD` (`inpcom.c:1034`) and the answer is
     `fold` — **not** `no such variable`. The variable's absence is itself one
     of the four divergence shapes and the read must not reproduce it.
4. **On a build without the feature the read must fail, not answer.** This is
   the property that makes it a capability probe and the reason it fixes the
   cross-binary lie: `ngspice-46` has no such variable, so the read is
   `Error: <name>: no such variable.`, which is distinguishable, where
   `preserve` was not. It also gives §9 of the guide a probe that sees
   `preserve`, which no identity-based probe can.
5. **Writing it is refused through the existing machinery.**
   `cp_usrset()` already has the arm: `plots` returns `US_READONLY`
   (`src/frontend/options.c:411`) and `cp_vset()` prints
   `Error: %s is a read-only variable.` (`src/frontend/variable.c:188`).
   Measured, `set plots=1` → `Error: plots is a read-only variable.` The new
   name takes the same path.
6. **It must not be shadowable by a loaded plot's environment.**
   `cp_enqvar()` (`options.c:52`) searches `plot_cur->pl_env` before anything
   else, `cp_getvar()` searches it as the third link of its fallback chain
   (`variable.c:703`), and `cp_usrset()` returns `US_READONLY` for any name
   found there (`options.c:414-417`). A rawfile can put an arbitrary key into
   that environment through its `Option:` line, so whatever name is chosen
   must either be refused as a plot-environment key or be resolved ahead of
   one. The mechanism is `doc/codex/issues/0061` and belongs there; what
   belongs here is the one measurement that says a read of this variable can
   be answered by a file. The rawfile is `repro/hdr_option.raw`, which
   `repro/run_all.sh`'s finding-1 block builds from `ascii_raw.raw` by
   splicing `Option: casemode=preserve` in after `Plotname:`; the session's
   effective mode is `fold`, the default. Full stdout, streams separated:

   ```
   $ printf 'echo before $casemode\nload hdr_option.raw\necho after $casemode\nquit\n' \
       | build-ver_50/src/ngspice -p -n >o 2>e

   $ cat o
   echo before $casemode
   before
   ngspice 10002 -> load hdr_option.raw
   Loading raw data file ("hdr_option.raw") ...
   done.
   Title:  * writes an ascii raw, so finding 1's header experiments can splice a line in
   Name: Operating Point
   Date: Thu Aug 13 00:16:41  2026

   Here are the vectors currently active:

   Title: * writes an ascii raw, so finding 1's header experiments can splice a line in
   Name: op1 (Operating Point)
   Date: Thu Aug 13 00:16:41  2026

       i(vs)               : current, real, 1 long
       v(in)               : voltage, real, 1 long [default scale]
   ngspice 10003 -> echo after $casemode
   after preserve
   ngspice 10004 -> quit
   ngspice-46+ done

   $ cat e
   Error: casemode: no such variable.
   ```

   `/usr/local/bin/ngspice` produces the same two streams byte for byte except
   for the closing banner, which reads `ngspice-46 done`.

   The same environment entry also makes the write fail, which is the other
   half of `cp_usrset()`'s behaviour and the reason the Summary's `US_OK` is
   qualified. Same rawfile, one command changed:

   ```
   $ printf 'load hdr_option.raw\nset casemode=fold\necho after $casemode\nquit\n' \
       | build-ver_50/src/ngspice -p -n 2>&1 | tail -6
   ngspice 10002 -> set casemode=fold
   Error: casemode is a read-only variable.
   ngspice 10003 -> echo after $casemode
   after preserve
   ngspice 10004 -> quit
   ngspice-46+ done
   ```

   (`2>&1` here because the point is the adjacency of the refusal and the
   answer; the `Error:` line is on stderr and the rest on stdout.) So after a
   `load`, `casemode` is simultaneously unwritable and wrong.

7. **A deck and a `.cmd` assert it.** The batch half is a `.cir` in
   `tests/regression/case/`, which can read a value with `if`/`echo` the way
   `harness-alive.cir` already does. The prompt half is a `.cmd` in
   `tests/regression/pipe/`, whose harness feeds `ngspice -p` on stdin
   (`tests/regression/pipe/Makefile.am:13-16`) and which asserts by exit code
   — `variable-keyword-case.cmd`'s `fail_count` / `quit 1` shape. That
   directory is the only place criterion 3's prompt cases are expressible at
   all, for the reason `doc/codex/issues/0048` gives: the reader folds a
   `.control` line before `com_set()` sees it.
8. **`make check` unchanged in all three modes.** Adding a variable moves
   nothing; the three `harness-alive.out` files in particular must not need
   editing — `tests/regression/case/`, `tests/regression/casedist/` and
   `tests/xspice/casedist/`. If the fix is taken as far as strengthening what
   any of those three decks asserts, which is what the Impact census argues
   for, that is a separate change with its own argument.

**Relation to the raw-header ask.** Finding 1 asks for the mode to be recorded
in the raw header, so it travels with the file. That is the same question for a
different consumer: finding 1 covers *files*, this covers *pipe and
interactive* use, and neither substitutes for the other — a live session has no
raw to read, and a raw read next year has no session. Finding 1's proposed
carrier — an `Option: casemode=<mode>` line, which existing readers already
parse where a new `Casemode:` key aborts them — is disqualified by
`doc/codex/issues/0061`, which has the mechanism and the argument. The
fragment of it that bears on *this* issue is criterion 6's measurement: the
carrier makes `$casemode` answer for a run other than the current one, so it
adds a divergence shape rather than removing one.

## Resolution

Closed 2026-08-13. The effective mode is now readable as **`curcasemode`**, a
second variable in the computed read-only family that already holds `curplot`,
`curplotname`, `curplottitle`, `curplotdate` and `plots`. `casemode` is
untouched and stays writable: it is still the only selector, and `libngspice`
still has no argv.

Three production edits, all in that family's existing machinery:

- `src/frontend/inpcom.c` — `inp_case_mode_name()`, beside `inp_case_mode()`
  and the `static int ng_case_mode` it reads, returning `"fold"`,
  `"preserve"` or `"distinguish"`. It lives in that file so the three
  spellings sit next to the three `cieq()` arms of `set_case_mode()` that
  accept them and cannot drift from them. Declared in
  `src/include/ngspice/fteext.h` beside the other four case predicates; it is
  the second function there that reports the mode itself rather than a derived
  yes/no.
- `src/frontend/options.c`, `cp_enqvar()` — an `eqc(word, "curcasemode")` arm
  returning `var_alloc_string(copy(word), copy(inp_case_mode_name()), NULL)`
  with `*tbfreed = 1`, placed **before** the `if (plot_cur)` block rather than
  inside it. Two consequences, both required: the read is answered in a
  session that has no current plot, and it is answered ahead of
  `plot_cur->pl_env`, which is criterion 6.
- `src/frontend/options.c`, `cp_usrset()` — an `eqc(var->va_name,
  "curcasemode")` arm returning `US_READONLY`, immediately after `plots`.

Nothing was added to the `.out` of any existing deck, no diagnostic was added
or moved, and `identity.baseline` is unchanged: both new comparisons have a
literal operand, which the lint never reports.

**Criterion by criterion.**

1. *A second, read-only variable.* `curcasemode`, whose value is
   `inp_case_mode_name()`'s and therefore exactly one of the three spellings
   `set_case_mode()` accepts. `casemode` is still an ordinary writable
   variable with no arm in `cp_usrset()`.
2. *Answers `inp_case_mode()` at the moment of the read.* Nothing is cached;
   `cp_enqvar()` calls the function per read. Asserted by
   `tests/regression/casedist/curcasemode-relatch.cir`, which sources a
   sub-deck it writes itself and watches the answer move `distinguish` →
   `preserve` across that `source`, then watches a following
   `set casemode=distinguish` with no read behind it *not* move it back. The
   `.include` half of the distinction stays where 0058 measured it, in
   `casemode-announce-report.cir`'s counts.
3. *Defined at every point the two can diverge.*
   - `.control` after the deck was read →
     `tests/regression/case/curcasemode-effect.cir`: entry `preserve`, a
     `set casemode=fold` on the next line, `$casemode` `fold` and
     `$curcasemode` still `preserve`.
   - prompt / `ngspice -p` → `tests/regression/pipe/curcasemode-effect.cmd`,
     checks 2 and 4, both directions. Measured by hand as well:

     ```
     $ printf 'set casemode=distinguish\necho req $casemode eff $curcasemode\nquit\n' \
         | build-ver_50/src/ngspice -p -n 2>/dev/null | grep '^req'
     req distinguish eff fold

     $ printf 'let ABC = 1\nset casemode=fold\necho req $casemode eff $curcasemode\nquit\n' \
         | build-ver_50/src/ngspice -p -n -D casemode=distinguish 2>/dev/null | grep '^req'
     req fold eff distinguish
     ```
   - `ngSpice_Command()` on a live shared-library session → same code path
     (`cp_enqvar()` is the shell's, not the binary's), documented in
     `src/include/ngspice/sharedspice.h` beside the latch semantics that were
     already there. **Not exercised**: no `libngspice` is built in
     `build-ver_50`. This is the one residual below.
   - no `casemode` variable at all → `fold`, not `no such variable`.
     `curcasemode-effect.cmd` check 1 asserts both halves, that `$?casemode`
     is 0 and that the read answers `fold`.
4. *On a build without the feature the read must fail.* It does, because the
   variable does not exist there and nothing else answers the name. Measured
   against `/usr/local/bin/ngspice` (`ngspice-46`), streams separated:

   ```
   $ printf 'echo v $curcasemode\nquit\n' | /usr/local/bin/ngspice -p -n >o 2>e
   $ cat o
   echo v $curcasemode
   v
   ngspice 10002 -> quit
   ngspice-46 done
   $ cat e
   Error: curcasemode: no such variable.
   ```

   against `v fold` and an empty stderr from `build-ver_50/src/ngspice` with
   no flag at all. This is the cross-binary lie closed: where `-D
   casemode=preserve` read back `preserve` from both binaries, `curcasemode`
   reads back `preserve` from one and errors on the other.
5. *Writing it is refused through the existing machinery.* `cp_usrset()`'s
   `US_READONLY` arm, so the message is `cp_vset()`'s
   (`src/frontend/variable.c:188`):

   ```
   $ printf 'set curcasemode=preserve\necho eff $curcasemode\nunset curcasemode\nquit\n' \
       | build-ver_50/src/ngspice -p -n 2>&1 >/dev/null
   Error: curcasemode is a read-only variable.
   Error: curcasemode is read-only.
   ```

   (the second is `cp_remvar()`'s arm for `unset`, which `plots` also gets).
   `curcasemode-effect.cir` asserts the message by capture, and
   `curcasemode-effect.cmd` check 5 asserts the consequence — that the write
   was not recorded, since `vareval()` searches the shell's own variable list
   ahead of `cp_enqvar()` and a recorded write would have answered the read.
6. *Not shadowable by a loaded plot's environment.* Satisfied by this issue's
   own change alone, by resolution order rather than by refusing the key:
   `cp_enqvar()` answers `curcasemode` before it reaches `plot_cur->pl_env`.
   `curcasemode-effect.cmd` check 6 writes a rawfile whose header carries
   `Option: casemode=preserve curcasemode=preserve`, loads it into a session
   whose latched mode is `distinguish`, and asserts both halves: `$casemode`
   answers `preserve` — the positive control, which is what stops the check
   passing for free by proving the environment entry really did land and
   really would have answered — and `$curcasemode` still answers
   `distinguish`. Re-measured on the criterion-6 rawfile shape in a default
   `fold` session: `AFTER-CASEMODE preserve`, `AFTER-CURCASEMODE fold`.
   `doc/codex/issues/0061` still owns the mechanism, and still has work to do:
   `casemode` itself remains shadowable and unwritable after a `load`, which
   is the other half of criterion 6's measurement and is untouched here.
7. *A deck and a `.cmd` assert it.* Three files, all new:
   `tests/regression/case/curcasemode-effect.cir` (+ `.out`),
   `tests/regression/casedist/curcasemode-relatch.cir` (+ `.out`),
   `tests/regression/pipe/curcasemode-effect.cmd`, the last asserting by exit
   code in `variable-keyword-case.cmd`'s fail-fast shape.
8. *`make check` unchanged in all three modes.* 303 passed, 0 failed, with
   every cached `*.log`/`*.trs` deleted first. The three `harness-alive.out`
   files were not edited, and no other reference file was.

**Left.**

- The shared-library route (criterion 3, third bullet) is documented and
  unexercised, for the reason the Impact section already gave: no
  `libngspice` is built in `build-ver_50`. It is the same `cp_enqvar()` call
  the prompt takes, so the risk is that the *build* differs, not the path.
- The Impact census's three weak liveness probes —
  `tests/regression/case/harness-alive.cir`,
  `tests/regression/casedist/harness-alive.cir`,
  `tests/xspice/casedist/harness-alive.cir` — still echo `$casemode` and are
  still satisfied by a binary that accepts the flag and ignores it. Criterion
  8 says strengthening them is a separate change with its own argument, and it
  was not made here. `curcasemode` is what such a change would use.
  `tests/regression/casedist/vector-pyplot-probe-report.cir:64`'s
  `strcmp cm "$casemode" distinguish` gate is likewise still correct by
  circumstance rather than by construction.
- Finding 1's raw-header ask is untouched and remains
  `doc/codex/issues/0061`'s: this closes the live-session half only. A raw
  read next year still carries no record of the mode that wrote it.
- `curcasemode` is matched with `eqc()` on both the read and the write, so
  `$CURCASEMODE` answers and `set CURCASEMODE=` is refused — deliberate, to
  match the `curplot`/`plots` family it joins. The asymmetry with
  `-D CaseMode=preserve` being a silent no-op on the *selector* is unchanged
  and is still `doc/codex/issues/0048`.
- **A rawfile may write `Option: curcasemode=…` and it will never be read
  back, while every other `Option:` key still answers.** That is criterion 6
  doing exactly what it was written to do — the whole point of the criterion is
  that a file cannot answer this name — but the file author is told nothing,
  so it is recorded here. Measured 2026-08-13, with `Option: curcasemode =
  distinguish` spliced in after `Plotname:` and the session in its default
  `fold`:

  ```
  $ printf 'load hdr_curcase.raw\necho read=$curcasemode\nset\nquit 0\n' | ngspice -p -n
  read=fold
  * curcasemode	fold
  $ printf 'load hdr_zzz.raw\necho read=$zzz\nquit 0\n' | ngspice -p -n
  read=HAHA
  ```

  The shape is worth stating precisely, because "silently ignored" undersells
  it in one direction and oversells it in another. The pair *is* filed: `set`
  lists it with the `*` that means plot environment, and `raw_write()`
  re-emits it, so nothing is lost on the way out. What is dropped is only the
  *value* on a read — `cp_vprint()` prints the name from `pl_env` but resolves
  the value through `vareval()`, which reaches `cp_enqvar()`'s computed arm
  first. Hence the third line above: the key is listed under the file's name
  with the session's answer.

  **A sentence and not a diagnostic, and the reason is measurable rather than
  a matter of taste.** The only place a warning could fire is the `Option:`
  arm as the file is read, and that arm is fed by files ngspice writes itself.
  The round trip is closed:

  ```
  $ printf 'set filetype=ascii\nload hdr_curcase.raw\nwrite rt.raw\nquit 0\n' | ngspice -p -n
  $ grep ^Option: rt.raw
  Option: curcasemode = distinguish
  $ printf 'set filetype=ascii\nload rt.raw\nwrite rt2.raw\nquit 0\n' | ngspice -p -n
  $ grep ^Option: rt2.raw
  Option: curcasemode = distinguish
  ```

  So a plain `load`; `write`; `load` — correct work, by a user who did nothing
  wrong and may not have written either file — reproduces the line and would
  fire the warning on every lap. Narrowing it to "warn only when the file's
  value differs from the mode in force" does not help: a file recorded under
  `distinguish` and re-read in a `fold` session differs by construction, and
  that is a record of another run rather than a mistake. This batch has twice
  shipped a diagnostic that fired on correct work; the demonstration here runs
  the other way from the one the sentence-or-warning question asks for, and
  shows the warning *would* misfire rather than that it could not.
- Documentation followed the fix, since a variable no one knows about fixes
  nothing: `doc/claude/casemode-distinguish-guide.md` §2 now leads with
  `echo $curcasemode` and keeps the three-way deck probe as the portable
  fallback, §9 gains the one-spawn client probe and its four-row measured
  table, and the guide's *"no pipe-mode probe can [see preserve]"* sentence is
  corrected — it was true only of identity-based probes.

## Addendum 2026-08-14 — the probe answers for its own cwd, and a client found the trap

Reported as **R5** of `doc/claude/feedback/reply_from_xschem_session/REPLY.md`
by the xschem session, which adopted `$curcasemode` in place of its whole probe
apparatus and then measured what it costs. Re-measured here against
`build-ver_50/src/ngspice` (build stamp `Fri Aug 14 20:52:09 UTC 2026`) from
their `repro2/`. Nothing below is a defect in `curcasemode` and nothing here
reopens the issue; it is the one usage constraint the Resolution did not state,
and it is stated now because the variable's whole value is that a client can
trust it.

**First, the property they are relying on, confirmed as intended and not
incidental.** They ask whether two things are by design. Both are.

1. *It reports the mode after `.spiceinit` has had its say* — the question a
   client actually has, "what will *this* run do?", rather than "what was
   requested". Yes, by construction: `cp_enqvar()` calls
   `inp_case_mode_name()`, which reads the `ng_case_mode` latch, and the latch
   is written by `set_case_mode()` from `inp_readall()` — after the `-D` getopt
   loop and after `.spiceinit` is sourced. That ordering is finding 5 of
   `doc/claude/feedback/ngspice_upstream/FINDINGS.md`, which is *deliberate*
   (`src/frontend/inpcom.c:1079`: the last writer before the deck is read
   wins). `curcasemode` reporting the effect rather than the request is the
   whole of criterion 2 and criterion 3; it is the reason the variable exists.
2. *Its absence on an older build is a clean negative* — empty stdout, an error
   on stderr, rc unchanged. Yes: criterion 4, measured against
   `/usr/local/bin/ngspice` there and re-measured in their round-2 script.
   There is no fallback answer, because nothing else in the tree answers the
   name.

**The constraint.** A `-p` probe has no deck, so it searches **cwd** for
`.spiceinit`, while the real run searches the **deck's** directory. Probe from
a different directory and it is confidently wrong. Their measurement, with a
`.spiceinit` holding `set casemode=fold` beside the deck and
`-D casemode=preserve` on both command lines, no `-n`:

```
cwd = probe/  (holds .spiceinit and deck.cir)
  probe says fold        the real run writes v(in), v(midnode)      agree
cwd = repro2/ (no .spiceinit), deck at probe/deck.cir
  probe says preserve    the real run writes v(in), v(midnode)      DISAGREE
```

So the rule the Resolution should have carried is not *"run the probe with the
real run's argv"* but *"run the probe with the real run's argv **and** its
cwd"*. They now `chdir` to the deck's directory before probing.
`doc/claude/casemode-distinguish-guide.md` §9 gains the table and the rule.

Two notes that came with it, both re-measured here:

- **The raw header catches the trap after the fact.** The disagreeing run above
  wrote `Option: casemode=fold` — the line `raw_write()` gained on 2026-08-14 —
  while the probe had said `preserve`. A consumer that reads the header turns a
  silent mislabelling into a detectable discrepancy. This is an argument for
  reading the header *even when you have probed*, and it is the best one this
  batch has produced.
- **`write` inside `.control` is cwd-relative, not deck-relative.** The
  disagreeing run's `deck.raw` landed in `repro2/` and not beside the deck.
  Same fix: run from the deck's directory.

`-n` removes the whole question at the price of the user's own `.spiceinit`,
which is finding 5's unchanged trade and not something this addendum moves.
