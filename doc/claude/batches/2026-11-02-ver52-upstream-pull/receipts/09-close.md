# Receipt 09: close of `ver_52`

Measured at `c97ea689c`, the tip before this receipt. All heavy commands went through `ngheavy` at `-j4`. No exit 137.
Nothing was pushed or posted. `ver_52` has no remote ref.

**Verdict: ready for the owner's review, with one failing test and six regressions against ver_51 left in on purpose or unnoticed until review. See section 2 and section 8.**

`ver_51..ver_52` is 91 commits: 68 upstream picks with `-x` (one reverted) and 23 written here (9 fix commits, 1 revert, 13 docs).
`src/`: 125 files, +3769, -1366.

## 1. Clean rebuild and `make check`

| step | exit |
|---|---|
| `make clean` | 0 |
| `make -j4` | 0 |
| `make install` | 0 |
| `make -k check` | 2, from the one FAIL below |

Binary: `build-ver_52/src/ngspice` equals `stage/bin/ngspice`, sha256 `cff94f024a114bf1784f...`.

**375 PASS, 1 FAIL, 0 SKIP, 0 XFAIL, 376 tests.**

| | PASS | FAIL | total |
|---|---|---|---|
| Baseline, this worktree | 363 | 1 | 364 |
| Baseline, as PLAN counted it, with the owner's uncommitted test edit | 364 | 0 | 364 |
| Close | 375 | 1 | 376 |

The FAIL is `tests/regression/pipe/resource-keyword-case.cmd`, the same test and cause as the baseline (receipt 00). It needs the owner's uncommitted edit of that test. It is not from this batch. **So PLAN's "0 FAIL" gate is met only with this named exception, and plain `make check` still stops in that directory.** `make -k` is how every count in this batch was taken.

**New tests, 12:**

| test | from |
|---|---|
| `tests/regression/misc/meas-sp-window.cir` | fork, `27d02928d` |
| `tests/regression/model/binning-edges.cir` | fork, `0f763b135` |
| `tests/regression/misc/meas-deriv-two-points.cir` | fork, `c957569c9` |
| `tests/regression/model/bjt-qco-1.cir` | upstream, `209f9bff6` |
| `tests/regression/misc/diode-rs-altermod.cir` | fork, `a291cdc7f` |
| `tests/xspice/analog/switch-ac.cir` | upstream, `613ba2ea5` and `a1148c969` |
| `tests/regression/misc/save-none.cir` | fork, `9551715a9` |
| `tests/regression/rng/option-seed-control.cir`, `-netlist`, `-random`, `-unusable` | fork, `69c754cdb` |
| `tests/regression/rng/option-seed-empty.cir` | fork, `968e7042a` |

**Changed tests: none.** `git diff --name-status ver_51 ver_52 -- tests` has no modified `.cir`, `.out`, `.err` or `.status`. The only modified files are 4 `Makefile.am`: `regression/misc`, `regression/model`, `regression/rng`, `xspice`. No reference was regenerated. `identity.lint` and `selftest.lint` PASS.

An open item from receipts 01 and 04 closes here: `ERROR: (internal) tried to destroy non-existent graph` is in the baseline crew's own `make -k check` log from 07:25, before the first pick, so it predates every pick. Still unexplained, and it is in a passing test.

## 2. Sky130: before and after

`measure/run.sh`, on the rebuilt binary, exit 0, saved in `measure/after-09/`. Against `after-08` at tolerance 0: **`TOTAL changed: 0`**. Against `baseline` at 1e-9: **73 changed**, the same 73 since group 01. Tolerance 0 adds the bandgap line, 14 small `ac_noise` values and one `tran_inv` value, 94 in all.

### Seen by the suite

| deck | quantity | before | after | relative | commit |
|---|---|---|---|---|---|
| `fet_op` | `cgs`, saturation, 24 devices | e.g. xn10 -2.91518e-15 | -2.91489e-15 | 1.1e-5 to 1.0e-4 | `f961c98d0` |
| `fet_op` | `cgs`, weak, 24 devices | e.g. xn10 -2.44245e-16 | -2.44242e-16 | 3.3e-6 to 1.4e-5 | `f961c98d0` |
| `ac_noise` | phase at the top frequency | 0.0632499 rad | 0.0632501 rad | 3.0e-6 | `f961c98d0` |
| `ac_noise` | magnitude | | | at most 2.0e-8 | `f961c98d0` |
| `ac_noise` | noise spectrum, worst point | 3.11496e-9 | 3.11495e-9 | 2.5e-6 | `f961c98d0` |
| `ac_noise` | `inoise_total` | 1.637289e-3 | 1.637289e-3 | 1.1e-7 | `f961c98d0` |
| `tran_inv` | `tpd` | 8.434517e-11 | 8.434516e-11 | 1.2e-7 | `f961c98d0` |
| `tran_inv` | `iavg`, `m_idd`, `m_o1`, `m_o2` | | | at most 1.7e-7 | `f961c98d0` |
| `tran_perf` | `m_idd`, `m_n1`, `s_n1` | | | at most 1.1e-6 | `f961c98d0` |
| `tran_perf` | `iavg`, `meas avg` | -2.253363e-4 | -2.253391e-4 | 1.2e-5 | `b277f48c0` |
| `tran_inv` | `ioavg`, `meas avg` | 1.489977 | 1.488786 | 8.0e-4 | `63d0fcf9d` |
| `tran_inv` | `davg`, dc ascending | 0.8566408 | 0.8580869 | 1.7e-3 | `7c2ebe189` |
| `tran_inv` | `davg2`, dc descending | 0.8421223 | 0.8580869 | 1.9e-2 | `7c2ebe189` |
| `tran_inv` | `dint2`, dc descending | 0 | 1.029741 | was wrong | `650ac305f` |
| `tran_inv` | `drms2`, dc descending | error, out of interval | 1.202470 | was an error | `650ac305f` |
| `bandgap` | `vr`, line sweep, 6 points | 1.145893371744 | 1.145893371717 | at most 2.4e-11, below the suite's 1e-9 | `c07143999`: first appears at `after-04`; hunk D by receipt 04's experiment, on a similar deck |

