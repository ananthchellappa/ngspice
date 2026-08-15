# Receipt 05 — 0064's scope, measured, for the owner to settle

Item 5 of `doc/claude/batches/2026-08-15-xschem-followup/PLAN.md`. Branch
`ver_50`.

**Nothing was decided.** The item was to measure, to split the mechanisms, and
to put the choice in front of the repo owner. The choice is at the bottom of
this file and at the end of `doc/codex/issues/0064`'s *Resolution*.

## What changed

- `doc/codex/issues/0064-a-wildcard-matching-one-vector-is-renamed-to-the-wildcard.md`
  — added, nothing removed. Status stays **Open**.
  - a Status note saying the 2026-08-15 re-measurement widened the issue;
  - a new section *Re-measured 2026-08-15* with the full deck-shape table, the
    four findings that are new, the corrected count rule for a consumer, and
    the `-r` workaround;
  - a correction, in place, of the "expect n, or n+1 when n is 1" advice the
    file gave a consumer — it is wrong, and the section says so and replaces it;
  - four bullets in *Impact*;
  - a new *Which mechanism produces which row* subsection under *Root Cause*
    with the isolating measurements;
  - a new *Scope: a decision for the repo owner* subsection under *Resolution*.
- This receipt.

Nothing under `src/` or `tests/` was touched. No fix was written.

## What was measured, and how

`build-ver_50/src/ngspice`, build stamp `Sat Aug 15 15:34:32 UTC 2026`, default
`casemode=fold`, `set filetype=ascii`. Cross-checks on `/usr/local/bin/ngspice`
(`ngspice-46`) and under `-D casemode=preserve` and `-D casemode=distinguish`.
All decks generated and run in a scratch directory under
`/tmp/claude-1000/-home-qflow-dev-ngspice-test/`; no artifact was left in the
repo. Neither audit claim was taken on trust; both were re-derived.

One netlist throughout: `Vs In 0`, `R1 In MidNode 1k`, `R2 MidNode 0 3k`.

### The table

`No. Vars` is the file's `No. Variables:`. Full table, with the `.dc`/`.ac` rows
and the mode cross-checks, is in the issue.

| analysis | `.save` | `write` argument | No. Vars | rows |
|---|---|---|---|---|
| `.op` | `v(In)` | *(bare)* | 2 | `v(in)` `v(all)` |
| `.op` | `v(In)` | `all` | 2 | `v(in)` `v(all)` |
| `.op` | `v(In)` | `allv` | 2 | `v(in)` `v(allv)` |
| `.op` | `v(In)` | `v(In)` | 2 | `v(in)` `v(In)` |
| `.op` | `v(In)` | `v(in)` | 2 | `v(in)` `v(in)` |
| `.op` | `v(In)` | `in` | **1** | `v(in)` |
| `.op` | `v(In)` | `v(In)+0` | 2 | `v(in)` `v(In)+0` |
| `.op` | `v(In)` | `set plainwrite`, `in` | **1** | `v(in)` |
| `.op` | `v(In)` | `-b -r`, no `.control` | **1** | `v(in)` |
| `.op` | `v(In) v(MidNode)` | *(bare)* / `all` / `allv` | 2 | `v(in)` `v(midnode)` |
| `.op` | `v(In) v(MidNode)` | `in midnode` | 2 | `v(in)` `v(midnode)` |
| `.op` | `v(In) v(MidNode)` | `v(In) v(MidNode)` | **3** | `v(in)` `v(In)` `v(MidNode)` |
| `.op` | `v(In) v(MidNode)` | `v(In)` | 2 | `v(in)` `v(In)` |
| `.op` | `v(In) v(MidNode)` | `v(MidNode)` | 2 | `v(in)` `v(MidNode)` |
| `.op` | `v(In) v(MidNode)` | `set plainwrite`, `midnode` | 2 | `v(in)` `v(midnode)` |
| `.op` | `v(In) v(MidNode)` | `-b -r`, no `.control` | 2 | `v(in)` `v(midnode)` |
| `.tran` | `v(In)` | *(bare)* / `all` | 2 | `time` `v(in)` |
| `.tran` | `v(In)` | `allv` | 2 | `time` `v(allv)` |
| `.tran` | `v(In)` | `ally` | 2 | `time` `v(ally)` |
| `.tran` | `v(In)` | `v(In)` / `time v(In)` | 2 | `time` `v(In)` |
| `.tran` | `v(In)` | `-b -r`, no `.control` | 2 | `time` `v(in)` |
| `.tran` | `v(In) v(MidNode)` | *(bare)* | 3 | `time` `v(in)` `v(midnode)` |
| `.tran` | `v(In) v(MidNode)` | `v(In) v(MidNode)` | 3 | `time` `v(In)` `v(MidNode)` |

