# Receipt 03 — the upstream `cp_remvar` submission, made current

Item 3 of `doc/claude/batches/2026-08-15-xschem-followup/PLAN.md`. Branch
`ver_50`, HEAD at start `9a848ff4f`.

**Nothing was sent.** No mail, no post, no push, no PR, no network call of any
kind. The item ends with a submission a human can paste and send, and the list
of actions that human still has to take is at the bottom of this file.

## 1. The ref the re-validation ran against

`remotes/upstream/master` is still `2f9c8ad4707f776d1ece0eeeb4bd5c98a0f99f98`
("Update the news for ngspice-47", 2026-08-08) — the same commit the first
validation used. It has **not** moved, and no fetch was made: the instruction
was to work with the refs the repo already has. `remotes/upstream/HEAD` points
at `pre-master-47` (`c5cd68015`, 2026-08-07), which is behind master, and
`master` is the newest of all 200-odd `refs/remotes/upstream/*` by commit date.

So "re-validated against today's upstream master" and "re-validated against
`2f9c8ad47`" are the same statement here. If the owner fetches before sending
and master has moved, the apply check below is the one thing worth repeating.

A detail worth acting on: a binary built from `2f9c8ad47` reports itself as
**ngspice-47**, and the tip commit is the 47 news update. A release looks
close, which is the argument for sending now rather than later.

## 2. What was re-measured today, on this machine

Scratch worktrees under
`/tmp/claude-1000/-home-qflow-dev-ngspice-test/.../scratchpad/`, removed at the
end. Nothing in `src/` or `tests/` of this repo was touched.

| claim | how it was re-checked | result |
| --- | --- | --- |
| both patches apply to `2f9c8ad47` | `git apply --check`, singly and together, on a clean worktree | clean, **no offsets** |
| the applied tree is the tree that was tested | `diff` of both files against the built worktree | identical |
| it builds | `./autogen.sh && ../configure && make -j12` | exit 0 |
| no new warnings in the touched files | grep of the 3.3k-line build log for `variable.c:`/`options.c:` warnings | none (265 warnings elsewhere, all pre-existing upstream noise) |
| six reproducers on released `ngspice-46` | `/usr/local/bin/ngspice`, each session run | 134, 134, 134, 134, 134, 139 — unchanged |
| six reproducers on the patched build | same six sessions | 0, 0, 0, 0, 0, 0 |
| six reproducers on **unpatched** `2f9c8ad47` | both patches reverted, rebuilt from the same worktree | 134, 134, 134, 134, 134, 139 — so the mail's "identical results on both" holds |
| which patch clears which | each patch built **alone**, two more rebuilds | **the documented split was wrong** — see §3 |
| the mail's "read-only" claim | patched build, which still has upstream's `pl_env` scan | `Error: myopt is read-only.`, rc=0 |
| the new `US_DONTRECORD` message | patched build | `Error: curplot cannot be unset.` |
| `README`'s quick-repro trio on `ngspice-46` | run verbatim | 134, 134, 139 |
| upstream's own `make check` | full run on the patched build | 58 PASS, **0 FAIL**, exit 0 — see §4 |

## 3. The defect this item found: the per-patch split was never measured

