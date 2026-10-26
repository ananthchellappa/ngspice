# Receipt: item 2 — P2, warn when `.option seed=` overrides `setseed`

PLAN item **2 — P2: warn when `.option seed=` overrides `setseed`**, from
`doc/claude/batches/2026-10-24-mc-v51/PLAN.md`. The owner approved every
recommendation under *Decisions for the owner*. For this item that means
decision 5: **warn only**, once per session, and only after an explicit
`setseed N`. The option keeps winning.

Crew P2, 2026-10-26, branch `ver_51`. HEAD at start **`16e41e0d8`**
(`docs: receipt for ver_51 item 1, the R model through R->B`). The code
commit is **`1c58365bd`** (`Warn once when .option seed= overrides
setseed`). This receipt is the docs commit on top of it.

**Code was touched.** Four production files, +55/−4 lines. The owner's
uncommitted `CLAUDE.md` and `tests/regression/pipe/resource-keyword-case.cmd`
were not staged or modified. `PLAN.md`, `LEDGER.md` and
`tests/lint/identity.baseline` were not edited.

Scratch: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/p2/`.
It holds the RED captures (`red/`), `autogen.log`, `build.log`,
`install.log`, `makecheck.log`, `shared-syntax.log`, and the draw comparison
decks and captures (`draws.cir`, `draws-noopt.cir`, `s7.cir`,
`before-*.txt`, `after-*.txt`).

## What changed, file by file

| file | change |
| --- | --- |
| `src/maths/misc/randnumb.c` | File-scope `setseed_used` and `seed_override_warned`. `com_sseed`'s explicit-value branch sets `setseed_used`, after its existing `srand`/`TausSeed`/`cp_vset` lines, which are unchanged. New `seed_from_option(int seed, bool is_random)` warns once to `cp_err` when `setseed_used`, then does the `cp_vset("rndseed")` + `com_sseed(NULL)` pair. New `rnd_reset_state()` clears both flags. Comments say that a bare `setseed` does not set the flag. |
| `src/include/ngspice/randnumb.h` | Declares the two new functions, and includes `ngspice/bool.h` because the header now uses `bool`. |
| `src/frontend/inp.c` | `eval_opt`'s two arms (`seed=random`, and `seed=N`) each call `seed_from_option()` in place of their `cp_vset` + `com_sseed(NULL)` pair. The `Multiple 'option seed'` warning is untouched. |
| `src/sharedspice.c` | `totalreset` calls `rnd_reset_state()` right after `destroy_wallace()`. |
| `tests/regression/rng/` | New directory, driven by `check_status.sh`: `Makefile.am` and four decks, each with a `.status` (0) and an `.err`. |
| `tests/regression/Makefile.am` | `rng` appended to `SUBDIRS`, after `exitstatus`. |
| `configure.ac` | `tests/regression/rng/Makefile` in `AC_CONFIG_FILES`. |

`./autogen.sh` was re-run, then the tree was rebuilt and `make install` was
run.

The decks:

- `option-seed-setseed.cir`: `.param rv=agauss(1k,100,1)`, `.option
  seed=5`, `foreach k 1 2 3` / `setseed $k` / `reset` / `op`. The `.err` holds
  the warning once. The deck keeps the k=1 and k=2 draws in `const` and does
  `quit 1` unless they are equal, so the warn-only behaviour is pinned.
- `option-seed-random-setseed.cir`: the `seed=random` wording, once over
  three `setseed 7; reset; op` passes. The draws depend on the clock, so the
  only other assertion is that `$rndseed` is no longer 7. This fourth deck is
  not in the plan. It is there because the `seed=random` arm has its own text,
  and that text would otherwise be untested.
- `option-seed-nosetseed.cir`: the option and `reset; op` only, with an empty
  `.err`. This is the false-positive guard.
- `option-seed-absent.cir`: the `setseed` loop without the option, with an
  empty `.err`. It also checks that `setseed 1` and `setseed 2` draw
  different values.

The warning texts, as committed:

```
Warning: '.option seed=5' re-seeds the random number generator on every 'reset',
    so it overrides 'setseed'. Remove the option to let 'setseed' choose the seed.
    (This warning is printed once per session.)
```

```
Warning: '.option seed=random' re-seeds the random number generator from the clock
    on every 'reset', so it overrides 'setseed'. Each seed it uses is stored in
    $rndseed. Remove the option to let 'setseed' choose the seed.
    (This warning is printed once per session.)
