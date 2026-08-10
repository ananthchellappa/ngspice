# Next-session prompt — issue 0019's remaining classification sites

Paste everything below the line into a fresh session at
`/home/qflow/dev/ngspice_test`, branch `ver_50`.

---

Close `doc/codex/issues/0019` on branch `ver_50`: seven lower-case
card-classification sites left over after `doc/codex/issues/0009`'s follow-up
table was closed, four of them outside `src/frontend/inpcom.c`. Two are
wrong-number defects that produce no diagnostic at all, which makes them the
highest-value rows in the file.

Read `doc/codex/issues/0019` first — it is the work list, and its Acceptance
Criteria already say which directory each deck belongs in. Then read
`doc/codex/issues/0009`, whose "Follow-up table: closed" section records how
the previous round was done and what it deliberately did not reach, and
`doc/codex/issues/0018`, which is the one site of that round's class that was
fixed without being on the table. Then
`doc/claude/checklists/phase2-differential-sweep.md` (its last section is the
baseline you are measuring against),
`doc/claude/suggestions/case-sensitive-identifiers-plan.md` and
`doc/claude/code_analysis/case-insensitivity-origins.md` — those three are
ground truth.

Do not start Phase 3. Do not touch issues 0011, 0014, 0015, 0016 or 0020.
`doc/codex/issues/0020`'s `M=` multiplier gap is the parameter-name half of a
pass whose device-letter half is already fixed, and it has to move with 0015,
not with this.

Phases 0, 1, 2 and issues 0009, 0010, 0013, 0017 and 0018 are committed. Do
not redo any of them. HEAD is `c740a0fb2`.

Baseline, measured on this tree at HEAD:

- full `make check` = **172 tests, 0 FAIL**. That is the acceptance bar.
- `python3 doc/claude/scripts/case_differential_sweep.py --jobs 12 --timeout 90`
  = **213 decks: OK=157, DIFF=53, PARSE-FAIL=0, NUM-DIFF=0, SKIP=3**.
  `PARSE-FAIL` and `NUM-DIFF` must both stay 0. The 3 SKIPs are stock-run
  timeouts (`tests/mesa/mesa-12.cir`, `tests/mesa/mesa12.cir`,
  `tests/vbic/FG.cir`) and stay. The sweep takes about 10 minutes. Compare it
  per deck against a baseline you measure yourself, not against the totals
  quoted here — one timing-sensitive deck moves between runs.
- Twelve of the 53 `DIFF` decks are the new case decks, and both causes are
  `doc/codex/issues/0020`, not this issue. Do not try to fix them here; do
  check that their count does not grow.

## Do the two area factors first

They are the only rows with a silent wrong number, they need no new harness,
and they are one token each.

```c
/* src/frontend/inpcompat.c:1101, pspice_compat() */
        if (*cut_line == 'q') {
```
```c
/* src/frontend/inpcompat.c:1153, pspice_compat() */
        else if (*cut_line == 'd') {
```

Both append `area=<n>` when the token after the model name is a bare positive
number. Under `-D casemode=preserve -D ngbehavior=ps` an upper-case
`Q1 c b e s qmod 2` or `D1 a k dmod 3` loses the area factor, the deck runs,
and the currents are wrong by the area ratio. Nothing is printed.

`tests/regression/case-pspice/` already exists and already passes
`-D casemode=preserve -D ngbehavior=ps`. **Put the device card in an `.inc`
file.** `pspice_compat()` runs on the card list `inp_read()` returns for an
*included* file (`src/frontend/inpcom.c:1754`), never on the top-level deck —
the first attempt at those decks last session was written inline and
exercised nothing. `pspice-tc-r-case.cir` is the template, `.inc` file and
`EXTRA_DIST` entry included.

## The rest, with anchors verified at HEAD

| File:line | Test | Harness |
| --- | --- | --- |
| `inpcompat.c:1101` | `*cut_line == 'q'` — BJT area | `case-pspice/`, wrong number |
| `inpcompat.c:1153` | `*cut_line == 'd'` — diode area | `case-pspice/`, wrong number |
| `inp.c:2454` | `switch (devline[0])` in `inp_savecurrents()` | `case/`, needs `.options savecurrents` |
| `inpcom.c:5108` | `equal_ptr[1] == 'v'` under a `ciprefix(".meas", …)` gate | `case/`, evidence a `.meas` result line |
| `inpcom.c:2755` | `expr[0] == 'v' && expr[1] == '('` in `replace_freq()` | `tests/xspice/case/`, sibling of the row already fixed at `:2745` |
| `inp.c:1828` | `*xline == 'x'` in `com_alterparam()` | `case/`; `alter-case.cir` is the precedent for driving a command |
| `inp.c:2030` | `switch (firstchar)` in `cktislinear()` | probably no numeric witness — say so with the argument |

