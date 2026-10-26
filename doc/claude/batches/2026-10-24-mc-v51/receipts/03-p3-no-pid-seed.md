# Receipt: item 3 — P3, stop seeding from the process id at load

PLAN item **3 — P3: stop seeding from the process id at load**, from
`doc/claude/batches/2026-10-24-mc-v51/PLAN.md`. The owner approved every
recommendation under *Decisions for the owner*. For this item that means
decision 6: remove the pid seeding from `initw` **and** re-seed from
`$rndseed` straight after `initw` returns.

Crew P3, 2026-10-26, branch `ver_51`. HEAD at start **`a51568215`**
(`docs: receipt for ver_51 item 2, the .option seed= warning`). The code
commit is **`5243f8e3b`** (`Seed the startup random stream from rndseed, not
the process id`). This receipt is the docs commit on top of it.

**Code was touched.** Three production files, +10/−4 lines, of which the
executable change is two deleted lines and two added calls. `com_sseed` and
`TausSeed` were not touched. The owner's uncommitted `CLAUDE.md` and
`tests/regression/pipe/resource-keyword-case.cmd` were not staged or
modified. `PLAN.md`, `LEDGER.md` and `tests/lint/identity.baseline` were not
edited. `tclspice.c` was left alone, as the plan says.

Scratch: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/p3/`.
It holds the RED capture (`red/`), `build.log`, `build2.log`, `install*.log`,
`autogen.log`, `targeted.log`, `makecheck.log`, the controls (`posctl.cir`,
`negctl7.cir`), the measurement decks (`startup.cir`, `s1.cir`, `s7.cir`,
`tn.cir`, `tn-nosetseed.cir`, `si/`, `un/`) and their captures (`before/`,
`after/`).

## What changed, file by file

| file | change |
| --- | --- |
| `src/frontend/trannoise/wallace.c` | `initw()` loses `srand((unsigned int) getpid());`, the commented `// srand(17);` and `TausSeed();`. A comment in their place says the caller seeds from `$rndseed` with `com_sseed(NULL)` before and after `initw()`. The `#define getpid _getpid` under `_MSC_VER` is now unused and was left, to keep the diff small. |
| `src/main.c` | `com_sseed(NULL);` straight after `initw();` in the `WaGauss` arm (was line 1371). |
| `src/sharedspice.c` | The same call after `initw();` (was line 1080). |
| `tests/regression/rng/startup-equals-setseed1.cir` (+ `.status` 0, empty `.err`) | The RED deck. `op`, `let const.startup = @r1[r]`, `setseed 1`, `reset`, `op`, `let const.seed1 = @r1[r]`, `quit 1` unless they are equal. Both draws are in one process, so it does not depend on libc. |
| `tests/regression/rng/setseed7-bit-identity.cir` (+ `.status` 0, empty `.err`) | The guard. `set numdgt=15`, `setseed 7`, `reset`, `op`, prints `@r1[r]`, and `quit 1` unless it is within 1e-9 ohm of `9.068279585256480e+02`. The header says it is glibc-only. |
| `tests/regression/rng/Makefile.am` | Both decks in `TESTS`, their siblings in `EXTRA_DIST`, their captures in `CLEANFILES`, and a comment block. |

`./autogen.sh` was re-run after the `Makefile.am` edit, then the tree was
rebuilt and `make install` was run. No `configure.ac` change was needed,
because item 2 already registered `tests/regression/rng/`.

Why the second `com_sseed(NULL)` is needed and not just the deletion: filling
the two Wallace pools in `initw()` draws from the uniform generator. Without
the re-seed, the startup stream would be deterministic but would equal no
`setseed` value. With it, startup equals `setseed 1`, and `$rndseed` tells
the truth. `initw()` runs after `.spiceinit` is read (`main.c:1261-1333`),
so a `setseed N` there now takes effect.

## The RED failure, verbatim