### Not seen by the suite, measured by the crews on scratch decks

| what | before | after | commit, receipt |
|---|---|---|---|
| **FET `id` with `sa`, `sb` set**, nfet w=1 l=0.15, sa=sb=1 | 3.253182e-05 | 3.241391e-05, -0.36 % | `3e8a21ae6`, 01. Attributed by reading |
| Same, sa=sb=0.3, nfet | 2.522516e-05 | 2.302594e-05, **-8.7 %** | same |
| Same, sa=sb=0.3, pfet | 1.632199e-07 | 1.844406e-07, **+13 %** | same |
| **FET, size on a bin edge written as an expression or without hsa**, worst `id`, ss or ff | lower card | upper card: `nfet_01v8` 66 %, 41 %; `pfet_01v8_hvt` 100 %, 79 % | `03ef19244`, 02 |
| Same, `nfet_03v3_nvt` at L=0.6, every corner | | 40 % to 79 % | same |
| Same, tt, sf, fs, other devices | | below 1 % | same |
| hsa, literal size, nf=1, the xschem form | | **0 exactly**, 0 bins differ | same |
| hsa, literal multi-finger on an inner edge (w=15 nf=5, L=1) | `.28` | `.20`: another card, and its corner step | `03ef19244`, `0f763b135`, 02 |
| hsa, literal size on the top edge, or finger 0.42 um | load error | loads | `0f763b135`, 02 |
| Sizes up to 1 nm outside the table, no hsa or expression | loaded | **load error** | `03ef19244`, 02 |
| Diode, forward transient, 45 decks: abort `Timestep too small` | **13** | **0** | `149688cd3`, 05 |
| Diode, fast transient, completed cards, worst | | 2.1 to 18.6 mV of 2.2 V | `149688cd3`, 05 |
| Diode q05, peak current on a stiff ramp, hand 0.172 uA | 11.6 uA | 0.172 uA | `149688cd3`, 05 |
| Diode, slowest transient-op fallback, `pd2nw_05v5_lvt` 125 C | 15.3 s | **208.9 s**; v(a) moves 3.5e-6 | `149688cd3`, 05 |
| Diode at 125 C, 0.3 V, 1k: `pw2nd_05v5_defet`, `pw2nd_11v0` | v(a) 0.28742, 0.29830 | 0.28751, 0.29765; unconverged both ways | `149688cd3`, 05 |
| BJT tran, worst v(cn3) | | 15.7 uV of 3.19 V, 4.9e-6 | `c07143999`, 04 |
| BJT AC at 1 Hz, gain 0.0744 | | 1.45e-7 | `c07143999`, 04 |
| BJT `onoise_total` | 2.351032882e-4 | 2.351032869e-4, 5.7e-9 | `c07143999`, 04 |
| Bandgap dc, worst v(ref) | 1.15878905 | 1.15878907, 1.6e-8 | `c07143999`, 04 |
| `.sens` BJT `:iss` (`iss=0`, so nothing else moves) | -5955.23 | -6279.81, 5.2e-2 | `c07143999`, 04 |
| `.sens` diode `:isr` at 0.6 V | 4.5697e16 | 4.2527e16 | `9cc5bdbea`, 05, by reading |
| Inverter `.tran 0.1f 2p`, timepoints | 20032 | 20008; value equal | `472403238`, 07 |

### What did not move

Compared with the baseline **at tolerance 0**, unless stated:

