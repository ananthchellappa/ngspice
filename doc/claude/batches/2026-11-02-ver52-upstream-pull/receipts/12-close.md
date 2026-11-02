# Receipt 12: final close of `ver_52`

Measured at `a20ddf99a`, the tip before this receipt. All heavy commands went through `ngheavy` at `-j4`. No exit 137.
Nothing was pushed or posted. This replaces sections 4 and 5 of receipt 09, and its section 6 list.

**Regressions against ver_51: not none. Four remain, none a crash and none a change in a schematic's sized-FET results. See section 6, items R1 to R4.**

- **R1** One diode op case that takes the last-resort path runs 13 times slower (16 s to 218 s).
- **R2** A size up to 1 nm outside the PDK table now stops with a load error. It used to load without hsa or as an expression.
- **R3** VDMOS `.sens` prints 6 `GET ERROR` lines, and `ttt1` written before `tt` is lost where ver_51 warned.
- **R4** `meas` accuracy: three narrow cases are further from truth than ver_51. They come with the rule chosen in group 10b, which is nearer on nearly every row. The largest on sky130 is `drms`, 3.83e-5 to 6.40e-5.

"ver_51" below is `build-ver_50/stage/bin/ngspice`. Re-run here, it matches `measure/baseline` with `TOTAL changed: 0`, so the baseline and xschem's binary are the same thing.

## 1. Clean rebuild and `make check`

| step | exit |
|---|---|
| `make clean` | 0 |
| `make -j4` | 0 |
| `make install` | 0 |
| `make -k check` | 2, from the one FAIL below |

Binary: `build-ver_52/src/ngspice` equals `stage/bin/ngspice`, sha256 `d56b9022f3c8cfaa...`.

**394 PASS, 1 FAIL, 0 SKIP, 0 XFAIL, 395 tests.** The FAIL is the named `tests/regression/pipe/resource-keyword-case.cmd`, which needs the owner's uncommitted edit. The log's two `ERROR:` lines are that FAIL's own message and `(internal) tried to destroy non-existent graph` in a passing test, which predates every pick (receipt 09).

`ver_51..HEAD` before this receipt: 113 commits, of which 68 upstream picks (one reverted), 26 fork code commits including the revert, 19 docs. `src/`: 130 files, +3995, -1540.

**New tests since receipt 09, 19** (376 to 395):

| test | commit |
|---|---|
| `regression/misc/save-none-circuits.cir` | `3b4ed4b2c` |
| `regression/misc/save-none-node.cir`, `-cards`, `-print` | `c607ae3ea` |
| `regression/misc/save-none-node-diff.cir`, `-diff-print` | `5fed2d161` |
| `regression/misc/meas-dc-on-ac.cir` | `1c7253057` |
| `regression/misc/meas-scale-length.cir` | `8be9bfcdd` |
| `regression/misc/meas-window-exact.cir` | `ef6445218`, reworked in `875ddd529` |
| `regression/misc/meas-when-length.cir` | `6a8cfa3d5` |
| `regression/misc/meas-when-on-ac.cir` | `9b87d50bc` |
| `regression/misc/meas-margin-length.cir` | `1a0955462` |
| `regression/misc/minbreak-default.cir`, `minbreak-pwl-corners.cir` | `6fa5af6cc` |
| `regression/model/vdmos-tj-ground.cir`, `-soa` | `3301d5cae` |
| `regression/model/binning-edges-draws.cir` | `d1d5feff5` |
| `regression/rng/option-seed-control-netlist-random.cir`, `-unusable` | `15e53439c` |

## 2. Sky130: what moved against ver_51

`measure/run.sh`, exit 0, saved in `measure/after-12/`.

