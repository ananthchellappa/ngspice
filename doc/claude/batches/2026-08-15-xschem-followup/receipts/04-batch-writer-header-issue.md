# Receipt 04 — the `-r` batch writer's missing case-mode line, filed

Item 4 of `doc/claude/batches/2026-08-15-xschem-followup/PLAN.md`. Branch
`ver_50`, HEAD at start `4df3482c4`.

**Filing only. No fix, no code.** Nothing under `src/` or `tests/` was opened
for writing. One new file:

- `doc/codex/issues/0071-a-raw-file-written-through-r-records-no-case-mode.md`

Number: **0071**, the next free one (`0070` was the previous highest). Status is
Open/filed; Resolution is "not started" and ends in the question the owner has
to settle.

## What was measured

All against `/home/qflow/dev/ngspice_test/build-ver_50/src/ngspice`
(`ngspice-46+`, build stamp `Sat Aug 15 15:34:32 UTC 2026`), and where a
released baseline is named, `/usr/local/bin/ngspice` (`ngspice-46`). Scratch
directory `/tmp/claude-1000/-home-qflow-dev-ngspice-test/…/scratchpad/rhdr`.
Nothing was written into the repo but the issue and this receipt.

Every deck is the same divider, nets spelled `In` and `MidNode`, so the mode is
visible in the column names as well as in the header.

### 1. The header `-r` writes, gate set and gate unset

`.control` block does `set casemodewrite`; `-D casemode=preserve`:

```
Title: * divider, nets In / MidNode
Date: Sat Aug 15 10:20:11  2026
Command: ngspice-46+, Build Sat Aug 15 15:34:32 UTC 2026
Plotname: Operating Point
Flags: real
No. Variables: 3
No. Points: 1
Variables:
	0	v(In)	voltage
	1	v(MidNode)	voltage
	2	i(Vs)	current
```

Line 5 is `Flags:`. Zero `Option:` lines. The same deck with the gate removed
differs in the `Date:` line and in nothing else — a whole-file `diff` gives one
hunk, line 2.

Same result for `-D casemodewrite` given on the command line, ahead of the
deck, in `fold`, `preserve` and `distinguish`: `Flags: real` on line 5, zero
`Option:` lines, three modes, six files.

### 2. The two writers, same session, same plot, seconds apart

Deck's `.control`: `set casemodewrite`, `set filetype=ascii`, `run`,
`write w_both.raw`; session is `-b -r r_both.raw -D casemode=preserve`. The
gate reads `TRUE` in the block and the mode in force is `preserve`.

```
$ diff <(cat -A r_both.raw) <(cat -A w_both.raw)
4a5
> Option: casemode=preserve$
7c8
< No. Points: 1       $
---
> No. Points: 1$
13c14
< 0^I^I3.000000000000000e+00$
---
>  0^I3.000000000000000e+00$
15a17
> $
```

So the two writers agree on `Title:`, `Date:`, `Command:`, `Plotname:`,
`Flags:`, `No. Variables:` and every row of the `Variables:` table, spelling
included, and differ in exactly three things: the `Option:` line, the 8-space
reserved field on `No. Points:` (the field `fileEnd()` seeks back to), and the
value-row layout.

### 3. It is the writer, not the flag

`run <file>` from the control language reaches the same `fileInit()` and writes
the same header — line 4 `Plotname: Operating Point`, line 5 `Flags: real`, no
`-r` on the command line. Any fix in `fileInit()` therefore covers both routes;
that widening is named in the issue as a scope question rather than assumed.

### 4. ASCII and binary are the same header

The `-r` writer's first eight lines are byte-identical in the two formats;
only the closing marker differs (`Values:` / `Binary:`). The line is missing
from both, not lost on one branch.

### 5. `set casemodewrite` does reach the writer — timing is not the reason

This was the item's open technical question and the answer is **no, timing is
not why the arm is absent**:

- Ordering, from one run's own stdout: the control block's `echo` is output
  line 7, its `write` line 8, the `-r` file opens at line 9 and `fileInit()`
  prints `No. of Data Columns` at line 13. The block completes first.
- `main()` calls `ft_dorun(ft_rawfile)` at `src/main.c:1571`, after
  `inp_spsource()` has returned, so a variable the block set is live.
- Proven independently of the gate: `set filetype=ascii` in the same block
  *does* reach this run and changes the file, because `dosim()` reads
  `filetype` at `runcoms.c:240`. The `-r` file that deck writes is ASCII.

So `cp_getvar_policy("casemodewrite", …)` in `fileInit()` would be answered
exactly as it is in `raw_write()`. Nothing is missing but the arm. A fix is
about four lines and needs no header, declaration or build-file change —
`cp_getvar_policy()` is already visible via `variable.h` (`outitf.c:26`) and
`inp_case_mode_name()` via `ftedefs.h` (`:16`).

### 6. Every other raw-header writer in the tree

