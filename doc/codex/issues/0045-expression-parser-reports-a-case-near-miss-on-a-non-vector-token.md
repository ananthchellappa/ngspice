# Issue: The Expression Parser Reports a Case Near Miss on a Token That Is Not a Vector

## Status

Open. Found by the `vec_get()` call-site audit that
`doc/claude/decisions/0009-let-definition-report.md` carries, while closing
`doc/codex/issues/0034`. Predates that work.

## Summary

`PP_mksnode()` (`src/frontend/parse.c:575`) resolves every bare identifier in
every expression through `vec_get()`, and on a miss builds a zero-length
placeholder `dvec` that carries the name forward:

```c
    v = vec_get(string);
    if (v == NULL) {
        nv = dvec_alloc(copy(string), SV_NOTYPE, 0, 0, NULL);
        p->pn_value = nv;
        return p;
    }
```

Most consumers then check the placeholder and report "no such vector", which is
a resolution and wants the near miss. **Three callers ask for a parse with
`check` false and use the placeholder as a name rather than as a failure**
(`src/frontend/define.c:108`, `src/frontend/plotting/plotit.c:794`,
`src/frontend/device.c:1433`). For those, a miss is the expected outcome, and
under `casemode=distinguish` it warns.

## Impact

Under `casemode=distinguish` only. Both measured at `abd0e2f17`.

A user-defined function's **formal parameter**, on a deck with a node `X`:

```
ngspice 1 -> define f(x) x*2
Warning: no vector named 'x'; 'X' differs only in case (casemode=distinguish)
ngspice 2 -> print f(2)
f(2) = 4.000000e+00
```

The parameter `x` is matched by name in `trcopy()` (`define.c:299`) and has
nothing to do with the net `X`. The definition is correct and the warning is
noise.

The **`vs` separator** of `plot`/`hardcopy`, on a deck with a node `VS`:

```
ngspice 1 -> hardcopy f.svg v(OUT) vs v(in)
Warning: no vector named 'vs'; 'VS' differs only in case (casemode=distinguish)
```

`plotit()` recognises the separator afterwards at `plotit.c:805`, by testing
`v_length == 0 && eqc(v_name, "vs")` — that is, by looking for exactly the
placeholder this warning was emitted about. Any deck with a supply node named
`VS` warns on every `plot a vs b`.

`device.c:1433` is `com_alterparam()`'s wordlist and is the third caller; it
was not measured.

## Root Cause

The same one as `doc/codex/issues/0034`: `findvec()` cannot tell why it was
called. Here the caller that knows is two frames up — `ft_getpnames(wl, check)`
already carries the bit that separates the two consumer classes, and
`parse.c:46` uses it only *after* the parse (`if (check && !checkvalid(pn))`),
so nothing reaches `PP_mksnode()` today.

## Acceptance Criteria

1. `define f(x)` and `plot a vs b` do not warn, in any mode.
2. An expression whose identifier really is a mistyped vector still warns —
   `let A = v(OUt)` and `print OUt` are the shapes, and
   `tests/regression/casedist/vector-let-report.cir` already pins the first of
   them, so this must not regress it.
3. The mechanism plumbs the existing `check` flag down to `PP_mksnode()`, or
   handles the three `check == FALSE` callers, rather than silencing
   `parse.c:575` outright. `vec_get_quiet()` (`src/frontend/vectors.c`) is the
   lookup to call once the flag is there; it exists with
   `doc/claude/decisions/0009-let-definition-report.md`.
4. A deck in `tests/regression/casedist/`, and the two halves need two
   different capture shapes. Measured, not assumed:

   - **`define` is on `src/frontend/control.c:62`'s `noredirect[]` list**, with
     `if`, `let`, `stop` and `circbyline`. `define f(x) x*2 >& f.txt` parses
     the redirect into the function body and dies with `PPerror: syntax error
     in line segment / x*2 > & f.txt`, writing no file. So the `define` half
     needs the sourced sub-deck of
     `tests/regression/casedist/vector-let-report.cir`, for the same reason
     that deck needs it.
   - **`hardcopy` is not on the list** and takes `>&` directly. `plot` is
     refused in batch mode ("command 'plot' is not available during batch
     simulation"), so the `vs` half is asserted through `hardcopy`, which
     reaches the same `plotit()` and the same `PP_mksnode()`. It writes a file
     — name it in `CLEANFILES`.

## Resolution

Not fixed. It is a third mechanism in a third file and needs the `check` flag
threaded, which is larger than the one-token call-site change
`doc/codex/issues/0034` needed.
