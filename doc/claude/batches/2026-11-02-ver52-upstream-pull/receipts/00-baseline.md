# Receipt 00: baseline for ver_52

Branch `ver_52` at `a407e017c`, no picks yet. Worktree `/home/analog/dev/ngspice-ver_52`.
All heavy commands ran through `ngheavy` at `-j4` or less. No exit 137.

## Build

`autogen.sh`, `configure --prefix=$PWD/stage --with-readline=yes --enable-pss --enable-cider`
in `build-ver_52`, `make -j4`, `make install`: each exit 0. The binary reports `ngspice-46+`.

## make check: 363 PASS, 1 FAIL, 0 SKIP, against 364 PASS expected

Plain `make check` stops at the first failing directory (98 PASS, 1 FAIL). `make -k check`
ran all of it: **363 PASS, 1 FAIL, no SKIP, no XFAIL**, 364 tests in all, which matches the
expected total.

The failure is `tests/regression/pipe/resource-keyword-case.cmd`:

    ERROR: lowercase 'rusage task' printed no simulator stat, line = []

It is not a regression and not from this batch. The committed test assumes the simulator
statistic is on line 2 of the `rusage task` report. With `--enable-cider`, `printres()` takes
its list arm, which prints a blank line and then every statistic, so line 2 is empty. The owner
has an uncommitted edit of this test in `/home/analog/dev/ngspice` that scans for the marker,
and has carried it through the ver_51 batches (their receipts list it as left unstaged). So the
364 at ver_51's close were counted with that edit in the tree. A clean worktree does not have it.

Measured on this binary, in a scratch directory, with nothing changed in either tree:

| test version | exit |
|---|---|
| committed in `ver_52` | 1 |
| owner's working copy | 0, "all uppercase resource keyword cases passed" |

I did not commit the owner's edit or change the test here. **Open for the driver:** PLAN's gate
is 0 FAIL, and plain `make check` aborts in `tests/regression/pipe`, before the later suites run,
until this is settled. Options are to commit the owner's edit to `ver_52` (their work, their
call), or to have the gates use `make -k check` and accept this one named FAIL.

## Measurement suite: `measure/`

`run.sh <ngspice> <outdir>` writes one normalised result file per deck: `name = value` lines,
`@@` section labels, the bin chosen, exit status, counted warnings. It runs in a temp dir with
`HOME` set to it and the `.spiceinit` the PDK asks for, so the user's own setup cannot leak in.
Timing goes in `timing.txt` only. `compare.sh <A> <B> [reltol] [absfloor]` lists every changed
quantity with old value, new value and relative change, and exits 1 if anything changed
(defaults 1e-9 and 1e-30). `mc_*` and `drawlog_*` compare as exact text. Run it as
`ngheavy measure/run.sh <binary> <dir>`, then `measure/compare.sh measure/baseline <dir>`.

| deck | covers | lines |
|---|---|---|
| `fet_op` | nfet and pfet, 12 sizes each: 5 mid-bin, 6 on bin edges, 1 with 4 fingers; 3 biases (sat, linear, weak with body bias); id gm gds gmbs vth cgs cgd cbd; bin chosen per instance | 674 |
| `ac_noise` | self-biased common-source stage: gain and phase, adc, f3db, ugf, pm, noise spectrum, totals | 41 |
| `sens` | central differences of v(out) and id against bsim gm, then `sens v(out)` on 14 parameters | 39 |
| `tran_inv` | inverter chain: meas avg, rms, integ, rise, fall, delay; step counts; last time; PWL `r=` clock; dc-sweep meas ascending and descending | 33 |
| `bjt` | npn and pnp, forward, mid injection, reverse, at -40, 27, 125 C | 47 |
| `dio_res` | `diode_pw2nd_05v5` (id gd cd) and `res_xhigh_po` (generic and 0p35), +0.75 to -4 V, three temperatures | 92 |
| `bandgap` | pnp 1:8, xhigh_po, pfet mirror, ideal amplifier: v(ref) over temperature and line. v(ref) = 1.16498 V at 27 C | 44 |
| `mc_r` | `foreach k; setseed k; reset; op`, 12 seeds, agauss, gauss, aunif, compared with 45.2 | 73 |
| `mc_sky130` | the same loop, 6 seeds, `tt_mm`: 3 nfets, pfet, pnp, compared with 45.2 | 55 |
| `mc_xhigh_po` | the same loop, 4 seeds, nfet, xhigh_po, nfet, compared with baseline only (see below) | 25 |
| `drawlog_r`, `drawlog_sky130` | `set rnddraws=`: header, footer, rows. Sky130 deck: 2 nfets, pnp, xhigh_po, 33 rows | 16, 35 rows |
| `tran_perf` | five-stage ring oscillator: step counts and elapsed time | 14 |

