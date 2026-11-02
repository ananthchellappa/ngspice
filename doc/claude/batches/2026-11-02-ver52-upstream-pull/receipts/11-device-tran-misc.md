# Receipt 11: group 11, device-tran-misc

Fork-written, on `ver_52`, all heavy commands through `ngheavy`, `-j4`, no exit 137.
"before" is this tree's binary at `ee66bc35f`, kept in scratch. "ver_51" is `build-ver_50/stage/bin/ngspice`, run only.

| item | commit | test in `make check` |
|---|---|---|
| R4 VDMOS Tj on ground | `3301d5cae` | `model/vdmos-tj-ground.cir`, `-soa.cir` |
| minbreak default | `6fa5af6cc` | `misc/minbreak-default.cir`, `minbreak-pwl-corners.cir` |
| fallback pruner | `d1d5feff5` | `model/binning-edges-draws.cir` |
| lint mark for the pruner (PLAN rule 3) | `075321903` | `lint/identity.lint` |
| seed warning | `15e53439c` | `rng/option-seed-control-netlist.cir`, `-random`, `-unusable` |
| `--disable-xspice` | `64ac2420b` | scratch build, below |

## Gates

- `make` 0, `make install` 0, installed binary `cmp`-equal to the built one.
- `make -k check`: **392 PASS, 1 FAIL, 0 SKIP**. The FAIL is the named `resource-keyword-case.cmd`. 385 after group 10 + 7 new decks. `identity.lint` passes, `identity.baseline` untouched. It failed in the first full run on the new `cistrstr(line, key)`; the mark is its own commit.
- The two `ERROR:` lines in the log are not test results: `rusage task` is that FAIL's own message, and `(internal) tried to destroy non-existent graph` is also in group 10's logs.
- `measure/run.sh`, `measure/after-11/`. `compare.sh after-10-fix after-11 0 0`: **TOTAL changed: 0**, so nothing to attribute. `mc_r`, `mc_sky130` identical to 45.2: yes.
- No existing `.out` changed. One existing `.err` changed, on purpose: see the seed section.

## R4: VDMOS Tj on ground (`u14`)

**What a grounded Tj means.** Thermal nodes hold degrees C: `VDMOSload` reads `Tj = rhsOld[tempNode]`, and `TjK = Tj + CONSTCtoK`. So node 0 is a junction held at 0 C by an ideal sink. `VDMOSsetup` agrees: it replaces only `-1` with an internal node and treats 0 as connected, still making `cktTemp` and `VdevTemp`. `vdmosconv.c` also still treats it as self-heating. `b3594401c`'s `VDMOSselfheat()` requires `tempNode > 0`, so `VDMOSload` skips all thermal stamps and the `cktTemp` row is empty: singular.

**Fix.** `tempNode >= 0`. `-1` is still rejected, for an instance setup has not seen. Everything else from `b3594401c` stays.

Measured, `numdgt=12`, u14's circuit, AC magnitude at 1 kHz, numeric = `dv(c)/dvg` over 1e-4 V:

| form | ver_51 v(c), AC | before v(c), AC | after v(c), AC | numeric, after |
|---|---|---|---|---|
| Tj grounded, Tc node | 3.500527434781, 8.038966415224 | **4.245122343589, aborted, singular** | 3.500527434781, 8.038966415224 | 8.03917964799 |
| Tj and Tc grounded | same | **4.245122343589, aborted, singular** | 3.500527434781, 8.038966415224 | 8.03917964808 |
| Tj held by a 0 V source | same | 3.500527434781, 8.038966415224 | same | 8.03917964799 |
| no thermal network, `temp=0` | same | 3.500527434781, 8.038966415224 | same | 8.03917964808 |

v(c) matches the quoted 3.50052743. AC does not match the quoted 8.03896704 past 7 digits: ver_51 itself gives 8.03896642 on this deck at 1 kHz, and after equals ver_51 to all 12 digits. The quoted figure's deck or frequency was not recorded. The physical check is that the four forms are one device at 0 C and agree to all 12 digits. AC is 0.003 % from the numeric slope. Also:

- `id` 6.499472565205 and `v(c)` sum to 10 V / 1 ohm. They did not before.
- Case node with Tj grounded: `v(tc)` 0.8709677. By hand: 27 * 0.1 / (0.1 + 2 + 1) = 0.870968.
- Tc grounded alone (always worked): `v(tj)` 11.83432 equals Rthjc * P = 0.5 * 3.84615 * 6.15385. AC 7.6632 is `b3594401c`'s, 7.6082 on ver_51; not touched.
- p-channel mirror: `-3.50053`, AC 8.038966. KLU: the same digits. `bypass=1`, both grounded: v(c) the same, AC 8.038966418. `--disable-xspice` scratch binary: the same at 7 digits.
- `tran` to 20 us: grounded against held by source max difference 2e-14 V, against `temp=0` 1.5e-14 V.

