# Stage 1b receipt — issue 0060

## Remediation

RECEIPT — issue 0060

1. FIXED — checker right. Re-measured guide §2's three-way probe as `repro/three_way_probe.cir`: `-D casemode=fold` → `caseprobe = 2.000000e+00` twice; `preserve` → `CaseProbe = 2.000000e+00` + `caseprobe = 2.000000e+00`; `distinguish` → `CaseProbe = 1.000000e+00` + `caseprobe = 2.000000e+00`; no flag → fold shape; `/usr/local/bin/ngspice -D casemode=preserve` → fold shape. Impact's guide paragraph rewritten into two bullets: §2 (`guide:52-62`) is a three-way deck probe that does see `preserve` and separates the binaries; §9 (`guide:194-197`) is the identity probe that cannot. The "no probe of any kind" sentence is gone; replaced by "there is no probe for a session that reads no deck".

2. FIXED — checker right. Consumer paragraph narrowed to "cannot ask a *live session*", now points deck-generating clients at §2's probe and at `guide:224-226` / `guide:234-248`, and names the shapes that genuinely have no answer (libngspice between `ngSpice_Circ()` calls, `-p` co-process, pre-generation capability check).

3. FIXED — checker right. Verified 0048:69-73 and 0033:45-46 both name prompt / `-p` / `ngSpice_Command()` and both exclude `.control`. Sentence rewritten: "Two of these three are also 0048's and 0033's routes, but for a different reason and with a different membership" + the reason contrast (reader folding a mixed-case variable *name* vs. the latch having already fired). Dropped the misleading bolded "and not from a deck's own card".

4. FIXED — checker right. Re-measured in `repro/`: `load hdr_option.raw` then `set casemode=fold` → `Error: casemode is a read-only variable.` / `after preserve` (via `options.c:414-417`, the `pl_env` `US_READONLY` loop). Summary's "every `set casemode=…` is `US_OK`" → "normally `US_OK` … not *always*", with the exception named and deferred to 0061; the transcript added to criterion 6.

5. FIXED — checker right. `grep -rn '\$casemode' tests/` → 4 lines. Census expanded to four bullets. Measured and added: `tests/regression/casedist/harness-alive.cir` gives `v(out) = 7.500000e-01` / `CASEMODE-IS distinguish` from **both** binaries; `tests/xspice/casedist/harness-alive.cir` gives `v(aout) = 5.000000e+00` / `CASEMODE-IS distinguish` from both (repo build needs `SPICE_SCRIPTS=.` in `build-ver_50/tests/xspice/casedist/`; baseline needs nothing).

6. FIXED — checker right. `grep -n 'Not withdrawn'` returns exactly one hit, `0001-distinguish.md:482`, inside Decision 5 (`:452`), and its list names `.TRAN`/`PULSE`/`PARAMS:`/`UIC`/`%VD`/`.model` type names/B-source function names — not the mode values. Text now says decision **5**, cites `:482`, and states the mode-value classification is an inference from that list, not a quotation.

7. FIXED — checker right. `bool inp_case_exact_ids(void)` is `inpcom.c:1060`. Citation changed `:1062` → `:1060`.

8. FIXED — checker right. `inp_case_folding()` (:1041) and `inp_case_exact_ids()` (:1060) read `ng_case_mode` directly; only `vec_name_eq()` (`vectors.c:415`) and `Evt_Node_Name_Eq()` (`evtplot.c:65`) call `inp_case_mode()`. Reworded to make the `static int ng_case_mode` (`:1034`) the authority and `inp_case_mode()` the accessor. Also corrected an error the checker did not raise: all four predicates are exported (`fteext.h:214-221`), so "the only thing exported" was false — now "only `inp_case_mode()` returns the mode itself".

9. FIXED — checker right on all three. (a) Restored `Vs#branch`/`vs#branch` to the `.spiceinit` block and marked both it and the two other `display` excerpts as excerpts. (b) The reverse-direction `-p` block is now the literal streams, captured separately: stdout 7 lines, stderr 5 lines including the experimental banner **twice** and the near-miss **twice**, with "Nothing is elided." (c) Criterion 6 now carries full stdout (echo, `Loading raw data file`, both `Title:`/`Name:`/`Date:` blocks, both vector lines) and the stderr `Error:` line separately. Also fixed an undisclosed edit the checker missed: the default-shape block dropped its first echoed line and mislabeled prompts as 10003/10004 — measured 10002/10003.

