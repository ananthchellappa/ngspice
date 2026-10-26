# Receipt: follow-up 3 — P3 fixes (review issues 3a–3d, gap B9)

Follow-up crew **"P3 fixes"** for the ver_51 MC batch. The issues are item 3's
open review issues in `doc/claude/batches/2026-10-24-mc-v51/receipts/06-close.md`
section C (3a–3d), and the glibc gap in section B (9), which is the same issue
as 3d. The item's own receipt is
`doc/claude/batches/2026-10-24-mc-v51/receipts/03-p3-no-pid-seed.md`. The spec
is that batch's `PLAN.md`, item 3, which the owner approved.

Crew P3-fixes, 2026-10-26, branch `ver_51`. HEAD at start **`3db79a2e3`**
(`docs: receipt for ver_51 follow-up 2, the P2 fixes`). The code commit is
**`73733be3f`** (`Skip the glibc-only rng decks on another libc`). This receipt
is the docs commit on top of it.

**Code was touched.** One production file, `src/frontend/trannoise/wallace.c`
(comment and dead includes only, no executable change), the test driver
`tests/bin/check_status.sh`, and `tests/regression/rng/`. The owner's
uncommitted `CLAUDE.md` and `tests/regression/pipe/resource-keyword-case.cmd`
were not staged or modified. No `LEDGER.md` or `PLAN.md` was edited, and
neither was `tests/lint/identity.baseline`. `build-ver_51-shared/` was not
touched, built in or deleted. Nothing was pushed or sent anywhere. Every heavy
command went through `ngheavy`, one at a time, with `-j4` at most.

Scratch: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/f3/`.
It holds `ngspice-old` (the pre-change binary, `dbb8e37dc9492eeb7d30`), the
non-glibc stand-in `muslrand.c`/`muslrand.so`, `red.log`, the forced driver
`check_status-forced.sh`, `skip.log`, `automake-skip.log`, `neg.log`,
`autogen.log`, `build.log`, `install.log`, `rng-check.log` and `check.log`.

## Issues taken

| issue | outcome | what |
| --- | --- | --- |
| 3a | fixed (comment) | `initw()`'s comment now names main.c and sharedspice.c as the callers that seed, and says the `HasMain` test `main()` does not. |
| 3b | fixed | The unused `#define getpid _getpid`, `<process.h>` and `<unistd.h>` are gone from `wallace.c`. |
| 3c | left, as for 2a | Running the `ngSpice_Init` re-seed needs a shared build. See *3c* and *For the owner* 1. |
| 3d / B9 | fixed, RED first | `check_status.sh` reads an optional `<base>.requires`. `libc glibc` makes the deck SKIP (exit 77) on another libc. The glibc guard of `rnddraws-funcs.cir` moves, unchanged, to a new deck `rnddraws-gold.cir`. |

## 3d / B9 — the glibc-only decks

### How a skip is reported

The tree had no skip convention. `grep -rn 'exit 77\|SKIP' tests/bin tests/*/Makefile.am`
finds nothing, and no driver in `tests/bin/` exits 77. Automake's serial test
harness (the tree uses `serial-tests`) counts exit 77 as SKIP, so that is what
was used. It is added to `check_status.sh` with the driver's own sibling-file
pattern (`.status`, `.invoke`, `.err`, `.files`, `.mustfail`):

```
#   <base>.requires  optional.  What the platform must provide for the
#                    deck's assertions to mean anything.  One key, at most one
#                    line; '#' starts a comment and blank lines are ignored:
#                        libc   glibc
```

- The check runs after the spec is validated and **before** any file is
  deleted or the simulator runs, so a skipped deck leaves the build directory
  as it found it.
- It is met when `getconf GNU_LIBC_VERSION` prints `glibc …`. That is the libc
  of the host, which is the libc a natively built `ngspice` links against.
  On musl, BSD, macOS and MSYS2/MinGW `getconf` has no such variable, prints
  nothing, and the deck is skipped.
- When it is not met, the driver prints
  `check_status: <base>: SKIP: the deck needs glibc, and 'getconf GNU_LIBC_VERSION' says '<x>'`
  and exits 77.
- An unknown key, a second `libc` line or a value other than `glibc` is a
  broken test spec (exit 1), like a bad `.invoke`. The line is split with
  `set -f`, as `.invoke` is.

### The decks

- **`setseed7-bit-identity.cir`** gets `setseed7-bit-identity.requires`
  (`libc glibc`). The deck is unchanged apart from its header comment, which
  now says it is skipped rather than "fails without anything being wrong".
- **`rnddraws-funcs.cir`** had two guards. One is portable: the log-on pass
  draws exactly what the log-off pass drew. The other is not: the log-off
  draws equal the pre-log glibc values. Skipping the whole deck would have
  thrown away the portable guard and the row checks on every non-glibc
  platform. So the glibc guard moved to a new deck, **`rnddraws-gold.cir`**,
  with `.requires`. It uses the same circuit, the same `setseed 3` pass, the
  same nine gold values and the same 1e-9 tolerance, copied line for line.
