# Receipt: item 5 — upstream material for 0072, prepared and unsent

PLAN item **5 — Upstream material for 0072, prepared and unsent**, from
`doc/claude/batches/2026-08-15-exit-status-testing/PLAN.md`.

Crew E, 2026-08-15, branch `ver_50`. HEAD at start **`95772f307`**.

**No code was touched.** `git status --porcelain -- src/ tests/` shows nothing
modified, staged or deleted — only the untracked case-mode scratch files that
were already there at the start of the batch. `git status --porcelain` with
untracked lines filtered out is empty. No `Makefile.am`, no `autogen.sh`, no
build. The only addition is one new directory of three documentation files.

**Nothing under `doc/claude/upstream/` was opened, listed, read or written**,
and nothing in this receipt or in the new material states or implies that any
upstream submission has been sent. The material says in as many words that it
has not been sent and that sending it is the owner's call.

**No branch was checked out.** `pre-master-47` was read with `git show
pre-master-47:<path>` and `git ls-tree pre-master-47`, never with `checkout`,
`switch` or `worktree`, so `build-ver_50/` was left alone. No build was run and
no `make check` was run.

**Binaries.** Both were run in place, never copied, and neither was rebuilt.

| role | path | version | Creation Date | sha256 |
| --- | --- | --- | --- | --- |
| before | `/usr/local/bin/ngspice` | `ngspice-46`, as released | `Sun Aug  2 23:29:26 UTC 2026` | `b127abce0b8d1e67ce99229a769d6a7cd4e6646c2c1002df51a1590766ab105b` |
| after | `build-ver_50/src/ngspice` | `ngspice-46+` | `Sun Aug 16 00:53:25 UTC 2026` | `1fa06748c09be050bebac6ca10a1d0856f49b75b4f29c7a14a096f96e6557686` |

The `after` hash is byte-identical to the AFTER binary receipt 04 finished on,
which is the check that this crew measured the fix that actually landed and did
not disturb the live build tree.

Scratch:
`/tmp/claude-1000/-home-qflow-dev-ngspice-test/d775575c-7554-4eec-9dca-17c55bd7d1cf/scratchpad`
(`upstream-dotcards.c`, `prefix-dotcards.c`, `up-*.c`, `msg.txt`, `up/`, `v1/`,
`v2/`, `w1/`, `w2/`, `named/`, `m/`). Nothing was left in the repository or the
build tree.

## What the material contains

`doc/claude/batches/2026-08-15-exit-status-testing/upstream-0072/`, three files,
nowhere else.

| file | bytes | what it is |
| --- | --- | --- |
| `0001-frontend-report-an-op-card-with-no-netlist.patch` | 4495 | `git format-patch` output, one file, one hunk: `src/frontend/dotcards.c`, 12 added lines, 1 removed |
| `REPORT.md` | ~16 k | the eight-section report a maintainer reads before the patch |
| `README.md` | ~1.4 k | what the two files are, that the material is prepared and not sent, and that sending it is the owner's call |

**The patch is the production change only.** It carries no
`tests/regression/exitstatus/` deck, no `tests/bin/check_status.sh`, no
`Makefile.am` and no issue file — measured, not assumed: `git ls-tree
pre-master-47 tests/regression/` has no `exitstatus` entry and `git ls-tree
pre-master-47 tests/bin/` has no `check_status.sh`, so both would be additions
upstream cannot use. Its diff touches exactly one path.

It was **generated against upstream**, not extracted from `ce48f9630`: a
scratch tree was populated with `git show
pre-master-47:src/frontend/dotcards.c`, the hunk applied there, and
`git format-patch -1` run in that tree. So the patch's `index` line names the
upstream blob and a `git am -3` fallback has something true to work with.

The commit message inside the patch is written for a maintainer with no access
to this repository: no issue numbers, no batch paths, no receipt references. It
carries the same `Co-Authored-By: Claude Opus 5 (1M context)` trailer the
in-tree commit does; if the owner would rather send it without, that is one
line to delete.

