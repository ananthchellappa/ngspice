# Next-session prompt — issue 0022, numparam's built-in function names

Paste everything below the line into a fresh session at
`/home/qflow/dev/ngspice_test`, branch `ver_50`.

---

Close `doc/codex/issues/0022` on branch `ver_50`: numparam matches its built-in
function list byte-exactly, so under `-D casemode=preserve` `{SQRT(1e6)}`,
`{MAX(a,b)}` and `{NINT(x)}` are not recognised as functions, fall through to
the parameter branch and abort the run with `Undefined parameter [SQRT]`. Take
`doc/codex/issues/0023` with it — the subcircuit multiplier pass leaks the
parameter pair it appends and can write one past the end of two arrays — because
it is three lines in a function the previous round already had open, and it is
not case-related, so it must not be argued as if it were.

Read `doc/codex/issues/0022` first; it is the work list, and its RED deck is
already written down. Then `doc/codex/issues/0023`,
`doc/claude/suggestions/case-sensitive-identifiers-plan.md` §1.1 and §1.2 (the
Phase 1 keyword class and the no-op argument that comes with it), and
`doc/codex/issues/0015`'s Resolution — this is the gap 0015 uncovered and
deliberately did not fix, and its Resolution says why.

Phases 0, 1 and 2 and issues 0009, 0010, 0013, 0015, 0017, 0018, 0019 and 0020
gap 1 are committed. Do not redo any of them. HEAD is `a1161ccdc`.

## Baseline, measured on this tree at HEAD

- full `make check` = **204 tests, 0 FAIL**. That is the acceptance bar.
- `python3 doc/claude/scripts/case_differential_sweep.py --jobs 12 --timeout 90`
  = **245 decks: OK=191, DIFF=51, PARSE-FAIL=0, NUM-DIFF=0, SKIP=3**.
  `PARSE-FAIL` and `NUM-DIFF` must both stay 0. The 3 SKIPs are stock-run
  timeouts (`tests/mesa/mesa-12.cir`, `tests/mesa/mesa12.cir`,
  `tests/vbic/FG.cir`) and stay. The sweep takes about 12 minutes.
  **Measure your own baseline and compare per deck, not against these totals.**
  The comparison that matters is a `diff` of the two runs' non-OK lists, which
  is all the sweep prints.
- Three of the 51 `DIFF` entries are this issue:
  `tests/regression/parser/xpressn-1.cir`, `xpressn-2.cir`, `xpressn-3.cir`.
  On the uppercased copy of `xpressn-1.cir` **every** diagnostic is
  `Undefined parameter [<FUNCTION NAME>]` — 19 of them `[NINT]`. Expect those
  three to improve; whether they reach `OK` is unmeasured, because those decks
  are large and may carry a second gap behind this one. Measure it, and if
  something else is left, enumerate it rather than chase it.

## The two pieces, in the order to take them

### 1. `doc/codex/issues/0022` — the built-in function list.

Anchors verified at HEAD:

```c
/* src/frontend/numparam/xpressn.c:92  */ static const char *fmathS = "sqr sqrt sin ..."
/* src/frontend/numparam/xpressn.c:630 */ keyword(const char *keys, const char *s, const char *s_end)
/* src/frontend/numparam/xpressn.c:640 */     while ((p < s_end) && (*p == *keys))
/* src/frontend/numparam/xpressn.c:1075 */    fu = keyword(fmathS, s, s_next);
```

`keyword()` has exactly one caller, so the fold goes inside `keyword()`.

Fold only the **deck** side: `fmathS` is already lower case, so
`tolower_c(*p) == *keys` is the whole change. Do not fold `*keys` as well and do
not rewrite the list.

Decide explicitly, and say it in the commit, that this is the Phase 1
**keyword** treatment — fold unconditionally, no `inp_case_folding()` gate —
because a built-in function name is not a name the user chose. The argument that
it is a fold-mode no-op is the Phase 1 one: under `fold` the reader has already
lowercased the card, so `tolower_c()` cannot change the compared bytes. State
it, do not imply it.

Two things not to break:

- The end-of-token test at `:642`, `(p >= s_end) && ((unsigned char)(*keys) <= ' ')`,
  is what keeps `sqr` from swallowing `sqrt`. Folding a character must not
  change how the list is walked (`strchr(keys, ' ')` at `:644`).
- `vec` and `var` take a string argument and the comment at `:88-90` says they
  must come last in the list. Leave the ordering alone.

