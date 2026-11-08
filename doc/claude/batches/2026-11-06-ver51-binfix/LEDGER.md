# Ledger: MOS bin-edge fix backported to ver_51 (2026-11-06)

**Requested by:** the xschem-private-c7 session, on the user's behalf. Collateral is at `~/private/xschem-private/doc/claude/worst_case_plan/ngspice_bin_edge/README.md`.

**Defect:** on ver_50 and ver_51, a MOS size exactly on an inner bin edge gets the lower card without a compatibility mode and the upper card under hsa. On sky130 at L=2u this is `nlowvt_model.4/.28` against `.3/.27`, found in the bandgap's lvt FETs.

## Driver's call

**Backport to ver_51, don't promote ver_52.** The fix is four commits. ver_52 brings about 114 more, which haven't been tried for daily use, among them R1 (diode op 16 s to 218 s). ver_51 is also the rebuild the user was already waiting on for `3beb0f2a1`.

## Rows

| # | crew | result | receipt |
|---|------|--------|---------|
| 01 | backport, in a detached worktree off `df2194696` | Picks: `3bca7c27a` (from 5282ca5f0, upstream 03ef19244), `f59023203` (from 0f763b135), `2578c64bf` (from d1d5feff5), `7151f36a9` (from 075321903). The only conflict is the `tests/regression/model/Makefile.am` TESTS line, because ver_51 lacks ver_52's bjt-qco and vdmos-tj tests. subckt.c, inpgmod.c and inpdefs.h are byte-identical to ver_52. inpcom.c differs by one line, from 826b43632 (bug 876, only on ver_52, not binning). RED on build-ver_50: binning-edges.cir and binning-edges-draws.cir both FAIL. This crew's build was cut off, and the reviewer finished it. | receipts/01-backport.md |
| 02 | independent review | No blocking issues. 365 PASS and 1 FAIL (resource-keyword-case.cmd, committed version, fails the same way on build-ver_50). bugs/889 nf shapes bin correctly under hsa. sky130 at L=2: no mode and hsa agree. | receipts/02-review.md |
| 03 | driver | `git merge --ff-only 7151f36a9` on ver_51 in the main checkout. The owner's staged CLAUDE.md and modified resource-keyword-case.cmd are untouched. | (this ledger) |
| 04 | build `build-ver_51` | autogen, configure (same flags as build-ver_50), make -j4 and install all rc 0. `make -k check` gives **366 PASS, 0 FAIL**, with resource-keyword-case.cmd run with the owner's working-tree edit. binning-edges: 0 of 42 on the wrong card and 0 of 14 disagreeing (ver_50 had 20/42 and 11/14). sky130 L=2: both modes give `nlowvt_model.3/.27` and `plowvt_model.3`, with identical Id. | receipts/04-build.md |

## Binary

- `/home/analog/dev/ngspice/build-ver_51/stage/bin/ngspice`, sha256 `ae06c0fb831fa7e9188ff3add256ebc3b210be8d1ff16f5d834a7c3acde56416`. The driver checked this hash.
- `build-ver_50` is unchanged, sha256 `c2bd1098ce5ff7e539f0c1c20c475ea4783a4694d45822875b6b65747474ab8e`. xschem stays on it until the user switches.

## Open, pre-existing (on ver_50, ver_51 and ver_52; follow-ups, not regressions)

- **N1:** a size on the outermost max under hsa, with the same bin cards in `.if`/`.else` branches, gives "could not find a valid modelname". `keep_binners_card()` dedups before `dotifeval()` and keeps the dead-branch card.
- **N2:** the hsa pruner always divides w by nf and ignores `wnflag=0`, so `w=4 nf=2 wnflag=0` loses its card.

## Not done

- Nothing was pushed. The user pushes `ver_51` themselves.
- Upstream: nothing new to file. The bugs/889 reply and patch from 2026-11-04 are still unposted, and posting them is the user's call.
