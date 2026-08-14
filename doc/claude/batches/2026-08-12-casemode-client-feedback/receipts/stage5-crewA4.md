# Stage 5 receipt — crew A4

## Implementation

All work is complete and verified. Final receipt:

```
CREW A4 — RECEIPT
=================

RESULT per item
| # | Item                                                            | Result |
|---|-----------------------------------------------------------------|--------|
| 1 | Narrow 0057's report to the near miss                           | DONE — report fires only on miss AND case variant; all 6 false positives silent structurally |
| 2 | Verify deck by deck, incl. the two examples/ decks              | DONE — 6 decks measured RED->GREEN by hand, 9-mutation matrix, full suite |
| 3 | Rewrite 0057: Summary, criteria 1/2/3/4/6/7/9, Resolution       | DONE — withdrawal stated plainly, its cost named |
| 4 | name_eq() byte-exactness has no test (0056 criterion 2)         | DONE — tests/regression/case/plot-scale-name-twin.cir; mutation M1 fails it |
| 5 | CLAUDE.md:79 vs AGENTS.md reason words                          | DONE — one-line mirror |

WHAT THE CODE NOW DOES
report_save_case_miss() (src/frontend/outitf.c, beginPlot Pass 2) prints
decision 2's line only when: casemode=distinguish, the circuit does not have
the token exactly (both node lists), and some name in dataNames[] or
ckt->CKTnodes differs from it only in case (cieq AND !eq). Otherwise it is
silent and the pre-existing `can't parse` line runs on its ORIGINAL
saves[i].analysis gate. fold and preserve are byte-identical to before 0057.

