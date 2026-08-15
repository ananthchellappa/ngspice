# Item 1 receipt — the two client documents, corrected for 0070

Crew for PLAN.md item 1. Branch `ver_50`, HEAD at start `7b5884249`.

Files edited (only these two, plus this receipt):

- `doc/claude/feedback/ngspice_upstream/RESPONSE.md`
- `doc/claude/casemode-distinguish-guide.md`

No `src/` or `tests/` change was needed. Nothing in the item turned out to
require code.

## What was measured, and against what

Every claim below was re-measured 2026-08-15 against
`/home/qflow/dev/ngspice_test/build-ver_50/src/ngspice` — `ngspice-46+`, build
stamp `Sat Aug 15 15:34:32 UTC 2026`, i.e. a build that carries `eb0b96c8c` —
with `/usr/local/bin/ngspice` (`ngspice-46`) for the one baseline number.
Scratch decks and raw files under
`/tmp/claude-1000/-home-qflow-dev-ngspice-test/.../scratchpad/m1`; nothing was
left in the repo.

### (a) The re-emitted pair is spelled closed up, and a copy is byte-identical

Original, written under `-D casemode=preserve` with `set casemodewrite` and
`set filetype=ascii`:

```
Plotname: Operating Point          <- line 4
Option: casemode=preserve          <- line 5
Flags: real
```

Loaded by a **folding** session (`-p -n -D casemode=fold`) with
`set casemodewrite`, then `write`:

```
Plotname: Operating Point
Flags: real
No. Variables: 3
No. Points: 1
Option: casemode=preserve          <- line 8
Variables:
	0	v(In)	voltage
```

`cat -A` on both lines: `Option: casemode=preserve$` — and
`diff <(grep casemode orig) <(grep casemode copy)` is empty. So: **same
spelling, different place** (line 5 vs line 8, i.e. under `Plotname:` vs after
`No. Points:`). Exactly one `casemode` line in the copy, and the variables are
still `v(In)`/`v(MidNode)` under the folding session.

**The writer's spaced-form exception, measured rather than read.** A hand-made
header carrying `Option: kcomma = ,b`, `Option: kle = <=y`,
`Option: kplain = hello` and `Option: casemode=preserve`, loaded and written
back out by the same folding session:

```
Option: kcomma = ,b$
Option: kle = <=y$
Option: kplain=hello$
Option: casemode=preserve$
```

which is the `pl_env` dumper's documented rule at `src/frontend/rawfile.c:262`
verbatim: spaces survive only for a value whose first character is `,` or which
begins `<=`/`>=`. **The client does need to know this**, in one clause, because
it is the reason the "trim both halves" half of the advice is still load
bearing: a foreign key can still arrive spaced even though `casemode` never
will. Both documents now say so and say that no mode name can reach that arm.

### (b) A copy of a file that recorded nothing records nothing

Same deck written with the gate **unset** under `preserve` — header has no
`Option:` line at all. Loaded by a folding session with `set casemodewrite`
and written out: `grep -c casemode copy_off.raw` → **0**, and `grep -n
'^Option:'` → nothing. Pre-`eb0b96c8c` this copy carried
`Option: casemode=fold`.

**One step further out, which neither document had:** a plot *derived* from a
loaded one records nothing either, even when the source file did record a mode.
One folding session, `set casemodewrite`, on a `preserve` tran file carrying
`Option: casemode=preserve`:

| written plot | `casemode` lines in the header |
| --- | --- |
| `linearize` of the loaded plot | 0 |
| `cutout` (`cut-tstart`/`cut-tstop`) | 0 |
| `fft v(MidNode)` | 0 |
| `psd 1 v(MidNode)` | 0 |
| `spec 500 20000 500 v(MidNode)` | 0 |
| the loaded plot itself, re-written | 1 — `Option: casemode=preserve` |

Contrast, same transforms on a plot the session **simulated** (`-b`,
`-D casemode=preserve`, gate set): `linearize` → `Option: casemode=preserve`,
`fft` → `Option: casemode=preserve`. So the derived plot inherits the
came-from-a-file mark but not the file's `Option:` pair — provenance survives a
copy and not a transform. Both documents now state this as its own caveat with
the measurement.

### (c) The commits

`git log` confirms all four are on `ver_50`: `9e341a8b7` (2026-08-14, header
line), `4e738fc3e` (2026-08-14, collision warning), `eb0b96c8c` (2026-08-15,
0070's fix), `7b5884249` (2026-08-15, the `constantplot` initialiser follow-up
to the same field). All three "in the working tree / not yet committed"
statements in `RESPONSE.md` are replaced by the hashes.

### One number checked and left alone

