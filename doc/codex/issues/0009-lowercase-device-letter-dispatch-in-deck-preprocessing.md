# Issue: Deck Preprocessing Dispatches on Lower-Case Device Letters

## Status

Fixed for the enumerated sites; see Resolution.  Acceptance criterion 4
(`PARSE-FAIL` = 0 in the differential sweep) is not met and cannot be met
within this issue: the residue is `doc/codex/issues/0010` and
`doc/codex/issues/0013`.

## Summary

Several deck-preprocessing passes in `src/frontend/inpcom.c` decide what a card
is by testing its first character against a lower-case literal. They are
correct only because `inp_read()` has already lowercased the card. The four
sites found so far:

```c
/* src/frontend/inpcom.c:5357, get_number_terminals() */
    switch (*c) {
        case 'r': case 'c': case 'l': case 'k': case 'f':
        case 'h': case 'b': case 'v': case 'i':
            return 2;
        case 'd':  ...
```

```c
/* src/frontend/inpcom.c:9704, inp_rem_unused_models() */
        switch (*curr_line) {
            case '*': case '.': case 'v': case 'i': case 'b': case 'x':
            case 'e': case 'h': case 'g': case 'f': case 'k': case 't':
                continue;
```

```c
/* src/frontend/inpcom.c:3313, get_subckts_for_subckt() */
/* src/frontend/inpcom.c:3399, comment_out_unused_subckt_models() */
            if (*line == 'x') {
                char *subckt_name = get_instance_subckt(line);
                nlist_adjoin(used_subckts, subckt_name);
            }
            else if (*line == 'a') {
```

```c
/* src/frontend/inpcom.c:961, inp_get_w_l_x() */
        if (*curr_line != 'x' || (!newcompat.hs && !newcompat.spe) || ...
```

The same family appears just downstream: `inp_rem_unused_models()` also tests
`*curr_line == 'a'` and `*curr_line != m->elemb` against a lower-case element
identifier from `inp_get_elem_ident()`.

This is precisely the enumeration gap the Phase 1 census wrote down and left
open: "Character-literal comparisons are not counted... a scan for
`== '<lowercase>'` returns thousands of hits, almost all of them parser
bookkeeping"
(`doc/claude/checklists/phase1-keyword-case-census.md`). These are the ones
that are not bookkeeping.

## Impact

With the reader folding, none of this is reachable, so `fold` mode is
unaffected and `make check` cannot see it.

With `casemode=preserve` (`doc/claude/suggestions/case-sensitive-identifiers-plan.md`
Phase 2) it is the dominant failure mode of the feature. An upper-case device
letter is the normal SPICE house style, so most real decks hit it. Two
mechanisms:

1. `get_number_terminals()` returns 0 for `R3`, `D1`, `Q1`, `P1` and the rest,
   so `inp_rem_unused_models()` never records the model reference on that card,
   and `rem_unused_xxx()` comments the `.model` card out. The instance then
   fails to parse.
2. `get_subckts_for_subckt()` and `comment_out_unused_subckt_models()` do not
   see `X1` as a subcircuit instance, so the `.subckt` body is commented out
   and `inp_subcktexpand()` later reports the instance as undefined.

Neither is a wrong-numbers failure: both abort the run with a diagnostic. But
the diagnostic names the model or subcircuit, not the device letter, so the
cause is not discoverable from the message.

Measured on this tree with the differential sweep
(`doc/claude/scripts/case_differential_sweep.py`): of 129 `.cir` decks under
`tests/`, 82 fail to parse under `preserve` or under `preserve` on a
mechanically uppercased copy, while the stock run parses. Every failure
inspected reduces to one of the two mechanisms above.

Minimal reproduction, entirely lower case except the device letter:

```spice
* semiconductor resistor, upper case device letter
V1 1 0 dc 1
R3 1 2 rmodel1 l=11u w=2u
R9 2 0 1k
.model rmodel1 r rsh=1000 narrow=1u
.control
op
print v(2)
.endc
.end
```

```
$ ngspice --batch min.cir                        -> v(2) = 0.000000e+00
$ ngspice -D casemode=preserve --batch min.cir   -> Error on line 3 or its substitute:
                                                     R3 1 2 rmodel1 l=11u w=2u
                                                     unknown parameter (rmodel1)
```