FILES CHANGED (mine; other crews' hunks untouched, `git diff --cached` empty)
- /home/qflow/dev/ngspice_test/src/frontend/outitf.c — report_save_miss() ->
  report_save_case_miss(), returns bool; generic line deleted; the paren test
  replaced by the restored analysis gate; case_twin_p() (cieq && !eq) shared by
  both twin scans; save_in_run(), save_resolved_note(), save_miss_resolved
  deleted as suppressions of a report that no longer exists.
- CLAUDE.md — adds helper/stored/neither to the case-lint reason words.
- doc/claude/decisions/0001-distinguish.md — decision 2's table gains the
  `.save` row (incl. the event-node gap); the name_eq() paragraph gains its deck.
- doc/codex/issues/0056-save-matches-vector-names-byte-exactly.md — Status,
  criterion 2, Resolution 2: criterion 2 was true and untested; deck named.
- doc/codex/issues/0057-a-failed-save-names-no-token.md — Status, Summary,
  criteria 1/2/3/4/6/7/9, Resolution rewritten to a near-miss contract.
- NEW  tests/regression/case/plot-scale-name-twin.cir + .out (+ Makefile.am, autogen)
- REWRITTEN tests/regression/casedist/save-undef-report.cir,
  save-near-miss-node-list.cir; tests/regression/misc/save-undef-report.cir,
  save-multianalysis-report.cir, save-crossplot-report.cir;
  tests/xspice/digital/save-event-node.cir;
  tests/xspice/casedist/save-event-node-case-split.cir — all with .out regenerated
  from verified runs (assertions changed deliberately; none regenerated to hide
  a failure).
- DELETED tests/xspice/case/save-event-node-case.cir + .out; that dir's
  Makefile.am is back at HEAD. Under the narrowed contract preserve cannot
  produce this report at all, so no single mutation could fail the deck.

RED (third-round binary, 07:25)
- examples/transient-noise/noi-ring51-demo.cir: 3x "no vector named 't1'"
- examples/probe/F5TurboV2-Probe.cir: 1x "no vector named 'mq4:3'"
- .save onoise_spectrum onoise_total + .noise ...1: 1 line, while the same run
  prints onoise_total = 2.864464e-08; .save all onoise_total: 1 line
- .save dout(state) on an adc/inverter/dac chain: "no vector named 'state'"
- deck assertions that flipped: SAVE-TOTAL-REPORTED, SAVE-PARTIAL-REPORTED,
  SAVE-ALLMISS-REPORTED, SAVE-NOTWIN-REPORTED, ABSENT-NAME-REPORTED,
  SPLIT-SAVE-REPORTED, nlines=1, mlines=1, SAVE-EXPR-UNPARSABLE
- new deck without its fix: SCALE-TWIN-DROPPED under mutation M1

GREEN (final binary, 08:02, mtime moved on every rebuild, no cp -p)
- all four false-positive reproductions: 0 lines, rc unchanged, onoise_total
  still printed; .save v(out)-v(in) now 0 lines on both this binary and
  /usr/local/bin/ngspice-46
- the case defect 0057 was filed for still reports: fold silent, preserve
  silent, distinguish rc=1 "no vector named 'midnode'; 'MidNode' differs only
  in case (casemode=distinguish)"
- 9-mutation matrix (each applied in place, rebuilt, restored md5sum -c clean):
  M1 name_eq->vec_name_eq        -> case/plot-scale-name-twin FAILS
  M2 distinguish mode test off   -> nothing fails (provably redundant; disclosed
                                    in 0057 Resolution 3, not hidden)
  M3 ckt_knows_name off          -> casedist/save-near-miss-node-list (BOTH),
                                    xspice/casedist (TWIN)
  M4 case_twin_p without !eq     -> casedist/save-near-miss-node-list (SELF)
  M5 dataNames scan skipped      -> casedist/save-near-miss-node-list (COLUMN)
  M6 CKTnodes fallback off       -> casedist/save-near-miss-node-list (NOISE)
  M7 dedup off                   -> casedist/save-near-miss-node-list (tlines=2)
  M8 Evt_Ckt_Has_Node off        -> xspice/casedist (TWIN)
  M9 wide report restored        -> ALL SEVEN decks of 0057 fail

SUITE
make -C build-ver_50 check with all cached *.log/*.trs deleted first:
EXIT=0, 302 PASS, 0 FAIL, 0 SKIP, 0 XFAIL. Same test set as the round's first
full run. identity lint 265 comparisons baseline matches, selftest 11.
Suite-wide stderr census: exactly 3 "no vector named" lines, all near-miss
lines under distinguish ('TIME' from the pre-existing vector-scale-case.cir,
'midnode' and 'vs#branch' from 0056's casedist/save-name-case.cir); 0
"can't parse", 0 "a save asks for it". The report can only fire under
casemode=distinguish, which only tests/regression/casedist and
tests/xspice/casedist select and no deck sets internally, so the census over
the other 600+ decks is structural as well as measured.

RESIDUALS (all written into doc/codex/issues/0057, Resolution 9)
1. An unresolved .save name with NO case variant is silent again, in every
   mode — the withdrawal itself. Cost: the Impact section's two failure shapes
   survive for a plain typo, exactly as in stock ngspice-46. A fix needs the
   question asked where it is answerable — after the simulation, not inside
   beginPlot() which sees one plot of one analysis — which is a different
   change with a different risk. Recorded in Status, criterion 1 (struck
   through, with the argument) and Resolution 1.
2. An event node's own case near miss is not reported: .save v(DOUT) against
   the event node dout under distinguish is silent, because the twin scan walks
   the analog lists only. Cost: one real case defect undiagnosed. A fix is an
   Evt_Ckt_Case_Twin() beside Evt_Ckt_Has_Node() (~15 lines) plus a deck.
   Asserted as a silence by xspice/casedist/save-event-node-case-split.cir's
   SPLIT case so closing it is a visible change.
3. The distinguish mode test at the head of report_save_case_miss() is provably
   redundant and no deck can pin it (M2 flips nothing). Kept deliberately; the
   reason is in Resolution 3.
4. The .print route's "can't parse" wording is still wrong and still
   unassertable (ft_savedotargs() runs only from src/main.c's batch path). This
   round stopped making it worse rather than fixing it; needs a harness decision.
5. .save @r99[i] for a nonexistent device: silent in stock and here, different
   code path, deserves its own issue.
6. Criterion 8's commit sequencing still owed — nothing committed, nothing
   staged, by instruction. Resolution 8 names the two commits and moves
   plot-scale-name-twin.cir into 0056's.
7. NEWS deliberately not touched: the surviving diagnostic lives inside
   casemode=distinguish, which NEWS already describes as experimental, and the
   wide report was never announced there.
```

## Verification

- verdict: **SIGN**

### Suite

make -C build-ver_50 check, run by me after deleting every cached *.log/*.trs and after restoring outitf.c to its pristine md5 and rebuilding: EXIT=0, 302 PASS, 0 FAIL, 0 SKIP, 0 XFAIL, 0 XPASS. tests/lint: 'identity lint: 265 comparisons, baseline matches' (identity.lint PASS) and 'identity lint: 11 comparisons, baseline matches' (selftest.lint PASS). Suite stderr census: exactly 3 'no vector named ... differs only in case' lines and 0 'can't parse' lines. I attributed all 3 by rebuilding with a unique marker in outitf.c's format string: 'TIME'/'time' comes from the pre-existing vec_warn_case_near_miss() in src/frontend/vectors.c:479, and 'midnode'/'MidNode' plus 'vs#branch'/'Vs#branch' come from A4's new site, both in casedist/save-name-case.cir. The single '^ERROR:' hit in the log is ngspice's own '(internal) tried to destroy non-existent graph' text inside a deck's output, not an automake test result.

### False positives remaining

- (none)

### Outstanding

- NOT IN THE RECEIPT'S RESIDUAL LIST - the twin scan does not include the run's reference vector, so '.save TIME' under distinguish against a tran whose scale is 'time' is silent. My deck: V1 In 0 ... / .save TIME v(Out) / .tran -> zero lines. Same shape as the reported 'midnode'/'MidNode' miss; harmless in effect (the scale is written to the plot regardless of the save list), but the residual list in 0057 Resolution 9 does not name it, and the adjacent deck A4 added (plot-scale-name-twin.cir) is precisely about the refName/name_eq relationship, so the omission is a natural one to close.
- Residual 2 reproduced and confirmed still open: '.save v(DOUT)' against the XSPICE event node dout under distinguish is silent (my own adc/d_inverter/dac deck). It is asserted as a silence by xspice/casedist/save-event-node-case-split.cir's SPLIT case, so closing it will be a visible change, as claimed.
- Residual 1 reproduced and confirmed as the deliberate cost: an unresolved .save with no case variant is silent in fold, preserve and distinguish, identical to stock ngspice-46. A deck whose only save is a typo still aborts with 'Error: no data saved ...; analysis not run' and no word about which token was at fault.
- Residual 3 confirmed real: M2 (deleting the distinguish mode test) flips no deck in the suite. The guard is genuinely unpinned; the receipt discloses this rather than hiding it, but it remains dead weight that no test protects.
- Criterion 8 still owed, as the receipt says: nothing is committed and nothing is staged, so the claim that 0056 moves make check on its own cannot be shown until the two commits exist in that order.
- Coverage caveat, not A4's doing: tests/Makefile.am puts vbic, general, filters, mos6, polezero, transient and others in DIST_SUBDIRS only, so make check never runs them. The suite-wide stderr census (3 'no vector named', 0 'can't parse') is therefore over the 302 decks that run, not over tests/ as a whole. tests/vbic/noise_scale_test.cir does print "Warning: can't parse 'inoise_spectrum': ignored" - but the stock binary prints the identical line, so it is pre-existing, not a regression.

### Regressions

- (none)