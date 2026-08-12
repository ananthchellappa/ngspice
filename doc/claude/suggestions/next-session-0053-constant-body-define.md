# Next-session prompt — `doc/codex/issues/0053`, the constant-bodied `define`

Paste everything below the line into a fresh session at
`/home/qflow/dev/ngspice_test`, branch `ver_50`.

Why this one next, ahead of `0051` and everything else open: it is the only
open defect that is **fatal in the shipped default mode from three lines of
input**, and its other face is a silently wrong number in a batch deck that
exits 0. Everything else in the queue is confined to the experimental mode, or
fails loudly, or is a missing diagnostic. It also **blocks `0051`** criterion 4:
the missing `free_pnode()` on `com_define()`'s replacement path cannot be added
safely until this is fixed.

---

Fix `doc/codex/issues/0053` on branch `ver_50`. It is **not** a `casemode`
defect — nothing in it is about case — but it lives in the middle of the file
this series has been editing, so read the case records for the file's
conventions and then set them aside.

Read, in this order and in full:

1. `doc/codex/issues/0053-define-with-a-constant-body-is-freed-on-first-use.md`
   — all six acceptance criteria. Criterion 3 names two candidate repairs and
   requires the loser's reasons.
2. `src/frontend/define.c` end to end. It is 550 lines and the whole defect is
   inside it plus one function in `parse.c`. Pay attention to the comment above
   `trcopy()` — *"we never free parse trees, so we needn't worry that they
   aren't trees here"* — which is **stale**: it dates from the initial revision
   (`978f1c32a`, 2000) and its premise was retired by `cbdd811aa` (2005),
   *"Fixed bug with define … `free_pnode()` now copes properly with parse trees
   that reuse leaf nodes"*, the commit that added `pn_use` refcounting to
   `free_pnode_x()` **and** the three `pn_use++` lines inside `trcopy()`
   itself. Read that commit before you touch anything; it is the design you are
   completing, not one you are overriding.
3. `free_pnode_x()`, `src/frontend/parse.c:700-722`, and the `free_pnode()`
   macro beside it. The `if (t->pn_use > 1)` test is the whole mechanism.
4. `PP_mkfnode()`, `src/frontend/parse.c:499-569`, specifically the
   `q = ft_substdef(func, arg); if (q) { … return q; }` block at `:517-523`.
   That is the frame that hands the stored node to the evaluator without
   bumping it.
5. `doc/claude/decisions/0013-user-defined-function-identity.md` — decision 3
   for the classification of this file's comparisons, which you must not
   disturb, and **decision 2 for the method**. That is a decision that shipped
   GREEN with a passing lint and a full Evidence section and was then reversed
   by an adversarial re-read which ran the case the Evidence had not. This
   session has exactly that risk: a refcount fix that looks right for the case
   in the issue and leaks or double-frees for a case beside it.

## The state, measured at `528c3defd`, default mode, no `casemode` set

At the prompt:

```
$ ngspice -p
ngspice 1 -> define c(x) 5
ngspice 2 -> print c(2)
c(2) = 5.000000e+00
ngspice 3 -> print c(2)
Segmentation fault          ($? = 139)
```

The same deck in `--batch`, which does **not** crash and is worse for it:

```
c(2) = 5.000000e+00
c(2) = 2.000000e+00        <- the ARGUMENT, not the body.  Exit status 0.
```

A third face, the body being a vector name rather than a constant:

```
let a = 5
define g(x) a
print g(1)   ->  5.000000e+00
print g(1)   ->  1.000000e+00      and stays wrong
```

Not affected, and this is the discriminator that localises it:

```
define c(x) 5+0    correct three times      (operator at the root)
define c(x) x      correct three times      (the root IS the formal)
```

`undefine` on an affected function is also fatal: `define c(x) 5`,
`print c(2)`, `undefine c` segfaults, and valgrind reports `Invalid free()`.

**The mechanism, as far as it has been read.** `trcopy()`'s three
node-building branches each allocate a fresh node and bump the child —
`pn->pn_left->pn_use++` and siblings. The `pn_value` branch does not: it
returns `tree`, the **stored** node, deliberately, because the design shares
leaves. The bump is always done by the parent inside `trcopy()`; when the
body's root *is* the value node there is no parent, `ft_substdef()` hands it up,
`PP_mkfnode()` returns it unbumped, and it reaches the evaluator with
`pn_use == 0`. `free_pnode_x()` then takes its `else` branch and frees the
node, its `pn_name`, and its `pn_value` unless that vector is `VF_PERMANENT`.

That reading is **not** a measurement. Confirm it before you fix it — a printf
or a debugger on `pn_use` at the three points is the cheapest way, and if the
reading is wrong the whole prompt below is wrong with it.

## The decision this session has to make and record

**WHERE THE MISSING REFERENCE COUNT GOES.** Criterion 3's two candidates, plus
one the issue does not name:

- **(a) `ft_substdef()` bumps what it returns**, when the returned tree is the
  stored root. Narrow, and it is the frame that knows the node came out of
  `udfuncs`. Its cost: it has to distinguish "I am returning the stored root"
  from "I am returning something `trcopy()` built", and `trcopy()`'s
  formal-substitution path returns `ntharg(i, args)` — a node from the
  *caller's* argument list, which the caller already owns. Bumping that one
  would leak.
- **(b) `trcopy()`'s `pn_value` branch bumps before returning.** Broader and
  simpler to write. Same hazard as (a) at the `ntharg()` return, one branch
  earlier, and it also fires for every *interior* value node — where the parent
  already bumps — so it would double-count unless the parent's bump is removed
  in the same edit. Work out whether that is a net simplification or two
  changes wearing one hat.
- **(c) `PP_mkfnode()` bumps `q`.** The one frame that knows the result is
  about to be handed to an evaluator that will free it. Its cost is that it is
  the wrong layer — it would bump for every user-defined call, including the
  ones `trcopy()` already counted.

There may be a fourth answer: make `ft_substdef()` return a **copy** of a
single-node body rather than the node. The issue's criterion 3 says the fix
should be a refcount and not a copy, and that is a preference rather than a
proof — if you can show the copy is correct and the refcount is not, say so and
overrule it. What you may not do is leave the sharing design half-implemented.

**Whichever you choose, the acceptance is not "the repro stops crashing".** It
is that all four shapes below behave, and that valgrind is clean on each:

| shape | today |
| --- | --- |
| `define c(x) 5` × 3 calls | 5, then SEGV / wrong |
| `define g(x) a` after `let a = 5`, × 3 | 5.0, then 1.0, 1.0 |
| `define c(x) 5+0` × 3 | correct — must stay correct |
| `define c(x) x` × 3 | correct — must stay correct |
| `define sq(x) x*x` × 3 | check it: the arg node appears twice in one result |
| `define f(x) x*3` then `define g(y) f(y)+1` then `undefine f` then `g(5)` | measured to give `1.6e+01` and 0 valgrind errors today; must not regress |

## Deliverables

- The mechanism, with the losing candidates' costs stated.
- A RED in `tests/regression/misc/` — **not** in a `case` directory; nothing
  here is about case, and putting it there would mislabel it. The batch failure
  is a **value**, `c(2) = 2.000000e+00` where `5.000000e+00` is right, so it
  needs no capture-and-read-back. It must fail at `528c3defd`; quote the
  failure. Add the deck to that directory's `TESTS` and re-run `./autogen.sh`.
- A second RED, or a documented reason there is not one, for the **prompt**
  face. `tests/regression/pipe/` runs `.cmd` files through `ngspice -p` and is
  where a segfault would be caught by exit status; check whether that harness
  can see a non-zero exit at all before assuming it can.
- `doc/claude/decisions/0014-<slug>.md`, and a Resolution on `0053`.
- A sentence in `doc/codex/issues/0051` recording the sequencing: its criterion
  4 is unblocked by this.

## Baselines, measured at `528c3defd`

- full `make check` = **282 PASS, 0 FAIL**, exit 0. That is the bar. Per
  directory: `regression/case` 113, `regression/casedist` 23,
  `regression/misc` 27, `regression/pipe` 16, `xspice/case` 21,
  `xspice/casedist` 17, `lint` 2.
- `make -C build-ver_50/tests/lint check` = **"identity lint: 268 comparisons,
  baseline matches"**, ~1.6 s. This session should not move it: a refcount is
  not a string comparison. If it does move, something unintended was edited.
- `python3 doc/claude/scripts/case_differential_sweep.py --jobs 8 --timeout 90`
  = 321 decks, `DIFF=61`, `OK=257`, `SKIP=3`, `PARSE-FAIL=0`, `NUM-DIFF=0`.
  The last two must stay 0. `tests/regression/case/udf-name-case{,-lower}.cir`
  are `DIFF` **by design** on `+'vm (x) = mag (v (x))'`, which is
  `doc/codex/issues/0051`'s residue and not yours.
- `python3 doc/claude/scripts/deck_output_differ.py --before X --after Y
  --jobs 8 --timeout 200 --mode {,preserve,distinguish}` — at no change, fold
  3, preserve 4, distinguish 0, where the four `alter-rebin` decks flap within
  a recorded ±2. **This change should move nothing in any mode**, because no
  shipped deck and no `ft_cpinit()` body has a single-node root; if a deck
  moves, find out why before explaining it away.
- valgrind is available and was used to measure this. `100 ×` a redefinition of
  `define f(x) x*3` loses **531 B each** (69 B direct = 64 B root `pnode` + 5 B
  packed name, plus 462 B indirect); the same 100 with an `undefine` between
  them lose 2 B total and report 0 errors. Those are `0051`'s numbers, quoted
  so you can tell its leak from your double-free.

## Hazards

- **The `trcopy()` comment is stale and will mislead you.** See reading item 2.
- **`ft_cpinit()`'s shipped `udfs[]` cannot reproduce this** — all fourteen
  bodies have an operator or a function at the root — so `make check` passing
  says nothing about this defect. Only your new deck will.
