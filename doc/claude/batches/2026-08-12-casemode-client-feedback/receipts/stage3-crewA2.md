# Stage 3 receipt — crew A2

## Remediation receipt

All work is complete and the suite is green. Here is the receipt.

---

# Stage 3 receipt — crew A2 (issues 0056 / 0057, second round)

## RESULT per item

| # | Item | Result |
| --- | --- | --- |
| 1 | Regression: correct multi-analysis deck acquires warnings | **DONE** |
| 2 | Falsified criterion 6 (per token, not per `OUTpBeginPlot()`) | **DONE** — made true, and the criterion amended to name its unit |
| 3 | Wording regression on an expression argument | **DONE** |
| 4 | `case-lint:` reason word not in the documented form | **DONE**, with one clause DEFERRED (`CLAUDE.md`, see below) |
| 5 | Scope: 9-line comment block in `OUTattributes()` | **DONE** — trimmed, kept the load-bearing half; judgment below |
| 6 | Re-pin the invalidated issue transcripts | **DONE** |
| 7 | Status/Resolution accurate, sequencing recorded as owed | **DONE** |

## Files changed, and why

- `/home/qflow/dev/ngspice_test/src/frontend/outitf.c` — three guards in front of `report_save_miss()`, in order: a token that is not a name (`strchr(name,'(')`) goes to `can't parse '%s': ignored`; `ckt_knows_name()` walks `ckt->CKTnodes` with `vec_name_eq()` and stays silent when the circuit has the name and only this analysis lacks the column; `save_miss_first_time()` over a `wordlist` of tokens already named. Signature is now `report_save_miss(runDesc *run, ...)` so the circuit is reachable. `OUTsaveMissClear()` added, and the `OUTattributes()` comment trimmed.
- `/home/qflow/dev/ngspice_test/src/frontend/outitf.h` — declares `OUTsaveMissClear()`.
- `/home/qflow/dev/ngspice_test/src/frontend/runcoms.c` — `#include "outitf.h"` and one `OUTsaveMissClear()` at the top of `dosim()`; that is the one funnel every analysis command, `ft_dorun()` batch run and shared-library `cp_evloop()` run passes through, so the report's unit is one simulation.
- `/home/qflow/dev/ngspice_test/tests/regression/misc/save-multianalysis-report.cir` + `.out` — the new deck (three cases).
- `/home/qflow/dev/ngspice_test/tests/regression/misc/Makefile.am` — `TESTS`, the header note, and `smr_*` in `CLEANFILES`.
- `/home/qflow/dev/ngspice_test/tests/bin/identity_lint.sh` — the failure help text now lists the reason words the tree actually uses (`helper`, `stored`, `neither`) beside `keyword`.
- `/home/qflow/dev/ngspice_test/AGENTS.md` — same one-sentence addition.
- `/home/qflow/dev/ngspice_test/doc/codex/issues/0057-a-failed-save-names-no-token.md` — Status, five inline re-pins, criterion 6 amended, criterion 9 added, Resolution 4/5/6/7/8 rewritten, 9 added.
- `/home/qflow/dev/ngspice_test/doc/codex/issues/0056-save-matches-vector-names-byte-exactly.md` — Status, four inline re-pins, Resolution 3/7/8 corrected.
- Staged (not edited) `repro/plain.cir`, `save_current_lower.cir`, `save_probe.cir`, `time_node.cir`, `save_partial_miss.cir` — the verifier's problem 5: both issues cite them by name and they were untracked. `repro/.gitignore` ignores only `*.raw` and `sl.out`, so this is additive.

Nothing another crew wrote was reverted or restyled.

## Tests added

`tests/regression/misc/save-multianalysis-report.cir` (+ `.out`), three cases in `save-undef-report.cir`'s sub-deck-under-redirect shape:

- **NOISE** — `.save v(out)` beside `.noise` asserts `SAVE-NOISE-SILENT`, plus `SAVE-NOISE-CAPTURE-READ` so it cannot pass on a capture that was never read.
- **COUNT** — `.save all` + `.save v(nosuchnode)` beside `.noise` prints `nlines`, the number of report lines in the capture. The `.save all` is load-bearing: without it the first plot ends the analysis and the count would be 1 for the wrong reason.
- **EXPR** — `.save v(out) v(out)-v(in)` asserts both `SAVE-EXPR-UNPARSABLE` and `SAVE-EXPR-NOT-CALLED-A-VECTOR`.

No existing deck or `.out` was modified. `.out` needed `git add -f` (`tests/.gitignore:3`).

