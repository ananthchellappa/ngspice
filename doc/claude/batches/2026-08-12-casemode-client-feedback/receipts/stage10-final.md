# Stage 10 receipt

## 0067 severity correction

**MEASURED** (scratch dir `/tmp/claude-1000/-home-qflow-dev-ngspice-test/7d5ac281-15ff-4791-9d45-6cc9aa6c5ad3/scratchpad/remvar`, `hdr_zzz.raw` built from `doc/claude/feedback/ngspice_upstream/repro/ascii_raw.cir` with `hdr_variants.sh`'s one-line awk splice, `Option: zzz=HAHA`). Three runs of every row; NEW = `/home/qflow/dev/ngspice_test/build-ver_50/src/ngspice`, STOCK = `/usr/local/bin/ngspice` (ngspice-46).

| sequence | NEW | STOCK |
|---|---|---|
| `load` + `unset zzz` + `set` | rc=139 3/3 | rc=139 3/3 |
| `load` + `unset zzz` + `display` | rc=139 3/3 | rc=139 3/3 |
| `load` + `unset zzz` + `write w.raw` | rc=139 3/3 | rc=139 3/3 |
| `load` + `unset zzz` + quit | rc=0 3/3 | rc=0 3/3 |
| `load` + `set` + `unset zzz` + quit | rc=0 3/3 | rc=0 3/3 |
| same 3-command line under `valgrind -q` | rc=0, 34 invalid reads | rc=0, 65 invalid reads |

So the report is confirmed exactly: the `unset` survives, the next command dies, and the valgrind numbers the issue quoted (34/65) are the tool's masking of that crash. Under gdb the fault is `#0 __strcmp_avx2`, reading the freed `pl_env` node's name during the print sort; valgrind names the block as "24 bytes inside a block of size 32 free'd" from inside `cp_remvar()`. BASE could not be re-measured — the twelve-file-reverted scratch tree is gone.

**FOUND, not in the correction — a third arm.** `US_SIMVAR` (`variable.c:649`) has the same tail bug and is reachable with no rawfile: `source <any deck>`, `set temp=27`, `unset temp` → `rc=134`, 3/3 on NEW **and** STOCK, `gmin`/`trtol`/`abstol` likewise. The arm unlinks from `ft_curckt->ci_vars` and `tfree(u)`s the node itself at `:659`; the tail then writes `v->va_next = NULL` into the freed block (valgrind: **invalid write** of size 8) and frees it again. It prints `it's a US_SIMVAR!` to stderr on that ordinary path. This matters for item 3: a fix enumerating `US_DONTRECORD` and `US_READONLY` passes the corrected criterion 2 and still aborts.

**FOUND — valgrind masks the abort too.** `unset curplot` under `valgrind -q` is rc=0 (3 invalid frees, 4 invalid reads) where bare glibc is rc=134. So the tool hides all three arms' exit statuses, not just the `pl_env` one; the issue now says the exit status must be taken with no tool underneath.

**EDITS**

`/home/qflow/dev/ngspice_test/doc/codex/issues/0067-cp-remvar-frees-a-node-it-chose-not-to-unlink.md`
- Status: added a dated **Corrected 2026-08-13** paragraph saying what was wrong and that 0065 was right; reworded "One issue and not two" to three arms ("three of the five preceding switch arms have already disposed of, two by leaving it linked and one by freeing it").
- Summary: added the `US_SIMVAR` and the three-command transcripts; table now has five rows including `load`+`unset`+`set` = **rc=139** and the two-command row kept as rc=0 with its meaning stated; BASE marked `n/m` with an explicit "*not measured*, not *not reproduced*" note. New passage on why it was missed (valgrind allocator), with the masking of `unset curplot` as evidence it is a trap.
- Impact: "Latent, allocator dependent, and today usually silent" is gone. Now: deterministic crash on this build and on released ngspice, per-arm rc, the one-command delay called out as not a difference in severity, third arm added. Shared-build claim relabelled as an inference from the link (nothing shared was run).
- Root Cause: `US_SIMVAR` arm quoted, the tail reframed as reached by every arm, `US_OK` named as the only arm whose contract with the tail is coherent.
- Criteria: 1 gains the `US_SIMVAR` case; **2 now leads with exit status** (`rc=0` with no tool underneath) and makes valgrind-clean the second half, which resolves the contradiction the report named; 3 requires "unlinked from every list *and* not already freed" and says a two-arm repair still fails criterion 1; 4 covers simvar and the stray debug print; 6 requires the deck to assert the status of the command *after* the `unset`, three sequences, one per arm.
- Related: 0065's rc=139 restored as correct and standing.

`/home/qflow/dev/ngspice_test/doc/codex/issues/0065-a-rawfile-option-line-shadows-a-computed-variable.md`
- Impact bullet (`:178`): tightened to say the fault lands on the command after the `unset`, keeping rc=139.
- Left bullet (`:472`): the "did not reproduce" claim replaced — the re-measurement stopped at the `unset`; rc=139 restored with the valgrind explanation and a pointer to 0067's third arm.

`/home/qflow/dev/ngspice_test/doc/codex/issues/0066-a-fatal-netlist-read-strands-the-reader-guard.md`
- `:32` "committed with this issue" → not committed, `doc/claude/feedback/` untracked in its entirety.
- Criterion 4: same correction, plus that calling it "the pin" describes what it can do, not what the repo guarantees.
- New **Left** bullet: the pin is not in the tree and where such a probe should live is undecided.

`/home/qflow/dev/ngspice_test/doc/claude/feedback/ngspice_upstream/repro/shared/netlist_guard_probe.c` — header no longer implies the probe is committed while only the inputs are not; says plainly that nothing there is committed.

`/home/qflow/dev/ngspice_test/doc/codex/issues/0062-shared-build-command-after-reset.md` — **not in the brief**: `:46` carried the identical "Three probes are committed with this issue"; corrected the same way.

`/home/qflow/dev/ngspice_test/doc/claude/batches/2026-08-12-casemode-client-feedback/receipts/stage9-crewF.md` — the item-2 receipt sentence annotated in place with a dated correction (original left standing, since it is a receipt); the "committed self-checking probe" line corrected too.

`/home/qflow/dev/ngspice_test/doc/claude/batches/2026-08-12-casemode-client-feedback/receipts/stage8-crewE.md` — **not in the brief**: `:70` is where the error entered ("did not reproduce today… allocator-dependent"). Annotated in place with the corrected measurement.

`/home/qflow/dev/ngspice_test/src/frontend/inpcom.c` — **this is the one edit inside `src/`**, and it conflicts with the "do not touch src/" header; item 5's second half asks for it and the comment lives nowhere else, so I made it and am flagging it. Comment only, above `inp_readall()`. Verified before writing: `Evtcheck_nodes()` is called only at `spiceif.c:185` inside `if_inpdeck()`, which is called only at `inp.c:1429` inside `inp_dodeck()`, which `inp_spsource()` reaches at `inp.c:1095` — long after the `inp_readall()` at `inp.c:532` returned. So the comment now keeps "There is no nesting today", names all three callers with the trace that rules each out, says the auto-bridge *looks* like the nesting path and is not, and ends with the honest justification (the count buys nothing today, costs an int, stays correct if a nested caller appears). `make -C build-ver_50/src/frontend inpcom.o` builds clean; the one `-Wcomment` warning at `inpcom.c:1747` is pre-existing upstream, not from this edit.

**NOT COVERED / left for you**
- `US_SIMVAR` is new evidence and I folded it into 0067 rather than filing separately, on 0067's own "one tail, one defect" argument. If you would rather it were its own issue, it is cleanly separable.
- BASE rows in 0067's table are `n/m`, not re-measured. Same gap the stage9 receipt already flagged for 0061's and 0067's tables.
- `stage9-crewF.md:43` records a still-unfiled live defect on the *write* side of the same `US_READONLY` arm: `Option: sourcepath=…` makes the reader's own `add_to_sourcepath()` fail with `cp_vset: Internal Error` on every binary tested. `0061:853` points at it; no issue owns it.
- Nothing staged, nothing committed. Pre-existing staged `.out` files from the batch were left as they were.

## Records refresh

Both files are now at stage 9. Summary of what changed and what I verified.

## Files updated
- `/home/qflow/dev/ngspice_test/doc/claude/batches/2026-08-12-casemode-client-feedback/LEDGER.md` (322 → 432 lines)
- `/home/qflow/dev/ngspice_test/doc/claude/batches/2026-08-12-casemode-client-feedback/CLOSEOUT.md` (686 → 1200 lines)

No `src/`, no `tests/`, no fixes. Working tree otherwise byte-identical to how I found it (24 modified, 15 staged `.out`, 278 untracked — same before and after).

## What I measured rather than read

**C1 rebuilt from scratch under `/tmp/.../scratchpad/c1`** — the stage-7 scratch tree was gone and three of the four files C1 hand-splits had been edited by stages 8–9, so the split had to be redone. `split0056.py` printed `RESIDUE: none`; `git diff --numstat` vs `HEAD:src/frontend/outitf.c` printed `72	20`. Then `./autogen.sh` rc=0 (`Success.`), `../configure` rc=0, `make -j8` rc=0 with 0 lines matching `error:`, `make check` **rc=0, 291 PASS, 0 FAIL, 0 SKIP/XFAIL/XPASS**, `identity lint: 265 comparisons, baseline matches` + the 11-comparison selftest, per-directory counts identical to stage 7's. RED in the same tree (outitf.c → HEAD, rebuild, caches cleared): `FAIL: save-name-case.cir`, `FAIL: save-name-case.cmd`, `FAIL: identity.lint`, `casedist` unchanged rc=0. Restored: case 116/116, pipe 18/18, lint 2/2. **C2–C10 are labelled reasoned, not tested.**

**Working tree suite**, caches cleared: **305 PASS, 0 FAIL**.

**0067's corrected severity**, 3/3 each on `build-ver_50/src/ngspice` and stock `/usr/local/bin/ngspice`: `load;unset zzz;quit` → rc=0; `load;unset zzz;set;quit` → **rc=139**; same line under `valgrind -q` → **rc=0, 34 invalid reads**; `unset curplot` with no rawfile → rc=134. Also re-measured 0061 criterion 1 (`midnode` with and without the poisoned load), 0060's asymmetry (`read=fold`, `* curcasemode fold` from a file saying `distinguish`), and the `* CurPlot op1` vs stock `HAHA` shift.

## The substantive additions

1. **Per issue** — 0060 and 0061 moved from deferred to closed; 0065/0066/0067 written up; a second disagreement table (stages 8–9, seven rows) beside the original five. Verifier right in all twelve; four of the seven new ones I re-ran myself.
2. **Commit split** — now **C0–C10** (was six). C0 gains `netlist_guard_probe.c`; C5 shrinks to 0062 alone; C6 = 0060, C7 = 0065 + 0067 filed, C8 = 0061, C9 = 0066, C10 = bookkeeping. Four source files now need hand-splitting instead of one (`inpcom.c` three ways, `fteext.h` three ways, `sharedspice.c` three ways, `options.c` two ways with *interleaved* hunks), and `pipe`/`casedist` `Makefile.am` are four-way. Flagged explicitly: **C8 alone ships a shared-build regression that C9 fixes** — squash if every commit must be shippable. All 15 `.out` need `git add -f`.
3. **Residuals** — 18 → 28 rows; 3 and 4 struck through as closed, 13/14 renumbered to 305, ten new (0067; the two understated shared-build bullets; the untracked pin; unguarded `bgtr()`; live `sourcepath` write-side defect; the `curcasemode` `Option:` asymmetry; the `Command:` residue; the unsynchronised `static int`; two doc inaccuracies).
4. **What this batch got wrong** — six failures → eight. **7**: 0067 filed its own defect as "latent, allocator dependent, usually silent" and overruled 0065's correct `rc=139`, because the crashing command line was run under `valgrind` and the tool's exit status was reported as the program's — with the internal contradiction (criterion 2 names the three commands that segfault) noted. **8**: 0061's depth counter leaks on a fatal read, unreachable by any harness here (nothing under `tests/` links `libngspice`), on a path 0061's own Resolution had claimed was covered — and it is the exact class `CLAUDE.md` names, which 0058 in the same batch got right. Three new "do differently" items follow from them.