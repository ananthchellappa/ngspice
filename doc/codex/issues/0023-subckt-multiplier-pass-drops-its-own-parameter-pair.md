# Issue: The Subcircuit Multiplier Pass Drops the Parameter Pair It Appends

## Status

Fixed, all three criteria met. Found while closing `doc/codex/issues/0020`
gap 1, in the function next to the one that was fixed. Neither half is
case-related and neither changes a number today.

## Summary

`inp_fix_subckt_multiplier()` (`src/frontend/inpcom.c:4346`) appends the
multiplier formal to its caller's arrays and reports the new count as its return
value:

```c
subckt_param_names[num_subckt_params] = copy("m");
subckt_param_values[num_subckt_params] = copy("1");
num_subckt_params++;
...
return num_subckt_params;
```

The one call site (`inpcom.c:4462`, in `inp_fix_inst_calls_for_numparam()`)
**discards** the return value, and its own free loop immediately after runs to
the pre-call count:

```c
inp_fix_subckt_multiplier(subckt_w_params, a->line,
        num_subckt_params, subckt_param_names, subckt_param_values);

for (i = 0; i < num_subckt_params; i++) {
    tfree(subckt_param_names[i]);
    tfree(subckt_param_values[i]);
}
```

Two consequences:

1. **Leak.** The `copy("m")` and `copy("1")` pair is never freed — two small
   allocations for every subcircuit that a caller gives a multiplier to.
2. **One past the end.** `inp_get_params()` exits only when it is asked to
   store assignment number `NPARAMS + 1`, so it can legitimately return exactly
   `NPARAMS` (10000). The write above then addresses index `NPARAMS` of a
   `char *[NPARAMS]` array. Unreachable in practice — it needs 10000
   assignments on one `.subckt` card — but it is a stack write past the end of
   two arrays, not a benign overflow.

The appended pair is otherwise dead: the values the *caller* later hands to
`inp_fix_inst_line()` come from a fresh `inp_get_params()` of the rewritten
`.subckt` card, which by then carries the `m=1` text the same function appended
to the card. So the pair is written, leaked, and never read.

## Impact

A bounded leak per parameterised subcircuit instantiated with `m`, on a
preprocessing path that runs once per deck. It matters for the shared library,
where `ngSpice_Reset` is expected to return the process to a clean state
(`CLAUDE.md`, commits `c5cd68015`, `5ad395d5e`), and not at all for a batch run.
The array write is latent.

## Root Cause

The return value was designed to be used — the function is the only place that
knows the count changed — and the call site was written as if it were `void`.

## Acceptance Criteria

1. The call site either takes the return value or the function stops writing
   into the caller's arrays; the `copy("m")`/`copy("1")` pair is freed on every
   path.
2. The append is bounded, so `num_subckt_params == NPARAMS` cannot write past
   the arrays.
3. `make check` unchanged, and no deck's numbers move: the pair is currently
   never read, so this is a refactor with no behavioural surface.

## Resolution

Fixed by deleting the append rather than by accounting for it, which satisfies
criteria 1 and 2 together: there is no pair left to free and no index left to
bound.

```c
-static int inp_fix_subckt_multiplier(struct names *subckt_w_params,
-        struct card *subckt_card, int num_subckt_params,
-        char *subckt_param_names[], char *subckt_param_values[])
+static void inp_fix_subckt_multiplier(struct names *subckt_w_params,
+        struct card *subckt_card)
```

The three parameters the deleted lines were the only users of go with them, and
the one call site drops the three arguments. The caller's free loop is left
byte-identical: it is correct on its own terms, freeing exactly what
`inp_get_params()` filled at `inpcom.c:4457-4458`.

The alternative — capturing the return value and freeing to the new count —
was rejected because it keeps writing a pair that is never read and still needs
a bound test against `NPARAMS`.

### Why deleting is safe

The Summary's claim that the pair is dead was checked rather than assumed. The
caller's second loop re-reads the parameters from the rewritten `.subckt` card
(`inpcom.c:4503-4504`), which by then carries the `m=1` text this function
appended to the card itself, so a second instantiation of the same subcircuit
finds `m` from the card and not from the array. No reader of an index at or
above `num_subckt_params` exists anywhere: `found_mult_param()`
(`inpcom.c:4333`) and `inp_fix_inst_line()` (`inpcom.c:4297`, `:4304`) both
loop to their count argument. The function's whole observable surface is the
rewrite of `subckt_card->line`, the `add_name()` call, and the `m={m}` appended
to each interior card — none of which this touches.

### Criterion 3, and the evidence that is not a test

`make check` with `casemode` unset is 206 tests, 0 FAIL, with no committed
`.out` touched. There is no failing test for this and there was not going to
be one: the leak is invisible to `make check` and the array write needs 10000
assignments on one card. The evidence is `valgrind --leak-check=full` on
`tests/regression/case/subckt-mult-param-case.cir`, same deck and same build
flags, the only difference between the two runs being this hunk:

```
before: definitely lost: 4 bytes in 2 blocks
after:  definitely lost: 0 bytes in 0 blocks
```

Two loss records of 2 bytes each — `copy("m")` and `copy("1")` — from adjacent
call sites in one stack. The binary carries no symbol table, so valgrind
prints `???` rather than naming `inp_fix_subckt_multiplier`; the attribution is
the size, the count, and the fact that the hunk is the only difference.
