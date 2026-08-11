# Issue: An XSPICE Event-Node Reference That Misses by Case Is Not Diagnosed

## Status

Fixed on branch `ver_50`; see Resolution. Found while closing Phase 3 gates 2
and 4. Those gates closed the
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

  **Amended when this was implemented.** As written this is a test on a
  *definition*, and decision 2 of `0001-distinguish.md` rejects warning on a
  definition by name: under `distinguish`, `Out` and `OUT` as two event nodes
  is the feature. The criterion that was implemented instead is the event
  analogue of `0002`'s `t_unclaimed` bit — an event node that **nothing
  drives** when a **driven** node differs from it only in case. The message
  text is unchanged. `doc/claude/decisions/0003-event-node-near-miss.md`
  carries the argument.
- Under `casemode=distinguish`, an event node that matches no analog node
  exactly but case-insensitively matches one produces the analogous warning at
  the auto-bridge, and the run continues.
- Nothing is printed under `fold` or `preserve`, in which the conditions are
  unreachable.
- Verified by hand and quoted in the commit, not asserted by a deck:
  `tests/bin/check.sh` captures stdout only and its `egrep -v` filter drops
  every line containing `Warning`.

## Resolution

Fixed on `ver_50` in two commits, one per site, plus a third that retires the
clause this issue had in `set_case_mode()`'s experimental-mode warning.

**The auto-bridge**, `report_bridge_case_miss()` in
`src/xspice/evt/evtcheck_nodes.c`, called from `Evtcheck_nodes()` once an
event node's scan of `ckt->CKTnodes` has finished without an exact match:

```
Warning: no analog node named 'A'; 'a' differs only in case (casemode=distinguish)
```

It stays silent when the deck has already joined the two nodes by hand —
`already_joined()`, one XSPICE instance with a port on both — because under
`distinguish` case-splitting the two sides of a hand-written bridge is a
legitimate and mode-specific way to write it, and warning on it is the false
positive decision 2 rejects. That guard was added after an adversarial review
of the first version of the fix; `tests/xspice/casedist/hand-bridge-node-case-split.cir`
is its deck.

The second, non-mutating pass over `ckt->evt->info.node_list` that this issue
asked for turned out not to be needed. The bridging loop mutates neither
`CKTnodes` nor the event node list — it accumulates cards, which are parsed
after it — so "this event node matched no analog node" is already decidable at
the end of the node's own inner loop. Reporting *inside* that loop would be
wrong for a different reason: a single failed comparison is not a failed
resolution, since `A` can miss `a` and still match `A` further down the list.

**The interner**, `EVTnode_case_check()` in `src/xspice/evt/evttermi.c`,
called from `src/frontend/spiceif.c` after `Evtcheck_nodes()` and before
`EVTinit()`:

```
Warning: no event node named 'dig'; 'Dig' differs only in case (casemode=distinguish)
```

It reports an event node with `num_outputs == 0` when another node in the list
has `num_outputs > 0` and the two names differ only in case. It must run after
the auto-bridge, because the `adc_bridge` the auto-bridge inserts is the
driver of an event node that takes its value from the analog side —
`find_bridge()` picks `MIF_IN` for exactly `num_outputs == 0`.

Both are gated on `inp_case_mode() == NG_CASE_DISTINGUISH`; under `fold` and
`preserve` the conditions are unreachable, so the guard is belt and braces.
Nothing that either site touches changes a computed value: both only write to
`stderr`.

Decks: `tests/xspice/casedist/event-node-case-split.cir` (the interner split,
`v(aout) = 0.0`), `tests/xspice/casedist/event-node-dangling-no-variant.cir`
(a dangling event node with no case variant, not reported),
`tests/xspice/digital/event-node-case-fold.cir` and
`tests/xspice/digital/auto-bridge-node-case-fold.cir` (the same spellings in
the default mode, no report, numbers unmoved), and
`tests/xspice/casedist/hand-bridge-node-case-split.cir` (the same case split
with a hand-written bridge across it, not reported, `v(dout) = 5.0`). The
auto-bridge half reused `tests/xspice/casedist/auto-bridge-node-case-split.cir`,
which already asserted `v(a) = 0.0`; its header called the miss silent and
pointed at `doc/codex/issues/0029` by mistake, and now names this issue and
quotes the line.

`doc/codex/issues/0028` is the neighbouring, larger question of diagnosing a
reference that misses with **no** case variant present, and stays open.
