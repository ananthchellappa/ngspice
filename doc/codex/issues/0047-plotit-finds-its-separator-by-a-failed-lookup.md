# Issue: `plotit()` Finds Its `vs` Separator by a Failed Vector Lookup

## Status

Open. Found while closing `doc/codex/issues/0045`
(`doc/claude/decisions/0010-probe-category.md` decision 3). Predates that work
by decades — the code says so itself.

## Summary

`plotit()` parses its whole argument list as one expression and then recovers
the `a vs b` boundaries by looking for the debris of failed lookups
(`src/frontend/plotting/plotit.c:783-796`, its own comment):

```c
        /* Now parse the vectors.  We have a list of the form
         * "a b vs c d e vs f g h".  Since it's a bit of a hassle for
         * us to parse the vector boundaries here, we do this -- call
         * ft_getpnames_quotes() without the check flag, and then look for
         * 0-length vectors with the name "vs"...  This is a sort of a gross
         * hack, since we have to check for 0-length vectors ourselves after
         * evaulating the pnodes...
         */
```

and at `plotit.c:831`:

```c
            if (pn_value && (pn_value->v_length == 0) &&
                eqc(pn_value->v_name, "vs")) {
```

So the separator is recognised **by the failure of a vector lookup on the word
`vs`**, which means the word is looked up at all, which is why
`doc/codex/issues/0045` existed.

## Impact

Three, in increasing order of how much they matter.

1. Under `casemode=distinguish`, a deck with a net named `VS` warned on every
   `plot a vs b`. Fixed by `0045` — but fixed by telling the parser that the
   separator spellings this wordlist holds are probes, which is a mechanism
   that exists only because of the hack.
2. Two degenerate shapes lose a diagnostic as a result. On a deck with a net
   `VS`, `hardcopy f.svg vs` and `hardcopy f.svg v(X) vs` used to print the
   near miss before `Error: misplaced vs arg` / `Error: missing vs arg` and now
   print only the error. `plotit()`'s rule is positional-agnostic, so the probe
   list matching it cannot be narrower.
3. A vector really named `vs` silently changes the meaning of the command.
   Measured: with a net `VS` present under `distinguish`,
   `hardcopy f.svg v(X) VS v(in)` plots **three** curves, because the lookup of
   `VS` succeeds and the word is data rather than a separator. That is not a
   case-mode defect — it is the same under `fold`, where any deck with a net
   named `vs` cannot use `vs` as a separator — but no diagnostic says so.

## Root Cause

The wordlist boundaries are known before the parse and thrown away by
`wl_flatten()`; the separator is then reconstructed from a side effect of the
lookup that flattening made necessary.

## Acceptance Criteria

1. `plotit()` splits its wordlist on standalone `vs` words **before** parsing,
   and parses each segment, so the word never reaches `vec_get()`.
2. The probe list at `plotit.c:807-820` and its `ft_getpnames_quotes_probe()`
   call go away with it; `ft_getpnames_quotes()` is enough again.
   `com_define()` keeps its list, so the entry point stays.
3. Impact 3 is decided explicitly rather than inherited: either a vector named
   `vs` still wins, or the separator wins and the deck is told.
4. `tests/regression/casedist/vector-probe-report.cir`'s VS case passes
   unchanged, and the two shapes of impact 2 get their near miss back.

## Resolution

Not fixed. It rewrites the argument handling of every `plot`, `hardcopy` and
`pyplot` in the tree, it is not about case, and
`doc/claude/decisions/0010-probe-category.md` was already changing two
diagnostics.
