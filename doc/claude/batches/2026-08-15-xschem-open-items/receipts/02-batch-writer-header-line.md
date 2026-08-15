# Receipt 02 — the batch raw writer records the case mode

Item 2 of `doc/claude/batches/2026-08-15-xschem-open-items/PLAN.md`. Branch
`ver_50`, HEAD at start `25e891ec3`. `doc/codex/issues/0071`, the owner's
answer: the `-r` writer gets the line, behind the same gate, off by default.
`src/frontend/rawfile.c` was not opened.

## The RED failure, verbatim

`tests/regression/pipe/rawfile-casemode-batch.cmd` was written and run against
the unmodified binary. From
`make -C build-ver_50/tests/regression/pipe check TESTS=rawfile-casemode-batch.cmd`:

```
ERROR: the batch header line after Plotname: is <Flags: real> and not <Option: casemode=fold>
FAIL: rawfile-casemode-batch.cmd
===========================================================
1 of 1 test failed
```

It failed for the stated reason and no other: everything ahead of that line
passed in the same run — the harness preconditions (`casemode` and
`casemodewrite` both unset at session start), the `fold` baseline, and check 0b
in **both** formats, which is the assertion that with the gate unset line 5 of
the batch header is `Flags: real`. The deck got as far as the first loop
iteration, wrote a file with `casemodewrite` set, and found the old header.

## The production change

One arm in `fileInit()` (`src/frontend/outitf.c`), between the `Plotname:` line
and the `Flags:` line: `if (cp_getvar_policy("casemodewrite", CP_BOOL, NULL,
0))` then `sprintf(buf, "Option: casemode=%s\n", inp_case_mode_name())`,
`n += strlen(buf)`, `fputs(buf, run->fp)` — four lines of code in this
function's own idiom (into `buf`, counted into `n`, `fputs`) rather than a bare
`fprintf`, because `n` is the fallback seek position `fileEnd()` backfills the
point count into when `ftell()` is unusable, plus a comment above them saying
why the key, the place, the value source and the policy read are each what
`raw_write()` made them and pointing at `doc/codex/issues/0071`, `0070`, `0067`
and `0060`. Nothing else under `src/` changed: no header, no declaration, no
`Makefile.am`, no new string comparison, and `tests/lint/identity.baseline`
untouched. `raw_write()` was not touched. **A shared helper was weighed and
declined**: the two call sites agree on the format string and on nothing else —
one `fprintf`s to a `FILE *` it was handed and the other must accumulate into
`n`, and `raw_write()` carries a `!pl->pl_fromfile` provenance test this writer
has no counterpart for — so a helper would have needed three arguments to save
one line and would have put a reader one indirection away from the header being
read. The house rule prefers existing helpers, not new ones.

**Scope, decided and not inherited.** Covering `run <file>` as well as `-r` is
**intended**, and the commit message says so. They are one writer reached
through one route (`dosim()` opens `rawfileFp`, `ft_getOutReq()` is what
`OUTpBeginPlot()` asks); splitting them would have meant testing the flag
rather than the writer. The seven CIDER state dumps are unchanged, as 0071
assumed.

## The two `make check` counts

| what | result |
|---|---|
| `make -C build-ver_50/tests/regression/pipe check` | **All 33 tests passed** (32 before this item; `rawfile-casemode-batch.cmd` is the 33rd) |
| `make check` from `build-ver_50` | **322 PASS, 0 FAIL**, `rc=0` |

321 was item 1's count, so 322 is that plus this item's one new deck: no test
changed state and **no committed `.out` file moved**, which is itself the
evidence 0071's criterion 11 asked for — with the gate unset nothing any
existing deck writes changed by a byte, including
`tests/regression/pipe/postcoms-keyword-case.cmd`, the in-tree consumer that
counts header lines off a raw file. The full-suite log has 322 `PASS:` lines
and no `FAIL:`, `XFAIL:`, `XPASS:` or `SKIP:`; the one `^ERROR:` line is
`ERROR: (internal)  tried to destroy non-existent graph` inside the output of
`vector-probe-report.cir`, which passes, and is pre-existing. `tests/lint`
passes unchanged — `identity lint: 264 comparisons, baseline matches`, and
`selftest.lint`, 11 comparisons. The run includes the two directories whose
harness fixes a mode:
`tests/regression/case` (`preserve`) and `tests/regression/casedist`
(`distinguish`).

## Byte-identical when the gate is unset

Whole files, not headers, and against the released binary rather than against
ourselves. Same deck, default mode on both sides, `Date:` and `Command:` lines
removed (they carry a clock and a build stamp), everything else compared byte
for byte including the data section:

```
binary, gate unset: build-ver_50 vs /usr/local/bin/ngspice (ngspice-46)
    IDENTICAL bytes, 860 bytes
ascii,  gate unset: build-ver_50 vs /usr/local/bin/ngspice (ngspice-46)
    IDENTICAL bytes, 2194 bytes
```

And the other direction, same build and same deck with only `-D casemodewrite`
added, so the delta is attributable:

```
DIFFER: h_on.raw (2216 bytes) vs h_off.raw (2194 bytes)
  first difference at line 3:
    A: b'Option: casemode=fold'
    B: b'Flags: real'
```

22 bytes, exactly `len("Option: casemode=fold\n")`. One line appears; nothing
else in the file moves.

## The rest of what was measured

All against `build-ver_50/src/ngspice` (`ngspice-46+`, build stamp
`Sat Aug 15 18:18:34 UTC 2026`), scratch outside the repo, deck a divider with
nets `In`/`MidNode` and a multi-point `.tran` so a moved point count would be
visible.

