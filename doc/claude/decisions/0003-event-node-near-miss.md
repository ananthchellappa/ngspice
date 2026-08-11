# Decision 0003 — the event-node near-miss diagnostic, `doc/codex/issues/0030`

## Status

Accepted, 2026-08-11, branch `ver_50`. Implements decision 2 of
`doc/claude/decisions/0001-distinguish.md` at the two XSPICE event sites Phase
3 gates 2 and 4 left silent: the auto-bridge
(`src/xspice/evt/evtcheck_nodes.c`) and the event-node interner
(`src/xspice/evt/evttermi.c`). It is the event-driven twin of
`doc/claude/decisions/0002-deferred-node-resolution-check.md`, which did the
same for the parser, and it re-decides nothing that record settled.

## Context

Both sites now compare with `ng_ideq()`, so under `casemode=distinguish` they
compare byte for byte, which is what `distinguish` means. What neither had was
the other half of decision 2: a warning when a *resolution* fails and a name
differing only in case is present.

Measured at `7304bba95`, with the run completing and printing a plausible
number in both:

- `tests/xspice/casedist/auto-bridge-node-case-split.cir` — digital `A`,
  analog `a`. No bridge is inserted, the analog net is driven by nothing but
  `R1` to ground, `v(a) = 0.000000e+00`, stderr carries only the
  experimental-mode warning.
- `tests/xspice/casedist/event-node-case-split.cir` (added here) — the
  `adc_bridge` drives event node `Dig`, the `d_buffer` reads event node `dig`.
  They are two nodes, the `d_buffer` sees the digital type's default for the
  whole run, `v(aout) = 0.000000e+00`, and again nothing on stderr.

Nothing in this record changes a computed value. Both changes only write to
`stderr`, and every number in the tree is identical before and after, in all
three modes.

---

## Decision 1 — the interner reports a node nothing drives, not a near-miss definition

**Decided: report an event node with `num_outputs == 0` when another node in
the list has `num_outputs > 0` and the two names differ only in case.**

`0030`'s acceptance criterion asked for the warning whenever "an A card names
an event node that differs only in case from an existing one". That criterion
is not implemented, and the deviation is the substance of this record.

It is a test on a **definition**. Decision 2 of `0001-distinguish.md` rejects
warning on a definition by name and gives the reason: under `distinguish`,
`Out` and `OUT` as two event nodes is not a mistake, it is the feature; a
warning there makes the mode unusable and trains users to ignore the warning
that matters. Implementing the criterion literally would have implemented the
option that record rejected, at the one site where the rejection is most
obviously right — every mention of an event node is an A card port, so *every*
event node is a definition and the literal criterion fires on all of them.

The diagnosable condition is the event analogue of `0002`'s `t_unclaimed` bit.
It could not be maintained the same way. On the parser side the bit records
*how* the entry was created, because `mkvnode()` is a reference and a device
card is a definition; on the event side there is no such split to record. So
the bit is not stored during parsing but read off the finished node:

- **`num_outputs == 0`** is the one state in which the node can never carry an
  event. Nothing ever posts a value to it, so every reader sees the node
  type's default for the whole run. That is a node a mention created and no
  driver ever claimed.
- **`num_ports` cannot answer it.** One port is legal both ways round: an
  output nothing reads and an input nothing drives are both single-port nodes,
  and only the second is a failed resolution. The obvious phrasing — one port,
  never otherwise claimed — would have reported the unread output too.
- **`inst_list` answers the opposite question.** It lists the instances that
  take the node as an *input*, so it is non-empty exactly for the reader whose
  lookup missed. Using it as the claim bit inverts the test.

The case variant is named only when it is itself driven, which is
`INPtermCaseCheck()`'s `!u->t_unclaimed` (`src/spicelib/parser/inpsymt.c:155`).
If nothing in the circuit drives either spelling, the deck's mistake is not the
case of the name and naming the sibling would mislead.

What the narrowing costs, stated plainly: an event node that is driven and read
by nothing else, whose name differs only in case from a real net — the
misspelling on the *driving* card, with no reader — is reported through its
victim, not through itself. `A1 in OUT buf` beside `A2 Out o2 buf` reports
`Out` and names `OUT`, which is the pair the author has to look at, but the
message points at the reader. And a deck in which *both* spellings are undriven
is silent, because there is no resolution to have missed. Neither case prints a
wrong number silently, which is decision 2's goal.

