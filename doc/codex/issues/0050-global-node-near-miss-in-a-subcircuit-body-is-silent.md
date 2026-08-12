# Issue: A `.global` Referenced From A Subcircuit Body Misses Silently Under `distinguish`

## Status

Open. Found by `doc/codex/issues/0049` while fixing the formal-pin lookup
immediately below this one in the same function, and deliberately not folded
into it — `0049` is about `preserve` and this is `distinguish` only.
`doc/claude/decisions/0012-subcircuit-formal-pin-identity.md` names it in its
closing list.

## Summary

`gettrans()` (`src/frontend/subckt.c`) resolves a node name inside a `.subckt`
body in two steps: a probe of the `.global` table, then the formal pin list.
The global probe folds its key through `glo_key()`:

```c
static char *glo_key(const char *name)
{
    char *key = copy(name);

    if (!inp_case_exact_ids())
        strtolower(key);

    return key;
}
```

Under `preserve` the key is folded, so two spellings of one global reach one
entry. Under `fold` the reader lowercased every card. Under **`distinguish`**
`inp_case_exact_ids()` is true, so the key is not folded at all — which is the
correct rule, because two spellings are two globals there — and a body
reference spelled differently from the `.global` card misses.

A miss is not an error. It falls through to the formal pin list, misses there
too, and `translate_node_name()` scopes the name to the instance. The body's
`vdd` becomes the subcircuit-local net `x1.vdd`, unconnected to the global,
and **nothing is reported**.

`doc/codex/issues/0049` was the same silence one loop below, at the formal pin
list, and it now reports through `report_pin_case_miss()`. The global probe
above it does not, because it is a hash lookup with no chain to walk for a
sibling: `report_pin_case_miss()` re-scans `table[]` case-insensitively, and
there is no equivalent scan of `glonodes`.

## Impact

**A silently wrong number under `distinguish`.** Measured at the commit that
closes `0049`, on this deck:

```spice
.OPTIONS noacct
.global VDD
vd VDD 0 dc 5
v1 1 0 dc 0
x1 1 2 amp
r9 2 0 1meg
.subckt amp IN OUT
r1 IN OUT 1k
r2 OUT vdd 1k
.ends
```

| mode | `v(2)` | `stderr` |
| --- | --- | --- |
| `fold` | 2.498751e+00 | — |
| `preserve` | 2.498751e+00 | — |
| `distinguish` | **0.000000e+00** | **nothing** |

The run completes and prints a plausible number. As with `0049`'s
`distinguish` half the *result* is defensible — `VDD` and `vdd` are two names
in that mode, so the body really did ask for a net nothing drives — but the
deck author is told nothing, and the deck is one that works in both shipped
modes.

Severity is below `0049`: that one moved `preserve`, a mode with shipped
decks, and this is `distinguish` only, which `set_case_mode()` still calls
experimental.

## Root Cause

`doc/claude/decisions/0001-distinguish.md` decision 3 lists `glo_key()` as a
Class A site and it took `inp_case_exact_ids()` correctly. Nothing about the
comparison is wrong. What is missing is decision 2's other half: a resolution
that misses beside a case variant should report, and this resolution misses in
silence.

`doc/claude/decisions/0012` decision 2 implemented that rule for the formal pin
list and stopped there, because the pin list is a small array that a second
pass can scan and `glonodes` is an `NGHASHPTR` whose keys under `distinguish`
are unfolded — so "is some global a case variant of this name?" needs either a
walk of the whole table or a second, folded index maintained beside it. Both
are larger than the line `0049` was about.

## Acceptance Criteria

1. A body reference that misses the `.global` table under `distinguish`, where
   a global differing from it only in case exists, is reported on `cp_err` in
   decision 2's form and with the noun that names the side that failed —
   `no global node named '%s'; '%s' differs only in case (casemode=distinguish)`.
2. A body reference that misses the global table with **no** case variant is
   silent, because it is an ordinary internal node. This is the same
   load-bearing silence as `0049` criterion 3 and needs its own case in the
   deck.
3. The scan does not fold `glonodes`' keys and does not install a
   case-insensitive hash function: `doc/claude/decisions/0001` decision 3's
   closing section, "fold the key, never swap the comparator", plus
   `src/misc/hash.c:549`, which changes a table from owning its keys to
   borrowing them when the hash function changes.
4. A deck in `tests/regression/casedist/` asserting the report and the
   silence, RED first, in the shape of `subckt-formal-pin-case.cir`.
5. `make check` green; the differential sweep moves no deck that is not
   already a known flapper. The sweep never runs `distinguish`, so it can only
   confirm that the two shipped modes are untouched — which the mode guard
   makes true by construction and which should be stated rather than measured
   away.

## Resolution

Not fixed.