### Both audit claims confirmed, and both understated

**(a) The `-r` route is clean.** Confirmed: `ngspice -b -r f.raw` on the
one-save `.op` deck writes `No. Variables: 1` and the row `v(in)`. It bypasses
`com_write()` entirely — `fileInit()`/`fileInit_pass2()`,
`src/frontend/outitf.c:930` and `:1046`, write the header from the analysis's
own vector set. **This is a workaround the client can use today** and has not
been told about. Its one cost is `doc/codex/issues/0071`: the `-r` writer emits
no `Option: casemode=` line.

**(b) The extra column is not wildcard-specific.** Confirmed, and worse than
the audit said in two ways:

- Two explicit names on a two-save `.op` plot write **three** variables. The
  defect is not confined to results of size one.
- Under `preserve`, under `distinguish` and on stock `ngspice-46`, the two
  columns of `.op` + `write f.raw v(In)` carry a **byte-identical name**
  (`v(In)` `v(In)`; stock `v(in)` `v(in)`). The client's stated defence —
  "we can filter it, but only by name" — does not reach that case at all. This
  is the single most consumer-relevant new fact and should go to them.

A third thing, not in the audit: on `.tran`/`.dc`/`.ac` the **count is never
inflated** but the **name is still corrupted**. `write f.raw allv` on a
one-voltage `.tran` plot writes `time` and `v(allv)` — a count check passes and
a phantom still reaches the browser. So the issue's own advice to a consumer,
"expect n, or n+1 when n is 1", is wrong; the corrected rule is in the issue.

## Mechanism split — two, not one

**Mechanism 1: the rename, `src/frontend/evaluate.c:80-84`.** It replaces the
*stored* name with the *typed* parse-node text on every single unchained
result. Isolated by three measurements:

- `write f.raw in` (typed text equals the stored name) → **1** variable;
  `write f.raw v(in)` (differs only by the `v()` wrapper the deck typed) → 2.
  So the trigger is neither case, nor the wildcard, nor the match size: it is
  the typed text differing from the stored name, which for `v(X)` syntax it
  always does.
- `set plainwrite` takes the `vec_get()` path (`src/frontend/postcoms.c:633-650`)
  and never calls `ft_evaluate()`: **1** variable where the pnode path gives 2,
  everything else identical.
- `all` on a two-vector plot chains through `v_link2`, the rename is withheld,
  and the result is correct.

**Mechanism 2: `com_write()`'s scale insurance,
`src/frontend/postcoms.c:681-696`.** `vec_eq()` identifies the plot's scale by
*name* (`src/frontend/vectors.c:1278-1297`); if it is not among what is being
written, `:692-696` prepends a copy. Correct and necessary on
`.tran`/`.dc`/`.ac`, where the scale is a real axis. On an `.op` plot it is not
an axis: `vec_new()` (`src/frontend/vectors.c:1122-1124`) makes the *first*
`VF_PERMANENT` vector the plot's default scale, so it is whichever saved node
voltage was created first. Isolated by: `set plainwrite` + `write f.raw midnode`
on a two-save `.op` plot → **2** variables, `v(in)` and `v(midnode)`, with the
rename structurally impossible on that path. One asked for, two written, no
rename involved.

