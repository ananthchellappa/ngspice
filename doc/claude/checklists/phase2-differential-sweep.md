# Phase 2 differential sweep — result

Deliverable 2.7 of `doc/claude/suggestions/case-sensitive-identifiers-plan.md`.
Produced by `doc/claude/scripts/case_differential_sweep.py`, run as

```sh
python3 doc/claude/scripts/case_differential_sweep.py --jobs 12 --timeout 90
```

against `build-ver_50` (XSPICE and OSDI on, CIDER off, WITH_PSS off,
RFSPICE on), at the head of the Phase 2 series.

## What it does that `make check` cannot

`make check` runs only the directories listed in `tests/Makefile.am` SUBDIRS —
`regression`, `xspice` and the model QA suites — so `tests/general`,
`tests/filters`, `tests/transmission`, `tests/vbic`, `tests/sensitivity`,
`tests/resistance`, `tests/polezero`, `tests/transient`, `tests/mesa` and the
rest of `DIST_SUBDIRS` are never executed. It compares stdout after an
`egrep -v` filter that removes most diagnostics. It never runs one deck twice.
And it never looks at a rawfile.

The sweep runs every `.cir` under `tests/` three ways — stock, `preserve` on
the original, and `preserve` on a mechanically uppercased copy — plus a second
stock run, and compares two things against the first stock run: case-normalised
stdout with stderr merged in, and the rawfile numeric payload with every name
and title excluded.

Two details make the numeric comparison trustworthy. A rawfile may hold several
plots back to back, so slicing at the first `Binary:` marker leaves the later
plots' *headers* — which carry variable names — inside the supposed payload;
each plot is therefore measured from its own header. And an AC rawfile is not
reproducible between two stock runs on this tree, so the sweep runs stock twice
and masks the words the two stock runs already disagree on — see
`doc/codex/issues/0012`.

The uppercasing is not a blind `str.upper()`: text inside double quotes is left
alone, and lines whose first token names a file are left alone entirely.
Uppercasing a path would test the filesystem, not ngspice.

## Result

| verdict | count | meaning |
| --- | ---: | --- |
| OK | 20 | stdout and rawfile agree across all three runs |
| PARSE-FAIL | 82 | a `preserve` run failed to parse a netlist the stock run parsed |
| DIFF | 24 | stdout differs, rawfile does not, no parse failure |
| NUM-DIFF | 0 | rawfile numerics differ |
| SKIP | 3 | the stock run itself timed out at 90 s |
| **total** | **129** | |

**No deck produces different numbers under `preserve` where it parses at all.**
That is the acceptance criterion the plan asks this sweep to test, and on the
decks that run it holds.

## The 82 parse failures

All of them reduce to one defect, written up as
`doc/codex/issues/0009`: deck preprocessing in `src/frontend/inpcom.c`
classifies cards by testing the first character against a lower-case literal,
which is correct only because the reader folded first. An upper-case device
letter therefore makes `inp_rem_unused_models()` comment out the `.model` card
and `comment_out_unused_subckt_models()` comment out the `.subckt` body.

36 decks fail on the original deck; a further 46 fail only on the uppercased
copy. The diagnostics, by frequency:

```
41  could not find a valid modelname          (D, Q, M, J, Z instances)
13  Unable to find definition of model <NAME>
19  Error: unknown subckt: X...
 6  unknown parameter (<modelname>)           (R instances)
 3  MIF-ERROR - unable to find definition of model
```

The three timeouts are `tests/mesa/mesa-12.cir`, `tests/mesa/mesa12.cir` and
`tests/vbic/FG.cir`; all three exceed 90 s in the stock run, so the sweep has
nothing to compare against and reports SKIP rather than guessing.

## The 24 stdout-only differences

None changes a number. They fall into four groups.

1. **Names, which is the point of the feature.** `write-roundtrip.cir` and
   `all-wildcard-case.cir` report `v(InA)` where the stock run reports
   `v(ina)`. The comparison normalises case, so these show up only where the
   uppercased copy also changed how many lines were printed.
