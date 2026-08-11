# Issue: `let` Warns About a Case Near-Miss When It Is *Defining* a Name

## Status

Open. Found while closing `doc/codex/issues/0027`. Predates that work: the
warning arrived with Phase 3 gate 3 and `com_let()` has always resolved its
left-hand side through `vec_get()`.

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

Not fixed. Recorded in `doc/claude/decisions/0004-unlet-vector-identity.md`
under what that decision does not decide. It is a false positive rather than a
wrong number, which is why `0027`'s fix did not wait for it, but it is the
loudest remaining case-mode defect in the frontend and it fires on the deck the
mode exists to support.
