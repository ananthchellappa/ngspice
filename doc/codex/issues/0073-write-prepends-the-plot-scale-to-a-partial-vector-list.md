# Issue: `write` Prepends the Plot's Scale to a Partial Vector List

## Status

**Open, filed 2026-08-15** on branch `ver_50`, by item 5 of
`doc/claude/batches/2026-08-15-xschem-open-items/PLAN.md`. **Filing only — no
fix is proposed and no code was written.** The repo owner has decided that the
rename half of `doc/codex/issues/0064` ships and that this one does not, for
now.

This is `0064`'s **second mechanism**, split out to its own number because it is
a separate defect: it inflates a rawfile's column count on its own, with the
rename structurally excluded from the path. `0064`'s first mechanism — the
`ft_evaluate()` wildcard rename — was fixed the same morning by commit
`25e891ec3`. This one survives that fix untouched; `src/frontend/postcoms.c` was
not edited by it.

**Pre-existing and upstream.** Every row below reproduces on
`/usr/local/bin/ngspice` (`ngspice-46`, no `casemode` support) as well as on
`build-ver_50/src/ngspice`. It is not a case-mode defect: the case mode changes
only *whether the two duplicate columns carry the same spelling*, and under
`fold` on a `.dc` plot they do anyway.

All measurements in this file were taken **this afternoon**, after both of
today's commits, against `build-ver_50/src/ngspice` (build stamp
`Sat Aug 15 18:18:34 UTC 2026`, `make -j8` reporting nothing to do at HEAD
`611076989`), with `set filetype=ascii`, in a scratch directory outside the
repo. Where a row differs from what
`doc/claude/batches/2026-08-15-xschem-followup/receipts/05-0064-scope-for-the-owner.md`
measured this morning, this file says so and says which commit moved it.

## Summary

`com_write()` (`src/frontend/postcoms.c:654-696`) copies the vectors it was
asked to write into a scratch plot, looks for the source plot's **default
scale** among them **by name**, and if it does not find it prepends a copy of
the scale as column 0. So a `write` that names a subset of a plot's vectors gets
back a file with a column the deck did not ask for.

That produces two consumer-visible shapes.

### Shape A — an extra signal the deck never named

`set plainwrite` takes the `vec_get()` path (`:633-650`) and never calls
`ft_evaluate()`, so no rename can occur on it. It still gains the column:

```
* op2_pwmid
Vs In 0 DC 3
R1 In MidNode 1k
R2 MidNode 0 3k
.save v(In) v(MidNode)
.op
.control
set filetype=ascii
run
set plainwrite
write op2_pwmid.raw midnode
.endc
.end
```

```
No. Variables: 2
No. Points: 1
Variables:
	0	v(in)	voltage
	1	v(midnode)	voltage
Values:
 0	3.000000000000000e+00
	2.250000000000000e+00
```

One signal asked for, two written. `v(in)` is a real net carrying a different
value — not a duplicate, an **extra signal**. This is the measurement that makes
this a defect of its own: the rename is impossible on this path.

The control that closes the isolation is the same deck with the argument list
changed to the vector that *is* the plot's scale:

```
* op1_in — .save v(In), write op1_in.raw in
No. Variables: 1
Variables:
	0	v(in)	voltage
```

One asked for, one written. Nothing about the argument count, the wildcard or
the case mode changed between those two runs — only whether the name the deck
typed equalled the name the plot stores for its scale.

### Shape B — a duplicate column under a byte-identical name

When the deck *does* name the scale, but in the `v(X)` form, the name comparison
misses and the same data is written twice:

```
.op, .save v(In), write f.raw v(In)      (casemode=preserve)
	0	v(In)	voltage
	1	v(In)	voltage
Values:
 0	3.000000000000000e+00
	3.000000000000000e+00
```

Two columns, one vector, **one name**. No consumer can tell them apart.

## Impact

