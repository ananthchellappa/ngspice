# Issue: A Parameter Named After a Built-in Function Evaluates to Zero Without a Diagnostic

## Status

Open. Not introduced by `doc/codex/issues/0022`; that fix only made it
reachable under `-D casemode=preserve` as well, which is how it was found.
Present in `fold` mode at every commit in this series and, as far as the code
shows, since `keyword()` was written.

## Summary

`formula()` consults the built-in function list before the symbol table
(`src/frontend/numparam/xpressn.c:1073-1084`):

```c
} else if (alfa(c)) {
    const char *s_next = fetchid(s, s_end);
    fu = keyword(fmathS, s, s_next);      /* numeric function? */
    if (fu > 0) {
        state = S_init;  /* S_init means: ignore for the moment */
    } else {
        ...
        u = fetchnumentry(dico, ds_get_buf(&tstr), &error);
        state = S_atom;
    }
```

An identifier that matches `fmathS` therefore never reaches
`fetchnumentry()`. If no argument list follows it, nothing sets `u`, the token
contributes 0, and `state = S_init` suppresses the operand/operator sanity
check that would otherwise reject the expression. The result is a zero with no
message from numparam at all.

The *assignment* side is unaffected — `keyword()` has exactly one caller and it
is inside `formula()`'s operand scanner, not the name parser — so `.param max=5`
installs the symbol normally and `listing param` still shows it. Only the
*uses* are shadowed.

## Impact

A silent wrong number, in the default mode, with no `-D` flag involved.

```spice
* parameter named after a built-in
.OPTIONS noacct
.param max=5
.param b={max*2}
V1 1 0 dc 4
R1 1 2 {b}
R2 2 0 3k
.control
op
print v(2)
.endc
.end
```

```
$ ngspice --batch shadow.cir
Warning: Value of resistor r1 is too small, set to 1.000000e-12
v(2) = 4.000000e+00                       <- b == 0

$ ngspice --batch shadow.cir            # with the name changed to mxx
v(2) = 3.986711e+00                       <- b == 10, the intended answer
```

The only diagnostic is the device layer's resistor clamp, which arrives long
after the mistake and names the resistor rather than the parameter. Without a
device that happens to complain about zero, there is no diagnostic at all.

After `doc/codex/issues/0022` the same deck behaves identically under
`preserve` with the name spelled `MAX`, which is the intended
one-policy-in-both-modes direction; the run above was measured in both modes and
printed `v(2) = 4.000000e+00` in each.

The exposed names are the whole of `fmathS`. The ones that read like ordinary
parameter names are the hazard: `max`, `min`, `int`, `log`, `abs`, `var`,
`limit`, `pow`, `sgn`, `vec`, `exp`, `ln`, `tan`, `pwr`, `ceil`, `floor`,
`nint`.

## Root Cause

`state = S_init` is used both for "this token is a function, wait for its
argument list" and for "ignore this token", and nothing later checks that the
argument list actually arrived. `keyword()` matching an identifier that is not
followed by `(` is treated as success.

## Acceptance Criteria

1. A built-in function name used as a bare operand is diagnosed rather than
   silently evaluated as 0 — either as a numparam error, or by falling through
   to the symbol table when the identifier is not followed by `(`.
2. Whichever is chosen, `{sqrt(4)}` and `{ternary_fcn(1,2,3)}` keep working and
   the `vec`/`var` string-argument forms keep working.
3. A deck under `tests/regression/` whose evidence is the number, not the
   diagnostic, since `tests/bin/check.sh` filters `Error` and `Warning` out of
   both sides.
4. `make check` unchanged.

## Resolution

Not fixed. Out of scope for the session that found it, whose scope was
`doc/codex/issues/0022` and `0023`. It is not a case defect — it is the same in
both modes — and unlike `0022` it needs a design decision: "shadow" and
"diagnose" are both defensible, and falling through to the symbol table changes
what a deck that today gets 0 will compute. That makes it a behaviour change to
the default mode, which is a different class of commit from the keyword folds
in this series.
