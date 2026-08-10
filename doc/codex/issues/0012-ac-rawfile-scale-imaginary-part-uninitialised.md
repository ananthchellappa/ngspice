# Issue: The Imaginary Part of an AC Rawfile's Frequency Scale Is Uninitialised

## Status

Open

## Summary

A binary rawfile written from an AC analysis is not reproducible byte for byte.
Two runs of the same deck, same binary, same options, differ in exactly one
word per data point: the imaginary component of the scale vector, variable 0,
`frequency`.

```
$ ngspice -r z1.raw --batch tests/filters/lowpass.cir
$ ngspice -r z2.raw --batch tests/filters/lowpass.cir
stock vs stock ndiff = 31
  word 1  5.16461007909764e-310  vs  5.3092185026098e-310
  word 9  5.16461007909764e-310  vs  5.3092185026098e-310
  word 17 5.16461007909764e-310  vs  5.3092185026098e-310
```

The plot has 4 variables and is complex, so the stride is 8 doubles per point
and words 1, 9, 17, ... are the imaginary half of variable 0 at each point.
The magnitudes, around 5e-310, are subnormal: this is a stale heap pattern
being written out, not a computed value.

Reproduced on `tests/filters/lowpass.cir` and
`tests/resistance/res_partition.cir`; both are AC analyses.

## Impact

Small but real, and it is the kind of thing that wastes an afternoon:

- Any tool that compares two rawfiles byte for byte reports a spurious
  difference. That is how this was found: the Phase 2 differential sweep
  (`doc/claude/scripts/case_differential_sweep.py`) compares the numeric
  payload of a rawfile across case modes, and these two decks were its only
  two numeric hits until the sweep was made to run the stock deck twice and
  mask the words the two stock runs already disagree on.
- A consumer that reads the scale vector as a complex number gets a garbage
  imaginary part instead of zero. Every in-tree consumer takes the real part,
  which is why nothing has noticed.
- It is an uninitialised read, so it is also an ASan/valgrind finding waiting
  to happen, and its value can in principle be a signalling NaN.

`make check` cannot see it: the regression harness compares filtered stdout,
never rawfile bytes.

## Root Cause

Not diagnosed. The scale vector for an AC plot is allocated as complex because
the plot is complex, and only its real component is ever assigned; whichever
allocation path produces it does not zero the imaginary half. `src/frontend/
outitf.c` and `src/maths/ni/` are the places to start, and the fact that the
value is stable within a run but varies between runs points at a single
allocation reused per point rather than per-point garbage.

## Acceptance Criteria

1. Two runs of the same AC deck with `-r` produce byte-identical rawfiles.
2. The imaginary component of the frequency scale vector is exactly zero.
3. A regression test compares two runs of one AC deck byte for byte, since no
   existing harness looks at rawfile bytes at all.

## Resolution

Not fixed here. Found by the Phase 2 differential sweep, and unrelated to
case sensitivity: it reproduces with the case mode unset and with the sweep's
own uppercasing disabled. The sweep now masks it rather than reporting it, so
the workaround is in the tree even though the defect is not fixed —
see `masked_equal()` and `noise_mask()` in
`doc/claude/scripts/case_differential_sweep.py`.
