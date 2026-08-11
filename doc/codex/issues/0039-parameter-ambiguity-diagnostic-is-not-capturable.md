# Issue: The Built-Name Parameter Ambiguity Diagnostic Writes To `stderr`, So No Deck Can Guard It

## Status

Open. Found while closing `doc/codex/issues/0036`, which moved the other three
diagnostics of this family off `stderr` for exactly this reason. This is the
fourth and last site, and it belongs to `doc/codex/issues/0029` rather than to
`0036`, so it was not carried along.

## Summary

Under `casemode=distinguish` there are five near-miss diagnostics. Four are
written to `cp_err`, which a control-language `>&` redirect retargets
(`src/frontend/streams.c:156`), so a deck can capture them and assert on the
text. One is not:

| Diagnostic | Site | Stream |
| --- | --- | --- |
| `no vector named` | `src/frontend/vectors.c:467` | `cp_err` |
| `no node named` | `src/spicelib/parser/inpsymt.c:156` | `cp_err`, since `0036` |
| `no analog node named` | `src/xspice/evt/evtcheck_nodes.c:934` | `cp_err`, since `0036` |
| `no event node named` | `src/xspice/evt/evttermi.c:239` | `cp_err`, since `0036` |
| `parameter '%s' is a name ngspice built; '%s' and '%s' both differ from it only in case, so neither is used` | `src/frontend/numparam/xpressn.c:511` | **`stderr`** |

`tests/bin/check.sh` captures stdout only and its `egrep -v` filter drops
every line containing `Warning`, so a diagnostic on raw `stderr` cannot be
reached by any deck in this harness.

`tests/xspice/casedist/auto-bridge-vcc-param-ambiguous.cir` asserts the number
that goes with this warning — `3.300000e+00`, the documented fallback — and
says in its own comment that the line itself cannot be asserted. That is true
today and is true only because of the stream.

## Impact

No wrong numbers. It is `0036`'s shape: the diagnostic has a **silent**
counterpart — it must say nothing when only one spelling is present, or when
the two spellings are not both case variants of a name ngspice built — and a
change that made it fire on a correct deck would produce every correct number
and pass the whole suite. `0036`'s Impact section is the argument and it
transfers without change.

## Root Cause

`0029`'s commit wrote the diagnostic the way `0002`'s and `0003`'s were
written, and all three inherited `stderr` from the surrounding code rather
than from a decision. `doc/claude/decisions/0001-distinguish.md` decision 2
says only "the warning goes to `stderr`", which `cp_err` satisfies, since
`cp_err` is `stderr` in every binary that parses a deck.

## Acceptance Criteria

1. `xpressn.c:511` writes to `cp_err`. Verify that `cp_err` is non-NULL on
   that path, which runs inside the numparam pass rather than at the end of
   the parse.
2. A deck in `tests/xspice/casedist/` asserts the warning and at least one
   silence the diagnostic owes, using the shape of
   `tests/xspice/casedist/auto-bridge-node-case-report.cir`: write the circuit
   with `echo`, `source` it under `>&`, scan the capture with a `while` loop
   over `fread`, echo one upper-case token. The phrase scanned for must be
   distinguishing — `is a name ngspice built` — and not a tail shared with the
   other four.
3. Proved RED against a binary with the diagnostic's `fprintf` removed, and
   run against an empty `.out` first.
4. The false sentence in `auto-bridge-vcc-param-ambiguous.cir`'s comment —
   "no deck in this harness can assert on it" — is already corrected to name
   the stream as the reason; when this is fixed it should name the new deck.

## Resolution

Not fixed.