**Three modes, the client's own command shape** (`-b -n -D casemode=M
-D casemodewrite -r out.raw`):

```
fold         line4=<Plotname: Transient Analysis> line5=<Option: casemode=fold>        line6=<Flags: real>
preserve     line4=<Plotname: Transient Analysis> line5=<Option: casemode=preserve>    line6=<Flags: real>
distinguish  line4=<Plotname: Transient Analysis> line5=<Option: casemode=distinguish> line6=<Flags: real>
```

**Both routes, one writer.** `-b -r` with `set casemodewrite` in the `.control`
block, and `run c_run.raw` from an interactive session with the same variable
set, each give `Plotname:` / `Option: casemode=preserve` / `Flags: real` as
header lines 4, 5 and 6.

**The stdout path, which the pipe harness cannot drive.** `ngspice -s` writes
its raw to stdout, where `fileEnd()` cannot seek and instead prints
`@@@ <pointPos> <count>` on stderr for an external consumer. That is the trap
the `n +=` guards, and it is measured by hand rather than by the deck:

```
gate unset: @@@ 180 21      true header length, 'Title:' to end of 'No. Points: ' = 180
gate set:   @@@ 206 21      true header length = 206  (180 + 26, the option line)
```

The fallback position is the true header length in both, and the point count is
unchanged. In a file the count is asserted by check 5 of the deck, which
compares it against the same run held in memory — `run <file>` takes this
writer, `run` with no argument takes `plotInit()`, so the two counts come from
different code.

**The file loads on the released binary.** A file this build wrote with the
line loads clean on `build-ver_50/src/ngspice` and on `/usr/local/bin/ngspice`
(`ngspice-46`) alike: no `strange line`, no `misplaced`, `display` shows
`v(In)` and `v(MidNode)`, `echo $casemode` answers `preserve` on both.

**A header cannot open the gate for this writer either.** An ASCII raw was
hand-edited to carry `Option: casemodewrite` after its `Plotname:` line. After
`load`, `$?casemodewrite` reads 1 — the pair is in the loaded plot's
environment — and the next `run f.raw` still writes `Flags: real` on line 5.
That is `cp_getvar_policy()` doing in `fileInit()` what it does in
`raw_write()`, and it is the measurement behind the claim both client documents
make.

## The deck

`tests/regression/pipe/rawfile-casemode-batch.cmd`, registered in that
directory's `TESTS` with `rcb_*.cir rcb_*.raw` added to `CLEANFILES`;
`./autogen.sh` re-run. It reaches the writer through `run <file>`, which is why
this directory can hold it at all — the harness runs `ngspice -p < deck` and
cannot pass `-r`. Six checks: the gate unset in both formats (0b), the option
line in all three modes in both formats (2 and 3), the value being the mode in
force rather than the `casemode` request (4, which moves the request without a
netlist read), the point count against an independent in-memory count (5), and
the gate closing again in the same session (6). Assertions are by position —
eight header lines are read under the open gate, so a duplicated or displaced
line fails on a position and not on a search.

**0071's optional criterion 8, a `tests/regression/casedist/` half, was not
taken.** It would reach the same writer by the same `run <file>` route and
differ only in where the mode came from (`-D` at start-up rather than
`set casemode` plus a `source`), which the pipe deck already covers for all
three modes; it was declined as duplicate coverage with a committed `.out` to
maintain, not because it could not be written. Recorded in 0071 against that
criterion.

## Documents corrected

Both client-facing documents stated the gap as a permanent caveat and were
re-measured, not just re-worded:

- `doc/claude/feedback/ngspice_upstream/RESPONSE.md` — the §2 caveat bullet now
  states that both writers carry the line, with the measured transcript and the
  note that this is `ver_50` and not a release; the "absence is not `fold`"
  bullet no longer names `-r`; the §2 preamble records the correction and the
  new build stamp; the client-program summary table's header row is corrected.
  The `-D casemodewrite=TRUE` trap (a string variable a boolean read cannot
  see) is stated for the client, since they will reach for the command-line
  form.
- `doc/claude/casemode-distinguish-guide.md` — the `casemodewrite` section
  gains a **Both writers** paragraph with the `-r` transcript and the
  control-block and command-line spellings; the "four caveats" paragraph no
  longer says the `-r` path is a different writer that does not carry it.

Every command quoted in either document was run in the exact form it is written
in and its output pasted from the run. **Neither document says anything about
the upstream submission having been sent, and neither was edited on that
subject.** Nothing under `doc/claude/upstream/` was touched.

`doc/codex/issues/0071` is updated: Status is Fixed with the owner's answer and
both riders, Resolution carries the shipped code, the RED failure, a
criterion-by-criterion account and what is left, and the Impact section's two
quotations are marked as the state at filing, since the documents they quote
have moved.

## What is left

- **The default is still off.** Both writers now flip together when
  `casemodewrite`'s default flips, which `0061` schedules for once `0067` has
  been in a release. That is the reason this was worth doing before the flip
  rather than after, and it is the note to re-read on that day.
- **The CIDER state dumps** still emit no `Option:` line, by decision. If the
  record is ever meant to be a property of the format rather than of a writer,
  that is its own issue.
- **The stdout (`-s`) path is measured but not asserted by a deck.** No harness
  in the tree can drive it: `tests/regression/pipe` compares no output but
  cannot pass `-s`, and `check.sh` directories compare a filtered stdout that a
  raw file dumped into it would fight. The numbers above are the record.
- Nothing sent anywhere. No push, no PR, no mail.
