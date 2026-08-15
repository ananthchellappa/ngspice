# Issue: A Plot Copied From a File That Recorded No Mode Is Stamped With the Copying Session's Mode

## Status

Fixed 2026-08-15 on branch `ver_50`, filed the same day. See Resolution.

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

Fixed as the criteria describe: `struct plot` (`src/include/ngspice/plot.h`)
gained `bool pl_fromfile`, `raw_read()` sets it on every plot it builds —
whether or not that file carried an `Option:` line — and the writer's
provenance test at `src/frontend/rawfile.c` is now `!pl->pl_fromfile` in place
of `!pl->pl_env`. `TMALLOC` and `plot_alloc()` zero the struct, so every other
origin defaults to "made here" without being touched.

RED first, both defects, quoted from the pre-fix binary:

```
ERROR: the copy of a file that recorded no mode carries 1 casemode lines, last <Option: casemode=fold>
ERROR: the copy re-spelled the line: <Option: casemode=preserve> became <Option: casemode = preserve>
```

Three new decks in `tests/regression/pipe/`, and the existing ones the
criteria named pass unmodified:

- `rawfile-casemode-foreign.cmd` — criteria 1 and 4. A `preserve` file written
  with the gate shut (asserted to carry no `Option:` line at all), loaded by a
  folding session that has asked for the line; the copy claims nothing, and
  still claims nothing after the plot has been left, re-selected by name,
  renamed and written from a later command. Check 5 is the control that this
  session's own plot still gets `Option: casemode=fold`.
- `rawfile-casemode-keysyntax.cmd` — criterion 5. A copy is byte-identical in
  its `Option:` line to the file it came from, a copy of the copy too, and the
  same for a hand-spliced key that is not `casemode`, so a fix that
  special-cased the one key would fail.
- `rawfile-casemode-derived.cmd` — the copy-path question below.

### The two open questions, settled

- **A loaded plot with no recorded mode says nothing**, as criterion 1 asks.
  The argument for `casemode=unknown` did not survive the reason the line is
  opt-in: it puts a key into the reader's variable space, which is what makes
  `0067` fatal on released binaries.
- **The copy paths do need the field, and three of them did not have it.**
  Measured, not assumed: `load` a `preserve` file, `linearize`, `write`, and
  the derived plot carried `Option: casemode=fold` over names no folding run
  can spell — the same defect one step downstream. `com_linearize()` and
  `com_cutout()` (`src/frontend/linear.c`) now carry `pl_fromfile` from the
  plot they derive from, and `com_fft()`, `com_psd()` (`src/frontend/com_fft.c`)
  and `com_spec()` (`src/frontend/spec.c`) take it from the vectors they were
  handed, through a new `vec_fromfile()` (`src/frontend/vectors.c`). The other
  paths were audited and need nothing: `com_write()` and `com_write_sparam()`
  (`src/frontend/postcoms.c`) `memcpy` the whole struct, so the field travels
  already; `plot_docoms()` creates no plot despite the name; `plot_alloc()`'s
  other callers are this session's own analyses. `oldread()` in
  `src/ngsconvert.c` sets the field for consistency — that program cannot open
  the gate today, so it is a claim about the data and not a fix.

### The key's syntax, and the one exception

The generic `pl_env` dumper now writes `Option: name=value`, agreeing with the
casemode writer, so a round trip is a fixed point. It keeps the spaces for
exactly two value shapes, because closing them up changes what the line means:
a value starting with `,` (`k = ,b` reads back `,b`, `k=,b` reads back `b`, and
a bare `,` takes the whole line down), and one starting with `<=` or `>=`
(which `cp_lexer()` holds together only at the start of a word). Everything
else — `;`, `&`, a lone `<` or `>`, lists, quoted strings, values containing
`=` — was measured to read back the same from either spelling. No mode name
can take the exception.

### One existing test changed behaviour

`tests/regression/pipe/unset-rawfile-option-key.cmd` asserted that a loaded
plot whose `casemode` key had just been unset still got a mode written into
the copy, on the reasoning that "the line raw_write() puts under `Plotname:`
is the writing session's and is written afresh every time" — which is a
description of this defect. It now asserts the opposite, and keeps its own
claim, which belongs to `0067`: the file survives the unset, reads back, and
still carries its data.