Re-locate every anchor by `grep` before editing; these files churn.

**Also verify and decide on `src/frontend/inpcompat.c:1771**, `else if
(*cut_line == 'r')` in `ltspice_compat()`'s `noiseless` → `noisy=0` rewrite.
It was flagged in a footnote last session and never confirmed. If it is real,
add a row to `doc/codex/issues/0019` and a deck to a new
`tests/regression/case-lt/` entry (that directory already passes
`-D ngbehavior=lt`); if it is unreachable, record why.

**Do not fold `src/frontend/inp.c:2784`.** `if (*curr_line == 'm')` in
`rem_unused_mos_models()` looks exactly like this class and is dead: the
function, its prototype and its call are all inside `#ifdef REM_UNUSED`
(`inp.c:61`, `:1084`, `:2682`) and `REM_UNUSED` is defined nowhere in `src/`.
`doc/codex/issues/0019` already records this; leave it.

## Conventions

- `elem_letter()` (`src/frontend/inpcom.c:934`) is the in-tree helper and is
  now **exported** — `src/frontend/inpcom.h:16` declares it, so `inp.c` and
  `inpcompat.c` can call it. Use it for a card's leading device letter. Do
  not add a second helper.
- For a byte in the middle of a token, use `tolower_c()` directly and say why
  in the commit — `elem_letter()` would be a lie about what is being read.
  `inpcom.c:4747`, `:7804` and `:8846` are the precedents.
- `strpbrk` needle sets cannot be folded; double the set, as
  `inpcom.c:5975`'s `"vithVITH"` does.
- Match the style of the file being edited: four-space indent, same-line
  opening brace, no reflowing, no whitespace churn. Keep the diff to
  single-token edits.
- Commit messages: short imperative summary, then problem, mechanism, and the
  RED evidence. State whether the change is fold-mode visible, with the
  argument, per site.

## Method — RED-first per AGENTS.md, non-negotiable

1. write or extend the test, 2. run it and show the failure is for the
expected reason, 3. smallest production change, 4. re-run the targeted test
then the directory's `make check`, 5. full `make check` before the commit.
Never weaken an assertion or regenerate a `.out` to hide a failure. If a site
genuinely cannot be given a failing test first, say why before implementing
it — five rows of the last round were in that position and each recorded its
own argument rather than borrowing one.

Twin-pair pattern: an upper/lower pair with byte-identical `.out` files, so
the assertion is "the spelling cannot change the number". Verify each
reference against the **fold-mode** run of both twins before committing it.

Split per mechanism. A single combined deck aborts on the first error and
hides the rest.

## Test harness hazards — most obvious tests are invalid without these

- `tests/bin/check.sh:20` filters BOTH sides through a large `egrep -v` that
  removes `Error`, `Warning`, timings, memory and index columns. **Run every
  new deck against an empty `.out` first; if it passes, its evidence is not a
  number and the deck is worthless.** Last round all 29 new numeric decks were
  put through this check.
- `check.sh` captures **stdout only**. Anything on stderr — every
  `warning, can't find model`, every `Error on line …` — is invisible to
  `make check`. The differential sweep, which merges stderr, is the only
  harness that sees those.
- **The first line of a deck is its title.** A deck whose first line is
  `.OPTIONS noacct` loses that card. Start every deck with a `*` comment.
- Start every deck with `.OPTIONS noacct` on the second line — the rusage
  block is not filtered and is machine-dependent.
- Use numeric node names for anything you print. Under `preserve` a node name
  keeps its case in the vector name, so `print v(Out)` and `print v(out)`
  produce different text and the twin decks stop matching.
- Do not print with `vm(...)`/`vp(...)`/`vdb(...)` if you can avoid it: the
  sweep's uppercased copy becomes `VM(2)`, which `preserve` cannot resolve.
  That is `doc/codex/issues/0020` and it costs a deck its `OK` verdict.
- A deck in `tests/xspice/case/` emits an XSPICE A device, so its `.out` must
  contain the line `Reducing trtol to 1 for xspice 'A' devices` — it survives
  the filter. Generate those references as
  `ngspice --batch deck | egrep -v "$FILTER"`, not by grepping for the number.
