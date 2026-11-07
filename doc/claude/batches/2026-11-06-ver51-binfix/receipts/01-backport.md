# 01 backport: bin-edge fix onto ver_51 (df2194696)

Worktree: /home/analog/dev/scratch/ver51-binfix (detached), build: build/

## Picks (plain `git cherry-pick`, no -x)
| source | new |
|---|---|
| 5282ca5f0 | 3bca7c27a |
| 0f763b135 | f59023203 (inpcom.c auto-merged) |
| d1d5feff5 | 2578c64bf (conflict, see below) |
| 075321903 | 7151f36a9 |
Tip: 7151f36a9

## Conflict
- d1d5feff5, tests/regression/model/Makefile.am TESTS line. ver_52 list has bjt-qco-1.cir,
  vdmos-tj-ground.cir, vdmos-tj-ground-soa.cir which are not in ver_51.
  Resolution: kept ver_51 list, appended binning-edges-draws.cir:
  `TESTS = binning-1.cir binning-edges.cir special-names-1.cir instance-defaults.cir binning-edges-draws.cir`
- Process note: a first attempt committed conflict markers (ae5883850, plus 5807b93e3 on top) because a
  shell chain short-circuited; both were discarded with `git reset --hard f59023203` in the detached
  worktree and the two picks redone. Neither commit is on any branch. Final tree: `git grep '^<<<<<<<'` empty.

## git diff ver_52 HEAD -- subckt.c inpgmod.c inpdefs.h inpcom.c
Only src/frontend/inpcom.c, 1 line (inp_compat, G source m= string, line ~7587):
ver_52 `"g%s %s_int1 0 %s %s 1e9 m=%s"` vs HEAD `"g%s %s_int1 0 %s %s %se9"`.
Comes from ver_52-only commit 826b43632 "Fixes bug 876" (not on ver_51). Unrelated to binning.
subckt.c, inpgmod.c, inpdefs.h: identical to ver_52.

## RED (old binary build-ver_50/stage/bin/ngspice, check.sh with ngspice_vpath set)
| test | exit | diff lines -/+ |
|---|---|---|
| binning-edges.cir | 1 (FAIL) | -24 / +16 |
| binning-edges-draws.cir | 1 (FAIL) | -13 / +1 |
| binning-1.cir (control) | 0 PASS | 0 |
binning-edges: 8 cases "could not find a valid modelname / circuit not parsed" (l=25 / l=20.2 / w=1010 / nf=5 outer-max sizes).
binning-edges-draws: same parse errors for w=3 l=20.2 cases; "two models" listing missing x1:nc.2 etc.
Full diffs: binning-edges.diff, binning-edges-draws.diff beside this file.

## Build
- ngheavy ./autogen.sh: exit 0 (autogen.log)
- ngheavy ../configure --prefix=.../build/stage --with-readline=yes --enable-pss --enable-cider: exit 0 (configure.log)
- ngheavy make -j4: STARTED, NOT FINISHED when this crew was forced to return (last log line: `CCLD libfte.la`, make.log).
  The background job ends with this crew; make install / make check NOT run.

## GREEN
Not run. Remaining: ngheavy make -C build -j4; ngheavy make -C build install;
ngheavy make -C build/tests/regression/model check; ngheavy make -C build check -j4.
