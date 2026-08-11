# Issue: The Case Near-Miss Diagnostics Ship Without Deck Coverage

## Status

Closed on `ver_50`, 2026-08-11. Found by an adversarial review of `doc/codex/issues/0027`'s fixing
commits, which asserted that no deck in this harness can assert on a
diagnostic. That assertion was false, and it had been inherited from
`doc/claude/decisions/0001-distinguish.md` decision 2, where it stood
uncorrected through three sessions.

`0027`'s own diagnostic is now covered by
`tests/regression/casedist/vector-unlet-report.cir`. The three that shipped
earlier are not.

## Summary

Decision 2 of `0001-distinguish.md` said, of the near-miss warning:

> `tests/bin/check.sh:29` captures stdout only and its `egrep -v` filter drops
> any line containing `Warning` from both sides anyway, so no deck in this
> harness can assert on it either way.

The first clause is true and the conclusion does not follow. The control
language redirects `cp_err` along with `cp_out` when a redirect carries `&`
(`src/frontend/streams.c:112-158`, `cp_err = fp` at `:156`), so a diagnostic
can be captured to a file and read back with `fopen`/`fread`/`strstr` and
reduced to a token the filter keeps. `tests/regression/pipe/shell-keyword-case.cmd:79`
was already using that shape for `history` output it had to take off disk.

Three shipped diagnostics are therefore assertable and unasserted:

| Diagnostic | Site | Record |
| --- | --- | --- |
| a node created by a reference that no card defines, whose name differs only in case from one that is defined | `INPtermCaseCheck()`, `src/spicelib/parser/inpsymt.c:155` | `doc/claude/decisions/0002-deferred-node-resolution-check.md` |
| an event node that matched no analog node except by case | `report_bridge_case_miss()`, `src/xspice/evt/evtcheck_nodes.c` | `doc/claude/decisions/0003-event-node-near-miss.md` |
| an event node that nothing drives when a driven node differs from it only in case | `EVTnode_case_check()`, `src/xspice/evt/evttermi.c` | `doc/claude/decisions/0003-event-node-near-miss.md` |

`findvec()`'s own near-miss warning (`src/frontend/vectors.c:234`) is the
fourth, and it is the one whose *false positive* is already filed separately as
`doc/codex/issues/0034`.

## Impact

No wrong numbers. This is a coverage gap, and its shape is what makes it worth
filing rather than shrugging at: each of these diagnostics has a **silent**
counterpart that decision 2 requires — silence on a *definition* — and silence
is exactly what a deck cannot notice by accident. A change that made any of
them fire on a definition would produce every correct number, write to a stream
no deck compares, and pass the whole suite. That is the failure the `0030`
session hit while writing one of these very diagnostics: its first version
fired on a correct deck, and only a hand review caught it.

The `.cir` decks that exercise these three paths already exist —
`tests/regression/case*/bsource-forward-ref.cir`,
`tests/xspice/casedist/auto-bridge-node-case-split.cir`,
`tests/xspice/casedist/event-node-case-split.cir` — and each currently asserts
only the number. Each needs the capture-and-read-back block added, or a sibling
deck that carries it.

## Root Cause

The `>&` form of the redirect is documented in the manual but is not used by any
deck outside `tests/regression/pipe/`, and the pipe directory does not compare
a `.out` at all, so the two halves of the technique — capture `cp_err`, and
assert on what you read back — had never been combined in a `check.sh`
directory. Decision 2's paragraph then made the untried conclusion load-bearing,
and two later sessions cited it rather than testing it.

## Acceptance Criteria

1. Each of the three diagnostics above is asserted by a deck in the directory
   that already covers its numbers, using the pattern in
   `tests/regression/casedist/vector-unlet-report.cir`: preseed the variable,
   redirect with `>&`, `fopen`/`fread`/`fclose`, `strstr` against the
   distinguishing phrase, and echo one upper-case token.
2. Each such deck also asserts the **silence** its record requires, in the same
   mode and the same run. A deck that only asserts the warning covers the half
   that was never in danger.
3. Each new deck is confirmed RED against a binary with the diagnostic reverted,
   not only GREEN against the current one — a token that is asserted but
   unreachable is worse than no assertion.
4. Any capture file gets a `CLEANFILES` entry in its directory's
   `Makefile.am`, as `tests/regression/casedist/Makefile.am` now has for
   `vuc_*.txt`.

## Resolution

Fixed on `ver_50` in three commits, one per diagnostic, plus one for the
documentation. `doc/claude/decisions/0006-diagnostic-deck-coverage.md` is the
record.

**The Summary above is itself half wrong, and that is the finding.** `>&`
retargets the *variable* `cp_err`, so it reaches a diagnostic only if the
diagnostic writes to `cp_err`. `vec_warn_case_near_miss()` does, which is why
`0027`'s deck worked; all three diagnostics named here used a literal
`fprintf(stderr, ...)` and were uncapturable as shipped. Measured at
`8557994bb`: a wrapper deck sourcing the circuit under `>&` captured the
sourced parse's `cp_out` — the `Circuit:` line, 22 bytes — and nothing else.
Each is now written to `cp_err`, which is `stderr` in every binary that parses
a deck (`src/main.c:916`, `src/sharedspice.c:921`, `src/tclspice.c:2486`), so
no byte moves on any existing deck in any mode; the change is one
six-character token per file with no line movement.

None of the three is a command, so there is nothing in their own decks to
redirect. Each new deck writes the circuits it is about with `echo` and
`source`s them from inside its own `.control` block: `source` re-enters
`inp_spsource()` and `if_inpdeck()`, so the whole parse runs again with the
wrapper's redirect in force, and the sourced circuit stays current so the
warning and the number are asserted in one run.

| Criterion | Where |
| --- | --- |
| 1, the warning | `tests/regression/casedist/bsource-node-case-report.cir`, `tests/xspice/casedist/auto-bridge-node-case-report.cir`, `tests/xspice/casedist/event-node-case-report.cir` — each scanning for its own noun, never the shared tail, and the two XSPICE report cases also asserting the other noun's absence |
| 2, the silence | four cases in the parser deck (miss, definition, forward reference claimed later with a case variant defined, miss with no variant) and three in each XSPICE deck (miss, plus `already_joined()`'s hand-written bridge and an exactly matched bridge; miss, plus a dangling node with no variant and an auto-bridged input node beside a driven case variant) |
| 3, RED | each deck proved RED twice: against `8557994bb`, and against a binary with its own diagnostic's `fprintf` deleted, restored afterwards and checked with `md5sum -c`. Each RED is a one-line `make check` diff |
| 4, `CLEANFILES` | `bnr_*` added to `tests/regression/casedist/Makefile.am`; `tests/xspice/casedist/Makefile.am` gains a `CLEANFILES` line it did not have, for `abr_*` and `enr_*` |

`make check` is **264 PASS, 0 FAIL**, up from 261 by exactly these three
decks. The differential sweep on a binary built at `8557994bb` and on the
binary after this work produced **byte identical logs** — 305 decks, DIFF=59,
OK=243, SKIP=3, PARSE-FAIL=0, NUM-DIFF=0 — so no deck moves. The three new
decks are among the `DIFF`s: their uppercased copies do not run, which is
`doc/codex/issues/0038`'s asymmetry seen from the sweep's side and the same
collateral `vector-rawfile-scale-case.cir` has.

`doc/codex/issues/0038` was found by this work: the fold-mode exemption list
for control lines is per command, so a file written by `echo` cannot be read
by `fopen` under a name containing upper case. It is wrong in the default
mode and is filed rather than fixed.
