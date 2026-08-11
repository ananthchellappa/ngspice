# Issue: Three Latent Defects in the Frontend Vector List, Unrelated to Case

## Status

Open. Bycatch of the `src/frontend/` name-matcher sweep done for
`doc/codex/issues/0027`. None is a case-mode question; all three are in the
functions that sweep walked through, and all three predate the case work.
Grouped because they are one read of one file and would be one patch.

## Summary

**(a) `vec_basename()` reads past the terminator.**
`src/frontend/vectors.c:1266`:

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

**Reachability, established while closing `doc/codex/issues/0032`.** Narrower
than the paragraph above suggests, and it needs saying before anyone budgets
this as a memory-safety fix. A `pl_typename` is built as `"%s%d"` from
`ft_plotabbrev()`'s answer and `plot_num` (`src/frontend/vectors.c`, in both
`plot_alloc()` and the rawfile path), and every `p_name` in `plotabs[]`
(`src/frontend/typesdef.c:67`) is a bare word — `tran`, `op`, `ac`, `sens2`.
So no plot ngspice names itself has a `.` in its typename, the first condition
`strchr(v->v_name, '.')` and the second `cieq(pl_typename, v_name)` cannot both
hold, and the branch is dead for every plot the simulator creates.

It is not *provably* dead: `deftype p <name> <pattern>` (`com_dftype()`, `src/frontend/typesdef.c:113`, its
`case 'p'` branch at `:182`) takes an arbitrary control-language word as the
plot type name and stores it in `plotabs[]`, so `deftype p my.type tran` makes
`my.type1` a typename with a `.` in it, and a vector named exactly `my.type1`
in that plot then reaches the read. That is the only route found.

**(b) `findvec_ally()` dereferences `pl_scale` unguarded.**
`src/frontend/vectors.c:284` compares `d->v_name` against
`pl->pl_scale->v_name` with no NULL check, while its siblings
(`findvec_all()`, `findvec_allv()`) have nothing to dereference. A plot with no
scale vector — which `is_scale_vec_of_current_plot()` at
`src/frontend/postcoms.c:41` explicitly guards against — faults on
`print ally`.

**(c) `com_remzerovec()` bypasses the scale guard and dereferences
`plot_cur`.** `src/frontend/postcoms.c:84-92` clears `VF_PERMANENT` on every
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
issue; recorded here so the read is not lost.

`doc/codex/issues/0032` then rewrote `vec_eq()`, which is `vec_basename()`'s
only structural caller, and considered taking (a) along. It did not, and the
reason is recorded in `doc/claude/decisions/0005-scale-vector-identity.md`
decision 6: (a) is not a typo with an obvious repair but an open question about
what `vec_basename()` should return for a dotted name, so it cannot be landed
RED-first off the back of a case fix, and the reachability note above downgrades
the urgency that made it look like the one to take first. It is still the one to
take first of the three, on the same reasoning — but as its own change, with its
own deck, and starting from what "strip the plot prefix" is supposed to mean.
