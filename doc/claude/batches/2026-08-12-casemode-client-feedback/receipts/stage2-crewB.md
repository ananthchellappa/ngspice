# Stage 2 receipt — crew B

Issues: doc/codex/issues/0058-casemode-diagnostics-fire-once-per-file-read.md and doc/codex/issues/0060-casemode-variable-reports-the-request-not-the-effect.md

## Implementation receipt

RESULT
- **0058 — DONE.**
- **0060 — DEFERRED.** The acceptance criteria fix every part of the contract except the one that is permanent and user-visible: the variable's name. Details at the end.

---

## 0058

**2. Files changed in `src/`**

| file | why |
| --- | --- |
| `src/frontend/inpcom.c` | splits `set_case_mode()` into establish + announce. New file statics `ng_case_announced` / `ng_case_announced_unknown` latch the whole *outcome* of the last call (mode + unrecognised value string); `case_outcome_is_new()` compares and re-latches on every call, so the announcement fires on a change of outcome, not once per file read. Both `fprintf(stderr, …)` become `fprintf(cp_err, …)` (criterion 3). Establishment is untouched and still runs on every `inp_readall()`, command files included (criterion 2). |
| `src/include/ngspice/fteext.h` | declares `inp_case_announce_reset()` beside the other `inp_case_*` accessors. |
| `src/sharedspice.c` | calls `inp_case_announce_reset()` from `totalreset()`, so a host that `ngSpice_Reset()`s and re-loads a `distinguish` circuit is told the mode's warranty again rather than once per process lifetime. Not compiled in `build-ver_50` (`--with-ngshared` off), so it was checked with `gcc -fsyntax-only -DTHREADS -DHAVE_LIBPTHREAD -DSHARED_MODULE …` — clean, no implicit declaration (`ftedefs.h` pulls in `fteext.h`). |

Lint: the one new runtime/runtime comparison, `strcmp(unknown, ng_case_announced_unknown)`, is annotated in place — `else /* case-lint: keyword - two casemode values, see above */` — because casemode values are words the control language defines (matched with `cieq()` two functions down) and the question is whether the warning would print the same text. **`tests/lint/identity.baseline` is unchanged by me**: no line added, none removed, no migration.

**3. Tests added**

