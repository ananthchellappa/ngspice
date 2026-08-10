# Next-session prompt — issue 0009's follow-up table

Paste everything below the line into a fresh session at
`/home/qflow/dev/ngspice_test`, branch `ver_50`.

---

Close `doc/codex/issues/0017` and then work `doc/codex/issues/0009`'s follow-up
table on branch `ver_50`: roughly 30 first-character device-letter tests in
`src/frontend/inpcom.c` that classify a card by comparing its leading byte to a
lower-case literal, plus four more that are the same defect on a character
inside a token. They are correct only because `inp_read()` lowercased the card
first, and under `-D casemode=preserve` that fold is gated off.

Read `doc/codex/issues/0009` first — its Resolution ends in the table you are
working, and its "Method note on this enumeration" says how the table was built
and what it is known to miss. Then read `doc/codex/issues/0017`, which is the
table's own highest-priority row and the only one with a measured, reproducible
failure at HEAD. Then `doc/claude/checklists/phase2-differential-sweep.md`,
`doc/claude/suggestions/case-sensitive-identifiers-plan.md` and
`doc/claude/code_analysis/case-insensitivity-origins.md` — those three are
ground truth. Also read `doc/codex/issues/0013`, the immediately preceding piece
of work: it introduced the `_1`-core/`_exact`-sibling shape you will see in
`inpcom.c`, and its Resolution's "Found while fixing this, deliberately not
fixed" section is a partial map of what you are about to touch.

Do not start Phase 3. Do not touch issues 0011, 0014, 0015 or 0016.

Phases 0, 1, 2 and issues 0009, 0010 and 0013 are committed. Do not redo any of
them. HEAD is `cf3edd306`.

Baseline, measured on this tree at HEAD:

- full `make check` = **114 tests, 0 FAIL**. That is the acceptance bar.
- `python3 doc/claude/scripts/case_differential_sweep.py --jobs 12 --timeout 90`
  = **154 decks: OK=108, DIFF=43, PARSE-FAIL=0, NUM-DIFF=0, SKIP=3**.
  `PARSE-FAIL` and `NUM-DIFF` must both stay 0. The 3 SKIPs are stock-run
  timeouts (`tests/mesa/mesa-12.cir`, `tests/mesa/mesa12.cir`,
  `tests/vbic/FG.cir`) and stay. The sweep takes about 9 minutes. Compare it
  per deck against a baseline you measure yourself, not against the totals
  quoted here — one timing-sensitive deck moves between runs.

## Do issue 0017 first, and finish it before opening anything else

It is one token, it has a RED that reproduces at HEAD, and until it lands every
other deck you write for an upper-case `F`/`H`/`M`/`O`/`U`/`Y`/`D`/`Q`/`J`/`Z`/
`S`/`W`/`X` card sits downstream of a live defect that can silently rewrite its
model name.

```c
/* src/frontend/inpcom.c:8735, inp_quote_params() */
        if (strchr("fhmouydqjzswx", *curr_line))
            num_terminals++;
```

RED, verbatim, reproduces at `cf3edd306`:

```spice
.OPTIONS noacct
.param nmod=7
V1 1 0 dc 5
V2 3 0 dc 5
R1 1 2 10k
M1 2 3 0 0 nmod
.model nmod nmos level=1 vto=1 kp=2e-5
.control
op
print v(2)
.endc
.end
```

```
$ ngspice --batch m.cir                        -> v(2) = 3.432236e+00
$ ngspice -D casemode=preserve --batch m.cir   -> Error on line 7 or its substitute:
                                                  Error: incomplete or empty netlist
```

The model name has been rewritten to `{nmod}`. Lower-casing `M1` alone makes
the `preserve` run agree. The `X` twin of the same shape — `.param divider=7`
with `X1 1 2 divider PARAMS: rtop=1k` — gives `Error: unknown subckt: X1 1 2
{divider} rtop=1k`, and is the one issue 0013 re-aimed: before 0013 the `x`
arm's terminal count was one too high and the two errors cancelled exactly.
Write both decks with lower-case twins, per issue 0017's acceptance criteria 2
and 3.

## The table, with anchors verified at HEAD

