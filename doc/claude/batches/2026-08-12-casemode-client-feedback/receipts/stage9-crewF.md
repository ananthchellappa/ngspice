# Stage 9 receipt — crew F

## Implementation

All four items are done. Final state verified: suite clean, nothing staged beyond what the batch already had.

**RECEIPT**

**Item 1 — the code defect: FIXED.**
Root cause is not where 0061 assumed. `inp_readall()` never decrements on a fatal read, and in the shared build `controlled_exit()` → `shared_exit()` (`ATTRIBUTE_NORETURN`) discards the stack. There are **three** routes out, not two: `ngSpice_Command` → `longjmp(errbufc)`, `ngSpice_Circ` → `longjmp(errbufm)`, and `bg_source` → `pthread_exit()`, which lands at **no** setjmp. So the fix is one line at `src/sharedspice.c:2192`, the last point common to all three, rather than at the two landing sites.

- RED (fix absent, library rebuilt): all three modes `rc=1`, `before=1 after=0 GUARD-STUCK` — `display` lists `v(in), i(vs)` before the bad deck and `i(vs), v(in)` after.
- GREEN: all three `rc=0`, `before=1 after=1 GUARD-OK`.
- Independence proved by breaking each assertion separately: fix removed → `HELD=1 after=0`; guard removed (`variable.c`) → `HELD=0 after=1`. Neither half passes on the other's mechanism.

**No `make check` test is possible** — nothing under `tests/` links `libngspice` and `build-ver_50` is not a shared build, so `shared_exit()` is never executed by the suite. Pinned instead by a self-checking probe — which, as the Problems section below notes, is *not* committed: `doc/claude/feedback/` is untracked in its entirety. Corrected in 0066 and in the probe's own header; where such a probe should live is left open under 0066's **Left**.

**Item 2 — 0065 contradiction: FIXED, one issue filed for both.** Criterion 6 now says `cp_vprint()` is correct and `cp_remvar()` is not. They are one defect: the unconditional free at `variable.c:671`-`:672` of a node that the `US_DONTRECORD` and `US_READONLY` arms deliberately left linked — differing only in which list still points at it (the `cp_usrvars()` chain, freed two lines later → deterministic double free, `rc=134`; or `pl_env`, never freed → dangling pointer, `rc=0` today but 34/65 invalid accesses under valgrind).

> **Correction, 2026-08-13.** The parenthesis above is wrong on its second half and the Problems section below is right. `pl_env` → dangling pointer is **not** `rc=0`: the `unset` returns, and the next command that walks the plot — `set`, `display`, `write` — segfaults, `rc=139`, 3/3, on `build-ver_50/src/ngspice` and on stock `ngspice-46` alike. The `rc=0` and the 34/65 counts were measured under `valgrind`, whose allocator keeps the freed block readable; that is the tool's exit status, not the program's. `doc/codex/issues/0065`'s `rc=139` was correct and has been restored. A third arm, `US_SIMVAR`, was also found: `source` a deck, `set temp=27`, `unset temp` → `rc=134`, and it is an invalid *write* into a block the arm itself freed. `doc/codex/issues/0067` carries all of it.

**Item 3 — 0060 asymmetry: SENTENCE, and the warning is shown to misfire.** `raw_write()` re-emits the pair, so `load`→`write`→`load` round-trips `Option: curcasemode = distinguish` from a file ngspice wrote itself; a load-time warning fires on that lap. Measured, both laps.

**Item 4 — one confirmed, one falsified.** Built the batch's own base commit (all twelve modified `src/` files reverted, in an isolated copy) and validated it against two known markers before trusting it.
- `* CurPlot op1` — **confirmed**, BASE/STOCK give `HAHA`. Cause is 0065's `cp_enqvar()` reordering, not 0061's guard (verified with the guard removed).
- `sourcepath` — **falsified twice over.** BASE is silent on the same input, so it is not this batch's; and it has not gone away — `source subdir/inner.cir` prints both lines on BASE, NEW **and** STOCK. The original observation sourced a deck from the cwd, where `.` is already on `sourcepath`, so `add_to_sourcepath()` returns before calling `cp_vset()`.