**They compose.** On a one-save `.op` plot mechanism 1 hides the scale's name
so mechanism 2 prepends a duplicate of the same data. That composition is the
column 0064 was filed about; neither mechanism alone produces it.

**Verdict: two defects.** Mechanism 2 inflates on its own, so it is not a
consequence of mechanism 1.

## The decision the owner must make

Three scopes, no obvious winner. Stated in full, with costs, at the end of
0064's *Resolution*. In brief:

| | closes | leaves | risk |
|---|---|---|---|
| **(i)** 0064 as filed — withhold the rename from the wildcard tokens | every `v(all)`/`v(allv)`/`v(ally)` row, incl. the bare-`write` default and the `.tran` name corruption; bare `write` then matches what `-r` writes | every explicitly-named row, incl. the unfilterable identical-name duplicate | **low** — nothing in the tree breaks; no `.out` expects a wildcard label, and `findvec()` (`vectors.c:173-189`) already makes a real vector named `all` unreachable |
| **(ii)** the wider shape — `No. Variables` equals what the deck asked for | everything | — | **high** — and the invariant as literally worded is not achievable: a `.tran` file without `time` is unplottable. Also changes output spellings: under `fold`, `write f.raw v(In)` writes `v(In)` *because of* the rename, and would become `v(in)` |
| **(iii)** both, sequenced | (i) ships cheap now, (ii) argued separately | — | two changes, two decks, client told twice; and (ii) may never happen once the visible pain is gone |

**Three things the owner should weigh before choosing.**

1. **(i) is sufficient for this client's actual decks.** Their generated decks
   use a bare `write`, which (i) fixes completely. It is insufficient only if
   they start naming vectors on the `write` line.
2. **(ii)'s invariant needs rewording before it can be scoped.** The
   defensible form is "the scale is written when it is a real axis, and never
   written twice under two names", not "`No. Variables` equals what the deck
   asked for". Settling the wording is a prerequisite, not a detail.
3. **(ii) has a client-visible cost in the opposite direction.** The rename is
   why a `fold` run writes the deck's own spelling (`v(In)`) rather than the
   stored one (`v(in)`). Narrowing it takes that away. The client has asked for
   their spelling back in earlier rounds; this is the same lever.

**And one sub-decision, deliberately not taken:** mechanism 2 is a separate
defect and could have its own issue number. **It was not filed** — the plan
says the crew does not pick. It is only worth a number under (ii) or (iii);
under (i) it stays a recorded observation inside 0064. Say the word and it can
be filed as `0072`.

## Left undone

- **`alle` was not measured**, only read. It belongs in the wildcard set by
  code: `get_all_type()` recognises it (`src/frontend/vectors.c:143-146`),
  `findvec()` dispatches before any name lookup (`:183-186`), and
  `findvec_alle()` chains through `v_link2` (`:329-343`) exactly as
  `FINDVEC_ALL_GEN` does — same exemption at two matches, same exposure at one.
  It was not run because every XSPICE digital deck in
  `build-ver_50/tests/xspice/digital` segfaults there today, including the
  committed `sen_evt.cir`. The `*.cm` code models in that tree are stamped
  `Aug 15 08:36` against a binary stamped `Aug 15 09:36` — stale models, a
  build-directory artifact that a `make` in that tree should clear. **Not
  investigated, and flagged here only so the next crew is not surprised by it.**
- **The client has not been told** about the `-r` workaround, about the
  identical-name duplicate, or about the corrected count rule. All three are
  now in 0064; putting them in front of the client is a separate act and no
  client-facing document was edited by this item.
- `doc/claude/feedback/ngspice_upstream/RESPONSE.md` §5 still carries the
  narrow account and the "expect n, or n+1 when n is 1" advice. It was left
  alone: this item's commit is 0064 plus this receipt and nothing else, and
  §5's correction depends on the scope the owner picks. Whoever writes the next
  round's reply should take the corrected count rule, the `-r` workaround and
  the identical-name duplicate from 0064.
