# Receipt 05 — 0064's second mechanism, filed as 0073

Item 5 of `doc/claude/batches/2026-08-15-xschem-open-items/PLAN.md`. Branch
`ver_50`, HEAD at start `611076989`. **Filing only. No fix, no code.** Nothing
under `src/` or `tests/` was touched or opened for editing.

## The issue

`doc/codex/issues/0073-write-prepends-the-plot-scale-to-a-partial-vector-list.md`
— *"`write` Prepends the Plot's Scale to a Partial Vector List"*. Status
**Open, filed**. Resolution **not started**, carrying the owner's standing
question.

0073 was the next free number: 0072 was taken by item 4 of this batch an hour
earlier.

## What changed

- **New**: `doc/codex/issues/0073-…`, house sections Status / Summary / Impact /
  Root Cause / Acceptance Criteria / Resolution.
- **Edited, additively**:
  `doc/codex/issues/0064-a-wildcard-matching-one-vector-is-renamed-to-the-wildcard.md`
  — two paragraphs gain the number `0073`, one in *Status* and one in
  *Resolution*'s "What was deliberately left". The *Status* addition also says
  which of 0064's own statements 0073 corrects. Nothing written into 0064 today
  by the measuring crew or the fixing crew was altered or removed; the pre-fix
  analysis section below "Everything below this line is as it was written before
  the fix" was not touched at all.
- **New**: this receipt.

## What was measured, and against what

`build-ver_50/src/ngspice`, build stamp `Sat Aug 15 18:18:34 UTC 2026`.
`make -j8` in `build-ver_50` reported *"Nothing to be done for 'all-am'"* at
HEAD `611076989`, so the binary is HEAD's and carries **both** of today's
commits. Cross-checks on `/usr/local/bin/ngspice` (`ngspice-46`) and under
`-D casemode=preserve` / `-D casemode=distinguish`. `set filetype=ascii`
throughout. Every deck was generated and run under
`/tmp/claude-1000/-home-qflow-dev-ngspice-test/…/scratchpad/m5`; no artifact was
left in the repo. One netlist family: `Vs In 0`, `R1 In MidNode 1k`,
`R2 MidNode 0 3k`.

Both claims the item handed me were re-run before a word was written. Both hold.

### Isolating deck 1 — `set plainwrite` with a named vector, mechanism 2 alone

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
Plotname: Operating Point
Flags: real
No. Variables: 2
No. Points: 1
Variables:
	0	v(in)	voltage
	1	v(midnode)	voltage
Values:
 0	3.000000000000000e+00
	2.250000000000000e+00
```

One signal asked for, two written, `ft_evaluate()` never called on that path.
**Mechanism 2 confirmed to inflate on its own** — and note the extra column is a
*distinct signal* (3.0 V vs 2.25 V), not a duplicate. That distinction is not in
receipt 05 of the predecessor batch and is in 0073.

### Isolating deck 2 — typed text equal to the stored name

```
* op1_in — .save v(In), then: write op1_in.raw in
No. Variables: 1
Variables:
	0	v(in)	voltage
