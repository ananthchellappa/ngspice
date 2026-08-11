# Issue: `vec_remove()` Matches a Vector Name Case-Insensitively in Every Mode

## Status

**Closed** at `756112c46`, branch `ver_50`. Found while closing Phase 3 gate 3,
the `src/frontend/vectors.c` lookup table. Not a regression: the `cieq` has
always been there and is correct in the two shipped modes. The record is
`doc/claude/decisions/0004-unlet-vector-identity.md`.

## Summary

`vec_remove()` (`src/frontend/vectors.c:509`), which is what `unlet` runs,
picks the vector to drop with an unconditional case-insensitive compare over
the plot's vector list:

```c
/* src/frontend/vectors.c:513-506 */
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

Fixed in two commits, `3db0cc4d8` and `756112c46`. Before them it was recorded
in `doc/claude/decisions/0001-distinguish.md` decision 6 item 4 as one of the
things that record deliberately does not decide; it is not one of the four
Phase 3 gates, because those produce wrong *numbers* from a correct deck while
this one destroys state the user explicitly asked to keep.

Against the criteria:

1. **Done.** `vec_remove()` selects with `vec_name_eq(ov->v_name, name)`,
   `findvec()`'s own Class C rule: `cieq()` under `fold` and `preserve`, and
   under `distinguish` `strcmp` plus `vec_wrapped_name_eq()`'s tolerance for
   the `v` of `v(1)`. The predicate stays `static` — all three callers reach
   the list through `vec_remove()`. Two of those callers had the same defect
   and are fixed with it: measured under `distinguish` at `9b6f1d318`,
   `compose Vaa values 1 2 3` beside a `VAA` left only `Vaa`, and
   `cross Wbb 0 in` beside a `WBB` left only `Wbb`, both silently.
   `doc/claude/decisions/0004-unlet-vector-identity.md` decision 4 argues why
   tightening those two is a fix and not a behaviour change.
2. **Audited, and deliberately not changed.** The keyword side is already
   exact — `clookup()` walks the trie byte for byte in every mode — so the pair
   is a mismatch and not a match: vectors are registered under `v_name` and
   `vec_remove()` is handed the spelling the user typed. The mismatch is
   mode-independent, pre-existing, and unobservable, because `cp_ccom()` has had
   no callers since upstream `4feb0c3cc` and the `CT_VECTOR` trie is write-only
   in this build. Filed as `doc/codex/issues/0033` with the audit; decision 5 of
   `0004` records the three reasons for not taking the one-word change here.
3. **Done**, two decks in `tests/regression/casedist/`:
   `vector-unlet-case.cir` unlets `Out` beside an `OUT` and asserts the whole
   plot with `print all`, and `vector-compose-case.cir` does the same for
   `compose OUT` beside an `Out`. Both were first run against an empty
   reference and failed, so the evidence is a number rather than an absence.
   The RED at `9b6f1d318` was a wrong label *and* a wrong number in the same
   slot: expected `OUT = 1.125000e+00`, got `Out = 7.500000e-01`.
4. **Done.** `make check` 257 PASS 0 FAIL with `casemode` unset — 255 before,
   plus these two decks. `tests/regression/case/` and
   `tests/regression/casedist/`'s other seven decks unchanged. The
   `doc/claude/scripts/case_differential_sweep.py` verdicts are unchanged per
   deck; the sweep never runs `distinguish`, so it is a collateral-damage
   guard rather than evidence about this fix.

One thing the fix added that the criteria did not ask for: exactness turns the
wrong removal into a silent no-op, so `unlet` now reports a miss that a case
variant would have matched, per decision 2 of `0001-distinguish.md`. `compose`
and `cross` stay silent because they *define* the name they pass. Verified by
hand on stderr and quoted in `756112c46`.

