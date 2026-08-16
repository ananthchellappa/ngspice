# Batch plan — exit-status testing, then the two things it unblocks

**Date** 2026-08-15. **Branch** `ver_50`. **HEAD at start** `f14fc04aa`.
**Baseline** 322 PASS / 0 FAIL, recorded for `731c01455` in
`doc/claude/batches/2026-08-15-xschem-open-items/LEDGER.md`; every commit since
is documentation only.

## The gap this batch closes

ngspice's regression suite cannot assert an exit status of a `--batch` run.
Measured, and stated in two issues already:

- `tests/bin/check.sh` runs `--batch` and **discards** the status. Line 29 is
  `$SPICE --batch $testdir/$testname.cir >$testname.test`; `$?` appears nowhere
  in the file, stderr is never redirected, and the only failure path is the
  `diff` at `:36`-`:39`. So no `check.sh` directory can see a status.
- `tests/regression/pipe/` **is** the one directory whose verdict is an exit
  status, but its `TESTS_ENVIRONMENT` is `ngspice -p < $$1`, which never enters
  batch mode and never reaches `ft_cktcoms()`. It also cannot pass `-r`.
- Roughly twenty `.cir` decks under `check.sh` directories end in `quit 1`.
  Those statuses are discarded; the `echo` text in the committed `.out` is what
  actually fails the test.

Two owed items are blocked on exactly that:

- **`doc/codex/issues/0072`** — a title line plus `.op` and nothing else aborts
  on `dotcards.c:225`, `rc=134`, on this build and on released `ngspice-46`
  alike. Filed 2026-08-15, not fixed. Its criterion 6 says in as many words
  that no existing harness can assert a fix.
- **`doc/codex/issues/0069` criterion 3** — a deck asserting the `$sim_status`
  guard shape. `grep -rl sim_status tests/` returns nothing; the guard is
  documented and measured in three places and asserted in none.

So: build the capability first, then spend it on both.

## Decisions taken by the owner before planning

1. **Harness shape — a new directory with a new driver.**
   `tests/regression/exitstatus/` driven by `tests/bin/check_status.sh`.
   Not an edit to `check.sh` (every directory in the suite depends on it) and
   not a `pipe/` deck shelling out to a second ngspice (`$program` is the
   compiled-in string `ngspice`, not the running binary's path, so such a deck
   tests whatever is on `PATH` — measured in 0072 criterion 6).
   `tests/lint/` is the in-tree precedent for a test directory that runs its
   own driver instead of `check.sh`.
2. **0072 guard site — measure both, then decide.** Item 3 measures
   `outitf.c:479` and `dotcards.c:225` as two scratch patches and reports the
   blast radius; the driver takes the decision on those numbers. 0072
   criterion 3 requires that whichever is chosen, **the other is named and the
   reason recorded**.
3. **0072 lands here and an upstream patch is prepared.** Option 2 of the
   issue's Resolution. The upstream material goes in this batch directory.
   **`doc/claude/upstream/` is not touched by any item, and nothing anywhere is
   to be described as sent** — the `cp_remvar` submission is unsent and the
   owner is sending it.
4. **One report at the end.** No mid-batch checkpoint.

## Standing rules for every crew

- RED first for anything that changes behaviour: write or extend the test,
  quote the failure verbatim in the receipt, then make the smallest production
  change. Never weaken an assertion or regenerate an `.out` to hide a failure.
- Targeted directory `make check`, then **full `make check` from
  `build-ver_50/`**, after every item that touches code or the build system.
  Both counts in the receipt as a two-row table.
- `tests/lint/identity.baseline` unchanged, or the change justified in the
  receipt. A fix of the expected shape here adds no string comparison at all.
- Re-run `./autogen.sh` after any `Makefile.am` or `configure.ac` change.
- Re-measure every "before" number rather than carrying it from a receipt.
  State the binary's build stamp.