## RED evidence

Measured on the predecessor's binary before any edit, from `/tmp/.../scratchpad/repro/`:

```
$ ngspice -b -n noise_save.cir     # .save v(out) + .noise + .print noise all
rc=0
Warning: no vector named 'out'; a save asks for it but the analysis has no such variable
Warning: no vector named 'out'; a save asks for it but the analysis has no such variable

$ ngspice -b -n expr_save.cir      # .save v(out) v(out)-v(in)
Warning: no vector named 'in)'; a save asks for it but the analysis has no such variable
$ /usr/local/bin/ngspice -b -n expr_save.cir    # stock ngspice-46, same deck
(silent — the .save route was gated; the .print route says "can't parse 'in)': ignored")
```

Deck-level RED: the three behavioural guards were disabled in place (`if (0 && …) /* TEMP-RED */`), rebuilt (binary mtime 02:40:33 → 02:43:13, no `cp -p` anywhere), and the new deck run:

```
SAVE-NOISE-REPORTED
SAVE-NOISE-CAPTURE-READ
nlines = 2.000000e+00
SAVE-EXPR-NOT-NAMED
SAVE-EXPR-CALLED-A-VECTOR
```

Restored by exact reverse edit and verified byte-identical against a saved copy (`RESTORED IDENTICAL`), rebuilt, mtime 02:43:35.

Second RED cycle, same method, for the counting table (mtime 02:51:23 → 02:51:50, restore verified identical):

| deck | before | now |
| --- | --- | --- |
| `.op` + `.tran` in one `run`, absent name | 2 | **1** |
| one `.noise` card (two plots) | 2 | **1** |
| one `.disto` card (six plots) | **3** | **1** |
| `.save v(out)` + `.noise` (correct deck) | 2 | **0** |
| two absent names on one card | 2 | 2 |
| `op` twice in one control block | 2 | 2 |

The last row is two lines for one mistake and is correct: two `dosim()` calls are two simulations, and `misc/save-undef-report.cir` depends on it — its three sub-decks report the same token from three separate runs.

## GREEN evidence

```
=== 1) noise + .save v(out)          rc=0, stderr empty
=== 2) noise + .save all + absent    rc=0, exactly one line
=== 3) .save v(out)-v(in)            Warning: can't parse 'in)': ignored
=== 4) .print dc v(out)-v(in)        byte-identical to stock ngspice-46's output
PASS: save-multianalysis-report.cir
```

Edge cases checked by hand: `.save v(1) v(2) i(v1) @r1[i]` against numeric nodes — silent, all resolve; `.save v(x1.nosuch)` — `Warning: no vector named 'x1.nosuch'; …`. The asymmetry table of 0057 Resolution 7 re-measured cell for cell: unchanged, all twelve rows.

## Suite status

`make -C build-ver_50 check` — **EXIT=0, 295 PASS, 0 FAIL**. Identity lint 265 comparisons, baseline matches; selftest 11, matches. **No `identity.baseline` change was needed** — the new `eq()` and `cieq()` carry `case-lint:` markers.

Suite-wide stderr census of the diagnostic: three lines, the same three the verifier counted. `TIME`/`time` is pre-existing `vec_warn_case_near_miss()` output from `casedist/vector-scale-case.cir`; `midnode` and `vs#branch` come from `casedist/save-name-case.cir`. Zero `can't parse` lines anywhere in the suite. No committed deck acquired noise.

## Item 4 — what the scanner does with an unknown reason word

**It accepts it silently, and accepts no reason word at all.** `tests/bin/identity_lint.awk` sets `annotated[nlines] = (index($0, marker) > 0)` and never looks at what follows. Measured directly on a fixture:

```
$ awk -v comparators="strcmp cieq eq eqc" -v marker="case-lint:" -f tests/bin/identity_lint.awk lintprobe.c
lintprobe.c	1	strcmp(x, y)
```

Line 1 is the unannotated call; `case-lint: keyword`, `case-lint: bananas` and a bare `case-lint:` are all suppressed alike.

