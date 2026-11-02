# Plan: ver_52, upstream fixes on top of ver_51

Owner, 2026-11-02: create `ver_52` and pull in whatever the driver deems fit,
leaving out the new RF suite. Triage: `../2026-11-01-upstream-triage/receipts/01-triage.md`
and `02-seed.md` (these are on `ver_51` and on this branch). Read the row for
each commit before picking it.

## Where

- Branch `ver_52`, off `ver_51`, in its own worktree `/home/analog/dev/ngspice-ver_52`.
  All work happens there. Never touch `/home/analog/dev/ngspice` (the owner's edits
  and `ver_51`) or `build-ver_50` (xschem measures with that binary), except to run it.
- Build tree `/home/analog/dev/ngspice-ver_52/build-ver_52`, configured
  `../configure --prefix=$PWD/stage --with-readline=yes --enable-pss --enable-cider`.
- Upstream commits are local; `refs/upstream/pre-master-48` is at `ac2798364`.
- Nothing is pushed or posted.

## Rules for every crew

1. One crew at a time. Prefix `autogen.sh`, `configure`, `make`, `make check`,
   `make install` and large simulations with `ngheavy`; `-j4` at most. Exit 137 means
   the memory cap killed the job: report it, never raise the cap.
2. `git cherry-pick -x <hash>`, oldest first within a group, so authorship stays with
   upstream. A clean pick keeps its message. A hand-resolved pick says what was resolved
   and ends with the `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>` trailer,
   as does every commit written here. Don't squash.
3. Gates after each group: `make` exit 0; `make install`; full `make check` with 0 FAIL,
   no new SKIP; `measure/run.sh` compared with the baseline. Never edit
   `tests/lint/identity.baseline`: a new runtime comparison gets `ng_ideq` or a
   `/* case-lint: <reason> */` comment in a separate commit.
4. Never weaken an assertion or regenerate an `.out` to get a pass. If a pick changes a
   fork test's result, work out why. Update the reference only when the change is the
   pick's stated purpose, and show old value, new value and reason in the receipt.
5. A pick that fails a gate, or that changes results in a way nobody can explain, is
   dropped with `git revert` or never picked, with the reason recorded. Dropping is a
   good outcome; do not force it. Report anything you could not verify as unverified.
6. Fork-written behaviour changes go RED first, with the failing output in the receipt.
7. Must stay true throughout, checked by `measure/`:
   - `setseed k` draws are bit-identical with `/usr/bin/ngspice` 45.2.
   - The draw log contract: field order, header, footer, instance path forms.
     A changed bin key `xmN:model.K` is expected from `03ef19244` and must be noted.
8. Receipts go in `receipts/NN-name.md` and are committed with explicit paths. Say what
   was measured, and quote the numbers.

## Not taken, with reasons

RF suite, arrowplot, CMake, line search, optimizer, checkpoint (features, owner's scope).
`940bb50fa` (project of its own: PDK load, log contract). `d7e64e28d` and `072980a57`
(defects still at upstream tip). `95d7f2c2b`, `059066b53`, `628d84aa5`, `cbb09dc25`
(seed: see group 8). rshunt rewrite and its followers, topology reverts, hicum2 removal,
quoted-string series, cosmetic and library-only rows.
