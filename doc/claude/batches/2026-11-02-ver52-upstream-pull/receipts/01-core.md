# Receipt 01: group 01, core fixes

All 13 picks landed clean with `git cherry-pick -x`, no hand resolution, none dropped. Each was
built, installed and measured on its own, so every changed quantity has one named commit.
All heavy commands went through `ngheavy` at `-j4`. No exit 137.

Order: true upstream order (`git rev-list --topo-order`), per PLAN rule 2. The task's list is not in
that order (`942ab1ec7` is older than it was listed). No pick depended on the difference.

| upstream | on `ver_52` | suite change at 1e-9 | what it is |
|---|---|---|---|
| `ba36e3bdc` | `5ff8ae233` | 0 | PWL `r=` clamp |
| `b277f48c0` | `0feb860c6` | 1 | `meas avg` tran window |
| `7c2ebe189` | `8cb875d71` | 2 | `meas avg` dc window |
| `650ac305f` | `0a70e54d0` | 4 | `meas rms/integ` descending dc |
| `63d0fcf9d` | `7b695e846` | 1 | `meas avg` final interval |
| `942ab1ec7` | `2f3396cb4` | 0 | `.sp` noise sqrt guard |
| `f961c98d0` | `269a2cc81` | 65 | BSIM3/BSIM4v5 dVdseffCV_dVb |
| `f4f9cbdfb` | `81dad005d` | 0 | tran/optran stop test |
| `3ef069fb1` | `826b43632` | 0 | behavioural C/L `m` |
| `277e7654a` | `79e456765` | 0 | retry from last accepted solution |
| `3e8a21ae6` | `4e13a942d` | 0 | STI/WPE mobility in `u0temp` |
| `5caff08fa` | `e233bb825` | 0 | breakpoint near TSTOP |
| `68b0d57b3` | `b789364fa` | 0 | CPL over 8 conductors |

`f4f9cbdfb`, `3ef069fb1` and `277e7654a` were also compared at tolerance 0: bit-identical.

## Gates

- `make` exit 0 and `make install` exit 0 after every pick.
- `make -k check`: **363 PASS, 1 FAIL, 0 SKIP**, the same counts as the baseline. The FAIL is the
  same test, `tests/regression/pipe/resource-keyword-case.cmd`, with the same message. It is the
  question left open in `00-baseline.md`, not from this group. So PLAN's "0 FAIL" gate is met only
  with that named exception, and plain `make check` still aborts in that directory. `identity.lint`
  and `selftest.lint` pass. `tests/lint/identity.baseline` was not touched. No `.out` was changed.
- `measure/run.sh`: exit 0. `mc_r` and `mc_sky130` `identical_to_ref = yes`. `mc_*` and `drawlog_*`:
  0 changes, so the draws and the draw log contract hold.
- Output is saved in `measure/after-01/` (92 KB). Against the baseline: **73 changed**, 0 unattributed.

## Every changed quantity

| deck | quantity | old | new | commit | reason |
|---|---|---|---|---|---|
| `tran_perf` | `iavg` | -2.253363e-04 | -2.253391e-04 (1.2e-5) | `b277f48c0` | window clipped to `from`, `to` |
| `tran_inv` | `davg` | 0.8566408 | 0.8580869 (1.7e-3) | `7c2ebe189` | dc window clipped |
| `tran_inv` | `davg2` | 0.8421223 | 0.8580869 (1.9e-2) | `7c2ebe189` | same; now equals ascending `davg` |
| `tran_inv` | `dint2` | 0 | 1.029741 | `650ac305f` | descending loop read index -1 |
| `tran_inv` | `drms2` | error "out of interval" | 1.202470 | `650ac305f` | same; now equals ascending `drms`. The 2 messages went with it |
| `tran_inv` | `ioavg` | 1.489977 | 1.488786 (8.0e-4) | `63d0fcf9d` | final 4 ps step was dropped from the divisor |
| `fet_op` | `cgs`, sat, 24 devices | | worst 1.0e-4 | `f961c98d0` | Cgb term; Cgs is `-(Cgg+Cgd+Cgb)` |
| `fet_op` | `cgs`, weak, 24 devices | | worst 1.4e-5 | `f961c98d0` | same |
| `ac_noise` | `m`, `p` at 3 high frequencies | | worst 3.0e-6 | `f961c98d0` | see below |
| `ac_noise` | noise spectrum, one point each | | 2.5e-6 | `f961c98d0` | see below |
| `ac_noise` | `inoise_total` | | 1.1e-7 | `f961c98d0` | see below |
| `tran_inv` | `iavg`, `tpd`, `m_o1`, `m_o2`, `m_idd` | | worst 1.7e-7 | `f961c98d0` | Newton path, see below |
| `tran_perf` | `m_n1`, `m_idd`, `s_n1` | | worst 1.1e-6 | `f961c98d0` | same |

