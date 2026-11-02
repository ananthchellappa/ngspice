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