- `fet_op`: `id`, `gm`, `gds`, `gmbs`, `vth`, `cgd`, `cbd`, on all 24 devices at 3 biases. Only `cgs` moved. All bins the same.
- `bjt.txt`: every npn and pnp value, every temperature. 0 lines differ.
- `dio_res.txt`: diode id, gd, cd; `res_xhigh_po`. 0 lines differ.
- `sens.txt`: 0 lines differ.
- `bandgap`: v(ref) over temperature. Only the line sweep moved, at 2.4e-11 at most.
- Step counts: iterations, timepoints, accepted, rejected, all decks. `tran_perf` 22833, 8016, 1 rejected. `tran_inv` 2655.
- `mc_r`, `mc_sky130`, `mc_xhigh_po`, `drawlog_r`, `drawlog_sky130`: 0 lines differ.
- Elapsed, `tran_perf` transient: 0.135 s, 0.139 s; two baseline runs differ by about 2 %, so no visible cost.

By the crews, off the suite: FET `id` at 21 FET and diode decks across group 06 (0 lines, `numdgt=17`); `DEVlimitlog` is never called on sky130 (0 calls); no sky130 card uses VBIC, VDMOS, OSDI, `rth0`, `isr`, `rco`.

**Limits.** All suite FET sizes are hsa, literal. The suite has no `sa`/`sb` deck, no size written as an expression, no diode transient. So its 73 say nothing on the three largest moves above, which come from the scratch decks. Those were measured across their group and not repeated on this binary.

## 3. Confirmations

| claim | result | how |
|---|---|---|
| `setseed k` bit-identical with 45.2 | **yes** | `mc_r`, `mc_sky130` `identical_to_ref = yes`, `numdgt=17`. By hand on this binary, k = 1, 2, 3, 7, 100, 2147483647: same 15 digits. `setseed7-bit-identity.cir` PASS |
| Exception to that, known | device after a sky130 `xhigh_po` | From ver_51's P1 (receipt 00), not this batch. `mc_xhigh_po`: 0 lines differ from baseline |
| Draw log contract | **holds** | `drawlog_r.tsv`, `drawlog_sky130.tsv`: 0 lines differ, tolerance 0. Header, footer, field order, instance forms the same. `logged=33 unlogged_calls=0`. `rnddraws-*` tests PASS |
| Bin keys | changed where expected only | See section 5 |
| `git diff ver_51 ver_52 -- tests/lint/identity.baseline` | **empty**, 0 lines | |
| Owner's checkout untouched | **yes** | HEAD `df2194696`, `ver_51`; `git status` the same 2 lines as at the start (`CLAUDE.md`, `resource-keyword-case.cmd`); no file there outside `.git` modified since 2026-11-02 00:00. `build-ver_50` binary dated 2026-10-26 |
| No RF commit | **none of 74** | Every hash in the triage's 4 RF rows: not an ancestor, no `-x` trailer, `git cherry` `+`. `git diff --name-status` has no RF, HB, nport or Touchstone file |
| None of the "not taken" | **none of 74** | Same three checks: arrowplot 7, solver and tools 5, CMake 3, rshunt 5 (with `385904847`, `b2a86d38c`), `940bb50fa`, `d7e64e28d`, quoted strings 3, seed 4, topology reverts 6, `5476dfb7d`, narrow NOs 5, `58e4cf26b`, SKIP rows 32 |
| `af240ba02` | picked, then reverted | `92511d009`, `fa5e0691f`. No diff across them in `dctrcurv.c`, `cktdisto.c`, `trcvdefs.h` |
| `613ba2ea5` | taken though the triage's SKIP lists it | `a1148c969`'s tests need it |
| `6a10f7af2` | noise half only | `1bd00dae5` keeps the pole-zero refusal |

**A bad provenance line, confirmed.** 68 `-x` trailers, 67 resolve. `efe72da1e` says `a1148c96938e9f3f4e78b3bd18daa07a70d5fc0e`: not an object. The real hash is `a1148c9699f58c32680344ab876ac459d8bb126e`. It can only be fixed by rewriting `ver_52` from that commit on, which I did not do. That is possible only before a push.

`git cherry` shows `+` for 7 picks (`b3594401c`, `8533ad9fb`, `c07143999`, `24f0f2314`, `b2f9fefa3`, `209f9bff6`, `472403238`): context or hand resolution changes the patch id, so for these the `-x` trailer is the evidence.

## 4. Release note

Grouped by what you will see. **[RESULTS]** marks a change in a number from a deck that used to run.

### Measurements

- **[RESULTS]** `meas avg` with `from=`, `to=` now uses exactly that window. Sky130 metrics moved 1.2e-5 to 1.9e-2 here. Monte Carlo metrics built on it will shift. It is a correction: a ramp from 2.1 to 7.3 with true average 4.7 gave 4.756 or 4.625 and now gives 4.700.
- **[RESULTS]** `meas rms` and `meas integ` on a descending dc sweep now work. They used to error or give 0.
- `meas sp` with a window no longer crashes, `meas sp` `rms` and `integ` now work, and `meas ac avg` of a real vector no longer crashes.
- New: `meas deriv` (also `derivative`), `meas integral` as another name for `integ`, `meas ac phase_margin`, `gain_margin`, `vdb()` and `vp()` in a `meas`.
- **Now an error:** `fft` of a vector longer than the plot's scale. Add `setscale`. `examples/various/FFT_tests.cir` shows how.
- `fft`, `fourier` and `deriv` of a short vector and over-long `meas` names give an error, not a memory overrun.
- **Known, not changed:** `avg` with `from=` skips the first sample in the window on tran and AC, as `rms` and `integ` always have. The error shrinks with the step, but it can be large on a kink.