Both the `README` ("`0001` … fixes five of the six reproducers", "`0002` …
fixes the sixth") and the report ("Fixes repros 2–6" / "Fixes repro 1")
attributed reproducers to patches. Neither figure came from a build; they were
read off the code. Both are wrong. Measured today by building each patch alone
on `2f9c8ad47` — four builds in all, so every cell below is a run and not an
inference:

| repro | neither | `0001` only | `0002` only | both |
| --- | --- | --- | --- | --- |
| 1 `set curplotdate=hello` | 134 | 134 | **0** | 0 |
| 2 `unset curplotdate` | 134 | 134 | 134 | **0** |
| 3 `unset curplot` | 134 | **0** | 134 | 0 |
| 4 `unset plots` | 134 | **0** | 134 | 0 |
| 5 `source`/`set temp`/`unset temp` | 134 | **0** | 134 | 0 |
| 6 `load`/`unset myopt`/`display` | 139 | **0** | 139 | 0 |

`0001` alone clears **four**, not five. `0002` alone clears **one** — and it is
reproducer 1, not "the sixth", which is what the old `README` row read as.
**Reproducer 2 reaches both defects and is clear only with both patches**:
`unset curplotdate` runs the static `free()` in `cp_usrset()` *and* the
unconditional free in `cp_remvar()`'s tail.

This matters to the recipient, not just to us. A maintainer who takes one patch
and not the other is left with a crashing `unset curplotdate` and no note
saying so. The mail now carries the split and asks for both patches; the
`README` table and the report's §6 are corrected, and the report gains the
matrix above.

The mail's own prose was already right on this point — "Patch 2 … is what the
first two reproducers hit" — and is unchanged.

## 4. Upstream's `make check`

Re-run in full on the patched build: **58 PASS, 0 FAIL, 0 skipped, exit 0.**
The 58 and the 0 reproduce the first validation exactly, which also confirms
the two runs used the same configure options (bare `../configure`).

The first validation's fourth figure, "across 42 test directories", does
**not** reproduce and was dropped rather than guessed at. No count taken from
the run yields 42: the recursion enters 21 directories under `tests/`, 20 of
which run a test; `tests/` has 39 `Makefile.am`s and 78 subdirectories; the
whole `make check` prints 110 "Making check in" lines over 99 distinct targets.
Whatever 42 counted is not recoverable from the log, so the clause is gone from
both the mail and the report. Nothing else in that block changed.

Model-QA directories print `DIFFER (max rel error is …)` lines throughout; those
are the harness's within-tolerance reporting, and every `qaSpec` still ends
`PASS`.

## 5. What was corrected

### `EMAIL-cp_remvar.txt`

- **The diffstat at the bottom was wrong.** It said `variable.c | 69` and
  `61 insertions(+)`. The patches produce `70` and `62 insertions(+)`. A
  maintainer who applies the patches and compares is the first person to see
  that, so it is the worst place in the mail to be off by one. Now taken
  verbatim from `git diff --stat` on the applied worktree.
- **How the patches arrive was not stated.** The subject is `[PATCH 0/2]`,
  which conventionally means a cover letter with two follow-ups; both patches
  are in fact attached to the one message, and they are plain `git apply`
  diffs, not `git am` mailboxes. Two sentences now say so, and offer a
  format-patch series instead.
- **`d.cir` in reproducer 5 was unexplained.** One line now says it is any deck
  with an analysis in it.
- **"across 42 test directories" removed** from the `Validation` block; see §4.
- **The per-patch split added** to the `Validation` block, with the request to
  take both patches; see §3.

Nothing else in the mail was stale. It makes no claim about the state of this
tree, about what is or is not committed here, or about the raw-header default
schedule — it never mentions `casemode`, and the one paragraph about this
branch ("I have deliberately left my branch's other changes to these two files
out") is still true after `9e341a8b7` added more of them to `variable.c`. That
scoping was already right and was left alone: the case-mode feature is our
business, not upstream's.

### `0001-cp_remvar-ownership.patch`

**Regenerated. Content lines unchanged** — verified by diffing the old file
against `git diff` output from the applied worktree: only two header lines
differed.

The shipped copy had been hand-fitted to upstream context after a late comment
edit, which left it with an `index` line naming *this branch's* blobs
(`058ef194f..b11dae687`, from `ac1819ff8`) rather than upstream's
(`bef5a72f9..4ef6e233c`), and hunk 4's new-side start line one low (`699` for
`700`). `git apply` tolerated both, reporting only "Hunk #4 succeeded at 700
(offset 1 line)" — but the `index` line is exactly what `git apply -3` and
`git am -3` look up when a hunk does not match, so on any base but this one it
would have turned a recoverable three-way merge into a hard failure. The file
is now `git diff` output and applies with no offset.

`0002` was already byte-identical to `git diff` output and is untouched.

### `cp-remvar-crash-and-the-header-default.md`

- **§6's per-patch attribution was wrong**, as §3 above sets out; it now
  carries the measured four-column matrix. §3 of the report gains one paragraph
  saying reproducer 2 is the shape that needs both.
- **§7's `make check` row lost the "42 directories" clause**, with a note
  saying why.
- **§8's second bullet was stale.** It told the reader to fix
  `doc/codex/issues/0070` before the default flips. `eb0b96c8c` fixed it (with
  `7b5884249` repairing the `constantplot` initialiser the new `pl_fromfile`
  member exposed), after this report was written. The bullet is struck through
  and the fix named, so the remaining prerequisite reads as the release and
  nothing else.
- **§6's "29 pipe-suite decks" is now 32.** The number was correct when
  measured and the split has not been re-measured since, so it is pinned to
  `23fee705b` rather than updated, and the sentence now says the three decks
  `eb0b96c8c` added are outside it. The "three `unset-*-name.cmd` guards" count
  is still right — the fourth `unset-*.cmd` deck does not match that glob.

### `README.md`

Rewritten around what the owner has to do. The file table now says which files
go to upstream and which do not; there are new **How the owner sends it** and
**What to expect afterwards** sections; both patch rows now give the measured
"on its own it clears …" instead of the wrong five/one split; and the
validation block is re-dated and split into what was re-measured today and what
was carried over. The header now says in its first two lines that nothing has
been sent and that sending is the owner's action.

## 6. Carried over from the first validation, not re-measured today

- **The `0019` independence measurement in its original form.** The claim that
  the read-only scan can stay while the crash is fixed *was* re-measured today
  on the patched upstream build, which has the scan — rc=0 and
  `Error: myopt is read-only.`. What was **not** re-run is the other half: the
  build of *this branch* with the ownership rule applied and the scan restored,
  and the resulting "three of 29 pipe decks fail" split. That number stays
  pinned to `23fee705b`.
- **`0002`'s regeneration history.** That it originally had to be regenerated
  because this branch had `eqc()` where upstream has `eq()` is carried over
  from the first validation; today's check only confirms the file matches
  upstream's context.
- **Anything about the mailing list itself.** No network call was made, so the
  list's posting policy, its attachment handling and whether the sending
  address is subscribed are all unverified. The `README` states the moderation
  caveat as an expectation, not a measurement.

## 7. What is left for the owner — the exact list

1. **Decide whether to fetch upstream first.** If you do and
   `remotes/upstream/master` has moved past `2f9c8ad47`, re-run
   `git apply --check` for both patches against the new tip and update the two
   places the mail names the base (subject line body, first paragraph, and the
   `Validation` heading line). If it has not moved, nothing to do.
2. **Send the mail.** Body is `doc/claude/upstream/EMAIL-cp_remvar.txt` from
   `Hello,` down, including the diffstat at the end. `To:` and `Subject:` are
   its first two lines. Plain text, not HTML.
3. **Attach both patch files** — `0001-cp_remvar-ownership.patch` and
   `0002-curplotdate-static.patch` — to that one message. Do **not** attach
   `cp-remvar-crash-and-the-header-default.md` (it is internal and names our
   issues and our client) or `repro-option-line.raw` (the mail offers it on
   request).
4. **Send from the address that should be credited.** The mail signs *Ananth
   Chellappa*. If that address is not subscribed to `ngspice-devel`, subscribe
   first or expect moderation delay.
5. **Record the outcome in `doc/claude/upstream/README.md`**: date sent,
   archive link or message-id, and any reply. The `README` says to; this is the
   thing that stops a later session re-sending or re-validating.
6. **Do not flip `casemodewrite` yet.** `doc/codex/issues/0061`'s condition is
   "0067 has been in a release" — acceptance upstream is not a release, and a
   release is not the same as the installed binaries in the field having it.

## 8. Not done, and why

- No code change. The item is documentation and validation; nothing in `src/`
  or `tests/` of this repo was touched, per the plan's rule.
- The two scratch worktrees this item created (`upstream-check`, `pristine`)
  were removed with `git worktree remove --force` and pruned. Three worktrees
  under another session's scratch directory remain in `git worktree list`; they
  are not this item's and were left alone.
- `PLAN.md` and `LEDGER.md` in this batch directory are untracked and are the
  driver's to commit. They are not in this item's commit.
