# Receipt 06: group 06, VBIC and VDMOS

All 6 picks landed, none dropped, all clean. No fork code was added, no `.out` changed.
All heavy commands went through `ngheavy`. No exit 137.

| date | upstream | on `ver_52` | what |
|---|---|---|---|
| 08-23 | `5c4e9ab1d` | `2c3230981` | VBIC stamps, polarity, Ith, ask, noise, LTE, KLU; removes model `dtemp` |
| 08-24 | `b3594401c` | `dfb468109` | VDMOS polarity, self-heating, SOA, rg noise; `DEVlimitlog` in `devsup.c` |
| 09-07 | `211db2633` | `f58ac432e` | VBIC excess-phase nodes on re-setup |
| 09-11 | `11136ee61` | `75b2e61f6` | VBIC `xrb`, `xrc` map to intrinsic |
| 09-11 | `c0a97461d` | `3979ef0fe` | VBIC `xrb`, `xrc` fall back to extrinsic too |
| 10-07 | `42f32880f` | `29ca1e1eb` | VDMOS AC/PZ quasi-saturation stamp |

Order is by commit date, as upstream has it: `b3594401c` is an ancestor of `211db2633`. The task list put it fifth. A binary was kept per pick, so moves below are bisected.

## `DEVlimitlog`: sky130 FET and diode results do not move

**By reading.** Only 4 callers: `dioload.c` (only `if (selfheat)`), `vbicload.c`, `vdmosload.c`, `osdicallbacks.c`. Sky130 FETs are BSIM4, which does not call it. Sky130 diode cards set no `rth0`, by the group 05 card check. For two non-NaN arguments the old and new code take the same path; only NaN inputs differ.

**By a probe.** A scratch binary with one `fprintf` at the function's entry, built then removed. The restored build is byte-identical to the saved `42f32880f` binary (sha256 `9a4f9ed6e1ec2aff...`), which is also the installed one.

| run | calls | NaN calls |
|---|---|---|
| All `measure/` decks | 0 | 0 |
| Sky130 sweep below | 0 | 0 |
| Positive control: self-heated diode, normal | 121 | 0 |
| Positive control: self-heated diode, runaway | 1845 | 118 |
| Positive control: VDMOS thermal, VBIC thermal | 7167, 167 | 0, 0 |

**By comparison, tolerance 0.** Before group (`ed299ca2e`) against after, `numdgt=17`, exact text:

- Sweep: 13 diode cards (`area=100 perim=1000`; 27 and 125 C; 0.6, 0.3, -1, -4 V; op, then 1 MHz AC) and 8 FET cards (`nfet_01v8`, `_lvt`, `pfet_01v8`, `_hvt`, `_lvt`, `nfet_g5v0d10v5`, `pfet_g5v0d10v5`, `nfet_03v3_nvt`; 27 and 125 C; op, AC, transient, iterations): **21 decks, 0 lines differ.**
- A ninth FET deck, `nfet_05v0_nvt`, stops at a netlist error with `w=2 l=0.5`, the same before and after. It compared nothing.
- `measure/` against `after-05-fix` at `0 0`: **`TOTAL changed: 0`**, probe binary too.
- Self-heated diode, the one caller off VBIC and VDMOS, `rth0=300`: identical to 15 digits, op and transient. Runaway `rth0=1e5`: same values, minus the message below.

**What does change, NaN only.** The once-per-process `The temperature limiting function received NaN` message is gone, on diode, VBIC and VDMOS. A runaway deck that aborted still aborts (`vs_run`: exit 1 before and after), so it now gives no thermal hint. Recovery now keeps the last usable value instead of 0. OSDI models with `$limit` log get this too; not run, none in sky130.

## Model `dtemp`, for the release note

**A deck using it neither errors nor warns, and a card with only model `dtemp` gives the same current.** `create_model` finds no model parameter, finds an instance parameter of that name, and stores the value as a default for each instance. So the card still works, by a different path.

| deck, `.model ... dtemp=50` unless noted | before | after |
|---|---|---|
| Model `dtemp` alone, ic; instance `dtemp=50` on a plain card is the same | 8.6111957e-4 | 8.6111957e-4 |
| Model 50 + instance `dtemp=10`, ic. **Silent change** | 1.33721e-3 (adds, 60) | 1.04726e-4 (instance wins, 10) |
| Model 50 + instance `temp=100`, ic. **Silent change** | 9.97583e-3 (150) | 2.25095e-3 (100) |
| No model dtemp, instance `temp=100 dtemp=10`, ic. **Silent change** | 3.23023e-3 (110) | 2.25095e-3 (100) |
| The same after one `altermod` | 4.48285e-3 (it accumulates) | 2.25095e-3 |
| `print @nd[dtemp]`; `altermod @nd[dtemp]=25` | 50; takes effect | `Error: no such parameter dtemp.`, value stays, exit 0 |
| `showmod q1 : dtemp` | 50 | `?????????` |

