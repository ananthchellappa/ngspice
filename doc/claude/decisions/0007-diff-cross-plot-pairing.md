# Decision 0007 — `diff`'s cross-plot pairing, `doc/codex/issues/0037` and `0040`

## Status

Accepted, 2026-08-11, branch `ver_50`. Closes `doc/codex/issues/0037`, which
`doc/claude/decisions/0005-scale-vector-identity.md` deliberately left, and
`doc/codex/issues/0040`, which this work found in the loop it was changing.
Applies decision 3's Class C rule of `doc/claude/decisions/0001-distinguish.md`
to the frontend's second folded-key table. It re-decides nothing those records
settled.

Commits: `3b6780fc2` the pairing, `03d59f0a1` the `i()` wrapper, `0b6ea913e` a
citation correction in the comment `3b6780fc2` added, `2b3824868` the second
wrapper deck, `b712fc9c6` the sweep script's verdict classification.

## Context

Every earlier issue in this series was wrong under `distinguish` — the
experimental mode — or was a coverage gap. `0037` is the first that is wrong
in the two **shipped** modes and right under `distinguish`, and it is silent:
`diff`'s answer is the list of differing values, so an empty list reads as
"the two plots agree".

Measured at `28d36a7c4`, a deck whose node `Out` the default reader folds to
`out`, beside hand-written rawfiles that keep `Out` and `OUT`, all three modes,
before and after:

| rawfile | mode | before | after |
| --- | --- | --- | --- |
| `Out` | fold | nothing | `op1.Out[0]=9  op3.out[0]=0.75` |
| `OUT` | fold | nothing | `op2.OUT[0]=8  op3.out[0]=0.75` |
| `Out` | preserve | reported | unchanged |
| `OUT` | preserve | nothing | `op2.OUT[0]=8  op3.Out[0]=0.75` |
| `Out` | distinguish | reported | unchanged |
| `OUT` | distinguish | nothing | unchanged |

That is `0037`'s Impact table re-measured, every row.

---

## Decision 1 — the fold is unconditional, and the chain is filtered

**Decided: `canonical_key()` folds every key, in every mode, and the duplicate
chain is filtered with `vec_name_eq()`.**

Section 2.3 of `doc/claude/suggestions/case-sensitive-identifiers-plan.md`
settles the half of this that is not a choice: fold the key, never swap the
comparator, because `src/misc/hash.c:549` copies the key on insert only when
`hash_func` is `NGHASH_DEF_HASH(NGHASH_FUNC_STR)` and every free of a key is
gated on the same test, so a custom hash function flips the table from owning
its keys to borrowing them. What section 2.3 does not settle is whether the
fold itself should be conditional on the mode.

Rejected: **fold the key only when the mode folds.** It reads like the more
conservative option and it is strictly worse. `vec_name_eq()`'s wrapper arm —
`vec_wrapped_name_eq()`, `src/frontend/vectors.c:355`, which accepts a
difference confined to a leading `v()` or `i()` wrapper's letter — exists
because `src/frontend/outitf.c:1164` stores a digit-leading node name as
`V(<name>)` with that upper case `V` in **every** mode. A conditional fold
keeps `V(1)` and `v(1)` in different buckets under `distinguish`, so the
predicate could never be applied to them from a chain, and the arm would be
dead at this site while live at `findvec()`. Measured: with the unconditional
fold a rawfile spelling `v(1)` pairs with the run's `V(1)` under `distinguish`,
which is the same rule that already lets `print v(1)` work in that mode.

Accepted, because it is also the shape the frontend already has.
`vec_rebuild_lookup_table()` lower cases the key at `src/frontend/vectors.c:74`
and `find_permanent_vector_by_name()` filters the chain at `:423`; this is that
mechanism applied to a second table, and `nghash_unique(crossref_p, FALSE)` and
the `nghash_find_again()` walk were already there. Under `fold` and `preserve`
the filter is a tautology — every candidate on a folded-key chain satisfies
`cieq()` by construction — so those two modes gain exactly the pairings the
issue is about and nothing else.

Cost: one `canonical_name()` per chain candidate. A chain holds the vectors of
one plot that share one folded name, which is one vector in every deck that is
not exercising this bug.

## Decision 2 — the filter compares canonical names, not stored names

The key is the canonical name, so the chain groups canonical forms:
`canonical_name()` rewrites `i(some_name)` as `some_name#branch` and a
digit-leading name as `v(<name>)`. Filtering on `v_name` instead would reject
`i(Out)` against `Out#branch`, which is one vector in every mode and pairs
today.

That costs a third dynamic string. `ibuf` holds the first plot's canonical
name for the whole chain walk, `kbuf` the folded key that
`nghash_find_again()` is called with, and `jbuf` the candidate's canonical
name inside the loop. Reusing `ibuf` for the candidate would invalidate both
the name being compared against and the key being searched with.

