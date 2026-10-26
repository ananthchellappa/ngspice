# Receipt: follow-up 2 — P2 fixes (review issues 2a–2c, gap B8)

Follow-up crew **"P2 fixes"** for the ver_51 MC batch. The issues are item 2's
open review issues in `doc/claude/batches/2026-10-24-mc-v51/receipts/06-close.md`
section C (2a–2c), and the P2 gap in section B (8). The item's own receipt is
`doc/claude/batches/2026-10-24-mc-v51/receipts/02-p2-seed-option-warning.md`.
The spec is that batch's `PLAN.md`, item 2, which the owner approved.

Crew P2-fixes, 2026-10-26, branch `ver_51`. HEAD at start **`da9023b75`**
(`docs: receipt for ver_51 follow-up 1, the P1 fixes`). The code commit is
**`e1db1d3df`** (`Print the .option seed= warnings once per session, as
written`). This receipt is the docs commit on top of it.

**Code was touched.** Three production files: `src/maths/misc/randnumb.c`,
`src/include/ngspice/randnumb.h` and `src/frontend/inp.c` (+41/−15 between
them). The owner's uncommitted `CLAUDE.md` and
`tests/regression/pipe/resource-keyword-case.cmd` were not staged or
modified. No `LEDGER.md` or `PLAN.md` was edited, and neither was
`tests/lint/identity.baseline`. `build-ver_51-shared/` was not touched, built
in or deleted. Nothing was pushed or sent anywhere. Every heavy command went
through `ngheavy`, one at a time, with `-j4` at most.

Scratch: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/f2/`.
It holds `red.log` (the RED run), `ngspice-old` (the pre-change binary,
`0d61507c992f9ecfd8be`), `autogen.log`, `build.log`, `install.log`,
`rng-check.log`, `check.log`, the probe deck `multi.cir` and the 2c negative
control `neg/noopt.cir`.

## Issues taken

| issue | outcome | what |
| --- | --- | --- |
| 2a | left, as the task says | The shared reset path needs a shared build. See *2a* and *For the owner* 1. |
| 2b | fixed, RED first | The warning now says the option re-seeds "each time the circuit is parsed, by 'source' and by every 'reset'", and quotes the value as the deck wrote it. |
| 2c | fixed | `option-seed-random-setseed.cir` uses `setseed 1000000`, a value the clock-based option cannot produce. |
| B8 | fixed, RED first | `eval_opt`'s `Multiple 'option seed=val\|random' found!` is printed once per session, not on every parse. |

## 2b — the warning's wording

**Before.** Both texts said the option re-seeds "on every 'reset'". It does
that on every parse, and the first `source` of a deck is a parse too. A
`setseed N` in `.spiceinit` followed by `source deck.cir` gets the warning at
that `source`, where no reset has happened. The fixed-seed text also printed
the parsed integer with `%d`, so `.option seed=05` was quoted as
`seed=5`, a string the user cannot find in the deck.

**After.** `seed_from_option()` takes a third argument, `written`, which is
`eval_opt`'s own token. Both texts print it with `%s`. The two texts now read:

```
Warning: '.option seed=05' re-seeds the random number generator each time the
    circuit is parsed, by 'source' and by every 'reset', so it overrides 'setseed'.
    Remove the option to let 'setseed' choose the seed.
    (This warning is printed once per session.)
```

```
Warning: '.option seed=random' re-seeds the random number generator from the clock
    each time the circuit is parsed, by 'source' and by every 'reset', so it
    overrides 'setseed'. Each seed it uses is stored in $rndseed. Remove the
    option to let 'setseed' choose the seed.
    (This warning is printed once per session.)
