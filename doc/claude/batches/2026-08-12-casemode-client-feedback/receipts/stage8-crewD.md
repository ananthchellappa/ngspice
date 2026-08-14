# Stage 8 receipt — crew D

## Implementation

RESULT: issue 0060 implemented and closed; all 8 acceptance criteria satisfied; `make check` 303 PASS / 0 FAIL. Nothing committed.

## Per item

| Item | Result |
|---|---|
| `curcasemode` in the computed read-only family | DONE |
| C1 second read-only variable, `casemode` untouched | DONE |
| C2 answers `inp_case_mode()` per read, never cached | DONE |
| C3 all four divergence shapes | DONE for three; shared-library shape documented, unexercised (no `libngspice` in `build-ver_50`) |
| C4 read FAILS on a featureless build | DONE, measured against `/usr/local/bin/ngspice` |
| C5 write refused via existing `US_READONLY` arm | DONE |
| C6 not shadowable by a loaded plot's environment | DONE — **passes now, crew E not required** |
| C7 a deck and a `.cmd` assert it | DONE, three new tests |
| C8 `make check` unchanged, no `harness-alive.out` edited | DONE |

**C6 is not left failing-by-design.** The brief anticipated it might have to be. It does not: the criterion's own text allows *either* refusing the key *or* resolving ahead of one, and resolving ahead is entirely inside `cp_enqvar()`, which is my file. The arm sits before the `if (plot_cur)` block, so `pl_env` is never reached for this name. `casemode` itself remains shadowable and unwritable after a `load` — that half is still 0061/0065 and I did not touch it.

## Files changed

- `/home/qflow/dev/ngspice_test/src/frontend/inpcom.c` — new `inp_case_mode_name()` beside `inp_case_mode()` and the `static int ng_case_mode`, so the three spellings sit next to `set_case_mode()`'s three `cieq()` arms and cannot drift.
- `/home/qflow/dev/ngspice_test/src/include/ngspice/fteext.h` — its declaration beside the other four case predicates.
- `/home/qflow/dev/ngspice_test/src/frontend/options.c` — `cp_enqvar()` arm placed *before* the `plot_cur` block (answers with no current plot, and ahead of `pl_env`); `cp_usrset()` arm returning `US_READONLY` after `plots`; two stale header comments corrected to name the new member.
- `/home/qflow/dev/ngspice_test/src/include/ngspice/sharedspice.h` — the read-back and the capability probe, beside the latch semantics already documented there.
- `/home/qflow/dev/ngspice_test/doc/claude/casemode-distinguish-guide.md` — §2 leads with `echo $curcasemode` and keeps the three-way deck probe as the portable fallback; §9 gains the one-spawn client probe with a measured four-row table; the guide's *"no pipe-mode probe can [see preserve]"* sentence corrected, it was true only of identity-based probes.
- `/home/qflow/dev/ngspice_test/doc/codex/issues/0060-casemode-variable-reports-the-request-not-the-effect.md` — Status → Closed 2026-08-13, full Resolution.
- Three `Makefile.am` (`case`, `casedist`, `pipe`) — `TESTS` + `CLEANFILES` + the directory-comment line each dir's convention requires. `./autogen.sh` re-run.

## Tests added

- `/home/qflow/dev/ngspice_test/tests/regression/case/curcasemode-effect.cir` (+`.out`) — C3 bullet 1 and C5, under `preserve`.
- `/home/qflow/dev/ngspice_test/tests/regression/casedist/curcasemode-relatch.cir` (+`.out`) — C2, under `distinguish`: a self-written sub-deck sourced to re-latch.
- `/home/qflow/dev/ngspice_test/tests/regression/pipe/curcasemode-effect.cmd` — C3 bullets 2 and 4, C5, C6, by exit code.

The two `.out` files are `git add -f`'d (index now 13 pre-existing + 2 = 15). The `.cir`/`.cmd` are untracked, matching the batch's existing pattern. Nothing else staged, nothing committed.

## RED evidence