Lower-casing `R3` to `r3` makes the preserve run agree with the stock run, with
every other character of the deck untouched.

The subcircuit half reproduces on a deck already in the tree:
`tests/regression/case/case-flag-noop.cir`, uppercased, gives
`Error: unknown subckt: X1 MID OUT DIVIDER`.

## Root Cause

The reader's fold is load-bearing for card classification, not only for
identifier identity. `src/spicelib/parser/inppas2.c:92-94` gets this right — it
copies the leading letter into a local and upper-cases the copy for dispatch,
mutating nothing — and `src/frontend/subckt.c:1136` uses `tolower_c(*name)`.
The `inpcom.c` preprocessing passes predate or ignore that convention.

The specification asserts the opposite of what the code does:
"Device letters. `R1` and `r1` dispatch to the same device type in every mode.
Already non-destructive (`src/spicelib/parser/inppas2.c:92-94`)"
(`doc/claude/specs/case-sensitive-identifiers.md`). That claim is true of the
parser and false of the preprocessor.

## Acceptance Criteria

1. Every first-character device-letter test in `src/frontend/inpcom.c` folds
   the character it tests, in the `inppas2.c` style: a local copy, no mutation
   of the card. `get_number_terminals()`, `inp_rem_unused_models()`,
   `get_subckts_for_subckt()`, `comment_out_unused_subckt_models()` and
   `inp_get_w_l_x()` at minimum.
2. `inp_get_elem_ident()`'s result and the `m->elemb` comparison agree on case.
3. A deck in `tests/regression/case/` with upper-case device letters on an R
   with a semiconductor model, a D, a Q and an X resolves all four and produces
   the same voltages as its all-lower-case twin.
4. The differential sweep reports no `PARSE-FAIL` for any deck whose stock run
   parses.
5. `make check` unchanged with `casemode` unset.

## Resolution

Fixed for the sites enumerated above, in two commits.

`src/frontend/inpcom.c` gained one static helper:

```c
static char elem_letter(const char *line)
{
    return line ? tolower_c(*line) : '\0';
}
```

It folds a copy and mutates nothing, the way `src/spicelib/parser/inppas2.c:92-94`
already does it for the parser's own dispatch. Every first-character
device-letter test in the affected passes now goes through it:
`inp_get_w_l_x()`, `get_model_name()`, both `strchr("*vibefghkt", *line)` skip
lists, the `'x'`/`'a'` tests in `get_subckts_for_subckt()` and
`comment_out_unused_subckt_models()`, the `switch` in `get_number_terminals()`,
and the `switch` plus the `'a'`, `'n'` and `m->elemb` comparisons in
`inp_rem_unused_models()`. The two `strchr` skip lists were not in the original
site list above; they are the same defect and are folded for the same reason.

`inp_get_elem_ident()` needed no change. It already folds its input with
`cieq`/`ciprefix` and always returns a lower-case letter, so acceptance
criterion 2 is met by folding the card side of the `m->elemb` comparison alone.
`get_adevice_model_name()` does no character-level dispatch — it takes the last
whitespace-delimited token. `is_a_modelname()` does, but only under
`newcompat.lt`; see the follow-up list.

One site outside `src/frontend/inpcom.c` had to be fixed for the subcircuit
half to work, and it is not in the site list above:

```c
/* src/frontend/numparam/spicenum.c:264, transform() */
    } else if (s[0] == 'x') {
```

That is numparam's line categorizer. Category `'X'` is what drives
`nupa_subcktcall()` and its `dicostack_push()`; the matching `.ends` is
categorized `'U'` and pops unconditionally. With an upper-case `X1` the push is
missing and the pop is not, so once the `.subckt` body stops being commented
out the deck dies one card later with `Subckt Stack underflow`. Folded the same
way, with `tolower_c(s[0])`.

Neither change is fold-mode visible. In the default mode the reader has already
lowercased the card, so the fold is the identity, and there is no path by which
an upper-case ordinary device card reaches these passes: the reader's fold
exemptions (`.lib`/`.inc`, the `.control` command whitelist, the print
redirection tail, the CIDER and XSPICE `.model` arms) all cover dot cards and
control lines rather than device cards.

### Evidence

