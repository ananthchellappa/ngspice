# Next-session prompt — `doc/codex/issues/0054`, nodes handed out with no owner

Paste everything below the line into a fresh session at
`/home/qflow/dev/ngspice_test`, branch `ver_50`.

Why this one next: `0053` closed the worst member of this family and left
three siblings measured and open, all in the two functions that session had
open in front of it. Class (a) is a use-after-free on an ordinary line that a
one-call deck survives with **exit 0** — the same silent face `0053` had, and
the only such defect left in `src/frontend/`. It is also cheap now in a way it
will not be later: the invariant is already written down, the seven probe
shapes already exist, and `0051` is queued behind both.

---

Fix `doc/codex/issues/0054` on branch `ver_50`. It is **not** a `casemode`
defect — class (a) was measured identical under `fold`, `preserve` and
`distinguish` — but it lives in the two functions the case series has been
editing, so read the case records for their conventions and then set them
aside.

Read, in this order and in full:

1. `doc/codex/issues/0054-nodes-handed-out-of-ft-substdef-with-no-owner.md` —
   all six acceptance criteria. Criterion 1 warns that the obvious deck for
   class (a) passes **vacuously**; understand why before writing it.
2. `doc/claude/decisions/0014-single-node-define-body.md` — this is the
   prompt's spine, not background. It states the `pn_use` invariant these three
   violate, it is where the seven-shape valgrind table and the control decks
   come from, and its "Evidence" section is the format this session's answer
   should take. Its method section matters as much as its conclusion: the
   winning candidate there was chosen by **building and running the loser**,
   not by reasoning about it.
3. `src/frontend/define.c` — `ft_substdef()`, `trcopy()`, `ntharg()`,
   `com_undefine()`. `copy_value_node()` at the bottom is `0053`'s repair and
   the shape a repair in this file now takes.
4. `PP_mkfnode()`, `src/frontend/parse.c:499-569`. **This is where two of the
   three defects are**, and the whole of class (a) is the four lines at
   `:518-524` — `if (q) { if (arg->pn_op && comma) free_pnode(arg); return q; }`.
5. `free_pnode_x()`, `src/frontend/parse.c:700-722`. The `if (t->pn_use > 1)`
   test and the `t->pn_use == 1` guard on `vec_free()` are two different
   thresholds and both matter here — (a) turns on the first, (c) on the second.
6. `git show cbdd811aa` — the 2005 commit that introduced `pn_use`. Same
   reading as `0053`: you are completing that design, not overriding it.

## The state, measured at `0eb1e6bc7`, default mode, no `casemode` set

Every figure below was taken at `0eb1e6bc7`. The commit after it, `5bd283f9e`,
touches `doc/` only, so the binary is unchanged and these hold at HEAD as
written — a rebuild is still the first thing to do, not an assumption.

**All three classes were measured both before and after `0053`'s fix and are
unchanged by it.** None is a regression from it; it neither caused nor fixed
any of them.

### (a) a bare formal as the body, at arity ≥ 2

```spice
.control
op
define d(x,y) x
print d(2,3)
.endc
```

```
checkvalid: Internal Error: bad node
ft_evaluate: Internal Error: bad node
```

The call count selects the face, and **this is the trap in the acceptance
criteria**:

| calls | `SURVIVED` | exit | valgrind |
| --- | --- | --- | --- |
| 1 | printed | **0** | `Invalid write`, `Invalid read` |
| 2 | never | **134** | — |
| 3 | never | **134** | 67 errors, 22 `Invalid read`/`write`, 0 bytes lost |

So a deck that calls once and asserts only the exit status **passes today**.
Assert the value, or the absence of the two `Internal Error` lines, or call
twice. No wrong number is ever printed — the node is gone before
`ft_evaluate()` reaches it — so this is not `0053`'s wrong-value shape and the
assertion has to be built differently.

Identical exit 0 on a one-call deck under all three case modes.

Not affected, and this is what localises it:

```
define d(x,y) x+0    correct twice      (an operator over the formal)
define p(x) x        correct three times (arity 1)
```

### (b) 64 B per call, arity 1, body ignores its formal

Three calls each, `definitely lost`:

| definition | lost |
| --- | --- |
| `define f(x) x*3` | 0 |
| `define f(x) x*x` | 0 |
| `define f(x) 5+0` | **192 B in 3 blocks** |
| `define f(x) 5` | **192 B in 3 blocks** |
| `define f(x,y) x+y` | 0 |
| `define f(x,y) x+0` | 0 |
| `define f(x,y) 5+0` | 0 |

`print vm(1)` × 3 loses **0** — no shipped function is exposed, because they
all use their formals. 2000 calls of `define c(x) 5` lose
`128,000 bytes in 2,000 blocks`.

### (c) 160 B per `undefine` of a value-rooted body

With **no call in between**:

| definition | lost |
| --- | --- |
| `define c(x) 5` | 160 B direct + 10 B indirect |
| `define c(x) x` | 160 B direct + 2 B indirect |
| `define c(x) 5+0` | 0 |

**The mechanism, as far as it has been read.** All three are the same missing
question at three frames — *who owns this node after it leaves here?*

- (a) `trcopy()`'s formal branch returns `ntharg(i, args)`. At arity ≥ 2 that
  is `args->pn_left`, whose `pn_use` of 1 is held by the comma node — and
  `PP_mkfnode()` then frees exactly that comma node, so `free_pnode()` reaches
  a child at the floor of its count and frees the node the caller is about to
  evaluate.
- (b) the same comma-node free is what *collects* an unused argument, so at
  arity 1, where there is no comma node, nothing does.
- (c) `free_pnode_x()` frees `pn_value` only at `pn_use == 1`, and a stored
  root rests at 0.

**(a) and (b) are one free seen from both sides.** Deleting the
`free_pnode(arg)` fixes (a) and worsens (b); adding an unconditional one fixes
(b) and worsens (a). A repair that treats them as two independent bugs will
oscillate. Say which single rule covers both.

That reading is **not** a measurement for (a) — the tables above are, but the
sentence about `args->pn_left` is inference from the source. Confirm it before
you fix it. `0053`'s session did this with three `fprintf`s on `pn_use` in
`trcopy()`, `ft_substdef()`, `PP_mkfnode()` and `free_pnode_x()`; that patch is
worth recreating, it costs one incremental build, and it is what turned a
plausible story into the one in decision 0014.

## The decision this session has to make and record

**WHERE THE OWNERSHIP OF THE ARGUMENT LIST GOES.** Candidates, none of them
yet measured:

- **(i) `trcopy()`'s formal branch bumps `ntharg()`'s return.** Narrow, and it
  is the frame that knows the node is being handed to someone else. Its cost:
  at arity 1 the node then reaches `free_pnode()` with `pn_use == 1` instead of
  0, which crosses the `t->pn_use == 1` threshold and starts `vec_free()`ing
  the argument's dvec. That dvec is in the current plot and `ft_evaluate()` may
  have handed it to a caller that frees it too — **check for a double free
  before believing this one**, and note that `0053` rejected a `pn_use = 1` on
  its copy for exactly this reason.
- **(ii) `PP_mkfnode()` stops freeing the comma node and frees the arguments it
  did not give away.** Puts both halves in one frame, which matches the "one
  rule" requirement. Its cost is that `PP_mkfnode()` does not know which
  arguments `ft_substdef()` kept.
- **(iii) `ft_substdef()` returns a tree that shares nothing with `args`,** the
  way `0053` made it share nothing with `ud_text` — `trcopy()`'s formal branch
  copies rather than returns `ntharg()`. Symmetric with the repair already in
  the file, and it makes `PP_mkfnode()`'s existing free correct as written.
  Its cost is a copy per substituted formal, and `x*x` substitutes twice.

`0053` overruled its own issue's stated preference on measurement. Do the same
here if the measurement says so: **the criteria in `0054` name no preferred
candidate, deliberately.**

**Whichever you choose, the acceptance is not "the Internal Error stops".** It
is that all of these behave and valgrind is clean on each:

