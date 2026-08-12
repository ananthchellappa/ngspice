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

## Re-run after `doc/codex/issues/0009` was fixed

The numbers above are the state at the head of the Phase 2 series and are kept
as the baseline. After the two commits that fold the leading device letter in
`src/frontend/inpcom.c` and in `src/frontend/numparam/spicenum.c`, the same
command reports:

| verdict | before | after |
| --- | ---: | ---: |
| OK | 20 | 67 |
| DIFF | 24 | 39 |
| PARSE-FAIL | 82 | 25 |
| NUM-DIFF | 0 | 0 |
| SKIP | 3 | 3 |
| **total** | **129** | **134** |

Compared per deck rather than on the totals, no deck's verdict got worse. The
five extra decks are the regression tests the fix added, and all five report OK.

The 25 that remain are not this defect. 20 reference a model in one case and
define it in another inside the deck itself (`MP10 ... p12l5` against `.MODEL
P12L5 PMOS` in `tests/mos6/simpleinv.cir`, `m1 ... n1` against `.Model N1 NMOS`
in `tests/bsim3soidd/nmosdd.mod`), and 3 more are the `lib-processing` decks
where the sweep uppercases the deck but not the `.lib` file it reads — all of
that is `doc/codex/issues/0010`. The last 2, `tests/polezero/pz{2,t}.cir`, are
a keyword fold rather than a name lookup: `search_plain_identifier()` is
`strstr`-based, so the `ac`-with-no-value fixup does not fire on `AC`. That is
written up as `doc/codex/issues/0013`.

## Re-run after `doc/codex/issues/0010` was fixed

After the three commits that fold the lookup key for the `.model`, `.subckt`
and `.global` name spaces, the same command reports:

| verdict | after 0009 | after 0010 |
| --- | ---: | ---: |
| OK | 66 | 96 |
| DIFF | 40 | 41 |
| PARSE-FAIL | 25 | 2 |
| NUM-DIFF | 0 | 0 |
| SKIP | 3 | 3 |
| **total** | **134** | **142** |

Both columns were measured with the same command on the same tree, immediately
before and after the series; the "after 0009" column differs by one deck from
the figures in the table above (OK=67, DIFF=39) because one timing-sensitive
deck moves between consecutive runs.

Compared per deck rather than on the totals, no deck's verdict got worse and no
deck that was OK became anything else. 20 decks went `PARSE-FAIL` to `OK`, 2
went `DIFF` to `OK`, and 3 — `tests/bsim3soi{dd,fd,pd}/ring51.cir` — went
`PARSE-FAIL` to `DIFF` for a warning, not a number: their card
`cout  buf ss 1pF` has an upper-case unit suffix, and `is_a_modelname()`'s test
at `inpcom.c:3206` is case-sensitive, so `1pF` is reported as a missing model.
That is on issue 0009's follow-up list. The 8 extra decks are the regression
tests issue 0010 added, and all 8 report OK.

The 2 remaining `PARSE-FAIL`s are `tests/polezero/pz{2,t}.cir`, which are
`doc/codex/issues/0013`. The 3 `SKIP`s are unchanged stock-run timeouts.

## Re-run after `doc/codex/issues/0013` was fixed

After the commit that folds the keyword literal inside `search_identifier()`
and `search_plain_identifier()`, the same command reports:

| verdict | after 0010 | after 0013 |
| --- | ---: | ---: |
| OK | 95 | 108 |
| DIFF | 42 | 43 |
| PARSE-FAIL | 2 | 0 |
| NUM-DIFF | 0 | 0 |
| SKIP | 3 | 3 |
| **total** | **142** | **154** |

Both columns were measured with the same command on the same tree, immediately
before and after the commit. The "after 0010" column differs by one deck from
the figures above (OK=96, DIFF=41) for the usual reason: one timing-sensitive
deck moves between consecutive runs.

Compared per deck rather than on the totals, exactly three decks moved and none
of them got worse. `tests/polezero/pz{2,t}.cir` went `PARSE-FAIL` to `DIFF`,
and `tests/regression/model/binning-1.cir` went `DIFF` to `OK`. The 12 extra
decks are the six twin pairs the fix added under `tests/regression/case/`, and
all 12 report OK. **`PARSE-FAIL` is now 0**, which is the first time acceptance
criterion 4 of `doc/codex/issues/0009` has been met.

