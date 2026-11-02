# Receipt 02: group 02, binning

`03ef19244` as a clean pick breaks legal sky130 sizes, so it landed with a fork fix. Both are on `ver_52`.
All heavy commands went through `ngheavy`. No exit 137.

| commit | on `ver_52` | what |
|---|---|---|
| `03ef19244` | `5282ca5f0` | upstream: `min <= x < max`, relative tolerance. Clean `cherry-pick -x` |
| fork | `0f763b135` | accept the outermost max; prune hsa sizes by the same rule, in double |

Binaries compared: **old** is `ver_52` before the pick (`inpgmod.c` from `0b1f8c12b`), **pick** is the pick alone,
**fix** is the fix on top. Old and pick were saved as separate binaries, so the only source difference is the commit named.

## What the pick did on its own

**Result is a hard load error, never a wrong model:** `could not find a valid modelname`, "circuit not parsed", exit 1.
Nothing ever bound to a wrong bin. Two groups of legal sizes break.

**1. Size on the outermost max.** Found by the triage, confirmed here. No hsa and hsa with an expression size used to load;
the pick rejects them. hsa with a literal size already failed before the pick (see below).

| `nfet_01v8`, one bin edge at a time | no hsa | hsa, literal | hsa, expression |
|---|---|---|---|
| L=20.2, W=2 | `.25` -> ERROR | ERROR -> ERROR | `.25` -> ERROR |
| L=1, W=1010 | `.5` -> ERROR | ERROR -> ERROR | `.5` -> ERROR |
| L=20.2, W=1010 | `.1` -> ERROR | ERROR -> ERROR | `.1` -> ERROR |

**2. Not in the triage, and the more common one: a literal multi-finger size under hsa, on an inner edge.**

| `nfet_01v8` L=1, hsa, literal | old | pick | fix |
|---|---|---|---|
| w=15 nf=5 (finger 3 um) | `.28` | ERROR | `.20` |
| w=21 nf=7 (finger 3 um) | `.28` | ERROR | `.20` |
| w=49 nf=7 (finger 7 um) | `.12` | ERROR | `.4` |
| w=3.75 nf=5 on `nfet_g5v0d10v5` (finger 0.75 um) | `.53` | ERROR | `.46` |
| w=2.1 nf=5, w=4.2 nf=10 (finger 0.42 um) | ERROR | ERROR | `.44` |

Cause: under hsa, `subckt.c` `doit()` prunes the cards of a literal size before binning, in **float**. `(float)scale * 15 / 5`
is 2.99999988e-06 and `(float)3e-6` is 3.00000011e-06, so only the lower card stayed. The old rule took that card (size on
its max). The pick rejects it, and the upper card is gone. Scanning, by a script that repeats the float arithmetic, all 11 binned sky130 FET files, nf 1 to 40: 234 of 2755
edge/nf pairs hit this, on finger widths 0.7, 0.75, 3 and 7 um, and 45 more on 0.42 um failed already. For `nfet_01v8` and
`pfet_01v8`: finger 3 um at nf = 5 7 10 13 14 17 20 21 25 26 27 28 33 34 35 40, and finger 7 um at nf = 7 11 14 17 19 22 25 28
31 34 37 38. With nf=1 no edge is affected.

The same half-open float test is why **hsa with a literal size at the outermost max failed long before this pick**: L=20.2 or
W=1010 removes every card. That path is what the triage's check missed: it checked `rem_unused_mos_models` in `inp.c`, which is
behind `#ifdef REM_UNUSED`, and nothing defines that, so it is dead code. `doit()` is the live pruner.

## The fix, `0f763b135`

RED first, new `tests/regression/model/binning-edges.cir`: its own 2x2 card table with sky130's edges, in sky130's order, inside a subcircuit,
14 sizes in 3 modes, each size a separate circuit. The script's `wanted card.K` is written next to what `showmod` reports. In `tests/bin/check.sh`:
pick exit 1, 18 of 36 legal cases `could not find a valid modelname`; old exit 1 (hsa literal top edge, nf rounding); fix exit 0, 36 of 36
as wanted and the 6 outside-the-table cases rejected.

