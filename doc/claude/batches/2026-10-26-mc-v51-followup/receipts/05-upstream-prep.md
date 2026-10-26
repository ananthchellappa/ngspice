# Receipt: follow-up 5 — upstream prep for P1, P2 and P3

Follow-up crew **"Upstream prep"** for the ver_51 MC batch. The task was to
prepare upstream patches for P1, P2 and P3 (not P4) against
`origin/pre-master-47`, following
`doc/claude/batches/2026-08-15-exit-status-testing/upstream-0072/`, and to
send nothing. This is PLAN decision 8's "follow-up batch, if wanted".

Crew Upstream-prep, 2026-10-26, branch `ver_51`. HEAD at start
**`f6353040f`** (`docs: receipt for ver_51 follow-up 4, the P4 fixes`).
`origin/pre-master-47` is **`c5cd68015`**. There are two commits:
**`f8ac6e67c`** (`Make the R->B temper decks fail when a vector is missing`,
tests only), and the docs commit that holds this receipt and `upstream/`.

**No production code on `ver_51` was touched.** The upstream patches are
dangling commits, made with `git commit-tree` from a temporary index whose
parent is `pre-master-47`. No ref was created or moved, and no branch was
switched. One `git worktree add --detach` under the scratchpad was used for
the `git apply --check` runs, as the task allowed. It was removed and pruned
afterwards (`git worktree list` shows only the main tree). Nothing was built
in it. The owner's `CLAUDE.md` and
`tests/regression/pipe/resource-keyword-case.cmd` were not staged or
modified. No `LEDGER.md`, `PLAN.md` or `tests/lint/identity.baseline` was
edited. `build-ver_51-shared/` was not touched. It is not on disk. Nothing was
pushed or sent anywhere.