The `ioavg` number is explained exactly: the first sample past `to` is within 100 ULP of it, so the
last 4 ps step was left out of the divisor, and 4 ps out of 5004 ps is 7.99e-4, as measured. A
trapezoid over `wrdata` output, written separately in Python, gives 1.4887860, the new value.

## How far `meas avg` moved

On sky130 decks, by the meas series: `tran_inv` `iavg` 0, `tran_perf` `iavg` 1.2e-5, `ioavg` 8.0e-4,
an ascending dc sweep 1.7e-3, a descending one 1.9e-2. By `f961c98d0` alone: `iavg`
1.7e-7, the last digit `meas` stores.

Analytic check, a ramp `v = t` from 2.1 to 7.3, where the true average is 4.7:

| case | ver_51 binary | ver_52 |
|---|---|---|
| tran, `avg` | 4.756 | 4.700 |
| dc ascending, `avg` | 4.625 | 4.700 |
| dc descending, `avg` | 4.635 | 4.700 |
| dc descending, `rms` | error | 4.93436 (ascending 4.93434) |
| dc descending, `integ` | 0 | 24.44 (true 24.44) |

**Monte Carlo metrics using `meas avg` will shift** by these amounts, and they are a correction.

Not fixed by this series: the triage says `avg` now matches `integ/(to-from)`. On the ramp it does.
On `v(o2)`, `avg` times the window is 7.44989e-9 and `integ` gives 7.44965e-9, 3e-5 apart, in both
binaries. The Python trapezoid agrees with `avg`. I did not look into why `integ` differs.

## Size of `f961c98d0` on sky130

Measured across this one pick, plus two scratch decks against the ver_51 binary.

- **Capacitance:** `cgs` worst 1.0e-4 in saturation, 1.4e-5 in weak inversion. `cgd` and `cbd` did
  not move at 1e-9.
- **AC:** common-source stage, worst 3.0e-6, on a phase near 0.06 rad; magnitude 2.0e-8. Both source
  and bulk are at AC ground there, which hides most of the term, so I added a source follower with
  bulk not tied to source: worst 1.7e-6 at 100 GHz, 5e-7 at 10 GHz, about 2e-7 up to 1 GHz.
- **Noise:** spectrum 2.5e-6 at worst, totals 1.1e-7 and 2.6e-8 (follower).
- **`.sens`:** DC `sens v(out)`, 0 changes. AC `.sens` on a sky130 FET **cannot be measured**: it
  aborts on both binaries ("doAnalyses: no such parameter", "sens simulation(s) aborted"). On a plain
  `level=54 version=4.5 capmod=2` card it runs, and 0 of 4869 numbers changed at 15 digits, though
  `ac` on that same card moves 3e-8. I did not find out why AC `.sens` does not see it.
- **Tran:** bias and step counts unchanged. Means move 1e-8 to 1e-6, `tpd` 1.2e-7.

Direction checked: finite differences of the gate charge are identical on both binaries to 15 digits,
so charges did not change, as the triage said. At three biases `cgb` moves toward the finite difference:

| vg, vd | finite difference | old | new |
|---|---|---|---|
| 0.9, 0.9 | -4.12577e-16 | -4.08435e-16 | -4.08462e-16 |
| 1.8, 1.8 | -3.68623e-16 | -3.61191e-16 | -3.61230e-16 |
| 0.7, 0.2 | -4.53445e-16 | -4.52827e-16 | -4.53103e-16 |

