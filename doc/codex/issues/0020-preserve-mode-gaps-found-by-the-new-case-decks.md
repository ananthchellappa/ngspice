# Issue: Two `preserve` Gaps the New Case Decks Exposed in the Differential Sweep

## Status

Gap 1 fixed; gap 2 still open. Both were found by running
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
`inp_fix_inst_line()` matched them against the subcircuit's formal names with
`strcmp`. Under `fold` the reader had already made those equal.

This paragraph originally named `found_mult_param()`, and that was wrong at
`e5913af03` — see the Resolution.

Gap 2: the control language's vector-function prefix is matched against
lower-case literals on text the reader stopped folding. It is the same
family as `doc/codex/issues/0016` (instance-name matchers outside
`DEVnameHash`) and adjacent to the `distinguish`-gated vector-table aliasing
at `src/frontend/vectors.c`.

## Acceptance Criteria

1. **Met.** `M=`, `m=` and `M =` on a subcircuit instance all select the
   multiplier, and `tests/regression/case/subckt-mult-param-case.cir` plus its
   twin prove the numbers agree.
2. `PRINT VM(2)`, `VDB(2)` and `VP(2)` resolve under `preserve`, with a twin
   pair in `tests/regression/case/`.
3. **Half met.** The differential sweep's `DIFF` count drops by gap 1's two
   entries and `NUM-DIFF` stays 0; gap 2's eight remain. The count of twelve
   is corrected in the Resolution.
4. **Met.** `make check` unchanged with `casemode` unset.

## Resolution

Gap 1 fixed, with `doc/codex/issues/0015`, in one commit. Gap 2 not fixed.

### Gap 1 was not where this issue said it was

`found_mult_param()` had **already** been folded, by commit `871f85f32` (Phase
1 census pass A, row 150): at `e5913af03` it reads `cieq(param_names[i], "m")`,
not `strcmp`. Folding it changed no output, which is why the sweep still
reported `DIFF` and why this issue's §1 and Root Cause looked right.

The decisive byte-exact site is one pass further on, in `inp_fix_inst_line()`:

```c
/* src/frontend/inpcom.c, before the fix */
if (strcmp(subckt_param_names[i], inst_param_names[j]) == 0) {
```

The left operand is the literal lower-case `"m"` that
`inp_fix_subckt_multiplier()` itself appends to the `.subckt` card; the right
is `M` as the user typed it. The compare fails, the formal's default `1`
survives, and `inp_fix_inst_line()` then truncates the X card at its first
assignment and re-emits **positional** values only — so `X1 1 2 divider M=2`
becomes `X1 1 2 divider 1` and no later reader can recover the multiplier.
That compare is the last chance, and it is the only production site: every
other `m` reader in the tree (`inpdpar.c:30`'s `find_instance_parameter()`,
`eval_m()`, `eval_mvalue()`, the G-source and table rewrites, `inpgmod.c:151`)
is already case-insensitive, and `nupa_subcktcall()` binds actual arguments
positionally.

`M = 2` needs no extra code: `inp_remove_excess_ws()` runs before this pass and
has already made it `M=2`.

### The fix, and the policy decision it carries

```c
/* src/frontend/inpcom.c */
static bool user_ident_eq(const char *a, const char *b)
{
    return inp_case_folding() ? (strcmp(a, b) == 0) : cieq(a, b);
}
```

used at that compare, at `find_function()`, and at the duplicate-`.param`
detection in `inp_sort_params()`.

The decision this issue's Acceptance Criteria left implicit, taken explicitly:
**`m` is treated as a user parameter name here, not as a keyword.** The
argument is that the compare is generic — it matches *every* subcircuit formal
against *every* instance parameter, and `m` only happens to be one of them —
so the alternative would have been to special-case the multiplier token and
leave `PARAMS: RVAL=9k` unable to see an instance's `rval=1k`, which is the
same silent wrong number one name away. Under `preserve` a user parameter name
is now one name whatever the spelling, matching what `doc/codex/issues/0015`
does for the numparam symbol it eventually becomes. The gate keeps the default
mode byte-identical, so `found_mult_param()`'s unconditional `cieq` and this
gated fold do not actually disagree about any reachable input: ordinary `X` and
`.subckt` cards are lowercased wholesale by the reader, and none of the fold
exemptions (`.lib`/`.inc`, the `.control` command whitelist, CIDER `.model`,
`is_xspice_model()`) can produce one.

### Evidence

`tests/regression/case/subckt-mult-param-case.cir` and its twin, three
instances of one subcircuit spelled `M=2`, `m=2` and `M = 2`, printing three
node voltages so that a wrong number cannot hide behind an abort:

```
$ ngspice -D casemode=preserve --batch subckt-mult-param-case.cir
v(2) = 1.000000e+00      <- M=2,   wrong, no diagnostic
v(3) = 1.333333e+00      <- m=2,   right
v(4) = 1.000000e+00      <- M = 2, wrong, no diagnostic
```

All three read `1.333333e+00` after the fix, and the lower twin was byte-equal
throughout. The deck was also run against an empty `.out` first, where it
fails — so its evidence is a number.

### Criterion 3's accounting, corrected

This issue said the sweep's `DIFF` count would drop by twelve entries. Twelve
was the number of case-directory decks reporting `DIFF`; the two gaps account
for ten of them, and gap 1 for exactly two — `subckt-mult-skip-case.cir` and
its twin, whose *uppercased* copies printed `v(2) = 1.000000e+00`. The other
two of the twelve are `harness-alive.cir` and `write-roundtrip.cir`, which
report `DIFF` by design. Gap 1's two entries now report `OK`.

### Gap 2

Gap 2 is control-language surface. `doc/codex/issues/0011` already covers one
way the preprocessor damages command text, and the `distinguish` design
decision for the vector table is still open; a fold here should be taken
together with those rather than in isolation.

Both are recorded in `doc/claude/checklists/phase2-differential-sweep.md` as
the reason twelve of the new decks report `DIFF` rather than `OK`.
