# Issue: The XSPICE Auto-Bridge Resolves Its Own Parameters by Byte-Exact Lower Case

## Status

Open. Found while closing Phase 3 gates 2 and 4, by an independent sweep of
`src/xspice/` for case dependencies that are not identifier comparisons. Four
defects, three of them in `src/xspice/evt/evtcheck_nodes.c` and one in a code
model. None is on any gate list; none is fixed by the gate commits, which
changed only the two identifier comparisons in that file's neighbourhood.

The reason they were not found by the identifier census is that none of them
is a `strcmp` between two identifiers. They are a symbol-table probe keyed by
a constructed literal, a parameter **value** embedded in a filename, and a
model **parameter value** compared against a lower-case literal — three shapes
that a `grep` for `strcmp`/`cieq` over identifier names cannot see.

## Summary

### (a) `vcc` and `family` are looked up under names ngspice spells in lower case

`find_bridge()` (`src/xspice/evt/evtcheck_nodes.c:475-528`) resolves the bridge
supply voltage and the logic family out of the numparam symbol table using
literals it builds itself:

```c
/* src/xspice/evt/evtcheck_nodes.c:482, :505, :525 */
        vcc_parm = "vcc";
...
            snprintf(dot + 1, sizeof buff - (size_t)(dot - buff), "family");
            family = nupa_get_string_param(buff);
...
        family = nupa_get_string_param("family");
```

`nupa_get_param()` and `nupa_get_string_param()` reach `entrynb()`
(`src/frontend/numparam/xpressn.c:413`) and thence `symbol_key()` (`:386`),
which is

```c
    if (inp_case_exact_ids())
        return s;
```

i.e. the key is folded only under `preserve`. Under `fold` the reader already
lower cased the deck, so a `.param VCC=5` arrives as `vcc` and the probe hits.
Under `preserve` the key is folded and the probe hits. Under `distinguish` the
table keeps `VCC` and the probe asks for `vcc`, so it misses.

### (b) the `family` parameter *value* becomes a filename and a subcircuit name

```c
/* src/xspice/evt/evtcheck_nodes.c:539, :589, :594 */
    snprintf(buff, sizeof buff, "bridge_%s_%s_%s.subcir", family, ...);
```

`family` here is a string **value** the deck wrote on a `.model` card. It is
embedded in a filename resolved by `inp_pathresolve()`, in an `.include`, and
in the generated `Xauto_bridge%d ... bridge_%s_%s_%s` subcircuit reference. A
POSIX filesystem is byte exact in every mode.

### (c) two casemode-independent defects in the same function

```c
/* src/xspice/evt/evtcheck_nodes.c:370-372, inside examine_device() */
                 *family = pdp->element->svalue; // May be NULL
                 if (family && !family[0])
                     family = NULL;     // Ignore empty string.
```

`family` is the `const char **` out-parameter and is never NULL there;
`family[0]` is `*family`; and the assignment clobbers the local
pointer-to-pointer instead of the caller's string. The empty-string guard is
therefore dead in both directions. The correct form is
`if (*family && !(*family)[0]) *family = NULL;`.

```c
/* src/xspice/evt/evtcheck_nodes.c:558 */
                if (!strcmp(family, bridge->family)) {
```

`bridge->family` is assigned from `family` at bridge creation (`:669`) and may
be NULL, while the enclosing `if (family)` guarantees only the left operand.
A family-bearing node reaching an existing family-less bridge of the same
`udn_index` and `direction` dereferences NULL.

### (d) `multi_input_pwl` exits on an upper-case model name

```c
/* src/xspice/icm/analog/multi_input_pwl/cfunc.mod:207-218 */
      if (!strcmp(model, "and"))
...
      else {
	  fprintf(stderr, "ERROR(cm_multi_input_pwl): unknown gate model type "
                  "'%s'; expecting 'and|or|nand|nor'.\n", model );
	  exit(-1);
```

