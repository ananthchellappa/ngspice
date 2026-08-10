# Issue: Model, Subcircuit and Global-Node Lookups Are Case Sensitive

## Status

Open

## Summary

Phase 2 of the case-sensitivity work made the parser's own interning tables
fold their lookup key, so nodes and instances keep one identity across
spellings (`src/spicelib/parser/inpsymt.c`). Three other name spaces were left
alone, and each still matches with `strcmp`:

| Name space | Site | Comparison |
| --- | --- | --- |
| `.model` name | `src/spicelib/parser/inplkmod.c:24` | `strcmp(i->INPmodName, name)` |
| `.model` name | `src/spicelib/parser/inpmkmod.c:38`, `:60` | nghash with `NGHASH_FUNC_STR`, i.e. `strcmp` |
| `.subckt` name | `src/frontend/subckt.c:604` | `eq(sss->su_name, s)` |
| `.subckt` translation table | `src/frontend/subckt.c:1610` | `eq(table[i].t_new, subname)` |
| `.global` node | `src/frontend/subckt.c:146`, `:1650` | nghash with `NGHASH_FUNC_STR` |
| instance name | `src/include/ngspice/cktdefs.h:322` `DEVnameHash`, used at `src/spicelib/analysis/cktdltm.c:36` | `strcmp` |

## Impact

Under `fold` none of this is reachable: the reader has already lowercased both
sides.

Under `casemode=preserve` the contract is that identity is unchanged — `R1` and
`r1` are the same device, `preserve` changes only what ngspice shows
(`doc/claude/specs/case-sensitive-identifiers.md`). These six sites break that
contract for a deck that spells one name two ways:

```spice
r3 1 2 RModel1 l=11u w=2u
.model rmodel1 r rsh=1000 narrow=1u
```

```
$ ngspice -D casemode=preserve --batch two-spellings.cir
Error on line 2 or its substitute:
  r3 1 2 RModel1 l=11u w=2u
  unknown parameter (RModel1)
```

The `.global` case is the quieter one, and the only one that can produce a
wrong answer rather than a diagnostic. `inpcom.c:1939` inserts a hard-coded
lower-case `.global gnd`, `collect_global_nodes()` stores the node name
verbatim into a `strcmp` hash (`subckt.c:146`, `:161`), and `gettrans()` probes
it with `nghash_find` (`subckt.c:1650`). A deck whose subcircuit port is
spelled `GND` therefore does not match the automatic global, so the port is
scope-renamed per instance instead of being shared. Note that
`inp_fix_gnd_name()` cannot rescue it: its delimiter test requires a character
after the token (`gnd[3]`), and the newline has already been zapped at
`inpcom.c:1889`, so a card that *ends* in `GND` — which is what a `.subckt`
header does — is never rewritten.

A user cannot see any of this in a rawfile, because a rawfile has no
case-policy field.

## Root Cause

The plan calls this the "fold-the-key refactor" and rates it separately from
the rest of Phase 2, at roughly 25% merge odds, because it "touches parser,
devices, XSPICE" with "no user-visible payoff the day it lands"
(`doc/claude/suggestions/case-sensitive-identifiers-plan.md`). It was scheduled
out of Phase 2 deliberately, not overlooked.

The constraint that makes it more than a one-line change is
`src/misc/hash.c:548`: `curTable->key = copy(user_key)` runs **only** when
`hash_func == NGHASH_DEF_HASH(NGHASH_FUNC_STR)`, and the frees at `:110`,
`:182`, `:393` and `:471` are gated the same way. Installing a custom
case-folding hash function silently flips every such table from owning its keys
to borrowing them, which is the exact class of defect commits `c5cd68015` and
`5ad395d5e` were written to fix. The key has to be folded at the call site
instead, in every insert and every probe, or `nghash` needs a first-class
case-insensitive string mode that keeps the copy-and-free behaviour.

## Acceptance Criteria

1. Under `preserve`, a `.model`, `.subckt` or `.global` name matches its
   references regardless of spelling, with the first-seen spelling retained for
   reporting.
2. No `nghash` table changes from owning its keys to borrowing them. Either the
   key is folded by the caller before `nghash_insert`/`nghash_find`, or
   `NGHASH_FUNC_STR` gains a case-insensitive sibling that keeps the
   `copy(user_key)` and the matching frees.
3. `tests/regression/case/` gains a deck that spells one model name, one
   subcircuit name and one global node two ways each and produces the same
   voltages as its consistently-spelled twin.
4. Instance-name lookup through `DEVnameHash` resolves `alter r1` against a
   deck that declared `R1`.
5. `make check` unchanged with `casemode` unset.

## Resolution

Not fixed here. Recorded so that the gap between what `preserve` promises —
identity unchanged — and what it currently delivers is written down rather than
discovered. Until this lands, `preserve` requires a deck to spell each model,
subcircuit and global-node name consistently; nodes and instances are already
covered by `inpsymt.c`.
