# Issue: Two `preserve` Gaps the New Case Decks Exposed in the Differential Sweep

## Status

Open. Both were found by running
`doc/claude/scripts/case_differential_sweep.py` over the decks added while
closing `doc/codex/issues/0009`'s follow-up table, and neither is that
issue's class: one is a parameter-name lookup and one is a control-language
function prefix. Neither is fixed, because each belongs to a name space with
its own open issue and its own design decision.

## Summary

The sweep runs every `.cir` under `tests/` three ways — stock, `preserve` on
the deck, and `preserve` on a mechanically uppercased copy. Six of the new
twin pairs report `DIFF` on the third run, and the differences reduce to two
defects, both of which reproduce by hand on a deck the sweep never touched.

### 1. An upper-case subcircuit parameter name silently loses its value

This is a wrong number with no diagnostic.

```spice
* upper M= multiplier
V1 1 0 dc 2
X1 1 2 divider M=2
R1 2 0 1k
.subckt divider a b
V2 a c dc 0
R2 c b 1k
.ends
.OPTIONS noacct
.control
op
print v(2)
.endc
.end
```

```
$ ngspice --batch mup.cir                        -> v(2) = 1.333333e+00
$ ngspice -D casemode=preserve --batch mup.cir   -> v(2) = 1.000000e+00
```

`m=2` in place of `M=2` makes the `preserve` run agree. The multiplier is
simply not applied: `inp_fix_subckt_multiplier()`'s caller
(`src/frontend/inpcom.c:4444`) asks `found_mult_param()` whether the
instance carries a parameter named `m`, and the instance parameter names
come back from `inp_get_params()` spelled as the user wrote them.

The device-letter half of that pass is fixed — `inpcom.c:4365` and `:4372`
now fold — and this is the parameter-name half, which is the same question
`doc/codex/issues/0015` asks about numparam symbols. It surfaces on
`tests/regression/case/subckt-mult-skip-case.cir` and its twin, whose
uppercased copies print `v(2) = 1.000000e+00`.

### 2. A control-language vector function prefix is not recognised in upper case

This one is a diagnostic, not a number.

```
$ ngspice --batch vmu.cir                        -> vm(2) = 8.467330e-01
$ ngspice -D casemode=preserve --batch vmu.cir   -> Error: no such function as VM,
                                                       or VM(2) is not available.
```

on a deck whose control block says `PRINT VM(2)`. The `vm`/`vp`/`vdb`/`vr`/
`vi` family is parsed as a prefix on the vector name, and under `preserve`
the reader no longer folds the command line, so the prefix arrives spelled
`VM`. The message quotes it back unfolded, so the lookup is byte-exact all
the way down.

It surfaces on the uppercased copies of the four new AC decks that print
`vm(...)`: `tests/regression/case/compat-c-behav-case.cir`,
`compat-l-behav-case.cir`, `compat-k-mutual-case.cir` and
`tests/regression/case-lt/rkm-c-case.cir`, `rkm-l-case.cir`, with their
twins.

## Impact

Gap 1 is the more serious of the two, and it is the shape this whole series
has been trying to rule out: a deck that runs, prints numbers, and prints
the wrong ones, with nothing on stderr. `M=` on a subcircuit instance is
ordinary SPICE house style.

Gap 2 cannot change a number — the run stops at the `print` — but it makes
every AC deck written in house style unusable under `preserve`.

Neither is a regression. Both were reachable before this series and are
newly *visible* because the decks that exercise them did not exist.

## Root Cause

Gap 1: `inp_get_params()` returns instance parameter names byte-exact, and
`found_mult_param()` compares them against the literal `"m"`. Under `fold`
the reader had already made those equal.

Gap 2: the control language's vector-function prefix is matched against
lower-case literals on text the reader stopped folding. It is the same
family as `doc/codex/issues/0016` (instance-name matchers outside
`DEVnameHash`) and adjacent to the `distinguish`-gated vector-table aliasing
at `src/frontend/vectors.c`.

## Acceptance Criteria

1. `M=`, `m=` and `M =` on a subcircuit instance all select the multiplier,
   and `tests/regression/case/` carries a twin pair proving the numbers
   agree.
2. `PRINT VM(2)`, `VDB(2)` and `VP(2)` resolve under `preserve`, with a twin
   pair in `tests/regression/case/`.
3. The differential sweep's `DIFF` count drops by the twelve entries these
   two gaps account for, and `NUM-DIFF` stays 0.
4. `make check` unchanged with `casemode` unset.

## Resolution

Not fixed.

Gap 1 must move with `doc/codex/issues/0015`: subcircuit parameter names and
`.param` symbols are one name space, and folding one side of it without the
other is what `doc/codex/issues/0013`'s Resolution argues against for its
three exact-match call sites.

Gap 2 is control-language surface. `doc/codex/issues/0011` already covers one
way the preprocessor damages command text, and the `distinguish` design
decision for the vector table is still open; a fold here should be taken
together with those rather than in isolation.

Both are recorded in `doc/claude/checklists/phase2-differential-sweep.md` as
the reason twelve of the new decks report `DIFF` rather than `OK`.