The client's stated invariant is that a rawfile's `No. Variables` equals the
number of signals their deck saved. This defect breaks it, and breaks it in the
one way their documented defence does not reach.

- **Naming vectors explicitly on the `write` line adds a column.** Measured on
  today's tree: two explicit names on a two-save `.op` plot write **three**
  variables.

  | deck | asked for | `No. Variables` | rows |
  |---|---|---|---|
  | `.op`, `.save v(In) v(MidNode)`, `write f v(In) v(MidNode)` | 2 | **3** | `v(in)` `v(In)` `v(MidNode)` |
  | `.op`, `.save v(In) v(MidNode)`, `write f v(MidNode)` | 1 | **2** | `v(in)` `v(MidNode)` |
  | `.op`, `.save v(In)`, `write f v(In)` | 1 | **2** | `v(in)` `v(In)` |
  | `.op`, `.save v(In)`, `write f v(In)+0` | 1 | **2** | `v(in)` `v(In)+0` |
  | `.op`, `.save v(In) v(MidNode)`, `set plainwrite`, `write f midnode` | 1 | **2** | `v(in)` `v(midnode)` |

- **The duplicate is not filterable by name, and this is confirmed and wider
  than 0064 recorded.** The claim as `0064` states it — that under `preserve`,
  `distinguish` and on stock the pair is byte-identical — is **correct**,
  re-measured this afternoon on `.op` with `.save v(In)` and `write f v(In)`:

  ```
  fold          -> 0 v(in)   | 1 v(In)
  preserve      -> 0 v(In)   | 1 v(In)
  distinguish   -> 0 v(In)   | 1 v(In)
  ngspice-46    -> 0 v(in)   | 1 v(in)
  ```

  Two things `0064` does not say, both measured here:

  - **`fold` is not exempt.** It escapes above only because the deck typed
    `v(In)` and `fold` had already stored `in`. A deck that types the folded
    spelling gets the identical pair under `fold` too: `.op`, `.save v(In)`,
    `write f v(in)` → `0 v(in)` `1 v(in)`.
  - **A `.dc` plot gets the identical pair in every mode**, including `fold`
    and including stock — see the next bullet.

- **A consumer that feeds a file's own variable names back into a `write` is
  the worst case.** `raw_write()` decorates every `SV_VOLTAGE` name with a
  `v(...)` wrapper on output (`src/frontend/rawfile.c:308-314`) while the plot
  stores it bare, so the names a consumer reads out of a file are not the names
  `com_write()` compares against. On a `.dc` plot the scale is stored `v-sweep`
  and reported `v(v-sweep)`:

  ```
  .dc Vs 0 3 1, .save v(In), write f.raw v(v-sweep) v(In)
	0	v(v-sweep)	voltage
	1	v(v-sweep)	voltage
	2	v(In)	voltage
  ```

  Two names asked for — exactly the two names the same tool wrote into the
  previous file — three columns back, two of them byte-identical in name and in
  data. Reproduced on stock `ngspice-46` (`v(v-sweep)` `v(v-sweep)` `v(in)`).
  `.tran` survives the same round trip only by luck: its scale is reported
  `time`, without a `v()` wrapper, so the comparison happens to succeed.

- **Which column is prepended depends on the `.save` card's order, not on
  anything the `write` line says.** On an `.op` plot the "scale" is whichever
  saved node voltage was created first (`vec_new()`,
  `src/frontend/vectors.c:1138-1139`). Measured, same `write` line both times:

  ```
  .save v(MidNode) v(In)  + set plainwrite + write f midnode -> 1 var:  v(midnode)
  .save v(In) v(MidNode)  + set plainwrite + write f midnode -> 2 vars: v(in) v(midnode)
  ```

  `display` confirms the marker moves with the order: `midnode ... [default
  scale]` in the first, `in ... [default scale]` in the second.