### MOS and sky130

- **[RESULTS]** Sky130 FET `cgs` moved by at most 1.0e-4, AC and noise by at most 3e-6, transient by about 1e-7. `id`, `gm`, `gds`, `vth` are unchanged.
- **[RESULTS]** Devices with `sa`, `sb` or WPE on BSIM4: the stress mobility now applies. Sky130 nfet at sa=sb=0.3 **-8.7 %**, pfet **+13 %**. This hits post-layout netlists. Schematics with the default are unchanged.
- **[RESULTS]** Binning: a size on a bin edge now always takes the upper bin, however it is written. **Literal sizes with nf=1 under hsa do not move.** Sizes written as an expression, or decks without hsa, can step a lot on ss and ff (up to 66 % in `id`, one case 100 %), and on `nfet_03v3_nvt` at L=0.6 at every corner. Literal multi-finger sizes on an inner edge also take another card.
- Sizes on the table's outermost edge, and finger width 0.42 um, now load in all forms.
- **Now an error:** sizes up to 1 nm outside the table. They loaded before in some forms.
- **[RESULTS]** BSIM3 and BSIM4v5 capacitance fix also applies to any other card of those versions.

### Diodes

- **[RESULTS]** Sky130 forward transients that aborted with `Timestep too small` now finish: 13 of 45 before, 0 after. Fast transients move by up to 18.6 mV at edges. Some completed runs take much longer, in one case 15 s to 209 s.
- **[RESULTS]** Level 1 `ibv` scales with area. Level 3 with `w l m` no longer counts `m` twice. `isr` reverse current and conductance corrected. Soft recovery with `isr` moves under 1 %.
- **[RESULTS]** `area=0` or `m=0` used to switch a diode off quietly. It now warns and uses 1, so the diode conducts.
- AC and pole-zero with self-heating or `rsw` are now right.

### BJT, VBIC, VDMOS, MOS2, HFET, MESA

- **[RESULTS]** BJT: AC and noise carry `gdsub`, transient moves with `xcjc < 1` (2.3 % at a switching edge in one deck), `rco` with `area`, ISS temperature. Sky130: op unchanged, AC 1.5e-7, transient 4.9e-6 at most, bandgap 1.6e-8, `.sens :iss` 5 %.
- `bjt` `rco` with `qco` no longer stops a transient early. `vo=0` no longer fails.
- New `@q[ft]`, `@q[ikf]` and others.
- **[RESULTS]** VBIC: self-heating, PNP, excess phase, `xrb`, `xrc` corrected. `@q[is]` changes sign. PNP with `td` no longer aborts. Gain with self-heating 17.37 to 25.38.
- **[RESULTS, silent]** VBIC model `dtemp` is gone. A card with only that still gives the same current, but combined with instance `dtemp` or `temp` it now gives a different temperature with no message: 1.337e-3 against 1.047e-4 in one case. `@q[dtemp]` on the model now errors.
- **[RESULTS]** VDMOS: self-heating AC and op, p-channel polarity, quasi-saturation AC, `rg` noise (noise decks with `rg` move: 4.42e-11 to 2.27e-10). New SOA messages with `Pd_max` under `warn=1`.
- **[RESULTS]** MOS2 `gm`, `gds`, `gmbs`: up to 15.6 % better; `id` unchanged. HFET and MESA: forward device beside a reversed one is now right.

### Transient and sources

- PWL with `r=` no longer drops to 0 V for a timepoint on long clocks.
- Several PWL corners at one instant no longer stop a transient. `.option minbreak` is honoured by the `tran` command.
- **[RESULTS]** Default `minbreak` is now 1e-15. No change at ordinary IC time scales. Long transients with fast pulses move: `.tran 1m 1000`, -6.5 % timepoints, with some edges stepped over. `.option minbreak=` sets it; the review matched the old default with 1e-10 times the max step.
- A rejected step retries from the last accepted solution. Behavioural C and L with non-integer `m` work. CPL over 8 conductors gives an error, not a crash.

### OSDI

- **[RESULTS, silent]** Instance `temp` on an OSDI device is now Celsius, not Kelvin. Moves 273.15 with no message.
- Noise totals corrected: 1.4394e-5 to 1.2875e-5 in one test. Internal node names after collapse change, so a script reading `n1#i1` reads a different node.
- OSDI 0.4 and 0.5 files load. A failing model or instance is named. absdelay could not be run on this machine.
- **Known, still silent:** `.model pm nres(r=10k)` with no space before `(` loses `r` on an OSDI model. Write the space.

### Save, options, seeds, others