```

One asked for, one written. Same code path as `write f.raw v(in)`, which writes
**2** (`v(in)` `v(in)`). So the trigger is the name the deck typed not matching
the name the plot stores for its scale — not the wildcard, not the case mode,
not the argument count. **Rename ruled out as necessary; the prepend needs only
a name mismatch.**

### The client's Impact claim — verified, and it is wider than 0064 says

`.op`, `.save v(In)`, `write f.raw v(In)`, today's tree:

```
fold          -> 0 v(in)   | 1 v(In)
preserve      -> 0 v(In)   | 1 v(In)
distinguish   -> 0 v(In)   | 1 v(In)
ngspice-46    -> 0 v(in)   | 1 v(in)
```

**The claim is correct.** Both columns carry 3.0 V and, under `preserve`,
`distinguish` and stock, one name. Filter-by-name — the defence RESPONSE.md gave
the client — does not reach it.

Two things the claim understates, both measured:

- **`fold` is not exempt.** It escapes above only because the deck typed `v(In)`
  while `fold` had stored `in`. `write f.raw v(in)` under `fold` gives
  `v(in)` `v(in)`.
- **`.dc` gives the identical pair in every mode including stock**, from the
  file's own reported names: `.dc Vs 0 3 1`, `.save v(In)`,
  `write f.raw v(v-sweep) v(In)` → **3** variables, `v(v-sweep)` `v(v-sweep)`
  `v(In)`, the first two identical in name and in data. The `.dc` scale is
  *stored* `v-sweep` and *reported* `v(v-sweep)`, because `raw_write()` adds the
  `v()` wrapper on output (`src/frontend/rawfile.c:307-314`) and `com_write()`
  compares against the bare stored name. A consumer that round-trips a file's
  own variable names into a `write` hits this every time.

Also measured: on read-back, `load` makes the phantom the loaded plot's default
scale — `v(in) : voltage, real, 1 long [default scale]`.

## What today's two commits moved

**`25e891ec3` (the rename fix) cured the wildcard rows and moved nothing else.**

| deck | receipt 05, this morning | today |
|---|---|---|
| `.op` 1-save, bare `write` | 2: `v(in)` `v(all)` | **1: `v(in)`** |
| `.op` 1-save, `write f all` | 2: `v(in)` `v(all)` | **1: `v(in)`** |
| `.op` 1-save, `write f allv` | 2: `v(in)` `v(allv)` | **1: `v(in)`** |
| `.tran` 1-save, `write f allv` | 2: `time` `v(allv)` | **2: `time` `v(in)`** |
| `.tran` 1-save, `write f ally` | 2: `time` `v(ally)` | **2: `time` `v(in)`** |
| `.op` 1-save, `write f v(In)` | 2: `v(in)` `v(In)` | unchanged |
| `.op` 1-save, `write f v(In)+0` | 2: `v(in)` `v(In)+0` | unchanged |
| `.op` 2-save, `write f v(In) v(MidNode)` | 3 | unchanged |
| `.op` 2-save, `plainwrite` + `write f midnode` | 2 | unchanged |

Exactly what item 1's scope promised: a bare or wildcard `write` is clean now, a
`write` that names vectors is not. Those five rows of the predecessor receipt's
table are stale and 0073 says so row by row.

**`731c01455` (the `-r` case-mode line) did not change the `-r` variable count.**
Re-checked with the gate unset and with `-D casemodewrite`:

| deck | gate unset | gate set |
|---|---|---|
| `.op`, `.save v(In)` | 1: `v(in)` | 1: `v(in)` |
| `.op`, `.save v(In) v(MidNode)` | 2: `v(in)` `v(midnode)` | 2: `v(in)` `v(midnode)` |
| `.tran`, `.save v(In)` | 2: `time` `v(in)` | 2: `time` `v(in)` |

The new line sits between `Plotname:` and `Flags:` and is header metadata, not a
variable row. **`-r` is still the clean workaround and is now strictly better
than this morning** — it never reaches `com_write()`, and it now carries the
case mode. Its remaining limit is inherent, not a defect: `-r` takes no vector
list, so it answers *"write what I saved"* and not *"write these three of the
twelve I saved"*.

## What I corrected in the predecessor's findings

**`.tran`/`.dc`/`.ac` *can* be inflated.** Receipt 05 and 0064 both state the
count is never inflated on an analysis with a real scale. That is wrong as a
general statement:

| deck | asked for | `No. Variables` | rows |
|---|---|---|---|
| `.tran`, `write f time v(In)` | 2 | 2 | `time` `v(In)` |
| `.tran`, `write f time+0 v(In)` | 2 | **3** | `time` `time+0` `v(In)` |
| `.dc`, `write f v(v-sweep) v(In)` | 2 | **3** | `v(v-sweep)` `v(v-sweep)` `v(In)` |
| `.ac`, `write f frequency v(In)` | 2 | 2 | `frequency` `v(In)` |
| `.ac`, `write f frequency+0 v(In)` | 2 | **3** | `frequency` `frequency+0` `v(In)` |

Corrected statement, now in 0073: the prepend is invisible on a real-scale
analysis whenever the deck does not name the scale, and inflates by one whenever
it names the scale in a spelling that is not the stored one — which the `v()`
wrapper always is for a voltage-typed scale. `.tran` looks safe only because its
scale is reported `time`, unwrapped.

Also new, and not in any predecessor document: **which column is prepended
depends on the `.save` card's order.** Same `write` line both times —

```
.save v(MidNode) v(In)  + plainwrite + write f midnode -> 1 var:  v(midnode)
.save v(In) v(MidNode)  + plainwrite + write f midnode -> 2 vars: v(in) v(midnode)
```

— because `vec_new()` (`src/frontend/vectors.c:1138-1139`) makes the first
`VF_PERMANENT` vector the plot's default scale. `display` shows the `[default
scale]` marker moving with the `.save` order. That is the sharpest evidence that
an `.op` plot's "scale" is not an axis.

## What `com_write()`'s guard is for — read, not guessed

The item required this and it changed the shape of the issue. The guard cannot
simply stop, for two independent reasons.

1. **`raw_write()` would crash.** `src/frontend/rawfile.c:281`:

   ```c
   for (lv = NULL, v = pl->pl_dvecs; v != pl->pl_scale; v = v->v_next) {
   ```

   **No NULL test.** Its only termination is finding `pl_scale` in the list.
   `com_write()` builds `newplot` by `memcpy` from `tpl`, so on entry
   `newplot.pl_scale` points into the *original* plot while `newplot.pl_dvecs`
   holds fresh copies — disjoint sets. The prepend at `postcoms.c:692-696` is
   what makes them intersect. Remove it and that loop walks off the tail on
   every partial write, of every analysis. `spar_write()` has the identical
   unguarded loop at `:995`.
2. **The format needs the abscissa first.** Measured: `load` of any file written
   here reports variable 0 as `[default scale]`. A `.tran` file with no `time`
   column has no abscissa.

The comment above the block — *"Maybe we shouldn't make sure that the default
scale is present if nobody uses it"* — is the original author already unsure,
and it has stood for decades with every downstream consumer seeing the current
column set. That is recorded in 0073 as the weight behind any change, and it is
why 0073's criterion 6 requires that either the scale stays in the list or both
`rawfile.c` loops gain a NULL test in the same change.

## The open decision, for the owner

0073's Resolution is **not started** and carries the standing question from
0064's scope discussion: **is making `No. Variables` equal what the deck asked
for achievable at all?**

Measured, **as literally worded it is not**, and that is not a defect:

- A `.tran` file with no `time` column is not a usable file, so
  `No. Variables == len(argument list)` cannot be the target on any analysis
  with a real scale.
- Removing the prepend without touching `raw_write()` crashes `write`.

0073's acceptance criteria are therefore written against the restatement:
**the scale is written when it is a real axis, under the plot's own name for it,
and never twice.** That gives a consumer `n + 1` with no exceptions on
`.tran`/`.dc`/`.ac`, and `n` on `.op` — but only if the owner answers 0073's
criterion 4, which is the genuinely open question and is a behaviour choice, not
a bug fix:

> Does an `.op` plot's pseudo-scale stop being prepended to a partial write — so
> `.save v(In) v(MidNode)` + `write f.raw v(MidNode)` writes one column — or does
> it keep being prepended and the issue close as working-as-intended for `.op`?

Two levers are named in 0073 and **neither is designed**: identify the scale by
identity rather than by name (fixes both shapes and the `.dc` round trip; needs
provenance on `vec_copy()`, which `vec_eq()`'s own comment says does not exist),
or suppress the prepend when the scale is not a real axis (narrower, `.op`-only,
and the codebase has no test for "this plot has a real scale"). The prerequisite
for scoping either is the owner settling criterion 4 and the invariant's
wording — not more measurement. The measurement in 0073 is believed complete.

## Left undone, deliberately

- **No fix, no `src/`, no `tests/`.** The item is filing.
- **No client-facing document was touched.** The corrected `.tran`/`.dc`/`.ac`
  rule, the identical-name `.dc` round trip, and the fact that `-r` now carries
  the case mode all belong in item 6's round-3 reply. The upstream submission
  **has not been sent** and nothing here says otherwise.
- **No test deck was written.** 0073's criterion 7 specifies one; writing it
  would have been the fix's RED step, which this item is not.