A first version of the fix took the first card with the size on its max. The sky130 runs showed `L=1 W=1010` staying on `.5` while
`L=1 W=1009.999` went to `.4`: an inner edge in one axis going to the lower card. The fix now prefers the upper card, and the test was reordered and got `l=1 w=1010`,
`l=20.2 w=3`. With only that preference taken out (a scratch build), the test fails: `card.4` becomes `card.2` and `card.3`.

    mode l w nf            wanted    old    pick   fix
    1    l=20.2 w=1        card.3    .3     NOBIN  .3
    1    l=0.5 w=1010      card.2    .2     NOBIN  .2
    1    l=20.2 w=1010     card.4    .4     NOBIN  .4
    2    l=20.2 w=1        card.3    NOBIN  NOBIN  .3
    2    l=0.5 w=15 nf=5   card.2    .1     NOBIN  .2
    2    l=0.5 w=2.1 nf=5  card.1    NOBIN  NOBIN  .1
    3    l=20.2 w=1010     card.4    .4     NOBIN  .4
    3    l=1 w=1010        card.4    .2     NOBIN  .4
    all  l=25 w=1          card.0    NOBIN  NOBIN  NOBIN
    1,3  l=20.2005 w=1     card.0    .3     NOBIN  NOBIN

(mode 1 no compatibility mode, 2 hsa literal, 3 hsa expression.)

Changes:

- `inpgmod.c`: the half-open rule stays. If no bin holds the size, `INPgetModBin()` takes one with the size on its max, the highest in L and W
  if there are two, so an inner edge in the other axis still goes up. A size on an inner edge in both axes finds its upper bin in the first pass. New `INPbinFits()` holds the rule.
- `subckt.c`: the pruner uses `INPbinFits()` in double, so it keeps exactly the card the binner will take. `card.w/l/nf` become double
  (`inpdefs.h`, `inpcom.c`). If no card fits it starts again from a fresh copy with the max included. I did not widen the first pass,
  because a kept card's `agauss` calls would move the draw stream for every size on an edge.

**Kept out on purpose:** sizes up to 1 nm outside the table stay rejected (`L=20.2005`, `L=0.1495`, `W=0.4195`, `W=1010.0005`). The
old rule took them with no hsa or with an expression. They are outside the PDK's box, the error is clear, and the task asked for the
outermost max only. A deck relying on that now stops with the error.

## Every outer edge, measured with the fix

`nfet_01v8`, `pfet_01v8`, `nfet_03v3_nvt`, `nfet_05v0_nvt`; 16 sizes each; no hsa and hsa; literal and expression; tt. 512 runs, plus 96 on
fingers. Corners and combinations L/W min and max, 1 nm inside, 0.5 nm outside. `set ng_nomodcheck` on all: without it, L=25.25 on `nfet_05v0_nvt` and
L=20.2 W=0.42 on `pfet_01v8` stop on the model check in old and in the fix alike (`pdits`, `Pclm`), which has nothing to do with binning.

- All 8 top and bottom edge sizes load in all 4 modes, on all 4 devices. Examples for `nfet_01v8`, no hsa, old -> fix: L=20.2 W=2 `.25` -> `.25`;
  L=20.2 W=1010 `.1` -> `.1`; L=1 W=1010 `.5` -> `.4` (L=1 is an inner edge, so it goes up like any other).
- The only `ERROR`s left: the 0.5 nm outside sizes (expected), and `W=2*wmax nf=2` with no hsa, where wnflag=0 makes it a real
  out-of-table width in old as well. `nfet_03v3_nvt` at L=1 in the finger set is my own bad choice: its table stops at L=0.908.

## Bias and corners: the triage's "results do not move" holds only in part

