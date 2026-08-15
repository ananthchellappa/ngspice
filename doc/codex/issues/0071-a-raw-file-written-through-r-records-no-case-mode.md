# Issue: A Raw File Written Through `-r` Records No Case Mode

## Status

Open, filed 2026-08-15 on branch `ver_50`. Not started; see Resolution, which
carries the shape a fix would take and the one question the owner has to
settle before it is worth writing.

Not a defect in `9e341a8b7` ("feat: record the case mode in the raw header,
opt-in", `doc/codex/issues/0061`, first addendum) and not a regression from it.
It is that feature's coverage: ngspice has two writers for the raw format, the
feature was added to one of them, and the other has never carried it. The gap
was named in `0061`'s first addendum under **Left** the day the feature landed,
disclosed to the client as a caveat in
`doc/claude/feedback/ngspice_upstream/RESPONSE.md` §2 and in
`doc/claude/casemode-distinguish-guide.md`, and filed nowhere until now.

All measurements taken 2026-08-15 against `build-ver_50/src/ngspice`
(`ngspice-46+`, build stamp `Sat Aug 15 15:34:32 UTC 2026`), and where a
released baseline is named, against `/usr/local/bin/ngspice` (`ngspice-46`,
no `casemode` support). Every deck below is the same three-resistor divider
whose nets are spelled `In` and `MidNode`, so the mode is visible in the
column names as well as in the header.

## Summary

`raw_write()` (`src/frontend/rawfile.c:203`) writes the mode when the session
has asked for it:

```c
    if (!pl->pl_fromfile &&
            cp_getvar_policy("casemodewrite", CP_BOOL, NULL, 0))
        fprintf(fp, "Option: casemode=%s\n", inp_case_mode_name());
```

`fileInit()` (`src/frontend/outitf.c:930`) writes the header for a run whose
output is streamed to a file, and has no such arm — no `Option:` line of any
kind, conditional or otherwise:

```c
    n = 0;
    sprintf(buf, "Title: %s\n", run->name);           /* outitf.c:945 */
    ...
    sprintf(buf, "Plotname: %s\n", run->type);        /* :954 */
    n += strlen(buf);
    fputs(buf, run->fp);
    sprintf(buf, "Flags: %s\n", run->isComplex ? "complex" : "real");   /* :957 */
    ...
    sprintf(buf, "No. Points: ");                     /* :963 */
    ...
    fprintf(run->fp, "0       \n"); /* Save 8 spaces here. */           /* :970 */
    fprintf(run->fp, "Variables:\n");                 /* :972 */
```

`fileInit_pass2()` (`:1046`) writes the variable table and the
`Values:`/`Binary:` marker and adds nothing else. Between them that is the
whole of the header this path emits.

**Both writers in one session, one deck, one plot.** The deck's `.control`
block does `set casemodewrite`, `set filetype=ascii`, `run`, `write w.raw`,
and the session is `-b -r r.raw -D casemode=preserve`, so the same data is
written twice by the two writers seconds apart:

```
$ ngspice -b -r r_both.raw -D casemode=preserve both.cir
GATE-IN-CONTROL=TRUE MODE=preserve

$ sed -n '1,8p' r_both.raw          $ sed -n '1,8p' w_both.raw
Title: * a divider ...              Title: * a divider ...
Date: Sat Aug 15 10:19:45  2026     Date: Sat Aug 15 10:19:45  2026
Command: ngspice-46+, Build ...     Command: ngspice-46+, Build ...
Plotname: Operating Point           Plotname: Operating Point
Flags: real                         Option: casemode=preserve
No. Variables: 3                    Flags: real
No. Points: 1                       No. Variables: 3
Variables:                          No. Points: 1
```

**It is not the flag, it is the writer.** `run <file>` from the control
language reaches `fileInit()` by the same route — `dosim()`
(`src/frontend/runcoms.c:289`-`:312`) opens `rawfileFp`, and `ft_getOutReq()`
(`:421`) is what `OUTpBeginPlot()` asks — so a deck that never sees `-r` has
the same header:

```
$ ngspice -b -D casemode=preserve runcmd.cir      # .control: run runcmd.raw
$ sed -n '4,5p' runcmd.raw
Plotname: Operating Point
Flags: real
```

**All three modes, and the mode is plainly visible in the same file.** The
gate is set in the deck's `.control` block; column 1 is header line 10:

```
fold         line5=<Flags: real>  col1=<1  v(midnode)  voltage>  Option lines=0
preserve     line5=<Flags: real>  col1=<1  v(MidNode)  voltage>  Option lines=0
distinguish  line5=<Flags: real>  col1=<1  v(MidNode)  voltage>  Option lines=0
```

against the same three modes through `write`, same deck, same gate:

```
fold         line5=<Option: casemode=fold>
preserve     line5=<Option: casemode=preserve>
distinguish  line5=<Option: casemode=distinguish>
```

**The gate makes no difference to this path at all.** Two `-r` runs of the
same deck, one with `set casemodewrite` in the `.control` block and one with
nothing, differ in the wall clock and in nothing else:

```
$ diff a_on.raw a_off.raw
2c2
< Date: Sat Aug 15 10:20:11  2026
---
> Date: Sat Aug 15 10:20:24  2026
```

and the same for `-D casemodewrite` given on the command line, ahead of
everything: `Flags: real` on line 5, zero `Option:` lines, in `fold`,
`preserve` and `distinguish` alike. (`-D casemodewrite=TRUE` sets a *string*
variable, which a `CP_BOOL` read does not see, so that spelling opens the gate
for neither writer — measured, and it is `cp_getvar()`'s general rule for bool
options rather than anything about this key. The bare `-D casemodewrite` is
the form that opens it, and it is the form used throughout this issue.)

**ASCII and binary are the same header.** The `-r` writer's first eight lines
are byte-identical in the two formats; only the closing marker differs
(`Values:` against `Binary:`). So this is not a format that lost the line on
one branch — the line is in neither.

**What the two writers do agree on.** Given the same plot, the two headers
differ in exactly three places, and only the first is this issue's subject:

| | `fileInit()` (`-r`, `run <file>`) | `raw_write()` (`write`) |
| --- | --- | --- |
| `Option: casemode=` | absent, always | present when `casemodewrite` is set |
| `No. Points:` | `1` padded to an 8-character field, `No. Points: 1␣␣␣␣␣␣␣` | `No. Points: 1`, unpadded |
| value rows | `0\t\t3.000…e+00` | `␣0\t3.000…e+00`, and a trailing blank line |

`Title:`, `Date:`, `Command:`, `Plotname:`, `Flags:`, `No. Variables:` and
every row of the `Variables:` table are identical, spelling included. The
padding on `No. Points:` is not cosmetic and is the shape of the whole
problem: it is the reserved field that `fileEnd()` seeks back to and overwrites
(`run->pointPos`, set at `outitf.c:968`, used at `:1148`), because this writer
emits its header before the analysis has produced a single point.

**Every other writer of a raw header in the tree, checked.**
`grep -rn "Plotname:" src/` and `grep -rln "No. Variables" src/` agree on nine
files. Two are the writers above. The other seven are the CIDER family —
`src/ciderlib/oned/oneprint.c:122`, `src/ciderlib/twod/twoprint.c:133`, and
the five device state dumps `nbjt/nbjtdump.c:117`, `nbjt2/nbt2dump.c:118`,
`numd/numddump.c:117`, `numd2/nud2dump.c:118`, `numos/nummdump.c:118`. None of
them emits an `Option:` line either, and each is a third grammar again: no
`Date:` line at all, `Plotname:` above `Command:` rather than below it, and a
fixed set of `Command: deftype` lines whose position differs even between the
two groups — the ciderlib printers put them after `Flags:`, the device dumps
before it. Their columns are language-defined
names (`x`, `psi`, `phin`, `v13`, `g22`), not names a deck wrote, so a case
mode describes only their `Title:`, which carries the instance name. They are
behind `#ifdef CIDER` (`src/spicelib/devices/dev.c:130`, `:197`), which is
off by default and off in `build-ver_50`. They are named here so that a future
reader does not have to re-run the grep, not because this issue asks for them
to change.

## Impact

**The client this feature exists for uses `-r`.** The xschem integrator
(`doc/claude/feedback/reply_from_xschem_session/REPLY.md`) reproduces with
`ngspice -b -n -D casemode=distinguish -r out.raw deck.cir` — that command line
is both of `RESPONSE.md` §1's reproducers (`:43`, `:49`) and a row of §6's
deck-shape table (`:476`). Their stated rule for a header with no `casemode`
line is the right one and is ours: *absence is not `fold`; treat it as unknown
and fall back to the probe.* So the consequence
here is not a wrong answer. It is that **a whole deck shape silently gets the
fallback**, for ever, however loudly the deck asks for the line.

That matters because the probe is what the feature was built to retire. The
fallback is a throwaway simulation — `doc/claude/casemode-distinguish-guide.md`
spells out the probe deck — spawned to learn something the writing session
already knew and had been asked to record.

**And the obvious workaround is not additive.** Under `-r` there is no
in-memory plot to write: `OUTpBeginPlot()` calls `fileInit()` *instead of*
`plotInit()` (`src/frontend/outitf.c:494`-`:497`). A `.control` block cannot
supply the missing line for the same data, because in a `-b -r` run the
control block finishes before the analysis begins. Measured, with the run's
own stdout line numbers:

```
7:CONTROL-BLOCK-RUNS-HERE                     <- the .control block, in full
8:ASCII raw file "early.raw"                  <- its 'write', before any analysis
9:ASCII raw file "r_order.raw"                <- the -r file opens
13:No. of Data Columns : 3                    <- fileInit() prints this

$ sed -n '4,5p' early.raw
Plotname: constants                           <- and so it wrote the constants
Option: casemode=preserve                        plot (doc/codex/issues/0059)
```

To get both the `-r` file and a `write` of the same data, a deck has to add its
own `run` to the control block, at which point the circuit is simulated twice —
that is the two-simulation deck shape `RESPONSE.md` §6 already tells this
client to stop writing, and it is a poor thing to recommend back to them to fix
a header line.

**Bounded, and worth saying how.** `casemodewrite` is off by default, so today
no file loses a line it would otherwise have had unless a user asked for it;
and the client's generated decks name their rawfile from `.control write`,
which is the writer that carries it. This is a caveat that costs the client a
probe, not a defect that gives anybody a wrong mode. It is disclosed in both
client documents:

- `RESPONSE.md` §2, in the caveat list: *"The `-r` batch path does not carry
  it. `ngspice -r out.raw deck.cir` is a different writer and writes no
  `Option:` line."*
- `doc/claude/casemode-distinguish-guide.md`, in the `casemodewrite` section:
  a file written by the `-r` batch path has no such line, *"which is a
  different writer"*.

Neither document has an issue to point at, which is what this file fixes. The
severity should be re-read if the default ever flips — `0061` schedules that
for once `0067` has been in a release — because from that day a `-r` file is a
file that recorded nothing while a `write` of the same run recorded the mode,
and the difference will be invisible to whoever chose the flag.

**Class**, for the taxonomy in `doc/claude/decisions/0001-distinguish.md`: none
of the three. No identifier is compared here. It is a coverage gap in a
provenance record.

## Root Cause

Two writers, one file format, and no shared code between them.

`raw_write()` serialises a `struct plot` that already exists: the analysis has
finished, the vectors are in memory, and their number and length are known, so
the function can write any header line it likes in any order. `fileInit()`
streams: it is called from `OUTpBeginPlot()` before the first data point
exists, writes the header immediately, reserves eight characters for a
`No. Points:` count it does not yet know, and `fileEnd()` seeks back to
`run->pointPos` to fill it in. The two share their grammar and nothing else —
not a helper, not a constant, not a table of header lines. A line added to one
does not appear in the other and nothing in the tree notices.

So when `9e341a8b7` implemented the ask, it implemented it where the plot was.
That is why the gap exists; it is not why it persists. It persists because it
was recorded as a bullet under **Left** in `0061`'s first addendum and never
turned into an issue with acceptance criteria.

**Timing is not the reason, and this is worth stating because it looks like it
should be.** A `.control` block in a `-b -r` run executes during
`inp_spsource()`, and `main()` calls `ft_dorun(ft_rawfile)` only afterwards
(`src/main.c:1561`-`:1576`), so a variable the block set is live when
`fileInit()` runs. Proven independently of the gate: `set filetype=ascii` in
the same `.control` block *does* reach this run and does change the file, since
`dosim()` reads `filetype` at `runcoms.c:240`. The `-r` file that deck writes
is ASCII. `cp_getvar_policy("casemodewrite", ...)` in `fileInit()` would be
answered exactly as it is in `raw_write()`. Nothing is missing but the arm.

One correction, so that a fix is looked for in the right place. `0061`'s first
addendum names this writer `fileInit_pass1()` at `src/frontend/outitf.c:945`.
There is no `fileInit_pass1()` in the tree. The function is `fileInit()` at
`:930`; `fileInit_pass2()` at `:1046` exists and writes the variable table.

## Acceptance Criteria

1. A raw file written by the batch writer with `casemodewrite` set carries
   `Option: casemode=<mode>` immediately after its `Plotname:` line — the same
   key, the same closed-up spelling and the same position as `raw_write()`'s —
   in `fold`, `preserve` and `distinguish`, and in both the ASCII and the
   binary format.
2. The value is the mode in force, `inp_case_mode_name()`
   (`src/include/ngspice/fteext.h:224`), the same source `raw_write()` uses and
   the one `curcasemode` answers, not the `casemode` request
   (`doc/codex/issues/0060`).
3. Both routes to that writer are covered, because they are one writer: the
   `-r` command-line flag and `run <file>` from the control language. A fix
   placed in `fileInit()` gets both for free; a fix placed anywhere else must
   show it does.
4. With `casemodewrite` unset the batch header is byte-identical to the one
   this build writes today, which is byte-identical to `ngspice-46`'s. Measured
   baseline, this build under the default mode against stock `ngspice-46`, both
   `-r`, `Date:` and `Command:` excluded: identical, `No. Points: 1␣␣␣␣␣␣␣`
   padding included.
5. `No. Points:` is still correct after the change, in a file and on stdout.
   This is the trap: `fileInit()` accumulates `n` as it writes and uses it as
   the fallback `pointPos` when `ftell()` is unusable — `run->fp == stdout ||
   (run->pointPos = ftell(run->fp)) <= 0` at `outitf.c:968` — so a new line
   written without adding its length to `n` leaves the backfill pointing into
   the middle of the header on the stdout path. Asserted with a multi-point
   analysis, not an `.op`, so a wrong count is visible.
6. The file still loads without a diagnostic, on this build and on stock
   `ngspice-46`, and the mode reads back. Already true of the byte sequence a
   fix would produce: the line spliced into a `-r` file at the position
   criterion 1 asks for loads clean on both binaries, `display` shows
   `v(MidNode)`, and `echo $casemode` answers `preserve` on both. So the
   criterion is about the fix not disturbing the rest of the header, not about
   the format.
7. **A deck asserts it, in `tests/regression/pipe/`.** That harness runs
   `ngspice -p < deck.cmd` and compares no output, so it cannot pass `-r`; it
   does not need to. `run <file>` from the control language goes through the
   identical writer (measured above), and the deck can then read the header
   back with `fopen`/`fread` and `quit 1` on a mismatch — the idiom
   `rawfile-casemode-header.cmd` already uses for `raw_write()`. Measured
   end-to-end on the current tree, which is this criterion's RED:

   ```
   BATCH-WRITER-LINE4=<Plotname: Operating Point>
   BATCH-WRITER-LINE5=<Flags: real>
   ```

   The gate must be asserted in both directions in the same session, as
   `rawfile-casemode-header.cmd` checks 0b and 6 do, so that criterion 4 fails
   loudly rather than passing on start-up state.
8. Optionally a batch-harness half in `tests/regression/casedist/`, whose
   `TESTS_ENVIRONMENT` runs `check.sh` with `-D casemode=distinguish` and
   compares a committed `.out`; there too the deck reaches the writer through
   `run <file>` rather than through `-r`, since the flag string is
   per-directory. `rawfile-casemode-header.cir` is the precedent and the place
   to put the third mode. Note for whoever writes it: `tests/xspice/digital/`
   is the one directory whose harness already passes `-r`
   (`ngspice -r foobaz`), and it still cannot assert this, because the file is
   written after the deck's control block has finished and `check.sh` compares
   only stdout.
9. No new `strcmp`-family comparison of a deck-written name without the
   annotation `tests/bin/identity_lint.sh` requires; `identity.baseline`
   unchanged. A fix of the expected shape adds no string comparison at all.
10. RED first: criterion 7's deck fails against the current tree with its
    failure quoted in the commit message, before the production change.
11. `make check` unchanged in all three modes. No committed `.out` file should
    move — but see the open question below, because whether one *does* move is
    itself evidence.

## Resolution

Not started.

**The shape a fix would take.** One conditional `fprintf` in `fileInit()`
(`src/frontend/outitf.c`), between the `Plotname:` line at `:954` and the
`Flags:` line at `:957`, reading the gate through `cp_getvar_policy()` and the
value through `inp_case_mode_name()` — the same two calls `raw_write()` makes,
and both are already visible in this file: `cp_getvar_policy()` is declared in
`src/frontend/variable.h`, which `outitf.c` includes at `:26`, and
`inp_case_mode_name()` in `src/include/ngspice/fteext.h`, which it reaches
through `ngspice/ftedefs.h` at `:16`. It must be written in this function's own
idiom rather than as a bare `fprintf`: `sprintf` into `buf`, `n += strlen(buf)`,
`fputs`, so criterion 5's `pointPos` fallback stays correct. That is on the
order of four lines, and no header, build file or declaration changes.

Nothing about the format needs deciding. The key, the position and the spelling
are all fixed by `raw_write()`'s existing line and by the reader's `Option:`
arm, which needs a current plot and so cannot take a line above `Plotname:`
(`src/frontend/rawfile.c:637`, `Error: misplaced Option: line`).

**The question the owner has to settle first, and this issue does not answer
it: is the fix wanted at all?**

The argument against is that this header is the oldest stable thing in the
format. Unlike `raw_write()`'s, it is written by every batch run of every
ngspice back to spice3, and a consumer that reads a `-r` file by counting lines
is reading a header that has been the same for its entire working life. This
tree contains exactly such a consumer, for the other writer:
`tests/regression/pipe/postcoms-keyword-case.cmd` reads a fixed number of
lines off an ASCII raw and asserts the next one is `Values:`, and it had to be
edited when `raw_write()`'s line landed. If a committed deck in this repository
counts header lines, third-party tooling does too.

The argument for is that the header is demonstrably not frozen and the
compatibility cost is measurable rather than hypothetical. Upstream added the
`Command:` line to this exact block on 2024-08-18 (`1087c6a0c`, Holger Vogt,
"Add simulator version info to raw file in batch mode"), moving every line
below it by one, and it shipped. The line a fix would add is behind a gate that
is off by default, so a file only changes when the session asks — with the gate
unset the batch header stays byte-identical to `ngspice-46`'s, which is
criterion 4 and is already measured. And the file the fix would produce is
known to load on the released binary, measured under criterion 6.

Both halves of that are measurements in this issue. Which of them decides it is
a judgement about consumers we cannot see, and it belongs to the owner.

Two smaller questions ride along with it and should be answered in the same
breath:

- **Scope.** A fix in `fileInit()` covers `-r` and `run <file>` together
  because they are one writer. Is that intended, or is the `run <file>` route
  meant to be left alone? Splitting them would mean testing the flag rather
  than the writer, which is a worse fix; it is named only so the widening is a
  decision and not a surprise.
- **The CIDER writers.** The seven state-dump writers listed in Summary emit no
  `Option:` line and never have. Their columns are language-defined names, so
  the mode describes only their `Title:`, and they are off by default. This
  issue assumes they stay as they are. If the owner wants the record to be a
  property of the format rather than of a writer, that assumption is the thing
  to revisit, and it should be its own issue rather than a rider on this one.

If the answer is that the fix is not wanted, that is a legitimate close and the
work is documentation: the caveat in `RESPONSE.md` §2 and in
`doc/claude/casemode-distinguish-guide.md` stops being a "not yet" and becomes
a statement of intent, and this issue records why.
