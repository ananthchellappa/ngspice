# Receipt 07: group 07, OSDI and narrow fixes

29 upstream commits judged. 27 are on `ver_52`; 1 was picked and reverted (`af240ba02`); 1 never landed (`58e4cf26b`). `6a10f7af2` is kept for noise only. Three fork commits close defects the picks brought. All heavy commands went through `ngheavy`, no exit 137.

## OSDI

`openvaf-r` v24.0.2mob is here, so 0.4 files were compiled and run. Probes: `resistor.va`, `tprobe`, `prange`, `iprange`, `coll`, `lim`, `ivar`, written for this; IHP `psp103.osdi`, built with the same compiler. Before is `b5a4bca2f`.

| upstream | on `ver_52` | result |
|---|---|---|
| `a4ff5a204` | `411d16678` | Names the culprit: `Error in model pm`, `Error in instance (or model for instance) n1`. Bad deck already stopped before. **Leaves `node_ids` and `node_repr` unfreed on that return**; fatal path, not fixed |
| `f1660838d` | `b344edc38` | Compiled; superseded |
| `a62ccd27e` | `0b11325b3` | **absdelay compiled, not run**, below |
| `9f398e8ae` | `6fd372b91` | 0.4 loads and runs, below. 0.5 not run. 0.3 not run: no such file here |
| `d9112d697` | `5425a7970` | Visual Studio project lines. Not built |
| `56d806eac` | `a1f2d9062` | 0.4 descriptor size 360 accepted |
| `24f0f2314` | `1bf0b994d` | Below |
| `0a8c29eec` | `564156870` | Below. **Release note** |
| `875513496` | `60c88eae4` | Below |
| `afe1de11a` | `9a8e434b6` | Warnings only |
| `b2f00c8c3` | `dd3171537` | **Compiled, not seen.** I could not make a limit diode fail, so no trouble report was printed |
| `9cf762afb` | `1e43f740f` | **Compiled, not run**: absdelay only |

Order is by ancestry: `a4ff5a204` (08-14) is older than `f1660838d`, though the task listed it later.

**No move on an OSDI model without the new features.** IHP PSP103 inverter, `numdgt=17`: op, AC, noise totals, transient, iterations and timepoints identical (832 points, 1768 iterations, `v(out) = 8.97572590331665809e-01`). Limit diode, transient: identical (3044 points, 6246 iterations). Valgrind on noise, collapse, `reset`, tran over 0.4 files: 0 errors.

**absdelay cannot run on this machine.** The compiler here emits 0.4 with no absdelay: `absdelay.va` gives `singular matrix: check node ndly2#implicit_equation_0`, before and after. The `absdelay.osdi` that `a62ccd27e` commits is 0.5 but built for AVX-512. On this CPU it stops in `setup_model_0` at `kmovd`, `internal error -- illegal instruction`, before and after. So the example shipped in `examples/osdi/absdelay/` crashes on a machine like this one; 26 KB binary, kept as upstream has it.

**`0a8c29eec`, for the release note.** Instance `temp` on an OSDI device is now Celsius. A deck written for Kelvin moves by 273.15 with no message. `tprobe` has I = V x T, so 1 V reads T:

| instance `temp=100` | before | after |
|---|---|---|
| device temperature, K | 100 | 373.15 |
| same deck, no instance `temp`, `.option temp=50` | 323.15 | 323.15 |

`grep temp=` over IHP's `sg13g2_moslv_mod.lib` finds no use of it; the other IHP libraries were not searched.

**`24f0f2314`.** Two 10 k resistors, v(2), 1 Hz to 1 MHz:

| quantity | before | after | truth |
|---|---|---|---|
| `inoise_total_n1_thermal` | 1.439446489e-05 | 1.287480080e-05 | `onoise_total_n1` / gain 0.5 = 1.287480080e-05 |
| `onoise_total_n1` | not reachable, name ended in a blank | 6.437400398e-06 | sqrt(sum) of the two gives `onoise_total` |
| `onoise_total`, `inoise_total` | 9.103858950e-06, 1.820771790e-05 | same | |

**`875513496`.** `coll` with `r1=0` collapses `i1`: names `n1#i1 = 0.8333`, `n1#i2 = 0.5` become `n1#i2 = 0.8333`, `n1#i3 = 0.5`. By hand i2 is 5/6 of the drop, so the new names are right. **A script reading the old names gets different nodes.** No collapse: unchanged.

## Narrow fixes

