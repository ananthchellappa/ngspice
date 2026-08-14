# Stage 8 receipt — crew E

## Implementation

RESULT: both items landed, both issues Closed, suite green.

## RESULT per item

- **0061, shape A** — DONE. A plot's environment no longer answers a `cp_getvar()` read taken while a netlist is being read. The pair is still filed, still listed by `set`, still readable through `$name`.
- **0065, the crash** — DONE, fixed with it, and the shadowing with it: `cp_enqvar()` answers what it computes before consulting `pl_env`, and `cp_usrvars()` links only nodes it owns.
- **Crew D**: nothing was failing by design. `tests/regression/pipe/curcasemode-effect.cmd` (incl. its criterion‑6 check) already passed and still passes; its check 6 now holds for a second, stronger reason — `curplot`'s whole family is unshadowable, not just `curcasemode`.

## Files changed (why, one line each)

- `/home/qflow/dev/ngspice_test/src/frontend/inpcom.c` — `inp_readall()` becomes a 5‑line wrapper raising a depth count over the read (body renamed `inp_readall_cards()`, signature identical); adds `inp_reading_netlist()` / `inp_netlist_read_reset()`. The window is the whole reader, not a name list.
- `/home/qflow/dev/ngspice_test/src/frontend/variable.c` — `cp_getvar()`'s plot‑environment link gains `&& !inp_reading_netlist()`. That link only; `cp_enqvar()` (the `$` and `set` path) is untouched, which is what keeps shape A's read‑back working.
- `/home/qflow/dev/ngspice_test/src/include/ngspice/fteext.h` — the two declarations beside `inp_readall()`.
- `/home/qflow/dev/ngspice_test/src/frontend/signal_handler.c` — `inp_netlist_read_reset()` in `ft_sigintr_cleanup()`: an interrupt during a `source` longjmps out of `inp_readall()`, which never lowers its own guard.
- `/home/qflow/dev/ngspice_test/src/sharedspice.c` — same call in `totalreset()`, one line after 0058's `inp_case_announce_reset()`. (Nothing of the existing batch touched.)
- `/home/qflow/dev/ngspice_test/src/frontend/options.c` — `cp_enqvar()`: the `curplot`-family and `plots` arms now precede the `pl_env` scan; `cp_usrvars()`: five repeated link pairs replaced by `usrvar_push()`, which drops a `tbfreed == 0` answer.
- `/home/qflow/dev/ngspice_test/tests/regression/pipe/Makefile.am` — two `TESTS` entries, `rop_*`/`rcn_*` in `CLEANFILES`; `./autogen.sh` re-run.

The predicate keys on **whether a netlist read is in progress**, not on any variable name — `casemode`, `ngbehavior`, `no_auto_gnd`, `addcontrol`, `sourcepath` and the next one are covered without being named. The 0065 rule keys on **ownership** (`tbfreed`), not on the five names.

## Tests added

- `/home/qflow/dev/ngspice_test/tests/regression/pipe/rawfile-option-parse-policy.cmd` (211 lines) — baseline read of a deck it writes; the same deck after `Option: casemode=preserve`, after `casemode=distinguish`, after `ngbehavior=hs`; plus an ordinary key read back. Case checks compare against the *first read of the same deck*, so the deck is mode‑independent; `ngbehavior` is asserted as a number via `0**0` (1 everywhere, 0 under `hs`). Every "nothing moved" check has a positive control that reads the value back first, so refusing the key at the `Option:` arm would fail this deck, not pass it.
- `/home/qflow/dev/ngspice_test/tests/regression/pipe/rawfile-option-computed-name.cmd` (151 lines) — five poisoned loads in a `foreach`; one raw naming all five; the two‑plot `setplot tran1` + `display`/`print`/`write` sequence; one capture scan for the file's string, with a line count so a missing capture fails instead of passing.

No `.out` files (this directory compares none), so nothing to `git add -f`. Nothing staged, nothing committed.