`RESPONSE.md` §1's byte counts are still exact on this build: the constants
artefact is **570** bytes with the gate unset, **592** with `casemodewrite`
set (22-byte delta = the header line), and **569** on stock
`/usr/local/bin/ngspice`. Measured in the default binary format — the 724/746
pair I first got was `filetype=ascii`, which is not the shape §1 is about. No
edit needed.

## What changed, section by section

`RESPONSE.md`

- Preamble (§ before §1) — the measurement provenance now separates the
  2026-08-14 body from the 2026-08-15 re-measurement of §2's caveats, and names
  `9e341a8b7`, `4e738fc3e`, `eb0b96c8c`, `7b5884249` in place of "in the same
  working tree and are not yet committed".
- §2 opening line — "It is in the working tree and measured" → committed, with
  the two hashes.
- §2 caveat list — "Three caveats" corrected to "The caveats" (there were
  already five bullets; there are now six). The `Option:`-key bullet keeps all
  of its advice (match the key, scan every `Option:` line, split on the first
  `=`, trim both halves, a line-5 check misses the second one) and now rests on
  the true fact: same spelling, two *places*, with the spaced-form exception
  named. The loaded-then-rewritten bullet now says a copy of a file that
  recorded nothing records nothing, cites 0070/`eb0b96c8c`, and says why the
  old stamp was wrong. A new bullet, "Provenance survives a copy, not a
  transform", carries the derived-plot measurement.
- §4 opening — "Shipped, in the working tree" → "Shipped and committed —
  `4e738fc3e`".
- Summary table — the `Option:`-key row now says "same spelling, two places",
  and a new row tells the client to treat a copy with no `casemode` line as
  unknown.

`doc/claude/casemode-distinguish-guide.md` (§ "The raw file can say which mode
wrote it")

- The read-it-as-a-key paragraph keeps the whole method and replaces "two
  spellings and two places" with "one key, one spelling, two places", plus the
  reason the trim still matters.
- "Three caveats" → "Four caveats".
- The loaded-then-rewritten caveat is split: what a copy keeps, then what a
  copy of a nothing-recording file does (nothing), with 0070 cited.
- New caveat paragraph "A derived plot claims nothing", with the measured list.

## Deliberately left alone

- **`doc/codex/issues/0061` carries both stale claims and was not edited**, as
  the item directs, and I agree with the direction. It quotes
  `Option: casemode = preserve` at eleven places (`:308`, `:754`, `:986`,
  `:1270-1271`, `:1275`, `:1383`, `:1453`, `:1485`, `:1493`, `:1531`) and its
  "Left" section (`:1587-1600`) states outright that "A loaded plot whose file
  recorded nothing still takes the writing session's mode", ending with "the
  rule is: … the copying session's otherwise". Both are now false. But 0061 is
  **Closed** (2026-08-13) and its Status says in terms that everything above
  the Resolution "is the measurement as it stood *before* the fix and is left
  as it was written"; its `:1453` spaced quote is an argument *from* the
  two-spellings gap, which is history the record should keep, and its "Left"
  paragraph is precisely the follow-up that 0070 was filed and fixed to close.
  Rewriting a closed issue's measurements to match a later build would destroy
  the record of why 0070 exists. 0061 is not client-facing; `RESPONSE.md` and
  the guide are, and they are corrected.
- `RESPONSE.md` §2's transcript blocks keep their 2026-08-14 build stamp. The
  `Option: casemode=preserve` line inside them re-measures identical; only the
  `Date:`/`Command:` stamps are from the older build, and the preamble now says
  so rather than my re-typing a header I did not produce.
- `doc/codex/issues/0070`'s own quotations of the spaced form (`:108`, `:116`,
  `:179`) are correct — they are the defect it fixed.

## For the driver or the owner

1. **Nothing blocks the client.** The consumer advice they are building
   against is unchanged in method; only the fact under it moved, and in their
   favour (one spelling, so a plain key match works).
2. **The derived-plot behaviour is a genuine provenance gap, not a defect I
   found and hid.** A `linearize`-then-`write` of loaded data loses the source
   file's mode record entirely. It is correct given 0070's rule (the session
   must not claim a mode for data it did not produce) and the pair is simply
   not carried into the derived plot. Closing it would mean carrying the
   source's `casemode` pair onto derived plots — the same "one field in
   `struct plot`" argument 0061's "Left" section makes. **No issue exists for
   it.** Worth filing if the owner wants it closed; I did not file one, as the
   item was documentation only.
3. `RESPONSE.md` is round-scoped correspondence and item 2 moves the durable
   parts into the guide. The derived-plot caveat and the corrected key/placement
   text are already in **both**, so item 2's crew should treat the guide's
   version as the survivor and not re-derive it.
