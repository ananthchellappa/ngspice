# Issue: numparam's Built-in Function Names Are Case Sensitive

## Status

Open. Found while closing `doc/codex/issues/0015`, and not that issue's class:
`0015` is about the symbol *tables*, and this is the keyword list that is
consulted before them.

## Summary

`keyword()` (`src/frontend/numparam/xpressn.c:624`) matches an identifier
against the built-in function list `fmathS` (`:92`) with a raw byte compare:

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
`formula()` (`xpressn.c:1069`) misses the built-in, falls through to the
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
3. `keyword()` has exactly one caller, `xpressn.c:1069`, so the fold can go
   inside `keyword()` itself; that was checked rather than assumed.
4. `make check` unchanged with `casemode` unset.

## Resolution

Not fixed. Out of scope for the session that found it, whose scope was
`doc/codex/issues/0015`, `0020` gap 1 and `0013`'s three exact-match sites. It
is cheap: one gate inside `keyword()`, and the RED deck above is already
written down.
