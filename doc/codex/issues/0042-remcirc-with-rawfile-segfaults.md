# 0042 — `remcirc` in a control block segfaults when a rawfile was requested

## Status

Open. Found while writing the decks for `doc/codex/issues/0028`
(`doc/claude/decisions/0008-undefined-node-diagnostic.md`), on branch
`ver_50`. Present at `a3edef17a`, before any of that work, so it is not caused
by it.

## Summary

A deck that removes its circuit with `remcirc` from inside a `.control` block
crashes at the end of a batch run if the run was given `-r`:

```spice
* remcirc with -r
.OPTIONS noacct
v1 in 0 dc 1
r1 in 0 1k
.control
op
print v(in)
remcirc
.endc
.end
```

```
$ ngspice -r out.raw --batch remcirc.cir
Segmentation fault (core dumped)     ; rc=139
$ ngspice --batch remcirc.cir
v(in) = 1.000000e+00                 ; rc=0
```

Measured on the binary built from `a3edef17a` and on the current tree; the
only difference between the two runs is the `-r` flag.

## Impact

`-r` is how `doc/claude/scripts/case_differential_sweep.py` runs every deck
and how `tests/xspice/digital` and `tests/xspice/case*` run theirs
(`check.sh "$(top_builddir)/src/ngspice -r foobaz"`), so a deck that uses
`remcirc` cannot be added to those directories and cannot be measured by the
sweep. It is also a crash rather than a diagnostic, which is the same class as
`doc/codex/issues/0021`.

The workaround, used by `tests/regression/misc/dotcard-undef-node-report.cir`,
is to source a circuit with no analysis in it instead of removing the current
one. That deck needs *some* way to stop batch mode from running the analyses
of the last circuit it sourced, and `remcirc` was the obvious one.

## Root Cause

Not diagnosed, and the two ends of it are:

- `com_remcirc()`, `src/frontend/runcoms2.c:191`, frees the current circuit;
- `src/main.c:1561`, the `rflag` branch of the batch epilogue - the second
  of the two, the one after the deck has been sourced - which
  calls `ft_dorun(ft_rawfile)` (`src/frontend/runcoms.c:398`) and then
  `ft_cktcoms(TRUE)` **after** the control block has run. Without `-r` neither
  is reached the same way, which is the whole of the difference between the
  two runs above.

So the crash is a run started on a circuit that no longer exists. The tree is
built without `-g`, so a symbolised stack needs a debug build, as
`doc/codex/issues/0041` also does.

## Acceptance Criteria

- The deck above completes with `-r` and writes whatever rawfile is correct
  for a session whose circuit has been removed — including an empty one, or
  none, as long as it is a decision rather than a crash.
- A regression deck exercises `remcirc` under `-r`. It cannot live in
  `tests/regression/misc`, whose harness passes no `-r`; `tests/xspice/digital`
  is the directory whose harness does.
- `doc/codex/issues/0021`'s question — whether this class of failure should be
  a diagnostic rather than a signal — is answered the same way for both.

## Resolution

None yet.