The two `polezero` decks land in `DIFF` rather than `OK` for a warning, not a
number: their `l1 1 0 1H` cards carry an upper-case unit suffix, and
`is_a_modelname()`'s test at `inpcom.c:3239` is case-sensitive, so under
`preserve` `1H` is reported as `warning, can't find model '1h'`. That is on
issue 0009's follow-up list. The 3 `SKIP`s are unchanged stock-run timeouts.

**`NUM-DIFF` has been 0 at every measurement in this table.**

## Interpretation

`preserve` does not change any number it can compute. That was true at the head
of Phase 2 and it has stayed true at every measurement since.

What `preserve` could not do at the head of Phase 2 was parse a deck written in
the ordinary SPICE house style, because upper-case device letters broke card
classification in the preprocessor. That was one defect with four or five
sites, mechanical to fix, and it was Phase 1's enumeration gap rather than
Phase 2's plumbing — the Phase 1 census wrote down that it does not count
character-literal comparisons. It is `doc/codex/issues/0009`, and its two
residues — name lookup by `strcmp` (`doc/codex/issues/0010`) and the keyword
searches hidden behind a helper (`doc/codex/issues/0013`) — are also closed.
Every deck under `tests/` whose stock run parses now parses under `preserve`
and under `preserve` on a mechanically uppercased copy.

Gaps recorded rather than fixed: the `gnd` rewrite runs over command text it
should not touch (`doc/codex/issues/0011`), `numnodes()` dispatches on a
lower-case device letter (`0014`), numparam symbol names are byte-exact
(`0015`), instance-name matchers outside `DEVnameHash` (`0016`), and
`inp_quote_params()` adjusts the terminal count on a lower-case device letter
(`0017`). The 43 `DIFF` decks remain the four groups described above plus
issue 0009's follow-up table.

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

## Re-run after `doc/codex/issues/0009`'s follow-up table was closed

After the nine commits that fold every remaining first-character
device-letter test and every lower-case-only character scan set in
`src/frontend/inpcom.c`, plus the two guards in `src/frontend/inp.c` and the
two in `src/frontend/inpcompat.c`, the same command reports:

| verdict | after 0013 | after the follow-up table |
| --- | ---: | ---: |
| OK | 107 | 157 |
| DIFF | 45 | 53 |
| PARSE-FAIL | 0 | 0 |
| NUM-DIFF | 0 | 0 |
| SKIP | 3 | 3 |
| **total** | **155** | **213** |

Both columns were measured with the same command on the same tree,
immediately before and after the series. The "after 0013" column differs
from the figures in the previous section (OK=108, DIFF=43, total=154) for the
usual reason plus one: one timing-sensitive deck moves between consecutive
runs, and the deck count differs by one from the figure recorded then.
**The baseline used here is the one measured on this tree, not the one
quoted in the previous section.**

Compared per deck rather than on the totals: **no deck's verdict got worse,
and no deck that was OK became anything else.** Exactly four pre-existing
decks moved, all of them `DIFF` to `OK`, and all four on the single commit
that folds `is_a_modelname()`'s `f`/`h` unit suffix:

```
tests/polezero/pz2.cir              DIFF -> OK   ("can't find model '1h'")
tests/polezero/pzt.cir              DIFF -> OK
tests/general/schmitt.cir           DIFF -> OK   ("'5pf'")
tests/regression/misc/ac-zero.cir   DIFF -> OK   ("'1uh'", "'1uf'")
```

That was measured with `--filter` immediately after that commit as well as
in the full run, because the previous section's prediction — that fixing
`inpcom.c:3239` should move `pz{2,t}` and `schmitt` — was a prediction and
not a measurement. It moved a fourth deck nobody had listed.

The three `bsim3soi{dd,fd,pd}/ring51.cir` decks did **not** move, as
predicted: their `1pF` warning is already gone and what remains is transient
`reference value` timing, which differs between two consecutive *stock* runs.

The 58 extra decks are the 29 twin pairs this series added: 20 under
`tests/regression/case/`, three under the new `tests/regression/case-lt/`,
two under the new `tests/regression/case-pspice/` and four under the new
`tests/xspice/case/`. 46 of the 58 report OK.