Decks use expression sizes (`l={0.5*1}`) and no hsa, so old takes the lower card and the pick takes the upper. 9 devices, all
edge sizes (63 for `nfet_01v8`), 5 biases (sat, on, weak, linear, body bias), 7 corner and temperature runs: 126 decks. Worst relative
change in `id`:

| device | tt 27 | tt -40 | tt 125 | ss | ff | sf | fs |
|---|---|---|---|---|---|---|---|
| `nfet_01v8` | 7.0e-03 | 9.5e-03 | 5.0e-03 | **6.6e-01** | **4.1e-01** | 6.8e-03 | 7.2e-03 |
| `pfet_01v8` | 7.2e-04 | 1.5e-03 | 2.0e-03 | **2.7e-01** | **3.4e-01** | 1.1e-03 | 4.4e-04 |
| `nfet_01v8_lvt` | 1.9e-03 | 2.5e-03 | 1.3e-03 | **2.4e-01** | **2.0e-01** | 1.7e-03 | 2.1e-03 |
| `pfet_01v8_lvt` | 4.3e-03 | 6.1e-03 | 2.9e-03 | **1.2e-01** | 5.7e-02 | 4.5e-03 | 4.0e-03 |
| `pfet_01v8_hvt` | 3.1e-04 | 4.1e-04 | 2.2e-04 | **1.0e+00** | **7.9e-01** | 3.1e-04 | 3.0e-04 |
| `nfet_g5v0d10v5` | 2.7e-03 | 3.4e-03 | 2.2e-03 | 4.3e-02 | 3.7e-02 | 2.5e-03 | 3.0e-03 |
| `pfet_g5v0d10v5` | 1.7e-04 | 7.8e-04 | 4.8e-04 | **2.2e-01** | **1.3e-01** | 1.8e-04 | 1.7e-04 |
| `nfet_03v3_nvt` | **5.9e-01** | **7.9e-01** | **4.0e-01** | **6.4e-01** | **5.5e-01** | **4.5e-01** | **7.5e-01** |
| `nfet_05v0_nvt` | 7.9e-04 | 7.9e-04 | 7.9e-04 | 6.4e-02 | 4.2e-02 | 7.9e-04 | 7.9e-04 |

So **the cards step at bin edges in ss and ff, and on `nfet_03v3_nvt` at L=0.6 at every corner.** Examples: `nfet_01v8` ff, weak, L=0.25
W=0.55, id 9.446e-08 -> 5.593e-08, `.47` -> `.38`. ff, `id` moves more than 1% in 93 of 315 points. `nfet_03v3_nvt` tt, weak, L=0.6 W=1: id 1.596e-05 -> 6.487e-06,
`.6` -> `.3`. `gm`, `gds`, `gmbs` move about as much, and `cgd` flips sign at some edges where it is tiny. tt, sf and fs elsewhere stay below 1%, but above what the triage printed: `pfet_01v8` at inner edges away from the top is as low as 1.6e-10, and the mixed top and inner edge sizes reach 2e-3.

**Who sees this:** only expression or `no hsa` sizes on an edge. It is the pick's stated purpose: the same size now gets the same card however it
is written. Measured with hsa, 3 devices, tt, ss, ff (48 or 8 non-top-edge sizes, 5 biases):

| comparison | bins differ | worst relative change, all 7 quantities |
|---|---|---|
| hsa literal: old against fix | 0 | **0 exactly** |
| fix: literal against expression | 0 | 5.3e-12 |
| old: literal against expression, `nfet_01v8` ff | 47 of 48 | 1.1 (`cgd`); up to 2.0 on `nfet_03v3_nvt` |

So a literal size at nf=1 under hsa, which is what xschem writes, does not move at all. **The old binary disagreed with itself by up to 41% in id at ff
for one size, depending on how it was spelled.** The fix removes that.

I don't know which card is physically right at an edge where they differ. The rule picks the upper one. Before, a literal hsa run also picked it.

