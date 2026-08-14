# Stage 1b receipt — issue 0061

## Remediation

All problems worked. Final receipt:

**1. SUBSTANCE / reproduced** — no action needed; checker confirms the finding. Left as is.

**2. CITATION MISATTRIBUTION (`cp_variablesubst` vs `vareval`)** — FIXED. Property 3 now reads: `cp_variablesubst()` (`variable.c:846`) evaluates each name through `vareval()`, called at `:866`; `vareval()` walks `variables` at `:1005` and falls back to `cp_enqvar()` at `:1021`. Verified by `grep -n`: `cp_variablesubst` opens at 846, `wordlist *nwl = vareval(var);` is line 866, `vareval` opens at 908.

**3. GREP EVIDENCE ≠ QUOTED CLAIM** — FIXED. The grep is now pasted with its whole real output (3 lines: `postcoms.c:1128`, `:1157`, `rawfile.c:483`), followed by why `:481` is invisible to that pattern (`vv->va_next = cp_setparse(wl)`), and by the indirect writer the checker found: `cp_remvar()` takes `p = &plot_cur->pl_env` (`variable.c:600`) and unlinks at `*p = v->va_next` (**`:630`**, not `:631` as the checker wrote — verified by `awk` line dump), whose `US_OK` arm is unreachable for a `pl_env` name because `cp_usrset()` returns `US_READONLY` at `options.c:415`-`:417`. The conclusion is now stated as resting on reading those functions, not on the grep.

**4. DATE STAMP DRIFT** — FIXED. Both header pastes regenerated and re-quoted from the run just made: `Date: Thu Aug 13 00:25:44  2026` in `hdr_option.raw` and in the `rt.raw` round trip. Status now says explicitly that `Date:`/`Command:` are wall-clock/build stamps regenerated with the file and will differ on re-run. (The repro dir is being rewritten concurrently by sibling agents running `run_all.sh`, which does `rm -f ./*.raw` — that is what moved the stamp between the write-up and the check.)

**5. REPRO ARTIFACTS ABSENT** — FIXED. Added and `git add`ed `doc/claude/feedback/ngspice_upstream/repro/mix.cir` (title comment first line) and `doc/claude/feedback/ngspice_upstream/repro/hdr_variants.sh`, which rebuilds `ascii_raw.raw` and splices all header variants: `hdr_option.raw`, `hdr_ngb.raw`, `hdr_numdgt.raw`, `hdr_cmd.raw`, `hdr_cmdset.raw`, the four name/value spelling variants, and `hdr_option_early.raw` (misplaced line). `*.raw` cannot be committed — `repro/.gitignore` excludes it and every header carries a clock stamp — so the generator is the committed artifact; the issue says so and names each file at the point it is used, including `hdr_cmdset.raw`'s content (`Command: set casemode=preserve`), which was previously stated nowhere. `run_all.sh` NOT touched: this evidence is 0061's, not one of the nine findings, and the dir is in concurrent use.

**6. OFF-BY-ONE CITATIONS** — FIXED, all three. Impact table now cites `eq()` at `variable.c:705` inside the loop opened at `:704`. Property 2 now says `cp_usrset()` tests the name at `options.c:355` and copies at `:357`/`:359`/`:361`. The `postcoms.c` pair is now described correctly: `:1128` is in `killplot()` (`:1072`), `:1157` is in `destroy_const_plot()` (`:1136`) and only the printf's *text* says killplot.

**7. OVERSTATEMENT in Summary** — FIXED. Now "a `cp_getvar()` consumer … any consumer whose name is not already in the global `variables` list or in `cp_usrvars()`, the two links ahead of the plot environment", with the same qualification added to the matching Impact bullet.

**8. OVERSTATEMENT, 'exactly'** — FIXED. Split into "the set an `Option:` line can *act on*" vs "the set it is *visible* in", the latter naming `cp_enqvar()` (`options.c:60`) for `$name` and `cp_vprint()` (`variable.c:1125`, `:1147`) for `set`'s listing, and pointing at criterion 3 which depends on the difference.