Deck A, before the fix — the variable answers the *write*, which is the defect:
```
CURCASEMODE-AT-ENTRY                       <- bare; "Error: curcasemode: no such variable." on stderr
CASEMODE-REQUEST-NOW fold
CURCASEMODE-AFTER-LATE-SET
SET-CURCASEMODE-ACCEPTED
CURCASEMODE-AFTER-OWN-SET distinguish      <- the read answered the set
```
Deck B, before the fix: all four `CURCASEMODE-*` tokens bare, four `Error: curcasemode: no such variable.` on stderr.

Pipe, before the fix: `rc=1` at the first check — `ERROR: with no casemode set, curcasemode did not read back fold`.

The C6 shadow, reproduced before the fix (a raw header carrying `Option: curcasemode=preserve` in a default-`fold` session): `AFTER-CURCASEMODE preserve`.

## GREEN evidence

All three PASS. Plus two mutation checks, because "passes with its own mechanism disabled" is the failure mode here:

1. Removed only the `cp_usrset()` `US_READONLY` arm → deck A diffs `-SET-CURCASEMODE-REFUSED / +SET-CURCASEMODE-ACCEPTED`, `CURCASEMODE-AFTER-OWN-SET preserve → distinguish`; pipe FAILs at check 5.
2. Moved the `cp_enqvar()` arm to *after* the `plot_cur` block, changing nothing else → pipe FAILs at check 6 (`a loaded rawfile's Option: line answered the curcasemode read`) while both `.cir` decks still PASS. So the arm's *position* is load-bearing and the deck catches it.

Implementation restored, rebuilt, mtime moved (14:59:44), suite re-run.

Cross-binary, the property that makes it a probe:
```
$ printf 'echo v $curcasemode\nquit\n' | /usr/local/bin/ngspice -p -n
stdout: v            stderr: Error: curcasemode: no such variable.
$ printf 'echo v $curcasemode\nquit\n' | build-ver_50/src/ngspice -p -n
stdout: v fold       stderr: (empty)
```
And `distinguish`/`preserve`/`fold`/no-flag all read back correctly under `-D`.

## Suite status

`make -C build-ver_50 check` with every cached `*.log`/`*.trs` deleted first: **rc=0, 303 PASS, 0 FAIL, 0 SKIP** — run three times (after the fix, after the doc/header edits, after restoring from the mutations). `tests/lint`: `identity lint: 265 comparisons, baseline matches` — **no `identity.baseline` change**, because both new comparisons have a literal operand and the lint never reports those. No `/* case-lint: */` marker was needed or added.

Binary mtime moved on every rebuild (14:43 → 14:51 → 14:59:44). Note the size dropped 9382264 → 8087984 on the first rebuild: `autogen.sh` triggered `config.status --recheck`, which relinked everything under the configured `CFLAGS` (`-O2 -s …`). `ac_cs_config=''` confirms the bare `../configure` was preserved, and `config.h` still has `XSPICE 1` / `OSDI 1`.

## Residuals

- **Shared-library route unexercised.** C3's third bullet is the same `cp_enqvar()` call the prompt takes, but no `libngspice` is built in `build-ver_50`, so the risk is that the *build* differs, not the path. Documented in `sharedspice.h` and named in the Resolution.
- **The three weak liveness probes are untouched.** `tests/regression/case`, `tests/regression/casedist` and `tests/xspice/casedist`'s `harness-alive.cir` still echo `$casemode` and are still satisfied by a binary that accepts the flag and ignores it; `tests/regression/casedist/vector-pyplot-probe-report.cir:64`'s gate is still correct by circumstance. C8 says strengthening them is a separate change with its own argument. `curcasemode` is what that change would use — this is the most valuable follow-on.
- **0061/0065 still own `casemode` itself**: after a `load` it is simultaneously unwritable and wrong. Only the new name is protected.
- `curcasemode` is matched with `eqc()` on both read and write, so `$CURCASEMODE` answers and `set CURCASEMODE=` is refused — deliberate, matching the `curplot`/`plots` family it joins. The selector's byte-exact name match (`-D CaseMode=preserve` a silent no-op) is unchanged and still 0048.
- Pre-existing, not mine: `src/frontend/inpcom.c:1657` `warning: "/*" within comment` (upstream, present at HEAD), and one `ERROR: (internal) tried to destroy non-existent graph` line inside `vector-probe-report.cir`'s passing output.