| upstream | on `ver_52` | verdict and measurement |
|---|---|---|
| `58e4cf26b` | none | **Declined.** See below |
| `af240ba02` | `92511d009`, reverted `fa5e0691f` | **Dropped.** See below |
| `935d85633` | `7d2345736` | Taken. `save @n1[k]`, OSDI integer: before 59 x `OUTpData: unsupported data type` and 0 points; after 59 points, value 3 |
| `6a10f7af2` | `77c6e09dc`, with `1bd00dae5` | **Noise half only.** See below |
| `dd3a8dd15` | `69fdfbc4b` | Taken. Second deck, no cshunt: before `1 capacitors added`, after none. No conflict with the fork's `cp_remvar` work |
| `2d76fefd6` | `56f56e4a7` | Taken. `Allowed_Types: [d, int]` and an empty string array: ver_50 `cmpp` refuses, this one builds `ifspec.c`. Generated model not run |
| `52634b164` | `b25802a11` | Taken. `int_ic` of 3 for `den_coeff` of 2: ran on ver_50, now stops with the error, exit 1. **A deck with the wrong length used to run** |
| `797409238` | `915fa8fc2` | Taken. `den_coeff=[1]`: ver_50 exit 139, now v(2) = 2 x v(1) |
| `b2f9fefa3` | `90e3e7bd2`, with `9551715a9`, `ec57c5d6f` | Taken, hand port. See below |
| `f6d11dbbd` | `13a0612ea` | Taken. `dc Vin 0 2 1`, `rinit=7k`: ver_50 `-inf`; now -1.42857e-04, -2.85714e-04. Op, AC, tran same |
| `613ba2ea5`, `a1148c969` | `106be1d38`, `efe72da1e` | Taken. `Makefile.am` resolved to `analog digital case casedist`. `switch-ac.cir` on ver_50: `regression failed`; here PASS |
| `472403238` | `c024fc365` | Taken. See below |
| `2f62073b3` | `46eb120d9` | Taken. By reading: the factor became 0 or 1. Run with 1.5, no error, value not read back |
| `1889179f7` | `ed2f238ab` | Taken. `make dist` not run |
| `ac2798364` | `4c049a9b6` | Taken. `gnuplot` of tran to 1 and tran to 5: 60 stray blank lines in `gp.data`, now 1. No `gnuplot` binary here, data file only |
| `023c9cbda` | `55651aed3` | Taken. **Not moot**: `613ba2ea5` adds the generated `Makefile.in`, this removes it |

Autotools rerun with `autogen.sh` and `configure` after the `Makefile.am` picks.

### `58e4cf26b`, declined

RED on the bug it fixes: `.model pm prange(R=2)` drops `R`. `i(v1)` is -1 where the spaced card gives -0.5; picked, both -0.5. Built-in cards never had the bug, the frontend splits them.

Tested first with the pick applied, uncommitted:

| deck | before | with the pick |
|---|---|---|
| `V1 a(b) 0 1`, `R1 a(b) c 1k` | runs, v(c) 0.5, node `a(b` | `unknown parameter (1)`, stops |
| `V1 net(0) 0 1` | runs | stops |
| `R3 gnd(x) 0 1k` | 1 k to ground | **`Value of resistor r3 is too small`: x to gnd at 1e-12** |

The last is a silent change of circuit for a card that has no other error. So it breaks what the fork accepts and it is declined. The OSDI card without a space still loses its first parameter, **the bug remains open**; a fix would belong in the `.model` path, not in the token routine.

### `af240ba02`, dropped by `git revert`

Works on a plain deck and an OSDI one, against `alter`: `dc @m1[w] 1u 5u 1u` gives -3.75000001e-04 at 3u, `alter` the same. `@d1[area]`, `@r2[r]`, nested with V1, `@n1[w]` right too. It fails where sensitivity work would use it:

| deck | sweep | truth |
|---|---|---|
| sky130 nfet, `w=2 l=0.5`, sweep to 5 | -1.69881620e-04, bin .29 | -1.82757428e-04, bin .13 |
| Same, sweep to 8 | -2.68406470e-04, bin .29 | -3.04534833e-04, bin .5 |
| `.option scale=1e-6`, after a sweep, `@m1[w]` | 1e-12 | 1e-6 |
| Same, next `op`, `i(vd)` | -1.26010e-10 | -1.25000e-04 |
| sky130, next `op` | Fatal: effective channel width | runs |
| Failing sweep, `CKTtrouble` | `= 3.45846e-323`, read through the wrong struct | a value |

It does not rebin, and it restores through the set path, which applies `scale` again. Fixing that is a project; a sweep that says nothing and is 12 % off is worse than none. It also needed a lint marker.

### `6a10f7af2`, noise half

Noise under `option klu`, common-emitter: before refused; after `onoise_total = 2.442291007e-04`, `inoise_total = 3.304025180e-04`, equal to Sparse at 10 digits, also at one spot frequency. So the adjoint fix is right.

Pole-zero is not. `tests/polezero/filt_multistage.cir` and that stage under KLU: `The input signal is shorted on the way to the output`; Sparse gives poles and zeros. Not understood; `filt_rc`, `pz2` did run. `1bd00dae5` restores the old `use option sparse` refusal. RED: the message above; after, the refusal, 4 times of 4.

### `b2f9fefa3`, hand port