- **On read-back the phantom becomes the plot's abscissa.** `load` of
  `op2_pwmid.raw` above gives `v(in) : voltage, real, 1 long [default scale]`
  and `v(midnode)` beside it. The extra column is not merely listed in the
  signal browser; it is structurally the loaded plot's scale, because the
  rawfile reader takes variable 0 as the scale.

- **Values are never wrong.** Every column carries correct data. The defect is
  in the column *set* and in the labelling.

- **The `-r` batch route remains a workaround** — see *Re-measured after
  today's commits* below.

## Root Cause

`com_write()`, `src/frontend/postcoms.c:654-696`. For each source plot it builds
a scratch `struct plot` by `memcpy` from the original, copies in the requested
vectors under their basenames, and tracks whether the original's `pl_scale` was
among them:

```c
            if (vec_eq(d, tpl->pl_scale)) {
                newplot.pl_scale = vv;
                scalefound = TRUE;
            }
        }
    }
    end->v_next = NULL;

    /* Maybe we shouldn't make sure that the default scale is
     * present if nobody uses it.
     */
    if (!scalefound) {
        newplot.pl_scale = vec_copy(tpl->pl_scale);
        newplot.pl_scale->v_next = newplot.pl_dvecs;
        newplot.pl_dvecs = newplot.pl_scale;
    }
```

Three facts compose into the defect.

1. **The scale is identified by name, not by identity.** `vec_eq()`
   (`src/frontend/vectors.c:1294-1313`) compares `vec_basename()` results
   through `vec_name_eq()`. `vec_basename()` (`:1345`) strips a plot
   qualification and trailing whitespace and **nothing else** — in particular it
   does not strip a `v(...)` wrapper. So `v(In)` never equals `In`, whatever the
   case mode. `vec_eq()`'s own comment says why the code is like this: *"Since
   vectors get copied a lot, we can't just compare pointers to tell if two
   vectors are 'really' the same."* By the time `com_write()` runs, `d` is
   already a copy made by `mkvnode()`, so pointer identity genuinely is not
   available at that point.

2. **The name it compares against may have been overwritten upstream.**
   `ft_evaluate()` (`src/frontend/evaluate.c:80-84`) relabels a single unchained
   result with the parse node's typed text. Since commit `25e891ec3` it withholds
   that for the wildcard tokens, but for everything else it still fires — and
   `v(X)` typed by a deck is exactly the form that will not match a bare stored
   name. This is where the two mechanisms still touch: it is why shape B exists.
   Shape A does not need it at all.

3. **On an `.op` plot the scale is not an axis.** `vec_new()`
   (`src/frontend/vectors.c:1138-1139`) makes the first `VF_PERMANENT` vector
   created in a plot its default scale. An operating point has no sweep, so the
   "scale" is an ordinary node voltage that happens to have been created first —
   which is why the prepended column is order-dependent, and why prepending it
   adds a *signal* rather than an *axis*.

### What the guard is for — read before proposing its removal

The block is not decoration and it cannot simply stop. Two things depend on it.

**It is what keeps `newplot.pl_scale` a member of `newplot.pl_dvecs`.**
`newplot` is a `memcpy` of `tpl`, so on entry `newplot.pl_scale` points at a
vector of the *original* plot, while `newplot.pl_dvecs` is a list of freshly
allocated copies — two disjoint sets. `raw_write()` then does this
(`src/frontend/rawfile.c:278-288`):

```c
    /* Before we write the stuff out, make sure that the scale is the first
     * in the list.
     */
    for (lv = NULL, v = pl->pl_dvecs; v != pl->pl_scale; v = v->v_next) {
        lv = v;
    }
```

There is **no `v != NULL` test in that loop.** Its only termination condition is
finding `pl_scale` in the list. Delete the prepend and leave `pl_scale` pointing
outside the list and that loop walks off the tail and dereferences NULL — a
crash in `write`, on every partial write, of every analysis. `spar_write()`
carries the identical unguarded loop at `:992-1000`. So any change here has to
either keep the scale in the list, or set `newplot.pl_scale` to something that
is, or fix both loops first.

