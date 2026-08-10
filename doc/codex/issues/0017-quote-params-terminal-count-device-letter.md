# Issue: `inp_quote_params()` Adjusts the Terminal Count on a Lower-Case Device Letter

## Status

Open. Found while closing `doc/codex/issues/0013`, and deliberately not fixed
there: it is a first-character device-letter test, which is
`doc/codex/issues/0009`'s follow-up class, and 0013's scope was the keyword
searches.

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

Not fixed. `doc/codex/issues/0013`'s scope was the keyword searches, and
`doc/codex/issues/0009`'s follow-up table was explicitly held out of it. The
fix is one token — `strchr("fhmouydqjzswx", elem_letter(curr_line))` — and it
belongs with the rest of that table, which is the next item on the
case-sensitivity critical path after 0013.
