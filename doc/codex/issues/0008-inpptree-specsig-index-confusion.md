# Issue: `mkspecial()` Indexes the Value Table With the Special-Signal Index

## Status

Open

## Summary

`mkspecial()` in `src/spicelib/parser/inpptree.c` registers a special signal
(`ft_sim->specSigs`, for example a `.noise` `onoise`/`inoise` reference inside
a B-source expression) by appending an `IF_STRING` entry to the parse task's
`values`/`types` arrays. Three of the four subscripts in that block use `i`,
the index of the match in `specSigs`, where the index of the slot in `values`
is meant:

```c
/* src/spicelib/parser/inpptree.c:1359-1381 */
for (i = 0; i < ft_sim->numSpecSigs; i++)
    if (!strcmp(ft_sim->specSigs[i], buf))
        break;
if (i < ft_sim->numSpecSigs) {
    for (j = 0; j < numvalues; j++)
        if ((types[j] == IF_STRING) && !strcmp(buf, values[i].sValue))
            break;                            /* values[j] intended */
    if (j == numvalues) {
        if (numvalues) {
            values = TREALLOC(IFvalue, values, numvalues + 1);
            types  = TREALLOC(int,     types,  numvalues + 1);
        } else {
            values = TMALLOC(IFvalue, 1);
            types  = TMALLOC(int, 1);
        }
        values[i].sValue = TMALLOC(char, strlen(buf) + 1);   /* [numvalues] */
        strcpy(values[i].sValue, buf);                       /* [numvalues] */
        types[i] = IF_STRING;                                /* [numvalues] */
        numvalues++;
    }
    p->valueIndex = i;
    p->type = PT_VAR;
    return (p);
}
```

The arrays have just been grown to `numvalues + 1` elements, so the only
in-bounds write is at subscript `numvalues`. The loop over `j` also reads
`values[i].sValue` while testing `types[j]`, so the type tag and the string it
guards come from different entries.

`p->valueIndex = i` is consistent with the intent that a `PT_VAR` node's
`valueIndex` is a `specSigs` index rather than a `values` index, which is what
makes the confusion easy to miss: one of the four uses of `i` is correct.

## Impact

The write is in bounds only when `i == numvalues`, that is, when the matched
special signal happens to sit at the same ordinal as the next free value slot.
That is true for the common case of the first special signal in an expression
with no prior string values (`i == 0`, `numvalues == 0`), which is why the code
survives the current test suite.

Outside that case the consequences are, in order of severity:

- `i > numvalues`: a heap write past the end of the freshly reallocated array —
  `values[i].sValue` and `types[i]` land in unowned memory.
- `i < numvalues`: an existing `values` entry is silently overwritten with the
  special-signal name and re-tagged `IF_STRING`, corrupting whichever earlier
  parameter occupied that slot.
- The duplicate check reads `values[i].sValue` for every `j`, so it either
  reports a spurious duplicate (and skips the append entirely, leaving
  `values`/`types` short of what `valueIndex` will later index) or reads an
  uninitialised pointer when slot `i` has never been written.

No diagnostic is produced in any of these cases.

Reaching it requires a simulator whose `specSigs` table has more than one entry
or a B-source expression that already carries a string-valued parameter before
the special signal is seen, so it is latent rather than routinely hit.

## Root Cause

A single loop variable is doing two jobs. `i` is the search cursor for
`ft_sim->specSigs` and is deliberately kept for `p->valueIndex`; `j` is the
search cursor for `values`. The append block was written against `i` because
the surrounding function uses `i` for every other lookup, and the two indices
coincide in the case anyone ever ran.

## Acceptance Criteria

1. The three writes use the index of the appended slot (`numvalues`, which
   equals `j` on that path), and the duplicate test reads `values[j].sValue`.
2. `p->valueIndex` continues to carry the `specSigs` index, since that is what
   `PT_VAR` evaluation consumes — this is not part of the fix.
3. A regression deck that puts two special signals, or a string parameter plus
   a special signal, into one B-source expression produces the same numbers
   before and after, and does not trip an ASan heap-buffer-overflow.
4. `make check` unchanged.

## Resolution

Not fixed here. Found while auditing `strcmp` sites for Phase 1 of the
case-sensitivity work (`doc/claude/checklists/phase1-keyword-case-census.md`);
it is an indexing defect, not a case defect, and fixing it needs a regression
deck that the Phase 1 series has no reason to carry.