**It is also a format-level requirement, on the analyses that have a real
axis.** The rawfile's first variable is the abscissa: measured, `load` of any
file written here reports variable 0 as `[default scale]`. A `.tran` file
carrying `v(in)` and no `time` has no abscissa and is not a usable file. On
`.tran`, `.dc` and `.ac` the prepend is doing exactly the job it was written
for.

The comment above the block — *"Maybe we shouldn't make sure that the default
scale is present if nobody uses it"* — is the original author already unsure,
and it has stood in the tree for decades. Every consumer downstream of ngspice
sees the current column set. That is the weight behind any change, and it is why
this issue is filed rather than fixed.

### Re-measured after today's commits

Two commits landed on `ver_50` this morning and both touch rows that
receipt 05 measured. Those rows are stale by design; the table below is the tree
as of this afternoon.

**`25e891ec3` — the wildcard rename fix — cured the wildcard rows and moved
nothing else here.**

| deck | receipt 05, this morning | today, after `25e891ec3` |
|---|---|---|
| `.op`, `.save v(In)`, bare `write` | 2: `v(in)` `v(all)` | **1: `v(in)`** |
| `.op`, `.save v(In)`, `write f all` | 2: `v(in)` `v(all)` | **1: `v(in)`** |
| `.op`, `.save v(In)`, `write f allv` | 2: `v(in)` `v(allv)` | **1: `v(in)`** |
| `.tran`, `.save v(In)`, `write f allv` | 2: `time` `v(allv)` | **2: `time` `v(in)`** |
| `.tran`, `.save v(In)`, `write f ally` | 2: `time` `v(ally)` | **2: `time` `v(in)`** |
| `.op`, `.save v(In)`, `write f v(In)` | 2: `v(in)` `v(In)` | 2: `v(in)` `v(In)` — unchanged |
| `.op`, `.save v(In)`, `write f v(in)` | 2: `v(in)` `v(in)` | 2: `v(in)` `v(in)` — unchanged |
| `.op`, `.save v(In)`, `write f v(In)+0` | 2: `v(in)` `v(In)+0` | 2: `v(in)` `v(In)+0` — unchanged |
| `.op` 2-save, `write f v(In) v(MidNode)` | 3 | 3 — unchanged |
| `.op` 2-save, `set plainwrite`, `write f midnode` | 2 | 2 — unchanged |

The pattern is exactly what item 1's scope promised: **a bare or wildcard
`write` is now clean; a `write` that names vectors is not.** The wildcard rows
were cured because the rename no longer hides the scale's name behind `all`, so
`vec_eq()` finds it and the prepend does not fire. Nothing in `com_write()`
changed and no row that does not involve a wildcard moved.

One `0064` side observation is also unchanged: `.op` + `.save v(In)` +
`write f ally` still writes **12** variables from the constants plot
(`yes FALSE TRUE boltz c e echarge i kelvin no pi planck`). That is
`doc/codex/issues/0059`'s shape, not this one's, and it is recorded here only
so the next measurement does not mistake it for a regression.

**`731c01455` — the `-r` writer's case-mode header line — did not change the
`-r` route's variable count.** Re-measured, `ngspice -b -r f.raw deck.cir` with
no `.control` block:

| deck | gate unset | `-D casemodewrite` |
|---|---|---|
| `.op`, `.save v(In)` | `No. Variables: 1`, `v(in)` | `No. Variables: 1`, `v(in)` |
| `.op`, `.save v(In) v(MidNode)` | `No. Variables: 2`, `v(in)` `v(midnode)` | `No. Variables: 2`, `v(in)` `v(midnode)` |
| `.tran`, `.save v(In)` | `No. Variables: 2`, `time` `v(in)` | `No. Variables: 2`, `time` `v(in)` |