The two decks were written first and run through `tests/bin/check_status.sh`
by hand against the current binary, `600e6fb61877d47e54b4` (the item-2
binary), before any `src/` edit:

```
check_status: startup-equals-setseed1: FAILED (status)
  expected exit status 0, got 1
  invocation: /home/analog/dev/ngspice/build-ver_50/src/ngspice --batch -n /home/analog/dev/ngspice/tests/regression/rng/startup-equals-setseed1.cir
  stderr was empty
check_status: startup-equals-setseed1: FAILED: status
rc=1
  [stdout] startup-equals-setseed1: the startup draw differs from setseed 1
check_status: setseed7-bit-identity: exit status 0 as expected
rc=0
```

This is the expected reason: the deck ran to its comparison and found the
pid-seeded startup draw different from the `setseed 1` draw. The guard was
green before the change, as the plan says.

Two controls, run on the same old binary from scratch copies:

- **Positive control** (`posctl.cir`, the RED deck with `setseed 1; reset`
  before the first `op` as well): `the startup draw equals setseed 1`. The
  `const` comparison can pass, so the RED failure is the seed and not the
  harness.
- **Negative control** (`negctl7.cir`, the guard with `setseed 8`): `setseed 8
  no longer draws the 45.2 value`. The guard can fail.

## The production change

See the table. The executable diff is:

```
-    srand((unsigned int) getpid());
-    // srand(17);
-    TausSeed();
```

in `initw()`, and `com_sseed(NULL);` after each of the two `initw();` calls.

## The two `make check` counts

| run | binary | PASS | FAIL | XFAIL | XPASS | SKIP | ERROR | `make` exit |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| targeted: `tests/regression/rng` | `a6c67b795d990487275b` | 6 | 0 | 0 | 0 | 0 | 0 | 0 |
| full `make check` from `build-ver_50/` | `a6c67b795d990487275b` | **344** | 0 | 0 | 0 | 0 | 0 automake results | **0** |

344 = item 2's 342 + this item's two decks. No existing test moved, so no
committed `.out` depends on the pid-seeded startup stream. One log line
matches `^ERROR:`, `ERROR: (internal)  tried to destroy non-existent graph`.
It is the pre-existing simulator output from `vector-probe-report.cir`,
which passes.

**Lint:** `identity lint: 264 comparisons, baseline matches` and `identity
lint: 11 comparisons, baseline matches`, the same counts as the baseline. The
change adds no string comparison.

**Binary:** `sha256sum build-ver_50/src/ngspice | cut -c1-20` →
`a6c67b795d990487275b`, and `stage/bin/ngspice` has the same hash after
`make install`. (The first build after the source edit was
`f72af106e5c760f36c45`. The `autogen.sh` re-run then relinked the tree, and
every number in this receipt after the RED step is on `a6c67b795d990487275b`.)

## What was measured

All with `ngheavy build-ver_50/src/ngspice -b -n`, `set numdgt=15` where
`@r1[r]` is printed, `.param rv=agauss(1k,100,1)`.

| measurement | before (`600e6fb6…`) | after (`a6c67b79…`) |
| --- | --- | --- |
| `startup.cir`, no `setseed`, process 1 | `9.781507695338864e+02`, `rndseed=1` | `8.860341506323210e+02`, `rndseed=1` |
| `startup.cir`, process 2 | `1.103860327505454e+03`, `rndseed=1` | `8.860341506323210e+02`, `rndseed=1` |
| `s1.cir`, `setseed 1; reset; op` | — | `8.860341506323210e+02` |
| `s7.cir`, `setseed 7; reset; op` | `9.068279585256480e+02` | `9.068279585256480e+02` (`cmp` identical) |
| `tn.cir` (TRNOISE after `setseed 1`), `v(1)[10]`, process 1 / 2 | `1.042182155998e-03` / `1.728409042920e-04` | `5.016149612778e-04` / `5.016149612778e-04` |
| `tn-nosetseed.cir` (same, no `setseed`), process 1 / 2 | `-1.20333678440e-03` / `1.965267521532e-04` | `5.016149612778e-04` / `5.016149612778e-04` |

