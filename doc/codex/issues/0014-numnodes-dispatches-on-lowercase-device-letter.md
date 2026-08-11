# Issue: `numnodes()` Dispatches on a Lower-Case Device Letter

## Status

Fixed. Criteria 1 and 3 met in full; criterion 2 met for `E`, `G`, `W` and `K`,
with the `X` half of it argued to be unreachable rather than skipped — see
Resolution. Found while fixing `doc/codex/issues/0010`; deliberately left
unfixed there because it is a device-letter fold, not a name lookup.

## Summary

`numnodes()` in `src/frontend/subckt.c:1716` decides how many tokens on a card
are node names, and it classified the card by testing its first character
against a lower-case literal:

```c
static int
numnodes(const char* line, struct subs* subs)
{
    switch (*line) {
        case 'e':
        case 'g':
        case 'w':
            return 2;
        case 'k':
            return 0;
        case 'x':
        {
            const char* xname_e = skip_back_ws(strchr(line, '\0'), line);
            const char* xname = skip_back_non_ws(xname_e, line);
            for (; subs; subs = subs->su_next)
                if (eq_substr_id(xname, xname_e, subs->su_name))
                    return subs->su_numargs;
        }
    }
    return get_number_terminals((char *)line);
}
```

This is the same defect class as `doc/codex/issues/0009`, in a file that issue
did not touch. Issue 0009 fixed `src/frontend/inpcom.c` and
`src/frontend/numparam/spicenum.c`; its follow-up table enumerates 26 further
sites, all of them in `inpcom.c`. This one is not on that list.

The rest of `subckt.c` already gets it right: `translate()` copies the letter
into a local and folds the copy (`subckt.c:1202`), `translate_inst_name()`
(`:1159`), `finishLine()` (`:1561-1562`, `:1583`), `numdevs()` enumerates both
cases explicitly (`:1750-1772`), and `devmodtranslate()` folds a local
(`:1906-1908`). `numnodes()` was the only unfolded device-letter dispatch left
in the file.

The two call sites are `subckt.c:1390`, inside `translate()`'s
`case 'e'/'f'/'g'/'h'` arm, and `subckt.c:1480`, its `default:` arm.

## Impact

Under `fold` this is unreachable: the reader has lowercased the card.

Under `casemode=preserve` an upper-case device letter on a card *inside a
subcircuit body* skipped its `case` and fell through to
`get_number_terminals()` (`inpcom.c:5400`, dispatch at `:5409`), which is
already folded by issue 0009 and so returns a plausible but different number
rather than failing:

| letter | `numnodes()` | `get_number_terminals()` fallback |
| --- | ---: | ---: |
| `E`, `G` | 2 | 4 (`inpcom.c:5462-5463`, `:5466`) |
| `W` | 2 | 3 (`inpcom.c:5456`, `:5458`) |
| `K` | 0 | 2 (`inpcom.c:5413`, `:5419`) |
| `X` | `su_numargs` | token count − 2 (`inpcom.c:5440-5452`) |

It reproduces as a **hard abort**, not a wrong number, and the diagnostic
misdirects. `translate()` folds its own local copy at `:1202`, so it does enter
`case 'e'`; `numnodes()` then returns 4 instead of 2 and eats the control
nodes, `numdevs()` returns 2, and the failure surfaces one loop later as
`too few devs` rather than as a node-count error.

Minimal reproduction, everything lower case except the one device letter:

```spice
* upper-case E inside a subckt body
.OPTIONS noacct
v1 1 0 dc 1
x1 1 2 amp
.subckt amp a b
E1 b 0 a 0 2.0
.ends
.control
op
print v(2)
.endc
.end
```

```
$ ngspice --batch ecase.cir                        -> v(2) = 2.000000e+00
$ ngspice -D casemode=preserve --batch ecase.cir   -> Error: too few devs: E1 b 0 a 0 2.0
                                                      Error: incomplete or empty netlist
```

Lower-casing `E1` to `e1` makes the preserve run agree with the stock run, with
every other character of the deck untouched. `G`, `W` and `K` were each
reproduced the same way, one deck per arm:

| arm | upper under `preserve` | lower twin under `preserve` |
| --- | --- | --- |
| `E1 b 0 a 0 2.0` | `Error: too few devs: E1 b 0 a 0 2.0` | `v(2) = 2.000000e+00` |
| `G1 0 b a 0 2m` | `Error: too few devs: G1 0 b a 0 2m` | `v(2) = 2.000000e+00` |
| `W1 a b vs wmod` | `Error on line 9 or its substitute` | `v(2) = 3.998667e+00` |
| `K1 L1 L2 0.5` | `Error: too few devs: K1 L1 L2 0.5` | `v(2) = 2.154559e-02,-1.01531e-01` |