```

The random variant does not say the run cannot be repeated, as
`00-scoping.md` requires.

## The RED failure, verbatim

The decks and `.err` files were written first and run through
`tests/bin/check_status.sh` by hand against the current binary,
`53e6e42b7a4ed38ca28a` (the item-1 binary), before any `src/` edit:

```
--- option-seed-setseed.stderr.expected
+++ option-seed-setseed.stderr.filtered
@@ -1,3 +0,0 @@
-Warning: '.option seed=5' re-seeds the random number generator on every 'reset',
-    so it overrides 'setseed'. Remove the option to let 'setseed' choose the seed.
-    (This warning is printed once per session.)
check_status: option-seed-setseed: FAILED (err)
rc=1
check_status: option-seed-nosetseed: exit status 0 as expected
rc=0
check_status: option-seed-absent: exit status 0 as expected
rc=0
--- option-seed-random-setseed.stderr.expected
+++ option-seed-random-setseed.stderr.filtered
@@ -1,4 +0,0 @@
-Warning: '.option seed=random' re-seeds the random number generator from the clock
-    on every 'reset', so it overrides 'setseed'. Each seed it uses is stored in
-    $rndseed. Remove the option to let 'setseed' choose the seed.
-    (This warning is printed once per session.)
check_status: option-seed-random-setseed: FAILED (err)
rc=1
```

This is the expected reason: stderr was empty. The two guards were green
before the change, as the plan says. In the same RED run, the status check
in `option-seed-setseed` passed and its stdout read `option-seed-setseed:
the option still wins`, so the `const` comparison is sound on the old
binary, not only on the new one. Because the `.err` comparison is a diff, a
warning printed on each of the three resets would fail it as well.

## The production change

See the table. `seed_from_option()` does nothing that `eval_opt` did not do
before, apart from the `fprintf`. The warning is printed at the first parse
after an explicit `setseed N`, which in a Monte Carlo loop is the first
`reset`. After that, `seed_override_warned` keeps it quiet until
`rnd_reset_state()`.

## The two `make check` counts

| run | binary | PASS | FAIL | XFAIL | XPASS | SKIP | ERROR | `make` exit |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| targeted: `tests/regression/rng` | `600e6fb61877d47e54b4` | 4 | 0 | 0 | 0 | 0 | 0 | 0 |
| full `make check` from `build-ver_50/` | `600e6fb61877d47e54b4` | **342** | 0 | 0 | 0 | 0 | 0 automake results | **0** |

342 = the baseline's 335 + item 1's three decks + this item's four. No
existing test moved. One log line matches `^ERROR:`, `ERROR: (internal)
tried to destroy non-existent graph`. It is the pre-existing simulator output
from `vector-probe-report.cir`, which passes.

**Lint:** `identity lint: 264 comparisons, baseline matches` and `identity
lint: 11 comparisons, baseline matches`, the same counts as the baseline. The
change adds no string comparison.

**Binary:** `sha256sum build-ver_50/src/ngspice | cut -c1-20` →
`600e6fb61877d47e54b4`. `make install` was run after the build.

## What was measured

- **The draws are unchanged.** With `set numdgt=15`, `draws.cir` (the first
  deck's shape, printing `@r1[r]` for k = 1, 2, 3) and `draws-noopt.cir` (the
  same without the option) were run before and after. Their `@r1[r]` lines
  are byte-identical (`cmp`). With the option every k reads
  `9.312133559184508e+02`. Without it they read `8.860341506323210e+02`,
  `9.251333325967927e+02` and `8.392277021496984e+02`.
- **§4 holds.** `s7.cir` (`setseed 7; reset; op`) reads `@r1[r] =
  9.0682795852564800e+02` after the change. That is the scoping crew's
  `9.068279585256480e+02`.
- **The shared build.** `build-ver_50` is not configured `--with-ngshared`,
  so `totalreset` is not compiled there, and `make check` does not run it.
  `sharedspice.c` was syntax-checked by hand (`gcc -fsyntax-only
  -DHAVE_CONFIG_H -DSHARED_MODULE -DHAVE_LIBPTHREAD -DTHREADS`, using the
  build tree's `config.h`): rc 0, no diagnostics. Nothing has run
  `ngSpice_Reset` and then a second session's warning.

## What was falsified in the plan

1. **Size.** The plan estimated about 25 lines. The production change is
   +55/−4. About half of the new lines are comments and the two warning texts.
2. **Three decks.** A fourth deck was added for the `seed=random` wording
   (see above). The plan's three decks are as specified.
3. **The header needed `bool.h`.** `randnumb.h` included only `wordlist.h`,
   which does not define `bool`. Every current includer gets it from
   `ngspice.h` first, but the header now includes `bool.h` itself.

Confirmed, not falsified: `inp.c:452-456, 465-466` are the two
`cp_vset` + `com_sseed(NULL)` pairs. `check.sh` would have deleted the
warning, so `check_status.sh` is the right driver. `totalreset` calls
`destroy_wallace()` (`sharedspice.c:2601`), and the new call goes next to it.

## What was left, deliberately

- **`eval_opt`'s `Multiple 'option seed=val|random' found!`** still repeats
  on every reset, as the plan says. Fixing it is out of scope.
- **A `setseed N` with no `reset` after it**, in a deck with the option, does
  not warn. No re-parse happens, so the option overrides nothing at that
  point. A `setseed N` in `.spiceinit`, before the first `source`, warns at
  that first parse, which is correct, because the option does override it
  there.
- **Upstream.** Upstream's `eval_opt` uses `strstr`/`eq` where this branch
  has the case-lint `cistrstr`/`eqc`. A later upstream patch of this change
  has to be rebased onto upstream's spelling. Nothing was sent.

## For the owner

1. **Behaviour is unchanged.** The only difference is the warning on stderr.
   A Monte Carlo deck with `.option seed=` and `setseed k` still draws the
   same value for every k, as decision 5 asked. The warning tells the user to
   remove the option.
2. **The shared-library path has been compiled but never run.** If libngspice
   matters before item 6, a `--with-ngshared` build plus a two-session
   driver (`ngSpice_Reset` between them) is the test that would prove that
   the second session warns again. That needs a second configured tree, which
   the standing rules forbid in this batch.
3. **`tests/regression/rng/` now exists.** Items 3 and 4 can add their decks
   there without another `configure.ac` change.

Nothing was pushed. Two commits on `ver_51`: `1c58365bd` (code and tests)
and this receipt.
