# Issue: The `gnd` Rewrite Runs Over Command Text It Should Not Touch

## Status

Fixed, all five criteria met. Criterion 4 turned out to be met already, by
`doc/codex/issues/0013`, and is recorded below rather than re-fixed.

This is the only defect in the case-sensitivity series that is **wrong in the
default mode with no `-D` flag**, which is what made it worth taking on its own
merits and separately from the rest of the feature.

## Summary

`inp_fix_gnd_name()` (`src/frontend/inpcom.c:2468` before the fix, `:2498`
after) walks every card of the deck and replaces a delimiter-bounded `gnd` with
` 0 `. Before the fix it skipped only cards whose first character is `*` and
cards inside a PSPICE subcircuit whose header names `gnd`. It had no
`.control`/`.endc` awareness and no quote awareness, and it runs at
`inpcom.c:1252`, before `inp_spsource()` extracts the control section at
`src/frontend/inp.c:711`.

Phase 1 converted the three scans from `strstr` to `cistrstr`, which is right
for a node name and wrong for the command text the reader has deliberately kept
case-preserved.

```spice
.control
op
echo GNDCHECK Gnd rail
echo GNDCHECK lower case: my gnd rail
.endc
```

```
$ ngspice --batch gnd.cir
GNDCHECK 0 rail
lower case: my 0 rail
```

Both halves reproduce **in the default mode with no `-D` flag**. The `Gnd` in an
`echo` argument became node 0; `GNDCHECK` itself is spared only because the
delimiter test requires whitespace, `(`, `)` or `,` on both sides. It is not
confined to `echo`: `shell echo "SH1 rail gnd here"` printed `SH1 rail 0 here`,
so the pass could change what a command with side effects actually did.

### The exclusions the function did have

There were **two** card exclusions before the fix, not one, and both are still
live:

- `*` at `:2475` (`:2505` after), unconditional. It also covers the `*#`
  commands, which is why the fix needs no separate test for them.
- the `newcompat.ps` block at `:2480-2484` (`:2510-2514` after), which sets
  `found_subckt` from a `.subckt` header that names `gnd` and clears it at
  `.ends`, so under PSpice mode a subcircuit may have a *port* called `gnd`.
  Measured on a deck whose `.subckt blk a b gnd` port is wired to a 0.5 V node:

  ```
  $ ngspice --batch ps.cir                   -> v(2) = 5.000000e-01   # gnd -> node 0
  $ ngspice -D ngbehavior=ps --batch ps.cir  -> v(2) = 7.500000e-01   # gnd stays a port
  ```

`nexttok()` is a third, positional exclusion: the first token of every line is
spared.

### The second exposure, and what it is not

The whole-line `inp_remove_ws()` at the end of the loop fired whenever *any*
`gnd` substring passed the card guard, even when no substitution was made, so a
line containing the letters g-n-d anywhere lost its whitespace runs and the
spaces around `=`.

This is the **narrower** of the two exposures, not the wider one, and an earlier
revision of this issue had it the wrong way round. Its only surface is `echo`
lines inside `.control`, because `inp_remove_excess_ws()` (`inpcom.c:4029`,
called from `:1205`, 47 lines before this pass) has already normalised every
other card, and its own skip list is exactly "`echo` inside `.control`"
(`:4045`). Measured on two `echo` lines identical except that one contains
`mygndtag`, where `gnd` is not delimiter-bounded and nothing is substituted:
only the `mygndtag` line lost its whitespace runs and its spaces around `=`.

The earlier claim that a `.lib` path with a `gnd` directory component "is still
whitespace-mangled by `inp_remove_ws()`" is therefore true of the deck and
**false of this function** — that card was mangled at `:1205` regardless.

Note also that the whitespace half is invisible to `tests/bin/check.sh`, which
diffs with `diff -B -w`. It has no regression deck and cannot have one in that
harness.

## Impact

Present in `fold` mode today, which is what distinguishes this issue from the
rest of the series. The lower-case half (`echo my gnd rail`) has been broken for
as long as the function has existed; Phase 1 extended it to `Gnd` and `GND`,
which is the half that reaches text the reader spared on purpose: the `write`,
`wrdata`, `codemodel`, `osdi`, `pre_osdi`, `echo`, `shell`, `source`, `cd`,
`load`, `setcs`, `strcmp` and `strstr` whitelist at `inpcom.c:1912-1923`, and
`.lib` cards.

Under `casemode=preserve` every card is case-preserved, so the exposure is the
whole deck rather than the whitelist.

No diagnostic is produced in any case. The failure surfaces as a shell variable
that does not exist, a file that cannot be opened, or an `echo` line that says
something else.

Set against that: the same `cistrstr` conversion is what makes `GND` and `Gnd`
work as ground aliases at all, which the specification requires in every mode
("Ground. `gnd`, `GND` and `Gnd` remain aliases for node `0` in every mode.
Ground is a reserved word, not an identifier",
`doc/claude/specs/case-sensitive-identifiers.md`). That behaviour is asserted by
`tests/regression/case/gnd-alias-case.cir`, which was verified to fail —
`Warning: singular matrix: check node mid`, `v(mid) = 0.000000e+00` — when the
substitution scan is reverted to `strstr`. The conversion is correct; its blast
radius was not.

## Root Cause