- A `.cir` absent from its directory's `TESTS` is silently never run.
- `tests/.gitignore` ignores `*.out`, so every reference needs `git add -f`.
- Tests that write files need a `CLEANFILES` entry.
- CIDER is off in `build-ver_50`. Do not claim coverage of a `#ifdef CIDER`
  path.

## Build

```
make -C build-ver_50 -j14
make -C build-ver_50/tests/regression/case check TESTS=<name>.cir
make -C build-ver_50 check                       # full suite, currently 172
```

Adding a test file to an existing `Makefile.am` needs only
`make -C build-ver_50/tests/regression/<dir> Makefile`. A **new** directory
needs `./autogen.sh`, an `AC_CONFIG_FILES` entry in `configure.ac`, a
`SUBDIRS` entry in the parent `Makefile.am`, and then
`cd build-ver_50 && ./config.status --recheck && ./config.status`.

Three case directories already exist and need no new plumbing:
`tests/regression/case/` (`-D casemode=preserve`),
`tests/regression/case-lt/` (`+ -D ngbehavior=lt`),
`tests/regression/case-pspice/` (`+ -D ngbehavior=ps`) and
`tests/xspice/case/` (own `spinit` loading `analog.cm` and `spice2poly.cm`,
`SPICE_SCRIPTS=.`).

Hazard: never pipe `make` through `head`. SIGPIPE kills make mid-build and
leaves a stale binary. Send build output to a file and grep the file.

Hazard: never interrupt `./autogen.sh` or `./config.status`.

Hazard: do not rebuild while the differential sweep is running.

Hazard: a shell `cd` persists between commands in this harness. Use absolute
paths or `cd` back to the repo root.

## Deliberately out of scope — enumerate, do not fix

- Issue 0011 (gnd rewrite corrupts case-preserved command text).
- Issue 0014 (`numnodes()` dispatches on a lower-case device letter).
- Issue 0015 (numparam `.param`/`.func` symbol names are byte-exact), and
  with it `doc/codex/issues/0020`'s `M=` multiplier gap and
  `doc/codex/issues/0013`'s three exact-match call sites.
- Issue 0016 (`show`/`showmod` match instance names outside `DEVnameHash`).
- Issue 0020's `PRINT VM(2)` gap — control-language surface, takes a
  `distinguish` decision with `doc/codex/issues/0011`.
- Everything gated behind `distinguish`: `src/spicelib/parser/inpptree.c:1256`
  `mkvnode` create-on-miss, `src/frontend/vectors.c:61/71/184`,
  `src/xspice/evt/evtcheck_nodes.c:720`, `evttermi.c:304`.

Anything new you find that you do not fix goes in
`doc/codex/issues/NNNN-slug.md` with the Status / Summary / Impact / Root
Cause / Acceptance Criteria / Resolution structure. Next free number is
**0021**.

## Doc bookkeeping

- Update `doc/codex/issues/0019` in place as rows land: mark each done with
  its line number at the fix and the deck that witnesses it, or the argument
  for why no deck exists.
- Add a `doc/claude/checklists/phase2-differential-sweep.md` section for the
  re-run, in the same shape as the "after issue 0009's follow-up table" one.
- If every row lands, revise `doc/codex/issues/0019`'s Status line.

## Stop and report

What changed per file, RED evidence per behavioural change, the before/after
sweep verdict counts compared per deck, which rows of issue 0019 are still
open, whether each change is fold-mode visible and the argument either way,
anything deliberately left undone, and the full `make check` result.

---

## Why this step, and what follows it

Chosen over `doc/codex/issues/0015` because it is where the remaining
*silent wrong numbers* are. `inpcompat.c:1101` and `:1153` drop a device's
area factor with no diagnostic on an ordinary PSpice deck, which is the one
failure mode this whole series exists to rule out, and the differential sweep
cannot see either of them because it never passes `-D ngbehavior=ps`. 0015 by
contrast is a name-space design decision that pulls `doc/codex/issues/0020`
and three of `doc/codex/issues/0013`'s call sites along with it; it is
cheaper to take once the mechanical residue is gone.

It also has the best evidence-per-edit ratio left. Every row is one token,
three of the four harnesses it needs already exist, and `elem_letter()` is
already exported to the two files that need it.

After this, in order: 0015 (numparam symbols, with 0020's `M=` gap and
0013's three exact-match sites), then the four `distinguish` gates
(`inpptree.c` `mkvnode`, `vectors.c` aliasing, `evtcheck_nodes.c`
auto-bridge, `evttermi.c` event nodes), then a build-enforced lint so the
sweep does not decay, then Phase 3 and its five RED decks.
