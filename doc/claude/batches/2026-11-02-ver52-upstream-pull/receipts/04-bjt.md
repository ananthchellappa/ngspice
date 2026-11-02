# Receipt 04: group 04, BJT

All 5 picks landed, none dropped. Four clean, one hand-resolved. No fork code was added.
All heavy commands went through `ngheavy`. No exit 137.

| upstream | on `ver_52` | what | pick |
|---|---|---|---|
| `c07143999` | `3fc7c4a5b` | BJT review: Jacobian, area, ISS, AC/PZ/noise stamps, bypass state, op queries | clean. 14 files, not 15: the `diodefs.h` hunk was already on `ver_52` |
| `75f22a99f` | `ceffabe06` | ISE/ISC > 1e-4 multiplier resolved in BJTtemp | clean |
| `209f9bff6` | `1e96a2847` | epi charge continuous, brings `bjt-qco-1` | `tests/regression/model/Makefile.am` TESTS line: kept the fork's `binning-edges.cir` and added upstream's test |
| `6dd312a6e` | `63736c7e5` | no division by VO | clean |
| `d329b9780` | `62f1c088d` | RCO, QCO scale with area | clean |

No later upstream commit touches `src/spicelib/devices/bjt` except these (`git log c07143999..refs/upstream/pre-master-48`) and CMake's `36fb6f51f`.

## Sky130: what moved, measured before and after

**before** is `ver_52` at `bbebe760e`, **after** is `62f1c088d`. Scratch decks, default tolerances, `numdgt=15`, compared with no tolerance on the raw files:
- `sky_bjt`: CE stages of `npn_05v5_W1p00L1p00`, `npn_05v5_W1p00L2p00`, `npn_11v0_W1p00L1p00`, `pnp_05v5_W3p40L3p40`, `pnp_05v5_W0p68L0p68`, 10k loads, Vbe 0.70. op, `ac dec 10 1 100g`, `noise`, `tran 2n 12u` with a 100 mV base pulse.
- `sky_bg`: the `measure/decks/bandgap.cir` core. op at -40, 27, 125, `ac`, `dc vdd 1.2 2.0 0.05`, 12 us power-up `tran 10n 40u`.
- `sky_sens`: `sens v(c)`, dc and ac, on the npn.

Two runs of the same binary agree with tolerance 0, so every difference below is real.

| quantity | before | after | relative | cause |
|---|---|---|---|---|
| sky_bjt op, 12 vectors | | | 0 | |
| sky_bg op at 3 temps, ac, tran; timepoints 4378, rejected 115 | | | 0 at 1e-12 | |
| sky_bjt ac, `\|v(cn)\|` at 1 Hz, gain 0.1953 | | | 1.03e-8 | hunk E |
| sky_bjt ac, `\|v(cp2)\|` at 1 Hz, gain 0.0744 | | | 1.45e-7 | hunk E |
| sky_bjt noise `onoise_total` | 2.351032882e-4 | 2.351032869e-4 | 5.7e-9 | hunk E |
| sky_bjt noise, 12 new `_rci` vectors | absent | all 0 | | `bjtnoise.c`, no RCO in sky130, value 0 |
| sky_sens ac, `vcc_acmag` | 9.99999990e-1 | 9.99999980e-1 | 1e-8 | hunk E |
| sky_sens dc, `q.xn1...:iss` | -5955.23 | -6279.81 | 5.2e-2 | `bjttemp.c` ISS |
| sky_bjt tran, worst v(cn3) at 6.01 us | | 15.7 uV apart on 3.19 V | 4.9e-6 | hunk A |
| sky_bjt tran, worst v(cp), v(cp2), iterations 12124, timepoints 6038 | | | 0 | |
| sky_bg dc, worst v(ref) | 1.15878905 | 1.15878907 | 1.6e-8 | hunk D |

