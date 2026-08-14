# Stage 4 receipt — crew C3

## Remediation

**RECEIPT — crew C3, stage 4**

## RESULT per item

| # | item | result |
|---|---|---|
| 1 | test that passes without its own mechanism | **DONE.** Deck now asserts the mechanism; disabling `removecirc` fails it under the real harness. |
| 2 | residual `setplot new` hole | **DONE — fixed, not documented-as-known.** Second test in `com_write()`, new deck, recorded in `0017` decision 4 and `0059` row 8. |
| 3 | `run_all.sh` finding 2 / `FINDINGS.md` | **DONE, and wider than asked:** findings 3, 4 and 7 were stale the same way and are fixed too. |
| 4 | criterion 2's migration-note half | **DONE.** The sentence is in `NEWS` (a new `Bug fixes:` block under Ngspice-47), and `0059`'s Status points there. |
| 5 | receipt arithmetic | **DONE**, corrected in place — and the verification's replacement figure was wrong too. |

## Files changed

| file | why |
|---|---|
| `/home/qflow/dev/ngspice_test/tests/regression/misc/destroy-removed-circuit.cir` `.out` | reads the `destroy` capture back and asserts `DESTROY-NOT-IN-LIST-CAPTURED`; header re-measured |
| `/home/qflow/dev/ngspice_test/tests/regression/misc/write-empty-plot.cir` `.out` | new — the chosen-but-empty plot, plus two controls that pin what must still be written |
| `/home/qflow/dev/ngspice_test/tests/regression/misc/Makefile.am` | `TESTS` + `CLEANFILES` (`wep_*`), then `./autogen.sh` |
| `/home/qflow/dev/ngspice_test/src/frontend/postcoms.c` | the one behavioural change: a second refusal on `com_write()`'s no-vector-list branch, `!plot_cur \|\| !plot_cur->pl_dvecs`; plus a paragraph at `killplot()` re-measuring what the clear's order now costs |
| `/home/qflow/dev/ngspice_test/doc/claude/decisions/0017-write-refuses-an-unchosen-plot.md` | decision 4; amendments to decisions 2 and 3 (both amended, neither reversed); migration note now records the `NEWS` landing; "does not decide" gains `vec_get()`'s fallback |
| `/home/qflow/dev/ngspice_test/doc/codex/issues/0059-...md` | Status, matrix row 8, Resolution for the second test and the two deck corrections |
| `/home/qflow/dev/ngspice_test/NEWS` | the user-visible sentence criterion 2 asked for |
| `/home/qflow/dev/ngspice_test/doc/claude/feedback/ngspice_upstream/repro/run_all.sh` | sections 2, 3, 4, 7 rewritten to what they now measure + a "what this script shows today" banner |
| `/home/qflow/dev/ngspice_test/doc/claude/feedback/ngspice_upstream/FINDINGS.md` | dated "fixed since measured" notes at findings 2, 3, 4, 7; pointer at the top; status line under the asks table |
| `/home/qflow/dev/ngspice_test/doc/claude/batches/2026-08-12-casemode-client-feedback/receipts/stage3-crewC2.md` | the arithmetic, struck and derived |

## Tests added

`tests/regression/misc/write-empty-plot.cir` — `op` → `setplot new` → bare `write` refuses; `write f.raw pi boltz` from the same empty plot still writes; one `let` and the bare write is honoured again. Mode independent, matches its reference under no `-D`, `fold`, `preserve` and `distinguish`.

## RED evidence

