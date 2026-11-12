# Receipt 03: final state

Tree: `ver_53` at `e96524b85` (code as of `c600cedfe`).

## make check

`env -u DISPLAY make -C build-ver_53 -k check`,
`build-ver_53/claude-check-final.log`: **434 PASS, 33 FAIL.** The 33 `FAIL:`
lines are the 33 of the baseline, line for line (receipt 01).

| directory | baseline PASS | now PASS |
| --- | --- | --- |
| tests/regression/stb (new) | - | 59 |
| tests/regression/case | 118 | 123 |
| tests/regression/casedist | 30 | 34 |
| tests/xspice/analog | 1 | 3 |
| tests/xspice/digital | 8 | 9 |
| every other directory | unchanged | unchanged |

`xvfb-run -a make -C build-ver_53/tests/regression/pipe -k check`: 32 PASS,
1 FAIL (`resource-keyword-case.cmd`), as at the baseline. With it the tree
stands at 466 PASS and 1 FAIL.

`make -C build-ver_53/tests/lint check`: `identity lint: 11 comparisons,
baseline matches`, both specs pass; `tests/lint/identity.baseline` has no
change in this batch.

## The fold-style decks under the other modes (by hand, not in make check)

The 59 decks of `tests/regression/stb` are lower case. Run with
`-D casemode=preserve` and `-D casemode=distinguish`:

| mode | verdicts pass | verdicts fail | the failing ones |
| --- | --- | --- | --- |
| preserve | 338 | 2 | `fail-upper-case-name`, `errors-upper-case-name`: each expects the message to name the probe in upper case as typed; under preserve the word takes the spelling the symbol table saw first |
| distinguish | 334 | 6 | `names-upper-case-path`, `names-upper-case-top`, `loopgain-port-upper-case`, `loopgain-klu-upper-case`, `alias-port-upper-case`, `alias-letter-upper-case`: each types a lower-case name of the deck in upper case, which is another name under distinguish |

All eight are what the mode means, not defects.

## Shared library

A second build, `../configure --with-ngshared --enable-pss --enable-cider`,
and a small C client of `libngspice`. Four rounds (fold, preserve,
distinguish, fold); each round: `ngSpice_Init`, `set casemode=...`, a deck
that fails to load while its probes are collected (unknown subcircuit),
`remcirc`, a deck with a `.stb` card on a port of a subcircuit instance,
`run`, the `stb` command with the same terminal by position, the command
`reset` followed by `alter` and `run`, a second load of the deck, `run`,
`ngSpice_Reset`. Checked each time: exactly one `inserted as` line per load,
no `Error: stb` line, and `loopgain[0]` equal to the closed form to 1e-9.
16 of 16 checks pass.
