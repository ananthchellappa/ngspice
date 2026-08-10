# Issue: `numnodes()` Dispatches on a Lower-Case Device Letter

## Status

Open. Found while fixing `doc/codex/issues/0010`; deliberately left unfixed
there because it is a device-letter fold, not a name lookup.

## Summary

`numnodes()` in `src/frontend/subckt.c` decides how many tokens on a card are
node names, and it classifies the card by testing its first character against a
lower-case literal:

```c
/* src/frontend/subckt.c:1673 */
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
into a local and folds the copy (`subckt.c:1179`, `char dev_type =
tolower_c(s[0]);`), `numdevs()` enumerates both cases explicitly
(`subckt.c:1748-1772`), and `devmodtranslate()` folds a local
(`subckt.c:1880-1882`). `numnodes()` is the only unfolded device-letter
dispatch left in the file.

## Impact

Under `fold` this is unreachable: the reader has lowercased the card.

Under `casemode=preserve` an upper-case device letter on a card *inside a
subcircuit body* skips its `case` and falls through to
`get_number_terminals()`, which returns a different number:

| letter | `numnodes()` | `get_number_terminals()` fallback |
| --- | ---: | ---: |
| `E`, `G` | 2 | 4 (`inpcom.c:5423-5429`) |
| `W` | 2 | 3 (`inpcom.c:5417-5421`) |
| `K` | 0 | 2 (`inpcom.c:5373-5382`) |
| `X` | `su_numargs` | token count − 2 (`inpcom.c:5403-5415`) |

`translate()` then rewrites the wrong number of tokens. For `E`/`G`/`W` the
run aborts; for `X` and `K` the two numbers usually agree, so the miss is
silent and only shows up on a card whose positional token count differs from
the subcircuit's formal count, or whose tail defeats the `params:`/`=`
heuristic in `get_number_terminals()`'s `'x'` arm.

The `'x'` arm has a second consequence specific to `preserve`: with an
upper-case `X` the `case 'x'` is never entered, so the `su_numargs` lookup —
including the case-insensitive name comparison that `doc/codex/issues/0010`
added at `subckt.c:1731` — is dead code for exactly the decks it was written
for.

Minimal reproduction, everything lower case except the one device letter:

```spice
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

Lower-casing `E1` to `e1` makes the preserve run agree with the stock run,
with every other character of the deck untouched.

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

Not fixed. Recorded during `doc/codex/issues/0010`, whose scope is the
`.model`, `.subckt` and `.global` **name** spaces; the leading device letter is
a separate axis with its own issue and its own follow-up table.
