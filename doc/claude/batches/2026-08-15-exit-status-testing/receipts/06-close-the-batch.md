# Receipt: item 6 — close the batch

PLAN item **6 — Close the batch**, from
`doc/claude/batches/2026-08-15-exit-status-testing/PLAN.md`.

Crew F, 2026-08-15, branch `ver_50`. HEAD at start **`4bdc935a4`**.

**No code was touched.** Nothing under `src/`, `tests/`, `tests/bin/` or `m4/`
was opened for editing; no `Makefile.am`, no `configure.ac`, no `autogen.sh`, no
build. `git status --short --untracked-files=no` before this commit listed
exactly two files, both under `doc/codex/issues/`. The fix, the decks and the
driver are untouched.

**`doc/claude/upstream/` was not opened, read, listed or written.** The only
command naming it is the one audit the item specifies,
`git diff --stat f14fc04aa..HEAD -- doc/claude/upstream/`, which reads no file
contents. **Nothing in this receipt, in `LEDGER.md`, or in the two issues states
or implies that any upstream submission has been sent — because none has.**

**Binary.** `build-ver_50/src/ngspice`, `ngspice-46+`, sha256
`1fa06748c09be050beba…`. Nothing was rebuilt: that hash is byte-identical to the
AFTER binary of receipt 04 and the `after` binary of receipt 05, which is the
check that the tree `make check` ran on at close is the tree item 5 left.

Scratch: `/tmp/claude-1000/-home-qflow-dev-ngspice-test/d775575c-7554-4eec-9dca-17c55bd7d1cf/scratchpad`
(`makecheck.log`). Nothing was left in the repository or the build tree.

## What changed

Five documentation edits and no sixth.

### 1. `LEDGER.md` — the two empty sections filled in

*Notes* is now what each crew **falsified**, which is the section's purpose in
the house format, one entry per crew with the measurement behind it. *Waiting on
the owner* is a numbered list of seven, every one of them taken from a receipt —
five from a *For the owner* section, two (`assert.h`, `.gitignore`) from a *What
was left, deliberately*. Nothing was invented. The item table's row 6 is closed,
the commit range is stated, and the final count and the standing-constraint
verifications are recorded with the commands that produced them.

The ledger's item-6 row carries no hash: a commit cannot contain its own, so it
names the summary instead, which is the convention `doc/codex/issues/0072`
already uses for `ce48f9630`.

### 2. `doc/codex/issues/0072` — item 5's two corrections propagated

Both were **re-verified before being written down**, not copied from receipt 05.

**(a) `stdout` is not empty for every shape.** The issue *did* overstate it. Its
*Summary* paragraph was headed `**stdout is empty.**` and explained the
mechanism as *"when `stdout` is a file or a pipe every byte of that is
discarded"* — a property of the defect, stated one paragraph above a table that
lists three deck shapes. The heading is now `**stdout is empty for this deck.**`
and a *Scoped 2026-08-15* paragraph follows the measurement: exact for the
six-byte reproducer and for `.op` + a `.control run`, **false for `.op` +
`.tran 1n 10n`**, which keeps 324 bytes because the transient path flushes on
its way past. The *Impact* bullet that read *"Zero bytes of `stdout` survive
under a redirect"* is corrected the same way. Two other `stdout` sentences were
checked and left: criterion 6's is scoped to "this deck" and is about what
`check.sh`'s `FILTER` would leave, and *Resolution*'s row 4 is about the eight
witnesses.

**(b) The upstream line number.** Added once, at the end of *Root Cause* step 2,
which is where `outitf.c:479` is attached to quoted code and therefore where a
reader goes looking. Re-measured against `pre-master-47` with `git show`, no
checkout:

```
$ git show pre-master-47:src/frontend/outitf.c | grep -n "if (numNames &&"
461:        if (numNames &&
$ grep -n "if (numNames &&" src/frontend/outitf.c
479:        if (numNames &&
$ git show pre-master-47:src/frontend/dotcards.c | grep -n "assert(plot_cur->pl_dvecs"
225:        assert(plot_cur->pl_dvecs != NULL);
```

`run->refIndex = -1` is `:291` upstream against `:303` here, so the note says
the file's `outitf.c` numbers are this branch's generally rather than at one
line only. It also says `dotcards.c` needs **no** translation — the assertion is
line 225 in both trees — which is the fact a reader following the fix upstream
actually needs. The issue is a branch document and keeps `:479` everywhere else.

**(c) Three stale clauses, not on the item's list, corrected because item 5 made
them false.** 0072 said in three places that the upstream material "is not
written yet": criteria row 9, *Where it lands*, and the closing *Nothing has
been sent upstream* paragraph. All three were true when item 4 wrote them and
were falsified by item 5 four hours later. Receipt 05's *For the owner* 6 flags
exactly this and hands it to item 6. Each is now one clause: the material is
written, it is at `upstream-0072/`, and it is **prepared and not sent**. The
"Nothing has been sent upstream" heading and the *Status* sentence *"Released
`ngspice-46` still aborts, because nothing has been sent upstream"* are still
true and are untouched.