### The twelve new decks that report DIFF, and why

None of them is a defect this series introduced, and none is a numeric
difference in the deck as written — every one is the sweep's *uppercased*
copy. They are written up as `doc/codex/issues/0020`.

- **Eight** — `compat-c-behav-case`, `compat-l-behav-case`,
  `compat-k-mutual-case` and `case-lt/rkm-c-case`, `rkm-l-case`, with their
  twins — print `vm(...)`. Uppercased that becomes `PRINT VM(2)`, and under
  `preserve` the control language does not recognise the `vm`/`vp`/`vdb`
  vector-function prefix in upper case: `Error: no such function as VM`.
  This is the same class as the group-2 uppercasing artifacts already
  described above, except that it is a real `preserve` gap rather than a
  limitation of the sweep's rule — it reproduces on a hand-written deck.
- **Four** — `subckt-mult-skip-case` and its twin — carry `X1 1 2 divider
  m=2`. Uppercased that is `M=2`, and under `preserve` the multiplier is not
  recognised, so the subcircuit is instantiated once instead of twice and
  `v(2)` reads `1.000000e+00` instead of `1.333333e+00`. That is a wrong
  number with no diagnostic, it reproduces by hand, and it is the more
  serious of the two. It is the parameter-name half of the pass whose
  device-letter half this series just fixed, and it has to move with
  `doc/codex/issues/0015`.

**Corrected 2026-08-11: the twelve is right and the split is not.** The two
bullets say eight and four; the decks they name are **ten** and **two**. Five
decks with their twins is ten — `tests/regression/case-lt/` carries
`rkm-c-case-lower.cir` and `rkm-l-case-lower.cir` as well — and
`subckt-mult-skip-case` with its twin is two, which is what
`doc/codex/issues/0020`'s gap 1 Resolution says it turned out to be. Ten of
the ten cleared when gap 2 was fixed and both of the two cleared when gap 1
was. The mis-split is what `0020`'s acceptance criterion 3 inherited.

**`NUM-DIFF` has been 0 at every measurement in this table, and `PARSE-FAIL`
has been 0 since `doc/codex/issues/0013`.**

### What the new test directories are not

`tests/regression/case-lt/`, `tests/regression/case-pspice/` and
`tests/xspice/case/` need `-D ngbehavior=lt`, `-D ngbehavior=ps` and a
`spinit` that loads code models respectively, and the sweep passes none of
those. Their decks therefore fail in the sweep's *stock* run too, so the
sweep reports them OK — three failing runs that agree — rather than
`PARSE-FAIL`. Fourteen of those eighteen decks report OK for that reason and
not because anything was proved about them; the other four are the
`rkm-c-case`/`rkm-l-case` pairs described above. `make check` is what covers
those directories.

## Re-run after `doc/codex/issues/0019`

`doc/codex/issues/0019` folded the seven remaining lower-case
card-classification sites — three in `src/frontend/inpcompat.c`, three in
`src/frontend/inp.c`, two in `src/frontend/inpcom.c` — and added eight twin
pairs. The same command reports:

| verdict | after the follow-up table | after issue 0019 |
| --- | ---: | ---: |
| OK | 157 | 173 |
| DIFF | 53 | 53 |
| PARSE-FAIL | 0 | 0 |
| NUM-DIFF | 0 | 0 |
| SKIP | 3 | 3 |
| **total** | **213** | **229** |

Both columns were measured on this tree with the same command, the left one
immediately before the series and the right one immediately after. As
before, the baseline used here is the one measured on this tree rather than
the figures quoted in the previous section.

Compared per deck rather than on the totals: **no deck's verdict got worse,
no deck that was OK became anything else, and no pre-existing deck moved at
all.** The comparison is a straight `diff` of the two runs' non-OK lists,
which the sweep prints in full; the only entries that differ are the sixteen
new decks, and all sixteen report OK.

The three SKIPs are unchanged — `tests/mesa/mesa-12.cir`, `tests/mesa/mesa12.cir`
and `tests/vbic/FG.cir`, whose *stock* run exceeds the 90 s timeout.

### The twelve `doc/codex/issues/0020` decks did not grow to fourteen