```

The random variant still does not say the run cannot be repeated, as
`00-scoping.md` requires. With `seed={random}` it now quotes `{random}`, as
written.

**Tests.** Two committed `.err` files assert the old text, so they were
updated to the new text: `option-seed-setseed.err` and
`option-seed-random-setseed.err`. This is not a weaker assertion. Each is
still a full-text diff that also fails if the warning is printed more than
once. A new deck, `option-seed-spelling.cir`, pins the echo: `.option
seed=05`, `setseed 3`, `reset`, `op`. Its `.err` expects `seed=05`, and the
deck does `quit 1` unless `$rndseed` is 5 again, so the option still wins.

## 2c — the flaky `$rndseed = 7` check

The deck called `setseed 7` before each reset and failed unless `$rndseed`
was no longer 7. The option's seed is `tv.tv_usec`, which runs from 0 to
999999, so in about one run in 10^6 the clock gave 7 and the check failed
with nothing wrong.

The deck now calls `setseed 1000000` and checks `$rndseed = 1000000`.
1000000 is a valid `setseed` value (`> 0`, `<= INT_MAX`), and `tv_usec` can
never equal it, so the check cannot fail by chance. A comment in the deck
says why.

**No RED run is possible** for a one-in-a-million flake. The negative control
shows that the stronger check still catches the failure it is there for. With
the `.option seed=random` line removed (`neg/noopt.cir`), nothing overrides
`setseed`, and the deck fails:

```
option-seed-random-setseed: setseed 1000000 was not overridden
rc=1
```

With the option it exits 0.

## B8 — the `Multiple` warning on every reset

**Before.** `eval_opt` printed `Warning: Multiple 'option seed=val|random'
found!` on every parse of a deck with two `.option seed=` cards. A
1000-sample MC loop printed it 1000 times. `multi.cir` (two cards, three
`reset; op`) printed it four times: once at the load and once per reset.

**After.** The `fprintf` moved into `seed_option_multiple()` in
`randnumb.c`. That function prints the line once, with
`    (This warning is printed once per session.)` under it, the same as P2's
warning, and then sets `seed_multiple_warned`. `rnd_reset_state()` clears
that flag, so in the shared build it is cleared by `totalreset` together with
the other two. The first line of the warning is unchanged. Which seed wins is
unchanged: each `seed=` still re-seeds in turn.

New deck `option-seed-multiple.cir`: `.option seed=5`, `.option seed=6`, and
`foreach k 1 2 3` / `reset` / `op`. It parses four times, and its `.err`
holds the warning once. No `setseed` is called, so the other warning stays
silent.

## The RED failure

The four `.err` files were written or updated first and run through
`tests/bin/check_status.sh` by hand against the unchanged binary
`0d61507c992f9ecfd8be`, before any `src/` edit. Trimmed:

```
--- option-seed-setseed.stderr.expected
+++ option-seed-setseed.stderr.filtered
-Warning: '.option seed=5' re-seeds the random number generator each time the
-    circuit is parsed, by 'source' and by every 'reset', so it overrides 'setseed'.
-    Remove the option to let 'setseed' choose the seed.
+Warning: '.option seed=5' re-seeds the random number generator on every 'reset',
+    so it overrides 'setseed'. Remove the option to let 'setseed' choose the seed.
check_status: option-seed-setseed: FAILED (err)
--- option-seed-random-setseed.stderr.expected
+++ option-seed-random-setseed.stderr.filtered
-    each time the circuit is parsed, by 'source' and by every 'reset', so it
-    overrides 'setseed'. Each seed it uses is stored in $rndseed. Remove the
-    option to let 'setseed' choose the seed.
+    on every 'reset', so it overrides 'setseed'. Each seed it uses is stored in
+    $rndseed. Remove the option to let 'setseed' choose the seed.
check_status: option-seed-random-setseed: FAILED (err)
--- option-seed-spelling.stderr.expected
+++ option-seed-spelling.stderr.filtered
-Warning: '.option seed=05' re-seeds the random number generator each time the
-    circuit is parsed, by 'source' and by every 'reset', so it overrides 'setseed'.
-    Remove the option to let 'setseed' choose the seed.
+Warning: '.option seed=5' re-seeds the random number generator on every 'reset',
+    so it overrides 'setseed'. Remove the option to let 'setseed' choose the seed.
check_status: option-seed-spelling: FAILED (err)
--- option-seed-multiple.stderr.expected
+++ option-seed-multiple.stderr.filtered
 Warning: Multiple 'option seed=val|random' found!
