# Issue: numparam's Built-in Function Names Are Case Sensitive

## Status

Fixed, all four criteria met. Found while closing `doc/codex/issues/0015`, and
not that issue's class: `0015` is about the symbol *tables*, and this is the
keyword list that is consulted before them.

## Summary

`keyword()` (`src/frontend/numparam/xpressn.c:630`) matches an identifier
against the built-in function list `fmathS` (`:92`) with a raw byte compare at `:640`:

```c
while ((p < s_end) && (*p == *keys))
    p++, keys++;
```

There is no `inp_case_folding()` gate and no `tolower_c()`. The list is
lower case only —

```
sqr sqrt sin cos exp ln arctan abs pow pwr max min int log log10 sinh cosh
tanh ternary_fcn agauss sgn gauss unif aunif limit ceil floor
asin acos atan asinh acosh atanh tan nint vec var
```

— so under `casemode=preserve`, where the reader no longer lowercases the card,
`formula()` (`xpressn.c:1075`) misses the built-in, falls through to the
parameter branch and looks the function name up as a symbol.

## Impact

A hard abort, not a wrong number:

```spice
* upper case built-in function under preserve
.OPTIONS noacct
.param rv={SQRT(1e6)}
V1 1 0 dc 4
R1 1 2 {rv}
R2 2 0 3k
.control
op
print v(2)
.endc
.end
```

```
$ ngspice --batch k1.cir                        -> v(2) = 3.000000e+00
$ ngspice -D casemode=preserve --batch k1.cir   -> Undefined parameter [SQRT]
                                                   Formula() error.
```

`{MAX(1k,2k)}` gives `Undefined parameter [MAX]`, `{ABS(-1k)}` gives
`Undefined parameter [ABS]`. `{sqrt(1e6)}` on the same deck runs.

Unreachable under `fold`, where the card arrives lower case.

The differential sweep sees this today, and it is not one reason among several:
on the mechanically uppercased copy of `tests/regression/parser/xpressn-1.cir`,
which exercises the whole function list, **every** diagnostic is this defect —

```
$ tr 'a-z' 'A-Z' < xpressn-1.cir > XP1.cir
$ ngspice -D casemode=preserve --batch XP1.cir 2>&1 | grep -c 'Undefined parameter'
     19 Undefined parameter [NINT]
      5 Undefined parameter [INT]
      5 Undefined parameter [FLOOR]
      5 Undefined parameter [CEIL]
      3 Undefined parameter [SGN]
      2 Undefined parameter [MIN] [MAX] [LOG] [LOG10] [LN] [EXP] [ABS] ...
```

— so `xpressn-1.cir`, `xpressn-2.cir` and `xpressn-3.cir` are three of the
sweep's 51 `DIFF` entries that this one gate would clear. Measured after
`doc/codex/issues/0015` was fixed, so nothing else is masking it.

## Root Cause

A Phase 1 miss. A built-in function name is a language keyword — the user did
not choose it — so it belongs to the class Phase 1 folded unconditionally, and
`keyword()` was not in the census's site list because it compares against a
`static const char *` list rather than against an individual literal.

## Acceptance Criteria

1. `{SQRT(x)}`, `{MAX(a,b)}` and `{TERNARY_FCN(c,a,b)}` resolve to the built-in
   under `preserve`, with a twin pair in `tests/regression/case/`.
2. The fold is unconditional, like the rest of Phase 1, since in `fold` mode
   the compared bytes are already lower case; that has to be stated with the
   argument, not assumed.
3. `keyword()` has exactly one caller, `xpressn.c:1075`, so the fold can go
   inside `keyword()` itself; that was checked rather than assumed.
4. `make check` unchanged with `casemode` unset.

## Resolution

Fixed. One character, on the deck side only, inside `keyword()`:

```c
while ((p < s_end) && (tolower_c(*p) == *keys))
```

`fmathS` is untouched. Criterion 3 was checked rather than assumed: a
repo-wide search finds exactly two mentions of `keyword`, its definition and
the call at `xpressn.c:1075`, so the fold has one caller and goes inside the
function. Criterion 2's argument is below. Criterion 4: `make check` with
`casemode` unset is 206 tests, 0 FAIL — 204 as before plus the twin pair this
issue added — and no committed `.out` was touched.

### Criterion 1's decks