Two of the eight new pairs reached a plot the analysis does not leave
current, and the obvious way to write that — `setplot noise2`, `setplot op1`
— is exactly the control-language-identifier gap of `doc/codex/issues/0020`:
plot names are generated in lower case, so the sweep's uppercased copy got
`error: no such plot named noise2`. `print mag(v(2))` had the same problem
one level down, because the function name is echoed back as the printed
label.

Measured first, then fixed in the decks rather than left as a footnote: the
noise pair drops `setplot` entirely (the noise analysis already leaves
`noise2` current) and the `noopac` pair uses `setplot previous`, which
reaches the same plot and survives uppercasing. Both pairs report OK. The
count of new-case decks reporting `DIFF` therefore stays at twelve, and all
twelve are still the two `doc/codex/issues/0020` gaps described above.

### What the sweep still cannot see, and what covered it instead

Five of the seven sites are invisible to the sweep by construction, and this
is the round where that stopped being a theoretical remark:

- `inpcompat.c:1101` and `:1153` need `-D ngbehavior=ps`, `:1771` needs
  `-D ngbehavior=lt`; the sweep passes neither.
- `inp.c:2454` needs `.options savecurrents`, which no deck under `tests/`
  set before this round.
- `inp.c:2030` needs `.options noopac` together with `.options keepopinfo`.

Two of those five were silently wrong numbers — the PSpice substrate rewrite
and the LTspice `noiseless` translation — so for this round `make check` was
the harness that mattered and the sweep was the regression guard. That is the
reverse of `doc/codex/issues/0009`, where the sweep found the defect and
`make check` could not see it.

## Re-run after `doc/codex/issues/0015` and `0020` gap 1

`doc/codex/issues/0015` folded the numparam symbol key at the single probe and
the single insert, plus the `.func` name and `.func` formal machinery in
`src/frontend/inpcom.c` that this issue's premise had mistaken for numparam;
`doc/codex/issues/0020` gap 1 folded `inp_fix_inst_line()`'s
formal-versus-instance parameter-name match; and `doc/codex/issues/0013`'s three
exact-match searches were folded with them. Eight new twin pairs. The same
command reports:

| verdict | after issue 0019 | after 0015 + 0020 gap 1 |
| --- | ---: | ---: |
| OK | 173 | 191 |
| DIFF | 53 | 51 |
| PARSE-FAIL | 0 | 0 |
| NUM-DIFF | 0 | 0 |
| SKIP | 3 | 3 |
| **total** | **229** | **245** |

Both columns were measured on this tree with the same command, the left one
immediately before this round and the right one immediately after.

Compared per deck rather than on the totals — a straight `diff` of the two runs'
non-OK lists, which is all the sweep prints — the entire delta is two lines:

```
< DIFF tests/regression/case/subckt-mult-skip-case-lower.cir
< DIFF tests/regression/case/subckt-mult-skip-case.cir
```

plus the sixteen new decks, none of which appears in either non-OK list, i.e.
all sixteen report OK. **No deck's verdict got
worse, no deck that was OK became anything else, and no pre-existing deck moved
except those two, which went from `DIFF` to `OK`.** They are gap 1: their
uppercased copies used to print `v(2) = 1.000000e+00` instead of
`1.333333e+00`, and that was the last enumerated silent wrong number.

The three SKIPs are unchanged — `tests/mesa/mesa-12.cir`, `tests/mesa/mesa12.cir`
and `tests/vbic/FG.cir`, whose *stock* run exceeds the 90 s timeout.

### The remaining 51, and the one new issue that explains three of them

The eight `PRINT VM(2)` entries are still `doc/codex/issues/0020` gap 2, which
is out of scope for this round: it is control-language surface and wants the
`distinguish` decision taken with `doc/codex/issues/0011`.

**Corrected after that fix landed, 2026-08-11.** There are **ten** of them,
not eight; see the correction under "The twelve new decks that report DIFF"
above, which is where the mis-split originates. And `PRINT VM(2)` is not the
defect: `vm` is a *user-defined function* that `ft_cpinit()` installs from
`cpitf.c`'s `udfs[]` table, and the byte-exact compare was `ft_substdef()`'s,
so every `define` in the control language had it. All ten cleared, with no
deck newly differing, when
`doc/claude/decisions/0013-user-defined-function-identity.md` landed.

