# Issue: An Analysis Mints Result Vectors With Capitals The Deck Never Typed, and `distinguish` Will Not Answer For Them

## Status

Open. Found on 2026-08-12 by flipping `set_case_mode()`'s default from
`NG_CASE_FOLD` to `NG_CASE_PRESERVE` and then to `NG_CASE_DISTINGUISH`,
rebuilding, and running `make -k check` — an experiment run to answer *how far
is the default from being case-sensitive*, not to change it. The default is
**unchanged**; `src/frontend/inpcom.c` was restored byte-identical and
verified with `cmp`.

```
default=fold (shipped)   282 PASS / 0 FAIL
default=preserve         278 PASS / 4 FAIL
default=distinguish      272 PASS / 10 FAIL
```

Six directories pin `casemode` themselves (190 decks), so only 92 decks were
exposed to the default. Of the ten `distinguish` failures, **two are the mode's
inherent semantics working correctly** — `tests/xspice/case/auto-bridge-node-case-fold.cir`
and `event-node-case-fold.cir` are the *fold twins* of the Phase 3 gate decks
and deliberately spell one mixed-signal net two ways, so under `distinguish`
they are two nets, no bridge is inserted and `v(a)` reads `0.000000e+00`
instead of `3.300000e+00`. That is `doc/claude/decisions/0001-distinguish.md`
decision 5's migration hazard, not a defect.

Of the remaining four, **three are deck artifacts and one is this issue.** The
misattribution is itself worth recording: all four decks blame something they
have not measured, and the first reading of this experiment repeated their
wording and called all four contract violations. They are not.

## Summary

### 1. `.tf` names its own results with capitals, so the documented spelling misses

Measured on a deck containing **no upper-case character anywhere** — checked
with `grep -c '[A-Z]'`, which returns 0:

```
circbyline * min
circbyline v1 in 0 dc 1
circbyline r1 in out 1k
circbyline rl out 0 3k
circbyline .end
tf v(out) v1
display
print transfer_function
```

`display` prints the same three vectors in **both** modes:

```
    Transfer_function   : voltage, real, 1 long [default scale]
    output_impedance_at_V(out): voltage, real, 1 long
    v1#Input_impedance  : voltage, real, 1 long
```

and only the lookup diverges:

```
fold         transfer_function = 7.500000e-01
distinguish  Warning: no vector named 'transfer_function'; 'Transfer_function'
                      differs only in case (casemode=distinguish)
             Warning from checkvalid: vector transfer_function is not available
                      or has zero length.
```

`transfer_function` is the spelling the language documents and the only one a
deck would type. Under `distinguish` the user must write
`print Transfer_function` — a capital the simulator chose and no card contains.

Three further findings that narrow it, each measured rather than read:

- **The dot-card form fails identically.** `.tf v(out) v1` with a `.control`
  block reaches the same three vector names and the same miss, so the
  command-form synthetic-card path through `if_run()`/`INPpas2()` is not the
  variable and `doc/codex/issues/0043` is **not** implicated.
- **The `v`/`i` accessor matcher is not implicated.** `dot_tf()` at
  `src/spicelib/parser/inp2dot.c:369` and `:392` really are `cieq(name, "v")`
  and `cieq(name, "i")` — a literal operand, correct in all three modes — and
  the spec's gate-table row claiming those `Done` is accurate.
  `tests/regression/pipe/analysis-keyword-case.cmd`'s echo,
  `ERROR: uppercase V() not accepted by tf`, is wrong on both counts.
- **Only the byte compare rejects.** Under `distinguish`,
  `print Transfer_function`, `print v1#Input_impedance` and
  `print output_impedance_at_V(out)` all succeed. Same plot, same lookup
  table, same folded hash chain — the only thing that differs is the `typed`
  operand, which isolates the difference to one string comparison.

### 2. `.noise`'s noise-figure vectors are the same shape, and are **not measured**

`grep 'IFnewUid' src/spicelib/analysis/*.c` returns four more capitalised
literals — `NF`, `NFmin`, `Rn`, `SOpt` — beside `Transfer_function`,
`Input_impedance` and `Output_impedance`. A plain `.noise` run does not mint
them: an all-lower-case two-resistor `.noise` deck produces only
`frequency`, `inoise_spectrum`, `onoise_spectrum` and the per-device
`onoise_r1*` family, identically in `fold` and `distinguish`. Reaching the
noise-figure path needs a deck this issue does not have, so whether
`print nf` misses under `distinguish` is **stated as unmeasured** rather than
assumed.

### 3. Four `tests/regression/pipe` decks build upper-case cards and query them lower case

This is what masked item 1, and it has to be fixed before that directory can
be evidence for any mode but the default.

| Deck | Card it writes | What it then asks for |
| --- | --- | --- |
| `analysis-keyword-case.cmd` | `circbyline V1 in 0 dc 1` | `tf V(out) v1` |
| `device-keyword-case.cmd` | `circbyline R1 in out 1k` | `save @r1[i]`, `save @r1[I]` |
| `spiceif-keyword-case.cmd` | `circbyline V1 in 0 dc 1` | `i(v1)`, `@r1[...]` |
| `exprcoms-keyword-case.cmd` | `circbyline V1-A na 0 dc 1` | `i(v1-a)` |

Under the default `fold` the reader lowercases the cards, so both sides agree
and every deck passes. Under `distinguish` the card's `V1` and the query's
`v1` are two devices **by design** — `0001` decision 5's *Withdrawn* list says
so in as many words: *"Two spellings of a name typed at the control language:
`alter`, `show`, `@dev[param]`, `let`, `print`, `save`. The typed name must now
match the stored spelling exactly."*

