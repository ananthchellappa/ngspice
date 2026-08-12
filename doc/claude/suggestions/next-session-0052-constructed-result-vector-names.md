# Next-session prompt — `doc/codex/issues/0052`, the constructed result-vector name

Paste everything below the line into a fresh session at
`/home/qflow/dev/ngspice_test`, branch `ver_50`.

Why this one next: it is the **third instance of a hazard
`doc/claude/decisions/0001-distinguish.md` decision 3 wrote down and left
open** — *"`distinguish` must be exact about names the deck chose and must stay
case-insensitive about names ngspice constructs"* — and the first where the
constructed name is a whole analysis *result* rather than a decoration on a
node name, and where the spelling the manual tells a user to type is the one
that fails. It also carries the reason the `distinguish` suite could not see
it: 23 decks in `tests/regression/casedist/` and not one of them uses the
control-language prompt.

---

Fix `doc/codex/issues/0052` on branch `ver_50`, and decide the mechanism
before writing it — there are two candidates and the issue deliberately does
not choose.

Read, in this order and in full:

1. `doc/codex/issues/0052-analysis-result-vector-names-carry-capitals.md`
   — all three of its items and all six acceptance criteria. Item 3 is not
   optional: it is what makes item 1 assertable.
2. `doc/claude/decisions/0001-distinguish.md` **decision 3**, and inside it the
   **Class C** section — specifically its "One exception inside the exception"
   paragraph on `src/frontend/outitf.c:1164`'s `V(<name>)`, and the paragraph
   that closes the section: *"The general form of the hazard is worth stating,
   because it will recur at gates 1, 2 and 4: `distinguish` must be exact about
   names the deck chose and must stay case-insensitive about names ngspice
   constructs. `V(1)` is the first instance. `q1#collCX` … is not handled."*
   That paragraph is the rule you are applying for the third time, and the two
   precedents disagree about the answer: `V(1)` got a comparator
   (`vec_wrapped_name_eq()`), `q1#collCX` got a documented limitation.
   Decide which this is and say why the other was rejected.
3. `doc/claude/decisions/0001-distinguish.md` **decision 6 item 1**'s closing
   paragraph on `doc/codex/issues/0029`'s `(a)`, and the
   `entrynb_constructed()` mechanism it describes. That is the *other*
   precedent: a query-side retry, exact first and case-insensitive only on the
   final component, in a separate entry point so that the strict path stays
   strict. It is the closest prior art to candidate B below.
4. `doc/claude/decisions/0005-scale-vector-identity.md` decision 1 — why a
   single helper was preferred over three call-site fixes — and
   `doc/claude/decisions/0009-let-definition-report.md` decision 1 — flag
   versus second entry point, decided on the ratio of callers. You will face
   the same choice.
5. `doc/claude/decisions/0013-user-defined-function-identity.md` **decision 2**
   — read this one for the method, not the subject. It is a decision that was
   made, shipped GREEN with a passing lint, and then **reversed** by an
   adversarial re-read that ran the case the Evidence section had not. The case
   it missed was one where making a comparator case-blind *widened* what a
   neighbouring defect destroyed. Candidate A below has exactly that shape.

## The state, measured at `2c4672266`

A deck with **no upper-case character in it at all** — verified with
`grep -c '[A-Z]'`, which returns 0:

```
circbyline * min
circbyline v1 in 0 dc 1
circbyline r1 in out 1k
circbyline rl out 0 3k
circbyline .end
tf v(out) v1
display
print transfer_function
```

`display` prints the same three vectors in every mode:

```
    Transfer_function   : voltage, real, 1 long [default scale]
    output_impedance_at_V(out): voltage, real, 1 long
    v1#Input_impedance  : voltage, real, 1 long
```

and only the lookup diverges:

```
fold, preserve   transfer_function = 7.500000e-01
distinguish      Warning: no vector named 'transfer_function';
                          'Transfer_function' differs only in case (casemode=distinguish)
                 Warning from checkvalid: vector transfer_function is not
                          available or has zero length.
```

The mint sites:

```c
/* src/spicelib/analysis/tfanal.c */
:91   IFnewUid(ckt, &tfuid,  NULL,          "Transfer_function", UID_OTHER, NULL);
:94   IFnewUid(ckt, &inuid,  job->TFinSrc,  "Input_impedance",   UID_OTHER, NULL);
:98   IFnewUid(ckt, &outuid, job->TFoutSrc, "Output_impedance",  UID_OTHER, NULL);
:100  name = tprintf("output_impedance_at_%s", job->TFoutName);
```