`REPORT.md`'s eight sections are, in order: the six-byte reproducer with the
exact command and output; the measurement on released `ngspice-46` with its
build stamp; the failure (`dotcards.c:225`, `rc=134` = `128+SIGABRT`, 106 bytes
of `stderr`, empty `stdout` under a redirect); why `.op` is the only analysis
affected; the before/after table for three deck shapes; the rejected
`beginPlot()` fix with `empty-1.cir` as the reason; what the patch does not do;
and how it was tested here, including that the three decks asserting it cannot
travel with the patch.

Its opening paragraph states plainly that this is a mistake-shaped input that
no correct deck produces, that nothing is silently corrupted, and that the
analysis has already finished when the abort fires. There are no adjectives
about severity anywhere in the file and no argument that upstream should take
the patch.

## What was measured

Everything numeric in `REPORT.md` was measured by this crew on the two binaries
above. Nothing was carried from receipts 03 or 04 except the four `make check`
counts and the `empty-1.cir` failure transcript, which the item directs to be
taken from receipt 03 and which cannot be re-measured without rebuilding the
live tree.

### The patch applies — how that was verified

Three independent checks, all against `pre-master-47`'s file content and none
of them a checkout:

1. **Blob identity.** `git rev-parse pre-master-47:src/frontend/dotcards.c` is
   `60a3e1ac65131fb28ec48a109ed2334f1e793deb`, and the patch's header reads
   `index 60a3e1a..2acadf1 100644`. The pre-image the patch names *is* the
   upstream file.
2. **`git am`.** A scratch git repository holding only
   `src/frontend/dotcards.c` at `pre-master-47`'s content:
   `git am 0001-frontend-report-an-op-card-with-no-netlist.patch` →
   `Applying: frontend: report an .op card with no netlist instead of
   aborting`, exit 0.
3. **`patch -p1`.** A plain non-git copy of the same file:
   `patch -p1 --dry-run` then `patch -p1` → `patching file
   src/frontend/dotcards.c`, exit 0. **No offset, no fuzz, no `.rej`, no
   `.orig`.**

Both results are byte-identical to each other, and the patched region (lines
205-255) is byte-identical to `ver_50` HEAD's `src/frontend/dotcards.c` — so
the patch reproduces the landed fix exactly. Checks 2 and 3 were re-run against
the committed copy of the patch file, not only against the scratch original.

### Where upstream differs from this branch, measured

The item asked for this explicitly rather than as an assumption.

| file | upstream vs `ver_50` pre-fix | effect on the patch |
| --- | --- | --- |
| `src/frontend/dotcards.c` | **differs** — 6 hunks, 9 changed lines, all case-mode work (`strcmp`→`cieq`, `eq`→`eqc`) at lines 123, 284, 299, 327, 356, 407 | **none.** Lines 205-245 are byte-identical, `assert(...)` is at `:225` in both, and the hunk's six context lines match exactly. The nearest branch change is at `:284`, 59 lines past the patch's last context line at `:228` |
| `src/spicelib/analysis/dcop.c` | identical | the `refName == NULL` argument holds verbatim upstream |
| `src/spicelib/analysis/cktnames.c` | identical | `*numNames = ckt->CKTmaxEqNum-1` is upstream's line 23 |
| `src/frontend/outitf.c` | differs earlier in the file | **line numbers only.** The `numNames &&` guard is `:461` upstream and `:479` here; `run->refIndex = -1` is `:291` upstream. The report uses the upstream numbers and says so |
| `tests/regression/misc/empty-1.cir` | **identical** | the rejection argument in section 6 reproduces in a maintainer's own tree, and the report says so |

### The reproducer, on released `ngspice-46`

