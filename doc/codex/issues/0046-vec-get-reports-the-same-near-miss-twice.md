# Issue: `vec_get()` Reports the Same Case Near Miss Twice

## Status

Open. Found by the `vec_get()` call-site audit that
`doc/claude/decisions/0009-let-definition-report.md` carries, while closing
`doc/codex/issues/0034`. Predates that work. Cosmetic.

## Summary

`vec_get()` searches two plots on a miss (`src/frontend/vectors.c`):

```c
    if (pl) {
        d = vec_fromplot_maybe_report(word, pl, report_case_miss);
        if (!d)
            d = vec_fromplot_maybe_report(word, &constantplot,
                    report_case_miss);
```

Each call reaches `findvec()`, and `findvec()` reports the near miss per plot.
When `plot_cur` **is** the const plot — which it is until the first analysis
runs — the two calls scan the same table and print the same line twice.

## Impact

Under `casemode=distinguish` only. Measured at the commit that closes
`doc/codex/issues/0034`, before any analysis:

```
ngspice 1 -> print PI
Warning: no vector named 'PI'; 'pi' differs only in case (casemode=distinguish)
Warning: no vector named 'PI'; 'pi' differs only in case (casemode=distinguish)
Warning from checkvalid: vector PI is not available or has zero length.
```

and after an `op`, when `plot_cur` is a real plot, the same command prints it
once. Two lines rather than one; the text and the outcome are right both times.

The const plot's permanent vectors are the twelve `predefs[]` of
`src/frontend/cpitf.c` — `yes`, `TRUE`, `no`, `FALSE`, `pi`, `e`, `c`, `i`,
`kelvin`, `echarge`, `boltz`, `planck` — created by `com_let()` at
`cpitf.c:211`. `TRUE` and `FALSE` are stored upper case, so a lookup of `true`
is a near miss against them. Single letters (`e`, `c`, `i`) and `pi` are
ordinary names for a script variable, which is what makes the const-plot retry
reachable at all.

## Root Cause

The report lives one layer below the retry. `findvec()` is per plot and cannot
know it is the second of two scans of the same table.

## Acceptance Criteria

1. One line per failed lookup, whatever `plot_cur` is.
2. The two-plot search itself does not change — a name found in the const plot
   must still be found.
3. The fix is most likely at `vectors.c`'s `if (!d) d = vec_fromplot_...(word,
   &constantplot, ...)`: skip the retry when `pl == &constantplot`, which is
   also one lookup less. Check that against `plot_prefix()`'s wildcard branch,
   which skips the const plot by name (`cieq(pl->pl_typename, "const")`) and so
   is already exempt.
4. A deck. `print` takes a `>&` redirect, so the capture shape of
   `tests/regression/casedist/vector-let-report.cir` reaches it; counting two
   occurrences of a phrase in a capture needs a counter rather than the
   found/not-found flag those decks use.

## Resolution

Not fixed. Cosmetic duplication of a correct diagnostic, on a path only
reachable before the first analysis.