Limit: `meas` stores about 7 significant digits, so a shift below roughly 1e-7 does not show in
a `meas` result. `tran_inv` and `tran_perf` also print means and a sampled point at full digits.

## Results

**Two runs identical.** `diff -r --exclude=timing.txt` of `measure/baseline` and a second run is
empty, and `compare.sh` at tolerance 0 reports `TOTAL changed: 0`. Elapsed for `tran_perf`:
wall 2.613 and 2.628 s, transient analysis 0.135 and 0.132 s. Most of the wall time is loading
the PDK. The whole suite takes about 40 s including the 45.2 runs.

**ver_52 matches the ver_51 binary.** `build-ver_50/stage/bin/ngspice` against the baseline at
1e-9: `TOTAL changed: 0`.

**45.2 draws: bit-identical.** `mc_r` (12 seeds) and `mc_sky130` (6 seeds), printed with
`numdgt=17`: `identical_to_ref = yes` for both. `mc_identity.txt` records this and `run.sh` exits
1 if it flips.

**Exception to PLAN rule 7, found here.** The rule says `setseed k` draws are bit-identical with
45.2. That does not hold for a device placed after a sky130 `xhigh_po`. In `mc_xhigh_po` under
45.2, seed 1:

| quantity | ver_52 | 45.2 |
|---|---|---|
| vth of the nfet before the resistor | same | same |
| head resistors rend1, rend2 | same | same |
| vth of the nfet after the resistor | 0.643695473082605 | 0.629105409652568 |
| v(r) | 1.22184410388e-02 | 1.24107149555e-02 |

So the stream moves inside the resistor, and later devices draw different numbers. The value
change is ver_51's P1 (the R-to-B rewrite now carries the model's TC), recorded in the
`2026-10-24-mc-v51` close. The fork's log has a row `b.xr1.brbody tc1 gauss`, which fits one extra
draw from that rewrite. That cause is inferred: 45.2 has no draw log to compare with. This exists
already on ver_51 and is not from this batch. `mc_sky130` therefore leaves the resistor out, and
`mc_xhigh_po` tracks it against the baseline.

**Draw log.** `drawlog_r`: 4 record sets, the last `seed=3 src=setseed offset=6`, each `logged=2
unlogged_calls=0`. `drawlog_sky130`: `seed=1 src=setseed offset=0`, `logged=33 unlogged_calls=0`,
instance forms `?`, `m.xm1.msky130_fd_pr__nfet_01v8`, `xm1:nshort_model.29`, `xr1`,
`r.xr1.rend1`, `c.xr1.xc0.c0`, `b.xr1.brbody`, `xr1:resbody`.

**Step counts** (baseline): `tran_perf` 22833 iterations, 8016 timepoints, 8015 accepted, 1
rejected. `tran_inv` 2655 timepoints, all accepted.

**Other baseline facts.**

- `tran_inv` descending dc sweep: `meas rms` errors with "out of interval" and `integ` gives 0.
  This is the bug `650ac305f` describes, so the `drms2` and `dint2` lines should change with that
  pick.
- `sens` emits repeated "Fatal error: BSIM4v5 ... Effective channel length <= 0" messages as it
  perturbs parameters, and still finishes. They are counted in the baseline, so a change shows up.

**The suite can see a change.** I edited a copy of the baseline: one last digit in `mc_r`, one
draw-log field, one bin name, and vth by 1e-15 relative. All were listed; the vth edit only at
tolerance 1e-15, as it should be. Running 45.2 itself reports 115 changes: `bandgap` 36,
`dio_res` 46, `mc_xhigh_po` 8, `tran_inv` 14, `tran_perf` 7, and `fet_op`, `ac_noise`, `bjt` and
`mc_*` none.

## Unverified

- Why `tran_inv` moves about 1e-3 between 45.2 and ver_51 (`irms` rel 1.3e-3, `tpd` 7.9e-4) when
  `fet_op` is identical. Between those two versions only, so not this batch's question. Not looked
  into.
- `dio_res` diode id moves 7.7e-7 relative between the same two versions. Not looked into.
- Sizes on bin edges were chosen by reading the sky130 limits; `bin.*` lines record the bin picked
  today. `03ef19244` is expected to change some of them.