### 3. Cross-references, in the two *Related* sections that earned them

- **0072 → 0069.** The existing 0069 bullet gains three sentences: 0069 is not
  fixed by decision, its last open criterion was closed on 2026-08-15 by four
  decks in `tests/regression/exitstatus/` — the same directory 0072's three live
  in — and neither could be asserted before `tests/bin/check_status.sh` existed.
- **0069 → 0072.** A new bullet: 0072 is fixed at `dotcards.c:225`, prepared for
  upstream and not sent; it was found by running 0069's own *Resolution* listing
  back when that listing had lost its netlist lines; the two are the same
  complaint from opposite ends; and both became assertable at the same moment,
  because the harness was built to close 0069's criterion 3 and 0072's decks are
  in the directory it created.

Nothing else in either file was restated.

### 4. `doc/claude/casemode-distinguish-guide.md` — **nothing added**

Read first, as instructed. §9's *Guard the run with `$sim_status`, not with the
exit status* does **not** invite a pointer, so none was added and the guide is
byte-unchanged.

The subsection is a prescription for a client program: a complete runnable deck,
a three-row measured table, three measured properties, two hazards. It names one
`doc/` path (`doc/codex/issues/0069`, for why the exit status cannot be read)
and no file under `tests/` — measured, `grep -niE "tests/|regression|asserted"`
over lines 338-426 returns nothing. The guide does cite a regression guard once,
at line 697, but in a different subsection and to back a different kind of claim:
*"`.save` stays byte-exact — permanently"* is a durability guarantee, and
`tests/regression/casedist/save-name-case.cir` is named as the thing that keeps
it that way. §9's guard subsection makes no such guarantee about the guard
shape; its one durability claim (*"That is not going to change, by decision"*)
is about ngspice's exit status, which the new decks do not assert. A pointer
there would be a repo-internal path in a client-facing document, supporting no
sentence that is in it.

### 5. `PLAN.md` and `LEDGER.md` are now tracked

Both were untracked for the whole batch. `doc/claude/batches/2026-08-15-xschem-open-items/`
tracks both of its own, so they are added here to match. This is why the commit
shows the plan as an addition; its content is unchanged from what the five crews
worked from.

## What was audited

Four verifications and two audits, all from the repository root.

### Full `make check`

```
$ cd build-ver_50 && make check
$ grep -c '^PASS:' makecheck.log   331
$ grep -c '^FAIL:' makecheck.log     0
```

**331 PASS / 0 FAIL**, `make` exit status 0. `XFAIL`, `SKIP` and automake
`ERROR:` results are all zero. One log line reads `ERROR: (internal)  tried to
destroy non-existent graph`; it is simulator output from inside
`tests/regression/casedist/vector-probe-report.cir`, which PASSes, and receipt
02 already recorded it as pre-existing. Both lint specs report
`identity lint: 264 comparisons, baseline matches` and `identity lint: 11
comparisons, baseline matches`.

### `tests/lint/identity.baseline`, across the whole batch

```
$ git diff --stat f14fc04aa..HEAD -- tests/lint/identity.baseline
$ git diff --stat f14fc04aa..HEAD -- tests/lint/
```

Both empty. Verified against **`f14fc04aa`**, the batch's start commit, not
against `4bdc935a4`. Nothing under `tests/lint/` moved at any point.

### `git status`

Before this commit: two modified tracked files, both `doc/codex/issues/`, plus
the two untracked batch documents above. That every other untracked path is
pre-existing was measured rather than assumed — each `??` entry's mtime was
compared against `f14fc04aa`'s commit date (2026-08-15 12:34:53 -0700), and the
**only** untracked files newer than it are `PLAN.md`, `LEDGER.md` and this
receipt, all three of which this commit adds. The `examples/plot/py*.py`,
`tests/regression/casedist/*.cir`, `one_save.raw` and
`doc/claude/suggestions/next-session-*.md` families all predate the batch. No
`core` or `core.*` file exists anywhere in the tree. `build-ver_50/tests/
regression/exitstatus/sim-status-guard-ok.raw` is present, which is correct —
it is the artefact the negative-control deck's `.files` line asserts, it is in
that directory's `CLEANFILES`, and it is in the build tree, not the repository.

### Audit 1 — text claiming an upstream submission has been sent

**None found.**

```
$ git diff f14fc04aa..4bdc935a4 --unified=0 | grep '^+' | grep -v '^+++' \
    | grep -ciE '\b(sent|submitted|submission)\b'
15
```

