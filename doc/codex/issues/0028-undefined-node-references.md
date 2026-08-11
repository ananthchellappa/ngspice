# 0028 — a node reference that no card defines is still silent when no case variant exists

## Status

Open. Found while closing Phase 3 gate 1 on branch `ver_50`
(`doc/claude/decisions/0002-deferred-node-resolution-check.md`). Deliberately
not fixed there; the gate needed the case near-miss, this is the general case.

## Summary

`mkvnode()` (`src/spicelib/parser/inpptree.c:1249`) resolves a `V()` reference
inside a B-source expression with a call that **creates the node on a miss**.
Gate 1 added `INPtermCaseCheck()` (`src/spicelib/parser/inpsymt.c`), which
reports such a node at the end of the parse **only** when a name differing from
it only in case is defined, and only under `casemode=distinguish`. Two gaps
remain:

1. **The plain typo.** `B1 out 0 V={V(mdi)*2}` against a net `mid` manufactures
   a node and says nothing, in all three case modes. The bit that would answer
   it is already recorded — `INPnTab.t_unclaimed` — so the diagnostic is one
   `if` away; what is missing is the decision and the evidence, not the
   mechanism.
2. **The other create-on-miss reference sites.** These resolve a node name with
   `INPtermInsert()` rather than `INPtermInsertRef()`:

   | Site | Card |
   | --- | --- |
   | `src/spicelib/parser/inp2dot.c:53`, `:59` | `.NOISE` output node |
   | `src/spicelib/parser/inp2dot.c:371`, `:376` | `.SENS` output |
   | `src/spicelib/parser/inp2dot.c:495`, `:501` | `.TF` output |
   | `src/spicelib/parser/inp2dot.c:677`, `:775` | `.PSS` oscnode |
   | `src/spicelib/parser/inpgval.c:90` | an `IF_NODE`-typed device parameter |

   Each is a reference, not a definition, so each has gate 1's defect in its
   own right. They are also a **false-negative source for the gate 1
   diagnostic**: because they insert as definitions, a `.SENS v(in)` naming the
   same manufactured node clears `t_unclaimed` and suppresses the near-miss
   warning that would otherwise fire.

## Impact

Gap 1: a misspelt node in a behavioural expression silently changes the circuit
— the expression reads 0 from a node that exists only because it was misspelt.
This is mode-independent and predates the `casemode` work entirely.

Gap 2: as gap 1, plus a hole in the gate 1 warning. Both are quiet wrong
answers rather than crashes.

## Root Cause

`INPtermInsert()` is used for two different jobs — putting a node on the
netlist, and naming one that is expected to exist — and its create-on-miss
behaviour is correct for the first and wrong for the second. Gate 1 separated
the two at `mkvnode()` only.

## Acceptance Criteria

- A deck whose B source references a node name that no card defines, with no
  case variant present, produces a diagnostic naming the reference.
- The decision on **which modes** it fires in is made explicitly and recorded.
  It is a behaviour change in the default mode, so it needs the full
  `make check` and a differential-sweep run behind it: a node mentioned once is
  a legal floating node in SPICE and some existing decks may rely on it.
- `inp2dot.c` and `inpgval.c` node references use `INPtermInsertRef()`, each
  with a deck that shows the reference resolving correctly when it is spelled
  correctly.
- A deck in which `.SENS` or `.NOISE` names the manufactured node still gets
  the gate 1 near-miss warning.

## Resolution

None yet.
