# Receipt 03: group 03, measure

All 6 picks landed clean with `git cherry-pick -x`, no hand resolution, none dropped. No fork code was added.
All heavy commands went through `ngheavy`. No exit 137.

| upstream | on `ver_52` | what |
|---|---|---|
| `fb6ba949d` | `2a2358403` | `meas deriv`, `.tf` current-output impedance, loop index in AC sens under KLU |
| `c235cf8a2` | `c0d49a574` | `vdb()` and friends in auto-save, `meas phase_margin`, `gain_margin` |
| `89ad00e07` | `a0d6b999b` | meas error buffer, `deriv`, `fourier`, `fft` length guards, ternary NULL guards |
| `35d29240f` | `e37258986` | `out_line` writes bounded |
| `68cd5f139` | `be090c420` | example `FFT_tests.cir` sets a scale |
| `81f5b2c6d` | `2035dc70c` | same, in `paranoia_parallel/` |

## The memory bugs, under valgrind

**before** is `ver_52` at `bd1e69584`, **after** is `2035dc70c`. Same decks: an RC, `.tran 10u 3m`, 312 points, then the line shown.
valgrind 3 with a full error log. Binary has no debug symbols, so the stacks show addresses; the before/after count is the evidence.

| line in `.control` | before | after |
|---|---|---|
| `let x = deriv(vecmin(v(2)))` | 4 invalid accesses, 1 invalid **write**, `ft_polyfit failed` 300+ times, exit 1 | 0 invalid, `x = 0`, exit 0 |
| `fourier 1k deriv(vecmin(v(2)))` | 4 invalid, 1 write, exit 1 | 0 invalid, "fourier needs at least 2 data points (got 1)", exit 0 |
| `let x = fft(1)` | 1 invalid write, exit 1 | 0 invalid, "fft/ifft needs an input vector of length >= 2 ...", exit 0 |
| `let x = fft(vector(5))` | 1 invalid write, exit 1 | 0 invalid, `length(x) = 3`, exit 0 |
| `meas tran b avg v(<360 char name>) from=0 to=1m` | `*** stack smashing detected ***`, exit 134 | error message, runs on, exit 0 |
| `meas tran a avg v(2) <360 char name>=1` | stack smashing, exit 134 | error message, runs on, exit 0 |
| `let x = deriv(vector(1))`, `fourier 1k vecmin(v(2))` | clean | clean |

Without FFTW, a separate scratch build (`--with-fftw3=no --disable-osdi`), where `before` differs from `after` only in `cmath4.c`:
`fft(vector(5))`, `fft(vector(2))`, a 3-point scale, `ifft` of 3 points. Before: 1 invalid write, exit 1. After: 0 invalid, lengths 5, 5, 5, 8, exit 0; a 4-point vector on that 3-point scale gets the new error.
A `.meas` name of 1200 characters in a batch deck: `out_line` is 1000 bytes. Before: the printed line is 1250 characters, so `sprintf` wrote 250 bytes past the buffer. valgrind does not see stack writes and nothing crashed, because `out_file[1000]` sits next to it. After, `35d29240f`: the line is cut at 999, the value is lost from the print, nothing is overrun.

## New features, against numbers worked by hand

| deck | expected | measured |
|---|---|---|
| RC step, tau 1 ms: `deriv v(2) at=1m` | exp(-1)/tau = 367.879 | 367.880 |
| same, `derivative v(2) when v(2)=0.63212` | 367.879 | 367.879 |
| same, `at=99` | out of interval | "out of interval", failed |
| `integral v(2) from=0 to=1m` (new alias of `integ`) | 3.67879e-4 | 3.67879e-4 |
| 2/(1+s/w)^3, 1 kHz: `phase_margin v(out)` | 67.60 deg at 766.42 Hz | 67.5986 at 766.415 |
| same: `gain_margin v(out)` | 12.041 dB at 1732.05 Hz | 12.0415 at 1732.06 |
| RC, no crossing | error | "no unity-gain crossover", "no -180 deg phase crossover" |

## Gates

