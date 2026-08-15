# Issue: A Wildcard That Matches Exactly One Vector Is Renamed to the Wildcard

## Status

**Fixed 2026-08-15** on branch `ver_50`, in the narrow scope the repo owner
chose: option (i) of *Scope* below, the rename and nothing else. Filed
2026-08-13. What shipped, and what was deliberately left behind, is in
*Resolution*.

The second mechanism this file measured — `com_write()`'s scale prepend — is
**not** fixed. It survives the fix, it is a separate defect, and it is filed
under its own number by item 5 of
`doc/claude/batches/2026-08-15-xschem-open-items/PLAN.md`.

**That number is `doc/codex/issues/0073`**, filed 2026-08-15. It re-measures
this file's table against the fixed tree and supersedes it for the rows
`com_write()` owns: it confirms the byte-identical-name duplicate, adds the
`.dc` round-trip case where the pair is identical under `fold` and on stock too,
and **corrects** this file's statement that `.tran`/`.dc`/`.ac` never inflate
the count — they do, whenever the deck names the scale itself in the `v()` form
the rawfile reports.

Pre-existing, upstream and mode independent. It reproduces byte for byte on
`/usr/local/bin/ngspice` (`ngspice-46`, no `casemode` support) and on
`build-ver_50/src/ngspice`, under no `-D` and under `fold`, `preserve` and
`distinguish` alike — no name here is looked up across a case boundary.

**Re-measured 2026-08-15**, and the measurement widened the issue: the extra
column is not wildcard-specific, and the `-r` batch route does not have it at
all. See *Re-measured 2026-08-15* below. Nothing here is withdrawn — the
wildcard rename is still real and still the sharpest symptom — but the scope of
a fix is now a decision for the repo owner and is stated at the end of
*Resolution*. Status stayed Open, unfixed and unscoped when that measurement
landed; the owner chose option (i) the same day, and the rename is now fixed.

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

That rule of thumb is **wrong**, and the 2026-08-15 measurement below is what
corrects it. The floor depends on the analysis and on what the `write` names,
not on the saved count: a `.tran` deck that saves one signal writes two
variables *correctly*, and an `.op` deck that saves two and writes both by name
writes three. The corrected advice is in the same section.

### Re-measured 2026-08-15: the shape is wider than this file described

Measured against `build-ver_50/src/ngspice`, build stamp
`Sat Aug 15 15:34:32 UTC 2026`, default `casemode=fold`, `set filetype=ascii`,
in a scratch directory outside the repo. One netlist throughout —
`Vs In 0`, `R1 In MidNode 1k`, `R2 MidNode 0 3k`, the source `DC 3` where only
an operating point is run and `pulse(0 3 0 1n 1n 1u 2u)` where a sweep is —
varying only the analysis card, the `.save` card and the `write` argument list.
`write` runs inside `.control` after `run` unless the row says `-r`. Only the
column count and the names are read; the values are not at issue and are
correct in every row.

`No. Vars` is the file's `No. Variables:`; `rows` is the variable table.