One behavioural consequence to write down rather than discover: after this,
`.param SQRT=2` under `preserve` is shadowed by the built-in, exactly as
`.param sqrt=2` already is under `fold`. That is the intended direction — one
policy in both modes — but it is a change, so it belongs in the commit message.

### 2. `doc/codex/issues/0023` — the multiplier pass's own parameter pair.

Anchors verified at HEAD:

```c
/* src/frontend/inpcom.c:4346 */ static int inp_fix_subckt_multiplier(...)
/* src/frontend/inpcom.c:4353 */     subckt_param_names[num_subckt_params] = copy("m");
/* src/frontend/inpcom.c:4462 */     inp_fix_subckt_multiplier(subckt_w_params, a->line, ...)
```

The call site drops the return value and its free loop runs to the pre-call
count, so the pair leaks; and `inp_get_params()` can return exactly `NPARAMS`
(`inpcom.c:70`, 10000), so the write at `:4353` can go one past the end of both
arrays. The pair is never read afterwards — the values that reach
`inp_fix_inst_line()` come from a fresh `inp_get_params()` of the rewritten
`.subckt` card.

**This one cannot be given a failing test**, and that is the reason to say so
before implementing rather than after: the leak is invisible to `make check`, and
the array write needs 10000 assignments on one card. Do not invent a deck for it.
If you want evidence, `valgrind --leak-check=full` on
`tests/regression/case/subckt-mult-param-case.cir` shows the two blocks; quote
that instead of a test. Keep it in its own commit so the 0022 commit stays one
mechanism.

## Conventions

- Fold with `tolower_c()` / `cieq()` / `cistrstr()` as the site demands.
  `elem_letter()` (`src/frontend/inpcom.c:930`) is for a card's **leading device
  letter** only — the rule `doc/codex/issues/0018` set.
- Anything gated on mode uses `inp_case_folding()`. This change is **not** gated;
  that is the point of the keyword class.
- Match the style of the file being edited: four-space indent, same-line opening
  brace, no reflowing, no whitespace churn.
- Commit messages: short imperative summary, then problem, mechanism, and the RED
  evidence. State whether the change is fold-mode visible, with the argument.

## Method — RED-first per AGENTS.md, non-negotiable

1. write or extend the test, 2. run it and show the failure is for the expected
reason, 3. smallest production change, 4. re-run the targeted test then the
directory's `make check`, 5. full `make check` before the commit. Never weaken an
assertion or regenerate a `.out` to hide a failure.

Twin-pair pattern: an upper/lower pair with byte-identical `.out` files, so the
assertion is "the spelling cannot change the number". Change **only the bytes
under test** between the twins.

Suggested deck: `tests/regression/case/builtin-func-case.cir` and its
`-lower` twin, one resistance computed from `{SQRT(...)}`, one from
`{MAX(...)}`, one from `{NINT(...)}` and one from `{TERNARY_FCN(...)}`, printed
as separate node voltages so a single abort cannot hide the rest. Pick arguments
whose results are exact in decimal.

## Test harness hazards — most obvious tests are invalid without these

- `tests/bin/check.sh:20` filters BOTH sides through a large `egrep -v` that
  removes `Error`, `Warning`, timings, memory, index columns, any line containing
  `---` (so every `listing param` line too), and the words `nalysis`, `oise`, `urrent`, `est`, `time`, `trans`,
  `Data`, `Total`. **Run every new deck against an empty `.out` first; if it
  passes, its evidence is not a number and the deck is worthless.**
- `check.sh` captures **stdout only**. `Undefined parameter [...]` goes to
  stderr, so this issue's failure mode is an empty stdout: the deck asserts a
  *missing* number, and the upper deck will pass against an empty `.out` while
  the lower twin fails. That asymmetry is the proof the pair is valid.
- **The first line of a deck is its title**, and it is consumed. Start every deck
  with a `*` comment and put `.OPTIONS noacct` on line 2.
- Use numeric node names for anything printed.
- Do not print with `vm(...)`/`vp(...)`/`vdb(...)` — `doc/codex/issues/0020`
  gap 2 — and do not reach a plot with `setplot <name>`; `setplot previous`
  survives the sweep's uppercasing. A control-language function such as
  `mag(...)` echoes its own name as the printed label and breaks the uppercased
  copy too.
