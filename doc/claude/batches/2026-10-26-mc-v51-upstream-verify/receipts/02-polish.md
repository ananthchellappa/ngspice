# 02: Polishing the three upstream patches for submission

Date: 2026-10-26. Base: `pre-master-48`, tip
`a1148c9699f58c32680344ab876ac459d8bb126e` (unchanged since `01-verify.md`),
cloned fresh from `https://git.code.sf.net/p/ngspice/ngspice` into the
scratchpad. Series: `p1`, `p4-crash/on-p1`, `p2`. `p3` was not touched, and
neither was the standalone `p4-crash/`.

## 1. What changed

The series was applied with `git am`, then each commit was amended. Author,
author date, subject and the `Co-Authored-By` trailer are unchanged.

**`p1` (inpcom.c: carry the R model through the R->B rewrite)**
- `tests/regression/temper/temper-res-behav-dup.cir`: the header no longer
  cites the fork-only `tests/regression/exitstatus/behav-r-model-warn.cir`.
  It now reads "Two cards that disagree leave the model unresolved: ngspice
  warns on stderr, and nothing is carried. This deck covers the other case:
  ...". It is comment-only.
- Message: after the 45.2 table, "pre-master-48 (a1148c969) gives the same
  two values". `785be3194` now says "(in ngspice-47 and later)". A new
  closing paragraph gives the four pre-patch maxerr values measured on
  `pre-master-48`, and says the wrong-scope figure was measured there too
  and that make check passes.

**`p4-crash/on-p1` (do not crash on a typeless .model in a deck with .if)**
- Message: "pre-master-48 (a1148c969) crashes the same way." after the 45.2
  example, and a recheck paragraph after the test list. There are no code
  changes.

**`p2` (randnumb.c: warn once when .option seed= overrides setseed)**
- Code comment above `seed_from_option()`: "the cp_vset() +
  set_sseed(NULL) pair eval_opt() does". It used to say "has always done",
  but upstream only switched to `set_sseed` in `cbb09dc25`. This is
  comment-only.
- Message: the `seed=5` example is now a two-column table, 45.2 next to
  `pre-master-48`. The `seed=abc` paragraph says the "Cannot convert" warning
  prints 4 times on both, and gives `@r1[r]` = 8.432359e+02 on
  `pre-master-48` (8.392277e+02 on 45.2). A recheck paragraph says why
  there is no make check test (stderr only).

**Searched for other fork references or stale numbers.** I grepped all
three messages and diffs for `exitstatus`, `check_status`, fork, `ver_5`,
batch, decision, `case-lint`, `drawlog`, `rnddraw`, `codex`, `ng_ideq`,
`elem_letter`, `inp_case`, `0074`, "has always", `pre-master-47` and
`c5cd68015`. Nothing is left. I re-measured every number the messages cite,
see section 2. `inp_dot_if()`, named in the dup deck, exists upstream. The
`setseed random` comment ("seed with the process id") matches upstream's
`set_sseed()`. `785be3194` is an ancestor of the tip (`ngspice-47~90`), so
it is a requirement that the tree already meets, not a dependency to send.

## 2. Measured numbers

The binaries are the unpatched tip and the tip with the series, both built
in the scratchpad. `/usr/bin/ngspice` is 45.2. Each deck ran with an empty
`HOME`.

| deck | 45.2 | pre-master-48 | pre-master-48 + series |
| --- | --- | --- | --- |
| p1 deck, 125 C: v(a) / v(b) | 7.300000e-02 / 5.694000e-02 | 7.300000e-02 / 5.694000e-02 | 5.694000e-02 / 5.694000e-02 |
| p1 deck, same `.model` in `.if`/`.else` | — | 7.300000e-02 / 5.694000e-02 | 5.694000e-02 / 5.694000e-02 |
| p4 `.if (0)` + `.model bad` | exit 139 | exit 139 (SIGSEGV) | runs, v(1) = 1.000000e-03, exit 0 |
| p4 `.if (1)` + `.model bad` | — | exit 139 | "Error: no model type given in line .model bad!", exit 1 |
| p4 no `.if` + `.model bad` | — | same error, exit 1 | same error, exit 1 |
| p2 `seed=5`, setseed 1/2/3 | 9.312134e+02 x3, silent | 7.974070e+02 x3, silent | 7.974070e+02 x3, override warning once |
| p2 `seed=abc`, setseed 3 x3 | 8.392277e+02 x3, "Cannot convert" x4 | 8.432359e+02 x3, x4 | 8.432359e+02 x3, x1 + "(printed once per session)" |

New tests through the tip's `tests/bin/check.sh`:

| test | unpatched tip | patched |
| --- | --- | --- |
| `temper-res-behav-1.cir` | FAIL, maxerr 0.314112 | PASS |
| `temper-res-behav-noise.cir` | FAIL, maxerr 0.144911 | PASS |
| `temper-res-behav-noise-subckt.cir` | FAIL, maxerr 0.156474 | PASS |
| `temper-res-behav-dup.cir` | FAIL, maxerr 0.282051, spread 0 | PASS |
| `model-typeless-if-dead.cir` | FAIL, Segmentation fault | PASS, 3 of 3 runs |

**Wrong-scope mutant** (`p1`'s "maxerr 0.445"). In the patched tree, the
lookup loop in `behav_r_model_args()` was made to start at
`card->level->next`, the parent scope. `temper-res-behav-noise-subckt.cir`
then fails with maxerr 0.444942. I restored the file and rebuilt, and the
test passes again.

**`p2` probe decks** (`../probes/p2decks/`, both builds). The override
warning is printed 0 times unpatched and once patched for `override`,
`random`, `setseedrandom` and `multiple`. "Multiple" goes from 4 to 1, and
"Cannot convert" from 4 to 1. `nooption` and `nosetseed` are silent in both
builds. Stdout is identical in both builds, except for `random`, which seeds
from the clock.

## 3. Build and make check

I reverted the tip's two `tests/xspice/analog` build lines (`configure.ac`,
`tests/xspice/Makefile.am`) in each worktree with
`git show a1148c969 -- ... | git apply -R`. That change was never committed.
Everything ran through `ngheavy`, with `-j4`, one job at a time, using plain
`../configure`.

- Patched: configure, make and make check all exit 0. **62 PASS, 0 FAIL**,
  0 ERROR, 0 SKIP. The build printed 328 compiler warnings, the same as in
  `01-verify.md`.

## 4. Apply check

I made a second fresh clone of `pre-master-48`
(`a1148c9699f58c32680344ab876ac459d8bb126e`, clean). Running
`git am p1/*.patch p4-crash/on-p1/*.patch p2/*.patch` with the regenerated
files applied all three, and the resulting tree is identical
(`git diff --stat` empty) to the one that was built and tested.

## 5. What changed in this repo

- `doc/claude/batches/2026-10-26-mc-v51-followup/upstream/p1/0001-...patch`,
  `.../p4-crash/on-p1/0001-...patch` and `.../p2/0001-...patch`, regenerated
  with `git format-patch`.
- `.../upstream/README.md`, with a new "Polished for submission" paragraph.
  Nothing else was changed.
- This receipt.

Nothing was posted, filed, mailed or pushed. The clones, worktrees, builds
and scratch decks were deleted from the scratchpad.