- `tests/regression/casedist/casemode-announce-report.cir` + `.out` (both `git add -f`'d — `tests/.gitignore` ignores `*.out`, so the reference file is invisible to a plain `git add -A`).
- `tests/regression/casedist/Makefile.am`: added to `TESTS`, `cma_*.txt cma_*.cir` to `CLEANFILES`, and a comment saying why it writes sub-decks. `./autogen.sh` re-run.

Shape is `doc/claude/decisions/0006` decision 2 (write sub-decks with `echo`, `source` under `>&`, scan the capture). Four cases, and each assertion is a **count**, not a presence: INHERIT (inner deck inherits the outer deck's outcome) → 0; BOGUS (`set casemode=bogus` first) → 1; REPEAT (same value again) → 0; BACK (`set casemode=distinguish` again) → 1. `CAPTURE-READ` tokens on both silences.

**4. RED evidence — proved RED twice, which is what separates the two halves of the fix**

RED 1, shipped binary (`make -C build-ver_50/tests/regression/casedist check TESTS=casemode-announce-report.cir`) — the diagnostics are on literal `stderr`, so `>&` cannot see them and both count assertions read zero:

```
@@ -8,14 +8,14 @@
-CASEMODE-BOGUS-ONCE
+CASEMODE-BOGUS-NONE
-CASEMODE-BACK-ONCE
+CASEMODE-BACK-NONE
FAIL: casemode-announce-report.cir
1 of 1 test failed
```

RED 2, binary carrying **only** the `stderr` → `cp_err` move and no latch — the two counts now pass and the two silences fail, i.e. the deck sees the once-per-read repetition itself:

```
@@ -4,14 +4,14 @@
-CASEMODE-INHERIT-SILENT
+CASEMODE-INHERIT-ANNOUNCED
 CASEMODE-INHERIT-CAPTURE-READ
 CASEMODE-BOGUS-ONCE
-CASEMODE-REPEAT-SILENT
+CASEMODE-REPEAT-ANNOUNCED
FAIL: casemode-announce-report.cir
```

**5. GREEN evidence** — same deck, same `.out`, never edited after it was written:

```
PASS: casemode-announce-report.cir
1 test passed
```

and the run now puts exactly **one** banner on the terminal where it put six.

The issue's seven-row acceptance table, re-measured on the fixed binary from `doc/claude/feedback/ngspice_upstream/repro/`, all now equal the "under the rule" column:

```
row1 SPICE_SCRIPTS=. -n : 1     (was 1)
row2 normal -n         : 1      (was 2)
row3 +.spiceinit       : 1      (was 3)
case_collision dist    : 1      (was 2)
am.cir bogus           : 1      (was 3);  is_now = 3.000000e-14  (model file still read)
switcher dist          : 1      (was 2)
switcher_d fold->dist  : 1      (was 1)   <- the change is still announced
switcher_b fold->bogus : 1      (was 1)   <- the misspelling is still announced
outer.cir bogus        : 1      (was 3)
-p, -p+1 source, -p+2  : 1 / 1 / 1        (was 2 / 3 / 4)
.include / .lib        : 1 / 1, v(midnode) = 2.250000e+00 both
auto-bridge dist/bogus : 1 / 1  (was 2 / 3); v(a) = 0 unbridged, 3.3 bridged
```

Criterion 2 (the call must not move) re-measured in `repro/spiceinit-fold/`: `fold` → `mixedvar 1`, `preserve` → `MixedVar 1`, unchanged. `switcher.cir` still holds two modes in one process (`op1`'s net folded, `op2`'s preserved).

**6. Suite status**

- `make -C build-ver_50/tests/regression/casedist check` → **all 26 tests passed**.
- `make -C build-ver_50/tests/lint check` → `identity lint: 265 comparisons, baseline matches` — PASS, both specs.
- `make check -C build-ver_50` → **293 PASS, 0 FAIL, rc=0** (includes `tests/regression/case` (preserve), `casedist` (distinguish), `xspice/case`, `xspice/casedist`, pipe, model QA, lint last).
- Criterion 5's suite effect, measured: `tests/regression/casedist` now emits **26** banners for its 26 decks (the issue's census measured 62 for the 23 it then had) and `tests/xspice/casedist` **17** for 17 (was 51) — one per deck.
- I also updated `doc/codex/issues/0058`'s Status and Resolution in place, per the repo's issue convention.

**Note on the working tree:** another crew has uncommitted work here (`src/frontend/outitf.c`, `tests/lint/identity.baseline`, the `save-name-case`/`save-undef-report` decks, `doc/claude/decisions/0001-distinguish.md`). My 293/0 includes their changes; the 23→25→26 deck count in `casedist` is theirs plus mine. My edits to `tests/regression/casedist/Makefile.am` interleave with theirs cleanly.

**7. Left undone (0058)**

- `doc/claude/scripts/case_differential_sweep.py` was **not** run. The argument, not a measurement: the `fold` and `preserve` arms print nothing in either version, no computed value is touched, and the only behavioural delta is on `cp_err`, which no deck but the new one reads. Full `make check` covers the preserve and distinguish directories.
- No `doc/claude/decisions/00NN` record written — the house style would want one; I left it to the batch owner rather than claiming a decision number that may collide with the other crew's.

---

## 0060 — DEFERRED

**The decision that has to be made: the name of the new read-only variable.** Everything else in 0060 is fully determined and I verified each mechanism against the source, so this is the only blocker:

- criterion 2 (live read of `inp_case_mode()`), 3 (the four divergence points), 4 (absent on `ngspice-46` → `Error: <name>: no such variable.`) and 5 (`cp_usrset()`'s `US_READONLY` arm, `options.c:411`, measured: `set plots=1` → `Error: plots is a read-only variable.`) are unambiguous;
- criterion 6's fork ("refused as a plot-environment key **or** resolved ahead of one") is settled by the issue's own Resolution: resolved ahead of `cp_enqvar()`'s `pl_env` loop (`options.c:58-65`);
- the `plots` / `curplot*` precedent settles the rest of the plumbing — synthesize in `cp_enqvar()`, add to `cp_usrvars()` (`options.c:185-214`) so it also reaches `cp_getvar()` and the `set` listing (measured: `plots` and `curplot*` do list, prefixed `*`), one `eqc()` arm in `cp_usrset()` returning `US_READONLY`.

The name appears nowhere: `grep` over `doc/`, `src/`, `tests/` finds no proposed spelling, FINDINGS finding 8 says only "expose the effective mode as a read-only variable", and the guide (`doc/claude/casemode-distinguish-guide.md`) proposes none. It is the whole user-facing surface — every client will hard-code `echo $<name>` — and it can never be renamed once shipped, so I am not inventing it.

**Options**

1. **`curcasemode`** — follows the tree's only precedent for a synthesized read-only variable (`curplot`, `curplotname`, `curplottitle`, `curplotdate`, `plots`), sorts beside them in a `set` listing, reads as "the case mode currently in effect". Against it: in this tree `cur*` has so far meant *of the current plot*, and that connotation is actively wrong here — `doc/codex/issues/0061` is precisely a rawfile's `Option:` line making a plot-scoped value answer a session-scoped question, which criterion 6 exists to prevent.
2. **`effectivecasemode`** — says exactly what it is, no plot connotation, and pairs visibly with `casemode` in an alphabetical listing. Long, and ngspice control variables are otherwise short single words (`ngbehavior`, `noclobber`, `sourcepath`).
3. **`casemodenow`** / **`casemodeineffect`** — keeps the `casemode` prefix so the pair sorts adjacent and the relationship is obvious; the first is short, the second unambiguous and ugly.
4. **`casemode_effective`** — clearest to read, but no ngspice control variable uses an underscore, so it would be the first.

**Recommendation: option 2, `effectivecasemode`.** The `cur*` precedent is real but it is the precedent for *plot* attributes, and criterion 6 plus 0061 make "this is not a property of the loaded plot" the single most important thing the name has to convey. If the batch owner prefers the shorter spelling, option 3's `casemodenow` is my second choice; I'd avoid `curcasemode` for the reason above.

**Three secondary points to settle in the same breath**, so the implementer does not have to guess:

- Should it appear in a bare `set` listing? Following `plots` exactly: yes, via `cp_usrvars()` — which is also what makes `cp_getvar()` and `$?name` see it. Silent-in-listing is the alternative and would need `cp_enqvar()` only.
- With a `cp_usrset()` `US_READONLY` arm, `-D <name>=x` on the command line becomes a new start-up `Error: <name> is a read-only variable.` That is probably right, but it is user-visible.
- `cp_enqvar()` matches with `eq()`, so `$EffectiveCaseMode` will not resolve — consistent with today's `casemode`, and `doc/codex/issues/0048`'s subject, not a new defect.

Estimated size once the name is fixed: ~30 lines across `src/frontend/options.c` (+ `inp_case_mode()` already exists and is already the single authority), one `.cir` in `tests/regression/case/` and one `.cmd` in `tests/regression/pipe/` per criterion 7. Nothing in it depends on the 0058 work beyond both touching `casemode`.

## Adversarial verification

- verdict: **NEEDS_WORK**
- red_confirmed: True · green_confirmed: True · lint_baseline_ok: True

### Suite status

Full `make check -C build-ver_50`: 293 PASS, 0 FAIL, rc=0 (every directory summary reads "All N tests passed"). tests/regression/casedist 26/26 PASS. tests/xspice/casedist 17/17 PASS. tests/lint 2/2 PASS ("identity lint: 265 comparisons, baseline matches" + selftest 11). Banner census re-measured on the fixed binary: casedist emits 26 banners for 26 decks (issue measured 62 for the 23 it then had), xspice/casedist 17 for 17 (was 51).

### Problems

1. 0060 is not done. DEFERRED with no code, no test, and the issue still reads Status: Open / Resolution: Unresolved (correctly - nothing was falsely closed). The deferral rationale is factually right as far as it goes: I confirmed no candidate name exists anywhere (grep -riE 'effectivecasemode|curcasemode|casemodenow|casemode_effective' over doc/ src/ tests/ is empty) and FINDINGS' ask 8 is only 'read-only "effective mode" variable'. But its load-bearing premise - 'it can never be renamed once shipped' - overstates the cost: `git show pre-master-47:src/frontend/inpcom.c | grep -c casemode` is 0, so the entire casemode feature is unreleased branch work and any name chosen now is still renameable before merge. Escalating the name is defensible; treating it as a hard blocker on ~30 lines of otherwise fully-specified work is a judgement the batch owner should re-take, not accept.

2. Criterion 1's wording vs the implementation. The criterion says announce when the outcome 'differs from the outcome last announced, and not otherwise'. case_outcome_is_new() re-latches on EVERY call, announcing or not, so the comparison is against the previous READ, not the previous ANNOUNCEMENT. The two readings diverge on a distinguish -> preserve -> distinguish sequence. I wrote that deck and measured the fixed binary: it prints the experimental banner TWICE (under the criterion's literal wording the second is owed nothing, since the last announced outcome was already DISTINGUISH). None of the issue's seven rows discriminates, and the code comment states the chosen semantics openly, so this is a defensible reading rather than a bug - but it is a silent reinterpretation of an acceptance criterion and should be confirmed (or the criterion reworded) before sign-off.

3. Unaccounted-for edit to the issue file. receipts/stage1b-0058.md records the issue as '577 lines, six headings in order, no seventh section'. It is now 637 lines with 569 lines before '## Resolution' - i.e. 8 lines disappeared from Status..Acceptance Criteria after stage 1b, while the crew's receipt discloses only 'Status and Resolution'. The file is untracked, so no diff is possible and a concurrent doc agent cannot be ruled out. Everything the stage-1b receipt lets me cross-check is intact: criterion 1's outcome-latch rule and its seven-row derivation, the 62/51/113 banner and 19840/16320/36160 byte census, and the 24-at-2 / nine-at-3 / seven-at-4-to-6 distribution all match the receipt exactly. I found no weakened criterion - but the 8 lines are unexplained.

4. Self-declared gaps, both real: doc/claude/scripts/case_differential_sweep.py (present in the tree) was not run - the crew substitutes an argument (the fold/preserve arms print nothing, no computed value moves) for a measurement; and no doc/claude/decisions/00NN record was written for a change that alters a user-visible diagnostic contract, which the crew left to the batch owner to avoid colliding with crew A's number.

5. Not crew B's, but surfaced while checking the lint gate: crew A's in-place annotations in src/frontend/outitf.c read '/* case-lint: stored - neither operand is a query */'. CLAUDE.md and identity_lint.sh's own help text document exactly one reason word, 'keyword'; the scanner only matches the marker, so 'stored' passes unexamined. That is how a third, undocumented category enters the tree by the back door. Worth a look by whoever verifies crew A.

### Weakened assertions

- (none found)

### Scope creep

- src/include/ngspice/fteext.h and src/sharedspice.c were edited although LEDGER.md assigns crew B only src/frontend/inpcom.c. Both are one-liners (a declaration, and inp_case_announce_reset() in totalreset()) and are demanded by the issue's Root Cause (the latch must survive ngSpice_Reset) and by CLAUDE.md, so the expansion is justified rather than gratuitous. Note it is compile-checked only: build-ver_50 is not --with-ngshared, so the reset call is never executed by any test. I re-verified it compiles: gcc -fsyntax-only -DSHARED_MODULE -DTHREADS ... src/sharedspice.c is clean (only a THREADS redefine warning caused by my own -D).
- doc/codex/issues/0058-...md was edited in place (Status -> Closed, Resolution added). That follows the repo's issue convention and the crew disclosed it, but see problem 4: the file is untracked, so there is no baseline for the Acceptance Criteria it also had to leave alone.