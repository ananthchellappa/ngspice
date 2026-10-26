# Receipt: close of the ver_51 follow-up batch

Close crew, 2026-10-26, branch `ver_51`. HEAD at start **`b920d3caf`**
(`docs: receipt for ver_51 follow-up 5, upstream prep for P1-P3`).

**No source changes.** This crew only built, installed, ran the tests and
wrote this receipt. Its only commit is this file. `PLAN.md`, every
`LEDGER.md` and `tests/lint/identity.baseline` were not edited.
`build-ver_51-shared/` was not touched. It is no longer on disk (see
"The shared-library question" below).

Scratch logs: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/close6/`
(`build.log`, `install.log`, `check.log`).

## Checks at close

| Check | Result |
|---|---|
| `ngheavy make -j4` (build-ver_50) | exit 0 |
| `ngheavy make install` | exit 0 |
| `ngheavy make check` (full) | exit 0. **353 PASS / 0 FAIL / 0 XFAIL / 0 XPASS / 0 SKIP / 0 ERROR** results |
| Identity lint, line 1 | `identity lint: 264 comparisons, baseline matches` |
| Identity lint, line 2 | `identity lint: 11 comparisons, baseline matches` |
| `tests/lint/identity.baseline` since `a664faa2d` | unchanged (`git diff --quiet a664faa2d HEAD` is clean) |
| Binary sha (`sha256sum build-ver_50/src/ngspice \| cut -c1-20`) | `c1341e751ffd39377859`, the same as follow-ups 4 and 5 |
| Owner's uncommitted edits | `CLAUDE.md` and `tests/regression/pipe/resource-keyword-case.cmd` are still ` M`, not staged. Their diff hash was `82d02a5f6ccb…` both before the build and after the check |

The one `^ERROR:` line in the log is the known
`ERROR: (internal)  tried to destroy non-existent graph` output from
`vector-probe-report.cir`, which passes. It is not an automake ERROR result.

353 = 347 at the start of the batch + 1 (P1, subckt noise deck) + 2 (P2) +
1 (P3, `rnddraws-gold.cir`) + 2 (P4). The upstream-prep item added no deck.
On a machine whose C library is not glibc, 2 of those decks report SKIP.
That is expected.

## Commits in the batch

`git log --oneline 6df77429c..HEAD` at close, before this receipt:

```
b920d3caf docs: receipt for ver_51 follow-up 5, upstream prep for P1-P3
f8ac6e67c Make the R->B temper decks fail when a vector is missing
f6353040f docs: receipt for ver_51 follow-up 4, the P4 fixes
efb8a7528 Test the startup, checkseed and option draw-log headers
0dc054bb5 docs: receipt for ver_51 follow-up 3, the P3 fixes
73733be3f Skip the glibc-only rng decks on another libc
3db79a2e3 docs: receipt for ver_51 follow-up 2, the P2 fixes
e1db1d3df Print the .option seed= warnings once per session, as written
5d88401af docs: record removal of the shared-library build tree
da9023b75 docs: receipt for ver_51 follow-up 1, the P1 fixes
82d8e7018 Warn about a temper TC only when the R model adds it
```

`5d88401af` is the driver's commit, not a crew's. Nothing was pushed.

## Per item

### Follow-up 1, P1 fixes: landed (`82d8e7018`, receipt `01-p1-fixes.md`)

Fixed:
- **1a.** Restored the missing space in `TESTS =` in the exitstatus `Makefile.am`.
- **1b.** The warning about a temper expression together with a TC now fires
  only when the *model* adds the TC, and it names which one (tc1, tc2 or
  both). A resistor's own tc1/tc2 never triggers it. The old `.err` line said
  "adds tc1/tc2" in a case where only tc1 was added. That was the wrong text
  this issue was about, so the line was corrected.
- **1c.** Corrected the reason a comparison was ruled out (a correction note,
  receipt 01 is unchanged).
- **1d.** A memory leak when a deck fails partway through conversion is fixed.
  The model lists are now freed on the fatal-exit and reset paths.
- **1e.** A new deck covers the noise branch inside a subcircuit. It fails on
  the old binary and passes now.

Declined:
- **B6, model-less cards lose dtemp/temp/tc=.** The plan requires cards
  without a model to convert byte for byte as before. Fixing this would change
  upstream behaviour for every such card.
- **B6, flicker noise.** It cannot be done in this design: the noise branch
  carries no DC current. The plan says to document it, not build it.
- **B6, tnom other than 27.** You declined this (decision 3).
- **B7, two identical `.model` cards carry nothing.** The fix needs a new
  comparison of two runtime strings, and the plan's identity-lint rule forbids
  that. The fix is one annotated line if you lift the rule.

### Follow-up 2, P2 fixes: landed (`e1db1d3df`, receipt `02-p2-fixes.md`)

Fixed:
- **2b.** Both `.option seed=` warnings now say exactly when re-seeding
  happens, and they print the value as you wrote it (`seed=05` stays `05`).
- **2c.** A test that could fail by chance about once in a million runs can
  no longer fail that way.
- **B8.** "Multiple option seed" now prints once per session, not once per
  parse, so a 1000-run Monte Carlo loop prints it once.

Declined:
- **2a.** The reset of the once-per-session flag only runs in the shared
  library. No shared-library build exists to run it in. The receipt gives the
  reason as "off limits"; the accurate reason is that the tree was deleted at
  your request.

### Follow-up 3, P3 fixes: landed (`73733be3f`, receipt `03-p3-fixes.md`)

Fixed:
- **3d/B9.** The two decks that depend on glibc's `rand()` now report SKIP on
  another C library instead of FAIL. On glibc they check exactly what they
  checked before.
- **3a.** Corrected a comment about where seeding happens.
- **3b.** Removed an unused define and two unused includes.

Declined:
- **3c.** The re-seed in `ngSpice_Init` exists only in the shared library, so
  the same reason as 2a applies.

### Follow-up 4, P4 fixes: landed (`efb8a7528`, receipt `04-p4-fixes.md`)

Fixed:
- **B10.** Checked by inspection that the Visual Studio project files list
  the two new source files. No edit was needed. It was not built with MSVC.
- **4b.** Corrected which document receipt 04 credits.
- **4c.** Two new decks test the `src=startup`, `src=checkseed` and
  `src=option` headers of the draw log.

Declined:
- **4a.** Same as the shared-library question below.
- **4d.** Speculative and cannot be tested here (no 32-bit libc). There is no
  difference on x86-64.
- **4e.** The real fix is a thread-safe random generator, which is beyond
  this batch.
- **4f.** A one-character bug (`strncmp` length 14 should be 15) in the
  shared-library probe. It cannot be compiled or run without a shared build,
  so it is unfixed and recorded.

### Follow-up 5, upstream prep: partial (`f8ac6e67c`, receipt `05-upstream-prep.md`)

Done:
- Three upstream-ready patches (P1, P2, P3), each one commit on
  `pre-master-47`, in `doc/claude/batches/2026-10-26-mc-v51-followup/upstream/`,
  with a `README.md` listing what was removed as fork-only. All three apply
  cleanly, alone and together. None was built or run on `pre-master-47`.
- Found and fixed a weak test: the three R->B temper decks passed even when a
  vector was missing, and passed on ngspice-45.2, which has the bug. They now
  fail there and pass here.

Not done:
- **The per-patch `REPORT.md` files.** The harness blocks subagents from
  writing report files. `README.md` and the commit messages cover the same
  ground in shorter form.
- **Building or running the patches on `pre-master-47`.** The task forbade
  building that tree.

## The shared-library question, item (1) on `build-ver_51-shared`

**What it is.** ngspice can be built in two forms from the same source:

- the **ngspice program**, which you run from a shell. `build-ver_50` builds
  this, and `make check` tests only this.
- the **libngspice shared library**, which other programs (KiCad, xschem's
  Monte Carlo loop) load and drive through `ngSpice_Init`, `ngSpice_Circ` and
  `ngSpice_Reset`. Building it needs a separate `configure --with-ngshared`
  tree.

`build-ver_51-shared/` was that second tree. Crew P4 made it to run one probe
program against the library. The plan said "no second configured tree", so
the earlier close receipt asked you to approve it or delete it. That is
what "(1)" was.

**Where it stands.** It is already decided. Commit `5d88401af` records that
you said delete, and the driver deleted it. The folder is not on disk.
Receipts 02 and 03 were written as if the question were still open and
recommend keeping it. Those recommendations are out of date; do not act on
them.

**Why it matters.** Some of this batch's code runs only inside the library,
never in the ngspice program:
- P3's re-seed in `ngSpice_Init` (3c),
- P2's reset of the once-per-session warning flag in `totalreset` (2a),
- P4's draw-log reset order,
- follow-up 1's fatal-exit free (1d), on its shared-library path,
- and the probe's one-character bug (4f).

This code compiles, and it was read. Its only run was the single probe run
in the original receipt 04, *before* follow-ups 1 to 4 changed it.

**The choices and what each one means.**

1. **Leave it deleted (the current state).** Costs nothing now, and the
   "one build tree" rule holds. The risk: if one of those library paths is
   wrong, nothing here can catch it. The symptom would be a wrong seed, a
   repeated or missing warning, or a crash inside a host program such as
   xschem or KiCad, and a library user would find it first. P2's and P3's
   upstream patches both change `sharedspice.c`, so the same untested code
   would also go upstream.
2. **Rebuild it once, verify, delete it again.** One crew makes the tree,
   makes the 4f fix, runs the existing probe plus the malformed-deck check for
   1d under valgrind, and deletes the tree when done. Cost: a few minutes of
   CPU, memory pressure on this shared box during the build (run with
   `ngheavy`), about 130 MB of disk for the duration, and a one-time
   exception to the plan's "no second configured tree" rule.
3. **Rebuild it and keep it.** Same as 2, and later batches can also test
   the library. Cost: disk space that stays in use, and the tree goes stale
   unless someone rebuilds it before each use, so a crew could test an old
   build without noticing.

**Recommendation: choice 2.** The value is in one verification run, not in
keeping the tree. Library users go through `ngSpice_Reset` on every reload,
so that is the path where a mistake does the most harm. It is also the path
that P2 and P3 change upstream. One run before anything is sent upstream
closes 2a, 3c, 1d and 4f together. After that, choice 1 is fine again.

## For the owner

You said you will decide on pushing. These are the other things, each with
my recommendation.

1. **The shared-library tree.** See the section above. Recommendation:
   rebuild it once, verify, delete it again (choice 2).
2. **B6 and B7, declined under plan rules.** Recommendation: leave B6's
   model-less cards and tnom as they are. Changing them would change behaviour
   for every upstream user of the conversion. B6's flicker noise cannot be
   done in this design. For B7, lift the identity-lint rule for that one
   comparison: it costs one annotated line, and today two identical `.model`
   cards silently lose their TCs, which is a wrong answer with no warning.
3. **New bug, not from this batch.** A `.model bad` card with no type, in a
   deck that uses `.if`, crashes ngspice (exit 139). It happens on the old and
   new binaries. Recommendation: open a `doc/codex/issues/` entry and fix it in
   its own batch. A crash on a typo is worth fixing.
4. **"Cannot convert option seed=... skipped!"** still prints on every parse,
   so a Monte Carlo loop repeats it. Recommendation: make it once per session
   like B8, for consistency. It is a small change.
5. **Minor test-strength issues the reviews found.** The P3 `.files` pattern
   `9*1*00$` accepts more values than its comment says. The P4 draw-log decks
   check only one of two record sets. The P1 subckt deck was never run against
   a wrong-scope mutation. None hides a known bug today, and on glibc the
   exact checks still run. Recommendation: one small follow-up crew to tighten
   them, since a weak test is how the R->B temper bug got through.
6. **A theoretical double free in 1d** if Ctrl-C lands within microseconds of
   a free. Recommendation: apply the reviewer's reorder (clear the pointer
   before freeing). It costs nothing and removes the window.
7. **4d and 4e.** Recommendation: leave them. 4d makes no difference on any
   machine you use. 4e needs a thread-safe generator, which is a project of
   its own.
8. **B10 (Visual Studio project files).** Checked by reading only. It is
   closed fully only if someone builds with MSVC on Windows. Recommendation:
   leave it unless you ship Windows builds.
9. **The upstream patches** are ready in
   `doc/claude/batches/2026-10-26-mc-v51-followup/upstream/{p1,p2,p3}/` and
   have not been sent. P3 is the one upstream is most likely to question:
   decks without `setseed` become reproducible from run to run. P1's tests
   need upstream commit `785be3194`. The missing `REPORT.md` files can be
   written from crew 5's output if you want them.