- `make` 0, `make install` 0. Full `make -k check`: **365 PASS, 1 FAIL, 0 SKIP**, same as group 02. The FAIL is `resource-keyword-case.cmd`, the baseline's named exception.
  `identity.lint`, `selftest.lint` PASS. `identity.baseline` untouched, no `.out` changed. The new `cieq` calls all have a literal operand.
- `measure/run.sh`: exit 0, `mc_r` and `mc_sky130` `identical_to_ref = yes`. Saved in `measure/after-03/` (92 KB).
- Against `after-02`: **0 quantities changed** at 1e-9, `mc_*` and `drawlog_*` exact. `compare.sh` prints `TOTAL changed: 1`, and that one is `binkeys.tsv`, a file group 02 added by hand, not a quantity.
  Against `baseline`: 73, the same 73 as groups 01 and 02.

So nothing to attribute in the suite. The suite has no `.tf` on a current, no `deriv` measure, no margin, no short vector. What does move, measured separately:

| what | old | new | commit | reason |
|---|---|---|---|---|
| `.tf i(vm) v1`: `vm#Output_impedance`, 1k + 2k in series | 1e+20 | 3000, the right value | `fb6ba949d` | clamp pinned it. Voltage output and gain unchanged |
| `meas deriv` | "currently not supported" | a number | `fb6ba949d` | stated purpose |
| `fft(a)`, `a` longer than the plot's scale | ran | **error** | `89ad00e07` | see below |
| `fourier` of a scalar | "lengths don't match" | "needs at least 2 data points" | `89ad00e07` | message only |

## A deck that used to work and now errors

`fft` of a vector longer than the plot's scale is now refused. The old `examples/various/FFT_tests.cir`, `let a = vector(16)` and `fft(ac)` with no `setscale`, ran clean under valgrind before (7 FFTs, 0 invalid). On `after` it gives 7 "needs an input vector ..." errors. That is why upstream changed the example in `68cd5f139`. The new example: 7 FFTs, 0 invalid on both. Accepted because it is the commit's stated rule, but an owner's script of this shape needs `setscale`.

## Unverified, and one defect left in

- **New segfault path, left as is:** `meas dc d deriv v(out) at=5` on an AC plot, exit 139. `measure_deriv_at()` exempts `dc` from its `v_realdata` check. Before this group that line said "not supported". It is the same user mistake as `meas dc f find v(out) at=5` on an AC plot, which segfaults before this group and in 45.2. Not fixed: it is outside the picks, and fixing only `deriv` leaves `find`. `meas tran` on an AC plot errors properly, both.
- **AC sens under KLU, the `cktsens.c` hunk, not verified:** `.options klu` with `sens v(2) ac dec 5 1 1k` segfaults on before, after and 45.2. `.options sparse`: 16 points on all three. The hunk does not run in any deck I could make, and the crash was not looked into.
- **Ternary NULL guards in `evaluate.c`, not reached:** `1?0[3]:9` gives 0 with a warning, `0[3]?1:2` and `nosuch?1:2` give a parse error, both binaries and no invalid access. I found no deck that gets there.
- `vdb()` auto-save: `.meas ac find vdb(out)` with `.save v(in)` gives the same numbers on both binaries, so the `gettok_iv` change has no visible case from me. It was read, and a bare `vdd` now stays `vdd` where it was cut to `vd`.
- `meas sp` margins, `.tf` on active circuits, `ngSpice_Reset`, tclspice (`get_measure2` signature change there is not compiled here).
- Found on the way, not from this group: `--disable-xspice` does not build, `cktsopt.c:254` `OPT_ENH_RSHUNT` undeclared, from upstream `ca2d99436`.

## Fix round

The independent reviewer found one blocking issue: `meas deriv` under-reads the heap on a 2-point sweep.

**Confirmed.** In `measure_deriv_at()`, `v_length == 2` gives j=1, i=0 -> 1 -> clamped back to 0, so `DERIV_SCL(i-1)` and `DERIV_VAL(i-1)` read index -1. My `0 invalid` table above never tried 2 points. The reviewer's deck on this binary: `at=10` 4.75e-01, `at=15` 5e-01, `at=20` 5.25e-01, exit 0, no warning.

