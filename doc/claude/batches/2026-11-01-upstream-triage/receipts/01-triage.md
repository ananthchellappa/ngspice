# Receipt: upstream triage of 212 commits for `ver_51`

Upstream tip `68b0d57b3` (`pre-master-48`), merge base `72620328a`, fork `ver_51` at `e66057427`. Read-only: nothing checked out, stashed, staged or built. `git merge-tree` wrote tree objects into `.git/objects` for the per-commit tests, as the task allowed; the chain tests used a private object directory. Nothing else in the repo changed except this file. I ran the existing `build-ver_50/stage/bin/ngspice` (reports `ngspice-46+`, built Oct 26) on small decks in the scratchpad.

Counts: FIX rows 70 (15 PULL, 42 MAYBE, 3 NO as is, 10 NO); FEATURE 99; hicum2 removal 1; upstream reverts 6; SKIP 33; not assessed 3. Total 212.

## How to read this

- **Applies alone** is `git merge-tree --write-tree --merge-base=<c>^ ver_51 <c>` on that commit by itself.
- **Chains** were applied one after another on `ver_51` in a private object store. See "Chains tested".
- **Fork has it?** "no" means: `git cherry` says `+`, a script found the commit's added lines mostly absent from the same files on `ver_51`, and `git log --grep` showed no equivalent. For rows marked R, I also read the changed site on `ver_51`. Partial line matches from that script (for example `d8b58aee5`, `a69b65ce4`) are existing lines moved around, not an earlier backport. No row turned out to be fixed independently in the fork.
- **read** column: R = I read the whole diff. P = part of the diff. M = commit message and stat only, so what it says is inferred from the message. R+run = also measured with the fork binary.
- PDK checked: `/home/analog/private/xschem-private/sky130A/models/libs.tech/combined/` (the copy the fork's MC batch used). Sky130 claims come from grepping and parsing that tree.

## 1. `940bb50fa` against the draw log and sky130 mismatch

**What the fork does now** (read `inp_fix_agauss_in_param`, `inpcom.c:8858`). Every `.param` holding agauss, gauss, aunif, unif or limit, at top level or in a subckt, becomes a `.func`, and each reference becomes a call, so each reference draws. A derived `.param` that merely references one draws once per reference inside its own expression and then keeps its value.

**Measured on the fork**, 3 `sky130_fd_pr__nfet_01v8` devices, no hsa (`-n`), `setseed 1`:

| corner | rows | `?` (global .param) | bin cards | `delvto` |
|---|---|---|---|---|
| `tt_mm` | 442 | 7 | 432 (3 per card: wint, lint, cjs) | 3 |
| `mc` | 1547 | 1112 | 432 | 3 |

So `sw_polycd`, `sw_activecd` and the junction parameters are drawn per bin card per instance. Under `.lib mc`, `lint` was `-1.71e-9` for `xm1:nshort_model.1`, `-0.95e-9` for `xm2:nshort_model.1`, and `+1.24e-9` for `xm1:nshort_model.25`. What sky130 calls process variation therefore behaves as per-device variation here, and the fork under-states common-mode spread. This is what the upstream commit message complains about, and on this evidence it is right. Under `tt_mm` these draws are multiplied by `MC_PR_SWITCH=0` but still use up the random stream.

**What would change if it were pulled as is:**

1. **Sky130 stops loading (inferred, not run on an upstream binary).** 36 lines have the shape `.param sw_polycd = {0.0+corner_factor*0.0} + process_mc_factor*MC_PR_SWITCH*AGAUSS(0,1.5e-09,1)`: 10 in `parameters_fet_tt.spice`, 21 in `parameters_res_nom.spice`, 5 in `parameters_cap_nom.spice`, with siblings for the other corners. Every `.lib` section includes them, `tt` too. All identifiers are top-level, so `stat_expr_is_toplevel` keeps them as `.param`. On the fork a plain `.param p2 = {1+0} + c*5` is fatal: `; sign expected`, `exit(1)` (decks at `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/t/q.cir` and `q2.cir` beside it). Only the `.func` conversion lets these lines parse today. Upstream's `xpressn.c` differs from the base by one error string, and I found no related change in `inpcom.c`. The commit message cites `parameters/critical.spice`, whose lines have no braces, so its author may never have hit this shape. The way out is `set stat_param_as_func`, or quoting those lines in the patched PDK copy (the owner already patched `nfet_20v0_nvt`, F48).
2. **Process variation becomes global** once it loads: one draw per load, shared by all devices and bins, with derived parameters correlated. `.lib mc` statistics change a lot. This is the correct behaviour.
3. **FET mismatch stays per instance, same distribution.** Six FET subckts hold their mismatch in a subckt `.param` referenced once (`nfet_01v8`, `nfet_01v8_esd`, `nfet_03v3_nvt`, `nfet_20v0_nvt`, `nfet_g5v0d16v0_base`, `esd_nfet_g5v0d10v5`). The rest have AGAUSS inline on the device line and are untouched. Under the new rule one draw per instance, as now.
4. **BJT and xhigh_po mismatch does change.** `var_is`, `var_bf`, `mm_is`, `mm_bf` in the npn and pnp subckts and `rhead`, `rbody` in `res_xhigh_po` are referenced 2 or 3 times per instance. The fork draws independently per reference; upstream shares one value in the instance. So is/bf (and the two ends of the resistor) become correlated. That is a real change to per-instance mismatch for sky130 bandgaps.
5. **The draw log changes form.** The FET rows above go from `m.xm1.msky130_fd_pr__nfet_01v8  delvto` to the subckt dot-card form `xm1  swx_vth`. xschem's census pins `m.xmN.m1/delvto` for `nfet_20v0_nvt` (mc_plan PLAN line 450) and treats the keys as contract (F35). Bin rows `wint/lint/cjs` go away, which is most of the 97 % in F40; rows drawn on a card itself (BJT `ise`) stay. Global rows drop to about one per parameter.
6. **Same seed, different numbers.** Stream consumption changes, so every stored seed baseline stops reproducing. F22's "hsa changes the draw sequence" mostly disappears, and F16 (alterparam on such a param) starts working.
7. **Fork tests.** `rnddraws-mm` and `rnddraws-order` assert `r.xm1.r1  r` for a subckt `.param sx`; they would fail and need the contract reopened. It merges cleanly, so nothing warns you at merge time.
8. **Case modes.** `stat_collect_toplevel` keys a hash on raw text. Under `casemode=preserve` a deck spelling a name two ways silently falls back to `.func`. The identity lint cannot see it, because nghash is not a `strcmp`.

**Verdict:** NO as is. It is the most important modelling question in this list, but it is a project (PDK or parser fix, xschem census, contract, baselines), not a cherry-pick.

## 2. `03ef19244` binning on sky130

Sky130 edges include L = 0.15, 0.18, 0.25, 0.5, 1, 2, 4, 8 and W = 0.42, 0.55, 1, 3, 5, 7, so many everyday sizes sit on an edge. On the common-size grid I simulated, 134 of 170 combinations for nfet_01v8 would pick a different bin (with the list searched last-parsed first, as `INPmakeMod` builds it).

But the results do not move. The fork already picks different bins for the same edge device with and without hsa, which allows a direct comparison: hsa with literal sizes picks the upper bin (the one the new rule picks), and no-hsa or a parametric size picks the lower one. Example: L=0.5, W=1 gives `nshort_model.29` against `.38`. Across 32 edge sizes where the two picks differ, nf=1, at one bias, tt, default temperature (24 sizes on nfet_01v8, pfet_01v8, nfet_01v8_lvt, pfet_01v8_hvt, nfet_g5v0d10v5, pfet_g5v0d10v5 with all five quantities below; 8 more on nfet_01v8 with id and gm, some also gds and cgs):

- **1.8 V flavours:** id, gm, gds, cgs, cgd identical to the 7 digits printed.
- **g5v0d10v5:** at most 1.5e-5 relative.

So the sky130 cards are continuous at bin edges in these quantities. A kink stays (slope changes at an edge, for example L=0.18, W=0.42) but there is no step. What does change: the `xmN:model.K` key in the log for parametric sizes at edges, and under `.lib mc` today which card's independent draw a device uses (only because of section 1). Not tested: other corners, other bias, derivatives beyond those printed.

The new rule also rejects a size exactly on the outermost max (L=20.2, W=1010, W=101 on native), and sizes up to 1 nm outside the outer box that the old rule accepted. Those sizes now find no bin. I checked that the hsa pruner's float comparison in `rem_unused_mos_models` keeps the bin the new rule wants at each edge listed above (W=0.55 rounds up in float but stays in the upper bin).

## 3. FIX table (ordered PULL first, ranked)

| hash | what it does | who it affects | fork has it? | alone on ver_51 | depends on | verdict | why | read |
|---|---|---|---|---|---|---|---|---|
| `ba36e3bdc` | PWL source with `r=` repeat: floor() rounding can leave time past the last point, source returns 0 V for one timepoint (a glitch on a clock). | PWL V/I sources with repeat, any PDK | no | clean | none | **PULL** | Real dropout on repeated PWL clocks, 2 clamp lines | R |
| `f961c98d0` | Wrong derivative dVdseffCV/dVb in saturation (`1.0-T5`, should be `T4-T5`) in BSIM3 and BSIM4v5 capmod 1 and 2. | **Every sky130 FET**: cards are `level=54 version=4.5 capmod=2`, which inpdomod.c maps to BSIM4v5. Charges are unchanged, so tran results move only through convergence; AC, noise, PZ and .sens capacitances change. | no | clean | none | **PULL** | The fork lacks it at both b4v5ld.c sites and has `T4-T5` already in bsim4, v6, v7. Directly affects sky130 small-signal and sensitivity | R |
| `03ef19244` | Bin rule becomes min <= x < max with relative tolerance, instead of inclusive at both ends with a 1 nm absolute tolerance. | All binned MOS. Sky130: picks a different bin for sizes on an edge, results do not move (measured, section 2). | no | clean | none | **PULL** | Correct rule, no measured result change on sky130. Log keys `xmN:model.K` move for parametric sizes | R+run |
| `3e8a21ae6` | `u0temp` was overwritten from the size-dependent value, losing STI stress and WPE mobility (BSIM4, v5, v6, v7). | BSIM4 family with SA,SB>0 or WPE. Sky130: no change with sa=sb=0 (subckt default), no card sets wpemod, no active mulu0. Changes post-layout netlists with sa/sb. | no | clean | none | **PULL** | Real bug, inert in common sky130 use | R |
| `b277f48c0` | `meas avg` window now starts at exactly `from` and ends at exactly `to` (it dropped the partial intervals). | Any `meas avg from= to=` | no | clean | take with the next three | **PULL** | Monte Carlo metrics using avg shift by O(dt) and now match integ/(to-from) | R |
| `7c2ebe189` | Same AVG clip for a dc sweep. | `meas avg` on dc | no | conflict: com_measure2.c | b277f48c0 | **PULL** | Part of the series | M |
| `650ac305f` | RMS/INTEG on a descending dc sweep read index -1 and returned 0. | `meas rms/integ` on descending dc | no | clean | 7c2ebe189 | **PULL** | Out-of-bounds read | P |
| `63d0fcf9d` | AVG always adds the final partial interval, even when the sample is within 100 ULP of `to`. | `meas avg` | no | conflict: com_measure2.c | b277f48c0 | **PULL** | Fixes a whole-step loss in b277f48c0 | R |
| `f4f9cbdfb` | Stop tests add `time >= final` in dctran and optran, and optran does not discard the final breakpoint: no run past TSTOP or endless OPtran. The sharedsync part is library only. | All tran and optran | no | clean | none | **PULL** | Hang class, small | R (analysis files), M (sharedspice.c) |
| `277e7654a` | A rejected tran step retries from the last accepted solution, not from the failed iterate (bug 879). Matters for B sources and other devices without a predictor. | Tran with non-convergence retries | no | clean | none | **PULL** | Convergence. Moves results only in runs that had a retry | R |
| `149688cd3` | Diode sidewall charge in forward bias beyond fc used the bottom F1 (bug 870). | Level 1/3 diodes with sidewall. All 15 sky130 diode cards set cjsw/mjsw. | no | clean | none | **PULL** | Real, small, forward-biased diodes only | R |
| `5caff08fa` | A breakpoint close to TSTOP no longer moves TSTOP earlier (patch 128). | All tran with a breakpoint within minBreak of TSTOP | no | clean | none | **PULL** | Small and safe. Half the diff is whitespace | R |
| `3ef069fb1` | Behavioural C/L with `m`: `%se9` pasted m into a number, so a non-integer or expression m gave garbage (bug 876). Live code: the block is the `#else` of `#if (0)`. | Behavioural C/L with m | no | clean | none | **PULL** | One line | R |
| `942ab1ec7` | `.sp` noise: clamp a sqrt argument so rounding does not give NaN. | RF `sp` noise | no | clean | none | **PULL** | Crash-class guard, safe | R |
| `68b0d57b3` | CPL with more than 8 conductors: error and exit instead of core dump. | CPL users | no | clean | none | **PULL** | Harmless guard. No sky130 effect | R |
| `940bb50fa` | Top-level .param with agauss etc. stays .param, drawn once; in a subckt, once per instance. `set stat_param_as_func` restores the old way. | All statistical decks, see section 1 | no | clean | needs PDK or parser work, xschem census change | **NO as is** | Inferred to stop sky130 loading (36 lines), and reopens the xschem log contract. Owner decision | R+run |
| `d7e64e28d` | Removes the floor of 100 Newton iterations; sets "defaults" to 100 in cktinit.c. | All analyses | no | clean | would need cktntask.c defaults set to 100 too | **NO as is** | Trap: CKTdoJob copies TSK values over CKT, and cktntask.c still has itl4=10, itl2=50 at upstream tip, so effective ITL4 falls from 100 to 10 and ITL2 from 100 to 50 | R |
| `472403238` | Default `minbreak` 1e-15 instead of 0, which disables the dctran fallback (10*delmin with XSPICE) (bugs 887, 888). | Every tran: breakpoint merging and the end test | no | clean | none | **MAYBE** | Moves steps on every tran. I could not read the bug reports | R |
| `e8285746c` | Level 1 diode: IBV scales with area, as in level 3. | Level 1 diodes with ibv and area != 1. Sky130: none, all 15 cards are level 3. | no | clean | none | **MAYBE** | Correct, but changes user decks and no sky130 benefit | R |
| `dfbbc64ca` | MOS2 gm/gds/gmbs derivative fixes. | Level 2 MOS only. Not sky130. | no | clean | none | **MAYBE** | Changes level 2 AC. No sky130 benefit | R |
| `c07143999` | BJT review: Jacobian, area counted twice in a fallback, ISS factor, AC/PZ/noise stamps, bypass state, op queries. State count 33 to 34. | All BJT. Sky130 npn/pnp are level 1 with no rco, iss=0, tlev=0, so few of these bite. Bandgaps possibly via convergence. | no | clean | none; 15 files | **MAYBE** | Its own message says run a .sens regression, as state slots moved. Do that first | M (message), R (bjttemp.c) |
| `75f22a99f` | ISE/ISC > 1e-4 multiplier resolved in BJTtemp, so a changed IS is followed. | BJT with multiplier-form ise. Sky130 ise values are absolute, so no change. | no | clean | c07143999 not needed | **MAYBE** | Only for altermod on IS | R |
| `209f9bff6` | Quasi-saturation epi charge continuous, fixes "timestep too small" with QCO. | BJT with rco and qco. Sky130: none. | no | clean | with next two | **MAYBE** | Good, no sky130 use | P |
| `6dd312a6e` | VO=0 with RCO no longer divides by zero. | As above | no | conflict: bjtload.c | 209f9bff6 | **MAYBE** | As above | M |
| `d329b9780` | RCO and QCO scale with area. | As above | no | conflict: bjtload.c | 209f9bff6 | **MAYBE** | As above | M |
| `8533ad9fb` | Diode soft recovery scaling and revision. Includes level 3 w/l m-squared fix, AC self-heating fixes, adds qpscale, state count 22 to 21. | Diodes with tt-soft/self-heat, level 3 w/l. Sky130 cards use area and no tt, rth0. | no | clean | none; 13 files | **MAYBE** | Large, little sky130 effect. Base of next row | M (message) |
| `d58ee1041` | Crash when a model has rth0 but an instance is not thermal. | Diode self-heating | no | conflict: diobindCSC.c, diosetup.c | 8533ad9fb | **MAYBE** | Crash, not reachable from sky130 cards | P |
| `fc601dcab` | Diode parameter info table correction. | Diode | no | conflict: dio.c | 8533ad9fb | **MAYBE** | Conflict is context only | M |
| `9cc5bdbea` | Derivative of recombination current fixed. | Diodes with isr. Sky130: no isr. | no | clean | a69b65ce4 context | **MAYBE** | Conductance only | R |
| `a69b65ce4` | Generation current in reverse and breakdown made continuous. | Diodes with isr. Sky130: no isr. | no | clean | none | **MAYBE** | No sky130 use | R |
| `5c4e9ab1d` | VBIC stamp review: wrong AC/PZ for PNP with rth or td, Ith sign, KLU corruption, noise. | VBIC. No sky130 card uses it. | no | clean | none | **MAYBE** | Clean, 14 files. Take only if someone uses VBIC. Removes model `dtemp` | M (message) |
| `211db2633` | VBIC node deletion on re-setup, no result change. | VBIC | no | clean | 5c4e9ab1d | **MAYBE** | As above | M |
| `11136ee61` | VBIC aliases xrb, xrc to intrinsic. | VBIC above tnom | no | clean | none | **MAYBE** | As above | M |
| `c0a97461d` | VBIC xrb, xrc fall back to extrinsic. | VBIC | no | conflict: vbic.c | 5c4e9ab1d | **MAYBE** | As above | M |
| `b3594401c` | VDMOS polarity, self-heat, soft recovery; also DEVlimitlog in devsup.c. | VDMOS. No sky130 use. | no | clean | none | **MAYBE** | Clean, 16 files | M (message) |
| `42f32880f` | VDMOS AC/PZ quasi-saturation stamp. | VDMOS | no | conflict: vdmosdefs.h | b3594401c | **MAYBE** | As above | P |
| `a4ff5a204` | OSDI: stop on model or instance init error instead of carrying on (bug 862). | OSDI | no | clean | none | **MAYBE** | Only if you load .osdi models. Leaks node_ids on the error path | R |
| `24f0f2314` | OSDI noise state stride, totals aliased; scratch length tracked. | OSDI noise | no | clean | none | **MAYBE** | Wrong totals, overrun class | R |
| `0a8c29eec` | OSDI instance `temp` is Celsius, not Kelvin. | OSDI with temp= | no | clean | none | **MAYBE** | Behaviour change | R |
| `875513496` | OSDI internal node names and is_flow after collapse. | OSDI with collapse | no | clean | none | **MAYBE** | Wrong node names | R |
| `b2f00c8c3` | OSDI: report the limiting instance, was NULL. | OSDI | no | conflict: osdiload.c | needs absdelay context (a62ccd27e, 9f398e8ae) or a small hand edit | **MAYBE** | Diagnostic, hand port | R |
| `f1660838d` | OSDI 0.4 version and descriptor size check. | openvaf-r models | no | clean | absdelay group | **NO** | Superseded by 9f398e8ae, and later found to reject valid files until 56d806eac | R |
| `56d806eac` | Fix the size check above. | openvaf-r models | no | conflict: osdilegacy.h, osdiregistry.c | 9f398e8ae | **MAYBE** | Only with the absdelay group | R |
| `9cf762afb` | Free absdelay buffers. | absdelay | no | conflict: osdisetup.c | a62ccd27e | **MAYBE** | Only with the absdelay group | M |
| `89ad00e07` | Meas error buffer, `deriv()` heap overrun on a short vector, fourier/fft length guards. | Control scripts | no | conflict: com_measure2.c | fb6ba949d, c235cf8a2, measure series | **MAYBE** | Real memory bugs. Clean only after fb6ba949d and c235cf8a2 (chain tested, 10/10) | P |
| `35d29240f` | sprintf to snprintf in measure. | Same | no | conflict: com_measure2.c | 89ad00e07 | **MAYBE** | Goes with the row above | M |
| `68cd5f139` | Example scale fix for E-225. | Example | no | clean | 89ad00e07 | **MAYBE** | Goes with the row above | M |
| `81f5b2c6d` | Example scale fix for E-225. | Example | no | clean | 89ad00e07 | **MAYBE** | Goes with the row above | M |
| `b2f9fefa3` | `.save none` sticks, made to work in the program; leaks fixed (bug 875). | `save` users | no | conflict: breakp2.c, outitf.c | hand port | **MAYBE** | Conflicts in breakp2.c, outitf.c with the fork case work: hand port. May help memory in large MC | M |
| `dd3a8dd15` | cshunt from an earlier deck is removed when no longer given. | Decks sourced in sequence with cshunt | no | clean | none | **MAYBE** | Check against the fork cp_remvar work first | R |
| `b2a86d38c` | .option rshunt/cshunt in a deck overrides spinit, restored on the next deck. | Same | no | conflict: inp.c | 385904847 | **NO** | Sits on the rshunt rewrite | M |
| `385904847` | rshunt rewritten as generic added resistors (with fbe33f99e, a59bed240, 04fde7a6e). | `.option rshunt` | no | conflict: inp.c, cktsetup.c | reverts below | **NO** | Conflicts in cktsetup.c with the topology code the fork keeps. Own design decision | M |
| `072980a57` | Quoted strings parsed in INPgetStr (bug 869). | XSPICE string parameters | no | clean | needs a one-word fix | **NO as is** | `while (strchr(GARBAGE, *point))` has no NUL guard: strchr matches the terminator, so a line ending in garbage reads past the buffer. Still so at upstream tip | R |
| `a9950f0e8` | Quotes kept and case kept in inpcom.c for XSPICE strings. | XSPICE string parameters | no | conflict: inpcom.c | 072980a57 | **NO** | Conflicts with the fork casemode work in the same lines | M |
| `aaba50ba3` | NULL terminator in the is_cider_model list. | None here | no | conflict: inpcom.c | a9950f0e8 | **NO** | The list only exists after a9950f0e8 | R |
| `2d76fefd6` | Two XSPICE model parse restrictions removed. | Custom code models | no | clean | none | **MAYBE** | Clean. Only if you write code models | M |
| `a1148c969` | aswitch, pswitch: derivative 0 past the limits, log region normalised, AC gain sane. | XSPICE switches in AC | no | conflict: Makefile.am | 613ba2ea5 for its tests | **MAYBE** | Makefile.am conflict is trivial | M |
| `f6d11dbbd` | Memristor code model uses rinit for ac/dc. | Memristor | no | clean | none | **MAYBE** | Clean, narrow | R |
| `797409238` | s_xfer crash when den_coeff has length 1. | s_xfer | no | clean | none | **MAYBE** | Clean, narrow | R |
| `52634b164` | s_xfer checks init_ic length. | s_xfer | no | clean | none | **MAYBE** | Clean, narrow | P |
| `d8b58aee5` | s_xfer cleanup. | s_xfer | no | clean | none | **NO** | Refactor | M |
| `2faaabefc` | HFET, MESA reverse and forward bias corrected (bugs 877, 878). | HFET, MESA | no | clean | none | **MAYBE** | Clean, narrow. No sky130 | P |
| `58e4cf26b` | `(` ends a token in INPgetNetTok, so `.model a b(p=1)` keeps its first parameter. | Mainly OSDI cards | no | clean | none | **MAYBE** | A node name containing `(` now splits. Test first | R |
| `6a10f7af2` | KLU adjoint solve used the wrong solve, so .sp noise under KLU was wrong; enables noise and PZ under KLU. | `option klu` only (default is sparse) | no | clean | none | **MAYBE** | Only if you turn on KLU | R |
| `935d85633` | Integer op vectors saved as reals. | Saving integer opvars | no | clean | none | **MAYBE** | Removes a message, low value | R |
| `cbb09dc25` | `setseed` also re-inits the tranoise generator; adds set_sseed. | trnoise users | no | conflict: inp.c | other agent | **MAYBE** | Seed area, see receipts/02-seed.md. Conflict in inp.c | P |
| `130bc1cb9` | Stats count fix for rshunt/cshunt devices. | Statistics only | no | conflict: inppas5.c | none | **NO** | Cosmetic, conflicts | R |
| `20d48496b` | Memory warning once only; `no_mem_check` honoured. | All | no | clean | none | **NO** | Cosmetic | R |
| `2e4aba542` | KLU warning once. | KLU | no | conflict: klusmp.c | none | **NO** | Cosmetic, conflicts | R |
| `821132413` | PSS: keep the step at a double breakpoint; extra UIC breakpoint. | `.pss` | no | clean | none | **NO** | Only PSS, and it edits dcpss.c ahead of the RF series | R |

## 4. Upstream reverts of something the fork keeps

`f82cc35ce fa8b6756b 47c77e421 ee38134f6 f5754446c a3662ba76` remove the dangling-passive topology reduction. The fork holds the original commits (they are `-` in `git cherry`) and still has `CKTtopologyReduce`, off unless `set topo_reduce` is given (read at `cktsetup.c:72`). **NO**: 3 of the 6 conflict, and `f82cc35ce` alone would flip the default to on. Leaving it costs nothing, but it is why the rshunt rewrite conflicts. Read: `f82cc35ce` diff, stat for others.

`5476dfb7d` removes the built-in hicum2 model, 210 files. **NO**: it drops a device and the fork has no need to.

## 5. FEATURE groups

| group | commits | hashes |
|---|---|---|
| RF: PSS, PAC, pnoise, PXF, periodic S-params | 24 | `a26fb5604` `0c18fbc17` `c72a309d4` `3d6f75246` `3c1a5f029` `7427a3c84` `cee82e96a` `83c21f856` `c79adc3d2` `e85ce75d1` `1df803f2d` `2f267f4ff` `b0c7ce898` `ef243eee9` `224d9bd95` `873a81616` `3dc3c226f` `9639389fa` `e67a88bb9` `9a227a94f` `552084701` `d6e8318a2` `6cfe9997a` `83cace64f` |
| RF: harmonic balance, QPSS, QPAC, QPnoise, QPXF, hbosc | 19 | `85c8380e9` `fd425cf06` `a6557f1e2` `c325fa366` `3f87501da` `43a586494` `e5f46070b` `3dd83d489` `10ebdef58` `cd6fe023a` `5213e2245` `27c517a68` `adf0287bf` `f1bc72687` `5d4321eef` `60f6ad4a6` `cd93e3224` `0080440ad` `a9e5983a1` |
| RF: nport device, Touchstone, snp2va, loadpull, rfstab | 21 | `4a2216abd` `44d9dbb3b` `f63e26fbf` `6fd8ac233` `e03154cc6` `1f9888ce6` `092fffd93` `4d5b8e85e` `87dabf8f6` `e62ccd2b7` `48b791d82` `33668056a` `599c4b07b` `9149b19ef` `8d7be20d8` `2ccc0e7be` `35ddae256` `ec8c1186b` `ff9733d51` `e40a88752` `0ccfd6c49` |
| RF: docs and examples | 10 | `a1d6c97da` `5ee624b8f` `61ddbf500` `4401d6ad7` `4a26e9cb1` `7ca406e8e` `46920ef55` `c30fabd0e` `fe2fbe3e9` `4d0ca4ecf` |
| Plot: arrowplot | 7 | `e130c05b8` `918ae2bed` `cc7c2302b` `daf4681a8` `c804cf110` `a207ed6ac` `e0179efb1` |
| Solver and tools: line search, optimizer, checkpoint | 5 | `e100535bb` `9a0a7a4b7` `5243d439c` `d8bbfbca3` `d92fec6e9` |
| Generic `.dc @inst[param]` sweep | 1 | `af240ba02` |
| OSDI absdelay | 4 | `a62ccd27e` `9f398e8ae` `d9112d697` `afe1de11a` |
| Measure additions: DERIV, vdb in meas, gain and phase margin | 2 | `fb6ba949d` `c235cf8a2` |
| CMake | 3 | `36fb6f51f` `99916057f` `a0999002f` |
| rshunt rewrite, followers | 3 | `fbe33f99e` `a59bed240` `04fde7a6e` |

Entanglement: the RF groups are one series, built on each other and on `dcpss.c`, `cktdefs.h`, `commands.c`, `pssdefs.h`. In date order on `ver_51`, 78 of 88 candidates apply; the first real conflict is `c79adc3d2` in `pssdefs.h`, and later ones follow from it plus `inpcom.c`, `plotit.c`, README and examples. The series is also patched by bare later commits (`cee82e96a` "prevent crash (preliminary, FIXME)", `ef243eee9` carries a FIXME). Not pullable piecemeal except `af240ba02` (`.dc @inst[param]`), which applies alone and may suit your sensitivity work; `e100535bb` line search (off by default) needs a `cktdefs.h` hand merge. `83cace64f` clears `VSRCfuncTGiven` on every parameter call, which only makes sense for the E-176 PSS code. I read `af240ba02`'s subject only, plus stats for the group; nothing here is assessed beyond grouping.

## 6. SKIP

| group | commits | hashes |
|---|---|---|
| Release chores | 7 | `773c5a263` `0a9c4ea79` `08c44faf1` `2f9c8ad47` `9592bcf9d` `a80f6e3e9` `d8c3c2dd9` |
| Typo, message, warning | 6 | `f34785cc9` `226bbe1ed` `d90947582` `e73cbd263` `56a152c72` `6e6d91f6b` |
| gitignore, binary files | 2 | `d81ce0922` `4bcfd22ce` |
| MSVC, Windows, macOS only | 7 | `1c57894c6` `ac9b6c046` `8b85d0d74` `cdf703cf6` `48f34a8b8` `894738b8b` `218f207d9` |
| Library only (the fork is used through the program) | 4 | `19b73c0b0` `148a7f35d` `d2ef427e0` `9ea3cfa84` |
| Examples, tests and plots with no simulator effect | 4 | `8e0a8c87a` `f127e9642` `613ba2ea5` `64926499d` |
| ghnggen (GHDL script), not used | 2 | `231740ed8` `a9da7bca6` |
| Code the fork does not have (E-62 distortion message) | 1 | `e059824e9` |

## 7. Not assessed

`95d7f2c2b`, `059066b53`, `628d84aa5`: the seed agent has them (`receipts/02-seed.md`).

## Chains tested (all on `ver_51`, private object store)

- Recommended PULL list, in the order above: **15 of 15 clean**. A variant adding `c07143999`, `75f22a99f`, `e8285746c` and leaving out `3ef069fb1`: 17 of 17.
- Measure, including `fb6ba949d c235cf8a2 89ad00e07 35d29240f 68cd5f139 81f5b2c6d`: 10/10.
- Diode (`8533ad9fb d58ee1041 149688cd3 fc601dcab e8285746c 9cc5bdbea a69b65ce4 2faaabefc`): 8/8. BJT: 5/5. VBIC: 4/4. VDMOS: 2/2.
- OSDI without absdelay: 4/5, `b2f00c8c3` conflicts. With absdelay (`f1660838d a62ccd27e 9f398e8ae d9112d697 56d806eac` first): 12/12.
- Rshunt: 5/7. Quoted strings: 2/4.

Clean merge means no text conflict only. Nothing was compiled or run with any of these applied.

## Not checked

- No upstream binary exists here, so every effect of a pulled commit is reasoned from code, except the measurements above, which use the fork as it stands.
- I did not read bug reports 879, 887, 888 (no network use). `472403238` is rated without knowing the symptom.
- No magnitude for `f961c98d0`. The wrong term is roughly `dT0_dVb*T3`, with T3 about 0.02 in deep saturation (my estimate), and it reaches Cgs through `-(Cgg+Cgd+Cgb)`.