## Draw log: which keys change

| case | key change | draws |
|---|---|---|
| hsa, literal, nf=1, size that loaded before | none (48 sizes, `drawlog_sky130`, 3 more seeds on 4 devices) | bit-identical to 45.2 |
| hsa, literal, finger width on an inner edge, the nf lists above | `xm1:nshort_model.29` -> `.21` for w=15 l=0.5 nf=5. 3 rows of 17 | values identical; the next device identical; only the key and device's card change |
| hsa, literal, top edge or finger 0.42 um | was a load error; new keys | new |
| expression sizes | rows unchanged, 589 rows bit-identical across old and fix, because all cards stay in the deck | which card's draw the device uses changes |

For expression sizes the card a device uses is what moves, listed per size for all 9 devices in `measure/after-02/binkeys.tsv`. Changed count of edge sizes: 59 of 63 on
`nfet_01v8`, `pfet_01v8`, `pfet_01v8_hvt`; 50 of 54 `nfet_01v8_lvt`; 52 of 56 `pfet_01v8_lvt`; 68 of 72 `nfet_g5v0d10v5`; 60 of 64 `pfet_g5v0d10v5`; 11 of 15 `nfet_03v3_nvt`; 20 of 24
`nfet_05v0_nvt`. The rule: every edge size moves to the upper card except the 4 corners of the table. That file comes from the rule applied to the card table, in a script; it matched
`showmod` on all 435 sizes of the 7 non-native devices. `showmod` cuts long model names, so the native devices are from the script alone, plus the 12 native sizes in the edge matrix. Under `tt_mm`, expression sizes at 3 edges, seed 1: vth moves
at most 2.3e-9 V, so that corner's card difference is small there.

## Gates

- `make` 0, `make install` 0. Full `make -k check`: **365 PASS, 1 FAIL, 0 SKIP**. One more PASS than group 01, the new test. The FAIL is still
  `resource-keyword-case.cmd`, the baseline's named exception. Plain `make check` still aborts in that directory. `identity.lint`, `selftest.lint` PASS; `identity.baseline`
  untouched; no existing `.out` changed.
- `measure/run.sh`: exit 0, `mc_r` and `mc_sky130` `identical_to_ref = yes`. Saved in `measure/after-02/` (124 KB). Against `after-01-fix`:
  **TOTAL changed 0** at 1e-9, `mc_*` and `drawlog_*` exact. Against `baseline`: 73, the same 73 as group 01. The suite cannot see this group because all its sizes are
  hsa, literal, nf=1 (or nf=4 with a non-edge finger). That is the reason, not that nothing changed: see the table above.

So every changed quantity in the suite stays attributed to group 01, and the tables here attribute to `03ef19244`.

## Unverified

- Other PDKs. Sky130's tables tile exactly (checked over the 11 binned FET files, no gaps or overlaps). A PDK with a gap under 1 nm between bins was accepted by the old absolute tolerance. Now it fails
  to load or falls to the lower card. Not tested, no such PDK at hand.
- `nfet_g5v0d16v0` and `pfet_g5v0d16v0` (wrapper subcircuit around `_base`), `spe` mode, and `altermod` through `spiceif.c`, which takes the new fallback: `make check`'s rebin tests pass, no sky130 run.
- Which card is physically right at an edge, see above. `pfet_01v8_hvt` at ss L=0.5 W=0.42: vth 0.859 -> -0.184 between `.46` and `.45`. It looks like a card problem. Not looked into.
- 63 FETs in parallel fail one bias of 5 on `nfet_01v8` ss (old and fix alike) and `pfet_01v8_hvt` ff (old only). Those 441 points are out of the tables. I did not find out why that one deck got better.
- With hsa and `.options wnflag=0`, the pruner still divides by nf and the binner doesn't, from before this group. Read in code, not run.
- `ngSpice_Reset`: no static state added; the shared build not run.
