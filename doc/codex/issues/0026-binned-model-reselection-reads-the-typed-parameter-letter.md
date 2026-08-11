# Issue: Binned-Model Re-Selection Reads the Typed Parameter Letter Byte-Exactly

## Status

Fixed. Found by the differential sweep while closing `doc/codex/issues/0016`
class (a); the sweep's uppercased copy of the new
`tests/regression/case/alter-rebin-case.cir` reports `DIFF`, and this is why.
Not a regression from that fix — the pre-fix binary produces the same wrong
number.

## Summary

`if_set_binned_model()` (`src/frontend/device.c:1250`) decides which of a
device's two geometry parameters the user just altered by testing the first
character of the typed parameter name against a lower-case literal:

```c
/* src/frontend/device.c:1272 */
    if (param[0] == 'w')
        w = *val->v_realdata; /* overwrite the width with the alter param */
    else
        l = *val->v_realdata; /* overwrite the length with the alter param */
```

Its only caller guards it with a **case-insensitive** test on the same string:

```c
/* src/frontend/device.c:1489 */
    if ((tolower_c(dev[0]) == 'm') && (eqc(param, "w") || eqc(param, "l")))
        if_set_binned_model(ft_curckt->ci_ckt, dev, param, dv);
```

So `alter M1 W=2u` is admitted by `eqc(param, "w")` at `:1489` and then falls
into the `else` at `:1272`, which assigns the new **width** to the local `l`.
The bin is then selected from the *unchanged* width and a fabricated length, and
the device keeps whichever bin it already had.

`src/frontend/device.c:1420` is what exposes this: it folds the typed `param`
and `dev` only under `inp_case_folding()`, which is correct, and is the same
gate that exposes `doc/codex/issues/0016`.

## Impact

Under `fold` this is unreachable: `device.c:1420` has lowercased `param`.

Under `casemode=preserve` an upper-case `W=` or `L=` on an `alter` command
silently applies the new geometry without re-binning. Measured on
`tests/regression/case/alter-rebin-case.cir` with only the typed parameter
letter changed, two BSIM3 bins differing in `vth0` and in which width band they
claim:

```
$ ngspice -D casemode=preserve --batch rebin.cir      # alter M1 w=2u
v(3) = 1.432887e+00
Notice: model has changed from nch.1 to nch.2.
v(3) = 1.481980e+00

$ ngspice -D casemode=preserve --batch rebinW.cir     # alter M1 W=2u
v(3) = 1.432887e+00
v(3) = 1.340324e+00
```

The `alter` itself works — the instance really does get the new width, which is
why there is no diagnostic:

```
@M1[w] = 2.000000e-06
@M1[l] = 1.000000e-06
```

— the device is 2 µm wide and still evaluated with `nch.1`'s `vth0=0.4`. It is
a wrong number with no diagnostic, the same failure mode as
`doc/codex/issues/0016`'s `device.c:1489` half, one call level deeper.

The stock run of the same deck re-bins correctly, so the two modes disagree.

## Root Cause

The same lower-case-literal dispatch class as `doc/codex/issues/0009`,
`0014` and `0016`. `device.c:1272` was written when `device.c:1420` folded the
typed name unconditionally, so `param[0]` was always lower case by the time it
was read. Phase 2 gated that fold, and the call-site guard at `:1489` was made
case-insensitive without the body that follows it being audited.

## Acceptance Criteria

1. `if_set_binned_model()` folds the character it tests, or takes the decision
   from the caller, which has already made it case-insensitively.
2. Under `preserve`, `alter M1 W=2u` and `alter M1 w=2u` select the same model
   bin, and `alter M1 L=…` still alters the length rather than the width.
3. `tests/regression/case/` gains a twin pair whose evidence is a drain voltage,
   in the shape of `alter-rebin-case.cir`, varying the **typed parameter**
   letter rather than the device letter.
4. `make check` unchanged with `casemode` unset.

## Resolution

Fixed. `src/frontend/device.c:1272` is now `tolower_c(param[0]) == 'w'`, which
is acceptance criterion 1's first option: fold the character being tested. The
caller at `:1489` already made the decision case insensitively with
`eqc(param, "w") || eqc(param, "l")`, so the two now agree.

Not fold-mode visible. `device.c:1420` folds `param` whenever
`inp_case_folding()` is true, so in the default mode `param[0]` is already
lower case when `:1272` reads it and `tolower_c()` returns the same byte. The
deck below confirms it: run without `-D`, the upper case twin already produced
the right numbers before the fix.

The twin pair is `tests/regression/case/alter-rebin-param-case.cir` and
`alter-rebin-param-case-lower.cir`, which differ only in the two typed
parameter letters on the `alter` commands and share one `.out`. RED, measured
under `casemode=preserve` before the change:

```
upper twin   v(3) = 1.432887e+00 / v(3) = 1.340324e+00 / v(3) = 1.484293e+00
lower twin   v(3) = 1.432887e+00 / v(3) = 1.481980e+00 / v(3) = 1.484293e+00
```

The third `op` follows `alter M1 L=1.5u` and is the guard on the other arm:
`L` must still reach the length. It was already correct before the fix,
because an upper case `L` fails the `== 'w'` test and falls into the `else`
that the length wants, and it stays correct after it.

It needed no `distinguish` decision: `w` and `l` are language keywords naming
instance parameters, not identifiers, so it was Phase 1 class work.
