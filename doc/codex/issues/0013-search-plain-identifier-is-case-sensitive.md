# Issue: `search_plain_identifier()` Matches Language Keywords Case-Sensitively

## Status

Fixed. Five of the six acceptance criteria are met; criterion 4's `X ...
PARAMS:` half is met as a guard rather than as a RED test, for the reason given
in the Resolution. See Resolution.

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

Fixed in one commit. The fold went **inside the helpers**, not at the call
sites, and it is gated on `inp_case_folding()` so the default mode runs the
same `strstr` on the same bytes as before.

```c
/* src/frontend/inpcom.c, next to the three searches */
static char *token_hit(char *str, const char *identifier, bool ci)
{
    return ci ? cistrstr(str, identifier) : strstr(str, identifier);
}
```

`search_identifier()` and `search_plain_identifier()` became thin wrappers over
a `_1` core that takes the `ci` flag, in the shape `inp_find_model_1()` and
`find_subckt_1()` already use in this file. Each gained a `_exact` sibling,
both `static`, for the call sites whose literal is a user name. The delimiter
tests are untouched.

### The three decisions the issue left open

**1. Inside the helpers or at the call sites? Inside.**

50 call sites pass a literal, and 47 of them are language keywords. Editing
each one would be 38 near-identical edits in a file that churns constantly
upstream, against three edits plus two new entry points here — and acceptance
criterion 2 asks for every site to be *reviewed* either way, which was done and
is tabulated below. For Phase 3 the separability that matters is the
identifier sites, not the keyword sites: under `distinguish` a keyword stays
case-insensitive, so the keyword path needs no further change, and the three
identifier sites are already isolated behind their own entry point.

**2. Is `ya_search_identifier()` reachable with an unfolded card, and should it
fold anyway? Reachable, and no.**

It has exactly one call site, `inp_quote_params()` (`inpcom.c:8745`), and the
chain `inp_spsource() -> inp_readall() -> inp_reorder_params() ->
inp_sort_params() -> inp_quote_params()` runs entirely inside `inp_readall()`,
so under `preserve` both sides arrive unfolded. But its `identifier` argument
is `deps[i].param_name`, the LHS of a user `.param` card — it is an identifier
search and never a keyword search. Folding it would not be the
`inpsymt.c:31` argument applied to a new name space; it would be a keyword fold
applied to a user name. Left byte-exact, with a comment saying why.

It also has a `static` forward declaration at `inpcom.c:5576` and a definition
with no storage class at `:6062`, and appears in no header. That is C11
6.2.2p4: the later `extern`-by-default declaration takes the visible internal
linkage, so it is file-static in fact, and `nm` finds no symbol for it in any
object in `build-ver_50`. Left alone; noted here so a future reader does not
mistake it for an export.

**3. Do the delimiter tests need anything? No, and it was checked rather than
assumed.**

`identifier_char()` (`inpcom.c:5964`) is `(c == '_') || isalnum_c(c)`;
`isalnum_c` and `isspace_c` are the `unsigned char` casts of the C library
predicates, and `is_arith_char()` (`src/misc/string.c:1055`) is a fixed set of
operator characters. None of the three can distinguish a letter from its other
case, so `is_X(c) == is_X(toupper(c))` for every `c` and the fence around the
hit is already case-blind. `cistrstr()` matches exactly `strlen(identifier)`
characters, so `search_plain_identifier`'s `str += strlen(identifier)` advance
stays correct and cannot skip a whole-token match that `strstr` would have
found.

### Every call site, reviewed

50 calls to the three helpers, in four files. Counted at the fix commit;
definitions and forward declarations excluded.

| File | keyword | user identifier |
| --- | ---: | ---: |
| `src/frontend/inpcom.c` — `search_plain_identifier` | 22 | 1 (`:5641`) |
| `src/frontend/inpcom.c` — `search_identifier` | 5 | 1 (`:8643`) |
| `src/frontend/inpcom.c` — `ya_search_identifier` | 0 | 1 (`:8745`) |
| `src/frontend/inpcompat.c` — `search_plain_identifier` | 17 | 0 |
| `src/frontend/inpc_probe.c` — `search_plain_identifier` | 1 | 0 |
| `src/frontend/inp.c` — `search_identifier` | 2 | 0 |
| **total** | **47** | **3** |

The three exceptions are all the same name space and all the same shape — a
user `.param` name used as a needle:

| Site | Needle | Function |
| --- | --- | --- |
| `inpcom.c:5641` | `deps[i].param_name` | `inp_sort_params()`, the parameter dependency graph |
| `inpcom.c:8643` | `f->funcname` | `inp_functionalise_identifier()`, the `agauss` `.param`-to-`.func` rewrite |
| `inpcom.c:8745` | `deps[i].param_name` | `inp_quote_params()`, brace quoting |

The issue named only the first. `inpcom.c:8643` is new: its `identifier`
argument reaches it from `inp_fix_agauss_in_param()`'s `funcs` list, whose
`funcname` is the LHS of a user `.param`. The `fcn` arguments at `inp.c:2595`
and `inpcom.c:8409` look like the same shape and are not — both trace to the
file-scope literal array `{"agauss", "gauss", "aunif", "unif", "limit"}`
(`inp.c:946`, `inpcom.c:1235`), so they are keywords.

All three are matched byte-exactly in **every** mode, rather than being routed
through the mode dispatch. Under `fold` that is what the reader already
guarantees. Under `preserve` it is a deliberate choice: numparam resolves the
same `.param` symbol byte-exactly (`doc/codex/issues/0015` is that gap), so a
case-insensitive quoter would hand numparam a spelling numparam cannot resolve
— `.param VAL=5` plus a card carrying a bare `val` would newly be rewritten to
`{val}` and then fail with `Undefined parameter [val]` on a deck that runs
today. The two have to move together; when 0015 lands, these three sites
become the mode dispatch and not before.

The 17 `inpcompat.c` sites were not enumerated anywhere in `doc/` before this.
All 17 are PSpice/LTspice keywords: `value`, `cur` (`:165-166`), `tc`
(`:934`), `t_abs`, `t_rel_global`, `t_measured` (`:1070-1074`) and the
`sidiode` arm's `d`, `roff`, `ron`, `rrev`, `vfwd`, `vrev`, `revepsilon`,
`epsilon`, `revilimit`, `ilimit`, `noiseless` (`:1809-1822`). Every one that
overwrites text in place writes exactly `strlen(literal)` bytes — `memcpy(t_str,
" temp", 5)`, the 9-byte `noiseless` blanking, `inpc_probe.c:115`'s 4-byte
`alli` blanking, `inpcom.c:9353`'s `acline + 2` — so a case-insensitive hit,
which matches exactly `strlen(literal)` characters, keeps all of them
length-correct.

Two of the 17 deserve a note rather than a change. `inpcompat.c:1809` searches
the single character `d` across a whole joined `.model` card; it is now
matched case-insensitively, so an isolated upper-case `D` token can hit it.
The blast radius is bounded by the parameter gate at `:1810-1818` and the
genuine type test `ciprefix("d", str)` at `:1831`, and the same over-match is
already reachable in the shipping default mode with the lower-case spelling.
`inpc_probe.c:115` searches `alli` over text that is otherwise user vector
names, so a vector spelled `ALLI` is now consumed as the keyword — which is
what `fold` mode already does for `alli`.

### The change IS fold-mode visible without the gate, and this was measured

Issue 0010's Resolution concluded that no fold-mode guard deck was possible.
That conclusion does **not** transfer, and acceptance criterion 5 was right to
say so. The reader's fold exemptions were walked again for these specific call
sites, all in `src/frontend/inpcom.c`, and three of them can deliver a live
card with upper-case text to one of these searches:

- **`.lib` / `.inc`, whole-card exempt at `:1915`.** Not reachable. Every
  `.include` card is commented out at `:1722` and every `.lib` reference at
  `:4151` before any pass runs; the contents are read by the same recursive
  `inp_read()` and folded identically. Under `ngbehavior=spice3` a live
  `.lib file SECTION` survives, but every one of these searches is guarded by
  a `.model`/`.subckt`/`.probe`/`VvIi`/device-letter prefix test that a `.lib`
  card cannot satisfy.
- **The 13-command `.control` whitelist at `:1916-1927`.** Reachable, once.
  `echo` begins with `e` and satisfies `replace_table()`'s `*cut_line == 'e'`
  guard (`inpcompat.c:163`), and `replace_table()` is called at
  `inpcompat.c:682` before `pspice_compat()` installs its `skip_control`
  counter. The `r`/`l`/`c` site at `inpcompat.c:934` *is* control-guarded.
- **The `print`/`eprint`/`eprvcd`/`asciiplot` redirection tail at
  `:1884-1894`.** Reachable. The arm stops folding at the first `>` whether or
  not the card is a command, and `Eprvcd 7 0 vol='v(1) > 2 ? V(2) : 0'` is a
  legal VCVS named `prvcd` whose first byte folds to `e`.
- **The `plot`/`gnuplot`/`hardcopy` arm at `:1820-1883`.** Reachable, the same
  way: `Gnuplot 4 0 xlabel VALUE 1.0` is a legal VCCS named `nuplot`, and the
  token after `xlabel` is spared at `:1856`.
- **`keep_case_of_cider_param()` at `:227`.** Off in this build
  (`build-ver_50/src/include/ngspice/config.h:11` is `/* #undef CIDER */`), but
  `line_contains_icfile()` matches *any* card containing `ic.file`, including a
  `d`/`m`/`q`/`x` instance card, so an `--enable-cider` build can put
  upper-case text into `get_number_terminals()`.
- **`is_xspice_model()` at `:420`.** Reachable, and this is the one that was
  proved end to end. XSPICE is on by default; any `.model` card whose text
  contains `filesource`, `table2d`, `table3d`, `d_state`, `d_source`,
  `d_process` or `d_cosim` is diverted to `keep_case_of_cider_param()`, which
  preserves everything between exactly two `"` characters.