`TFoutName` is built at `src/spicelib/parser/inp2dot.c:390` and `:516` as
`tprintf("V(%s)", nname1)` — the same capital `V` that
`src/frontend/outitf.c:1164` produces and that `vec_wrapped_name_eq()`
(`src/frontend/vectors.c:366`) exists to fold. **That helper cannot help
here**: it requires the whole string to be a `V(...)`/`I(...)` wrapper, and
`output_impedance_at_V(out)` is a wrapper buried in a longer name. Re-read it
before assuming otherwise.

The predicate that rejects is `vec_name_eq()`, `src/frontend/vectors.c:415-419`,
with `v_name = "Transfer_function"` and `typed = "transfer_function"`. Both its
arms are **permanent residents** of `tests/lint/identity.baseline` — see that
file's header — so a change to the helper itself is a baseline event and a
change at a call site may not be.

Four things already eliminated, each by measurement rather than by reading, so
do not re-derive them and do not let a subagent tell you otherwise:

- **The dot card fails identically.** `.tf v(out) v1` in a deck with a
  `.control` block produces the same three names and the same miss, so
  `doc/codex/issues/0043`'s command-form/dot-card split is **not** implicated.
- **`dot_tf()`'s accessor matcher is correct.** `inp2dot.c:369` and `:392` are
  `cieq(name, "v")` and `cieq(name, "i")` — a literal operand, right in all
  three modes. `tests/regression/pipe/analysis-keyword-case.cmd`'s
  `ERROR: uppercase V() not accepted by tf` is wrong on both counts and is one
  of the four texts criterion 4 asks you to correct.
- **Only the byte compare rejects.** Under `distinguish`,
  `print Transfer_function`, `print v1#Input_impedance` and
  `print output_impedance_at_V(out)` all succeed, so the plot, the folded hash
  key and the duplicate chain are all fine.
- **The instance-parameter letter is not the problem and contract point 2 is
  not violated.** On a circuit whose card spells the instance `R1`, one `save`
  per run: `@R1[i]` and `@R1[I]` are both length 1 under `preserve` and
  `distinguish`. `0052` item 3 has the full table. A first reading of this
  experiment called four decks contract violations; three of them are deck
  artifacts and the fourth is item 1.

## The decision this session has to make and record

**WHICH SIDE GETS THE TOLERANCE.** Two candidates, and the issue's criterion 1
requires you to say why the loser lost.

**Candidate A — fold the literals at the mint.** Change `"Transfer_function"`
to `"transfer_function"`, `"Input_impedance"` to `"input_impedance"`,
`"Output_impedance"`, and `inp2dot.c`'s `tprintf("V(%s)", …)` to `v(%s)`.
Simplest, and it makes the stored name match the documentation. Its costs, all
of which have to be measured rather than argued:

- It changes `display`, `print all`, `write` and the `-r` rawfile in **every
  mode**, including the default. That is a user-visible rename of four vectors
  for every existing deck, so it needs a RED of its own and it will move decks
  in `deck_output_differ.py --mode` for all three modes.
- Criterion 2 says `print Transfer_function` must keep working. Under `fold`
  and `preserve` it will, by `cieq`. Under `distinguish` it will **not**, and a
  deck that already spells it the stored way would break. Decide whether that
  is acceptable and say so.
- `inp2dot.c:390`/`:516`'s `V(%s)` is also the `outname` a `.sens` card uses.
  Check what else reads it before touching it — a grep for `outname` is the
  minimum.
- **Read `0013` decision 2 first.** That is a case where a comparator was made
  case-blind and the widening destroyed something a neighbouring defect had
  been reaching for. Ask the analogous question here: what else compares
  against these four literals, and does lowering them make some *other*
  match start succeeding?

**Candidate B — tolerate on the query side, for constructed names only.**
`0029`'s `entrynb_constructed()` is the precedent: a second entry point that
tries the exact match first and retries case-insensitively only for a name the
simulator built. Its costs:

- It needs a way to know a stored name is constructed. There is no flag on
  `struct dvec` for it today. Inventing one is a data-structure change; keying
  off the four literals is a hard-coded list that will rot.
- It weakens `distinguish` for a class of names rather than for one, which is
  the thing `0001` decision 3 kept refusing at `findvec()`.
- It cannot be a change to `vec_name_eq()` itself without moving every one of
  its callers, and there are six.

There may be a third answer. `q1#collCX` is precedent for **documenting the
limitation and fixing nothing**, and if that is where you land, then criteria 1
and 2 are answered by an edit to
`doc/claude/specs/case-sensitive-identifiers.md` and to
`set_case_mode()`'s experimental warning, and the deck of criterion 5 asserts
the *capitalised* spelling working plus the near-miss warning firing on the
lower-case one. That is a legitimate outcome and it is cheaper than either
mechanism; it is not the lazy one, because the near-miss warning already makes
the failure audible, which is the bar `0002` set for gate 1.

