# Issue: Two `preserve` Gaps the New Case Decks Exposed in the Differential Sweep

## Status

Both gaps fixed. Gap 2 closed on 2026-08-11 by
`doc/claude/decisions/0013-user-defined-function-identity.md`, and **wider
than this issue states it**: the defect is not the `vm()` prefix, it is that a
user-defined function name is matched byte-exactly, so every function the
control language holds — the user's own `define`s as well as the shipped
`vm`/`vp`/`vdb`/`vr`/`vi` — answered only to the spelling it was defined with.
Section 2 and the gap 2 Root Cause paragraph below are corrected in the
Resolution rather than rewritten in place, so that a reader arriving at
criterion 2 through the `vm()` wording finds the wider rule.

Both were found by running
`doc/claude/scripts/case_differential_sweep.py` over the decks added while
closing `doc/codex/issues/0009`'s follow-up table, and neither is that
issue's class: one is a parameter-name lookup and one is a control-language
function prefix. *Neither was fixed when this was written*, because each
belonged to a name space with its own open issue and its own design decision;
gap 1 closed on 2026-08-10 and gap 2 on 2026-08-11, each with the decision
record its name space needed.

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

**This heading names the symptom, not the defect.** `vm` is not a prefix and
is not a built-in; it is a user-defined function ngspice defines for itself,
and so are `vp`, `vdb`, `vr` and `vi`. The defect is that *any* user-defined
function name is matched byte-exactly. See the Resolution.

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

**That sentence is wrong about the name space this defect is really in.** The
same byte-exact compare reaches `trcopy()`'s formal-parameter match, where
`define g(X) x*2` leaves the body's `x` unsubstituted under `preserve`, so
`g(5)` is unavailable where `fold` gives 10; and `com_undefine()`, where
`undefine F` is a silent no-op. A missing `print` and a function that will not
undefine are both missing numbers rather than wrong ones, so the paragraph's
conclusion survives its premise. See the Resolution.

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

This paragraph is wrong — there is no such site, and there is no `vm` in
`ft_funcs[]` either. See the Resolution. It is the second Root Cause paragraph
of this issue to be wrong about where the defect is; gap 1's named the wrong
function and this one names a mechanism, the lower-case literal, that does not
exist anywhere on the path.

## Acceptance Criteria

1. **Met.** `M=`, `m=` and `M =` on a subcircuit instance all select the
   multiplier, and `tests/regression/case/subckt-mult-param-case.cir` plus its
   twin prove the numbers agree.
2. **Met, and wider than written.** `PRINT VM(2)`, `VDB(2)` and `VP(2)`
   resolve under `preserve`, with the twin pair
   `tests/regression/case/udf-name-case.cir` and `udf-name-case-lower.cir`.
   So does a user's own `define` called back in the other case, which is the
   same defect and is what this criterion should have asked for.
3. **Met.** The differential sweep's `DIFF` count dropped by gap 1's two
   entries and then by gap 2's, and `NUM-DIFF` stayed 0 throughout. The
   count of twelve is corrected in gap 1's Resolution; gap 2's count of
   eight is corrected in gap 2's.
4. **Met.** `make check` unchanged with `casemode` unset.

## Resolution

Gap 1 fixed, with `doc/codex/issues/0015`, in one commit. Gap 2 fixed on
2026-08-11 in a second, with
`doc/claude/decisions/0013-user-defined-function-identity.md`.

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

### Gap 2 was not where this issue said it was either, and is wider than it says

Fixed on 2026-08-11 by
`doc/claude/decisions/0013-user-defined-function-identity.md`.

