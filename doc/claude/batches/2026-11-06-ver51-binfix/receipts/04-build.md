# 04 — final build of ver_51 at 7151f36a9 (build-ver_51)

Date: 2026-11-07. Main checkout /home/analog/dev/ngspice, branch ver_51, HEAD 7151f36a9.
No git writes. Owner's staged CLAUDE.md and modified tests/regression/pipe/resource-keyword-case.cmd
untouched (git status before and after: `M  CLAUDE.md`, ` M tests/regression/pipe/resource-keyword-case.cmd`).
build-ver_50/ not touched.

## Build
All via ngheavy, -j4. Logs in /home/analog/dev/ngspice/build-ver_51/claude-*.log.
- `ngheavy ./autogen.sh` — rc 0 (claude-autogen.log)
- `ngheavy ../configure --prefix=/home/analog/dev/ngspice/build-ver_51/stage --with-readline=yes --enable-pss --enable-cider` — rc 0 (claude-configure.log)
- `ngheavy make -j4` — rc 0, first attempt, not cut off (claude-make.log)
- `ngheavy make install` — rc 0 (claude-install.log)

## make -k check -j4 (claude-check.log)
rc 0. **366 PASS, 0 FAIL**, 0 XFAIL/XPASS/SKIP. Includes model QA (bsim*, hisim*, hicum2, bsimsoi qaSpec)
and tests/lint (identity.lint, selftest.lint).
- binning-edges.cir PASS, binning-edges-draws.cir PASS.
- resource-keyword-case.cmd (owner's working-tree edit): **PASS** here (pipe dir has no .out comparison).
- The single `ERROR:` line in the log, "(internal) tried to destroy non-existent graph", is ngspice
  output inside a passing test (after vector-let-report.cir), not a test result.

## Binary
- `build-ver_51/stage/bin/ngspice -v` runs: "ngspice-46+ : Circuit level simulation program, Compiled with KLU".
- sha256 build-ver_51/stage/bin/ngspice: `ae06c0fb831fa7e9188ff3add256ebc3b210be8d1ff16f5d834a7c3acde56416`
- sha256 build-ver_50/stage/bin/ngspice: `c2bd1098ce5ff7e539f0c1c20c475ea4783a4694d45822875b6b65747474ab8e` (unchanged, starts c2bd1098)

## Repro (README in .../worst_case_plan/ngspice_bin_edge/)
Outputs in /home/analog/dev/scratch/ver51-binfix-receipts/build/. Decks copied there; the sky130 deck's
relative `.lib` was rewritten to the absolute path
/home/analog/private/xschem-private/sky130A/models/libs.tech/combined/sky130.lib.spice (only change).
Run with empty HOME, from plain/ (empty .spiceinit) and hsa/ (`set ngbehavior=hsa`, `set ng_nomodcheck`).

### sky130_l2_edge.cir (L=2u, tt)
| mode | xn5 | xn6 | xp13 | Id xn5 / xn6 / xp13 |
|---|---|---|---|---|
| no-mode (sky_ver51_plain.log) | nlowvt_model.3 | nlowvt_model.27 | plowvt_model.3 | 1.150472795668e-04 / 2.335770639790e-05 / 3.164499697235e-05 |
| hsa (sky_ver51_hsa.log) | nlowvt_model.3 | nlowvt_model.27 | plowvt_model.3 | same |

No-mode and hsa agree, on the upper bin, matching build-ver_52. (build-ver_50 no-mode picked .4/.28/.4.)

### binning-edges.cir (edges_ver51.log)
tab.py: 42 rows, **0 wrong on ver_51**, 0 of 14 sizes where no-mode and hsa disagree.
Against ver_50 (edges_table.txt): ver_50 20 wrong / 11 of 14 disagree. Against ver_52 (edges_table_vs52.txt):
both 0 wrong. (tab.py's column headers are fixed labels; the second column is ver_51.) The 6
"circuit not parsed" errors in the log are the expected card.0 "no model" rows, same count as edges_ver52.log.
