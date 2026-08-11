# Issue: numparam's String-Argument Built-ins Reach the Control-Language Name Spaces Case Sensitively

## Status

Open. Found while closing `doc/codex/issues/0022`, in the branch of `formula()`
that runs immediately after the `keyword()` match that issue folded. Not fixed
here: two of the three sites resolve a name in a control-language name space,
which is the `distinguish` decision that `doc/codex/issues/0011` and `0020`
gap 2 are already waiting on.

## Summary

`fmathS` ends with `vec` and `var`, the two built-ins that take a *string*
argument (`src/frontend/numparam/xpressn.c:88-96`). After `keyword()` matches
one of them, `formula()` hands the raw argument text to the control-language
name spaces, and every compare on that path is byte exact.

| # | Site | Compare |
| --- | --- | --- |
| 1 | `xpressn.c:1040` — `var()` | `cp_getvar(vec_name, CP_REAL, &u, sizeof u)` |
| 2 | `xpressn.c:1019` — `vec()` with a `plot.vector` qualifier | `vec_get()` → `plot_prefix()` |
| 3 | `spicenum.c:367` — the `interactive` probe | `cp_getvar("interactive", CP_BOOL, NULL, 0)` |

`cp_getvar()` (`src/frontend/variable.c`) scans four lists — `variables`,
`cp_usrvars()`, `plot_cur->pl_env`, `ft_curckt->ci_vars` — and each scan is
`eq(name, v->va_name)`, i.e. `strcmp`.

`vec_get()`'s qualifier half splits the argument at the first `.` and walks the
plot list with `plot_prefix(buf, pl->pl_typename)` (`src/frontend/vectors.c:607`,
definition at `:1392`), whose loop is `if (*pre != *str) break;`. The *vector*
half of the same lookup is already case-insensitive: the per-plot lookup table
is built from `ds_cat_str_case(&dbuf, d->v_name, ds_case_lower)`, so only the
plot qualifier is exposed.

Site 3 is the odd one out: it is a *keyword* — the literal is written in the
source, not in the deck — so it belongs to the Phase 1 class rather than to the
`distinguish` decision. It only misses when the deck spells the variable
`set Interactive` under a mode that does not fold, and its effect is limited to
whether numparam prompts instead of aborting.

## Impact

Sites 1 and 2 are the worst shape a case defect takes: **a silent wrong
number**. Neither reports a miss.

```c
/* xpressn.c:1039 */ } else if (fu == XFU_VAR) {
                         if (!cp_getvar(vec_name, CP_REAL, &u, sizeof u))
                             u = 0;
/* xpressn.c:1035 */ if (d && d->v_length > 0 && isreal(d))
                         u = d->v_realdata[0];
                     else
                         u = 0;
```

So under `-D casemode=preserve`, `.param rv={var(MYVAR)}` against a
`set myvar = 1k` silently yields 0, and `{vec(CONST.myv)}` silently yields 0.
Both are unreachable under `fold`, where the deck arrives lower case and the
stored names were folded on the way in.

The same silent-zero shape, reached from the other direction, is
`doc/codex/issues/0025`.

## Root Cause

`doc/codex/issues/0022` folded the *function name*. The function's *argument*
is a different name space: `var()` reaches the `set` variables and `vec()`
reaches the plot/vector name space, and both of those are still byte exact
because they are the control-language surface that no phase has taken yet.

## Acceptance Criteria

1. `{var(MYVAR)}` under `preserve` resolves `set myvar` (or the decision is
   recorded that it deliberately does not, together with `0011`, `0014`, `0016`
   and `0020` gap 2).
2. `{vec(CONST.myv)}` under `preserve` finds the plot.
3. Whichever way 1 and 2 go, a miss stops being silent, or the silence is
   recorded as intended in `doc/codex/issues/0025`.
4. `spicenum.c:367` folds as a keyword, ungated, like the rest of Phase 1.
5. Twin pairs in `tests/regression/case/`, and `make check` unchanged with
   `casemode` unset.

## Resolution

Not fixed. Sites 1 and 2 cannot be settled before the `distinguish` decision
that `doc/claude/suggestions/case-sensitive-identifiers-plan.md` defers: they
are the netlist side reaching into the control-language name spaces, so folding
them unilaterally would contradict whatever that decision says about
`src/frontend/vectors.c`. Site 3 is a one-line Phase 1 fold and is only left
here so the group stays one write-up.
