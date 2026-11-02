# Receipt 05: group 05, diode, MOS2, HFET, MESA

All 9 picks landed, none dropped, all clean. No fork code was added, no `.out` changed.
All heavy commands went through `ngheavy`. No exit 137.

| date | upstream | on `ver_52` | what |
|---|---|---|---|
| 08-24 | `8533ad9fb` | `27814fb05` | soft recovery scaling (`qpscale`), level 3 m^2, AC/PZ self-heating, state count 22 to 21 |
| 09-07 | `d58ee1041` | `daf0f3226` | crash from `8533ad9fb`: rth0 on the model, instance not thermal |
| 09-18 | `149688cd3` | `adf329f52` | sidewall charge beyond fcs used the bottom F1 (bug 870) |
| 10-01 | `fc601dcab` | `3bb39c9aa` | `cd`, `cdsw` descriptions |
| 10-07 | `dfbbc64ca` | `aa3faa0d6` | MOS2 gm, gds, gmbs derivatives |
| 10-12 | `2faaabefc` | `e057f2b10` | HFET, MESA `inverse` stuck across instances (bugs 877, 878) |
| 10-23 | `e8285746c` | `9154a50a3` | level 1 IBV scales with area |
| 10-26 | `9cc5bdbea` | `1875b8a99` | isr conductance |
| 10-26 | `a69b65ce4` | `92778039f` | isr generation current in reverse and breakdown |

Order is by commit date, so `dfbbc64ca` and `2faaabefc` come before `e8285746c`.

## Sky130: the triage underestimated `149688cd3`

Checked the cards first: 15 subckts, all `level = 3`, all with cjsw, mjsw, php. None sets tt, isr, nr, vp, rth0, rsw, fcs, or uses w/l; all use `area` and `pj`. tm1 and tm2 are 0.

Scratch decks, `area=100 perim=1000` unless noted, `numdgt=15`, compared with tolerance 0. A binary was kept per pick, so every move below is bisected.

| quantity | before (`60ef0aaa6`) | after (`92778039f`) | cause |
|---|---|---|---|
| Forward transient, 45 decks (15 cards x slow 27 C, slow 125 C, fast 27 C): abort with `Timestep too small` | **13** | **0** | `149688cd3` |
| Same 3 aborting decks on `/usr/bin/ngspice` 45.2 | | all abort | not a fork defect |
| Hand: q05 deck (05v5 numbers, no rs), stiff ramp -0.5 to 0.3 V, peak current | 11.6 uA, sign-flipping | 0.172 uA; hand C(0.3 V) dV/dt + dc = 0.172 uA | `149688cd3` |
| q05 charge from the ramp start | 1.77943e-13 C | 1.40839e-13 C; hand 1.3365e-13 + 7.2e-15 dc = 1.4086e-13 | `149688cd3` |
| Fast transient, 05v5 default tolerances, worst distance from a tight run (`reltol=1e-5`, 5 ps) | 5.4 mV | 1.4 mV | `149688cd3`. The tight run itself aborts before the pick |
| Fast transient, worst difference on all completed cards | | 2.1 to 18.6 mV of a 2.2 V swing, at edges and the knee | `149688cd3` |
| Op + AC, stiff drive (1 ohm), 13 cards complete, 6 biases, 3 temps: 936 op values, 4080 AC rows | | 5 op values at or under 1.3e-13, AC worst 4.7e-14 | `149688cd3` for `dd10` (bisected). `dd06` is one `id` at 3e-16, not bisected |
| Op + AC, 1k drive: `pw2nd_05v5_defet` and `pw2nd_11v0` at 125 C, 0.3 V | v(a) 0.28742, 0.29830 | 0.28751, 0.29765 | `149688cd3` via transient-op fallback; id and gd there are numerically unconverged both before and after, see below |
| `sky_pz` pole, `onoise_total`, `inoise_total` | | identical | |
| measure `dio_res` (05v5 id, gd, cd) | | 0 changed | |

Why `149688cd3`: beyond `fcs * php` the sidewall charge used the bottom junction's F1. Crossing the knee, charge jumps by `Cjsw (F1 - F1SW)`. For 05v5 that knee is 0.1 V, F1 = 0.4188, F1SW = 0.1000, so 1.15e-14 C at this size, and 05v5 aborted at exactly that crossing. The cards' small mjsw and php make the jump large. So this is not "real, small": a forward-biased sky130 diode often could not be simulated at all.

