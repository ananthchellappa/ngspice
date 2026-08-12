# Issue: A Subcircuit Formal Pin Is Matched Byte-Exactly Under `preserve`

## Status

Open. Found while measuring what remains on
`doc/claude/specs/case-sensitive-identifiers.md`'s acceptance list after
`doc/claude/decisions/0011-identity-lint.md`. It is the "subcircuit formal
pins" bullet, and it turns out to be a **shipped-mode** defect rather than a
`distinguish` gap — the same discovery shape as Phase 3 gates 2 and 4, which
were also filed under `distinguish` and turned out to move `preserve`.

## Summary

`gettrans()` (`src/frontend/subckt.c:1682`) resolves a node name inside a
`.subckt` body against the formal pin list with a byte-exact comparison:

```c
    for (i = 0; table[i].t_old; i++)
        if (eq_substr(name, name_end, table[i].t_old)) {   /* subckt.c:1705 */
```

Its sibling forty-five lines below, matching a `.subckt` **name**, uses the
case-aware predicate that `doc/claude/decisions/0001-distinguish.md` decision 3
introduced for exactly this:

```c
                if (eq_substr_id(xname, xname_e, subs->su_name))  /* :1734 */
```

`eq_substr_id()` (`:1665`) is `eq_substr()` under `inp_case_exact_ids()` and a
`tolower_c` loop otherwise. `gettrans()` never got it.

Under `fold` the reader has lowercased both the `.subckt` line and the body, so
byte-exact is right. Under `preserve` it is wrong: the formal pin keeps the
spelling of the `.subckt` card and the body keeps its own, and two spellings of
one pin are one pin in that mode.

## Impact

**Silent wrong topology in a mode that has shipped.** A miss is not an error —
`translate_node_name()` falls through to prefixing the name with the instance
scope, so the body's `in` becomes the subcircuit-local net `x1:in` instead of
the caller's node, and the subcircuit is simply not connected to the outside.
Nothing is reported.

Measured at `b48b765d8` on this deck:

```
.subckt div IN OUT
r1 in out 1k
r2 out 0 1k
.ends
v1 a 0 dc 3
x1 a b div
```

| deck | `fold` | `preserve` | `distinguish` |
| --- | --- | --- | --- |
| `.subckt` UPPER, body lower | 1.5 V | **no `v(b)` at all** | no `v(b)` |
| `.subckt` lower, body UPPER | 1.5 V | **no `v(b)` at all** | no `v(b)` |
| both lower (control) | 1.5 V | 1.5 V | 1.5 V |

`preserve` is the row that matters: it is a shipped mode giving a disconnected
subcircuit where `fold` gives a working divider, for a deck that is legal in
both. The only sign is `Warning from checkvalid: vector b is not available or
has zero length` on `cp_err`, which says nothing about pins or about case.

Under `distinguish` the *result* is defensible — two spellings are two names,
so the body's `in` really is a different net from the pin `IN` — but it is
silent, and `doc/claude/decisions/0001-distinguish.md` decision 2 says a
resolution that misses beside a case variant should report. This site resolves
and misses and says nothing, so the `distinguish` half is a diagnostic gap even
though the topology is right.

## Root Cause

`doc/claude/decisions/0001-distinguish.md` decision 3 enumerated
`subckt.c:1665` `eq_substr_id()` and the `.subckt`-name call site that uses it.
The formal-pin call site in `gettrans()` is a *different* function reached from
`translate_node_name()` and was not on the list, because the list was built by
re-reading sites that already asked `inp_case_folding()` and this one asked
nothing — it was a bare `eq_substr`, the same shape as the two bare `strcmp`s
that became gates 2 and 4.

`tests/regression/casedist/` has twenty-one decks and none exercises a
subcircuit formal pin, which is why the spec's acceptance list still marks it
untested; `tests/regression/case/` (`preserve`) does not either.

## Acceptance Criteria

1. `gettrans()`'s formal-pin loop compares with `eq_substr_id()`, so two
   spellings of one pin are one pin under `preserve` and two under
   `distinguish`. The global-node probe above it already folds its key through
   `glo_key()` and needs no change.
2. A deck in `tests/regression/case/` asserts the `preserve` fix: a `.subckt`
   whose formal pins are upper case and whose body spells them lower case
   produces the same node voltage as the all-lower-case deck. RED-first — it
   must fail at the parent commit with the value missing, not merely differing.
3. A deck in `tests/regression/casedist/` asserts the `distinguish` half: the
   two spellings stay two nets, and whatever decision 2 of
   `doc/claude/decisions/0001-distinguish.md` requires of the miss is either
   implemented or recorded as deliberately not.
4. The `.subckt`-name row is checked for the same asymmetry in the other
   direction, and `settrans()` (`:1621`), which builds `table[]`, is read for
   whether it can put two spellings of one pin into two rows.
5. `make check` green in all three modes; the differential sweep moves no deck
   that is not already a known flapper.

## Resolution

Not fixed.
