# Receipt: close of ver_51 follow-up batch 2

Close crew, 2026-10-26, branch `ver_51`. HEAD at start **`e9e1e83ce`**
(`docs: refresh the upstream patches for ver_51 follow-up 2`).

**No source changes.** This crew built, installed, ran the full test suite
and wrote this receipt. This file is its only commit. No `PLAN.md`, no
`LEDGER.md` and not `tests/lint/identity.baseline` was edited. No shared-library
tree was made. Nothing was pushed or sent anywhere. Every heavy command went
through `ngheavy`, one at a time, with `-j4`.

Scratch logs: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/`
(`make.log`, `install.log`, `check.log`, `owner_before.txt`).

## Checks at close

| Check | Result |
|---|---|
| `ngheavy make -j4` (build-ver_50) | exit 0. Nothing to rebuild: the tree was already current with HEAD |
| `ngheavy make install` | exit 0. `stage/bin/ngspice` is byte-identical to `src/ngspice` |
| `ngheavy make check` (full) | exit 0. **362 PASS / 0 FAIL / 0 XFAIL / 0 XPASS / 0 SKIP** results |
| Identity lint, line 1 | `identity lint: 264 comparisons, baseline matches` (`PASS: identity.lint`) |
| Identity lint, line 2 | `identity lint: 11 comparisons, baseline matches` (`PASS: selftest.lint`) |
| `tests/lint/identity.baseline` since `a664faa2d` | unchanged. `git diff --quiet a664faa2d HEAD -- tests/lint/identity.baseline` exits 0, and no commit since `a664faa2d` touches it |
| Binary sha (`sha256sum build-ver_50/src/ngspice \| cut -c1-20`) | `68e577b28b297b0054d9`, the same as items 3 and 4 |
| Owner's uncommitted edits | `CLAUDE.md` and `tests/regression/pipe/resource-keyword-case.cmd` are still ` M` and not staged. Their combined diff hash was `82d02a5f6ccb…` before the build and after the check, and so were both file hashes. It is also the same diff hash as at the close of follow-up batch 1 |

The one log line starting with `ERROR:` is the known
`ERROR: (internal)  tried to destroy non-existent graph`, printed by
`vector-probe-report.cir`, which passes. It is not an automake ERROR result.

362 = 353 at the close of follow-up batch 1 + 7 (item 1, the typeless-model
decks in `tests/regression/exitstatus/`) + 1 (item 2,
`temper-res-behav-dup.cir`) + 1 (item 3, `option-seed-unusable.cir`). Item 4
added no deck to this tree.

## Commits in the batch

`git log --oneline f0c8c25cd..HEAD` at close, before this receipt:

```
e9e1e83ce docs: refresh the upstream patches for ver_51 follow-up 2
eecdc5a97 docs: receipt for ver_51 follow-up 2, item 3, the seed warning
f8f0f60d5 Print the .option seed 'Cannot convert' warning once per session
c2b63479e docs: receipt for ver_51 follow-up 2, item 2, P1 hardening
3a904f87d Carry an R model whose duplicate cards in one scope agree
8a671e1c0 docs: receipt for ver_51 follow-up 2, item 1, the typeless model crash
b70e5123e Report a .model card with no type in a deck with .if
```

Nothing was pushed.

## Per item

### Item 1, the typeless-model crash: landed (`b70e5123e`, receipt `01-typeless-model-segfault.md`)

A `.model` line with no type (or with no name either) crashed ngspice in any
deck that uses `.if`, even when the line sat in a branch that was switched
off. There was a second crash behind the first when such a line was inside a
subcircuit. Both are fixed. The run now stops with
`Error: no model type given ...` and exit 1, with or without `.if`. Written
up as `doc/codex/issues/0074-a-typeless-model-in-a-deck-with-if-segfaults.md`.
Seven new decks; six failed with a crash on the old binary.

Declined, and left for you: the sticky `has_if` flag, and the order of the
R->B model collection relative to `.if` (both below).

### Item 2, P1 hardening: landed (`3a904f87d`, receipt `02-p1-hardening.md`)

- A model written byte for byte the same way in both branches of an `.if` now
  keeps its temperature coefficients when an R is rewritten into a B source.
  Copies that differ at all still warn and carry nothing.
- The R->B model lists are now cleared before they are freed, which closes the
  small double-free window from batch 1, item 6.
- The subcircuit noise test was weaker than batch 1 said. It now fails, on the
  numbers, when the model lookup uses the wrong scope.

### Item 3, the seed warning: landed (`f8f0f60d5`, receipt `03-cannot-convert-once.md`)

`Cannot convert option seed=... skipped!` now prints once per session, like
the other two `seed=` warnings. A Monte Carlo loop prints it once, not on
every run. The one-character probe bug 4f (`strncmp` length 14 to 15) is
fixed, but the probe passed only a syntax check, because there is no
shared-library build to run it against.

### Item 4, upstream refresh: landed (`e9e1e83ce`, receipt `04-upstream-refresh.md`)

The upstream patches in `doc/claude/batches/2026-10-26-mc-v51-followup/upstream/`
were regenerated: P1 gains item 2's duplicate rule and test, and P2 gains
item 3's once-per-session warning. A new crash fix is in `p4-crash/`, with a
variant `p4-crash/on-p1/` for when P1 goes first. The patches were applied in
a scratch worktree in several orders, but nothing was built or run on
`pre-master-47`.

## For the owner

1. **The batch is green.** 362 tests pass, both lint lines match the baseline,
   and your two uncommitted edits were not touched.
2. **The crash is fixed here and upstream has it too.** A typo such as a bare
   `.model bad` crashes upstream ngspice in any deck that uses `.if`. The
   45.2 release crashes on all five such decks. The fix is ready in
   `upstream/p4-crash/`. If you send P1 as well, send P1 first and then
   `p4-crash/on-p1/`. Recommendation: send the crash fix, since it is small
   and stands on its own.
3. **`has_if` never resets** (`src/frontend/inpcom.c`, the static at line
   124). Once one deck with `.if` has been read, every later deck in the same
   session, or in a program using the library, skips removing unused models.
   It no longer causes a crash or a missing error. Recommendation: a small
   follow-up to clear it on reset, since it is the same kind of bug as the
   other `ngSpice_Reset` fixes.
4. **R->B model collection runs before `.if` is decided.** So a typeless card
   in a switched-off branch is still reported when a resistor in the deck is
   rewritten, and models from switched-off branches are collected. This comes
   from P1. Probe deck in item 1's receipt. Recommendation: low priority; the
   result errs towards an error message, never a wrong value.
5. **The shared-library paths are still unrun.** The reset of the once-per-session
   warning flags (item 3 and batch 1), the free-order change in item 2, and the
   probe fix 4f only make a difference in the library, and no library build
   exists. The earlier recommendation stands: rebuild the library tree once,
   run the probe, and delete it again, before P2 or P3 goes upstream.
6. **Minor review notes, no action needed.** In item 1, `skip_token()`
   replaced `nexttok()`, so a card like `.model a,nmos(...)` is now split the
   same way as everywhere else; and a comment there says ".if" where the
   check runs on every deck. In item 2, the `case-lint: stored` reason is
   used for model-card text rather than names, as the brief allowed; and one
   `Makefile.am` comment line runs long.
