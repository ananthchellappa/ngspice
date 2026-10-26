# Receipt: follow-up 4 — P4 fixes (review issues 4a–4f, gap B10)

Follow-up crew **"P4 fixes"** for the ver_51 MC batch. The issues are item 4's
open review issues in `doc/claude/batches/2026-10-24-mc-v51/receipts/06-close.md`
section C (4a–4f), and gap B10 in section B (the unbuilt `visualc/*.vcxproj`
edits). The item's own receipt is
`doc/claude/batches/2026-10-24-mc-v51/receipts/04-p4-rnddraws-log.md`. The spec
is that batch's `PLAN.md`, item 4, which the owner approved.

Crew P4-fixes, 2026-10-26, branch `ver_51`. HEAD at start **`0dc054bb5`**
(`docs: receipt for ver_51 follow-up 3, the P3 fixes`). The test commit is
**`efb8a7528`** (`Test the startup, checkseed and option draw-log headers`).
This receipt, with a one-paragraph correction to receipt 04 (issue 4b), is the
docs commit on top of it.

**No production code was touched.** Nothing under `src/` or `visualc/` was
edited. The test commit adds two decks to `tests/regression/rng/` and their
`Makefile.am` lines. The log contract (field order, header, footer, the meaning
of each field) is unchanged; no fix needed a contract change. The owner's
uncommitted `CLAUDE.md` and `tests/regression/pipe/resource-keyword-case.cmd`
were not staged or modified. No `LEDGER.md` or `PLAN.md` was edited, and
neither was `tests/lint/identity.baseline`. `build-ver_51-shared/` was not
touched, built in or deleted (it is not on disk: see *For the owner* 1).
Nothing was pushed or sent anywhere. Every heavy command went through
`ngheavy`, one at a time, with `-j4` at most.

Scratch: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/f4/`.
It holds `neg/` and `neg.log` (the negative controls), `autogen.log`,
`build.log`, `install.log`, `rng-check.log` and `check.log`.

## Issues taken

| issue | outcome | what |
| --- | --- | --- |
| B10 | checked, no edit needed | `drawlog.c` and `drawlog.h` are the only sources the batch added, and all three `.vcxproj` files list both, next to `randnumb.c`/`randnumb.h`, in the neighbours' form. No `.filters` file exists. Still not built. |
| 4a | left | Same as A1, the shared build tree. The owner is deciding. |
| 4b | fixed (docs) | Receipt 04's opening paragraph now credits "`value` is the raw draw" to the LEDGER note of 2026-10-24, not decision 7. |
| 4c | fixed (tests) | Two new decks assert `src=startup`, `src=checkseed offset=1` and `src=option` in tree. |
| 4d | declined | Speculative, can't be tested here, and no shipped configuration is affected. Reason below. |
| 4e | declined | The counter is no more racy than the generator state it counts. Reason below. |
| 4f | left | The probe runs only against a `--with-ngshared` build. |

## B10 — the Visual Studio project files

The batch's commits (`git diff --name-status a664faa2d..HEAD -- src`) add two
source files: `src/maths/misc/drawlog.c` and `src/include/ngspice/drawlog.h`.
Every other `src/` entry is `M`. Tests and docs don't belong in a vcxproj.

Each project file got the same two lines in `29940ed39`, right after the
`randnumb` neighbour in the same item group, with the same attributes (none)
and CRLF line endings:

```
visualc/sharedspice.vcxproj    816 randnumb.h  817 drawlog.h  1416 randnumb.c  1417 drawlog.c
visualc/vngspice-fftw.vcxproj 1065 randnumb.h 1066 drawlog.h  1669 randnumb.c  1670 drawlog.c
visualc/vngspice.vcxproj      1080 randnumb.h 1081 drawlog.h  1684 randnumb.c  1685 drawlog.c
```

- The `+` lines of the three diffs, with `\r` stripped, have the same md5
  (`1d342ae36cd209f13e4029835940bcd2`), so the three edits are identical.
- All three files are still well-formed XML (Python's `xml.dom.minidom`).
- `git ls-files | grep filters$` is empty, so there is no `.filters` file to
  update. No other project file in the tree (`*.vcxproj*`, `*.vcproj`,
  `*.mak`, CMake) names `randnumb`, so nothing else lists that directory's
  sources.
- `drawlog.c` uses only C89 stdio, `strerror`/`errno` and ngspice helpers.
  It has no POSIX header that MSVC lacks.

So no edit was needed. It is still not built with MSVC, because that can't
be done here.

## 4b — the source of "value is the raw draw"

Receipt 04's opening paragraph listed "`value` is the raw draw" among the
points of decision 7. Decision 7 (`PLAN.md:79-93`) doesn't mention `value`,
and the item spec asked for the `%g`-rounded value. The raw draw comes from
xschem's acceptance in `LEDGER.md` (2026-10-24: "`value` as the raw draw").
The paragraph is corrected in place, with a note saying this follow-up
corrected it. Nothing else in receipt 04 changes. Its falsified item 3 already
named the right source. No source comment or test cites decision 7 for this
(`grep -rn -i 'decision 7' src tests` finds only an unrelated reference to
decision 0008).

## 4c — the other three header sources, in tree

`rnddraws-mm.cir` asserts only `src=setseed`. The other three values had been
checked only by hand, or only by the probe and the fixture.

- **`rnddraws-src.cir`**: `pre_set rnddraws=` logs the initial load, then
  `set rndseed=11`, `let u = sunif(1)` (one control-language draw, so
  `checkseed()` re-seeds from 11 and makes one uniform call), then `reset`.
  Its `.files` require the headers `seed=1 src=startup offset=0` and
  `seed=11 src=checkseed offset=1`, a row for each set, and the footer.
- **`rnddraws-option.cir`**: `.option seed=5`, `pre_set rnddraws=`, `reset`.
  Its `.files` require `seed=5 src=option offset=0`, the row and the footer.
  It has no `setseed`, so the P2 warning doesn't print and `.err` stays
  empty.

The header line is anchored at both ends (`^…$`). The row values depend on the
libc's `rand()` and aren't frozen, so the decks run on any libc and need no
`.requires`.

### RED evidence

This is new coverage of behaviour that already ships, so there is no
production change to go RED against. The new decks pass on the current binary
the first time. To show they can fail, each was run with its expected source
changed in a copy of its `.files` (`neg/spec/`). Trimmed, from `neg.log`:

```
  grep rnddraws-src.tsv '^#	rnddraws	v1	seed=11	src=setseed	offset=1$': not found; the file holds:
      #	rnddraws	v1	seed=1	src=startup	offset=0
      ...
      #	rnddraws	v1	seed=11	src=checkseed	offset=1
