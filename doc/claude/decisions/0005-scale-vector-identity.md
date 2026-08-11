# Decision 0005 — the vector name matchers outside `findvec()`, `doc/codex/issues/0032`

## Status

Accepted, 2026-08-11, branch `ver_50`. Closes `doc/codex/issues/0032`, which
was the whole of `set_case_mode()`'s experimental-mode clause. Applies decision
3's Class C rule of `doc/claude/decisions/0001-distinguish.md` to the six name
matchers that decision did not reach, and takes the step decision 2 of
`doc/claude/decisions/0004-unlet-vector-identity.md` deliberately left: it
exports `vec_name_eq()`. It re-decides nothing those records settled.

Commits: `18f751b4b` the three scale rows, `a08fc2504` `diff.c`, `d0ebd6b64`
`rawfile.c`, and the `set_case_mode()` text.

## Context

`findvec()` acquired the mode-aware predicate when Phase 3 gate 3 closed, and
`vec_remove()` took it with `0027`. Every other vector name matcher in the
frontend kept the `cieq()` it was written with, because the gate was scoped to
the lookup table and these do not use it.

Three of the six were the same bug: the current plot's **scale vector** was
identified by a case-insensitive name compare, so under `distinguish` a vector
whose name differed from the scale's only in case was treated as the scale.
Measured at `ca7d2bc07`, one tran deck with scale `time` and `let TIME = time
* 2` beside it:

```
print TIME    no scale column at all
print ally    TIME absent
unlet TIME    Warning: Scale vector 'time' of the current plot cannot be
              deleted!   -- naming a vector the user did not ask about
write f TIME  No. Variables: 1,  0 TIME notype   -- time dropped from the file
```

Two of the four are silent wrong output. Nothing in this record changes a
computed value under `fold` or `preserve`.

---

## Decision 1 — export `vec_name_eq()`; there is no `vec_is_plot_scale()`

**Decided: `vec_name_eq()` becomes non-`static` and is declared in
`src/include/ngspice/fteext.h`, and all six sites call it.**

`0032`'s acceptance criterion 1 offers the alternative — "each site given a
helper beside it (`vec_is_plot_scale(struct dvec *)` would cover three of the
six)" — and `0004` decision 2 states the constraint without choosing. The
helper reads better than a bare name compare, so it is worth saying exactly why
it is not available: **the three scale rows do not share a signature.**

- `is_scale_vec_of_current_plot()` (`src/frontend/postcoms.c`) has a *name* and
  no `struct dvec *`. Its caller `com_unlet()` has only the word the user
  typed, and it must decide whether to refuse *before* resolving it. Resolving
  first to obtain a `dvec` would be a second lookup with `findvec()`'s
  near-miss report attached, on a path that is about to report its own miss
  through `vec_remove(name, TRUE)`.
- `findvec_ally()` (`src/frontend/vectors.c`) has a `(struct plot *, struct
  dvec *)` pair, and the plot is the one being listed, which need not be
  `plot_cur`. A helper reading `plot_cur` would be wrong here.
- `vec_eq()` has two `struct dvec *` of which the second is the *plot's* scale
  at `postcoms.c:681` and `:846` but a *vector's own* scale, `d->v_scale`, at
  `:706` and `:871`. "Is this the plot scale?" is not the question it asks.

Three signatures, one shared question: are these two vector names the same
name? That is `vec_name_eq()`, exported once, reading the same at every site.
The header comment says so, so the next site does not have to re-derive it.

Rejected: **`Evt_Node_Name_Eq()`** (`src/xspice/evt/evtplot.c`). It is the
event side's twin of this same rule and none of these should call it. The two
are deliberately separate: the event predicate has no `v()`/`i()` wrapper arm,
because no event node is stored as `V(1)`, and coupling them would mean the
next change to either had to be reasoned about on both sides of the
mixed-signal boundary.

Rejected: **`inp_case_exact_ids()`**, for `0004` decision 1's reason verbatim.
These are lookups, not identity tests between two card tokens, and in `fold`
mode the query may still arrive with upper case in it.

## Decision 2 — `vec_eq()`'s six callers move as one, and `write` is the reason it matters

`vec_eq()` is the widest of the six: one `cieq()` decided `print`'s scale
column (`postcoms.c:306`), both `write` sites (`:681`, `:706`), both
`write_sparam` sites (`:846`, `:871`) and `agraf.c:62`'s x axis. All six are
scale comparisons, so they are one mechanism and one commit.