`doc/codex/issues/0009`'s line numbers are against `e96b4cd5c`. The offset to
`cf3edd306` is **+33 below `inpcom.c:6037`** and **+96 above it** — the two
commits since then added 33 lines before the whole-token search helpers and 63
more inside them. Do not trust the offset; every anchor below was re-located by
`grep` at HEAD, and you should re-locate again before editing, because the file
churns.

**Group A — ungated, or gated only on *not* `ngbehavior=s3`.** These are the
ones `tests/regression/case/` can test as it stands.

| e96b4cd5c | HEAD | Function | Test |
| ---: | ---: | --- | --- |
| 8639 | **8735** | `inp_quote_params()` | `strchr("fhmouydqjzswx", *curr_line)` — **issue 0017** |
| 4331 | 4364 | `inp_fix_subckt_multiplier()` | `strchr("*vehaknopstuwy", curr_line[0])` |
| 4338 | 4371 | `inp_fix_subckt_multiplier()` | `curr_line[0] == 'b'` |
| 5041 | 5074 | `inp_fix_param_values()` | `*line == 'b'` |
| 6289 | 6385 | `inp_compat()` | `*curr_line == 'e'` |
| 6569 | 6665 | `inp_compat()` | `*curr_line == 'g'` |
| 6813 | 6909 | `inp_compat()` | `*curr_line == 'f'` |
| 6860 | 6956 | `inp_compat()` | `*curr_line == 'h'` |
| 6911 | 7007 | `inp_compat()` | `*curr_line == 'r'` |
| 6993 | 7089 | `inp_compat()` | `*curr_line == 'c'` |
| 7093 | 7189 | `inp_compat()` | `*curr_line == 'l'` |
| 7152 | 7248 | `inp_compat()` | `*curr_line == 'k'` |
| 7509 | 7605 | `inp_bsource_compat()` | `*curr_line == 'b'` |
| 8806 | 8902 | `inp_vdmos_model()` | `curr_line[0] == 'm' && cistrstr(...)` |
| 8943 | 9039 | `inp_meas_current()` | `*v == 'a' && s[-1] == '%'` |
| 8951 | 9047 | `inp_meas_current()` | `*s == 'v'` — inside `i(...)`, not a card's first char |
| 9056 | 9152 | `inp_meas_current()` | `(tok[0] == 'e') \|\| (tok[0] == 'h')` |
| 9902 | 9998 | `inp_poly_2g6_compat()` | `switch (*thisline)` |
| 9946 | 10042 | `inp_poly_2g6_compat()` | second `switch (*thisline)`, unreachable only because the first `continue`s |

The four `inp_compat()` `value`/`table` searches issue 0013 folded
(`inpcom.c:6393`, `:6416`, `:6673`, `:6698`) sit **inside** the `6385`/`6665`
arms, so an all-upper deck still never reaches them. Fixing 6385 and 6665 is
what makes 0013's fold observable there; that is the cleanest pairing in this
group and worth doing as one commit with one deck.

**Group B — `newcompat.lt` gated.** `tests/regression/case/` cannot run these:
its `TESTS_ENVIRONMENT` passes `-D casemode=preserve` only.

| e96b4cd5c | HEAD | Function | Test |
| ---: | ---: | --- | --- |
| 3121 | 3154 | `is_a_modelname()` | `newcompat.lt && *line == 'r'` — RKM `R1 1 2 4k7` |
| 3127 | 3160 | `is_a_modelname()` | `newcompat.lt && *line == 'c'` — `C1 2 0 4u7` |
| 3133 | 3166 | `is_a_modelname()` | `newcompat.lt && *line == 'l'` — `L1 1 3 4m7` |

**Group C — `#ifdef XSPICE`.** `tests/regression/case/` and
`tests/regression/misc/` both use `tests/bin/spinit`, which is three lines long
and loads **no** code models, so nothing XSPICE can be tested there and a
`quit 0` skip is indistinguishable from a pass. `tests/xspice/digital/` is the
precedent: its `Makefile.am` sets `SPICE_SCRIPTS=.` so `check.sh` picks up a
local `spinit`.

| e96b4cd5c | HEAD | Function | Test |
| ---: | ---: | --- | --- |
| 2794 | 2827 | `replace_freq()` | `pt = (*line == 'e') ? 'v' : 'i';` — silently picks the wrong controlling quantity |
| 2873 | 2906 | `inp_chk_for_e_source_to_xspice()` | `*line == 'e' && inp_chk_for_multi_in_vcvs(...)` |
| 2875 | 2908 | `inp_chk_for_e_source_to_xspice()` | `*line != 'e' && *line != 'g'` |

