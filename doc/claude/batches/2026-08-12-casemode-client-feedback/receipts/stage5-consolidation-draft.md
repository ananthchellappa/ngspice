# Stage 5 — consolidation agent's return

Report written to `/home/qflow/dev/ngspice_test/doc/claude/batches/2026-08-12-casemode-client-feedback/CLOSEOUT.md`. Nothing else in the tree was changed (`git diff --stat` is still 26 files / 714 insertions / 65 deletions, `git status --short` still 274 entries). Full report follows.

---

# Closeout — casemode client-feedback batch, 2026-08-12/13

Independent pass over `LEDGER.md`, all 23 receipts, issues 0056–0064, and the working tree. The index is empty; everything is a working-tree change. Nothing is committed.

Measurements labelled **[verified here]** were run by this pass on 2026-08-13 against `build-ver_50/src/ngspice` rebuilt from the tree as it stands (no source file was modified; `make -C build-ver_50/src` reported nothing to do, so the binary already matched), and against a second tree materialised under `/tmp`.

**Tree state, measured from cleared logs.** `make -C build-ver_50 check` after deleting every cached `*.log` (except `config.log`) and `*.trs` under `build-ver_50/tests`: **rc=0, 303 PASS, 0 FAIL, 0 SKIP, 0 XFAIL**, `identity lint: 265 comparisons, baseline matches` plus the 11-comparison selftest. The one `^ERROR:` line is `casedist/vector-probe-report.cir`'s own stderr and predates the batch. Per directory: `case` 116, `casedist` 27, `misc` 37, `pipe` 18, `xspice/digital` 8, `xspice/casedist` 18.

## 1. Per issue

### 0056 — `.save` matches vector names byte-exactly — FIXED AND VERIFIED

Verified end to end by this pass, including the RED half (part 2).

- **Code:** `src/frontend/outitf.c` — `name_unwrap()` factored out; `name_eq()` keeps `strcmp` and is now the single reference-vector caller; the four query callers (`:298`, `:312`, `:445`, `:450`) call the new `name_eq_query()`, which is `vec_name_eq()`. `OUTattributes()`'s two `strcmp`s classified in place with `case-lint: stored`. +60/−20 of that file's total +276/−19.
- **Also:** `tests/lint/identity.baseline` (three `outitf.c` records removed with a header note, 265 records); `doc/claude/decisions/0001-distinguish.md` — the "and no vector-name matcher outside them" sentence corrected (criterion 9).
- **Tests:** `tests/regression/case/save-name-case.cir` + `save-name-case-lower.cir` twin; `tests/regression/case/plot-scale-name-twin.cir` (guard for the compare that deliberately stays byte-exact — added late, after a verifier measured that mutating `name_eq()` to `vec_name_eq()` passed the entire suite); `tests/regression/casedist/save-name-case.cir` (mode-boundary guard, passes before and after); `tests/regression/pipe/save-name-case.cmd`.
- **[verified here]** 0056 alone on top of `720c8743a`, built in a scratch tree: `make check` **291 PASS, 0 FAIL, rc=0**, lint 265. With only `outitf.c` reverted to HEAD and rebuilt: `FAIL: save-name-case.cir`, `FAIL: save-name-case.cmd`, `PASS: save-name-case-lower.cir`. So this commit moves `make check` on its own — what 0057 criterion 8 asked for and every stage recorded as owed.
- **Residual:** Resolution 7's closing sentence, "`CLAUDE.md`'s sentence still names only `keyword`", is now false — `CLAUDE.md` was edited late in the batch.

### 0057 — a failed `.save` names no token — FIXED AS A NARROWER THING THAN IT ASKED FOR

The issue to read before believing any receipt in this batch. What shipped is a **case near-miss report only**. The general "name the unresolved token" report — criterion 1, and the client's finding 4 headline — was built, shipped through three verification rounds, and **withdrawn**.