## VBIC, measured

Deck: `IS=1e-16 ... RTH=3000 CTH=1e-9`, `q1 c b 0 0 dt M` (self-heating needs the fifth node; 4 terminals ignore RTH), 0.8 V, 500 ohm. All `5c4e9ab1d`; `5c4e9ab1d` and final are identical on the PNP deck.

| what | before | after | truth |
|---|---|---|---|
| PNP with `td`, no thermal node: op, AC, PZ, DC, transient | all abort | run | mirror of the NPN to 8 digits |
| PNP, self-heating: all analyses | all abort, NaN, exit 1 | run, exit 0 | v(c) -2.0658114 against NPN 2.06581137 |
| NPN self-heat, gain at 1 mHz | 17.373 | 25.3844 | numeric dv/dv 25.3843 |
| NPN self-heat, v(c), default tolerances | 2.06592798 | 2.06581137 | before with `reltol=1e-9` gives 2.06581137: before stopped unconverged |
| Energy balance, P x Rth against v(dt) | 11.67385, 11.67033 | 11.674658, 11.674658 | equal |
| Thermal pole | -3.957e5 | -2.710e5 | AC real part at that f: -23.007; midpoint hand -22.989 |
| KCL with parasitic on (Vc 0.2 V): `ic+ib+ie+is` | 1.69e-5 | -2.2e-19 | 0 |
| `@q1[ic]` there; terminal current 1.16776e-3 | 1.15086e-3 | 1.16776e-3 | |
| `@q1[is]` sign, a script reading it will notice | +1.69e-5 | -1.69e-5 | terminal -1.69e-5 |
| `onoise_q1_rbp` at 1 MHz | 4.243e-12 | 1.496e-15 | new branch, not independently derived |
| `onoise_q1` against root-sum-square of its rows | 1.3298920e-10, 1.3298926e-10 | 1.32921546e-10, equal | adds an `onoise_q1_ibex` row |
| KLU, `td` and thermal, AC | `matrix is singular`, aborts | equal to sparse at printed digits | |
| Same deck, no self-heat or td, NPN op: | | moves 3e-9 in `@ic` only | gmin no longer on `Irci`, and `ic` definition |
| ECL DFF example, transient iterations, timepoints, final v(q) | 81491, 40545, -0.26337303 | 81489, 40545, -0.26337305 | no cost |

`11136ee61`, `c0a97461d`, at 125 C, ic, against a twin with the specific exponents, by pick:

| card | twin | `5c4e9ab1d` | `11136ee61` | `c0a97461d` |
|---|---|---|---|---|
| `xrb=1.5` | 9.1956773e-3 | 9.3724118e-3 | 9.4489993e-3 | **9.1956773e-3** |
| `xrc=2` | 9.6043464e-3 | 9.6384927e-3 | 9.6095051e-3 | **9.6043464e-3** |
| `xrb=1.5 xrbi=0.5` | 9.3207618e-3 | equal | 9.5859897e-3 | **equal** |
| `xrc=2 xrci=1` | 9.6254794e-3 | equal | 9.6299600e-3 | **equal** |

So only the final state is right, and `11136ee61` alone would be wrong: both must stay. `showmod` now shows the resolved `xrbi=1.5`, where before it was 0.

`211db2633`: `altermod @nt[td]=0` gives the card twin (AC 7.1173514) before and after; every printed value on `vb_resetup` equal to `5c4e9ab1d`, as the message says. The leak it fixes was not observed: `Circuit Equations` says 19 at each.

## `tests/vbic` does not run in `make check`

It is in `DIST_SUBDIRS` and not in `SUBDIRS`, so the gate has no VBIC coverage. Run by hand, filter as `check.sh`:

| test | before: diff lines | after: diff lines | note |
|---|---|---|---|
| `FG` (PNP, td) | hung, killed at 40 s, also 5 min earlier | **0** | `5c4e9ab1d` |
| `FO` | 2 | **0** | `5c4e9ab1d`, worst rel move 1.3e-6 |
| `temp` | 0 | 0 | |
| `CEamp` | 1712 | 1712 | Stale reference, 45.2 has the same 1712. Values identical before and after |
| `noise_scale_test` | 258 | 258 | Stale reference, 45.2 has the same 258. Worst move 1.44 %, `5c4e9ab1d` noise fixes |
| `diffamp` | 634, op fails, all `-nan` | 672, op and AC now run | Its `.out` has the `dtemp` row, and tran stops at `no data saved for Transient analysis` on both |

