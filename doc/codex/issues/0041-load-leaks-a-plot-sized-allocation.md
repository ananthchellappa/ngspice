# Issue: `load` Leaks One Plot-Sized Allocation

## Status

Open, not fixed, and **not** a case issue. Found while checking the decks of
`doc/codex/issues/0037` under valgrind, which was done because that work adds
two dynamic strings to `com_diff()`. Those are freed; this is somewhere else
and predates the work.

## Summary

A deck that `load`s a rawfile leaks one block. Measured at `28d36a7c4`, the
binary this session started from, on
`tests/regression/misc/diff-pairing-case.cir` with its `diff` lines removed so
that only `load` runs:

```
definitely lost: 80 bytes in 1 blocks
498 (80 direct, 418 indirect) bytes in 1 blocks are definitely lost
ERROR SUMMARY: 1 errors from 1 contexts
```

The same deck with `diff` restored leaks the same one block, and it is
byte-identical between the binary at `28d36a7c4` and the binary after `0037`
and `0040`, so `com_diff()` is not the site. A control deck with no `load`
(`tests/regression/misc/fold-mixed-case.cir`) is clean: `definitely lost: 0
bytes in 0 blocks`.

One block for a deck that loads **two** rawfiles, which is what makes the size
suggestive: 80 direct bytes with 418 indirect reads like one `struct plot` and
its strings, not one per file.

## Impact

Small and bounded per `load` in the `ngspice` binary, where the process exits
soon after. The reason to record it rather than ignore it is
`src/sharedspice.c`: anything that accumulates across `ngSpice_Reset` in the
shared library is the class of defect commits `c5cd68015` and `5ad395d5e`
fixed, and a host that loads rawfiles in a loop would accumulate this one.
Not demonstrated in the shared build; the measurement above is the `ngspice`
binary.

## Root Cause

Not determined. The configured tree at `build-ver_50` is built without `-g`
and the binary carries no symbol table, so valgrind reports the stack as six
frames of `???`. Symbolising it needs a debug build
(`../configure --enable-debug`), which was out of scope for the session that
found it.

The stack is 6 frames deep below `main` and allocates with `calloc`, which
narrows it to the rawfile reader's own allocations rather than to a `tmalloc`
call site.

## Acceptance Criteria

1. The site is identified from a debug build, not guessed.
2. `load` of one rawfile, then of two, leaks nothing in `--leak-check=full`.
3. The check is repeated in the shared build across a `ngSpice_Reset`, since
   that is where the impact argument lives.

## Resolution

Not fixed.