`model` is a string-valued instance parameter. `and`/`or`/`nand`/`nor` are
keywords naming a behaviour, and keywords are case insensitive in every mode
by the spec's compatibility contract point 2, so `cieq` is the right
comparison here in all three modes.

## Impact

- (a) `distinguish` only, and it is a **silent wrong number**: with the `.param`
  invisible, `find_bridge()` falls through to `vcc = 3.3` for digital
  (`evtcheck_nodes.c:519`) and to no family, so a deck that set `.param VCC=5`
  gets a 3.3 V bridge and no diagnostic. This is the reason `set_case_mode()`
  still calls `distinguish` experimental after gates 2 and 4 closed.
- (b) `preserve` and `distinguish`, and `preserve` has shipped decks. A deck
  writing `family='74HCT'` finds no `bridge_74HCT_d_out.subcir`, so the family
  bridge silently degrades to the built-in default bridge — a different device
  with different levels, with no message. Under `fold` the same deck works,
  because the reader lower cased the card.
- (c) both are mode independent. The clobber is a dead guard; the missing NULL
  check is a crash on a reachable combination.
- (d) `preserve` and `distinguish`, and it is a hard `exit(-1)` rather than a
  wrong answer: `model=OR` runs under `fold` and kills the process in the two
  other modes.

## Root Cause

The spec's identifier scope is nets, instances, pins, `.model` and `.subckt`
names. Every defect above is a name **outside** that scope — a parameter name
ngspice constructs, a parameter value, a filename, a behaviour keyword — and
each was written when `fold` was the only mode, so "the deck is already lower
case" was a safe assumption at the point of use. `preserve` and `distinguish`
withdrew that assumption without these sites being re-read, because they are
not identifier comparisons and no census that greps for identifier comparisons
finds them.

(a) is the mirror image of the Class C rule in
`doc/claude/decisions/0001-distinguish.md` decision 3: there the query comes
from outside and the table holds deck text, so the query must be tolerant.
Here the query is a lower-case literal ngspice constructed and the table holds
deck text, so the same rule — "distinguish must be exact about names the deck
chose and case insensitive about names ngspice constructs" — says the probe
must be tolerant, which under an exact `symbol_key()` it cannot be.

## Acceptance Criteria

- A deck under `casemode=distinguish` that writes `.param VCC=5` and relies on
  the auto-bridge gets a 5 V bridge, or a diagnostic; not a silent 3.3 V.
- A deck under `casemode=preserve` that writes `family='74HCT'` selects the
  same bridge it selects under `fold`, or says why it cannot.
- `examine_device()`'s empty-`family` guard actually clears the caller's
  string, with a test that a `.model` carrying `family=""` behaves as if the
  parameter were absent.
- `find_bridge()` does not dereference a NULL `bridge->family`.
- `A1 %vd(...) ... model=OR` under `preserve` runs and behaves as `model=or`.
- Decks for each, under the mode that fails: `tests/xspice/case/` for the
  `preserve` items and `tests/xspice/casedist/` for the `distinguish` one.
  Both directories exist.

## Resolution

Not fixed. Recorded here rather than folded into the Phase 3 gate commits
because none of these is an identifier comparison, so none of them is what
those commits are about, and (b) and (d) are `preserve` regressions that need
their own evidence in the mode that has shipped decks.

Suggested shapes, none of them decided:

- (a) fold the *probe*, not the table: give numparam a lookup that asks for a
  constructed name case insensitively, or have the auto-bridge try the deck's
  own spelling first. The narrow fix is that `symbol_key()` is right and the
  caller is wrong to hand it a literal.
- (b) resolve the filename case insensitively, or fold `family` once where it
  is read out of the model parameter at `evtcheck_nodes.c:369-372` — the spec's
  "Fold sites outside the reader" section already lists `family` as a fold-once
  candidate (`doc/claude/specs/case-sensitive-identifiers.md:279-281`).
- (c) the two one-line corrections quoted above.
- (d) `cieq` for the four keywords.
