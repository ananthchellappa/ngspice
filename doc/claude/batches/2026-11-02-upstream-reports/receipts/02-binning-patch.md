# Receipt 02: binning patch for bugs/889

Date 2026-11-04. Nothing posted, pushed or filed. All configure, make, make check, and the sky130 sweeps went through `ngheavy`, `-j4`. No exit 137. The short single-deck runs were direct.
Scratch: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/binpatch` (`up` is a separate git repo, not a worktree; `logs/` and `sky/` hold the raw output). HOME was a scratch directory, with no `.spiceinit` for the builds and checks.

## Results first

- Base: `refs/upstream/pre-master-48` tip **`97f8de1a747c8934153b50656fd3bac2ce40c1d2`**. Not already fixed.
- Patch: `patches/0001-subckt.c-bin-in-double-with-the-same-test-as-INPgetM.patch`, commit `230a0a81b` on local `scratch/binning-hsa-finger`. 7 files, +153 -18; src is +19 -17.
- `make -k check`: 59 PASS 0 FAIL before, 60 PASS 0 FAIL after. The only difference is the new test.
- Nothing moved except the sizes that used to stop.

## A. Fetch and ticket

`git fetch https://git.code.sf.net/p/ngspice/ngspice pre-master-48:refs/upstream/pre-master-48`: `3adcd96a0..97f8de1a7`, 2 new commits (`c426c8092` cmake, `97f8de1a7` seeding). `git log 03ef19244..97f8de1a7 -- src/frontend/subckt.c src/spicelib/parser/inpgmod.c src/include/ngspice/inpdefs.h` is empty, so struct card, the pruner and `INPgetModBin()` are as filed.

Ticket: https://sourceforge.net/p/ngspice/bugs/889/, open, no owner; Dietmar Warning's comment is the only one. **Read with WebFetch only, which returns a model's summary with short quotes, not the raw page.** A plain `curl` got 2 bytes. A second try with a browser user agent was refused by this session's permission check, so I dropped it. The owner should read the wording on the ticket itself. As returned:

- "Please send the patch against pre-master-48."
- The pruner should compare in double and use the same in-range test as `INPgetModBin()`, "preferably by calling one shared function".
- "the reference simulator also rejects a size exactly on the outermost lmax/wmax". Keep current behaviour; accepting it could be a separate feature request.
- He confirms on a commercial reference simulator (name not given in the summary) that the upper bin is right on an inner edge.

The brief asked for the same comparison, the ticket prefers one function, so the patch shares one.

## B. Baseline

`up` from the fetched tip. `ngheavy ./autogen.sh`, `ngheavy ../up/configure --prefix=... --with-readline=no`, `ngheavy make -j4`, `ngheavy make -k check`: rc 0, **59 PASS, 0 FAIL**, no XFAIL or SKIP. Logs `logs/check-base.log`.

## C. RED

`tests/regression/model/binning-hsa.cir`, in `TESTS` of that directory's `Makefile.am` (`EXTRA_DIST` follows from it). `make dist` on the fix carries both `.cir` and `.out`.

Setting hsa: `set_compat_mode()` runs at the start of `inp_readall()`, so `set ngbehavior=hsa` in `.control` reaches a circuit read later. The circuit is given with `circbyline`, in one file. No test in `tests/` sets ngbehavior before this one.

`check.sh` compares stdout only and its filter drops any line with `est`, `Error` and more, so an `INFO: 0 of 2 tests failed` line as in `binning-1` is not compared. The test therefore prints ratios, `print i(vx1)/i(vr1)`, to a plain model at the same size. No ratios when the deck does not load.

```
$ make -C build/tests/regression/model check TESTS=binning-hsa.cir      (unpatched)
Error on line 3 or its substitute:
  m.x1.mn dx1 g 0 0 x1:card l= 5.0e-07 w= 1.5e-05 nf= 5.0e+00
could not find a valid modelname
Error on line 4 or its substitute:
  m.x1.mn dx1 g 0 0 x1:card l= 5.0e-01 w= 1.5e+01 nf= 5.0e+00
could not find a valid modelname
FAIL: binning-hsa.cir
```

The final `.out` against the saved unpatched binary: `check.sh` rc 1, the same two errors, all 8 ratio lines missing.

The test can fail the other way too. On the fix, with the references of x1 and x3 swapped to the wrong vth0: 4.204503e-01 and 2.382026e+00. With the `set ngbehavior` line removed: 1.543091e-01, 1.0, 6.434725e-02, 1.629339e-01.

The 8 ratios of 1.000000e+00 were written by hand into the expected file before the fix ran. The fix matched them; the only addition was the `option SCALE` stdout line. The committed `.out` is the real output, with one trailing space removed so `git am` gives no whitespace warning.

## D. Fix

`in_range()` becomes exported `INPbinInRange()` in `inpgmod.c`, declared in `inpdefs.h` with `#include "ngspice/bool.h"`. In `doit()`, floats become doubles and the comparison calls it. `inp_get_w_l_x()` drops its `(float)` casts. `struct card` w, l, nf become double: 16 to 32 bytes for those fields, so 16 bytes per card on 64-bit (computed, not measured).

