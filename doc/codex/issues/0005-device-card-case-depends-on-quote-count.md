# Issue: Device Card Case Preservation Depends On Quote Count

## Status

Open

## Summary

`keep_case_of_cider_param()` preserves the case of quoted text only when the
line contains **exactly two** double quotes. A card carrying two quoted
parameters has four quotes, falls through the test, and is lower-cased in its
entirety, including both quoted strings.

The function is reached for XSPICE code-model cards from `is_xspice_model()`,
directly beneath a comment that states the opposite intent:

```c
/* lower case excluded for text in quotes for .model of code models
   filesource, rable2d, table3d, d_state, d_source, d_process, d_cosim */
else if (is_xspice_model(buffer)) {
    s = keep_case_of_cider_param(buffer);
}
```

So the behaviour the comment describes holds for a card with one quoted
parameter and silently stops holding when a second one is added.

## Impact

On a case-sensitive filesystem the affected parameters name files that do not
exist. Two of the listed code models routinely carry two quoted parameters:

- `d_cosim` with both `simulation=` and `sim_args=`;
- `filesource` with a file name and a further quoted parameter.

The two affected parameters fail differently, and one of them fails silently:

- `simulation=` is opened by ngspice, so a mangled path at least produces
  `d_cosim failed to load simulation binary ...`.
- `sim_args=` is passed through to the code model. ngspice reports nothing at
  all. The run completes, the analog rawfile is correct, and whatever the code
  model was supposed to write under that name is either written under the
  lower-cased name or not written at all, depending on whether the containing
  directory happens to exist in lower case.

In both cases the simulation exits 0.

## Reproduction

Two decks differing only in the presence of a second quoted parameter, with a
code model present on disk as `./Mixed.so`:

```
* two quotes -- case preserved, model loads
.model counter d_cosim simulation="./Mixed.so" delay=1p

* four quotes -- whole line folded
.model counter d_cosim simulation="./Mixed.so" sim_args=["Tr.vcd"] delay=1p
```

```sh
$ ngspice -b two.cir
(no diagnostic; the model loads)
$ echo $?
0

$ ngspice -b four.cir
Instance: a1   Message: d_cosim failed to load simulation binary ./mixed.so.
$ echo $?
0
```

Deleting only the `sim_args=` term from `four.cir` restores the first
behaviour, so the difference is the quote count and not the content of either
string.

Measured on ngspice-46 and on the current master snapshot (46+), Ubuntu
24.04.4, x86-64. Both binaries behave identically.

## Root Cause

`src/frontend/inpcom.c:223`, `keep_case_of_cider_param()`:

```c
int numq = 0, keep_case = 0;
...
for (s = buffer; *s; s++)
    if (*s == '\"')
        numq++;

if (numq == 2) {
    ...
    if (*s == '\"')
        keep_case = (keep_case == 0 ? 1 : 0);
    if (!keep_case)
        ...tolower...
}
```

The toggle logic inside the block is already general: it flips `keep_case` on
every quote and folds only what lies outside a quoted region. It is the guard
that is too narrow. With `numq` equal to four the block is skipped and the
caller's fallback lower-cases the whole line.

The callers are `src/frontend/inpcom.c:1812`, `:1815` (CIDER) and `:1820-1822`
(XSPICE code models).

## Acceptance Criteria

- Text inside every quoted region of an affected card keeps its case,
  independent of how many quoted regions the line contains.
- Text outside quoted regions is still lower-cased exactly as today.
- A card with exactly two quotes behaves bit-for-bit as it does now.
- A line with an odd number of quotes is malformed; it is either diagnosed or
  handled deterministically, and the chosen behaviour is stated in a comment.
- A regression covers a `d_cosim` card carrying both `simulation=` and
  `sim_args=`, asserting that both strings survive with their case intact.
- `git diff --check` passes.

## Implementation Plan

1. Add a regression under `tests/regression/misc/` (or `tests/xspice/`) that
   sources a `.model ... d_cosim` card with two quoted parameters, both
   containing capitals, and asserts the case is preserved. Confirm it fails
   against the current code (RED).
2. Replace the `numq == 2` guard with a check that the quote count is even and
   non-zero, leaving the existing toggle loop to do the work. The loop already
   handles any number of quoted regions.
3. Decide and document the odd-count case. Folding the whole line, as today,
   is defensible for a malformed card; whichever is chosen, say so where the
   guard used to be.
4. Re-run the new regression (GREEN), then the XSPICE suite and
   `make -C tests check`.
5. Finish with `git diff --check`.

## Design Note

The function's name refers to CIDER, but its XSPICE caller is what makes this
reachable for ordinary code-model users. Renaming it is out of scope for the
fix; a comment noting the second caller would make the next reader's job
easier. The comment above `is_xspice_model()` should also stop promising
behaviour the function only sometimes delivers.
