# Closeout — casemode client-feedback batch, 2026-08-12/13

Written 2026-08-13 after stage 7, as a full replacement for two earlier drafts,
and **brought to stage 9 the same evening**. The batch was closed at stage 7
and then re-opened by the repo owner's three decisions; stages 8 and 9 added
two fixes, three new issues, one defect *in* one of those fixes, and one
severity correction to an issue the batch had filed about itself. Everything
below is current as of that correction. The structure is stage 7's and the two
superseded drafts are still gone.

Sources read for this pass, newest first: the in-place correction to
`doc/codex/issues/0067` and to the two receipts it touched, the stage-9 receipt
`receipts/stage9-crewF.md`, the three stage-8 receipts (`stage8-crewE.md`,
`stage8-crewD.md`, `stage8-issue0065.md`), then `LEDGER.md` and this file's
stage-7 text, then issues `0056`–`0067`, decisions `0001`, `0016`, `0017`, the
`git diff` of all twelve modified files under `src/`, and the tree itself. The
stage 1–7 receipts were read at the previous pass and have not changed except
where the 0067 correction touched `stage8-crewE.md`.

**Labels are load-bearing.** `[measured]` means this pass ran it and quotes
what it printed. `[read]` means it comes from a receipt or an issue file and
was not re-run here. `[unverified]` means nobody in the batch could show it,
including me.

## Tree state [measured]

```
$ git rev-parse HEAD
720c8743a69ae9dffe4731f5d173daf14b9baf45
$ git diff --stat | tail -1
 24 files changed, 896 insertions(+), 102 deletions(-)
$ git diff --cached --name-only | wc -l
15
$ git diff --cached --name-only | grep -vc '\.out$'
0
$ git status --short -uall | grep -c '^??'
278
```

- **Nothing is committed.** HEAD is still `720c8743a`, the batch's starting
  point, after nine stages.
- **The index holds 15 `.out` files and nothing else.** Stage 7 force-added 13;
  stage 8 added two more (`case/curcasemode-effect.out`,
  `casedist/curcasemode-relatch.out`). A plain `git commit` today would commit
  fifteen reference files and no deck, no source and no issue.