## RED evidence (at `720c8743a` + decks, before the production change)

```
$ ngspice -p < tests/regression/pipe/rawfile-option-parse-policy.cmd ; echo rc=$?
ERROR: a loaded Option: casemode=preserve moved the mode the next read latched
rc=1
```
with the earlier checks removed so the later ones are reached: `ERROR: a loaded Option: casemode=distinguish moved the mode the next read latched` (rc=1) and `ERROR: a loaded Option: ngbehavior=hs changed how the next deck was parsed` (rc=1). Raw probe of both halves at once: `D1=-32` (title `MidNodeProbe` vs `midnodeprobe`), `D2=10` (`$curcasemode` preserve vs fold), `C1=0` (the `Option:` pair did land).

```
$ ngspice -p < tests/regression/pipe/rawfile-option-computed-name.cmd ; echo rc=$?
Segmentation fault (core dumped)
rc=139
```
no marker printed — it dies inside the first `load` of the loop.

## GREEN evidence

Both decks `rc=0`. Criterion measurements re‑run on the fixed binary (`build-ver_50/src/ngspice`, mtime moved 15:12 → 16:01:21):

- `load hdr_option.raw ; source mix.cir` and `source mix.cir` now give the identical four lines (`in`, `midnode`, `vs#branch`, folded title).
- `ngbehavior`: `Note: No compatibility mode selected!` with and without the load.
- read‑back kept: `after=preserve` / `effect=fold` (`$casemode` = the request, `$curcasemode` = the effect).
- ten‑row table: `rc=0` and marker on all ten; `echo $curplot` = `op1`; two‑plot `setplot tran1` + `display` / `print OUT` / `write` = `rc=0` each.
- valgrind (`load` + `display` on `Option: curplot=HAHA`): no Invalid read/write/free; 26 bytes in 2 blocks definitely lost, **both pre-existing** — 2 bytes appear on a raw with no `Option:` line, and `24+9` is reported byte‑identically by stock `/usr/local/bin/ngspice`. Stock on the same input: `Invalid read of size 8`, `Invalid read of size 1`, `Address … free'd`.
- mechanism isolation (each edit disabled one at a time, deck unchanged): guard removed from `cp_getvar()` → deck 1 fails, deck 2 passes; `cp_enqvar()` ordering reverted with `usrvar_push()` kept → **no fault at all**, deck 2 fails on the value instead; ordering kept with `!tbfreed` removed → deck 2 passes (disclosed in 0065's Resolution: the ownership rule is the invariant, the ordering is what the deck detects in the shipped pair).
- `src/sharedspice.c` compile-checked (`gcc -fsyntax-only -DSHARED_MODULE -DTHREADS`): 0 errors, no implicit declaration.

## Suite status

`make -C build-ver_50 check` with every cached `*.log`/`*.trs` deleted first: **305 passed, 0 failed** (303 before + the two new decks). Identity lint: `265 comparisons, baseline matches` — `identity.baseline` unchanged and needs no migration; the change adds no comparison, and the moved `eq(vv->va_name, word)` is recorded without a line number so it still matches. No existing `.out` moved. Targeted `tests/regression/pipe`: 21/21.

## Residuals (all named in the issues)

- `Command: set casemode=preserve` still reconfigures the next read (writes the global `variables` list, ahead of everything). Criterion 4 by design; `man/man1/ngsconvert.1:109`; measured after the fix.
- `set casemode=…` is still refused while a plot carrying that key is current (`cp_usrset()` `US_READONLY`, untouched) — a nuisance, no longer a parsing hazard.
- `raw_write()` still round-trips `Option:`; only the reconfiguration is gone.
- The window is `inp_readall()`; elaboration after it in `inp_spsource()` is outside it (no policy read of this shape found there).
- **New measurement, out of scope, wants its own file**: `unset curplot` (and `unset plots`) aborts, `rc=134`, on **stock ngspice-46 with no rawfile at all**. After the fix the poisoned-load case gives exactly that same pre-existing abort instead of `SIGSEGV` — i.e. the file no longer changes the outcome. The `unset zzz` fault 0065's Impact recorded as 139 did not reproduce today (`rc=0` on both binaries); same `cp_remvar()` arm, allocator-dependent. **Corrected 2026-08-13: this half is wrong, and it is where the error entered.** The measurement stopped at the `unset`, which does return `rc=0`; the fault is on the command after it. `load`, `unset zzz`, then `set` (or `display`, or `write`) is `rc=139`, 3/3, on `build-ver_50/src/ngspice` and on stock `ngspice-46`. Nothing here is allocator-dependent. 0065's `rc=139` was right; see `doc/codex/issues/0067`.
- One behaviour shift worth knowing: `$CurPlot` after `Option: CurPlot=HAHA` now answers `op1`, because the computed arm it sits behind matches with `cieqn()` — the same rule `cp_usrset()` dispatches the write by. The key is still filed and still listed by `set`.
- Shared build not executed (no `libngspice` in `build-ver_50`); `pl_env` node still leaked at exit (pre-existing, `doc/codex/issues/0041`).

## Verification

- verdict: **SIGN**

### Suite

`make -C build-ver_50 check` with every cached *.log/*.trs deleted first: 305 PASS, 0 FAIL, exit 0 (the single `^ERROR:` grep hit is `ERROR: (internal) tried to destroy non-existent graph` inside a PASSing plotting deck, pre-existing and unrelated). tests/lint: `identity lint: 265 comparisons, baseline matches` plus `selftest.lint: 11 comparisons, baseline matches` — identity.baseline carries no crew-E edit, and both options.c `eq(vv->va_name, word)` records still match after the scan moved because the baseline stores no line number. tests/regression/pipe targeted: 21/21. Shared build additionally exercised for the first time: `configure --with-ngshared && make -j8` exits 0 and produces libngspice.so.0.0.15; a host program driving ngSpice_Init/ngSpice_Command survives all five poisoned loads and parses a deck identically with and without a poisoned load. Working tree left exactly as found: `git diff --stat` = 24 files, 851 insertions, 102 deletions (identical to session start), 39 tracked porcelain entries, 15 staged files all `.out` and none of them crew E's, untracked set identical to the session-start set (comm empty in both directions after removing the 142 deck-output files my own corpus runs created under examples/ and tests/).

### Criteria met

- 0061-1 loading a raw does not change how the next netlist is parsed — verified with my own hand-built raw + my own mixed-case deck: identical vector spellings (in/midnode/vs#branch) with and without a preceding `load` of Option: casemode=preserve and of casemode=distinguish. RED re-derived by reverting ONLY src/frontend/variable.c to HEAD and rebuilding (mtime moved 16:01→16:09): the same load then yields In/MidNode/Vs#branch.
- 0061-2 stated as a rule about which reads, not which names — the predicate is inp_reading_netlist(), no variable name appears. Independently confirmed on a THIRD switch neither the fix nor either deck names: `Option: no_auto_gnd` makes the next deck report `Warning: singular matrix: check node gnd` on stock and nothing on the new binary. `Option: ngbehavior=hs` likewise, and I proved that check live by stripping checks 2-3 from the crew deck and running it at guard-off: rc=1, 'a loaded Option: ngbehavior=hs changed how the next deck was parsed'.
- 0061-3 backward compatibility decided explicitly (shape A) — pair still filed: `$casemode`=preserve after the load, `$curcasemode`=fold, `set` lists `* casemode preserve`, and `write` round-trips `Option: casemode = preserve`. Round-trip of a mixed-case key is byte-identical to stock (`Option: CurPlot = HAHA` from both binaries).
- 0061-4 the Command: path is untouched — `Command: echo COMMAND-LINE-EXECUTED` still executes on both binaries; `Command: set casemode=preserve` still reconfigures the next read (MidNode + after=preserve). Residual, disclosed in the issue and the receipt.
- 0061-5 a deck asserts it — tests/regression/pipe/rawfile-option-parse-policy.cmd is fail-fast by exit code, every 'nothing moved' check is preceded by a live positive control (c1..c4 read the key back, so a shape-B refusal at the Option: arm fails this deck), and it is RED with the guard removed at the exact check the receipt quotes.
- 0061-6 make check unchanged — `make -C build-ver_50 check` with every cached *.log/*.trs deleted first: 305 passed, 0 failed, exit 0. No .out file moved (git status shows no modified .out). tests/lint: 'identity lint: 265 comparisons, baseline matches' and selftest 11.
- 0065-1 all ten rows rc=0 with the vector list printed — measured on my own raws spliced from my own base header: curplot/curplotname/curplottitle/curplotdate/plots/ngbehavior/casemode/sourcepath/zzz/CurPlot all rc=0, marker printed, `Here are the vectors currently active` followed by i(vs)/v(in). The same ten on /usr/local/bin/ngspice: the five computed names give rc=139, no marker.
- 0065-2 two-plot sequence rc=0 — my own two-plot raw whose FIRST plot carries `Option: curplot=HAHA`: `load` + `setplot op1` + each of `display`, `print v(in)`, `write x.raw` gives rc=0 and the marker on the new binary; 139 on stock for all three.
- 0065-3 valgrind clean — load+display on Option: curplot=HAHA: zero Invalid read, zero Invalid write, zero Invalid free; ERROR SUMMARY 2 errors from 2 contexts, both definite-loss. Both pre-existing and independently confirmed: the 2-byte block appears on a raw with NO Option: line on stock AND new; the 24+N block is reported byte-identically by stock on Option: zzz=HAHA. A 14-command battery (set/display/print/let/setplot new/setplot op1/$curplot/$plots/source/display/destroy/set) on the poisoned plot: ERROR SUMMARY 0 errors from 0 contexts. Stock on the same input: Invalid read of size 8/1, Invalid write, Invalid free, 296 errors from 74 contexts.
- 0065-4 the rule is ownership, not five names — usrvar_push() keys on tbfreed and the reorder keys on 'what this function computes'. I read every path of cp_enqvar(): no path can now return a borrowed node with *tbfreed==1, and the ci_vars tail still sets *tbfreed=0 before the scan.
- 0065-5 survives 0061 shape A — rawfile.c's Option: arm is untouched, the pair is still filed (proved by the write round-trip) and both decks depend on it.
- 0065-7 a deck asserts it — tests/regression/pipe/rawfile-option-computed-name.cmd is RED at HEAD sources: Segmentation fault, rc=139, no marker, dying inside the first load of the loop — exactly the receipt's transcript. GREEN on the shipped tree.
- 0065-8 make check unchanged, no .out moved — same measurement as 0061-6.
- Test-liveness discipline (task 2) — each added test flips when the mechanism it names is disabled and rebuilt. (a) variable.c→HEAD: parse-policy FAILS ('moved the mode the next read latched'), computed-name PASSES, crew D's curcasemode-effect.cmd PASSES — 20/21. (b) cp_enqvar ordering reverted with usrvar_push kept: computed-name FAILS on the value ('a rawfile Option: key answered a read of a computed variable'), no fault at all, $curplot answers HAHA, valgrind clean — 20/21. Each revert was non-destructive and each restore verified by md5 and by git diff --stat matching the original exactly.

### Criteria unmet

- 0065-6 (the other two cp_usrvars() callers inspected and named) is met as an inspection but the write-up is internally inconsistent: the Resolution says cp_remvar() and cp_vprint() are 'both correct with no change of their own', while the same document's 'Left' section records that `unset curplot` aborts inside cp_remvar(). I confirmed the abort: cp_remvar() finds the name in the uv1 chain, US_DONTRECORD leaves *p linked, then frees v and then walks and frees uv1 — a double free within cp_usrvars()' own memory. rc=134 on the new binary AND on stock with no rawfile at all, so it is pre-existing and not caused here, but 'correct' is the wrong word and criterion 6's sentence should be amended before the issue is closed on it.

### Regressions

- None found in the standalone binary. Controlled comparison: I built the HEAD sources in the SAME build dir (all 12 modified src files reverted with `git show HEAD:<path>`, rebuild confirmed by mtime) and ran all 677 .cir decks under examples/ and tests/ from each deck's own directory under both binaries, capturing rc and filtered stderr: 0 rc differences and 0 stderr differences. Sources then restored and verified byte-identical by md5.
- Minor, shared build only, previously unmeasured: a fatal netlist read that longjmps out of inp_readall() leaves inp_netlist_read_depth stuck at 1 for the rest of the host process, so plot_cur->pl_env stops answering cp_getvar() permanently. Measured with a host program against a freshly built libngspice (--with-ngshared, clean build): with `Option: nosort` in the loaded plot, `display` lists v(in) then i(vs) before `ngSpice_Command("source badinc.cir")` (missing .include) and i(vs) then v(in) after it. The shell `reset` command does NOT clear it (com_rset is not totalreset); only the ngSpice_Reset() API does, which I confirmed restores the unsorted order. Mitigating: ngspice prints 'ngspice.dll cannot recover and awaits to be reset or detached' on that path, and the only thing lost is the plot environment answering a C-level read — the very behaviour the change removes. Not a blocker, but 0061's Resolution claims its two reset sites cover 'the paths that leave inp_readall() without returning through it' and that is one path short; the shared-build longjmp at sharedspice.c:2193 is covered only if the host calls ngSpice_Reset().

### Correct work now refused or diagnosed

- `set` after `Option: CurPlot=HAHA` now lists `* CurPlot op1` where stock lists `* CurPlot HAHA`. cp_vprint() prints the name from pl_env but re-resolves the value through vareval()→cp_enqvar(), whose cieqn() curplot arm now precedes the environment scan. 0065's Resolution says the key 'is still filed under its own spelling and still listed by set' — true of the name, not of the value; the shift is disclosed for `$CurPlot` but not for `set`. The pair is genuinely still filed: `write` round-trips `Option: CurPlot = HAHA` byte-identically to stock.
- `Option: sourcepath=( subdir )` followed by a source used to make the reader emit `Error: sourcepath is a read-only variable.` and `cp_vset: Internal Error: it was already there too!!` (add_to_sourcepath at inpcom.c:1660 saw the file's value through cp_getvar and then failed to write it). Both messages are gone on the new binary. No capability lost — the .include was not found in either binary — so this is the fix removing two spurious diagnostics, not breaking correct work.
- `unset curplot` / `unset plots` abort with rc=134 on the new binary. Not caused here: rc=134 identically on stock ngspice-46 with no rawfile involved anywhere. After a poisoned load the new binary gives that same pre-existing 134 where stock gives 139, i.e. the file no longer changes the outcome — exactly as the receipt's residual states. `unset zzz` after a load is rc=0 on both binaries today, also as stated.
- Interrupting a `source` mid-read with SIGINT: my first sweep showed the new binary aborting 3/5 while stock was clean, but repetition at a fixed delay produced rc=134 and rc=139 on stock too. Non-deterministic heap corruption on interrupt, pre-existing on both binaries, not attributable to this change. The guard itself is correctly lowered by ft_sigintr_cleanup(): after a SIGINT during the read, an `Option: nosort` key living only in the loaded plot's environment is still honoured (v(in) before i(vs)).
- The requested stock-vs-new corpus comparison is not a usable control and its 148 rc differences are all environment, not code. /usr/local/bin/ngspice is a different configuration (CIDER 1.b1 + --enable-predictor) and, decisively, build-ver_50/src/spinit loads /usr/local/lib/ngspice/*.cm — stock's code models — so every XSPICE example segfaults under the uninstalled build. Every one of those 148 decks gives the identical rc under the HEAD-built binary, which is why the HEAD-vs-working comparison is the one I report.