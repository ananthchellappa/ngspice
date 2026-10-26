# Receipt: follow-up 2, item 3 — the "Cannot convert" seed warning, once

Crew **"Seed warning"**, ver_51 follow-up batch 2, 2026-10-26, branch
`ver_51`. HEAD at start **`c2b63479e`** (`docs: receipt for ver_51 follow-up
2, item 2, P1 hardening`). The code commit is **`f8f0f60d5`** (`Print the
.option seed 'Cannot convert' warning once per session`). This receipt is the
docs commit on top of it.

The item closes *For the owner* 4 of
`doc/claude/batches/2026-10-26-mc-v51-followup/receipts/02-p2-fixes.md`, and
the probe bug **4f** of `doc/claude/batches/2026-10-24-mc-v51/receipts/06-close.md`.

**Code was touched**, in three production files: `src/maths/misc/randnumb.c`,
`src/include/ngspice/randnumb.h` and `src/frontend/inp.c` (+24/−3 between
them). The owner's uncommitted `CLAUDE.md` and
`tests/regression/pipe/resource-keyword-case.cmd` were not staged or
modified. No `LEDGER.md`, `PLAN.md` or `tests/lint/identity.baseline` was
edited. No shared-library tree was created and no worktree was used. Nothing
was pushed or sent anywhere. Every heavy command went through `ngheavy`, one
at a time, with `-j4` at most, and the build was followed by `ngheavy make
install`.

Scratch: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/s3/`.
It holds `ngspice-old` (the pre-change binary, `ff62bd5239326574ff8f`),
`red.log`, `autogen.log`, `build.log`, `install.log`, `rng-check.log` and
`check.log`.

## What changed

**Before.** `eval_opt()` printed `Warning: Cannot convert 'option seed=%s' to
seed value, skipped!` straight from `inp.c` on every parse. `reset` is a
parse, so a 1000-sample MC loop over a deck with `.option seed=abc` printed it
1000 times.

**After.** The `fprintf` moved into a new `seed_option_unusable(const char
*written)` in `randnumb.c`, built the same way as `seed_option_multiple()`:

- it prints the line once, with `    (This warning is printed once per
  session.)` under it, and sets a new flag, `seed_unusable_warned`;
- `rnd_reset_state()` clears that flag together with the other three, so in
  the shared build `totalreset()` lets a new session warn again;
- the first line is unchanged, and it still quotes the value as the deck
  wrote it (`eval_opt`'s token).

What the option does is unchanged: the value is still skipped on every parse
and never seeds. The comment block above the flags names the new function.
The prototype is in `randnumb.h`, next to `seed_option_multiple()`.

The new text:

```
Warning: Cannot convert 'option seed=abc' to seed value, skipped!
    (This warning is printed once per session.)
```

**Test.** New deck `tests/regression/rng/option-seed-unusable.cir`, driven by
`check_status.sh` like the rest of the directory. It has `.option seed=abc`
and `foreach k 1 2 3` / `setseed 3` / `reset` / `op`, so it parses four
times. Its `.err` holds the warning once. The deck does `quit 1` unless
`$rndseed` is still 3 after each reset, which pins that the skipped option
does not seed. Its `.status` is 0. `tests/regression/rng/Makefile.am` lists it
in `TESTS`, `EXTRA_DIST`, `CLEANFILES` and its header comment.

## The RED failure

The deck, `.err` and `.status` were written first and run through
`tests/bin/check_status.sh` by hand against the unchanged binary
`ff62bd5239326574ff8f`, before any `src/` edit. Trimmed:

```
--- option-seed-unusable.stderr.expected
+++ option-seed-unusable.stderr.filtered
@@ -1,2 +1,4 @@
 Warning: Cannot convert 'option seed=abc' to seed value, skipped!
