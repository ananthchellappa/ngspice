# Receipt 10b: the meas integration rule, `when` on an AC plot, margin bound

Fork-written, on `ver_52`, all heavy commands through `ngheavy`, `-j4`, no exit 137.
Replaces R6's rule from receipt 10 (`ef6445218`), keeping the rest of that commit.

| | commit | test in `make check` |
|---|---|---|
| integration rule | `875ddd529` | `meas-window-exact.cir`, `meas-sp-window.cir`, both reworked |
| `when` guard | `9b87d50bc` | `meas-when-on-ac.cir` |
| margin bound | `1a0955462` | `meas-margin-length.cir` |

## Gates

- `make` 0, `make install` 0, installed binary `cmp`-equal to the built one.
- `make -k check`: **394 PASS, 1 FAIL, 0 SKIP**. The FAIL is the named `resource-keyword-case.cmd`.
  392 after group 11 + 2 new decks. `identity.lint` passes, baseline untouched. The two `ERROR:` lines
  in the log are the ones receipt 11 explains.
- No test from before this batch changed: this job's only changed references are `meas-window-exact`
  and `meas-sp-window`, both new in this batch, plus the two new decks.
- `measure/after-10b/`, below. `mc_r`, `mc_sky130` identical to 45.2: yes. Draw logs unchanged.

## 1. The rule

integ and rms: the samples inside the window go through the rule from before `ef6445218`, Simpson 3/8,
else 1/3, else trapezoid, of `y` or `y*y`, from the low end of the scale. Each edge is interpolated, and
the partial step from it to the nearest sample is added with the trapezoid. avg is integ over the width.

Kept from `ef6445218`: a window inside one step, descending sweeps (a sweep down now gives the same
number as up), `from=0` and `to=0`, the length check, `u05`'s read of the scale.

One choice beyond the brief: a sample within 100 ulps outside an edge counts as on the edge. On the
inverter's dc sweep the old code behaved as if `from=0.3` lay just above the sweep's `0.3` and skipped
that sample: the same defect, by rounding. That is inferred from its value (below), not read from the parse.

### Smooth signals, `g10-meas-smooth-analytic.cir`

Error relative to the closed form, `NGSPICE_MEAS_PRECISION=12`. Receipt 10's table was scored from 6
printed digits, so its small entries carry up to 5e-6 of rounding: `6.21e-5` is `6.38e-5` here. The RC
closed form now includes the 1 ns ramp. "ver_52 before" is the binary before `ef6445218`, "`ef6445218`"
the rule it replaced. ver_51 equals ver_50 on all rows.

| signal | metric | ver_50 | ver_52 before | `ef6445218` | **new** | new vs before |
|---|---|---|---|---|---|---|
| dc `sin x`, step 0.1 | avg | 2.34e-2 | 8.33e-4 | 8.33e-4 | 2.71e-5 | = integ |
| | integ | 6.38e-5 | 6.38e-5 | 8.33e-4 | 2.71e-5 | nearer |
| | rms | 1.46e-4 | 1.46e-4 | 8.33e-4 | 4.83e-5 | nearer |
| tran sine, 100 a period | avg | 5.20e-2 | 2.58e-4 | 3.28e-4 | 1.61e-5 | = integ |
| | integ | 1.14e-4 | 1.14e-4 | 3.28e-4 | 1.61e-5 | nearer |
| | rms | 1.64e-5 | 1.64e-5 | 3.29e-4 | 2.89e-6 | nearer |
| | rms, whole periods | 1.11e-5 | 1.11e-5 | 3.29e-4 | 7.62e-7 | nearer |
| tran sine, 20 a period | avg | 2.06e-1 | 3.94e-3 | 8.24e-3 | 1.65e-3 | = integ |
| | integ | 8.51e-3 | 8.51e-3 | 8.24e-3 | 1.65e-3 | nearer |
| | rms | 6.27e-4 | 6.27e-4 | 8.18e-3 | 1.62e-4 | nearer |
| | rms, whole periods | 9.18e-4 | 9.18e-4 | 8.19e-3 | 1.14e-4 | nearer |
| tran RC, adaptive | avg | 1.56e-3 | 1.88e-6 | 2.35e-6 | 8.10e-6 | = integ |
| | integ | 7.51e-6 | 7.51e-6 | 2.35e-6 | **8.10e-6** | **further, by 5.9e-7** |
| | rms | 7.38e-6 | 7.38e-6 | 2.73e-6 | 7.30e-6 | nearer |
| ac lin, RC magnitude | avg | 5.41e-3 | 4.75e-4 | 4.26e-4 | 3.31e-6 | = integ |
| | integ | 8.57e-5 | 8.57e-5 | 4.26e-4 | 3.31e-6 | nearer |
| | rms | 3.14e-4 | 3.14e-4 | 4.76e-4 | 1.01e-5 | nearer |
| ac dec, RC magnitude | avg | 5.71e-2 | 1.27e-3 | 1.27e-3 | 1.27e-3 | = integ |
| | integ | 1.27e-3 | 1.27e-3 | 1.27e-3 | 1.27e-3 | nearer, 1.2710e-3 from 1.2721e-3 |
| | rms | 1.60e-3 | 1.60e-3 | 1.01e-3 | 1.59e-3 | nearer, 1.5946e-3 from 1.5993e-3 |

