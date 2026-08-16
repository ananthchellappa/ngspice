# Receipt: item 3 — 0072 criterion 5, the `-r` route

PLAN item **3 — 0072 criterion 5: the `-r` route**, from
`doc/claude/batches/2026-08-15-harness-invocations/PLAN.md`.

Crew 03, 2026-08-15, branch `ver_50`. HEAD at start `0e45a27dd`
(`test: assert 0072's two other routes into the .op guard`, item 2's commit),
verified with `git log -1`.

**No production code was touched.** One deck, three sidecars, one
`Makefile.am`, one issue file. `src/frontend/dotcards.c` was reverted *in the
working tree only*, to hold the "by not moving" half of criterion 5, and was
restored with `git checkout --`; `git diff -- src/` is empty and
`git status --porcelain src/` returns nothing. `tests/bin/check_status.sh` was
read and **not modified** — no driver change was needed. `tests/bin/check.sh`,
`doc/codex/issues/0069`, `doc/claude/upstream/` and
`doc/claude/batches/2026-08-15-exit-status-testing/upstream-0072/` were not
opened, read or written, and nothing here states or implies that any upstream
material has been sent.

Scratch: `/tmp/claude-1000/-home-qflow-dev-ngspice-test/535437bd-d68e-4ce6-af8c-934f0a7a07c3/scratchpad`.

### The binaries, by sha256

Crew 02's falsification — the build stamp is a *configure*-time constant and
does not identify a binary — is taken as binding and is independently
confirmed below. Four builds were used:

| sha256 (`build-ver_50/src/ngspice`) | Creation Date | what was measured on it |
| --- | --- | --- |
| `3759a4ee11a207b4…` | `Sun Aug 16 03:12:30 UTC 2026` | the re-measured **334** baseline; the `-r` header re-measurement |
| `c44592690b98205c…` | `Sun Aug 16 03:29:50 UTC 2026` | after the first `./autogen.sh`: RED (a), RED (b), the green run, the load-back |
| `0061c9a9ddf9614a…` | `Sun Aug 16 03:29:50 UTC 2026` | **the guard reverted to the assertion** |
| `c8c5217f0bea8e6f…` | `Sun Aug 16 03:38:54 UTC 2026` | after the second `./autogen.sh`: the final targeted and full `make check` |

Rows 2 and 3 are **different binaries with the same Creation Date**, measured
here for the second time by a second crew: reverting `dotcards.c`, recompiling
and relinking leaves the stamp untouched. Restoring the guard and rebuilding
returned the sha256 to `c44592690b98205c…` exactly, so the restore is
byte-identical and not merely functional.

## The RED failure, verbatim

The deck passes the moment it is written, so a green run proves nothing. Two
corruptions, both run through the real harness with the same
`TESTS_ENVIRONMENT` automake builds, each restored afterwards and `cmp`-checked
against a saved copy.

### (a) the header assertion corrupted — `No. Variables: 1`

`op-empty-raw.files`'s first grep changed to the value the nodeless **`.tran`**
`-r` file carries. `make -C build-ver_50/tests/regression/exitstatus check
TESTS=op-empty-raw.cir`, exit status 2:

```
  grep op-empty-raw.raw '^No\. Variables: 1$': not found; the file holds:
      Title: *
      Date: Sat Aug 15 20:31:33  2026
      Command: ngspice-46+, Build Sun Aug 16 03:29:50 UTC 2026
      Plotname: Operating Point
      Flags: real
      No. Variables: 0
      No. Points: 1       
      Variables:
      Binary:
check_status: op-empty-raw: FAILED (files)
  invocation: ../../../src/ngspice --batch -n -r op-empty-raw.raw ../../../../tests/regression/exitstatus/op-empty-raw.cir
check_status: op-empty-raw: FAILED: files
  The captured output is in op-empty-raw.stdout and op-empty-raw.stderr.
FAIL: op-empty-raw.cir
===========================================================
1 of 1 test failed
```

The driver's dump of the file's head is the whole 193-byte header, nine lines,
and it is also the re-measurement the item asked for: `Date:` is a wall clock
and `Command:` carries the build stamp, so the two greps are anchored on the
other seven lines' worth of content. The `invocation:` line shows `-r
op-empty-raw.raw` on the command line, which is the sidecar being honoured,
read off the driver rather than off the diff.

### (b) the `flags` line deleted from `.invoke`

The hard witness. Same command, exit status 2:

```
check_status: op-empty-raw: FAILED (status)
  expected exit status 0, got 1
  invocation: ../../../src/ngspice --batch -n ../../../../tests/regression/exitstatus/op-empty-raw.cir
  stderr was:
    Error: incomplete or empty netlist
           or no node to report an operating point for;
    no operating point printed!
  present op-empty-raw.raw: the run did not create it
  grep op-empty-raw.raw: no such file
  grep op-empty-raw.raw: no such file
check_status: op-empty-raw: FAILED (files)
  invocation: ../../../src/ngspice --batch -n ../../../../tests/regression/exitstatus/op-empty-raw.cir
check_status: op-empty-raw: FAILED: status files
  The captured output is in op-empty-raw.stdout and op-empty-raw.stderr.
FAIL: op-empty-raw.cir
===========================================================
1 of 1 test failed
```

**Every check this deck has moves, and the status is one of them** — see *What
I falsified*, 1. The `invocation:` line has lost its `-r`, the run takes the
plot path instead of the file path, the `dotcards.c:225` guard fires, and the
deck becomes a sixth copy of `op-empty-netlist.cir`.

**This deck's sidecar is load-bearing, and it is one of only two committed
decks here of which that is true** — verified, not repeated: four committed
decks carry an `.invoke` (`selftest-invoke.cir`, `op-empty-stdin.cir`,
`op-empty-notty.cir`, `op-empty-raw.cir`), and crew 02 measured that corrupting
the two route decks' sidecars moved the verdict in **none of five cases**. So
before this item, `selftest-invoke.cir` was the only committed witness that
`check_status.sh` reads `.invoke` at all. There are now two, and this one
witnesses the `flags` key on a deck whose subject is the simulator rather than
the driver.

### Restored

`cmp` clean against saved copies of both sidecars, then:

```
check_status: op-empty-raw: exit status 0 as expected
PASS: op-empty-raw.cir
```

## The production change

No file under `src/`. Five files.

### `tests/regression/exitstatus/op-empty-raw.cir`

`printf '*\n.op\n'` — six bytes, sha256 `899d8397112b0288…`, **identical to
`op-empty-netlist.cir`, `op-empty-stdin.cir` and `op-empty-notty.cir`**, which
is 0072's reproducer byte for byte. Like them it carries no comment block: the
deck *is* the reproducer. The commentary is in the three sidecars and in
`Makefile.am`.

### `op-empty-raw.invoke`

One key, `flags -r op-empty-raw.raw`, and **no `route` line** — the route is
the default `batch`, so the only difference from `op-empty-netlist.cir` is the
flag, which is the whole subject. (Crew 01 measured that `flags` alone is
"batch plus flags"; confirmed here by the `invocation:` lines above.) Its
comment block names `outitf.c:494`-`:497`, says why the status is 0, and states
that unlike the two route decks this one *does* witness its own sidecar.

### `op-empty-raw.status`

`0`, above a comment block that spends fifteen lines saying why zero is the
right answer here rather than a bug — because a reader who finds one `0` among
five `1`s in a directory full of `op-empty-*` decks will otherwise "fix" it.
The chain: `-r` sets `rflag`, `OUTpBeginPlot()` takes its `writeOut` arm and
calls `fileInit()` instead of `plotInit()` (`outitf.c:494`-`:497`, verified in
this branch), `plot_list` gains nothing, `setcplot("op")` answers NULL, and the
body holding the `dotcards.c:225` guard is never entered.

### `op-empty-raw.files`

Three lines, each with its reason above it:

```
present op-empty-raw.raw
grep op-empty-raw.raw ^No\. Variables: 0$
grep op-empty-raw.raw ^Plotname: Operating Point$
```

The driver's `grep` verb passes its text to `grep -q -e` unchanged, so the text
is a **BRE** — the driver's header comment says so and it is correct. The `.`
is therefore escaped, and both patterns are anchored. Measured rather than
assumed, through the driver's own `read -r verb path rest` loop:

```
verb=[grep] path=[zero.raw] rest=[^No\. Variables: 0$] -> MATCH
verb=[grep] path=[zero.raw] rest=[^Plotname: Operating Point$] -> MATCH
--- negative controls (must be NO MATCH):
'^No\. Variables: 1$' on the .op file          NO MATCH
'^No\. Variables: 0$' on a file holding 'Nox Variables: 0'   NO MATCH
'^No. Variables: 0$'  on the same file         MATCH: unescaped dot matches Nox
```

The last line is why the escape is there and not decoration.

### `tests/regression/exitstatus/Makefile.am`

`op-empty-raw.cir` into `TESTS`; its three sidecars into `EXTRA_DIST`; its four
capture files **and `op-empty-raw.raw`** into `CLEANFILES`. Two comment blocks
extended: the header block gains two paragraphs saying what this deck is for,
why its expected status is 0, and that it is one of the only two committed
decks here whose `.invoke` is load-bearing; the `CLEANFILES` comment gains the
rawfile and is corrected on the `core` count — see *What I falsified*, 2.

`./autogen.sh` from the repo root, then `make -C build-ver_50 -j8` **before**
any `make check`, per crew 01's measurement that `config.status` rewrites
`config.h` and the binary otherwise relinks mid-suite. Done twice, because a
second round of comment edits followed the first measurement pass.
`Makefile.in` is gitignored, so nothing generated is committed. No `git add -f`
was needed.

`tests/lint/identity.baseline` is **unchanged** — this item adds no C.
`git status --porcelain tests/lint/` is empty; both lint specs report
`baseline matches`, 264 and 11 comparisons.

### `doc/codex/issues/0072`

Two edits, and nothing else in the file. Row 1, the fix-commit hash, the
`assert.h` note and `doc/codex/issues/0069` were left for later crews, as the
item required.

1. **Criteria table row 5.** `met, by not moving; measured, not asserted by a
   deck` → `met, by not moving; asserted by a deck for the status and the
   header, with the load-back a recorded measurement`. The cell's opening
   claim — that the driver's command line is `--batch -n <deck>` with no `-r`
   and no per-deck flag file — is gone, because crew 02 falsified the same
   sentence where it appeared in row 1 and in the Status paragraph, and it was
   the last copy of it. The cell now names the deck, its three sidecars and the
   two anchored header patterns; records that the "by not moving" half is held
   by the guard-revert run below rather than by a note; and names the load-back
   as a **recorded measurement**, deliberately, because the owner decided
   against a load-back concept in the driver.
2. **The Status paragraph**, which row 5 forces: it said *"All nine Acceptance
   Criteria are met. One route within them still rests on a recorded
   measurement rather than on a committed deck: criterion 5's `-r` route."*
   That is now false. It says instead that every route named in the criteria is
   asserted by a committed deck, that three of them were measurements until
   2026-08-15 for one shared reason that stopped being true at `38717a031`, and
   that what remains a measurement inside criterion 5 is the narrower
   load-back.

## The two `make check` counts

| run | result |
| --- | --- |
| `make -C build-ver_50/tests/regression/exitstatus check` | **13 PASS / 0 FAIL** — "All 13 tests passed", `make` exit status 0 |
| `make check` from `build-ver_50` | **335 PASS / 0 FAIL**, `make` exit status 0 |

Baseline **re-measured, not carried**: at `0e45a27dd`, before any file of this
item existed, `make check` from `build-ver_50` was **334 PASS / 0 FAIL**,
`make` exit status 0. So 335 is 334 plus exactly the one new deck, and no
existing test moved. The item's stated 334 is confirmed.

## What was measured

### 1. The `-r` header, re-measured before the greps were written

Receipt 03 of the previous batch recorded 193 bytes. Re-measured on
`3759a4ee11a207b4…`: **193 bytes, nine lines, no binary payload at all** — the
nine header lines sum to exactly 193, so `Binary:` is followed by nothing,
which is what `No. Variables: 0` and `No. Points: 1` imply.

```
Title: *$
Date: Sat Aug 15 20:25:30  2026$
Command: ngspice-46+, Build Sun Aug 16 03:12:30 UTC 2026$
Plotname: Operating Point$
Flags: real$
No. Variables: 0$
No. Points: 1       $
Variables:$
Binary:$
```

(`cat -A`; the trailing spaces on `No. Points:` are real, which is a third
reason not to anchor a grep there.) Two lines are unfit to assert on — `Date:`
is a wall clock, `Command:` carries the configure-time build stamp — and the
two chosen are neither.

### 2. The guard genuinely does not move this row, held by the deck

The item left this to my judgement. It was done, because it is what criterion
5's *"by not moving"* actually claims and it is cheap. `dotcards.c`'s guard was
reverted to `assert(plot_cur->pl_dvecs != NULL)` in the working tree —
`git diff` reads `index f10a17686..2d2c8879e`, the exact reverse of the fix
commit's own hunk, so the file is byte-identical to the pre-fix file — and the
binary rebuilt (`0061c9a9ddf9614a…`). The **whole directory**, one run:

```
PASS: tran-empty-netlist.cir
PASS: selftest-status.cir
PASS: selftest-invoke.cir
PASS: sim-status-guard.cir
PASS: sim-status-guard-ok.cir
PASS: sim-status-properties.cir
PASS: sim-status-unset.cir
FAIL: op-empty-netlist.cir
FAIL: op-empty-tran.cir
FAIL: op-empty-control.cir
FAIL: op-empty-stdin.cir
FAIL: op-empty-notty.cir
PASS: op-empty-raw.cir
5 of 13 tests failed
```

Five of the six `op-empty-*` decks fail; **this one passes**. The rawfile it
wrote under the assertion build is 193 bytes and identical to the one the
restored build writes apart from its `Date:` line — the `Command:` line is
identical too, because the stamp did not move across the revert.

That passing row is also the *direct* proof of the mechanism the `.status`
comment describes. Had `setcplot("op")` returned a non-NULL empty plot on this
path, the assertion would have fired; it did not, so `plot_list` really is
empty under `-r`. The row does not move, and it is now a deck that says so.

### 3. `-r` puts this deck on the other `ft_cktcoms()` call site

Not in the item's text, and worth having. `ft_cktcoms()` has exactly two
callers. Under `-r`, `main.c:1561`'s `if (rflag)` arm is taken — it is the
*first* arm of the batch epilogue and it runs whatever the deck contains — and
it calls `ft_cktcoms(**TRUE**)` at `main.c:1573`. Every other `op-empty-*` deck
reaches `ft_cktcoms(FALSE)` at `main.c:1581`. So the function is entered on
this route; what does not happen is the `if (plot_cur != NULL)` body. The three
comment blocks say it that way rather than the looser "never called".

### 4. The load-back, re-measured on the file the committed deck writes

Named in the issue as a recorded measurement, and re-measured here rather than
carried from receipt 03 — on `build-ver_50/tests/regression/exitstatus/op-empty-raw.raw`,
the artefact the committed deck itself produces:

```
$ printf 'load zero2.raw\ndisplay\nquit 0\n' | ngspice -p -n
Loading raw data file ("zero2.raw") ...
done.
Title:  *
Name: Operating Point
Date: Sat Aug 15 20:34:39  2026

There are no vectors currently active.
ngspice 10002 -> display
There are no vectors currently active.
rc=0
```

Criterion 5's "a valid raw file or no file at all, not a third thing" is
therefore still answered, by measurement. Asserting it would need a second
invocation in the driver, which PLAN decision 3 rules out.

### 5. The `No. Variables: 1` used in RED (a) is real

```
$ printf '*\n.tran 1n 10n\n' > tinytran.cir
$ ngspice --batch -n -r t.raw tinytran.cir      rc=0, 681 bytes
Plotname: Transient Analysis
No. Variables: 1
```

681 bytes and `No. Variables: 1`, matching receipt 03's table 4. So the
corrupted assertion in RED (a) is not an arbitrary wrong number: it is the
value the *neighbouring* row of criterion 5 carries, which is the mistake a
future editor is most likely to make.

### 6. `probe2.evidence` — a stray from a hand demonstration. Deleted.

Crew 02 reported it as an item-1 leftover. Establishing which it was, three
ways:

- Its content is `batchmode=0 inputdir=/tmp/…/scratchpad` — the `inputdir` it
  recorded is **crew 01's scratchpad**, so the deck that wrote it never lived
  in the repository.
- Its mtime, `Aug 15 19:47`, did not move across the 334-pass baseline run, two
  full directory runs and the full 335-pass run, while every other artefact in
  that directory did.
- `grep -rn probe2 tests/` finds only vector names in
  `tests/regression/pipe/unset-rawfile-option-key.cmd`, an unrelated deck in
  another directory driven by another harness.

Then the direct test: deleted, and the whole directory re-run. It did not come
back, and the directory now holds only `Makefile` plus the four artefacts
`CLEANFILES` names. **No repository change is owed**; it does not belong in
`CLEANFILES`, because nothing that `make` runs creates it.

## What I falsified

### 1. RED (b) moves the deck's *status*, not just its `present` line — and that makes this deck strictly stronger than the item predicted

The item says: *"(b) Delete the `flags` line from `.invoke` and quote the
failure: no rawfile written, so `present` fails."* Measured: **four** checks
move, and the first of them is the status, `expected exit status 0, got 1`,
with the three-line diagnostic captured beside it. Without `-r` the run stops
being a criterion-5 run at all; it takes the plot path, hits the guard and
returns 1.

That is not a quibble about how many lines the driver printed. It means
`op-empty-raw.cir` is **the first committed deck in this directory whose
declared exit status is only correct if its `.invoke` was honoured**.
Everything before it either declared a status that was route-independent
(crew 02's two decks: five corruptions, verdict unmoved in all five) or was a
self-test whose subject is the driver. A harness that silently stopped reading
`.invoke` would be caught here by the `.status` line alone, before any `.files`
assertion is consulted.

### 2. My own `Makefile.am` edit, caught and corrected before commit

The first version extended the existing `CLEANFILES` comment from *"'core' and
'core.*' are here for the five op-empty-* decks"* to *"the six"*. That is
wrong, and wrong in the direction this whole deck exists to prevent: the
comment's reason is *"the state they assert used to abort"*, and
`op-empty-raw.cir`'s state is the one that **never** aborted. It now reads
"five of the six op-empty-* decks — every one but `op-empty-raw.cir`, whose
whole subject is that its route never aborted". Recorded because an inherited
count updated by reflex is exactly how a comment stops being true.

### 3. A claim I wrote into the sidecars and then measured, which was false

The first draft of the three comment blocks said this deck is *"the only deck
in this directory that reaches `ft_cktcoms()` from `main.c:1573`"*. False:
`selftest-invoke.cir` carries `flags -r selftest-invoke.raw` too and takes the
same arm, because `if (rflag)` at `main.c:1561` is the first arm of the batch
epilogue and does not care what else is in the deck. Corrected to name it, with
the distinction that matters — that deck's subject is the driver, this one's is
the simulator.

### 4. Crew 02's stamp finding, confirmed independently

Not a falsification of theirs but of the standing rule they falsified. Two
different binaries in this item — `c44592690b98205c…` with the guard and
`0061c9a9ddf9614a…` with the assertion — carry the **same** Creation Date,
`Sun Aug 16 03:29:50 UTC 2026`. Reverting a `src/` file, recompiling and
relinking does not move it. Every number here is tied to a sha256.

### 5. Everything else in item 3 checked out

`outitf.c:494`-`:497` is right in this branch: `:494` `if (run->writeOut) {`,
`:495` `fileInit(run);`, `:496` `} else {`, `:497` `plotInit(run);`. The `grep`
verb is a BRE, as the driver's header says. Receipt 03's 193 bytes, its `Date:`
and `Command:` warning, its load-back and its 681-byte `No. Variables: 1`
`.tran` file all re-measured to the same values. The 334 baseline re-measured
to 334.

## What was left, deliberately

- **No `.err` sibling for this deck.** `stderr` is empty on this route (0
  bytes, measured), and an empty `.err` would assert only that the diagnostic
  is absent — which the `.status` of 0 already implies, since the guard cannot
  print without returning 1. Named here because a later crew may reasonably
  want it; it would be one empty file and one `EXTRA_DIST` line.
- **No `absent rawspice.raw` line.** Under `-r` the output goes to the named
  file, and `op-empty-netlist.files` already asserts that these six bytes leave
  no default rawfile on the plot path. A second copy would assert nothing new.
- **No load-back and no second-invocation concept**, per PLAN decision 3 and
  the owner's explicit ruling. Measured in *What was measured*, 4, and named in
  the issue as a measurement.
- **`tests/bin/check_status.sh` untouched.** No driver change was needed; the
  `flags` key and the `grep`/`present` verbs did everything this item asked
  for.
- **Row 1, the fix-commit hash, the `assert.h` note, `doc/codex/issues/0069`,
  and 0072's stale suite counts** (331 in the Status paragraph, `328 → 331` in
  row 8) — items 4 and 5, and crew 02's *For the owner*, 5.
- **The 3 shapes × 3 routes cross product and a `-r` × shapes product.** The
  `-r` route is now asserted once, on the reproducer's shape, for the same
  reason item 2 gave: the dimension that was unasserted is the invocation, not
  the deck.

## For the owner

1. **This directory now has exactly two decks that can catch a `check_status.sh`
   which stopped reading `.invoke`**, and they catch different keys:
   `selftest-invoke.cir` catches `route` (via `$?batchmode` in an artefact) and
   `op-empty-raw.cir` catches `flags` (via its status and its rawfile). The two
   route decks catch neither, measured by crew 02. If either of these two is
   ever weakened, the `.invoke` mechanism goes unwitnessed for its remaining
   key while five decks keep passing.
2. **`op-empty-raw.cir` is the only deck under `tests/regression/exitstatus/`
   whose expected status is 0 among six `op-empty-*` decks, and the only one
   whose subject is that nothing happens.** Its `.status` comment is long on
   purpose. If the directory ever grows a convention that `op-empty-*` means
   "exits 1", this deck breaks it deliberately.
3. **`build-ver_50/tests/regression/exitstatus/probe2.evidence` is resolved**:
   a hand-demonstration stray, deleted, no `CLEANFILES` entry owed. Verdict and
   evidence in *What was measured*, 6.
4. **The load-back stays a measurement, in two places that must not drift
   apart** — 0072's row 5 and this receipt. If a second-invocation concept is
   ever added to the driver, that is the sentence to come back to; today both
   say the same thing and both name the owner's decision.

Nothing was pushed. One commit on `ver_50`, carrying the deck, its three
sidecars, the `Makefile.am`, the two 0072 edits and this receipt.