`0032`'s prompt asked whether the two `write` callers should be tightened given
that the previous session had **failed** to make `write` record the wrong
scale: the loop at `postcoms.c:664-684` assigns `newplot.pl_scale = vv` on
every match so the last match wins, `vec_new()` prepends, and a `let`-created
twin is therefore always earlier in `pl_dvecs` than the real scale and always
loses. That reading is correct and looks at the wrong limb.

The limb that bites is `scalefound`, one loop later. Measured, `distinguish`,
`set filetype=ascii`, `let TIME = time * 2` beside a tran scale `time`:

```
write out.raw TIME   before  No. Variables: 1    0  TIME  notype
                     after   No. Variables: 2    0  time  time
                                                 1  TIME  notype
```

Before, `vec_eq(TIME, tpl->pl_scale)` was true, so `scalefound` was set and the
block that would have copied the real scale into the new plot was skipped: the
rawfile declared the twin to *be* the scale and dropped `time` entirely. That
is the same loss `print` showed, in a file rather than on a terminal, and it is
not order-dependent luck.

It would have been worth fixing even if it had been. A behaviour that is
correct only because `vec_new()` prepends is a behaviour that a change to list
order silently breaks, and nothing in the tree records that `write` depends on
it.

## Decision 3 — the guard narrows, and the scale is protected exactly

`is_scale_vec_of_current_plot()` exists because deleting the scale crashes
redrawing (`postcoms.c:34-35`), and this record makes it narrower. The question a
narrowing has to answer is whether the real scale is still protected under
every spelling it can have.

It is, by construction rather than by inspection. `com_unlet()` refuses exactly
when `vec_name_eq(pl_scale->v_name, word)` and otherwise calls
`vec_remove(word, TRUE)`, which selects with `vec_name_eq(ov->v_name, word)`.
The guard and the removal are now **the same predicate over the same word**, so
the set of vectors `unlet` can reach is exactly the set the guard covers, minus
the scale. Any spelling that would resolve to `pl_scale` still refuses,
including one differing only in a `v()` or `i()` wrapper's letter, which is all
`vec_wrapped_name_eq()` admits.

What the narrowing gives up is refusals for words that `vec_remove()` would
have resolved to some *other* vector. That is not protection; it is the defect.
Under `fold` and `preserve` the predicate is `cieq()`, `cieq()` is symmetric,
and the swapped argument order is the same comparison, so the guard is byte
identical to the historical one in both shipped modes.

The residue is not new and is not this record's: `com_compose()` and
`com_cross()` call `vec_remove()` with no scale guard at all, and
`com_remzerovec()` clears `VF_PERMANENT` with none either — `doc/codex/issues/0035`
item (c). Those are missing guards, not narrowed ones.

## Decision 4 — `rawfile.c`'s `scale=` binding is tightened, not filed

**Decided: `vec_name_eq()`, and a sentence in the migration note.**

`0032`'s acceptance criterion 2 asked for this one to be *decided* rather than
tightened, because exactness converts a silent mis-bind into a loud refusal and
that is a visible change to `load`. Three reasons to tighten it:

1. It is the same Class C rule as the other five, and a single surviving
   `cieq()` among six name matchers is the shape that produced this issue —
   `0027` closed one function and left five.
2. It is a no-op in both shipped modes, so the visible change is confined to
   the experimental one, and there it is the mode's own semantics: a `scale=`
   naming a variable the file does not define is a dangling reference when two
   spellings are two names.
3. The loud answer already exists. The `else` branch prints
   `Error: no such vector %s` and leaves `v_scale` NULL, which is what the
   reader does for every other unresolvable reference. Nothing new had to be
   written to make the failure visible.

That third reason is what separates this from `0004` decision 5, where the
honest disposition *was* an issue: `cp_remkword()`'s mismatch is unfalsifiable
because `cp_ccom()` has no callers, so no change there could be landed
RED-first. This one is assertable, and is asserted.

`ngspice`'s own raw *writer* never emits `scale=`; only the reader understands
it. So the affected input is a hand-written rawfile, which is also how the deck
reaches it.

## Decision 5 — the experimental clause names the mode's limitation, not the next defect

**Decided: keep the word `experimental`, empty the clause of defects, and give
it decision 5's migration hazard as its subject.**

