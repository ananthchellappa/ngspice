# Issue: Vector Name Matchers Outside `findvec()` Are Still Case-Insensitive

## Status

Open. Found while closing `doc/codex/issues/0027`, by sweeping
`src/frontend/` for every place that matches a vector *name* rather than
going through `findvec()`. `0027` fixed one of them, `vec_remove()`; these are
the rest. Not a regression — every `cieq()` below has always been there and is
correct in the two shipped modes.

Named by `set_case_mode()`'s experimental-mode warning as of
`496110f27`: "a vector whose name differs only in case from the current plot's
scale vector is still confused with it."

## Summary

Six sites, all Class C by `doc/claude/decisions/0001-distinguish.md` decision
3 — a stored name compared against a query, so the rule is `cieq()` under
`fold` and `preserve` and exact only under `distinguish`, which is what
`vec_name_eq()` (`src/frontend/vectors.c:393`) implements and what `findvec()`
already uses.

| Site | Compare | Under `distinguish` |
| --- | --- | --- |
| `src/frontend/postcoms.c:49` `is_scale_vec_of_current_plot()` | `cieq(v_name, pl_scale->v_name)` | too loose: a case variant of the scale's name cannot be `unlet` at all. **Loud** |
| `src/frontend/vectors.c:285` `findvec_ally()` | `!cieq(d->v_name, pl->pl_scale->v_name)` | too loose: `ally` silently omits a case variant of the scale |
| `src/frontend/vectors.c:1235` `vec_eq()` | `cieq(s1, s2)` over `vec_basename()` | too loose: a case variant of the scale is taken *for* the scale. Six callers — `postcoms.c:301` (`print`'s scale column), `:676` and `:701` (`write`), `:841` and `:866` (`write_sparam`), `src/frontend/plotting/agraf.c:62` |
| `src/frontend/rawfile.c:611` | `cieq((char *) v->v_scale, nv->v_name)` | too loose: a rawfile naming both `time` and `TIME` binds a variable's scale to whichever comes first |
| `src/frontend/diff.c:89`, `:98` | `cieq(n1, n2)` | too loose: `diff` reports two spellings as one vector, so a real difference between `Out` and `OUT` across two plots is invisible |

The scale-vector rows are one bug wearing three hats, which is why the warning
names it as one: whether the user asks `print`, `ally` or `unlet`, a vector
that merely resembles the scale is treated as the scale.

## Impact

None under `fold` or `preserve`: two spellings of a name are one vector in
both, so a case variant of the scale cannot exist.

Under `casemode=distinguish`, measured on one tran deck with scale `time` and
`let TIME = time * 2` beside it, at `496110f27`:

```
print out     ->  Index   time            out
print TIME    ->  Index   TIME                         <- no scale column at all
print ally    ->  Index   time  V1#branch  in  out     <- TIME absent
unlet TIME    ->  (stderr) Warning: Scale vector 'time' of the current plot
                           cannot be deleted!
                           Command 'unlet TIME' is ignored.
```

So `TIME` is a vector `distinguish` itself created, it prints without the
scale that gives its numbers meaning, it cannot be listed with `ally`, and it
cannot be deleted for the life of the plot. Two of the three are silent; the
refusal is the loudest failure in the sweep and the only one the user can see,
and it names a vector they did not ask about.

`rawfile.c:611` and `diff.c` are read from the source and not exercised: both
need a hand-built rawfile or a two-plot session to reach.

Not reachable from any committed deck. No deck under `tests/regression/case*`
or `tests/xspice/case*` uses `unlet`, `ally`, `write` or `diff` on a
case-variant name.

## Root Cause

The same class `0027` closed, one function away. `findvec()` acquired the
mode-aware predicate when Phase 3 gate 3 closed; every other name matcher in
the frontend kept the `cieq()` it was written with, because the gate was
scoped to the lookup table and these do not use it. `vec_eq()` is the widest
of them: it is the shared "are these two vectors really the same" helper, so
one `cieq()` there decides six behaviours.

## Acceptance Criteria

1. Each site above compares with the Class C rule — `cieq()` under `fold` and
   `preserve`, exact under `distinguish`. `vec_name_eq()` is that rule and is
   currently `static` in `vectors.c`; `is_scale_vec_of_current_plot()` and
   `diff.c` are in other translation units, so the predicate has to be
   exported, or each site given a helper beside it
   (`vec_is_plot_scale(struct dvec *)` would cover three of the six).
2. `rawfile.c:611` is decided rather than tightened. Its `else` branch already
   prints `Error: no such vector %s`, so exactness converts a silent mis-bind
   into a loud refusal for any rawfile whose scale reference differs in case
   from the variable it names. That is a visible change to `load` and needs a
   sentence in the migration note.
3. A deck in `tests/regression/casedist/` asserts the scale-vector case:
   a vector whose name differs from the scale's only in case prints *with* its
   scale column, appears in `ally`, and can be removed by `unlet`. All three
   are assertable. The first two are stdout, with the caveat that the filter
   eats any line containing `Index` or `time`, so the evidence is the column
   count of the *data* rows and the tran wants few enough points to keep the
   reference short. The refusal is on `cp_err` and is captured with `>&` and
   read back, the way `tests/regression/casedist/vector-unlet-report.cir`
   does — see `doc/claude/decisions/0001-distinguish.md` decision 2, whose
   claim that no deck can assert a diagnostic was corrected at `c42cf6748`.
4. `make check` unchanged with `casemode` unset, and `tests/regression/case/`
   unchanged.

## Resolution

Not fixed. `doc/claude/decisions/0004-unlet-vector-identity.md` decision 2
records why `vec_name_eq()` was left `static` when `0027` closed: exporting it
for a caller that does not exist yet would leave the tree with a public
predicate and one user. Exporting it is the first step of this issue's fix.
