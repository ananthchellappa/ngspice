# 0028 — a node reference that no card defines is still silent when no case variant exists

## Status

**Closed**, both gaps, 2026-08-11 on branch `ver_50`; see Resolution.
Found while closing Phase 3 gate 1 on branch `ver_50`
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

Closed by `17e429993` (gap 1) and `7b18264c4` (gap 2), recorded in
`doc/claude/decisions/0008-undefined-node-diagnostic.md`.

Gap 1. `INPtermCaseCheck()` is one scan with two reports. An unclaimed entry
with a defined case variant keeps the near-miss text and its `distinguish`
guard; one with no variant gets

```
Warning: no node named 'mdi'; it is referenced but no card defines it
```

in **all three modes**, because nothing about a plain undefined node is about
case. Guarded by `tests/regression/misc/bsource-undef-node-report.cir` — the
typo reported, a legal floating node silent, a forward reference silent — which
is in `misc` and not in `casedist` because the report fires with no `casemode`
set. `tests/regression/casedist/bsource-node-case-report.cir`'s NOTWIN case,
which pinned this silence as this issue's class, now asserts the new text by
name and that the near-miss text is absent.

Gap 2. All nine sites take `INPtermInsertRef()`. Criterion 4 is
`tests/regression/casedist/sens-node-case-report.cir`: a `.sens` naming the
node a `V()` reference manufactured used to clear the bit and suppress the
near miss, and now does not. Criterion 3 is
`tests/regression/misc/dotcard-undef-node-report.cir`, which shows `.noise`,
`.tf`, `.sens` and `.pz` resolving correctly when spelled correctly, staying
silent when the dot cards precede the cards that define their nodes, and
reporting each misspelling by name.

Three corrections to this issue's own table, re-read at `17e429993`:
`:371`/`:376` is `.TF` and `:495`/`:501` is `.SENS`, not the other way round;
`:775` is `.hb`, not a second `.PSS`; and `inpgval.c:90` is not reachable from
a device parameter — no device or model in the tree declares an `IF_NODE`
parm, and `.pz`'s four nodes are that site's whole reachable surface. `.pss`
(`#ifdef WITH_PSS`) and `.hb` (`WITH_HB`, defined nowhere) cannot be exercised
by any deck in this build and were changed mechanically.

The acceptance criterion the decision spent most of its evidence on is the
second: every `.cir` file under `tests/` — 309 before, 312 after — was run
under each of the three modes, in its own directory's harness, and **none
trips the report**. The bit is set by a reference and cleared by any
definition, so a legal floating node — named by one card — is not a candidate
for it. The tool is `doc/claude/scripts/undefined_node_census.py`, added with
this work.

Two things this closure does not cover, both found by reviewing its own
commits. `2d57025d4` had to remove a false positive it introduced: a card
whose syntax `INP2dot()` rejects abandons pass 2, and the nodes the unparsed
cards would have defined were reported as undefined. And `doc/codex/issues/0043`
is the same four analyses typed as **commands** rather than as dot cards:
they reach `INPpas2()` through `if_run()`, which never calls
`INPtermCaseCheck()`, so the report does not fire there.

`make check` 271 PASS, 0 FAIL, up from 268 by exactly the three new decks.
