# Issue: Deck Preprocessing Dispatches on Lower-Case Device Letters

## Status

Open

## Summary

Several deck-preprocessing passes in `src/frontend/inpcom.c` decide what a card
is by testing its first character against a lower-case literal. They are
correct only because `inp_read()` has already lowercased the card. The four
sites found so far:

```c
/* src/frontend/inpcom.c:5357, get_number_terminals() */
    switch (*c) {
        case 'r': case 'c': case 'l': case 'k': case 'f':
        case 'h': case 'b': case 'v': case 'i':
            return 2;
        case 'd':  ...
```

```c
/* src/frontend/inpcom.c:9704, inp_rem_unused_models() */
        switch (*curr_line) {
            case '*': case '.': case 'v': case 'i': case 'b': case 'x':
            case 'e': case 'h': case 'g': case 'f': case 'k': case 't':
                continue;
```

```c
/* src/frontend/inpcom.c:3313, get_subckts_for_subckt() */
/* src/frontend/inpcom.c:3399, comment_out_unused_subckt_models() */
            if (*line == 'x') {
                char *subckt_name = get_instance_subckt(line);
                nlist_adjoin(used_subckts, subckt_name);
            }
            else if (*line == 'a') {
```

```c
/* src/frontend/inpcom.c:961, inp_get_w_l_x() */
        if (*curr_line != 'x' || (!newcompat.hs && !newcompat.spe) || ...
```

The same family appears just downstream: `inp_rem_unused_models()` also tests
`*curr_line == 'a'` and `*curr_line != m->elemb` against a lower-case element
identifier from `inp_get_elem_ident()`.

This is precisely the enumeration gap the Phase 1 census wrote down and left
open: "Character-literal comparisons are not counted... a scan for
`== '<lowercase>'` returns thousands of hits, almost all of them parser
bookkeeping"
(`doc/claude/checklists/phase1-keyword-case-census.md`). These are the ones
that are not bookkeeping.

## Impact

With the reader folding, none of this is reachable, so `fold` mode is
unaffected and `make check` cannot see it.

With `casemode=preserve` (`doc/claude/suggestions/case-sensitive-identifiers-plan.md`
Phase 2) it is the dominant failure mode of the feature. An upper-case device
letter is the normal SPICE house style, so most real decks hit it. Two
mechanisms:

1. `get_number_terminals()` returns 0 for `R3`, `D1`, `Q1`, `P1` and the rest,
   so `inp_rem_unused_models()` never records the model reference on that card,
   and `rem_unused_xxx()` comments the `.model` card out. The instance then
   fails to parse.
2. `get_subckts_for_subckt()` and `comment_out_unused_subckt_models()` do not
   see `X1` as a subcircuit instance, so the `.subckt` body is commented out
   and `inp_subcktexpand()` later reports the instance as undefined.

Neither is a wrong-numbers failure: both abort the run with a diagnostic. But
the diagnostic names the model or subcircuit, not the device letter, so the
cause is not discoverable from the message.

Measured on this tree with the differential sweep
(`doc/claude/scripts/case_differential_sweep.py`): of 129 `.cir` decks under
`tests/`, 82 fail to parse under `preserve` or under `preserve` on a
mechanically uppercased copy, while the stock run parses. Every failure
inspected reduces to one of the two mechanisms above.

Minimal reproduction, entirely lower case except the device letter:

```spice
* semiconductor resistor, upper case device letter
V1 1 0 dc 1
R3 1 2 rmodel1 l=11u w=2u
R9 2 0 1k
.model rmodel1 r rsh=1000 narrow=1u
.control
op
print v(2)
.endc
.end
```

```
$ ngspice --batch min.cir                        -> v(2) = 0.000000e+00
$ ngspice -D casemode=preserve --batch min.cir   -> Error on line 3 or its substitute:
                                                     R3 1 2 rmodel1 l=11u w=2u
                                                     unknown parameter (rmodel1)
```

Lower-casing `R3` to `r3` makes the preserve run agree with the stock run, with
every other character of the deck untouched.

The subcircuit half reproduces on a deck already in the tree:
`tests/regression/case/case-flag-noop.cir`, uppercased, gives
`Error: unknown subckt: X1 MID OUT DIVIDER`.

## Root Cause

The reader's fold is load-bearing for card classification, not only for
identifier identity. `src/spicelib/parser/inppas2.c:92-94` gets this right — it
copies the leading letter into a local and upper-cases the copy for dispatch,
mutating nothing — and `src/frontend/subckt.c:1136` uses `tolower_c(*name)`.
The `inpcom.c` preprocessing passes predate or ignore that convention.

The specification asserts the opposite of what the code does:
"Device letters. `R1` and `r1` dispatch to the same device type in every mode.
Already non-destructive (`src/spicelib/parser/inppas2.c:92-94`)"
(`doc/claude/specs/case-sensitive-identifiers.md`). That claim is true of the
parser and false of the preprocessor.

## Acceptance Criteria

1. Every first-character device-letter test in `src/frontend/inpcom.c` folds
   the character it tests, in the `inppas2.c` style: a local copy, no mutation
   of the card. `get_number_terminals()`, `inp_rem_unused_models()`,
   `get_subckts_for_subckt()`, `comment_out_unused_subckt_models()` and
   `inp_get_w_l_x()` at minimum.
2. `inp_get_elem_ident()`'s result and the `m->elemb` comparison agree on case.
3. A deck in `tests/regression/case/` with upper-case device letters on an R
   with a semiconductor model, a D, a Q and an X resolves all four and produces
   the same voltages as its all-lower-case twin.
4. The differential sweep reports no `PARSE-FAIL` for any deck whose stock run
   parses.
5. `make check` unchanged with `casemode` unset.

## Resolution

Not fixed here. Found by the Phase 2 differential sweep. The fix is
mechanical but it is Phase 1 work — making language-level matching case
insensitive — reaching a class of comparison the Phase 1 census explicitly
excluded from its enumeration, and Phase 2 was scoped to the mode switch and
the fold sites rather than to extending that sweep. It is the single largest
blocker to `preserve` being usable on a real deck, and it should be closed
before `preserve` is offered to anyone.