`8533ad9fb`, `d58ee1041`, `fc601dcab`, `e8285746c`, `9cc5bdbea`, `a69b65ce4` move no sky130 op, AC or transient number at tolerance 0: same iterations and timepoints. `dfbbc64ca` and `2faaabefc` do not touch the path.

**A cost: slower transient-op fallback.** `pd2nw_05v5_lvt`, 125 C, 0.6 V through 1 ohm, on the path after gmin and source stepping fail: 44.6M iterations, 15.3 s before; 695.6M iterations, 208.9 s after; v(a) moves by 3.5e-6. Bisected to `149688cd3`. I did not establish why that ramp needs more steps. Completed slow transients also run long, for example `s27_07`, which aborted at 4322 iterations and now completes at 18.1M. Kept: an abort is worse.

**Not from this group, still open.** The cards' `rs` of order 1e-9 / area gives G around 1e11 S, so the double-precision floor on the internal node is about 1e-5 A. That is the +-7.6 uA noise in `i(v)` on `sky_q`, and why `id` differs from the resistor current by up to 21 % at the 125 C points above. On before and after alike, and 45.2 shows the same fallbacks: `op` often reaches transient-op, 8 of 15 1k decks do not finish 40 s, `dd01` does not finish 100 s, `s27_06` not 40 s. The five transient decks that timed out at 120 s before finish under 40 s after.

## `.sens`, which `8533ad9fb` called for

`sens` here is `cktsens.c`. States moved, and it did not care: 85 rows on `sens_lvl1` and `sens_rr` (with vp), dc and ac, identical to 12 digits across `8533ad9fb`, except:

| what | before | after | commit |
|---|---|---|---|
| New row | absent | `d1:qpscale = 0` on every diode | `8533ad9fb`. A script indexing by position would notice |
| `.sens` level 3, `_m` (true value by `alter` on `s3`: -2.25e-2) | 0, wrong | -7.13e-5, also wrong | `8533ad9fb` |
| `.sens` level 3, messages | none | `area must be positive` 6 times on `s3`, 1126 on `sky_sens` | `8533ad9fb` |
| `:isr`, `:nr` on a card with no isr, 0.6 V (`sky_sens`) | 4.5697e16, -2.5223e-3 | 4.2527e16, -2.3658e-3 | `9cc5bdbea` by reading and by the level 1 chain; a forward bias does not reach `a69b65ce4`'s branches |

Cause of the middle two: perturbing `w` and `l` sets their Given flags, which stay set, so `DIOsetup` recomputes area as 0. Before, area 0 went silently and later rows read 0. Now area is forced to 1 with a warning. Both wrong: **`.sens` on a level 3 diode was already wrong, and stays wrong**; `_pj` is also off by 10 (-1.61e-3 against -1.54e-4), identically before and after, and the `op` after `sens` is left altered (0.59999 before, 0.59837 after; true 0.53101). Same class as the BJT `.sens` defect in receipt 04. `_area` is right. Not fixed here.

The state reorder puts voltage, current, conduct, capCharge, capCurrent in slots 0-4, which is what `diosload.c` and `diosacl.c` save with `i <= 4`. So `--enable-sense2` should be better, by reading only.

## Off sky130: the purpose of each pick

