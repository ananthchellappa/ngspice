# Issue: A Wildcard That Matches Exactly One Vector Is Renamed to the Wildcard

## Status

Open, filed 2026-08-13 on branch `ver_50`.

Pre-existing, upstream and mode independent. It reproduces byte for byte on
`/usr/local/bin/ngspice` (`ngspice-46`, no `casemode` support) and on
`build-ver_50/src/ngspice`, under no `-D` and under `fold`, `preserve` and
`distinguish` alike — no name here is looked up across a case boundary.

Filed because it is visible in the client-facing evidence base:
`doc/claude/feedback/ngspice_upstream/repro/run_all.sh` section 2 prints the
variable list of `save_lower.raw`, and the second line of it is this defect. The
caption there now points at this file.

**Found independently by the client on 2026-08-14, before they were told this
file existed**, as **R3** of
`doc/claude/feedback/reply_from_xschem_session/REPLY.md`. That is worth
recording for two reasons: it is the first item in this batch that a consumer
hit without being pointed at it, and their reproduction isolates the trigger
more sharply than the original did. See *Independent reproduction* below. They
have been told the issue number and the root cause in the round-2 response.

## Summary

When `all`, `allv`, `alli` or `ally` matches **exactly one** vector, the result
is renamed to the text of the wildcard, and the name the deck gave the net is
lost from that result.

Measured, a deck whose `.save v(MidNode)` leaves the operating-point plot
holding one vector, `set filetype=ascii`, bare `write`:

```
Variables:
	0	v(midnode)	voltage
	1	v(all)	voltage
Values:
 0	2.250000000000000e+00
	2.250000000000000e+00
```

Two columns, one vector: the same number twice, once under the net's name and
once under the wildcard's. `write f.raw allv` gives `v(allv)` in the same place.

`print` shows the same rename with no second column to soften it:

```
print all    ->  all = 2.250000e+00
print allv   ->  allv = 2.250000e+00
```

where the same command on a plot with two or more vectors prints each vector
under its own name.

The plot itself is **not** corrupted — `display` still lists `midnode` before
and after — because what is renamed is a copy.

### Independent reproduction, 2026-08-14

The client's decks are `repro2/one_save.cir` and `repro2/two_save.cir` under
`doc/claude/feedback/reply_from_xschem_session/`, and they reach the defect
from a `.save` card rather than from a `write` of a one-vector plot. Re-run
here against `build-ver_50/src/ngspice` (build stamp
`Fri Aug 14 20:52:09 UTC 2026`) and `/usr/local/bin/ngspice`:

```
.save v(In)                      -> 0 v(In) voltage      | 1 v(all) voltage
.save v(In) / .save v(MidNode)   -> 0 v(In) voltage      | 1 v(MidNode) voltage
.save v(In) v(MidNode)           -> 0 v(In) voltage      | 1 v(MidNode) voltage
stock ngspice-46, .save v(In)    -> 0 v(in) voltage      | 1 v(all) voltage
```

`No. Variables:` is 2 in every row, which is the sharp form of the defect: the
count is the same whether the second column is a net or the wildcard's text.

**Their isolation of the trigger is better than this file's and is adopted
here.** It is not "a `.save` that misses", not "a bare `write`" and not a
property of any one card: it is the **total count of saved vectors in the deck
being exactly one**. Two `.save` cards and one `.save` card with two tokens are
both clean, measured above, which is what makes `!d->v_link2` in the Root Cause
the whole of the condition — `findvec_all()` chains two or more matches and
exempts them. A deck that saves one signal is the common shape for a schematic
tool plotting a single trace, and it is the only shape that reaches this.

Their consumer-side statement of the cost, kept in their words because it is
the part this file could not have written: the extra column *"reaches a
consumer as a signal indistinguishable from a net: our signal browser lists
`v(all)` beside the real trace whenever the user plots exactly one thing. We
can filter it, but only by name, which is not a defence we like."*

One interaction worth naming, because two issues in this batch push in opposite
directions. `doc/codex/issues/0059` and `doc/codex/issues/0069` both leave a
consumer with a vector-count floor as part of its defence against a bogus
rawfile. This defect means the floor cannot be a simple equality against the
number of tokens the deck saved: a one-signal deck writes two variables. Until
this is fixed, a count check has to expect n, or n+1 when n is 1.

## Impact

- **A rawfile consumer reads a variable that does not exist**, with a duplicate
  of a real column's data under it. `No. Variables:` is 2 where the plot has 1.
- **The net's name is absent from `print`'s output** in the single-vector case,
  which is the case a `.save` of one node produces — a common shape.
- Not silent-wrong-number: the *values* are right in both columns. The defect
  is in the labelling and the column count.
- Reached by the command's default. A bare `write` substitutes `all`
  (`com_write()`, `src/frontend/postcoms.c`), so a deck need not type a wildcard
  to get here.

## Root Cause

Two mechanisms in series; the first is the defect.

1. `ft_evaluate()` (`src/frontend/evaluate.c:80-84`) renames its result to the
   parse node's own text whenever the result is a single vector:

   ```c
   if (node->pn_name && !ft_evdb && d && !d->v_link2) {
       if (d->v_name)
           tfree(d->v_name);
       d->v_name = copy(node->pn_name);
   }
   ```

   That is right for an expression — `print v(a)+v(b)` should be labelled
   `v(a)+v(b)` — and wrong for a wildcard, whose text is not a name for what it
   matched. The `!d->v_link2` test is what makes the bug conditional on the
   match size: `findvec_all()` and its siblings
   (`FINDVEC_ALL_GEN`, `src/frontend/vectors.c:261-294`) chain their results with
   `v_link2`, so two or more matches are exempt and exactly one is not. The
   rename lands on a copy, because `mkvnode()` (`src/frontend/parse.c:656-667`)
   copies every vector in the chain before the pnode takes it, which is why the
   plot survives intact.

2. The duplicate column is `com_write()` reacting correctly to the corrupted
   name. It looks for the plot's scale among the vectors it is about to write
   with `vec_eq()`, which compares *names* (`vec_eq()` →`vec_name_eq()`,
   `src/frontend/vectors.c`). The renamed copy no longer matches `midnode`, so
   `scalefound` stays `FALSE` and the "make sure the default scale is present"
   branch prepends a second copy — this time under the real name.

## Acceptance Criteria

1. A bare `write` of a plot holding exactly one vector produces a rawfile with
   `No. Variables: 1` and that vector's own name.
2. `print all` on the same plot prints the vector's name, not `all`.
3. `print v(a)+v(b)` still labels its column `v(a)+v(b)` — the rename is not
   removed, only withheld from the wildcards.
4. The same holds for `allv`, `alli` and `ally`, and for the `alle` event
   wildcard under XSPICE.
5. A deck in `tests/regression/misc/` covers 1 and 2 with `set filetype=ascii`
   so the column count and the names are in the compared output, and the
   two-vector case is a control in the same deck.

## Resolution

**None yet.** The change most likely to be right is to teach the rename to skip
a node whose text is one of the wildcards — the set `get_all_type()`
(`src/frontend/vectors.c:105`) already recognises, which is also the set that
chains its results — rather than to change `com_write()`, which is behaving
correctly given the name it is handed. `ft_evaluate()` is on the path of every
expression in the control language, so the change wants the whole regression
suite behind it and a deck of its own; it is out of scope for the case-mode
batch that found it.

Not to be confused with `doc/codex/issues/0059`, which is about a bare `write`
producing a file of the *wrong plot*. This one is about the wrong *name* inside
a file of the right plot, and the two are independent: `0059`'s guards refuse
before this can happen, and after them this still happens.