Scratch: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/`.
It holds:
- `up/`, `pre-master-47`'s `src/` from `git archive`, used for the syntax
  checks;
- `p1w/`, `p2w/` and `p3w/`, the patched upstream files;
- `p1t/` and `p3t/`, the test runs;
- `repro/`, the minimal decks;
- `msg/`, the commit messages;
- `check.log`, the full `make check`.

## What was made

`doc/claude/batches/2026-10-26-mc-v51-followup/upstream/`:

| item | patch | squashes from `ver_51` | `git apply --check` on `pre-master-47` |
| --- | --- | --- | --- |
| P1 | `p1/0001-inpcom.c-carry-the-R-model-through-the-R-B-rewrite.patch` | `4e614b4da`, `82d8e7018`, `f8ac6e67c` (the test hardening below) | clean |
| P2 | `p2/0001-randnumb.c-warn-once-when-.option-seed-overrides-set.patch` | `1c58365bd`, `e1db1d3df` | clean |
| P3 | `p3/0001-wallace.c-do-not-re-seed-from-the-process-id-at-star.patch` | `5243f8e3b`, the `wallace.c` half of `73733be3f` | clean |

The three also apply together, and P3 applies before P2. They share
`sharedspice.c`, in different hunks. `README.md` covers several things:

- which patch is which;
- what was checked and what was not;
- what each patch leaves out compared with `ver_51`;
- the before/after test table;
- that nothing has been sent.

Each patch's commit message is written in upstream style: the problem, a
minimal deck with the 45.2 numbers, the fix, the behaviour change and the
tests.

**The per-item `REPORT.md` files are missing.** The harness refused the
`Write` of `p1/REPORT.md`, `p2/REPORT.md` and `p3/REPORT.md` with "Subagents
should return findings as text, not write report files". I did not work
around that. Their text went to the batch driver in this crew's final
response, and *For the owner* 4 says what to do with it.

**Checks on the patches:**
- `git diff --check` is clean for all three.
- Every patched C file was compiled with `gcc -fsyntax-only` against
  `pre-master-47`'s own headers (with `build-ver_50`'s `config.h`; with
  `-DSHARED_MODULE -DHAVE_LIBPTHREAD` for `sharedspice.c`). In each case the
  warning count equals the unpatched file's:

  | file | count |
  | --- | --- |
  | `inpcom.c` | 3 |
  | `inp.c` | 5 |
  | `main.c` | 3 |
  | `randnumb.c`, `wallace.c`, `sharedspice.c` | 0 |

- Not built, not run on `pre-master-47`.

## What changed between `ver_51` and the upstream patches

**P1** (`inpcom.c`, ported by hand: 2 of 8 hunks did not apply to upstream's
file):
- Removed: the `behav_r_models_root` / `inp_netlist_read_reset()` hook
  (follow-up 1, issue 1d). `inp_netlist_read_reset()` exists only in the fork
  (`3d3e56e75`). Without it, a fatal exit inside `inp_compat()` in
  `libngspice` leaks the model lists. On that same path upstream already leaks
  the scope tree and the deck.
- Changed: `model_name_match(a, b, !inp_case_exact_ids())` became upstream's
  2-argument form. `elem_letter(...)` became `*line == 'r'` /
  `*curr_line != 'r'`. Upstream lowercases the deck.
- Removed: the "decision 4:" prefix of one comment.
- Removed: `tests/regression/exitstatus/behav-r-model-warn.*`, because it
  needs `check_status.sh`, which upstream lacks.
- Kept: the three temper decks, their `.sp`/`.lib`/`.out`, and the
  `Makefile.am` lines. They run under `check.sh`, which is byte-identical in
  upstream.

**P2:**
- Removed: everything P4 put into the same functions (the `rng_*` seed
  record, `drawlog.h`, `ng_drawlog_*`). The P2 code was rebuilt on upstream's
  `randnumb.c`/`.h`, which are identical to the fork's pre-batch blobs.
- Removed: `tests/regression/rng/`, its `configure.ac` and `Makefile.am`
  lines, and `check_status.sh`. No upstream harness can see stderr, so the
  patch has no test, as 0072's did not. Upstream's `eval_opt()` keeps
  `strstr`/`eq`. No case-lint annotation was involved.

**P3:**
- Removed: the `check_status.sh` `.requires` feature and the two
  `check_status.sh` decks.
- Added: one new upstream-form deck, `tests/regression/misc/startup-seed.cir`
  with its `.out`, plus its `Makefile.am` line.

## Issue taken: the temper decks passed without checking anything

While checking P1's decks against 45.2 for the "before" column, I found that
all three **passed on 45.2**, where P1's bug is present. 45.2 cannot create
`const.x` with `let`, because it lacks `785be3194`. So every `const.*` vector
was missing, and each deck's `if <failure condition>` took its `else` branch,
which printed `INFO: success`. On `ver_51`, a binary with a broken const plot
would pass in the same way, so the test was weak. The fix makes the success
condition the tested branch: `if const.maxerr <= 1e-9 and ... ` →
`INFO: success`, `else` → `ERROR`. The edit is three lines per deck. No
assertion was weakened. Each condition is the exact negation of the old one.

RED, under `pre-master-47`'s `check.sh` (identical to ours), the old decks
on `/usr/bin/ngspice` (45.2):

```
temper-res-behav-1 rc=0
temper-res-behav-noise rc=0
temper-res-behav-noise-subckt rc=0
...
Warning from checkvalid: vector const.maxerr is not available or has zero length.
Error: RHS "abs(v(a)/const.v25 - 1)" invalid
Note: maxerr = spread =
INFO: success
```

After the change:

| binary | `-behav-1` | `-behav-noise` | `-behav-noise-subckt` |
| --- | --- | --- | --- |
| 45.2 | rc=1 `-INFO: success` | rc=1 | rc=1 |
| pre-P1 fork binary (`7e7b5610a68aa28a079e`) | rc=1, maxerr 0.314112 | rc=1, maxerr 0.144911 | rc=1, maxerr 0.156474 |
| `build-ver_50` (`c1341e751ffd39377859`) | rc=0 | rc=0 | rc=0 |

The new P3 deck was written so that this cannot happen. It keeps the startup
draw in a control-language variable (`set startup = $&startup`), and the
success line is the tested branch. Its RED on 45.2 is 3 of 3 runs
`-INFO: success / +ERROR: the startup draw differs from the draw after
setseed 1`. It also fails on the pre-P1 fork binary. On `build-ver_50` it
passes 3 of 3 runs. 45.2's startup draw was `911.76`, against `886.034`
after `setseed 1`.

## Measurements used in the commit messages

Minimal decks in `scratchpad/repro/`. Before = `/usr/bin/ngspice` (45.2,
Debian `45.2+ds-1`, `25b6a73fafd9b489b923`). After = `build-ver_50/src/ngspice`.

| deck | 45.2 | patched |
| --- | --- | --- |
| P1, 125 °C, `v(a)` converted / `v(b)` plain | `7.300000e-02` / `5.694000e-02` | `5.694000e-02` / `5.694000e-02` |
| P2, `.option seed=5`, `setseed 1` then `2` | `9.312134e+02` twice, stderr empty | the same twice, and the warning once |
| P3, first draw, two processes | `9.900914e+02`, `1.084044e+03` | `8.860342e+02` twice |
| P3, TRNOISE mean, two processes | `-3.32073e-01`, `-2.47175e-01` | `-6.28075e-03` twice |
| P3, `.spiceinit` `setseed 7` | — | `9.068280e+02` twice, `$rndseed` 7 |
| P3, `.spiceinit` `unset rndseed` | — | `9.570431e+02`, `1.096830e+03` (pid seeds) |

## The two `make check` counts

Only `.cir` files changed, so nothing was rebuilt and `make install` was not
needed. The binary is unchanged.

| run | binary | PASS | FAIL | XFAIL | XPASS | SKIP | ERROR | `make` exit |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| targeted: `tests/regression/temper` | `c1341e751ffd39377859` | 7 | 0 | 0 | 0 | 0 | 0 | 0 |
| full `make check` from `build-ver_50/` | `c1341e751ffd39377859` | **353** | 0 | 0 | 0 | 0 | 0 automake results | **0** |

The count is the same 353 as follow-up 4. The single log line beginning
`ERROR:` is the known
`ERROR: (internal)  tried to destroy non-existent graph` from
`vector-probe-report.cir`, which passes.

**Lint**, from the log:

```
identity lint: 264 comparisons, baseline matches
identity lint: 11 comparisons, baseline matches
```

**Binary:** `sha256sum build-ver_50/src/ngspice | cut -c1-20` →
`c1341e751ffd39377859`, unchanged.

## For the owner

1. **What (1), `build-ver_51-shared/`, means, in plain terms.** ngspice
   ships in two forms:
   - the `ngspice` program, which you run on a deck and which `make check`
     tests;
   - `libngspice`, a library that other programs (KiCad's simulator, and any
     front end that embeds ngspice) load and drive by calling
     `ngSpice_Init`, `ngSpice_Command` and `ngSpice_Reset`.

   `build-ver_50/` builds only the program. `build-ver_51-shared/` was a
   second build of the same source as the library, made once by crew P4 to
   run one test program (`probes/rnddraws_reset_probe.c`). The ledger says you
   chose to delete it, and it is gone.

   Some of this batch's code runs **only** in the library: P3's re-seed in
   `ngSpice_Init`, P2's warning flags cleared in `totalreset`, and P4's
   draw-log reset order. It was run once, before follow-ups 1–4 changed it,
   and not since. The choice:
   - **Rebuild it (once).** That costs one extra configure and build on this
     box: minutes of CPU, some memory, about 130 MB of disk (`build-ver_50/`
     is 126 MB). A crew then
     runs the probe and the one-character probe fix (4f). That would catch a
     wrong seed or a crash that only a library user would see.
   - **Leave it gone.** It costs nothing now. A bug on those paths would
     reach KiCad-style users, and xschem if it ever links the library, with
     nothing here to catch it.

   **My call, since you asked me to make it: rebuild it once, run the probe,
   then delete it again.** The value is in that one verification run, not in
   keeping the tree. P2 and P3's upstream patches both change `sharedspice.c`.
   Upstream's main embedder is KiCad, which uses exactly those paths, so
   "verified in the library" is worth being able to say before sending. That
   is one crew, run under `ngheavy`, and it needs a one-off exception to the
   "no second configured tree" rule. The batch driver can schedule it. This
   crew was not allowed to.
2. **What is ready.** Three patches, each one commit on `pre-master-47`, each
   clean under `git apply --check`. P2 has no test, and can't have one with
   upstream's harness. The minimal deck in its commit message shows the
   change. P1 and P3 carry `check.sh` tests that ran on our binaries but not
   on a `pre-master-47` build.
3. **One thing to know before sending P1:** its tests need `let const.x`
   from `785be3194`. That is in `pre-master-47`, but not in any release up to
   45.2. If a maintainer tests on an older base, the decks now fail visibly.
   They no longer pass vacuously.
4. **The missing `REPORT.md` files.** The harness blocked them (see *What
   was made*). The full text of all three is in this crew's final response
   to the batch driver. If you want them in the tree beside the patches, as
   in the 0072 precedent, the driver or you can save that text as
   `upstream/p1/REPORT.md`, `p2/REPORT.md` and `p3/REPORT.md`. Without them,
   the commit messages and `README.md` carry the same substance, in shorter
   form.
5. **P3's behaviour change is the one an upstream maintainer may push back
   on.** Decks without `setseed` become reproducible across processes, and
   TRNOISE too. The commit message gives the opt-outs (`.option seed=random`,
   `unset rndseed`).

Nothing was pushed or sent. Two commits on `ver_51`: `f8ac6e67c` (tests) and
the docs commit with this receipt and `upstream/`.