| analysis | `.save` | `write` argument | No. Vars | rows |
|---|---|---|---|---|
| `.op` | `v(In)` | *(none — bare `write f.raw`)* | 2 | `v(in)` `v(all)` |
| `.op` | `v(In)` | *(no filename either — bare `write`)* | 2 | `v(in)` `v(all)` |
| `.op` | `v(In)` | `all` | 2 | `v(in)` `v(all)` |
| `.op` | `v(In)` | `allv` | 2 | `v(in)` `v(allv)` |
| `.op` | `v(In)` | `v(In)` | 2 | `v(in)` `v(In)` |
| `.op` | `v(In)` | `v(in)` | 2 | `v(in)` `v(in)` |
| `.op` | `v(In)` | `in` | **1** | `v(in)` |
| `.op` | `v(In)` | `v(In)+0` | 2 | `v(in)` `v(In)+0` |
| `.op` | `v(In)` | `set plainwrite`, then `in` | **1** | `v(in)` |
| `.op` | `v(In)` | `set plainwrite`, then bare | **1** | `v(in)` |
| `.op` | `v(In) v(MidNode)` | *(bare)* | 2 | `v(in)` `v(midnode)` |
| `.op` | `v(In) v(MidNode)` | `all` | 2 | `v(in)` `v(midnode)` |
| `.op` | `v(In) v(MidNode)` | `allv` | 2 | `v(in)` `v(midnode)` |
| `.op` | `v(In) v(MidNode)` | `in midnode` | 2 | `v(in)` `v(midnode)` |
| `.op` | `v(In) v(MidNode)` | `v(In) v(MidNode)` | **3** | `v(in)` `v(In)` `v(MidNode)` |
| `.op` | `v(In) v(MidNode)` | `v(In)` | 2 | `v(in)` `v(In)` |
| `.op` | `v(In) v(MidNode)` | `v(MidNode)` | 2 | `v(in)` `v(MidNode)` |
| `.op` | `v(In) v(MidNode)` | `set plainwrite`, then `midnode` | 2 | `v(in)` `v(midnode)` |
| `.op` | `v(In)` | `-b -r f.raw`, no `.control` | **1** | `v(in)` |
| `.op` | `v(In) v(MidNode)` | `-b -r f.raw`, no `.control` | 2 | `v(in)` `v(midnode)` |
| `.tran 1u 2u` | `v(In)` | *(bare)* | 2 | `time` `v(in)` |
| `.tran 1u 2u` | `v(In)` | `all` | 2 | `time` `v(in)` |
| `.tran 1u 2u` | `v(In)` | `allv` | 2 | `time` `v(allv)` |
| `.tran 1u 2u` | `v(In)` | `ally` | 2 | `time` `v(ally)` |
| `.tran 1u 2u` | `v(In)` | `v(In)` | 2 | `time` `v(In)` |
| `.tran 1u 2u` | `v(In)` | `time v(In)` | 2 | `time` `v(In)` |
| `.tran 1u 2u` | `v(In)` | `set plainwrite`, then `in` | 2 | `time` `v(in)` |
| `.tran 1u 2u` | `v(In) v(MidNode)` | *(bare)* | 3 | `time` `v(in)` `v(midnode)` |
| `.tran 1u 2u` | `v(In) v(MidNode)` | `v(In) v(MidNode)` | 3 | `time` `v(In)` `v(MidNode)` |
| `.tran 1u 2u` | `v(In)` | `-b -r f.raw`, no `.control` | 2 | `time` `v(in)` |
| `.dc Vs 0 3 1` | `v(In)` | *(bare)* | 2 | `v(v-sweep)` `v(in)` |
| `.dc Vs 0 3 1` | `v(In)` | `v(In)` | 2 | `v(v-sweep)` `v(In)` |
| `.ac dec 5 1 1k` | `v(In)` | `v(In)` | 2 | `frequency` `v(In)` |

Four things in that table are new, and each of them changes the issue.

**1. The `-r` batch route is already clean, and the client has not been told.**
`ngspice -b -r f.raw deck.cir` on the one-save `.op` deck writes
`No. Variables: 1` and the row `v(in)`. It never reaches `com_write()` —
`fileInit()`/`fileInit_pass2()` in `src/frontend/outitf.c` write the header
straight from the analysis's own vector set — so neither mechanism below can
touch it. **This is a usable workaround today**: a deck that produces its
rawfile through `-r` instead of through a `.control` `write` sees the invariant
hold, with no phantom column and no duplicate. Its one known cost is
`doc/codex/issues/0071`: the `-r` writer emits no `Option: casemode=` line, so
a consumer that reads the mode out of the header loses it on that route.

**2. The extra column is not wildcard-specific.** `write f.raw v(In)` on the
one-save `.op` plot writes two variables, and two explicit names on a two-save
`.op` plot write **three**. No wildcard is typed in either. The summary at the
top of this file describes one symptom of a mechanism with a wider reach.

**3. Where the deck's own spelling is used, the duplicate cannot be filtered by
name.** Re-measured across the three modes and on stock, `.op` with
`.save v(In)` and `write f.raw v(In)`:

```
fold          -> 0 v(in)   | 1 v(In)
preserve      -> 0 v(In)   | 1 v(In)
distinguish   -> 0 v(In)   | 1 v(In)
ngspice-46    -> 0 v(in)   | 1 v(in)
```

Under `preserve`, under `distinguish` and on stock the two columns carry a
**byte-identical name**. The client's stated defence — "we can filter it, but
only by name" — works against `v(all)` and does not work here. That is a
consumer-visible escalation of the same defect and it should be in whatever the
client is told next.