- `save none` and `.save none` work. `save v(1)` then `save none v(2)` keeps only V(2). `save v(typo)` still errors.
- **Known:** a node really called `none` cannot be saved by `save v(none)`.
- `option seed=N` and `option seed=random` in `.control` now seed the generator. `.option seed=` with no value warns where it crashed. A `.option seed=` in the deck wins at the next `reset`, with no message.
- Noise under `option klu` works and now matches Sparse, including `.sp` noise. Pole-zero under KLU stays refused.
- `.tf` with a current output gives the right output impedance where it was stuck at 1e20.
- XSPICE `s_xfer` with the wrong `int_ic` length now **errors**. `s_xfer` with a 1-long `den_coeff` no longer crashes. Switch AC gain is sane. Memristor `rinit` is used in ac and dc.
- `set cshunt_value=` in `.spiceinit` is now ignored without `.option cshunt`.
- A script indexing by position will notice new rows: `_rci` in BJT noise, `_rg` in VDMOS noise, `_ibex` in VBIC noise, `qpscale` in `.sens`.

## 5. What xschem must know

**Log**

1. The draw log format did not change: field order, header, footer, instance forms. 0 lines differ on the suite's two decks.
2. `setseed k` draws are unchanged, so stored seed baselines hold. The known exception is a device after `xhigh_po`, from ver_51.
3. `option seed=N` in `.control` now takes effect and logs `src=option offset=0`. On `reset` after `seed=random`, the record is `src=option` with a non-zero offset: it does not re-draw. `.option seed=random` still re-seeds per parse.
4. If xschem ever writes `option seed=` in `*ng_script` or `.spiceinit`, it still does nothing.

**Bin keys `xmN:model.K`**

5. Literal size, nf=1, hsa: **no change**. 48 sizes plus `drawlog_sky130`, bit-identical. This is what xschem writes.
6. Literal multi-finger, hsa, finger width on an inner edge: **key changes**, `xm1:nshort_model.29` to `.21` for w=15 l=0.5 nf=5. 3 rows of 17. Values and the next device are unchanged. `nfet_01v8`, `pfet_01v8`: finger 3 um at nf = 5 7 10 13 14 17 20 21 25 26 27 28 33 34 35 40; finger 7 um at nf = 7 11 14 17 19 22 25 28 31 34 37 38. Other devices: `measure/after-02/binkeys.tsv` and receipt 02.
7. Literal size on the top edge, or finger 0.42 um: was a load error, now loads with **new keys**. Corner case: a size on the top edge in one axis and an inner edge in the other keeps 2 cards, logging 3 extra rows and shifting the next device's draws (receipt 02's table does not say this; see section 8, g02).
8. Expression sizes: rows unchanged, 589 rows bit-identical, but the card a device uses moves: every edge size except the table's 4 corners. Listed per size in `measure/after-02/binkeys.tsv`.

**Numbers. Against the ver_51 binary xschem measures with**

9. FET `cgs` by up to 1.0e-4. `id`, `gm`, `gds`, `gmbs`, `vth`, `cgd`, `cbd` do not move.
10. Anything from `meas avg`: 1.2e-5, 8e-4, 1.7e-3, 1.9e-2 in the suite. An xschem census or MC metric using `avg` should be re-baselined. `meas rms` and `integ` on descending sweeps were wrong.
11. Post-layout decks with `sa`, `sb`: -8.7 %, +13 % in `id`.
12. Diode transients: results where there was an abort; up to 18.6 mV; some much slower. A timeout may need raising.
13. Bandgap: op, AC, tran identical; dc line at 1.6e-8 or less.

**The binary:** xschem still uses `build-ver_50`, untouched. Nothing here moves until it is pointed at `ver_52`.

## 6. Upstream defects worth reporting

Decks are in `upstream-decks/`, committed, exit codes in `upstream-decks/exits.txt`. `u28` has a placeholder PDK path; it was run with the real one. Each ran on the upstream binary from the triage (`68b0d57b3`; `refs/upstream/pre-master-48` is 4 commits later, touching none of these paths except one line of `cktsopt.c`), this binary and 45.2. Nothing has been sent.

### New in upstream's recent work (most useful to them)

