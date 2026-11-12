# Receipt 02: the port, commit by commit

After every commit: `./autogen.sh` where a `Makefile.am` or `configure.ac`
changed, `make -C build-ver_53 -j6`, `make install`, then
`make -C build-ver_53/tests/<dir> -k check` for `regression/stb`,
`regression/case`, `regression/casedist`, `lint`, and the XSPICE directories
where they were touched, all with `DISPLAY` unset. The logs are
`build-ver_53/claude-check-<step>-<dir>.log`; a step named `-red` is the code
as picked, before the fork's change.

RED-first: each new case deck was written with its expected output (the
`stbchk ... pass` lines) before the fork's change, run on the code as picked
and seen to fail for the reason given, then run again after the change. The
committed `.out` files are the output of the passing run; their verdict lines
are the ones written beforehand.

## 01 `7c68fb832` -> `af1c1b90e` niaciter.c: split the load and factor step

Clean pick. lint 2 PASS.

## 02 `ec1d4e137` -> `1c0d71374` Add the stb loop-gain (stability) analysis

Conflicts:

| file | resolution |
| --- | --- |
| `configure.ac`, `tests/regression/Makefile.am` | the fork's directories kept, `stb` added |
| `src/spicelib/analysis/CMakeLists.txt` | not in this tree; dropped |
| `src/frontend/outitf.c` | only `stbdefs.h` is included (the other two includes belong to an upstream progress bar this tree lacks); the save exemption is written around the fork's `ft_savenone()` |
| `src/frontend/runcoms.c` | both sides: the fork's `OUTsaveMissClear()` and the new `before` |
| `src/spicelib/parser/inp2dot.c` | the `.stb` dispatch uses `cieq()` like its neighbours here |
| `tests/xspice/analog/Makefile.am`, `spinit.in` | the fork's TESTS list plus `stb-xspice.cir`; `spinit.in` gains both code model paths the deck loads (upstream had one of them already) |

Build fix: `ft_optimizing` does not exist here; the guard is dropped.

RED on the code as picked (probe word folded, exact lookup):

```
tests/regression/case/stb-vprobe-case.cir       11 of 12 verdicts fail
tests/regression/casedist/stb-vprobe-case.cir    8 of 12 verdicts fail
```

GREEN: stb 16, case 119, casedist 31, xspice/analog 2, lint 2, 0 FAIL.

## 03 `66d8731f2` -> `41bcf5b21` stb: add the margins plot and the log lines

Conflicts: `outitf.c` (upstream calls `EVTswitch_plot()` after
`EVTsetup_plot()`, this tree does not: only the companion test is added);
CMake list dropped. Build fix: second `ft_optimizing` guard dropped.
stb 20, case 119, casedist 31, xspice/analog 2, lint 2, 0 FAIL.

## 04 `a353c4c52` -> `f290dc49c` stb: say what is wrong with a bad command

Clean pick. stb 22, case 119, casedist 31, lint 2, 0 FAIL.

## 05 `bc9f63414` -> `d428e339a` stb: raw files, .print, .plot, meas, keepopinfo

Conflict: `stban.c`, the `stb_log()` call. Lint on the code as picked: 1 report
(`eq(analName, STB_OP_TITLE)`).

RED: `tests/regression/case/stb-keepop-save-case.cir`, 1 of 1 verdict fails:
under preserve `save out` did not select the node `Out` in the operating point
plot, which kept its seven vectors.

GREEN: stb 30, case 120, casedist 31, lint 2, 0 FAIL.

## 06 `455a566a6` -> `3a3d7f53f` stb: accept a terminal of an instance

Conflicts:

| file | resolution |
| --- | --- |
| `src/frontend/CMakeLists.txt` | dropped |
| `src/frontend/inp.c` | `inp_stb_restore()` before the fork's `if (!comfile)` block; `inp_stb_insert()` after `inp_chk_model_types()`; `inp_stb_discard()` beside `ng_drawlog_end()` on the failure return |
| `src/frontend/subckt.c` | both includes |
| `src/spicelib/analysis/stban.c` | the fork's `stb_key()` lookup kept, the new variables added |
| `visualc/sharedspice.vcxproj` | `inpc_stb.c` added where this tree's list has its neighbours |