`tests/regression/case/device-letter-model-case.cir` — upper-case `R` with a
semiconductor `.model`, upper-case `D`, upper-case `Q`; RED before the
`inpcom.c` commit with `unknown parameter (rmod)`, `could not find a valid
modelname` twice.

`tests/regression/case/device-letter-case.cir` — the criterion-3 deck, all
four of `R`/`D`/`Q`/`X` in one deck; RED before the `spicenum.c` commit with
`Subckt Stack underflow`.

Both carry an all-lower-case twin with a byte-identical `.out`, which is the
actual assertion: the leading letter's case cannot change a number. Model and
subcircuit names are lower case in both decks, because name lookup is still
`strcmp` — that is `doc/codex/issues/0010` and is a separate question.

`tests/regression/misc/device-letter-case.cir` is the fold-mode guard. It
cannot be RED, for the reason given above; it pins that the fold direction
stays "to lower case".

Differential sweep, `doc/claude/scripts/case_differential_sweep.py --jobs 12
--timeout 90`, before and after:

| verdict | before | after |
| --- | ---: | ---: |
| OK | 20 | 67 |
| DIFF | 24 | 39 |
| PARSE-FAIL | 82 | 25 |
| NUM-DIFF | 0 | 0 |
| SKIP | 3 | 3 |
| **total** | **129** | **134** |

No deck's verdict got worse — the comparison was made per deck, not on the
totals — and `NUM-DIFF` stays 0. The deck count rises by the five decks this
issue added, all of which report OK. The three SKIPs are unchanged: they are
`tests/mesa/mesa-12.cir`, `tests/mesa/mesa12.cir` and `tests/vbic/FG.cir`,
whose *stock* run exceeds the 90 s timeout, so the sweep has nothing to compare
against.

15 of the 57 decks that stopped failing landed in DIFF rather than OK. None of
those differences is numeric. `tests/general/mosamp.cir` is the one that looks
numeric — `Reference value : 1.36501e-06` against `1.33460e-06` — and is not:
that line is a timing measurement and differs between two consecutive *stock*
runs by more than it differs between modes. `tests/general/schmitt.cir` gains
`warning, can't find model '5pf'`, which is the `inpcom.c:3206` unit-suffix site
in the follow-up table. The rest are the sweep's own uppercasing artifacts
already documented in `doc/claude/checklists/phase2-differential-sweep.md`
— control-language variables defined outside quotes and referenced inside them,
and plot names generated in lower case by the analysis.

### Acceptance criterion 4 is not met, and cannot be within this issue

Criterion 4 asks for `PARSE-FAIL` to reach 0. It stands at 25, and none of the
25 is this defect:

- **20 decks** — `tests/bsim2/test.cir`, `tests/bsim3soi{dd,fd,pd}/*`,
  `tests/mos6/simpleinv.cir` — reference a model in one case and define it in
  another *inside the deck itself*: `MP10 ... p12l5` against `.MODEL P12L5
  PMOS`, `m1 d g s e n1` against `.Model N1 NMOS` in an `.include`d file. Only
  the reader's fold ever made those equal. That is `doc/codex/issues/0010`,
  model and subcircuit name lookup by `strcmp`.
- **3 decks** — `tests/regression/lib-processing/ex{1a,2a,3a}.cir` — fail only
  on the sweep's mechanically uppercased copy, because the deck is uppercased
  and the `.lib` file it pulls the subcircuit from is not. Same issue 0010,
  plus a limitation of the sweep's uppercasing rule.
- **2 decks** — `tests/polezero/pz{2,t}.cir` — fail on `IIN 1 0 AC` with
  `parameter value out of range or the wrong type`. `search_plain_identifier()`
  is `strstr`-based, so the `ac`-with-no-value fixup at `inpcom.c:9272` does not
  fire on `AC`. That is a *keyword* fold, not a device-letter fold; written up
  as `doc/codex/issues/0013`.

Criteria 1, 2, 3 and 5 are met. `make check` is 93 tests, 0 FAIL, against a
baseline of 88 with `casemode` unset; the three added decks and the two twins
account for the difference.

## Follow-up: first-character device-letter tests deliberately left alone

Same defect class, each needing its own deck under its own compatibility mode,
so each is its own change. Line numbers are against commit `e96b4cd5c`, before
the fix. None is covered by the commits above, and none is exercised by the
differential sweep today except where noted.

