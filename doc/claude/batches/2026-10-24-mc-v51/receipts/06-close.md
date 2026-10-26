# Receipt: item 6 — close the ver_51 MC batch

PLAN item **6 — Close the batch**, from
`doc/claude/batches/2026-10-24-mc-v51/PLAN.md`. The owner approved every
recommendation under *Decisions for the owner* (LEDGER, 2026-10-25).

Close crew, 2026-10-26, branch `ver_51`. HEAD at start **`773e60c4d`**
(`docs: review round 1 for ver_51 item 4, reset-order fix`). Batch range
**`a664faa2d..HEAD`** (`a664faa2d` is `docs: plan the ver_51 Monte Carlo
batch`).

**No code was touched.** Nothing under `src/` or `tests/` was edited, and no
`Makefile.am`, `configure.ac` or `autogen.sh` run. This commit adds this
receipt only.

**`LEDGER.md` and `PLAN.md` were not edited.** They belong to the driver. PLAN
item 6 also asks for the ledger's item table, a release note and a reply to the
xschem session. This crew was told not to edit the ledger, so those three are
not done here. The material for them is below: per-item outcomes, *For the
owner*, and *Left for xschem*.

The owner's uncommitted `CLAUDE.md` and
`tests/regression/pipe/resource-keyword-case.cmd` were not staged or modified.
Nothing was pushed or sent upstream, and `doc/claude/upstream/` was not opened.

Scratch: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/close/`
(`build.log`, `install.log`, `check.log`).

## What was run

All from `build-ver_50/`, one heavy command at a time.

| command | result |
| --- | --- |
| `ngheavy make -j4` | exit 0, `Nothing to be done for 'all-am'` |
| `ngheavy make install` | exit 0 |
| `ngheavy make check` | exit 0 |

**Binary.** `build-ver_50/src/ngspice` sha256 **`e53fb551360f6cea4f69`**,
before and after the build. It is the same as item 4's recorded binary. Item
4's review fix (`a269a8a54`) touches only `sharedspice.c`, which links into
libngspice only, so the hash did not move. `cmp src/ngspice stage/bin/ngspice`
reports them identical.

### Full `make check`

| PASS | FAIL | XFAIL | XPASS | SKIP | ERROR | make exit |
| --- | --- | --- | --- | --- | --- | --- |
| 347 | 0 | 0 | 0 | 0 | 0 | 0 |

That is 335 at baseline, plus 3 from item 1, 4 from item 2, 2 from item 3 and 3
from item 4. One log line starts with `ERROR:`. It reads
`ERROR: (internal)  tried to destroy non-existent graph`. This is simulator
output from `tests/regression/casedist/vector-probe-report.cir`, which passes,
and it was there at baseline. It is not an automake result.

**Lint**, both lines from the log:

```
identity lint: 264 comparisons, baseline matches
identity lint: 11 comparisons, baseline matches
```

These are unchanged from the baseline receipt.

### Standing-constraint checks

```
$ git diff a664faa2d..HEAD -- tests/lint/identity.baseline
$                                   (empty, 0 bytes)
$ git log a664faa2d..HEAD -- CLAUDE.md tests/regression/pipe/resource-keyword-case.cmd
$                                   (empty, 0 bytes)
$ git status --short --untracked-files=no
 M CLAUDE.md
 M tests/regression/pipe/resource-keyword-case.cmd