2. **Artifacts of the mechanical uppercasing.** The uppercaser protects quoted
   text, which means a control-language variable defined outside quotes and
   referenced inside them stops resolving:
   `setcs KeptVar = "MiXeD"` becomes `SETCS KEPTVAR = "MiXeD"` while
   `"$KeptVar"` is untouched. That is what `fold-mixed-case.cir`,
   `script-case.cir`, `dollar-1.cir`, `resume-1.cir` and the four `sens-*`
   decks are showing. It is a limitation of the sweep's uppercasing rule, not
   of ngspice, and it is why the rule is documented in the script.
3. **Files created by name.** `print-tail-case.cir` writes `PrintTail.txt` and
   reads it back through the shell; uppercased, it writes `PRINTTAIL.TXT` and
   the `shell cat PrintTail.txt`, being inside no quotes, also uppercases, so
   the two agree — but the deck's own expected line moves.
4. **The sweep's own probe deck.** `harness-alive.cir` echoes `$casemode`,
   which by construction does not exist in the stock run.

## Interpretation

`preserve` does not change any number it can compute. What it cannot yet do is
parse a deck written in the ordinary SPICE house style, because upper-case
device letters break card classification in the preprocessor. That is one
defect with four or five sites, it is mechanical to fix, and it is Phase 1's
enumeration gap rather than Phase 2's plumbing — the Phase 1 census wrote down
that it does not count character-literal comparisons.

Two further gaps are recorded rather than fixed: model, subcircuit and
global-node names still match with `strcmp`
(`doc/codex/issues/0010`), and the `gnd` rewrite runs over command text it
should not touch (`doc/codex/issues/0011`).

## Full output

