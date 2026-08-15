# Issue: A Plot Copied From a File That Recorded No Mode Is Stamped With the Copying Session's Mode

## Status

Open, filed 2026-08-15 on branch `ver_50`.

Introduced by `9e341a8b7` ("feat: record the case mode in the raw header,
opt-in", `doc/codex/issues/0061`) — this is a defect in that fix and not a
pre-existing one, because the header line it writes is the thing that can be
wrong. It is reachable only with `casemodewrite` set, which is off by default,
so no file this build writes today carries the wrong claim unless the user
asked for the line.

All measurements taken 2026-08-15 against `build-ver_50/src/ngspice`
(`ngspice-46+`, build stamp `Sat Aug 15 02:03:55 UTC 2026`).

Flagged at the close of the round-2 commit batch as the one item that survived
it — `doc/claude/batches/2026-08-12-casemode-client-feedback/`, stage 14 — and
written up here rather than left as a ledger note. It matters because the
client integration this whole batch serves
(`doc/claude/feedback/reply_from_xschem_session/REPLY.md`, question 1) is
building the consumer that trusts this line, and has said it would drop its
own heuristic in favour of it.

## Summary

`raw_write()` decides whether to write `Option: casemode=<mode>` by testing
whether the plot carries an environment:

```c
/* src/frontend/rawfile.c:189 */
if (!pl->pl_env &&
        cp_getvar_policy("casemodewrite", CP_BOOL, NULL, 0))
    fprintf(fp, "Option: casemode=%s\n", inp_case_mode_name());
```

`pl_env` is filled by `raw_read()` and by nothing else in the tree, so a
non-empty `pl_env` does prove the plot came from a file. The converse does not
hold, and that is the bug: **an empty `pl_env` does not prove this session
produced the plot.** A plot loaded from a file that carried no `Option:` lines
has an empty `pl_env` too, and is indistinguishable here from a plot the
running simulation just made.

Every raw file written by every released ngspice has no `Option:` lines. So
does every file this build writes with `casemodewrite` unset, which is the
default. The mis-identified case is therefore not a corner — it is essentially
every raw file in existence.

The comment above the test states the reasoning it actually implements —
"a plot this session produced carries no environment at all" — and that
sentence is true. The claim the code needs is the reverse one, and it is false.

## Impact

Load an old raw file, write it out with `casemodewrite` set, and the new file
asserts the copying session's mode as though it were a property of the data.
The same source file, copied three times:

```
$ printf 'set casemodewrite\nload prov_old.raw\nwrite out.raw\nquit 0\n' \
    | ngspice -p -D casemode=$m

session fold         -> Option: casemode=fold
session preserve     -> Option: casemode=preserve
session distinguish  -> Option: casemode=distinguish
```

`prov_old.raw` is one file, produced by one `preserve` run, and it says nothing
about its own mode. Nothing in it changed between the three copies.

The `fold` row is the one that shows the damage, because it is not merely
unsupported — it is **self-contradictory**. A genuine `fold` run of the same
deck writes:

```
Variables:
        0       v(in)   voltage
        1       v(mid)  voltage
        2       i(v1)   current
```

while the file claiming `casemode=fold` carries:

```
Variables:
        0       v(In)   voltage
        1       i(V1)   current
        2       v(Mid)  voltage
```

Capitals that a folding run cannot produce, in a file whose header says a
folding run produced it. A consumer that trusts the header — the one this
feature exists to serve — concludes fold semantics and then reads `v(In)`.
Its two available responses are both wrong: believe the header and mis-handle
the names, or believe the names and learn that the header is not worth reading.

The severity is bounded by the gate. `casemodewrite` is off by default, so this
needs a user who has opted into the line, and the client has said it will read
the line as soon as it ships. It should be correct before the default flips —
which `0061` schedules for once `0067` has been in a release.

## Secondary finding: the round-trip re-spaces the key

The same path has a second, smaller defect. Two writers emit `Option:` lines
and they disagree on syntax:

- `rawfile.c:191`, the new line: `Option: casemode=preserve`
- `rawfile.c:211`, the generic `pl_env` dumper: `Option: casemode = preserve`

So a file that carries the line and is loaded and written out again comes back
re-spaced:

```
$ head -5 rch_hdr.raw            ->  Option: casemode=preserve
$ ... load, write out_d.raw ...
$ grep '^Option:' out_d.raw      ->  Option: casemode = preserve
```

ngspice reads both — the parse arm at `rawfile.c:560` goes through `cp_lexer()`
and `cp_setparse()`, which is the control language's own `set` parser, and
`$casemode` reads `preserve` from either spelling (measured). The exposure is
to third-party readers: `FINDINGS.md` finding 1 and the client's question 1
both quote the key as `casemode=<mode>`, and a consumer that matches that
literally fails on a file ngspice itself wrote. Note that this arm is the
*correct* half of the provenance logic — it re-emits what the file said, which
is right — so the fix is to the format, not to the branch.

## Root Cause

`pl_env` is doing double duty as data and as provenance. It answers "what
options did this plot's file carry" and is being read as "did this plot come
from a file". Those are the same question only when a file carries at least one
option, which is the rare case rather than the common one.

`struct plot` (`src/include/ngspice/plot.h`) has no field recording where the
plot came from. `raw_read()` knows — it is the only writer of `pl_env` and the
only reader of files — and discards the fact.

## Acceptance Criteria

1. A plot loaded from a raw file that carries **no** `Option:` lines is not
   given a `casemode` line when it is written out, with `casemodewrite` set,
   under any of the three modes. Asserted against a file produced by a run in a
   *different* mode from the writing session, so that a wrong answer is
   distinguishable from a right one.
2. A plot loaded from a file that **does** carry `Option: casemode=<mode>`
   still comes out carrying that same mode and only that mode — the existing
   behaviour, which `tests/regression/pipe/rawfile-casemode-rewrite.cmd`
   already pins. That test must keep passing unmodified.
3. A plot this session's simulation produced still gets the line under
   `casemodewrite`, in all three modes — the existing behaviour, pinned by
   `tests/regression/pipe/rawfile-casemode-header.cmd` and
   `tests/regression/casedist/rawfile-casemode-header.cir`. Both must keep
   passing unmodified.
4. The provenance record survives a plot being made current, renamed, and
   written from a later command, not only written immediately after `load`.
5. Both writers agree on the key's syntax, and a file that is loaded and
   written out again is byte-identical in its `Option: casemode=` line to the
   file it came from. A round trip must be a fixed point.
6. No new `strcmp`-family comparison of a deck-written name without the
   annotation `tests/bin/identity_lint.sh` requires; `identity.baseline`
   unchanged or the addition justified in the commit message.
7. RED first: each of 1 and 5 written as a failing assertion against the
   current tree before the fix, with the failure quoted in the commit message.

## Resolution

Not started. The shape the criteria are written against is a provenance field
on `struct plot` — set by `raw_read()` for every plot it builds, regardless of
whether that file carried any options, and tested at `rawfile.c:189` in place
of `!pl->pl_env`. That is the minimum that distinguishes the two origins an
empty `pl_env` currently conflates.

Two things to settle when it is picked up, neither of which the criteria
prejudge:

- **Whether a loaded plot with no recorded mode should say so explicitly**
  rather than say nothing. An `Option: casemode=unknown` would let a consumer
  tell "this file predates the feature" from "this file was written by a build
  that had the feature switched off", which silence cannot. Against it: it puts
  a key into the reader's variable space, which is the exact shape that makes
  `0067` fatal on released binaries, and the whole reason the line is opt-in.
  Silence is the safe default and is what criterion 1 asks for.
- **Whether `plot_docoms()` and the other places that copy or derive a plot**
  need to propagate the field. Criterion 4 exists to force that question to be
  answered by measurement rather than assumed; the copy paths were not audited
  while filing this.