-    (This warning is printed once per session.)
+Warning: Multiple 'option seed=val|random' found!
+Warning: Multiple 'option seed=val|random' found!
+Warning: Multiple 'option seed=val|random' found!
check_status: option-seed-multiple: FAILED (err)
check_status: option-seed-nosetseed: exit status 0 as expected
check_status: option-seed-absent: exit status 0 as expected
```

Each fails for the reason it was written for: the old wording, `seed=5` for
`seed=05`, and three extra copies of the `Multiple` line. In the same run,
the only failing check in each is `err`. The status checks passed, so the
`$rndseed = 5` test in `option-seed-spelling.cir` is sound on the old binary
as well. The two guards were green before and after.

## The two `make check` counts

`tests/regression/rng/Makefile.am` gained the two decks in `TESTS`,
`EXTRA_DIST`, `CLEANFILES` and its header comment. `./autogen.sh`,
`ngheavy make -j4` and `ngheavy make install` were run, in that order. The
build log has no new warning from `randnumb.c` or `inp.c`. The `inp.c`
warnings it shows are at lines 505–687, which this change does not touch.

| run | binary | PASS | FAIL | XFAIL | XPASS | SKIP | ERROR | `make` exit |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| targeted: `tests/regression/rng` | `dbb8e37dc9492eeb7d30` | 11 | 0 | 0 | 0 | 0 | 0 | 0 |
| full `make check` from `build-ver_50/` | `dbb8e37dc9492eeb7d30` | **350** | 0 | 0 | 0 | 0 | 0 automake results | **0** |

350 is follow-up 1's 348 plus the two new decks. One log line starts with
`ERROR:`. It reads `ERROR: (internal)  tried to destroy non-existent graph`,
the known simulator output from `vector-probe-report.cir`, which passes.

**Lint**, both lines from the log:

```
identity lint: 264 comparisons, baseline matches
identity lint: 11 comparisons, baseline matches
```

These are unchanged. The change adds no string comparison: the warning
prints `eval_opt`'s token, it does not compare it.

**Binary:** `sha256sum build-ver_50/src/ngspice | cut -c1-20` →
**`dbb8e37dc9492eeb7d30`** (was `0d61507c992f9ecfd8be`).

**The draws are unchanged.** The change adds and moves `fprintf` calls only.
Every `srand`/`TausSeed`/`cp_vset` line is as it was.
`setseed7-bit-identity.cir`, `startup-equals-setseed1.cir` and the three
`rnddraws-*` decks pass unchanged.

## 2a — left

`totalreset()` in `sharedspice.c` calls `rnd_reset_state()`, which now also
clears `seed_multiple_warned`. That line is in the shared library only.
`build-ver_50` is not configured `--with-ngshared`, so `make check` neither
compiles nor runs it. `sharedspice.c` was not edited in this item. The only
shared tree is `build-ver_51-shared/`, which this crew was told not to touch.
So the claim "a second shared-library session warns again" is still read
from the code, not run.

## For the owner

1. **What "A1" in the close receipt is asking you, and what each answer
   means.** Crew P4 configured a second build directory,
   `build-ver_51-shared/`, built as the shared library (`libngspice`, the
   form KiCad and other host programs load) instead of the `ngspice`
   program. The batch plan said "no second configured tree", so the close
   receipt asked you to confirm that it was allowed, or to delete it. It is
   untracked and git ignores it. Nothing in the repo depends on it.
   - **Keep it.** It is the only place where the shared-library code paths
     can be run: 2a here, the item-3 re-seed (3c), follow-up 1's fatal-exit
     fix (1d), and the B5 gap. A crew would rebuild it against current
     `ver_51` and run the existing probe plus a two-session test for these
     warnings. The cost is disk space, and a tree that goes stale unless
     someone rebuilds it before use.
   - **Delete it.** Nothing is lost that can't be rebuilt. One `configure
     --with-ngshared` and a build would recreate it. Until then, every
     shared-only path stays compiled-but-not-run, and future receipts will
     keep saying so.
   - My recommendation: **keep it** and let one crew use it to close 2a,
     3c, 1d and B5 together. libngspice users go through `ngSpice_Reset` on
     every reload, and that is exactly the path none of these items has
     ever run.
2. **Anyone quoting the old warning text** (the xschem reply in the close
   receipt, *Left for xschem* 5, only paraphrases it) should use the new
   text above. The xschem reply's content is still right: the warning is
   once per session, on stderr.
3. **"Once per session" has a cost in interactive use.** In a standalone
   `ngspice` that sources deck A and then deck B, both with two `seed=`
   cards, only A warns. The same is true of P2's warning. This is the
   trade-off the driver asked for, and it is what makes a 1000-sample loop
   print the warning once.
4. **Not changed:** `Warning: Cannot convert 'option seed=%s' to seed value,
   skipped!` still prints on every parse. It was not in this item's list. If
   you want it once per session too, it is the same three-line change.

Nothing was pushed. Two commits on `ver_51`: `e1db1d3df` (code and tests)
and this receipt.
