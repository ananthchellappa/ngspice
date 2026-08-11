# Issue: `pyplot`'s File-Name Probe Reports a Vector Case Near Miss

## Status

Open. Found by the `vec_get()` call-site audit that
`doc/claude/decisions/0009-let-definition-report.md` carries, while closing
`doc/codex/issues/0034`. Predates that work.

## Summary

`com_pyplot()` decides whether its first word is an output file name or the
first plot expression by asking whether the word names a vector
(`src/frontend/com_pyplot.c:53`):

```c
        const char *w = wl->wl_word;
        bool is_expr = (strchr(w, '(') != NULL) || (vec_get(w) != NULL);
        if (!is_expr) {
            fname = wl->wl_word;
            wl = wl->wl_next;
        }
```

That is a **probe**, not a resolution: a miss is how the caller learns the word
is the file name it is about to create. Under `casemode=distinguish` the miss
goes through `findvec()` and reports a case near miss, so naming the output
file after the net being plotted warns and then works.

## Impact

Under `casemode=distinguish` only. Measured at `abd0e2f17`, a deck with a node
`Out`:

```
ngspice 1 -> pyplot out v(Out)
Warning: no vector named 'out'; 'Out' differs only in case (casemode=distinguish)
```

and `out.py`/`out.data` are then written, correctly. `out` was always going to
be a file name; nothing was mistyped.

This is the same class as `doc/codex/issues/0034` — a diagnostic on a lookup
whose miss is the ordinary outcome — but it is **not** the same category.
`com_let()`'s left-hand side is *defining* the name, which decision 2 of
`doc/claude/decisions/0001-distinguish.md` names; a probe is a third thing that
decision does not name, and this issue is where that gap is recorded.

## Root Cause

`findvec()` cannot tell why it was called;
`doc/claude/decisions/0009-let-definition-report.md` gives `vec_get()` a silent
sibling, `vec_get_quiet()`, for callers whose miss is not a failure, and this
call site was left out of it deliberately.

## Acceptance Criteria

1. Decide whether a **probe** is silent. It is not obvious, and the argument
   against silence is real and was measured: `pyplot out` with **no plot
   arguments** takes `out` as the file name, finds `wl` empty and returns at
   `com_pyplot.c:88-89` without writing anything or saying anything, so at that
   site the near miss is the *entire* diagnostic a user who mistyped the case
   of a vector name will get. Silencing it turns a case-typo'd `pyplot out`
   into a silent no-op. Against that: naming the file after the net is the
   ordinary deck, and it is the one the warning fires on.
2. Whichever way it goes, record it as a third row of `0001` decision 2's rule,
   beside *definition* and *resolution* — not as a one-line change to this call
   site. `measure.c:83` and `:100` are probes of the same shape (`com_meas()`
   asks whether a token names a vector before handing it to `com_measure2`),
   and their miss is already re-reported by `com_measure2.c:400`/`:403`, so a
   rule covers three sites and a patch covers one.
3. A deck. It is available and cheap, which is worth writing down because it
   looks as though it is not: `pyplot <name>` with no further arguments runs
   the probe and then returns before `plotit()`, so no `.py` is written and no
   python interpreter is started. Measured. `pyplot` is not on
   `src/frontend/control.c:62`'s `noredirect[]` list, so `>&` attaches to it
   directly and the capture can be read back the way
   `tests/regression/casedist/vector-let-report.cir` does.

## Resolution

Not fixed. `0034`'s session left it because it needs criterion 2's decision
first, and because a session that changes two diagnostics changes neither
carefully — the reasoning `0034`'s own prompt applies to
`doc/codex/issues/0039`.
