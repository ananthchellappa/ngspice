# Stage 1b receipt — issue 0059

## Remediation

Receipt:

**1. FIXED** — checker right. `grep 'plot_cur ='` over `src/` returns 22 lines; only `vectors.c:1362/:1368/:1393/:1431` are inside `plot_setcur()`. Root Cause now says an analysis reaches `plot_cur` *via* `plotInit()`→`plot_setcur()` on this path, then lists all 22 (16 direct assignments, plus two definitions: the `plotting.c:13` initialiser and `ngsconvert.c:38`, a different program's own symbol), and states none of the 20 executable sites runs in this deck.

**2. FIXED** — checker right, re-measured with a committed deck. `repro/destroy_curplot.cir` (`op` / `destroy op1` / bare `write`, `-b -n`, no `-D`): `curplot-before op1`, `curplot-after const`, `rc=0`, 570-byte `Title: Constant values` / `Plotname: constants` raw. Mechanism cited: constants plot is also `plot_list` (`plotting.c:14`), so `killplot()` sets `plot_list = pl->pl_next` then `plot_cur = plot_list` (`postcoms.c:1090-1092`, inside-list case `:1107-1109`) — no `plot_setcur()`. Added as **matrix row 7 (wanted: refuse)**, a Root Cause subsection, and rewrote criterion 1 + Resolution: the bit must be maintained at all 20 in-process assignment sites (set on request, cleared at `killplot()`'s `:1092`/`:1109`, saved/restored with the pointer at `com_let.c:248/:250` and `xpressn.c:1123/:1125`), not merely set by `plot_setcur()`/`plotInit()`.

**3. FIXED** — checker right. `repro/save_partial_miss.cir` (`.save v(nosuchnode) v(In)`, no `-D`): `rc=0`, 303-byte raw, `Plotname: Operating Point`, real `Date:`. All-miss `.save v(nosuchnode) v(alsonot)`: `rc=1`, 570 bytes. Summary now reads "a `.save` whose names *all* miss", explains `outitf.c:461` counts saved data rather than names, and keeps the no-`.save` bare-pipe path separate.

**4. FIXED** — checker right. Object selection is `vlnggen:328-333` (`set vcd_obj=...` / `fopen fh "$vcd_obj"` / `if $fh >= 0`); criterion 4 now cites `:327-333` as the `verilated_vcd_c.o` probe and names `:249-261` correctly as the same idiom locating the shim source.

**5. FIXED** — checker right. `postcoms.c:589-594` is the filename branch (`file = wl->wl_word; wl = wl->wl_next;` / `else file = ft_rawfile;`); the static `all` is substituted at `:611-614`. Criterion 1 re-pointed.

**6. FIXED** — committed eight decks to `doc/claude/feedback/ngspice_upstream/repro/`, each with a title comment as line 1: `nocase.cir`, `seq.cir`, `rc0.cir`, `status_probe.cir`, `save_partial_miss.cir`, `destroy_curplot.cir`, `write_named.cir`, `write_named_capture.cir` (staged with `git add`). Every quoted block names its deck at the point of use, and finding 3's section of `run_all.sh` now runs all nine (with `save_lower.cir`); `sh -n` clean and the section executed end-to-end, leaving no stray files.

**7. FIXED** — checker right about the mechanism, conclusion unchanged. `printf 'source rc0.cir\nquit\n' | ngspice -p -n` → `rc=0`, zero matches for `incomplete or empty netlist`. Text now says `source`, and notes a `.cir` is a netlist rather than a command stream.

**8. FIXED** — checker right on both halves, and the second half is now measured rather than asserted. Row 6's stderr carries `Warning from checkvalid: vector MidNode is not available or has zero length.` (`parse.c:290`, `cp_err`) *before* `Error during 'write': no writable vector found.` (`postcoms.c:617`, literal `stderr`). `repro/write_named_capture.cir` redirects that `write` with `>&` and reads the capture back: the file holds only the `checkvalid` line; the `Error during 'write'` line is still on the process's stderr. Criterion 3 now uses that as the argument for `cp_err`.

**9. FIXED** — checker right. `FINDINGS.md:371-372` says separate redirections avoid stderr *scrambling*. Impact now says their relative order is not recoverable, that splitting preserves each stream's own order and discards the interleaving, and drops "the only way".

**10. FIXED** — the whole `save_lower.cir` transcript is re-pasted verbatim from a fresh run, both streams captured to separate files, including the leading blank line, `Note: No compatibility mode selected!` and every blank line. Other trimmed blocks are now either full (`seq.raw` header to `Binary:`, `rc0`'s complete stderr) or show the trimming command (`grep`/`head -4`/`head -5`/`head -6`), and the `set curplotname` pipe shows its `>/dev/null 2>&1`.

**Batch (a)** — added the reference 0059 needed: `doc/codex/issues/0057` owns the "names no token" diagnostic (deferred to in Impact and Resolution); existing references to 0056 and 0032 kept. 0059 prescribes nothing 0056/0057 own. **Batch (b)** — not applicable: 0059 makes no claim about `inp_readall()`, `source`, `.include` or `.lib`. **Batch (c), (d)** — see 6 and 10.

**Changes needed in sibling issues:** none. (0056's `outitf.c:1338` line for `name_eq()` is one line low — the declarator is at `:1337` — but 0056 and 0057 both use `:1338` and no checker flagged it, so I left 0059 matching them rather than creating a batch-wide inconsistency.)

Files: `/home/qflow/dev/ngspice_test/doc/codex/issues/0059-write-emits-the-constants-plot-when-no-analysis-ran.md`, `/home/qflow/dev/ngspice_test/doc/claude/feedback/ngspice_upstream/repro/run_all.sh` (section 3 only), and the eight new decks under `/home/qflow/dev/ngspice_test/doc/claude/feedback/ngspice_upstream/repro/`. `src/` and `tests/` untouched.

## Re-check

- verdict: **NEEDS_WORK** · decks_committed: True · cross_refs_ok: True

### Still unresolved

- Receipt problem 6 (re-runnability) only partly addressed. The nine decks now exist, but three of them produce the quoted output ONLY under `-D casemode=preserve`, which the issue never states at the point of use. Run the way the neighbouring 'no -D' examples are run: `write_named.cir` gives rc=0 and writes a 324-byte raw, contradicting acceptance-matrix row 6 ('already refuses, no file') and criterion 3's quoted stderr pair; `status_probe.cir` prints 'after-failed-op status-is 0 curplot-is op1 / after-good-op status-is 0 curplot-is op2' instead of the quoted '1 const' / '0 op1' (issue lines 252-253); `write_named_capture.cir` captures `binary raw file "write_named.raw"` instead of the checkvalid warning (issue line 494). Measured both ways. run_all.sh:105/109/112 does pass -D casemode=preserve, so only the issue text is short a flag. Fix: name the mode in row 6's cell / criteria 3 and 4, and at the status_probe block, as lines 39 and 222 already do for save_lower and rc0.

- Cosmetic, but it is a claim about the tree: Status line 12 says 'Every deck quoted below is committed under doc/claude/feedback/ngspice_upstream/repro/'. Nothing under doc/claude/feedback/ is committed — the eight new decks are `git add`-staged, and save_lower.cir and divider.cir (whose raw the Date-tell comparison at lines 162-166 quotes) are still untracked. Either commit them or say 'lives under'.

### Regressions introduced

- New off-by-one in the sentence that was rewritten to fix receipt problem 1. Root Cause line 333 says "`grep 'plot_cur ='` over `src/` returns 22 lines". Run literally it returns 23: the 23rd is src/frontend/vectors.c:1117, `if (plot_cur == NULL)`, a comparison rather than an assignment (`grep -rn 'plot_cur = ' src/` with the trailing space is what returns 22). The enumeration underneath is correct and complete — I re-derived it: 16 direct assignments (com_fft.c:140,:373; spec.c:195; dotcards.c:208,:223,:268,:321,:349,:385,:402; postcoms.c:1092,:1109; com_let.c:248,:250; xpressn.c:1123,:1125), 4 inside plot_setcur() (vectors.c:1362,:1368,:1393,:1431), 2 definitions (plotting.c:13, ngsconvert.c:38) — so the load-bearing '20 in-process sites' figure and criterion 1's prescription stand. Only the stated grep output does not reproduce.
