# Stage 6 receipt — crew C5

## Implementation

# RECEIPT — crew C5

## ROUTE: **B — the guard is withdrawn.** `doc/codex/issues/0059` is Open.

**Why B, and why not A.** A was investigated to the point of proof before any code moved, and it does not close. A correct predicate needs three clauses at once, and one of them has no home:

1. *Subject* — "the plot holds nothing this session put there". Buildable (the twelve built-ins come from `ft_cpinit()`'s `predefs[]` loop via `com_let()` and carry no flag, so it needs one), but **not sufficient alone**: `setplot const` + `write` legitimately asks for exactly those twelve, so the provenance test is still needed beside it.
2. *Empty plot* — still needs its own arm (row 8: `setplot new` + `write` reaches the constants plot through `vec_get()`'s fallback).
3. *"Is this a request for the whole current plot?"* — **this one is unreachable at either layer, and that is the finding.**
   - *Before resolution* the wordlist is unparsed: `get_all_type()` (`vectors.c:105-153`) folds five tokens, `ft_getpnames_quotes()` strips double quotes **later**, `$`-expansion already happened, and a repeated wildcard is still a wildcard. Matching text here means re-implementing the resolver — which is exactly the loop that produced three rounds of missed spellings.
   - *After resolution the information is destroyed.* `write f.raw all` reached via the constants fallback and `write f.raw const.all` (row 4, which **must** write) produce the identical dvec list from the identical plot. Nothing in the result separates them. And when the plot is empty — row 8, the row that most needs covering — "covers the whole current plot" is vacuously true of *every* request, so `write f.raw pi boltz` from a `setplot new` plot would be refused. Over-refusal again, one layer down.

So A cannot come out clean in one pass; per the instruction, B. The trade also decides itself: row 1's defect writes a file that labels itself (`Title: Constant values`, `Plotname: constants`) — that is how every measurement in the issue identifies it — while the guard wrote **no file at all** for correct work, including through `ngspice -p`.

`plot_cur_chosen` went out with it: with no reader it was a global written at 14 sites and read at none, and it is state a shared build would have to reset. Its design is preserved in 0059's Resolution instead of in the tree.

## Files changed

| file | what |
|---|---|
| `/home/qflow/dev/ngspice_test/src/frontend/postcoms.c` | both refusals + `whole_current_plot` removed from `com_write()`; the `plot_cur_chosen` clear removed from `killplot()`. **Byte-identical to HEAD** (`git diff` empty) |
| `/home/qflow/dev/ngspice_test/src/frontend/plotting/plotting.c`, `vectors.c`, `dotcards.c`, `spec.c`, `com_fft.c` | `plot_cur_chosen` definition + all 14 assignments removed. **All byte-identical to HEAD** |
| `/home/qflow/dev/ngspice_test/src/include/ngspice/fteext.h` | surgical: only the `plot_cur_chosen` extern removed. Crew B's `inp_case_announce_reset()` hunk untouched (file now `+4`) |
| `/home/qflow/dev/ngspice_test/NEWS` | the `Bug fixes:` entry removed — nothing user-visible changed. **Byte-identical to HEAD** |
| `/home/qflow/dev/ngspice_test/tests/regression/misc/Makefile.am` | four decks out of `TESTS`, `write-let-session.cir` in; `wup_*`/`drc_*`/`wep_*`/`wsa_*` out of `CLEANFILES`, `wls_*.raw` in; four `EXTRA_DIST` comment paragraphs replaced by one. `./autogen.sh` re-run (rc=0) |
| `doc/codex/issues/0059-…md` | Status → **Open**; matrix gains **row 10** (the `let` session, legitimate, must not move) and rows 8/9 lose their deleted-deck citations; row 9 rewritten as a class with the four bypasses recorded; **Resolution rewritten** — three attempts, how each was falsified, the four constraints, the two layers where clause 3 fails |
| `doc/claude/decisions/0017-…md` | Status → **WITHDRAWN**; decision 1 explicitly survives (it is a decision *not* to change `write`); the migration-note section's "It is in `NEWS`" corrected to "it was, until the withdrawal" |
| `doc/codex/issues/0063-…md` | **criterion 4 discharged** — the deck that asserted the internal error *is* printed is gone, so 0063 is unblocked; Impact and Resolution amended off `plot_cur_chosen` |
| `doc/codex/issues/0057-…md` | its "none of its three parts survives at HEAD" corrected to two of three; the raw file is written again |
| `doc/claude/feedback/ngspice_upstream/FINDINGS.md` | head note, finding 3's box, and the asks table: finding 3 / ask 3 are **not done** |
| `…/repro/run_all.sh` | banner + finding 3 section rewritten to `(OPEN)`, measured on both binaries, with the `let` shape added as the shape that ended the attempt |
| `doc/claude/batches/…/CLOSEOUT.md` | marked amendment on the 0059 subsection and on the C4 commit in the split (it listed four decks that are now deletions and a `NEWS` hunk that no longer exists — the commit plan would not have applied) |

## Tests

**Added** — `/home/qflow/dev/ngspice_test/tests/regression/misc/write-let-session.cir` + `.out`. Pins row 10: `let wls_x = vector(5)` / `let wls_y = wls_x*2`, then bare `write`, spelled `write … all`, and the named form. The bare file is written ASCII and **read back inside the deck** so the assertion is that it holds `wls_x` and `wls_y`, not merely that a file appeared. Mode independent — measured identical under no `-D`, `fold`, `preserve`, `distinguish`.

**Deleted** (each pinned withdrawn behaviour; stated as required):
`write-unchosen-plot.cir`, `write-empty-plot.cir`, `write-spelled-all.cir`, `destroy-removed-circuit.cir`, with their `.out` files. Deleting the last one is also what discharges 0063's criterion 4. Their 13 orphaned run byproducts (`wup_*`, `drc_*`) were removed from `tests/regression/misc/`, since no `CLEANFILES` glob covers them any more.

## RED

New deck against the delivered (guarded) binary, before any source change:
```
WRITE-LET-BARE-ABSENT / -MISSING-X / -MISSING-Y / WRITE-LET-ALL-ABSENT
stderr: Error: no plot has been selected, "wls_bare.raw" not written.  (×2)
only wls_named.raw on disk
```
Stock `/usr/local/bin/ngspice` on the same deck: all five assertions `PRESENT`/`HOLDS`, three files.

**All three saved attack decks, delivered binary** (`/tmp/claude-1000/…/scratchpad/atk/`):
- `letonly.cir` `--batch` — refusal, **no file**; stock writes 3275 B, 14 variables, `x`/`y` at 12/13.
- `p.cmd` `ngspice -p` — refusal, **no file**; stock 3275 B.
- `p2.cmd` `ngspice -p` — `pt2.raw` (the `all` form) refused; `pt3.raw` (named) written.

## GREEN

Same three decks, withdrawn binary (`build-ver_50/src/ngspice`, 10:43:36; refusal string absent from the binary, `plot_cur_chosen` absent from `nm`):

| deck | refusal lines | file |
|---|---|---|
| `letonly.cir --batch` | 0 | `letonly.raw` 3276 B |
| `p.cmd` via `-p` | 0 | `pipe_test.raw` 3276 B |
| `p2.cmd` via `-p` | 0 | `pt2.raw` 3276 B, `pt3.raw` 602 B |

`letonly.raw` diffed against stock's: identical except the `Date:`/`Command:` stamps and `FALSE`/`TRUE` vs `false`/`true` — the latter is committed `ver_50` case work, not this change (every file that could affect naming is byte-identical to HEAD).

The six spellings that walked past the guard now behave uniformly, nothing run: `all` 724 B, `ALL` 724, `ally` 724, `ALLY` 724, `"all"` 724, `all all` 1254 — zero refusals, i.e. HEAD behaviour, honestly recorded as the open defect rather than half-blocked.

`run_all.sh` re-run end to end: rc=0, 323 lines, and every caption in the rewritten finding-3 section agrees with the output printed under it (verified line by line, including the new `letonly` block showing `No. Variables: 14` and rows 12/13).

## Suite

`make -C build-ver_50 -j8` rc=0, build confirmed current (0 compile/link actions on re-run). Every cached `*.log` (except `config.log`) and `*.trs` deleted first — 0 of each remaining, verified.

**`make -C build-ver_50 check` → rc=0, 300 PASS, 0 FAIL, 0 XFAIL, 0 XPASS, 0 SKIP.** (303 → 300 = −4 deleted decks + 1 new; `PASS: write-let-session.cir`.) Targeted `make -C build-ver_50/tests/regression/misc check` → 34/34.

`identity lint: 265 comparisons, baseline matches` plus the 11-comparison selftest; `tests/lint/identity.baseline` untouched by me. The single `^ERROR:` line in the whole log is `ERROR: (internal)  tried to destroy non-existent graph`, immediately followed by `PASS: vector-probe-report.cir` — pre-existing, predates this batch.

**Nothing staged** (`git diff --cached` empty). **No other crew's hunks touched**: the final diffstat differs from the session-start snapshot by exactly the insertions/deletions of the withdrawn work (714→546 insertions = 168, accounted for line-for-line by NEWS 13 + plotting.c 18 + postcoms.c 84 + dotcards.c 26 + vectors.c 7 + spec.c 1 + com_fft.c 2 + fteext.h 5 + Makefile.am 12); crew A4's `outitf.c` (+295), `runcoms.c`, `outitf.h`, and crew B's `inpcom.c`, `sharedspice.c`, `evtplot.c`, `evtproto.h` are unchanged.

## The three items to settle — settled

- **0059 matrix row 9** no longer freezes `all`. It reads "every spelling" and names the four bypasses measured (`ally`, `ALLY`, `"all"` double-quoted — noting single quotes *were* stripped and refused, so the guard was inconsistent between quoting forms — and `all all` at 1254 B), with `get_all_type()`'s five tokens and `findvec_ally()` cited. It is stated as a class any fix must satisfy, not an enumeration.
- **NEWS boundary clauses** are gone with the whole entry.
- **The falsified code comment** at `postcoms.c:596-614` is gone with the guard. Its false claim ("the token is matched the way the resolver matches it") is recorded in 0059's Resolution as the specific way attempt 3 failed.

## Residuals

1. **`LEDGER.md` not updated** — the orchestrator's file. Its lines 83 and 173 still describe `destroy-removed-circuit.cir` and the `plot_cur_chosen` clear placement. `CLOSEOUT.md` **is** updated (marked amendments at three sites) because it carries the commit split, which would not have applied against the tree as it now stands.
2. **The defect is unfixed and 0059 is Open.** Rows 1, 7, 8 and 9 all reproduce. That is the deliberate outcome, not an omission; the Resolution says what the next attempt needs, and points at `0064` (the four wildcard spellings, the single-vector rename) as the thing to settle first, since the workable shape is to make the resolver report what it expanded rather than have `com_write()` guess ahead of it.
3. **0063 is unblocked but still unfixed.** Its criterion 4 is discharged; whoever takes it now needs a fresh RED deck for criteria 1-3, and with the internal error no longer pinned the available discriminator is `$curplot`/`display` disagreeing.
4. **`doc/claude/feedback/…/repro/letonly.raw`** is a new byproduct filename in that untracked repro directory; `run_all.sh`'s own `rm -f ./*.raw` at its head clears it on the next run, like every other `.raw` there.
5. **Attempt 1's real merit is now only in the docs.** `plot_cur_chosen` correctly distinguished "chosen" from "fallen back to" and two rounds of verification confirmed it; it is recorded in 0059's Resolution and 0017 so a future implementer can rebuild it, but it is not in the tree, because a global with no reader is not independently sound.

## Verification

- verdict: **NEEDS_WORK** · route: B
- attack decks pass: True

### Suite

Real numbers, taken from cleared caches. Deleted every *.log except config.log and every *.trs under build-ver_50 and verified 0 of each remained. `make -j8` rebuilt with 0 compile/link actions (binary current with the tree). `make -C build-ver_50 check` -> rc=0, PASS=300, FAIL=0, XFAIL=0, XPASS=0, SKIP=0, including `PASS: write-let-session.cir`. Targeted `make -C build-ver_50/tests/regression/misc check` -> "All 34 tests passed". identity lint: 265 comparisons baseline matches, plus the 11-comparison selftest. Exactly one `^ERROR:` line in the whole log - `ERROR: (internal)  tried to destroy non-existent graph`, immediately before `PASS: vector-probe-report.cir` in tests/regression/casedist; it originates in src/frontend/plotting/graphdb.c:309, which is untouched in the working tree, and belongs to another crew's deck. After the temporary-guard experiment I restored postcoms.c with `git checkout` (numstat empty), rebuilt, re-ran the misc dir green 34/34, and re-ran the full suite with no failures. STDERR SWEEP: 288 example decks and 67 tests decks run under both binaries with stderr compared. No line is attributable to C5 - expected, since C5's source diff against HEAD is empty. Of the 138 differing example decks, 55 are CIDER (this build has CIDER off, the installed binary has it on) and most of the rest were the build-tree binary SEGFAULTING - traced to build-ver_50/src/spinit hard-coding `codemodel /usr/local/lib/ngspice/*.cm`, so the new binary loads the Aug-2 installed code models and crashes in cm_adc_bridge. Re-running the 83 non-CIDER decks with the freshly built .cm files cut the differences to 17, all build-configuration or missing-tool noise (no soundfile support, pyplot/matplotlib, gnuplot/gv/gtkwave absent, newer .cm accepting noise_voltage, a UTF-8 parse difference from committed ver_50 work). examples/plot/combplot.cir cores on STOCK and not on the delivered binary.

### Fourth route

- `set plainwrite` - com_write() has a SECOND branch (postcoms.c:633-645) that skips ft_getpnames_quotes() entirely and calls vec_get() on the same static {"all"} list. Measured from the row-1 state (.save v(nosuchnode), op not run, $curplot=const): writes the 724-byte ASCII constants raw. The string `plainwrite` appears nowhere in 0059 and nowhere in tests/. 0059 constraint 3 names ft_getpnames_quotes()/vec_get() but never names this branch, so a guard placed where all three attempts sat is bypassed by one `set plainwrite`.
- `set appendwrite` - the constants plot is APPENDED to an existing rawfile instead of replacing one. Measured: acc.raw holds `Plotname: Operating Point` (3 vars) followed by `Plotname: constants` (12 vars). This falsifies the sentence the whole withdrawal decision rests on - 0059 Resolution's "Row 1's defect produces a file that at least labels itself - Title: Constant values, Plotname: constants" - because the file's Title and FIRST Plotname are the real run's, so a consumer reading an accumulating dataset sees real data with twelve constants silently appended. `appendwrite` appears nowhere in 0059 and nowhere in tests/.
- `wrdata f.dat all` - a different command reaching the same vec_get() fallback. Measured from the row-1 state: 401 bytes of the twelve physical constants with NO header at all - no Title, no Plotname, nothing self-labelling, so the trade argument fails outright here. PROVED UNGUARDABLE FROM com_write(): with a guard of exactly the withdrawn shape compiled in, `write g_write.raw` and `write g_brace.raw {all}` were refused while `wrdata g_wrdata.dat all` wrote the constants anyway. `wrdata` appears nowhere in 0059; the two decks in tests/ that use it (misc/gnd-command-text.cir, pipe/options-keyword-case.cmd) name vectors explicitly and never reach the fallback.
- Minor, inside row 9's class: `{all}` also reaches the constants file (724 B); it is brace-stripped to the token `all` before com_write() sees it, so the class statement covers it. Dead ends checked and reportable as non-routes: `alle`/`allv`/`alli` fail with "no writable vector found" (the constants plot has no v/i/e vectors); `$`-expanded `all` arrives as the bare token; `unlet` cannot empty a plot because the scale vector is protected (src/frontend/postcoms.c:36-55), so row 8 has no unlet variant; `remcirc` leaves the plots in plot_list so its bare `write` is legitimate; `removecirc` does strand plot_cur outside plot_list (0063 reproduces on both binaries) but the bare `write` before `destroy` still writes the deck's own data.

### Problems

- BLOCKING FOR COMMIT: tests/.gitignore line 3 is `*.out`, so tests/regression/misc/write-let-session.out is gitignored and untracked (`git ls-files --error-unmatch` fails on it). The receipt says the test was "Added ... + .out", but in git terms the reference file was not added at all. A default `git add` of the batch commits write-let-session.cir into TESTS with no reference .out, and `make check` then fails for everyone. It needs `git add -f`. The same trap applies to the other three new misc decks in this batch (save-undef-report, save-multianalysis-report, save-crossplot-report), so it is batch-wide - but C5 authored one of the four and the receipt does not mention it.
- 0059's forward-looking record is incomplete in a way that changes the shape of the next fix. Its four constraints are written entirely in terms of com_write()'s !plainwrite branch. `set plainwrite` takes a second branch in the same function that never touches ft_getpnames_quotes(); `wrdata` is a different command reaching the same fallback and cannot be guarded from com_write() at all (I proved this with a guard of the withdrawn shape compiled in). Neither string occurs in 0059, and neither occurs in tests/.
- The sentence that decides the whole trade - 0059 Resolution's "Row 1's defect produces a file that at least labels itself" - is false for two measured shapes. `set appendwrite` puts the constants plot BEHIND a real run's Title and first Plotname inside one file; `wrdata` writes 401 bytes of constants with no header whatsoever. The withdrawal is still the right call on the evidence (a silent no-op on correct input is worse), but the stated justification is narrower than the defect and should be qualified.
- Not independently verifiable: the four deleted decks were untracked additions from this batch (none appears in `git ls-files`), so I could not read them to confirm they pinned only withdrawn behaviour. Circumstantial support is good - LEDGER.md line ~83 already records that destroy-removed-circuit.cir "passes identically with `removecirc` commented out - it does not pin what it claims to pin" - and I confirmed no committed coverage was lost. But that item rests on the receipt plus that note, not on my own reading.
- Disclosed residual, unchanged and confirmed: LEDGER.md lines 83 and 173 still describe destroy-removed-circuit.cir and the plot_cur_chosen clear placement. CLOSEOUT.md carries amendment banners at three sites, but stale text survives BELOW them (line 186 "The comment now at postcoms.c says so"; line 450 "Rows 1 and 7 are now absent files"), covered by the banners but readable as current if skimmed.
- MINE, not C5's, needs cleanup: my stderr sweep left ~53 untracked byproducts in the repo - examples/cider/bjt/, examples/cider/diode/, examples/cider/cider-gnuplot/ (44 .data/.plt), examples/digital/compare/adder_x.vcd, examples/sp/new_137mhz_bpf.s2p, examples/xspice/{d_source/d_s.out, delta-sigma/digi4b.txt, pll/pll.raw, state/spifsim.raw, state/spifsim.vcd}. My removal command was denied by the permission classifier. They are untracked and harmless but should be cleaned before any `git add -A`.
- ENVIRONMENTAL, not C5's, but it undermines hand measurements: build-ver_50/src/spinit hard-codes `codemodel /usr/local/lib/ngspice/*.cm`, so running the build-tree binary on any XSPICE deck outside the test harness loads the Aug-2 installed code models and segfaults (gdb: cm_adc_bridge in /usr/local/lib/ngspice/digital.cm). With the freshly built .cm files the same deck exits 0 with empty stderr. Any by-hand measurement in this batch that ran an XSPICE deck with the build-tree binary is exposed to this.