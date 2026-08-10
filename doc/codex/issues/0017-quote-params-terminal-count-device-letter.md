# Issue: `inp_quote_params()` Adjusts the Terminal Count on a Lower-Case Device Letter

## Status

Fixed in `f2111b152`. All four acceptance criteria are met; see Resolution.

## Summary

`inp_quote_params()` decides where a device card's parameter references begin
by counting terminals and then adding one for the device letters whose next
token is a model name, a controlling source name or a subcircuit name:

```c
/* src/frontend/inpcom.c:8727-8737 */
        num_terminals = get_number_terminals(curr_line);

        if (num_terminals <= 0)
            continue;

        /* There are devices that should not get quotes around token directly
           following the terminals. These may be model names, control voltages
           or subckt names. See bugs 384, 730 or Skywater issue 327 */
        if (strchr("fhmouydqjzswx", *curr_line))
            num_terminals++;
```

The scan set is lower case only. `get_number_terminals()` itself was taught to
fold its leading letter by `doc/codex/issues/0009` (`elem_letter()`), and this
consumer of its result was not — it is the entry `doc/codex/issues/0009`'s
follow-up table calls "**Highest priority of this group**", listed there as
`inpcom.c:8639` against `e96b4cd5c`. It is `:8668` at `33da7d1fe` and `:8735`
after the `doc/codex/issues/0013` fix.

## Impact

Under the default `fold` mode the reader has already lowercased the card, so
the scan set matches and nothing is reachable.

Under `casemode=preserve` an upper-case `F`, `H`, `M`, `O`, `U`, `Y`, `D`, `Q`,
`J`, `Z`, `S`, `W` or `X` card — the ordinary SPICE house style — does not get
the `+1`. The brace-quoting loop then starts one token early and can wrap the
token the `+1` exists to protect. A deck that declares a `.param` whose name
collides with that token fails to parse:

```spice
.OPTIONS noacct
.param nmod=7
V1 1 0 dc 5
V2 3 0 dc 5
R1 1 2 10k
M1 2 3 0 0 nmod
.model nmod nmos level=1 vto=1 kp=2e-5
.control
op
print v(2)
.endc
.end
```

```
$ ngspice --batch m.cir                        -> v(2) = 3.432236e+00
$ ngspice -D casemode=preserve --batch m.cir   -> Error on line 7 or its substitute:
                                                  Error: incomplete or empty netlist
```

Lower-casing `M1` alone makes the `preserve` run agree with the stock run. The
model name has been rewritten to `{nmod}`.

The `x` arm is the one that interacts with `doc/codex/issues/0013`, and the
interaction is worth recording because it looks like a regression and is not.
Before 0013, an upper-case `X1 1 2 divider PARAMS: rtop=1k` counted one
terminal too many, because `search_plain_identifier(inst, "params:")` missed
`PARAMS:` and the scan ran on to `rtop=1k`. That extra terminal happened to
cancel this missing `+1` exactly, so the scan started at the right token. With
0013's keyword fold the count is correct and the cancellation is gone, so this
defect becomes reachable for that card shape too:

```
Error: unknown subckt: X1 1 2 {divider} rtop=1k
```

on a deck carrying `.param divider=7`. The `M` reproduction above is
independent of 0013 — its card has no `off`/`=` token, so 0013 does not change
its terminal count — which establishes that this is a pre-existing defect that
0013 re-aims rather than one 0013 introduces.

No such deck is in `tests/`. The shape needs a `.param` whose name equals a
model, controlling-source or subcircuit name, which is unusual but legal.

## Root Cause

The same one as `doc/codex/issues/0009`: card classification by a lower-case
character literal, correct only because `inp_read()` folded first. 0009 fixed
the producer of this number and left its consumers to a follow-up table; this
is that table's first row.

## Acceptance Criteria

1. `inpcom.c:8735` folds the character it tests, in the `elem_letter()` style
   0009 established — a local copy, no mutation of the card.
2. A deck in `tests/regression/case/` with an upper-case `M` card and a
   `.param` named after its model resolves the model and produces the same
   voltage as its lower-case twin.
3. The `X`-card twin of the same shape, with a `.param` named after the
   subcircuit, likewise.
4. `make check` unchanged with `casemode` unset, and the differential sweep
   shows no deck's verdict getting worse.

## Resolution

Fixed in `f2111b152`, in one token, as predicted:

```c
/* src/frontend/inpcom.c:8737, inp_quote_params() */
        if (strchr("fhmouydqjzswx", elem_letter(curr_line)))
            num_terminals++;
```

### Acceptance criteria

1. **Met.** `inpcom.c:8737` folds the character it tests through
   `elem_letter()`, which copies and mutates nothing.
2. **Met.** `tests/regression/case/quote-params-model-case.cir`, with
   `.param nmod=7` and an upper-case `M1 2 3 0 0 nmod`, resolves the model and
   prints `v(2) = 3.432236e+00`, byte-identical to its lower-case twin.
3. **Met.** `tests/regression/case/quote-params-subckt-case.cir` is the `X`
   twin, with `.param divider=7` and `X1 1 2 divider PARAMS: rtop=1k`, and
   prints `v(2) = 3.000000e+00`.
4. **Met.** `make check` went from 114 tests, 0 FAIL to 118 tests, 0 FAIL with
   `casemode` unset — the four decks this issue adds. The differential sweep
   was measured across the whole series that closes
   `doc/codex/issues/0009`'s follow-up table rather than on this commit alone;
   see `doc/claude/checklists/phase2-differential-sweep.md`. No deck's verdict
   got worse.

### Evidence

RED at `cf3edd306`, reproduced exactly as the Impact section predicts:

```
$ ngspice --batch quote-params-model-case.cir
  v(2) = 3.432236e+00
$ ngspice -D casemode=preserve --batch quote-params-model-case.cir
  M1 2 3 0 0    7.000000000000000e+00
  could not find a valid modelname
  Error: incomplete or empty netlist
```

The `X` deck fails with `Error: unknown subckt: X1 1 2 {divider} rtop=1k`,
which is the cancellation `doc/codex/issues/0013` removed.

Both decks were run against an empty `.out` first and both FAILED, so their
evidence is a number rather than a diagnostic — the check
`tests/bin/check.sh`'s filter makes necessary and which this directory keeps
re-teaching.

### Not fold-mode visible, and the argument was made rather than inherited

`inp_quote_params()` reads only `curr_line[0]`; it skips `.` and `*` cards
and everything between `.control` and `.endc`. Of the reader's fold
exemptions, only the `.lib`/`.inc` and command-whitelist branch at
`inpcom.c:1917` leaves a first byte unfolded, and none of its three triggers
can deliver a device card here: `starhash` implies `buffer[0] == '*'`,
`is_control` is the `.control` body, and `comfile` never reaches this pass at
all, because the whole preprocessing block is inside `if (!comfile && cc)` at
`inpcom.c:1166`. The `plot`/`gnuplot`/`hardcopy` and print-redirection arms
fold up to their spared token, so the first byte is folded;
`keep_case_of_cider_param()` folds everything outside one pair of double
quotes, and a card's first byte is never inside it.