**9. WEAK numdgt EVIDENCE** — FIXED with a better control than the one suggested. Re-measured: `load hdr_numdgt.raw ; print v(in)` → `v(in) = 3.000000e+00`; `load hdr_option.raw ; set numdgt=12 ; print v(in)` → `v(in) = 3.000000000000e+00` (no error — `numdgt` is not in *that* plot's `pl_env`). Added a second new measurement showing the line was parsed and filed, only unable to reach the latch: `load hdr_numdgt.raw ; echo dollar-numdgt=$numdgt` → `dollar-numdgt=12`. `grep -rn 'cp_getvar("numdgt"' src/` empty (rc=1); `cp_numdgt` is written only at `options.c:357/359/361`.

**10. UNDER-QUOTE of the `inpcom.c:1079` comment** — FIXED. Root Cause now enumerates all three writers the comment names (`-D` getopt loop, `.spiceinit`, libngspice `ngSpice_Command("set casemode=...")`) and calls the loaded raw file a *fourth* writer.

**11. SCOPE of the `Command:` section** — no action; checker judged it load-bearing and not stray.

**12. NO CONTRADICTIONS** — no action.

**13. ALL OTHER CITATIONS EXACT / one transcription liberty** — FIXED the liberty: the `Command:` code block now carries the real `rawfile.c:457` comment and marks the `:461`-`:468` cut with an annotated ellipsis instead of a bare `...`.

**BATCH (a) CROSS-REFERENCES** — FIXED. Status now states 0061 owns the mechanism for the batch and that 0060 hands it here (0060 already defers, verified at its criterion 6); Summary cites 0058 for what counts as a netlist read; property 5 cites 0059 for "a bare `write` writes whatever plot is current", which that paste depends on; criterion 3 now warns that keeping the read-back manufactures a fresh instance of 0060's request/effect split. 0048 cross-ref was already present and accurate. No fix in this issue is duplicated by a sibling, so nothing is handed away.

**BATCH (b) CONTRADICTION 0060 vs 0058** — does not touch 0061 (it makes no claim about `.include`/`.lib`), but measured anyway, and **0058 is right**: counting the once-per-`inp_readall()` `unknown casemode` warning on `build-ver_50/src/ngspice -b -n -D casemode=bogus`, a plain deck gives 2, the same deck with `.include sub.inc` gives 2, with `.lib lib.lib rgsec` gives 2, and with a `source inc.cir` inside `.control` gives 3. Shell-level `source` likewise: `printf 'quit\n' | ngspice -p -n -D casemode=bogus` → 2, `printf 'source mix.cir\nquit\n'` → 3. I used that (`source` = a second read) in my Summary, attributed to 0058.

**BATCH (c) UNCOMMITTED DECKS** — see 5; every input the issue quotes is now either committed or regenerable by a committed script named in the issue.

**BATCH (d) SILENT TRANSCRIPT EDITS** — FIXED throughout. Every `printf | ngspice` line is now quoted with the exact filter it ran with and its complete output; where the first two lines of a paste are `load`'s own echo, the prose says so rather than deleting them. The one deliberate partial (`sed -n '1,9p' rt.raw`) is labelled "the first 9 of 15 lines". The header paste is now the whole file (`cat`), not its first seven lines.

**CHANGE NEEDED IN A SIBLING ISSUE (orchestrator to route):** `doc/codex/issues/0060`, acceptance criterion 2 — its claim that a `source` *or* an `.include` from inside a `.control` block is an `inp_readall()` is falsified by the counts above (`.include` and `.lib` stay at 2, `source` goes to 3). Only the `source` half survives; 0058 has it right and 0060 should defer to 0058 for the definition. Nothing else in 0060 needs to move on 0061's account — its criterion 6 already hands the `Option:` mechanism here, and 0061 now acknowledges that ownership.

## Re-check

- verdict: **GOOD** · decks_committed: True · cross_refs_ok: True

### Still unresolved

- Problem 5 (repro artifacts) is only partly closed. `mix.cir` and `hdr_variants.sh` are staged (`git add`ed, not yet in HEAD), but `ascii_raw.cir` -- the seed deck every spliced header is built from, and which the issue's Status paragraph names at line 20 -- is still untracked (`git status` shows `?? doc/claude/feedback/ngspice_upstream/repro/ascii_raw.cir`). From a fresh checkout `hdr_variants.sh` cannot run: `"$NG" -b -n ascii_raw.cir` fails with stderr suppressed, `ascii_raw.raw` is never made, and all eleven awk splices emit empty files. So the remediation receipt's claim that 'every input the issue quotes is now ... regenerable by a committed script named in the issue' is not yet true; one `git add doc/claude/feedback/ngspice_upstream/repro/ascii_raw.cir` closes it. Mitigating: the whole feedback dir is staged piecemeal (FINDINGS.md, README.md, run_all.sh are also untracked), so this is batch-level staging state rather than a claim invented by this issue, and hdr_option.raw is quoted inline in full so the header is reconstructible by hand.

- Cosmetic, not raised in the receipt: the quoted command lines are not runnable verbatim from any single cwd -- the input files are named bare (`hdr_option.raw`, `mix.cir`, cwd = repro/) while the binary is repo-root-relative (`build-ver_50/src/ngspice`). Every output still reproduces once the reader resolves the two paths.

### Regressions introduced

- (none)
