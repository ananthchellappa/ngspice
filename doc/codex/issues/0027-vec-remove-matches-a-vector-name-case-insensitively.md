# Issue: `vec_remove()` Matches a Vector Name Case-Insensitively in Every Mode

## Status

Open. Found while closing Phase 3 gate 3, the `src/frontend/vectors.c` lookup
table. Not a regression: the `cieq` has always been there and is correct in the
two shipped modes.

## Summary

`vec_remove()` (`src/frontend/vectors.c:500`), which is what `unlet` runs,
picks the vector to drop with an unconditional case-insensitive compare over
the plot's vector list:

```c
/* src/frontend/vectors.c:504-506 */
    for (ov = plot_cur->pl_dvecs; ov; ov = ov->v_next)
        if (cieq(name, ov->v_name) && (ov->v_flags & VF_PERMANENT))
            break;
```

It does not go through `findvec()`, so the duplicate-chain filter that closes
the `distinguish` aliasing at `find_permanent_vector_by_name()` does not apply
to it. `cp_remkword(CT_VECTOR, name)` on the next line has the same shape.

## Impact

None under `fold` or `preserve`: two spellings of a vector name are one vector
in both, so the first `cieq` hit is the only hit.

Under `casemode=distinguish`, with two case-variant nets `Out` and `OUT` in the
plot, `unlet OUT` removes whichever of the two appears first in `pl_dvecs`,
which is list order and not the spelling asked for. The user asked to drop one
vector and dropped the other, with no diagnostic. A subsequent `print OUT` then
reports the surviving vector's numbers under the requested name.

Not reachable from any deck in `tests/`, because no committed deck runs under
`distinguish` except the two in `tests/regression/casedist/`, and neither uses
`unlet`.

## Root Cause

The same class as the `findvec()` aliasing this gate closed, one function away
and on a different code path. `vec_remove()` predates the lookup table and
walks `pl_dvecs` directly, so it never acquired the table's folded key and
never acquired the filter that was added on top of it.

## Acceptance Criteria

1. `vec_remove()` selects the vector by the same identity rule the rest of the
   frontend uses: case-insensitive under `fold` and `preserve`, exact under
   `distinguish`.
2. `cp_remkword(CT_VECTOR, name)` on the following line is audited with it —
   the keyword table is a second name space and `cp_remkword` has its own
   comparison.
3. A deck in `tests/regression/casedist/` creates two case-variant vectors,
   `unlet`s one by its exact spelling, and asserts on the value of the other.
4. `make check` unchanged with `casemode` unset, and
   `tests/regression/case/` unchanged.

## Resolution

Not fixed. Recorded in `doc/claude/decisions/0001-distinguish.md` decision 6
item 4 as one of the things that record deliberately does not decide. It is not
one of the four Phase 3 gates: those produce wrong *numbers* from a correct
deck, this one destroys state the user explicitly asked to keep, which is a
different failure and a smaller one.