**Group D — same class, not a card's first character.** Issue 0009 lists these
separately and says they belong with 0013. 0013 did not take them.

| e96b4cd5c | HEAD | Function | Test |
| ---: | ---: | --- | --- |
| 3206 | **3239** | `is_a_modelname()` | `(*st == 'f') \|\| (*st == 'h')` — unit suffix inside a value token |
| 2711 | 2744 | `replace_freq()` | the identifier scan accepts `'a'..'z'` only |
| 4713 | 4746 | `inp_do_macro_param_replace()` | `strchr("vi", p[-1])` |
| 5940 | 5974 | `b_transformation_wanted()` | `strpbrk(p, "vith")` feeding `cieqn` comparisons |
| 8748 | 8844 | `inp_vdmos_model()` | `cut_line[5] == 'p'` tested right after `cieqn(cut_line, "vdmos", 5)` matched case-insensitively, so `.model m1 VDMOSP` loses its channel type |

**`inpcom.c:3239` is the highest-value row after 0017, and the only one the
sweep can already see.** It is why `tests/polezero/pz{2,t}.cir` report `DIFF`
rather than `OK` today (`warning, can't find model '1h'` on `l1 1 0 1H`) and why
`tests/general/schmitt.cir` reports `DIFF` (`'5pf'` on `cload 7 0 5pf`). Fixing
it should move all three from `DIFF` to `OK` — measure that, do not assume it.
The three `tests/bsim3soi{dd,fd,pd}/ring51.cir` decks will **not** move: their
`1pF` warning is gone from the diff already and what remains is transient
`reference value` timing, which differs between two consecutive *stock* runs.

## Also in scope: the two lower-case-only guards outside `inpcom.c`

Issue 0013's Resolution found these while enumerating; they are the same defect
in the same class, in a file issue 0009 did not sweep, and each blocks a keyword
site 0013 already folded.

```
src/frontend/inpcompat.c:164   *cut_line == 'e' || *cut_line == 'g'
                                 gates replace_table()'s "value"/"cur" searches
src/frontend/inpcompat.c:923   *cut_line == 'r' || *cut_line == 'l' || *cut_line == 'c'
                                 gates the "tc" search
src/frontend/inp.c:2259        strchr("*vbiegfh", curr_line[0])
src/frontend/inp.c:2592        *curr_line != 'b'
                                 gate the two search_identifier() sites in inp.c
```

`src/frontend/inpcom.c:7671` already spells the same test
`strchr("*vbiegfhVBIEGFH", curr_line[0])`. That is the pattern someone started
and did not finish; `elem_letter()` (`inpcom.c`, added by issue 0009) is the
in-tree helper for the single-character form.

## Method — RED-first per AGENTS.md, non-negotiable

1. write or extend the test, 2. run it and show the failure is for the expected
reason, 3. smallest production change, 4. re-run the targeted test then the
directory's `make check`, 5. full `make check` before the commit. Never weaken
an assertion or regenerate a `.out` to hide a failure. If a site genuinely
cannot be given a failing test first, say why before implementing it — issue
0013 had exactly one such site and recorded the argument rather than skipping
it.

RED tests: new decks in `tests/regression/case/`, which already runs every deck
under `-D casemode=preserve`. Follow the twin-pair pattern of
`tests/regression/case/device-letter-case.cir` and its `-lower` twin: an
upper/lower pair with byte-identical `.out` files, so the assertion is "the
spelling cannot change the number" rather than "this run printed this". Verify
each reference against the **`fold`-mode** run of both twins before committing
it, the way issue 0013 did.

Split per mechanism. A single combined deck aborts on the first error and hides
the rest; that happened on 0009, on 0010 and on 0013.

Groups B and C need a harness that does not exist yet. Decide explicitly, and
say which: a new `tests/regression/case-lt/` with
`-D casemode=preserve -D ngbehavior=lt` on `check.sh`'s `$1` (the
`tests/regression/case/Makefile.am` line is the precedent — the extra flags ride
on the unquoted `$1`), or a `tests/xspice/`-side directory with its own
`spinit` and `SPICE_SCRIPTS=.` (the `tests/xspice/digital/Makefile.am`
precedent). Do not put an XSPICE deck in `tests/regression/case/` and call it
covered.

