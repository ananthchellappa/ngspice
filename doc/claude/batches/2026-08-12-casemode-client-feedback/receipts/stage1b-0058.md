# Stage 1b receipt — issue 0058

## Remediation

Everything re-measured and the issue rewritten. Final receipt:

**1. FIXED** — `make check` banner totals corrected. Re-measured all 40 decks with the harness's own env: `tests/regression/casedist` 62 banners / 19840 bytes, `tests/xspice/casedist` 51 / 16320, total **113 / 36160 bytes**, not "~80 / ~25 kB". Added the distribution (24 decks at 2; nine at 3, each generating an auto-bridge; seven at 4–6, exactly `2 + grep -c '^source '` plus one bridge for the two XSPICE report decks — verified by running the generated sub-decks standalone: `abr_exact.cir` 3, `abr_hand.cir` 2, `abr_miss.cir` 2, `enr_bridged.cir` 3, `enr_miss.cir` 2, `enr_notwin.cir` 2). Minor correction to the checker: it is nine `auto-bridge-vcc/family` decks at 3, not eleven — `auto-bridge-node-case-split` and `hand-bridge-node-case-split` are at 2. Its totals were exact.

**2. FIXED** — `src/main.c:1503` (`if (tempfile && …)`) → `src/main.c:1508` (the `inp_spsource()` call) at all three uses.

**3. FIXED** — the false exhaustiveness claim is gone. The table is now two tables: the four genuine `inp_readall()` call sites (`inp.c:532`, `device.c:1598`, `evtcheck_nodes.c:232`, `nutinp.c:38`), then the `inp_spsource()` callers that reach `inp.c:532` through its `if (fp || intfile)` guard at `inp.c:531` — including the previously omitted `main.c:1524`, `cpitf.c:306` (Windows-only), `inp.c:1678`/`:1718`, `inp.c:2105` (`ngSpice_Circ`) — plus the two that do not reach it (`inp.c:415`, `inp.c:1644`, both falling to the reload `else` at `inp.c:544`).

**4. FIXED** — `switcher.cir` and `mid.cir` committed; `mid.cir` has its own `display`, and the transcript is re-run and quoted with the plot names (`op1`/`op2`). "one plot list" → "Two modes, one process, two plots".

**5. FIXED** — every quoted deck committed: `repro/reads/{plain,inc_deck,inc_tail.inc,lib_deck,lib_tail.lib,am.cir,am.mod,switcher,mid,switcher_d,switcher_b,outer,capture}` and `repro/spiceinit-fold/{.spiceinit,d.cir}`, each with a title comment, each named at its point of use. The three `-p` commands now carry real `grep -c 'unknown casemode'` (2/3/4) and the `<same deck, bridge removed>` placeholder is replaced by a *better* measurement: the committed `tests/xspice/casedist/auto-bridge-node-case-split.cir` run twice, `distinguish` → 2 banners with `v(a) = 0.000000e+00` (no bridge) vs `bogus` → 3 with `v(a) = 3.300000e+00` (bridge). Added `v(midnode) = 2.250000e+00` for the include/lib decks and `is_now = 3.000000e-14` for `am.cir` so the negative results are not vacuous.

**6. FIXED** — the checker is right and the rule is replaced. Measured `repro/reads/switcher_b.cir` (`switcher.cir` with `set casemode=bogus` before the `source`): `Warning: unknown casemode 'bogus', using 'fold'`, count **1** today, and a mode-keyed latch would print 0 because `inpcom.c:1089` establishes `NG_CASE_FOLD` and `:1126` changes nothing. The issue now latches the *outcome* of `set_case_mode()` — mode plus the unrecognised value string when there was one — initialised to (`NG_CASE_FOLD`, none), and derives the result against a seven-row table of the measurements rather than asserting 1/1/1. Acceptance criterion 1 rewritten accordingly.

**7. FIXED** — Status now reads "two of `set_case_mode()`'s four arms — the `distinguish` arm, and the fall-through arm that any unrecognised value takes".

**8. FIXED** — "arrived in two commits rather than one: the unknown-value warning with `set_case_mode()` itself (`424ebaf75`), the `distinguish` banner later (`d06326d05`)". Confirmed by `git log -S` on both strings.

**9. FIXED** — no longer claims decision 2 governs the emission. It says decision 2 is scoped to a near-miss report and does not reach a mode banner, and carries over only the *reason* behind its second 2026-08-11 correction (`0001-distinguish.md:143-160`, verified), which applies regardless.

**10. FIXED (my half)** — cross-references added: `0060` (both directions — it defers to 0058 for the count and call-site table, 0058 defers to it for `$casemode`-reports-the-request), `0061` (finding 1's raw-header ask), `0056`/`0057` (they quote transcripts where the doubling appears; 0058 owns the count), plus the existing `0036` and decisions `0001`/`0006`.

**Batch (b) — no sibling change needed.** Re-measured: `.include` 2, `.lib` 2, plain 2 (all three printing `v(midnode) = 2.250000e+00`, so the cards really land); `source` +1 each (2/3/4 in `-p`, 3 in batch). `0060` has already been corrected on its side — its acceptance criterion 2 now says `.include`/`.lib` are **not** `inp_readall()`s (citing `inpcom.c:1741`, `:599`) and names 0058 as owner. The contradiction is resolved; 0058 now cites those two recursion sites instead of the vaguer `inpcom.c:1184`.

**Batch (c) note — `run_all.sh` deliberately not touched.** A sibling agent is editing it concurrently (observed mid-task: new finding-8 lines for `report_probe.cir`), so adding to it would conflict. All new decks live in `repro/reads/` and `repro/spiceinit-fold/` (new subdirectories, no filename collision with the `repro/plain.cir` a sibling just added), each command in the issue is literally runnable from the deck's directory, and `repro/reads/.gitignore` covers the four files `capture.cir` generates.

**Batch (d)** — the one elision in the `set_case_mode()` excerpt is now marked `/* ...17 lines of comment elided, inpcom.c:1100-1116... */` (count verified); the `capture.cir` excerpt marks its two cuts with `...`; all other transcripts are pasted whole.

**No change required in any sibling issue.**

Files: `/home/qflow/dev/ngspice_test/doc/codex/issues/0058-casemode-diagnostics-fire-once-per-file-read.md` (577 lines, six headings in order, no seventh section); new decks under `/home/qflow/dev/ngspice_test/doc/claude/feedback/ngspice_upstream/repro/reads/` and `/home/qflow/dev/ngspice_test/doc/claude/feedback/ngspice_upstream/repro/spiceinit-fold/`. `src/` and `tests/` untouched (`git status` clean for both).

## Re-check

- verdict: **GOOD** · decks_committed: True · cross_refs_ok: True

### Still unresolved

- (none)

### Regressions introduced

- (none)