**The margin, 1e-6, used once: RC integ.** That row is set by the waveform, not by the rule. On one run
of that source, simulated `v(4)` differs from the closed form by up to 2.47e-5 at the samples in the
window. Scored by a Python copy of each rule on those samples, in a deck with only this source, so not
the same samples as the table:

| data at the run's samples | old rule, integ, rms | new rule, integ, rms |
|---|---|---|
| simulated | 7.94e-6, 7.32e-6 | 8.18e-6, 7.31e-6 |
| closed form | 2.63e-7, 3.13e-9 | 1.81e-8, 1.23e-8 |

On the true curve the new integ is nearer. The old rule's error partly cancels the simulator's by
accident. `avg` does the same in this row: 1.88e-6 before, against 7.51e-6 for its own integ.

### `u10`

| | ver_50 | ver_51 | ver_52 before | `ef6445218` | new |
|---|---|---|---|---|---|
| dc avg, ac avg | 0.5, 0.5 | 0.5, 0.5 | 0.4375, 0.1875 | 0.4375, 0.4375 | **0.4375, 0.4375** |
| dc integ, ac integ | 0.075, 0.075 | 0.075, 0.075 | 0.075, 0.075 | 0.175, 0.175 | **0.175, 0.175** |

The first sample is used, and this deck gets 0.4375 and 0.175: only one step lies inside its window, so
no Simpson. **Simpson does change the answer elsewhere on the same data:** window 1.2 to 1.6 is two equal
steps across the kink and gives integ 0.2667 and avg 0.6667, where the line has 0.2 and 0.5, and whole
sweep integ 0.225 where the line has 0.2. Integ is what ver_50 gave there. **avg is new:** ver_50's avg was
a trapezoid, 0.5, and is now tied to integ, as this job asked. Not special-cased.

### References changed

`meas-window-exact.cir`, values by hand from the rule, table in its header. Each check is 2e-6.

| window on y, kink at 1.4 | avg, old to new | integ, old to new | rms, old to new | why |
|---|---|---|---|---|
| 1.3..1.7 | 0.4375 | 0.175 | 0.5590 to `sqrt(0.40625)` 0.6374 | head of `y*y` is a trapezoid, 0.0625 |
| 1.2..1.6 | 0.5 to 2/3 | 0.2 to 0.8/3 | 0.5774 to `sqrt(2/3)` | Simpson 1/3 across the kink |
| 1.42..1.58 | 0.5 | 0.08 | 0.5508 to `sqrt(0.41)` | one step: trapezoid of `y*y` |
| 1.1..1.9 | 0.25 to 0.28125 | 0.2 to 0.225 | 0.4082 to `sqrt(0.28125)` | 3/8 on 3 steps |
| 1.3..end | 0.25 to 0.15/0.7 | 0.175 to 0.15 | 0.4226 to `sqrt(0.1375/0.7)` | head + 3/8 |
| start..1.7 | 0.2857 to 0.225/0.7 | 0.2 to 0.225 | 0.4364 to `sqrt(0.225/0.7)` | 3/8 |
| all | 0.2 to 0.225 | 0.2 to 0.225 | 0.3651 to `sqrt(0.225)` | 3/8 then 1/3 |

- dc down: same values as up. ac: scaled by 1e9, and by the part of `z`. Through zero: `zr1`
  `sqrt(5/3)` to `sqrt(1.625/0.75)`, `zr2` `sqrt(1/3)` to `sqrt(0.5)`; avg and integ unchanged.