**SOA, a change the macro alone would cause.** `vdmossoachk.c` takes power from the thermal network when `VDMOSselfheat`. A grounded Tj takes the heat past Rthjc, so that reads near zero. With the macro alone `.option warn=1`, `pd_max=10` printed nothing at 22.75 W. The power is now reconstructed from voltages for that instance: `Pd=22.75 W at Vd=3.501 V and Te=0 C`. 3.5005 * 6.4995 = 22.75.

**Open, not touched:** Tj held by a 0 V *source* has this same silent SOA check, on `before` and after. It is upstream's. ver_51 printed nothing either, as its `pd_max` was overwritten by the copy-paste bug `b3594401c` fixed.

RED, `check.sh`, before binary:
```
-v(c1) = 3.500527e+00
-v(c2) = 3.500527e+00
...
-op: grounded and held at 0 V and temp=0 agree
+v(c1) = 4.245149e+00
+v(c2) = 4.245149e+00
+op: grounded and held at 0 V and temp=0 differ
-vm(c1) = 8.038966e+00  (all six lines missing: "doAnalyses: matrix is singular")
+tran: grounded and held at 0 V and temp=0 differ
+a = 8.746776e-01
```
```
-Instance: m1 Model: ms Pd=22.75 W at Vd=3.501 V and Te=0 C has exceeded Pd_max=10 W
+Instance: m1 Model: ms Pd=24.66 W at Vd=4.245 V has exceeded Pd_max=10 W
```

## Minbreak

Unset is 0 again in `CKTnewTask`, which still copies the option (bug 888). The default is `MAX(1e-15, 10*delmin)` in `dctran.c`.

**Beyond the one formula, each for a stated reason:**

- The other readers of 0 keep their old formula with the same floor, `CKT_MINBREAK_FLOOR`: `dctran.c` without XSPICE and on resume (`maxStep*5e-5`), `dcpss.c` (three sites), `optran.c` on resume. So they match ver_51 instead of a flat 1e-15.
- An `op` job under `optran` sets none, so it takes the floor. A tran job has already set it in `dctran.c`.
- In `dctran.c` the default is now set before `add_bkpt()`, so that shared-library path keeps the pick's floor instead of seeing 0. **Not built or run**: no shared build in this batch.

| deck | ver_51 | before (flat 1e-15) | after |
|---|---|---|---|
| reviewer's `h`, PULSE `.tran 1m 1000`, timepoints | 1439373 | 1345038 | **1439373** |
| `c`, `.tran` to 100 | 131252 | 129088 | 131252 |
| `d`, to 1e4 | 13410 | 13176 | 13410 |
| `a b e f g` | unchanged on all three | | |

Edge misses, reviewer's `edge.py`, end-of-rise corner not hit, default minbreak:

| deck | edges | ver_51 | before | after |
|---|---|---|---|---|
| `s2` `.tran 100u 20` | 2561 | 0 | 3 | **0** |
| `s1` `.tran 1m 100` | 1285 | 270 | 297 | **270** |
| `s4` `.tran 1m 1000` | 128 | 23 | 12 | **23** |
| `s3` `.tran 10u 5` | 6306 | 0 | 0 | 0 |

**Not every row improves.** `s4` was better under the flat 1e-15, 12 against 23, the "mixed direction" the reviewer saw. The restored default is ver_51's behaviour and its 270 and 23 misses, not a cure. These decks' default equals the same deck with an explicit 1e-13 or 1e-14: 162254, 310708 and 1005053 points, misses 270, 0, 23.

Bugs, `vecmax(time)`:

| deck | ver_51 | before | after |
|---|---|---|---|
| 887 | stops, 1.000e-03, `timestep = 0` | 1.002e-03 | 1.002e-03 |
| 888a, `tran` command, `.option minbreak=1e-15` | stops | 1.002e-03 | 1.002e-03 |
| 888b, `.tran` and `run` | 1.002e-03 | 1.002e-03 | 1.002e-03 |

Why 887 needs the floor: max step 4n gives 10*delmin = 4e-19, below the floor. `--disable-xspice` scratch binary on 887: 1.002e-03.

**Reporting.** `option minbreak=` in `.control` takes effect: 162254 points by default and with 1e-13, 150370 with 1e-15. On ver_51 the last did not change the run. The bare `option` printout has **no minbreak line**, on ver_51, before and after; it is not askable either (`IF_SET` only). Its `delmin = 1e-20` for `tran 1n 1u` is true (1e-11 * 1n) on all three. I did not add a line.

`optran` deck (2096 timepoints) and a PSS deck (565, the same non-convergence message) are the same on all three. Two PSS examples printed nothing my filter matched, so they show nothing either way.