```
$ printf '*\n.op\n' > tiny.cir
$ /usr/local/bin/ngspice --batch -n tiny.cir ; echo rc=$?
ngspice: ../../../src/frontend/dotcards.c:225: ft_cktcoms: Assertion `plot_cur->pl_dvecs != NULL' failed.
Aborted (core dumped)
rc=134
$ /usr/local/bin/ngspice --batch -n tiny.cir > so.txt 2> se.txt ; echo rc=$?
rc=134
$ wc -c so.txt se.txt
  0 so.txt
106 se.txt
```

`kill -l 6` is `ABRT`, so `134 = 128 + SIGABRT`. Under `script -qec` the same
run's `stdout` does appear, ending at `No. of Data Rows : 1` — the analysis
finishes and the epilogue is what dies.

### Before and after, three deck shapes

`ngspice --batch -n <deck> > out 2> err`. Before is stock `ngspice-46`; after
is the patched `build-ver_50` binary.

| deck | before rc | before stdout | before stderr | after rc | after stdout | after stderr |
| --- | --- | --- | --- | --- | --- | --- |
| `*` / `.op` (6 B) | 134 | **0** | 106 | 1 | 173 | 115 |
| `.op` + `.tran 1n 10n` (24 B) | 134 | **324** | 106 | 1 | 347 | 115 |
| `.op` + `.control run` (30 B) | 134 | **0** | 106 | 1 | 293 | 115 |

`stderr` is byte-identical across the three shapes in each column
(md5 `a67a4c83…` before, `64812293…` after), so this is one defect reached
three ways.

**One number contradicts the item's brief and the report says so.** The brief
states `stdout` is empty under a redirect; that is exact for the reproducer and
for the `.control` shape, and **false for the `.op` + `.tran` shape**, which
keeps 324 bytes. The transient path flushes on its way past, so the banner,
`No. of Data Rows : 1` and an empty `Initial Transient Solution` header survive
while the transient's own results do not. Reported as measured rather than
generalised.

### `empty-1.cir`, re-measured

The deck the rejection argument rests on, run by this crew:

| binary | rc | stdout | stderr |
| --- | --- | --- | --- |
| stock `ngspice-46` | 0 | 626 B | empty |
| patched `build-ver_50` | 0 | 627 B | empty |

The one-byte difference is the deck's own closing `echo`: `ngspice-46 done`
against `ngspice-46+ done`. `diff` reports that line and nothing else, and both
runs print three `No. of Data Rows` lines. So the fix leaves it alone and the
deck passes on the released binary — both halves of the argument, measured here
rather than quoted.

### The `-r` route, re-measured

`ngspice -b -n -r out.raw tiny.cir`: `rc=0` on both binaries, 192 bytes on
stock and 193 on the patched build, identical once the `Date:` and `Command:`
lines are removed (the extra byte is the `+` in `ngspice-46+`). Loaded back
through `printf 'load out.raw\ndisplay\nquit 0\n' | ngspice -p -n`: `Loading
raw data file … done.`, `Name: Operating Point`, `There are no vectors
currently active.`, exit 0. A valid raw file, not a third thing.

Receipt 04 recorded 193 bytes before and after; that was two builds of this
branch. Against stock it is 192, and the report states both.

### Carried, not re-measured

Named here so no number in `REPORT.md` is mistaken for this crew's:

- 327 PASS / 1 FAIL under the `outitf.c` patch, 328 PASS / 0 FAIL baseline, and
  the recursion aborting after 53 tests — receipt 03.
- The verbatim `FAIL: empty-1.cir` transcript with its `.out` diff — receipt 03.
- The narrow variant's 328 PASS / 0 FAIL and the three reasons `check.sh`
  cannot see what it did — receipt 03.
- The three unrelated rows (`.print tran`, `.print ac`, `.four`) and the two
  `-r` rows the wider patch moves — receipt 03.
- 328 → 331 PASS / 0 FAIL for the landed fix, and the eight `.op` witnesses
  being unmoved — receipt 04.

Re-measuring any of these means a full `make check`, which means rebuilding
`build-ver_50/`, which the item forbids.

## What was left, deliberately

- **`#include <assert.h>` at `dotcards.c:12` is not removed by the patch.**
  Receipt 04 flagged it as now-dead and left the decision to this crew. It is
  left in: removing it would make the patch differ from the change that landed
  here and would mix a tidy-up into a one-line fix. `REPORT.md` §7 names it as
  a one-line follow-up so a maintainer who wants it does not have to notice it
  themselves. `grep -n assert` confirms it is the only remaining occurrence in
  the file.
