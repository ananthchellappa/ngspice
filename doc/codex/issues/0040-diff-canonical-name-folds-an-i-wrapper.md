# Issue: `diff`'s Canonical Name Folds The Name Inside An `i()` Wrapper

## Status

Fixed on `ver_50` with `doc/codex/issues/0037`, in the commit after it. Found
while fixing `0037`, in the loop that issue changes, by asking what the
pairing predicate actually compares once the hash key is folded.

## Summary

`canonical_name()` (`src/frontend/diff.c:30`) rewrites `i(some_name)` as
`some_name#branch` so that the two spellings of one current pair with each
other, and its `make_i_name_lower` argument additionally lower cases the name
inside the wrapper. `com_diff()` passed `TRUE`; `nameeq()` has always passed
`FALSE`.

The fold predates the case modes and is invisible in the two shipped modes,
where the pairing is case insensitive anyway. Under `distinguish` it is wrong:
`i(Vsrc)` and `i(VSRC)` are two names, and the canonical form made them one.

Measured at `28d36a7c4` and again with `0037` fixed, two hand-written rawfiles
loaded as `op1` and `op2`, `-D casemode=distinguish`:

```
op1.i(Vsrc)[0] = 7.000000e+00    op2.i(VSRC)[0] = 6.000000e+00
```

Two vectors that the mode says are two different currents are reported as one
current whose value disagrees between the plots.

## Impact

Confined to `distinguish`, which is experimental, and to plots that spell a
current with the `i()` wrapper. ngspice stores its own branch currents as
`name#branch`, so the wrapper spelling reaches a plot only from a rawfile
another tool wrote, which is the same input class as `0037`'s.

The direction is the opposite of `0037`'s: too loose rather than too strict.
Nothing is silent — a line is printed — but it names a vector pair that the
mode says does not exist, and the two currents' values are compared as though
they were one signal.

## Root Cause

The fold was doing the job of a hash key before there was one to do it in.
With `0037` fixed the key is folded for every name, so every spelling of a
name is already on one chain, and the only thing `make_i_name_lower` still
decided was the *identity* test — which belongs to `vec_name_eq()` and to the
mode.

## Acceptance Criteria

1. `com_diff()`'s canonical names keep the spelling inside an `i()` wrapper,
   so the pairing predicate sees it. The `#branch` rewrite itself is unchanged:
   `i(Vsrc)` and `Vsrc#branch` are one vector in every mode.
2. A deck asserts it under `distinguish`, and its silence is proved
   non-vacuous by a pairing that is reported in the same `diff`. A second deck
   asserts criterion 1's other direction, that the pairing the rewrite exists
   for still happens.
3. No movement in `fold` or `preserve`: a folded key plus `cieq()` accepts the
   same set with or without the inner fold.

## Resolution

Fixed. `com_diff()`'s three `canonical_name()` calls pass `FALSE`, which is
what `nameeq()` already passed. Two decks, both RED against the binary with
`0037` fixed and this not, both under `distinguish`:

- `tests/regression/casedist/vector-diff-wrapper-case.cir` — the loose half.
  One extra reported line before the fix, the `i(Vsrc)`/`i(VSRC)` pair.
- `tests/regression/casedist/vector-diff-branch-case.cir` — the strict half,
  which the Summary above did not name and the fix also moves: `i(V1)` in a
  rawfile and the `V1#branch` the simulator stores are one current in every
  mode, and the fold made the canonical name `v1#branch` meet a stored name
  that keeps its capital, so they did not pair. One line missing before the
  fix.

A change that both tightens and loosens needs an assertion in each direction,
which is why there are two.

`make_i_name_lower` now has no caller that passes `TRUE`. The parameter is
kept rather than deleted: `canonical_name()` is otherwise untouched by this
work, and the argument at each call site is where a reader looks to see that
the fold is deliberate rather than absent.
