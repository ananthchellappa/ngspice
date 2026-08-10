# Issue: numparam Symbol Names Are Case Sensitive

## Status

Fixed. All five acceptance criteria are met. Found while fixing
`doc/codex/issues/0010`; that issue's scope is the `.model`, `.subckt` and
`.global` name spaces, and this is a fourth one.

Two things this issue asserted turned out to be wrong, and the Resolution says
so in detail: `.func` names and `.func` formal parameters are **not** numparam
symbols, and the site table's line numbers had drifted by one.

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

1. **Met.** Under `preserve`, a `.param`, `.func` or `.func` formal matches its
   references regardless of spelling, with the first-seen spelling retained for
   reporting — the `.func` half in `inpcom.c`, not numparam; see Resolution.
2. **Met.** No `nghash` table changes from owning its keys to borrowing them:
   the key is folded before `nghash_insert`/`nghash_find` and no `hash_func` is
   installed.
3. **Met**, as four twin pairs rather than one deck — a combined deck would
   abort on the first mechanism and hide the rest: `param-name-case`,
   `subckt-formal-name-case`, `func-name-case`, `func-formal-case`.
4. **Met.** The `NUPA_SUBCKT` special case added by issue 0010 (`xpressn.c:552`,
   `:579`, `spicenum.c:195` at `e5913af03`) is removed, so there is one policy
   rather than two.
5. **Met.** `make check` unchanged with `casemode` unset: 204 tests, 0 FAIL,
   no committed `.out` touched.

## Resolution

Fixed. The fold went into the single probe and the single insert, and the
`.func` half went where `.func` actually lives, which is not numparam.

### Two corrections to this issue's own premise

**1. `.func` names and `.func` formals never reach `entrynb()`/`attrib()`.**
`.func` is macro-expanded by `inp_expand_macros_in_deck()`
(`src/frontend/inpcom.c`) before numparam sees the deck. Its name space is the
`struct function` list, and the two byte-exact tests were
`find_function()`'s `strcmp(f->name, name)` and, for the formals,
`search_func_arg()`'s `strncmp` plus the `accept` first-character filter that
`inp_get_func_from_line()` builds. `grep func src/frontend/numparam/*.c`
returns only comments and `mathfunction`. Criteria 1 and 3 were satisfied at
those `inpcom.c` sites instead; the deck evidence is the same shape either way.

**2. Five name kinds go through the pair, not four.** `.param` names,
subcircuit formal parameters, subcircuit names, instance-qualified
`inst.param` names composed in `dicostack_pop()`, and `.meas` result names
(`nupa_add_param()` from `src/frontend/measure.c`). All five now fold together.

Every line number in the Summary's site table is one low: `nghash_init()` is at
`xpressn.c:269`/`:482`, `attrib()`'s probe and insert at `:421`/`:433`,
`entrynb()`'s probe at `:388`, the issue-0010 folds at `:552`/`:579` and
`spicenum.c:195`.

### The fix

```c
/* src/frontend/numparam/xpressn.c, next to entrynb() */
static char *symbol_key(DSTRINGPTR key_p, char *s)
{
    if (inp_case_folding())
        return s;
    ds_clear(key_p);
    if (ds_cat_str(key_p, s) != DS_E_OK) { ... }
    strtolower(ds_get_buf(key_p));
    return ds_get_buf(key_p);
}
```

`strtolower()` rather than `ds_cat_str_case(key_p, s, ds_case_lower)`: the
dstring primitive folds with `tolower()` on a plain `char`, this with
`tolower_c()`'s `unsigned char` cast, and only the second is defined for the
high-bit bytes `alfa()` admits inside an identifier. It is also byte for byte
what issue 0010 folded the `.subckt` key with, so no subcircuit name changes
key.

`entrynb()` and `attrib()` each fold their key once, into a `DS_CREATE(key,
100)` buffer, and pass the folded key to `nghash_find`/`nghash_insert`.
`attrib()` still stores `entry->symbol = copy(t)`, the **unfolded** spelling,
so the deck's own spelling survives.