**4. On `.tran`, `.dc` and `.ac` the count is never inflated, but the name is
still corrupted.** `write f.raw allv` on a one-voltage `.tran` plot writes
`time` and `v(allv)`: two variables, which is arithmetically what a consumer
expects for one signal plus its axis, under a name that names nothing. A count
check passes and the signal browser still shows a phantom.

**The corrected count rule for a consumer**, replacing the "n, or n+1 when n
is 1" above: for `.tran`, `.dc` and `.ac` a bare `write` gives *n+1* — the
saved signals plus the analysis axis — for every n. For `.op` a bare `write`
gives *n*, except that n=1 gives 2. Naming vectors explicitly on the `write`
line adds one more on an `.op` plot in every case. The `-r` route gives n+1 for
`.tran`/`.dc`/`.ac` and n for `.op`, with no exception at n=1.

Two side observations from the same session, recorded so they are not
re-derived:

- `write f.raw ally` on the one-save `.op` plot writes **12 variables** —
  `yes FALSE TRUE boltz c e echarge i kelvin no pi planck` — because
  `findvec_ally()` excludes the plot's scale, the `.op` plot's only vector *is*
  its scale (see mechanism 2 below), the match is therefore empty, and the
  lookup falls through to the constants plot. That is
  `doc/codex/issues/0059`'s shape, not this one's, and is recorded here only as
  corroboration of it.
- `write f.raw TIME v(In)` under `casemode=distinguish` is refused —
  `no vector named 'TIME'; 'time' differs only in case` — so there is no row
  for it. Under `fold` it succeeds and labels the axis column `TIME`, the
  deck's spelling, which is mechanism 1 acting on the scale itself.

`alle` was **not** measured. By code reading it belongs in the wildcard set:
`get_all_type()` recognises it (`src/frontend/vectors.c:143-146`), `findvec()`
dispatches to `findvec_alle()` before any name lookup (`:183-186`), and
`findvec_alle()` chains its results through `v_link2`
(`:329-343`) exactly as `FINDVEC_ALL_GEN` does, so it carries the same
exemption at two or more matches and the same exposure at exactly one. It was
not run because every XSPICE digital deck in `build-ver_50/tests/xspice/digital`
segfaults in that tree today: the `*.cm` code models there are stamped
`Aug 15 08:36` and the binary `Aug 15 09:36`, i.e. the models are stale
relative to the binary. That is a build-directory artifact, not a product
defect, and a `make` in that tree is expected to clear it. It was not
investigated further.

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

Added 2026-08-15, from the re-measurement above:

- **A deck need not type a wildcard at all.** `write f.raw v(In)` inflates the
  same one-save `.op` plot to 2, and `write f.raw v(In) v(MidNode)` inflates a
  two-save `.op` plot to 3. The wildcard is one entry point among several.
- **The duplicate is not always distinguishable by name.** Under `preserve`,
  under `distinguish` and on stock `ngspice-46`, the two columns of
  `write f.raw v(In)` carry a byte-identical name. Filtering by name — the
  client's stated defence — does not reach that case.
- **The name is corrupted on `.tran`/`.dc`/`.ac` too**, where the count is not
  inflated. `write f.raw allv` on a one-voltage `.tran` plot writes `time` and
  `v(allv)`: a count check passes and a phantom signal still reaches the
  browser.
- **The `-r` batch route is unaffected**, so a consumer has a workaround today.

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

### Which mechanism produces which row, measured 2026-08-15

The two mechanisms above are **separable, and each of them produces inflated
rows on its own.** That is the finding that decides whether this is one defect
or two, so the isolating measurements are given rather than asserted.

**Mechanism 1 — the `ft_evaluate()` rename, `src/frontend/evaluate.c:80-84`.**
It replaces the *stored* vector name with the *typed* parse-node text on every
single unchained result, wildcard or not. Three measurements pin it:

- `write f.raw in` on the one-save `.op` plot — the same code path, the same
  rename, but the typed text `in` happens to equal the stored name — writes
  **1** variable. `write f.raw v(in)`, differing only in the `v()` wrapper the
  deck typed, writes **2**. So the trigger is not case, not the wildcard and
  not the match size: it is the typed text differing from the stored name, and
  for `v(X)` syntax on a plot whose vectors are stored bare it always does.
- `set plainwrite` (`src/frontend/postcoms.c:633-650`) takes the `vec_get()`
  path and never calls `ft_evaluate()`. `set plainwrite` + `write f.raw in`
  writes **1** variable where the pnode path writes 2, with everything else
  identical.