- **Code:** `src/frontend/outitf.c` — `report_save_case_miss()` (gated on `NG_CASE_DISTINGUISH`, then `ckt_knows_name()`, then a twin scan over `dataNames[]` with a `ckt->CKTnodes` fallback), `case_twin_p()`, `ckt_case_twin()`, `save_miss_first_time()` + `OUTsaveMissClear()`; `outitf.h`; `runcoms.c` (`OUTsaveMissClear()` at the top of `dosim()`, which makes the unit one simulation rather than one `OUTpBeginPlot()`); `src/xspice/evt/evtplot.c` + `src/include/ngspice/evtproto.h` (`Evt_Ckt_Has_Node()`); `AGENTS.md`, `CLAUDE.md`, `tests/bin/identity_lint.sh` (the `case-lint` reason words); `doc/claude/decisions/0001-distinguish.md` (new decision-2 table row).
- **Tests:** `tests/regression/misc/{save-undef-report, save-multianalysis-report, save-crossplot-report}.cir`; `tests/regression/casedist/{save-undef-report, save-near-miss-node-list}.cir`; `tests/xspice/digital/save-event-node.cir`; `tests/xspice/casedist/save-event-node-case-split.cir`. Resolution 9 records a nine-mutation matrix; M9 — restoring the wide report — fails all seven decks, which is what pins the withdrawal.
- **[verified here]** `.save v(In) v(midnode)` against a net `MidNode`: `fold` 0 lines, `preserve` 0, `distinguish` exactly 1 (`Warning: no vector named 'midnode'; 'MidNode' differs only in case (casemode=distinguish)`). `.save v(In) v(nosuchnode)` under `distinguish`: **0 lines** (the withdrawn half). `.save onoise_spectrum onoise_total` beside `.noise … 1`: silent, identical to stock `ngspice-46`. Both tracked example decks the stage-4 verifier caught — `examples/transient-noise/noi-ring51-demo.cir`, `examples/probe/F5TurboV2-Probe.cir` — are silent again (0 `no vector named` lines each).
- **Crew vs verifier:**
  - A3 marked item 4b (`onoise_total` cross-plot) **DONE**; its verifier called it fixed in one direction only and produced a fresh false positive on a correct deck. **The verifier was right**, and the point is moot for a stronger reason: the mechanism A3 added for it (`save_resolved_note()` / `save_miss_resolved`) is **not in the tree** — round four replaced it.
  - A3's receipt says "No committed deck acquired noise"; its verifier found two shipped, tracked `examples/` decks that did. **The verifier was right**; A3's census covered `tests/` only.
  - A3's receipt lists `tests/xspice/case/save-event-node-case.cir` among tests added. **That file does not exist in the tree** — correctly so, since under `preserve` the narrowed report cannot fire, but the receipt misdescribes what it left behind.
- **Residuals** (Resolution 9, not fixed): an event node's own case near miss is unreported from this site; the `.print` route's wording is inherited-wrong and no deck shape in this tree can assert a change; `.save @r99[i]` for a non-existent device is silent.

### 0058 — casemode diagnostics fire once per file read — FIXED AND SIGNED

The only issue whose verifier returned **SIGN**, after re-running the whole suite from cleared logs and rebuilding the rejected latch rule to measure it.

- **Code:** `src/frontend/inpcom.c` (establish/announce split, `ng_case_last_outcome`/`ng_case_last_unknown`, `case_outcome_is_new()`, both `fprintf(stderr,…)` → `cp_err`); `src/include/ngspice/fteext.h` (declaration); `src/sharedspice.c` (call from `totalreset()`).
- **Tests:** `tests/regression/casedist/casemode-announce-report.cir`, six cases, every assertion a count with a `CAPTURE-READ` token beside each silence. `doc/claude/decisions/0016-case-mode-announcement-latch.md` records the rule and four rejected alternatives.
- **Residuals:** (a) a latent one-liner found by B2's verifier and closed by nobody — under `--enable-gc` (`HAVE_LIBGC`) `tfree()` expands to nothing, so `inp_case_announce_reset()` leaves `ng_case_last_unknown` set and a repeated misspelled casemode stays silent after a reset. It is written down **only in that receipt**, nowhere in `doc/codex/`. (b) The `totalreset()` call site still cannot be exercised end to end, because of 0062.

