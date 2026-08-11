# Issue: The Case Near-Miss Diagnostics Ship Without Deck Coverage

## Status

Open. Found by an adversarial review of `doc/codex/issues/0027`'s fixing
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

Not fixed for the three earlier diagnostics. `0027`'s is covered, decision 2's
paragraph is corrected in place with the mechanism spelled out, and
`doc/claude/decisions/0004-unlet-vector-identity.md` decision 3 records that
`756112c46` shipped the claim before it was tested.