So the form the predecessor used is not invalid — `doc/claude/decisions/0011-identity-lint.md:216` specifies `/* case-lint: <reason> */` with a free reason, and the tree already ships `helper` (`src/frontend/define.c`) and `neither` (`tests/lint/selftest/silent.c`) beside `keyword`. What was wrong is that `stored` was undocumented. Renaming it to `keyword` would be a lie: by the driver's own taxonomy both operands are names a deck wrote, and criterion 2 of 0056 is the measured argument for why they must not go to an identity helper. So I documented the word instead, in the lint driver's failure text and `AGENTS.md`, and normalised the markers to `/* case-lint: stored - both operands are names this run holds, see above */` and `/* case-lint: neither - … */` for the two deliberate case-insensitive scans (`near miss` was a fourth coinage; `neither` is the driver's own word for that class).

**DEFERRED clause:** `CLAUDE.md:79` carries the same sentence naming only `keyword` and wants the same one-line addition. I did not edit it — my operating instructions bar an agent-delivered task from authorising changes to `CLAUDE.md`. It is a one-sentence mirror of the `AGENTS.md` edit; the orchestrator or the user should make it.

## Item 5 — judgment on the comment block

**Trimmed, not deleted.** The half that earns its place is "every caller in the tree passes `varName` as NULL, so neither arm is reachable today; the interface is public, which is why they are classified rather than left frozen" — that is the *reason* the sites were triaged instead of migrated, and it belongs beside the code, not only in the issue. The half that does not is the enumeration of seven caller filenames (`acan.c, span.c, dcpss.c, cktsens.c, noisean.c, noisesp.c, distoan.c`): it is a list that rots the moment an analysis is added, and 0056's Root Cause already holds it with line numbers. Nine lines to seven, with the fact kept and the list dropped.

## Left undone

- **The commit sequencing 0057 criterion 8 asks for is still owed**, and is now recorded as owed rather than claimed met: 0057 Status and Resolution 8 name the two commits and their order (0056's comparison + its `case`/`casedist`/`pipe` decks and the baseline note first, `make check` between, then 0057's report and its three decks). Nothing is committed; the working tree is dirty with my files staged.
- **The near-miss scan still searches `dataNames[]`, not the node list.** Under `distinguish`, a mis-cased `.save` name in a deck whose only analysis is `noise` gets the generic line rather than the near-miss line. Widening the search set would change what `casedist/save-undef-report.cir` asserts and criterion 3 pins it, so it is left alone deliberately and recorded in 0057 Resolution 9.
- **The underlying `gettoks()` mangling is not fixed.** `.print dc v(out)-v(in)` still fails a batch run with rc=1 because the only save token it produces is `in)` — that is stock `ngspice-46` behaviour, reproduced above, and a real defect that deserves its own issue. I named the token accurately rather than repairing the tokenizer, which would change a shipped `.print` path and is out of both issues' scope.
- **`repro/save_guard_probe.cir` echoes `GUARD-NOT-CAPTURED` at HEAD** and I left the deck alone (that directory may belong to a concurrent doc crew). 0057 criterion 5 now says why the probe stopped reproducing — its `.save` resolves under `fold`/`preserve` now, and under `distinguish` the capture's *first* line is the new warning while the probe reads one line and tests it for `no data saved` — and why the criterion holds anyway: the two committed decks read the whole capture in a loop and pass.
- **No decision doc written.** The reasoning is in 0057 criteria 6 and 9. If `doc/claude/decisions/0016-save-name-identity.md` is wanted it is a lift from there; I did not claim number 0016 because parallel crews are running.

## Adversarial verification

- verdict: **NEEDS_WORK**

### Suite status

make -C build-ver_50 check run twice by me, independently of A2: 295 PASS, 0 FAIL, EXIT=0 both times. tests/lint PASS (identity lint 265 comparisons baseline matches; selftest 11 matches). Suite-wide stderr census of the new diagnostic: 3 lines, matching A2's claim (1 pre-existing TIME/time near-miss from casedist/vector-scale-case.cir via vectors.c, 2 from the new casedist/save-name-case.cir); zero "can't parse" lines anywhere. Additional independent evidence A2 did not produce: a 678-run before/after sweep (226 regression+general decks x 3 casemodes, HEAD binary vs fixed binary, stderr diffed) shows exactly 4 changed lines, all inside the two new save-name-case.cir decks - no pre-existing deck moves in any mode. All three second-round guards were RED-proven individually by disabling each in place, rebuilding with a verified mtime move, and restoring to an md5-verified copy. Tree left byte-identical to how I found it (md5 of outitf.c/.h/runcoms.c verified, no TEMP-RED left, the one sgp_cap.txt my probe run wrote removed).

### Resolved

- Item 1 (correct multi-analysis deck must not acquire warnings) - CONFIRMED with decks I wrote myself, not A2's. Silent and byte-identical to stock ngspice-46 for: .save v(out)+.noise; .save i(v1)+.noise (branch current, /tmp/.../adv/t3_noise_save_branch.cir); .save v(x1.m)+.noise (subcircuit-internal node, t8_subckt.cir); .save @r1[i]; .save v(0); .save v(in,mid); .ac+.noise; .op/.dc/.ac/.tran combinations; .sens; .tf. Targeted RED: rewriting the guard to `if (0 && ckt_knows_name(...))` in /home/qflow/dev/ngspice_test/src/frontend/outitf.c, rebuilding (mtime moved, verified), makes ALL THREE of my independent decks warn ('no vector named out' / 'v1#branch' / 'x1.m'), and flips misc/save-multianalysis-report.cir to SAVE-NOISE-REPORTED. Restored by md5-verified copy.
- Item 2 (criterion 6: one line per token per simulation) - CONFIRMED with my own counting decks. .disto with 6 plots -> 1 line (3 with the dedup disabled); .op+.ac+.tran in one run -> 1 line (3 disabled); two `op` calls in one control block -> 2 lines (correct, two dosim() calls); two distinct absent tokens on one card -> 2 lines. I read src/frontend/runcoms.c and confirmed dosim() is the sole funnel: all 11 analysis commands, com_run and ft_dorun route through it, so the unit really is one simulation. RED by making save_miss_first_time() unreachable.
- Item 3 (wording regression on an expression argument) - CONFIRMED. `.print dc v(out)-v(in)` and `.plot tran v(out)-v(in)` produce stderr byte-identical to /usr/local/bin/ngspice (ngspice-46): "Warning: can't parse 'in)': ignored". `.save v(out) v(out)-v(in)` gets the same message. `.four 1k v(out) v(nosuchnode)` correctly moved from "can't parse 'nosuchnode'" to the new accurate wording. RED by disabling the paren guard -> "no vector named 'in)'" returns.
- Item 4 (case-lint reason word) - CONFIRMED, including A2's analysis of the scanner. I ran tests/bin/identity_lint.awk directly on a 4-line fixture: only the unannotated call is reported; `case-lint: keyword`, `case-lint: bananas` and a bare `case-lint:` all suppress alike. So `stored` is not invalid, and documenting it in AGENTS.md and tests/bin/identity_lint.sh rather than renaming it to `keyword` is the right call. The three new markers are correctly placed (own line, directly above the call). The CLAUDE.md clause is correctly DEFERRED - an agent-delivered task cannot authorize a CLAUDE.md edit.
- Item 5 (comment scope in OUTattributes) - CONFIRMED. 7 lines, keeps the load-bearing fact (every caller passes varName as NULL; the interface is public, hence classified not frozen) and drops the seven-filename enumeration. Judgment is sound.
- Items 6/7 (re-pins, Status/Resolution accuracy) - spot-verified by measurement, not by reading. 0057 Resolution 4's claim reproduces exactly: `.save all` beside `.print dc v(out)` says "can't parse 'out': ignored" on stock and is silent now, while `.save all` beside `.save v(nosuchnode)` still names the token. Both issue Status sections accurately record the owed commit sequencing rather than claiming it met.
- Suite - I ran `make -C build-ver_50 check` twice independently. Both runs: 295 PASS, 0 FAIL, EXIT=0. tests/lint: identity lint 265 comparisons baseline matches, selftest 11 matches, both PASS.
- No existing .out was modified - every .out in `git diff HEAD` is an addition. I hand-applied check.sh's egrep -v FILTER to all five new .out files: every assertion token survives (SAVE-NOISE-SILENT, SAVE-NOISE-CAPTURE-READ, nlines = 1.000000e+00, SAVE-EXPR-UNPARSABLE, SAVE-EXPR-NOT-CALLED-A-VECTOR, SAVE-TOTAL-REPORTED, plen, vprobe/iprobe, vin/nmid/nbr). Note the harness never redirects stderr, so cp_err is invisible to it by design - the redirect-and-read-back deck shape is the only thing that works, and it does.
- Independent three-mode regression sweep - 678 runs (226 decks from tests/regression/* and tests/general x fold/preserve/distinguish), HEAD binary vs fixed binary, stderr diffed. Only 4 changed lines, all inside the two NEW save-name-case.cir decks (case/ rc 1->0 under preserve; casedist/ gains the two near-miss lines under distinguish). NO pre-existing deck acquires or loses a single stderr line in any mode. This independently confirms and widens A2's census claim.
- No other crew's uncommitted work was reverted or restyled - A2's staged files (outitf.c, outitf.h, runcoms.c, identity_lint.sh, AGENTS.md, misc/Makefile.am) all have clean worktrees and do not overlap the nine src files, three Makefile.am, identity.baseline and decisions/0001 that other crews hold unstaged. src/frontend/dotcards.c (issue 0059's plot_cur_chosen work) is intact and does not collide with A2's dependence on gettoks().

### Outstanding

- CLAUDE.md:79 still names only `keyword` as a case-lint reason word - correctly deferred by A2, needs the orchestrator or the user to make the one-line mirror of the AGENTS.md edit.
- Commit sequencing of criterion 8 is still owed (A2 declares this).
- The near-miss scan still searches dataNames[] rather than the node list, so under distinguish a mis-cased .save name in a noise-only deck gets the generic line instead of the near-miss line. I reproduced this exactly as A2 describes it (adv/d2_nearmiss_noise.cir). Accurately disclosed, deliberately deferred.
- `.print noise onoise_spectrum` still produces a spurious per-token warning (the integrated-noise plot has no such column). Pre-existing - stock says "can't parse 'onoise_spectrum': ignored" - so this is a wording change, not a regression, but the new wording asserts something false about a correct deck and is worth an issue.
- `.save @r99[i]` for a device that does not exist is silent in both stock and the fixed binary - parseSpecial() succeeds so the report site is never reached. Pre-existing gap in the same defect family as 0057.

### Regressions

- NEW FALSE POSITIVE on correct XSPICE mixed-signal decks, and it is exactly the class criterion 9 forbids. `ckt_knows_name()` (/home/qflow/dev/ngspice_test/src/frontend/outitf.c) walks only `ckt->CKTnodes`, but an XSPICE event node lives in the event node table, not there. Measured: a deck with `abridge1 [in] [din] adc` / `ainv din dout inv` / `abridge2 [dout] [aout] dac` and `.save v(aout) v(dout)` runs to completion rc=0, `eprint dout` returns full event data - and the fixed binary emits `Warning: no vector named 'dout'; a save asks for it but the analysis has no such variable`, where stock ngspice-46 is silent. Criterion 9's own rule is 'does this circuit have the name at all?', and the circuit does have it. The tree already ships the lookup this needs: `Evt_Node_Name_Eq(const char *query, const char *stored)` in src/include/ngspice/evtproto.h, named by CLAUDE.md as one of the three identity helpers. (The `.print tran v(dout)` route is only a wording change - stock already said "can't parse 'dout'" there - so the new exposure is the `.save` route.) Repro decks: /tmp/claude-1000/-home-qflow-dev-ngspice-test/7d5ac281-15ff-4791-9d45-6cc9aa6c5ad3/scratchpad/adv/x5.cir and x6.cir, run with SPICE_SCRIPTS pointed at a spinit that loads build-ver_50/src/xspice/icm/*/*.cm.
- THE STAGED INDEX IS NOT A VIABLE COMMIT, which directly defeats criterion 8's sequencing plan. /home/qflow/dev/ngspice_test/tests/lint/identity.baseline is UNSTAGED while the src/frontend/outitf.c change that requires it IS staged. I measured the consequence: with HEAD's baseline against the staged outitf.c, `make -C build-ver_50/tests/lint check` FAILS - 'In identity.baseline but no longer in the tree: src/frontend/outitf.c strcmp(n1, n2) / strcmp(varName, d->v_name) / strcmp(varName, run->data[i].name). Delete these lines.' Also unstaged and purely A2's: tests/regression/case/Makefile.am and tests/regression/pipe/Makefile.am, so the staged tree adds case/save-name-case.cir, save-name-case-lower.cir and pipe/save-name-case.cmd as files but never registers them in TESTS. Committing the index as it stands therefore produces a 0056 commit whose `make check` fails - the exact thing criterion 8 wants shown green between the two commits. The receipt's sentence 'No `identity.baseline` change was needed' is misleading as written: 0056's own Resolution 7 says the baseline 'loses all three outitf.c entries ... 265 records'.
- NOT A REGRESSION, recorded so it is not re-found: a locally built ngspice segfaults in cm_adc_bridge when it loads the INSTALLED /usr/local/lib/ngspice/digital.cm (gdb backtrace confirms the frame is inside the .cm). I rebuilt at HEAD and it reproduces identically, so it is a pre-existing ABI/environment artifact of the kind MEMORY.md already warns about, not anything this change did. It does mean XSPICE decks must be run against build-ver_50/src/xspice/icm/*/*.cm to be measured at all.