All hunks are in `c07143999`. `75f22a99f`, `209f9bff6`, `6dd312a6e`, `d329b9780` move nothing on these decks, with tolerance 0. `sky130` sets no `rco`, and its ise and isc are absolute.

Attribution was by experiment, not by reading. I built `c07143999` with hunks taken out, and restored the source after each build:

| hunk taken out | what goes back to identical with before |
|---|---|
| all of A, B, C, D, E | every sky130 number above except ISS, which is `bjttemp.c` and was left in |
| E, `gdsub` in `bjtacld.c`, `bjtpzld.c` | sky_bjt ac, noise, sky_sens ac |
| A, `vsub` extrapolated on MODEINITPRED | sky_bjt tran |
| D, `geqcb = 0` in a dc sweep | sky_bg dc |
| B and T, `vbx` predictor and INITTRAN read | no sky130 number. `xcjc = 1` on every sky130 card I read, so `capbx` is 0 |
| C, `vbx` in the bypass test | no sky130 number |

Why each is right:

- **E, AC.** With `iss = 0`, `gdsub` is gmin, 1e-12 S. DC already carried it and AC did not. Across a 10k load 1e-12 x 10k gives the measured 1e-8. In the pnp, which is lateral by default, it runs from base' to collector, so the size is G/gm: Ic 0.2 uA, gm 8e-6, 1.25e-7 against 1.45e-7 measured.
- **A, D, numeric path.** The solution did not move, the stopping point of Newton did. With `reltol=1e-6 abstol=1e-15 vntol=1e-9 chgtol=1e-17`: sky_bjt tran worst 15.7 uV -> 84 nV, sky_bg dc v(ref) 1.6e-8 -> 3.7e-11. Default `reltol` is 1e-3.
- **ISS.** The card has `iss = 0`, so nothing moves in any analysis. `sens` perturbs it, and the temperature factor at 27 against tnom 30 changed from `exp(factlog/nr)` to `exp(factlog/ns)`. `nr` 0.8976, `ns` 1: ratio 1.054 measured, about 1.05 by hand. The commit's stated fix.

The result is what the triage guessed: sky130 op does not move, and everything else is at or under solver tolerance or is gmin. Bandgap op, AC and transient are identical.

## .sens, which `c07143999` says to check

`gp_sens`: a full Gummel-Poon card, `xcjc=.5`, with `area=1.5` and a lateral pnp with `area=2`. dc and `ac dec 3 1k 100meg`.

| | changed at 1e-9 | note |
|---|---|---|
| dc, areas as written | 6 of 422: `q1_area` 3.46895e-2 -> 3.46892e-2, `q1:ise`, `q1:is`, `q1:tnom`, `q1:irb` at 1e-5 to 2e-7 | `BJTBCtSatCur` scaled by area twice |
| dc, the same deck at area 1 | **0 of 422** | that hunk does nothing at area 1 |
| ac | 149 of 502, all parameters with no effect, 0 -> order 1e-17 | hunk E, goes to 0 changed with E out. Round-off on zero |

So the moved state slots change nothing in `.sens` here. `sens` in this build is `cktsens.c`, which does not use `BJTsensxp*`; those are `WANT_SENSE2` only.

**A defect that is not from this group.** `.sens` is wrong on a BJT, on 45.2, before and after alike, identical numbers:

| parameter | central difference by `altermod` | `.sens` |
|---|---|---|
| `is` | -7.4e13 | -7.44e13, right |
| `rb` | 2.45e-5 | 2.43e-5, right |
| `bf` | -1.95e-3 | 1.02e-9, **wrong** |
| `vaf` | 4.6e-4 | 1.92e-9, **wrong** |

And an `op` after `sens` gives v(c) = 12.0 where it was 8.32: `sens` leaves the circuit altered. Same on 45.2. Not touched here, not in the picks.

## Off sky130: the purpose of each pick