Item 1, under the real harness (token survives `check.sh`'s filter):
```
-DESTROY-NOT-IN-LIST-CAPTURED
+DESTROY-NOT-IN-LIST-MISSING
 WRITE-REMOVED-ABSENT
 WRITE-REMOVED-REFUSAL-CAPTURED
FAIL: destroy-removed-circuit.cir
```
Item 2, deck written first, run against the tree as delivered:
```
WRITE-EMPTY-PRESENT / WRITE-EMPTY-REFUSAL-MISSING
-rw-r--r-- 570 wep_empty.raw   Title: Constant values
Date: Thu Aug 13 13:14:50 UTC 2026 == the Build: stamp   Plotname: constants
```
Each guard is pinned separately: removing the first leaves `write-unchosen-plot.cir`'s FRESH case writing a file (its `plot_cur` is the constants plot, which is not empty); removing the second is the RED above.

## GREEN evidence

`WRITE-EMPTY-ABSENT` / `-REFUSAL-CAPTURED`, no file, capture holds `Error: the current plot has no vectors, "wep_empty.raw" not written.`; controls unmoved at 264 and 234 bytes. `make -C build-ver_50/tests/regression/misc check` → **36/36**.

Re-measured for the docs, not assumed: with the `plot_cur_chosen` clear moved below the unlink (backup by md5, rebuilt, binary mtime moved 07:01:41, restored + `touch` + rebuilt 07:02:02) `destroy-removed-circuit.cir` no longer writes a file — the empty-plot test catches it — but the capture reads `the current plot has no vectors` instead of `no plot has been selected`, so the deck still fails on the placement. Crew C2's claim that the reordering writes a 570-byte constants raw is now historical and is marked as such at all four places it appears.

## Suite status

`make -C build-ver_50 check` from a log-and-trs-cleaned tree → **rc=0, 302 PASS, 0 FAIL, 0 ERROR result lines, 0 XFAIL/XPASS/SKIP**. `identity lint: 265 comparisons, baseline matches` plus the 11-comparison selftest; `identity.baseline` untouched (my additions contain no string comparison). The one `ERROR: (internal) tried to destroy non-existent graph` in the log is `casedist/vector-probe-report.cir`'s own stderr, immediately followed by its `PASS`, and predates this batch.

## Left undone, and disagreements to route

- **I extended `run_all.sh` past finding 2.** Findings 3, 4 and 7 had the identical defect — captions asserting what their own output contradicts, and in three places the evidence had silently vanished (`ls save_lower.raw` printed nothing, `rc0.raw` and `destroy_curplot.raw` likewise, `GUARD-NOT-CAPTURED` printed under a `-->` reading GUARD-CAPTURED). Findings 4 and 7 are crew A's and crew B's fixes; I rewrote the captions and the flags, not their fixes. If the orchestrator wants those two reviewed by their own crews, the whole change is in one file.
- **A phantom `v(all)` column.** A bare `write` of an op plot whose scale is the only saved vector emits `v(MidNode)` **and** `v(all)` into the rawfile. It reproduces on the untouched `/usr/local/bin/ngspice` (`ngspice-46`), so it predates everything here and is not mine to fix — but it is now visible in the client's evidence base at `run_all.sh` section 2, and it is unfiled.
- **`com_removecirc()`'s dangling `plot_cur`** stays filed, not fixed (`0017` decision 2). With the empty-plot test in, adding `plot_cur = plot_list;` to `killplot()`'s failing arm would change no `write` outcome and would not weaken the deck, which asserts the internal error directly.
- **`LEDGER.md` not updated** — it is the orchestrator's file. Its stage-3 residual list at lines 82-87 ("a test that passes without its own mechanism", "`op`, `setplot new`, bare `write`") is closed by this work, and the crew C3 line in stage 4 covers all three of my headline items.
- **0061 untouched**, still deferred.

## Verification

- verdict: **NEEDS_WORK**

### Suite

Run by me from a tree with every cached *.log (except config.log) and *.trs deleted first, and with the build confirmed already up to date before the run ('Nothing to be done for all-am', binary mtime 07:14:28 = the delivered sources). `make -C build-ver_50 check` -> rc=0, 302 PASS, 0 FAIL, 0 XFAIL, 0 XPASS, 0 SKIP. Exactly the receipt's numbers. One line matches '^ERROR:' - 'ERROR: (internal)  tried to destroy non-existent graph' at log line 6723, which is tests/regression/casedist/vector-probe-report.cir's own stderr and is immediately followed by 'PASS: vector-probe-report.cir'; it is not a result line and predates the batch, so '0 ERROR result lines' is accurate. tests/lint prints both expected lines: 'identity lint: 265 comparisons, baseline matches' and the 11-comparison selftest, and tests/lint/identity.baseline is unmodified by this crew. Per-directory spot checks: tests/regression/misc 36/36 (re-run 36/36 green after all my experiments and the restore). Every intermediate experiment ran the misc directory from freshly deleted logs too.

### Resolved

- Item 1, the hollow pin - CONFIRMED FIXED, and I broke it two ways to prove it. /home/qflow/dev/ngspice_test/tests/regression/misc/destroy-removed-circuit.cir now reads its 'destroy' capture back. On my own copy with 'removecirc' commented out (/tmp/.../scratchpad/exp/drc_norem.cir) it prints DESTROY-NOT-IN-LIST-MISSING, i.e. it would FAIL - the exact configuration that passed silently before. Separately I re-derived the placement RED non-destructively: moved the `if (pl == plot_cur_chosen) plot_cur_chosen = NULL;` block from above the unlink to just below it in killplot() (/home/qflow/dev/ngspice_test/src/frontend/postcoms.c:1146), rebuilt (binary mtime 07:14:28 -> 07:22:27), and got FAIL: destroy-removed-circuit.cir, 1 of 36, with drc_bare.txt holding 'Error: the current plot has no vectors' where the deck requires 'no plot has been selected' and no drc_bare.raw. Restored by file copy (md5 50d6c685f8b26a531b8f5016e66e880f) + rebuild; 36/36 green again.
- Item 2, the setplot-new hole - CONFIRMED FIXED for the form it names, and each guard is independently pinned, which I measured rather than took on trust. (a) Disabled the second guard only (`if (0 && !wl && (!plot_cur || !plot_cur->pl_dvecs))`), rebuilt (mtime 07:21:01): FAIL: write-empty-plot.cir, 1 of 36, output WRITE-EMPTY-PRESENT / WRITE-EMPTY-REFUSAL-MISSING, and wep_empty.raw reappears at exactly 570 bytes with 'Title: Constant values', 'Plotname: constants' and Date: byte-equal to the Build: stamp. write-unchosen-plot.cir and destroy-removed-circuit.cir stayed PASS. (b) Disabled the first guard only, rebuilt (07:21:53): FAIL on write-unchosen-plot.cir and destroy-removed-circuit.cir, write-empty-plot.cir PASS. (c) Made the second guard over-broad by dropping its `!wl` term: FAIL: write-empty-plot.cir - so the NAMED and FILLED controls really do pin over-refusal, not just under-refusal. Controls measured at 264 and 234 bytes exactly as the receipt states, capture text verbatim. All three decks filtered-match their .out under no -D, fold, preserve and distinguish (12/12 MATCH, run by hand with check.sh's own FILTER).
- Item 3, run_all.sh and FINDINGS.md - CONFIRMED, and the 'wider than asked' claim is true. I ran the delivered /home/qflow/dev/ngspice_test/doc/claude/feedback/ngspice_upstream/repro/run_all.sh in a scratch copy against both binaries: rc=0, 269 lines, and every caption I checked in sections 1-9 now agrees with the output printed under it. I then extracted the staged (pre-C3) version with `git show :.../run_all.sh` and ran it: it prints GUARD-NOT-CAPTURED on one line and '--> GUARD-CAPTURED, and nothing between it and SHELL-CAT-ABOVE' on the next, and its section 3 header reads 'the failed .save still writes a well-formed raw -- of the constants plot' directly above a 296-byte 'Plotname: Operating Point' raw. Both defects are gone in the delivered version. FINDINGS.md's head pointer, the four dated 'Fixed since this was measured' notes at findings 2/3/4/7 and the status line under the asks table are all present and accurate.
- Item 4, the migration note - CONFIRMED. /home/qflow/dev/ngspice_test/NEWS gains a 'Bug fixes:' block under Ngspice-47 (a section that had none) carrying the sentence, and doc/codex/issues/0059's Status names NEWS and points at the Resolution section. Decision 0017's migration-note section records the landing.
- Item 5, receipt arithmetic - CONFIRMED, exactly reproducible with the derivation the receipt publishes. `git diff HEAD -- src/frontend/dotcards.c src/frontend/postcoms.c` gives 88 added lines: 61 comment, 3 blank, 24 code (dotcards 26 = 19+0+7, postcoms 62 = 42+3+17). That is the 88/61/3/24 the correction quotes, and the C2-state figures it derives (61 = 42+2+17, plus 27 C3 lines) are internally consistent with it. The struck original ('63 added lines, all comment') and the earlier verification's replacement ('61 additions, 26 non-comment') are both wrong, as the correction says.
- Disclosed-and-true side claims I spot-checked: the phantom v(all) rawfile column does reproduce on the untouched /usr/local/bin/ngspice (ngspice-46) - `.save v(MidNode)` + bare write yields a 295-byte raw listing v(midnode) and v(all) - so it is pre-existing and correctly routed as not-C3's; ./autogen.sh really ran (write-empty-plot.cir is in tests/regression/misc/Makefile.in and in build-ver_50/tests/regression/misc/Makefile); EXTRA_DIST picks the new deck up automatically via $(TESTS)/$(TESTS:.cir=.out); CLEANFILES covers wep_*.

### Outstanding

- BLOCKING, and the receipt is silent about it: the git index does not commit. I materialised it (`git write-tree` -> 8c3ad824c2e9ae4a3a39764717d9d87876938cc6, extracted with `git archive`) and the staged tree contains ZERO of crew C's src fix - src/frontend/postcoms.c, plotting/plotting.c, vectors.c, dotcards.c, spec.c, com_fft.c and include/ngspice/fteext.h are all byte-identical to HEAD in the index (grep for plot_cur_chosen in the staged postcoms.c returns 0) - while tests/regression/misc/write-unchosen-plot.cir and destroy-removed-circuit.cir ARE staged with their .out files AND listed in the staged TESTS. I measured that exact configuration in experiment (b) above: both decks FAIL without the guard. So `git commit` as the index stands produces a commit whose `make check` fails two tests. Crew B is in the same shape (src/frontend/inpcom.c, src/sharedspice.c unstaged, its casedist decks staged). Additionally tests/regression/misc/write-empty-plot.cir and .out are fully staged while the staged Makefile.am does not list write-empty-plot.cir in TESTS - a staged test nothing runs. This is the same defect class stage 3 raised against crew A, inverted, and C3 added its own new test files to the index without staging either the Makefile.am hunk or the postcoms.c guard they need.
- SUBSTANTIVE, found with my own deck, not theirs: the new guard keys on the command's syntax, not on its subject, so spelling the default argument bypasses both refusals. com_write() substitutes a static `all` wordlist when wl is empty (src/frontend/postcoms.c:583 `static wordlist all = { "all", NULL, NULL };`), but both guards test `!wl`. Measured on the delivered binary: `op` / `setplot new` / `write y_all.raw all` -> rc=0, SILENT, y_all.raw = 570 bytes, 'Title: Constant values', 'Plotname: constants', $curplot reads unknown1 - byte-for-byte row 8's file, the state decision 0017 decision 4 exists to close, one keystroke from the form it closes. The same spelling reopens row 1 (`write y_fresh.raw all` with nothing run: rc=1, 570 bytes) and row 7 (`op` / `destroy` / `write y_destroy.raw all`: rc=0, 570 bytes, Date: byte-equal to the Build: stamp). This is pre-existing rather than a regression, and decision 0017's 'does not decide' item 4 covers it generically ('any named lookup that misses in the current plot and hits in the constants plot still resolves silently'), but `all` is not a name a user picks to mean the constants - it is the command's own default, and com_write()'s header comment says the two spellings are the same operation. 0059's matrix, NEWS ('with no vector list...') and the receipt all leave a reader believing the state is closed when it is one word away. Nothing in tests/ or examples/ spells `write <file> all` today, so nothing regresses either way; it needs naming in 0059/0017 at minimum, and the guard would move to the resolved subject at best.
- The phantom v(all) column is disclosed as unfiled and still sits uncommented in the client-facing evidence: run_all.sh section 2 prints 'Variables: 0 v(MidNode) 1 v(all)' directly under a caption reading '--> both resolve, and the raw carries the netlist's own spelling'. Verified pre-existing on ngspice-46, so not C3's to fix - but a client reading the section is shown a bogus variable with nothing saying so.
- run_all.sh section 4's caption '.print and .save name the token alike now' sits above numbers that are not alike: asym_save_nosuch fold stderr-hits=1 vs asym_print_nosuch fold=2, asym_save_mid distinguish=1 vs asym_print_mid distinguish=3. The caption does state its metric and the numbers are all non-zero, so this is looseness rather than the contradiction class C3 was sent to fix - but it is the same shape, in a section C3 rewrote.
- LEDGER.md not updated (disclosed as the orchestrator's file); 0061 untouched and still deferred (correct); com_removecirc()'s dangling plot_cur filed not fixed (correct - I confirmed adding `plot_cur = plot_list;` would not weaken destroy-removed-circuit.cir, because the deck now asserts the internal error directly rather than only the refusal).

### Regressions

- None found. I hunted specifically for false refusals with my own decks and found none: bare `write` after ac (3551 B), tran (161430 B), tran+linearize (160330 B, curplot tran2), dc sweep (649 B), noise (279 B, curplot noise2), tf (337 B, curplot tf1), destroy-all + load of a rawfile (288 B, curplot op1) and tran+linearize+fft (16278 B, curplot sp2) all still write, rc=0, zero refusal lines. The set of plot_cur writers is closed and I enumerated it: 22 sites, every request-answering one records the choice, the two killplot() fallbacks deliberately do not, and com_let.c/xpressn.c are save/restore pairs.
- No other crew's uncommitted hunks were reverted. Crew A's src/frontend/outitf.c, runcoms.c and src/xspice/evt/evtplot.c are worktree-identical to the index (git diff --quiet passes), tests/lint/identity.baseline is untouched at mtime 00:50, and crew B's inpcom.c latch hunks are present (12 added lines matching latch/announce/once). All three crews' decks pass in my full run: casemode-announce-report.cir, save-event-node{,-case,-case-split}.cir, save-multianalysis-report.cir, write-unchosen-plot.cir, destroy-removed-circuit.cir, write-empty-plot.cir.
- Tree left exactly as delivered: src/frontend/postcoms.c restored to md5 50d6c685f8b26a531b8f5016e66e880f, binary rebuilt from it, git status still 331 entries, postcoms.c diff still 62 insertions. Only the file's mtime moved (07:25 vs 07:14 at delivery).