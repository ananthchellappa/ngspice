# Issue: Vector Name Matchers Outside `findvec()` Are Still Case-Insensitive

## Status

Closed. Found while closing `doc/codex/issues/0027`, by sweeping
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
`vec_name_eq()` (`src/frontend/vectors.c:402`) implements and what `findvec()`
already uses.

| Site | Compare | Under `distinguish` |
| --- | --- | --- |
| `src/frontend/postcoms.c:54` `is_scale_vec_of_current_plot()` | `cieq(v_name, pl_scale->v_name)` | too loose: a case variant of the scale's name cannot be `unlet` at all. **Loud** |
| `src/frontend/vectors.c:284` `findvec_ally()` | `!cieq(d->v_name, pl->pl_scale->v_name)` | too loose: `ally` silently omits a case variant of the scale |
| `src/frontend/vectors.c:1244` `vec_eq()` | `cieq(s1, s2)` over `vec_basename()` | too loose: a case variant of the scale is taken *for* the scale. Six callers — `postcoms.c:306` (`print`'s scale column), `:681` and `:706` (`write`), `:846` and `:871` (`write_sparam`), `src/frontend/plotting/agraf.c:62` |
| `src/frontend/rawfile.c:618` | `cieq((char *) v->v_scale, nv->v_name)` | too loose: a rawfile naming both `time` and `TIME` binds a variable's scale to whichever comes first |
| `src/frontend/diff.c:127`, `:136` | `cieq(n1, n2)` | too loose: `diff`'s argument list selects the case twin as well, so `diff p1 p2 OUT` also reports `Out` |

**Corrected while closing.** The `diff.c` row originally read "`diff` reports
two spellings as one vector, so a real difference between `Out` and `OUT`
across two plots is invisible". That describes the *pairing* of a vector in
one plot with its twin in the other, which `nameeq()` does not do: the pairing
is a hash lookup on canonical names (`diff.c:233-270`) whose comparator was
`nghash`'s `strcmp` (`src/misc/hash.c:260`), byte exact in all three modes.
`nameeq()` is the filter that throws out the vectors *not* named in `diff`'s
argument list, and that is what the row should have said. The pairing has a
defect of its own, in the opposite direction and in both shipped modes; it is
`doc/codex/issues/0037`, and it is now **fixed** — the key is folded and the
chain is filtered with `vec_name_eq()`, so the two sites of this function
answer the same question the same way. `doc/claude/decisions/0007-diff-cross-plot-pairing.md`.

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

`rawfile.c:618` and `diff.c` are read from the source and not exercised: both
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
2. `rawfile.c:618` is decided rather than tightened. Its `else` branch already
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

Fixed on `ver_50`, 2026-08-11, in three commits.
`doc/claude/decisions/0005-scale-vector-identity.md` is the record.

`vec_name_eq()` is exported in `src/include/ngspice/fteext.h` — the step
`doc/claude/decisions/0004-unlet-vector-identity.md` decision 2 left for this
issue — and all six sites call it. A `vec_is_plot_scale(struct dvec *)` helper
was considered, as acceptance criterion 1 suggests, and rejected: the three
scale rows do not share a signature. Decision 1 of `0005` has the argument.

- `18f751b4b` the three scale rows: `is_scale_vec_of_current_plot()`,
  `findvec_ally()`, and `vec_eq()`, which carries all six of its callers.
  `tests/regression/casedist/vector-scale-case.cir` asserts all three hats —
  `print`'s scale column and `ally`'s membership as data-row column counts,
  `unlet`'s refusal through a `>&` capture — and fails at `ca7d2bc07` on
  exactly those three and nothing else.
- `a08fc2504` `diff.c`'s `nameeq()`, both arms, with
  `tests/regression/casedist/vector-diff-case.cir`.
- `d0ebd6b64` `rawfile.c`'s `scale=` binding, with
  `tests/regression/casedist/vector-rawfile-scale-case.cir`, which writes the
  hand-built rawfile it loads because the raw *writer* never emits `scale=`.

Criterion 2 asked for `rawfile.c:618` to be decided rather than tightened, and
the decision was to tighten it: it is the same Class C rule as the other five,
it is a no-op in both shipped modes, and the loud answer its `else` branch
gives is already the right one. Decision 4 of `0005`. The migration note in
`doc/claude/decisions/0001-distinguish.md` decision 5 carries the sentence.

Both sites the issue called unexercised were reached. `rawfile.c` is reachable
from a deck that writes its own rawfile with `echo` and its redirect, and
`diff.c` from a deck with two plots of one circuit — an `op` and a `tran` —
whose lengths differ, so `diff`'s report is one line per selected vector on
`cp_out`.

`make check` is 261 PASS, 0 FAIL, up from 258 by exactly the three new decks.
`tests/regression/case/` is untouched, and the differential sweep moves no
deck.

This issue was the whole of `set_case_mode()`'s experimental-mode clause, so
closing it emptied the clause. It was not retargeted to another defect; see
`0005` decision 5.