Three more are now attributable to a single new defect. The uppercased copies of
`tests/regression/parser/xpressn-1.cir`, `xpressn-2.cir` and `xpressn-3.cir`
fail *entirely* on `doc/codex/issues/0022`: numparam's built-in function list is
matched byte-exactly, so `{SQRT(x)}`, `{NINT(x)}` and the rest of `fmathS` are
looked up as parameters under `preserve`. Measured after 0015 landed, so no
other gap is masking it — on `xpressn-1.cir` every diagnostic is
`Undefined parameter [<FUNCTION NAME>]`. One ungated `keyword()` compare.

**Corrected after the fix landed.** The count in this paragraph's heading is
wrong: `doc/codex/issues/0022` explains *one* of the three, not three. The
`keyword()` fold removed every `Undefined parameter` from all three uppercased
decks, but only `xpressn-2.cir` reached `OK`; `xpressn-1.cir` and
`xpressn-3.cir` stayed `DIFF` on a second mechanism that was hidden behind the
aborts and that is not an ngspice defect. See "Re-run after
`doc/codex/issues/0022` and `0023`" below.

### What the sweep could not see this round, again

The `.func` half of `doc/codex/issues/0015` and all three of
`doc/codex/issues/0013`'s exact-match sites are invisible to the sweep by
construction: the sweep uppercases a whole deck, so the definition and the
reference move together and the mismatch it is looking for cannot arise. Every
one of those four mechanisms needed a hand-written twin pair under
`tests/regression/case/`, and each pair had to be run against an empty `.out`
first, because the failure mode is an abort on stderr and `check.sh` compares
stdout only. This is the reverse of `doc/codex/issues/0009` and the same shape
as `doc/codex/issues/0019`: for this round `make check` was the harness that
mattered, and the sweep was the regression guard that proved nothing else moved.

## Re-run after `doc/codex/issues/0022` and `0023`

`doc/codex/issues/0022` folded the deck side of numparam's built-in function
list inside `keyword()`; `doc/codex/issues/0023` deleted the dead parameter
pair the subcircuit multiplier pass wrote into its caller's arrays, which has
no behavioural surface. One new twin pair. The same command reports:

| verdict | after 0015 + 0020 gap 1 | after 0022 + 0023 |
| --- | ---: | ---: |
| OK | 191 | 195 |
| DIFF | 51 | 49 |
| PARSE-FAIL | 0 | 0 |
| NUM-DIFF | 0 | 0 |
| SKIP | 3 | 3 |
| **total** | **245** | **247** |

Both columns were measured on this tree with the same command, the left one
immediately before this round and the right one immediately after. The baseline
run was started before the two new decks existed, which is why its total is 245
rather than 247; the per-deck comparison is unaffected.

Compared per deck rather than on the totals — a straight `diff` of the two
runs' non-OK lists, which is all the sweep prints — the entire delta is two
lines:

```
< DIFF tests/regression/model/binning-1.cir
< DIFF tests/regression/parser/xpressn-2.cir
```

plus the two new decks, neither of which appears in either non-OK list, i.e.
both report `OK`. **No deck's verdict got worse and no deck that was `OK`
became anything else.**

Only one of those two lines belongs to this round.

### `binning-1.cir` is a false `DIFF`, not a fix

Its baseline detail was `preserve-UPPER: stdout +'reference value :
0.00000e+00'`. That string is `src/frontend/outitf.c:695`, inside a block
guarded by

```c
if ((currclock-lastclock) > (0.25*CLOCKS_PER_SEC)) {
    fprintf(stdout, " Reference value : % 12.5e\r", ...);
```

— a progress line printed at most every quarter second of CPU time. Whether it
appears at all depends on how loaded the machine is, so a 12-job sweep can
produce it in one of the three runs and not in the others. It is not in the
sweep's `NOISE` regex, so it becomes a `DIFF`.

Measured rather than assumed: with both fixes stashed, the tree rebuilt at
`a1161ccdc` and the sweep run with `--filter model/binning-1`, `binning-1.cir`
reports `OK` twice. The deck also contains no braces, no `.param` and no
quotes, so numparam's expression evaluator never runs on it and the
`keyword()` fold cannot reach it.

The sweep script was deliberately **not** changed this round, so the two runs
compared above used a byte-identical tool. The fix for the next round is one
alternation in `NOISE` in `doc/claude/scripts/case_differential_sweep.py`:
`reference value`.

