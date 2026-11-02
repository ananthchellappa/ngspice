# Receipt 10: save and meas fixes (R1, R5, R2, R3, R6)

Fork-written, one commit each, on `ver_52`. **R6 diverges from upstream on purpose.**
**R6 meets "exact for piecewise-linear" and does not meet "must shrink toward truth" on smooth
signals: see the section "The conflict". It needs the driver's decision.**

| | commit | test in `make check` |
|---|---|---|
| R1 | `3b4ed4b2c` | `save-none-circuits.cir` |
| R5 | `c607ae3ea` | `save-none-node.cir`, `-cards`, `-print` |
| R2 | `1c7253057` | `meas-dc-on-ac.cir` |
| R3 | `8be9bfcdd` | `meas-scale-length.cir` |
| R6 | `ef6445218` | `meas-window-exact.cir`, tightened `meas-sp-window.cir` |

Gates: `make` exit 0, `make install`, `make -k check` **382 PASS, 1 FAIL** (the named
`resource-keyword-case.cmd`), 0 SKIP. 375 + 7 new. `identity.lint` passes; the baseline is untouched.
The full check ran before a comment-only reflow of the new deck; that deck and `meas-sp-window` were
re-run after it and pass.

## R1 `save none` use-after-free (`u16`)

The receipt's suggestion was right: `ft_curckt->ci_dbs = dbs;` after the purge.

| valgrind, invalid reads | before | after |
|---|---|---|
| `u16-save-none-uaf.cir` | 4 | 0 |
| `save-none-circuits.cir` | 9 | 0 |

RED, seen by `check.sh`: `setcirc` back to the circuit lost an entry.

```
move to the deck with the purge:
	save none, then switching circuits away and back
3    save none
```
GREEN has `2    trace 1` above that line.

## R5 `save v(none)` (`u19`)

Unwrapping gave the bare word and the keyword test ran after it. Three places compared the name.
The keyword is now decided on the word as written and stored as a new type, `DB_SAVENONE`;
`outitf.c` asks `ft_savenone()`. `gettoks()` keeps `v(none)` wrapped so `settrace()` can tell.

Value 9 was taken by `DB_DEADIPLOT`, so it is 10. Storing the node as `v(none)` does not work:
`name_unwrap()` returns `(none`.

The same defect was on the cards, so they are fixed and tested too: `.save v(none)`,
`.print tran v(none)`, `.meas tran x max v(none)`.

RED, old binary:
```
Warning from checkvalid: vector none is not available or has zero length.
Error: RHS "v(none)[3]" invalid
    time                : time, real, 1 long [default scale]
Error: measure  vmax  max(TRIG) : no such vector as 'v(none)'
```
GREEN: `none : voltage, real, 59 long`, value 1, `vmax = 1.00000e+00`. Plain `save none` keeps one
point of the scale, also in a deck with the node; `save v(2) none` keeps the scale only;
`save none v(none)` keeps the node. Valgrind 0.

Left as it is: `status` prints a node called none as `save none`, like the keyword.

## R2 `meas dc deriv` and `find` on an AC plot (`u08`, `u08b`)

The same guard covers both: `measure_real_plot()` wants real data in the vector and the scale for
dc and tran. It looks at the scale because a real `let` on an AC plot still has a complex one.

| | before | after |
|---|---|---|
| `u08` | exit 139 | error, `survived`, exit 0 |
| `u08b` | exit 139 | error, `survived`, exit 0 |
| `meas-dc-on-ac.cir` | exit 139 | 6 refusals, then `a7 = 8.46733e-01`, `a8 = -1.68110e-03` from `meas ac` |

Behaviour change beyond the crash: `meas dc deriv` of a real `let` on an AC plot used to return a
number and now refuses.

**Still open, not in this job:** `meas dc w1 when ry=0.5` on an AC plot exits 139, in
`com_measure_when()`, a different function with a different guard. By reading, `find ... when` and
`deriv ... when` reach it too. Deck in the appendix.

## R3 scale length (`u07`)

`min(d->v_length, dScale->v_length)`, in both.

| | before | after |
|---|---|---|
| valgrind, `u07` | 8 invalid reads | 0 |
| valgrind, `meas-scale-length.cir` | 8 | 0 |

RED, seen by `check.sh`, numbers made from the heap, different on each run:
```
u2 = 1.71300e+03   u3 = -3.50000e-01   u4 = 2.00000e+00   u5 = -3.50000e-01   u6 = 2.00000e+00
n1 = 2.50000e-01
```
GREEN: `u1` to `u6` are `out of interval`. `n1 = 1.00000e-01`, the slope over the two samples that
have a scale value. The reference's u-lines are gone, so a garbage number fails the test.

## R6 `avg`, `rms`, `integ` windows (`u10`)

