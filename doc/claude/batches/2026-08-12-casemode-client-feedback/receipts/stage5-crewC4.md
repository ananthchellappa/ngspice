# Stage 5 receipt — crew C4

## Implementation

**RECEIPT — crew C4, final fix round**

## RESULT per item

| # | item | result |
|---|---|---|
| 1 | guard keys on syntax, not subject | **DONE — fixed, with a deck that spells the argument.** Both refusals moved off `!wl` onto a predicate over the request. New test `write-spelled-all.cir`, 7 cases, mode independent. |
| 2 | phantom `v(all)` column | **DONE — both halves.** Explained at the point of use in `run_all.sh` section 2 *and* filed as `doc/codex/issues/0064` with a measured root cause (`ft_evaluate()` renames a single-vector result to the wildcard's own text). |
| 3 | section 4's caption vs its numbers | **DONE, and wider than asked.** Crew A4's narrowing had landed, so three more captions in that section were false, not one. Header, banner, absent-name subsection, the `.print`/`.save` table caption and `FINDINGS.md` finding 4 all rewritten against output measured after A4's change. |
| 4 | `com_removecirc()`'s dangling `plot_cur` | **DONE.** Filed as `doc/codex/issues/0063` in the repo's defect tracker, with its own acceptance criteria; `0017` decision 2 and "does not decide" item 2 now point there. |

## Files changed

| file | why |
|---|---|
| `/home/qflow/dev/ngspice_test/src/frontend/postcoms.c` | the one behavioural change: `whole_current_plot = !wl \|\| (!wl->wl_next && eqc(wl->wl_word, "all"))`, computed once, read by both refusals; the two pre-existing guard comments amended where they said "with no vector list" |
| `/home/qflow/dev/ngspice_test/tests/regression/misc/write-spelled-all.cir` `.out` | new — the three refusals reached by spelling the default, the `ALL` spelling, three over-refusal controls |
| `/home/qflow/dev/ngspice_test/tests/regression/misc/Makefile.am` | `TESTS` + `CLEANFILES` (`wsa_*`), then `./autogen.sh` |
| `/home/qflow/dev/ngspice_test/doc/codex/issues/0063-removecirc-leaves-plot-cur-outside-plot-list.md` | new — item 4, filed where a reader looks for open defects |
| `/home/qflow/dev/ngspice_test/doc/codex/issues/0064-a-wildcard-matching-one-vector-is-renamed-to-the-wildcard.md` | new — item 2, the `v(all)` column, root-caused to `evaluate.c:80-84` |
| `/home/qflow/dev/ngspice_test/doc/claude/decisions/0017-write-refuses-an-unchosen-plot.md` | decision 5 (syntax → subject, incl. why `eqc` and why one token wide); decision 2 and "does not decide" 2 now cite `0063`; item 4 reworded for decisions 4+5 |
| `/home/qflow/dev/ngspice_test/doc/codex/issues/0059-...md` | Status, matrix **row 9**, Resolution paragraph for row 9, `v(all)` parenthetical now points at `0064` |
| `/home/qflow/dev/ngspice_test/NEWS` | two clauses on the existing `Bug fixes:` entry giving the boundary (`write f.raw all` refused, `write f.raw all pi` not) |
| `/home/qflow/dev/ngspice_test/doc/claude/feedback/ngspice_upstream/repro/run_all.sh` | head banner (finding 4 → HALF), section 2 `v(all)` explanation, section 4 header + absent-name captions + the `.print`/`.save` table caption |
| `/home/qflow/dev/ngspice_test/doc/claude/feedback/ngspice_upstream/FINDINGS.md` | finding 4's note rewritten as fixed/withdrawn halves; head sentence and asks-table paragraph corrected to match |

## Tests added

`tests/regression/misc/write-spelled-all.cir` — FRESH-ALL / EMPTY-ALL / UPPER-ALL refuse with the *right* message read back from a `>&` capture; NAMED, GOOD-ALL, SETPLOT-ALL still write. Mode independent (the resolver folds `all` in every mode), matches its reference under the harness.

## RED evidence

Deck written first, run against the delivered binary (guards still `!wl`):

```
WRITE-ALL-FRESH-PRESENT / -REFUSAL-MISSING
WRITE-ALL-EMPTY-PRESENT / -REFUSAL-MISSING
WRITE-ALL-UPPER-PRESENT / -REFUSAL-MISSING
WRITE-ALL-DESTROYED-PRESENT / -REFUSAL-MISSING
wsa_fresh.raw wsa_empty.raw wsa_upper.raw wsa_destroyed.raw = 570 B each,
"Title: Constant values", "Plotname: constants", Date: == the Build: stamp
```

Under the real harness, with the predicate reverted to `!wl` (rebuilt, binary mtime moved 08:55:37 → 08:58:48): six assertions flip, `FAIL: write-spelled-all.cir`, 1 of 37. Second experiment, `eq` in place of `eqc` (08:59:26): **only** `UPPER-ALL` flips — so the case-folding half of the predicate is pinned separately, not incidentally. Both experiments restored by file copy (md5 verified) + rebuild.

## GREEN evidence

All seven tokens as the reference: three `-ABSENT`/`-REFUSAL-CAPTURED`, capture holds `Error: no plot has been selected, "wsa_fresh.raw" not written.`; `wsa_named.raw` 264 B, `wsa_good.raw` 328 B, `wsa_setplot.raw` 570 B. `make -C build-ver_50/tests/regression/misc check` → **37/37**.

Item 3 measured, not assumed: `run_all.sh` re-run twice against the post-A4 tree (`outitf.c` mtime 08:44:16, binary 09:14:26) — rc=0, 299 lines, stable across runs apart from timestamps, and every caption in sections 1-9 now agrees with the output printed under it.

Item 2's root cause measured, not inferred: `.save v(MidNode)` + `set filetype=ascii` + bare `write` gives `No. Variables: 2`, `v(midnode)` and `v(all)`, the same value twice; `print all` prints `all = 2.25`; `write f.raw allv` gives `v(allv)`; a two-vector plot is exempt; byte-identical on `/usr/local/bin/ngspice` (`ngspice-46`).

## Suite status

`make -C build-ver_50 check` from a tree with every cached `*.log` (except `config.log`) and `*.trs` deleted, build confirmed current first → **rc=0, 303 PASS, 0 FAIL, 0 XFAIL, 0 XPASS, 0 SKIP** (302 + the new deck). `identity lint: 265 comparisons, baseline matches` plus the 11-comparison selftest; `tests/lint/identity.baseline` untouched by me (mtime 00:50) — my `eqc(wl->wl_word, "all")` has a literal operand and the lint never reports those. The single `^ERROR:` line is `casedist/vector-probe-report.cir`'s own stderr, immediately followed by its `PASS`, and predates this batch. Nothing staged (`git diff --cached` empty); no other crew's hunks touched.

Third-party check on the one shipped deck that actually exercises the changed path: `examples/xspice/pll/pll-xspice-fstep.cir` (`write pll.raw all` after tran + two `resume`s) runs to completion against this binary with the build tree's own code models — rc=0, `pll.raw` 9,927,787 B, zero refusal lines.

## RESIDUALS

1. **`write f.raw all pi` still resolves `all` through `vec_get()`'s constants fallback.** Deliberate: `!wl->wl_next` keeps a list that names a vector outside the refusal, because refusing it is over-refusal in the direction decision 4's controls exist to prevent. Written down in `doc/claude/decisions/0017` decision 5 and its "does not decide" item 4, in `0059`'s row 9 paragraph, and in the deck's own header ("Not covered, and deliberately"). Cost: a deck that types both a wildcard and a name from an unchosen plot can still get a constants file. A fix means changing `vec_get()`'s fallback itself, which is item 4's subject and much wider than `0059`.
2. **`doc/codex/issues/0064` filed, not fixed.** The wildcard rename lives in `ft_evaluate()`, on the path of every control-language expression; the change (withhold the rename for `get_all_type()`'s token set) needs the whole suite behind it and a deck of its own, and it is not a case-mode defect. Acceptance criteria and the proposed shape are in the issue; the client evidence now explains the column where it appears.
3. **`doc/codex/issues/0063` filed, not fixed**, as `0017` decision 2 already decided and the verifier confirmed. Criterion 4 there is the trap for whoever takes it: `destroy-removed-circuit.cir` asserts the internal error *is* printed and needs another discriminator first.
4. **Finding 4 is now recorded as half fixed** in `FINDINGS.md` and `run_all.sh`. That is a description of crew A4's withdrawal (`0057` Status), not a decision of mine; if A4 lands further changes to `outitf.c` after `08:44:16`, section 4's numbers and captions need re-measuring — everything I wrote there was measured at 09:05-09:18 against that source state.
5. **`LEDGER.md` not updated** — the orchestrator's file. Its crew C3 residual list is closed by this work except for the four residuals above.

## Verification

- verdict: **NEEDS_WORK**

### Suite

GREEN, and the receipt's numbers reproduce exactly. Ran `make -C build-ver_50 check` myself from a tree with every cached *.log (except config.log) and every *.trs deleted (verified 0 of each remaining beforehand), build confirmed current first: exit 0, 303 PASS, 0 FAIL, 0 XFAIL, 0 XPASS, 0 SKIP. tests/lint: `identity lint: 265 comparisons, baseline matches` (PASS: identity.lint) plus `identity lint: 11 comparisons, baseline matches` (PASS: selftest.lint); tests/lint/identity.baseline mtime 00:50, i.e. untouched by C4 -- consistent with its claim that `eqc(wl->wl_word, "all")` has a literal operand and is never reported. The single `^ERROR:` line in the whole log is `ERROR: (internal)  tried to destroy non-existent graph`, immediately followed by `PASS: vector-probe-report.cir`, exactly as the receipt describes. Targeted: `make -C build-ver_50/tests/regression/misc check` = 37/37 with the delivered predicate, and 36/37 under each of my four mutations, restoring to 37/37 afterwards.

### False positives remaining

- A plain nutmeg data-processing deck that builds vectors with 'let' and writes them. Deck (batch form): '.control / set filetype=ascii / let x = vector(5) / let y = x*2 / write letonly.raw / .endc / .end'. Stock /usr/local/bin/ngspice writes a 3275-byte ASCII rawfile whose Variables list holds the deck's own x and y at indices 12 and 13; the delivered binary prints 'Error: no plot has been selected, "letonly.raw" not written.' + '       Run an analysis, or name the plot or the vectors to write.' and writes nothing. A HEAD baseline with this batch's diff absent behaves like stock, so the diagnostic is introduced entirely by this batch. Saved at /tmp/claude-1000/-home-qflow-dev-ngspice-test/7d5ac281-15ff-4791-9d45-6cc9aa6c5ad3/scratchpad/atk/letonly.cir
- The same deck through the interactive pipe route, which is the workflow -p exists for: 'ngspice -p' fed 'set filetype=ascii / let x = vector(5) / let y = x*2 / write pipe_test.raw / quit'. Stock: 3275-byte file. Delivered: same refusal, no file. Saved at /tmp/claude-1000/-home-qflow-dev-ngspice-test/7d5ac281-15ff-4791-9d45-6cc9aa6c5ad3/scratchpad/atk/p.cmd
- The spelled form of the same thing, which is C4's own widening rather than an inherited one: 'ngspice -p' fed 'set filetype=ascii / let x = vector(5) / let y = x*2 / write pt2.raw all / write pt3.raw x y / quit'. Stock and HEAD baseline write pt2.raw (3275 B) and pt3.raw; the delivered binary prints 'Error: no plot has been selected, "pt2.raw" not written.' for the 'all' form while the named form still writes. Reverting only the predicate to !wl makes pt2.raw write again, so this line is attributable to decision 5 alone. Saved at /tmp/claude-1000/-home-qflow-dev-ngspice-test/7d5ac281-15ff-4791-9d45-6cc9aa6c5ad3/scratchpad/atk/p2.cmd
- NOTE ON SCOPE: no deck shipped in examples/ or tests/ acquires a diagnostic the baseline does not print -- 726 decks compared, zero differences. Everything my stock-vs-delivered sweep flagged on a correct deck was reproduced on the HEAD baseline too and is therefore committed ver_50 history, not this batch: examples/control_structs/new-check-4.sp ('Error: RHS "const5 > unwanted_output_file_1" invalid' + a checkvalid warning, from the committed 'let' redirect-parsing change), examples/measure/simple-meas-tran.sp (the 'meas tran tmax ... failed!' line moved stdout->stderr), tests/general/schmitt.cir ('Note: Starting/Dynamic gmin stepping completed'), and tests/mesa/mesa-12.cir (90 s timeout where stock exits 1). None of these are C4's and none belong to this batch.

### Outstanding

- ITEM 1 IS NOT DONE AS CLAIMED -- the guard still keys on one exact token spelling, not on the subject, and FOUR requests for the whole current plot walk past all three refusals and write the identical 570-byte 'Plotname: constants' rawfile that doc/codex/issues/0059 exists to keep out of a reader's hands. Measured on the delivered binary in all three refusal cases (nothing run, after 'setplot new', after 'destroy'): (a) 'write f.raw ally' -> 570 B, 12 variables, Plotname: constants; (b) 'write f.raw ALLY' -> same; (c) 'write f.raw "all"' with DOUBLE quotes -> same (note 'all' single-quoted IS stripped and refused, so the guard is inconsistent between the two quoting forms); (d) 'write f.raw all all' -> 1254 B, 24 variables, the constants written twice. Six files, 570 bytes each, byte-identical in outcome to the pre-fix 'write f.raw all'.
- The stated boundary does not cover any of them. doc/claude/decisions/0017 'does not decide' item 4 excludes only 'write f.raw all pi', on the ground that it 'names something the current plot does not hold, which is a request in its own right'. Neither 'ally' nor '"all"' nor 'all all' names anything beyond the current plot -- each is the same request one letter, one quote-pair, or one repetition along. Decision 5's own justification for folding case ('a byte-exact test here would refuse a strictly smaller set than the one that gets to the constants plot') applies verbatim and was not applied.
- The new code comment at /home/qflow/dev/ngspice_test/src/frontend/postcoms.c:596-614 is falsified by its own subject: it says 'The token is matched the way the resolver matches it.' get_all_type() (/home/qflow/dev/ngspice_test/src/frontend/vectors.c:105-153) recognises all, allv, alli, ally and alle, each case-folded; the guard recognises one of the five. findvec_ally (vectors.c:293-295) is 'every permanent vector except the scale', which on the constants plot is all twelve -- i.e. ally is a whole-plot wildcard, not a filtered one.
- The author was in a position to see this. /home/qflow/dev/ngspice_test/doc/codex/issues/0064, filed by this same crew in this same batch, opens with 'When all, allv, alli or ally matches exactly one vector...' -- the sibling tokens are named in C4's own issue text and are absent from decision 0017, from 0059's row 9, from the NEWS clauses and from write-spelled-all.cir.
- This is the third consecutive generation of the same gap in the same guard. 0017's own Status records decision 4 being added after verification found a state no row named, then decision 5 after verification found the guard keyed on syntax rather than subject. The subject is STILL not captured: the predicate asks 'is the word literally "all"' where the question is 'does this request resolve to the whole current plot'. The natural fix is to key on get_all_type(wl->wl_word) != ALL_TYPE_NONE over a cp_unquote'd word, with a deck extending write-spelled-all.cir.
- 0059's matrix row 9 and the NEWS boundary clauses both need widening once the predicate is: they currently freeze 'all' as the whole of the spelled-default case.
- Not re-litigated, and correctly left open by the receipt: residual 1 (write f.raw all pi), residual 2 (0064 filed not fixed), residual 3 (0063 filed not fixed), residual 5 (LEDGER.md). All four are honestly stated and I found no misstatement in them.

### Regressions

- C4's item-1 change WIDENS a pre-existing over-refusal onto a second spelling. Attribution is exact, measured against three binaries: on a plain nutmeg session that builds its own vectors with 'let' (they land in the constants plot, which is plot_cur at startup and which nothing ever 'chose'), stock /usr/local/bin/ngspice writes, a HEAD baseline built from committed ver_50 with this batch's uncommitted diff absent ALSO writes, and only the delivered tree refuses -- for BOTH 'write out.raw' (earlier round, decisions 1-4) and 'write out.raw all' (C4's decision 5). Reproduces in --batch and in -p. See false_positives_remaining for the deck and the exact text.
- That shape is not among write-spelled-all.cir's three over-refusal controls (NAMED / GOOD-ALL / SETPLOT-ALL), all of which either name vectors, run an analysis, or setplot -- none covers a plot the deck populated itself without any command choosing it. The NEWS entry's 'when no analysis has run' technically covers it but the migration note never says that a deck which builds vectors with 'let' and writes them now needs 'setplot const' first.
- NO regression on any shipped deck. I built a HEAD baseline (git archive HEAD -> autogen.sh -> configure -> make -j8, exit 0) and diffed baseline-vs-delivered stderr and rc over 726 decks from examples/ and tests/, each binary running against ITS OWN code models via SPICE_SCRIPTS: ZERO differences, zero rc changes. Harness positive-controlled -- a deliberately planted let/write deck was detected with both refusal lines.
- No other crew's hunks reverted and nothing staged: 'git diff --cached' is empty and 'git diff --stat' is 26 files / 714 insertions / 65 deletions, byte-identical to the snapshot taken at the start of this session, after all four of my mutations were restored and the tree rebuilt. All five of C4's new or modified artefacts are present. I removed the four in-place run byproducts I created (examples/digital/digital_devices/counter.vcd, examples/control_structs/unwanted_output_file_3, examples/measure/ttt.data, examples/measure/ttt.plt); every other untracked file predates this session.
- HARNESS HAZARD for whoever verifies next, not a code defect: build-ver_50/src/spinit points its codemodel lines at /usr/local/lib/ngspice/*.cm, i.e. the INSTALLED ngspice-46 code models. Running the build-tree binary on an XSPICE deck therefore loads mismatched-ABI models and SIGSEGVs in cm_adc_bridge -- my first sweep produced ~140 spurious 'crashes' across examples/digital/** and examples/xspice/** from exactly this. With SPICE_SCRIPTS pointed at the tree's own icm/*/*.cm, examples/digital/digital_devices/counter.cir runs rc=0. Any before/after on an XSPICE deck must set SPICE_SCRIPTS per side.