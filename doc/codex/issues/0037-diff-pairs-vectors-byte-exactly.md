# Issue: `diff` Pairs Vectors Across Plots Byte-Exactly In Every Mode

## Status

Open. Found while closing `doc/codex/issues/0032`, by reading `com_diff()`
around the `nameeq()` call that issue names. Not a regression — the hash has
always been there. Unlike almost everything else in the `casemode` series this
is wrong in the two **shipped** modes and right under `distinguish`, which is
the shape `doc/codex/issues/0016`, gate 2 and gate 4 also turned out to have.

## Summary

`com_diff()` (`src/frontend/diff.c:198-215`) pairs each vector of the first plot with
its twin in the second through a hash table:

```
crossref_p = nghash_init(NGHASH_MIN_SIZE);      /* :198 */
nghash_unique(crossref_p, FALSE);
... nghash_insert(crossref_p, canonical_name(v2->v_name, ...), v2);
... nghash_find(crossref_p, canonical_name(v1->v_name, ...));
```

`nghash_init()` installs `NGHASH_DEF_CMP(NGHASH_FUNC_STR)`
(`src/misc/hash.c:130`), and every lookup path compares those keys with
`strcmp` (`src/misc/hash.c:260`, `:310`, `:374`, ...). So the pairing is byte
exact in all three modes, and `canonical_name()` folds nothing except the
inner name of an `i(...)` wrapper.

That is the correct rule under `distinguish` only. Under `fold` and `preserve`
two spellings of a name are one identifier, so two plots that spell one vector
two ways hold one vector between them and `diff` must pair them.

This is a different site from the one `doc/codex/issues/0032` fixed.
`nameeq()` decides which vectors survive `diff`'s *argument list*; this decides
which vectors are compared with which. `0032`'s table originally described the
pairing while naming `nameeq()`, and that row is corrected in place.

## Impact

Measured at `d0ebd6b64`, one deck with node `Out`, a hand-written rawfile
loaded as the second plot, and `diff op1 op2 all`. `op1.Out` is 9 V from the
file and `op2.Out` is 0.75 V from the run, so a correct pairing must report a
difference:

```
rawfile spells   mode           reported on stdout
Out              fold           nothing            <- wrong
Out              preserve       op1.Out[0] = 9.000000e+00  op2.Out[0] = 7.500000e-01
Out              distinguish    op1.Out[0] = 9.000000e+00  op2.Out[0] = 7.500000e-01
OUT              fold           nothing            <- wrong
OUT              preserve       nothing            <- wrong
OUT              distinguish    nothing            <- correct
```

The `fold` row with a matching spelling is the sharpest one: the deck's node
`Out` is lowercased by the reader to `out` while the rawfile keeps `Out`, so
under the **default** mode a rawfile written by any other tool, or by an
ngspice built before the fold, fails to pair with a live run and `diff`
reports no differences at all.

Failure is silent on stdout. Each unpaired vector does produce
`>>> real vector X in p1 not in p2, or of wrong type` on `cp_err`, so the
information is there, but `diff`'s answer — the list of differing values — is
empty, which reads as "the two plots agree".

Not reachable from any committed deck: no deck under `tests/regression/case*`
uses `diff`.

## Root Cause

`nghash_init()` is the string-keyed constructor and its comparator is fixed.
`doc/claude/suggestions/case-sensitive-identifiers-plan.md` section 2.3 —
"fold the key, never swap the comparator" — rules out installing a
case-insensitive hash function here: `src/misc/hash.c:548` copies the key on
insert only for `NGHASH_DEF_HASH(NGHASH_FUNC_STR)`, and the frees at `:110`,
`:182` and `:393` are gated identically, so a different hash function silently
flips the table from owning its keys to borrowing them.

The shape the frontend vector table already uses is the answer: fold the key,
let `nghash_unique(..., FALSE)` keep the duplicate chain, and filter the chain
with `vec_name_eq()`. `com_diff()` already calls `nghash_unique(crossref_p,
FALSE)` at `:199` and already walks the chain with `nghash_find_again()`, so the filter
has a place to go; what is missing is the fold on the key.

## Acceptance Criteria

1. The pairing matches `vec_name_eq()`'s rule: `cieq()` under `fold` and
   `preserve`, exact under `distinguish`.
2. The key is folded and the comparator is not swapped, per section 2.3.
3. A deck asserts the `fold` row above, which is the default mode and needs no
   `casemode` at all. `tests/regression/casedist/vector-rawfile-scale-case.cir`
   shows how a deck can write the rawfile it loads, so the second plot needs no
   support file.
4. `tests/regression/casedist/vector-diff-case.cir` still passes: under
   `distinguish` the pairing must stay exact.

## Resolution

Not fixed.