| shape | today |
| --- | --- |
| `define d(x,y) x` × 1 | `Internal Error` ×2, exit 0 |
| `define d(x,y) x` × 3 | `Internal Error` ×2, exit 134 |
| `define t(x,y,z) y` × 1 | `Internal Error` ×2, exit 0 |
| `define d(x,y) x+0` × 2 | correct — must stay correct |
| `define p(x) x` × 3 | correct — must stay correct |
| `define sq(x) x*x` × 3 | correct, 0 lost — the arg node appears twice |
| `define f(x) 5+0` × 3 | correct, **192 B lost** — class (b) |
| `define c(x) 5` / `undefine c`, no call | **170 B lost** — class (c) |
| `print vm(1)` × 3 | correct, 0 lost — must stay 0 |
| `define f(x) x*3` / `define g(y) f(y)+1` / `undefine f` / `g(5)` | `1.6e+01`, unchanged since `0053` |

## Deliverables

- The mechanism for all three classes, confirmed by instrumentation and not by
  reading, with the losing candidates' costs stated.
- A RED in `tests/regression/misc/` — **not** in a `case` directory. Class (a)'s
  face is two `Internal Error` lines and, from the second call, a lost
  `SURVIVED`; neither is a value, so say in the deck's header what the
  assertion actually rests on. It must fail at `0eb1e6bc7`; quote the failure.
  Add it to that directory's `TESTS` and re-run `./autogen.sh`.
- A decision on whether (b) and (c) get decks at all. They are leaks, and
  `make check` has no leak harness — so either a deck asserts something
  observable about them or the issue records that the valgrind figures in the
  Resolution are the only assertion. Do not invent a passing deck that does not
  test them.
- `doc/claude/decisions/0015-<slug>.md`, and a Resolution on `0054`.
- Re-read `doc/codex/issues/0051` criterion 4 once more: it is unblocked, and
  class (c) is in `free_pnode()`'s treatment of a root, which is the same call
  `0051` wants to add to `com_define()`'s replacement path. Record whether
  `0051` should now land **with** this or after it.

## Baselines, measured at `0eb1e6bc7`

- full `make check` = **285 PASS, 0 FAIL**, exit 0. That is the bar. Per
  directory: `regression/case` 113, `regression/casedist` 23,
  `regression/misc` 29, `regression/pipe` 17, `xspice/case` 21,
  `xspice/casedist` 17, `lint` 2.
- `make -C build-ver_50/tests/lint check` = **"identity lint: 268 comparisons,
  baseline matches"**. This session should not move it: an ownership fix is not
  a string comparison. If it does move, something unintended was edited.
- `python3 doc/claude/scripts/case_differential_sweep.py --jobs 8 --timeout 90`
  = **323 decks, `DIFF=62`, `OK=258`, `SKIP=3`**, `PARSE-FAIL` and `NUM-DIFF`
  absent from the summary, which is how that script prints zero. Those two must
  stay 0. Note the `0053` prompt recorded `DIFF=61, OK=257` for the 321-deck
  tree; that was re-measured at `1eb641b24` with the two new decks removed and
  is **`DIFF=62, OK=256`** — the recorded 61 was off by one and it is not worth
  chasing again.
- `python3 doc/claude/scripts/deck_output_differ.py --before X --after Y
  --jobs 8 --timeout 200 --mode {,preserve,distinguish}` — across `0053` the
  only decks that moved other than the two intended ones were
  `alter-rebin-case-lower`, `alter-rebin-param-case{,-lower}` and `binning-1`,
  flapping within the recorded ±2, at fold 3 / preserve 1 / distinguish 2.
  **This change should move nothing**: no shipped deck defines a function of
  arity ≥ 2 whose body is a bare formal. If a deck moves, find out why before
  explaining it away.
- valgrind is available and was used for every figure above.

## Hazards

- **A binary copied out of the build tree resolves a different `spinit` and
  loads a different set of code models.** Two `cmp`-identical binaries gave
  `426,370` vs `449,433` allocs on one deck for this reason alone, and it
  invalidated a whole before/after leak table in the `0053` session before it
  was caught. Keep snapshots in scratch, but `cp` the one under test **into**
  `build-ver_50/src/ngspice` and run it from there, so both sides execute from
  the identical path. Only the two `doc/claude/scripts/*.py`, which take an
  absolute `--ngspice` and pass their own `--spinit`, are safe to point at a
  snapshot directly — and they still need the path to be **ABSOLUTE**; a
  relative one dies with `FileNotFoundError` and leaves a ~1.7 KB output file
  that looks like a finished run.