- A `.cir` absent from its directory's `TESTS` is silently never run.
- `tests/.gitignore` ignores `*.out`, so every reference needs `git add -f`.
- CIDER is off in `build-ver_50`. Do not claim coverage of a `#ifdef CIDER` path.

## Build

```
make -C build-ver_50 -j14
make -C build-ver_50/tests/regression/case check TESTS=<name>.cir
make -C build-ver_50 check                       # full suite, currently 204
```

Adding a test file to an existing `Makefile.am` needs only
`make -C build-ver_50/tests/regression/<dir> Makefile` — touch the `Makefile.am`
first or make will call it up to date.

Four case directories already exist and need no new plumbing:
`tests/regression/case/` (`-D casemode=preserve`),
`tests/regression/case-lt/`, `tests/regression/case-pspice/`,
`tests/xspice/case/`.

Hazard: never pipe `make` through `head`. SIGPIPE kills make mid-build and leaves
a stale binary. Send build output to a file and grep the file.

Hazard: do not rebuild while the differential sweep is running.

Hazard: a shell `cd` persists between commands in this harness. Use absolute
paths or `cd` back to the repo root.

Hazard: do not write a background wait loop whose condition greps `ps` or
`pgrep` for a command string — it matches its own command line and never exits.
Wait on a sentinel file instead.

## Deliberately out of scope — enumerate, do not fix

- Issue 0011 (the `gnd` rewrite runs over command text it should not touch).
- Issue 0014 (`numnodes()` dispatches on a lower-case device letter,
  `src/frontend/subckt.c`).
- Issue 0016 (`show`/`showmod` match instance names outside `DEVnameHash`).
- Issue 0020 gap 2, `PRINT VM(2)` — control-language surface; it takes a
  `distinguish` decision together with issue 0011.
- Issue 0021 (a stale `.cm` segfaults the loader).
- Everything gated behind `distinguish`: `src/spicelib/parser/inpptree.c:1256`
  `mkvnode` create-on-miss, `src/frontend/vectors.c:59/71/184`,
  `src/xspice/evt/evtcheck_nodes.c:720`, `evttermi.c:304`.
- Phase 3 and its five RED decks. Do not start it.

Anything new you find that you do not fix goes in
`doc/codex/issues/NNNN-slug.md` with the Status / Summary / Impact / Root Cause /
Acceptance Criteria / Resolution structure. Next free number is **0024**.

## Doc bookkeeping

- Update `doc/codex/issues/0022` in place as its four criteria land, and revise
  its Status line if all four are met.
- Update `doc/codex/issues/0023` the same way.
- Add a `doc/claude/checklists/phase2-differential-sweep.md` section for the
  re-run, in the same shape as the "Re-run after `doc/codex/issues/0015` and
  `0020` gap 1" one, and correct that section's "three of them" arithmetic if
  the three `xpressn-*` decks do not all clear.
- `doc/claude/suggestions/next-session-0013-keyword-search-fold.md` and
  `next-session-0015-numparam-symbols.md` are still untracked, unlike
  `next-session-0019-...md`. Commit them or say why not.

## Stop and report

What changed per file, RED evidence per behavioural change, the before/after
sweep verdict counts compared per deck, which criteria of 0022 and 0023 are still
open, whether each change is fold-mode visible and the argument either way,
anything deliberately left undone, and the full `make check` result.

---

## Why this step, and what follows it

It is the last piece that is mechanical. `doc/codex/issues/0022` is one gate in
one function with exactly one caller, it is measurable in the sweep — three
`DIFF` entries whose diagnostics are known to be nothing else — and it needs no
design decision, because a built-in function name is unambiguously a keyword.
`0023` rides along because it is in a function the last round left open and
because leaving a known one-past-the-end write recorded but unfixed is worse than
the three-line change.

After this, the `distinguish` design decision is unavoidable: the four gates
(`inpptree.c` `mkvnode` create-on-miss, `vectors.c` aliasing,
`evtcheck_nodes.c` auto-bridge, `evttermi.c` event nodes), then issues
0011/0014/0016 and `0020` gap 2, which are cheap once it is settled, then a
build-enforced lint so the sweep does not decay, then Phase 3. The plan's own
prerequisites list — the `visualc/*.vcxproj` checklist, `man/man1/ngspice.1` and
`NEWS` for `-D`, the `sharedspice.h` contract note, the OSDI decision record, the
CIDER/`tclspice` scope paragraph — is still entirely open and none of it needs
code.