**Files changed**
- `src/sharedspice.c` — the fix in `shared_exit()`; and `totalreset()`'s comment corrected, since it no longer describes the path that needed it.
- `src/frontend/inpcom.c` — comment only: `inp_netlist_read_reset()` now enumerates three callers, not two.
- `doc/codex/issues/0066-a-fatal-netlist-read-strands-the-reader-guard.md` — new, closed.
- `doc/codex/issues/0067-cp-remvar-frees-a-node-it-chose-not-to-unlink.md` — new, open.
- `doc/codex/issues/0065…md` — criterion 6 corrected; both Left bullets point at 0067.
- `doc/codex/issues/0060…md` — Left gains the asymmetry with the misfire demonstration.
- `doc/codex/issues/0061…md` — the compile-checked-only bullet superseded; the missed third route recorded; the two behaviour changes recorded, one confirmed and one falsified.
- `doc/claude/feedback/ngspice_upstream/repro/shared/netlist_guard_probe.c` — new probe, self-contained (writes its own four inputs), self-checking, three modes.

**Suite:** 305 passed, 0 failed, caches cleared first — `PASS:` list byte-identical to the pre-edit run. Identity lint 265 comparisons, baseline matches. Staged nothing (no deck added, so no `.out`); no commit.

**Residuals**
- `shared_exit()` calls `bgtr()` unguarded at `sharedspice.c:2198` while guarding `ngexit()` at `:2200`, so a host passing `NULL` for `BGThreadRunning` segfaults inside the error path. Pre-existing, unrelated, worked around in the probe in one line; worth its own issue.
- The depth count is a plain `static int` and `bg_` puts a read on a second thread; not reachable today, and a property of 0061's design rather than of this fix.
- `Option: sourcepath=…` still makes the reader's own `add_to_sourcepath()` fail with `cp_vset: Internal Error` on every binary tested — a live defect on the write side of the same `US_READONLY` arm 0067 owns from the `unset` side; belongs to the `Option:` arm's own issue.

## Verification

- verdict: **NEEDS_WORK**
- depth_leak_fixed: True · counter_correct_all_cases: True · e_fix_still_holds: True

### Suite