Rejected: reporting at `EVTnode_insert()` itself, i.e. at the A card. Beyond
being the definition test above, it cannot see the auto-bridge — see decision
3 — so it would report every auto-bridged input node in the tree.

## Decision 2 — the auto-bridge reports per event node, after that node's scan

**Decided: report from `Evtcheck_nodes()` at the end of an event node's inner
loop over `ckt->CKTnodes`, not at the failed comparison and not from a second
pass over the event node list.**

`0030` expected "a second, non-mutating pass over `ckt->evt->info.node_list`
after the bridging loop, in the style of `INPtermCaseCheck()`". It buys
nothing here. The bridging loop mutates neither `CKTnodes` nor the event node
list: it accumulates card text, and those cards are parsed by `INPpas1`/
`INPpas2` after the loop has finished. So "this event node matched no analog
node" is already decidable at the end of the node's own inner loop, and a
second pass would re-derive it.

Reporting *inside* the inner loop, at the comparison that failed, is wrong for
a different reason and this is the one that decides the shape: a single failed
comparison is not a failed resolution. The loop compares one event node
against every analog node, so `A` can miss `a` and still match `A` further
down the list — under `distinguish` both spellings can be live analog nets at
once. Reporting at the miss would warn about a net that was in fact bridged,
and would warn once per analog node besides.

The nodes the bridging adds are not missed by checking only the pre-bridge
list. A generated bridge card carries the **analog** spelling in both port
lists, so any event node it interns matches an analog node exactly by
construction.

## Decision 2a — a bridge the deck wrote by hand is not a failed resolution

**Decided: `already_joined()` — say nothing when some XSPICE instance has a
port on this event node and a port on the analog node that differs from it
only in case.**

This was found by reviewing the first version of decision 2 adversarially, and
it is the case that version got wrong. Under `distinguish` the two sides of a
bridge are two names, so writing them as one name in two cases is a natural
way to spell the device:

```spice
A2 Dig DOUT dbuf
A3 [DOUT] [dout] dac1
R1 dout 0 1k
```

`v(dout) = 5.0`, every net is driven, and the deck is correct — and only
correct under `distinguish`, because `fold` and `preserve` collapse `DOUT` and
`dout` into one name and the auto-bridge then inserts a second, redundant
bridge onto the net. The first version of this record reported it. A warning
on the legitimate deck is what decision 2 of `0001-distinguish.md` rejects in
so many words: it trains users to ignore the warning that matters, and here it
fired hardest on the decks the mode exists to enable.

The test is on the connection, not on the name: one instance with a port on
both nodes is exactly the connection the auto-bridge would have made. The
event side is matched by node index, the way `scan_ports()` does it, and the
analog side by `CKTnode` number, so no comparison of spellings enters it.

Rejected: **suppress when the event node is both driven and read**
(`num_outputs > 0 && num_insts > 0`), which is the liveness test the interner
half uses and was the reviewer's proposed fix. It re-admits the silent wrong
number this issue exists to remove:

```spice
A2 Dig A dbuf
A3 A x dbuf
R1 a 0 1k
```

Event node `A` is driven by the first card and read by the second, so that
test says nothing, and the analog net `a` is still floating and still prints
`0.0`. `already_joined()` reports it, correctly, because no instance ties `A`
to `a`.

What the connection test costs, stated plainly: an event node bridged by hand
to some *other* analog node, in a circuit that also has an unrelated analog
node differing from it only in case, is still reported. The report is then
about a coincidence of spelling. That is the same residual the parser's
`INPtermCaseCheck()` accepts, and it is the direction to err in — a false
positive is a line on `stderr`, a false negative is a wrong number nobody
sees.

## Decision 3 — the interner's check runs after the auto-bridge

**Decided: `EVTnode_case_check()` is called from `src/frontend/spiceif.c`,
after `Evtcheck_nodes()` and before `EVTinit()`.**

An event node that takes its value from the analog side is driven by the
`adc_bridge` the auto-bridge inserts — `find_bridge()` picks `MIF_IN` for
exactly `num_outputs == 0` — and that instance does not exist until
`Evtcheck_nodes()` has pushed its cards through `INPpas2`. Running the check
any earlier would report every auto-bridged input node in every `distinguish`
deck, which is a false positive on correct decks and the failure mode decision
2 most wants to avoid.

