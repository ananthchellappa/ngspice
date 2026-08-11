# Issue: `let` Warns About a Case Near-Miss When It Is *Defining* a Name

## Status

Closed on `ver_50` by `doc/claude/decisions/0009-let-definition-report.md`.
Found while closing `doc/codex/issues/0027`. Predates that work: the warning
arrived with Phase 3 gate 3 and `com_let()` has always resolved its left-hand
side through `vec_get()`.

## Summary

`findvec()` reports a case near-miss on a failed lookup
(`src/frontend/vectors.c:234`):

```c
    if (!d && inp_case_mode() == NG_CASE_DISTINGUISH) {
        vec_warn_case_near_miss(pl_lookup_table, word);
    }
```

`com_let()` (`src/frontend/com_let.c:101`) resolves its **target** name through
`vec_get()` to decide whether to overwrite an existing vector or create a new
one. When it creates, the lookup has just failed, so the warning fires — on a
*definition*.

`doc/claude/decisions/0001-distinguish.md` decision 2 rejects warning on a
definition, by name and with the reason: "Under `distinguish`, `Out` and `OUT`
as two real nets is not a mistake, it is the feature; a warning there makes the
feature unusable and trains users to ignore the warning that matters."

## Impact

Under `casemode=distinguish` only. Measured at `496110f27`, tran deck with
scale `time`:

```
let TIME = time * 2
  (stderr) Warning: no vector named 'TIME'; 'time' differs only in case (casemode=distinguish)
```

and the `let` then succeeds, correctly, creating a second vector. Same for
`let VAA = 2` beside an existing `Vaa`.

So the mode's headline capability — two spellings are two vectors — cannot be
used from the control language without a warning saying the opposite. The
result is right and the diagnostic is noise, which is the failure mode decision
2 called out as the one that trains users to ignore diagnostics.

There is a second-order inconsistency now that `0027` has closed:
`let Vaa = 1` beside a `VAA` warns, while `compose Vaa values 1` beside a `VAA`
is deliberately silent, because `compose` reaches `pl_dvecs` directly and never
calls `findvec()` for its result name. Two commands that both define a vector,
two different diagnostics.

Nothing in the harness can see it: `tests/bin/check.sh` captures stdout only
and filters every line containing `Warning` from both sides.

## Root Cause

The near-miss check lives in `findvec()`, which cannot tell why it was called.
Every other caller of `findvec()` is resolving a name the user wants to read,
so the check is right for them; `com_let()`'s left-hand side is the one query
whose miss is the expected outcome. The same distinction `doc/codex/issues/0027`
had to make between `unlet` (resolution) and `compose`/`cross` (definition),
one layer up.

## Acceptance Criteria

1. `com_let()`'s resolution of its own target name does not emit the near-miss
   warning, in any mode. Its *right-hand side* must keep it — `let A = v(OUt)`
   with a typo there is exactly what the warning is for.
2. The mechanism separates definition from resolution at the call site rather
   than inside `findvec()`, the way `vec_remove()`'s `report_case_miss` does.
   `vec_get()` is the shared entry point, so this probably wants a sibling that
   does not report, or `findvec()` split into a reporting and a silent form.
   Note that `vec_get()` has many other callers whose miss *should* report, so
   changing `vec_get()` itself is wrong.
3. Audit the other definition-shaped `vec_get()` callers with it. `com_let()`
   is the one that was measured; `com_setscale.c:19` and the `@dev[param]`
   paths are the next candidates.
4. A deck in `tests/regression/casedist/` that creates a case-variant vector
   with `let` and asserts on both values. It cannot assert on the absence of
   the warning — stderr is not compared — so the deck guards the values and the
   commit quotes the stderr, per decision 2.

## Resolution

Fixed on `ver_50`; `doc/claude/decisions/0009-let-definition-report.md` is the
record. `vec_get()` keeps its name and its behaviour for its other forty
callers and gains a silent sibling, `vec_get_quiet()`, which threads
`report_case_miss` through `vec_fromplot()` into `findvec()`. `com_let()` calls
it for its left-hand side. Criterion by criterion:

1. **Met, with one boundary the criterion did not anticipate.** The *indexed*
   form keeps `vec_get()`: `let x[0] = 1` cannot create `x` — `com_let()`
   refuses it with *"When creating a new vector, it cannot be indexed"* — so
   that lookup resolves a name that must already exist and its miss is the
   reported error. Blanket suppression made that error unexplainable, and an
   adversarial review of the first version of the fix is what found it.
   `0009` decision 2.
2. **Met.** The split is at the call site, `findvec()` decides nothing.
   `0009` decision 1 also says why the answer differs in shape from
   `vec_remove()`'s flag: that function has three callers split 1 : 2, this one
   has forty-one split 40 : 1, so a flag would write `TRUE` at forty
   uninformative sites.
3. **Met, and the answer is that there are none.** All forty-one call sites are
   classified in `0009` decision 3 — the issue's count of "45 in 19 files" is
   wrong; it is **41 in 17**. `com_let()`'s left-hand side is the only one that
   creates the name it looks up. `com_setscale.c:19` is a resolution, as this
   issue guessed, and the `@dev[param]` paths are resolutions whose near miss
   cannot be reached, because `vec_get()` allocates its `@` vectors without
   `VF_PERMANENT`. The audit did find a *third* category the rule does not
   name — a **probe**, a lookup asking whether a name exists in order to decide
   what kind of token it is — at `com_pyplot.c:53` and, for three of its
   callers, `parse.c:575`. Those are `doc/codex/issues/0044` and `0045`.
4. **Met and exceeded.** `tests/regression/casedist/vector-let-report.cir`
   asserts both values, and the criterion's premise that a deck "cannot assert
   on the absence of the warning" is out of date since
   `doc/claude/decisions/0006-diagnostic-deck-coverage.md`: the deck asserts
   four diagnostics as well, each silence beside something in the same capture
   that must be reported. It needs a sourced sub-deck to do it, because `let`
   is on `noredirect[]` (`src/frontend/control.c:62`) and cannot take a `>&`
   of its own — `let a = b > c` needs `>` as an operator.

Two additions to this issue's Impact, measured while closing it: the warning
fires in a session with **no user vectors at all**, because `vec_get()` retries
the const plot and `let PI = 3` near-misses the predefined `pi`; and it reaches
`.meas` and `.csparam`, which run `com_let()` on names the deck wrote.
