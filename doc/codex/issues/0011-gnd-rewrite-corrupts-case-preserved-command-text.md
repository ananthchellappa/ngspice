# Issue: The `gnd` Rewrite Runs Over Command Text It Should Not Touch

## Status

Open

## Summary

`inp_fix_gnd_name()` (`src/frontend/inpcom.c:2374`) walks every card of the
deck and replaces a delimiter-bounded `gnd` with ` 0 `. It skips only cards
whose first character is `*`. It has no `.control`/`.endc` awareness and no
quote awareness, and it runs at `inpcom.c:1174`, before `inp_spsource()`
extracts the control section at `src/frontend/inp.c:711`.

Phase 1 converted the three scans from `strstr` to `cistrstr`
(`inpcom.c:2399`, `:2406`, `:2420`), which is right for a node name and wrong
for the command text the reader has deliberately kept case-preserved.

```spice
.control
op
echo GNDCHECK Gnd rail
.endc
```

```
$ ngspice --batch gnd.cir
GNDCHECK 0 rail
```

The `Gnd` in an `echo` argument became node 0. `GNDCHECK` survives only
because the delimiter test requires whitespace, `(`, `)` or `,` on both sides.

There is a second, wider exposure on the same function: the whole-line
`inp_remove_ws()` at `inpcom.c:2441` fires whenever *any* `gnd` substring
matched the guard at `:2399`, even when no substitution was made. A line
containing the letters g-n-d anywhere therefore has its whitespace runs
collapsed and the spaces around `=` deleted.

## Impact

Present in `fold` mode today. The lower-case half (`echo my gnd rail`) has been
broken for as long as the function has existed; Phase 1 extended it to `Gnd`
and `GND`, which is the half that reaches text the reader spared on purpose:
the `write`, `wrdata`, `codemodel`, `osdi`, `pre_osdi`, `echo`, `shell`,
`source`, `cd`, `load`, `setcs`, `strcmp` and `strstr` whitelist at
`inpcom.c:1832-1843`, and `.lib` cards.

Under `casemode=preserve` every card is case-preserved, so the exposure is the
whole deck rather than the whitelist. A `.lib` path containing a `gnd`
directory component survives the substitution (the `/` delimiters do not
match) but is still whitespace-mangled by `inp_remove_ws()`.

No diagnostic is produced in any case. The failure surfaces as a shell variable
that does not exist, a file that cannot be opened, or an `echo` line that says
something else.

Set against that: the same `cistrstr` conversion is what makes `GND` and `Gnd`
work as ground aliases at all, which the specification requires in every mode
("Ground. `gnd`, `GND` and `Gnd` remain aliases for node `0` in every mode.
Ground is a reserved word, not an identifier",
`doc/claude/specs/case-sensitive-identifiers.md`). That behaviour is asserted
by `tests/regression/case/gnd-alias-case.cir`, which was verified to fail —
`Warning: singular matrix: check node mid`, `v(mid) = 0.000000e+00` — when
`:2406` is reverted to `strstr`. The conversion is correct; its blast radius is
not.

## Root Cause

One pass is doing two jobs on two kinds of text. `inp_fix_gnd_name()` is a
netlist rewrite, but it is applied to the entire card list at a point where the
card list still contains control-language commands, library references and
quoted paths, and it has none of the exclusions that its neighbours have —
compare `inp_stripcomments_line()`, which does skip `"` and `'` runs at
`inpcom.c:3632-3645`, and `inp_rem_unused_models()`, which does track
`.control`/`.endc` nesting at `inpcom.c:9648-9660`.

Two further gaps in the same function, both pre-existing:

- `search_plain_identifier(c->line, "gnd")` at `inpcom.c:2392`, the PSPICE
  guard that stops `GND` on a `.subckt` line being shorted to 0, is `strstr`
  based (`inpcom.c:5954`) and so is case sensitive. Under `newcompat.ps` plus
  `preserve` the guard stops working exactly when it is needed.
- A card that *ends* in `gnd` is never rewritten, because the delimiter test
  reads `gnd[3]` and the newline was zapped at `inpcom.c:1889`. That is what
  keeps the auto-inserted `.global gnd` intact, and it is also why a `.subckt`
  header ending in `GND` keeps a name that `.global gnd` will not match — see
  `doc/codex/issues/0010`.

## Acceptance Criteria

1. `inp_fix_gnd_name()` skips cards between `.control` and `.endc`, skips the
   command whitelist and `*#`-prefixed commands, and skips `.lib`/`.inc` cards.
2. It does not rewrite text inside single or double quotes.
3. `inp_remove_ws()` runs only when a substitution was actually made.
4. `search_plain_identifier()` matches the identifier case insensitively, or
   `inpcom.c:2392` uses something that does.
5. A deck asserting `echo GNDCHECK Gnd rail` prints `GNDCHECK Gnd rail`, while
   `tests/regression/case/gnd-alias-case.cir` still passes.

## Resolution

Not fixed here. Found while writing the Phase 2 ground-alias guard. The
`cistrstr` conversions are Phase 1 work and are correct; the containment is a
separate change with its own regression decks, and it touches `inpcom.c`
control flow, which Phase 2 committed to touching exactly once.