- tran, raw samples: avg and integ unchanged, since no equal steps reach across a corner. rms depends
  on the adaptive samples, so the check is now from the line's value to 0.5% above it: **looser than
  2e-6, on purpose**. A Python copy of the rule on the dumped samples gives tr1 5.99943e-1, tr4
  5.14098e-1, tr5 4.53514e-1, tr6 3.43221e-1, tr7 4.97010e-1, all equal to the `.out`.
- New: tran on equal steps (`linearize`), by hand to 2e-6; a window with one sample in it (0.75, 0.15,
  `sqrt(0.625)`); a cubic on equal steps, where the rule is exact on any window on samples.

`meas-sp-window`:

| line | old | new | reason |
|---|---|---|---|
| `sprmsc`, `sprmsr` | 2.81839e9 | 2.81850e9 | `sqrt(22.243/2.8)`, worked in the deck. 2.81839e9 is the curve's, 2.81869e9 was pre-`ef6445218`: nearer than that |
| `s11ic`, `s11ir` | 1.41298e9 | 1.41299e9 | sp lin 4001 gives 1.412990e9 |
| `s11rc`, `s11rr` | 4.56307e-1 | 4.56313e-1 | sp lin 4001 gives 4.56312e-1 |
| `s11c`, `s11r` | 4.55799e-1 | 4.55804e-1 | sp lin 4001 gives 4.55803e-1 |
| `s11m`, `s11mr` | 4.61286e-1 | 4.61292e-1 | avg follows integ. Not checked against a reference |

All S lines move toward the fine sweep, which closes receipt 10's "direction not checked" for these.
`sprmsall`, closed form, stays. New check: `s11c = s11ic / 3.1e9`, to 1e-5.

RED, the reworked decks on `ef6445218`'s rule:
```
dc up misses: 17     dc down misses: 9    dc through zero misses: 2    ac misses: 9
tran misses: 0       tran equal steps misses: 7    exact on a cubic misses: 3
ui2 = 2.00000e-01 (0.2667)   ur1 = 5.59017e-01 (0.6374)   li1 = 1.87500e-10 (1.96875e-10)   ei1 = 2.12400e+00 (2.106)
sp bad = 2     sprmsc = 2.81839e+09 (2.81850e9)
```
`tran misses: 0` is expected: its avg and integ did not change and its rms band holds both rules. GREEN:
0 in all, `sp bad = 0`. Valgrind on `meas-window-exact`: 0.

### `measure/`

`compare.sh after-11 after-10b`: 11 changed. `compare.sh after-09 after-10b`: 8 changed. Reference: the
receipt 10 decks with `tmax` 1/20 of the step and dc step 0.001, re-run on this binary.

| metric | after-09 | after-11 | after-10b | reference | err 09 | err 11 | err 10b |
|---|---|---|---|---|---|---|---|
| `tran_perf` `iavg` | -2.253391e-4 | -2.253411e-4 | -2.253407e-4 | -2.255036e-4 | 7.28e-4 | 7.20e-4 | 7.22e-4 |
| `tran_inv` `iavg` | -5.878068e-6 | -5.878068e-6 | -5.864154e-6 | -5.87078e-6 | 1.24e-3 | 1.24e-3 | 1.13e-3 |
| `tran_inv` `ioavg` | 1.488786 | 1.488786 | 1.488738 | 1.488754 | 2.15e-5 | 2.15e-5 | 1.07e-5 |
| `tran_inv` `irms` | 4.763981e-5 | 4.757651e-5 | 4.763981e-5 | 4.769365e-5 | 1.13e-3 | 2.46e-3 | 1.13e-3 |
| `tran_inv` `vrms` | 1.268701 | 1.268604 | 1.268701 | 1.268685 | 1.26e-5 | 6.38e-5 | 1.26e-5 |
| `tran_inv` `vint` | 9.003461e-9 | 9.003441e-9 | 9.003461e-9 | 9.003142e-9 | 3.54e-5 | 3.32e-5 | 3.54e-5 |
| `tran_inv` `davg`, `davg2` | 0.8580869 | 0.8580869 | 0.8581184 | 0.858101 | 1.63e-5 | 1.63e-5 | **2.03e-5** |
| `tran_inv` `drms`, `drms2` | 1.202470 | 1.202133 | 1.202593 | 1.202516 | 3.83e-5 | 3.18e-4 | **6.40e-5** |
| `tran_inv` `dint2` | 1.029741 | 1.029704 | 1.029742 | 1.029721 | 1.94e-5 | 1.65e-5 | **2.04e-5** |