## Decision 3 — `make_i_name_lower` goes, and it is its own commit

`canonical_name()`'s second argument lower cases the name **inside** an `i()`
wrapper. `com_diff()` passed `TRUE`; `nameeq()` has always passed `FALSE`.
That fold predates the case modes and is invisible under `fold` and
`preserve`, and under `distinguish` it was wrong in both directions:

- `i(Vsrc)` and `i(VSRC)` are two currents and it paired them;
- `i(V1)` and the `V1#branch` ngspice stores are one current and it did not
  pair them, because the folded canonical name `v1#branch` met a stored name
  that kept its capital.

It is `doc/codex/issues/0040`, filed rather than folded into `0037` because it
is a second defect: the fold was doing a hash key's job before there was one to
do it in, and decision 1's key is what makes it removable. Its own commit, its
own RED, and two decks — the loose half and the strict half — because a change
that both tightens and loosens needs an assertion in each direction.

Rejected: **delete the parameter.** It now has no caller that passes `TRUE`,
which is the usual argument for removing it. `canonical_name()` is otherwise
untouched by this work, and the explicit `FALSE` at each call site is where a
reader learns that the absence of a fold is deliberate. A parameter that is
always `FALSE` is cheaper to read than a silently missing one.

## Decision 4 — the `fold` row's deck cannot live in `casedist`

`tests/regression/casedist` forces `-D casemode=distinguish` on every deck it
holds, and the sharpest row of `0037` is the **default** mode. So the deck
that asserts it is in `tests/regression/misc`, which runs `check.sh` with no
`casemode` flag at all and already holds the default-mode case decks
(`fold-mixed-case.cir`, `keyword-fold-guard.cir`, `all-wildcard-case.cir`).
That is the first deck in this series to land outside a `case*` directory, and
it is the natural consequence of the first defect in the series that is wrong
in the shipped modes.

`casedist` still gets three decks, because `distinguish` is where the exactness
has to be asserted from the other side: the pairing that must **not** happen.

## Decision 5 — what the one-to-one claim does when a plot holds both spellings

The pairing links each vector to at most one twin, and the fold widens the set
of candidates without widening that. When one plot holds `Out` and `OUT` while
the mode says they are one name, the first unclaimed candidate on the chain
wins and the loser is reported unpaired on `cp_err`.

Measured, a rawfile holding both against a live plot holding `out`:
`diff op1 op2` claims `Out` and `diff op2 op1` claims `OUT`. So which twin is
claimed depends on `pl_dvecs` order and on which plot is named first —
`nghash_insert()` prepends within a bucket, so a chain is the reverse of the
insertion order.

**Decided: leave it.** It is the historical behaviour for two identically
named vectors in one plot, unchanged by the fold; nothing is dropped silently,
because the loser is named; and the alternative — a deterministic rule such as
"prefer the exact spelling" — would be a new language rule invented in a bug
fix, unasserted by any deck and unrequested by any issue. It is named here so
that the next reader finds it decided rather than overlooked.

## Decision 6 — the sweep's verdict, and why a script fix rode along

`case_differential_sweep.py` classified a run as `NUM-DIFF` — its one
must-stay-zero verdict — when any problem string contained the word
`rawfile`. A stdout problem quotes the deck's own output, and
`vector-diff-wrapper-case.cir` prints `OP information in rawfile.`
(`src/frontend/dotcards.c:228`), so the first full sweep of this work reported
`NUM-DIFF=1` on a deck whose numbers were identical.

The test is now for the marker string rather than the word. This is the
`0027`-style hazard from the other end: a heuristic that matches text a deck
controls will eventually match it.

## Evidence

Four decks, all proved RED through `make check` rather than by hand:

| Deck | RED, and against what | Asserts |
| --- | --- | --- |
| `tests/regression/misc/diff-pairing-case.cir` | `28d36a7c4` | the `fold` row twice: a rawfile spelling `Out` and one spelling `OUT` must both pair with the live `out` |
| `tests/regression/casedist/vector-diff-pairing-case.cir` | `28d36a7c4` | under `distinguish`, `OUT` must not pair and a rawfile's `v(1)` must pair with the run's `V(1)` |
| `tests/regression/casedist/vector-diff-wrapper-case.cir` | `3b6780fc2` | under `distinguish`, `i(Vsrc)` must not pair with `i(VSRC)` |
| `tests/regression/casedist/vector-diff-branch-case.cir` | `3b6780fc2` | under `distinguish`, `i(V1)` must pair with `V1#branch` and `i(v1)` must not |

Each of the three that assert a silence reports a pairing in the same `diff`,
so the silence cannot pass on a block that never ran — `0036`'s rule, in the
form this deck shape allows. `diff-pairing-case.cir` was also run against an
empty `.out` and fails there with 12 added lines, six of them blank, so its
evidence is output rather than an absence.

