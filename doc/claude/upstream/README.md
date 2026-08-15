# Upstream submission: the `cp_remvar()` crash

What is here, what goes in the mail, and what the repo owner has to do to send
it. **Nothing here has been sent.** Sending is the owner's action; no session
may do it.

| file | what it is | goes to upstream? |
| --- | --- | --- |
| `EMAIL-cp_remvar.txt` | the message to send to `ngspice-devel@lists.sourceforge.net`, ready to paste | **yes** — the body |
| `0001-cp_remvar-ownership.patch` | `src/frontend/variable.c` — the ownership rule. On its own it clears reproducers 3–6 | **yes** — attach |
| `0002-curplotdate-static.patch` | `src/frontend/options.c` — the `Spice_Build_Date` guard. On its own it clears reproducer 1 | **yes** — attach |
| `repro-option-line.raw` | a 377-byte ASCII raw file carrying `Option: myopt = seven`, for the loaded-plot reproducer | only on request — the mail offers it |
| `cp-remvar-crash-and-the-header-default.md` | the long-form report: the bug, the six shapes, and why it gates our raw-header default | **no** — ours, and it names our issues and our client |

## How the owner sends it

1. Open `EMAIL-cp_remvar.txt`. Its first two lines are `To:` and `Subject:`;
   drop them if the composer has its own fields, and paste the rest from
   `Hello,` down as the body. The diffstat at the bottom is part of the body.
2. Attach `0001-cp_remvar-ownership.patch` and
   `0002-curplotdate-static.patch`. Both, to this one message — the body says
   so, so a maintainer does not sit waiting for a `1/2` and a `2/2` that never
   come.
3. Send **plain text, not HTML.** A list that reflows the body will break the
   inline code and the diffstat.
4. Do not attach `cp-remvar-crash-and-the-header-default.md` or
   `repro-option-line.raw`. The report is internal. The raw file is offered in
   the last paragraph of the mail and can go in a follow-up if asked for; it is
   four header lines and three numbers, and a maintainer can build one faster
   than they can un-quarantine an attachment.
5. The mail signs *Ananth Chellappa*. Send from the address that should own the
   patches, because that is the address a maintainer will credit and reply to.
6. `ngspice-devel` is a SourceForge list. If the sending address is not
   subscribed, expect the post to sit in moderation rather than appear at once;
   subscribing first is the shorter path.

## What to expect afterwards

- A reply on-list, not off it. The likely asks are a reword of the comments, a
  split, or a regeneration against a newer base. The mail's last paragraph
  already offers all three, and either patch can be regenerated from this
  branch in minutes.
- **Master is at the ngspice-47 news update.** `2f9c8ad47`'s summary is
  "Update the news for ngspice-47" and a binary built from it reports
  `ngspice-47`, so a release looks close. Taken before it, the fix ships in 47;
  taken after, 48. That is the whole reason to send now rather than next month.
- When — and only when — the fix is in a *released* ngspice,
  `doc/codex/issues/0061`'s **Left** section unblocks: "the condition is that
  0067 has been in a release." The flip is a one-word change in `raw_write()`
  plus check 0b and check 6 in two decks. Note what the condition is really
  about: the binaries that will open our files are other people's installs, so
  the release date is the earliest the flip may be *considered*, not the date
  it becomes safe.
- **Record the outcome here.** Add the date sent, the archive link or
  message-id, and any reply, so that no later session re-sends this or re-runs
  the validation below.

## Both patches are against upstream `master`, not this branch

They were generated and validated against a clean worktree of
`remotes/upstream/master` at `2f9c8ad47` ("Update the news for ngspice-47").

**Re-validated 2026-08-15**, at `9a848ff4f`, against the same ref — the local
`remotes/upstream/master` has not moved since the first validation, and no
fetch was made:

- `git apply --check`, singly and together — both clean, no offsets
- `./autogen.sh && ../configure && make -j12` — exit 0, no warning in either
  touched file
- all six reproducers on the patched build — rc=0
- all six on **unpatched** `2f9c8ad47`, built from the same worktree — 134,
  134, 134, 134, 134, 139, the same as `ngspice-46`
- **neither patch alone clears all six**, measured by building each in
  isolation: `0001` alone leaves reproducers 1 and 2 aborting, `0002` alone
  leaves 2 through 6, and reproducer 2 reaches both defects. Earlier text here
  and in the report said `0001` fixed "five of the six" / "repros 2–6"; that
  was never measured and is wrong. The mail now asks for both patches and says
  why.
- the two new diagnostics print: `Error: curplot cannot be unset.`,
  `Error: plots is read-only.`
- all six reproducers on released `/usr/local/bin/ngspice` (`ngspice-46`) —
  134, 134, 134, 134, 134, 139, unchanged from the first measurement
- upstream's own `make check` on the patched build — 58 tests, 0 failures,
  0 skipped, exit 0. The first validation's "across 42 test directories" does
  not reproduce by any count and has been dropped from the mail and the report;
  the receipt at
  `doc/claude/batches/2026-08-15-xschem-followup/receipts/03-upstream-submission-ready.md`
  §4 records what was counted instead.

`0002` needed regenerating from our commit at first submission, because this
branch had changed the surrounding `eq()` to `eqc()` for casemode work; the
version here matches upstream's context and is byte-identical to what `git
diff` produces from the applied upstream worktree.

`0001` was **regenerated 2026-08-15** and its content lines are unchanged. The
shipped copy had been hand-fitted to upstream context after a late comment
edit, which left two header lines wrong: an `index` line naming this branch's
blobs rather than upstream's, and hunk 4's new-side start line one low. Neither
stopped `git apply`, but the wrong `index` line is what `git apply -3` and
`git am -3` look up when a hunk does not match, so it would have turned a
trivially recoverable rebase into a failure. The file here is now `git diff`
output from the patched upstream worktree.

## Deliberately not included

The deletion of the `pl_env` read-only scan in `cp_usrset()`
(`doc/claude/decisions/0019`) is ours, not upstream's business: it is a
behaviour change tied to the casemode feature, and it is not needed to fix any
crash. That independence is measured, not assumed — the patched upstream build
above still has the scan, and the loaded-plot reproducer returns rc=0 there,
refusing the `unset` with `Error: myopt is read-only.` instead of crashing.
Re-measured 2026-08-15 on that build.

## Reproducing, quickly

```sh
printf 'unset curplot\nquit 0\n'                     | ngspice -p -n ; echo $?
printf 'set curplotdate=x\nquit 0\n'                 | ngspice -p -n ; echo $?
printf 'load repro-option-line.raw\nunset myopt\ndisplay\nquit 0\n' \
                                                     | ngspice -p -n ; echo $?
```

Released `ngspice-46` and unpatched `master` give 134, 134, 139. Patched gives
0, 0, 0. The `ngspice-46` column was re-run 2026-08-15 and is unchanged.

Capture the exit status straight into a variable. Writing
`echo "... rc=$?"` with a command substitution earlier in the same line reports
the substitution's status instead of ngspice's, which made a first pass at
these measurements read 0 everywhere.

## Why it matters here

`doc/codex/issues/0061` writes `Option: casemode=<mode>` into the raw header.
Any ngspice that loads such a file gets a `casemode` variable in the loaded
plot's environment, and a released ngspice that unsets it dies. That is why the
line is opt-in behind `casemodewrite`, default off, and the default cannot flip
until this fix is in a public release. See the report for the full argument.
