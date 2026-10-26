# Upstream material for ver_51 items P1, P2 and P3

Three patches for an upstream maintainer, one directory per item. P4 (the `rnddraws` draw log) is not
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

Each patch's commit message is its report: the problem, a minimal deck with
the before/after numbers, the fix and the tests. Read it with `git am`, or as the
head of the `.patch`. There are no separate `REPORT.md` files. The harness
refused the crew's writes of them, and their text was not kept anywhere else.

**Library paths not run.** P2 and P3 both change `src/sharedspice.c`, which is
compiled only in a `--with-ngshared` build. Those hunks were syntax-checked, not
run. Before sending P2 or P3, do a one-off library build and run
`../../2026-10-24-mc-v51/probes/rnddraws_reset_probe.c` against it.