So the change is fold-mode visible *if the fold is ungated*, and
`tests/regression/misc/keyword-fold-guard.cir` pins it. That deck carries

```spice
.model pmodv vdmos (vto=-1 kp=1 mfg="d_state PCHAN")
```

`d_state` diverts it to `keep_case_of_cider_param()`, `PCHAN` survives the
fold inside the quotes, and `inp_vdmos_model()`'s
`search_plain_identifier(cut_line, "pchan")` does not match it, so the model is
`vdmosn` and `v(3)` is `-6.92888e-01`. The gate was removed experimentally and
the tree rebuilt: the same deck becomes `vdmosp` and prints `-1.24988e-03`.
**It was the only failure in the whole suite** — 113 tests, 1 FAIL — which is
the reason the deck is worth committing: without it an ungated version of this
change would have looked clean in `make check`.

The deck pins the case policy, not the over-match it rides on. A quoted
annotation deciding a model's channel type is a separate defect, and the
lower-case spelling triggers it today in every mode.

### Evidence

Six twin pairs under `tests/regression/case/`, which runs every deck with
`-D casemode=preserve`, plus the fold-mode guard above. Each upper-case deck
has a lower-case twin with a byte-identical `.out`, so the assertion is "the
spelling cannot change the number" rather than "this run printed this". The
references were checked against the `fold`-mode run of both twins before being
committed.