## The other work, which is not optional

**Criterion 4, the four `tests/regression/pipe` decks.** Each writes an
upper-case `circbyline` card and then queries it in lower case, which is
invisible under the default `fold` and makes the deck fail under any other mode
for a reason that is not what its `ERROR:` text says. Fix the seeds, fix the
four texts, and then **assert the claim**: the directory should pass under
`-D casemode=preserve` and `-D casemode=distinguish` as well as the default.
Today that is not assertable at all, because
`tests/regression/pipe/Makefile.am`'s `TESTS_ENVIRONMENT` pins no mode. Decide
whether to add twin directories, a mode loop, or a note — and if it is a note,
say what the note buys.

**Criterion 3, `.noise`.** `grep 'IFnewUid' src/spicelib/analysis/*.c` returns
`NF`, `NFmin`, `Rn` and `SOpt` beside the three `.tf` names. An all-lower-case
two-resistor `.noise` deck does **not** reach them — it produces only
`frequency`, `inoise_spectrum`, `onoise_spectrum` and the per-device
`onoise_r1*` family, identically in `fold` and `distinguish`. So this is
recorded as **unmeasured** and you have to build the deck that reaches the
noise-figure path before you can say whether `print nf` misses. If it does, it
moves with item 1; if it cannot be reached from a deck at all, record that
instead.

## Deliverables

- The mechanism, or the recorded decision not to have one, with the loser's
  costs stated.
- A RED in `tests/regression/casedist/` — the first deck in that directory to
  exercise the control-language prompt path, which is criterion 5 and is the
  structural gap that hid this. It must fail at `2c4672266`; quote the failure.
  Remember `tests/bin/check.sh:20`'s `egrep -v` filter eats every line
  containing `Error` or `Warning`, so assert on a **value**, or capture the
  diagnostic with `>&` and read it back the way
  `tests/regression/casedist/vector-unlet-report.cir` does.
- The four `pipe` decks corrected, and their directory's mode coverage decided.
- `doc/claude/decisions/0014-<slug>.md`, and a Resolution on `0052`.
- If candidate A is chosen, the spec's Compatibility contract and
  `set_case_mode()`'s warning both need re-reading for statements that become
  false.

## Baselines, measured at `2c4672266`

- full `make check` = **282 PASS, 0 FAIL**, exit 0. That is the bar.
  Per directory: `regression/case` 113, `regression/casedist` 23,
  `regression/misc` 27, `regression/pipe` 16, `xspice/case` 21,
  `xspice/casedist` 17, `lint` 2.
- `make -C build-ver_50/tests/lint check` = **"identity lint: 268 comparisons,
  baseline matches"**, ~1.6 s. Run it after every `src/` edit.
- `python3 doc/claude/scripts/case_differential_sweep.py --jobs 8 --timeout 90`
  = 321 decks, `DIFF=61`, `OK=257`, `SKIP=3`, `PARSE-FAIL=0`, `NUM-DIFF=0`.
  The last two must stay 0. Note that
  `tests/regression/case/udf-name-case{,-lower}.cir` are `DIFF` **by design**
  on one line, `+'vm (x) = mag (v (x))'` — `doc/codex/issues/0051`'s residue —
  and clear when that lands.