The new line is header metadata between `Plotname:` and `Flags:`, not a variable
row:

```
Plotname: Operating Point
Option: casemode=fold
Flags: real
No. Variables: 1
```

**The `-r` route is therefore still the clean workaround, and it is now strictly
better than it was this morning**: it never reaches `com_write()` —
`fileInit()`/`fileInit_pass2()` (`src/frontend/outitf.c`) stream the header from
the analysis's own vector set — and with `casemodewrite` set it now also carries
the case mode that `0071` said it lost. Its remaining limitation is inherent, not
a defect: `-r` writes the whole saved set and takes no vector list, so it is a
workaround for *"write me what I saved"* and not for *"write me these three of
the twelve I saved"*.

**Correction to receipt 05 — `.tran`, `.dc` and `.ac` *can* be inflated.**
Receipt 05 and `0064` both state that on an analysis with a real scale the count
is never inflated, only the name corrupted. That is **wrong as a general
statement**, and the qualification matters because the counter-example is the
round-trip shape a tool is most likely to produce. It holds only while the deck
does not name the scale itself, or names it in a spelling `vec_eq()` accepts.
Measured this afternoon:

| deck | asked for | `No. Variables` | rows |
|---|---|---|---|
| `.tran`, `write f time v(In)` | 2 | 2 | `time` `v(In)` |
| `.tran`, `write f TIME v(In)` (`fold`) | 2 | 2 | `TIME` `v(In)` |
| `.tran`, `write f time+0 v(In)` | 2 | **3** | `time` `time+0` `v(In)` |
| `.dc`, `write f v(v-sweep) v(In)` | 2 | **3** | `v(v-sweep)` `v(v-sweep)` `v(In)` |
| `.ac`, `write f frequency v(In)` | 2 | 2 | `frequency` `v(In)` |
| `.ac`, `write f frequency+0 v(In)` | 2 | **3** | `frequency` `frequency+0` `v(In)` |

The `.dc` row is the one to keep: those are the file's own reported names, the
duplicate pair is byte-identical, it happens under default `fold`, and it
reproduces on stock `ngspice-46`. The corrected statement is: **on an analysis
with a real scale the prepend is correct and invisible whenever the deck does
not name the scale, and inflates by one whenever the deck names the scale in a
spelling that is not the stored one** — which the `v()` wrapper always is for a
voltage-typed scale.

## Acceptance Criteria

The invariant these criteria serve cannot be *"`No. Variables` equals the number
of vectors on the `write` line"* — see the standing question in *Resolution*.
They are written against the weaker statement that is defensible: **the scale is
written when it is a real axis, is written under the name the plot stores for
it, and is never written twice.**

1. `.op`, `.save v(In)`, `write f.raw v(In)` writes `No. Variables: 1` and one
   column, in all three case modes. No column of any file this command writes
   is a copy of another column of the same file.
2. `.dc Vs 0 3 1`, `.save v(In)`, `write f.raw v(v-sweep) v(In)` writes
   `No. Variables: 2`. A file's own reported variable names, fed back into a
   `write`, reproduce that file's column set exactly — for `.tran`, `.dc` and
   `.ac`.
3. `.tran`, `.save v(In)`, `write f.raw v(In)` still writes `time` and `v(In)`:
   the axis is still supplied when the deck omits it. Deleting the guard
   outright is not an acceptable fix, and criterion 6 says why.
4. The `.op` case is decided explicitly and the decision is recorded, because it
   is not a bug fix but a behaviour choice: either an `.op` plot's pseudo-scale
   stops being prepended to a partial write — so
   `.save v(In) v(MidNode)` + `write f.raw v(MidNode)` writes one column — or it
   keeps being prepended and the issue is closed as working-as-intended for
   `.op`. Criterion 4 is not met by leaving it ambiguous.
