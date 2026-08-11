# Issue: Three Latent Defects in the Frontend Vector List, Unrelated to Case

## Status

Open. Bycatch of the `src/frontend/` name-matcher sweep done for
`doc/codex/issues/0027`. None is a case-mode question; all three are in the
functions that sweep walked through, and all three predate the case work.
Grouped because they are one read of one file and would be one patch.

## Summary

**(a) `vec_basename()` reads past the terminator.**
`src/frontend/vectors.c:1257`:

```c
    if (strchr(v->v_name, '.')) {
        if (cieq(v->v_plot->pl_typename, v->v_name))
            (void) strcpy(buf, v->v_name + strlen(v->v_name) + 1);
        else
            (void) strcpy(buf, v->v_name);
```

The offset is `strlen(v->v_name) + 1`, which is one byte **past** `v_name`'s
NUL, so the branch copies whatever follows the string in the heap until it
finds a zero. The function's stated job is to strip the plot prefix, so the
intent is almost certainly `strlen(v->v_plot->pl_typename) + 1` — and the
guard should then be a prefix test rather than `cieq()` of the whole name,
because a vector whose entire name equals its plot's typename has no prefix to
strip.

Reached when a vector's name contains `.` and equals its plot's typename
ignoring case. Established by reading; not exercised by a deck.

**(b) `findvec_ally()` dereferences `pl_scale` unguarded.**
`src/frontend/vectors.c:285` compares `d->v_name` against
`pl->pl_scale->v_name` with no NULL check, while its siblings
(`findvec_all()`, `findvec_allv()`) have nothing to dereference. A plot with no
scale vector — which `is_scale_vec_of_current_plot()` at
`src/frontend/postcoms.c:41` explicitly guards against — faults on
`print ally`.

**(c) `com_remzerovec()` bypasses the scale guard and dereferences
`plot_cur`.** `src/frontend/postcoms.c:79-92` clears `VF_PERMANENT` on every
zero-length vector without `com_unlet()`'s `is_scale_vec_of_current_plot()`
check, so it will de-permanent a zero-length scale vector that `unlet` refuses
to touch — and that refusal exists because "then redrawing the graph crashes
ngspice" (`postcoms.c:36`). It also walks `plot_cur->pl_dvecs` at `:85` with no
NULL check, unlike `postcoms.c:39`.

## Impact

(a) is a heap over-read into a fixed `BSIZE_SP` stack buffer, so it is both an
information leak and a potential overflow of `buf` if the following heap bytes
run long. `vec_basename()` feeds `vec_eq()`, which decides scale identity for
`print`, `write`, `write_sparam` and `agraf`, so the blast radius is wider than
the function.

(b) and (c) are NULL dereferences in states the surrounding code already knows
to check for elsewhere in the same file.

All three are mode-independent: they behave identically under `fold`,
`preserve` and `distinguish`.

## Root Cause

(a) is a transcription error in prefix-stripping arithmetic, guarded by a
condition rare enough that it has never been hit in the suite.
(b) and (c) are missing guards that exist in sibling functions, which is the
signature of code added beside a checked path without copying the check.

## Acceptance Criteria

1. `vec_basename()` strips the plot prefix using the prefix's length and a
   prefix test, and a unit-level or deck-level case reaches the branch — a
   vector whose name contains `.` inside a plot whose typename matches it.
2. `findvec_ally()` returns an empty result rather than faulting when
   `pl->pl_scale` is NULL, and a deck runs `print ally` on such a plot.
3. `com_remzerovec()` skips the current plot's scale vector, with the same
   reason `com_unlet()` gives, and guards `plot_cur`.
4. `make check` unchanged in all three modes.

## Resolution

Not fixed. Out of scope for `doc/codex/issues/0027`, which was a case-mode
issue; recorded here so the read is not lost. (a) should be taken first: it is
the only one of the three that can corrupt memory rather than crash.