- `write f.raw all` on a **two**-vector `.op` plot writes 2 correct rows: the
  chain sets `v_link2`, the rename is withheld, the stored names survive and
  the scale is recognised.

Mechanism 1 is therefore responsible for every row in the table whose name is
not the stored name: `v(all)`, `v(allv)`, `v(ally)`, `v(In)`, `v(MidNode)`,
`TIME`. It causes an inflated count only when the requested set contains the
plot's scale, because that is when hiding a name hides the scale.

**Mechanism 2 — `com_write()`'s scale insurance,
`src/frontend/postcoms.c:681-696`.** `vec_eq(d, tpl->pl_scale)` compares
basenames (`vec_eq()` → `vec_basename()` + `vec_name_eq()`,
`src/frontend/vectors.c:1278-1297`); if the plot's scale is not among what is
about to be written, the block at `:692-696` prepends a copy of it. On `.tran`,
`.dc` and `.ac` that is correct and necessary — the scale is `time`,
`v-sweep` or `frequency`, a real axis the file would be unreadable without.

On an **`.op` plot it is not an axis at all.** `vec_new()`
(`src/frontend/vectors.c:1122-1124`) makes the *first* `VF_PERMANENT` vector
created in a plot its default scale, so on an `.op` plot the "scale" is
whichever saved node voltage happened to be created first. `display` confirms
it: `in : voltage, real, 1 long [default scale]`. Prepending it adds an
ordinary signal the `write` line did not name.

The isolating measurement is the row `set plainwrite` + `write f.raw midnode`
on the two-save `.op` plot: **2** variables, `v(in)` and `v(midnode)`, with the
rename structurally impossible on that path. One signal asked for, two written,
mechanism 1 not involved.

**How they compose.** On a one-save `.op` plot both fire: mechanism 1 renames
the only vector, so mechanism 2 no longer recognises it as the scale and
prepends a second copy of the same data. That composition — and only that
composition — produces the duplicate-under-two-names column this issue was
filed about.

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

### Shipped 2026-08-15 — option (i), the rename and nothing else

`ft_evaluate()` no longer copies the parse node's text over its result's name
when that text is one of the wildcards. The test is
`vec_is_all_wildcard()`, a three-line publication of the existing
`get_all_type()` (`src/frontend/vectors.c`, declared in
`src/include/ngspice/fteext.h`), so the exempt set is not a second list: it is
by construction the same set `findvec()` intercepts before any name lookup, and
therefore the same set whose results are chained through `v_link2` when two or
more match. `all`, `allv`, `alli`, `ally` and `alle` are all exempt, and the
match is case-insensitive because `get_all_type()` is — `write f.raw ALL` is
the same wildcard as `write f.raw all`, which is what
`tests/regression/misc/all-wildcard-case.cir` already asserts one call deeper.

**`alle` is in the set, deliberately.** Not from the name but from the callers:
`get_all_type()` returns `ALL_TYPE_ALLE` for it (`src/frontend/vectors.c:143`),
`findvec()` dispatches it to `findvec_alle()` before any name lookup
(`:184-186`, under `XSPICE`), and `findvec_alle()` (`:299-343`) builds its
result by writing `v_link2` exactly as `FINDVEC_ALL_GEN` does — so a run with
exactly one event node reaches `ft_evaluate()` with `v_link2 == NULL` and had
the same exposure as `all`. Excluding it would have left one wildcard in five
behaving differently from the other four for no reason a reader could find in
the code. A live single-event-node measurement was attempted and is **not**
reported here because `alle` could not be made to produce output in this tree
at all: `print alle` on a one-event-node deck says *"Warning from checkvalid:
vector alle is not available or has zero length"*, and on a two-event-node deck
prints nothing. That is a separate matter on the `alle` path, was not
investigated, and is unrelated to the rename.

**RED first.** `tests/regression/misc/wildcard-rename.cir` was written and run
before the change. Its failure, verbatim from
`make -C build-ver_50/tests/regression/misc check TESTS=wildcard-rename.cir`:

```
--- wildcard-rename.out_tmp	2026-08-15 10:58:35.905559007 -0700
+++ wildcard-rename.test_tmp	2026-08-15 10:58:35.901558991 -0700
@@ -5,15 +5,15 @@
 
 
 WCR-ONE-PRINT-ALL
-in = 1.000000e+00
+all = 1.000000e+00
 WCR-ONE-PRINT-ALLV
-in = 1.000000e+00
+allv = 1.000000e+00
 ASCII raw file "wcr_one.raw"
 WCR-ONE-WRITE-HOLDS-NET
-WCR-ONE-WRITE-NO-WILDCARD
+WCR-ONE-WRITE-HOLDS-WILDCARD
 ASCII raw file "wcr_allv.raw"
 WCR-ALLV-WRITE-HOLDS-NET
-WCR-ALLV-WRITE-NO-WILDCARD
+WCR-ALLV-WRITE-HOLDS-WILDCARD
 WCR-TWO-PRINT-ALL
 in = 1.000000e+00
 wcr_b = 2.000000e+00
FAIL: wildcard-rename.cir
```

Four lines, and only four: the two controls in the same deck — the same
wildcard on a two-vector plot, and `print v(in)+wcr_b` — already matched before
the change and still match after it, which is the evidence that the rename was
withheld and not removed.

**Acceptance criteria 1 to 4 are met**, re-measured against the fixed binary:

| deck | before | after |
|---|---|---|
| `.op`, `.save v(in)`, bare `write` | `No. Variables: 2`, `v(in)` `v(all)` | `No. Variables: 1`, `v(in)` |
| `.op`, `.save v(in)`, `print all` | `all = 1.000000e+00` | `in = 1.000000e+00` |
| `.op`, `.save i(v1)`, `write f alli` | `No. Variables: 2`, `i(v1)` `i(alli)` | `No. Variables: 1`, `i(v1)` |
| `.op`, `.save i(v1)`, `print alli` | `alli = -1.00000e-03` | `v1#branch = -1.00000e-03` |
| `.tran`, `.save v(in)`, `write f allv` | `time` `v(allv)` | `time` `v(in)` |
| `.tran`, `.save v(in)`, `write f v(In)` | `time` `v(In)` | `time` `v(In)` — unchanged |
| `.tran`, `.save v(in)`, `write f v(In)+0` | `time` `v(In)+0` | `time` `v(In)+0` — unchanged |
| `.op`, `.save v(in)`, `write f v(In)` | `v(in)` `v(In)` | `v(in)` `v(In)` — unchanged |

Every "before" in that table was run, not carried over: the pre-fix binary was
rebuilt from `git checkout src/frontend/evaluate.c` for the two rows the
predecessor batch's table did not contain (`alli`, and `v(In)+0` on `.tran`),
and the fix restored and re-verified afterwards.

Criterion 5 is met by `tests/regression/misc/wildcard-rename.cir`, with the
two-vector case as a control in the same deck. Criterion 4's `alle` half is met
by code, not by a deck, for the reason given above.

**What was deliberately left.** Mechanism 2 — `com_write()`'s scale prepend,
`src/frontend/postcoms.c:681-696` — is untouched, and `src/frontend/postcoms.c`
was not edited at all. It still adds a column to any *partial* write of an `.op`
plot: measured after the fix, `.op` + `.save v(in)` + `write f.raw v(In)` still
writes `No. Variables: 2`, `v(in)` and `v(In)`, and under `preserve`,
`distinguish` and stock those two columns still carry a byte-identical name. It
is a separate defect with its own issue number — **`doc/codex/issues/0073`**,
filed by item 5 of this batch.
The consequence for a consumer is unchanged from what *Scope* option (i)
predicted: a bare `write` is now clean, and naming vectors on the `write` line
is not.

The test deck asserts **names, not column counts**, for exactly that reason. A
count assertion would have been an assertion about `com_write()`.

### The analysis that led there

*Everything below this line is as it was written before the fix, including the
three scopes the owner chose between. Nothing in it is withdrawn; the option
the owner took is (i).*

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

### Scope: a decision for the repo owner, 2026-08-15

The measurement above leaves three defensible scopes and no obvious winner. The
choice is the repo owner's; this section states the options and their costs and
does not pick.

The acceptance criteria above were written for option (i) and are still correct
for it. Options (ii) and (iii) would need criteria of their own.

**Option (i) — this issue as filed. Withhold the rename from the wildcard
tokens only.** `ft_evaluate()` skips the assignment when the node's text is one
of the set `get_all_type()` recognises.

