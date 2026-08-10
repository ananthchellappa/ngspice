# Issue: numparam Symbol Names Are Case Sensitive

## Status

Open. Found while fixing `doc/codex/issues/0010`; that issue's scope is the
`.model`, `.subckt` and `.global` name spaces, and this is a fourth one.

## Summary

numparam keeps `.param`, `.func` and `.subckt` symbols in a per-scope `nghash`
table created with `nghash_init()`, i.e. `NGHASH_FUNC_STR`, i.e. `strcmp`:

| Site | Role |
| --- | --- |
| `src/frontend/numparam/xpressn.c:268`, `:481` | table creation, `nghash_init()` |
| `src/frontend/numparam/xpressn.c:420`, `:432` | `attrib()`, the insert |
| `src/frontend/numparam/xpressn.c:387` | `entrynb()`, the probe |

`doc/codex/issues/0010` folded the key for the `NUPA_SUBCKT` entries only —
`defsubckt()` at `xpressn.c:551`, `findsubckt()` at `xpressn.c:578` and
`findsubname()` at `spicenum.c:194` — because a subcircuit name is one of the
three name spaces that issue covers. Every other numparam symbol is still
matched byte-exactly.

## Impact

Under `fold` this is unreachable: the reader has lowercased the card, so both
the definition and the reference are lower case.

Under `casemode=preserve` a `.param` whose name is spelled one way at the
definition and another at the reference is not found, and the run aborts:

```spice
* numparam parameter name case
.OPTIONS noacct
.param RVal=1k
v1 1 0 dc 4
r1 1 2 {rval}
r2 2 0 3k
.control
op
print v(2)
.endc
.end
```

```
$ ngspice --batch param.cir                        -> v(2) = 3.000000e+00
$ ngspice -D casemode=preserve --batch param.cir   -> Error in netlist line no. 6,
                                                        new internal line no. 5:
                                                      Undefined parameter [rval]
                                                      Cannot compute substitute
                                                      ERROR: fatal error in ngspice, exit(1)
```

This is a hard abort, not a wrong number, so it cannot produce a silently
different answer. It is nevertheless the largest remaining `preserve` gap by
deck count, because parameterised decks are the normal modern style and
`.param NAME` with `{name}` is a common spelling habit.

The plan already anticipates it, in the Phase 3 prerequisites: "`.param VDD`
with `{vdd}`: numparam is the highest-likelihood break and has no test"
(`doc/claude/suggestions/case-sensitive-identifiers-plan.md`). The Phase 1
census also flagged the neighbouring line categorizer,
`spicenum.c:241,245,254,256,258`, which was fixed in Phase 1 and in
`doc/codex/issues/0009`.

The differential sweep does not report this today: `.param` and `.func` names
in the tree's decks are lower case on both sides, and the sweep's mechanical
uppercasing uppercases both sides of the same card.

## Root Cause

The same one the whole feature has: identity was delegated to the reader's
fold, so no lookup layer folds anything. numparam's tables own their keys
through `NGHASH_FUNC_STR`'s `copy(user_key)` at `src/misc/hash.c:548`, so the
fix has the same shape as issue 0010's — fold the key at
`attrib()`/`entrynb()`, never install a custom `hash_func` — but it is a
larger blast radius, because `entrynb()` is the single probe for every kind of
numparam symbol and `attrib()` the single insert.

`.func` formals, `dicostack_push()`'s scoped instance symbols, and the
`nupa_subcktcall()` actual-argument binder all go through the same pair.

## Acceptance Criteria

1. Under `preserve`, a `.param`, `.func` or `.func` formal matches its
   references regardless of spelling, with the first-seen spelling retained for
   reporting.
2. No `nghash` table changes from owning its keys to borrowing them: the key is
   folded by the caller before `nghash_insert`/`nghash_find`.
3. `tests/regression/case/` gains a deck that spells a `.param` name, a
   `.func` name and a `.func` formal two ways each and produces the same
   voltages as its consistently-spelled twin.
4. The `NUPA_SUBCKT` special case added by issue 0010 at `xpressn.c:551`,
   `:578` and `spicenum.c:194` is removed once the general fold lands, so there
   is one policy rather than two.
5. `make check` unchanged with `casemode` unset.

## Resolution

Not fixed. Recorded so that the remaining distance between what `preserve`
promises — identity unchanged — and what it delivers is written down. Until
this lands, `preserve` requires a deck to spell each `.param` and `.func` name
consistently.