- The prompt face and the batch face disagree because the outcome is an
  allocator accident. Do not treat "it stopped crashing" as GREEN; the batch
  wrong number is the assertable half.
- **Run each repro in its own process.** Two probes in one session contaminate
  each other — this bit the `0052` measurement, where two `save` commands in
  one run reported a mode as broken in both directions when it was not.
- **The X server in this WSL environment dies under load.** Two full
  `make check` runs made while a measurement script was busy each failed a
  different `tests/regression/pipe` deck immediately after
  `X connection to :0 broken (explicit kill or server shutdown)` — Xlib's fatal
  handler killing ngspice. Each deck passes on its own. Do not run `make check`
  concurrently with the differ, and re-run before believing a lone `pipe`
  failure.
- `make -k check` when you want the whole suite: a failing directory otherwise
  aborts automake's recursion and the totals become a lower bound.
- **BOTH measuring scripts `chdir` per deck**, so every binary path passed to
  them must be **ABSOLUTE**; a relative one dies with `FileNotFoundError` and
  leaves a ~1.7 KB output file that looks like a finished run. Snapshot the
  binary first: `cp build-ver_50/src/ngspice <scratch>/ngspice-baseline`.
- `tests/.gitignore` ignores `*.out`; every reference needs `git add -f`.
- `./autogen.sh` after any `Makefile.am` edit, then `../configure` in
  `build-ver_50`. A `.cir` absent from its directory's `TESTS` is silently
  never run.
- Do not run a deck from inside its own source directory — it writes rawfiles
  into `tests/`. Use a scratch directory.
- A shell `cd` persists between calls. Never pipe `make` through `head`.
  Re-grep every line number after your edits.

## Deliberately out of scope — enumerate, do not fix

- **`doc/codex/issues/0051`**, the `prefix()` over-match in the same function:
  `define f(x)` destroys a stored `foo(y)`, `define m(a,b)` destroys the
  shipped `min`. It is the natural next session and its criterion 4 depends on
  yours, but it is a different mechanism and belongs in a different commit.
  Its scoping is done: `udf_name_eq(udf->ud_name, b) && (arity == udf->ud_arity)`
  is the whole comparison repair — measured, `cieq` is `ciprefix` plus a final
  `return *s == '\0';`, which is exactly the missing terminator test, and
  `cieq("VD","vdb")` is 0 where `ciprefix` is 1, so the widening that forced
  `0013` decision 2 belongs to `ciprefix` and not to case-insensitivity.
- **The default `casemode`.** It stays `fold`, by decision, for backward
  compatibility.
- `doc/codex/issues/0052` (the `.tf` result vector names — its own prompt
  exists), `0050`, `0048`, `0047`, `0046`, `0043`, `0042`, `0041`, `0039`,
  `0038`, `0035`, `0033`, `0031`, `0025`, `0024`, `0021`, `0016` class (b),
  `0012`, `0008`, `0007`, `0006`, `0005`, `0004`, and `com_let()`'s `plainlet`
  leading space per `0009` deferral 4.
- The ~159 B per `define` that stays **reachable** rather than lost —
  `PP_mknnode()` `vec_new()`s the constant's dvec into the current plot and
  `savetree()` then shadows it with a fresh copy. Measured, not a leak, not
  this issue.

Anything new you find and do not fix goes in `doc/codex/issues/NNNN-slug.md`
with the Status / Summary / Impact / Root Cause / Acceptance Criteria /
Resolution structure. Next free number is **0054**.

## Conventions

- Match the style of the file being edited; `src/frontend/` is four-space,
  same-line brace, and puts a static function's return type on its own line.
  Do not reflow.
- `make check` lints every `strcmp`/`cieq`/`eq`/`eqc` in `src/` whose operands
  are both runtime expressions. You should not add one; if you do, classify it
  in the source with a `case-lint:` comment rather than in the baseline. See
  `AGENTS.md` and `doc/claude/decisions/0011-identity-lint.md`.
- One commit per mechanism, one for the documentation. Commit messages: short
  imperative summary, then Problem / Mechanism / RED / Modes.
- **Review your own commits adversarially before declaring it done.** For a
  refcount change the specific questions are: does every path that returns a
  shared node now bump exactly once, does every path that returns an owned node
  still bump zero times, and is there a shape where the new bump makes the node
  immortal — a leak traded for a crash. Run valgrind on all six shapes in the
  table above, not on the one in the issue. Then ask the general ones: is every
  token I assert reachable, can any assertion pass vacuously, does each
  artefact fail for the reason I say, is every factual claim in my commit
  message something I ran against THIS commit, and does `git show --stat` list
  only files I meant to touch?

## Stop and report

What changed per file, the RED before and the PASS after for both faces, which
candidate you chose for the reference count and what the losers' costs were,
the valgrind result for each of the six shapes, whether `tests/regression/pipe`
can see a non-zero exit status, the sweep and differ verdicts compared per deck
in all three modes, your adversarial review of your own commits, and the full
`make check` result.