5. `write f.raw v(In)+0` still labels its column `v(In)+0`. The expression
   labelling that `ft_evaluate()` exists for is not withdrawn, and the fix to
   `0064` that withheld the rename from the wildcards is not widened by
   accident.
6. Whatever changes, `newplot.pl_scale` is still a member of
   `newplot.pl_dvecs` when `raw_write()` is called, **or** the unguarded loops
   at `src/frontend/rawfile.c:278-288` and `:992-1000` gain a NULL test in the
   same change. A partial `write` must not crash.
7. A deck in `tests/regression/misc/` covers 1, 2, 3 and 5 with
   `set filetype=ascii`, so the column count and the names are both in the
   compared output, and carries the `set plainwrite` form of shape A as a
   control — it is the one that isolates this mechanism from `0064`'s.
8. Full `make check` from `build-ver_50` is green. `com_write()` is on the path
   of every `write` in the suite.

## Resolution

**Not started.** Filed for the record on the repo owner's instruction; the
`0064` rename shipped and this one deliberately did not.

### The standing question the owner has to settle first

`0064`'s scope discussion left one question open and this file inherits it:
**is making `No. Variables` equal what the deck asked for achievable at all?**

Measured, the invariant as the client words it is **not achievable**, and the
reason is not a defect:

- A `.tran` file with no `time` column has no abscissa and is not a usable file.
  A deck that writes `v(in)` alone from a transient run *must* get two columns
  back. So `No. Variables == len(argument list)` cannot be the target on any
  analysis with a real scale.
- Removing the prepend without touching `raw_write()` crashes `write` on the
  first partial write of any plot, per criterion 6.

What is achievable, and what this issue proposes the owner scope instead, is
the restatement in *Acceptance Criteria*: the scale is written when it is a real
axis, under the plot's own name for it, and never twice. On `.tran`, `.dc` and
`.ac` that gives a consumer the rule `No. Variables == n + 1` with no
exceptions. On `.op` it gives `n` — but only if criterion 4 is answered in the
first of its two ways, and that is a behaviour change visible to every consumer
of ngspice and not only to the client who reported it.

Two levers exist and neither is designed here; naming them only so the cost is
visible:

- **Identify the scale by identity rather than by name.** `vec_copy()` would
  have to carry a provenance back-pointer to the vector it copied, which
  `vec_eq()`'s own comment says does not exist today. Wider blast radius —
  `vec_eq()` has many callers — but it is the change that fixes shape A and
  shape B together and fixes the `.dc` round trip for free.
- **Suppress the prepend when the plot's scale is not a real axis.** Narrower,
  `.op`-only, and it needs a test for "this plot has a real scale" that the
  codebase does not currently have — `vec_new()` marks the first permanent
  vector as the scale unconditionally, so nothing distinguishes an operating
  point's pseudo-scale from a sweep's abscissa after the fact. It leaves the
  `.dc` and `.tran` duplicate rows untouched.

Neither is chosen. **The prerequisite for scoping either one is the owner
settling criterion 4 and the wording of the invariant**, not more measurement;
the measurement in this file is believed complete.

### Related

- `doc/codex/issues/0064` — the first mechanism, the `ft_evaluate()` wildcard
  rename, **fixed** 2026-08-15 by `25e891ec3`. This issue is the second
  mechanism `0064` measured and did not fix. They still touch: shape B needs
  `0064`'s rename to fire for a non-wildcard name, which it still does.
- `doc/codex/issues/0071` — the `-r` writer's missing case-mode line,
  **fixed** 2026-08-15 by `731c01455`, which is what makes the `-r` route a
  workaround with no remaining cost.
- `doc/codex/issues/0059` — a bare `write` emitting the constants plot when no
  analysis ran. Independent: `0059`'s guards refuse before this can happen, and
  after them this still happens. The 12-variable `ally` row above is `0059`'s
  shape, recorded here only so it is not mistaken for this one.
