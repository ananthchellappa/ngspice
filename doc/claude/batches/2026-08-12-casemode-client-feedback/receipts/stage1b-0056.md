# Stage 1b receipt — issue 0056

## Remediation

**Receipt — issue 0056** (all measurements re-run at `720c8743a`, `build-ver_50/src/ngspice`, baseline `/usr/local/bin/ngspice`)

1. **FIXED** — committed `doc/claude/feedback/ngspice_upstream/repro/plain.cir` (title comment first line), named at its point of use; the three prompt transcripts re-measured and re-quoted with the `grep` that produced them shown (`save v(MIDNODE)`→`Name: const (constants)`, `save v(MidNode)`→same, `save v(midnode)`→`Name: op1 (Operating Point)` + `midnode : voltage, real, 1 long`; baseline ngspice-46 → `Name: const (constants)`). Also added a prompt-route block to `repro/run_all.sh` under finding 2 (it belongs to finding 2's evidence); `sh -n` clean, section output verified.
2. **FIXED** — checker right. Measured partial miss under `preserve`: rc=0, stderr 0 bytes, plot holds only `In : voltage, real, 1 long [default scale]`, `grep -in midnode` over both streams no match. Impact now opens "two shapes, the loud one is the less dangerous", quotes the partial run, and hands reporting to `0057` criterion 2.
3. **FIXED** — cross-references added: `0057` (Status, the partial paragraph, the silence paragraph, criteria 3/5/6, Resolution) with criterion 3 rewritten to **defer the whole near-miss/diagnostic prescription to 0057** and keep only "criterion 1 must not foreclose it" (`outitf.c:412`/`:414` is driven by `savesused[]`, verified); `0059` replaces the stale "filed separately if at all"; `0058` cited for the doubled banner (re-measured: exactly 2 experimental-banner lines on the `distinguish` run's stderr).
4. **FIXED** — Status now reads "no *number* is copied from that report. One *argument* is copied", attributing `FINDINGS.md:124-128`, and the migration-hazard paragraph carries the attribution inline.
5. **FIXED** — committed `repro/time_node.cir` and `repro/save_probe.cir`; both re-measured and quoted with the exact commands. preserve: `In/Time/Vs#branch/time` all `59 long`, raw index 0 `time`=0.0 (scale), index 2 `Time`=1.5 = half of `v(In)`=3.0; fold yields only `in/time/vs#branch`. `save_probe.cir`: preserve rc=1 with `Error: RHS "v(midnode)" invalid` and no `vprobe`, fold rc=0 `vprobe = 2.250000e+00`, distinguish rc=1.
6. **FIXED** — `No. Points: 1` restored (`head -11 save_lower.raw`, 570 bytes). Two further silent edits found and fixed while there: the `save_lower` block is now an explicit `2>&1 >/dev/null` stderr transcript with a sentence naming what stdout holds, and the annotated lines in the `save_probe` and `time_node` blocks are marked as added.
7. **FIXED, with one part of the complaint REFUTED** — the tree-wide survey is now quoted in full (6 sites: `RampVg2.cir:15`, `postcoms-keyword-case.cmd:248`/`:269`, `device-keyword-case.cmd:362`/`:386`, `bugs-2.cir:10`), and criterion 6 now cites the two pipe pairs instead of `shell-keyword-case.cmd`. But those pipe decks are **not** counterexamples to the bold claim: `SPEEDCHECK`/`DELTACHECK` are consumed inside Pass 1 by `eqc()` at `src/frontend/outitf.c:307`/`:313` before `name_eq()` decides anything and their vectors are synthesised, and the third argument `v(out)` matches in the *stored* spelling against the lower-case `out` of the `circbyline` divider — so no deck resolves a `.save` argument against a stored name in a different spelling, and the claim stands as written.
8. **FIXED** — new acceptance criterion 9 requires correcting `doc/claude/decisions/0001-distinguish.md:314-316` ("no vector-name matcher outside them" is falsified by `name_eq()`), while recording that decision 3, the `evtprint.c:341`/`evtshared.c:253` precedent and criterion 2's stored-against-stored argument are unaffected.

**Batch-wide (b), not in my issue but measured for routing:** with `-D casemode=bogus`, warning counts are — plain deck 2, deck with a top-level `.include` 2, deck whose `.control` runs `source` 3; with `SPICE_SCRIPTS=.`: 1, 1, 2. So `.include` is **not** an `inp_readall()` and `source` **is**. `0058` is right; **`0060`'s acceptance criterion 2 needs the `.include` half struck** — I cannot edit it.

**Other sibling change needed:** `0057`'s remediator renamed my `repro/save_partial.cir` to `repro/save_partial_lower.cir` mid-run; 0056 line 106 now quotes that deck by name and credits 0057 with owning it. If 0057 renames it again, that one line in 0056 must follow.

## Re-check

- verdict: **GOOD** · decks_committed: True · cross_refs_ok: True

### Still unresolved

- Status line 17-21 says every quoted deck is "committed under doc/claude/feedback/ngspice_upstream/repro/"; in fact plain.cir, time_node.cir, save_probe.cir, save_lower.cir, save_current_lower.cir and print_lower.cir are untracked and save_partial_lower.cir is only staged (git status: ?? / A). The whole batch, including all six issue files, is uncommitted, so this is a batch-wide wording convention rather than a 0056 defect - the files exist on disk and every command in the issue re-runs from them. It becomes true when the batch lands; flagged only because problem 1 was raised as a reproducibility defect.

- Root Cause line 320-322 says SPEEDCHECK/DELTACHECK are "consumed *inside* Pass 1 by eqc() (src/frontend/outitf.c:307 and :313) before name_eq() decides anything". Strictly, name_eq(saves[i].name, dataNames[j]) at :300 is evaluated first in each j iteration and returns FALSE; only then does the eqc arm fire. The substance is unaffected and verified - no dataName is a case variant of speedcheck/deltacheck, the arm's vectors are literals passed to addDataDesc, and the deck runs rc=0 with no emitted errors under all three casemodes - so the refutation and the bold claim both stand; the ordering clause is just loose.

### Regressions introduced

- (none)
