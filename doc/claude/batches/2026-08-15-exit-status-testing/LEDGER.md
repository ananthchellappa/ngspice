# Batch ledger — exit-status testing

**Branch** `ver_50`. **HEAD at start** `f14fc04aa`. **Baseline** 322 PASS /
0 FAIL (`731c01455`; every commit since is documentation only).
Plan: `PLAN.md`. Receipts: `receipts/`.

## Items

| # | item | crew | outcome | commit |
| --- | --- | --- | --- | --- |
| 1 | harness: `check_status.sh` + `tests/regression/exitstatus/` | 01 | landed, 324 PASS / 0 FAIL | `baccfa570` |
| 2 | 0069 criterion 3: `$sim_status` guard deck | 02 | landed, 328 PASS / 0 FAIL | `2699f9969` |
| 3 | 0072: measure both guard sites | 03 | measured; **B (`dotcards.c:225`) chosen** | `1c07b937b` |
| 4 | 0072: the fix, RED-first, with a deck | 04 | landed, 331 PASS / 0 FAIL | `ce48f9630`, `95772f307` |
| 5 | 0072: upstream material, prepared and unsent | 05 | landed, patch verified against `pre-master-47` | `4bdc935a4` |
| 6 | close the batch | 06 | closed, 331 PASS / 0 FAIL re-verified | *this commit* |

Six commits, `f14fc04aa..HEAD`. Two of them changed behaviour — `baccfa570`
added a test driver and a test directory, `ce48f9630` changed one function in
`src/frontend/dotcards.c` — and both went RED first. The other four are
documentation. Item 6 carries no hash here because a commit cannot contain its
own; its summary is *docs: close the exit-status batch*.

**Final `make check`** from `build-ver_50/`, at close, on the whole batch:
**331 PASS / 0 FAIL**, `make` exit status 0. `XFAIL`, `SKIP` and automake
`ERROR` results are all zero. (One line of the log reads `ERROR: (internal)
tried to destroy non-existent graph`; it is simulator output from inside
`tests/regression/casedist/vector-probe-report.cir`, which PASSes, and it
pre-dates this batch.) Both lint specs report `identity lint: 264 comparisons,
baseline matches` and `identity lint: 11 comparisons, baseline matches`;
`git diff f14fc04aa..HEAD -- tests/lint/identity.baseline` is empty across the
whole batch, not merely since the last commit.

## Notes

*What each crew falsified.* Every one of the five did, and in three cases the
falsified statement was in the document the item was working from.

- **Item 1** falsified its own item's RED instruction. The item asks for "an
  `absent` line for a file that exists"; that corruption **cannot fail**,
  because requirement 6 of the same item makes the driver delete every asserted
  path *before* the run. Measured rather than reasoned — a stale `redfiles.raw`
  in the build directory, asserted `absent`, passes — and the two load-bearing
  corruptions were run instead. It also measured that where the shell's
  `Aborted (core dumped)` line lands depends on which shell `configure` picked
  (dash writes it into the *redirected* capture, bash does not), so an
  unfiltered `.err` would have frozen a property of the build machine into a
  reference file.
- **Item 2** falsified the item's deck count and its expectation that something
  would prove unassertable. The item asks for three decks and asks which of the
  three `$sim_status` properties could not be asserted; the answer is that all
  three can, and that it takes four decks. `.err` is an exact comparison of the
  whole filtered stderr, so folding property 2's single stderr line into a deck
  that also runs a failing analysis would have frozen `doc/codex/issues/0057`'s
  wording — a message expected to change — into a directory whose subject is the
  exit status. Property 2 is asserted in two places because no control block can
  read its own stderr.
- **Item 3** is the item that existed to falsify things, and it falsified three.
  (a) **0072's Impact** — *"No deck in the tree has an analysis card and an empty
  netlist"* is false: `tests/regression/misc/empty-1.cir` is exactly that, via a
  bare `op` inside a `.control` block, which the issue's scan for `.op` **dot
  cards** (`^\s*\.op(?![a-z])`) could not see. That deck is upstream, is titled
  *"check that we can survive emptiness"*, and candidate A fails it — 327 PASS /
  1 FAIL, with plain `make check` aborting the recursion after 53 tests.
  (b) **0072's Resolution**, which preferred `beginPlot()` "on the merits"
  because stopping the plot from existing would fix consumers the issue had not
  enumerated: the unenumerated consumer *wants* the empty plot, and it is a test.
  (c) **The option that looked safest.** The relaxed variant A′ scores 328 PASS /
  0 FAIL and is not safe: it still stops `empty-1.cir`'s `op`, and the change is
  invisible only because `check.sh`'s `FILTER` contains `Data`, because
  `check.sh` never captures stderr, and because the deck's own `quit 0` sets the
  status. Three blind spots at once. That measurement is the reason this item was
  run rather than reasoned about.
- **Item 4** falsified the wording criterion 2 nominates. `no simulations run!`
  is producible at the chosen site byte for byte and is **false** there — the
  operating point did run, and `No. of Data Rows : 1` is on `stdout` immediately
  above the error — so the shipped clause is `no operating point printed!`. It
  also found a fourth part of 0072 arguing against its own Resolution: *Root
  Cause*'s closing sentence said the decision "belongs at `outitf.c:479` rather
  than in the epilogue", which is the claim item 3 had just falsified; corrected
  in place, though not on the item's list. And it verified the `empty-1.cir`
  fact directly instead of inheriting it from candidate A's failure — the same
  deck with `echo $curplot` and `display` added prints `curplot is op1` and
  `There are no vectors currently active.`, so the empty plot the assertion
  forbade is manufactured by an in-tree test today.