### 0059 — `write` emits the constants plot when no analysis ran — FIXED (4 rows), 1 row DECIDED

- **Code:** `plotting/plotting.c` (`plot_cur_chosen`); `fteext.h` (declaration); `vectors.c` (`plot_setcur()`'s four arms); `dotcards.c` (seven sites); `spec.c`, `com_fft.c` (three plot-creating commands); `postcoms.c` (two refusals in `com_write()` keyed on a `whole_current_plot` predicate; the `plot_cur_chosen` clear in `killplot()`); `NEWS`.
- **Decision:** `doc/claude/decisions/0017-write-refuses-an-unchosen-plot.md`, five decisions, four "does not decide" items.
- **Tests:** `tests/regression/misc/{write-unchosen-plot, destroy-removed-circuit, write-empty-plot, write-spelled-all}.cir`.
- **[verified here]** `write y_fresh.raw all` with nothing run → refused, no file. `op` / `setplot new` / `write y_setplot.raw all` → refused ("the current plot has no vectors"), no file. `op` / `destroy` / `write y_destroy.raw all` → refused, no file. Control: `op` then bare `write` after a good analysis still writes (252 bytes). The syntax-vs-subject bypass the stage-4 verifier found is closed.
- **Crew vs verifier:** the stage-2 verifier's problem 3 said the `plot_cur_chosen` clear belongs *after* the unlink. C2 inverted it and kept the clear above, with a deck; C2's own verifier reproduced the RED both ways (moved the block, rebuilt with the binary mtime confirmed moving, got a 570-byte constants raw with a successful `op` behind it). **I believe C2 and its verifier**: the reordering re-opens row 1 on a deck-reachable path.
- **Residuals:** 0063 filed not fixed (decision 0017 decision 2); `Error during 'write': no writable vector found.` still on literal `stderr`; rawfile analysis provenance — the only thing that closes the consumer's half — untouched.

### 0060 — `$casemode` reports the request, not the effect — DEFERRED, FILED ONLY

No code, no test; `Open` / `Unresolved.` Correctly not closed.

- **Waits on:** the *name* of the read-only companion variable. Everything else is verified against the source (live read of `inp_case_mode()`, resolution ahead of `cp_enqvar()`'s `pl_env` loop, `cp_usrset()`'s `US_READONLY` arm, `cp_usrvars()` for the `set` listing) and costed at ~30 lines plus two decks. Crew B recommends `effectivecasemode`, second choice `casemodenow`, and argues against `curcasemode` because `cur*` means *of the current plot* here and 0061 is exactly a plot-scoped value answering a session-scoped question.
- **Verifier's counter, which I agree with:** crew B's load-bearing premise — "it can never be renamed once shipped" — is false. `git show pre-master-47:src/frontend/inpcom.c | grep -c casemode` is 0, so the feature is unreleased branch work and the name is renameable before merge. Escalating is defensible; treating it as a hard blocker is a judgement the owner should re-take.

### 0061 — a rawfile `Option:` line reconfigures the next netlist read — DEFERRED, FILED ONLY

No code, no test; `Open` / `Unresolved.` Re-measured and still reproducing.

- **Waits on:** how much of a loaded file's `Option:` payload may reach the session's variable lookups — whether `plot_cur->pl_env` stays a namespace `cp_getvar()` consults. Three options are worked out and costed (A narrow the readers / B narrow the writer / C make `pl_env` data); crew C recommends **C**, with the `US_READONLY` question (`options.c:415-417`) answered in the same change. Entangled with 0060 and adjacent to the documented, deliberate `Command:`-executes-via-`cp_evloop()` property.
- This also blocks FINDINGS finding 1: its proposed carrier is disqualified by this issue.

### 0062 — shared build after `ngSpice_Reset()` — FILED ONLY (doc-only, correctly)

- The filer **overturned the attribution it was given**: `CKTmodCrt()` is a symptom. `totalreset()` is a de-initialiser by design (`d0ae65acc`'s own message; `examples/shared/shx.c` re-`Init`s after every reset) and nothing states or enforces it; `ngSpice_Command()`'s guard returns 1 in silence while the `no_init` message 23 lines below is dead code, and `ngSpice_Circ()` checks nothing. Which frame dies is selected by the function-static at `src/spicelib/parser/inp2v.c:20`, so a fix aimed at `CKTmodCrt()` relocates the crash.
- Minimal repro is four API calls; three probes committed under `repro/shared/`. Invisible to `make check`. I did not re-run the probes (they need the gitignored 98 MB `build-shared/`).

### Also filed in the last hours, not in the brief's list

`0063` (`removecirc` leaves `plot_cur` outside `plot_list`) and `0064` (a wildcard matching exactly one vector is renamed to the wildcard — the phantom `v(all)` column sitting in the client's evidence base). Both pre-existing, mode-independent, reproduce on stock `ngspice-46`, both filed rather than fixed.

## 2. The commit split

Crew A3's four-commit proposal was written against the **staged index at 07:32** and is wrong about the tree as it stands:

- omits `plot-scale-name-twin.cir` from 0056 (it is 0056's criterion 2);
- omits `src/frontend/spec.c` and `src/frontend/com_fft.c` from 0059;
- omits `NEWS`, `write-empty-plot.cir`, `write-spelled-all.cir`, decisions `0016`/`0017`, `CLAUDE.md`, and every `doc/codex/issues/` file;
- lists `tests/xspice/case/save-event-node-case.cir`, which does not exist;
- treats `doc/claude/decisions/0001-distinguish.md` and `src/include/ngspice/fteext.h` as single-owner files. **Each has two independent hunks owned by two different commits.**
- says nothing about the four `Makefile.am` files whose `TESTS` lists interleave three commits' decks, nor about the ~190 stray run artifacts now in `tests/` and `examples/`.

### Recommended sequence — six commits

Every `.out` under `tests/` is hidden by `tests/.gitignore:3` and needs **`git add -f`**. A plain `git add -A` both drops the references and sweeps in the junk.

**C0 — `docs: the xschem casemode report and its repro set`** — `doc/claude/feedback/ngspice_upstream/**` (65 files: `FINDINGS.md`, `README.md`, `repro/` decks, `run_all.sh`, `hdr_variants.sh`, `reads/`, `spiceinit*/`, `count/`, `shared/*.c`, two `.gitignore`s). First, because every issue file in C1–C4 cites these paths and committing an issue whose evidence is absent is the exact defect the stage-2 verifier raised. Wart: `run_all.sh`/`FINDINGS.md` already describe fixes landing in C1–C4, so as commit 0 they read one commit ahead; the alternative leaves four commits of dangling citations. Green trivially.

**C1 — `fix: .save resolves a vector name the way every other site does` (0056)**
- `src/frontend/outitf.c` — **0056 hunks only** (`name_unwrap()`, the `name_eq()`/`name_eq_query()` split, four query call sites, `OUTattributes()`'s comment and two `case-lint: stored` markers)
- `tests/lint/identity.baseline`
- `tests/regression/case/{save-name-case.cir,.out, save-name-case-lower.cir,.out, plot-scale-name-twin.cir,.out, Makefile.am}`
- `tests/regression/casedist/{save-name-case.cir,.out}` + that `Makefile.am`'s `TESTS` entry for that deck only
- `tests/regression/pipe/{save-name-case.cmd, Makefile.am}`
- `doc/claude/decisions/0001-distinguish.md` — **the "no vector-name matcher outside them" paragraph only**
- `AGENTS.md`, `CLAUDE.md`, `tests/bin/identity_lint.sh` (the reason words; `stored` is introduced here, `neither` is documented one commit early — harmless, nothing reads the reason word)
- `doc/codex/issues/0056-…md`, plus regenerated `Makefile.in`s from `./autogen.sh`

**C2 — `fix: a .save name that misses by case names the twin` (0057)**
- `src/frontend/outitf.c` (the rest), `outitf.h`, `runcoms.c`, `src/xspice/evt/evtplot.c`, `src/include/ngspice/evtproto.h`
- `tests/regression/misc/{save-undef-report, save-multianalysis-report, save-crossplot-report}.{cir,out}` + `TESTS`, `sur_*`/`smr_*`/`scr_*` `CLEANFILES`, header comments
- `tests/regression/casedist/{save-undef-report, save-near-miss-node-list}.{cir,out}` + `TESTS`, `sur_*`/`snm_*` `CLEANFILES`
- `tests/xspice/digital/{save-event-node.cir,.out}` + `Makefile.am`; `tests/xspice/casedist/{save-event-node-case-split.cir,.out}` + `Makefile.am`
- `doc/claude/decisions/0001-distinguish.md` — **the new decision-2 table row only**
- `doc/codex/issues/0057-…md`; no `identity.baseline` change (its comparisons are annotated in place)

**C3 — `fix: announce the case mode once per run, not once per file read` (0058)**
- `src/frontend/inpcom.c`, `src/include/ngspice/fteext.h` (**`inp_case_announce_reset()` hunk only**), `src/sharedspice.c`
- `tests/regression/casedist/{casemode-announce-report.cir,.out}` + `TESTS` + `cma_*` `CLEANFILES`
- `doc/claude/decisions/0016-…md`, `doc/codex/issues/0058-…md`

**C4 — `fix: a bare write refuses a plot nothing chose` (0059)**
- `plotting/plotting.c`, `vectors.c`, `dotcards.c`, `postcoms.c`, `spec.c`, `com_fft.c`, `src/include/ngspice/fteext.h` (**`plot_cur_chosen` hunk only**)
- `tests/regression/misc/{write-unchosen-plot, destroy-removed-circuit, write-empty-plot, write-spelled-all}.{cir,out}` + `TESTS`, `wup_*`/`drc_*`/`wep_*`/`wsa_*` `CLEANFILES`, header comments
- `NEWS`, `doc/claude/decisions/0017-…md`, `doc/codex/issues/0059-…md`, `0063-…md`, `0064-…md`

**C5 — `docs: file the three casemode defects this round did not fix`** — `doc/codex/issues/{0060,0061,0062}-…md`, and optionally the batch directory (LEDGER, 23 receipts, this file).

C3 and C4 are independent and may be swapped. C2 must follow C1 (its decks assert silences that depend on `name_eq_query()`'s fold). C1 must be separate from C2 — that is 0057 criterion 8, the one sequencing claim asserted at every stage and never demonstrated.

**Test-count arithmetic as a check:** HEAD 286 → C1 291 (+5: case +3, casedist +1, pipe +1) → C2 298 (+7: misc +3, casedist +2, xspice/digital +1, xspice/casedist +1) → C3 299 (+1) → C4 303 (+4). 303 is the measured whole-tree number, so the split accounts for every new deck and orphans none.

### What was actually run to verify C1

1. `git archive HEAD | tar -x -C /tmp/…/scratchpad/c1` (HEAD = `720c8743a`).
2. A 0056-only `outitf.c` produced by removing, from the working copy, the 0057 function block, the `report_save_case_miss()` call in `beginPlot()`, the five 0057 static declarations and the `#ifdef XSPICE` include block. Residue grep for `report_save_case_miss|ckt_knows_name|Evt_Ckt_Has_Node|OUTsaveMissClear|save_miss_reported|case_twin_p|evtproto`: none. Diff vs HEAD: +60/−20.
3. The rest of C1's files copied in; `tests/regression/casedist/Makefile.am` hand-edited to register `save-name-case.cir` only; `doc/claude/decisions/0001-distinguish.md` reconstructed from HEAD with the 0056 paragraph alone.
4. `./autogen.sh` → rc=0. `mkdir build && cd build && ../configure` → rc=0. `make -j8` → **rc=0**.
5. `make check` → **rc=0, 291 PASS, 0 FAIL**; `identity lint: 265 comparisons, baseline matches` + selftest 11; per directory case 116, casedist 24, misc 30, pipe 18, xspice/digital 7, xspice/casedist 17.
6. RED: `outitf.c` replaced by `git show HEAD:src/frontend/outitf.c`, `make -C src -j8` (binary mtime moved 10:21:19 → 10:21:51), `make -C tests/regression/case check` and `…/pipe check` → **`FAIL: save-name-case.cir`**, **`FAIL: save-name-case.cmd`**, `PASS: save-name-case-lower.cir`, `PASS: plot-scale-name-twin.cir` (the last passes both ways by design — its RED is the `name_eq()` mutation, not HEAD).
7. Restored, rebuilt (mtime 10:22:14), case + pipe + lint green again.

C2–C4 were **not** materialised or built. The whole tree is green (303/0) and each is source that compiles independently plus decks exercising only its own change — but that is reasoning, not measurement. The most likely place for a middle commit to break is the hand-splitting in step 3: the two-owner `fteext.h` and `0001-distinguish.md`, and the four interleaved `Makefile.am`s.

### Before committing anything

`git status --short` is 274 entries and most must not be committed: **43 untracked artifacts under `examples/`** (`.vcd`, `.raw`, `.data`, `.plt`, `.py`, `.out`, `.log`, `.svg`, `.s2p`, `.snap`) and **~150 under `tests/`** (`bnr_*`, `cma_*`, `sur_*`, `snm_*`, `snr_*`, `sfp_*`, `vlr_*`, `vpr_*`, `vdb_*`, `vdp_*`, `vdw_*`, `vsc_*`, `vsr_*`, `vuc_*`, `wup_*`, `drc_*`, `wsa_*`, `scr_*`, `smr_*`, `dun_*`, `bun_*`, `dpc_*`, `pkc_*`, `abr_*`, `enr_*`, `ses_*`, `sen_*`, `*.raw`, `*.log`, `PrintTail.txt`, `test.log`, `VPR_*.CIR`) — the sub-decks and captures the report decks write, produced by hand-runs **in the source tree**. `doc/claude/suggestions/next-session-*.md` predates this batch. Only ~33 new files under `tests/` are real: 16 decks, 16 `.out`s, one `.cmd`.

## 3. Residuals and known gaps

| # | What is left | Where it is written down | Cost |
| --- | --- | --- | --- |
| 1 | **The client's finding 4 is half answered.** An unresolved `.save` name with no case variant is silent again — rc=0, vector missing, nothing on either stream. This is the defect 0057 was filed for; only its case half shipped. | `0057` Status + Resolution 1; `FINDINGS.md` marks finding 4 HALF; `run_all.sh` §4 | A design decision first (what may be reported without firing on correct decks; `decisions/0008` decision 1 is the precedent that does not transfer unchanged), then ~1 day |
| 2 | **0060 blocked on one word** — the variable's name. Everything else specified and verified. | `0060`; `receipts/stage2-crewB.md` | Owner picks the name; ~30 lines + 2 decks |
| 3 | **0061 blocked on a contract call** — how much of `pl_env` a loaded file may reach. | `0061`; `receipts/stage2-crewC.md` (options A/B/C, recommends C) | A decision doc, then ~1 day for C incl. the `US_READONLY` half |
| 4 | **0062 unfixed** — after `ngSpice_Reset()`, `ngSpice_Command()` runs nothing and `ngSpice_Circ()` segfaults. Invisible to `make check`. | `0062` + three committed probes | A lifecycle decision, then ~½ day; plus a shared-build test target to guard it |
| 5 | **`--enable-gc` latch leak (0058).** `tfree()` is a no-op under `HAVE_LIBGC`, so a repeated misspelled casemode stays silent after a reset. **Found by a verifier, closed by nobody, recorded only in `receipts/stage3-crewB2.md`.** | that receipt and this file only | One line (`ng_case_last_unknown = NULL;`) + a sentence in 0058 |
| 6 | **0063** — `removecirc` leaves `plot_cur` outside `plot_list`; `display` lists vectors of a plot `setplot` denies. | `0063`, `0017` decision 2 | ~1 line + a deck; read decision 0017 first |
| 7 | **0064** — a wildcard matching exactly one vector is renamed to the wildcard (`v(all)` beside `v(midnode)`). In the client's evidence base. | `0064`, `run_all.sh` §2 | Unscoped; touches wildcard expansion |
| 8 | **An event node's own case near miss is unreported** by 0057's site. The silence is asserted by a deck, so closing it is a visible change. | `0057` Resolution 9; `save-event-node-case-split.cir` SPLIT | ~15 lines (`Evt_Ckt_Case_Twin()`) + a deck |
| 9 | **The `.print` route's `can't parse '%s': ignored` names a `gettoks()` fragment**, and **no deck shape in this tree can assert a change** — `ft_savedotargs()` runs only from `src/main.c`'s batch path. | `0057` Resolution 9 | A harness decision before any code |
| 10 | **`.save @r99[i]` for a non-existent device is silent** in stock and here. | `0057` Resolution 9 | Own issue; not yet filed |
| 11 | **`Error during 'write': no writable vector found.` is on literal `stderr`**, uncapturable by a deck. | `0059` criterion 3; `0017` "does not decide" 1 | One line, user-visible stream change |
| 12 | **No analysis provenance in the rawfile** — the only thing that closes the consumer's half of 0059. | `0017` "does not decide" 3 | Format decision; blocked with finding 1 by 0061 |
| 13 | **FINDINGS finding 1 blocked** — its carrier `Option:` is disqualified by 0061. | `LEDGER.md`; `0061` | Decision first |
| 14 | **F5, F9, F6** — `.spiceinit` beating `-D` has no decision record; two identifiers folding together needs a decision before code; F6 folds into open `0048`. | `LEDGER.md` "Not in this batch" | Three short decision docs |
| 15 | **`alter-rebin*` decks are flaky** under `doc/claude/scripts/case_differential_sweep.py` (DIFF 3/4/4 on an *unchanged* binary), making that tool's count meaningless run to run. | `receipts/stage3-crewB2.md` verification only | Unfiled; worth an issue |
| 16 | **`run_all.sh` §4's caption** ("`.print` and `.save` name the token alike now") sits above counts that are not alike. | `receipts/stage4-crewC3.md` verification | 10 minutes |
| 17 | **Stale numbers in the write-ups.** `0056` Resolution 7 says `CLAUDE.md` names only `keyword` (no longer true); `0057` Resolution 8 quotes 302 PASS (tree is 303). | the issue files | 15 minutes |
| 18 | **The batch record is incomplete.** `LEDGER.md` still shows stage 5 `[ ]` for crews A4 and C4 and for the closeout, and **there are no stage-5 receipts** — the round that withdrew 0057's report, closed the `write … all` bypass, filed 0063/0064 and rewrote the client-facing artifacts. Source mtimes 08:44–09:45 vs the last receipt at 07:32. | here | 30 minutes to reconstruct from the issue Resolutions, or accept the loss |
| 19 | **~190 stray artifacts** in `tests/` and `examples/`. | here | `git clean -n` review before committing |

## 4. What this batch got wrong

**A diagnostic was grown by guard instead of being specified, three times.** 0057 shipped as "name every unresolved `.save` token, in every mode". Round 1's verifier found a correct multi-analysis deck (`.save v(out)` + `.noise`) that warned; round 2 added `ckt_knows_name()`, and its verifier found XSPICE event nodes warned; round 3 added `Evt_Ckt_Has_Node()` and a cross-plot memory, and its verifier found `onoise_total` (order-dependent, because `.noise` opens two plots), `dout(state)` reduced to `state` by `gettoks()`, and two **shipped, tracked** `examples/` decks. Round 4 threw all of it away and used the condition in `decisions/0001` decision 2 — fire only when the resolution fails **and** a case variant exists — whereupon every false positive went silent structurally. Three remediation rounds across stages 2, 3 and 4, and the answer was in a decision record the issue itself had quoted on the day it was filed. *Rule: when the second guard is proposed for the same diagnostic, stop and re-derive the condition from the decision record. A predicate needing a new exception per verification round is the wrong predicate; say so in the receipt instead of adding the exception.*

**"Correct decks stay silent" was tested against `tests/` and called done.** A3's census wrote "No committed deck acquired noise"; the verifier found two tracked `examples/` decks printing new warnings. *Rule: that claim is measured over `tests/` **and** `examples/` — 626 `.cir` files, as the verifier's own census showed — or it is not made.*

**A test was written that passed with its own mechanism removed.** C2's `destroy-removed-circuit.cir` redirected `destroy`'s stderr into `drc_kill.txt` and never read it; commenting out `removecirc` changed nothing. Caught by a verifier, not by its author — and the directory's `Makefile.am` comment already claimed the assertion existed. *Rule: every capture a deck writes is read back and asserted on in the same deck. The author's own mutation check is "comment out the command the deck is named for", not "disable the fix".*

**Four crews staged work into one index and produced a tree that could not build.** Crew A staged `outitf.c` but not `identity.baseline`, so the staged tree failed its own lint. Crews B and C staged decks while leaving the `src` those decks need unstaged; C3 added new test files to the index without the `Makefile.am` hunk or the guard they depend on. Measured by two independent verifiers: `make check` on the extracted index exits 2 after 51 PASS and aborts in `tests/regression/misc`, never reaching the lint directory the sequencing claim was about; a `git write-tree` extraction showed **zero** of crew C's `src` fix present. The orchestrator's fix — `git reset`, keep the working tree — was right, but arrived at stage 4. *Rule: the index is not a shared workspace. If crews run sequentially in one tree, nobody stages anything; the commit split is written at the end and verified by materialising it — one `git archive` plus one build, which is the only way "each commit is green" means anything.*

**Fixes invalidated the evidence in their own issues, and it took two rounds to notice.** 0056's fix made `.save v(midnode)` resolve under `preserve`, falsifying six repro decks quoted in 0059, most of 0057's transcripts, and `run_all.sh`'s captions for findings 2, 3 and 4 — including a header reading "of the constants plot" printed directly above a healthy 296-byte `Operating Point` raw, and a `-->` caption reading `GUARD-CAPTURED` above a line reading `GUARD-NOT-CAPTURED`. Every one was found by a verifier in a later stage, not by the crew whose fix caused it. *Rule: a crew that changes behaviour re-runs the transcripts of every issue in its own batch, not only its own — `grep -l` the repro deck names across `doc/codex/issues/` and the repro script. Two minutes.*

**Receipts asserted arithmetic nobody could reproduce.** "63 added lines, all comment" (wrong on both halves); the verifier's replacement "61 additions, 26 non-comment" (also wrong); "recorded at 12 sites" (14); "577 lines" (578); "8 lines disappeared" (nothing ever disappeared — reconstructed from 28 edits, monotonic growth). Three of these consumed a whole remediation item each. *Rule: any number in a receipt is quoted with the command that produced it, or left out.*

**Two receipts claimed things untrue of the tree.** A3 listed a test file that is not there; A2 wrote "No `identity.baseline` change was needed" while the tree carried a +20/−3 change to exactly that file (true of that crew's second round, false as a description of the tree); B2 wrote that the `.out` was "never edited after it was written" and it had been — additively, so the substance survived, but that is precisely the claim a verifier must not take on trust. *Rule: the "files changed" and "tests added" sections are generated from `git status`/`git diff`, not written from memory.*

**The bookkeeping stopped one round before the work did.** The last receipt is 07:32; source files were still changing at 09:45. The round that withdrew 0057's report, closed the `write … all` bypass, filed 0063 and 0064 and rewrote the client-facing artifacts left no receipt, and `LEDGER.md` still shows it unstarted. The issue Resolutions are good enough to reconstruct what happened, which is the only reason this closeout could be written. *Rule: the ledger entry and the receipt are part of the round, not a follow-up; a round with no receipt did not happen, and the next reader has to take the diff's word for it.*

**What went right, and is worth keeping.** Every stage was adversarially verified by an agent that re-ran the suite itself. Seven of the eight fix-stage verifications returned NEEDS_WORK, and every substantive defect named in this closeout was found by a verifier rather than a crew — including two inversions where the crew was right and the *verifier's own predecessor* was wrong (`killplot()`'s clear placement; 0062's attribution to `CKTmodCrt()`). The mutation matrices — nine in 0057, four in 0058, three in 0059 — are why the decks in this batch pin what they claim to pin, and are what caught the untested `name_eq()` byte-exactness that had been asserted since stage 1. Both are cheap; both should be standing practice.