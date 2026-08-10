# Issue: The Subcircuit Multiplier Pass Drops the Parameter Pair It Appends

## Status

Open. Found while closing `doc/codex/issues/0020` gap 1, in the function next to
the one that was fixed. Neither half is case-related and neither changes a
number today.

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

Not fixed. Out of scope for the session that found it, whose scope was
`doc/codex/issues/0015`, `0020` gap 1 and `0013`'s three exact-match sites, and
it is not a case-mode defect at all. Recorded here rather than folded into the
gap 1 commit so that commit stays one mechanism.