### What is left on `xpressn-1.cir` and `xpressn-3.cir`

Not `doc/codex/issues/0022`, and not an ngspice defect. Their remaining detail
is `+'error: n: no such variable.'` (the sweep lower-cases both sides before
comparing, so the real message is `Error: N: no such variable.`).

Those decks drive their self-check from a control-language loop that composes a
vector name inside a **double-quoted** string:

```
foreach n $&tests
  set n_test = "n{$n}_t"
```

The sweep's uppercaser leaves double-quoted text alone by design — uppercasing
a quoted path would test the filesystem — so in the `preserve-UPPER` copy the
loop variable becomes `N` while the quoted reference still asks for `$n`. Under
`preserve` that is *correct* case-sensitive behaviour: the deck the sweep
produced is internally inconsistent, and ngspice is right to say the variable
does not exist.

Confirmed by uppercasing the file in full, quotes included, which is not what
the sweep does:

```
$ tr 'a-z' 'A-Z' < tests/regression/parser/xpressn-1.cir > XP1.cir
$ ngspice -D casemode=preserve --batch XP1.cir
INFO: 0 OF 118 TESTS FAILED          # 0 'Undefined parameter', 0 'no such variable'
```

Five decks share that idiom and are the whole `n: no such variable` family in
the sweep: `tests/regression/parser/xpressn-1.cir`, `xpressn-3.cir`,
`bxpressn-1.cir`, `tests/regression/subckt-processing/global-1.cir` and
`model-scope-5.cir`. The `tests/regression/sens/sens-*.cir` entries print the
same message from the same shape, `foreach n i1_acmag c1 r1` with
`set n_test = "$n"`. None of them is a fold site; they are an artefact of the
transform, and the honest reading is that these six-plus decks are outside what
this sweep can decide. That is a limit of the tool, not a defect list.

`xpressn-2.cir` has no such loop — its checks are written out one `if` per
line — which is why it is the one of the three that reached `OK`.

### What the sweep could not see this round

`doc/codex/issues/0022`'s own mechanism, again by construction: the sweep
uppercases a whole deck, so a built-in and its arguments move together and the
deck still parses. Only the fact that the *list* is lower case makes the
uppercased copy fail, which is why the sweep saw it at all — and the fold-mode
side of the change is invisible to the sweep entirely, since
`tests/regression/case/` runs its whole suite under `-D casemode=preserve`.
The fold-mode argument had to be made against the reader's exemption chain and
measured by hand; it is written out in `doc/codex/issues/0022`.

`doc/codex/issues/0023` is invisible to both harnesses: a leak and a latent
one-past-the-end write, neither of which changes a number. Its evidence is
`valgrind`, quoted in that issue.

## Re-run after `doc/codex/issues/0011`, `0014` and `0016` class (a)

`doc/codex/issues/0011` scoped the `gnd` rewrite off command text;
`doc/codex/issues/0014` folded the device letter `numnodes()` dispatches on;
`doc/codex/issues/0016` class (a) folded three one-character device-letter
comparisons in `gens.c`, `device.c` and `outitf.c`. Eight new decks, seven of
them in `tests/regression/case/`. The same command reports:

| verdict | after 0022 + 0023 | after 0011 + 0014 + 0016(a) |
| --- | ---: | ---: |
| OK | 197 | 204 |
| DIFF | 47 | 48 |
| PARSE-FAIL | 0 | 0 |
| NUM-DIFF | 0 | 0 |
| SKIP | 3 | 3 |
| **total** | **247** | **255** |

**Both columns were measured on this tree with the amended tool.** The `NOISE`
one-liner the previous round asked for — `reference value`, commit
`351d95d8f` — was taken **before** the baseline run, precisely so that the two
runs would use a byte-identical script.

The left-hand column therefore reads 197/47 where the previous round recorded
195/49 for the same tree. Two decks differ, and **which two is not determined
here**: the previous round did not preserve a per-deck list, and both candidate
causes are non-behavioural — the amended `NOISE` alternation, and the
`reordered only` run-to-run flip demonstrated on `ring51.cir` below. It does
not need determining, because the comparison that decides is before against
after *within this round*, both measured with the same script on the same tree.
`binning-1.cir` is not one of the two: it reports `OK` in both of this round's
runs, and it was already `OK` in the previous round's closing numbers.