The correction is small against the remaining gap: the reported capacitances still differ from the
charge derivative, `cgb` by up to 2% and `cgs` by up to 4.5% (vg = vd = 1.8). Why is outside this pick
and I did not look into it.

Conclusion: the fix goes the right way and is small. No sky130 small-signal result here moved by
more than 1e-4, and most by far less.

## Picks the suite cannot see, checked by hand (ver_51 binary against ver_52)

| pick | deck | old | new |
|---|---|---|---|
| `ba36e3bdc` | PWL `r=0` 1 ns clock, ending high, 4 us, 260004 points | 8 dropouts to 0 V, first at 1.911 us | 0 |
| `3ef069fb1` | behavioural `c=` with `m=1.5`, `v(b)` at 1 ns | 1.000 (capacitor gone) | 0.2670 (hand estimate 0.270) |
| `68b0d57b3` | CPL with 9 conductors | segfault, exit 139 | "Too many conductors (9)", exit 1 |
| `3e8a21ae6` | sky130, see below | | |

**`3e8a21ae6` is large on any deck that sets `sa`, `sb`.** Sky130, vg = vd = 0.9, `w=1 l=0.15`:

| device | old id | new id | change |
|---|---|---|---|
| nfet, default `sa`, `sb` | 3.288926e-05 | 3.288926e-05 | 0 |
| nfet, `sa=sb=1` | 3.253182e-05 | 3.241391e-05 | -0.36% |
| nfet, `sa=sb=0.3` | 2.522516e-05 | 2.302594e-05 | -8.7% |
| pfet, `sa=sb=0.3` | 1.632199e-07 | 1.844406e-07 | +13% |

The suite has no `sa`, `sb` deck, so it reports 0. The triage called this "inert in common sky130
use", which holds for schematic netlists, but post-layout netlists that carry `sa`, `sb` will move
by this much. It is upstream's stated purpose, so I kept it. Attributing this table to `3e8a21ae6`
is from reading the code: it is the only pick that touches DC mobility. I did not run it across
that pick alone, and no reference simulator confirms the new values.

## Unverified

- `277e7654a`: no deck here retries after non-convergence. `tran_perf` has 1 rejected step and stays
  bit-identical, which fits a rejection by truncation error, but that is my inference. Bug 879 not
  reproduced. `optran.c` not run.
- `f4f9cbdfb`: `dctran` unchanged. The optran hang and the `sharedsync` change were not run.
  `sharedspice.c` is not in this build, so I compiled it alone in a scratch `--with-ngshared` configure:
  exit 0. The library was not linked or run, and `ngSpice_Reset` with the new `CKTrhsAccepted` was
  not exercised.
- `5caff08fa`: a PWL breakpoint 1e-17 s before TSTOP gives `tlast` and 1015 points in both binaries,
  so the deck did not reproduce the old behaviour. Half its diff is whitespace.
- `942ab1ec7`: no `.sp` noise deck run; only `make check` covers that file.
- `make check` prints "ERROR: (internal) tried to destroy non-existent graph" in the passing test
  `vector-probe-report.cir`. No pick touches plotting. It did not appear running that deck alone
  under either binary, and the baseline's log was not kept, so I cannot say it predates this group.

## Fix round

The independent reviewer found one blocking issue: the meas series segfaults on `.sp`.

**Confirmed, on a two-port deck of my own** (`portnum` V sources, 100 ohm, 1 pF, `sp lin 21 1g 5g`),
so the numbers differ from the reviewer's but the pattern is the same:

| `meas sp a ...` | ver_50 binary | ver_52 before | ver_52 after |
|---|---|---|---|
| `avg s_1_1 from=1.3g to=4.1g` | 4.52269e-01 | exit 139 | 4.52287e-01 |
| `avg s_1_1 from=1.3g` | 4.41083e-01 | exit 139 | 4.42256e-01 |
| `avg s_1_1 to=4.1g` | 4.56973e-01 | exit 139 | 4.55799e-01 |
| `avg vm(s_1_1) from=1.3g to=4.1g` | 4.58195e-01 | exit 139 | 4.58167e-01 |
| `avg s_1_1 from=1g to=5g` | 4.45730e-01 | 4.45730e-01 | 4.45730e-01 |
| `integ s_1_1 from=1.3g to=4.1g` | exit 139 | exit 139 | 1.26640e+09 |
| `rms s_1_1 from=1.3g to=4.1g` | exit 139 | exit 139 | 4.52712e-01 |