| Deck | RED before |
| --- | --- |
| `ac-value-case.cir` | no output; `Error on line 3 or its substitute: IIN 1 0 AC / parameter value out of range or the wrong type` |
| `diode-off-case.cir` | no output; `warning, can't find model 'OFF'`, then `could not find a valid modelname` |
| `mos-off-case.cir` | as above, on `M1 2 3 0 0 nmod OFF` |
| `bjt-off-case.cir` | as above, on `Q1 3 2 0 qmod OFF` |
| `vdmos-pchan-case.cir` | `v(3) = -6.92888e-01` against an expected `-1.24988e-03` |
| `subckt-params-case.cir` | not RED; see below |
| `misc/keyword-fold-guard.cir` | not RED at this commit; RED with the gate removed |

`vdmos-pchan-case.cir` is the one that matters and the only one
`tests/bin/check.sh` could have caught unaided: its failure is a number.
`check.sh:20` filters `Error` and `Warning` out of both sides, so the other
four were confirmed to be RED with an empty `.out` first — all four *passed*
against an empty reference, because after filtering their entire output is
diagnostics. That is the harness hazard this directory keeps re-teaching.

The per-mechanism split was necessary for the same reason it was on issues 0009
and 0010: a single combined deck aborts on the first `could not find a valid
modelname` and hides the rest.

