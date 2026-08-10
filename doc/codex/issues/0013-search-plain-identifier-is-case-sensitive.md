# Issue: `search_plain_identifier()` Matches Language Keywords Case-Sensitively

## Status

Open

## Summary

`search_plain_identifier()` (`src/frontend/inpcom.c:6025`) finds a whole-token
occurrence of a literal in a card. It is delimiter-aware — it checks the
characters either side of the hit — but the hit itself is `strstr`:

```c
/* src/frontend/inpcom.c:6025-6047 */
char *search_plain_identifier(char *str, const char *identifier)
{
    if (str && identifier && *identifier != '\0') {
        char *str_begin = str;
        while ((str = strstr(str, identifier)) != NULL) {
            ...
```

Every one of its twenty-odd call sites passes a lower-case *language keyword*,
not an identifier: `ac`, `off`, `thermal`, `params:`, `tnodeout`, `save`,
`print`, `value`, `table`, `pchan`, `alli`, `gnd`, `mfg`, `icrating`, `vceo`,
`type`. These match only because `inp_read()` lowercased the card first. With
`casemode=preserve` the fold is gated off and every one of them stops matching
on a deck written in upper case.

This is the keyword half of the defect whose device-letter half is
`doc/codex/issues/0009`. It is not the same fix: 0009 folds a single leading
character, this needs a case-insensitive whole-token search.

## Impact

Two classes, both live under `casemode=preserve`.

**A hard parse failure.** `inp_check_syntax()` rewrites a source card that says
`ac` with no value into `ac ( 1 0 )`:

```c
/* src/frontend/inpcom.c:9272 */
            acline = search_plain_identifier(acline, "ac");
            if (acline == NULL)
                continue;
```

With `AC` the fixup never fires and the card reaches `INPdevParse()`, which
matches the `ac` keyword case-insensitively via `cieq` and then calls
`INPgetValue()` on an empty tail:

```
$ ngspice --batch pz2-UPPER.cir
Note: iin: has no value, DC 0 assumed         <- parses

$ ngspice -D casemode=preserve --batch pz2-UPPER.cir
Error on line 2 or its substitute:
  IIN 1 0 AC
parameter value out of range or the wrong type
    Simulation interrupted due to error!
```

Reproduces on `tests/polezero/pz2.cir` and `tests/polezero/pzt.cir` uppercased.
These are 2 of the 25 decks still reported `PARSE-FAIL` by
`doc/claude/scripts/case_differential_sweep.py` after issue 0009 was fixed.

**Silently wrong terminal counts.** Seven of the calls are inside
`get_number_terminals()` itself — the function issue 0009 just taught to
recognise an upper-case device letter:

```
inpcom.c:5387   search_plain_identifier(c, "thermal")          D, PSpice self-heating
inpcom.c:5393   search_plain_identifier(inst, "off")           D
inpcom.c:5394   search_plain_identifier(inst, "thermal")       D
inpcom.c:5408   search_plain_identifier(inst, "params:")       X
inpcom.c:5443   search_plain_identifier(inst, "off")           M
inpcom.c:5445   search_plain_identifier(inst, "tnodeout")      M
inpcom.c:5446   search_plain_identifier(inst, "thermal")       M
inpcom.c:5485   search_plain_identifier(name[i], "off")        Q
inpcom.c:5489   search_plain_identifier(name[i], "save")       Q, #ifdef CIDER
inpcom.c:5491   search_plain_identifier(name[i], "print")      Q, #ifdef CIDER
```

Each of these terminates a token scan. `D1 1 2 dmod OFF`, `M1 d g s b nmod
TNODEOUT`, `X1 a b sub PARAMS: w=1u` and `Q1 c b e qmod OFF` therefore count
one terminal too many under `preserve`, which is the same failure surface issue
0009 was about — a wrong `num_terminals` feeds `get_model_name()`, which then
picks the wrong token as the model name.

No such deck is in `tests/`, so the sweep does not currently show this; the
absence of evidence here is an absence of coverage, not an absence of defect.

`is_a_modelname()` produces a third, cosmetic class: an unrecognised
upper-case unit suffix is reported as a missing model. `warning, can't find
model '1H'` on `tests/polezero/pz2.cir` and `'5pf'` on
`tests/general/schmitt.cir` are that. Those come from the separate
`(*st == 'f') || (*st == 'h')` test at `inpcom.c:3206`, listed in issue 0009's
follow-up table.

## Root Cause

`search_plain_identifier()` was written for a world in which the reader had
already destroyed case, so `strstr` against a lower-case literal was a
case-insensitive keyword search by construction. It is the same load-bearing
assumption recorded in `doc/claude/code_analysis/case-insensitivity-origins.md`
and the same enumeration gap: the Phase 1 census counted
`strcmp`/`strncmp`/`strstr`/`eq(` against lower-case literals, but
`search_plain_identifier()` hides its `strstr` behind a helper, so a scan for
the call sites finds a function name rather than a string literal.

Note that the callers *around* these sites are already correct:
`inpcom.c:9242` guards the `ac` fixup with `strchr("VvIi", *cut_line)` and
`inpcom.c:7575` uses `strchr("*vbiegfhVBIEGFH", curr_line[0])`. The device
letter was thought about; the keyword was not.

## Acceptance Criteria

1. `search_plain_identifier()` matches its literal case-insensitively, using
   `cistrstr()` rather than `strstr()`, with the delimiter check unchanged.
   `cistrstr()` already exists in `src/misc/string.c` and is the convention
   named in `AGENTS.md`.
2. Every call site is reviewed for whether its literal is a language keyword
   (convert) or a user identifier (leave). All twenty-odd present literals look
   like keywords; `inpcom.c:5623`, which passes a caller-supplied `param`
   string, is the one that is not and needs a separate decision.
3. A deck in `tests/regression/case/` with `IIN 1 0 AC` and no value produces
   the same node voltages as its `ac` twin.
4. A deck in `tests/regression/case/` per affected terminal count — at minimum
   `D ... OFF`, `M ... TNODEOUT`, `X ... PARAMS:` and `Q ... OFF` — resolves its
   model and produces the same voltages as its lower-case twin. These are the
   decks issue 0009's fix cannot be trusted without.
5. `make check` unchanged with `casemode` unset. The change is a no-op in
   `fold` mode for ordinary cards, by the same argument as issue 0009: the
   reader has already lowercased them. It is *not* a guaranteed no-op on the
   lines the reader exempts from folding (`.lib`/`.inc` at `inpcom.c:1826`, the
   `.control` whitelist, the print-redirection tail), and `search_plain_identifier`
   is called on `.subckt` and `.probe` cards; that has to be reasoned about
   explicitly rather than assumed, exactly as
   `doc/claude/suggestions/case-sensitive-identifiers-plan.md` section 1.2 says.
6. The differential sweep shows `tests/polezero/pz2.cir` and
   `tests/polezero/pzt.cir` no longer `PARSE-FAIL`, and `NUM-DIFF` stays 0.

## Resolution

Not fixed. Found while closing `doc/codex/issues/0009`, whose scope was the
device-letter dispatch and explicitly not the keyword matching. It is Phase 1
work by the taxonomy in
`doc/claude/suggestions/case-sensitive-identifiers-plan.md`, and it belongs
with the rest of section 1.1 rather than with the `casemode` plumbing.

It should be closed before `preserve` is offered to anyone: the terminal-count
class produces a wrong model-name token with no diagnostic that names the
cause, which is the same property that made issue 0009 hard to find.