**Enumerated in the original scope of this issue.** All in
`src/frontend/inpcom.c`.

| Line | Function | Test | Deck needed |
| --- | --- | --- | --- |
| 2794 | `replace_freq()` | `pt = (*line == 'e') ? 'v' : 'i';` | `#ifdef XSPICE`; an AC deck with an upper-case `E1 out 0 FREQ {v(in)} = db ...` table source against its lower-case twin, evidence `vdb(out)`. Upper case silently picks the wrong controlling quantity. |
| 2873 | `inp_chk_for_e_source_to_xspice()` | `*line == 'e' && inp_chk_for_multi_in_vcvs(...)` | `#ifdef XSPICE`, no dialect; a multi-input VCVS, `E1 out 0 nand(2) in1 0 in2 0 (...) (...)`, evidence `v(out)`. |
| 2875 | `inp_chk_for_e_source_to_xspice()` | `*line != 'e' && *line != 'g'` | as 2873, plus the `G`-source twin. |
| 3121 | `is_a_modelname()` | `newcompat.lt && *line == 'r'` | `-D ngbehavior=lt -D casemode=preserve`, upper-case `R1 1 2 4k7` — an RKM value must not be mistaken for a model name. |
| 3127 | `is_a_modelname()` | `newcompat.lt && *line == 'c'` | as 3121 with `C1 2 0 4u7`. |
| 3133 | `is_a_modelname()` | `newcompat.lt && *line == 'l'` | as 3121 with `L1 1 3 4m7`. |
| 3206 | `is_a_modelname()` | `(*st == 'f') \|\| (*st == 'h')` | Not a device letter — the unit suffix inside a value token. `1H` or `5pF` written upper case is not recognised as a number and is reported as a missing model. Already observable in the sweep as `warning, can't find model '1H'` on `tests/polezero/pz2.cir` and `'5pf'` on `tests/general/schmitt.cir`; warning only, no numeric effect. |
| 5041 | `inp_fix_param_values()` | `*line == 'b'` | Ungated; upper-case `B1 out 0 i = v(in)/2k` against its lower-case twin, evidence `v(out)`. B-source expressions must be exempt from parameter quoting. |
| 6289 | `inp_compat()` | `*curr_line == 'e'` | Ungated (needs only *not* `ngbehavior=s3`); `E1 out 0 VOL={v(in)*2}`, evidence `v(out) = 2.0`. |
| 6569 | `inp_compat()` | `*curr_line == 'g'` | as 6289 with `G1 out 0 cur={v(in)*1m}`. |
| 6813 | `inp_compat()` | `*curr_line == 'f'` | as 6289 with a CCCS carrying a `temper` expression and `.options temp=75`. |
| 6860 | `inp_compat()` | `*curr_line == 'h'` | as 6289 with the CCVS form. |
| 6911 | `inp_compat()` | `*curr_line == 'r'` | as 6289 with a behavioural resistor `R1 1 0 R = {1k/V(2)}`, evidence `i(V1)`. |
| 6993 | `inp_compat()` | `*curr_line == 'c'` | as 6289 with a behavioural capacitor `C = {...}`. |
| 7093 | `inp_compat()` | `*curr_line == 'l'` | as 6289 with a behavioural inductor `L={1m*(1+v(ctrl))}`, evidence a `.tran` branch current. |
| 7152 | `inp_compat()` | `*curr_line == 'k'` | as 6289 with a three-inductor `K1 l1 l2 l3 0.5`, evidence two AC node voltages. |
| 7509 | `inp_bsource_compat()` | `*curr_line == 'b'` | Ungated; an upper-case `B` source whose expression needs the compat rewrite, paired with its lower-case twin. |
| 8951 | `inp_meas_current()` | `*s == 'v'` | Not a card's first character — the device letter inside `i(...)`. `i(V1)` inside a B-source expression must not be converted to a current probe. Needs a deck with `b1 out 0 v = 1k*i(V.x1.vs)`. |

**Not enumerated in the original scope, found while fixing this issue.** Same
class, same file, and the anchor list missed them:

| Line | Function | Test | Note |
| --- | --- | --- | --- |
| 8639 | `inp_quote_params()` | `strchr("fhmouydqjzswx", *curr_line)` | **Highest priority of this group.** It *adds one* to the `get_number_terminals()` result that this issue just made correct, so an upper-case `F`/`H`/`M`/`Q`/`S`/`X` card now gets a terminal count that is off by one and the brace-quoting loop starts one token early — it can wrap the model name or the controlling source name. Not observed to change a number in the sweep, which is not the same thing as safe. |
| 4331 | `inp_fix_subckt_multiplier()` | `strchr("*vehaknopstuwy", curr_line[0])` | A third skip list of exactly the shape of the two at 3296/3373. Under `preserve` an upper-case card inside a subcircuit is no longer skipped and gets ` m={m}` appended. |
| 4338 | `inp_fix_subckt_multiplier()` | `curr_line[0] == 'b'` | Guards the "skip a voltage-mode B source" case, so an upper-case `B1 n1 n2 V={...}` gets ` m={m}` appended to a voltage source. |
| 8806 | `inp_vdmos_model()` | `curr_line[0] == 'm' && cistrstr(...)` | Thermal VDMOS instance detection: an upper-case `M1 d g s tj tc mymod thermal` skips both the five-node syntax check and the instance model lookup. |
| 8943 | `inp_meas_current()` | `*v == 'a' && s[-1] == '%'` | `v` is the head of the card, so this is a first-character test. An upper-case XSPICE `A1 %i(node) ...` loses the `%i(` escape and the port is rewritten as a current probe. |
| 9056 | `inp_meas_current()` | `(tok[0] == 'e') \|\| (tok[0] == 'h')` | `tok` is the card's first token. An upper-case linear `E1`/`H1` referenced by `i(E1)` no longer takes the undo path. |
| 9902, 9946 | `inp_poly_2g6_compat()` | two separate `switch (*thisline)` with `'e'`, `'f'`, `'g'`, `'h'` | Structurally identical to 9704, which *is* fixed. An upper-case SPICE2 `POLY` source never gets `poly(1)` inserted. Both switches need folding: the second is unreachable today only because the first `continue`s first. |

**Same `preserve` class, not a first-character test.** Recorded so the next
sweep over this file does not have to rediscover them; they belong with
`doc/codex/issues/0013` rather than here.

| Line | Function | Test |
| --- | --- | --- |
| 2711 | `replace_freq()` | the "is the expression a simple identifier" scan accepts `'a'..'z'` only, so `FREQ {Vin}` with an upper-case node breaks out early |
| 4713 | `inp_do_macro_param_replace()` | `strchr("vi", p[-1])` — an upper-case `V(` is not recognised, so a node name inside `V(n1,n2)` can be substituted as a `.func` formal |
| 5940 | `b_transformation_wanted()` | `strpbrk(p, "vith")` is a lower-case-only scan set feeding comparisons that are themselves `cieqn` |
| 8748 | `inp_vdmos_model()` | `cut_line[5] == 'p'` tested case-sensitively right after `cieqn(cut_line, "vdmos", 5)` matched case-insensitively, so `.model m1 VDMOSP` loses its channel type |

Three sites in the same family are already written correctly and are listed
here only so a future sweep does not "fix" them twice: `inpcom.c:7575`
(`strchr("*vbiegfhVBIEGFH", curr_line[0])`), `inpcom.c:9242`
(`strchr("VvIi", *cut_line)`) and `inpcom.c:3545` (`*string == 'x' || *string
== 'X'`). The CIDER `ignore_line()` switch at `inpcom.c:334-358` already lists
each device letter in both cases.

### Method note on this enumeration

The table above was produced by three independent sweeps of the file — a direct
scan for first-character comparisons, a walk of the call chains from the five
named entry points, and an adversarial search for a `fold`-mode path that could
reach these passes with an upper-case card — followed by per-site annotation
and a completeness critic run against the combined list. The critic is what
found 8943, 9056 and the second `inp_poly_2g6_compat()` switch, and what
correctly rejected 3206 and 8951 from the original scope list as not being
card-first-character tests. Two claims from that pass were checked by hand
against `e96b4cd5c` before being written down here, because the working tree
was 15 lines longer than `HEAD` while it ran and several agents reported
working-tree line numbers.