Three conflict hunks: keep `eqc` for the keywords, extended to `none`, and `name_eq_query`. Not large, so taken.

| deck | before | after |
|---|---|---|
| `save none`, `.save none`, tran | `no data saved`, aborted | runs; `time` of 1 point |
| `delete all`, `save v(2)`, run | 508 points | 508 points |
| `save NONE`, then `save v(2)` | `V(2)` kept | same |
| `save v(1)`, `save none v(2)` | V(1), V(2) | V(2) only, stated purpose |

Upstream's change broke two things here, fixed as fork commits:

- `9551715a9`. Upstream puts the `no data saved` check under `#if 0`. `save v(nosuch)` then runs and keeps only the scale, without a word. Now guarded by `!savenone`. RED, as picked: `plot after a name that is nowhere: tran2`; now `tran1`, with the error. New test `tests/regression/misc/save-none.cir`, RED on before at `points after save none:` with no value.
- `ec57c5d6f`. Upstream dropped the exception for `all` from the duplicate check. `tests/polezero/filt_bridge_t.cir` (`.OP`, `.PZ`, `.PRINT PZ ALL`) then reached `no data saved for pole-zero analysis`. Now poles -2.61803e+03, -3.81966e+02, zeros -1.00000e+03, as before. **`tests/polezero` is in `DIST_SUBDIRS` and not `SUBDIRS`, so `make check` does not run it and would not have caught this.** I found it by chance. Run by hand, all 6 of that directory fail on before and after with the same diff counts, on stale print formats; the pole and zero rows are identical before and after. No reference touched.

**Known limit, left as upstream has it.** A node named `none`: `save v(none) v(2)` now loses `none` and keeps V(2). `none` is matched after `v()` is stripped. A fix touches `outitf.c` too.

Shared library: the path the bug report is about was not built or run.

### `472403238`, taken

Bug 887 (read on SourceForge): several PWL corners at one instant stop a tran with `Timestep too small`; result depends on absolute time. Bug 888: `.option minbreak` ignored by a `tran` command, because `CKTnewTask` does not copy it. The fix sets 1e-15 and copies it.

| deck | before | after |
|---|---|---|
| Bug 887 deck, `vecmax(time)` | stops, 1.000e-03, `time = 0.001, timestep = 0` | 1.002e-03 |
| Bug 888, `tran` command, `minbreak=1e-15` | stops | 1.002e-03 |
| Bug 888, `.tran` and `run` | 1.002e-03 | 1.002e-03 |

The triage expected it to move every tran. **Measured, it does not at normal scales**: breakpoints merge only when closer than `minBreak`, and the old default was `10 x delmin` (`dctran.c`). Sky130 inverter, `numdgt=17`, `v(o)` at a probe time at 6 digits, iterations exact:

| `.tran` | timepoints before, after | iterations before, after |
|---|---|---|
| 1p 5n | 5038, 5038 | 10607, 10607 |
| 10n 200u | 20082, 20082 | 40213, 40213 |
| 100n 2m | 20058, 20058 | 40204, 40204 |
| 0.1f 2p, edges of 10 fs | **20032, 20008** | **40063, 40015** |

Value at the probe time, 2.55720, equal. Only the last row moves, where steps come near 1e-15 s. `measure/` at tolerance 0: `TOTAL changed: 0`. `optran.c` and `dcpss.c` also read `minBreak`; optran and PSS decks are covered only by `make check`.

## Gates

- `make` 0 after each round, `make install` 0; installed binary is the tested one (sha256 `9c62b67da137...`).
- Full `make -k check`: **370 PASS, 1 FAIL, 0 SKIP**. The FAIL is `resource-keyword-case.cmd`, the baseline's named exception. The 2 over group 06 are `switch-ac.cir`, `save-none.cir`. `identity.lint`, `selftest.lint` PASS, `identity.baseline` untouched. No `.out` regenerated; one new, for the new test.
- `measure/run.sh`: exit 0, `mc_r` and `mc_sky130` `identical_to_ref = yes`. Saved in `measure/after-07/` (96 KB).
- Against `after-06`: **`TOTAL changed: 0`** at 1e-9 and at 0, `mc_*` and `drawlog_*` exact. Nothing to attribute. No deck in the suite uses OSDI, switches, `s_xfer`, a memristor, KLU or `save none`, so the 0 says sky130 did not move. The numbers above are the evidence for the picks.

## Unverified

- absdelay at all: `a62ccd27e`, `9f398e8ae`'s 0.5 path, `9cf762afb`. OSDI 0.3 files.
- `b2f00c8c3`: no trouble report provoked.
- `save none` in the shared library, `ngSpice_Reset`.
- Pole-zero under KLU: why it fails.
- `make dist`, Visual Studio, a code model using `2d76fefd6`, gnuplot's own rendering.
- Before for code models means ver_50's `stage`, since the `.cm` files are installed, not linked: `f6d11dbbd`, `797409238`, `52634b164`, `a1148c969`.