Two full runs of `make -C build-ver_50 check`, each from cleared *.log/*.trs: 305 PASS, 0 FAIL, 0 SKIP, 0 XFAIL, exit 0 both times. Run 1 was on the tree as received; run 2 after I restored the two files I temporarily broke and rebuilt. The two PASS lists are byte-identical (diff clean). tests/lint: `identity lint: 265 comparisons, baseline matches` (PASS identity.lint) and `identity lint: 11 comparisons, baseline matches` (PASS selftest.lint). The single `^ERROR:` string in the log is deck stdout ('(internal) tried to destroy non-existent graph', from vector-let-report.cir), not an automake result line. Staged: 15 files, all .out, 0 non-.out. 12 modified files under src/, unchanged from the batch's set; src/sharedspice.c and src/frontend/variable.c md5-verified identical to their pre-verification state after my break-it experiments, and both build trees rebuilt clean.

### Doc items checked

- 0066 root cause and placement: verified. src/sharedspice.c:2192 is the last point common to all three routes out; sharedspice.c is in libngspice_la_SOURCES only (src/Makefile.am:499-505) and in EXTRA_DIST for the binary, so `make check` genuinely cannot reach it. No bare exit() in inpcom.c/inp.c/subckt.c bypasses controlled_exit().
- 0067 mechanism and line numbers: exact. cp_remvar() at :577, four list walks at :585/:592/:600/:608, v=*p at :615, US_OK unlink at :627-:632, US_DONTRECORD at :634, US_READONLY at :641, unconditional tail at :671-:672, chain free at :674.
- 0067 US_DONTRECORD symptom: reproduced. `unset curplot|curplotname|curplottitle|curplotdate|plots` each rc=134 on build-ver_50; `curplot` prints `Internal Error: var 99`, `plots` prints `Error: plots is read-only.` then `var 112` (i.e. the first character's ordinal, as documented).
- 0067 valgrind counts: exact. `load hdr_zzz.raw; unset zzz; set; quit 0` under valgrind -q gives 34 invalid reads on build-ver_50 and 65 on /usr/local/bin/ngspice (ngspice-46), matching the doc's 34 (NEW) / 65 (STOCK).
- 0065 criterion 6 correction: present and accurate. It now reads `cp_vprint() is correct. cp_remvar() is not`, and both Left bullets (lines 461, 466) point at 0067 as one issue.
- 0060 Left asymmetry: reproduced verbatim. `Option: curcasemode = distinguish` spliced after Plotname: gives `read=fold` and `* curcasemode\tfold`; load->write->load re-emits `Option: curcasemode = distinguish` on both laps, rc=0 each. The claim that a load-time warning would misfire on a file ngspice wrote itself is sound.
- 0061 behaviour change 1 (`* CurPlot op1`): confirmed. NEW gives `* CurPlot op1` and `$CurPlot=op1`; STOCK ngspice-46 gives `HAHA` for both. The causal sub-claim also holds: I removed `!inp_reading_netlist()` from cp_getvar and rebuilt, and `set` still lists `op1`, so it is not 0061's guard.
- 0061 behaviour change 2 (sourcepath): falsification confirmed on the two binaries I have. NEW prints 0 of the two error lines for a cwd deck and 2 for `source subdir/inner.cir`; STOCK prints 2 in both. The stated cause is directly verified: `$sourcepath` on STOCK carries /usr/local/share/ngspice/scripts twice, on NEW once, so the dedup in add_to_sourcepath() misses on STOCK and hits on NEW. The difference is between installations, not builds.
- Residual (unguarded bgtr()): real. sharedspice.c calls bgtr() through an unchecked pointer at :2198 while guarding ngexit() at :2200; my own probe had to supply a BGThreadRunning callback for the bg mode to survive.

### Problems

- FALSIFIED SEVERITY CLAIM in doc/codex/issues/0067 (new, open, shipped by this crew), and the receipt repeats it. 0067's Impact says the US_READONLY arm is 'Latent, allocator dependent, and today usually silent', that 'it does not crash today', and that "doc/codex/issues/0065's Impact recorded it as rc=139; that did not reproduce on re-measurement, then or now." Measured by me, deterministic 3/3, on build-ver_50 AND on stock /usr/local/bin/ngspice: `printf 'load hdr_zzz.raw\nunset zzz\nset\nquit 0\n' | ngspice -p -n` -> Segmentation fault, rc=139. Same for `display` and for `write w.raw` in place of `set`. rc=0 holds only for the two-command sequence with nothing after the unset, which is the table row; the prose generalises from it and overrides a correct earlier measurement. 0065's rc=139 was right. Crew F evidently only ran the `set` variant under valgrind, whose allocator masks the fault (valgrind rc=0, 34 invalid reads) -- the doc's own quoted command line is the crashing one.
- 0067 contradicts itself: its acceptance criterion 2 asks that `set`, `display` and `write` after the unset be clean under valgrind -- i.e. it names the exact three commands that segfault -- while its Impact says the symptom does not crash. A maintainer triaging 0067 would rank the US_READONLY arm as latent/low priority when it is an immediate deterministic SIGSEGV on the ordinary next command (and, in the shared build, takes the host process down).
- The receipt's item-2 sentence 'or pl_env, never freed -> dangling pointer, rc=0 today but 34/65 invalid accesses under valgrind' carries the same error into the handoff.
- Not independently re-verified: the BASE column of 0061's and 0067's three-binary tables. I did not rebuild the twelve-file-reverted scratch tree. The STOCK measurements plus the directly-confirmed sourcepath-duplication explanation independently support both attributions, so I have no reason to doubt BASE, but it is unchecked.
- Minor: 0066 and the probe header describe netlist_guard_probe.c as 'committed with this issue'. Nothing in this batch is committed and doc/claude/feedback/ is entirely untracked. Consistent with the batch's convention, but inaccurate as written.
- Minor: the comment above inp_readall() justifies a count rather than a flag with 'inp_readall() has three callers and one of them, the XSPICE auto-bridge, reads a deck ngspice writes itself'. That caller is not a nesting path -- Evtcheck_nodes() runs from spiceif.c:185 under inp_dodeck() (inp.c:1095), long after inp_readall() at inp.c:532 has returned. The comment does say 'There is no nesting today' first, so it is not false, but the named caller is not evidence for the count. I confirmed depth is only ever 0 or 1 in practice.