-    (This warning is printed once per session.)
+Warning: Cannot convert 'option seed=abc' to seed value, skipped!
+Warning: Cannot convert 'option seed=abc' to seed value, skipped!
+Warning: Cannot convert 'option seed=abc' to seed value, skipped!
check_status: option-seed-unusable: FAILED: err
```

It fails for the reason it was written for: three extra copies, one per
reset. The only failing check is `err`. The status check passed, so the
`$rndseed = 3` test was sound on the old binary too.

## The two `make check` counts

`./autogen.sh`, `ngheavy make -j4` and `ngheavy make install` were run, in
that order. The build log has no warning from `randnumb.c`, and none from
`inp.c` near the edited line.

| run | binary | PASS | FAIL | XFAIL | XPASS | SKIP | `make` exit |
| --- | --- | --- | --- | --- | --- | --- | --- |
| targeted: `tests/regression/rng` | `68e577b28b297b0054d9` | 15 | 0 | 0 | 0 | 0 | 0 |
| full `make check` from `build-ver_50/` | `68e577b28b297b0054d9` | **362** | 0 | 0 | 0 | 0 | **0** |

362 is item 2's 361 plus `option-seed-unusable.cir`. One log line starts with
`ERROR:`. It reads `ERROR: (internal)  tried to destroy non-existent graph`,
the known simulator output from `vector-probe-report.cir`, which passes. It
is not an automake result.

**Lint**, both lines from the log:

```
identity lint: 264 comparisons, baseline matches
identity lint: 11 comparisons, baseline matches
```

Unchanged. The change adds no string comparison: the warning prints the
token, it does not compare it.

**Binary:** `sha256sum build-ver_50/src/ngspice | cut -c1-20` →
**`68e577b28b297b0054d9`** (was `ff62bd5239326574ff8f`).

**The draws are unchanged.** The change moves one `fprintf` and adds a flag.
No `srand`/`TausSeed`/`cp_vset` line was touched, and
`setseed7-bit-identity.cir`, `startup-equals-setseed1.cir` and the
`rnddraws-*` decks pass unchanged.

## Probe bug 4f — fixed, not run

`doc/claude/batches/2026-10-24-mc-v51/probes/rnddraws_reset_probe.c`, session
3's footer check:

```c
-        check(strncmp(footer, "#\tend\tlogged=1\t", 14) == 0 && u && unlogged == 0,
+        check(strncmp(footer, "#\tend\tlogged=1\t", 15) == 0 && u && unlogged == 0,
```

The literal is 15 bytes (`#`, tab, `end`, tab, `logged=1`, tab). With 14 the
tab after `logged=1` was not compared, so a footer reading `logged=12` would
have passed. The other four `strncmp` lengths in the file were counted and
are right (14, 6, 26, 26, 6).

**It was not run.** The probe links against `libngspice`, and there is no
shared-library build: the owner deleted `build-ver_51-shared/`, and this crew
was told not to recreate it. The only check made is `gcc -fsyntax-only -Wall
-Isrc/include` on the file, which exits 0 with no warning. So the fix, and the
`totalreset()` → `rnd_reset_state()` path that clears the new flag, are read
from the code, not run.

## For the owner

1. **All three `.option seed=` warnings are now once per session**: the one
   about overriding `setseed`, the `Multiple` one, and now `Cannot convert`.
   A 1000-sample loop over a deck with a bad `seed=` value prints it once.
2. **The same trade-off as before.** In one standalone `ngspice` session that
   sources deck A with `seed=abc` and then deck B with `seed=xyz`, only A
   warns. A shared-library host gets the warning again after `ngSpice_Reset`
   (from the code; not run, see above).
3. **The probe fix is a one-character change to a test program nobody can run
   right now.** It makes the probe stricter. When a shared build exists again,
   that probe, plus a two-session test of these warnings, is what would close
   the shared-only gaps the earlier receipts list.

Nothing was pushed. Two commits on `ver_51`: `f8f0f60d5` (code, test and probe
fix) and this receipt.