Two decks the issue asked for could not be written as specified. `M ...
TNODEOUT` is rejected by a level-1 MOSFET in *every* mode, so it has no
lower-case twin that runs; `off` exercises the same literal list in the same
arm and is used instead. The Q arm's `save` and `print` searches are inside
`#ifdef CIDER`, which is off in `build-ver_50`, so they are folded without
being covered and no coverage is claimed.

### Criterion 4's `X ... PARAMS:` half is a guard, not a RED test

`subckt-params-case.cir` cannot be made RED, and the reason is worth recording
rather than working around. The `x` arm's terminal count has four consumers and
none of them can turn it into a wrong number:

- `numnodes()` (`src/frontend/subckt.c:1717`) resolves an `x` card against the
  `.subckt` definition's own `su_numargs` and only falls through to
  `get_number_terminals()` for an undefined subcircuit, which fails anyway;
- `get_subckts_for_subckt()` (`inpcom.c:3355`) and
  `comment_out_unused_subckt_models()` (`:3444`) take the `'x'` arm and call
  `get_instance_subckt()`, never `get_model_name()`;
- `inp_rem_unused_models()` skips `'x'` outright;
- `inp_quote_params()` (`:8727`) is the only real consumer, and the *wrong*
  count made it start its brace-quoting scan one token later, which is strictly
  more conservative.

The deck therefore asserts that correcting the count does not move a number,
which it does not. What that last consumer leaves behind is
`doc/codex/issues/0017`.

### Acceptance criteria

1. **Met.** Both `search_identifier()` and `search_plain_identifier()` match
   case-insensitively via `cistrstr()`, gated on `inp_case_folding()`, with the
   delimiter checks unchanged. `ya_search_identifier()` deliberately does not;
   see decision 2.
2. **Met.** All 50 call sites reviewed and tabulated above. `inpcom.c:5637` is
   one of three exceptions, not one of one, and all three got a separate
   exact-match entry point.
3. **Met.** `tests/regression/case/ac-value-case.cir` and its twin.
4. **Met for `D`, `M` and `Q`; met as a guard for `X`.** The `X` half cannot be
   RED; the argument is above and the residue is `doc/codex/issues/0017`.
   `vdmos-pchan-case.cir` was added beyond what the criterion asks, because it
   is the one deck in the set whose failure is a wrong number.
5. **Met, and the argument had to be redone rather than inherited.** `make
   check` is 114 tests, 0 FAIL with `casemode` unset, against a baseline of
   101; the 13 decks added here account for the difference. The change is a
   no-op in `fold` mode by construction of the gate, *not* by the exemption
   argument — three exemptions can deliver an unfolded card to these searches,
   and `tests/regression/misc/keyword-fold-guard.cir` proves it.