`0032` was the last thing the clause named, and `0004` decision 6 predicted
that closing it would need either a new subject or an empty sentence. Both
options were taken up and both rejected.

Rejected: **retarget to `doc/codex/issues/0034`**, `findvec()`'s near-miss
warning firing on `com_let()`'s left-hand side. It is the obvious candidate and
it is the wrong kind of thing. Every previous subject of this clause — `0029`,
`0027`, `0032` — was a *silence*: something quietly wrong in the output. `0034`
is a false positive. The deck it fires on produces every correct number; what
is wrong is that a warning appears beside them. Naming it in this sentence
would tell a user their output may be wrong at the one moment it is right, and
would train them to discount the sentence.

Rejected: **empty the clause and stop at "is experimental"**. `0004` decision 6
argues the word survives an empty clause, and it does. But it does not argue
that the clause *should* be empty, and the sentence is the one thing every
`distinguish` user reads. Spending it on nothing wastes it.

Accepted: the clause names decision 5 of `0001-distinguish.md` — a deck that
spells one net two ways becomes a deck with two nets, silently, because both
spellings are definitions and decision 2 deliberately does not warn on a
definition. It is a silence, so the sentence keeps its shape. It is the thing a
user most needs told at the moment they turn the mode on. And unlike every
previous subject it will never be closed: it cannot be fixed without
withdrawing the feature, only documented, which is `0004` decision 6's own
"strongest single reason for the word".

So the clause stops being a changelog of open issues and becomes a statement of
what the mode does. The word does not rest on it alone: `doc/codex/issues/0028`
is open on both sides of the mixed-signal boundary, and a name ngspice
constructs with upper case of its own — `q1#collCX` — must be typed as the
simulator spells it.

## Decision 6 — `vec_basename()` stays filed

**Decided: `doc/codex/issues/0035` item (a) does not ride along.**

It was the obvious passenger: `vec_basename()` is `vec_eq()`'s input, this
record rewrites `vec_eq()`, and `0035` calls (a) the only one of its three that
can corrupt memory rather than crash.

It stays filed for two reasons.

It is not a typo with an obvious repair. The offset
`v->v_name + strlen(v->v_name) + 1` is wrong, but so is the guard above it —
`cieq(pl_typename, v_name)` is a whole-name equality where a prefix test
belongs, and a vector whose entire name *equals* its plot's typename has no
prefix to strip. Fixing the offset without settling what
`vec_basename("tran1.out")` should return would replace a dead branch with a
live one whose behaviour nobody has decided. That cannot be landed RED-first
off the back of a case fix, and `AGENTS.md` makes RED-first non-negotiable.

And its urgency is lower than `0035` records. Reading the reachability while
working on `vec_eq()` showed the branch is dead for every plot ngspice names
itself: `pl_typename` is `"%s%d"` over `plotabs[]`, whose every `p_name` is a
bare word, so a typename never contains `.` and the two conditions cannot both
hold. The only route found is `deftype p my.type tran` plus a vector named
exactly `my.type1`. The analysis is added to `0035`, which is the deliverable
here.

## Evidence

Three decks, all in `tests/regression/casedist/`, all proved RED through
`make check` rather than by hand:

| Deck | RED, and against what | Asserts |
| --- | --- | --- |
| `vector-scale-case.cir` | `ca7d2bc07` | `print`'s scale column, `ally`'s membership, `unlet`'s refusal, and that the twin is really gone afterwards |
| `vector-diff-case.cir` | `18f751b4b` | one `>>>` line per selected vector instead of two |
| `vector-rawfile-scale-case.cir` | the predicate at `rawfile.c:618` reverted to `cieq()` and the tree rebuilt, then restored bit-identically (`md5sum -c` and an empty `git diff`) | the reader's refusal, captured, and `display`'s `scale =` column |

`vector-scale-case.cir` was also run against an empty `.out` and fails there, so
its evidence is output rather than an absence.

- `make check`: **261 PASS, 0 FAIL**, up from 258 by exactly these three decks.
- `tests/regression/casedist`: 13 for 13.
- `tests/regression/case/`, the `preserve` twin: not touched, and passes.
- `python3 doc/claude/scripts/case_differential_sweep.py --jobs 12 --timeout 90`
  on the binary before this work and the binary after, on the same tree:
  **302 decks, DIFF=53, OK=246, SKIP=3, PARSE-FAIL=0, NUM-DIFF=0 in both**, and
  the non-OK lines are byte identical between the two logs. No deck moves,
  including the `alter-rebin*` and `ring51` decks that flap run to run.