## Conventions

- `elem_letter()` in `src/frontend/inpcom.c` is the existing helper: it folds a
  copy and mutates nothing, the way `src/spicelib/parser/inppas2.c:92-94` does
  for the parser's own dispatch. Use it. Do not add a second helper.
- For `strchr` skip lists, prefer `strchr(list, elem_letter(line))` over
  doubling the list, unless the list is already spelled in both cases nearby.
- Match the style of the file being edited: four-space indent, same-line
  opening brace, no reflowing, no whitespace churn. `inpcom.c` churns constantly
  upstream, so keep the diff to single-token edits where possible.
- Anything touching global or static simulator state must survive repeated
  `ngSpice_Reset` in the shared build.
- Commit messages: short imperative summary, then problem, mechanism, and the
  RED evidence. State whether the change is fold-mode visible.

## The fold-mode question

Issue 0009's Resolution argued its change was fold-mode invisible because "the
reader's fold exemptions all cover dot cards and control lines rather than
device cards". **That argument is now known to be false**, and issue 0013's
Resolution says why with a worked example: `is_xspice_model()`
(`inpcom.c:420`) diverts any `.model` card mentioning `d_state` and friends to
`keep_case_of_cider_param()`, which preserves everything between two quotes;
the `plot`/`gnuplot`/`hardcopy` arm at `:1820` and the print redirection tail at
`:1884` both spare text on cards whose first byte still folds to a device
letter, so `Gnuplot 4 0 xlabel VALUE 1.0` is a legal VCCS named `nuplot`
carrying an unfolded token. `tests/regression/misc/keyword-fold-guard.cir` pins
one of these on a number.

Unlike issue 0013, most of this table **cannot** be gated: folding a device
letter with `elem_letter()` is unconditional by design, and issue 0009 already
shipped it that way. So for each site you must decide whether a card that the
reader spared can reach it with an upper-case first byte, and if so whether the
fold changes what the default mode does. Where it can, add a
`tests/regression/misc/` deck and a paragraph in the commit message. Where it
cannot, say so with the argument, not with a reference to 0009's argument.

## Test harness hazards — most obvious tests are invalid without these

- `tests/bin/check.sh:20` filters BOTH actual and expected through `egrep -v`
  `"SPARSE|KLU|CPU|Dynamic|Note|Circuit|Trying|Reference|Date|Doing|---|v-sweep|
  time|est|Error|Warning|Data|Index|trans|acan|oise|nalysis|ole|Total|memory|
  urrent|Got|Added|BSIM|bsim|B4SOI|b4soi|codemodel|^binary raw file|
  ^ngspice.*done|Operating"`.
  A test whose only evidence is a diagnostic passes whether or not the bug
  exists. Issue 0013 proved this: four of its five RED decks **passed against an
  empty `.out`**. Run every new deck against an empty `.out` first; if it
  passes, its evidence is not a number and the deck is worthless.
- Start every deck with `.OPTIONS noacct` — the rusage block is not filtered and
  is machine-dependent.
- Use numeric node names for anything you print. Under `preserve` a node name
  keeps its case in the vector name, so `print v(Out)` and `print v(out)`
  produce different text and the twin decks stop having identical `.out` files.
- A `.cir` absent from its directory's `TESTS` is silently never run, and a RED
  test that never ran looks exactly like a passing one.
- `tests/.gitignore` ignores `*.out`, so every expected-output file needs
  `git add -f`.
- Tests that write files need a `CLEANFILES` entry in the local `Makefile.am`.
- CIDER is off in `build-ver_50`
  (`build-ver_50/src/include/ngspice/config.h:11`). Do not claim coverage of a
  `#ifdef CIDER` path from a build that cannot compile it.

## Build

```
make -C build-ver_50 -j14
make -C build-ver_50/tests/regression/case check TESTS=<name>.cir
make -C build-ver_50 check                       # full suite, currently 114
```