`tests/regression/case/builtin-func-case.cir` and its `-lower` twin, with
byte-identical `.out` files. Four dividers of 1k over 3k on a 4 V source, one
per function, so a single abort cannot hide the rest and each built-in has to
return exactly 1000:

| expression | value |
| --- | --- |
| `{SQRT(1e6)}` | 1000 |
| `{MAX(1k,500)}` | 1000 |
| `{NINT(1000.4)}` | 1000 |
| `{TERNARY_FCN(1,1k,2k)}` | 1000 |

Before the fix the upper deck printed, all of it on stderr,

```
Undefined parameter [SQRT]
Undefined parameter [MAX]
Undefined parameter [NINT]
Undefined parameter [TERNARY_FCN]
ERROR: fatal error in ngspice, exit(1)
```

with an empty stdout, while the lower twin printed the four voltages.
`tests/bin/check.sh` compares stdout only, so the pair was first run against an
empty `.out`: the upper deck passes there and the lower twin fails. That
asymmetry is the proof the assertion is the missing number and not a
diagnostic.

### Criterion 2, and where the no-op argument stops being free

The fold is unconditional. In `fold` mode the compared bytes are already lower
case, so `tolower_c(*p) == *p` and the predicate is unchanged — but that has to
be argued against the reader's exemption chain, not asserted, because the
general lowercasing loop at `src/frontend/inpcom.c:1924-1926` sits at the end
of six branches that keep their case.

| exemption | why it cannot reach `keyword()` |
| --- | --- |
| `.lib` / `.inc` (`inpcom.c:1911`) | commented out before numparam runs; the file's contents are re-read through the same folding reader |
| control-command whitelist, `*#` | `.control` bodies and `*#` lines are unlinked from the deck before `subckt.c` invokes numparam |
| plot / gnuplot / hardcopy titles | spares only the text after `title`/`xlabel`/`ylabel` |
| print redirection (`inpcom.c:1880`) | spares only the text after `>` |
| XSPICE code-model `.model` quoted span (`inpcom.c:1907-1909`) | measured, see below |
| CIDER `.model` quoted span (`inpcom.c:1898-1901`) | `#ifdef CIDER`, off in `build-ver_50`; **unmeasured** |

The XSPICE branch is the one that looked live on paper — those spans keep their
case and numparam's brace stripper is quote-blind — so it was measured rather
than argued. Those cards never reach numparam's evaluator at all:

```
$ ngspice --batch  # .model x d_cosim (p={NOSUCHPARAM})   quoted or unquoted
v(1) = 1.000000e+00                       # runs to completion, no diagnostic
$ ngspice --batch  # .model rmod r (rsh={NOSUCHPARAM})
Undefined parameter [nosuchparam]         # and exit 1
```

Same for `filesource`, `table2d` and `d_state`, i.e. for the whole
`is_xspice_model()` list. So the correct statement is: a no-op in `fold` mode
for every card numparam evaluates, with the CIDER twin unverified because that
build is off.

### The behavioural change that is not the bug fix

Under `preserve`, a user parameter whose name case-insensitively equals a
built-in is now shadowed by the built-in. `.param SQRT=2` still installs the
symbol — `keyword()` is inside `formula()`'s operand scanner, not the name
parser — but `{SQRT*3}` is read as a function token and evaluates to 0 with no
diagnostic. That is exactly what `.param sqrt=2` already does under `fold`, so
this makes one policy of two rather than inventing a failure mode, and it is
the intended direction. The silent 0 itself is a separate defect, recorded as
`doc/codex/issues/0025`. Names at risk are the whole of `fmathS`: `MAX`, `MIN`,
`INT`, `LOG`, `ABS`, `VAR`, `LIMIT`, `POW`, `SGN`, `VEC`, `EXP`, `LN`, `TAN`,
`PWR`, `CEIL`, `FLOOR`, `NINT` among them.

A user `.func` named after a built-in still wins in both modes, because
`inp_expand_macros_in_deck()` expands `.func` textually before numparam sees
the card.

### Sweep

`DIFF` 51 → 49 with no deck's verdict worse. `xpressn-2.cir` reached `OK`.
`xpressn-1.cir` and `xpressn-3.cir` did not, and what is left on them is not
this defect and not an ngspice defect at all — see
`doc/claude/checklists/phase2-differential-sweep.md`. The `Undefined parameter`
count on the uppercased copies of all three went from 19-plus to 0.