- **Item 5** falsified two statements in its own brief, and both are now
  propagated into 0072 by item 6. (a) The `beginPlot()` guard is
  **`outitf.c:461` upstream** where this branch numbers it `:479`, because that
  file carries unrelated branch changes earlier on; re-verified at close against
  `pre-master-47`, together with `run->refIndex = -1` at `:291` there against
  `:303` here. `dotcards.c` needs no translation —
  `assert(plot_cur->pl_dvecs != NULL)` is line 225 in both trees. (b) **`stdout`
  is not empty for every shape of the defect.** Empty is exact for the six-byte
  reproducer and for `.op` + a `.control run`; the `.op` + `.tran` shape keeps
  **324 bytes**, because the transient path flushes on its way past. It also
  measured that `.gitignore` line 78 is `**/*.patch`, so the prepared patch is
  tracked only because of `git add -f`.
- **Item 6** falsified nothing new. It re-verified item 5's two corrections
  against `pre-master-47` and the build tree before writing them into 0072, and
  audited the batch for the two things the plan forbids: no file under
  `doc/claude/upstream/` is added or modified by any commit
  (`git diff --stat f14fc04aa..HEAD -- doc/claude/upstream/` is empty), and no
  text added anywhere in the batch states or implies that a submission has been
  sent — all fifteen added lines matching *sent*, *submitted* or *submission*
  over `f14fc04aa..4bdc935a4` are denials, and this commit adds only denials.
  Three stale clauses in 0072 saying the upstream
  material "is not written yet" were true when item 4 wrote them and were made
  false by item 5; they are corrected to *prepared and not sent*.

## Waiting on the owner

1. **Whether to send the upstream material.**
   `doc/claude/batches/2026-08-15-exit-status-testing/upstream-0072/` — a patch
   against `pre-master-47`, a report and a README — is **prepared and unsent**.
   Nothing from this repository has been sent, and the `cp_remvar` material for
   `doc/codex/issues/0067` is likewise still unsent and was not touched by any
   item. If it goes out: the patch carries the `Co-Authored-By` trailer because
   the in-tree commit does, and deleting that one line from the `.patch` file is
   all that is needed to send it without. How to deliver it — mailing list, bug
   tracker, merge request — was deliberately not chosen. (receipt 05)
2. **The diagnostic's last clause.** 0072 criterion 2's literal sentence ends
   `no simulations run!`; the fix ships `no operating point printed!`, because
   the operating point ran and says so on the line above. Receipt 03 measured
   that the site can print the literal sentence byte for byte, so reverting is
   one string and three `.err` files. It is the only place the fix does not do
   exactly what the criterion's text says. (receipt 04)
3. **0072's *Root Cause* was corrected without being on item 4's list.** Its
   closing sentence said the guard belongs at `outitf.c:479`, which the
   measurement falsified; leaving it would have left the file arguing against
   its own Resolution. Say if you would rather it had been left alone.
   (receipt 04)
4. **0072 names its fix commit by summary, not by hash.** One commit was asked
   for and a commit cannot carry its own hash. The hash is `ce48f9630` and it is
   in receipt 04. A one-line follow-up if you want the issue to carry it.
   (receipt 04)
5. **0069's Status still reads "Open, filed 2026-08-14", and all five of its
   criteria are now met** — criterion 3 last, by item 2. Whether an issue whose
   criteria are all met stays Open is a call crew B did not make, because its
   instruction was to touch only the criterion-3 paragraph. The decision 0069
   records — that the exit status does not change — is unaffected either way.
   (receipt 02)
6. **0069 criterion 4 is documented, not asserted.**
   `sim-status-guard-ok.cir`'s stdout now carries two `Doing analysis` blocks
   for one deck, which is the two-simulation shape; nothing asserts that count,
   because `check_status.sh` deliberately does not compare stdout. Asserting it
   would be a `.out`-style comparison and a change to the driver's contract.
   (receipt 02)
7. **Two one-line tidy-ups no item authorised.** `#include <assert.h>` at
   `dotcards.c:12` is now dead; it was kept in-tree *and* kept out of the
   upstream patch on purpose, so that patch matches the change that landed, with
   `REPORT.md` §7 naming it for a maintainer. And `.gitignore` line 78 is
   `**/*.patch`, so the prepared patch is tracked only because of `git add -f`;
   a negation rule for that one directory would be tidier and is a change to a
   root build-metadata file. (receipts 04, 05)

## Standing constraints

1. `doc/claude/upstream/` is not touched by any item, and nothing written here
   states or implies that any submission has been sent. The `cp_remvar`
   submission is unsent; the owner is sending it.
2. Full `make check` from `build-ver_50/` after every code item.
3. `tests/lint/identity.baseline` unchanged, or the change justified.

All three held. 1 and 3 are verified above against `f14fc04aa`, the batch's
start commit, and not merely against the previous one.
