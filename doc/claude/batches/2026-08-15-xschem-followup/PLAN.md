# Plan: the xschem follow-up, after 0070

Driver session, 2026-08-15, branch `ver_50`, HEAD at start `7b5884249`.

Five items, in this order, one crew each. The driver does no item work: each
crew reads its item here, does the whole of it, writes a receipt under
`receipts/`, and commits. The driver records the outcome in `LEDGER.md`.

The order is the client's, not ours. Item 1 is first because the client is
building the consumer **now** against two claims that stopped being true when
`eb0b96c8c` landed.

## Item 1 — correct the two client documents for 0070

`doc/claude/feedback/ngspice_upstream/RESPONSE.md` and
`doc/claude/casemode-distinguish-guide.md` both predate `eb0b96c8c` and now
tell the client things this build does not do.

- The re-emitted pair is no longer spelled `Option: casemode = preserve`.
  Both writers close it up, and a load-and-write-again is byte-identical in
  that line. `RESPONSE.md:226-231`, `:553`; guide `:388-390`.
- A file that recorded *nothing* no longer picks up the re-writing session's
  mode on a copy. That was the defect. `RESPONSE.md:236-240`; guide `:412-419`.
- `RESPONSE.md:14`, `:158`, `:337` say round 2's changes are "not yet
  committed". They are: `9e341a8b7`, `4e738fc3e`.

The advice built on the first bullet — match the `Option:` **key**, not a line
number, and scan the whole header — stays correct and must survive the edit:
the two writers still put the line in two different places.

## Item 2 — move the durable answers into the guide

`RESPONSE.md` is round-scoped correspondence. The guide is what outlives it,
and today it answers none of questions 2, 3 or 4: zero occurrences of
`sim_status`, of `.save`, of "contract", of the collision warning.

Carry across, in the guide's own voice:

- **Q2** — `distinguish` keeping `.save` byte-exact is a permanent contract,
  from `doc/claude/decisions/0001-distinguish.md` decision 5's withdrawal list.
  A UI author can word the warning as permanent.
- **Q3** — the collision warning: all three modes, once per pair, at the
  parser's node symbol table so it sees `.include`d cards. With its silences,
  which under `fold` include a second spelling that appears only on an `X`
  instance line or inside a subckt body.
- **Q4** — `$sim_status` and `quit <n>` in place of `rc`, with the three
  properties (per analysis, last writer wins; absent before the first
  analysis; `0` is not "data was produced"), and the two-simulation deck shape
  that an analysis dot card plus a `.control run` produces.

## Item 3 — the upstream cp_remvar submission

`doc/claude/upstream/` holds the mail and two patches, validated against
`remotes/upstream/master` at `2f9c8ad47`. Nothing has been sent, and the
`casemodewrite` default flip is scheduled for "once 0067 has been in a
release", so this is the only thing that unblocks it.

The crew re-validates against today's upstream master and brings the text up
to date. **Sending the mail is the repo owner's action, not a crew's** — the
item ends with everything ready to paste and a receipt saying so.

## Item 4 — file the `-r` batch writer's missing header line

`ngspice -b -r out.raw deck.cir` writes its header in `fileInit()`
(`src/frontend/outitf.c`), which has no `Option:` arm at all, so the case mode
is absent from every file that path writes whatever `casemodewrite` says. It
is a documented caveat in `RESPONSE.md` and has no issue. File one in
`doc/codex/issues/`, to the house structure. Filing only, no fix.

## Item 5 — 0064's scope, for the owner to settle

`doc/codex/issues/0064` prescribes withholding the rename from the wildcard
tokens. Two measurements from the 2026-08-15 audit are not in it:

- the `-r` route is already clean — the same deck through `-r` writes
  `No. Variables: 1`. The client has not been told this.
- the extra column is not wildcard-specific: `write f.raw v(In)` on the same
  plot writes `0 v(in)` and `1 v(In)`. Any `write` whose arguments evaluate to
  exactly one vector gets the scale prepended.

So the client's real invariant — `No. Variables` equals what the deck saved —
is broken more widely than the issue says. The crew re-measures both, adds
them to 0064, and states the scope choice for the owner. **The decision is the
owner's**; the crew does not pick.

## Rules for every crew

- Branch `ver_50`. Read `CLAUDE.md` and `AGENTS.md` first.
- Behavioural change to `src/` is RED-first with the failure quoted. Items 1-5
  as written are documentation and issue work; if an item turns out to need
  code, stop and say so in the receipt rather than writing it.
- Measure before you write. Every number in a client-facing document is
  re-measured against `build-ver_50/src/ngspice`, not carried over.
- Never weaken a test to make something pass.
- Commit your own item, imperative summary, `Co-Authored-By: Claude Opus 5
  (1M context) <noreply@anthropic.com>`. Nothing else in the commit.
- Receipt at `receipts/<NN>-<slug>.md`: what changed, what was measured, what
  was left, and anything the next crew or the owner has to decide.