- Receipt per crew at `receipts/NN-slug.md`, in the house shape: PLAN item,
  branch, HEAD at start, whether code was touched, *The RED failure, verbatim*,
  *The production change*, *The two `make check` counts*, *What was measured*,
  *What was left, deliberately*, *For the owner*.

## Items

### 1 — The harness: `check_status.sh` and `tests/regression/exitstatus/`

Build the capability, with nothing riding on it yet.

**Driver** `tests/bin/check_status.sh`, invoked the way `check.sh` is —
simulator path first (possibly a multi-word string), test file appended by
automake. It must:

- run `$SPICE --batch -n <deck>` with `stdout` and `stderr` both captured to
  files;
- capture `$?` and compare it against `<base>.status`, a required sibling file
  holding one integer;
- report a status `>= 128` as a distinct failure naming the signal, because
  "did not die on a signal" is the property 0072 criterion 1 actually needs;
- if `<base>.err` exists, filter the captured `stderr` and `diff -B -w` it
  against that file;
- if `<base>.files` exists, evaluate its lines after the run — `absent <path>`,
  `present <path>`, `grep <path> <text>` — so a deck can assert that a rawfile
  was or was not written and what its header says. This is what 0069
  criterion 3 and 0072 criterion 5 both need;
- delete the artefacts it is about to assert on **before** the run, so a stale
  file from an earlier run cannot pass a `present` line;
- exit 0 on all checks passing, 1 otherwise, printing which check failed.

**Self-test.** Follow `tests/lint/`'s two-spec pattern: the directory tests its
own driver. At minimum a deck whose `.status` deliberately mismatches must be
shown to fail, and the RED evidence for this item is that demonstration — a
driver that cannot fail is worth nothing.

**Registration.** `tests/regression/exitstatus/Makefile.am` (`TESTS`,
`TESTS_ENVIRONMENT`, `EXTRA_DIST` covering the `.cir` and every sibling
assertion file, `CLEANFILES` for the captured output and any rawfile a deck
writes, `MAINTAINERCLEANFILES`), the directory added to
`tests/regression/Makefile.am`'s `SUBDIRS`, an `AC_CONFIG_FILES` entry in
`configure.ac` beside the other `tests/regression/*/Makefile` lines, then
`./autogen.sh`. `tests/.gitignore` ignores `*.out` globally — the new
assertion files must not use that extension, and they do not.

**First real deck, so the directory is not empty:** one deck already known to
exit non-zero for a good reason, independent of both issues — the
`.tran`-with-empty-netlist case, which 0072's table measures at `rc=1` with
`Error: incomplete or empty netlist`. It pins the contrast that 0072's whole
argument rests on.

Acceptance: targeted `make check` green, full suite green with the new count
recorded, and the deliberate-mismatch demonstration quoted.

### 2 — 0069 criterion 3: a deck that asserts the `$sim_status` guard

Uses item 1's driver. No production change.

Three decks, in the client's exact shape (analysis dot card **plus** a
`.control run`), from 0069's Resolution:

- **guard fires** — `.save v(nosuchnode)`, which misses in every case mode, so
  the deck needs no `-D` flag and belongs in the default-mode directory:
  `run`, `if $sim_status ne 0` / `echo RUN-FAILED` / `quit 1`, then a `write`
  that must never be reached. Assert `rc=1` **and** the rawfile absent.
- **negative control** — the same deck with a resolvable `.save`. Assert
  `rc=0` and the rawfile present with `Plotname: Operating Point`.
- **the hazard 0069 property 2 records** — `$?sim_status` is 0 before the first
  analysis and reading `$sim_status` there puts
  `Error: sim_status: no such variable.` on stderr. Assert whichever of the
  three properties can be asserted by status and artefact; say in the receipt
  which could not and why.

RED for a characterisation deck is the guard's own removal: 0069 measures the
unguarded deck at `rc=0` in every mode. Delete the four guard lines, watch the
new test fail, put them back. Quote both.

Then update 0069's criterion-3 paragraph from *not met* to met, naming the
deck. Do not restate anything else in that issue.