`tests/regression/casedist/vector-diff-case.cir`, `0037`'s acceptance
criterion 4, still passes: it exercises `nameeq()`, which selects `diff`'s
argument list and is untouched here.

- `make check`: **268 PASS, 0 FAIL**, up from 264 at `28d36a7c4` by exactly
  these four decks. Measured at each step: 266 after `3b6780fc2`, 267 after
  `03d59f0a1`, 268 after `2b3824868`. The 264 is the count this session
  inherited rather than re-ran; the four increments are measured, as are
  `tests/regression/misc` at 24 before and 25 after and
  `tests/regression/casedist` at 14 before and 17 after.
- `python3 doc/claude/scripts/case_differential_sweep.py --jobs 12 --timeout 90`
  on the binary before this work and on the binary after, same tree: **305
  decks, DIFF=58, OK=244, SKIP=3** before, **308 decks, DIFF=60, plus the
  misclassified verdict of decision 6, OK=244, SKIP=3** after. Compared per
  deck, the non-OK lines are identical except for the three new decks the
  after-sweep also ran and one `alter-rebin` line.
- The `alter-rebin` line is the known flap, re-measured on **both** binaries
  with `--filter alter-rebin`, three runs each: DIFF = 4, 1, 3 on the before
  binary and 4, 3, 2 on the after binary. It moves on an unchanged binary, so
  it is not evidence about this work.
- The four new decks are permanent `preserve-UPPER` `DIFF`s of the class
  `vector-rawfile-scale-case.cir` already had: the sweep's mechanical
  uppercasing rewrites `echo ... > vdw_one.raw` but not `load vdw_one.raw`,
  because `load` is on its `FILE_CARDS` list and `echo` is not, so the
  uppercased copy reads a file it never wrote. Re-run after decision 6's fix:
  `DIFF=4, NUM-DIFF=0`. The sweep never runs `distinguish`, so it is a
  collateral-damage guard for the two shipped modes and not evidence about the
  `casedist` decks at all.

## Corrections to this record's own commits

Found by reviewing the commits after they landed, which is the step `0027`'s
session added to this workflow.

**`3b6780fc2`'s comment cites `hash.c:108`, and the gate is at `:110`.** The
number came from section 2.3 of the plan, which has it stale; the issue,
`0037`, has `:110`. Re-grepped: the gates are `:110` in `nghash_resize()`,
`:182` in `nghash_empty()`, `:393` in `nghash_delete()`, and three the plan
does not name — `:471` in `nghash_delete_special()`, `:787` in
`nghash_merge()` and `:866` in `nghash_deleteItem()`. The copy at `:549` in
`nghash_insert()` was right. Corrected in the comment by `0b6ea913e` and in
the plan by this commit. The argument is strengthened rather than weakened:
six sites, not three.

**`3b6780fc2` says `make check` is "up from 264 at 28d36a7c4".** This session
did not run `make check` at `28d36a7c4`; 264 is the number its prompt
recorded, and what was measured here is 266 with two decks added, 267 with
three and 268 with four, with 0 FAIL throughout. A `FAIL` aborts its
directory, so a clean 268 does establish that every earlier directory ran.

**The first full sweep after the work reported `NUM-DIFF=1`, and no number
had moved.** Decision 6. Worth recording as a near miss rather than as a
script bug alone: the verdict that exists to be the alarm was raised by the
text of a deck this work added, and a session that trusted the verdict would
have hunted a numeric regression that was not there.

## What this decision does not decide

1. **`doc/codex/issues/0038`**, the reader's per-command fold exemption for
   control lines. Both new rawfile-writing decks depend on `echo` and `load`
   being on that list and would break if it were narrowed; neither depends on
   `fopen`, which is the missing entry the issue is about.
2. **The sweep's uppercasing of an `echo` redirect target.** It is the same
   asymmetry as `0038` seen from the script side, and it is why four decks are
   permanent `DIFF`s. Not fixed: the script's `FILE_CARDS` exemption is
   line-first-token based, and teaching it about redirects is a change to the
   thing that measures rather than to the thing measured.
3. **The claim order of decision 5.**
4. **`diff`'s split of its own report between `cp_out` and `cp_err`.** The
   unpaired vectors go to `cp_err` and the differing values to `cp_out`, which
   is what made `0037` silent on stdout. Changing it would move output no deck
   asked to move.
5. **The build-enforced lint** that would stop a new `strcmp` or `cieq`
   against a vector name from re-entering the tree, per `0005` item 7. With
   this record the frontend has two folded-key tables filtered by one
   predicate and no vector-name matcher outside them, which is the state the
   lint would freeze.