Attribution:

- `irms`, `vrms`, `vint`: back to the after-09 numbers exactly, as the rule is back. All rms lines are
  nearer than after-11, the gate receipt 10 missed.
- `iavg`, `ioavg`: the kept first sample; both nearer than after-09.
- **Three further than after-09: `davg`, `drms`, `dint2`, on the dc sweep.** That is Simpson's phase on a
  kink, not the window rule. The old code's value matches Simpson started two steps later, which is what skipping
  the sample at 0.3 gives; old binary, 9 digits: 1.202470213 at `from=0.3`, 1.202443081 at `from=0.29999999999`.
  New binary: 1.202592763 at both. Keeping the sample starts Simpson at the window. Python copy of the rule on
  the sweep, against step 0.001:

| Simpson starts | rms err | integ err |
|---|---|---|
| at the window (new) | 6.34e-5 | 2.03e-5 |
| one step later | 6.10e-5 | 8.83e-5 |
| two steps later (after-09's value) | 3.85e-5 | 1.90e-5 |

So on the inverter's transition with step 0.02 the rule's own error is about 2e-5 to 9e-5 and depends on
where the window starts. Neither is "right". `davg` further than after-09 is the cost of `avg = integ/width`
here: it was a trapezoid at 1.63e-5, and now shares integ's 2.03e-5.

## 2. `when` on a plot that is not real

The guard is `measure_real_plot()`, on the vector, the second vector and the scale, for dc and tran. It
moved above `com_measure_when()`. The function now starts `m_measured = NAN`: on failure the callers
tested an allocated 0, which printed as a result (`meas tran x when v(5)=0.5`: "no real data", then `x = 0`).

| exit, `ry` a real `let` on `.ac` | ver_50 | before | after |
|---|---|---|---|
| dc, tran `when ry=0.5` | 139, 139 | 139, 139 | refused, 0 |
| dc, tran `find ry when ry=0.5` | 139, 139 | 139, 139 | refused, 0 |
| dc, tran `deriv ry when ry=0.5` | **0, 0** | **139, 139** | refused, 0 |
| dc, tran `trig ry ... targ ry ...` | 139, 139 | 139, 139 | refused, 0 |
| dc `trig at=20 targ ry`, tran `trig ry targ at=20` | 139, 139 | 139, 139 | refused, 0 |
| dc `when v(5)=0.5`, dc `when ry=half`, `ry=v(5)` | 139 | 139 | refused, 0 |
| tran `when v(5)=0.5` | 0, `= 0` | 0, `= 0` | refused, 0, no value |

RED: `meas-when-on-ac.cir` exit 139 on the first line. GREEN: 16 refusals, 0 invalid reads under valgrind.
Valid measures in the same deck: `meas ac` k1 76.2816, k2 -0.443016, k3 -2.15716e-3, k4 24.9077, unchanged
from before; k1, k2, k4 equal ver_50. Real dc and tran `when`, `find`, `deriv`, `trig/targ` give r1 to r8.

## 3. `measure_margin()`

`n = MIN(d->v_length, dScale->v_length)`, as in `8be9bfcdd`.

| | before | after |
|---|---|---|
| valgrind, `meas-margin-length.cir` | 1 invalid read | 0 |

RED, seen by `check.sh` too, crossovers only past the 3-point scale:
```
u1 = 1.80000e+02 deg at= 0.00000e+00
u2 = -0.00000e+00 dB at= 0.00000e+00
```
The `at=` comes from the heap. GREEN: both "no crossover found". Crossovers inside the scale, on long
and on equal vectors, give 180 deg at 1.41421 and 0 dB at 2.44949, worked by hand.

## Seen, not changed

- `meas sp ... when` and `trig/targ` on a `.sp` plot, and on an ac plot, still exit 139 in the same
  function: `sp_check` reads `dScale->v_realdata`. Receipt 09 item 21, `u03`, `u04`, 139 on all versions.
  The `ac_check` arm of that read looks like the fix, untested. Not in this job's list.

## Unverified

- Shared library, `--disable-xspice`, `--disable-sp`, MSVC, MinGW builds.
- `examples/measure/` decks were not compared.
- The `s11m` line against a reference.
- Which rule is nearer on real circuit data with kinks in general: three dc metrics above say it depends
  on the window's start.
