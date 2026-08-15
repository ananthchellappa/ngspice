# Overnight batch — casemode defects from the xschem client feedback

Source: `doc/claude/feedback/ngspice_upstream/FINDINGS.md` (nine findings, all
measured 2026-08-12 against `build-ver_50/src/ngspice`, `ngspice-46+`).

Main session orchestrates only. Every issue write-up and every fix is done by a
subagent crew; receipts land in `receipts/`.

Marks: `[x]` done · `[F]` failed · `[D]` deferred · `[ ]` not started.

## Stage 1 — file the issues

All six written and all six adversarially checked. Every checker returned
**NEEDS_WORK**, and every checker also confirmed the central finding of the
issue it was attacking reproduces. The defects were in the write-ups, not in
the defects: decks whose output was quoted but never committed, citations off
by a few lines, a handful of overstated or false sub-claims, no
cross-references between the six, and one direct contradiction between 0058
and 0060 over whether `.include` is an `inp_readall()`.

Receipts: `receipts/stage1-<n>.md`. Remediation dispatched (stage 1b).

Stage 1b remediated all six against their problem lists and re-checked. Four
signed off; two came back with residuals and went to a stage 1c polish pass.
Receipts: `receipts/stage1b-<n>.md`.

- [x] 0056 `.save` matches vector names byte-exactly in every case mode — 8 problems worked, signed
- [ ] 0057 a failed `.save` names no token — 10 worked, 2 residual + 1 regression, in polish
- [x] 0058 casemode diagnostics fire once per file read, not once per run — 10 worked, signed
- [ ] 0059 `write` emits the constants plot when no analysis ran — 10 worked, 2 residual + 1 regression, in polish
- [x] 0060 `$casemode` reports the request, not the effective mode — 11 worked, signed
- [x] 0061 a rawfile `Option:` line reconfigures the next netlist read — 13 worked, signed

What the checkers changed about the batch itself:

- **0056's scope was wrong in the original brief.** The byte-exactness is not a
  `preserve` defect. `save v(MIDNODE)` typed at the prompt under `casemode=fold`
  kills the run too, and stock `ngspice-46` fails identically — it is a
  pre-casemode defect on every route the reader's fold does not cover.
- **0059's trigger was overstated.** The constants-plot write needs *every*
  `.save` name to miss. A partial miss exits 0, writes a real plot, and drops
  the vector silently — worse, because nothing signals at all. Reachable with
  no failed analysis at all, via `op` then `destroy` then a bare `write`.
- **0057 found the diagnostic already half-exists.** `outitf.c:419` emits
  `Warning: can't parse '%s': ignored`, gated on `saves[i].analysis`. `.save`
  cards arrive with that field NULL, so it is suppressed; `.print`- and
  `.meas`-derived saves set it and do fire.
- **Batch contradiction (b) settled by measurement.** `.include` and `.lib` are
  *not* an `inp_readall()` (they recurse below it); `source` is. 0058 was
  right, 0060 now agrees.

## Stage 2 — fix the defects

Grouped by the file each crew edits, and run **sequentially**, because three of
the six pairs touch a shared file and a parallel edit in one working tree would
corrupt it.

All three crews returned code, full `make check` green in each case (292 / 293 /
294 PASS, 0 FAIL), and all three verifiers returned NEEDS_WORK. Receipts:
`receipts/stage2-crew<X>.md`. Stage 3 dispatched against the three problem
lists.

- [~] Crew A — 0056 + 0057 · `outitf.c`, `runcoms.c` — the `.noise` regression is
  fixed; a **new one** took its place, see below. In stage 4.
- [x] Crew B — 0058 · `inpcom.c`, `fteext.h`, `sharedspice.c` — **signed** at
  stage 3 by an independent verifier who re-ran the whole suite from cleared
  logs. **0060 deferred.**
- [~] Crew C — 0059 · `plotting.c`, `vectors.c`, `postcoms.c`, `dotcards.c`,
  decision 0017 — the `killplot()` placement bug and the NULL invariant are
  fixed; three residuals in stage 4. **0061 deferred.**

### Stage 3, what the verifiers caught

- **A regression replaced a regression.** The `.noise` false positive is gone —
  `ckt_knows_name()` now stays quiet when the circuit has a name and only this
  analysis lacks the column. But it walks `ckt->CKTnodes` only, and an XSPICE
  event node does not live there, so a correct mixed-signal deck saving an
  event node now warns. Same class its own criterion 9 forbids. Stage 4 has it.
- **The staged index does not commit.** `tests/lint/identity.baseline` is
  unstaged while the `outitf.c` change requiring it is staged; the staged tree
  alone fails the lint. That also defeats 0057 criterion 8's sequencing plan.
- **A test that passes without its own mechanism.**
  `misc/destroy-removed-circuit.cir` passes identically with `removecirc`
  commented out — it does not pin what it claims to pin. *(Settled at stage 6:
  the deck is **deleted**, along with the three other decks that pinned the
  withdrawn guard. It existed only to serve attempt 1's `killplot()` ordering,
  and it also asserted that `Internal Error: kill plot -- not in list` **is**
  printed, which `doc/codex/issues/0063` wants to stop being true — so deleting
  it discharged that issue's criterion 4 as well. Nothing in `tests/` names it
  now.)*
- **`op`, `setplot new`, bare `write`** still writes the 570-byte constants raw
  silently. Named in neither 0059's matrix nor decision 0017's "does not
  decide" list.

## Stage 4 — close out

- [~] Crew A3 — event nodes fixed via `Evt_Node_Name_Eq()` with a 4×4 mutation
  matrix; **two further false positives found**, see below
- [~] Crew C3 — hollow pin closed, `setplot` hole recorded in decision 0017;
  a **bypass** found in the guard
- [x] 0062 filed — and the filer **overturned the attribution it was given**.
  `CKTmodCrt()` is a symptom, not the cause: `totalreset()` is a de-initialiser
  by design (`d0ae65acc`'s own message says so, and `examples/shared/shx.c`
  re-`Init`s after every reset), but nothing states or enforces it —
  `ngSpice_Command()`'s guard returns 1 in silence and the `no_init` message 23
  lines below is dead code. Which frame dies is selected by a function-static
  cache at `inp2v.c:20`, so a fix aimed at `CKTmodCrt()` would relocate the
  crash rather than remove it. Minimal repro is four API calls.

## Stage 5 — converge (final fix round)

The 0057 diagnostic did not converge. Three rounds each removed a class of
false positive and revealed another: multi-analysis decks → `.noise` columns →
XSPICE event nodes → `dout(state)` → two **tracked example decks**. A guard
list that grows once per verification round is the wrong shape, so this round
narrows the report instead of guarding it again.