```
PARSE-FAIL tests/bsim1/test.cir                                         preserve: could not find a valid modelname
PARSE-FAIL tests/bsim2/test.cir                                         preserve: could not find a valid modelname
PARSE-FAIL tests/bsim3soidd/RampVg2.cir                                 preserve: could not find a valid modelname
PARSE-FAIL tests/bsim3soidd/inv2.cir                                    preserve: could not find a valid modelname
PARSE-FAIL tests/bsim3soidd/ring51.cir                                  preserve: could not find a valid modelname
PARSE-FAIL tests/bsim3soidd/t3.cir                                      preserve: could not find a valid modelname
PARSE-FAIL tests/bsim3soidd/t4.cir                                      preserve: could not find a valid modelname
PARSE-FAIL tests/bsim3soidd/t5.cir                                      preserve: could not find a valid modelname
PARSE-FAIL tests/bsim3soifd/RampVg2.cir                                 preserve: could not find a valid modelname
PARSE-FAIL tests/bsim3soifd/inv2.cir                                    preserve: could not find a valid modelname
PARSE-FAIL tests/bsim3soifd/ring51.cir                                  preserve: could not find a valid modelname
PARSE-FAIL tests/bsim3soifd/t3.cir                                      preserve: could not find a valid modelname
PARSE-FAIL tests/bsim3soifd/t4.cir                                      preserve: could not find a valid modelname
PARSE-FAIL tests/bsim3soifd/t5.cir                                      preserve: could not find a valid modelname
PARSE-FAIL tests/bsim3soipd/RampVg2.cir                                 preserve: could not find a valid modelname
PARSE-FAIL tests/bsim3soipd/inv2.cir                                    preserve: could not find a valid modelname
PARSE-FAIL tests/bsim3soipd/ring51.cir                                  preserve: could not find a valid modelname
PARSE-FAIL tests/bsim3soipd/t3.cir                                      preserve: could not find a valid modelname
PARSE-FAIL tests/bsim3soipd/t4.cir                                      preserve: could not find a valid modelname
PARSE-FAIL tests/bsim3soipd/t5.cir                                      preserve: could not find a valid modelname
PARSE-FAIL tests/general/diffpair.cir                                   preserve-UPPER: could not find a valid modelname
PARSE-FAIL tests/general/fourbitadder.cir                               preserve-UPPER: Error: unknown subckt: X1 1 2 3 4 5 6 7 8 9 10 11 12 0 13 99 FOURBIT
PARSE-FAIL tests/general/mosamp.cir                                     preserve-UPPER: could not find a valid modelname
PARSE-FAIL tests/general/mosmem.cir                                     preserve-UPPER: could not find a valid modelname
PARSE-FAIL tests/general/rca3040.cir                                    preserve-UPPER: could not find a valid modelname
PARSE-FAIL tests/general/rtlinv.cir                                     preserve-UPPER: could not find a valid modelname
PARSE-FAIL tests/general/schmitt.cir                                    preserve-UPPER: could not find a valid modelname
PARSE-FAIL tests/hfet/id_vgs.cir                                        preserve-UPPER: Unable to find definition of model HFET
PARSE-FAIL tests/hfet/inverter.cir                                      preserve-UPPER: Error: unknown subckt: X1 1 2 3 INV
PARSE-FAIL tests/jfet/jfet_vds-vgs.cir                                  preserve-UPPER: Unable to find definition of model MODJ
PARSE-FAIL tests/mes/subth.cir                                          preserve-UPPER: Unable to find definition of model MESMOD
SKIP     tests/mesa/mesa-12.cir                                       stock run timed out
PARSE-FAIL tests/mesa/mesa.cir                                          preserve-UPPER: Unable to find definition of model ENHA
PARSE-FAIL tests/mesa/mesa11.cir                                        preserve-UPPER: Unable to find definition of model MESMOD
SKIP     tests/mesa/mesa12.cir                                        stock run timed out
PARSE-FAIL tests/mesa/mesa13.cir                                        preserve-UPPER: Unable to find definition of model MESMOD
PARSE-FAIL tests/mesa/mesa14.cir                                        preserve-UPPER: Unable to find definition of model ENHA
PARSE-FAIL tests/mesa/mesa15.cir                                        preserve-UPPER: Unable to find definition of model MESMOD
PARSE-FAIL tests/mesa/mesa21.cir                                        preserve-UPPER: Unable to find definition of model MESMOD
PARSE-FAIL tests/mesa/mesgout.cir                                       preserve-UPPER: Unable to find definition of model DRIVER
PARSE-FAIL tests/mesa/mesinv.cir                                        preserve-UPPER: Unable to find definition of model DRIVER
PARSE-FAIL tests/mesa/mesosc.cir                                        preserve-UPPER: Error: unknown subckt: XINV01 1 2 3 MESINV
PARSE-FAIL tests/mos6/mos6inv.cir                                       preserve: Error: unknown subckt: XNDINV1 1 11 12 13 2 100 NDINV
PARSE-FAIL tests/mos6/simpleinv.cir                                     preserve: could not find a valid modelname
PARSE-FAIL tests/polezero/pz2.cir                                       preserve-UPPER: Simulation interrupted due to error!
PARSE-FAIL tests/polezero/pzt.cir                                       preserve-UPPER: Simulation interrupted due to error!
PARSE-FAIL tests/regression/case/case-flag-noop.cir                     preserve-UPPER: Error: unknown subckt: X1 MID OUT DIVIDER
DIFF     tests/regression/case/harness-alive.cir                      preserve: stdout -'error: casemode: no such variable.'; -'casemode-is'; +'casemode-is preserve'
DIFF     tests/regression/case/write-roundtrip.cir                    preserve-UPPER: stdout -'done.'; -'title: * preserved names survive the write path and come back off disk'; -'name: operating poin
DIFF     tests/regression/func/func-1.cir                             preserve-UPPER: stdout -'doing analysis at temp = 27.000000 and tnom = 27.000000'; -'using sparse 1.3 as direct linear solver'; -'
PARSE-FAIL tests/regression/lib-processing/ex1a.cir                     preserve: Error: unknown subckt: X1 9 0 sub1
PARSE-FAIL tests/regression/lib-processing/ex1b.cir                     preserve: Error: unknown subckt: X1 9 0 sub1
PARSE-FAIL tests/regression/lib-processing/ex2a.cir                     preserve: Error: unknown subckt: X1 7 0 sub1_in_lib
PARSE-FAIL tests/regression/lib-processing/ex3a.cir                     preserve: Error: unknown subckt: X1 7 0 sub1_in_lib
PARSE-FAIL tests/regression/lib-processing/scope-1.cir                  preserve-UPPER: Error: unknown subckt: X1001_T N1001_T 0 SUB1
PARSE-FAIL tests/regression/lib-processing/scope-2.cir                  preserve-UPPER: Error: unknown subckt: X1001_T N1001_T 0 SUB1
PARSE-FAIL tests/regression/lib-processing/scope-3.cir                  preserve: Error: unknown subckt: x1001_t.X1 n1 n2 sub  numparm__________00000002
DIFF     tests/regression/misc/ac-zero.cir                            preserve: stdout +"warning, can't find model '1uh' from line"; +'l1 2 3 1uh'; +"warning, can't find model '1uf' from line"; +(1 mo
DIFF     tests/regression/misc/all-wildcard-case.cir                  preserve-UPPER: stdout -'done.'; -'title: * uppercase all wildcard'; -'title: * uppercase all wildcard'; -(7 more); +'all-wildcard
DIFF     tests/regression/misc/alter-vec.cir                          preserve-UPPER: stdout -'info: success'; +'warning from checkvalid: vector eq is not available or has zero length.'
DIFF     tests/regression/misc/asrc-tc-1.cir                          preserve-UPPER: stdout -'doing analysis at temp = 127.000000 and tnom = 27.000000'; -'doing analysis at temp = 127.000000 and tnom
DIFF     tests/regression/misc/asrc-tc-2.cir                          preserve-UPPER: stdout -'doing analysis at temp = 127.000000 and tnom = 27.000000'; -'doing analysis at temp = 127.000000 and tnom
DIFF     tests/regression/misc/bugs-1.cir                             preserve-UPPER: stdout -'doing analysis at temp = 27.000000 and tnom = 27.000000'; -'using sparse 1.3 as direct linear solver'; -'
PARSE-FAIL tests/regression/misc/case-flag-noop.cir                     preserve-UPPER: Error: unknown subckt: X1 MID OUT DIVIDER
DIFF     tests/regression/misc/convergence.cir                        preserve: stdout -'doing analysis at temp = 27.000000 and tnom = 27.000000'; -'using sparse 1.3 as direct linear solver'; -'no. of
DIFF     tests/regression/misc/dollar-1.cir                           preserve-UPPER: stdout -'test: >1< should be >1<'; -'test: >1< should be >1<'; -'test: >2< should be >2<'; -(3 more); +'error: foo
DIFF     tests/regression/misc/fold-mixed-case.cir                    preserve-UPPER: stdout -'strcmpval 0'; -'strstrval 2'; +'error: keptvar: no such variable.'; +'error: keptvar: no such variable.';
DIFF     tests/regression/misc/log-functions-1.cir                    preserve-UPPER: stdout -'doing analysis at temp = 27.000000 and tnom = 27.000000'; -'using sparse 1.3 as direct linear solver'; -'
DIFF     tests/regression/misc/print-tail-case.cir                    preserve-UPPER: stdout -'v(mid) = 7.500000e-01'; +'cat: printtail.txt: no such file or directory'
DIFF     tests/regression/misc/resume-1.cir                           preserve-UPPER: stdout -'note: maxerr = 2.94934e-08'; +'pperror: syntax error in line segment'; +'(time le tstop) * trace1 + (time
DIFF     tests/regression/misc/script-case.cir                        preserve: stdout -'set-value: mixedplain'; +'error: plainfolded: no such variable.'; +'set-value:'; preserve-UPPER: stdout -'strcm
PARSE-FAIL tests/regression/misc/test-noise-2.cir                       preserve-UPPER: unknown parameter (RMODEL)
PARSE-FAIL tests/regression/misc/test-noise-3.cir                       preserve-UPPER: unknown parameter (RMODEL)
PARSE-FAIL tests/regression/model/binning-1.cir                         preserve-UPPER: could not find a valid modelname
PARSE-FAIL tests/regression/model/instance-defaults.cir                 preserve-UPPER: unknown parameter (MYRES)
PARSE-FAIL tests/regression/model/special-names-1.cir                   preserve-UPPER: could not find a valid modelname
DIFF     tests/regression/parser/bxpressn-1.cir                       preserve-UPPER: stdout -'doing analysis at temp = 27.000000 and tnom = 27.000000'; -'using sparse 1.3 as direct linear solver'; -'
DIFF     tests/regression/parser/xpressn-1.cir                        preserve-UPPER: stdout -'doing analysis at temp = 27.000000 and tnom = 27.000000'; -'using sparse 1.3 as direct linear solver'; -'
DIFF     tests/regression/parser/xpressn-2.cir                        preserve-UPPER: stdout -'doing analysis at temp = 27.000000 and tnom = 27.000000'; -'using sparse 1.3 as direct linear solver'; -'
PARSE-FAIL tests/regression/parser/xpressn-3.cir                        preserve-UPPER: Error: unknown subckt: X1084_T N1084_G N1084_N N1084_B TEST_NINT 2.6 3
DIFF     tests/regression/pz/ac-resistance.cir                        preserve-UPPER: stdout +'warning from checkvalid: vector ne is not available or has zero length.'
DIFF     tests/regression/sens/sens-ac-1.cir                          preserve-UPPER: stdout +'error: n: no such variable.'; +'error: n: no such variable.'; +'error: n: no such variable.'; +(18 more)
DIFF     tests/regression/sens/sens-ac-2.cir                          preserve-UPPER: stdout +'error: n: no such variable.'; +'error: n: no such variable.'; +'error: n: no such variable.'; +(39 more)
DIFF     tests/regression/sens/sens-dc-1.cir                          preserve-UPPER: stdout +'error: n: no such variable.'; +'error: n: no such variable.'; +'error: n: no such variable.'; +(32 more)
DIFF     tests/regression/sens/sens-dc-2.cir                          preserve-UPPER: stdout +'error: n: no such variable.'; +'error: n: no such variable.'; +'error: n: no such variable.'; +(46 more)
PARSE-FAIL tests/regression/subckt-processing/global-1.cir              preserve: Error: unknown subckt: X1 n1 su1
PARSE-FAIL tests/regression/subckt-processing/model-scope-5.cir         preserve-UPPER: Error: unknown subckt: X1 N1001_T SUB1
PARSE-FAIL tests/regression/temper/temper-1.cir                         preserve: could not find a valid modelname
PARSE-FAIL tests/regression/temper/temper-2.cir                         preserve-UPPER: could not find a valid modelname
PARSE-FAIL tests/regression/temper/temper-3.cir                         preserve-UPPER: unknown parameter (RTEST)
PARSE-FAIL tests/regression/temper/temper-res-1.cir                     preserve-UPPER: unknown parameter (RTEST)
PARSE-FAIL tests/resistance/res_array.cir                               preserve: unknown parameter (rmodel1)
PARSE-FAIL tests/sensitivity/diffpair.cir                               preserve-UPPER: could not find a valid modelname
PARSE-FAIL tests/transient/fourbitadder.cir                             preserve-UPPER: Error: unknown subckt: X1 1 2 3 4 5 6 7 8 9 10 11 12 0 13 99 FOURBIT
PARSE-FAIL tests/transmission/cpl3_4_line.cir                           preserve: Unable to find definition of model LOSSYMODE
PARSE-FAIL tests/transmission/cpl_ibm2.cir                              preserve-UPPER: Unable to find definition of model CPL1
PARSE-FAIL tests/transmission/ltra1_1_line.cir                          preserve-UPPER: could not find a valid modelname
PARSE-FAIL tests/transmission/ltra2_2_line.cir                          preserve-UPPER: could not find a valid modelname
PARSE-FAIL tests/transmission/txl1_1_line.cir                           preserve-UPPER: could not find a valid modelname
PARSE-FAIL tests/transmission/txl2_3_line.cir                           preserve-UPPER: could not find a valid modelname
PARSE-FAIL tests/vbic/CEamp.cir                                         preserve: could not find a valid modelname
SKIP     tests/vbic/FG.cir                                            stock run timed out
PARSE-FAIL tests/vbic/FO.cir                                            preserve: could not find a valid modelname
PARSE-FAIL tests/vbic/diffamp.cir                                       preserve: could not find a valid modelname
PARSE-FAIL tests/vbic/noise_scale_test.cir                              preserve: could not find a valid modelname
PARSE-FAIL tests/vbic/temp.cir                                          preserve: could not find a valid modelname
PARSE-FAIL tests/xspice/digital/d_ram.cir                               preserve-UPPER: MIF-ERROR - unable to find definition of model D_SOURCE1
PARSE-FAIL tests/xspice/digital/d_source.cir                            preserve-UPPER: MIF-ERROR - unable to find definition of model D_SOURCE1
PARSE-FAIL tests/xspice/digital/d_state.cir                             preserve-UPPER: MIF-ERROR - unable to find definition of model D_SOURCE1

129 decks: DIFF=24, OK=20, PARSE-FAIL=82, SKIP=3
```