Adding a test file to a `Makefile.am` does **not** need a full `./autogen.sh`:
`make -C build-ver_50/tests/regression/<dir> Makefile` re-runs `automake` and
`config.status` for that one directory in about two seconds. A **new**
directory does need `./autogen.sh` plus a `SUBDIRS` entry in
`tests/regression/Makefile.am` and an `AC_CONFIG_FILES` entry in
`configure.ac`.

Existing out-of-tree tree is `build-ver_50` (bare `../configure`). XSPICE and
OSDI on, CIDER off, WITH_PSS off, RFSPICE on.

Hazard: never pipe `make` through `head` or anything that can close the pipe
early. SIGPIPE kills make mid-build and leaves a stale binary, and the next test
run reports a failure that has nothing to do with your change. Send build output
to a file and grep the file.

Hazard: never interrupt `./autogen.sh` or `./config.status`. A killed run leaves
half-regenerated build files and the next build fails in `src/xspice/verilog`
with `relocation R_X86_64_PC32 against symbol stderr ... recompile with -fPIC`.
That is not a real defect; re-run both to completion and it goes away.

Hazard: do not rebuild while the differential sweep is running. It swaps the
binary underneath the sweep and the result is meaningless.

## Deliberately out of scope — enumerate, do not fix

- Issue 0011 (gnd rewrite corrupts case-preserved command text). Its acceptance
  criterion 4 was unblocked by issue 0013; note that and move on.
- Issue 0014 (`numnodes()` dispatches on a lower-case device letter). It is the
  same defect class in `src/frontend/subckt.c`, and it is tempting. It has its
  own issue and its own RED; leave it.
- Issue 0015 (numparam `.param`/`.func` symbol names are byte-exact). Issue
  0013's three exact-match call sites are coupled to it and must move with it,
  not before it.
- Issue 0016 (`show`/`showmod` and friends match instance names outside
  `DEVnameHash`).
- Everything gated behind `distinguish`: `src/spicelib/parser/inpptree.c:1256`
  `mkvnode` create-on-miss, `src/frontend/vectors.c:61/71/184` vector-table
  aliasing, `src/xspice/evt/evtcheck_nodes.c:720` and `evttermi.c:304`. If your
  change makes an XSPICE deck behave differently, stop and write it up rather
  than following it.
- `src/frontend/inpc_probe.c:21` re-declares `search_plain_identifier()`, which
  `src/frontend/inpcom.h:14` already declares. Dead duplication; leave it.

Anything new you find that you do not fix goes in
`doc/codex/issues/NNNN-slug.md` with the Status / Summary / Impact / Root Cause
/ Acceptance Criteria / Resolution structure. Next free number is **0018**.

## Doc bookkeeping

- Update `doc/codex/issues/0009`'s follow-up table in place as rows land: mark
  each done with its HEAD line number, and leave the rest with their
  `e96b4cd5c` numbers plus the measured offset. Do not rewrite the table.
- Add a `doc/claude/checklists/phase2-differential-sweep.md` section for the
  re-run, in the same shape as the "after 0013" section.
- If you close all of Group A and D, `doc/codex/issues/0009`'s Status line
  ("Fixed for the enumerated sites") needs revising.

## Stop and report

What changed per file, RED evidence per behavioural change, the before/after
sweep verdict counts compared per deck, which of issue 0017's four acceptance
criteria are met and which rows of issue 0009's table are still open, whether
each change is fold-mode visible and the argument either way, anything
deliberately left undone, and the full `make check` result.

---

## Why this step, and what follows it

Chosen because it is the only remaining item with a measured, reproducible
failure at HEAD — issue 0017's `M`-card RED — and because it is the last block
of Phase 1 work. Everything after it is either a name space that needs a design
decision (0015's numparam symbols) or `distinguish` plumbing that cannot be
tested until the four silent-failure gates are closed.

It also has the best evidence-per-edit ratio left on the roadmap. Most rows are
one token. `inpcom.c:3239` alone should take three decks from `DIFF` to `OK` in
the sweep, and issue 0017 is what makes every future upper-case device-card deck
trustworthy.

After this, in order: 0015 (numparam), then the four `distinguish` gates
(`inpptree.c` `mkvnode`, `vectors.c` aliasing, `evtcheck_nodes.c` auto-bridge,
`evttermi.c` event nodes), then a build-enforced lint so the sweep does not
decay, then Phase 3 itself and its five RED decks — none of which exist yet, and
one of which needs its own `spinit`.