- **The one-call deck for class (a) exits 0 today.** This is the single most
  likely way to ship a vacuous test this session. See criterion 1.
- **`ft_cpinit()`'s shipped `udfs[]` cannot reproduce (a) or (b)** — every one
  of the fourteen uses its formals, and the two-argument ones wrap them in
  operators — so `make check` passing says nothing. Only your new deck will.
- **Run each repro in its own process.** Two probes in one session contaminate
  each other; class (a) corrupts the heap, so a second probe after it in the
  same run is measuring rubble.
- **The X server in this WSL environment dies under load.** Do not run
  `make check` concurrently with either measuring script, and re-run before
  believing a lone `tests/regression/pipe` failure.
- `make -k check` when you want the whole suite: a failing directory otherwise
  aborts automake's recursion and the totals become a lower bound.
- `tests/.gitignore` ignores `*.out`; every reference needs `git add -f`.
- `./autogen.sh` after any `Makefile.am` edit, then `../configure` in
  `build-ver_50`. A `.cir` absent from its directory's `TESTS` is silently
  never run.
- Do not run a deck from inside its own source directory — it writes rawfiles
  into `tests/`. Use a scratch directory.
- A shell `cd` persists between calls. Never pipe `make` through `head`.
  Re-grep every line number after your edits.

## Deliberately out of scope — enumerate, do not fix

- **`doc/codex/issues/0051`**, the `prefix()` over-match in `com_define()`.
  Unblocked by `0053` and the natural session after this one; its scoping is
  done and recorded in its criterion 4.
- **The default `casemode`.** It stays `fold`, by decision.
- The ~159 B per `define` that stays **reachable** rather than lost —
  `PP_mknnode()` `vec_new()`s the constant's dvec into the current plot and
  `savetree()` then shadows it with a fresh copy. Measured, not a leak.
- `doc/codex/issues/0052`, `0050`, `0048`, `0047`, `0046`, `0043`, `0042`,
  `0041`, `0039`, `0038`, `0035`, `0033`, `0031`, `0025`, `0024`, `0021`,
  `0016` class (b), `0012`, `0008`, `0007`, `0006`, `0005`, `0004`, and
  `com_let()`'s `plainlet` leading space per `0009` deferral 4.

Anything new you find and do not fix goes in `doc/codex/issues/NNNN-slug.md`
with the Status / Summary / Impact / Root Cause / Acceptance Criteria /
Resolution structure. Next free number is **0055**.

## Conventions

- Match the style of the file being edited; `src/frontend/` is four-space,
  same-line brace, and puts a static function's return type on its own line.
  Do not reflow. `copy_value_node()` in `define.c` is a fresh example.
- `make check` lints every `strcmp`/`cieq`/`eq`/`eqc` in `src/` whose operands
  are both runtime expressions. You should not add one; if you do, classify it
  in the source with a `case-lint:` comment rather than in the baseline. See
  `AGENTS.md` and `doc/claude/decisions/0011-identity-lint.md`.
- One commit per mechanism, one for the documentation. Commit messages: short
  imperative summary, then Problem / Mechanism / RED / Modes.
- **Review your own commits adversarially before declaring it done.** For an
  ownership change the specific questions are: does every path that returns a
  node the caller will free now own it exactly once, does any path now free a
  node twice, and does the new rule cross `free_pnode_x()`'s *second*
  threshold — the `pn_use == 1` guard on `vec_free()` — for a dvec that lives
  in a plot or that `ft_evaluate()` has already handed to a caller. Run
  valgrind on all ten shapes in the table above, not on the one in the issue.
  Then ask the general ones: is every token I assert reachable, can any
  assertion pass vacuously, does each artefact fail for the reason I say, is
  every factual claim in my commit message something I ran against THIS commit,
  and does `git show --stat` list only files I meant to touch?

## Stop and report

What changed per file, the RED before and the PASS after, which candidate you
chose for the argument list's ownership and what the losers' costs were, the
valgrind result for each of the ten shapes, whether classes (b) and (c) got
decks and why, the sweep and differ verdicts compared per deck in all three
modes, your adversarial review of your own commits, and the full `make check`
result.