RED, `check.sh`, before binary:
```
-default: the same run as minbreak=1e-13
-option: minbreak=1e-15 is honoured and gives another run
+default: not the same run as minbreak=1e-13
+tran1.tran1.ndef = 1.503700e+05
+tran2.tran2.nold = 1.622540e+05
+option: minbreak=1e-15 did not change the run
```
`minbreak-pwl-corners` is green on the before binary, being a guard for the pick. It fails on ver_51: `last = 1.000000e-03`, `Timestep too small`.

**`tests/transient` is not in `make check`** (`DIST_SUBDIRS` only). I first put the decks there, and saw `fourbitadder.cir` fail. It fails with identical rows on ver_51, before and after (same md5 of the data rows), against its old committed reference, so nothing here caused it. The decks moved to `tests/regression/misc`.

## Fallback pruner (item 6)

`keep_binners_card()` runs only after the fallback pass, and drops what `INPgetModBin()` would not take, per model name. It applies the binner's rule in the binner's order: last parsed first, moving on only to a card with lmin and wmin both at least as large. The per-name part matters, as a subckt can hold several models.

Sky130 `tt_mm`, `w=3 l=20.2`, then `xm2`, `setseed 1`:

| | before | after | `w=3 l=10`, control |
|---|---|---|---|
| cards drawn for `xm1` | `.17`, `.25` | `.17` | `.17` |
| log lines | 20 | 17 | 17 |
| `xm2` vth | 0.6421787716 | **0.6331797046** | 0.6331797046 |

`xm1` stays on `nshort_model.17` throughout.

**Literal `nf=1` bit-identical:** 80 sizes in one sky130 deck, w 0.42 to 100, l 0.15 to 20, `setseed 3`. Draw logs `cmp`-equal before and after, 329 lines; `i(vd)` -3.0245677247843269e-03 in both. Plus `measure/` at tolerance 0. Sizes on the top edge do change, by design. The grid has none.

Overlapping cards, literal against the binner, both orders agree: card.2 then card.1. Two models in one subckt keep `nc.2` and `pc.2`. Valgrind on the deck: exit 0, no invalid read.

RED, `check.sh`, before binary:
```
-draws: x2 draws the same after either size of x1
+draws: x2 draws differently after l=20.2
+const.vth10 = 6.571491e-01
+const.vth20.2 = 6.671464e-01
```

## Seed warning (item 39)

`option seed=` that seeds now arms the existing once-per-session warning, and the warning names what was overridden. Its `setseed` wording is byte-identical: `option-seed-setseed` and `-random-setseed` pass with their `.err` unchanged. `seed_explicit`, a string, replaces `setseed_used`, and `rnd_reset_state()` clears it.

**One existing reference changed, as the stated purpose:** `option-seed-control-netlist.err`, empty, now holds the warning, and the deck asserted the silence in its comment. Old: nothing. New:
```
Warning: '.option seed=7' re-seeds the random number generator each time the
    circuit is parsed, by 'source' and by every 'reset', so it overrides 'option seed='.
    Remove the option to let 'option seed=' choose the seed.
    (This warning is printed once per session.)
```
The deck was tightened at the same time: two resets, one warning, and the draw after each equals the deck's first draw.

- Draws do not change: `measure/` 0 changed, identity with 45.2 yes, `setseed7-bit-identity`, `rnddraws-gold` pass.
- Neither form in the deck: `option-seed-nosetseed`, `option-seed-control`, `option-seed-absent` still have empty `.err`.
- `option seed=abc` seeded nothing, so it arms nothing: only its own warning. That deck is a guard, green before too.

RED, `check_status.sh`, before binary: `-netlist` and `-netlist-random`, `FAILED: err`, with the warning lines missing from stderr.

## `--disable-xspice`

`OPT_ENH_RSHUNT` is declared under `#ifdef XSPICE` in `optdefs.h`. The `#else` arm is gone. It could never have run: `rshunt` is in `OPTtbl` under XSPICE only.

RED, scratch tree, `configure --disable-xspice --enable-pss --enable-cider`:
```
cktsopt.c:254:10: error: 'OPT_ENH_RSHUNT' undeclared (first use in this function)
make[3]: *** [Makefile:644: cktsopt.lo] Error 1
```
It was the only `error:` under `make -k`. After: `make` exit 0, `src/ngspice` runs a deck. The tree is deleted.

**Worth knowing:** `.option rshunt=1e9` without XSPICE ran with no message. The warning `ca2d99436` meant to add was unreachable before this too, so nothing is lost, but the option is ignored silently. Not widened.

## Unverified

- Shared library: `add_bkpt()` order, `rnd_reset_state()` with `seed_explicit`. Not linked or run.
- `make check` on the `--disable-xspice` tree: not run, only the decks named above.
- Non-XSPICE `maxStep*5e-5` default: only 887 and the VDMOS deck ran on it.
- SOA with Tj held by a 0 V source stays silent. Open, upstream's.