All three now share one walk in `measure_rms_integral()`, which integrates the line through the
samples. Edges interpolate, every sample inside is used, widths are unsigned, so a descending sweep
needs no reversal. `avg` left `measure_minMaxAvg()`.

| rule | before | after |
|---|---|---|
| first sample in the window | replaced by the `from` point | kept |
| integ | Simpson over equal steps, else trapezoid | trapezoid |
| rms | Simpson or trapezoid of `y*y` | `h(u*u + u*v + v*v)/3` on each segment |
| window inside one step | avg and integ 0, rms error | exact |
| `from=0`, `to=0` on a dc sweep through 0 | not clipped | clipped |

`u10`, against truth avg 0.4375, integ 0.175:

| | ver_50 | ver_52 before | after |
|---|---|---|---|
| dc avg | 0.5 | 0.4375 | 0.4375 |
| dc integ | 0.075 | 0.075 | 0.175 |
| ac avg | 0.5 | 0.1875 | 0.4375 |
| ac integ | 0.075 | 0.075 | 0.175 |

`meas-window-exact.cir`, checks to 2e-6 against values worked by hand: dc up, dc down, dc through
zero, ac (real, complex, `vi`, `vm`, `meas sp` on an ac plot), tran (pwl source). `misses` per
section: RED 12, 8, 3, 18, 4 (the last with fewer checks than the final deck). GREEN 0 in all five.
RED lines: `ui1 = 7.5e-02` (true 0.175), `ur2 = 8.16497e-01` (true 0.57735), `ua3 = 0` (true 0.5),
`ui7 = 2.25e-01` (true 0.2). The fft plot is not in this deck; `meas-sp-window` covers it on ramps.

**Upstream.** Tip gives what ver_52 gave here: 0.1875 and 0.075, per receipt 09 row 28. This is
deliberately not upstream's behaviour. A report to them should cite the first-sample skip, not offer
this rule without the section below.

## The conflict

Exact on every piecewise-linear signal fixes the rule uniquely: given the samples, the line through
them is one function with one integral. So Simpson and the trapezoid of `y*y` had to go, and both
are better on a smooth curve. The two requests cannot both hold on smooth data, and this measures it.

`upstream-decks/g10-meas-smooth-analytic.cir`, error relative to the closed form:

| signal | metric | ver_50 | ver_52 before | after |
|---|---|---|---|---|
| dc `sin x`, step 0.1 | avg | 2.34e-2 | 8.33e-4 | 8.33e-4 |
| | integ | 6.21e-5 | 6.21e-5 | **8.30e-4** |
| | rms | 1.45e-4 | 1.45e-4 | **8.33e-4** |
| tran sine, 100 steps a period | avg | 5.20e-2 | 2.58e-4 | **3.29e-4** |
| | integ | 1.14e-4 | 1.14e-4 | **3.30e-4** |
| | rms | 1.66e-5 | 1.66e-5 | **3.29e-4** |
| | rms, whole periods | 1.16e-5 | 1.16e-5 | **3.29e-4** |
| tran sine, 20 steps a period | avg | 2.06e-1 | 3.94e-3 | **8.24e-3** |
| | integ | 8.52e-3 | 8.52e-3 | 8.24e-3 |
| | rms | 6.27e-4 | 6.27e-4 | **8.17e-3** |
| | rms, whole periods | 9.18e-4 | 9.18e-4 | **8.19e-3** |
| tran RC step, adaptive | avg | 1.55e-3 | 1.66e-6 | **2.84e-6** |
| | integ | 6.36e-6 | 6.36e-6 | 3.43e-6 |
| | rms | 7.56e-6 | 7.56e-6 | 2.93e-6 |
| ac lin, RC magnitude | avg | 5.41e-3 | 4.76e-4 | 4.26e-4 |
| | integ | 8.73e-5 | 8.73e-5 | **4.27e-4** |
| | rms | 3.15e-4 | 3.15e-4 | **4.76e-4** |
| ac dec, RC magnitude | avg | 5.71e-2 | 1.27e-3 | 1.27e-3 |
| | integ | 1.27e-3 | 1.27e-3 | 1.27e-3 |
| | rms | 1.60e-3 | 1.60e-3 | 1.01e-3 |

Bold is further from truth than ver_52 before. Against ver_50, `avg` is better everywhere; `integ`
and `rms` are worse wherever the bold is. The `avg` growth is the old skip cancelling part of the
chord error by accident, so it cannot be kept while fixing the skip.

The worst practical line: rms of a sine over whole periods, 1e-5 to 3.3e-4 at 100 points a period,
and 9e-4 to 8e-3 at 20. The error scales as `(w h)^2 / 12`, always low, so `.tran` with a coarse
`tmax` under-reads rms.