The `'x'` arm has a second consequence specific to `preserve`: with an
upper-case `X` the `case 'x'` was never entered, so the `su_numargs` lookup —
including the case-insensitive name comparison that `doc/codex/issues/0010`
added at `subckt.c:1731` — was dead code for exactly the decks it was written
for.

## Root Cause

Same as issue 0009: the reader's fold is load-bearing for card classification,
not only for identifier identity, and `subckt.c` was audited for name lookups
rather than for character dispatch.

## Acceptance Criteria

1. `numnodes()` folds the character it tests, in the `inppas2.c:92-94` style: a
   local copy, no mutation of the card.
2. A deck in `tests/regression/case/` with an upper-case `E`, `G`, `W` and `K`
   inside a subcircuit body produces the same voltages as its all-lower-case
   twin, and the `X` arm's `su_numargs` path is exercised by a nested
   parameterised call.
3. `make check` unchanged with `casemode` unset.

## Resolution

Fixed. Two lines.

```c
-    switch (*line) {
+    switch (elem_letter(line)) {
```

plus `#include "inpcom.h"` in `subckt.c`'s include block.

### Criterion 1, and why `elem_letter()` rather than a local `tolower_c()`

Both were on the table. `doc/codex/issues/0018` set the rule that
`elem_letter()` (`src/frontend/inpcom.c:930`) is *the* helper for a card's
leading device letter, and this is exactly that case: `numnodes()` is handed a
card and reads its first byte. The argument against was that `subckt.c` does not
include `inpcom.h`, and pulling a header into a large file is a cost.

That cost turned out not to exist. `src/frontend/inpcom.h` is 21 lines and four
declarations; it is not a large header. `subckt.c` already carries hand-written
`extern` declarations for `inpcom.c` symbols (`get_number_terminals()` at
`subckt.c:80`), so the include is if anything the tidier of the two, and
`elem_letter()` is `tolower_c(*line)` with a comment explaining why — the same
fold, named. Taken, and the local-fold alternative was not also applied
anywhere.

`elem_letter()` folds a copy and does not mutate the card, which is criterion 1
and the `inppas2.c:92-94` style it names. That precedent has not drifted.

### Criterion 2, including the part that cannot be tested

`tests/regression/case/numnodes-letter-case.cir` and its `-lower` twin put all
four reachable arms in one subcircuit body and assert three voltages:

```
v(2) = 2.000000e+00     the E arm
v(3) = 4.000000e+00     the G arm
v(4) = 1.997004e-03     the W arm
```

The `K` arm has no witness of its own by construction: if the two inductor
names are eaten as node names the coupling has nothing to refer to and the deck
aborts, so its evidence is that the other three voltages appear at all. RED was
an empty stdout — the upper deck aborted before printing anything, and the
diagnostic is on stderr, which `tests/bin/check.sh` does not capture. The `.out`
files of the two twins are byte identical.

**The `X` half of criterion 2 is not met and cannot be.** `get_number_terminals()`'s
`'x'` arm returns the card's token count minus two (`inpcom.c:5443-5452`), and
for any well-formed subcircuit call that equals `su_numargs`. The `case 'x'`
path in `numnodes()` is only entered at all when the last token of the card is
the subcircuit name — a call carrying `params:` or `name=value` ends in a
parameter, `skip_back_non_ws()` picks that up as `xname`, no `subs` entry
matches and the code falls through to the same fallback. So the two numbers
agree on every deck a user can write, whatever the letter's case. A nested
parameterised call — `X1 1 2 outer rtop=1k` into `X2 a b inner rval={rtop}` —
was written and measured **on a binary built without this fix**: `preserve`
with the upper-case `X` and `preserve` with the lower-case twin both give
`v(2) = 3.000000e+00`, and so does the stock run. There is nothing for a deck
to assert. What folding the letter does buy is structural — the `case 'x'` arm and with it issue 0010's
case-insensitive `eq_substr_id()` compare stop being dead code under `preserve`
— and that claim is recorded here rather than asserted by a deck that cannot
distinguish it.

### Criterion 3, and fold-mode visibility

**This change is not fold-mode visible.** Under `fold` the reader has already
lowercased the card, so `elem_letter()` is the identity on every byte it sees
and the `switch` takes the same arm it took before. `make check` with `casemode`
unset is 214 tests, 0 FAIL, with no committed `.out` touched. The differential
sweep does not move: no deck under `tests/` has an upper-case `E`, `G`, `W`,
`K` or `X` inside a `.subckt`.

Why it needs no `distinguish` decision: `numnodes()` reads no identifier. It
reads one byte of card *syntax* to decide how many tokens are node names, and
there is no dialect in which `E1` and `e1` are different device types. It is
not on the plan's Phase 3 gate list.