The baseline run was started before the eight new decks existed, which is why
its total is 247 rather than 255; the per-deck comparison is unaffected.

### The sweep was a pure regression guard this round

Stated in advance and confirmed after. None of the three issues clears a single
sweep entry, because no deck under `tests/` uses `show`, `showmod`,
`save alli`, an upper-case `E`/`G`/`W`/`K`/`X` inside a `.subckt`, or a `gnd`
token in command text. Its job was to prove nothing moved, and the decisive
harness was `make check` plus hand-written twin pairs, as it was for `0015`,
`0019` and `0022`.

Compared per deck rather than on the totals, the entire delta is three lines:

```
< DIFF tests/bsim3soidd/ring51.cir
> DIFF tests/regression/case/alter-rebin-case-lower.cir
> DIFF tests/regression/case/alter-rebin-case.cir
```

**No deck's verdict got worse and no deck that was `OK` became anything else.**
Neither of the two remaining lines is a fix and neither is a regression.

### `ring51.cir` is a run-to-run flip, not a fix

`tests/bsim3soidd/ring51.cir` reported `DIFF` in the baseline and `OK` in the
after-run, with detail `preserve: stdout reordered only; preserve-UPPER: stdout
reordered only` — the sweep found the same lines in a different order, not
different numbers. Its identical twin `tests/bsim3soifd/ring51.cir` carries the
same detail and did **not** flip, which is the tell.

Measured rather than assumed: re-running the sweep with `--filter ring51` on
the *fixed* binary reports `DIFF` for both decks again. So the deck's verdict on
the tree as it now stands is unchanged from the baseline, and the after-run's
`OK` was the flake. `reordered only` is a nondeterministic class the sweep
cannot currently normalise; it joins the `binning-1.cir` lesson as a reason to
compare per deck and to re-measure a single-deck flip before calling it a fix.

### `alter-rebin-case.cir` reports `DIFF`, and that is a new issue

The new twin pair for `doc/codex/issues/0016`'s `device.c:1489` half is `DIFF`
in the `preserve-UPPER` column only, with detail:

```
-'notice: model has changed from nch.1 to nch.2.'
+'v(3) = 1.340324e+00'
```

The sweep uppercases the whole deck, so the `alter` command becomes
`ALTER M1 W=2U`. `doc/codex/issues/0016`'s fix at `device.c:1489` admits it —
that guard is `eqc(param, "w")` and always was — and the body it calls then
tests `param[0] == 'w'` byte-exactly at `device.c:1272`, takes the `else` arm,
assigns the new width to the local **length**, and bins on the unchanged width.

This is a real defect and a new one: `doc/codex/issues/0026`. It is **not** a
regression from this round — a binary built with all three fixes stashed
produces `v(3) = 1.340324e+00` for the same deck, i.e. the identical wrong
number. The committed decks type `w=2u` in lower case and pass; only the
sweep's mechanical uppercasing reaches the fourth site.

Worth recording as a method note: this is the first time the sweep has found
something the hand-written decks did not, and it found it *by uppercasing a deck
that was written for a different question*. The new decks are worth having in
the sweep's input set for exactly that reason.

### What the sweep could not see this round

All three fixes, by construction, and this was known before the runs started.
`0011` lives in `.control` command text, which no deck under `tests/` puts a
`gnd` token into; `0014` needs an upper-case `E`/`G`/`W`/`K` inside a
`.subckt`, which no deck has; `0016` class (a) needs `show`, `showmod` or
`save alli`, which no deck uses. The fold-mode half of `0011` — the half that
matters, since it is the only defect in the series that is wrong with no `-D`
flag — is invisible to the sweep twice over, because the sweep only ever
compares `preserve` against stock and never asserts anything about stock alone.
That half is pinned by `tests/regression/misc/gnd-command-text.cir`, which
carries no flag at all.

## Re-run after `doc/codex/issues/0026` and Phase 3 gate 3

`doc/codex/issues/0026` folded the parameter letter `if_set_binned_model()`
tests; Phase 3 gate 3 made `casemode=distinguish` real and stopped the frontend
vector table aliasing case-variant names. Seven new decks: two in
`tests/regression/case/` and five in the new `tests/regression/casedist/`. The
same command reports:

| verdict | baseline (HEAD `a47808879`) | after 0026 + gate 3 |
| --- | ---: | ---: |
| OK | 204 | 208 |
| DIFF | 48 | 51 |
| PARSE-FAIL | 0 | 0 |
| NUM-DIFF | 0 | 0 |
| SKIP | 3 | 3 |
| **total** | **255** | **262** |

Both columns were measured on this tree with the same script. The baseline was
taken before any change in this session and reproduces the previous round's
closing numbers exactly, which is the first time that has happened and is worth
recording: the `ring51.cir` flip landed on `tests/bsim3soifd` this time and on
`tests/bsim3soidd` last time, and the totals were unaffected.

Compared per deck rather than on the totals, the entire delta is three lines:

```
> DIFF tests/regression/case/alter-rebin-param-case-lower.cir
> DIFF tests/regression/case/alter-rebin-param-case.cir
> DIFF tests/regression/casedist/harness-alive.cir
```

**No deck's verdict got worse and no deck that was `OK` became anything else.**
All three additions are decks that did not exist in the baseline.

### `alter-rebin-case.cir` did not clear, and 0026 was still fixed

The prompt for this session predicted that fixing `doc/codex/issues/0026` would
clear the two `alter-rebin-case{,-lower}.cir` `DIFF`s. It did not, and the
reason is worth having in writing, because the totals hide it. What changed is
the *reason*, not the verdict:

```
baseline  alter-rebin-case.cir   preserve-UPPER: stdout
                                 -'notice: model has changed from nch.1 to nch.2.'
                                 +'v(3) = 1.340324e+00'
after     alter-rebin-case.cir   preserve: stdout reordered only;
                                 preserve-UPPER: stdout reordered only
```

`1.340324e+00` was the defect: the width was applied without re-binning. It is
gone from both decks. What remains is the `reordered only` class — the sweep
found the same lines in a different order — which is the same nondeterministic
class as `ring51.cir` and is not a numeric disagreement at all. The two new
`alter-rebin-param-case{,-lower}.cir` decks join the same class for the same
reason: the `Notice: model has changed` line is emitted on a different stream
from the `print` output, so its position relative to the voltages is not
stable.

**A verdict of `DIFF` is therefore not by itself evidence of a defect, and the
detail column is the thing to read.** That is the third lesson of this kind,
after `binning-1.cir` and `ring51.cir`.

### The two `harness-alive.cir` decks are `DIFF` by design

```
preserve: stdout -'error: casemode: no such variable.'; -'casemode-is';
                 +'casemode-is preserve'
```

Both liveness probes `echo CASEMODE-IS $casemode`, which is the whole point of
them: the variable is unset in the sweep's stock column and set in its preserve
column, so the two columns must differ. `tests/regression/case/harness-alive.cir`
has reported `DIFF` for this reason since it was written and did so in the
baseline too.

**Added 2026-08-11**: `tests/regression/case/udf-name-case.cir` and its twin
join that by-design list, on one line — `+'vm (x) = mag (v (x))'`. Their last
case defines `VM(x)` beside the shipped `vm`, and under `preserve`
`com_define()`'s replace-or-prepend lookup is a byte-exact *prefix* test, so
the shipped entry is shadowed rather than replaced and `define vm` lists three
entries where the stock column's folded card lists two. That is
`doc/claude/decisions/0013-user-defined-function-identity.md` decision 2's
residue and `doc/codex/issues/0051`; both entries should clear when it lands,
which makes them a live check on it rather than noise.

### What the sweep could not see this round

`casemode=distinguish`, entirely. The sweep runs stock, `preserve` and
`preserve` on a mechanically uppercased copy; it has no `distinguish` column and
adding one would be meaningless, because uppercasing a deck under `distinguish`
changes the circuit rather than only its spelling. The four
`tests/regression/casedist/*-case-split.cir` decks are `OK` in the sweep for
exactly that reason — all three of its columns run them in a mode where the two
spellings are one name — and their real assertion is `make check`, which runs
that directory with `-D casemode=distinguish`.

Its job this round was the same as last: prove that fifteen identity call sites
changed predicate without moving a single number in `fold` or `preserve`. It
did.