The sweep never runs `distinguish`, so it is a collateral-damage guard for
`fold` and `preserve` and not evidence about this work. The three new decks
appear in its DIFF list for that reason — they are being run in the wrong mode
on purpose — exactly as `harness-alive.cir` and the other `casedist` decks
already did.

## Corrections to this record's own commits

Found by reviewing the three commits after they landed, which is the step
`0027`'s session added to this workflow after its own commit message asserted
something false.

**`18f751b4b` says the deck "fails with 61 added lines" against an empty
`.out`. It is 60.** The count came from `grep -c '^+'` over the `diff -u`
output, which also counts the `+++ vector-scale-case.test_tmp` header. The
claim it supports — that the deck's evidence is output rather than an absence,
so an empty reference cannot pass it — is unaffected.

**`18f751b4b` and `a08fc2504` quote line numbers from before their own edits.**
Both commits add comments above the lines they cite, so `postcoms.c:301`,
`:676`, `:701`, `:841`, `:866` and `:659-679`, and `diff.c:192-215`, are each
short by the comment they introduced. The current numbers are `306`, `681`,
`706`, `846`, `871`, `664-684` and `198-215`, and they are what the docs cite.
This is the hazard `ca7d2bc07` fixed for `0032` and `0035` one commit before
this work started, arriving again by the same route; re-grepping *after* the
edit rather than before is the habit that prevents it.

The `set_case_mode()` comment was rewritten to occupy exactly the seventeen
lines the old one did, for the same reason: `src/frontend/inpcom.c` is by far
the most heavily line-cited file in `doc/` — 205 citations, 189 of them below
the comment, across 21 documents — and a comment that grew by twelve lines
would have moved every one of those 189. The argument that would have made it
longer belongs in this record anyway.

**`src/frontend/streams.c:153` is a blank line.** Four places cite it for the
fact that `>&` redirects `cp_err` as well as `cp_out` — `0001` decision 2's
correction, `0004` decision 3, `vector-unlet-report.cir` and, copied from them,
`vector-scale-case.cir`. The assignment is `cp_err = fp` at `:156`, and all
four now say so. Not caused by this work; found by checking a citation before
reusing it, which is the same habit stated above from the other end.

The 65 citations the other four files' comments *did* move were rewritten by
the exact shift each edit introduced, which is the one transform that cannot
turn a correct citation into a wrong one and leaves an already-stale one
pointing where it always pointed.

## What this decision does not decide

1. **`doc/codex/issues/0037`**, `diff`'s pairing of a vector in one plot with
   its twin in the other. Found by this work, in the function this work
   changed, and the opposite direction: `nghash`'s `strcmp` is byte exact in
   all three modes, so it is too strict under `fold` and `preserve`. Measured,
   not assumed. Fixing it is the fold-the-key-and-filter-the-chain shape the
   vector table already uses, and it is a shipped-mode defect rather than part
   of `0032`.
2. **`doc/codex/issues/0034`**, per decision 5. Rejected as the clause's
   subject, not fixed.
3. **`doc/codex/issues/0035`**, per decision 6, all three items.
4. **`doc/codex/issues/0033`**, `cp_remkword()`'s spelling, per `0004`
   decision 5.
5. **`doc/codex/issues/0036`**, deck coverage for `0002`'s and `0003`'s three
   diagnostics. Cheaper now than when it was filed: `vector-rawfile-scale-case.cir`
   adds a `while` loop over `fread` to the capture technique, which removes the
   need to know how many lines a command writes before its diagnostic.
6. **Whether `rawfile.c`'s `scale=` miss should report the near miss** rather
   than only the resolution failure. It reports `Error: no such vector %s`,
   which is loud but does not name the twin. `vec_warn_case_near_miss()` wants
   the plot's lookup table and a loaded plot has none (`vectors.c:614` leaves
   `keywords[CT_VECTOR]` NULL), so reusing the wording would mean duplicating
   it. Recorded in `0001-distinguish.md` decision 2's table.
7. **The build-enforced lint** that would stop a new `strcmp` or `cieq` against
   a vector name from re-entering the tree. It should go last, after there is
   nothing left for it to flag.
