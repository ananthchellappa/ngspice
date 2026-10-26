# Upstream material for ver_51 items P1, P2 and P3

Three patches for an upstream maintainer, one directory per item, each with a
`REPORT.md` to read before the patch. P4 (the `rnddraws` draw log) is not
here: it is fork work agreed with xschem, not an upstream fix.

| dir | patch | what it fixes | files |
| --- | --- | --- | --- |
| `p1/` | `0001-inpcom.c-carry-the-R-model-through-the-R-B-rewrite.patch` | an R rewritten into a B source loses its `.model` (TC, tnom) and its `dtemp`/`temp` | `inpcom.c`, 9 test files in `tests/regression/temper/` |
| `p2/` | `0001-randnumb.c-warn-once-when-.option-seed-overrides-set.patch` | `.option seed=` silently overrides `setseed` on every `reset` | `randnumb.c`, `randnumb.h`, `inp.c`, `sharedspice.c` |
| `p3/` | `0001-wallace.c-do-not-re-seed-from-the-process-id-at-star.patch` | the startup random stream comes from the process id while `$rndseed` reads 1 | `wallace.c`, `main.c`, `sharedspice.c`, one test in `tests/regression/misc/` |

Each patch is one commit generated with `git format-patch` against
`origin/pre-master-47` (`c5cd68015`). Its `index` lines name the
`pre-master-47` blobs. Each squashes the item's code and test commits on
`ver_51`, including this follow-up batch's fixes, with the fork-only parts
removed. Each report's last section lists what was removed.

**Checked:** each patch passes `git apply --check` on a clean checkout of
`pre-master-47`, on its own and all three together. P2 and P3 both touch
`src/sharedspice.c`, in different hunks, and they apply in either order. Each
patched C file was also compiled with `gcc -fsyntax-only` against
`pre-master-47`'s headers. The warning count matched the unpatched file's in
every case.

**Not checked:** none of the patches was built or run on `pre-master-47`.
The behaviour in the reports was measured on `ver_51`
(`build-ver_50/src/ngspice`, sha `c1341e751ffd39377859`), where the same code
passes the full suite. The "before" numbers come from the distribution's
`/usr/bin/ngspice` (ngspice-45.2, Debian `45.2+ds-1`). The code is the same
logic as on `ver_51`. The only changes are where upstream differs: the
2-argument `model_name_match()` and `*line == 'r'`, because upstream lowercases
the deck.

Each patch's commit message has the problem, a minimal deck with the
before/after numbers, the fix and the tests. Read it with `git am`, or as the
head of the `.patch`. The upstream reports for this material were written as
`p1/REPORT.md`, `p2/REPORT.md` and `p3/REPORT.md`, but this session's harness
refused those files. Their text is in the crew's final response to the batch
driver (receipt `../receipts/05-upstream-prep.md`, "For the owner" item 4).
What each patch leaves out is below.

## What each patch leaves out, compared with `ver_51`

**P1.**
- `tests/regression/exitstatus/behav-r-model-warn.*`. That deck asserts the
  five new warnings on stderr under `tests/bin/check_status.sh`, a downstream
  harness. Upstream's `check.sh` never captures stderr, and its filter deletes
  `Warning` lines.
- The fatal-exit hook (`behav_r_models_root`, freed in
  `inp_netlist_read_reset()`). That function exists only in the fork. Without
  the hook, a `controlled_exit()` inside `inp_compat()` in `libngspice` leaks
  the model lists. Upstream already leaks the scope tree and the deck on that
  path.
- The fork's case-sensitive spellings. The fork's
  `model_name_match(a, b, !inp_case_exact_ids())` becomes upstream's
  `model_name_match(a, b)`, and `elem_letter(line)` becomes `*line`.
- One comment's reference to the fork's batch ("decision 4"). The code has no
  identity-lint annotations to remove.

**P2.**
- All of its tests. That is `tests/regression/rng/` (the `option-seed-*`
  decks), plus its `configure.ac` and `tests/regression/Makefile.am` lines and
  `check_status.sh`. They assert stderr, which no upstream harness can see.
- The fork's draw-log (P4) code in the same functions. That is the seed record
  and call counter in `randnumb.c`, the `rng_*` declarations, `drawlog.h` and
  the `ng_drawlog_*` calls.
- `eval_opt()`'s comparisons are left as upstream spells them (`strstr`/`eq`;
  the fork has `cistrstr`/`eqc`).

**P3.**
- `startup-equals-setseed1.cir`, which reports through its exit status under
  `check_status.sh`. It is replaced by `tests/regression/misc/startup-seed.cir`
  in `check.sh` form. That deck keeps the first draw in a control-language
  variable, so it also runs on 45.2, and fails there for the right reason.
- `setseed7-bit-identity.cir`, a glibc-only guard that the fork skips through
  a `.requires` file only `check_status.sh` reads, and that `.requires`
  support itself.
- The fork's P4 hunks in `sharedspice.c`.

## Test results behind the patches

| deck | ngspice-45.2 | fork, pre-patch (`7e7b5610a68aa28a079e`) | fork, patched (`c1341e751ffd39377859`) |
| --- | --- | --- | --- |
| P1 `temper-res-behav-1.cir` | FAIL (no const-plot `let`) | FAIL, maxerr 0.314 | PASS |
| P1 `temper-res-behav-noise.cir` | FAIL | FAIL, maxerr 0.145 | PASS |
| P1 `temper-res-behav-noise-subckt.cir` | FAIL | FAIL, maxerr 0.156 | PASS |
| P3 `startup-seed.cir` | FAIL, 3 of 3 runs | FAIL | PASS, 3 of 3 runs |

All of these runs used `pre-master-47`'s own `tests/bin/check.sh`, which is
byte-identical to the fork's. The P1 decks need `785be3194`
(`let plot.vec`), which `pre-master-47` has and 45.2 lacks.

**Nothing here has been sent.** No mail, bug report or merge request exists
for any of it. Whether to send it is the owner's decision.