- [x] **Crew A4 — SIGNED, zero false positives remaining.** Narrowing worked:
  the report now fires only under `distinguish`, only when the token misses and
  a name differing from it only in case exists. All six false positives went
  silent structurally, including the two tracked `examples/` decks. Verified by
  a 9-mutation matrix, each mutation applied in place and rebuilt: M9
  (restoring the wide report) fails all seven of 0057's decks; M1
  (`name_eq`→`vec_name_eq`) fails the new `plot-scale-name-twin.cir`, which is
  the deck 0056 criterion 2 lacked. Suite 302 PASS, 0 FAIL. Its own verifier
  found one gap the crew had not listed — `.save TIME` against a tran whose
  scale is `time` is silent, because the twin scan skips the reference vector.
- [F] **Crew C4 — not signed. Its widening refused correct work.** A plain
  nutmeg session that builds its own vectors and writes them —
  `let x = vector(5)` / `let y = x*2` / `write out.raw` — is refused by the
  delivered binary and written as a 3275-byte file by stock. Same through
  `ngspice -p`. A HEAD baseline behaves like stock, so this batch introduced
  it. The verifier also walked past all three refusals with four different
  requests for the whole current plot.

  **The premise was wrong, not the implementation.** "Refuse when the plot is
  `constants`" assumed that plot is always garbage. It is where a nutmeg
  session's own `let` vectors live. Keying on the plot's identity, or on the
  argument's spelling, cannot work — 0064, filed by this same crew, already
  records that `all`/`allv`/`alli`/`ally` are four spellings of one request.

## Stage 6 — resolve 0059

- [D] **Crew C5 took route B: the guard is WITHDRAWN, 0059 stays Open.** The
  write path is now byte-identical to HEAD — verified by construction, `git
  diff` empty across `postcoms.c`, `plotting.c`, `vectors.c`, `dotcards.c`,
  `spec.c`, `com_fft.c` and `NEWS` — so the regression is gone and no new
  behaviour ships. All three attack decks write again. `plot_cur_chosen` went
  with it: 14 assignments and no readers left, and it was state a shared build
  would have had to reset.

  It proved route A impossible before writing any code. A correct predicate
  needs three clauses, and the third — *"is this a request for the whole current
  plot?"* — has no home. Before resolution the wordlist is unparsed, so matching
  text means re-implementing the resolver, which is the loop that produced three
  rounds of missed spellings. After resolution the information is destroyed:
  `write f.raw all` reached via the fallback and `write f.raw const.all` (which
  must write) produce an identical dvec list from an identical plot.

  **Its verifier then found three more routes, and they settle it.**
  `set plainwrite` takes a second branch of `com_write()` that no attempt ever
  saw. `wrdata f.dat all` reaches the same fallback from a different command
  and was proved unguardable from `com_write()` — with a guard of the withdrawn
  shape compiled in, `write` was refused and `wrdata` wrote the constants
  anyway. And `set appendwrite` puts the twelve constants *behind a real run's
  Title and first Plotname*, which falsifies the sentence the withdrawal trade
  rested on: the bad file does not always label itself. The defect is worse
  than 0059 documented, and a guard would have needed four routes across two
  commands.

- [x] Closeout written: `CLOSEOUT.md`, 685 lines, both stale drafts discarded
  rather than patched. Commit C1 materialised under `/tmp` and built —
  `autogen.sh` → `configure` → `make -j8` → `make check` all rc=0, 291 PASS,
  0 FAIL — with RED confirmed in the same tree by reverting `outitf.c` to HEAD.
  C2/C3/C4 are reasoned, not built, and the report says so.

- [~] ~~Crew C5 — one item, two acceptable outcomes, chosen on evidence:~~
  **(A)** a predicate keying on the *subject* — refuse only a plot holding
  nothing this session put there, so the failed analysis is refused and the
  `let` session is written; or **(B)** withdraw the guard, restore HEAD
  behaviour, and rewrite 0059 to record that the defect is real, that the
  discriminator was attempted three times, and what a correct one would need.
  B is not failure: a guard that refuses a legitimate `ngspice -p` session is
  worse than the defect, which produces a bad file where this produces no file
  for correct work.
- [ ] Closeout refreshed against final state.
- [ ] Closeout report — an independent pass over every receipt and issue,
  a buildable commit split verified by materialising the first commit, and an
  unsparing account of what this batch got wrong.

**The index was cleared** (`git reset`, working tree untouched). Several crews
had staged partial work, and the staged tree could not build: `make check` on
it aborted at `tests/regression/misc` after 51 PASS, because staged decks
depended on unstaged `src`. Everything is now a working-tree change and the
commit split is documented rather than staged. Nothing has been committed —
that is the repo owner's call.

What the verifiers caught, worst first:

- **A regression, crew A.** A correct multi-analysis deck now emits warnings:
  `.save v(out)` + `.noise v(out) v1 dec 10 1 100` + `.print noise all` runs to
  completion, rc=0, full output — and prints two `Warning: no vector named out`
  lines. A deck saves names that only some of its analyses have. Being fixed.
- **A bug in crew C's own hunk.** `postcoms.c:1109-1111` clears
  `plot_cur_chosen` *before* the unlink, and the branch below has an early
  return on the "not in list" path. *(Moot at stage 6: the guard was withdrawn
  and `plot_cur_chosen` was removed entirely — the definition, the extern and
  all 14 assignments — so there is no clear left to place. `killplot()` is
  byte-identical to HEAD. The ordering itself is worth keeping in mind only if
  a fourth attempt rebuilds an equivalent flag; 0059's Resolution records the
  design for that reason.)*