`grep -rn "Plotname:" src/` and `grep -rln "No. Variables" src/` agree on nine
files. Two are the writers above; the other seven are the CIDER family
(`ciderlib/oned/oneprint.c`, `ciderlib/twod/twoprint.c`, and the five device
dumps `nbjt`, `nbjt2`, `numd`, `numd2`, `numos`). None emits an `Option:` line,
and each writes a third grammar again — no `Date:` at all, `Flags:` before
`Command:`. Their columns are language-defined names (`x`, `psi`, `v13`,
`g22`), so a mode would describe only their `Title:`, which carries the
instance name. All seven are behind `#ifdef CIDER` (`dev.c:130`, `:197`), off
by default and off in `build-ver_50` (`config.status` shows a bare
`../configure`).

### 7. Two measurements for the owner's decision, one each way

- **The header is not frozen.** Upstream added the `Command:` line to this
  exact block on 2024-08-18 — `1087c6a0c`, Holger Vogt, "Add simulator version
  info to raw file in batch mode" — moving every line below it by one, and it
  shipped. `git log -L 944,972:src/frontend/outitf.c` is the whole history.
- **But it is stable in practice, and a line-counting consumer exists in this
  repository.** With the gate unset, this build's `-r` header is byte-identical
  to stock `ngspice-46`'s (`Date:`/`Command:` excluded), padding included. And
  `tests/regression/pipe/postcoms-keyword-case.cmd` counts header lines off an
  ASCII raw; it had to be edited when `raw_write()`'s line landed.

Also measured, so that criterion 6 is not a guess: the line spliced into a `-r`
file at the position a fix would use loads clean on **both** binaries —
`display` shows `v(MidNode)`, `echo $casemode` answers `preserve`, no
`strange line` and no `misplaced`.

### 8. Which test directory could hold the deck

Read `tests/bin/check.sh` and the `Makefile.am` of each candidate.

- **`tests/regression/pipe/` is the answer.** It runs `ngspice -p < deck.cmd`
  and compares no output, so it cannot pass `-r` — and does not need to,
  because `run <file>` from the control language is the identical writer (§3).
  The deck then reads the header back with `fopen`/`fread` and `quit 1`s, which
  is exactly what `rawfile-casemode-header.cmd` already does for `raw_write()`.
  Proven end-to-end on the current tree, which is the RED the issue quotes:

  ```
  BATCH-WRITER-LINE4=<Plotname: Operating Point>
  BATCH-WRITER-LINE5=<Flags: real>
  ```

- `tests/regression/casedist/` can hold a batch-harness half with a committed
  `.out` (`rawfile-casemode-header.cir` is the precedent), again reaching the
  writer through `run <file>`, since its flag string is per-directory.
- `tests/xspice/digital/` is the **only** directory whose harness already
  passes `-r` (`ngspice -r foobaz`), and it still cannot assert this: the file
  is written after the deck's control block has finished, and `check.sh`
  compares stdout only.

### 9. One correction to an earlier document

`doc/codex/issues/0061`'s first addendum names this writer `fileInit_pass1()`
at `outitf.c:945`. There is no `fileInit_pass1()` in the tree. It is
`fileInit()` at `:930`; `fileInit_pass2()` at `:1046` exists and writes the
variable table. The correction is recorded inside 0071's Root Cause rather than
by editing 0061, which is closed.

A measurement caveat worth carrying: `-D casemodewrite=TRUE` sets a **string**
variable, which a `CP_BOOL` read does not see, so that spelling opens the gate
for neither writer. The bare `-D casemodewrite` is the form that works. This is
`cp_getvar()`'s general rule for bool options, not anything about this key; it
is a footnote in 0071 and not a second finding.

## What the owner must decide

**Is the fix wanted at all?** 0071 states the question and deliberately does
not answer it. The evidence on both sides is §7 above: the block took a new
line from upstream two years ago and shipped, the gate is off by default so no
file changes unless a session asks, and the resulting file is measured to load
on `ngspice-46` — against a header that has otherwise been stable since spice3
and at least one in-tree consumer that counts its lines.

Two riders to answer in the same breath:

1. **Scope.** A fix in `fileInit()` covers `-r` and `run <file>` together,
   because they are one writer. Intended, or is `run <file>` to be left alone?
2. **The CIDER writers.** 0071 assumes the seven state dumps stay as they are.
   If the record is meant to be a property of the *format* rather than of a
   writer, that assumption is the thing to revisit — as its own issue.

If the answer is no, that is a legitimate close and the work becomes
documentation: the caveat in `RESPONSE.md` §2 and in
`doc/claude/casemode-distinguish-guide.md` stops being a "not yet" and becomes
a statement of intent.

## What was left

- No fix, by instruction. No RED deck was committed — §8's probe was run from
  the scratch directory and is reproduced in the issue's criterion 7 so
  whoever takes the fix starts from a measured failure rather than a guess.
- The severity should be re-read if `casemodewrite`'s default ever flips
  (`0061` schedules that for once `0067` has been in a release). From that day
  a `-r` file records nothing while a `write` of the same run records the mode,
  and the difference is invisible to whoever chose the flag.
- Nothing in `src/`, `tests/`, `RESPONSE.md` or the guide was touched.
