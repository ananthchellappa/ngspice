# Receipt: baseline — `make check` on `ver_51` before any code item

The baseline that `PLAN.md`'s header and `receipts/00-scoping.md`'s *For the
owner* both say has not been taken. It comes before item 1's RED step.

Baseline crew, 2026-10-25, branch `ver_51`. HEAD at start **`a664faa2d`**
(`docs: plan the ver_51 Monte Carlo batch`).

**No code was touched.** Nothing under `src/`, `tests/`, `m4/` or any
`Makefile.am` was opened for editing; no `autogen.sh`, no `configure`. The only
file written in the repository is this receipt. `PLAN.md` and `LEDGER.md` were
read and not edited.

**The tree was not clean, and the baseline includes that.** `git status
--short` at start listed the owner's two uncommitted edits, `CLAUDE.md` and
`tests/regression/pipe/resource-keyword-case.cmd` (53 lines changed, +47/−14
across both). Neither was staged or modified. The second is a test the suite
runs, so the baseline count is for the working tree, not for `a664faa2d` alone:
`PASS: resource-keyword-case.cmd` is the owner's edited deck, and it passes.

Scratch: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad`
(`build.log`, `install.log`, `makecheck.log`, `makecheck.exit`).

## What changed

One file, this receipt. Nothing else.

## Build

```
$ cd build-ver_50 && ngheavy make -j4      # exit 0
$ ngheavy make install                     # exit 0
```

**Nothing was recompiled.** `build.log` contains no compiler or libtool line;
every directory reported *Nothing to be done*. That is expected:
`git diff --stat 54f3ac942..HEAD -- src/` is empty — the four commits since the
exit-status batch closed are three `test:` commits and the plan — and the
binary (mtime 2026-09-10) is newer than the last commit to touch `src/`
(2026-08-15). `make install` refreshed `stage/` from the same binary.

**Binary.** `build-ver_50/src/ngspice`, `ngspice-46+`, banner *Creation Date:
Fri Sep 11 03:44:45 UTC 2026*.

```
$ sha256sum build-ver_50/src/ngspice | cut -c1-20
2976d0243ee800c7f461
```

Identical before and after the `make`/`make install`. This is the binary item
1's RED step should run against; if its hash differs, something was rebuilt in
between.

## RED evidence

None. This item writes no test and changes no behaviour, so there is no RED
step. What it supplies is the green reference later REDs are measured against.

## `make check`

```
$ cd build-ver_50 && ngheavy make check > makecheck.log 2>&1; echo $?
0
$ for k in PASS FAIL XFAIL XPASS SKIP ERROR; do grep -c "^$k:" makecheck.log; done
```

| PASS | FAIL | XFAIL | XPASS | SKIP | ERROR |
| --- | --- | --- | --- | --- | --- |
| **335** | 0 | 0 | 0 | 0 | 0 automake results |

`make` exit status **0**. **No test fails at baseline**, so there is no failure
list.

The one line matching `^ERROR:` is not an automake result: it is
`ERROR: (internal)  tried to destroy non-existent graph`, simulator output from
inside `tests/regression/casedist/vector-probe-report.cir`, which PASSes. Receipt
06 of the exit-status batch recorded the same line as pre-existing. The other
`ERROR:` strings in the log are the pipe directory echoing its `.cmd` scripts'
`echo "ERROR: tstNNN failed"` text, not output from a failing check.

Per directory (PASS count from the `make[N]: Entering directory` that precedes
each result):

| dir | PASS | | dir | PASS |
| --- | --- | --- | --- | --- |
| `regression/case` | 118 | | `regression/parser` | 5 |
| `regression/misc` | 36 | | `regression/sens` | 4 |
| `regression/pipe` | 33 | | `regression/temper` | 4 |
| `regression/casedist` | 30 | | `regression/model` | 3 |
| `xspice/case` | 21 | | `regression/subckt-processing` | 2 |
| `xspice/casedist` | 18 | | `regression/func` | 1 |
| `regression/exitstatus` | 13 | | `regression/pz` | 1 |
| `regression/case-lt` | 8 | | `bsim3`, `bsim4`, `bsimsoi` | 2 each |
| `regression/case-pspice` | 8 | | `hisim`, `hisimhv1`, `hisimhv2` | 2 each |
| `xspice/digital` | 8 | | `hicum2` | 1 |
| `regression/lib-processing` | 7 | | `lint` | 2 |

They sum to 335. The exit-status batch closed at 331; the four added since are
in `regression/exitstatus/`, from `38717a031`, `0e45a27dd` and `ccebdf2a2`.
There is no `tests/regression/rng/` yet — items 2–4 create it.

## Lint

```
identity lint: 264 comparisons, baseline matches
identity lint: 11 comparisons, baseline matches
```

Both specs match. These are the same two counts the exit-status batch closed
on, which fits `src/` being unchanged since. `tests/lint/identity.baseline` was
not opened.

## What was falsified in the plan

Nothing in `PLAN.md` was contradicted. Two small things are now settled that
the plan left open:

- *"Baseline not yet measured"* (PLAN header, LEDGER header, 00-scoping *For
  the owner*) is now measured: **335 / 0 / 0 / 0 / 0, exit 0**, on binary
  `2976d0243ee800c7f461`. The plan said the first code crew would take it; a
  separate baseline crew took it instead, so item 1's crew can start from this
  number and need not re-run it before RED (it still must after its change).
- The plan's assumption that `build-ver_50/` serves `ver_51` unchanged holds:
  `ver_51` has no source delta from `ver_50`'s last `src/` commit, so no rebuild
  was needed and the binary is the one `ver_50` already had.

## For the owner

1. **The baseline includes your uncommitted `resource-keyword-case.cmd` edit**,
   and it passes. If you revert or rework that file, the 335 still holds only if
   the committed version also passes; that was not measured, because measuring
   it would mean touching your working copy.
2. **`LEDGER.md`'s *Baseline not yet measured* line is the driver's to update**;
   this crew did not edit it. The numbers to copy are in the table above.

Nothing was pushed. One commit on `ver_51`.
