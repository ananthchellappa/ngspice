# Plan: the open items after the xschem follow-up

Driver session, 2026-08-15, branch `ver_50`, HEAD at start `16a0b3156`.
Predecessor batch: `doc/claude/batches/2026-08-15-xschem-followup/`, whose
five receipts are the evidence base for everything below. Read the receipt
named in your item before you measure anything yourself.

Six items, in this order, one crew each. The driver does no item work.

The owner has answered the four decisions the last batch left open. Their
answers are the scope of this one:

- **0064** — fix the wildcard rename only. The duplicate-column mechanism is
  filed for the record and not fixed.
- **0071** — the `-r` writer gets the line, behind the same gate.
- Also: correct 0069's broken example, file the `dotcards.c` abort, give
  0064's second mechanism its own number, and write the round-3 client reply.
- **The upstream mail is not sent and the owner is sending it.** No crew
  touches `doc/claude/upstream/`, and no document in this batch may say the
  submission has gone out.

## Item 1 — fix the wildcard rename (0064, narrow scope)

`ft_evaluate()` (`src/frontend/evaluate.c:80-84`) renames its result to the
parse node's own text when the result is a single vector (`!d->v_link2`).
Right for an expression — `print v(a)+v(b)` should be labelled `v(a)+v(b)` —
and wrong for a wildcard, whose text names nothing. So a deck that saves one
signal and writes it gets a vector called `v(all)`, which reaches the client's
signal browser as a fake net.

Fix: withhold the rename when the pnode's text is one of the wildcard tokens.
`get_all_type()` (`src/frontend/vectors.c:105`) is the existing list; decide
whether `alle` belongs and say why in the commit.

Scope discipline: `com_write()`'s scale prepend is **out of scope**. It gets
its own issue in item 5. Receipt 05 of the predecessor batch separates the two
mechanisms with the decks that isolate each; read it first.

## Item 2 — the `-r` writer records the case mode (0071)

`fileInit()` (`src/frontend/outitf.c`) writes the batch path's header and has
no `Option:` arm, so `ngspice -b -r out.raw deck.cir` carries no case mode
whatever `casemodewrite` says. Receipt 04 establishes that timing is **not**
the cause — the `.control` block completes before `fileInit()` runs — and that
`run <file>` reaches the same writer.

Fix: the same gated line, in the same place relative to `Plotname:`, valued
from `inp_case_mode_name()`, so that a consumer's one branch works on both
writers. Gate unset must leave the header byte-identical to what it writes
today. 0071's own acceptance criteria and its note on which test directory can
assert on a `-r` file are the specification.

## Item 3 — correct 0069's Resolution

`doc/codex/issues/0069`'s Resolution prints a guard deck with no netlist lines
in it and names three decks (`guard_both.cir`, `guard_absent.cir`,
`ctl_noop.cir`) that exist nowhere in the tree. The guide now carries a
complete, runnable version of the same guard, measured and re-extracted.
Correct the issue so its Resolution is reproducible from the repo.

## Item 4 — file the `.op`-with-no-netlist abort

Found by the previous batch while running 0069's listing verbatim: `.op` with
no circuit loaded aborts on an assertion at `src/frontend/dotcards.c:225`,
rc=134, identically on stock `ngspice-46`. Pre-existing, upstream, unfiled.
File it to the house structure. **Filing only, no fix.**

## Item 5 — file 0064's second mechanism

`com_write()` prepends the plot's scale whenever the argument list evaluates
to exactly one vector, which is a different defect from the rename and
survives item 1's fix. Receipt 05 has the isolating decks, including the row
that matters most to the client: under `preserve`, `distinguish` and stock,
the duplicate pair carries a **byte-identical name**, so filter-by-name does
not reach it. File it. **Filing only, no fix.** Cross-reference 0064.

## Item 6 — the round-3 client reply

Last, because it reports everything above. The client's round 2 is
`doc/claude/feedback/reply_from_xschem_session/REPLY.md`; our round-2 answer
is `doc/claude/feedback/ngspice_upstream/RESPONSE.md`, which item 1 of the
previous batch has already corrected for 0070. Write round 3 in the same
place and the same voice: what moved, what was wrong in round 2 and is now
corrected, what is filed and not fixed, and what still waits on an upstream
release. Say plainly that the upstream submission has **not** been sent.

## Rules for every crew

- Branch `ver_50`. Read `CLAUDE.md` and `AGENTS.md` first.
- Items 1 and 2 change behaviour: **RED first**. Write the test, run it, quote
  the failure in the commit message, then make the smallest change that turns
  it green. Never weaken an assertion or regenerate an `.out` to hide a
  failure.
- Items 1 and 2 each run the targeted directory's `make check` and then the
  full `make check` from `build-ver_50`, and report both counts in the receipt.
  Re-run `./autogen.sh` after adding a test.
- The identity lint is part of `make check`. A new `strcmp`-family comparison
  of a deck-written name needs the annotation `tests/bin/identity_lint.sh`
  requires; if `identity.baseline` changes, justify it in the commit message.
- Items 3, 4 and 5 are documentation. If one turns out to need code, stop and
  say so in the receipt rather than writing it.
- Items 4 and 5 each take the next free number in `doc/codex/issues/`. 0071 is
  taken. Check what exists when you start; the crews run one at a time, so the
  number you find free is free.
- Measure before you write. Every number in an issue or a client-facing
  document is re-measured against `build-ver_50/src/ngspice`. Scratch files go
  under `/tmp/claude-1000/-home-qflow-dev-ngspice-test/`, never in the repo.
- Nothing in this batch is sent anywhere. No mail, no push, no PR.
- Commit your own item, imperative summary, `Co-Authored-By: Claude Opus 5
  (1M context) <noreply@anthropic.com>`. Nothing unrelated in the commit.
- Receipt at `receipts/<NN>-<slug>.md`: what changed, what you measured with
  the actual output, the RED failure if your item had one, what you left, and
  anything the next crew or the owner must decide.