| what | before | after | commit |
|---|---|---|---|
| Level 3, `w l m=2` against two m=1 in parallel, i | 2.04815e-3 against 1.02408e-3 | 1.02408e-3, equal | `8533ad9fb` |
| Self-heating AC, thermal response at 1, 10 kHz over dc; hand with 1.59 kHz pole 0.846, 0.157 | 1.000, 1.000 | 0.846, 0.156 | `8533ad9fb` |
| Self-heating PZ pole; hand -1/(rth0 cth0) = -1e4 | none | -9.95e3 | `8533ad9fb` |
| PZ with `rsw`, against AC at 6 frequencies, 1e6 to 1e11 | one pole; off by 14x at 1e9 | two poles; equal to 5 digits | `8533ad9fb` |
| Soft recovery `@d[cd]`; hand (1-vp) tt gd = 1.0961e-8 | 2.1923e-8 | 1.0961e-8 | `8533ad9fb` |
| Soft recovery with isr, Qrr, Irr | 2.49072e-10, 1.28816e-2 | 2.47281e-10, 1.28407e-2 | `8533ad9fb`: recombination no longer drives Qp |
| Soft recovery without isr: If, Irr, Qrr, also tight | | identical to 6 digits; iterations 5058 to 5063 | `8533ad9fb` |
| The same with `qpscale=1` | | 5058, as the message promises | `8533ad9fb` |
| `area=0`, `m=0` at 0.6 V, each | 6e-13 A, silent | 1.187e-4 A, warning, 1 used | `8533ad9fb`. **A deck using area=0 to switch a diode off now conducts** |
| rth0, instance not thermal, sparse and KLU | fine | abort in `spGetElement` at `27814fb05`, fine at `daf0f3226` | `d58ee1041` |
| `reset` with rs, rsw, qp nodes, 3 times | | identical to 14 digits | `8533ad9fb` unsetup |
| `@d1[cdsw]` | no such parameter | a value. It is 0 without `rsw`; `cd` holds both. `showmod` adds `qpscale` and moves `rth0`, `cth0` | `8533ad9fb`, `fc601dcab` |
| MOS2 worst gmbs error against central difference, nfs given | 15.6 % | 0.09 % | `dfbbc64ca` |
| MOS2 gds error, xj and nfs, saturation | 7.7 % | 0.27 % | `dfbbc64ca` |
| MOS2 id, all 15 points | | identical | `dfbbc64ca`: derivatives only |
| Forward HFET beside a reversed one; alone -1.55693e-4 | 5.51220e-5 | -1.55693e-4 | `2faaabefc` |
| Forward MESA beside a reversed one; alone -1.32209e-4 | -3.12279e-7 | -1.32209e-4 | `2faaabefc` |
| Level 1 breakdown, -5.05 V, area=2; m=2 gives 1.11434e-2 | 6.13782e-3 | 1.11434e-2 | `e8285746c`. Changes any level 1 deck with ibv and area != 1 |
| isr, gd against numeric slope, forward 0.02 to 0.38 V, worst | 2.3 % | 0.05 % | `9cc5bdbea` |
| isr, gd against numeric slope at -3nVt | 99.9 % | 0.02 % | `a69b65ce4` |
| isr, reverse current at -0.3, -5 V | -8.110e-11, -8.110e-11 | -1.151e-10, -2.314e-10 | `a69b65ce4`: depletion growth, stated purpose. Value of a deck with isr changes |

MOS2 residuals. `dfbbc64ca` is not all better: linear region `m2`, `m3` at vds 0.4, gmb error goes 0.26 % to 0.77 % and 0.53 % to 0.84 %. Weak inversion with xj: gds still 172 % off. No sky130 use.

## Gates

- `make` 0 after every pick, `make install` 0.
- Full `make -k check`: **367 PASS, 1 FAIL, 0 SKIP**, same as group 04. The FAIL is `resource-keyword-case.cmd`, the baseline's named exception. `identity.lint`, `selftest.lint` PASS. `identity.baseline` untouched. `tests/` unchanged, so the fork's diode, `mosamp`, `hfet`, `mesa` references hold.
- `measure/run.sh`: exit 0, `mc_r` and `mc_sky130` `identical_to_ref = yes`. Saved in `measure/after-05/` (96 KB).
- Against `after-04`: **`TOTAL changed: 0`** at 1e-9, `mc_*` and `drawlog_*` exact. So nothing to attribute in the suite. Its only diode deck is 05v5 op by direct drive, which no pick moves; the suite has no diode transient, which is why it did not see the largest change in this group. Worth adding one.

## Unverified

- `--enable-sense2` not built.
- Not run: distortion, SOA with these picks, `ngSpice_Reset` (only the `reset` command), diode noise beyond `sky_pz`.
- By reading: `DIOtF1SW` uses the temperature-adjusted mjsw and `dioload.c` uses the plain one, so with tm1 or tm2 not 0 the sidewall continuity is inexact. Sky130 has both 0. Upstream tip has this too.
- Op and AC coverage on 1k drive is partial where decks time out: 8 of 15 reach 0 to 9 of 15 bias points. Stiff drive compares 13 of the 15 cards in full. `dd01` (`ps2dn`) gives no op values before or after in 100 s; its 1k deck `op01` completed all 15 points unchanged. `dd12` (`pd2nw_05v5_lvt`) is the slow case above: on it only 27 C 0.6 V and 0.3 V and 125 C 0.6 V were compared.
- Fast transient differences are not compared against a tight run, except on 05v5.