**There is no lower-case literal, and no `vm` in `ft_funcs[]`.**
`PP_mkfnode()` (`src/frontend/parse.c:506`) copies the typed name, lower cases
the copy and matches `ft_funcs[]` with `eqc()` — case-insensitive twice over,
in every mode. `vm`, `vi`, `vr`, `vp`, `vdb`, `vg`, `gd`, `max` and `min` are
not in that table at all: `ft_cpinit()` (`src/frontend/cpitf.c:56`) holds them
as source text in a `udfs[]` array and installs them by calling
`com_define()`, in lower case, at start-up. They are **user-defined
functions**, and the byte-exact compare is the one that resolves any of them:

```c
/* src/frontend/define.c, ft_substdef(), before the fix */
        if (eq(name, udf->ud_name)) {
```

So the subject is not the `vm()` family and not a prefix. It is that **every**
user-defined function answers only to the spelling it was defined with:

```
.control
define f(x) x*3
print F(2)      Error: no such function as F, or F(2) is not available.
print f(2)      f(2) = 6.000000e+00
.endc
```

The `vm` family is that rule applied to the nine functions ngspice defines for
itself, which is why they are the half a deck notices.

Three more sites in the same file went with it, none of them named here:
`prdefs()`, so `define VM` printed nothing; `com_undefine()`, so `undefine F`
was a silent no-op with `f` still callable — `doc/codex/issues/0027`'s shape
one name space over; and `com_define()`'s own replace-or-prepend lookup, so
`define VM(x)` beside the shipped `vm` left two entries under one identifier
in the mode whose rule is that two spellings are one identifier. A fifth
comparison in the file, `trcopy()`'s formal-parameter match, is Class A rather
than Class C and was a `preserve` defect of its own: `define g(X) x*2` never
substituted the body's `x`.

**The predicate is Class C, not Class A**, and the reason is measured rather
than argued: at `4d36cf61d`, under the **default** `fold` mode, `print F(2)`
and `print VM(1)` typed at the `ngspice -p` prompt both failed. The reader
folds a control-language word only when it arrived through `inp_readall()`, so
an identity predicate here would have left the shipped default mode unable to
call its own shipped functions from the prompt, from `ngSpice_Command()` and
from the shared library. `doc/claude/decisions/0004-unlet-vector-identity.md`
decision 1 refused the same thing at `vec_remove()` on the same evidence.

### Criterion 3's accounting for gap 2, corrected

This criterion says gap 2 accounts for eight of the sweep's `DIFF` entries and
then names five decks *"with their twins"*, which is ten. The list is right
and the count is not: `tests/regression/case-lt/` does carry
`rkm-c-case-lower.cir` and `rkm-l-case-lower.cir`, so all five have twins.
**Ten** of the named decks cleared, measured over a frozen tree with the two
binaries, and none newly differed.

The eight and gap 1's *"the two gaps account for ten of them"* are not two
statements about one census and should not be reconciled: gap 1's paragraph
counts only `tests/regression/case/`, where six of gap 2's ten live, and the
other four are in `tests/regression/case-lt/`. What is measured rather than
inferred is the list of ten and the totals beside it — over a frozen tree of
321 decks, `DIFF` went **74 → 61** with **no deck newly differing**, and
`NUM-DIFF` and `PARSE-FAIL` were 0 in both runs. Three of the thirteen that
cleared are not gap 2's: this work's own `casedist` deck, one `alter-rebin`
flap and one `stdout reordered only` on `tests/bsim3soidd/ring51.cir`.

Two decks *stay* `DIFF`, by design and knowingly:
`tests/regression/case/udf-name-case.cir` and its twin report
`+'vm (x) = mag (v (x))'`, which is the residue
`doc/claude/decisions/0013-user-defined-function-identity.md` decision 2
leaves — under `preserve` a `define VM(x)` shadows the shipped `vm` instead of
replacing it, because the line that would replace it is a prefix test and
`doc/codex/issues/0051` has to fix that first. Those two entries should clear
when `0051` lands.

Gap 2 was recorded in `doc/claude/checklists/phase2-differential-sweep.md` as
one of the reasons twelve of the new decks reported `DIFF` rather than `OK`.