- **Two processes now agree.** The two startup captures and the two TRNOISE
  captures are byte-identical (`cmp`). The startup draw equals the
  `setseed 1` draw, `886.034…`, which is the value the scoping crew saw after
  `setseed 1`.
- **`setseed 7` is bit-identical with 45.2**: `9.068279585256480e+02`, the
  same as before this change and as the scoping crew's 45.2 measurement.
- **`.spiceinit` is honoured.** With `setseed 7` in a local `.spiceinit`
  (`si/`, run without `-n`), the startup draw is `9.068279585256480e+02` and
  `$rndseed` reads 7. Before the change the pid re-seed overrode it.
- **The escape hatch works.** With `unset rndseed` in `.spiceinit` (`un/`),
  two processes drew `9.775327195599084e+02` and `1.115581726608031e+03`,
  with `$rndseed` reading the pid (`1870424`, `1870430`). `com_sseed(NULL)`
  already falls back to `getpid()` and records it, so a run that wants to
  differ can still be recovered.
- **TRNOISE does not respond to `setseed`.** With and without `setseed 1` the
  sample is the same, because the Wallace pool was filled once at startup and
  `setseed` never refills it. That is the risk the plan already records, and
  it was there before this change.
- **The shared build** is not configured in `build-ver_50`, so the
  `sharedspice.c` call was compiled but not run.
  `gcc -fsyntax-only -DHAVE_CONFIG_H -DSHARED_MODULE -DHAVE_LIBPTHREAD
  -DTHREADS` on `sharedspice.c` returned 0, with only the existing `THREADS
  redefined` note caused by the command-line define.

## What was falsified in the plan

1. **Nothing in the scope.** The line numbers held: `wallace.c:83-85`,
   `main.c:1371`, `sharedspice.c:1080`. The change is 4 executable lines,
   inside the plan's 4-6, plus comments.
2. **"TRNOISE becomes deterministic per process"** (decision 6) holds and
   goes a little further. TRNOISE is now the same with or without `setseed`,
   because the pool is filled from the `rndseed=1` stream before any deck
   runs. That follows from the recorded no-refill risk rather than
   contradicting it.
3. **The guard deck's assertion** is a 1e-9 ohm tolerance, not a printed-text
   comparison. `check_status.sh` does not compare stdout, so the value is
   printed for a human and asserted numerically. The negative control shows
   the assertion bites.

## What was left, deliberately

- `com_sseed`, `TausSeed` and `tclspice.c` are untouched.
- The now-unused `#define getpid _getpid` in `wallace.c`'s `_MSC_VER` block.
- The Wallace pool refill on `setseed`, and the `oldseed`/`gauss0` state that
  survives `setseed` and `ngSpice_Reset` (`randnumb.c`). Both are recorded in
  the plan as out of scope.
- The cross-process check is a measurement in this receipt, not a test,
  because no in-tree driver runs a deck twice.

## For the owner

1. **Behaviour change.** Every deck that never calls `setseed` now draws the
   same values in every process: the `setseed 1` stream, TRNOISE included.
   Anyone who relied on runs differing must use `.option seed=random` or
   `unset rndseed` in `.spiceinit`. The commit message says so.
2. **`setseed N` in `.spiceinit` now works**, which xschem can use instead of
   a leading `setseed` in every deck.
3. **The shared-library path has been compiled but never run**, as in item 2.
4. **`setseed7-bit-identity.cir` is glibc-dependent.** On another libc it
   fails without anything being wrong. If the test suite is ever run on such a
   platform, the deck needs a skip, not a new value.

Nothing was pushed. Two commits on `ver_51`: `5243f8e3b` (code and tests)
and this receipt.