### 3 — 0072: measure both guard sites, decide, record

Read-only with respect to the repository: scratch patches, no commit of
production code. Produces the decision input for item 4.

Measure, on `build-ver_50/src/ngspice` with the stamp recorded, and re-measure
the "before" row of every table rather than quoting 0072:

- **Candidate A, `outitf.c:479`** — drop or relax the `numNames &&` conjunct so
  the existing `Error: no data saved for %s; analysis not run` fires and
  `E_NOTFOUND` propagates. Then measure: every row of 0072's analysis table
  (`.op`, `.op`+`.tran`, `.op`+`.control run`, `.tran`, `.dc`, `.ac`, `.print`
  variants, `.four`, `.tf`, `.options` only); every row of its route table
  (`--batch`, `-b < deck`, no-tty, `-r`, tty, `-p`); the netlist-body table
  (`r1 0 0 1k`, `.model m1 nmos`, a real one-node deck); and **all eight
  in-tree `.op` decks named in 0072's Impact**, whose committed `.out` files
  may not move. Full `make check` with the scratch patch applied is the honest
  way to measure that last one.
- **Candidate B, `dotcards.c:225`** — replace the assertion with a test, a
  diagnostic and a return in the idiom the same function already uses at
  `:204`. The same tables.
- For both: what happens to the `-r` route (0072 criterion 5 — the file is a
  valid raw file or the run refuses to write one, not a third thing), and what
  the diagnostic text is. `Error: incomplete or empty netlist … no simulations
  run!` is the candidate 0072 criterion 2 names; if the chosen site cannot
  produce that wording, say what it produces and why a reader still knows to
  look at the netlist.

Deliverable: a receipt with both blast radii and a recommendation. The driver
decides from it; the rejected site and the reason are recorded, per criterion 3.

### 4 — 0072: the fix, RED-first, with a deck in the new directory

RED first, using item 1's driver: a deck of `printf '*\n.op\n'` with a
`.status` asserting the non-zero, non-signal status and an `.err` asserting the
diagnostic. Confirm it fails today for the right reason — `rc=134` where the
file says otherwise — before touching production code.

Then the guard at the site item 3 chose, its diagnostic, and nothing else: no
new header, no `Makefile.am`, no new helper, no string comparison. Add the deck
variants criterion 1 lists that the harness can express (`.op` with `.tran`,
`.op` with a `.control run` block; the `-b -n < deck` and no-tty routes are the
same path and need not each be a separate deck if the receipt says so).

Then: full `make check`, the eight in-tree `.op` decks' `.out` files verified
unmoved, `tests/lint/identity.baseline` unchanged, and 0072's Status and
Resolution rewritten — fixed, where, why not the other place, and the state of
each of its nine criteria.

### 5 — Upstream material for 0072, prepared and unsent

A patch and a report in `doc/claude/batches/2026-08-15-exit-status-testing/upstream-0072/`:
the minimal diff against upstream (production change only — not this repo's
test directory, which upstream does not have), the six-byte reproducer, the
measurement that it reproduces on released `ngspice-46`, and the before/after.

**`doc/claude/upstream/` is not to be opened, edited, or referenced as a
destination, and nothing in this batch may state or imply that any submission
has been sent.**

### 6 — Close the batch

`LEDGER.md` filled in: the item table, what each crew falsified, the final
`make check` counts, and what is left for the owner. Any cross-reference the
new capability earns — 0069's and 0072's *Related* sections, and a pointer from
`doc/claude/casemode-distinguish-guide.md` §9 to the deck that now asserts the
guard it documents, if and only if that section already invites one.

## Out of scope

- `doc/claude/upstream/` in any form.
- `doc/codex/issues/0059`, `0071`, `0073` — named in passing by both issues,
  none is owed here.
- Teaching `tests/bin/check.sh` anything. It is untouched by every item.
- The `-s` stdout path and CIDER state dumps, both recorded as unassertable by
  the previous batch.