check_status: rnddraws-src: FAILED: files
rc=1
  grep rnddraws-option.tsv '^#	rnddraws	v1	seed=5	src=startup	offset=0$': not found; the file holds:
      #	rnddraws	v1	seed=5	src=option	offset=0
check_status: rnddraws-option: FAILED: files
rc=1
```

On the real specs, both return 0. The logs they wrote (the gaps are tabs):

```
#	rnddraws	v1	seed=1	src=startup	offset=0
1	r.xm1.r1	r	agauss	0,1,1	-1.13965849367679	-1.13965849367679
#	end	logged=1	unlogged_calls=0
#	rnddraws	v1	seed=11	src=checkseed	offset=1
11	r.xm1.r1	r	agauss	0,1,1	0.41095749122115915	0.41095749122115915
#	end	logged=1	unlogged_calls=0
```

```
#	rnddraws	v1	seed=5	src=option	offset=0
5	r.xm1.r1	r	agauss	0,1,1	-0.68786644081549131	-0.68786644081549131
#	end	logged=1	unlogged_calls=0
(the reset's set is the same)
```

These match the hand checks in the review (`seed=5 src=option offset=0`,
`seed=11 src=checkseed offset=1`).

## 4d — x87 rounding of the split draw: declined

The review calls this speculative. That still holds, and it can't be tested
here.

- On x86-64, under either ABI (SysV or Windows x64), `gauss1()` returns its
  double in `xmm0`, already rounded to double. So `z = gauss1(); r = nominal
  + stdvar*z` and the old `nominal + stdvar*gauss1()` compute the same thing.
  `rnddraws-gold.cir` asserts this bit for bit with the log on and off.
- The difference could only appear on the i386 ABI, where the result comes
  back in `st(0)` with excess precision, and the compiler may or may not spill
  `z`. The host has no 32-bit libc (`gcc -m32` fails: `cannot find Scrt1.o`),
  so this can't be measured.
- On that target, the old code wasn't reproducible either. Under GCC's default
  `-fexcess-precision=fast`, whether a value stays in an 80-bit register
  depends on optimisation level and register pressure. "Bit-identical with
  45.2" was never a property of an x87 build.
- Forcing a spill (`volatile`) or a cast wouldn't restore the old rounding,
  because the old rounding was never defined. It would only add a new, equally
  arbitrary one.

## 4e — the unsynchronised `rng_calls++`: declined

`rng_calls++` sits in `CombLCGTaus()`, next to the generator's state, which is
also static and also unsynchronised. A background transient that races with a
parse already corrupts that state and the draw sequence. Making only the
counter atomic would leave the draws just as wrong, and an exact count of a
corrupted sequence is no use. The tree has no `_Atomic`/`stdatomic.h` use
(`grep -rn` finds none), and MSVC's C11 atomics support is recent. The real
fix would be a thread-safe generator. That changes upstream behaviour beyond
this batch's scope. The counter is read only inside a parse window, as the
review says.

## 4f — the probe's `strncmp(..., 14)`: left

The probe, `doc/claude/batches/2026-10-24-mc-v51/probes/rnddraws_reset_probe.c`,
links against `libngspice` and has only ever run on `build-ver_51-shared/`.
Tightening the compare would be a one-character change. But a probe change
that has never been compiled or run would be an unverified test change, and
the standing rules exclude fixes that need the shared tree. The fix needed:
compare 15 characters, so the trailing tab after `logged=1` is included.

## The two `make check` counts

`./autogen.sh` was re-run for the `Makefile.am` edit. Then `make -j4` (which
re-ran `config.status --recheck` and rebuilt the tree), `make install`, the
targeted directory and the full suite. All were run from `build-ver_50/`
through `ngheavy`, one at a time.

| run | binary | PASS | FAIL | XFAIL | XPASS | SKIP | ERROR | `make` exit |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| targeted: `tests/regression/rng` | `c1341e751ffd39377859` | 14 | 0 | 0 | 0 | 0 | 0 | 0 |
| full `make check` from `build-ver_50/` | `c1341e751ffd39377859` | **353** | 0 | 0 | 0 | 0 | 0 automake results | **0** |

353 is follow-up 3's 351 plus the two new decks. One log line starts with
`ERROR:`. It reads `ERROR: (internal)  tried to destroy non-existent graph`,
the known simulator output of `vector-probe-report.cir`, which passes.

**Lint**, both lines from the log:

```
identity lint: 264 comparisons, baseline matches
identity lint: 11 comparisons, baseline matches
```

**Binary:** `sha256sum build-ver_50/src/ngspice | cut -c1-20` →
`c1341e751ffd39377859`. `cmp` against `stage/bin/ngspice` after `make install`
reports them identical. The hash moved from follow-up 3's
`e6a151ac9331fb943d20` with no source change. The configure recheck re-stamps
`NGSPICEBUILDDATE` (`configure.ac:1024`, into `Spice_Build_Date` in
`src/conf.c`), and the binary now holds `Mon Oct 26 16:33:01 UTC 2026`.

## For the owner

1. **`build-ver_51-shared/` (A1 / 4a).** This task said you are still
   deciding, but the tree is not on disk, and commit `5d88401af`
   (`docs: record removal of the shared-library build tree`) records in
   `LEDGER.md` that you said delete and the driver deleted it. If that stands,
   A1/4a is closed and needs nothing more. What it means either way:
   - **Keep a shared build (recreate it):** the `libngspice` paths of items
     2–4 can be run again: `rnd_reset_state()` in `totalreset`, the
     `ngSpice_Init` re-seed and the draw-log reset order. 4f and 3c/2a could
     then be fixed and verified. The cost is a second configured tree and its
     build time and memory on this box. The plan's "no second configured
     tree" rule would need an explicit exception.
   - **Stay without one (as now):** the box is simpler and the rule holds.
     But nothing exercises those `libngspice` paths. A regression in them
     would show up only in a shared-library user such as xschem's loop. The
     probe source stays in `probes/` for whoever builds one later.
2. **4f is a one-character probe change** (`14` → `15` in the `logged=1`
   compare). It is left until a shared build exists to run it.
3. **4d and 4e are declined** for the reasons above. Neither affects a shipped
   x86-64 build or a single-threaded run.
4. **B10 is checked, not built.** An MSVC build on Windows is the only way to
   close it fully.

Nothing was pushed. Two commits on `ver_51`: `efb8a7528` (tests) and the docs
commit with this receipt and the receipt-04 correction.