6. **Met.** `tests/polezero/pz2.cir` and `tests/polezero/pzt.cir` no longer
   `PARSE-FAIL`, `PARSE-FAIL` is 0 across the whole sweep, and `NUM-DIFF` stays
   0.

### Differential sweep

`python3 doc/claude/scripts/case_differential_sweep.py --jobs 12 --timeout 90`,
measured on this tree immediately before and after the commit:

| verdict | before | after |
| --- | ---: | ---: |
| OK | 95 | 108 |
| DIFF | 42 | 43 |
| PARSE-FAIL | 2 | 0 |
| NUM-DIFF | 0 | 0 |
| SKIP | 3 | 3 |
| **total** | **142** | **154** |

Compared per deck rather than on the totals: **no deck's verdict got worse, and
no deck that was OK became anything else.** Exactly three decks moved.
`tests/polezero/pz{2,t}.cir` went `PARSE-FAIL` to `DIFF`, and
`tests/regression/model/binning-1.cir` went `DIFF` to `OK`. The 12 extra decks
are the twin pairs added here; all 12 report OK.

The two `polezero` decks land in `DIFF` rather than `OK` for a defect that is
neither new nor numeric: their `l1 1 0 1H` cards carry an upper-case unit
suffix, and `is_a_modelname()`'s test at `inpcom.c:3239` is case-sensitive, so
under `preserve` `1H` is reported as `warning, can't find model '1h'`. That is
already on issue 0009's follow-up list, it is a warning only, and the rawfile
numerics are identical. The three `SKIP`s are the unchanged stock-run timeouts.

The before column differs by one deck from the figure recorded after issue 0010
(OK=96, DIFF=41). Both were produced by the same command at the same commit;
that is the run-to-run variation in one timing-sensitive deck which the sweep
already documents.

### Found while fixing this, deliberately not fixed

- `doc/codex/issues/0017` — `inp_quote_params()` (`inpcom.c:8735`) adjusts the
  terminal count on a lower-case-only `strchr("fhmouydqjzswx", *curr_line)`.
  Issue 0009's follow-up class and that table's stated highest priority. RED:
  `.param nmod=7` with an upper-case `M1 2 3 0 0 nmod` fails to parse under
  `preserve` and runs under `fold`. It is pre-existing — that deck's count is
  not touched by this change — but this fix re-aims it: an upper-case
  `X1 1 2 divider PARAMS: ...` used to have two bugs cancel, and now reaches it.
- Several keyword sites stay unreachable for a fully upper-case deck even with
  this fix, because their enclosing branch tests a lower-case first character:
  `inp.c:2259` `strchr("*vbiegfh", curr_line[0])` and `inp.c:2592`
  `*curr_line != 'b'`; `inpcom.c:6909` `*curr_line == 'f'` and `:6956`
  `*curr_line == 'h'`; `inpcom.c:6385` `*curr_line == 'e'` and `:6665`
  `*curr_line == 'g'`, which gate the four `value`/`table` searches;
  `inpcompat.c:164` `*cut_line == 'e' || *cut_line == 'g'` and `:923`
  `*cut_line == 'r' || 'l' || 'c'`, which gate three of that file's 17. All are
  issue 0009's follow-up class. `inpcom.c:7671` already spells the same test
  `strchr("*vbiegfhVBIEGFH", curr_line[0])`, which is the pattern the rest of
  them need.
- `src/frontend/inpc_probe.c:21` re-declares `search_plain_identifier()`, which
  `src/frontend/inpcom.h:14` already declares. Dead duplication; left alone to
  keep the diff to the searches.
- `src/frontend/numparam/xpressn.c:1565` already implements this exact gate
  inline (`inp_case_folding() ? strstr(...) : cistrstr(...)`). It was not
  refactored onto the new helper, because it is a different delimiter rule in a
  different subsystem.