All fifteen were read. Every one is a denial or a restatement of the
constraint — the four receipt preambles (*"nothing here states or implies that
any upstream submission has been sent"*), receipt 05's *"The material says in as
many words that it has not been sent"*, `README.md`'s *"This material is
prepared and has not been sent"*, and 0072's *"Nothing has been sent
upstream"*. A wider sweep adding `posted|mailed|upstreamed|merged upstream|
accepted|filed upstream` returns no further line. This commit adds only
denials, and it removes the three clauses that were stale in the other
direction — they understated, saying the material was not written; none of them
claimed it had been sent.

### Audit 2 — files under `doc/claude/upstream/`

**None. The directory is untouched by every commit in the batch.**

```
$ git diff --stat f14fc04aa..HEAD -- doc/claude/upstream/
$
```

Empty output, exit 0. This is the only command in this item that names that
path, and it reads no file contents.

### The commit range

**`f14fc04aa..HEAD`**, six commits, confirmed with `git log --oneline`:

| commit | summary |
| --- | --- |
| `baccfa570` | `test: assert a batch run's exit status` — the driver, the directory, two decks |
| `2699f9969` | `test: assert the $sim_status guard shape` — four decks, 0069 criterion 3 |
| `1c07b937b` | `docs: measure 0072's two guard sites` — receipt 03 only, no production code |
| `ce48f9630` | `fix: report an .op card with no netlist instead of aborting` — the guard, three decks, 0072 rewritten |
| `95772f307` | `docs: receipt for 0072's fix at dotcards.c:225` |
| `4bdc935a4` | `docs: prepare 0072's upstream patch and report` — `upstream-0072/`, three files |
| *this commit* | `docs: close the exit-status batch` — ledger, two issues, this receipt |

`git diff --stat f14fc04aa..4bdc935a4` — the five landed commits — is 43 files,
3368 insertions, 66 deletions: one production file
(`src/frontend/dotcards.c`), `configure.ac`, `tests/bin/check_status.sh`, the
whole of `tests/regression/exitstatus/`, `tests/regression/Makefile.am`, and
`doc/`. This commit adds only `doc/`.

## What was left, deliberately

- **`doc/claude/casemode-distinguish-guide.md` is byte-unchanged.** The reason is
  in *What changed* 4. The condition the item set was not met, so nothing was
  added rather than something small being added anyway.
- **`doc/claude/feedback/ngspice_upstream/RESPONSE.md` was not touched**, though
  receipt 02 noted it now has a deck it could point at. The item names 0069,
  0072 and the guide and no fourth file, and the round-3 reply is a client-facing
  document whose four upstream-dependent passages the previous batch's ledger
  already flags as having to move together. Not a cross-reference to make in
  passing.
- **0069's Status line still reads "Open, filed 2026-08-14"** and its Resolution
  is untouched. All five criteria are met; whether the issue closes is the
  owner's call and is *Waiting on the owner* 5.
- **0072 does not carry its own fix commit's hash.** *Waiting on the owner* 4.
  Adding it is a one-line follow-up and was not this item's to take.
- **No further rewriting of 0072.** It has been rewritten twice today. Beyond
  the two assigned corrections, only the three clauses this batch itself
  falsified were touched, and each is one clause.
- **`.gitignore`, `#include <assert.h>`, and the `upstream-0072/` material** were
  all left exactly as item 5 left them. The first two are *Waiting on the
  owner* 7; the third is item 1.
- **No test, no `.out`, no `Makefile.am`, no rebuild.** `make check` was run, not
  changed.

## For the owner

1. **Three edits to 0072 were not on this item's list, and you may want to
   reverse them.** The item named two corrections; the third is the *"not
   written yet"* clause in three places, which item 5 made false and which
   receipt 05 explicitly handed to this item. Each is one clause and each says
   *prepared and not sent*. If you would rather 0072 had been left saying the
   material does not exist, they revert cleanly.
2. **Nothing was added to the guide, and that is a judgement call.** §9's guard
   subsection does not invite a pointer, on the reading in *What changed* 4. If
   you read *"That is not going to change, by decision"* as the same kind of
   durability claim the `.save` subsection makes, then a one-sentence pointer to
   `tests/regression/exitstatus/sim-status-guard.cir` is earned and I did not
   make it. Receipt 02's *For the owner* 2 is the caveat that would have to go
   with it: those decks assert the guard's shape, not the exit-status decision.
3. **`PLAN.md` is now tracked.** It was untracked for the whole batch; the
   xschem batch tracks its own, so this one matches. If plans are meant to stay
   out of the tree, this is the commit to drop it from.
4. **The seven-item hand-off is in `LEDGER.md`, not here**, so there is one
   place to read it. Item 1 of that list is the only one that blocks anything:
   whether the `upstream-0072/` material is sent. Nothing from this repository
   has been sent.

Nothing was pushed. One commit on `ver_50`.