- *Closes*: every row whose name is `v(all)`, `v(allv)`, `v(ally)` — including
  the bare-`write` default, which is the shape a generated deck produces, and
  including the `.tran` name corruption at rows `allv` and `ally`. A bare
  `write` then gives n on `.op` and n+1 on `.tran`/`.dc`/`.ac`, matching what
  `-r` already writes.
- *Leaves*: every row where the deck names vectors explicitly. `.op` +
  `write f.raw v(In)` still writes 2, and the two columns still carry an
  identical name under `preserve`, `distinguish` and stock — the case the
  client cannot filter.
- *Breaks*: nothing found in the tree. No `.out` reference contains a wildcard
  as a variable name or a `print` label (grepped). A deck cannot have a real
  vector named `all`: `findvec()` intercepts the wildcard set at
  `src/frontend/vectors.c:173-189`, before any name lookup, so such a vector is
  already unreachable and withholding the rename cannot regress it.
- *Regression risk*: **low.** `ft_evaluate()` is on the path of every
  control-language expression, but the guard is a text test on the parse node
  and fires only for five reserved words that are already special-cased one
  call deeper. Still wants the full `make check` behind it and a deck of its
  own, per the note above.
- *Sufficient for the client?* For their current deck shape, yes — their
  generated decks use a bare `write`. Not sufficient if they ever name vectors
  on the `write` line.

**Option (ii) — the wider shape. Make `No. Variables` equal what the deck asked
for in every case.**

- *First, the invariant as literally stated is not achievable and should not
  be.* A `.tran` rawfile that carries `v(in)` and no `time` is unplottable, and
  a consumer that reads it has no abscissa. The rawfile format requires the
  plot's scale. The defensible restatement is: **the scale is written when it
  is a real axis, and is never written twice under two names.** The owner
  should settle the wording before the scope, because the two readings imply
  very different changes.
- *What it needs beyond (i)*: the rename has to stop hiding the scale for
  explicitly named vectors too, or `com_write()` has to stop identifying the
  scale by name. Two levers exist and neither is designed here — this crew was
  told not to write the fix. Naming them only so the cost is visible: narrow
  the rename further (a mechanism-1 change), or give `com_write()` a way to
  recognise the scale by identity rather than by name before the rename lands
  (a mechanism-2 change, `src/frontend/postcoms.c:665-696`).
- *Breaks*: **a client-visible behaviour they may value.** Under `fold` the
  stored name is `in`, and `write f.raw v(In)` writes the column as `v(In)` —
  the deck's own spelling — *because of* the rename. Narrowing the rename makes
  that column `v(in)`. Any consumer relying on getting its own spelling back
  from a `fold` run would see the spelling change. Measured: `fold`,
  `.save v(In)`, `write f.raw v(In)` gives `v(in)` `v(In)` today; without the
  rename it is a single `v(in)`.
- *Also breaks, potentially*: the `.op` scale prepend is 30-year-old behaviour
  with a comment in the source doubting itself — "Maybe we shouldn't make sure
  that the default scale is present if nobody uses it",
  `src/frontend/postcoms.c:689-691`. Any change there alters the column set of
  every partial `write` of every `.op` plot, in every consumer downstream of
  ngspice, not only this client's.
- *Regression risk*: **high**, and concentrated in `com_write()` and in
  `ft_evaluate()`'s expression labelling, which `print`, `plot`, `wrs2p`,
  `fft`, `spec` and `psd` all share.

**Option (iii) — both, sequenced: (i) now, (ii) considered separately.**

- *Cost*: two changes, two test decks and two review passes instead of one,
  and a client told twice.
- *Benefit*: (i) is cheap, low-risk, closes the shape the client actually hits,
  and can ship without settling the argument in (ii). It also removes the only
  case where the extra column's name is *not* a real net, which is the case a
  consumer cannot reason about at all.
- *Risk*: (ii) never happens once (i) has removed the visible pain — the
  explicit-name rows stay wrong indefinitely, and a consumer that names vectors
  on the `write` line still gets an unfilterable duplicate.

**One defect or two.** Measured, they are **two**: mechanism 2 inflates an
`.op` plot's column count on its own, with the rename structurally excluded
(the `set plainwrite` row). Whether the second one gets its own issue number is
part of the decision above and has deliberately not been filed — it is only
worth a number under options (ii) or (iii), and under (i) it stays a recorded
observation in this file.