`meas ac ... avg` on the same plot agrees with the new `meas sp`: 4.52287e-01 and 4.58167e-01.
Upstream tip has the same `measure_interpolate()`, so the defect is upstream's as well.

**Wider than reported.** Two more new crashes had the same cause, both exit 139 before and 0 after:
`meas ac avg` of a real `let` vector with a window (the ac branch read `v_compdata`), and `meas sp avg`
on an `fft` plot, whose scale is real.

**Fix, `27d02928d`, fork-written:** `measure_interpolate()` reads x and y by what each holds, not by analysis
name. `measure_rms_integral()` gets an sp branch. Tran and dc data have no `v_compdata`, so they take
the same values as before.

**RED:** new `tests/regression/misc/meas-sp-window.cir` on the unfixed ver_52 binary: exit 139 with no
measurement printed. It also crashes `build-ver_50` and `/usr/bin/ngspice` 45.2 (exit 139), on the
rms and integ lines. It was then extended after the fix, with `zramp` and `to=`-only S parameter lines, so that
final text ran only against the fixed binary. To show its checks can fail, I changed 4 gold values in a copy: it reported
`sp bad = 2`, `ac bad = 1`, `fft bad = 1`.

**Oracle:** ramps, whose answers are exact: `avg` 2.7e9 and `integ` 7.56e18 on sp, ac and fft plots; `vr`, `vi`, `vm`
of `f + 2fj` give 2.7e9, 5.4e9, 6.03738e9. The windowed `rms` of a ramp is 2.81869e9 against a true 2.81839e9;
a Python replica of the sum, with the first-sample rule below, gives 2.81869e9. The deck is gated on `SP_WANTED`.
A `--disable-sp` build was not run.

### Gates

- `make`, `make install`: exit 0.
- `make -k check`: **364 PASS, 1 FAIL, 0 SKIP**. One more PASS than before, the new test. The FAIL is still
  `resource-keyword-case.cmd`, the baseline's. No existing `.out` changed; `identity.baseline` untouched.
- `measure/run.sh`, saved in `measure/after-01-fix/`: exit 0, `mc_r` and `mc_sky130` `identical_to_ref = yes`.
  Against `after-01` at tolerance 0: **0 changed**, timing aside.

### Found, not fixed: for the driver to decide

1. **`avg` with `from=` skips the first sample in the window.** On interpolating, the picked code puts the
   boundary value in place of sample `i`, so the first segment is a chord across two steps. This is in
   `b277f48c0` and at upstream tip, on every analysis. `rms` and `integ` have long done the same.
   Measured with a 1 MHz sine, 25 ns steps, `tran`, `from=0.26u to=1.13u`:

   | | avg |
   |---|---|
   | ver_50 | -1.57622e-01 |
   | ver_52 | -1.36497e-01 |
   | Python trapezoid, all samples | -1.36432e-01 |
   | Python trapezoid, first sample skipped | -1.36497e-01 |
   | analytic | -1.36716e-01 |

   The `.sp` deck above shows the same: 4.52294e-01 with all samples, 4.52287e-01 skipped, and `meas` gives
   the latter. The pick is still a large net gain, and the error shrinks with the step. The ramp table earlier
   in this receipt could not see this, as a ramp's chord is exact. Fixing it would move `meas avg` results
   again and depart from upstream, so I left it. The new deck avoids freezing it: its S parameter lines use `to=` alone
   (`avg` 4.55799e-01, Python 4.55799e-01), and only the ramp `rms` above carries the rule into `.out`.
2. **`meas sp ... when` still segfaults**, on `build-ver_50` and `ver_52` alike: `measure_when()` reads
   `dScale->v_realdata` on sp, NULL for a `.sp` scale. Not from this series and not part of the issue. No test covers it.