10. FIXED — checker right. Measured both binaries; streams are byte-identical except the closing banner (`ngspice-46+ done` vs `ngspice-46 done`). Stated in criterion 6 rather than claimed.

11. FIXED — checker right. Sentence is now "In a session that sources nothing the latch runs during start-up and never again — a `source` typed later is a second `inp_readall()` and does move it, which is criterion 2's subject".

BATCH-WIDE

(a) FIXED. Added `doc/codex/issues/0058` in criterion 2, explicitly ceding the once-per-file-read count and its call-site table to it. `0061` already owned the plot-environment mechanism and 0060 still defers (Summary + criterion 6 + the raw-header paragraph). `0048` retained for the variable-*name* defect; `0046` retained for the doubled near-miss. `0033` kept but its claim corrected per problem 3. 0060 prescribes nothing that another issue in the batch prescribes.

(b) FIXED in 0060; 0058 is correct and needs no change. Measured: `count` of `unknown casemode` under `-D casemode=bogus` — `divider.cir` 2, `include_count.cir` (`.include`) 2, so `.include` adds no `set_case_mode()`; and `relatch_source.cir` under `-D casemode=preserve` yields `In`/`MidNode`/`Vs#branch` from the top deck but `aa`/`bb`/`vt#branch` from the `source`d deck, so `source` does re-latch. Code path confirmed: `inp_read()` recurses at `inpcom.c:1741` (`.include`) and `:599` (`.lib`), below `inp_readall()` (`:1168`); `source` → `com_source()` → `inp_spsource()` → `inp_readall()` (`inp.c:532`). Criterion 2 rewritten to say `.include` and `.lib` are **not** `inp_readall()`s.

(c) FIXED. Six decks added under `doc/claude/feedback/ngspice_upstream/repro/`, each with a title comment as its first line and each named in the issue at its point of use: `report_probe.cir`, `three_way_probe.cir`, `relatch_source.cir`, `relatch_inner.cir`, `include_count.cir`, `include_count.inc`. All staged. The nameless "probe.cir" the issue used to quote is now `report_probe.cir`. `hdr_option.raw` is named as generated by `run_all.sh`'s finding-1 block. A finding-8 block covering `report_probe.cir`, `three_way_probe.cir`, `relatch_source.cir` and `include_count.cir` was appended to `repro/run_all.sh` (finding 8 is one of the nine); `./run_all.sh` re-runs rc=0 after a sibling's concurrent edits and reproduces every number quoted.

(d) FIXED. See 9. Every remaining trimmed block is now labeled ("excerpted from the batch output"), every stream split is stated, and the one quotation ellipsis in the guide quote is marked and explained.

SIBLING CHANGES NEEDED: none. 0058's `.include`/`.lib` measurement was right and 0060 was wrong; the correction was entirely inside 0060. Files touched: `/home/qflow/dev/ngspice_test/doc/codex/issues/0060-casemode-variable-reports-the-request-not-the-effect.md`, `/home/qflow/dev/ngspice_test/doc/claude/feedback/ngspice_upstream/repro/run_all.sh`, and the six new decks in `/home/qflow/dev/ngspice_test/doc/claude/feedback/ngspice_upstream/repro/`. `src/` and `tests/` untouched.

## Re-check

- verdict: **GOOD** · decks_committed: True · cross_refs_ok: True

### Still unresolved

- (none)

### Regressions introduced

- MINOR, non-blocking (issue:66-68): the label on the two-binary report_probe.cir block says the rest of the batch output is "otherwise identical between the two binaries except for the title line and the version banner". Measured (diff of the two full stdouts): there is no version banner in batch mode at all, and two title-carrying lines differ, the `Circuit:` line and the `Title:` line, not one. The claim over-discloses rather than hides, so nothing quoted is wrong.

- MINOR, non-blocking (issue:246-248): "The gap is therefore narrower than 'no probe': there is no probe for a session that reads no deck" reads flatly against the guide-§9 bullet two lines above it, which documents exactly such a probe (pipe mode, no deck) that answers one bit of the question. The intended claim - no probe that determines the *mode* in a no-deck session - is what criterion 4 and guide:220-224 state precisely; this summary sentence drops the qualifier its own bullets just supplied. Same shape as the sentence problem 1 attacked, materially narrowed but still literally overstated.