| compared with `after-12` | tolerance | TOTAL changed |
|---|---|---|
| `after-10b` | 0 | 0 |
| `after-09` | 1e-9 | 8 |
| `after-11` | 1e-9 | 11 |
| `baseline` (ver_51) | 1e-9 | 74 (`after-09`'s 73 plus `drms`) |
| `baseline` (ver_51) | 0 | 95 |

### Seen by the suite

"Error" is against the fine-step reference of receipt 10b.

| deck | quantity | ver_51 | ver_52 | relative move | error, ver_51 to ver_52 | commit |
|---|---|---|---|---|---|---|
| `fet_op` | `cgs`, 24 devices, 2 biases | e.g. -2.91518e-15 | -2.91489e-15 | 3.3e-6 to 1.0e-4 | | `f961c98d0` |
| `ac_noise` | phase at the top frequency | 0.0632499 | 0.0632501 | 3.0e-6 | | `f961c98d0` |
| `ac_noise` | noise spectrum, worst | 3.11496e-9 | 3.11495e-9 | 2.5e-6 | | `f961c98d0` |
| `tran_inv` | `tpd`, `m_idd`, `m_o1`, `m_o2` | | | at most 1.5e-7 | | `f961c98d0` |
| `tran_perf` | `m_idd`, `m_n1`, `s_n1` | | | at most 1.1e-6 | | `f961c98d0` |
| `tran_perf` | `iavg`, ring | -2.253363e-4 | -2.253407e-4 | 2.0e-5 | 7.42e-4 to 7.22e-4 | `b277f48c0`, `875ddd529` |
| `tran_inv` | `iavg` | -5.878067e-6 | -5.864154e-6 | 2.4e-3 | 1.24e-3 to 1.13e-3 | `875ddd529` |
| `tran_inv` | `ioavg` | 1.489977 | 1.488738 | 8.3e-4 | 8.2e-4 to 1.07e-5 | `63d0fcf9d`, `875ddd529` |
| `tran_inv` | `davg`, dc up | 0.8566408 | 0.8581184 | 1.7e-3 | 1.70e-3 to 2.03e-5 | `7c2ebe189`, `875ddd529` |
| `tran_inv` | `davg2`, dc down | 0.8421223 | 0.8581184 | 1.9e-2 | 1.86e-2 to 2.03e-5 | `7c2ebe189`, `875ddd529` |
| `tran_inv` | **`drms`, dc up** | 1.202470 | 1.202593 | 1.0e-4 | **3.83e-5 to 6.40e-5, further** | `875ddd529`: R4 |
| `tran_inv` | `dint2`, dc down | 0 | 1.029742 | was wrong | to 2.04e-5 | `650ac305f`, `875ddd529` |
| `tran_inv` | `drms2`, dc down | error | 1.202593 | was an error | to 6.40e-5 | `650ac305f`, `875ddd529` |
| `bandgap` | `vr`, line sweep, 6 points | 1.145893371744 | 1.145893371717 | 2.4e-11, below tolerance | | `c07143999` |

**Not moved**, tolerance 0: `id`, `gm`, `gds`, `gmbs`, `vth`, `cgd`, `cbd` on every `fet_op` device and bias; `bjt`, `dio_res`, `sens`; bandgap v(ref) over temperature; step counts on all decks; `mc_r`, `mc_sky130`, `mc_xhigh_po`, both draw logs. `irms`, `vrms`, `vint` of `tran_inv` equal ver_51's digits.

### Re-measured on this binary, off the suite

The suite has no `sa`/`sb` deck, no size written as an expression, and no diode transient. So these were run again here, against ver_51.

Bin edges: every edge size that changes card per `measure/after-02/binkeys.tsv`, 3 biases, worst relative change in `id`. "expr" means hsa with an expression. No hsa gave the same numbers as expr on every row.

| device | form | tt | ss | ff |
|---|---|---|---|---|
| `nfet_01v8` | expr, no hsa | 7.3e-3 | **7.6e-1** | **3.2e-1** |
| `pfet_01v8` | expr, no hsa | 3.0e-3 | **3.3e-1** | **2.8e-1** |
| `pfet_01v8_hvt` | expr, no hsa | 2.0e-4 | **1.0** | **6.1e-1** |
| `nfet_03v3_nvt`, L=0.6 | expr, no hsa | **2.5e-1** | **2.4e-1** | **2.6e-1** |
| all four | hsa literal, nf=1 | **0 exactly** | **0 exactly** | **0 exactly** |
| `nfet_01v8`, multi-finger literal, 5 sizes | hsa | 7.2e-9 | 5.7e-3 | 7.9e-3 |
| `pfet_01v8`, multi-finger literal, 5 sizes | hsa | 5.7e-11 | 1.8e-2 | 1.8e-2 |

- The literal row is 12 decks: 144 points each, 24 on `nfet_03v3_nvt`, none differing. It leaves out sizes on the top edge, which are a load error on ver_51.
- Multi-finger sizes: `l=1 w=15 nf=5`, `l=0.5 w=15 nf=5`, `l=1 w=21 nf=7`, `l=1 w=49 nf=7`, `l=0.5 w=21 nf=7`. Only those five, so a larger move on another device is possible.
- Receipt 02's wider run, 5 biases and 9 devices, reached 79 % on `nfet_03v3_nvt` and 100 % on `pfet_01v8_hvt`. The worst `nfet_01v8` point here is 76 % against its 66 %, at a weak bias it did not use.

| other | ver_51 | ver_52 | commit |
|---|---|---|---|
| nfet w=1 l=0.15 sa=sb=0.3, vg=vd=0.9 | 2.522516e-05 | 2.302594e-05, **-8.7 %** | `3e8a21ae6` |
| pfet, same size, source at 1.8, vg=vd=0.9 | 1.855270e-05 | 2.055862e-05, **+10.8 %**; receipt 01's own bias +13 % | `3e8a21ae6` |
| `pd2nw_05v5_lvt` 125 C, 0.6 V through 1 ohm: elapsed | 16.5 s, 44.6M iterations | **218.5 s**, 695.6M iterations | `149688cd3`: R1 |
| Same: v(a) | 0.5969486 | 0.5969507, 3.5e-6 | |

Not repeated here, from their groups: the diode abort count, 13 of 45 to 0; diode fast transients up to 18.6 mV; the BJT rows. `compare.sh` shows the suite unchanged across the groups since.

## 3. Confirmations

| claim | result | how |
|---|---|---|
| `setseed k` bit-identical with 45.2 | **yes** | `mc_r`, `mc_sky130` `identical_to_ref = yes`. By hand, k = 1, 2, 3, 7, 100, 2147483647, `sgauss`, `sunif`, `rnd`, `numdgt=17`: 24 lines, `cmp` equal. `setseed7-bit-identity` PASS |
| Known exception | device after a sky130 `xhigh_po` | ver_51's P1, not this batch. `mc_xhigh_po.txt` `cmp` equal to baseline |
| Draw log contract | **holds** | `drawlog_r.tsv`, `drawlog_sky130.tsv`, and their `.txt`, `cmp` equal to baseline. `logged=33 unlogged_calls=0`. `rnddraws-*` PASS |
| `git diff ver_51 HEAD -- tests/lint/identity.baseline` | **empty** | 0 lines; nothing under `tests/lint` differs |
| No pre-batch test or `.out` modified | **yes** | `git diff --name-status ver_51 HEAD -- tests`: the only `M` are `regression/misc`, `regression/model`, `regression/rng` and `xspice` `Makefile.am`. Every changed reference named in receipts 10, 10b, 11 is on a deck new in this batch |
| Owner's checkout untouched | **yes** | `ver_51`, `df2194696`; status the same 2 lines (`CLAUDE.md`, `resource-keyword-case.cmd`); no file outside `.git` newer than 2026-11-02 00:00 |
| `build-ver_50` binary unchanged | **yes** | dated 2026-10-26 11:49, sha256 `c2bd1098ce5ff7e5...` |
| No RF commit | **none** | 74 RF hashes: no `-x` trailer, not an ancestor. No RF, HB, nport or Touchstone file in the diff |
| None of the not-taken | **none** | No pick since receipt 09: 0 `cherry picked` lines in `5d80f945e..HEAD`. Arrowplot 7, solver and tools 5, CMake 3, rshunt, `940bb50fa`, `d7e64e28d`, `072980a57`, `95d7f2c2b`, `059066b53`, `628d84aa5`, `cbb09dc25`, `58e4cf26b`: no trailer. The eight named: not ancestors |
| Two named exceptions in the triage rows | | `f1660838d`, a triage NO, picked in group 07 for the OSDI chain, which depends on it (receipt 07). `613ba2ea5` (receipt 09). Neither is on PLAN's not-taken list |

## 4. Upstream

### Receipt 09's section 6, against this binary

`tip` is the triage's upstream binary, `68b0d57b3`; the 4 commits to `ac2798364` touch none of these paths except one line of `cktsopt.c`. Exit codes: `upstream-decks/exits.txt`, regenerated.

| 09's # | now in the fork | how |
|---|---|---|
| 1, 3, 4, 5, 11, 12 | fixed, as receipt 09 said | re-run, values match |
| 6 | **now fixed**, `8be9bfcdd` | `u07` invalid reads: tip 8, ver_52 0 |
| 7 | **now fixed**, `1c7253057` | `u08`, `u08b`: tip 139, ver_52 0 |
| 8 | **now fixed**, `3301d5cae` | `u14`: tip singular, ver_52 v(c) 3.500527, id 6.499473 |
| 10 | **now fixed**, `3b4ed4b2c` | `u16` invalid reads: tip 4, ver_52 0 |
| 13 | **now fixed**, `c607ae3ea`, `5fed2d161` | `u19`: tip scale only, ver_52 `none` 59 long |
| 22 | **now fixed**, shared scale read, `ef6445218` | `u05`: tip, ver_51, 45.2 all 139, ver_52 0 |
| 28 | **now fixed in the fork** | `u10`: tip 0.075, 0.1875; ver_52 0.175, 0.4375 |
| 14 | refused in the fork | `u20` |
| 2, 15, 16, 17, 18, 20 | commit not taken or reverted, so not on the fork | `u28`, `u24`, `u25`, `u26`, `u27` |
| 9 | **open** | `u15` |
| 21 | **open**, as on ver_51 and 45.2 | `u03`, `u04` exit 139 |
| 23 | **open**, as on ver_51 and 45.2 | `u22` exit 139 |
| 24, 26, 27 | **open**, same output as ver_51 | `u21`, `u13`, `u09` |
| 19, 25, 29, 30, 31, 32 | open, unchanged | see below |

### The corrected list worth sending, most useful first

Nothing has been sent. Every row with a deck was re-run on tip for this list. `tests/regression/...` decks are this fork's and run as they are. The fork's rule for `integ`, `rms` and `avg` is deliberately not offered.

**Blocks or breaks a whole class of deck**

| # | defect | from | on tip | deck |
|---|---|---|---|---|
| 1 | **Sky130 does not load**: `; sign expected`, exit 1. Inferred from `940bb50fa`, not bisected | `940bb50fa` | exit 1 | `u28-sky130-load.cir`, set the path |
| 2 | A legal size on the outermost bin max: `could not find a valid modelname`. Under hsa, a multi-finger size pruned in float loses its upper card too | `03ef19244` | exit 1 | `u11-binning-outer-max.cir`, `tests/regression/model/binning-edges.cir` |
| 3 | `(` ends a token: `r3 gnd(x) 0 1k` gets value 0, `V1 net(0) 0 1` stops | `58e4cf26b` | warns, wrong | `u27-paren-node.cir` |

**Silently wrong numbers**

| # | defect | from | on tip | deck |
|---|---|---|---|---|
| 4 | `.dc @m1[w]` restores through the set path, so `scale` applies again: next op -1.25000e-04 becomes -1.26010e-10 | `af240ba02` | yes | `u26-dc-inst-param-scale.cir` |
| 5 | VDMOS `thermal`, Tj on ground: singular, v(c) 4.245 and id do not sum to 10 V / 1 ohm, exit 0. `VDMOSselfheat` wants `tempNode > 0`; setup treats 0 as connected | `b3594401c` | yes | `u14-vdmos-tj-ground.cir` |
| 6 | `DIOunsetup` leaves the node: rs=0, `altermod` rs=100, resistor stays shorted: -2.70830e-01 against -1.36781e-03 | `8533ad9fb` | yes | `u12-diode-altermod-rs.cir` |
| 7 | **`integ` and `rms` skip the first sample inside a `from=` window, on dc, ac and tran; `avg` too on ac and tran.** Over 1.3 to 1.7 of `u10`'s data, where the samples give integ 0.175 and avg 0.4375: tip integ 0.075, ac avg 0.1875; dc avg 0.4375. Described as the defect only | older; `avg` from `b277f48c0` | yes | `u10-meas-avg-from-skips-sample.cir` |
| 8 | `save v(typo)` runs and keeps only the scale; the check went under `#if 0` | `b2f9fefa3` | yes | `u17-save-nosuch.cir` |
| 9 | `save v(none)`, `.print ... v(none)`, `.save v(1,none)` on a node called `none`: scale only | `b2f9fefa3` | yes | `u19-print-save-none-node.cir`, `tests/regression/misc/save-none-node-diff.cir` |
| 10 | `.print pz all` next to `.op`: no poles, no message, exit 0 | `b2f9fefa3` | yes | `u18-print-pz-all.cir` |
| 11 | `meas deriv` on a 2-point sweep reads index -1: 0.475, 0.5, 0.525 where the slope is 0.5 | `fb6ba949d` | yes | `u06-meas-deriv-two-points.cir` |
| 12 | Default `minbreak` flat at 1e-15: long transients change. Default no longer equals 1e-13: 150370 against 162254 points, and `.option minbreak` cannot then undo it. `MAX(1e-15, 10*delmin)` kept bugs 887 and 888 fixed and long runs as they were | `472403238` | yes | `tests/regression/misc/minbreak-default.cir`, `minbreak-pwl-corners.cir` |
| 13 | Pole-zero under KLU: `input signal is shorted`; Sparse gives 3 poles. Cause not found | `6a10f7af2` | yes | `u20-pz-klu.cir` |

**Seed**

| # | defect | from | on tip | deck |
|---|---|---|---|---|
| 14 | `setseed k; reset` leaves `$rndseed` at 1: `1 1 1`, 45.2 `1 2 3` | `95d7f2c2b` | yes | `u24-setseed-rndseed.cir` |
| 15 | `.option seed=random`: same sample on every reset, 1.123222e3 three times | `95d7f2c2b` | yes | `u25-seed-random-repeat.cir` |

**Crashes and memory, new work**

| # | defect | from | on tip | deck |
|---|---|---|---|---|
| 16 | `meas sp ... avg from=` and `meas ac avg` of a real vector: exit 139 | `b277f48c0` series | 139 | `u01-meas-sp-window.cir`, `u02-meas-ac-real-let.cir` |
| 17 | `meas dc deriv` on an AC plot: exit 139, was a clean error | `fb6ba949d` | 139 | `u08-meas-dc-on-ac.cir` |
| 18 | New here: `meas dc deriv ... when` on an AC plot: exit 139, was "not supported" on 45.2 | `fb6ba949d` | 139 | `u30-meas-deriv-when-on-ac.cir` |
| 19 | `save none` purge leaves `ci_dbs` dangling, one line to fix: `ft_curckt->ci_dbs = dbs` | `b2f9fefa3` | 4 invalid reads, 0 on 45.2 | `u16-save-none-uaf.cir` |
| 20 | `meas deriv` and `find` scan by the vector's length, not the scale's | `fb6ba949d`, older for `find` | 8 invalid reads, 1 on 45.2 | `u07-meas-deriv-beyond-scale.cir` |
| 21 | New here: `meas phase_margin`, `gain_margin` read past a short scale, and return a margin at a frequency from the heap | `c0d49a574` | 1 invalid read | `tests/regression/misc/meas-margin-length.cir` |

**Older, also on 45.2**

| # | defect | on tip | deck |
|---|---|---|---|
| 22 | New here: `meas dc when` on an AC plot: exit 139. Also tran, `find ... when`, `trig/targ` | 139 | `u29-meas-when-on-ac.cir`, `tests/regression/misc/meas-when-on-ac.cir` |
| 23 | `meas dc find` on an AC plot: exit 139 | 139 | `u08b-meas-dc-find-on-ac.cir` |
| 24 | `meas sp ... when` and `trig/targ`: exit 139 | 139 | `u03-meas-sp-when.cir`, `u04-meas-sp-trig.cir` |
| 25 | `meas ac avg`, `rms` on an fft plot: exit 139 | 139 | `u05-meas-ac-fft-plot.cir` |
| 26 | `sens v(2) ac` under KLU: exit 139 | 139 | `u22-ac-sens-klu.cir` |
| 27 | New here: `meas when` loops by the vector's length, not the scale's. Tip printed `w1 = 4.98e-310` | 2 invalid reads | `u31-meas-when-short-scale.cir` |
| 28 | `.sens` on a BJT: `bf`, `vaf` read 0 where central difference gives -2.62e-2 and 8.14e-3. `op` after `sens` gives 12.0 where it was 8.968 | yes | `u21-bjt-sens.cir` |
| 29 | `rsw` with no `ns` does not limit the sidewall: -1.64059e-03 against -1.09948e-03 with `ns=1`. `rsw=0` halves the current | yes | `u13-diode-rsw.cir` |
| 30 | VDMOS `ttt1` etc. not in `VDMOSmAsk`: `<<NAN, error = 7>>`, `GET ERROR` under `.sens`; `ttt1` before `tt` lost: peak 1.36487 against 1.36727. From `b3594401c` | yes | `u15-vdmos-ttt1-ask.cir` |
| 31 | New here: `from=` with a sign or an exponent reaches the window rounded to 7 digits: `from=1.23456789e-1` gives 1.061728400, `from=0.123456789` gives 1.061728394. Small | yes | `u32-meas-window-rounding.cir` |
| 32 | `when` crossing in the first interval is not found | yes | `u09-meas-when-first-interval.cir` |

**Not ready to send, no deck:** VDMOS AC with quasi-saturation and self-heating, +2.6 %; diode level 3 `.sens`; VBIC `.sens` stopping in `DEVsetup()`; `DIOtF1SW` mjsw; `--enable-sense2` slot 21; `a4ff5a204` leak; the `d7e64e28d` and `072980a57` readings. `tests/vbic` and `tests/polezero` missing from `SUBDIRS` needs no deck, only a note. `examples/osdi/absdelay/absdelay.osdi` being built for AVX-512 is one line, no deck.

**Not for upstream:** `--disable-xspice` (the fork's own), the fallback pruner `d1d5feff5` (it cannot be reached on tip because of item 2).

## 5. Final release note and list for xschem

### 5a. Release note

Grouped by what you will see. **[RESULTS]** marks a change in a number from a deck that used to run.

**Measurements**

- **[RESULTS]** `meas avg`, `integ` and `rms` with `from=`, `to=`: the window is now honoured and its first sample is used. An edge between samples is interpolated and the part step to the first sample is added as a trapezoid. Samples inside use the same rule as before, Simpson where steps are equal. `avg` is now `integ` over the width, on every plot.
  - On a dc sweep `avg` used to ignore `to=`: `from=1.2 to=1.6` printed `to=2.0`.
  - Sky130 metrics moved: `avg` 2.0e-5 to 1.9e-2, `rms` 1.0e-4. Monte Carlo metrics built on `avg` will shift.
  - It is a correction nearly everywhere. Three cases go the other way, under Known.
- **[RESULTS]** `meas integ`, `rms` and `avg` on a descending dc sweep now work and match the ascending sweep. They used to give 0, an error, or another number.
- A window inside a single step now gives a value. It used to give 0 or an error.
- `meas sp` with a window no longer crashes; `meas sp` `rms`, `integ` work. `meas ac avg` of a real vector and `meas ac avg`, `rms` on an fft plot no longer crash.
- **Now refused:** `meas dc` or `meas tran` with `deriv`, `find`, `when`, `trig/targ` on a plot that is not real (an AC plot). Most used to crash. `meas tran x when v(5)=0.5` on an AC plot used to print `x = 0`, and now prints an error and no value.
- **Now an error:** `meas` `rms` or `integ` of a vector whose length differs from the scale, as `avg` always was.
- `meas deriv`, `find`, `when` and margins no longer read past a short scale.
- New: `meas deriv` (also `derivative`), `meas integral`, `meas ac phase_margin`, `gain_margin`, `vdb()` and `vp()` in a `meas`.
- **Now an error:** `fft` of a vector longer than the plot's scale. Add `setscale`. `examples/various/FFT_tests.cir` shows how.
- `fft`, `fourier` and `deriv` of a short vector and over-long `meas` names give an error, not a memory overrun.
- **Known:**
  - `avg` across a corner on equal steps now carries Simpson's overshoot: a tent from 1.2 to 1.6 gives 0.6667, where the straight line through the samples gives 0.5 and ver_51 gave 0.5.
  - An adaptive RC `integ` is 8.10e-6 off where it was 7.51e-6, mostly the simulator's own error.
  - The inverter's dc `drms` is 6.40e-5 off where it was 3.83e-5.
  - `from=0` at the end of a descending sweep misses by rounding (`-1.5e-15`), so its last step is a trapezoid and it differs a little from up: rms 0.700713 against 0.700639.
  - `meas sp ... when` and `trig/targ` still crash, on `.sp` and AC plots, as they always have.

**MOS and sky130**

- **[RESULTS]** Sky130 FET `cgs` moved by at most 1.0e-4, AC and noise by at most 3e-6, transient by about 1e-7. `id`, `gm`, `gds`, `vth` are unchanged.
- **[RESULTS]** Devices with `sa`, `sb` or WPE on BSIM4: the stress mobility now applies. Sky130 w=1 l=0.15, sa=sb=0.3, nfet **-8.7 %**, pfet **+11 %**. This hits post-layout netlists. Schematics with the default are unchanged.
- **[RESULTS]** **Bin edges.** With expression sizes or no hsa, a device exactly on a bin edge now uses the upper card.
  - On some corners and devices that moves `id` by tens of percent, because the PDK's cards don't agree there. Worst seen: `nfet_01v8` 76 % at ss and 32 % at ff; `pfet_01v8_hvt` 100 % at ss and 61 % at ff; `nfet_03v3_nvt` at L=0.6, 25 % here and up to 79 % in the wider run, at every corner. At tt, on the first three devices, under 1 %.
  - Literal hsa nf=1, which xschem writes, doesn't move: 0 of all points, tt, ss, ff.
  - Literal multi-finger sizes on an inner edge change card too: `id` up to 1.8 % at ss and ff on the five sizes measured, about 1e-8 at tt. Which card is physically right is not known; the rule now gives the same card however the size is written.
- Sizes on the table's outermost edge, and finger width 0.42 um, now load in all forms.
- **Now an error:** sizes up to 1 nm outside the table, which used to load without hsa or as an expression. `l=20.2005` gave a result on ver_51 and is now `could not find a valid modelname`.
- **[RESULTS]** BSIM3 and BSIM4v5 capacitance fix also applies to any other card of those versions.

**Diodes**

- **[RESULTS]** Sky130 forward transients that aborted with `Timestep too small` now finish: 13 of 45 before, 0 after. Fast transients move by up to 18.6 mV at edges.
- **Slower:** one op that falls through gmin and source stepping to the transient-op ramp went from 16 s to 218 s (`pd2nw_05v5_lvt`, 125 C). Its value moves 3.5e-6. A timeout may need raising.
- **[RESULTS]** Level 1 `ibv` scales with area. Level 3 with `w l m` no longer counts `m` twice. `isr` reverse current and conductance corrected. Soft recovery with `isr` moves under 1 %.
- **[RESULTS]** `area=0` or `m=0` used to switch a diode off quietly. It now warns and uses 1, so the diode conducts.
- AC and pole-zero with self-heating or `rsw` are now right. `altermod` of `rs` on a diode that had none now takes effect.

**BJT, VBIC, VDMOS, MOS2, HFET, MESA**

- **[RESULTS]** BJT: AC and noise carry `gdsub`, transient moves with `xcjc < 1` (2.3 % at a switching edge in one deck), `rco` with `area`, ISS temperature. Sky130: op unchanged, AC 1.5e-7, transient 4.9e-6 at most, bandgap 1.6e-8, `.sens :iss` 5 %.
- BJT `rco` with `qco` no longer stops a transient early. `vo=0` no longer fails. New `@q[ft]`, `@q[ikf]` and others.
- **[RESULTS]** VBIC: self-heating, PNP, excess phase, `xrb`, `xrc` corrected. `@q[is]` changes sign. PNP with `td` no longer aborts. Gain with self-heating 17.37 to 25.38.
- **[RESULTS, silent]** VBIC model `dtemp` is gone. A card with only that gives the same current, but combined with instance `dtemp` or `temp` it now gives a different temperature with no message: 1.337e-3 against 1.047e-4 in one case. `@q[dtemp]` on the model now errors.
- **[RESULTS]** VDMOS: self-heating AC and op, p-channel polarity, quasi-saturation AC, `rg` noise (noise decks with `rg` move: 4.42e-11 to 2.27e-10). New SOA messages with `Pd_max` under `warn=1`.
- VDMOS with Tj on ground works, a junction held at 0 C, and gives ver_51's op and AC. New `ttt1`, `ttt2`, `tm1`, `tm2`.
- **Known:** those four cannot be read back, `.sens` on VDMOS prints 6 `GET ERROR` lines, and `ttt1` written before `tt` is lost with no message. Write it after. SOA with Tj held by a 0 V source stays silent.
- **[RESULTS]** MOS2 `gm`, `gds`, `gmbs`: up to 15.6 % better; `id` unchanged. HFET and MESA: forward device beside a reversed one is now right.

**Transient and sources**

- PWL with `r=` no longer drops to 0 V for a timepoint on long clocks.
- Several PWL corners at one instant no longer stop a transient (bugs 887, 888). `.option minbreak` and `option minbreak=` are honoured by the `tran` command.
- Default `minbreak` is `10*delmin`, as on ver_51, with a floor of 1e-15. Long transients are as on ver_51: `.tran 1m 1000`, 1439373 timepoints on both. **[RESULTS]** Only runs with very small steps meet the floor: `.tran 0.1f 2p`, 20032 to 20008 timepoints, value equal.
- A rejected step retries from the last accepted solution. Behavioural C and L with non-integer `m` work. CPL over 8 conductors gives an error, not a crash.

**OSDI**

- **[RESULTS, silent]** Instance `temp` on an OSDI device is now Celsius, not Kelvin. Moves 273.15 with no message.
- Noise totals corrected: 1.4394e-5 to 1.2875e-5 in one test. Internal node names after collapse change, so a script reading `n1#i1` reads a different node.
- OSDI 0.4 and 0.5 files load. A failing model or instance is named. absdelay could not be run on this machine.
- **Known, still silent:** `.model pm nres(r=10k)` with no space before `(` loses `r` on an OSDI model. Write the space.

**Save, options, seeds, others**

- `save none` and `.save none` work. `save v(1)` then `save none v(2)` keeps only V(2). `save v(typo)` still errors. A node called `none` can be saved: `save v(none)`, `.save v(none)`, `.save v(1,none)`, `.print`, `.meas`.
- `option seed=N` and `option seed=random` in `.control` now seed the generator.
- `.option seed=` with no value warns where it crashed.
- A `.option seed=` in the deck wins at the next `reset`, and when that overrides an `option seed=` a warning now says so, once per session.
- Noise under `option klu` works and now matches Sparse, including `.sp` noise. Pole-zero under KLU stays refused.
- `.tf` with a current output gives the right output impedance where it was stuck at 1e20.
- XSPICE `s_xfer` with the wrong `int_ic` length now **errors**. `s_xfer` with a 1-long `den_coeff` no longer crashes. Switch AC gain is sane. Memristor `rinit` is used in ac and dc.
- `set cshunt_value=` in `.spiceinit` is now ignored without `.option cshunt`.
- A script indexing by position will notice new rows: `_rci` in BJT noise, `_rg` in VDMOS noise, `_ibex` in VBIC noise, `qpscale` in `.sens`.
- `--disable-xspice` builds again.

### 5b. What xschem must know

**Log**

1. The draw log format did not change: field order, header, footer, instance forms. `cmp` equal on the suite's two decks.
2. `setseed k` draws are unchanged, so stored seed baselines hold. The known exception is a device after `xhigh_po`, from ver_51.
3. `option seed=N` in `.control` now takes effect and logs `src=option offset=0`. On `reset` after `seed=random`, the record is `src=option` with a non-zero offset: it does not re-draw. `.option seed=random` still re-seeds per parse.
4. If xschem ever writes `option seed=` in `*ng_script` or `.spiceinit`, it still does nothing.
5. **New message:** a deck with both `option seed=` and `.option seed=` prints a 4-line `Warning: '.option seed=...'` once per session. Draws are unchanged. A script counting warnings will see it.

**Bin keys `xmN:model.K`, and which card a device uses**

6. Literal size, nf=1, hsa, which is what xschem writes: **no change**. Keys bit-identical, `id` 0 lines differ on four devices at tt, ss, ff.
7. Literal multi-finger, hsa, finger width on an inner edge: **key and card change**.
   - `xm1:nshort_model.29` to `.21` for w=15 l=0.5 nf=5, 3 rows of 17. Draw values and the next device are unchanged.
   - `id` moves with the card: up to 1.8 % at ss and ff, about 1e-8 at tt, on five sizes.
   - `nfet_01v8`, `pfet_01v8`: finger 3 um at nf = 5 7 10 13 14 17 20 21 25 26 27 28 33 34 35 40; finger 7 um at nf = 7 11 14 17 19 22 25 28 31 34 37 38. Other devices: `measure/after-02/binkeys.tsv` and receipt 02.
8. Literal size on the top edge, or finger 0.42 um: was a load error, now loads with **new keys**. A size on the top edge in one axis and an inner edge in the other now keeps one card, like any other (`d1d5feff5`); receipt 09's "3 extra rows" no longer holds.
9. **Expression sizes, or no hsa, exactly on a bin edge: the device now uses the upper card.** Log rows are unchanged, but on ss, ff and on `nfet_03v3_nvt` at L=0.6 `id` moves by tens of percent, up to 100 %, because the PDK's cards don't agree there. Every edge size moves except the table's 4 corners; listed per size in `measure/after-02/binkeys.tsv`. xschem does not write this form, but a hand-edited deck or `.param` size does.
10. A size up to 1 nm outside the table, as an expression or without hsa, is now a load error.

**Numbers, against the binary xschem measures with**

11. FET `cgs` by up to 1.0e-4. `id`, `gm`, `gds`, `gmbs`, `vth`, `cgd`, `cbd` do not move.
12. Anything from `meas avg`: 2.0e-5, 8.3e-4, 1.7e-3, 2.4e-3, 1.9e-2 in the suite. An xschem census or MC metric using `avg` should be re-baselined. On a dc sweep `avg` now honours `to=`, so a windowed dc metric can move far more than these.
13. `meas rms` and `integ`: 1.0e-4 on the suite's dc sweep. Its three transient windows (`from=2n`, `2.5n`) kept ver_51's digits, but a window that used to lose its first sample moves, and on a corner by a lot: `u10` integ 0.075 to 0.175. On descending sweeps they were wrong.
14. A `meas` on a plot that is not real now gives no value, where it gave `x = 0` or crashed.
15. Post-layout decks with `sa`, `sb`: -8.7 %, +11 % in `id`.
16. Diode transients: results where there was an abort; up to 18.6 mV; one op case 13 times slower. A timeout may need raising.
17. Bandgap: op, AC, tran identical; dc line at 1.6e-8 or less.

**The binary:** xschem still uses `build-ver_50`, untouched. Nothing here moves until it is pointed at `ver_52`.

## 6. All open items

**Regressions against ver_51: four, R1 to R4.** None is a crash. Everything else in this section is not a regression.

| | item | evidence | call needed |
|---|---|---|---|
| **R1** | Run time. Diode op after gmin and source stepping fail: 44.6M to 695.6M iterations | 16.5 s to 218.5 s, this binary. `149688cd3`, bisected in receipt 05, why not established | Accept, as the same pick ends 13 of 45 aborts, or look into the ramp |
| **R2** | A deck that loaded now stops: size up to 1 nm outside the table, no hsa or expression | `l=20.2005`: -6.62e-07 on ver_51, load error here. `03ef19244`, kept out on purpose in receipt 02 | Owner to hear it |
| **R3** | VDMOS: `.sens` prints 6 `GET ERROR` (`tm1`, `tm2`), and `ttt1` before `tt` is lost silently where ver_51 warned `unrecognized parameter`. Numbers on that card equal ver_51's | `u15`; a `.sens` deck run on both. `b3594401c` | Small fix, not done here: this job does not change code |
| **R4** | `meas` further than ver_51 in places, under the rule the driver chose in 10b. Sky130 `drms` 3.83e-5 to 6.40e-5. `g10` RC integ 7.51e-6 to 8.10e-6, and on 30 of 30 random RC windows; on the true curve the new rule is nearer, and with tight tolerances the gap falls to 6e-9. `avg` on a corner on equal steps: 0.5 to 0.6667. Individual smooth windows too: `tran100` 2.373m to 2.514m rms 1.88e-8 to 1.67e-4, by Simpson's phase; new worse on rows of 7 of 32, 4 of 32, 2 of 35. For rms the worst new error per signal group is no larger than the worst old | Receipt 10b, review of it, section 2 | Accepted by that choice, stated so it can be undone |

**Behaviour changes that can surprise, by design, in the release note:** VBIC model `dtemp` gone, silent. OSDI `temp` Celsius, silent. Bin-edge cards for expression sizes. `fft` of a long vector errors. `s_xfer` `int_ic` length errors. `cshunt_value` in `.spiceinit` ignored. OSDI node names. `area=0` warns and uses 1. New noise and `.sens` rows.

**`meas`**

1. `from=0` at the end of a descending sweep: `dc v1 3.2 0 -0.1` ends at -1.5e-15, outside the 100-ulp test. rms 0.700713 down, 0.700639 up. `from=-3 to=0` returns 1.08e-15 going down and fails going up. So "a sweep down gives the same number as up" does not hold at 0. No `from`/`to`: equal to all digits.
2. A window edge inside a step on a coarse curved sweep inherits the chord error, by the rule's design. `.sp` 24.75 MHz steps, 120 to 125 MHz: avg -0.48056, closed form -0.48331, 5.7e-3.
3. `meas-window-exact.cir`'s tran rms checks use a 0.5 % band, on purpose. Exact protection there is the 6-digit `.out`.
4. `meas sp ... when` and `trig/targ`: exit 139 on `.sp` and AC plots, as on ver_51 and 45.2. `u03`, `u04`.
5. `from=`, `to=` with a sign or an exponent are rounded to 7 digits. Confirmed on this binary, ver_51, 45.2 and tip: `u32`.
6. No overlap: `integ` 0 and `from= nan`; `avg` 0; `rms` "out of interval". As on ver_51, except `to=` of `integ` now also prints `nan`.
7. `when` in the first interval is not found, and `deriv ... when` cannot succeed on a 2-point sweep. `u09`. Error, not a wrong number.
8. Which rule is nearer on real circuit data with kinks: three dc metrics say it depends on the window's start. `s11m` has no reference.

**Binning**

9. Fork differs from upstream's rule on the outer max. In section 4 as item 2.
10. Which card is physically right at a bin edge. `pfet_01v8_hvt` ss L=0.5 W=0.42: vth 0.859 to -0.184 between `.46` and `.45`. It looks like a card problem. Not looked into.
11. hsa with `.options wnflag=0`: the pruner divides by nf, the binner does not. By reading, from before this batch.
12. Not reproduced by a reviewer: pick-alone RED, the `pfet_01v8_hvt` vth example. The literal "0 exactly" is now re-measured in section 2.

**Diode, BJT, VBIC, VDMOS**

13. `rsw` with no `ns`; `rsw=0` disconnects the sidewall with no warning, as on ver_51. `u13`.
14. `DIOunsetup` calls `CKTdltNNum` twice on one number with rs>0 and no rsw. Harmless.
15. Receipt 04's labels swapped in `gp_tran`: v(c) 10.2 mV to 0.72 mV, v(b) 2.33 mV to 0.52 mV. The reviewer's `before` binary there is the crew's, by provenance only.
16. PSS shift, re-measured: `colpitt_osc_pss_ctrl.cir` 2.9256 MHz at iteration 8 on ver_51, 3.3436 MHz at iteration 10 here; the deck predicts 3.30435 MHz, so it moved nearer. `vackar` does not converge on either, nearest iteration 9 and 48. Not a regression, not attributed by experiment.
17. `examples/vdmos/100W.sp` 8th harmonic 2.78667e-5 to 2.81366e-5, 7e-7 of the fundamental. Noise.
18. `--enable-sense2`: `BJTgeqcb` at slot 21 outside the saved 0..20. By reading.
19. Receipt 06 says `DEVlimitlog` has 4 callers; there are 5. Results identical.
20. VDMOS AC, quasi-saturation with self-heating, +2.6 %. Better than ver_51.
21. VBIC `.sens` exits 1 before and after. VDMOS `id_max` sensitivity 2.76e-7 to 2.76e-106 from the changed default. Disto all zero before and after.
22. SOA with Tj held by a 0 V source stays silent, ver_51 too. `.sens` BJT, `u21`, as on ver_51.
23. `tests/vbic` and `tests/polezero` not in `make check`. `tests/transient` not in it either; `fourbitadder.cir` there fails identically on ver_51.

**OSDI, narrow, tran**

24. `efe72da1e`'s `-x` line names a wrong hash; the right one is in the ledger. Fixable only by a rewrite before a push.
25. No test for `ec57c5d6f`, `1bd00dae5`, or noise under KLU.
26. `.model pm nres(r=10k)` runs at default 1k, silent. Deserves a tracked fork issue.
27. Not reproduced by a reviewer: `af240ba02` sky130 table, `58e4cf26b` "with the pick", IHP PSP103 no-move, valgrind on OSDI decks.
28. `minbreak` misses 270 of 1285 and 23 of 128 edges: ver_51's own. The flat 1e-15 was better on the second, 12. Not a cure, not worse.
29. The bare `option` printout has no `minbreak` line, on ver_51 too. `.option rshunt` without XSPICE is ignored silently, on ver_51 too.

**Save and seed**

30. `status` prints a node called none as `save none`. Differential `save v(1,2)` in control language gives `no data saved`, `.meas ... v(1,3)` `no such vector`, on 45.2 and ver_51 too.
31. Warning quotes the parsed value: `5abc` prints `5e-18`, `"5"` prints `seed=5`. Cosmetic.
32. `option seed=5k` seeds 5000, `.option seed=5k` seeds 5 by `atoi`. `1e3`: 1000 and 1.
33. `option seed=5 seed=6` seeds 5: first wins. No test.
34. `.option seed=,`, `.option seed=5 seed=`, `.option rndseed=`: no crash, not the clean path.
35. `option seed=5` in `*ng_script` with a sourced circuit is ignored with no message.
36. `option reltol=2e-3 seed=` drops `reltol`. Run now, no longer by reading: `Error: seed equals what?`, reltol stays 0.001, on ver_51 too.

**Closed since receipt 09:** its items 1, 3, 5, 6, 10, 11, 13, 23, 28, 30, 32, 35, 39, 46, and its R1 to R6. Its items 33 and 34 are carried by the release note, and 21 by the list below.

### Unverified at this close

- Shared library: `ngSpice_Reset` with `CKTrhsAccepted`, `save none` and `DB_SAVENONE`, `option seed=`, `seed_explicit`, the `add_bkpt()` order in `dctran.c`. No shared build was linked or run in this batch.
- `--enable-sense2`, `--disable-sp`, MSVC, MinGW, `make dist`. `make check` on `--disable-xspice`.
- OSDI absdelay, OSDI 0.3 and 0.5, `b2f00c8c3`'s trouble report.
- Other PDKs with gaps under 1 nm.
- Scratch-deck moves not repeated on this binary: listed at the end of section 2.
- xschem itself was not run against this binary.

## For the driver

1. R1 to R4: accept and carry, or fix. R3 is small; R1 is a look into an upstream pick.
2. `resource-keyword-case.cmd`: the owner's edit, the owner's call.
3. `efe72da1e`'s trailer: rewrite before any push, or leave it.
4. Section 4: what to send, and when. The owner sends.