- `python3 doc/claude/scripts/deck_output_differ.py --before X --after Y
  --jobs 8 --timeout 200 --mode {,preserve,distinguish}`. At no change the
  counts measured on 2026-08-12 were fold 3, preserve 4 (two of them this
  session's own decks), distinguish 0 — the four `alter-rebin` decks flap
  within ±2, so treat any of them as noise and everything else as signal.

## Hazards

- **BOTH measuring scripts `chdir` per deck**, so every binary path passed to
  them must be **ABSOLUTE**. A relative `--ngspice` or `--after` dies with
  `FileNotFoundError` and leaves a ~1.7 KB output file that looks like a
  finished run.
- Snapshot the binary before you measure:
  `cp build-ver_50/src/ngspice <scratch>/ngspice-baseline`.
- `tests/.gitignore` ignores `*.out`; every reference needs `git add -f`.
- `./autogen.sh` after any `Makefile.am` edit, then `../configure` in
  `build-ver_50`.
- A `.cir` absent from its directory's `TESTS` is silently never run.
- **`ngspice -p` + `-D casemode=...` works**, and it is how `0052` was
  measured — but `set_case_mode()` runs per netlist read, so a command typed
  before any circuit exists still sees the previous mode. Every repro here
  builds its circuit with `circbyline ... .end` first for that reason.
- **One `save` per run.** A first attempt at the `@dev[param]` measurement
  issued two `save` commands in one session and reported `fold` as broken in
  both directions, which it is not. Two probes in one plot contaminate each
  other.
- **The X server in this WSL environment dies under load.** Two full
  `make check` runs made while the differ was busy each failed a different
  `tests/regression/pipe` deck immediately after
  `X connection to :0 broken (explicit kill or server shutdown)` — Xlib's fatal
  handler killing ngspice. Each deck passes on its own. Do not run `make check`
  concurrently with the differ, and if you see one `pipe` failure preceded by
  that line, re-run before believing it.
- `make -k check` when you want the whole suite: a failing directory otherwise
  aborts automake's recursion and the totals become a lower bound. That is how
  the `default=preserve` measurement first read `41 PASS` instead of `278`.
- A shell `cd` persists between calls. Use absolute paths.
- Never pipe `make` through `head`; SIGPIPE leaves a stale binary.
- Re-grep every line number after your edits.

## Deliberately out of scope — enumerate, do not fix

- **The default `casemode`.** It stays `fold`. `0052`'s Status records the
  `preserve` = 278/4 and `distinguish` = 272/10 measurement; that experiment
  was run to size the gap, and the decision has been taken to keep the default
  for backward compatibility. Do not flip it, and do not treat those numbers as
  a target.
- **`save @R1[i]` failing under the default `fold`** when the card spells the
  instance `R1`. Real, shipped-mode, `doc/codex/issues/0016`'s family — an
  instance-name matcher that does not go through
  `INPretrieve()`/`DEVnameHash`. `0052` item 3 has the measured table. It wants
  its own session and its own issue number.
- **`tests/xspice/case/auto-bridge-node-case-fold.cir` and
  `event-node-case-fold.cir` failing under `distinguish`.** They spell one
  mixed-signal net two ways on purpose; under `distinguish` that is two nets by
  design. `0001` decision 5's migration hazard, not a defect.
- `doc/codex/issues/0051` (the `define` prefix overwrite, which also blocks one
  line of `0013`), `0050`, `0048`, `0047`, `0046`, `0043`, `0042`, `0041`,
  `0039`, `0038`, `0035`, `0033`, `0031`, `0025`, `0024`, `0021`, `0016` class
  (b), `0012`, `0008`, `0007`, `0006`, `0005`, `0004`, and `com_let()`'s
  `plainlet` leading space per `0009` deferral 4.
- The rawfile half of the spec's `distinguish` acceptance bullet: no deck
  asserts that two case-variant vectors are separately present in a `-r`
  rawfile.

Anything new you find and do not fix goes in `doc/codex/issues/NNNN-slug.md`
with the Status / Summary / Impact / Root Cause / Acceptance Criteria /
Resolution structure. Next free number is **0053**.

## Conventions

- Match the style of the file being edited; `src/frontend/` and
  `src/spicelib/` are four-space, same-line brace. Do not reflow.
- `make check` lints every `strcmp`/`cieq`/`eq`/`eqc` in `src/` whose operands
  are both runtime expressions. A comparison of a name a deck wrote goes
  through `ng_ideq()`, `vec_name_eq()` or `Evt_Node_Name_Eq()`; a word the
  language defines stays byte-exact and takes a
  `/* case-lint: keyword - <why> */` comment on its own line or the line above.
  A new helper's body is annotated, not baselined. See `AGENTS.md` and
  `doc/claude/decisions/0011-identity-lint.md`.
- One commit per mechanism, one for the documentation. Commit messages: short
  imperative summary, then Problem / Mechanism / RED / Modes. State what moves
  in each of the three modes and name the decks.
- **Review your own commits adversarially before declaring it done.** The
  `0013` session shipped a decision GREEN, with a passing lint and a full
  Evidence section, and an adversarial re-read then found that the change
  destroyed a shipped function in the default mode — because the Evidence
  tested the pair that was already broken rather than the pair the change newly
  reached. Ask: is every token I assert reachable, can any assertion pass
  vacuously, does each artefact fail for the reason I say, is every factual
  claim in my commit message something I ran against THIS commit, and does
  `git show --stat` list only files I meant to touch?

## Stop and report

What changed per file, the RED before and the PASS after, which candidate you
chose for the tolerance and what the loser's costs were, whether `.noise`'s
four names are reachable from a deck, what you did about
`tests/regression/pipe`'s mode coverage, the sweep and differ verdicts compared
per deck in all three modes, your adversarial review of your own commits, and
the full `make check` result.