Lint on the code as picked: 17 reports.

RED (probe folded, byte-exact compares):

```
tests/regression/case/stb-term-case.cir       8 of 10 verdicts fail
    stb: probe x1:in not inserted: no instance named x1
tests/regression/casedist/stb-term-case.cir   5 of 9 verdicts fail
    stb: probe x1:in not inserted: x1 has no terminal named in (... In Out)
```

GREEN: stb 49, case 121, casedist 32, xspice/analog 2, lint 2, 0 FAIL.

## 07 `5a36ce8e0` -> `4f4d69344` stb: terminal probes inside subcircuits

Conflicts: `src/CMakeLists.txt` dropped; `inpc_stb.c` (the picked file with
the fork's compares of row 06 applied again); `stban.c` (the transistor hint
uses the folded device letter); TESTS lists of `tests/xspice/analog` and
`tests/xspice/digital`, where the two `CLEANFILES` lines became one.
Lint on the new code as picked: 12 reports.

RED (row 06's changes in place, the new code as picked):

```
tests/regression/case/stb-term-hier-case.cir       6 of 15 verdicts fail
    stb: probe X6.XL.G1:3 not inserted: no instance named X6.XL.G1
    stb: probe M1:G not inserted: M1 has no terminal named G (1 to 4)
    stb: probe X9:sup not inserted: X9 has nothing connected to that port ...
tests/regression/casedist/stb-term-hier-case.cir   3 of 20 verdicts fail
```

GREEN: stb 55, case 122, casedist 33, xspice/analog 3, xspice/digital 9,
lint 2, 0 FAIL.

Correction (review, receipt 04): the two logs first saved for this row,
`claude-check-c7-regression_case.log` and
`claude-check-c7-regression_casedist.log`, do not show these figures. They
were written before the final `.out` of `stb-term-hier-case.cir` was
committed and show 121 PASS, 1 FAIL and 32 PASS, 1 FAIL, the failure being
that deck: every verdict in them is `pass`, and the diff is the lines the
`.out` of that moment did not yet have. The figures above are those of a
re-run on a tree checked out at `4f4d69344` and rebuilt:
`claude-check-c7-rerun-regression_case.log` (122 PASS, 0 FAIL),
`claude-check-c7-rerun-regression_casedist.log` (33 PASS, 0 FAIL),
`claude-check-c7-rerun-regression_stb.log` (55 PASS) and
`claude-check-c7-rerun-lint.log` (2 PASS).

## 08 `469b9f016` -> `0817a5329` stb: add diag=1 and localgnd=<node>

Conflict: `stban.c`, the first log line. Lint on the code as picked: 1 report
(`eq(node->name, name)`).

RED (the localgnd node folded and compared byte for byte):

```
tests/regression/case/stb-localgnd-case.cir       4 of 7 verdicts fail
tests/regression/casedist/stb-localgnd-case.cir   4 of 6 verdicts fail
```

The first RED run of these two decks was taken with a circuit that lacked the
source tying the probe's second node to ground; it was repeated on the
corrected decks, and the numbers above are those of the repeat
(`claude-check-c8-red2-*.log`).

GREEN: stb 59, case 123, casedist 34, xspice/analog 3, xspice/digital 9,
lint 2, 0 FAIL.

## 09 `3af952a7e` -> `c600cedfe` Document the stb loop-gain analysis

Conflict: `NEWS` (the entry goes under this tree's top heading). `README.stb`:
the sentence on case is replaced by the case-mode rule. The three decks of
`examples/stb` give the numbers of their headers (phase margin 47.477 deg and
gain margin 20.8377 dB; 51.9005 and 79.1867 deg; mean 51.41, sigma 2.12 over
100 runs).
