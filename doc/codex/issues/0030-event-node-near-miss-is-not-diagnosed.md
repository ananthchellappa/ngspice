# Issue: An XSPICE Event-Node Reference That Misses by Case Is Not Diagnosed

## Status

Open. Found while closing Phase 3 gates 2 and 4. Those gates closed the
**comparators** at `src/xspice/evt/evtcheck_nodes.c` and
`src/xspice/evt/evttermi.c`; they did not add the near-miss diagnostic that
decision 2 of `doc/claude/decisions/0001-distinguish.md` asks for at a failed
resolution. This is the event-driven half of what
`doc/claude/decisions/0002-deferred-node-resolution-check.md` did for the
parser, and it is the same shape.

## Summary

Two resolutions on the event side fail silently under `casemode=distinguish`
when a name differing only in case is present.

**The interner.** `EVTnode_insert()` (`src/xspice/evt/evttermi.c`) is
find-or-create over `ckt->evt->info.node_list`. It now compares with
`ng_ideq()`, which is byte exact under `distinguish`, so an A card writing
`OUT` against an existing event node `Out` creates a second event node — which
is what `distinguish` means — and says nothing. Nothing distinguishes that
from a deliberate second node, and an event node with one port is a legal
dangling node, exactly as a node mentioned once is a legal floating node in
the analog world.

**The auto-bridge.** `Evtcheck_nodes()` (`src/xspice/evt/evtcheck_nodes.c`)
now matches the event side against `ckt->CKTnodes` with `ng_ideq()`. Under
`distinguish` a digital `A` and an analog `a` correctly do not match, so no
bridge is inserted and the analog net is driven by nothing. The run completes
and prints a number — `tests/xspice/casedist/auto-bridge-node-case-split.cir`
asserts `v(a) = 0.000000e+00` — with no message.

## Impact

`distinguish` only. Under `fold` the reader lower cased both spellings, so the
condition is unreachable; under `preserve` both comparisons are case
insensitive, so a near miss resolves rather than missing and the condition is
again unreachable. The gate commits leave both modes byte identical.

Under `distinguish` the failure is the one decision 2 exists to prevent: a
resolution misses, the miss produces a plausible number instead of an error,
and the deck author cannot see the difference between `A` and `a` in their own
netlist.

## Root Cause

Decision 2 of `0001-distinguish.md` splits definition from resolution: silence
when a name is being defined, a warning when a name is being *resolved*, the
resolution fails, and a case variant exists. The event side has no place where
that test is made. `EVTnode_insert()` creates on a miss for the same reason
`INPtermInsert()` does — an A card may name a node no earlier card mentioned —
so, exactly as at Phase 3 gate 1, the diagnostic cannot be raised at the
reference and has to be deferred to a point where "created and never otherwise
claimed" is decidable.

## Acceptance Criteria

- Under `casemode=distinguish`, an A card naming an event node that differs
  only in case from an existing one produces on `stderr`

  ```
  Warning: no event node named 'OUT'; 'Out' differs only in case (casemode=distinguish)
  ```

  matching the wording decisions 0001 and 0002 already use for the vector
  table and for `mkvnode`.
- Under `casemode=distinguish`, an event node that matches no analog node
  exactly but case-insensitively matches one produces the analogous warning at
  the auto-bridge, and the run continues.
- Nothing is printed under `fold` or `preserve`, in which the conditions are
  unreachable.
- Verified by hand and quoted in the commit, not asserted by a deck:
  `tests/bin/check.sh` captures stdout only and its `egrep -v` filter drops
  every line containing `Warning`.

## Resolution

Not fixed. Deliberately out of the gate-2 and gate-4 commits, which are about
what the simulator *computes*; this is about what it *says*, it is
`distinguish`-only where those commits are `preserve`-only, and it is the
larger of the two changes because the auto-bridge half needs a second,
non-mutating pass over `ckt->evt->info.node_list` after the bridging loop, in
the style of `INPtermCaseCheck()` (`src/spicelib/parser/inpsymt.c`).

`doc/codex/issues/0028` is the neighbouring, larger question of diagnosing a
reference that misses with **no** case variant present. This issue is the
narrow one that decision 2 already answered in principle and that only needs
implementing at two sites.