$ git diff --stat a664faa2d..HEAD -- doc/claude/upstream
$                                   (empty)
```

So both owner edits are still modified and uncommitted, no batch commit touched
them, and the identity baseline never moved.

### The commit range

`git log --oneline a664faa2d..HEAD`, before this commit:

```
773e60c4d docs: review round 1 for ver_51 item 4, reset-order fix
a269a8a54 Close the draw log before zeroing the RNG counter on ngSpice_Reset
8ce1ea5e9 docs: receipt for ver_51 item 4, the rnddraws draw log
29940ed39 Log parse-time random draws with set rnddraws=<file>
e955bc093 docs: receipt for ver_51 item 3, no pid seeding at load
5243f8e3b Seed the startup random stream from rndseed, not the process id
a51568215 docs: receipt for ver_51 item 2, the .option seed= warning
1c58365bd Warn once when .option seed= overrides setseed
16e41e0d8 docs: receipt for ver_51 item 1, the R model through R->B
4e614b4da Carry the R model through the R->B rewrite
ae2223e41 docs: record the ver_51 batch baseline
```

`git diff --stat a664faa2d..HEAD` gives 71 files, +4872/−37. Of that, `src/`
is 16 files, +931/−31.

## Per-item outcomes

| # | item | outcome | commits | binary | full `make check` |
| --- | --- | --- | --- | --- | --- |
| 0 | baseline | landed | `ae2223e41` | `2976d0243ee800c7f461` | 335 / 0 |
| 1 | P1: R model through R→B | landed, review pass | `4e614b4da`, `16e41e0d8` | `53e6e42b7a4ed38ca28a` | 338 / 0 |
| 2 | P2: `.option seed=` warning | landed, review pass | `1c58365bd`, `a51568215` | `600e6fb61877d47e54b4` | 342 / 0 |
| 3 | P3: no pid seeding at load | landed, review pass | `5243f8e3b`, `e955bc093` | `a6c67b795d990487275b` | 344 / 0 |
| 4 | P4: `set rnddraws=<file>` | landed, review pass after round 1 | `29940ed39`, `8ce1ea5e9`, `a269a8a54`, `773e60c4d` | `e53fb551360f6cea4f69` | 347 / 0 |
| 5 | P5: native `mcrun` | deferred by plan | — | — | — |
| 6 | close | this receipt | this commit | `e53fb551360f6cea4f69` | 347 / 0 |

Every code item has RED evidence in its receipt, taken against the previous
item's binary. Every full count is 0 FAIL / 0 XFAIL / 0 XPASS / 0 SKIP / 0
ERROR, make exit 0. Every item reports both lint lines matching the baseline.

Tests added, by item:

- **1:** `tests/regression/temper/temper-res-behav-1.cir`,
  `temper-res-behav-noise.cir`, `tests/regression/exitstatus/behav-r-model-warn.cir`.
- **2:** `tests/regression/rng/option-seed-setseed.cir`,
  `option-seed-random-setseed.cir`, `option-seed-nosetseed.cir`,
  `option-seed-absent.cir`.
- **3:** `tests/regression/rng/startup-equals-setseed1.cir`,
  `setseed7-bit-identity.cir`.
- **4:** `tests/regression/rng/rnddraws-mm.cir`, `rnddraws-order.cir`,
  `rnddraws-funcs.cir`. Outside `make check`: the shared-library probe
  `doc/claude/batches/2026-10-24-mc-v51/probes/rnddraws_reset_probe.c`.

## What each crew falsified

**Baseline.** Nothing in the plan was contradicted. "Baseline not yet measured"
is now measured as 335/0/0/0/0, exit 0. `ver_51` had no `src/` change since
`ver_50`, so the build rebuilt nothing and the hash did not change.

**Item 1 (P1).**
- Skipping the optional value token was not enough. `get_model_name()` skipped
  numeric values but not `{..}`, so for `R n 0 {eq} model` the model was dropped
  as unused before the rewrite ran. Fixed in the same commit. This also fixes
  plain value-first R cards, which used to fail with `unknown parameter`.
- The "several exact matches that disagree" rule was not built. What ships is
  stricter: more than one exact match in the winning scope counts as not
  resolved, identical duplicates included. The receipt says the lint makes the
  original rule impossible. Review corrected that: a `/* case-lint: stored */`
  annotation would allow it, and the frozen baseline is what actually stopped
  the crew.
- The plan said nothing about `tce` models. The real R swaps tc1/tc2 for tce
  (`restemp.c:102-105`), so the rewrite warns and carries nothing from them.
- "Pass dtemp/temp through also without a model" conflicts with the
  byte-for-byte rule for cards with no model token. The byte-for-byte rule won,
  so pass-through happens only on the model path.
- Size: estimated 130–180 lines; the actual change is +366/−10 in `inpcom.c`,
  about a third of it comments.

**Item 2 (P2).**
- Size: estimated about 25 lines; production is +55/−4, half of it comments
  and the two warning texts.
- The plan had three decks. A fourth, `option-seed-random-setseed.cir`, was
  added to test the `seed=random` wording.
- `randnumb.h` had no `bool`, so it now includes `ngspice/bool.h`.

**Item 3 (P3).**
- Nothing in scope was contradicted. `wallace.c:83-85`, `main.c:1371` and
  `sharedspice.c:1080` matched the plan, and the change is 4 executable lines.
- Decision 6's side effect goes further than stated. TRNOISE is now the same
  whether or not `setseed` is called, because the Wallace pool is filled once
  from the rndseed=1 stream and `setseed` never refills it.
- The guard deck asserts numerically within 1e-9, not by printed text, because
  `check_status.sh` does not compare stdout.

**Item 4 (P4).**
- The plan's `rng_reseed()` wrapper around srand+TausSeed was not built. The
  §4 rule keeps those pairs byte for byte, so each pair is left as it was and
  followed by a recording call, `rng_note_seed()`.
- "TRNOISE/TRRANDOM/RTS and control-language draws count toward
  `unlogged_calls`" cannot hold with a footer at the end of the parse.
  `unlogged_calls` covers the parse window only. Later uniform calls show up as
  the next set's `offset`. TRNOISE never touches `CombLCGTaus`.
- The item spec said eval_agauss logs the `%g`-rounded spliced value. What
  ships is the raw, unrounded draw. That follows xschem's acceptance in LEDGER
  (2026-10-24), not PLAN decision 7, which the receipt names as its source
  (see review issue 4c below).
- The separate log-off guard deck was folded into `rnddraws-funcs.cir`.
- The file list missed the three `visualc/*.vcxproj` files. They are updated
  with CRLF kept, but were not built.
- Review round 1: the first static review of `totalreset` checked that each
  static is reset, but not in what order. A record set left open by an aborted
  parse got a wrapped `unlogged_calls` (18446744073709544674) on
  `ngSpice_Reset`. Fixed in `a269a8a54`: `ng_drawlog_reset()` now runs before
  `rnd_reset_state()`. The probe failed before the fix and passes all 19 checks
  after it.

## For the owner

Everything from every receipt's *For the owner* section and every open review
issue is merged here, grouped by what it asks of you. Nothing below blocks the
branch. Every review verdict was **pass**, and every open review issue is rated
minor.

### A. Decisions or confirmations you need to make

1. **`build-ver_51-shared/` was a second configured tree.** PLAN's standing
   constraints say "no second configured tree", and item 4's section grants no
   exception. Receipt 04 says the task allowed it, which can't be checked from
   the plan (review 4a). The tree is still present, untracked and covered by the
   `build*/` ignore rule. It is the only place the shared-library fixes from
   items 2–4 have actually been run. Either confirm it was sanctioned or delete
   it.
2. **The release note has not been written.** PLAN item 6 asks for one, but this
   crew was told not to edit the ledger, so it was left for the driver. It must
   say:
   - **P1 changes results** for every converted R that names a model, sky130
     `xhigh_po` included. A converted R with a temper expression and a model TC
     now gets the TC applied, plus a warning.
   - **P3 makes decks without `setseed` deterministic across processes**
     (startup stream = `setseed 1`, TRNOISE included). To get runs that differ,
     use `.option seed=random` or `unset rndseed` in `.spiceinit`. `setseed N`
     in `.spiceinit` now takes effect at startup.
   - P2 only adds a stderr warning. The draws are byte-identical.
3. **The LEDGER item table, the falsified notes and the final counts** are
   still the driver's to fill in. The figures are above.
4. **Whether P2/P3 go upstream.** By decision 8, nothing was prepared or sent.
   That would be a follow-up batch.

### B. Known gaps that were left on purpose

5. **The shared-library paths are not covered by `make check`.**
   `rnd_reset_state()` in `totalreset` (item 2) and the `ngSpice_Init` re-seed
   in `sharedspice.c:1080-1083` (item 3) were syntax-checked with
   `-DSHARED_MODULE` but never run under `build-ver_50`. The item-4 probe, run
   against `build-ver_51-shared`, uses the shared library: its 19 checks include
   draw-log reset and a startup set. The tree has no shared-library harness, so
   the probe is not part of `make check`.
6. **P1 still leaves these unfixed:** a converted R with no model drops instance
   `dtemp`/`temp`/`tc=`, as before. Flicker noise is not reproduced. Under
   `.option tnom≠27`, a model without `tnom` keeps ASRC's 27 °C reference
   (decision 3).
7. **P1 duplicates:** identical duplicate `.model` cards in one scope now carry
   nothing, and the warning names them as not resolved.
8. **P2:** `eval_opt`'s existing "Multiple option seed" warning still repeats
   on every reset. Out of scope, per the plan.
9. **The glibc-only decks** `setseed7-bit-identity.cir` and the guard half of
   `rnddraws-funcs.cir` have no automatic skip. On another libc they will
   FAIL. They need a skip there, not new values (review 3d).
10. **Not built:** the three `visualc/*.vcxproj` updates from item 4.
11. **P1's warnings deck lives in `tests/regression/exitstatus/`,** the only
    `check_status.sh` directory at the time. A dedicated directory would need a
    `configure.ac` entry. `tests/regression/rng/` now exists and is registered.

### C. Open review issues (all minor, none fixed in this batch)

Item 1:
- **1a.** `tests/regression/exitstatus/Makefile.am`: `TESTS = ` became
  `TESTS =` (lost the space). This is whitespace churn with no effect.
- **1b.** `behav_r_model_args()`: the temper+TC warning tests `(tc1 || tc2)`
  after the instance has overridden the model. It also fires when only the
  instance gives tc1/tc2, and then its text "model %s adds tc1/tc2" is wrong.
  A converted R with instance tc1 and temper but no model token gets no
  warning. Only the message is affected.
- **1c.** Receipt 01's falsified item 2 gives an imprecise reason (see the
  falsified list above).
- **1d.** In a deck with `.if`, `inp_collect_models()` now runs where it did
  not before, so its `controlled_exit(EXIT_BAD)` on a typeless `.model` is a new
  exit path. It is reached only when a converted R names a model. In the shared
  build, leaving through `controlled_exit` skips `inp_free_models()`, so the
  lists leak, but only on an error path.
- **1e.** `temper-res-behav-noise.cir` covers top-level cards only. The subckt
  noise branch, where the model name is translated during expansion, was
  checked by hand only (`xs:rm` picked, onoise_total 1.118932e-06 for both at
  125 °C). No committed test covers it.

Item 2:
- **2a.** The shared reset path (`sharedspice.c:2602`) was syntax-checked only.
  This is the same gap as B5.
- **2b.** The warning text says the option re-seeds "on every reset", but it
  also does so on every `source`. The text also prints the parsed integer
  (`seed=05` is shown as `seed=5`). Both are wording only.
- **2c.** `option-seed-random-setseed.cir`'s `$rndseed = 7` check is weak. It
  fails spuriously about one time in 10^6 (`tv_usec == 7`).

Item 3:
- **3a.** `initw()`'s new comment says the caller has already seeded. That is
  not true of the `HasMain` standalone `main()` in `wallace.c:502ff`, which the
  tree never builds.
- **3b.** `#define getpid _getpid`, `<process.h>` and `<unistd.h>` are now
  unused in `wallace.c`. They were left to keep the diff small.
- **3c.** The `ngSpice_Init` re-seed has not been run. This is the same gap as
  B5. Reading the code shows it is correct.
- **3d.** The glibc-only deck has no skip. This is the same as B9.

Item 4:
- **4a.** The second configured tree. This is the same as A1.
- **4b.** "value is the raw draw" is credited to PLAN decision 7. The actual
  source is the LEDGER note of 2026-10-24.
- **4c.** In tree, only `src=setseed` (and the `offset != 0` stale set) are
  asserted. `src=option` and `src=checkseed` were checked by hand only
  (`seed=5 src=option offset=0`; `seed=11 src=checkseed offset=1`), and
  `src=startup` only by the probe and the fixture.
- **4d.** Bit identity is proven on x86-64/SSE only. A 32-bit x87 build might
  round the split `z = gauss1(); r = nominal + stdvar*z` differently. This is
  speculative; no shipped configuration is affected.
- **4e.** `rng_calls++` in `CombLCGTaus()` is an unsynchronised static. A
  background transient in the shared library could race with it. This is
  cosmetic: the counter is read only during a parse, and the generator itself is
  not thread-safe.
- **4f.** The probe's `strncmp(..., 14)` drops the trailing tab, so
  `logged=10` would also match, though the receipt calls the check "exact".
  This is trivial.

### D. Carried over from scoping (recorded, not planned)

- Handoff Q2 (`reset` about 20 % slower than 45.2). Not profiled. The
  hypotheses are in `00-scoping.md`.
- TRNOISE is not reproducible across loop iterations, because `setseed` never
  refills the Wallace pool. This is also in the ledger's *Possible later asks*.
- `oldseed`, `seedinfo` and `gauss0`'s cache survive `ngSpice_Reset`. The
  `{random}` arm in `eval_opt` is dead code. `insertnumber` can leave
  `dico->cardline` dangling. `tclspice.c` still seeds from the pid.
- P5 `mcrun` stays deferred. If it is revisited, it carries "`setseed` wins
  over `.option seed`".

## Left for xschem

PLAN item 6's reply to the xschem session has **not been written or sent**
from here. Its content:

1. **The log contract as shipped matches decision 7** and xschem's 2026-10-24
   acceptance. Field order: `seed instance_path param func args z value`.
   Header and footer:
   `#\trnddraws\tv1\tseed=<n>\tsrc=<src>\toffset=<n>` …
   `#\tend\tlogged=<n>\tunlogged_calls=<n>`. The reply should also state these
   details:
   - `src` is one of `startup|setseed|option|checkseed`.
   - `offset` counts uniform-generator calls. Drop a set when `offset != 0`.
   - `unlogged_calls` covers the parse window only. Control-language and RTS
     draws after the parse show up as the next set's `offset`. TRNOISE never
     counts.
   - `value` is the raw, unrounded draw. The deck itself gets the `%g` text.
   - `?` stands for a positional value and for a global `.param`.
   - `.model` rows are keyed by the binned model name.
   - `set rnddraws=` truncates the file.
   - `pre_set rnddraws=` logs the initial load, and reset does not re-run it.
2. **The parser fixture is committed:**
   `doc/claude/batches/2026-10-24-mc-v51/fixtures/rnddraws-sample.tsv`, made
   with its deck `rnddraws-sample.cir` on the real sky130 PDK. It has five
   record sets of 297 rows each.
3. **Revert the `fixp.cir`-style temper patch** when adopting ver_51. P1 now
   applies the model TC, so with the patch the TC is applied twice, and the new
   warning flags it.
4. **Determinism:** with no `setseed`, the startup draw is now the `setseed 1`
   stream in every process (8.860341506323210e+02 in the scoping deck).
   `setseed 7` still gives 9.068279585256480e+02, bit-identical with 45.2.
5. **`.option seed=` still wins over `setseed`.** It now warns once per session,
   on stderr, after the first explicit `setseed`. xschem's loop should not set
   the option.
6. **Not delivered:** transient-noise MC reproducibility (Wallace refill) and
   `mcrun`.

Nothing was pushed. One docs commit on `ver_51`.