- **278 untracked files**: 125 under `doc/` (66 feedback — one more than at
  stage 7, `repro/shared/netlist_guard_probe.c`; 34 batch bookkeeping including
  32 receipts; 12 issues `0056`–`0067`; 2 decisions; 11 `next-session-*.md`
  suggestions that **predate this batch** and are not the batch's to commit)
  and 153 under `tests/`, of which **19 are deliverable decks** and 134 are
  sub-decks and captures the report decks write when run by hand in `srcdir`.
  The 134 is unchanged since stage 7: stages 8 and 9 added five decks and no
  strays.
- `examples/` is clean **[measured: 0 untracked entries under `examples/`]**.
- Suite, from cleared caches — every `*.trs` and every `*.log` except
  `config.log` deleted first, then `make -C build-ver_50 -j8`, then
  `make -C build-ver_50 check` **[measured]**:
  **rc=0, 305 PASS, 0 FAIL, 0 SKIP, 0 XFAIL, 0 XPASS** **[measured, this
  pass]**.
  `identity lint: 265 comparisons, baseline matches` plus the 11-comparison
  selftest. The single `^ERROR:` line in the whole log is
  `ERROR: (internal)  tried to destroy non-existent graph`, deck output from a
  `casedist` report deck that passes; it predates the batch.
- All **nineteen** new decks pass by name — 14 `.cir` plus five `.cmd`, which
  need no `.out`.
- **The `0059` write path is byte-identical to HEAD**, verified by
  construction rather than by reading the receipt: `git diff --stat` names none
  of `postcoms.c`, `plotting/plotting.c`, `vectors.c`, `dotcards.c`, `spec.c`,
  `com_fft.c` or `NEWS`, and `grep -rn plot_cur_chosen src/` returns nothing.
- **Twelve files under `src/` are modified** **[measured]**: `inpcom.c`,
  `options.c`, `outitf.c`, `outitf.h`, `runcoms.c`, `signal_handler.c`,
  `variable.c`, `evtproto.h`, `fteext.h`, `sharedspice.h`, `sharedspice.c`,
  `evtplot.c`. Three of them — `inpcom.c`, `fteext.h`, `sharedspice.c` — now
  carry hunks from three or four different issues each, which is §2's problem
  and is worse than it was at stage 7.

305 is the current number. Receipts quote 292–305; each was right when written.
The moves after stage 7 were `300 + 3` (crew D's three decks) `+ 2` (crew E's
two) `+ 0` (crew F, whose production change is in a file `make check` does not
compile).

---

## 1. Per issue — true state

### 0056 — `.save` matches vector names byte-exactly — **FIXED AND VERIFIED**

The best-evidenced item in the batch, and the only one whose commit has been
materialised and built (part 2).

- **Code.** `src/frontend/outitf.c`: `name_unwrap()` factored out of
  `name_eq()`; the four query callers (`beginPlot()`'s save-list resolution at
  the former `:286`, `:300`, `:427`, `:432`) call the new `name_eq_query()`,
  which is `vec_name_eq()` — `cieq` under `fold`/`preserve`, exact under
  `distinguish`; `name_eq()` keeps its `strcmp` for the one stored-against-
  stored caller, with the measured reason above it. `OUTattributes()`'s two
  unreachable `strcmp`s are classified in place with `case-lint: stored`.
  **+72/−20** against `HEAD:src/frontend/outitf.c` **[measured]**, out of that
  file's total +276/−19.
- **Also.** `tests/lint/identity.baseline` — the three `outitf.c` records
  removed with the header note the file's convention asks for; 265 records
  **[measured: 265 tab-separated lines]**. `doc/claude/decisions/0001-distinguish.md`
  — the "and no vector-name matcher outside them" sentence corrected, which is
  criterion 9. `AGENTS.md`, `CLAUDE.md`, `tests/bin/identity_lint.sh` — the
  `case-lint` reason words `helper` / `stored` / `neither`.
- **Tests.** `tests/regression/case/save-name-case.cir` +
  `save-name-case-lower.cir` (twin); `tests/regression/case/plot-scale-name-twin.cir`;
  `tests/regression/casedist/save-name-case.cir` (mode boundary, passes before
  and after by design); `tests/regression/pipe/save-name-case.cmd`.
- **Verified here as a standalone commit** — see part 2 for the transcript.
- **Residual, one line.** `0056` Resolution 7 still ends "`CLAUDE.md`'s
  sentence still names only `keyword` and wants the same one-line addition".
  `CLAUDE.md` was edited later in the batch and no longer does **[measured:
  the working-tree diff of `CLAUDE.md` adds `helper`, `stored`, `neither`]**.

`plot-scale-name-twin.cir` is worth naming separately: criterion 2 — the one
comparison this issue deliberately left byte-exact — had **no deck at all**
until round four, when a verifier mutated `name_eq()` to `vec_name_eq()` and
the entire suite passed. The deck is the answer to that, and it asserts the
node's *presence in the plot* rather than a value, because `findvec()` folds
under `preserve` and would answer a read of either spelling with a number.

### 0057 — a failed `.save` names no token — **FIXED, AS A STRICTLY NARROWER THING THAN IT ASKED FOR**

Fixed **as a case near-miss report**. The general unresolved-name report —
criterion 1 as originally written, and what the client asked for — was built,
shipped for three rounds, and **withdrawn** in round four. Signed at stage 5
with zero false positives remaining, by a verifier who ran his own decks
**[read: `receipts/stage5-crewA4.md`]**.

- **Code.** `src/frontend/outitf.c`: `report_save_case_miss()` at Pass 2,
  beside (not in place of) the `saves[i].analysis` gate, returning TRUE when it
  speaks; helpers `ckt_knows_name()`, `case_twin_p()` (`cieq` **and** `!eq` —
  the exclusion of the token from its own scan), `ckt_case_twin()`,
  `save_miss_first_time()` over the `save_miss_reported` wordlist.
  `src/frontend/outitf.h`: `OUTsaveMissClear()`. `src/frontend/runcoms.c`: that
  call at the head of `dosim()`, which is what makes the unit one *simulation*
  rather than one `OUTpBeginPlot()`. `src/xspice/evt/evtplot.c`:
  `Evt_Ckt_Has_Node()`, declared in `src/include/ngspice/evtproto.h`.
- **Tests, seven decks.** `casedist/save-undef-report.cir`,
  `casedist/save-near-miss-node-list.cir`,
  `xspice/casedist/save-event-node-case-split.cir` carry the report;
  `misc/save-undef-report.cir`, `misc/save-multianalysis-report.cir`,
  `misc/save-crossplot-report.cir`, `xspice/digital/save-event-node.cir` carry
  the silences in the default mode. Every silence assertion sits beside a
  positive read of the same capture. A nine-mutation matrix (M1–M9) pins them;
  M9 — restoring the wide report — fails **all seven** **[read]**.
- **What the withdrawal costs, stated not argued away.** An unresolved
  `.save`/`save` name with **no case variant** is silent again, in every mode,
  exactly as in stock `ngspice-46`. That is the defect the issue was filed for.
  The client's finding 4 is half answered.
- **The reason to believe the narrowing rather than regret it**: the wide
  report fired on two decks this repository *ships* —
  `examples/transient-noise/noi-ring51-demo.cir` (3 lines) and
  `examples/probe/F5TurboV2-Probe.cir` (1) — both true positives about stale
  deck tokens, neither of them a case defect, neither anybody's business in a
  `casemode` batch.
- **Gaps recorded in `0057` Resolution 9**: an event node's own case near miss
  (the twin scan walks `CKTnodes` and `dataNames[]`; an event node is on
  neither — and the silence is *asserted* by a deck, so closing it is a visible
  change); the `.print` route's wording, which **no deck in this tree can
  assert** because `ft_savedotargs()` runs only from `src/main.c`'s batch path;
  `.save @r99[i]` for a device that does not exist, silent in stock too.
- **One gap that is in no issue file**: `.save TIME` against a `tran` whose
  scale is `time` is silent under `distinguish`, because the twin scan skips
  the run's reference vector. It exists only in `receipts/stage5-crewA4.md` and
  in this file (residual 6).
- **Criterion 8 (sequencing) is owed at commit time** and is now demonstrated
  but not discharged: part 2 shows `0056` moving `make check` on its own in a
  materialised tree. It is discharged when the two commits exist in that order.

### 0058 — casemode diagnostics fire once per file read — **FIXED AND SIGNED**

The only issue in the batch that a verifier signed at the first re-check, and
the verification is the strongest in the batch: he built the *rejected* rule as
a counterfactual binary and measured that the deck discriminates the two
**[read: `receipts/stage3-crewB2.md`]**.

- **Code.** `src/frontend/inpcom.c`: `set_case_mode()` split the way
  `set_compat_mode()`/`print_compat_mode()` already is, gated not on `comfile`
  but on a file-static latch of the **whole outcome of the last read**
  (`ng_case_last_outcome` + `ng_case_last_unknown`, compared by
  `case_outcome_is_new()`, which re-latches either way); both `fprintf`s moved
  to `cp_err`. `src/include/ngspice/fteext.h`: `inp_case_announce_reset()`.
  `src/sharedspice.c`: that call in `totalreset()`, so a host that resets and
  re-loads a `distinguish` circuit is told the mode's warranty again.
- **Test.** `tests/regression/casedist/casemode-announce-report.cir` — six
  cases (INHERIT, BOGUS, REPEAT, BACK, VIA, RETURN) as *counts*, each silence
  carrying a `CAPTURE-READ` token. Proved RED twice, against the shipped binary
  and against a `cp_err`-only binary.
- **Decision.** `doc/claude/decisions/0016-case-mode-announcement-latch.md`
  records which of two readings the latch implements, with the counterfactual.
- **Residual, one line, in a receipt only**: under `--enable-gc` (`HAVE_LIBGC`)
  `tfree()` is a no-op, so `inp_case_announce_reset()` leaves
  `ng_case_last_unknown` set and a repeated misspelled `casemode` stays silent
  after a reset. Fix is `ng_case_last_unknown = NULL;` after the `tfree`.

### 0059 — `write` emits the constants plot when no analysis ran — **WITHDRAWN. THE DEFECT IS REAL AND THE ISSUE IS OPEN.**

Three discriminators were built, verified, and each falsified by the next
round. The third refused correct work, and the guard came out. **No source
line of it survives** — verified by construction here, not read off the
receipt.

What remains in the tree from this issue is one deck and two documents:
`tests/regression/misc/write-let-session.cir` + `.out`, which pins **row 10**
(a nutmeg session's own `let` vectors, written by a bare `write`, by the
spelled `write f.raw all`, and by name — with the bare file *read back inside
the deck*); `doc/codex/issues/0059`, rewritten to hold the record; and
`doc/claude/decisions/0017`, marked WITHDRAWN with decision 1 explicitly
surviving, because that one is a decision *not* to change `write`.

**Why the withdrawal is right, and why I believe it over the three crews that
built the guard.** The guard's premise was "the current plot is a plot nothing
chose, so it is garbage". The constants plot is where a nutmeg session's own
`let` vectors live, precisely *because* nothing has chosen a plot. Rows 1 and
10 are the same plot in the same state of chosenness. Stock `ngspice` and a
HEAD baseline both write that session's 3275-byte file; all three guards wrote
**nothing at all**, in `--batch` and through `ngspice -p` alike. A silent no-op
on correct input is worse than a bad file on incorrect input, because the bad
file can be inspected and the no-op leaves nothing behind.

**Three further routes were found *after* the withdrawal, and I re-measured
all three myself** against `build-ver_50/src/ngspice` **[measured]**:

```
$ ngspice -b -n good.cir              # op, then write acc.raw
acc.raw after the real run: 305 bytes
Title: * a real run whose plot is written first
Plotname: Operating Point
No. Variables: 3
$ printf 'set appendwrite\nwrite acc.raw\nquit\n' | ngspice -p -n
--- 875 bytes
1:Title: * a real run whose plot is written first
4:Plotname: Operating Point
16:Plotname: constants
18:No. Variables: 12
$ printf 'wrdata wd.dat all\nquit\n' | ngspice -p -n
--- wd.dat 401 bytes; lines matching Title|Plotname: 0
 1.00000000e+00  0.00000000e+00  1.00000000e+00 ... 1.38064852e-23 ...
$ printf 'set plainwrite\nwrite pw.raw\nquit\n' | ngspice -p -n
--- plainwrite 570 vs default 570 ; cmp: IDENTICAL
```

So: `set appendwrite` puts the twelve constants **behind a real run's `Title:`,
`Date:` and first `Plotname:`** inside one file; `wrdata … all` writes 401
bytes of bare columns with **no header of any kind**; `set plainwrite` reaches
the fallback through a **second branch of `com_write()`** that never calls
`ft_getpnames_quotes()`, and its file is `cmp`-identical to the first branch's.
These are rows 11–13 of the issue's matrix, added by stage 7.

Two consequences, both now in `0059`:

1. The sentence the withdrawal trade originally rested on — "the defect at
   least produces a file that labels itself" — is **false** for two of the four
   routes. The corrected statement is that the defect's output is *recoverable*
   and the guard's is not. The verdict does not change; the argument for it was
   narrower than the defect.
2. A guard of the attempted shape would have had to cover **four routes across
   two commands**, and one of them, `wrdata`, was proved unguardable from
   `com_write()` **by experiment** — with a guard of the withdrawn shape
   compiled in, `write` was refused and `wrdata … all` wrote the constants
   anyway.

Rows 1, 7, 8 and 9 reproduce. The issue's Resolution now names five
constraints a correct discriminator has to meet at once and concludes the fix
belongs in the resolver (`vec_get()`, the one point all four routes pass
through), not in a command. It also says to settle `0064` first.

### 0060 — `$casemode` reports the request, not the effect — **FIXED AND CLOSED, 8/8**

Deferred at stage 7 on one word. The owner supplied it — **`curcasemode`** —
and crew D shipped the rest at stage 8. The name joins the existing computed
read-only family (`curplot`, `curplotname`, `curplottitle`, `curplotdate`,
`plots`), so it needed no new convention: computed on read in `cp_enqvar()`,
refused on write through `cp_usrset()`'s existing `US_READONLY` arm.

- **Code, three edits.** `src/frontend/inpcom.c` — `inp_case_mode_name()`
  beside `inp_case_mode()` and the `static int ng_case_mode` it reads, so the
  three spellings sit next to `set_case_mode()`'s three `cieq()` arms and
  cannot drift. `src/include/ngspice/fteext.h` — its declaration.
  `src/frontend/options.c` — an `eqc(word, "curcasemode")` arm in
  `cp_enqvar()` placed **before** the `if (plot_cur)` block, and an
  `eqc(var->va_name, "curcasemode")` arm in `cp_usrset()` returning
  `US_READONLY`. Also `src/include/ngspice/sharedspice.h` (the read-back and
  the capability probe) and `doc/claude/casemode-distinguish-guide.md`.
- **Tests, three.** `tests/regression/case/curcasemode-effect.cir` + `.out`,
  `tests/regression/casedist/curcasemode-relatch.cir` + `.out`,
  `tests/regression/pipe/curcasemode-effect.cmd`.
- **The capability probe is the part the client asked for and it works**
  **[read: `receipts/stage8-crewD.md`, re-derived here in spirit but not
  re-run]**: `echo $curcasemode` answers `fold`/`preserve`/`distinguish` on
  this build and gives `Error: curcasemode: no such variable.` on stock
  `ngspice-46`. No identity-based probe can do that, because `preserve` folds
  identity exactly as `fold` does.
- **The sequencing premise was wrong, in crew D's favour.** The brief expected
  criterion 6 (not shadowable by a loaded plot's environment) to need `0061`'s
  fix, and allowed crew D to leave one deck failing by design. It did not need
  it: the arm sits ahead of the `pl_env` scan, which the criterion's own text
  permits. Crew E's work then gave criterion 6 a second and wider reason —
  the whole `curplot` family became unshadowable, not just this name.
- **Its verifier returned NEEDS_WORK with no unmet criterion and no
  regression.** The verdict hangs entirely on one disclosure: a rawfile may
  write `Option: curcasemode=…`, the pair is filed and round-tripped by
  `raw_write()`, and the *value* is never answered on a read, while every other
  `Option:` key still answers — and nothing warns. Crew F took it as an item
  and settled it as a sentence rather than a diagnostic, having measured that
  the diagnostic would misfire: a plain `load` → `write` → `load` of a file
  ngspice wrote itself reproduces the line and would fire the warning on every
  lap. **Re-measured here [measured]**: with `Option: curcasemode = distinguish`
  spliced in after `Plotname:` in a default `fold` session,
  `echo read=$curcasemode` prints `read=fold` and `set` lists
  `* curcasemode	fold`.
- **Residual.** Crew D's receipt says the shared-library route (criterion 3
  bullet 3) is unexercised; its **verifier exercised it** in a `build-shared/`
  tree with its own host program and reported six reads moving correctly across
  two `ngSpice_Circ()` calls. The issue's Left still says "documented and
  unexercised", which is now understated. Residual 20.

### 0061 — a rawfile `Option:` line reconfigures the next netlist read — **FIXED AND CLOSED, SHAPE A, AND ITS OWN FIX HAD A BUG**

Deferred at stage 7 on a contract question. The owner chose **shape A**: the
`Option:` pair keeps being filed into `plot_cur->pl_env` and keeps being
readable — which is the client's finding 1 — and only the reconfiguration
stops. Crew E shipped it and was **SIGNED**, the only crew in the batch signed
at first verification apart from `0058` and `0057`.

- **The narrowing is a rule, not a name list, and this is the batch's one
  clean answer to its own worst habit.** A plot's environment may answer a
  question about the session; it may not answer a question asked on behalf of
  a file being turned into cards. So `cp_getvar()`'s plot-environment link
  gains `&& !inp_reading_netlist()`, and the window is the whole of
  `inp_readall()`. No variable name appears in the predicate. `casemode`,
  `ngbehavior`, `no_auto_gnd`, `addcontrol`, `sourcepath` and the next one
  somebody adds are all covered without being named — and the verifier proved
  that on a **third** switch neither the fix nor either deck mentions
  (`Option: no_auto_gnd`) **[read]**.
- **Code, five edits.** `src/frontend/inpcom.c` — `inp_readall()` becomes a
  five-line wrapper raising a depth count, the body renamed
  `inp_readall_cards()` with an identical signature; plus
  `inp_reading_netlist()` and `inp_netlist_read_reset()`.
  `src/frontend/variable.c` — that one `&&`. `src/include/ngspice/fteext.h` —
  two declarations. `src/frontend/signal_handler.c` and `src/sharedspice.c` —
  the reset call on the two paths that leave `inp_readall()` by a longjmp.
- **Test.** `tests/regression/pipe/rawfile-option-parse-policy.cmd`, fail-fast
  by exit code, every "nothing moved" check preceded by a positive control that
  reads the key back — so a shape-B refusal at the `Option:` arm would fail
  this deck rather than pass it.
- **Criterion 1, re-measured here [measured]**: a deck whose net is spelled
  `MidNode`, sourced with and without a preceding `load` of a header carrying
  `Option: casemode=preserve`, gives `midnode` both times. Shape A's read-back
  survives in the same session: `after=preserve effect=fold`.
- **Residual, by design.** `Command: set casemode=preserve` in a raw header
  still reconfigures the next read, because `cp_evloop("set …")` writes the
  global `variables` list, which is the first link of `cp_getvar()`'s chain and
  is not narrowed. Criterion 4, disclosed, documented at
  `man/man1/ngsconvert.1:109`, and the same on stock.
- **The fix had a defect of its own, and it is the most interesting thing in
  stages 8–9.** The depth count is raised and lowered by `inp_readall()`; a
  netlist read that ends *fatally* never comes out that way. Harmless in the
  standalone binary (`controlled_exit()` calls `exit()`); in a shared build it
  calls `shared_exit()`, the stack is discarded, and the count stays at 1 for
  the rest of the host process — from which point `plot_cur->pl_env` answers
  **no** `cp_getvar()` at all, which is this issue's narrowing applied
  permanently and to everything. `0066`, below.

### 0062 — `ngSpice_Reset()` leaves the shared library unusable — **FILED ONLY**

Doc-only, and correctly so: nothing here is reachable from `make check`,
because no directory under `tests/` links `libngspice`. The filer **overturned
the attribution it was handed**: `CKTmodCrt()` is where the crash lands in one
harness, `INPtypelook()` one frame earlier in a fresh process, and which frame
dies is selected by a function-static cache at `inp2v.c:20` — so a fix aimed at
`CKTmodCrt()` would relocate the crash. Pre-existing and upstream (`d0ae65acc`,
2024). Four probe programs sit with it under
`doc/claude/feedback/ngspice_upstream/repro/shared/` **[measured: 4 `.c` files
there]** — they go in with C0, and nothing in this batch is committed yet.
**Not re-verified by this pass** — it needs a `--with-ngshared` build. The decision awaited is a
lifecycle one: `totalreset()` re-initialises, or `ngSpice_Reset()` is
documented as a de-initialiser and every entry point checks `is_initialized`.

### 0063 — `removecirc` leaves `plot_cur` outside `plot_list` — **FILED ONLY, NOW UNBLOCKED**

`display` lists three vectors of a plot `setplot` says does not exist and
`print` cannot reach. Filed rather than fixed by decision (`0017` decision 2).
Its criterion 4 was **discharged** by the withdrawal: the deck that asserted
`Internal Error: kill plot -- not in list` *is* printed was deleted with the
guard, so nothing in the tree now pins that as correct behaviour. Whoever takes
it needs a fresh RED deck for criteria 1–3; with the internal error no longer
pinned, the available discriminator is `$curplot` and `display` disagreeing.
The one-line fix in `killplot()` is deliberately *not* the fix: the invariant
is broken by `com_removecirc()`.

### 0064 — a wildcard matching one vector is renamed to the wildcard — **FILED ONLY**

`ft_evaluate()` renames a single-vector result to the parse node's own text, so
`write` of a one-vector plot emits `v(all)` beside `v(midnode)` — two columns,
one vector, `No. Variables: 2` where the plot has 1. Pre-existing, upstream,
mode independent, and **visible in the client-facing evidence**
(`run_all.sh` section 2). Not fixed because `ft_evaluate()` is on the path of
every control-language expression. `0059` names it as the thing to settle
first, because whatever the resolver learns to report about a wildcard it
should report once.

### 0065 — a rawfile `Option:` line shadows a computed variable — **FIXED AND CLOSED**

Filed at stage 8, found while explaining `0061` to the repo owner, fixed by
crew E in the same change. **Not a wrong value: a `SIGSEGV` on a bare `load`,
`rc=139`, and identically on stock `ngspice-46`** — pre-existing and upstream.

`cp_enqvar()` scanned `plot_cur->pl_env` *before* the arms that compute the
`curplot` family and `plots`, so a file-supplied string was returned where a
computed value was expected. The consumer that dies is not a reader of
`$curplot` — nothing in `src/` calls `cp_getvar("curplot")` — but
`cp_usrvars()`, which runs on **every** `cp_getvar()` whatever the name asked
for, and which declared `int tbfreed`, passed its address five times, never
read it, and linked a node borrowed from the plot into a chain all three of its
callers free.

- **Two edits, neither a list of names.** The computed arms now precede the
  `pl_env` scan — what this function computes, it answers itself. And
  `usrvar_push()` replaces five repeated link pairs, dropping any answer with
  `tbfreed == 0`: an ownership rule that holds for any name `cp_enqvar()` ever
  learns to answer from an environment.
- **Test.** `tests/regression/pipe/rawfile-option-computed-name.cmd`, RED at
  HEAD sources with `Segmentation fault` and no marker.
- **Each half measured separately, which is the right way to ship two edits**
  **[read]**: with the ordering reverted and `usrvar_push()` kept there is *no
  fault at all* and the deck fails on the value instead — so the ownership rule
  is what stops the memory-safety fault, measured doing it. With the ordering
  kept and `!tbfreed` removed the deck passes — the guard is not what this deck
  detects, and it is kept because it is the invariant the contract at
  `options.c:38`-`:50` already states. The issue discloses both.
- **One behaviour change, ours, confirmed on three binaries [measured here on
  two]**: `set` after `Option: CurPlot=HAHA` now lists `* CurPlot	op1` and
  `$CurPlot` answers `op1`, where stock `ngspice-46` gives `HAHA` for both.
  `cp_vprint()` prints `va_name` straight from `pl_env` but re-resolves the
  value through `vareval()`, and the arm that now precedes the scan matches
  with `cieqn()`. The pair is genuinely still filed — `write` round-trips
  `Option: CurPlot = HAHA` byte-identically to stock.
- **Its criterion 6 was wrong when signed, and that is `0067`.** See below.
- **Stale bullet.** The issue's Left says "the shared build is still not
  exercised". Crew E's verifier built `--with-ngshared` and drove all five
  poisoned loads through `ngSpice_Init`/`ngSpice_Command` **[read]**. Residual
  20.

### 0066 — a fatal netlist read strands the reader guard — **FIXED AND CLOSED. A DEFECT IN THIS BATCH'S OWN FIX.**

Filed and closed by crew F at stage 9. Found by `0061`'s verifier, in the first
`--with-ngshared` build anyone in the batch made, and **no test in this
repository could have found it**: nothing under `tests/` links `libngspice`, so
`make check` never executes `shared_exit()` at all.

- **Root cause is not where `0061` assumed.** `0061`'s Resolution named two
  landing sites (`ft_sigintr_cleanup()`, `totalreset()`) and claimed they cover
  "the paths that leave `inp_readall()` without returning through it". There
  are **three** routes: `ngSpice_Command` → `longjmp(errbufc)`, `ngSpice_Circ`
  → `longjmp(errbufm)`, and `bg_source` → `pthread_exit()`, which lands at no
  `setjmp` at all. The fix is one line at `src/sharedspice.c:2192`, the last
  point common to all three, rather than at the landing sites.
- **RED/GREEN, all three modes [read]**: `before=1 after=0 GUARD-STUCK` with
  the line absent and the library rebuilt; `before=1 after=1 GUARD-OK` with it.
  Independence proved by breaking each assertion separately — fix removed gives
  `HELD=1 after=0`, guard removed gives `HELD=0 after=1` — so neither half
  passes on the other's mechanism.
- **The direction of the failure is safe**, which is worth stating: a stranded
  count makes the guard *more* closed, never less. Nothing is corrupted;
  answers just stop coming, for the life of the host process.
- **Its pin is not in the tree.** Criterion 4 says so plainly and names
  `doc/claude/feedback/ngspice_upstream/repro/shared/netlist_guard_probe.c` as
  the substitute — self-contained, self-checking, three modes. That file is
  untracked, as is every other probe and deck this batch wrote. Where a probe
  that cannot live under `tests/` should live is left open. Residual 21.
- **I have not re-run the probe** — it needs a `--with-ngshared` build, which
  `build-ver_50` is not. Everything in this entry is **[read]** except the
  source line itself, which I read in `git diff`.

### 0067 — `cp_remvar()` frees a node it chose not to unlink — **FILED, OPEN, AND ITS SEVERITY WAS WRONG FOR TWO DRAFTS**

Filed by crew F at stage 9 out of two loose ends in `0065`'s Left, corrected
in place after its own verification. Pre-existing and upstream: nothing in this
batch caused it, touched it, or is needed to reproduce it.

`cp_remvar()` walks four lists to find the variable, then switches on
`cp_usrset()`. `US_OK` is the only arm whose contract with the tail is
coherent — it unlinks and does not free, and the tail at `variable.c:671`-`:672`
frees and does not unlink. Three other arms reach that tail having already
disposed of the node: `US_DONTRECORD` and `US_READONLY` deliberately leave it
linked (and *print a diagnostic because they detect exactly that*), and
`US_SIMVAR` frees it itself at `:659`.

**I re-measured all of it [measured]**, three runs of each, on
`build-ver_50/src/ngspice` and on stock `/usr/local/bin/ngspice`:

```
$ printf 'unset curplot\nquit 0\n' | ngspice -p -n
cp_remvar: Internal Error: var 99
free(): double free detected in tcache 2
   rc=134, 3/3, both binaries

$ printf 'load hdr_zzz.raw\nunset zzz\nquit 0\n' | ngspice -p -n
   rc=0, 3/3, both binaries

$ printf 'load hdr_zzz.raw\nunset zzz\nset\nquit 0\n' | ngspice -p -n
   Segmentation fault (core dumped)   rc=139, 3/3, both binaries

$ printf 'load hdr_zzz.raw\nunset zzz\nset\nquit 0\n' | valgrind -q ngspice -p -n
   rc=0, 34 invalid reads
```

Those four blocks are the whole story of the correction and are in §4 as
failure 7. The `unset` survives; **the next thing the user types does not**;
and `valgrind`'s allocator keeps the freed block readable, so the tool answers
`rc=0` on the sequence that segfaults under `glibc`.

- **One issue and not three**, on the argument that the three arms differ only
  in which list still points at the node and therefore in *when* the freed
  memory is noticed. Its criterion 3 requires the fix to be stated as an
  ownership rule rather than a list of arms, and observes that a repair
  enumerating `US_DONTRECORD` and `US_READONLY` would satisfy criterion 2 and
  still abort on criterion 1.
- **Criterion 6 asks for the exit status of the command *after* the `unset`**,
  because a deck written around the `unset` itself passes today. That is the
  correction made structural.
- Two of the three shapes need no rawfile and no unusual input: `unset curplot`
  and `unset temp` after a `set` are things a user types to tidy up a session.

### Where a crew and a verifier disagreed — stages 2–6

Five substantive disagreements, and **the verifier was right in all five**.
(Of the eleven verification passes over the fix stages 2–6, nine returned
NEEDS_WORK and two signed — `0058` at stage 3 and `0057` at stage 5. These five
are the ones where a crew had asserted the opposite of what the verifier then
measured.)
This is the batch's clearest signal and the reason its verification structure
earned its cost.

| # | Crew claimed | Verifier measured | Who was right |
| --- | --- | --- | --- |
| 1 | `0057` criterion 6 met — "once per unresolved token, not once per `OUTpBeginPlot()` call" | The report hangs off `beginPlot()`: `.noise` → 2 lines, `.disto` → 3, for one mistake | **Verifier.** Fixed in round 3 by `save_miss_first_time()` + `OUTsaveMissClear()` in `dosim()`; re-measured 1 line for `.disto`'s six plots |
| 2 | Stage-2 crew C: "0059 — DONE" | Criterion 2 unmet (no decision record), plus a placement bug in `killplot()` | **Verifier.** And the whole guard was withdrawn four rounds later |
| 3 | Stage-3 crew A2: "no `identity.baseline` change was needed" | The staged tree fails its own lint — `outitf.c` staged, the baseline it requires unstaged | **Verifier.** `0056` Resolution 7 itself says the baseline loses three records. A3 then staged it, and the *next* verifier measured the index still aborting at `tests/regression/misc` after 51 PASS, because staged decks depended on other crews' unstaged `src` |
| 4 | Stage-5 crew C4: item 1 done, the guard keys on the subject | Four spellings walk past it (`ally`, `ALLY`, `"all"`, `all all`), and it refuses a correct `let` session in `--batch` and through `ngspice -p` | **Verifier.** Route B followed |
| 5 | Stage-6 crew C5: the new deck was "Added … + `.out`" | `tests/.gitignore` line 3 is `*.out`; in git terms the reference file was not added at all | **Verifier.** Stage 7 then staged all 13 with `git add -f` |

I believe the verifiers in all five because each disagreement was settled by a
measurement I can re-run, and in cases 2, 4 and 5 I did: the write path is
byte-identical to HEAD, the three post-withdrawal routes reproduce on my own
runs, and the thirteen `.out` files are in the index only because someone
forced them there.

### Where a crew and a verifier disagreed — stages 8–9

Seven more, and **the verifier was right in all seven** — carrying the batch's
tally to twelve for twelve. Four of the seven were settled by a measurement I
re-ran myself.

| # | Crew claimed | Verifier measured | Who was right |
| --- | --- | --- | --- |
| 6 | Crew D: the shared-library route of `0060` criterion 3 is documented and unexercised | Built `libngspice` from `build-shared/`, ran its own host, and watched six reads move correctly across two `ngSpice_Circ()` calls | **Verifier**, and in the crew's favour. The issue's Left is now understated rather than wrong |
| 7 | Crew E: `0065` criterion 6 — `cp_remvar()` and `cp_vprint()` "both correct with no change of their own" | `cp_remvar()` aborts, and the same document's Left section already recorded it: `rc=134` on this build and on stock, no rawfile anywhere | **Verifier.** `cp_vprint()` is correct; `cp_remvar()` is not. Crew F corrected the sentence and filed `0067`. **Re-measured here: `rc=134`, 3/3, both binaries** |
| 8 | Crew E's residual: the `unset zzz` fault `0065` recorded as `rc=139` "did not reproduce today (`rc=0` on both binaries); allocator-dependent" | The measurement stopped at the `unset`. Add the ordinary next command and it is `rc=139`, deterministically | **Verifier.** `0065`'s `rc=139` was right and is restored. **Re-measured here: `rc=0` for the two-command form, `rc=139` for the three-command form, 3/3 each, both binaries** |
| 9 | Crew E: `0061`'s two reset sites cover the paths that leave `inp_readall()` without returning | A third route, `bg_source` → `pthread_exit()`, lands at neither; measured raising the guard exactly as the other two do | **Verifier.** `0066` |
| 10 | Crew E: the fix removes two spurious `sourcepath` diagnostics (`Error: sourcepath is a read-only variable.`, `cp_vset: Internal Error`) | Falsified twice over: BASE is silent on the same input, so it was never this batch's; and the lines still print on all three binaries when the deck is sourced from a directory genuinely new to `sourcepath` | **Verifier.** The original observation sourced from the cwd, where `.` is already on `sourcepath`, so `add_to_sourcepath()` returns before calling `cp_vset()` **[read]** |
| 11 | Crew F, in `0067` as first written: the `US_READONLY` arm is "latent, allocator dependent, and today usually silent", and `0065`'s `rc=139` "did not reproduce" | Deterministic `SIGSEGV`, `rc=139`, 3/3, on both binaries. Crew F had run the doc's own quoted command line under `valgrind` and reported the tool's exit status | **Verifier.** Corrected in place; §4 failure 7. **Re-measured here, including the `valgrind` masking: `rc=0` with 34 invalid reads** |
| 12 | Crew E, in a code comment: a count rather than a flag is justified because "`inp_readall()` has three callers and one of them, the XSPICE auto-bridge, reads a deck ngspice writes itself" | That caller is not a nesting path — `Evtcheck_nodes()` runs under `inp_dodeck()`, long after the `inp_readall()` at `inp.c:532` has returned | **Verifier**, on a small point. The comment says "There is no nesting today" first, so it is not false; the named caller is simply not evidence for the count **[read]** |

The pattern of stages 2–6 held exactly: **verifiers who built the
counterfactual produced findings that survived; the crews found none of their
own defects.** Stage 9 adds a new twist to it — a verifier catching a *crew's
verification method*, not its code. Crew F's measurement was not careless; it
was run under a tool, and the tool was the error.

---

## 2. The commit split

**Eleven commits, C0–C10, up from six at stage 7.** Stages 8 and 9 added four
of them (C6–C9) and changed what C5 holds.

**Every `.out` under `tests/` is hidden by `tests/.gitignore` (`*.out`) and
needs `git add -f`.** All fifteen: thirteen for C1–C4 and two more for C6
(`case/curcasemode-effect.out`, `casedist/curcasemode-relatch.out`). A plain
`git add -A` drops every one of them and sweeps in 134 stray sub-decks and
captures; a `git commit -a` today would commit the fifteen `.out` files alone,
because they are what is currently staged. **Run `git reset` first**, then
stage each commit explicitly, and `git add -f` every `.out` by name.

Three of the five `.cmd` decks (C6's, C7's, C8's) live in
`tests/regression/pipe/`, which compares no `.out` at all — so those three
commits add a test with no reference file, correctly, and only C6 needs the
force-add.

`Makefile.in`, `configure` and `aclocal.m4` are **not tracked** **[measured:
`git ls-files` returns nothing for them]**, so no commit carries a regenerated
build file; `./autogen.sh` is a local step after editing any `Makefile.am`.

**C0 — `docs: the xschem casemode report and its repro set`**
`doc/claude/feedback/ngspice_upstream/**` — **66 committable files**
**[measured: `git ls-files -o --exclude-standard doc/claude/feedback | wc -l`]**:
`FINDINGS.md`, `README.md`, `repro/` (decks, `run_all.sh`, `hdr_variants.sh`,
`reads/`, `spiceinit*/`, `count/`, `shared/*.c`, the two `.gitignore`s). The
66th is stage 9's `repro/shared/netlist_guard_probe.c`, which is `0066`'s pin —
so **the shared-build probe is in this commit**, beside `0062`'s three. The 16
`.raw` byproducts and `sl.out` there are gitignored and stay out.
First, because every issue file in C1–C9 cites these paths. Wart to accept:
`FINDINGS.md` and `run_all.sh` already describe C1–C4's outcomes, so as the
first commit they read one commit ahead of themselves; the alternative is four
commits of dangling citations. No build input, so green trivially.

A second wart, new at stage 9 and worth naming rather than hiding: putting
`netlist_guard_probe.c` here means the probe lands eight commits before the
one-line fix it exercises (C9), and its own header comment describes it as
"committed with this issue". Moving it to C9 instead is defensible and costs
nothing; what is *not* available is leaving it out, because `0066` criterion 4
names it as the only pin the defect can have.

**C1 — `fix: .save resolves a vector name the way every other site does` (0056)**
- `src/frontend/outitf.c` — **0056 hunks only**: `name_unwrap()`, the
  `name_eq()`/`name_eq_query()` split with `name_eq()`'s new comment, the four
  query call sites, `OUTattributes()`'s comment and its two `case-lint: stored`
  markers
- `tests/lint/identity.baseline`
- `tests/regression/case/{save-name-case,save-name-case-lower,plot-scale-name-twin}.{cir,out}`
  + that directory's `Makefile.am` — the two `TESTS` lines and the `pst_*.txt`
  `CLEANFILES` glob. **Changed at stage 8**: the whole of that file's diff is
  no longer C1's, because crew D appended `curcasemode-effect.cir` to the
  `show-device-letter-case.cir` line and `cce_*.txt` to `CLEANFILES`
- `tests/regression/casedist/save-name-case.{cir,out}` + **only that deck's**
  `TESTS` entry in its `Makefile.am`
- `tests/regression/pipe/save-name-case.cmd` + **only that deck's** `TESTS`
  entry. **Changed at stage 8**: that file's diff is no longer whole-commit
  C1's either — it now registers three more `.cmd` decks and three
  `CLEANFILES` globs across C6, C7 and C8
- `doc/claude/decisions/0001-distinguish.md` — **the "no vector-name matcher
  outside them" paragraph only**
- `AGENTS.md`, `CLAUDE.md`, `tests/bin/identity_lint.sh` — the `case-lint`
  reason words. `stored` is introduced by this commit's markers; `helper` and
  `neither` are documented one commit early, which is harmless because nothing
  reads the reason word
- `doc/codex/issues/0056-…md`

**C2 — `fix: a .save name that misses by case names the twin` (0057)**
- `src/frontend/outitf.c` (the rest), `src/frontend/outitf.h`,
  `src/frontend/runcoms.c`, `src/xspice/evt/evtplot.c`,
  `src/include/ngspice/evtproto.h`
- `tests/regression/misc/{save-undef-report,save-multianalysis-report,save-crossplot-report}.{cir,out}`
  + that directory's three `TESTS` entries, the `sur_*`/`smr_*`/`scr_*`
  `CLEANFILES` globs and the three header comment paragraphs
- `tests/regression/casedist/{save-undef-report,save-near-miss-node-list}.{cir,out}`
  + their `TESTS` entries, the `sur_*`/`snm_*` `CLEANFILES` and their comments
- `tests/xspice/digital/save-event-node.{cir,out}` + `Makefile.am` (`TESTS`,
  the new `CLEANFILES` line, its header comment)
- `tests/xspice/casedist/save-event-node-case-split.{cir,out}` + `Makefile.am`
- `doc/claude/decisions/0001-distinguish.md` — **the new decision-2 table row
  only**
- `doc/codex/issues/0057-…md`
- no `identity.baseline` change: its new comparisons are annotated in place

**C3 — `fix: announce the case mode once per run, not once per file read` (0058)**
- `src/frontend/inpcom.c` — **0058 hunks only**: `ng_case_last_outcome` /
  `ng_case_last_unknown` with their comment, `inp_case_announce_reset()`,
  `case_outcome_is_new()`, and `set_case_mode()`'s rewrite (the two `fprintf`s
  moved to `cp_err`). **Not** `inp_case_mode_name()` (C6) and **not** the
  `inp_readall()` wrapper, `inp_reading_netlist()` or
  `inp_netlist_read_reset()` (C8)
- `src/include/ngspice/fteext.h` — **the `inp_case_announce_reset()`
  declaration only**; the file is three-owner now (C3, C6, C8)
- `src/sharedspice.c` — **the `inp_case_announce_reset()` call in
  `totalreset()` only**; the file is three-owner now (C3, C8, C9)
- `tests/regression/casedist/casemode-announce-report.{cir,out}` + its `TESTS`
  entry, the `cma_*` `CLEANFILES` and its comment paragraph
- `doc/claude/decisions/0016-case-mode-announcement-latch.md`
- `doc/codex/issues/0058-…md`

**C4 — `test: a nutmeg session's own let vectors stay writable` (0059, withdrawn)**
- **No `src/` file at all**, and that is the content of the commit: the six
  files the guard touched and `NEWS` are back at `720c8743a`.
- `tests/regression/misc/write-let-session.{cir,out}` + that directory's
  `TESTS` entry, the `wls_*.raw` `CLEANFILES` glob and its comment paragraph
- `doc/claude/decisions/0017-write-refuses-an-unchosen-plot.md` (WITHDRAWN)
- `doc/codex/issues/{0059,0063,0064}-…md`

**C5 — `docs: file the shared-build lifecycle defect this round did not fix`**
- `doc/codex/issues/0062-…md` — **this commit is smaller than it was at stage
  7**: `0060` and `0061` were in it as deferred write-ups and have moved to
  C6 and C8, which fix them. `0062` is the only issue left that this batch
  filed, could not fix, and does not carry into a later commit.
- **Not** the batch bookkeeping any more either; that is C10, so that it can
  describe stages 8 and 9 without reading nine commits ahead of itself.
- **Not** the 11 `doc/claude/suggestions/next-session-*.md`: they predate the
  batch (they are in the session's opening `git status`).

**C6 — `feat: report the case mode in force as curcasemode` (0060)**
- `src/frontend/inpcom.c` — **`inp_case_mode_name()` and its comment only**
- `src/include/ngspice/fteext.h` — its declaration only
- `src/frontend/options.c` — **0060 hunks only**: the `curcasemode` arm in
  `cp_enqvar()` *before* the `if (plot_cur)` block, the `US_READONLY` arm in
  `cp_usrset()` after `plots`, and the two header comments that name the new
  member. **Not** the `pl_env`-scan move or `usrvar_push()` (C7)
- `src/include/ngspice/sharedspice.h` — whole diff, C6's alone
- `doc/claude/casemode-distinguish-guide.md` — whole diff, C6's alone (§2's
  lead, §9's client probe, the corrected "no pipe-mode probe can" sentence)
- `tests/regression/case/curcasemode-effect.{cir,out}` + that directory's
  `TESTS` addition and the `cce_*.txt` `CLEANFILES` glob and its comment
- `tests/regression/casedist/curcasemode-relatch.{cir,out}` + its `TESTS`
  entry, the `ccr_*.cir` `CLEANFILES` and its comment
- `tests/regression/pipe/curcasemode-effect.cmd` + its `TESTS` entry and the
  `ccp_*` `CLEANFILES` globs
- `doc/codex/issues/0060-…md`
- Two of the fifteen force-added `.out` files are this commit's.

**C7 — `fix: a computed variable is answered by its computation, not by a file` (0065)**
- `src/frontend/options.c` — **0065 hunks only**: the `pl_env` scan moved to
  *after* the `curplot`-family and `plots` arms with its comment, and
  `usrvar_push()` replacing `cp_usrvars()`' five link pairs with its comment
- `tests/regression/pipe/rawfile-option-computed-name.cmd` + its `TESTS` entry
  and the `rcn_*` `CLEANFILES` globs
- `doc/codex/issues/0065-…md` (with criterion 6 already corrected)
- `doc/codex/issues/0067-…md` — new, open, and filed here rather than in C5
  because it is the issue `0065`'s corrected criterion 6 points at, and both
  documents then land together
- No `.out`: `tests/regression/pipe/` compares none.

**C8 — `fix: a loaded plot does not answer the reader's policy questions` (0061, shape A)**
- `src/frontend/inpcom.c` — **0061 hunks only**: `inp_reading_netlist()`,
  `inp_netlist_read_reset()`, the `inp_readall()` wrapper and the rename of
  the body to `inp_readall_cards()`, with their comments
- `src/frontend/variable.c` — whole diff, C8's alone (the `&&
  !inp_reading_netlist()` and its comment)
- `src/frontend/signal_handler.c` — whole diff, C8's alone
- `src/include/ngspice/fteext.h` — the two reader declarations
- `src/sharedspice.c` — the `inp_netlist_read_reset()` call in `totalreset()`
- `tests/regression/pipe/rawfile-option-parse-policy.cmd` + its `TESTS` entry
  and the `rop_*` `CLEANFILES` globs
- `doc/codex/issues/0061-…md`
- No `.out`.

**C9 — `fix: a fatal netlist read lowers the reader's guard` (0066)**
- `src/sharedspice.c` — the `inp_netlist_read_reset()` call in `shared_exit()`
  with its comment, and `totalreset()`'s comment corrected because it no longer
  describes the path that needed it
- `src/frontend/inpcom.c` — comment only: `inp_netlist_read_reset()`'s header
  enumerates three callers instead of two
- `doc/codex/issues/0066-…md`
- No test, and the commit message should say why: nothing under `tests/` links
  `libngspice`, so `make check` cannot execute `shared_exit()` at all. The pin
  is `netlist_guard_probe.c`, in C0.

**C10 — `docs: the casemode batch's ledger and receipts`**
- `doc/claude/batches/2026-08-12-casemode-client-feedback/` — LEDGER, 32
  receipts, this file: **34 files** **[measured]**. Drop it if batch
  bookkeeping is not wanted in history; it is the one commit no other commit
  depends on.

**Ordering, and it is tighter than it was.**

1. **C1 before C2**, separate — `0057` criterion 8, the one sequencing claim
   asserted at every stage and demonstrated (again, this pass) in §2's
   materialised tree.
2. **C6 before C8.** `0061` shape A leaves a session able to report
   `casemode=preserve` from a header while folding; the thing that makes that
   readable rather than a lie is `curcasemode`, and `0061`'s own criterion 3
   rests on it. Shipping C8 first would put a knowingly confusing state in
   history for one commit.
3. **C9 after C8**, necessarily: it fixes a defect C8 introduces. **C8 alone
   ships a shared-build regression** — a fatal netlist read strands the guard
   and `pl_env` stops answering `cp_getvar()` for the life of the host process.
   If every commit has to be shippable in a `--with-ngshared` build, squash C9
   into C8; if the history is meant to show that the batch's own verification
   caught it, keep them apart. My preference is to keep them apart and say so
   in C9's message, because the finding is the more valuable artefact.
4. **C7 and C8 are independent of each other**, and crew E measured it both
   ways rather than asserting it **[read]**: with `variable.c` reverted to
   HEAD, `rawfile-option-computed-name.cmd` (C7's) still passes and
   `rawfile-option-parse-policy.cmd` (C8's) fails; with the `cp_enqvar()`
   ordering reverted and `usrvar_push()` kept, the reverse. Either order works;
   C7 first is chosen so that `0067` is filed before `0061`'s Left cites it.
5. C3, C4 and C5 are independent of everything else and of each other.

**Files with more than one owner — the real hazard, and stages 8–9 made it
`src/`'s problem too:**

| file | split |
| --- | --- |
| `src/frontend/inpcom.c` | **three ways**: `0058`'s latch (C3), `inp_case_mode_name()` (C6), the reader guard (C8) — plus C9's one-comment edit inside C8's block. The three regions are far apart in the file and do not interleave |
| `src/frontend/options.c` | **two ways**: the `curcasemode` arms (C6) and the `pl_env`-scan reorder + `usrvar_push()` (C7). These **do** interleave: C6's arm sits immediately above the `if (plot_cur)` block C7 rewrites |
| `src/include/ngspice/fteext.h` | **three ways**: `inp_case_announce_reset()` (C3), `inp_case_mode_name()` (C6), `inp_reading_netlist()` + `inp_netlist_read_reset()` (C8) |
| `src/sharedspice.c` | **three ways**: `inp_case_announce_reset()` in `totalreset()` (C3), `inp_netlist_read_reset()` in `totalreset()` (C8), the `shared_exit()` line plus the corrected `totalreset()` comment (C9). The two `totalreset()` calls are adjacent lines |
| `src/frontend/outitf.c` | C1's +72/−20 and C2's remainder **[measured this pass: `72 20` by `git diff --numstat` against HEAD]** |
| `tests/regression/casedist/Makefile.am` | **four ways**: `save-name-case.cir` (C1), `save-undef-report.cir` + `save-near-miss-node-list.cir` + `sur_*`/`snm_*` (C2), `casemode-announce-report.cir` + `cma_*` (C3), `curcasemode-relatch.cir` + `ccr_*` (C6) |
| `tests/regression/pipe/Makefile.am` | **four ways**: `save-name-case.cmd` (C1), `curcasemode-effect.cmd` + `ccp_*` (C6), `rawfile-option-computed-name.cmd` + `rcn_*` (C7), `rawfile-option-parse-policy.cmd` + `rop_*` (C8) |
| `tests/regression/case/Makefile.am` | **two ways** (new at stage 8): the three `0056` decks + `pst_*` (C1), `curcasemode-effect.cir` + `cce_*` (C6) |
| `tests/regression/misc/Makefile.am` | **two ways**: the three `save-*-report.cir` + `sur_*`/`smr_*`/`scr_*` (C2), `write-let-session.cir` + `wls_*.raw` (C4) |
| `doc/claude/decisions/0001-distinguish.md` | two independent hunks: the decision-2 table row (C2, near line 119) and the "no vector-name matcher" paragraph (C1, near line 313) |

The stage-7 draft said `fteext.h` was C3's alone. That was true then and is
**false now** **[measured: its diff carries three separate comment-plus-
declaration blocks]**. `outitf.c` is no longer the only source file that has to
be split by hand; there are four.

### What was run to verify C1 [measured, re-done this pass]

C1 was materialised and built at stage 7 and **again at stage 9b**, from
scratch, because three of the four files C1 hand-splits had been edited by
stages 8 and 9 in between and the old scratch tree was gone. Everything below
is this pass's run, not a copy of the stage-7 transcript.

The tree was materialised as **HEAD + C0 + C1** under
`/tmp/claude-1000/-home-qflow-dev-ngspice-test/7d5ac281-…/scratchpad/c1`. C0
contributes no build input (no `Makefile.am`, no source), so this is C1's
build.

1. `git archive HEAD | tar -x -C …/c1` → HEAD
   `720c8743a69ae9dffe4731f5d173daf14b9baf45`.
2. A 0056-only `src/frontend/outitf.c` produced from the working copy by
   `…/scratchpad/split0056.py`, which deletes four anchored blocks — the
   `#ifdef XSPICE` include block, the five 0057 forward declarations (keeping
   `name_eq_query()`, which is 0056's), the `report_save_case_miss()` call and
   its comment in `beginPlot()`, and the 0057 function block between
   `name_eq_query()` and `getSpecial()` — asserting each anchor occurs exactly
   once. It printed:

   ```
   RESIDUE: none
   ```

   for the grep
   `report_save_case_miss|ckt_knows_name|Evt_Ckt_Has_Node|OUTsaveMissClear|save_miss_reported|save_miss_first_time|case_twin_p|ckt_case_twin|evtproto|evt\.h`,
   and `git diff --numstat` against `git show HEAD:src/frontend/outitf.c`
   printed **`72	20`** — the same +72/−20 the stage-7 pass measured, which is
   the cross-check that stages 8 and 9 did not touch this file.
3. C1's other files copied in, and **three** `Makefile.am` hand-edited rather
   than one, which is the part that changed since stage 7:
   `tests/regression/case/Makefile.am` taken from the working tree with crew
   D's `curcasemode-effect.cir` and `cce_*.txt` removed;
   `tests/regression/casedist/Makefile.am` and
   `tests/regression/pipe/Makefile.am` taken from HEAD and given one `TESTS`
   entry each. Each edit asserts the strings it must not contain
   (`curcasemode`, `rawfile-option`, `save-undef-report`,
   `casemode-announce`). `doc/claude/decisions/0001-distinguish.md`
   reconstructed from HEAD with only the 0056 paragraph substituted, asserting
   that `report_save_case_miss` does not appear in the result.
4. `./autogen.sh` → **rc=0**, last line `Success.`
5. `mkdir build && cd build && ../configure` → **rc=0**.
6. `make -j8` → **rc=0**, **0 lines matching `error:`** in the log.
7. `make check` → **rc=0**, and counted from the log:
   **291 `PASS:`, 0 `FAIL:`, 0 `SKIP:`/`XFAIL:`/`XPASS:`**.
   `identity lint: 265 comparisons, baseline matches` plus
   `identity lint: 11 comparisons, baseline matches`. Per directory:
   `regression/case` 116, `regression/misc` 30, `regression/casedist` 24,
   `xspice/case` 21, `regression/pipe` 18, `xspice/casedist` 17,
   `xspice/digital` 7 — i.e. exactly HEAD's directories plus C1's five decks,
   identical to the stage-7 numbers. By name:
   `PASS: save-name-case.cir` (twice, `case` and `casedist`),
   `PASS: save-name-case-lower.cir`, `PASS: plot-scale-name-twin.cir`,
   `PASS: save-name-case.cmd`.
   The one `ERROR:` string in the log is the pre-existing
   `ERROR: (internal)  tried to destroy non-existent graph` deck output.
8. **RED, in the same tree**: `src/frontend/outitf.c` replaced by
   `git show HEAD:src/frontend/outitf.c`, `make -j8` → rc=0, caches cleared,
   the four affected directories re-run →

   ```
   FAIL: save-name-case.cir      (regression/case, rc=2)
   FAIL: save-name-case.cmd      (regression/pipe, rc=2)
   FAIL: identity.lint           (tests/lint,      rc=2)
   ```

   with `regression/casedist` **rc=0, unchanged** — `save-name-case-lower.cir`
   and `plot-scale-name-twin.cir` pass both ways, exactly as their headers say
   they should. The `identity.lint` failure is the third RED signal no receipt
   mentions: the 265-record baseline no longer lists the three comparisons
   HEAD's code still contains.
9. Restored, rebuilt (rc=0), re-run: `case` 116/116, `pipe` 18/18, `lint` 2/2.

**Honest caveat, unchanged.** Step 2 is a hand split of one C file between two
commits, and an earlier pass at this same split was **off by one line and did
not compile** (`error: continue statement not within a loop`, from deleting an
`if (…) {` and leaving its `continue;`). This one is anchored on whole blocks
and asserts uniqueness — but the reason C1 is trustworthy is that it was
*built*, not that the ranges look right.

**C2–C10 are reasoned, not tested.** None of them was materialised or built.
The whole tree is green (305/0 **[measured]**) and each commit is source that
compiles independently plus decks that exercise only its own change — but that
is **reasoning, not measurement**, and it is a weaker claim now than it was at
stage 7, because there are nine unbuilt commits instead of three and four
source files to hand-split instead of one. The likeliest places for a middle
commit to break, in order:

1. **C6 and C7 in `options.c`.** They interleave: C6's `curcasemode` arm sits
   immediately above the `if (plot_cur)` block C7 rewrites, so the hunks are
   adjacent rather than distant. Both orders compile in principle; neither has
   been compiled.
2. **C3, C6 and C8 in `fteext.h` and `inpcom.c`.** Three declarations in one
   header and three regions in one 10 000-line file. The regions are far apart
   and do not interleave, but a `git add -p` that takes one hunk too many gives
   a commit that references a function it does not define.
3. **`pipe/Makefile.am` four ways and `casedist/Makefile.am` four ways.** A
   `TESTS` entry landing one commit early registers a deck whose file is not
   there yet, which is a `make check` failure and not a build failure — the
   trap stage 3 and stage 4 both fell into with the index.

### Test-count arithmetic, as a check on the split

HEAD **286** → C1 **291** (+5: `case` +3, `casedist` +1, `pipe` +1) → C2
**298** (+7: `misc` +3, `casedist` +2, `xspice/digital` +1,
`xspice/casedist` +1) → C3 **299** (+1) → C4 **300** (+1) → C5 **300** (docs)
→ C6 **303** (+3: `case` +1, `casedist` +1, `pipe` +1) → C7 **304** (+1
`pipe`) → C8 **305** (+1 `pipe`) → C9 **305** (no test possible) → C10 **305**
(docs).

Both ends are measured this pass — C1's **291** and the whole tree's **305** —
so the split accounts for all nineteen new decks and orphans none. The 286
follows from 291 by subtraction; every intermediate number is arithmetic, not
measurement.

---

## 3. Residuals — one list a human can work from

"Where" is where it is written **today**. ⚠ marks a residual whose only record
is a receipt or this file — a record the next person to open the issue tracker
will not find.

| # | What is left | Where | Cost |
| --- | --- | --- | --- |
| 1 | **Client finding 3 is unanswered.** `0059` is Open: `write` still emits the 570-byte constants raw after a failed analysis, after `destroy`, and out of a chosen-but-empty plot, by every wildcard spelling — and by three further routes (`plainwrite`, `appendwrite`, `wrdata`) across two commands | `0059` Status, matrix rows 1/7/8/9/11/12/13, Resolution (three attempts, five constraints); `0017` WITHDRAWN; `FINDINGS.md` finding 3 | Constraint 1 alone needs a flag on `ft_cpinit()`'s twelve built-ins; the fix belongs in `vec_get()`, not in a command. A design doc, then 1–2 days |
| 2 | **Client finding 4 is half answered.** An unresolved `.save` name with no case variant is silent again — rc=0, the vector missing, nothing on either stream | `0057` Status + Resolution 1; `FINDINGS.md`; `run_all.sh` section 4 | A design decision first (what may be reported without firing on correct decks, asked *after* the simulation rather than inside `beginPlot()`), then ~1 day |
| 3 | ~~0060 blocked on one word~~ **CLOSED at stage 8** — the owner named it `curcasemode`; 8/8 criteria, three decks | `0060` Resolution | done |
| 4 | ~~0061 blocked on a contract call~~ **CLOSED at stage 8, shape A** — a loaded plot's environment no longer answers a `cp_getvar()` taken while a netlist is being read. **Client finding 1 is still blocked**: the raw header still carries no record of the mode that wrote it, and `Option:` is still disqualified as its carrier | `0061` Resolution; `0060` Left bullet 3 | Finding 1 needs a decision doc for a new header key, then ~1 day |
| 5 | **0062 unfixed** — after `ngSpice_Reset()`, `ngSpice_Command()` runs nothing and `ngSpice_Circ()` segfaults. Invisible to `make check`. **Not re-verified by this pass** | `0062` + four probes | A lifecycle decision, then ~½ day; plus a `tests/shared/` target gated on the existing `SHARED_MODULE` conditional if it is to be guarded |
| 6 | ⚠ **`.save TIME` against a tran whose scale is `time` is silent** under `distinguish` — the twin scan skips the reference vector | **only `receipts/stage5-crewA4.md` and this file**; `0057` Resolution 9 does *not* list it | ~10 lines beside the `dataNames[]` scan + a case in an existing deck |
| 7 | ⚠ **`--enable-gc` latch leak (0058)** — `tfree()` is a no-op under `HAVE_LIBGC`, so a repeated misspelled `casemode` stays silent after a reset | **only `receipts/stage3-crewB2.md` and this file** | One line + a sentence in `0058` |
| 8 | **An event node's own case near miss is unreported** from `0057`'s site. The silence is *asserted* by a deck, so closing it is a visible change | `0057` Resolution 9; `xspice/casedist/save-event-node-case-split.cir` SPLIT case | ~15 lines (`Evt_Ckt_Case_Twin()` beside `Evt_Ckt_Has_Node()`) + a deck |
| 9 | **0063** — `removecirc` leaves `plot_cur` outside `plot_list`. Unblocked by the withdrawal; needs a fresh RED deck | `0063`; `0017` decision 2 | ~1 line in `com_removecirc()` (not in `killplot()`) + a deck |
| 10 | **0064** — a wildcard matching one vector is renamed to the wildcard. In the client's evidence base, uncaptioned as a defect until now | `0064`; `run_all.sh` section 2 | Unscoped: `ft_evaluate()` is on every control-language expression's path |
| 11 | **The `.print` route's `can't parse '%s': ignored` quotes a `gettoks()` fragment**, and **no deck shape in this tree can assert a change to it** — `ft_savedotargs()` runs only from `src/main.c`'s batch path | `0057` Resolution 9 | A harness decision (a deck shape that captures a batch run's stderr) before any code |
| 12 | **`.save @r99[i]` for a non-existent device is silent**, in stock and here. **No issue filed** | `0057` Resolution 9 only | Deserves its own issue |
| 13 | **Stale numbers in the write-ups.** `0057` Resolution 8 quotes 302 PASS and `0060` Resolution 8 quotes 303; the tree is **305**. `0056` Resolution 7's last sentence about `CLAUDE.md` is now false; `0058`'s 293/295 are dated and defensible | the issue files | 15 minutes |
| 14 | ⚠ **`make check` does not run most of `tests/`.** `vbic`, `general`, `filters`, `mos6`, `polezero`, `transient` and others are in `DIST_SUBDIRS` only, so every "suite-wide census" in this batch is over the 305 decks that run. `tests/vbic/noise_scale_test.cir` does print `can't parse 'inoise_spectrum': ignored` — identically on stock, so pre-existing | **only `receipts/stage5-crewA4.md` and this file** | Unfiled; worth an issue |
| 15 | ⚠ **Harness hazard.** `build-ver_50/src/spinit` hard-codes `codemodel /usr/local/lib/ngspice/*.cm`, so the build-tree binary on an XSPICE deck outside `make check` loads the *installed* models and SIGSEGVs in `cm_adc_bridge`. One sweep produced ~140 spurious "crashes" from this alone. Any by-hand XSPICE measurement in this batch is exposed to it | **only `receipts/stage5-crewC4.md`, `stage6-crewC5.md` and this file** | Documentation; extends the existing auto-memory note on running XSPICE decks by hand |
| 16 | ⚠ **`alter-rebin*` decks are flaky** under `doc/claude/scripts/case_differential_sweep.py` (DIFF 3/4/4 on an *unchanged* binary), which makes that tool's DIFF count meaningless run to run | **only `receipts/stage3-crewB2.md` and this file** | Unfiled; worth an issue |
| 17 | **F5 (`.spiceinit` beats `-D casemode=`) has no decision record**; F9 (two identifiers folding together silently) needs a decision before any code; F6 folds into the open `0048` | `LEDGER.md` "Not in this batch" | Three short decision docs |
| 18 | **134 stray files under `tests/`** — sub-decks and captures written by the report decks when run by hand in `srcdir`. They are in the build directory's `CLEANFILES`, not in the source tree's. Review `git clean -nxd` before committing, not after | this file | Minutes, but it must happen before `git add` |
| 19 | **0067 unfixed, and it is the sharpest thing this batch leaves behind.** `cp_remvar()` frees a node three of its five `switch` arms have already disposed of. `unset curplot` / `unset plots` → `rc=134`; `source` + `set temp=27` + `unset temp` → `rc=134`; `load` + `unset <key>` + `set`/`display`/`write` → **`rc=139`**. All deterministic, all on stock `ngspice-46`, two of the three needing no rawfile at all. **Re-measured this pass, 3/3 each, on both binaries** | `0067` (Open); `0065` Left, both bullets | One line's worth of ownership rule at `variable.c:671`-`:672`, modelled on `cp_vset()`'s `if (v_free)` guard, **plus** criterion 4's behaviour question (what `unset curplot` *should* do) and criterion 5's bogus `Internal Error: var %d`, which prints a character's ordinal. Three `pipe/` sequences, one per arm |
| 20 | ⚠ **Two issue bullets are now understated, in the safe direction.** `0060` Left says the shared-library route is "documented and unexercised" and `0065` Left says "the shared build is still not exercised" — both were exercised, by crew D's verifier and crew E's verifier respectively, in `--with-ngshared` trees they built themselves | **only `receipts/stage8-crewD.md`, `stage8-crewE.md` and this file** | Two sentences; no code |
| 21 | **The pin for 0066 (and for 0062) is untracked.** `make check` cannot reach either defect — nothing under `tests/` links `libngspice` — and both issues name probes under `doc/claude/feedback/ngspice_upstream/repro/shared/` as the substitute. That directory is untracked in its entirety, so the pin vanishes with the working copy unless C0 is committed; and even committed there, nothing runs it | `0066` criterion 4 + Left; `0062` criterion 5 | A decision on where a probe that cannot live under `tests/` belongs. A `tests/shared/` gated on the existing `SHARED_MODULE` conditional is the obvious answer and nobody has argued it |
| 22 | **`shared_exit()` calls `bgtr()` through an unchecked pointer** at `sharedspice.c:2198` while guarding `ngexit()` at `:2200`, so a host that passes `NULL` for `BGThreadRunning` — which `ngSpice_Init()` accepts — segfaults inside the error path. Pre-existing, upstream, found only because crew F's probe tripped over it. **No issue filed** | `0066` Left; `receipts/stage9-crewF.md` | Two lines. Deserves its own issue first |
| 23 | **`Option: sourcepath=…` still breaks the reader's own `add_to_sourcepath()`** — `Error: sourcepath is a read-only variable.` then `cp_vset: Internal Error: it was already there too!!`, on **every binary tested** including stock, whenever the deck is sourced from a directory genuinely new to `sourcepath`. This is the `US_READONLY` arm again, from the *write* side; `0067` owns it from the `unset` side. **No issue filed** | `0061` Left (behaviour change 2, falsified); `receipts/stage9-crewF.md` | Belongs to the `Option:` arm's own issue, which does not exist yet |
| 24 | **A rawfile may write `Option: curcasemode=…` and never have it read back**, while every other `Option:` key still answers, and nothing warns. Required by `0060` criterion 6; a load-time warning was **measured to misfire** on a `load`→`write`→`load` lap of a file ngspice wrote itself. Recorded as a sentence, deliberately | `0060` Left, with both laps measured | None, unless someone reopens the warning question |
| 25 | **`Command:` in a raw header still reconfigures the next netlist read**, by design: `cp_evloop("set …")` writes the global `variables` list, ahead of everything `0061` narrowed. Same on stock; documented at `man/man1/ngsconvert.1:109` | `0061` criterion 4 + Left | Its own issue and its own compatibility argument |
| 26 | **`set casemode=…` is still refused while a plot carrying that key is current** (`cp_usrset()`'s `US_READONLY` arm, untouched). No longer a parsing hazard after `0061`; still a nuisance — the only way to override the header is to `setplot` off that plot first | `0061` Left | Narrowing that arm is a change to what `set` may write; wants its own argument |
| 27 | ⚠ **The reader-guard depth count is a plain `static int`** with no synchronisation, and `bg_` puts a netlist read on a second thread. Not reachable today (`runc()` starts at most one worker, the API is documented as one call at a time) and a property of `0061`'s design rather than of `0066`'s fix | `0066` Left; `receipts/stage9-crewF.md` | Nothing today; a note in `0061` if anyone adds a second reader thread |
| 28 | ⚠ **Two small doc inaccuracies shipped.** `0066` and the probe's own header describe `netlist_guard_probe.c` as "committed with this issue" — nothing in this batch is committed. And `inpcom.c`'s comment justifies a count over a flag by naming the XSPICE auto-bridge as a caller that "reads a deck ngspice writes itself" — true, but it is not a nesting path, so it is not evidence for the count | **only `receipts/stage9-crewF.md` and this file** | Two sentences |

---

## 4. What this batch got wrong

**Eight failures.** Six of them share one shape, stated below; stages 8 and 9
added two more that do not, and they belong here beside the two guards because
they are the same kind of thing one layer down — **a measurement that answered
a question nobody had checked was the question being asked.** Everything below
was measured by somebody in the batch and is re-checkable; failures 7 and 8
were re-measured by this pass.

### The shape, stated once

**When verification falsifies a guard, the batch widened the guard instead of
asking whether the contract was right.** Both guards — `0057`'s report and
`0059`'s refusal — were re-verified three times, and each round the answer was
one more clause. Nobody asked what the condition was *supposed* to be until
round four, and in both cases the answer had been written down in the tree
before the batch started: for `0057`, decision 2 of
`doc/claude/decisions/0001-distinguish.md` at `:76-78` conditions this family
of warning on the resolution failing **and** a case variant existing, which is
the narrowing that finally worked; for `0059`, nothing licensed refusing a plot
because of *how it became current*, which is the premise all three guards
shared and which row 10 falsified.

A guard list that grows once per verification round is not a converging fix. It
is a signal that the predicate is keyed on the wrong thing, and the specific
wrong thing was the same both times: **the syntax of the request rather than
its subject.** `0057` tested "did this token fail to resolve" instead of "is
this a case mistake"; `0059` tested "did the deck type a vector list" and then
"did the deck type the word `all`" instead of "does this plot hold anything
this session put there".

### The eight failures

**1. `0057`'s report was widened three times before it was narrowed.** Each
round removed one class of false positive and revealed another: a
multi-analysis deck → a `.noise` column only the plot the analysis opens
*second* lacks (`onoise_total`) → an XSPICE event node, which is not on
`ckt->CKTnodes` → a token `gettoks()` had already reduced to a fragment
(`dout(state)` arriving as `state`) → **two decks this repository ships and
tracks**, `examples/transient-noise/noi-ring51-demo.cir` and
`examples/probe/F5TurboV2-Probe.cir`. The last one is the tell: the census that
was supposed to catch new noise covered `tests/` only, so shipped `examples/`
decks acquired warnings for three rounds without anybody noticing.

**2. `0059`'s guard was widened three times and then refused correct work.**
Attempt 1 keyed on "the plot was never chosen" and was falsified by a plot that
*was* chosen and was empty. Attempt 2 added the empty arm and was falsified by
`write f.raw all`, the argument the command supplies for itself. Attempt 3
matched the token `all`, case-folded, and was walked past four ways in one
afternoon (`ally`, `ALLY`, `"all"` in double quotes — single quotes *were*
stripped and refused, so the guard was inconsistent between quoting forms —
and `all all`). Then the fatal one: a plain nutmeg session,
`let x = vector(5)` / `let y = x*2` / `write out.raw`, was **refused, in
`--batch` and through `ngspice -p`**, where stock and a HEAD baseline both
write a 3275-byte file. That is the workflow `ngspice -p` exists for, and it
took three rounds and a verifier's own deck to find, because every deck the
crews wrote started with a circuit.

**3. A test passed with its own mechanism removed.**
`tests/regression/misc/destroy-removed-circuit.cir` passed identically with its
`removecirc` line commented out — it did not pin what it claimed to pin. The
same class again: `0056` criterion 2 — the one comparison the issue
deliberately left byte-exact — had **no deck at all**, and a verifier found it
by mutating `name_eq()` to `vec_name_eq()` and watching the entire suite pass.
Two guards in this batch were protected by nothing, and in both cases the way
it was found was mutation, not review.

**4. The staged index could not build, twice, and it defeated the one
sequencing claim the batch cared about.** At stage 3 the `outitf.c` change was
staged while the `identity.baseline` it requires was not, so the staged tree
failed its own lint; the `Makefile.am` hunks registering three new decks were
unstaged, so the staged tree added decks nothing ran. At stage 4 the index was
worse: `make check` on it exited 2 after 51 PASS and **aborted at
`tests/regression/misc`**, never reaching `tests/lint` — the directory the
sequencing claim is about. The index was a shared staging area across three
crews working in one tree, which is not a commit and cannot be one. The
orchestrator cleared it at stage 4; stage 7 then put thirteen `.out` files back
in it, which is again not a commit.

**5. The issues' own fixes invalidated the evidence they quote.** `0056` made a
`.save` name resolve under `preserve`, so **six decks in `0059`** and most of
`0057`'s transcripts stopped reproducing under the flag their own text named.
The repair — re-pinning every transcript under `distinguish`, where the numbers
reproduce exactly — took a whole polish stage, and the files now carry
"historical, at `720c8743a`" banners in three places each. An issue that quotes
a transcript pins itself to a binary; when three fixes land in one working
tree, every transcript in every sibling issue is a claim about a binary that no
longer exists.

**6. Two of the four `0059` routes were found *after* the guard was withdrawn,
and one of them falsified the argument the withdrawal was written on.** `set
plainwrite` takes a second branch of `com_write()` that no attempt ever read;
`wrdata` reaches the same fallback from a different command and was proved
unguardable from `com_write()` by compiling a guard in and watching `wrdata`
walk past it; `set appendwrite` puts the constants *inside* a real run's
rawfile, which falsifies "the bad file at least labels itself" — the sentence
the trade rested on. Three rounds of implementation and four rounds of
verification never read the other branch of the function they were editing.

**7. An issue this batch filed understated its own defect as non-crashing,
because it was measured under `valgrind`, whose allocator masked the fault.**
`0067`, filed at stage 9, described its `US_READONLY` arm as "latent, allocator
dependent, and today usually silent", said it "does not crash today", and
explicitly overruled `0065`'s earlier `rc=139` as not reproducing. Every part
of that was wrong, and the way it went wrong is worth the space.

The evidence was a two-command sequence — `load`, `unset zzz` — which really
does return `rc=0`. The `unset` is not what dies. What dies is the next command
that walks the plot's environment, and the issue's own quoted command line
already had one in it. Re-measured by this pass, three runs of each, on
`build-ver_50/src/ngspice` and on stock `/usr/local/bin/ngspice` alike
**[measured]**:

```
load hdr_zzz.raw ; unset zzz ; quit 0                    rc=0    3/3  both
load hdr_zzz.raw ; unset zzz ; set ; quit 0              rc=139  3/3  both
  ... under valgrind -q, same command line                rc=0, 34 invalid reads
unset curplot ; quit 0        (no rawfile at all)        rc=134  3/3  both
```

The third line is the whole of the error: **the crash was measured with a tool
underneath, and the tool's exit status was reported as the program's.**
`valgrind` does not return a freed block to anyone, so the walk that segfaults
under `glibc` merely reads what is still sitting there. And it masks all three
of the issue's arms, not just this one — `unset curplot` under `valgrind` is
also `rc=0`, with 3 invalid frees, where bare `glibc` aborts it.

The internal contradiction should have caught it before any verifier did:
`0067`'s acceptance criterion 2 asks that `set`, `display` and `write` after
the `unset` be clean — it names the exact three commands that segfault — while
its Impact said the symptom does not crash. A maintainer triaging it would have
ranked an immediate deterministic `SIGSEGV` on the ordinary next command, in a
released ngspice, as low priority. It is corrected in place, and the corrected
text says how the error was made rather than quietly replacing the numbers.

**8. A fix's own state-tracking leaked on an error path that no test in this
tree can reach.** `0061`'s narrowing is a depth count raised and lowered by
`inp_readall()`. A netlist read that ends fatally — a missing `.include` — does
not leave through `inp_readall()`. In the standalone binary that is harmless;
in a `--with-ngshared` build `controlled_exit()` calls `shared_exit()`, the
stack is discarded, and the count stays raised for the rest of the host
process, from which point `plot_cur->pl_env` answers **no** `cp_getvar()` ever
again. The fix's own narrowing, applied permanently and to everything, at the
first bad deck.

Three things make this worth listing beside the guards rather than filing it as
an ordinary bug:

- **`0061`'s Resolution had already claimed the paths were covered.** It named
  two reset sites and said they cover "the paths that leave `inp_readall()`
  without returning through it". There are three. The third, `bg_source` →
  `pthread_exit()`, lands at no `setjmp` at all — so the two landing sites were
  the wrong *place* for the fix, not merely an incomplete list, and the repair
  is one line at the point common to all three.
- **No harness in this repository can reach it.** Nothing under `tests/` links
  `libngspice` and `build-ver_50` is not a shared build, so `make check` never
  executes `shared_exit()`. The suite was 305/0 with the defect in the tree and
  is 305/0 with it fixed. It was found because a verifier built a
  `--with-ngshared` tree to check something else.
- **It is the class `CLAUDE.md` names in so many words** — *anything touching
  global or static simulator state must survive repeated `ngSpice_Reset` in the
  shared build* — with two commits cited as precedent. The rule was in the
  contributor guide the whole time, and the crew that added a file-static
  counter did not apply it. `0058` in the same batch *did* (its latch is reset
  from `totalreset()`), which makes this an inconsistency inside one batch
  rather than an unknown.

### What a next batch should do differently

1. **Cap the widening at one.** If a guard needs a second clause because
   verification found a correct deck it fires on, that is information about the
   contract, not about the implementation. Stop and re-derive the condition —
   from the decision record if there is one, and from what the *subject* of the
   request is if there is not.
2. **Write the negative decks first, and from outside the intended workflow.**
   Every crew deck in this batch began with a circuit. The two shapes that
   killed the two guards were a shipped `examples/` deck and a nutmeg session
   with no circuit at all. A guard's RED set should include: no circuit; the
   command's default argument spelled out; the same command through
   `ngspice -p`; and the whole of `examples/`, not only `tests/`.
3. **Read every branch of the function you are guarding.** `com_write()` has
   two, and `plainwrite` was in neither the issue nor any deck in the tree. One
   grep of the enclosing function would have found it.
4. **One crew per file, or one file per crew.** Three crews in one working tree
   produced an index that could not build twice, five `Makefile.am` files whose
   `TESTS` lists interleave three commits, and one C file that has to be split
   by hand between two commits. If crews must share a file, the split has to be
   defined before the edits, not reconstructed afterwards.
5. **A commit split is a claim; materialise it.** The first commit of this
   batch was materialised and built twice, and the first attempt did not
   compile. Middle commits are cheaper to check while the tree is fresh than
   after it is committed.
6. **Test the reference files, not just the decks.** `tests/.gitignore` hides
   `*.out`. A new deck whose `.out` was never `git add -f`-ed produces a commit
   that fails `make check` for everyone else and passes for its author. That
   trap caught this batch once per new deck — fifteen times, thirteen at
   stage 6 and two more at stage 8 — and was found by a verifier, not by any of
   the seven crews that added a deck.
7. **Take a crash's exit status with nothing underneath it, and use `valgrind`
   only to say what the memory did.** Failure 7 is one command line run one way
   and reported the other. The rule generalises past `valgrind`: when a tool
   changes the allocator, the scheduler or the address space, its exit status
   is the tool's answer to a different question. Quote both, and label which is
   which.
8. **When a fix adds state, ask what happens to that state on the paths that
   do not return.** Failure 8 was reachable by reading `controlled_exit()`,
   which the fix's own comment already cited. `CLAUDE.md` states the rule for
   the shared build in so many words, and one crew in this same batch followed
   it. A checklist item — *what resets this, and which paths skip the reset?* —
   would have caught it in review rather than in a verifier's own shared build.
9. **Build a `--with-ngshared` tree once per batch, whatever the batch is
   about.** This one found `0062`, `0066` and the unguarded `bgtr()` — three
   defects, one of them the batch's own — and `make check` is structurally
   blind to all of them, because nothing under `tests/` links `libngspice`.

### What went right, and is worth keeping

Adversarial verification found every one of the eight failures above, and the
crews found none of them. Verifiers who *built* the counterfactual — the
rejected latch rule in `0058`, the nine-mutation matrix in `0057`, the guard
compiled in to prove `wrdata` bypasses it, the shared library built to test one
line of `0061` — produced findings that survived re-checking; verifiers who
only read produced findings that had to be re-measured. Stage 9 extends the
lesson one step: its verifier caught not a defect in the code but a defect in
how the code had been *measured*, which is the harder catch and the one no
amount of reading finds.

Two other things are worth keeping. **Both stage-8 fixes are rules rather than
name lists** — "a plot's environment does not answer a read taken while a
netlist is being read" and "what this function computes, it answers itself" —
which is exactly what §4's shape says the batch failed to do twice with its
guards; the same batch that made that mistake twice avoided it twice at the
end, after the owner insisted on it in the decision. And **every fix in stages
8–9 was measured with its own mechanism disabled**, one edit at a time, so no
deck in this batch's late work is passing on a neighbour's mechanism.

The final state: six issues fixed and closed, three open and filed, one
withdrawn with its issue honestly open, two more filed about defects the batch
did not cause, one of those fixed. Working tree `make check` **305/0**
**[measured]**, nothing committed, and the first of eleven commits built and
proved RED-and-GREEN in a throwaway tree.
