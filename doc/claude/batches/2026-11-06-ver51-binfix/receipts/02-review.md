# 02 — independent review of the ver_51 binning backport

Worktree /home/analog/dev/scratch/ver51-binfix, detached, base df2194696, tip 7151f36a9.
Binary under test: build/stage/bin/ngspice (2026-11-06 23:53). The 01 receipt left the build unfinished; this review ran
`ngheavy make -C build -j4` (exit 0, review-make.log, no warnings in subckt.c/inpgmod.c/inpcom.c) and
`ngheavy make -C build install` (exit 0, review-install.log).

## Verdict
No blocking issues. Two non-blocking defects, both present on ver_50 and ver_52 too (not regressions).

## 1. Commits
`git log --oneline df2194696..HEAD`:
```
7151f36a9 Mark the bin limit scan in subckt.c for the identity lint
2578c64bf Keep one card per model when a size falls to the fallback bin pass
f59023203 Accept a size on the outermost bin max, and bin hsa sizes in double
3bca7c27a Correct the limits for binning of MOS model ... Patch provided by Vilem Kledrowetz.
```
`git range-diff 5282ca5f0^..075321903 df2194696..HEAD`:
- 5282ca5f0 = 3bca7c27a, 0f763b135 = f59023203, 075321903 = 7151f36a9 (identical).
- d1d5feff5 ! 2578c64bf: the only delta is the tests/regression/model/Makefile.am TESTS context line (ver_51 has no
  bjt-qco-1 / vdmos-tj-ground* tests). ver_51: `TESTS = binning-1.cir binning-edges.cir special-names-1.cir instance-defaults.cir binning-edges-draws.cir`.
- Authors kept: 3bca7c27a Vilem <vilemk@users.sourceforge.net>, no trailer; the three fork commits Ananth, each with one
  `Co-Authored-By: Claude Opus 5.5` trailer, same as on ver_52.
- `git diff HEAD ver_52 -- src/frontend/subckt.c src/spicelib/parser/inpgmod.c src/include/ngspice/inpdefs.h`: empty.
  inpcom.c differs by one line (inp_compat G-source `1e9 m=%s`, ver_52-only bug-876 commit 826b43632), unrelated.

## 2. Code read (worktree)
- No float left in the bin path: inpdefs.h `struct card` w/l/nf are double; inp_get_w_l_x() (inpcom.c ~1034/1049/1063)
  assigns INPevaluate() without casts; subckt.c pruner limits and csl/csw are double; `grep float src/frontend/subckt.c`: none.
- Pruner calls INPbinFits() (inpgmod.c), same predicate as INPgetModBin(); fallback pass with max included only when
  no card fit (`have_bins && !bin_kept`), then keep_binners_card() dedups per base name.
- 3beb0f2a1 (typeless .model with .if) touches only inp_collect_models()/inp_chk_model_type() in inpcom.c, none of the
  bin code; 3beb0f2a1 is also an ancestor of ver_52, so the picked code was reviewed on top of it already.
  Ordering checked: inp_subcktexpand() (inp.c:942) runs before dotifeval() (inp.c:1028) — see note N1.
- Pruner csw = scale*w/nf; INPgetModBin w = w/nf*scale. Order differs, absorbed by relative is_equal (1e-9).

## 3. Tests on the new binary
| suite | result |
|---|---|
| `ngheavy make -C build/tests/regression/model check` | 5/5 PASS (binning-1, binning-edges, special-names-1, instance-defaults, binning-edges-draws) |
| `ngheavy make -C build/tests/lint check` | PASS identity.lint, PASS selftest.lint |
| `ngheavy make -C build/tests -k check -j4` (review-check-k.log) | 365 PASS, 1 FAIL: regression/pipe/resource-keyword-case.cmd |

resource-keyword-case.cmd fails the same way on the old binary
(`/home/analog/dev/ngspice/build-ver_50/stage/bin/ngspice -p < .../resource-keyword-case.cmd` →
`ERROR: lowercase 'rusage task' printed no simulator stat, line = []`). Not binning. It is the file the owner has
modified (uncommitted) in the main checkout. Every other directory passes, including xspice, the model QA (bsim3, bsim4,
bsimsoi, hisim, hicum2, hisimhv1/2) and lint. (A plain `make check` stops at pipe/; -k was needed to reach the rest.)

### binning-edges.cir, no-mode vs hsa (edges-new.log)
All 12 in-table sizes give the wanted card in mode 1 (no mode), mode 2 (hsa literal) and mode 3 (hsa expression):
card.1 card.3 card.2 card.3 card.2 card.4 card.1 card.2 card.2 card.1 card.4 card.4. The 2 out-of-table sizes
(l=25, l=20.2005) give "could not find a valid modelname" in all three modes. 0 of 14 sizes disagree between modes.

### bugs/889 shape: nf finger on an inner w edge under hsa (decks/nf-inner-edge.cir, nf-inner-edge.result)
Bins w [0.42,0.65) card.1, [0.65,7) card.2, [7,100] card.3, l=0.5. Sizes chosen so float32 scale*w/nf lands one ulp under the edge.
| w / nf | hsa new | hsa build-ver_50 |
|---|---|---|
| 1.3/2, 2.6/4, 5.2/8 (0.65 edge) | card.2 | card.1 (wrong) |
| 49/7 (7 edge) | card.3 | card.2 (wrong) |
| 2.1/5, 4.2/10 (0.42 bottom edge) | card.1 | could not find a valid modelname |
| 200/2 (100 top edge) | card.3 | could not find a valid modelname |
In no mode wnflag defaults to 0 (no division by nf), so 2.1 and 4.2 go to card.2 and 200 has no card, on both binaries. That is correct for that mode.

### sky130 L=2 inner edge (decks/sky130_l2_edge.cir, from the peer collateral, sky130.result)
plain and hsa both: xn5 nlowvt_model.3, xn6 nlowvt_model.27, xp13 plowvt_model.3;
id = 1.150472795668e-04, 2.335770639790e-05, 3.164499697235e-05. Identical to the peer's sky_ver52_* rows.

## 4. Non-blocking notes
N1. keep_binners_card() vs .if inside a subckt, on the top edge (decks/if-branches-top-edge.cir, if-branches.result).
    The subckt holds the same bin cards in `.if (corner==1)` and `.else` branches. Size l=20.2 (the table's lmax) under hsa
    takes the fallback pass, and keep_binners_card() then keeps one card.2 across both branches. The tie rule (`>=`)
    keeps the earlier-parsed card, which is in the dead .if branch. dotifeval() runs after the subckt is expanded and
    drops that card, so the live card is lost: `could not find a valid modelname`. In no mode the same deck gives
    x1:card.2 vth0=0.7. The inner size l=10 works in both modes (no dedup is done there). ver_52's build-ver_52 binary and
    build-ver_50 fail the same way, so this is not a regression from the backport (before the fix a top-edge size failed
    regardless). It is worth a follow-up on ver_52: dedup only within one .if branch, or after dotifeval.
N2. The hsa pruner always divides w by nf. INPgetModBin() respects wnflag=0 (instance or option). decks/hsa/wnflag0.cir
    (instance wnflag=0, x1 w=4 nf=2) gives `could not find a valid modelname` under hsa on the new binary, ver_52 and
    ver_50 alike. This was there before; the picks do not change it.
N3. The 01 receipt recorded the build and GREEN as not run. They are run here. stage/ was installed by this review.
