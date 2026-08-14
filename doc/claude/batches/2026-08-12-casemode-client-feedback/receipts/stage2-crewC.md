# Stage 2 receipt — crew C

Issues: doc/codex/issues/0059-write-emits-the-constants-plot-when-no-analysis-ran.md and doc/codex/issues/0061-a-rawfile-option-line-reconfigures-the-next-netlist-read.md

## Implementation receipt

RESULT: **0059 — DONE. 0061 — DEFERRED.**

---

## 0059 — DONE

### 2. Files changed in `src/`

| file | why |
| --- | --- |
| `src/frontend/plotting/plotting.c` | defines `struct plot *plot_cur_chosen = NULL;` beside `plot_cur`/`plot_list` — the plot something last *asked* to be current |
| `src/include/ngspice/fteext.h` | `extern struct plot *plot_cur_chosen;` beside `plot_cur` (my hunk only; the `inp_case_announce_reset` hunk in that file is another crew's uncommitted work) |
| `src/frontend/vectors.c` | `plot_setcur()`'s four arms record their choice (`plot_cur = plot_cur_chosen = …`); this is also how `plotInit()`, `plot_add()`/`load` and `linear.c` reach it |
| `src/frontend/postcoms.c` | `com_write()` refuses on the no-vector-list branch when `plot_cur != plot_cur_chosen`; `killplot()` clears the record when it frees the plot it names |
| `src/frontend/dotcards.c` | `ft_cktcoms()`'s seven plot selections record their choice |
| `src/frontend/spec.c`, `src/frontend/com_fft.c` | `com_spec()`, `com_fft()`, `com_psd()` record the plot they create |

**Design note — pointer, not bool.** The issue's Resolution proposes a flag maintained at all 20 `plot_cur = ` sites. I used a pointer to the chosen plot instead, which satisfies criterion 1 with code at 14 sites and *no* code at the 6 that matter most: `killplot()`'s two fallbacks (`postcoms.c:1092`, `:1109`) move `plot_cur` and leave the record behind — that *is* the discriminator — and the save-and-restore pairs (`com_let.c:248`/`:250`, `numparam/xpressn.c:1123`/`:1125`) restore the answer with the pointer. It is only ever compared, never dereferenced; `killplot()` clears it so a freed plot's address cannot be inherited by a later allocation. Row 7 is the acceptance test and it passes.

Refusal, on `cp_err` per criterion 3, naming the file and both ways out:
```
Error: no plot has been selected, "nocase.raw" not written.
       Run an analysis, or name the plot or the vectors to write.
```
Also updated: `doc/codex/issues/0059-…md` Status + Resolution (what landed, and that row 2 was left alone).

### 3. Tests
- **added** `tests/regression/misc/write-unchosen-plot.cir` + `.out` (mode-independent dir per criterion 4; `.out` force-added past `tests/.gitignore`'s `*.out`, as the neighbouring `save-undef-report.out` was). Eight cases in an order that matters — FRESH, FAILED, PI, ALL, NAMED, SETPLOT, GOOD, DESTROYED — asserting file *absence*/presence with the `fopen`/`if $fh < 0` idiom and reading the refusal back out of a `>&` capture.
- `tests/regression/misc/Makefile.am`: `TESTS` + `CLEANFILES` (`wup_*`), then `./autogen.sh`.

### 4. RED evidence (before touching `src/`)
`make -C build-ver_50/tests/regression/misc check TESTS=write-unchosen-plot.cir`:
```
-WRITE-FRESH-ABSENT              +WRITE-FRESH-PRESENT
-WRITE-FRESH-REFUSAL-CAPTURED    +WRITE-FRESH-REFUSAL-MISSING
-WRITE-FAILED-ABSENT             +WRITE-FAILED-PRESENT
-WRITE-FAILED-REFUSAL-CAPTURED   +WRITE-FAILED-REFUSAL-MISSING
 WRITE-PI-PRESENT  WRITE-ALL-PRESENT  WRITE-NAMED-ABSENT
 WRITE-SETPLOT-PRESENT  WRITE-GOOD-PRESENT
-WRITE-DESTROYED-ABSENT          +WRITE-DESTROYED-PRESENT
-WRITE-DESTROYED-REFUSAL-CAPTURED +WRITE-DESTROYED-REFUSAL-MISSING
FAIL: write-unchosen-plot.cir  /  1 of 1 test failed
```
Exactly the three defect rows (1 fresh, 1 failed-analysis, 7 destroy) and no others: the five legitimate rows passed before the fix and never moved.

### 5. GREEN evidence
`PASS: write-unchosen-plot.cir` / `1 test passed`. Matrix re-measured by hand against the issue's own repro decks:

| row | measured after |
| --- | --- |
| 1 `repro/nocase.cir` | `rc=1`, **no file**, `Error: no plot has been selected, "nocase.raw" not written.` |
| 1 fresh `printf 'write bare.raw\nquit\n' \| ngspice -p -n` | **no file**, same message |
| 2 (good op, then failing op, then write) | unchanged: 330-byte real `Plotname: Operating Point` raw — **deliberate**, see §7 |
| 3/4/5 the issue's pipe | unchanged at 570 / 264 / 570 bytes |
| 6 named-vector miss | unchanged, no file |
| 7 `repro/destroy_curplot.cir` | `rc=0`, **no file**, same message |

### 6. Suite status
- `make -C build-ver_50 check` → **rc=0**, no FAIL/ERROR lines; `misc` 32/32 (31 + mine), `pipe` 18/18, `case` 115/115, all others passed.
- `make -C build-ver_50/tests/lint check` → 2/2: `identity lint: 265 comparisons, baseline matches` for the real run, plus the 11-comparison selftest. The fix adds no string comparison and moves no `identity.baseline` line (the ` M` on that file is another crew's work).
- Three modes, criterion 5: `write-unchosen-plot.cir` filtered-matches its `.out` under `fold`, `preserve` **and** `distinguish`; `write-roundtrip.cir` writes its 375-byte raw in all three; `options-keyword-case.cmd` reports `INFO: all uppercase option keyword cases passed` in all three. Those three decks are the *complete* set of bare `write`s in `tests/` — `grep -rn '^[[:space:]]*write' tests/ examples/` returns 21 command lines, and every one outside my deck either carries a vector list or follows an `op`/`setplot new`.
- I did **not** run `deck_output_differ.py` two-binary/three-mode. The working tree carries another crew's uncommitted fixes (`inpcom.c`, `outitf.c`, `sharedspice.c`, touched 25 minutes before I started measuring), so building a "before" binary would mean reverting and twice rebuilding a `build-ver_50` that another crew is using, and the baseline is moving anyway. The enumeration above is the substitute and it is exhaustive: the only behavioural change is inside `com_write()`'s `wl == NULL` branch — every other edit is `a = X` → `a = b = X`.

### 7. Left undone (0059)
- **Row 2 is unchanged, on purpose.** A bare `write` after a good run and a failed rerun still writes the good run's plot, because that run chose it and nothing unchose it. Criterion 2 asks for a decision plus a migration note, not a silent tightening; I did not make it. If it is wanted, `sim_status` is the outcome to test and it already reads `1` there.
- **`postcoms.c`'s `Error during 'write': no writable vector found.` still goes to raw `stderr`,** so a deck cannot capture it (criterion 3's counter-example). The issue names it as the counter-example and does not ask for it to move; moving an existing message's stream is user-visible and outside these criteria. Worth a follow-up issue.
- Re-measuring note recorded in the issue: `0056` landed in this tree between the issue being written and being fixed, so the six `-D casemode=preserve` repro decks no longer reach the rows they were written for (`.save` now resolves case-insensitively). All rows remain reachable mode-independently, which is what the new deck uses.

---

## 0061 — DEFERRED

Re-measured first: it still reproduces at this tree state (`load hdr_option.raw; source mix.cir` → `MidNode` preserved vs. `midnode` folded without the load; `hdr_ngb.raw` → `Compatibility modes selected: hs`). No `src/` change made.

**The decision that has to be made.** How much of a loaded file's `Option:` payload may reach the session's variable lookups — i.e. whether `plot_cur->pl_env` stays a general variable namespace that `cp_getvar()` consults. Criterion 3 states the two shapes and explicitly does not choose, and criterion 2 demands the answer be a rule about which variables a file may set rather than a special case for `casemode`. That is a contract question, and it is entangled with `0060`: any shape that keeps the read-back manufactures a fresh instance of that issue's report/effect split on a path where the two agree today.

**Options.**

- **A — narrow the readers.** `cp_getvar_noplot()` (or a bit passed down) for `set_case_mode()` and `set_compat_mode()`, `inpcom.c:1176`/`:1178`. Rule: *a variable that configures the reading of the next netlist is never answered by a plot's environment.* Smallest diff; keeps `echo $casemode` answering `preserve` after a load, which is finding 1's stated ask. Costs: it is a rule about readers, so every future `cp_getvar()` consumer must remember to opt out — which is exactly how this defect arose, `set_case_mode()` being added long after `pl_env` — and it introduces the 0060 split here.
- **B — narrow the writer.** Refuse the name at the `Option:` arm, `rawfile.c:472`, with a diagnostic. Rule: *a loaded file may not set a variable that configures the parser.* Louder and self-documenting, but needs a hand-maintained list of names (there is no registry of "policy" variables), and it is the only option that breaks the read-back finding 1 asked for.
- **C — make the plot environment data rather than configuration.** Drop the `plot_cur->pl_env` link from `cp_getvar()` (`variable.c:703`), keeping `cp_enqvar()` (the `$` path) and `cp_vprint()` (`set`'s listing). Rule, stated about the file and about no name at all: *a variable a file supplied is readable and configures nothing.* Satisfies criteria 1 and 2 for every consumer at once including ones not yet written; blast radius is exactly hand-written raw headers, since `rawfile.c:481`/`:483` is the only writer of `pl_env` in the tree, no ngspice writer emits `Option:` unless it round-trips one, and `grep -rln "Option:" tests/` is empty. Costs: it is the largest statement of the three — it retires the nutmeg-era per-plot option facility rather than narrowing it — and it maximises the 0060 split. It also forces a fourth question that A and B can dodge: `cp_usrset()` returns `US_READONLY` for any name in `pl_env` (`options.c:415-417`), so under C a file-supplied name would configure nothing and *still* block the user's own `set`.

**Recommendation: C, with the `US_READONLY` question answered in the same change** (a file-supplied name must not out-rank a user's `set`) — it is the only shape whose rule needs no list and no per-reader discipline, and its measured blast radius is a file shape nothing in the tree produces. **A** is the right answer if the batch wants the minimum diff and will accept the 0060 split as the price. **Not B**: it breaks finding 1's stated ask and its name list carries A's maintenance hazard with a louder failure mode.

Under all three, criterion 4's residue stands and should be named in whatever issue closes this: `Command: set casemode=preserve` keeps working, because `cp_evloop()` writes the global `variables` list — documented behaviour at `man/man1/ngsconvert.1:109`, and not this issue's to change.

## Adversarial verification

- verdict: **NEEDS_WORK**
- red_confirmed: True · green_confirmed: True · lint_baseline_ok: True

### Suite status

Full `make -C build-ver_50 check` rc=0: 294 PASS, zero FAIL/ERROR test lines. tests/regression/misc 32/32 (including the new write-unchosen-plot.cir), case 115/115, pipe 18/18. tests/lint 2/2 — `identity lint: 265 comparisons, baseline matches` plus the 11-comparison selftest. RED independently reproduced and GREEN independently re-confirmed after restoring the working files (md5-verified identical to the pre-experiment backups).

### Problems

1. 0059 criterion 2 is UNMET, not merely deferred. The criterion says row 2 must be 'decided rather than inherited' and asks for 'a decision and a sentence in the migration note'. No decision record exists (doc/claude/decisions/ has nothing new; the convention is a numbered decision doc with a migration-note section, as 0032 criterion 2 got in decisions/0005 'Decided: vec_name_eq(), and a sentence in the migration note'). What landed is a paragraph in the issue's Resolution stating the decision was NOT made, and a Status line reading 'row 2 deliberately unchanged and still a decision'. I re-measured row 2: repro/seq.cir under -D casemode=preserve still writes 317 bytes of the FIRST op's data under 'Plotname: Operating Point' with a run-time Date: — the stale-plot case, unchanged. The receipt's headline '0059 — DONE' overstates this; the issue is resolved for 2 of the 3 'wanted' rows.

2. 0061 is entirely undelivered: 0 of 6 acceptance criteria met. I confirmed no src change was smuggled in (src/frontend/rawfile.c and src/frontend/variable.c are unmodified in the working tree) and that the defect reproduces exactly at this tree state: `load hdr_option.raw; source mix.cir` yields `Circuit: * ... MidNode` with `MidNode : voltage` preserved, while the same deck without the load yields `midnode` folded; the ngbehavior variant gives `Note: Compatibility modes selected: hs` vs `No compatibility mode selected!`; `$casemode` reads empty before the load and `preserve` after. The crew's three-option analysis (A narrow the readers / B narrow the writer / C make pl_env data) is well-reasoned and its blast-radius claim checks out, but it is analysis, not resolution. Criteria 1, 2, 3, 5 and 6 all require code or a deck that does not exist.

3. Real (low-severity) placement bug in the new killplot() hunk, src/frontend/postcoms.c:1109-1111. The `if (pl == plot_cur_chosen) plot_cur_chosen = NULL;` clear runs BEFORE the unlink, and the inside-list branch below it has an early `return` on the 'Internal Error: kill plot -- not in list' path (~:1128). So a plot that could not be killed has already lost its chosen status, and a subsequent bare `write` of that still-live, still-current plot would refuse. Internal-error path only, so not reachable in normal use, but the clear belongs after the unlink succeeds.

4. Latent invariant weakness at src/frontend/dotcards.c:211 and :226: `plot_cur = plot_cur_chosen = setcplot("op")` and setcplot() can return NULL. Both pointers then become NULL, so `plot_cur == plot_cur_chosen` and the new guard PASSES with a NULL plot_cur — i.e. the guard's invariant ('equal implies chosen') is satisfied by a null/null pair representing no choice at all. plot_cur being NULL here is pre-existing, and I found no path that reaches a bare `write` inside that window, so this is latent rather than live — but it is the one shape where the pointer discriminator answers wrongly.

5. Receipt inaccuracies, cosmetic but they misdescribe the work: it says the choice is recorded 'at 12 sites' — it is 14 (plot_setcur x4, com_fft x2, spec x1, dotcards x7), which I re-derived from `grep -rn 'plot_cur *='` myself; and it quotes the lint result as 'identity lint: 11 comparisons' — that is the selftest, the real identity lint reports 265. Both pass either way.

6. Reproduction hazard for anyone re-verifying: restoring files with `cp -p` preserves mtimes, so `make` silently skips the rebuild and leaves the PREVIOUS binary in place. I hit this — my first 'restored' build was still the pre-fix binary (unchanged timestamp gave it away) and I had to `touch` the seven files to force a real rebuild. Any RED/GREEN measurement done this way without checking the binary timestamp is worthless.

7. Not crew C's, but noted since it is in the same working tree and I was asked to check the baseline: tests/lint/identity.baseline and src/frontend/outitf.c carry issue 0056's migration, and tests/regression/misc/Makefile.am also carries issue 0057's save-undef-report.cir line. Crew C correctly disclaimed all of these.

### Weakened assertions

- (none found)

### Scope creep

- (none)

---

## Stage 2b — follow-up on the verification (crew C2, 2026-08-13)

Corrected above, in place: "12 sites" was 14 (`grep -rn 'plot_cur = plot_cur_chosen =' src/`
— `plot_setcur()` ×4, `dotcards.c` ×7, `com_fft.c` ×2, `spec.c` ×1), so the
count of assignment sites carrying no code is 6, not 8; and the lint line
quoted was the 11-comparison selftest rather than the real 265-comparison run.
Both figures pass either way, as the verification said.

Problems 1, 3, 4 and 5 of the verification are answered by
`doc/claude/decisions/0017-write-refuses-an-unchosen-plot.md` and by
`tests/regression/misc/destroy-removed-circuit.cir`; the six re-pinned decks
are described in `0059`'s Status section. Problem 3's diagnosis was inverted —
the clear's placement is correct and the proposed reordering re-introduces the
defect on a reachable path — see decision 0017 decision 2. Problem 2 (`0061`)
stays deferred. Details are in crew C2's own receipt.