| Site | Change |
| --- | --- |
| `xpressn.c` `entrynb()` | probe key folded; one fold covers all scope depths |
| `xpressn.c` `attrib()` | probe and insert key folded; `entry->symbol` unchanged |
| `xpressn.c` `defsubckt()`, `findsubckt()` | issue-0010 `strtolower` **deleted** |
| `spicenum.c` `findsubname()` | issue-0010 `strtolower` **deleted** |
| `spicenum.c` `nupa_get_entry()` | was a second, unfolded copy of `entrynb()`'s scope walk; now delegates to `entrynb()` |
| `inpcom.c` `find_function()` | `strcmp` → `user_ident_eq()` (`.func` name) |
| `inpcom.c` `search_func_arg()` | whole-formal compare gated `strncmp`/`cieqn` |
| `inpcom.c` `inp_get_func_from_line()` | the `accept` filter admits both cases of each formal's first character under a non-folding mode |

`nupa_get_entry()` was the fourth probe and is not in this issue's table. It
serves `nupa_get_param()`/`nupa_get_string_param()`, i.e. `.meas` results and
the XSPICE auto-bridge (`src/xspice/evt/evtcheck_nodes.c`), and it probed the
same tables byte-exactly. Left alone it would have become the one lookup that
could not find what `attrib()` had just stored.

### Criterion 2, and why no `hash_func` was installed

`src/misc/hash.c:548` copies the key **only** for
`NGHASH_DEF_HASH(NGHASH_FUNC_STR)`, and the five frees — `:110`, `:182`,
`:393`, `:471`, `:866` — are gated on the same test (this issue's Root Cause
cited three of the five). Because `NGHASH_DEF_HASH` is a cast and
`NGHASH_FUNC_STR` is 0, that test is literally `hash_func == NULL`, and the
copy/free gate is on `hash_func` while the *comparator* gate is on
`compare_func`. Installing a case-insensitive hash while leaving the default
comparator would therefore keep `strcmp` semantics and silently flip every
numparam table from owning its keys to borrowing them — commits `c5cd68015`
and `5ad395d5e` are what that costs. Folding the key at the two call sites
also means the `DS_CREATE` buffer can be freed immediately after the call,
which is what both functions do.

### Criterion 1's reporting half

`entry->symbol` keeps the first spelling seen, and it is the only spelling
`listing param` prints (`dump_symbol_table()`, `spicenum.c`). Every diagnostic
prints the spelling of the site that failed, not the stored one, so
`Undefined parameter [NoSuchP]` still quotes the deck back to the user
verbatim. Measured by hand, since `tests/bin/check.sh`'s filter removes any
line containing `--->`:

```
$ ngspice -D casemode=preserve --batch l2.cir   ->        ---> RVal = 1000
                                                      v(2) = 3.000000e+00
$ ngspice --batch l2.cir                        ->        ---> rval = 1000
```

on a deck whose definition says `.param RVal=1k` and whose reference says
`{rval}`.

### Fold-mode visibility, per site

All of it is gated on `inp_case_folding()`, so in the default mode
`symbol_key()` returns its argument, `find_function()` runs `strcmp`,
`search_func_arg()` runs `strncmp`, and the `accept` filter holds exactly the
characters it held before. The default mode hashes and compares the same bytes
as it did at `e5913af03`, and `make check` confirms it: 204 tests, 0 FAIL, with
no committed `.out` touched.

### RED evidence

Four twin pairs in `tests/regression/case/`, one per mechanism, each with a
byte-identical `.out`, split so that no deck can hide another's abort:

| Deck | Mechanism | Failure before the fix |
| --- | --- | --- |
| `param-name-case.cir` | `.param` symbol | `Undefined parameter [rval]` |
| `subckt-formal-name-case.cir` | subcircuit formal | `Undefined parameter [rtop]` |
| `func-name-case.cir` | `.func` name (`find_function`) | `Undefined parameter [dbl]` |
| `func-formal-case.cir` | `.func` formal (`search_func_arg`) | `Undefined parameter [xx]` |

All four write to stderr and print nothing, so what `make check` sees is an
empty stdout and the assertion is the *missing* voltage. Each was first run
against an empty `.out`, where all four upper decks pass and all four lower
twins fail — which is the proof that the evidence is the number and not a
diagnostic.