- One `.files` row in `rnddraws-funcs.files` also depended on libc:
  `limit(1k,100)`'s row was required to end in `1100`. `limit()` gives 900 or
  1100 depending on the draw, so on another libc it fails half the time. That
  exact check moved to `rnddraws-gold.files`, under glibc, unchanged. In
  `rnddraws-funcs.files` the row now ends in `9*1*00$`, which accepts `900`
  and `1100` and refuses `1000`, `110` and the like (checked with `grep -e`).
  So **on glibc no assertion is weaker**: every check that was there still
  runs, in one deck or the other. On another libc the portable ones now run
  instead of failing.

`Makefile.am` gained `rnddraws-gold.cir` in `TESTS`, the new siblings in
`EXTRA_DIST`, the captures and `rnddraws-gold.tsv` in `CLEANFILES`, and a
note on `.requires` in the header comment.

### The RED failure

There is no non-glibc machine here. To stand in for one, `muslrand.so`
re-implements `srand()`/`rand()` the way musl does
(`seed = 6364136223846793005*seed + 1; return seed >> 33`). It is loaded with
`LD_PRELOAD` in front of the unchanged binary `dbb8e37dc9492eeb7d30` and the
unchanged driver, before any edit. Trimmed:

```
check_status: setseed7-bit-identity: FAILED (status)
  expected exit status 0, got 1
check_status: setseed7-bit-identity: FAILED: status
rc=1
setseed7-bit-identity: setseed 7 no longer draws the 45.2 value
check_status: rnddraws-funcs: FAILED (status)
  expected exit status 0, got 1
check_status: rnddraws-funcs: FAILED: status
rc=1
rnddraws-funcs: max change from the old draws 173.338 log on vs off 0
rnddraws-funcs: the log-off draws are not the draws from before the log
```

These are the failures B9 describes: a different `rand()` and the decks FAIL,
though nothing in ngspice is wrong. `log on vs off 0` shows that the portable
guard was still passing inside the failing deck.

### The skip path, triggered

`check_status-forced.sh` is a scratch copy of the new driver with one line
changed, the probe:

```
<                 libc=`getconf GNU_LIBC_VERSION 2>/dev/null`
>                 libc=`echo musl 1.2.5`  # forced in this scratch copy
```

Run through automake with `LD_PRELOAD=muslrand.so`, so both the libc check
and `rand()` say "not glibc" (`automake-skip.log`):

```
PASS: startup-equals-setseed1.cir
SKIP: setseed7-bit-identity.cir
PASS: rnddraws-mm.cir
PASS: rnddraws-order.cir
PASS: rnddraws-funcs.cir
SKIP: rnddraws-gold.cir
======================
All 10 tests passed
(2 tests were not run)
```

The six `option-seed-*` decks pass as well. The driver's own words for the
two skips:

```
check_status: setseed7-bit-identity: SKIP: the deck needs glibc, and
  'getconf GNU_LIBC_VERSION' says 'musl 1.2.5'
```

So under a non-glibc `rand()` the directory is 10 PASS and 2 SKIP, where it
was 2 FAIL before. `rnddraws-funcs.cir` now passes there: under the musl
stand-in its `limit` row logged `1100`, and the new pattern takes 900 too.

### Controls

- **The real driver with the musl stand-in** (glibc detected, wrong
  `rand()`): `setseed7-bit-identity` and `rnddraws-gold` still FAIL, rc 1. The
  skip is keyed to the platform, not to the values going wrong.
- **The moved `1100` check bites.** A scratch `rnddraws-gold.files` expecting
  `900` gives `check_status: rnddraws-gold: FAILED (files)` (`neg.log`).
- **A bad `.requires` is a broken spec.** `libc musl` gives
  `broken test spec: …requires: 'libc musl'; the only libc this driver can check is glibc`.
- **On this machine** (glibc 2.43) every deck runs: 12 PASS, 0 SKIP.

## 3a — `initw()`'s comment

The comment said "the caller has already seeded" the generators. The
`HasMain` test `main()` at the end of `wallace.c` calls `initw()` without
seeding. It now reads:

```
    /* The uniform generators are not seeded here.  In ngspice, main.c and
       sharedspice.c have already seeded them from $rndseed with
       com_sseed(NULL), and call it again after initw() returns, so the
       startup stream equals 'setseed 1' instead of depending on the
       process id.  The HasMain test main() at the end of this file does
       not seed them before calling initw(). */
```

Comment only, and nothing asserts the text, so no RED test. I did not add
seeding to the test `main()`. The tree never builds it, and it cannot build as
it stands: under `HasMain` it does not include `ngspice.h`, but it uses
`TMALLOC`, and its build line links a `CombTaus.o` that no longer exists.