### An alternative, measured and not committed

rms by the trapezoid of `y*y`, every other change kept: smooth errors mostly back near ver_52 (sine,
whole periods 1.10e-6; dc `sin x` 3.12e-4), but `meas-window-exact.cir` then has 26 misses, on the rms
line on data with a corner. Tent over 1.3 to 1.7: 0.6374 against a true 0.5590.

Neither rule wins on the sky130 metrics below: it brings `drms` nearer and leaves `irms` further
than ver_52.

## `measure/` against `after-09`

`compare.sh after-09 after-10`: **7 changed**, all from R6. `mc_r` and `mc_sky130` identical to 45.2,
draw logs unchanged. R1, R2, R3, R5 moved nothing.

Truth here is an estimate: the same decks with `tmax` 1/20 of the step and dc step 0.001. At that
step the old and new rules agree to 8e-6 at worst (`irms`), so the reference does not depend on the
rule.

| metric | after-09 | after-10 | move | reference | error before | error after | |
|---|---|---|---|---|---|---|---|
| `tran_perf` ring `iavg` | -2.253391e-4 | -2.253411e-4 | 8.9e-6 | -2.255036e-4 | 7.29e-4 | 7.21e-4 | shrinks |
| `tran_inv` `vint` | 9.003461e-9 | 9.003441e-9 | 2.2e-6 | 9.003142e-9 | 3.54e-5 | 3.32e-5 | shrinks |
| `tran_inv` `dint2` | 1.029741 | 1.029704 | 3.6e-5 | 1.029721 | 1.94e-5 | 1.65e-5 | shrinks |
| `tran_inv` `vrms` | 1.268701 | 1.268604 | 7.6e-5 | 1.268685 | 1.26e-5 | **6.38e-5** | **grows** |
| `tran_inv` `irms` | 4.763981e-5 | 4.757651e-5 | 1.3e-3 | 4.76933e-5 | 1.12e-3 | **2.45e-3** | **grows** |
| `tran_inv` `drms` | 1.202470 | 1.202133 | 2.8e-4 | 1.202516 | 3.83e-5 | **3.18e-4** | **grows** |
| `tran_inv` `drms2` | 1.202470 | 1.202133 | 2.8e-4 | 1.202516 | 3.83e-5 | **3.18e-4** | **grows** |

Attribution, by rule: `iavg` the kept first sample; `vint`, `dint2` the kept sample and Simpson's
removal; the four rms lines the rms segment rule. `avg` of a dc sweep (`davg`, `davg2`) and `iavg`,
`ioavg` of `tran_inv` did not move past tolerance.

With the alternative rms: `irms` 4.778267e-5 (error 1.87e-3), `vrms` 1.268646 (3.07e-5), `drms`
1.202502 (1.16e-5).

**Every `avg` and `integ` metric shrinks. All 4 `rms` lines grow.** That is the gate this job set,
missed on rms.

## A reference that changed

`meas-sp-window.out`, R6's stated purpose:

| line | old | new | reason |
|---|---|---|---|
| `sprmsc`, `sprmsr` | 2.81869e9 | 2.81839e9 | a ramp: closed form 2.818392e9. The check tightens from 1e-3 to 2e-6 |
| `s11ic`, `s11ir` | 1.41299e9 | 1.41298e9 | Simpson's removal, curved data. Direction against truth not checked |
| `s11rc`, `s11rr` | 4.56313e-1 | 4.56307e-1 | rms rule, curved data. Direction against truth not checked |

## Beyond the list, and why

- `avg`'s check of vector length against scale length now covers `rms` and `integ`, as the code is
  shared. Before: `let a = vector(5000)`, `meas dc integ a` on a two-point sweep gave 6 invalid reads
  in valgrind, 0 now. A vector of another length now errors there too, as `avg` always did.
- The shared read of the scale ends `u05`'s exit 139 (`meas ac avg`, `rms` on an fft plot): exit 0.
  No test added for it.
- `integ` with no overlap still gives 0 and `from= nan`: unchanged, not asked (receipt 09, item 4).

## Unverified

- Shared library: `save none`, `DB_SAVENONE` and `ngSpice_Reset`. Not linked or run.
- `--disable-xspice`, `--disable-sp`, MSVC, MinGW builds.
- `examples/measure/` decks were not compared.
- The `meas-sp-window` S parameter lines, against truth.
- `sp` plots from `.sp` with a kink: covered through `meas sp` on an ac plot, and ramps on `.sp`.

## Appendix: the `when` crash left open

```
v3 4 0 dc 0 ac 1
r3 4 5 1k
c3 5 0 1u
.ac dec 1 10 100
.control
run
let ry=real(v(5))
meas dc w1 when ry=0.5
.endc
.end
```