**Wider than reported, on AC.** The complex read is further off than the DC one: the slope has the wrong sign at two of three points.

**Fix, `c957569c9`, fork-written:** with two samples, take the slope of the line through them. The comment said this happened at every interval end, which it does not: with 3 or more samples the stencil shifts inward. The comment is corrected. Nothing changes for 3 or more samples. The pick stays.

**RED,** new `tests/regression/misc/meas-deriv-two-points.cir`, reference written by hand from the values below, before the fix (FAIL), and PASS after:

| `meas`, on | expected | before | after |
|---|---|---|---|
| `dc v1 10 20 10`, `deriv v(2) at=10` | 0.5 | 4.75000e-01 | 5.00000e-01 |
| same, `at=15` | 0.5 | 5.00000e-01 | 5.00000e-01 |
| same, `at=20` | 0.5 | 5.25000e-01 | 5.00000e-01 |
| `dc v1 20 10 -10`, `at=10` | 0.5 | 4.75000e-01 | 5.00000e-01 |
| same, `at=20` | 0.5 | 5.25000e-01 | 5.00000e-01 |
| `dc v1 10 30 10`, `at=10`, `20`, `30` | 0.5 | 5.00000e-01 each | 5.00000e-01 each |
| `ac dec 1 10 100`, RC tau 1 ms, `deriv vm(5) at=10` | -1.68110e-03 by hand | 8.96548e-02 | -1.68110e-03 |
| same, `at=50` | -1.68110e-03 | 8.46733e-03 | -1.68110e-03 |
| same, `at=100` | -1.68110e-03 | -9.30170e-02 | -1.68110e-03 |

valgrind on that deck: before 5 invalid reads, "Address ... is 8 bytes before a block of size 8,192"; after 0 invalid. The reviewer's `dc2wrong`, `dc2batch` (a `.meas` card in batch) and `dc3ok` decks: 0 invalid, all 5.00000e-01.

The `.out` needs `git add -f`: `tests/.gitignore` has `*.out`.

### Gates

- `autogen.sh`, `make`, `make install`: 0. Full `make -k check`: **366 PASS, 1 FAIL, 0 SKIP**; the one new PASS is the new test. The FAIL is `resource-keyword-case.cmd`, the baseline's named exception.
  `identity.lint`, `selftest.lint` PASS. `identity.baseline` untouched, no existing `.out` changed. The new code has no string comparison.
- `measure/run.sh`: exit 0, `mc_r` and `mc_sky130` `identical_to_ref = yes`. Saved in `measure/after-03-fix/` (96 KB).
- Against `after-03`: `TOTAL changed: 0`. Against `baseline`: 73, the same 73.

### For upstream, not posted

The code at `refs/upstream/pre-master-48` is the same, so upstream has this defect. Text for a note: "`meas dc d deriv v(2) at=10` on a 2-point sweep (`.dc v1 10 20 10`) reads index -1 in `measure_deriv_at()`: i is clamped to 1 and then to `v_length - 2` = 0. It prints a wrong number with no error. A two-point slope when `v_length == 2` fixes it." The driver or the owner decides whether and when to send it.

### Found on the way, not fixed

- A `when` crossing in the first interval of a sweep is not found: `when v(2)=7` on 4.5, 9.5 or 4.5, 9.5, 14.5 is "out of interval", on this binary and on 45.2, for `when`, `find` and `deriv` alike. So `deriv ... when` cannot be used on a 2-point sweep, and the new test uses `at=` only. `when v(2)=12` on the 3-point sweep gives at=25 and 0.5. This is older than the group.
- `measure_deriv_at()` bounds its scan by the vector's length, not the scale's. `let a = vector(5)` on a 2-point sweep, `deriv a at=25` and `find a at=25`: "out of interval", 0 invalid under valgrind, because the sweep's block is 8,192 bytes. Reading past the scale's valid length is not proven harmless. Not looked into further.