I did not wire this in or touch a reference: not this crew's job. `FG`, `FO`, `temp` would pass today.

## VDMOS, measured

Deck: IRFP240-like, 5.5 V gate, 10 V, 1 ohm, `thermal` with `Rthjc=0.5 Rthca=10`.

| what | before | after | truth, commit |
|---|---|---|---|
| Plain n and p, no thermal, no `rq`: op, AC, PZ, numeric | | identical at 8 digits | `b3594401c`, `42f32880f` leave it alone |
| Thermal, gain at 1 mHz | 5.5092 | 6.54278 | tight numeric 6.54282, `b3594401c` |
| Thermal, v(c), default tolerances, n | 4.09075560 | 4.09064071 | before with tight tolerances gives 4.09064071, `b3594401c` |
| Thermal, p against n, v(c) | -4.0903527 against 4.0907556 | mirror to 8 digits | `b3594401c` |
| Thermal, PZ | aborted | 4 poles, -754.4 rad/s lowest | `b3594401c`; pole not checked by hand |
| Soft recovery, thermal, p against n: If | -4.19840, -4.19942 | -4.19942, equal | `b3594401c` |
| Soft recovery, no thermal, n and p | | identical | |
| Quasi-saturation, gain at 1 mHz | 19.3686 | 19.5623 | tight numeric 19.5623, `42f32880f` |
| Quasi-saturation, op | | identical | `42f32880f`: small-signal only |
| `Pd_max=10`, no `Id_max`, 58.76 W; hand from V and R 58.76 W | no message | `Pd=58.76 W ... exceeded` | `b3594401c` |
| `rg=3` noise at 1 MHz, `onoise_m1_rg`; hand sqrt(4kT Rg) x gain 7.158 = 1.5963e-9 | no row | 1.59630e-9 | `b3594401c` |
| Same, `inoise_spectrum`. **Any noise deck with rg moves** | 4.42e-11 | 2.27e-10 | stated purpose |
| `rg=0`, all noise rows | | identical, plus a 0 `_rg` row | |
| KLU against sparse, all 6 decks | | equal at printed digits | |
| `reset` x2, `altermod` vp 0 and back, equations 15, 14 | | stable to 11 digits | |

**Numeric caution.** At default tolerances a DC central difference on quasi-saturation gives 19.4646, between the two, and looks like a defect in `42f32880f`. Tight tolerances resolve it: `reltol` is larger than the 1e-4 V step moves.

**Not reproduced.** The message says a p-channel soft recovery aborts at the first time point. Here it completed before the pick, equal to the n-channel, on `sr_p_a`, `vs_p_sr`. The fork's picks may have changed that; I did not chase it.

Unsetup, read for the group 05 defect: VDMOS and VBIC zero the node numbers outside the condition, so they do not have it.

## Gates

- `make` 0 after every pick, `make install` 0.
- Full `make -k check`: **368 PASS, 1 FAIL, 0 SKIP**, same as group 05. The FAIL is `resource-keyword-case.cmd`, the baseline's named exception. `identity.lint`, `selftest.lint` PASS. `identity.baseline` untouched. `tests/` unchanged.
- `measure/run.sh`: exit 0, `mc_r` and `mc_sky130` `identical_to_ref = yes`. Saved in `measure/after-06/` (96 KB).
- Against `after-05-fix`: **`TOTAL changed: 0`** at 1e-9 and at 0, `mc_*` and `drawlog_*` exact. Nothing to attribute: no deck in the suite uses VBIC or VDMOS, so its 0 says only that sky130 did not move. The numbers above are the evidence for the picks.

## Unverified

- OSDI `$limit` on the new NaN path, `ngSpice_Reset` (only the `reset` command), `--enable-sense2`, VBIC and VDMOS `.sens`, distortion.
- `211db2633`'s leak: not reproduced, so the fix is unproven beyond no change.
- VDMOS thermal pole, VBIC `rbp` noise: no independent value. `Tj` clamp not exercised beyond the runaway decks.
- Sky130 sweep: `nfet_05v0_nvt` compared nothing. Corner `tt` only. Diodes by direct drive only, not through a resistor.