The call sits beside `INPtermCaseCheck()`, which is the same kind of
end-of-parse sweep for the analog side, so the two deferred diagnostics are
adjacent and read as one rule. The order of the three messages a deck can now
produce is auto-bridge, then parser, then event interner.

## Decision 4 — a warning on `stderr`, and the guard is on the mode

**Decided: `Warning:` on `stderr`, run continues, both sites gated on
`inp_case_mode() == NG_CASE_DISTINGUISH`.**

Decision 3 of `0002-deferred-node-resolution-check.md` argued warn-not-abort
for the parser and the argument transfers without change: an unresolved node
is legal in SPICE, the immediate neighbours warn and continue, and by the time
either check fires the circuit is built.

The mode guard is belt and braces rather than policy, in both modes and at
both sites:

- Under **preserve** `ng_ideq()` is case insensitive at both sites, so a name
  that matches when case is ignored has already matched — it was bridged, or
  it was interned onto the existing node — and neither condition can arise.
- Under **fold** the reader has lowercased every card, so two spellings of one
  name are one name. The residual risk that made the parser's guard a real
  decision — names ngspice constructs for itself with upper case, such as
  `q1#collCX` — does not exist on this path: the only spellings that reach the
  event node list other than the deck's own are the ones the auto-bridge
  generates, and it copies those from analog node names the reader lowercased.

`tests/xspice/digital/event-node-case-fold.cir` and
`tests/xspice/digital/auto-bridge-node-case-fold.cir` hold the deck's
spellings constant and change only the mode, so they pin that the guard is on
the mode and not on the spelling.

Text, unchanged from `0030` and from the form decisions `0001` and `0002`
already use:

```
Warning: no event node named 'dig'; 'Dig' differs only in case (casemode=distinguish)
```

and, at the auto-bridge, the same sentence with the noun that names the side
whose lookup failed — the event node was searching `ckt->CKTnodes`:

```
Warning: no analog node named 'A'; 'a' differs only in case (casemode=distinguish)
```

**Guarded since 2026-08-11**, by
`tests/xspice/casedist/event-node-case-report.cir` and
`tests/xspice/casedist/auto-bridge-node-case-report.cir`. Each asserts its own
noun, asserts that the other noun is absent, and asserts the silences these
decisions owe: `already_joined()`'s hand-written bridge and the ordinary
exactly-matched bridge for decision 2, and a dangling event node with no case
variant plus an auto-bridged input node beside a driven case variant for
decisions 1 and 3 — that last one reports if the check is moved before
`Evtcheck_nodes()`, which is what decision 3 is about. Both decks source
their circuits from inside a `.control` block so that `>&` can reach the
parse, which needed both diagnostics to write to `cp_err` rather than to
`stderr`. `doc/codex/issues/0036`,
`doc/claude/decisions/0006-diagnostic-deck-coverage.md`.

Rejected: calling both "node". The two sites fail on opposite sides of the
mixed-signal boundary, and a deck that gets both messages should be able to
tell which is which without reading the source.

## What this decision does not decide

1. **The generic undefined-node diagnostic**, `doc/codex/issues/0028` — a
   reference that misses with **no** case variant present, on either side of
   the boundary. Unchanged and still open.
2. **`doc/codex/issues/0027`**, `vec_remove()`'s unconditional `cieq`. It was
   what `set_case_mode()`'s experimental-mode warning still named, and was the
   only thing it named. **Closed since**, at `756112c46`, with
   `doc/claude/decisions/0004-unlet-vector-identity.md`; the warning's clause
   now names `doc/codex/issues/0032` instead. `0004` decision 3 reaches the same
   conclusion this record's decision 1 did, at a different site and by the same
   argument: `unlet` resolves a name and is reported, while `compose` and
   `cross` define one and are silent.
3. **`doc/codex/issues/0031`**, a family-less node reusing whatever family
   bridge was created first. Mode independent, so no diagnostic here touches
   it.
4. **Whether an undriven event node deserves a diagnostic of its own**, with
   no case variant involved. That is the event half of `0028` and has the same
   risk profile: a dangling event node is legal, so it would fire on correct
   decks and needs the suite and the differential sweep behind it.