## 3b — the dead includes

`initw()` was the only caller of `getpid()` in `wallace.c`. With it gone, the
`_MSC_VER` block (`<process.h>`, `#define getpid _getpid`) and the
`<unistd.h>` include had no use. `grep` finds no other `unistd.h` function in
the file. The six lines are removed. The Linux build compiles `wallace.lo`
with no warning (`build.log:758`). The MSVC build was not run. The only thing
`<process.h>` gave this file was `_getpid`.

## 3c — left

The `ngSpice_Init` re-seed (`sharedspice.c`, after `initw()`) lives in the
shared library only. `build-ver_50` is not configured `--with-ngshared`, so
`make check` neither compiles nor runs it. The only shared tree is
`build-ver_51-shared/`, which this crew was told not to touch. As in 2a, the
code was read and not run. Nothing here changed it.

## The two `make check` counts

`./autogen.sh`, `ngheavy make -j4` and `ngheavy make install` were run, in
that order.

| run | binary | PASS | FAIL | XFAIL | XPASS | SKIP | ERROR | `make` exit |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| targeted: `tests/regression/rng` | `e6a151ac9331fb943d20` | 12 | 0 | 0 | 0 | 0 | 0 | 0 |
| full `make check` from `build-ver_50/` | `e6a151ac9331fb943d20` | **351** | 0 | 0 | 0 | 0 | 0 automake results | **0** |

351 is follow-up 2's 350 plus `rnddraws-gold.cir`. One log line starts with
`ERROR:`. It reads `ERROR: (internal)  tried to destroy non-existent graph`,
the known simulator output from `vector-probe-report.cir`, which passes.

**Lint**, both lines from the log:

```
identity lint: 264 comparisons, baseline matches
identity lint: 11 comparisons, baseline matches
```

These are unchanged. No string comparison was added in `src/`.

**Binary:** `sha256sum build-ver_50/src/ngspice | cut -c1-20` →
**`e6a151ac9331fb943d20`** (was `dbb8e37dc9492eeb7d30`). `stage/bin/ngspice`
has the same hash. The source change has no executable effect (dead includes
and a comment). The new hash comes from the rebuild: `autogen.sh` re-ran
configure, and the whole tree recompiled. The draws are unchanged:
`setseed7-bit-identity.cir`, `startup-equals-setseed1.cir`, `rnddraws-gold.cir`
(the old gold values) and the other `rnddraws-*` decks all pass.

## For the owner

1. **What A1 (`build-ver_51-shared/`) means, and what each choice does.**
   ngspice builds two ways. The `ngspice` program is what `make check` tests.
   `libngspice` is a shared library that KiCad, xschem-style front ends and
   other host programs load and drive through `ngSpice_Init`/`ngSpice_Reset`.
   `build-ver_50/` builds only the program. Crew P4 made a second build
   directory, `build-ver_51-shared/`, configured `--with-ngshared`, to run a
   probe against the library. The batch plan said "no second configured
   tree", so the close receipt asked whether that was allowed. The directory
   is untracked and ignored by git, and nothing in the repo depends on it.
   - **Keep it.** It is the only place where the library-only code from this
     batch can actually run. That code is the re-seed in `ngSpice_Init` (3c),
     the reset of the warning flags in `totalreset` (2a), follow-up 1's
     fatal-exit fix (1d), and the B5 gap. Today all of it is compiled and read,
     never run. One crew would rebuild the tree on current `ver_51`, run the
     existing probe, and add checks for these paths. Costs: disk space, and the
     tree is stale until someone rebuilds it.
   - **Delete it.** Nothing is lost that can't be recreated. One
     `configure --with-ngshared` and one build bring it back. Until then those
     paths stay unrun, and every receipt keeps saying so.
   - **My recommendation is to keep it,** and have one crew close 3c, 2a, 1d
     and B5 there in a single pass. A library host goes through
     `ngSpice_Reset` on every reload, and that is the path none of these items
     has run. A bug there reaches users as a wrong seed or a crash in their
     host program, and `make check` cannot catch it.
2. **`check_status.sh` gained a feature** (`.requires`, exit 77). A skipped
   deck now shows as SKIP in `make check`'s count. Here the count stays at
   0 SKIP. If a future receipt shows 2 SKIP, the machine is not on glibc, and
   that is expected.
3. **The glibc probe is the host's libc**, not the binary's. That is the same
   thing for a native build. A cross-compiled binary run under emulation
   would be judged by the build host. No shipped configuration does that.
4. **Not covered by this:** 4d (x87 rounding on a 32-bit build) could make the
   gold decks differ on glibc too. That is speculative and was not in this
   item.

Nothing was pushed. Two commits on `ver_51`: `73733be3f` (code and tests)
and this receipt.