Cause confirmed in C: `(float)15e-6 / 5` and `(float)1e-6 * 15 / 5` are both 2.9999998787e-06, `(float)3e-6` is 3.0000001061e-06.

Field users: fields renamed temporarily, `make -k`, every "no member" error listed, restored. Only `inpcom.c` (`inp_get_w_l_x()`, lines 944 to 1000) and `subckt.c` (the pruner and the three copies at 824, 869, 928). Default config, plus a grep of `->nf` over all of `src/`.

### .options wnflag=0, hsa

```
bins .42-3u, 3-7u, 7u-1.01m, l=0.5u; base and fix:
w=15u   nf=5   base no model    fix no model     (pruner 3u card.2, binner 15u card.3)
w=14.5u nf=5   base no model    fix no model     (pruner 2.9u card.1, binner 14.5u card.3)
w=6u    nf=2   base card.2      fix card.2       -9.73942e-04 both
w=21u   nf=3   base card.3      fix card.3       -3.54692e-03 both
w=2u    nf=2   base card.1      fix card.1       -3.07126e-04 both
bins .42-3u, 3u-1.01m:
w=15u   nf=5   base no model    fix card.2       -2.43486e-03
```

The pruner never looks at wnflag. That gap is older and is not widened; the last row is the only change, and it is the float fix itself. Reported in the reply, not fixed. An instance `wnflag=0` on the level 54 line, which `INPgetModBin()` reads, stops with `unknown parameter (wnflag)` on both, so that route could not be exercised on BSIM4.

## E. Verify

Targeted test: PASS. Full, on the committed tree (`logs/check-fix2.log`):

```
$ diff <(grep -E '^(PASS|FAIL): ' check-base.log) <(grep -E '^(PASS|FAIL): ' check-fix2.log)
16a17
> PASS: binning-hsa.cir
60 PASS, rc 0
```

Report's decks, `.spiceinit` with hsa or empty as noted:

| deck | mode | base | fix |
|---|---|---|---|
| 2-hsa-finger | hsa | no model, exit 1 | -2.43486e-03, x1:card.2, exit 0 |
| 2-hsa-finger-noscale | hsa | no model, exit 1 | -2.43486e-03, x1:card.2, exit 0 |
| 2-hsa-finger | plain | -2.43486e-03, x1:card.2 | same |
| 2-outer-max | plain | no model, exit 1 | same |
| 2-outer-max | hsa | no model, exit 1 | same |
| 2-outer-max-hsa | hsa | no model, exit 1 | same |

### sky130

Not loadable on the tip: `940bb50fa` is still the last functional change to its path and `; sign expected` stops every size. So scratch copy `up138`: only the `inpcom.c` hunk of #138's `agauss-brace.patch`, built with and without the fix (138base, 138fix). PDK `/home/analog/private/xschem-private/sky130A/.../sky130.lib.spice`, tt, `.spiceinit` hsa and ng_nomodcheck.

| size | tip, fix | 138base | 138fix |
|---|---|---|---|
| l=1 w=15 nf=5 | ; sign expected | no model | -1.86245e-04, nshort_model.20 |
| l=1 w=49 nf=7 | ; sign expected | no model | -6.37262e-04, nshort_model.4 |
| l=0.5 w=1 | ; sign expected | -1.69329e-05, nshort_model.29 | same |
| l=20.2 w=2, hsa and plain | | no model | no model |
| l=1 w=1010, hsa and plain | | no model | no model |

The currents are the ones the report gives for a0999002f. The sweep is `sky/gen.py`, `cmp.py`, `perB.py` over the 48 bins of nfet_01v8, parsed from the model file; a rule in Python gives the expected card.

| sweep | sizes | result |
|---|---|---|
| A, off any edge, 200 sampled from each bin's corners and middle, nf 1,2,3,5,7, one deck | 200 | 138base and 138fix output byte-identical (`cmp`); card and i(vd) equal; all on the rule's card |
| B, on inner edges, each in its own deck on both | 144 | 138base stops on 16; 138fix on 0. Those 128 that loaded on both: same card and i(vd). All 144 on the rule's card on 138fix |

The 16: w=2.1 nf=5, w=15 nf=5, w=21 nf=7, w=49 nf=7, each at l=0.15, 0.5, 1, 4.

### git am

Fresh clone at `97f8de1a7`: `git am 0001-*.patch` rc 0 and no warnings; result tree `0865f62dd` equals the tested `up` tree. A first try with `--whitespace=error` found one trailing space in the `.out`, which is why it was removed.

That tree, built `--with-ngshared --enable-cider`: compiles and links, rc 0, no related warnings, `nm -D` shows `INPbinInRange` in `libngspice.so`. Not run.

## Not verified

- CMake and MSVC builds, and `ctest`. The CMake test list comes from the same `TESTS` line.
- libngspice at run time, and repeated `ngSpice_Reset`. The fix adds no state.
- Other sky130 devices and corners; other PDKs.
- The ticket's exact wording: see section A.
- Memory growth, computed only.