| # | defect | from | on tip | `ver_52` | deck, fork fix |
|---|---|---|---|---|---|
| 1 | Size on the outermost bin max: `could not find a valid modelname`. Also, under hsa, a multi-finger size pruned in float loses its upper card | `03ef19244` | exit 1 | fixed | `u11-binning-outer-max.cir`, `tests/regression/model/binning-edges.cir`, `0f763b135` |
| 2 | **Sky130 no longer loads**: `; sign expected`, exit 1 | `940bb50fa`, inferred: `a1148c969` and tip both fail, and the triage found 36 lines of that shape. Not bisected | exit 1 | not taken | `u28-sky130-load.cir` |
| 3 | `DIOunsetup` leaves the node: rs=0, `altermod` rs=100, resistor stays shorted | `8533ad9fb` | -2.70830e-01 against -1.36781e-03 | fixed | `u12-diode-altermod-rs.cir`, `a291cdc7f` |
| 4 | `meas sp ... avg from=` exit 139; `meas ac avg` of a real vector exit 139 | `b277f48c0` series | 139 | fixed | `u01-meas-sp-window.cir`, `u02-meas-ac-real-let.cir`, `27d02928d` |
| 5 | `meas deriv` on a 2-point sweep reads index -1: 0.475, 0.5, 0.525, slope is 0.5 | `fb6ba949d` | wrong, no error | fixed | `u06-meas-deriv-two-points.cir`, `c957569c9` |
| 6 | `meas deriv` and `find` scan by the vector's length, not the scale's. Valgrind invalid reads | `fb6ba949d`, older for `find` | 8; `find` prints 2332 | **8, open** | `u07-meas-deriv-beyond-scale.cir` |
| 7 | `meas dc deriv` on an AC plot: exit 139, was a clean error | `fb6ba949d` | 139 | **139, open** | `u08-meas-dc-on-ac.cir`; `u08b-` for `find` |
| 8 | VDMOS `thermal`, Tj tied to ground: singular matrix, v(c) 4.245 and id 5.762 do not sum to 10 V / 1 ohm, exit 0 | `b3594401c` | yes | **open** | `u14-vdmos-tj-ground.cir` |
| 9 | VDMOS `ttt1` etc. not in `VDMOSmAsk`: `<<NAN, error = 7>>`; `ttt1` before `tt` silently lost: peak 1.36487 against 1.36727 | `b3594401c` | yes | **open** | `u15-vdmos-ttt1-ask.cir` |
| 10 | `save none` purge leaves `ci_dbs` dangling. Valgrind invalid reads, 0 on ver_50 and 45.2. One line: `ft_curckt->ci_dbs = dbs` | `b2f9fefa3` | 4 | **4, open** | `u16-save-none-uaf.cir` |
| 11 | `save v(typo)` runs silently and keeps only the scale, as the check went under `#if 0` | `b2f9fefa3` | yes | fixed | `u17-save-nosuch.cir`, `9551715a9` |
| 12 | `.print pz all` next to `.op`: no poles, no message, exit 0 | `b2f9fefa3` | yes | fixed | `u18-print-pz-all.cir`, `ec57c5d6f` |
| 13 | `save v(none)` on a node called `none`: scale only | `b2f9fefa3` | yes | **open** | `u19-print-save-none-node.cir` |
| 14 | Pole-zero under KLU: `input signal is shorted`; Sparse gives 3 poles. Cause not found | `6a10f7af2` | yes | refused | `u20-pz-klu.cir`, `1bd00dae5` |
| 15 | `setseed k; reset` leaves `$rndseed` at 1: `1 1 1`, 45.2 `1 2 3` | `95d7f2c2b` | yes | not taken | `u24-setseed-rndseed.cir` |
| 16 | `.option seed=random`: same sample on every reset, 8.94288e2 three times | `95d7f2c2b` | yes | not taken | `u25-seed-random-repeat.cir` |
| 17 | `.dc @m1[w]` restores through the set path, so `scale` applies again: next op -1.25000e-04 becomes -1.26010e-10. Sky130: no rebin, 12 % off | `af240ba02` | yes | reverted | `u26-dc-inst-param-scale.cir` |
| 18 | `(` ends a token: `r3 gnd(x) 0 1k` gets value 0 and "too small"; `V1 net(0) 0 1` stops | `58e4cf26b` | yes | not taken | `u27-paren-node.cir` |
| 19 | `examples/osdi/absdelay/absdelay.osdi` is built for AVX-512: illegal instruction on this CPU | `a62ccd27e` | | kept as upstream has it | the example |
| 20 | Removing the 100-iteration floor is undone by `cktntask.c`; `while (strchr(GARBAGE, *point))` has no NUL guard | `d7e64e28d`, `072980a57` | **by reading only** (triage) | not taken | none |

### Older, still there