The comparison that rejects is `vec_name_eq()`
(`src/frontend/vectors.c:415-419`) with `v_name = "V1#branch"` against
`typed = "v1#branch"`, or `ent_eq()` (`src/spicelib/parser/inpsymt.c:35`) for
the `@dev[param]` instance token. **Both already carry the predicate they
should have** and neither should change; `vec_name_eq()`'s two arms are
permanent residents of `tests/lint/identity.baseline`.

Separated by experiment, on a circuit whose card spells the instance `R1`,
one `save` per run:

| | `fold` | `preserve` | `distinguish` |
| --- | --- | --- | --- |
| `save @R1[i]` | **length 0** | 1 | 1 |
| `save @r1[i]` | 1 | 1 | length 0 — correct, `r1` is not `R1` |
| `save @R1[I]` | **length 0** | 1 | 1 |

So the instance-parameter letter is case-insensitive wherever the instance
resolves, and contract point 2 is **not** violated. `device-keyword-case.cmd`'s
`INPaName rejected the parameter` is wrong; `INPaName` never saw a bad
parameter.

That table's left column is a **separate, shipped-mode** finding and is not
this issue: with `R1` on the card, `save @R1[i]` fails under the default
`fold`, because the typed `@R1[…]` is control-language text the reader never
folds and the instance lookup on that path is byte-exact. It belongs to
`doc/codex/issues/0016`'s family — instance-name matchers that do not go
through `INPretrieve()`/`DEVnameHash` — and wants its own measurement.

## Impact

Item 1 is not a wrong number: the lookup fails loudly, with `0001` decision
2's near-miss warning naming both spellings, and the value is reachable under
the capitalised spelling. It is a usability defect in `distinguish` and a
documentation contradiction — the manual's `transfer_function` is not the name
the simulator stores.

It also matters more than its size, because it is the **third instance of a
hazard `0001` decision 3 wrote down and left open**: *"`distinguish` must be
exact about names the deck chose and must stay case-insensitive about names
ngspice constructs. `V(1)` is the first instance. `q1#collCX` is the next one,
and it is not handled."* `V(1)` got `vec_wrapped_name_eq()`; `q1#collCX` is a
documented limitation; this is the first case where the constructed name is a
whole *result* of an analysis rather than a decoration on a node name, and
where the spelling a user is told to type is the one that fails.

Item 3 has no user impact — the decks pass in the shipped mode — but it costs
the project the only test directory that exercises the control language
through `ngspice -p`. `tests/regression/casedist/` has 23 decks and none of
them uses the prompt, which is structurally why item 1 went unseen: the
`distinguish` suite cannot reach the path, and the suite that can is pinned to
`fold`.

## Root Cause

Item 1: four string literals and one `tprintf`, none of them folded in any
mode.

```c
/* src/spicelib/analysis/tfanal.c */
:91   IFnewUid(ckt, &tfuid,  NULL,            "Transfer_function", UID_OTHER, NULL);
:94   IFnewUid(ckt, &inuid,  job->TFinSrc,    "Input_impedance",   UID_OTHER, NULL);
:98   IFnewUid(ckt, &outuid, job->TFoutSrc,   "Output_impedance",  UID_OTHER, NULL);
:100  name = tprintf("output_impedance_at_%s", job->TFoutName);
```

`TFoutName` is itself constructed, at `src/spicelib/parser/inp2dot.c:390` and
`:516`, as `tprintf("V(%s)", nname1)` — the same capital `V` that
`src/frontend/outitf.c:1164` produces for a node name beginning with a digit,
and which `vec_wrapped_name_eq()` (`src/frontend/vectors.c:366`) exists to
fold. That helper cannot help here: it requires the *whole* string to be a
`V(...)` or `I(...)` wrapper, and `output_impedance_at_V(out)` is a wrapper
buried in a longer name.

`fold` and `preserve` absorb all of it at `vec_name_eq()`'s `cieq` arm
(`vectors.c:416`). `distinguish` takes the `strcmp` arm at `:418` and rejects.

Item 3: the decks were written under the default mode and their `circbyline`
seeds were never made case-consistent, because under `fold` it cannot matter.

## Acceptance Criteria

1. Under `distinguish`, an all-lower-case deck that runs `.tf` or `tf` can
   `print transfer_function`, `print v1#input_impedance` and
   `print output_impedance_at_v(out)` — i.e. the spelling the manual
   documents resolves. The mechanism is a decision, not a given: fold the
   literals to lower case at the mint (which changes `display` output in
   **every** mode, so it needs its own RED), or extend the query-side
   tolerance the way `0029`'s `entrynb_constructed()` and
   `vec_wrapped_name_eq()` do — tolerance on the query for a name the
   simulator built. `0001` decision 3's closing paragraph is the rule to
   apply, and whichever is chosen must say why the other was not.
2. Whichever is chosen, `print Transfer_function` keeps working, because a
   deck may already use it.
3. `.noise`'s `NF`, `NFmin`, `Rn` and `SOpt` are **measured** under
   `distinguish`, with a deck that actually reaches them, and either fixed with
   item 1 or recorded as a separate case with the reason.
4. The four `tests/regression/pipe` decks are made case-consistent in their
   `circbyline` seeds, and each misleading `ERROR:` text is corrected to name
   what it actually measures. After that the directory passes under
   `-D casemode=preserve` and `-D casemode=distinguish` as well as under the
   default — and that becomes an assertable claim, which it is not today.
5. A deck asserts item 1 where `make check` can see it. It cannot live in
   `tests/regression/pipe` while that directory is pinned to `fold`, so it
   belongs in `tests/regression/casedist/` — which would make it the first
   deck there to exercise the control-language prompt path.
6. `make check` unchanged with `casemode` unset — 282 PASS / 0 FAIL — and the
   differential sweep's `NUM-DIFF` and `PARSE-FAIL` stay 0.

## Resolution

Open.