## Verification

- verdict: **NEEDS_WORK**

### Suite

make -C build-ver_50 check, run twice from deleted *.log/*.trs (once on arrival, once after restoring the tree from five mutations): rc=0, 303 PASS / 0 FAIL / 0 SKIP / 0 XFAIL both times. tests/lint: `identity lint: 265 comparisons, baseline matches` and `identity lint: 11 comparisons, baseline matches`, both PASS, identity.baseline carries no crew-D entry. The three new tests PASS by name in the full run. Working tree verified byte-identical to its start-of-session state after all mutations (md5 of options.c and inpcom.c OK, 213 porcelain entries, same M/A set); index holds 15 added .out files and nothing else, and index == worktree for every staged path.

### Criteria met

- C1 second read-only variable, casemode untouched — MET on my own measurement. `set casemode=preserve` is still accepted with no error; `curcasemode` answers exactly `fold`/`preserve`/`distinguish` under `-D casemode=fold|preserve|distinguish`, and `-D casemode=bogus` answers `fold` (the value the latch actually establishes), so the value set is closed over the three spellings `set_case_mode()` accepts.
- C2 answers inp_case_mode() per read, never cached — MET. My own deck (not the crew's): under `-D casemode=distinguish`, AT_ENTRY distinguish → `set casemode=preserve` → still distinguish → `source inner.cir` → preserve → `set casemode=distinguish` → still preserve. Mutation M4 (make `inp_case_mode_name()` cache on first call, nothing else changed) rebuilt clean and turned `curcasemode-relatch.cir` and pipe check 3 red while `curcasemode-effect.cir` stayed green — the no-cache property is genuinely bound by a test.
- C3 all four divergence shapes — MET, including the third bullet the receipt left unexercised. (a) `.control` after the read: ran the issue's own `repro/late_set.cir` with the echo extended — `casemode-is preserve effective-is fold` while `display` shows the folded `midnode`. (b) prompt/`-p`: `set casemode=distinguish` → `req distinguish eff fold`; `-D casemode=distinguish` then `set casemode=fold` → `req fold eff distinguish`. (c) shared library: I built libngspice from the repo's already-configured `build-shared/` tree and ran my own host — CUR1 fold (fresh session) → `set casemode=preserve` → CUR2 fold → `ngSpice_Circ()` → CUR3 preserve → write refused, CUR4 preserve → `set casemode=distinguish` → CUR5 preserve → second `ngSpice_Circ()` → CUR6 distinguish. Exactly what `sharedspice.h` now claims. (d) no `casemode` variable at all → `fold`, not `no such variable`.
- C4 read FAILS on a featureless build — MET. The guide's exact pipeline against `/usr/local/bin/ngspice` (ngspice-46) yields zero stdout lines and `Error: curcasemode: no such variable.` on stderr, with or without `-D casemode=distinguish`; `build-ver_50/src/ngspice` yields `fold`/`preserve`/`distinguish` on stdout.
- C5 write refused through the existing US_READONLY arm — MET. `set curcasemode=preserve` → `Error: curcasemode is a read-only variable.` (cp_vset's message), `unset curcasemode` → `Error: curcasemode is read-only.` (cp_remvar's), value unchanged in both cases. Also refused on the `-D curcasemode=preserve` command-line route, so the selector cannot be shadowed from argv either; `set CURCASEMODE=` is refused and `$CURCASEMODE` answers, matching the `curplot`/`plots` family's eqc() convention. Mutation M2 (drop only the `cp_usrset()` arm) turned `curcasemode-effect.cir` red and pipe check 5 red, relatch correctly indifferent.
- C6 not shadowable by a loaded plot's environment — MET, by resolution order, which criterion 6's own text permits. My own rawfile carrying `Option: casemode=preserve curcasemode=preserve`, loaded into a `-D casemode=distinguish` session: `$casemode` answers `preserve` (the positive control — the entry really landed and really would have answered) while `$curcasemode` still answers `distinguish`; in a default session it answers `fold`. `let curcasemode = 3` (a vector) and circuit variables cannot shadow it either. Mutation M3 (move the arm to after the `plot_cur` block, nothing else changed) failed pipe check 6 while both decks stayed green, so the arm's position is load-bearing and covered.
- C7 a deck and a .cmd assert it — MET. Three new tests, all RED-derived non-destructively (`git show HEAD:src/frontend/options.c > src/frontend/options.c`, rebuild, mtime moved 14:59:44→15:06:00): all three FAIL, with `curcasemode-effect.cir` reproducing the receipt's RED transcript byte for byte (bare tokens, `SET-CURCASEMODE-ACCEPTED`, `CURCASEMODE-AFTER-OWN-SET distinguish`) and the pipe test failing at check 1. Restored, rebuilt, all three PASS. Fifth mutation M5 (arm answers `cp_getvar("casemode")` instead of the latch — the 'echo $casemode under another name' impostor) fails all three.
- C8 make check unchanged, no harness-alive.out edited — MET. `make -C build-ver_50 check` from deleted *.log/*.trs: rc=0, 303 PASS / 0 FAIL / 0 SKIP / 0 XFAIL, run twice (once on arrival, once after restoring from five mutations). `git status` shows no modification to any of the three `harness-alive.out` files nor to any other reference file; the only added files are new `.out`s. tests/lint: `identity lint: 265 comparisons, baseline matches` — the two new comparisons each have a literal operand, which the lint never reports, so no baseline entry and no `/* case-lint: */` marker were needed.

### Criteria unmet

- (none)

### Regressions

- None attributable to crew D. 677 decks under examples/ and tests/ were run in a sandbox copy of both trees (so the repo was never polluted) against three binaries: red = crew D's `options.c` reverted to HEAD, green and green2 = the working tree. Exit codes are identical on all 677 between red and green2. After normalising timestamps, timing lines and the wall-clock-driven `Reference value :` progress lines, red-vs-green2 differs on exactly the green-vs-green2 noise-floor set (16 chaotic/noise/measure decks) plus the two curcasemode decks. Zero attributable output change.
- The 178 rc differences between stock /usr/local/bin/ngspice and the new binary are all pre-existing configuration effects and reproduce identically on the red binary: CIDER is off in build-ver_50 (rc=1 where stock runs), and XSPICE code models do not load when a deck is run from an arbitrary cwd (rc=139), which is the documented `cwd must be the build test dir` behaviour. Two decks improve (define-const-body-listing 139→0, define-formal-body 134→0) and belong to other crews.
- No new memory error or leak. valgrind: five `$curcasemode` reads give the same profile as five `echo x` (2 bytes definitely lost, 0 errors, both). The refusal path leaks identically to the pre-existing `set plots=` arm (98 bytes / 4 blocks definitely lost in both; the indirect delta 42 vs 24 bytes is just the longer name).
- 0065's crash surface is not widened: `Option: plots=1` + `set` SIGSEGVs on stock and new alike (pre-existing), while `Option: curcasemode=preserve` + `set` exits 0, because the new name is answered before `pl_env` and is not among the five names `cp_usrvars()` asks for.

### Correct work now refused or diagnosed

- `set curcasemode=<anything>` and `-D curcasemode=<anything>` are now refused where any value was previously accepted silently. I grepped the whole tree: no deck, script, example, man page or upstream source uses the name outside this change, so nothing correct is broken. Intended by C5.
- A rawfile whose `Option:` line names `curcasemode` is now silently ignored for reads of that name, while every other `Option:` key still answers. Intended by C6 and required by it, but it is a real asymmetry a file author could trip over, and nothing warns.