# Receipt 13: VDMOS `ttt1`, `ttt2`, `tm1`, `tm2`, regression R3

Fork-written, on `ver_52`, all heavy commands through `ngheavy`, `-j4`, no exit 137. Nothing was pushed or posted.
"before" is this tree's binary at `bdb997d21`, kept in scratch. "ver_51" is `build-ver_50/stage/bin/ngspice`, run only.

**R3 is fixed. On the fork, item 30 of receipt 12's upstream list no longer applies. Upstream tip still has it, and nothing was sent.**

| item | commit | test in `make check` |
|---|---|---|
| R3, read back and order | `c85f0ac88` | `model/vdmos-tempco-ask.cir` |

## Root cause

Both come from `b3594401c`.

**Reads.** It added the four to `VDMOSmPTable` and to `VDMOSmParam`, not to `VDMOSmAsk`. So the ask falls to `default: E_BADPARM`, which `showmod` prints as `<<NAN, error = 7>>`. `CKTsens` lists every parameter marked set and ask and calls ask on it, and each failure prints `GET ERROR`. In a dc `.sens` that is `tm1` and `tm2` only, three passes each, so 6 lines for one device. `ttt1` and `ttt2` are `IOPA`, which dc skips and ac does not: an ac `sens` on one device prints 16 lines, 4 for each of the four.

**Order.** `VDMOSmParam` already had these lines, in `VDIO_MOD_TT` and `VDIO_MOD_MJ`:

```c
model->VDIOtranTimeTemp1 = 0;
model->VDIOtranTimeTemp2 = 0;
```

and the same for `VDIOgradCoeffTemp1`, `2` under `m`. On ver_51 nothing could set those fields, so the lines were harmless. Once the pick gave them set cases, `tt` wiped a `ttt1` already read, while `Given` stayed true so setup kept the 0. This was not limited to `ttt1` and `tt`: `tm1` before `m` was lost the same way, and so would be anything that sets `tt` or `m` later. `altermod` does, and so does the sens perturbation.

## Fix

- `vdmosmask.c`: four cases, next to `tt` and `m`.
- `vdmosmpar.c`: the four zeroing lines removed. The diode's `DIOmParam` has none. `VDMOSsetup` already sets each to 0 when not given, so a card that never writes them is unchanged.

The table entries stay as the pick wrote them, `IOPA` for `ttt1`, `ttt2` and `IOP` for `tm1`, `tm2`.

## RED

The new deck, run with `check.sh` on the before binary: exit 1.

```
-@ta[ttt1] = 1.000000e-03     (all 16 reads missing: if_getparam, no such parameter)
+       ttt1 <<NAN, error = 7>>
 pa = 1.364472e+00
-pb = 1.364472e+00
+pb = 1.367241e+00            (equals pn, the card without ttt1: lost)
-tt: same result in either order
+tt: the order changes the result
-qb = 9.995037e-01
+qb = 9.992773e-01            (equals qn, the card without tm1: lost)
-m: same result in either order
+m: the order changes the result
+GET ERROR: VDMOS:sm:msm -> mparam tm1 (130)     (and on, every device)
```

`check.sh`'s filter is case-sensitive on `Error`, so `GET ERROR` lines reach the diff, as the above shows.

The deck: read-back with `@model[param]` and `showmod`; tt and m each written before and after their coefficients, compared exactly; all four written as 0 against not written; the coefficients do change the result; `altermod tt` keeps `ttt1`; `.sens` on the card and `sens ... ac`, then read-back after each.

## Gates

- `autogen.sh` 0, `make` 0, `make install` 0, targeted test PASS, `tests/regression/model` all 9 PASS.
- `make -k check`: **395 PASS, 1 FAIL, 0 SKIP**, 396 tests. The FAIL is the named `resource-keyword-case.cmd`. 394 + this deck.
- `tests/lint/identity.baseline` untouched, no existing `.out` changed. `tests/.gitignore` has `*.out`, so the new reference needed `git add -f`.
- `measure/run.sh`, `measure/after-13/`. `compare.sh after-12 after-13 0 0`: **TOTAL changed: 0**. `mc_r`, `mc_sky130` identical to 45.2: yes. The suite has no VDMOS, so this says only that nothing else moved.

## Measured

| check | before | after | ver_51 |
|---|---|---|---|
| `u15` pk1, `ttt1` after `tt` | 1.36487 | 1.36487 | 1.36727, warned |
| `u15` pk2, `ttt1` before `tt` | **1.36727, silent** | **1.36487** | 1.36727, warned |
| `u15` `showmod` `ttt1` | `<<NAN, error = 7>>` | 0.001 | `?????????` |
| `u14` v(c), id | 3.500527, 6.499473 | same | same |
| one-device dc `sens`: `GET ERROR` lines | 6 | 0 | 0 |
| one-device ac `sens`: `GET ERROR` lines | 16 | 0 | 0 |

So a card with these parameters does not match ver_51 and is not meant to: ver_51 ignored them with a warning, and now they apply wherever they are written. Receipt 12's "numbers on that card equal ver_51's" was true only of the lost case.

**The value is right, by hand.** 100 C, tnom 27, dt = 73:

- `Tt=1u Ttt1=1e-3 Ttt2=2e-6` against plain `Tt=1.083658u`: peak 1.364514296 both, 9 digits.
- `M=0.4 Tm1=2e-3 Tm2=3e-6` against plain `M=0.4647948`: ac magnitude 0.9995036805 both, 10 digits.

**Any order.** One 30-parameter card with all four coefficients, 12 orderings (as written, reversed, 10 shuffled), `numdgt=12`, tran peak, ac magnitude and `showmod`: after, 1 distinct output; before, 9 distinct.

**A card that never writes them is unchanged.** Before against after, whole output with only the date dropped, 0 lines differ: the 30-parameter card without the four, `vdmos-tj-ground`, `-soa`, `vdmos-thermal-nodes-case`, `vdmos-pchan-case`, `vdmos-type-suffix-case`. The eight existing `vdmos-*` decks in `make check`, under `regression/model` and `regression/case`, PASS.

**A visible change on such a card, in `sens`.** Its result gains `m1:tm1` and `m1:tm2`, as any parameter with a working ask does. That is the only difference apart from the removed `GET ERROR` lines. `ac` also gains `m1:ttt1`, `m1:ttt2`.

## Open, not touched

- **`sens` on VDMOS is not usable, on ver_51 too.** Most rows print `-nan`, `m1:tt` and `m1:is` among them, and the new `m1:tm1`, `m1:tm2` join them. A `op` after a `sens` on the same circuit fails with `-nan` node voltages, on ver_51 as well. `kp`, `vto`, `lambda`, `rd` and `theta` rows are numbers. R3 asked for no `GET ERROR`, not for this. The deck prints only `msm:kp`, and runs `reset` between its two sens runs for this reason. Cause not looked for.
- Item 30 stays on upstream tip's list, deck `u15`. The fork's commit is a ready fix to offer. Nothing sent.
- R1, R2, R4 of receipt 12: unchanged, accepted in the ledger. The ledger's group 13 row is not added here.