- **The patch text was not re-worded for upstream style.** The diagnostic, the
  comment and the `return 1` are byte-identical to `ce48f9630`. Anything else
  would be a second version of a fix that is already tested here.
- **No cover letter, no mailing-list header, no bug-tracker text.** The item
  asked for a patch, a report and a README. How this is delivered — mailing
  list, bug tracker, merge request — is a decision this crew has no basis for,
  and picking one would have implied it.
- **No `make check` was run and nothing was rebuilt.** The item says none is
  needed, and `build-ver_50/` belongs to another crew. Both counts in the
  report are attributed to receipts 03 and 04 above.
- **`doc/codex/issues/0072`, `src/frontend/dotcards.c`, and every file under
  `tests/` were not edited.** The issue already says the upstream material is
  item 5 of this batch and is not in `ce48f9630`. Now that the material exists,
  a pointer from the issue to this directory would be accurate — but it is
  item 6's to make, and adding it here would have been a second change to a
  file item 4 owns.
- **`doc/claude/upstream/` was not opened, listed or referenced.** Out of scope
  for the whole batch.
- **`.gitignore` was not edited to make room for the patch.** Its line 78 is
  `**/*.patch`, under a `# Patches` heading, so the file needed `git add -f`
  and is now tracked. A negation rule for this one directory would have been
  the tidier answer and is a change to a root build-metadata file that this
  item does not authorise. Recorded because a crew that regenerates the file
  will hit the same rule and may think the `git add` silently worked.

## For the owner

1. **The patch applies cleanly to `pre-master-47`, and the blob id proves it.**
   The pre-image the patch names is exactly the branch's blob. `dotcards.c`
   *does* differ between upstream and `ver_50` — six case-mode hunks, nine
   changed lines — but the nearest is 59 lines past this hunk's last context
   line, and `assert(plot_cur->pl_dvecs != NULL)` is line 225 in both trees.
   `git am` and `patch -p1` were both run and both succeeded with no fuzz.
2. **One line-number correction to the item's brief, carried into the report.**
   The `beginPlot()` guard is `outitf.c:479` on this branch but **`:461`
   upstream** — that file carries unrelated branch changes earlier on. The
   report uses the upstream numbers throughout and says why, since a maintainer
   reading `:479` in their own tree would land in the wrong place.
3. **One measurement correction, also carried.** `stdout` is not empty for the
   `.op` + `.tran` deck; 324 bytes survive because the transient path flushes.
   Empty is exact for the six-byte reproducer and the `.control` shape. Stated
   as measured, because a maintainer who checks the claim and finds it
   overstated stops trusting the rest of the file.
4. **The patch carries the `Co-Authored-By` trailer.** It is in the in-tree
   commit, so it is in the patch. If you would rather send it without, delete
   that line from the `.patch` file before sending; nothing else depends on it.
5. **Nothing has been sent, and the material says so twice** — once in
   `README.md`'s own paragraph and once by omission: the patch's commit message
   makes no reference to a report, a ticket or a thread. The `cp_remvar`
   material for `0067` is likewise unsent and was not touched.
6. **Item 6 may want a `Related` line in 0072.** The issue's *Where it lands*
   currently reads "it is not in this commit and it is not written yet". That
   sentence is now stale by half. Not corrected here, because item 4 owns that
   file and the item's constraints forbid editing it.

Nothing was pushed. One commit on `ver_50`, this receipt and the three files in
`upstream-0072/`.