| what | before | after | commit |
|---|---|---|---|
| `bjt-qco-1.cir`, RED on `ceffabe06` | `ERROR: BJT QCO transient stopped early`, 1152 rows | `INFO: success`, 3030 rows | `209f9bff6` |
| Irci Jacobian: rco=50, vo=2, Vbe 0.8, Vc 1, AC conductance against dc slope 1.5863e-3 S | 1.5588e-3, 1.7 % off | 1.5863e-3 | `c07143999`, `rKp1_Vbci` |
| rco, area=2, op current against m=2, -5.30300e-2 | -3.3548e-2 | -5.30300246e-2, equal to 9 digits; ac at 3 points equal | `c07143999` -3.6707e-2, then `d329b9780` |
| rco, `vo=0` | op fails, "timestep too small" | op found, warning "VO = 0 ..." once per setup | `6dd312a6e` |
| `is=1e-15 ise=100`, `altermod is=2e-15`, Ib | -6.5477e-9, `showmod` ise 1e-13 | -8.1242e-9 = twice the first, `showmod` ise 100 | `75f22a99f`. A deck that reads ise back sees the card value now |
| `gp_tran`: `xcjc=.5`, cjs, iss, max distance from a tight run at `reltol=1e-6` with max step 0.02n | v(c) 2.33 mV, v(b) 10.2 mV | 0.52 mV, 0.72 mV. Iterations 6094 -> 6092 | `c07143999`, `vbx` hunks. Before and after converge to 14 uV of each other when tight |
| `@q1[ft]` | "no such parameter" | 7.47934e8; gm/(2 pi (Cpi+Cmu)) by hand 7.4794e8 | `c07143999` |
| `@q1[ikf]`, `[collectorconduct]`, `[emitterconduct]`, area=2 | "no such parameter" | 0.02, 0.4, 1.0, each right by hand | `c07143999` |

`gp_tran` is the largest move at default tolerance, 2.3 % on v(c) at a switching edge. It only happens with `xcjc < 1`; it is the old bypass bug and the new number is closer to the tight run.

## Gates

- `make` 0, `make install` 0.
- Full `make -k check`: **367 PASS, 1 FAIL, 0 SKIP**. 366 at group 03's fix round plus `bjt-qco-1.cir`. The FAIL is `resource-keyword-case.cmd`, the baseline's named exception. `identity.lint`, `selftest.lint` PASS. `identity.baseline` untouched. No `.out` changed; the only new one is upstream's.
- `measure/run.sh`: exit 0, `mc_r` and `mc_sky130` `identical_to_ref = yes`. Saved in `measure/after-04/` (96 KB).
- Against `after-03-fix`: **`TOTAL changed: 0`** at 1e-9, `mc_*` and `drawlog_*` exact. So nothing to attribute in the suite. Its BJT decks are op and a dc on one line of the bandgap; the moves above are under its tolerance or in analyses it does not run.

## Unverified, and two things to know

- **`--enable-sense2` not built.** The slots moved under it: `BJTsensxp*` follow the macro and `BJTnumStates`, read, consistent. But `bjtsload.c` and `bjtsacl.c` save and restore states `0..20` by index, and the new `BJTvbx` at slot 4 pushes `BJTgeqcb` from 20 to 21, out of that range. Upstream's commit lists the sensitivity files as "not addressed". By reading only, and only with that flag.
- A consequence of `c07143999`'s noise hunk: every BJT gets a new `_rci` vector in `noise` output, 0 when there is no RCO. A script that indexes noise vectors by position would notice; a script by name does not.
- Not run: pole-zero with `gdsub`, `tlev=3`, `is<=0` and `xcjc` warnings, SOA checks (`bjtsoachk.c`), lateral `d329b9780` with `areac`, `ngSpice_Reset`.
- `make check`'s log carries `ERROR: (internal) tried to destroy non-existent graph` just before `vector-probe-report.cir`, which passes. Run alone, on before or after, that deck does not print it. Not looked into further, and no BJT in that test.