| # | defect | tip, `ver_52`, 45.2 | deck |
|---|---|---|---|
| 21 | `meas sp ... when` and `trig/targ`: exit 139 | 139 on all | `u03-meas-sp-when.cir`, `u04-meas-sp-trig.cir` |
| 22 | `meas ac avg`, `rms` on an fft plot: exit 139 | 139 on all | `u05-meas-ac-fft-plot.cir` |
| 23 | `sens v(2) ac` under KLU: exit 139 | 139 on all | `u22-ac-sens-klu.cir` |
| 24 | `.sens` on a BJT: `bf` and `vaf` read 0 where central difference gives -2.62e-2 and 8.14e-3. `op` after `sens` gives 12.0 where it was 8.968: `sens` leaves the circuit altered | the same on all | `u21-bjt-sens.cir` (the deck runs `altermod` first; receipt 04's plain deck gave 1.02e-9 and 1.92e-9) |
| 25 | Diode level 3 `.sens`: `_m` -7.13e-5 against -2.25e-2, `_pj` off by 10; `op` after is altered. New `area must be positive` messages from `8533ad9fb` | receipt 05 | none minimal |
| 26 | `rsw` with no `ns` does not limit the sidewall: -1.64059e-03 against -1.09948e-03 with `ns=1`. `rsw=0` disconnects it: half the current | tip, `ver_52` | `u13-diode-rsw.cir` |
| 27 | `when` crossing in the first interval is not found | on all | `u09-meas-when-first-interval.cir` |
| 28 | `integ`, `rms` skip the first sample in the window on dc, ac, tran; `avg` too on ac and tran, from `b277f48c0`. Tip gives integ 0.075 (true 0.175), ac avg 0.1875 (true 0.4375); dc avg is right, 0.4375 | tip, `ver_52` | `u10-meas-avg-from-skips-sample.cir` |
| 29 | `tests/vbic` and `tests/polezero` are in `DIST_SUBDIRS`, not `SUBDIRS`. `FG`, `FO`, `temp` of vbic would pass now | also at upstream | `tests/Makefile.am` |
| 30 | VDMOS AC with quasi-saturation and self-heating: +2.6 % against numeric, n-channel m=1. Cause inferred | group 06 review | none minimal |
| 31 | VBIC `.sens` stops with `node allocation in DEVsetup()`. `DIOtF1SW` uses the temperature-adjusted mjsw, `dioload.c` the plain one (by reading). `--enable-sense2` BJT state range 0..20 misses slot 21 (by reading) | reviews | none |
| 32 | `a4ff5a204` leaves `node_ids`, `node_repr` unfreed on its error return (by reading, fatal path) | | none |

**Correction to receipt 03.** `--disable-xspice` failing at `cktsopt.c:254` is **not** for upstream. Upstream's tip no longer has `OPT_ENH_RSHUNT`: `a59bed240`, in the rshunt series not taken, removed it. It is the fork's to fix.

## 7. Unverified at close

- Shared library: `ngSpice_Reset` with the new `CKTrhsAccepted`, `save none` in libngspice, `option seed=` there. No shared build was linked or run in this batch.
- `--enable-sense2`, `--disable-sp`, MSVC, MinGW, `make dist`.
- OSDI absdelay, OSDI 0.3 and 0.5, `b2f00c8c3`'s trouble report.
- Which card is physically right on a bin edge where they differ. Other PDKs with gaps under 1 nm.
- The scratch-deck moves in section 2 were not re-measured on this binary. `compare.sh` shows the suite unchanged since each was taken, and the later groups' sky130 sweeps report 0 where they ran.
- xschem itself was not run against this binary.

## 8. Open review items

Nothing below has been fixed: no commit follows the reviews. **Marked items are regressions against ver_51.** I re-ran those with a deck above.

**Regressions against ver_51, for a decision:**

- **R1** `save none` use-after-free, `u16`, 4 invalid reads against 0. Narrow path, one-line fix (g07).
- **R2** `meas dc deriv` on an AC plot segfaults where it used to error, `u08` (g03).
- **R3** `meas deriv` invalid reads past the scale, and it can print a garbage number, `u07` (g03).
- **R4** VDMOS Tj on ground goes singular, `u14` (g06).
- **R5** `save v(none)` keeps only the scale, `u19` (g07).
- **R6** `avg from=` skips the first sample; a kink gives 0.1875 where ver_50 gave 0.5 and truth is 0.4375, `u10` (g01). Net better on smooth signals. Driver's call.

**g01 core**
1. `avg from=` first-sample skip: the receipt's "large net gain" does not hold on a kink. R6.
2. `meas sp ... when` and `trig/targ` segfault; the receipt lists only `when`. Not from this series.
3. `meas ac avg`, `rms` on an fft plot segfault. Not from this series.
4. Window narrower than a sample step on `.sp`: `integ` 0 and `from= nan`, `rms` "out of interval", `avg` 0. Past the data, the same.
5. `--disable-xspice` does not build, `cktsopt.c:254`. Fork's, see section 6.

**g02 binning**
6. Fallback pass in `subckt.c` can keep two cards under hsa, against the receipt's "exactly the card": `w=3 l=20.2` on `tt_mm`, xm2 vth 0.64218 against 0.63318. Were load errors before. No test.
7. Sizes 1 nm outside the table now error. Disclosed; owner should hear it directly.
8. The fork differs from upstream's rule on the outer max. Not reported upstream.
9. Not reproduced by the reviewer: pick-alone RED, the hsa literal "0 exactly" table in full, the `pfet_01v8_hvt` vth example.

**g03 measure**
10. Scale-length bounds: confirmed, not from the fix. R3. Suggested: bound by `min(d->v_length, dScale->v_length)`, for `find` too.
11. `meas dc deriv` on an AC plot, also reached with a 2-point sweep. R2.
12. `when` in the first interval: `deriv ... when` cannot succeed on a 2-point sweep. An error, not a wrong number.
13. RED not re-run by the reviewer; `measure/run.sh` not re-run by the reviewer. The second is now done here: 0 changed.

**g04 bjt**
14. Receipt swaps two labels in the `gp_tran` row: v(c) is 10.2 mV to 0.72 mV, v(b) 2.33 mV to 0.52 mV.
15. PSS shift, not in the receipt: `colpitt_osc_pss_ctrl.cir` converges at 2.9256 MHz, iteration 8, before; 3.3436 MHz, iteration 10, after. `vackar` near iteration 9 to 48. Likely a chaotic search under a tiny change; not attributed by experiment.
16. `examples/vdmos/100W.sp` 8th harmonic 2.78667e-5 to 2.81366e-5, 1 %, 7e-7 of the fundamental. Noise.
17. `--enable-sense2`: `BJTgeqcb` at slot 21 outside the saved 0..20. By reading.
18. The reviewer's `before` binary is the crew's, by provenance only.

**g05 diode**
19. `rsw=0` on level 3 disconnects the sidewall with no warning, `u13`. Not from the group.
20. `DIOunsetup` calls `CKTdltNNum` twice on one number with rs>0 and no rsw. Harmless, no action.
21. `ngSpice_Reset`, `--enable-sense2` unverified. Other devices' `unsetup`: a regex found no other device with the form.

**g06 vbic-vdmos**
22. Receipt says `DEVlimitlog` has 4 callers; 5, with `hicumL2.cpp:1782`. Results identical. Sky130 unaffected. NaN change reaches HICUM too.
23. VDMOS Tj on ground. R4.
24. `VDMOSmAsk` lacks `ttt1`, `ttt2`, `tm1`, `tm2`; `.sens` prints `GET ERROR` 6 times, `u15`.
25. `ttt1` before `tt` silently lost, `u15`.
26. VDMOS AC, quasi-saturation with self-heating, +2.6 %. Better than before.
27. VBIC `.sens` exits 1 before and after. VDMOS `.sens` runs; `id_max` sensitivity 2.76e-7 to 2.76e-106 from the changed default. Disto all zero before and after.
28. Release note lines for `@q1[is]` sign and `warn=1` SOA messages: added in section 4.
29. `tests/vbic` not in `make check`: `FG`, `FO`, `temp` could be wired in. Stale counts near the receipt's, not identical.

**g07 osdi-narrow**
30. `472403238`: receipt says only the 0.1f row moves. For `tstep <= 100n` 1e-15 is above the old default, and above 1e-5 it is below it, unmeasured there. PULSE `.tran 1m 1000`: 1439373 to 1345038 timepoints, -6.5 %. Edges stepped over at long times, mixed direction: 0 to 3 of 2561, 270 to 297 of 1285, 23 to 12 of 128. `MAX(1e-15, 10*delmin)` would keep long transients as they were, not built. Release note carries it.
31. `efe72da1e` wrong `-x` hash. Section 3.
32. `b2f9fefa3` use-after-free. R1.
33. `dd3a8dd15`: `set cshunt_value=1p` in `.spiceinit` now ignored. Release note carries it.
34. `SMPcaSolve` also moves `.sp` noise under KLU, to the right values: NF 2.163 to 3.40634. Release note carries it.
35. `save v(none)`. R5.
36. No test for `ec57c5d6f`, `1bd00dae5`, the 887/888 decks, noise under KLU.
37. Declined `58e4cf26b`'s bug stays, silent: `.model pm nres(r=10k)` runs at default 1k. Deserves a tracked fork issue. Release note carries it.
38. Not reproduced by the reviewer: `af240ba02` sky130 table, `58e4cf26b` "with the pick" column, IHP PSP103 no-move, valgrind on OSDI decks.

**g08 seed**
39. `.option seed=7` silently overrides `option seed=5` on `reset`. Upstream warned. Release note carries it. Owner's eye.
40. Warning quotes the parsed value: `5abc` prints `5e-18`, `"5"` prints `seed=5`. Cosmetic.
41. `option seed=5k` seeds 5000, `.option seed=5k` seeds 5 by `atoi`. `1e3`: 1000 and 1.
42. `option seed=5 seed=6` seeds 5: first wins. No test.
43. `.option seed=,`, `.option seed=5 seed=`, `.option rndseed=`: no crash, not the clean path.
44. `option seed=5` in `*ng_script` with a sourced circuit is ignored without a message.
45. `option reltol=2e-3 seed=` drops `reltol`, by reading.
46. libngspice, MSVC, MinGW not verified; measure decks not re-run by the reviewer. Now done here: 0 changed.

## For the driver

1. `resource-keyword-case.cmd`: commit the owner's edit, or keep the named FAIL. Owner's work, owner's call.
2. R1 to R6: fix on `ver_52`, or accept and carry.
3. `efe72da1e`'s trailer: rewrite before any push, or leave it.
4. Section 6: what to send upstream and when. The owner sends.
5. `.option seed=` override, `avg from=`, 1 nm sizes: the three behaviours the reviewers asked the owner to hear.