One pass was doing two jobs on two kinds of text. `inp_fix_gnd_name()` is a
netlist rewrite, but it was applied to the entire card list at a point where the
card list still contains control-language commands, library references and
quoted paths, and it had none of the exclusions that its neighbours have —
compare `inp_stripcomments_line()`, which skips `"` and `'` runs at
`inpcom.c:3720-3735`, and `inp_rem_unused_models()`, which tracks
`.control`/`.endc` nesting at `inpcom.c:9752-9763`.

Two further gaps in the same function were recorded here, and both have been
re-checked at HEAD:

- The PSPICE `.subckt` guard, `search_plain_identifier(c->line, "gnd")`, was
  described as `strstr`-based and therefore case-sensitive, citing
  `inpcom.c:5954`. **That citation is stale and the claim is no longer true.**
  The helper is now `search_plain_identifier_1()` at `inpcom.c:6103`, and the
  wrapper at `:6131` passes `ci = !inp_case_folding()`, so under `preserve` —
  the mode where it matters — the guard is case-insensitive. This is
  `doc/codex/issues/0013`'s work and it closes criterion 4 without a change
  here.
- A card that *ends* in `gnd` is never rewritten, because the delimiter test
  reads `gnd[3]` and the newline was zapped at `inpcom.c:1978`. Verified still
  true: `echo tail token is gnd` comes back unchanged while
  `echo paren form v(gnd)` became `v( 0 )`. That is what keeps the
  auto-inserted `.global gnd` intact, and it is also why a `.subckt` header
  ending in `GND` keeps a name that `.global gnd` will not match — see
  `doc/codex/issues/0010`. It is deliberately left alone.

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

Fixed as containment: a predicate over which cards and which spans of a card the
pass may look at. Nothing about what `gnd` matches changed, which is why this
needed no `distinguish` decision — ground is a reserved word, not an identifier,
and `gnd`/`GND`/`Gnd` alias node 0 in every mode by specification, so the
identity question has no jurisdiction here.

`doc/claude/suggestions/case-sensitive-identifiers-plan.md` §2.2 commits Phase 2
to touching `inpcom.c` control flow exactly once. The fix respects that: every
change is inside `inp_fix_gnd_name()` plus one new file-static helper next to
it. The caller at `inpcom.c:1252` is untouched.

**Criterion 1**, `inpcom.c:2519-2532`. `.control` nesting counted the way
`inp_rem_unused_models()` counts it, plus a `.lib`/`.inc` skip:

```c
        if (ciprefix(".control", gnd)) {
            skip_control++;
            continue;
        }
        else if (ciprefix(".endc", gnd)) {
            skip_control--;
            continue;
        }
        else if (skip_control > 0)
            continue;

        /* a library or include reference is a path, not a node list */
        if (ciprefix(".lib", gnd) || ciprefix(".inc", gnd))
            continue;
```

The whitelist needs no test of its own. It fires only when
`comfile || is_control || starhash` (`inpcom.c:1913`): `starhash` cards begin
with `*` and are already skipped, `is_control` is the block this now skips, and
`comfile` never reaches here at all — `inp_readall()` returns command files at
`inpcom.c:1162` before the pass runs.

**Criterion 2**, a new file-static `unquoted_hit()` at `inpcom.c:2470`, used by
all three scans:

```c
static char *unquoted_hit(char *str, const char *what, bool ci)
```

It walks the line tracking `"` and `'` the way `inp_stripcomments_line()` does
and returns only hits outside a quoted run. Resuming the scan past a hit starts
with a fresh quote state, which is correct precisely because a hit is never
returned from inside a quoted run. The two KiCad scans use it too, so the
criterion holds in every compatibility mode.

**Criterion 3**, a `replaced` flag set at each of the three substitution points
and tested at `inpcom.c:2589`:

```c
        if (replaced)
            c->line = inp_remove_ws(c->line);
```

**Criterion 4** was already met — see Root Cause. No change.

**Criterion 5**, `tests/regression/misc/gnd-command-text.cir`, and note the
directory. It is **not** in `tests/regression/case/`: everything there runs
under `-D casemode=preserve`, which is not the mode this defect lives in. The
deck asserts three lines, chosen to survive `check.sh`'s `egrep -v` filter:

```
ZAPMARK Gnd rail
ZAPMARK my gnd rail
v(2) = 3.000000e+00
```

RED before the fix was `ZAPMARK 0 rail` and `ZAPMARK my 0 rail`. `v(2)` is the
guard against passing by disabling the feature: `r2`'s second node really is
spelled `gnd`, so the netlist rewrite has to still fire.
`tests/regression/case/gnd-alias-case.cir` still passes.

### Fold-mode visibility

**This change is fold-mode visible, and that is the point of it.** The RED deck
carries no `-D` flag. Both the lower-case half and, through the reader's
case-preserving whitelist, the mixed-case half were wrong in the shipping
default.

Two behaviours were checked to be unchanged by it:

- PSpice `gnd` ports: 0.5 V stock and 0.75 V under `-D ngbehavior=ps`, the same
  pair as before the fix.
- The whitespace side effect is now gone from `echo` lines that contain `gnd`
  without a delimiter-bounded occurrence — they keep their spacing. That is
  criterion 3 working, and it is invisible to `check.sh`'s `diff -B -w`.

`make check` is 214 tests, 0 FAIL. The differential sweep does not move: no deck
under `tests/` has a `gnd` token in command text, so it was a regression guard
only for this change.