- **A falsified acceptance criterion.** 0057 criterion 6 ("fires once per
  unresolved token, not once per `OUTpBeginPlot()` call") was marked Met and is
  not — the report hangs off `beginPlot()`, so the count tracks the call.
- **The fixes invalidated the issues' own evidence.** `.save v(midnode)` now
  resolves under `preserve`, so six decks in 0059 and most of 0057's
  transcripts no longer reproduce under the flag they name. `distinguish`
  reproduces every number exactly (`vec_name_eq()` is exact there, which is
  what the old `strcmp` was in every mode), so they re-pin rather than rot.

### Deferred, then decided by the repo owner 2026-08-13

- [x] **0060 — approved, named `curcasemode`.** The name sits in the existing
  computed read-only family — `curplot`, `curplotname`, `curplottitle`,
  `curplotdate`, `plots` — all computed on read in `cp_enqvar()` and refused on
  write through `cp_usrset()`'s `US_READONLY` arm, so it needs no new
  convention. Criterion 4 is what the client actually wanted: on a build
  without the feature the read *fails* rather than answers, and
  `Error: curcasemode: no such variable.` is the capability probe that no
  identity-based probe can be, because `preserve` folds identity exactly as
  `fold` does.
- [x] **0061 — shape A.** The `Option:` pair keeps landing in the plot
  environment, so a value can still be read back out of a header — finding 1's
  actual ask — but the reads that steer parsing stop consulting it. The
  objection to shape A was that it manufactures a fresh instance of 0060's
  split; that is retired by `curcasemode` shipping first, because one reading
  is then labelled the request and the other the effect. The narrowing must be
  a rule about which reads a plot environment may answer, not a list of
  variable names: `ngbehavior` is the same defect, and this batch has already
  shipped a name-list predicate twice.
- [x] **0065 filed and fixed with 0061** — see below.

## Not in this batch

- [D] F5 `.spiceinit` beats `-D casemode=` — not a defect. Deliberate per the
  comment at `src/frontend/inpcom.c:1079`; what is missing is a `decisions/`
  entry weighing it, which is a judgement call and not an overnight task.
- [D] F9 two identifiers folding together is silent — needs a decision first
  (a warning is noise under `fold`; the argument only holds under `preserve`).
  `decisions/0001` decision 2 already declines to warn on a definition.
- [D] F6 near-miss check on `-D` variable names — already covered by the open
  `doc/codex/issues/0048-control-variable-names-are-compared-inconsistently.md`.
  Fold it in there rather than filing a duplicate.
- [D] F1 record the case mode in the raw header — a format change, so it wants
  a decision doc first. Its obvious carrier is disqualified by 0061: an
  `Option:` line is not inert metadata, it reconfigures the reading session.
  — **superseded 2026-08-14.** The repo owner decided it, and the carrier was
  no longer disqualified: 0061's narrowing shipped first, so an `Option:` line
  no longer reconfigures anything. `raw_write()` writes
  `Option: casemode=<mode>` after `Plotname:`, valued from
  `inp_case_mode_name()`. Two decks, `make check` 307 PASS. The measurements
  are in 0061's addendum and the note at the head of finding 1.

## Stage 10 — the client's round 2

The xschem session replied at
`doc/claude/feedback/reply_from_xschem_session/REPLY.md` with six findings and
four questions, all re-measured here against `build-ver_50/src/ngspice` (build
stamp `Fri Aug 14 20:52:09 UTC 2026`) by running their `repro2/run_round2.sh`
unmodified. All six reproduce. Documentation and issues only; no `src/` or
`tests/` in this stage.

The four answers, all the repo owner's:

| question | answer |
| --- | --- |
| `Option: casemode=` in the raw header | **yes, shipped** (crew G) |
| `distinguish` keeps `.save` byte-exact | **yes, permanent contract** — 0001 decision 5's withdrawal list |
| warn on a case collision | **yes, all three modes, shipped** (crew H, decision 0018) |
| exit status inside `.control` | **no change** — `$sim_status` instead, measured working on stock |

What each finding got:

- **R1** (rc=0 for the failure inside `.control`) — filed as
  `doc/codex/issues/0069`. Their diagnosis needed one correction: the variable
  is `-r`, not `.control`. A `.control`-only deck exits 1; a deck with an
  analysis dot card *and* a `.control run` runs the analysis twice and exits on
  the second. The owner's decision and the `$sim_status` guard are the issue's
  Resolution, measured in their deck shape and on stock.
- **R2** — corroboration added to `0059`, with their four-row table and the
  note that its rc=0 rows are 0069's second epilogue arm rather than a conflict
  with 0059's rc=1 transcripts. Withdrawal untouched; priority raised.
- **R3** — already `0064`. Their independent reproduction and their sharper
  isolation of the trigger (the deck's *total* saved-vector count being one)
  are now in it, plus the interaction with the vector-count floor 0059/0069
  ask a consumer for.
- **R4** — recorded in `0057` criterion 6 and Resolution 6 as a known
  interaction: the five-row table showing lines == simulations, the contract
  stated plainly, and the one-line deck edit that avoids it. Not fixed; a
  memory outliving the simulation would silence a deliberate re-run.
- **R5** — theirs, and ours to document. New subsection of
  `doc/claude/casemode-distinguish-guide.md` §9 and an addendum to `0060`
  confirming both properties they relied on are intended. The payoff nobody had
  seen: the new header line catches a wrong-cwd probe after the fact.
- **R6** — independent confirmation noted in `0067`, which is the scrutiny
  RESPONSE.md's own "least-scrutinised" flag asked for.

`doc/claude/feedback/ngspice_upstream/RESPONSE.md` rewritten as the round-2
reply. It opens by **correcting round 1's own advice** — "`rc` and a
vector-count sanity check are still worth having", and "`rc` — the obvious
defence" — which R1 falsifies for their deck shape. The vector-count and
header checks are kept, because those do still hold.

## Stage 11 — crew H's residuals (crew L)

Crew H shipped the node-name collision report in all three modes and its
verifier signed the mechanism. Three residuals and one question for the owner
went to crew L. No behaviour changed in this stage: the diff under `src/` is
comments only.

- [x] **P1 — decision 0018 decision 3 was wrong about its own cause, and the
  gap is three times the stated size.** Item 3 blamed `card_word_boundary()`
  for leaving `+ - * /` inside words. Falsified twice: `(` and `)` **are**
  boundaries, so `card_spelling()` compiled out of `inpsymt.c` verbatim
  returns `VFreq` from the exact line of `examples/various/FFT_Leakage.cir`
  the item cites; and the silence survives on `B1 mid 0 I = 1m`, where the
  second spelling is a plain node field with no expression anywhere near it.
  The real cause is `inp_bsource_compat()` (`src/frontend/inpcom.c`), which
  comments **every** B card out and inserts a replacement built by
  `insert_new_line()` — no `line_case`, so no node named anywhere on a B card
  can supply a spelling under `fold`, terminals included. `inp_compat()` does
  the same to an expression-valued `E`, `G`, `R`, `C` or `L`; measured one
  deck per shape, with a plain linear `E` as the control that still reports.
  **236 B cards in 53 of the 875 decks.** A fourth gap was found while
  re-deriving: `[` and `]` are not boundaries either, so an XSPICE `a` card
  written `[in1 in2]` yields no spelling where `[ in1 in2 ]` yields one — no
  corpus deck is affected, and it is recorded rather than fixed.
- [x] **P2 — acceptance criterion 5 was marked Met with nothing testing it.**
  Confirmed: with both `t_unclaimed` guards deleted from `INPtermCaseCheck()`
  and the tree rebuilt, `tests/regression/case` (118) and
  `tests/regression/casedist` (30) both passed in full. Three cases added —
  NMDEF and NMREF in `casedist/node-case-collision-report.cir`, NMPAIR in
  `case/node-case-collision-report.cir` — each asserting a count rather than a
  presence. Against the same mutation each directory now fails on exactly the
  one deck, and each guard was then deleted **on its own** and rebuilt to show
  neither case is redundant: outer-only fails NMDEF and NMPAIR and leaves
  NMREF passing, inner-only fails NMREF alone. Two cases under `distinguish`
  rather than one because the bucket chain is prepended to, so interning order
  decides which of the two guards is asked. ~~Nothing is owed by
  `tests/regression/misc`: `t_unclaimed` is set only from a B source
  expression and a B card has no `line_case`, so under `fold` the guards are
  unreachable — a consequence of P1's gap.~~ **Struck 2026-08-14 by stage 12,
  see below: that sentence is false and `fold` was the one mode the criterion
  had no guard in at all.**
- [x] **P3 — three `.out` files need `git add -f`.** `tests/.gitignore` line 3
  is `*.out`; the 117 / 28 / 34 already tracked in `case` / `casedist` / `misc`
  were force-added. See the handoff list below.
- [x] **The question for the owner: one line per instantiation, measured.**
  Under `preserve` and `distinguish` a single two-way spelling in one
  `.subckt` body is reported once per instantiation. On
  `examples/klu/Circuits/85/c1355/c1355.net`, spelling the `nand2` body's one
  internal node `sig3` as `SIG3` on a single card produces **416 warning
  lines**, one per instantiation; **454** on `c5315.net` and **1028** on
  `c7552.net`, the largest multi-instance deck in `examples/` this tree can
  parse. `not1` is instantiated 1410 times in that netlist and in each of its
  three PDK variants. A formal *pin* spelled two ways is silent — it is a
  body's internal node, which the expander scopes rather than substitutes,
  that multiplies.
  Every line is true; the question is whether the diagnostic should be one per
  *pair* or one per *mistake*. Three options and their costs are written up in
  `doc/codex/issues/0068` under "Open question for the owner"; `0018` decision
  1 costed "one line per pair" and never considered that one mistake can be
  many pairs, so the answer belongs there as an amendment.

### The nine tracked decks that now emit a line in the DEFAULT mode

`casemode=fold` is what ships. Every line is true — each deck really does
spell one node two ways — and a reviewer should see the list rather than a
count:

`examples/hicum2/hic2_ft.sp` (`c`/`C`, `b`/`B`) ·
`examples/hicum2/hic2_gain.sp` (`c`/`C`, `b`/`B`) ·
`examples/noise/baker-252-gain.cir` (`Vplus`, `Vplusa`) ·
`examples/p-to-n-examples/switch-oscillators.cir` (`VDD`) ·
`examples/p-to-n-examples/switch-oscillators_inc.cir` (`VDD`) ·
`examples/soi/Inv_chain.sp` (`out0`…`out3`, four pairs) ·
`examples/tclspice/tcl-testbench3/FB14.cir` (four pairs) ·
`examples/xspice/see/CMOSComparator/Fig27_12_see.sp` (`VDD`) ·
`tests/vbic/noise_scale_test.cir` (`VOUT`).

**`make check` stays green, and that is not evidence.** `tests/bin/check.sh`
captures stdout only (`>$testname.test`) while the report goes to `cp_err`,
and its `egrep -v` filter then drops every line containing `Warning` from both
sides of the diff. The suite would not have gone red if the report were wrong
on any of these decks either. `tests/vbic/noise_scale_test.cir` is the only
one of the nine the suite runs at all. Everything the suite says about this
diagnostic comes from the three guard decks, which reach it only by
re-entering the parse under a `>&` redirect.

### Handoff, as stage 11 left it — superseded by stage 12's table below

| file | why |
| --- | --- |
| `tests/regression/misc/node-case-collision-report.out` | `tests/.gitignore:3` is `*.out` |
| `tests/regression/case/node-case-collision-report.out` | same — **updated by this stage** with NMPAIR's two lines |
| `tests/regression/casedist/node-case-collision-report.out` | same — **updated by this stage** with NMDEF's and NMREF's four lines |

A fourth ignored `.out` is on disk in the same directory —
`tests/regression/casedist/rawfile-casemode-header.out`, from crew G's rawfile
header work, not this stage's. It needs `git add -f` for the same reason and is
named here so the commit does not lose it.

### The sweep, re-run independently

875 decks × 3 modes, re-measured on the same tree rather than taken from crew
H's receipt. `fold` 28 lines / 19 files, `preserve` 38 / 25, `distinguish`
29 / 16 — crew H's table reproduces exactly. `fold` and `distinguish` are each
a strict subset of `preserve`, confirmed line by line. **Ten** `preserve`
reports have no `fold` twin, not the nine the issue said: four are gap 1
(subcircuit body), three gap 2 (`X` card actual), three gap 3 (rebuilt card).
The "nine" was the `distinguish` side's number, which is right.

## Stage 12 — the sentence that closed criterion 5 was false (crew N)

Crew L's verifier falsified the one sentence crew L used to close acceptance
criterion 5 of `doc/codex/issues/0068`, and the sentence had already shipped in
four places. Nothing about the diagnostic's behaviour changed in this stage:
the diff under `src/` is one comment.

- [x] **P1 — the false claim, reproduced and corrected in four places.** The
  claim was that `t_unclaimed` is set only by `mkvnode()` (B source only), that
  a B card has no `line_case`, and that under `fold` both pair-report guards are
  therefore unreachable and `tests/regression/misc` owes nothing. `mkvnode()` is
  one of **seven** callers of `INPtermInsertRef()`: `INPgetValue()`
  (`src/spicelib/parser/inpgval.c`, an `IF_NODE` value) and `dot_noise()`,
  `dot_tf()`, `dot_sens()`, `dot_pss()` and `dot_hb()`
  (`src/spicelib/parser/inp2dot.c`) are the others, and a dot card is the deck's
  own text, so it carries `line_case` and answers `card_spelling()` like any
  other card. The verifier's deck reproduces on this tree ~~in all three
  modes~~ (**imprecise, corrected by stage 13 below**: the one-entry shape the
  guard holds is `fold` and `preserve`; under `distinguish` the two spellings
  are two entries and the deck reaches the pair line only when *both* guards
  go):

  ```
  .options noacct rshunt=1e9
  V1 in 0 dc 0 ac 1
  R1 in 0 1k
  .noise v(MISS) V1 dec 10 1 100
  .tf v(Miss) V1
  ```

  Shipped, one undefined-node line. With the outer guard deleted, that line
  **and** `node names 'MISS' and 'Miss' differ only in case and name one node
  (casemode=fold)` — the double diagnostic criterion 5 forbids, under the
  default mode. (True as written, for `fold`; the sentence stage 12 put in
  `0018` decision 3 generalised it to all three modes and stage 13 corrects
  that.) Corrected in `doc/codex/issues/0068` criterion 5,
  `doc/claude/decisions/0018` (twice — the decision-2 paragraph and the gap-3
  consequence), this ledger's stage 11 P2 bullet (struck), and the
  `term_insert()` header comment in `src/spicelib/parser/inpsymt.c`, which
  carried the same "Only mkvnode() does that" sentence. Crew L wrote no receipt
  file; stage 11's record is this ledger's section, so there was no receipt to
  annotate — see crew N's receipt.
- [x] **P2 — criterion 5 actually closed for `fold`.** Measured first: with
  both guards deleted and the tree rebuilt, `tests/regression/case` and
  `tests/regression/casedist` fail on their one deck each and
  `tests/regression/misc` **passes in full** — `fold` had no guard at all.
  `NMPAIR` added to `tests/regression/misc/node-case-collision-report.cir`: two
  `.tf` cards naming one undefined node `MISS` / `Miss`, asserting
  undefined-node ×1 and pair ×0. RED against the mutation (`pair5 = 1` where the
  reference says `0`), GREEN on the shipped binary. Each guard deleted on its
  own: outer-only now fails all three directories, inner-only still fails
  `casedist` alone, because the chain scan the inner guard sits in is
  `distinguish`-only and `fold` returns before it. The deck uses two `.tf` cards
  rather than the falsifier's `.noise` + `.tf` because a `.noise` whose output
  node does not exist aborts the run, and `remcirc` after the `source` so that
  `--batch` does not run the sub-deck's two transfer-function analyses after
  `.endc`.
- [x] **P3 — litter removed.** `preserve.txt` at the repo root (19 lines of raw
  sweep output, in no crew's residual list), and
  `examples/xspice/table/bsim4p-2d-1.table` and
  `examples/xspice/table/qinn-clc409-2d-1.table`, both untracked byproducts of
  the generator decks in that directory. See crew N's receipt for the evidence
  that each was untracked and deck-written before it was deleted.

### Handoff — files that a plain `git add` will not stage

Updated by stage 12. `tests/.gitignore` line 3 is `*.out`, and every `.out`
already tracked under `tests/` was force-added. All four need `git add -f` **by
name** at commit time, or the commit ships tests with no reference output and
every one of them fails on a fresh clone. The matching `.cir` decks are
untracked but *visible*, so a plain `git add` stages those — which is the
failure mode: the deck lands and its reference does not.

| file | why |
| --- | --- |
| `tests/regression/misc/node-case-collision-report.out` | `tests/.gitignore:3` is `*.out` — **updated by stage 12** with NMPAIR's two lines |
| `tests/regression/case/node-case-collision-report.out` | same — updated by stage 11 with NMPAIR's two lines |
| `tests/regression/casedist/node-case-collision-report.out` | same — updated by stage 11 with NMDEF's and NMREF's four lines |
| `tests/regression/casedist/rawfile-casemode-header.out` | same — crew G's rawfile header work, not this stage's, named here so the commit does not lose it |

## Stage 13 — the correction of stage 12's correction, and stage 11's missing receipt (crew Q)

Stage 12 removed a false claim from `doc/claude/decisions/0018` decision 3 and
put a new one in the paragraph it wrote to remove it. Its verifier caught the
new one. Nothing under `src/` or `tests/` changed in this stage: the diff is
`doc/` only.

- [x] **P1 — `0018` decision 3's gap-3 consequence paragraph, re-measured in
  all three modes and corrected.** The sentence "with the outer guard deleted
  it prints that line and the pair line both, in `fold` as in the other two
  modes" is false for `distinguish`. Re-measured on the verifier's own deck,
  one build per column, `-D casemode=` selecting the mode:

  | mode | shipped | outer deleted | inner deleted | both deleted |
  | --- | --- | --- | --- | --- |
  | `fold` | 1 undefined-node line | + the pair line | unchanged | + the pair line |
  | `preserve` | 1 undefined-node line | + the pair line | unchanged | + the pair line |
  | `distinguish` | **2** undefined-node lines | **unchanged** | unchanged | + the pair line |

  Under `fold` and `preserve` the two spellings share one entry, so
  `t_casetwin` holds them and the outer guard is the only thing in the way.
  Under `distinguish` they are two entries with no `t_casetwin` between them,
  and the only route to the pair line is the chain scan, whose own
  `!u->t_unclaimed` rejects the other unclaimed entry — both guards have to go.
  The same asymmetry is why `distinguish` prints the plain undefined-node line
  twice rather than the near miss once. The guards *are* load-bearing in every
  mode; what shows that under `distinguish` is `casedist`'s NMDEF and NMREF,
  not this deck. Verified by re-running stage 12's mutation matrix: outer-only
  fails `misc`, `case` and `casedist` (the last on NMDEF's `pair5`),
  inner-only fails `casedist` alone (on NMREF's `pair6`).
- [x] **P2 — the same claim did not travel.** `doc/codex/issues/0068`
  criterion 5 scoped its outer-guard sentence to `fold` and is not wrong; it
  gained a paragraph making the `distinguish` column explicit, because the
  ambiguity in "it was measured in all three modes" is what the `0018` sentence
  read as a licence to generalise. This ledger's stage 12 P1 bullet is
  annotated in the same place for the same reason. Stage 12's receipt was
  already correct — it measured *both* guards deleted.
- [x] **P3 — the rest of decision 3 re-read and re-measured, since it has now
  been corrected twice in two days.** Nothing else is stale. Reproduced on this
  tree: the six-row `B`/`E`/`G`/`R` table including the plain linear `E`
  control (5 silent under `fold`, all 6 reported under `preserve`); gap 4's
  spaced-versus-tight XSPICE `a` card (spaced reports under `fold`, tight does
  not, both report under `preserve`); `line_case` last in `struct card`; the
  auto-bridge clearing it in `evtcheck_nodes.c`; and every deck citation in
  gaps 1 and 2 (`Fig27_12_see.sp` `Vr`/`x1.vr`, `FB14.cir` `Vcc`/`vcc`,
  `switch-oscillators.cir`'s `.subckt invertern In Out …` over a body using
  `out`, `global-node-case.cir` and `name-lookup-case.cir` `vss`/`VSS`). The
  "236 B cards in 53 of 875 decks" size is the right order — a looser regex
  over the same trees gives 74 files of 892 — and the text already says it is
  syntactic rather than exact.
- [x] **P4 — `receipts/stage11-crewL.md` written, marked reconstructed.** Crew
  L left no receipt; `receipts/` ran stage1 → stage10 plus `stage9-crewF` and
  then jumped to stage 12. The new file is assembled from this ledger's stage
  11 section and the shipped text of `0018` and `0068`, and its first paragraph
  says so in those words. It records P2's closing sentence only as the thing
  that was struck, per crew N's residual. What could not be reconstructed —
  crew L's commands, its RED/GREEN hunks, whether each `.out` was hand-written
  before first green, and its own residuals — is listed as such.
- [x] **P5 — "seven call sites" corrected where it was written.**
  `INPtermInsertRef()` has seven *calling functions* and *ten* call sites;
  `dot_noise()`, `dot_tf()` and `dot_sens()` call it twice each. The wrong noun
  is `stage12-crewN.md` only — every shipped document says "callers", which is
  right — and it is annotated there. `0018` decision 3 now states both numbers
  so the next reader does not have to re-derive them.

## Stage 14 — round 2 committed and audited

Five commits on `ver_50` above `58496a8dc`, one committer and one independent
auditor. Verdict **SIGN**. Receipt: `receipts/stage14-commit-round2.md`.

| SHA | subject |
| --- | --- |
| `8c3fc233a` | docs: the client's round-2 reply and its repro set |
| `ac1819ff8` | fix: cp_remvar frees a node only when nothing points at it |
| `4e738fc3e` | feat: report two spellings of one node name |
| `9e341a8b7` | feat: record the case mode in the raw header, opt-in |
| `f829c9191` | docs: round 2's issues, guide and client reply |

Each commit's **staged** tree was materialised into its own worktree,
configured and built from scratch, and put through a full `make check` there —
305 → 308 → 311 → 317 → 317 PASS, 0 FAIL throughout, each step's PASS set the
previous set plus exactly the decks it adds, diffed by name. `--enable-oldapps`
was built separately at R3 and `ngsconvert` links, which closes the breakage
standing at `58496a8dc`.

**The ordering constraint held**: `0067` (`ac1819ff8`) precedes the raw header
line (`9e341a8b7`). Reversed, history would hold a commit at which every raw
file the build writes is a crash trigger for released `ngspice-46`.

- [x] **Two disclosed departures from the proposed split.** R2 needed four
  source files the brief omitted (`inpcom.c`, `inpdefs.h`, `inppas2.c`,
  `evtcheck_nodes.c`) or it does not compile. Two of `0067`'s five decks moved
  R1 → R3 because they write a raw file and load it back, so they cannot pass
  before the writer exists; they would have been a red R1.
- [x] **The auditor's problem 1, fixed in a following commit.**
  `casedist/rawfile-casemode-header.cir:37` justified the unset-before-load
  order by a read-only rule that `ac1819ff8` had just deleted. The replacement
  is measured, not asserted: a file written under `preserve` and read by a
  session requesting `distinguish` reads `preserve` in **both** orders. The
  deck's other stated reason is the true one. Deck re-run: PASS.
- [D] **The auditor's problem 2, accepted as committed.** `ac1819ff8` carries
  `options.c`'s `pl_env` scan deletion, which is `0061`/`0019` work, not
  `0067`. Decision `0019` ships in the same commit, so the record travels with
  the code; splitting further would have separated them.
- [D] **The auditor's problem 3, noted not repaired.** The pre-commit backup
  omits the four gitignored `.out` files, so the backup cannot witness their
  byte-identity. The R2/R3 suites pin them instead.

**A vacuous instruction this batch repeated for stages**: "clear the cached
`*.log`/`*.trs`". `configure.ac:37` sets `serial-tests`, which writes neither.
Nothing was ever cleared by it. It is struck from any future brief.

## Log

| when | event |
| --- | --- |
| batch open | ledger created, stage 1 dispatched |
| stage 1 | 12 agents, 0 errors. Six issues filed, six checked, all NEEDS_WORK. Receipts written. |
| stage 1b | remediation dispatched: 6 remediators + 6 re-checkers, each working its own receipt's problem list |
| stage 1b | 12 agents, 0 errors. 0056/0058/0060/0061 signed; 0057 and 0059 held for polish. |
| stage 1c + 2 | dispatched together — polish touches `doc/`, crews touch `src/` and `tests/`, so they cannot collide. Crews run sequentially among themselves because all three share files. |
| stage 1c + 2 | 11 agents, 0 errors, ~1h44m. Four defects fixed, two deferred, three verifiers NEEDS_WORK, one regression. |
| stage 3 | dispatched: three sequential remediation crews, each working its own verifier's problem list, each re-pinning the issue transcripts its own fix invalidated. |
| stage 3 | 6 agents, 0 errors, ~3h14m. B2 **SIGN**. A2 and C2 NEEDS_WORK. `make check` 295-296 PASS, 0 FAIL throughout. |
| stage 4 | dispatched: A3 and C3 sequential on the residuals, plus a doc-only agent filing 0062 in parallel. |
| stage 4 | 5 agents, 0 errors, ~1h48m. 0062 filed with the attribution corrected. A3/C3 NEEDS_WORK. `make check` 301-302 PASS, 0 FAIL. |
| stage 4 | index cleared by the orchestrator — staged tree could not build on its own. Working tree untouched. |
| stage 5 | dispatched as the final fix round: narrow rather than guard, then an independent closeout. |
| stage 5 | 5 agents, 0 errors, ~2h56m. A4 **SIGN**, zero false positives. C4 NEEDS_WORK — its guard refuses correct work. 302 PASS, 0 FAIL. |
| stage 6 | dispatched: C5 to fix or withdraw the 0059 guard, then the closeout refreshed against final state. |
| stage 6 | 3 agents, 0 errors, ~1h18m. Route B. Write path back to HEAD, 300 PASS 0 FAIL. Verifier found three further bypass routes. |
| stage 7 | dispatched: fold the three routes into 0059, `git add -f` the gitignored `.out` files, clean ~53 stray byproducts, final closeout. |
| stage 7 | 2 agents, 0 errors, ~32m. Three routes folded into 0059, 13 `.out` force-added, 113 byproducts removed, `CLOSEOUT.md` rewritten. **Batch closed.** |
| stage 8 | re-opened by the repo owner's three decisions. Dispatched: 0065 filed in parallel; crews D and E sequential on the decided work. |
| stage 8 | 5 agents. 0065 filed. Crew D all 8 criteria, verifier NEEDS_WORK on one disclosure and no unmet criterion. Crew E **SIGNED**. 300 → 303 → 305 PASS, 0 FAIL. |
| stage 9 | dispatched: crew F on the four items crew E's verification left open. |
| stage 9 | 2 agents. 0066 filed and closed, 0067 filed, two doc corrections, one behaviour claim falsified. Verifier NEEDS_WORK on 0067's severity. 305 PASS, 0 FAIL. |
| stage 9b | 0067's severity corrected in place against a re-measurement with no tool underneath; 0065's Left bullet and both receipts corrected with it. |
| closeout | this pass: C1 re-materialised under `/tmp` and rebuilt (291 PASS), the suite re-run (305 PASS), 0067's corrected numbers re-measured independently, `LEDGER.md` and `CLOSEOUT.md` brought to stage 9. |
| stage 11 | crew L on crew H's three residuals and the owner's question. Decision 0018 gap 3 re-derived and corrected (the named cause was not the cause, and the gap is the whole card rather than the expression), a fourth gap found, criterion 5 guarded by three new cases proved RED against each guard deletion separately, the `git add -f` list recorded, and the per-instantiation volume measured at 1028 lines from one character on a shipped deck. Sweep re-run independently: 28 / 38 / 29, crew H's table reproduces. `make check` 315 PASS, 0 FAIL from cleared caches; identity lint 264 comparisons, baseline matches. |
| stage 12 | crew N on crew L's falsified justification. The sentence that closed criterion 5 reproduced false on the verifier's `.noise` + `.tf` deck and corrected in four places — `0068`, `0018` twice, this ledger, and the `term_insert()` comment that carried it too. `mkvnode()` is one of seven callers of `INPtermInsertRef()`; a dot card carries `line_case`, so `fold` reaches the outer guard. Criterion 5 kept **Met** and actually closed: `tests/regression/misc` passed in full under the mutation before this stage, and now carries NMPAIR. Three litter files deleted. Handoff list re-issued with four `.out` files. |
| stage 13 | crew Q on crew N's own new error. The paragraph crew N wrote to remove a false claim generalised an outer-guard measurement to all three modes; re-measured, `distinguish` needs **both** guards deleted before it says anything, because its two spellings are two entries with no `t_casetwin`. Corrected in `0018` decision 3 with the three-mode table, made explicit in `0068` criterion 5, annotated here. The rest of decision 3 re-read and re-measured after two corrections in two days — nothing else stale. `receipts/stage11-crewL.md` written and plainly marked reconstructed. "Seven call sites" corrected to seven calling functions / ten call sites. `doc/` only; `make check` 315 PASS, 0 FAIL. |

## Final state at stage 7 (superseded by stage 9, below)

`make -C build-ver_50 check` — **300 PASS, 0 FAIL**, from cleared caches.
Identity lint 265 comparisons, baseline matches. 14 new decks, all passing.

| issue | outcome |
| --- | --- |
| 0056 `.save` byte-exact | **fixed, verified** — `outitf.c` split into `name_eq()` (stored-vs-stored, still exact) and `name_eq_query()` → `vec_name_eq()` |
| 0057 failed `.save` names no token | **fixed after narrowing** — a case near-miss report under `distinguish`, not a general unresolved-name report. Signed, zero false positives |
| 0058 diagnostics per file read | **fixed, signed** — announced once per run, latched on the outcome |
| 0059 constants plot written | **WITHDRAWN, issue stays Open** — three discriminators attempted, three more bypass routes found afterwards. Write path byte-identical to HEAD |
| 0060 `$casemode` reports the request | **[D]** — everything but the new variable's name; naming is permanent user-visible surface |
| 0061 `Option:` reaches `pl_env` | **[D]** — needs a ruling on how much of `pl_env` a loaded file may reach |
| 0062 shared build after `ngSpice_Reset` | **filed** — minimal repro is four API calls; attribution corrected during filing |
| 0063 `killplot` internal error | **filed** — criterion 4 discharged when the deck asserting the error was deleted |
| 0064 `all`/`allv`/`alli`/`ally` | **filed** — four spellings of one request; the fact that made 0059's syntax-keyed guards unworkable |

Nothing is committed. The index held 13 force-added `.out` files and nothing
else — `tests/.gitignore` ignores `*.out`, so a plain `git add -A` would commit
tests with no reference output. (Stage 8 added two more, for 15.) The commit
split is in `CLOSEOUT.md` §2.

## Stage 8 — the deferred decisions, made

The repo owner decided all three on 2026-08-13, after the batch closed.

- [x] 0065 — **new crash, found while explaining 0061.** A raw file whose header
  carries `Option: curplot=HAHA` segfaults ngspice on a bare `load`: rc=139,
  pipe mode and batch alike, and **identically on stock `ngspice-46`**, so it is
  pre-existing and not this batch's doing. `cp_enqvar()` searches `pl_env`
  *before* the arms that compute the `curplot` family and `plots`, so a
  file-supplied string is returned where a computed value is expected.
  `Option: ngbehavior=hs` on the same path does not crash — it silently takes
  effect, which is 0061's subject, not this one.
- [x] Crew D — 0060 `curcasemode` — all 8 criteria met, 0 regressions, 303 PASS.
  Its verifier returned NEEDS_WORK with **no unmet criterion and no
  regression**: the verdict hangs on one disclosure, that a rawfile may write
  `Option: curcasemode=…` and never have it read back while every other
  `Option:` key still answers, with nothing warning. Crew F took that as its
  item 3 and settled it — a sentence in the issue, not a diagnostic, because
  the diagnostic was measured to misfire on a file ngspice wrote itself.
- [x] Crew E — 0061 shape A + 0065 — **SIGNED**, 14 of 15 criteria, 305 PASS.
  mechanism one layer apart: a computed name must win over a plot-environment
  key, which is also what 0060's criterion 6 needs.

**Sequenced D → E, and the sequencing premise turned out to be wrong in crew
D's favour.** The brief expected 0060's criterion 6 (the variable must not be
shadowable by a loaded plot's environment) to need 0061's fix, so crew D was
allowed to leave that one deck failing-by-design rather than weaken it. It did
not need it: `cp_enqvar()` answers `curcasemode` *before* it reaches
`plot_cur->pl_env`, which the criterion's own text permits, so crew D closed
all eight on its own and crew E's contribution to that criterion was a second,
stronger reason (the whole `curplot` family became unshadowable, not just
`curcasemode`).

## Stage 9 — the fix's own residue

Dispatched after crew E signed, on four items its verification had left open.

- [x] **Crew F — item 1, the code defect: FIXED.** 0061's guard is a depth
  count raised and lowered by `inp_readall()`; a netlist read that ends
  *fatally* does not leave through `inp_readall()`, so the count stays up. In
  the standalone binary that is harmless (`controlled_exit()` calls `exit()`);
  in a `--with-ngshared` build it calls `shared_exit()`, the stack is
  discarded, and from that moment `plot_cur->pl_env` answers **no**
  `cp_getvar()` for the rest of the host process — 0061's narrowing applied
  permanently and to everything. **Three routes out, not the two 0061 named**:
  `ngSpice_Command` → `longjmp(errbufc)`, `ngSpice_Circ` → `longjmp(errbufm)`,
  and `bg_source` → `pthread_exit()`, which lands at no `setjmp` at all. One
  line at `src/sharedspice.c:2192`, the last point common to all three. RED
  `before=1 after=0 GUARD-STUCK` on all three modes, GREEN `after=1 GUARD-OK`;
  each half broken separately to prove neither passes on the other's
  mechanism. Filed and closed as **0066**.
- [x] **Item 2 — 0065's criterion 6 contradiction: FIXED, and 0067 filed.**
  The criterion had called `cp_remvar()` and `cp_vprint()` "both correct with
  no change of their own" while the same document recorded `unset curplot`
  aborting inside `cp_remvar()`. `cp_vprint()` is correct; `cp_remvar()` is
  not, and its two symptoms are one unconditional free at
  `variable.c:671`-`:672` of a node the `switch` arms above have already
  disposed of. Filed as **0067**, one issue for both, a third arm
  (`US_SIMVAR`) found while writing it.
- [x] **Item 3 — 0060's asymmetry: a sentence, and the alternative falsified.**
  A load-time warning about `Option: curcasemode=…` would fire on a
  `load` → `write` → `load` lap of a file ngspice wrote itself. Measured, both
  laps. Recorded in 0060's Left rather than implemented.
- [x] **Item 4 — one behaviour change confirmed, one falsified.** Confirmed:
  `set` after `Option: CurPlot=HAHA` now lists `* CurPlot op1` where BASE and
  STOCK list `HAHA` — caused by 0065's `cp_enqvar()` reordering, not 0061's
  guard. Falsified twice over: the `sourcepath` diagnostics were never this
  batch's (BASE is silent on the same input) and have not gone away (all three
  binaries print both lines when the deck is sourced from a directory that is
  genuinely new); the original observation sourced from the cwd, where `.` is
  already on `sourcepath`.
- [F] **Crew F's verifier: NEEDS_WORK, and it was right.** 0067 as first
  written called its `US_READONLY` arm "latent, allocator dependent, and today
  usually silent" and overruled 0065's `rc=139` on the strength of a
  two-command sequence. The verifier measured the three-command sequence with
  no tool underneath: `load`, `unset zzz`, `set` is `rc=139`, 3/3, on
  `build-ver_50/src/ngspice` **and** on stock `ngspice-46`. Crew F had run it
  under `valgrind`, whose allocator keeps the freed block readable — `rc=0`
  and 34 invalid reads is the tool's exit status, not the program's.
- [x] **0067 corrected in place**, after the verification, by a following
  agent: Status, Summary, Impact and criteria 2 and 6 rewritten around the
  exit status; the `US_SIMVAR` arm added; 0065's Left bullet and both stage-8
  and stage-9 receipts corrected to match. 0065's `rc=139` is restored as
  correct.

## Final state after stage 9

`make -C build-ver_50 check` — **305 PASS, 0 FAIL**, from cleared caches,
re-measured 2026-08-13 by the closeout pass. Identity lint 265 comparisons,
baseline matches, plus the 11-comparison selftest. 19 new decks, all passing.
Working tree: 24 modified files (12 under `src/`), 15 force-added `.out` files
in the index and nothing else, 278 untracked.

| issue | outcome |
| --- | --- |
| 0056 `.save` byte-exact | **fixed, verified** — the only commit materialised and built |
| 0057 failed `.save` names no token | **fixed after narrowing** — a case near-miss report, not a general one. Signed, zero false positives |
| 0058 diagnostics per file read | **fixed, signed** |
| 0059 constants plot written | **WITHDRAWN, issue stays Open** — write path byte-identical to HEAD |
| 0060 `$casemode` reports the request | **fixed, closed** — `curcasemode`, the owner's name, in the computed read-only family. 8/8 criteria |
| 0061 `Option:` reaches `pl_env` | **fixed, closed** — shape A, the owner's decision: the pair is still filed and still readable, and a plot's environment stops answering `cp_getvar()` while a netlist is being read |
| 0062 shared build after `ngSpice_Reset` | **filed, open** — not reachable from `make check` |
| 0063 `killplot` internal error | **filed, open, unblocked** |
| 0064 `all`/`allv`/`alli`/`ally` | **filed, open** |
| 0065 `Option:` shadows a computed variable | **fixed, closed** — `cp_enqvar()` answers what it computes ahead of `pl_env`; `cp_usrvars()` links only nodes it owns. `SIGSEGV` on a bare `load`, pre-existing and reproducing on stock |
| 0066 fatal read strands the reader guard | **fixed, closed** — a defect in 0061's own fix, found by its verifier, unreachable by any harness in this repo |
| 0067 `cp_remvar()` frees a node it did not unlink | **filed, open** — pre-existing and upstream, three arms, one line. `rc=134` or `rc=139` deterministically on stock `ngspice-46` |

Six fixed and closed, three open and filed, one withdrawn-with-the-issue-open,
two filed by the batch about defects it did not cause (0065 is closed, 0067 is
not). **Nothing is committed**; the commit split is `CLOSEOUT.md` §2, now
eleven commits (C0–C10, up from six), and only the first of them has been
built.

Two things the owner still has to decide, and neither is a fix:

- where a probe that cannot live under `tests/` should live. `make check`
  cannot reach 0062 or 0066 at all — nothing under `tests/` links
  `libngspice` — and both issues name a probe under
  `doc/claude/feedback/ngspice_upstream/repro/shared/` as their pin. That
  directory is untracked in its entirety, so as things stand the pin vanishes
  with the working copy. 0066's Left records the question and declines to
  answer it.
- whether `doc/claude/batches/` (this ledger, 32 receipts, the closeout) goes
  into history at all. It is commit C10 of the split and is the one commit that
  can be dropped without breaking any other.