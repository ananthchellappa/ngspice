# Next-session prompt — issue 0015, numparam symbol names

Paste everything below the line into a fresh session at
`/home/qflow/dev/ngspice_test`, branch `ver_50`.

---

Close `doc/codex/issues/0015` on branch `ver_50`: numparam keeps `.param`,
`.func` and `.func`-formal symbols in `nghash` tables created with
`nghash_init()`, i.e. `strcmp`, so under `-D casemode=preserve` a `.param`
spelled one way at the definition and another at the reference is not found
and the run aborts. Take `doc/codex/issues/0020`'s `M=` multiplier gap with
it — that one **is** a silent wrong number and it is the last known one — and
take a decision, with the argument written down, on
`doc/codex/issues/0013`'s three exact-match call sites.

These three move together because they are one question: whether a name the
*user* chose, as opposed to a language keyword, is matched case-insensitively
under `preserve`. Folding one side of that name space and not the other is
exactly what `doc/codex/issues/0013`'s Resolution argues against.

Read `doc/codex/issues/0015` first — it is the work list. Then
`doc/codex/issues/0020` (both gaps; only gap 1 is in scope),
`doc/codex/issues/0010` (whose `NUPA_SUBCKT` special case this issue's
criterion 4 says to delete), and `doc/codex/issues/0013`'s Resolution,
specifically "decision 2" and the three exceptions. Then
`doc/claude/suggestions/case-sensitive-identifiers-plan.md` §2.3 "Identity
discipline" — it is the constraint that decides the shape of the fix — and
`doc/claude/code_analysis/case-insensitivity-origins.md`. Those are ground
truth.

Phases 0, 1 and 2 and issues 0009, 0010, 0013, 0017, 0018 and 0019 are
committed. Do not redo any of them. HEAD is `e5913af03`.

## Baseline, measured on this tree at HEAD

- full `make check` = **188 tests, 0 FAIL**. That is the acceptance bar.
- `python3 doc/claude/scripts/case_differential_sweep.py --jobs 12 --timeout 90`
  = **229 decks: OK=173, DIFF=53, PARSE-FAIL=0, NUM-DIFF=0, SKIP=3**.
  `PARSE-FAIL` and `NUM-DIFF` must both stay 0. The 3 SKIPs are stock-run
  timeouts (`tests/mesa/mesa-12.cir`, `tests/mesa/mesa12.cir`,
  `tests/vbic/FG.cir`) and stay. The sweep takes about 12 minutes.
  **Measure your own baseline and compare per deck, not against these
  totals** — one timing-sensitive deck moves between runs. The comparison
  that matters is a `diff` of the two runs' non-OK lists; the sweep prints
  only those.
- Twelve of the 53 `DIFF` decks are case decks and both causes are
  `doc/codex/issues/0020`. Four of those twelve — `subckt-mult-skip-case` and
  its twin, and their uppercased copies — are gap 1 and **should go OK** when
  you fix it. The other eight are gap 2, `PRINT VM(2)`, which is out of scope.

## The three pieces, in the order to take them

### 1. `doc/codex/issues/0020` gap 1 — the `M=` multiplier. Do this first.

It is the only silent wrong number left in the enumerated set, it needs no
new harness, and it is small. Anchors verified at HEAD:

```c
/* src/frontend/inpcom.c:4319 */
static bool found_mult_param(int num_params, char *param_names[])
```
`inp_get_params()` at `:4211` returns instance parameter names spelled as the
user wrote them; `found_mult_param()` compares them against the literal
`"m"`. Callers at `:4440`, `:4444`, `:4447`, `:4450`, `:4493`, `:4495`.

```
$ ngspice --batch mup.cir                        -> v(2) = 1.333333e+00
$ ngspice -D casemode=preserve --batch mup.cir   -> v(2) = 1.000000e+00
```
on a deck carrying `X1 1 2 divider M=2`. The multiplier is simply not
applied: the subcircuit is instantiated once instead of twice, the deck runs,
nothing is printed. `doc/codex/issues/0020` §1 has the deck.

`tests/regression/case/subckt-mult-skip-case.cir` already exists and already
passes; it is the *device-letter* half of the same pass. The new deck is the
parameter-name half, and it must spell `M=` upper case while the device
letter stays lower, the way `bsource-vnode-case.cir` isolates its one byte.

Decide explicitly whether `M` is a keyword (fold unconditionally, like the
rest of Phase 1) or a user parameter name (fold only under `preserve`). It is
a keyword — the user did not choose the name `m` — so the Phase 1 treatment
is probably right, but say so in the commit rather than leaving it implied.

### 2. `doc/codex/issues/0015` — the numparam symbol tables.

Anchors verified at HEAD:

```c
/* src/frontend/numparam/xpressn.c:378 */   entrynb(dico_t *dico, char *s)
/* src/frontend/numparam/xpressn.c:414 */   attrib(dico_t *dico, NGHASHPTR htable_p, char *t, char op)
/* src/frontend/numparam/xpressn.c:269 */   dico->symbols[0] = nghash_init(NGHASH_MIN_SIZE);
/* src/frontend/numparam/xpressn.c:482 */   dico->symbols[dico->stack_depth] = nghash_init(NGHASH_MIN_SIZE);
```

`entrynb()` is the single probe and `attrib()` the single insert for **every**
kind of numparam symbol, which is the blast radius. Their call sites, all of
them:

| site | what flows through |
| --- | --- |
| `xpressn.c:401` | the generic lookup wrapper |
| `xpressn.c:486` | the generic define |
| `xpressn.c:581` | `findsubckt()` — already folded by issue 0010 |
| `xpressn.c:1208` | the substitution scan |
| `spicenum.c:197` | `findsubname()` — already folded by issue 0010 |
| `spicenum.c:526` | `.func` formals / prototype symbols |
| `spicenum.c:559` | `dico->inst_symbols`, the `nupa_subcktcall()` binder |

**Identity discipline, non-negotiable** (plan §2.3): fold the key at the
caller, never install a custom `hash_func`. `src/misc/hash.c:548` does
`copy(user_key)` **only** for `NGHASH_DEF_HASH(NGHASH_FUNC_STR)`, and the
frees at `:108`, `:182`, `:393` are gated the same way, so a custom hash
silently flips the table from owning its keys to borrowing them. Commits
`c5cd68015` and `5ad395d5e` are what that class of bug costs.

The first-seen spelling must survive for reporting; `inpsymt.c:41-46` and
`:158-163` are the precedent for how the interning tables do it.

Criterion 4 asks you to **delete** issue 0010's `NUPA_SUBCKT` special case
once the general fold lands — `defsubckt()` at `xpressn.c:518`,
`findsubckt()` at `:566`/`:581`, `findsubname()` at `spicenum.c:149`/`:197` —
so there is one policy rather than two. Do not skip that; two policies in one
table is how the next sweep gets a false negative.

RED deck: `tests/regression/case/` gains a pair that spells a `.param` name,
a `.func` name and a `.func` formal two ways each. Today the upper deck dies
with `Undefined parameter [rval]` on **stderr**, so what `make check` sees is
an empty stdout — the evidence is the missing voltage, and the deck must be
confirmed to FAIL against an empty `.out` before it is committed.

### 3. `doc/codex/issues/0013`'s three exact-match call sites — decide, then act.

`search_identifier()` and `search_plain_identifier()` fold their literal
under `preserve`; three call sites were deliberately routed to exact-match
entry points instead, because they pass a name the *user* chose rather than a
keyword:

```c
/* src/frontend/inpcom.c:6053  */ static char *search_identifier_exact(...)
/* src/frontend/inpcom.c:6127  */ static char *search_plain_identifier_exact(...)
/* src/frontend/inpcom.c:6064  */ char *ya_search_identifier(...)
```
called at `inpcom.c:5643`, `:8646` and `:8748`.

If issue 0015 makes user parameter names case-insensitive under `preserve`,
these three become inconsistent with it: numparam would resolve `{RVAL}`
against `.param rval` while the textual substitution scan that feeds it would
not. Work out whether that inconsistency is reachable — build a deck and try
— and either fold them under the same gate or record why they must stay
exact. Either answer is acceptable; an unexamined one is not.

## Conventions

- Fold with `tolower_c()` / `cieq()` / `cistrstr()` as the site demands.
  `elem_letter()` (`src/frontend/inpcom.c:934`, declared `inpcom.h:19`) is for
  a card's **leading device letter** only — it would be a lie about what is
  being read anywhere else, which is the rule `doc/codex/issues/0018` set.
- Anything gated on mode uses `inp_case_folding()`, the way
  `search_plain_identifier()` already does.
- Match the style of the file being edited: four-space indent, same-line
  opening brace, no reflowing, no whitespace churn.
- Commit messages: short imperative summary, then problem, mechanism, and the
  RED evidence. State whether the change is fold-mode visible, with the
  argument, per site.

## Method — RED-first per AGENTS.md, non-negotiable

1. write or extend the test, 2. run it and show the failure is for the
expected reason, 3. smallest production change, 4. re-run the targeted test
then the directory's `make check`, 5. full `make check` before the commit.
Never weaken an assertion or regenerate a `.out` to hide a failure. If a site
genuinely cannot be given a failing test first, say why before implementing
it.

Twin-pair pattern: an upper/lower pair with byte-identical `.out` files, so
the assertion is "the spelling cannot change the number". Change **only the
bytes under test** between the twins — the last two rounds both had decks
weakened by an incidental second variable.

Split per mechanism. A single combined deck aborts on the first error and
hides the rest.

## Test harness hazards — most obvious tests are invalid without these

- `tests/bin/check.sh:20` filters BOTH sides through a large `egrep -v` that
  removes `Error`, `Warning`, timings, memory, index columns and the words
  `nalysis`, `oise`, `urrent`, `est`, `time`, `trans`, `Data`, `Total`.
  **Run every new deck against an empty `.out` first; if it passes, its
  evidence is not a number and the deck is worthless.**
- `check.sh` captures **stdout only**. Every `Undefined parameter [...]`, every
  `Error on line ...` is invisible to `make check`. Issue 0015's failure mode
  is exactly this, so the deck asserts a *missing* number.
- **The first line of a deck is its title**, and it is consumed. Start every
  deck with a `*` comment and put `.OPTIONS noacct` on line 2.
- Use numeric node names for anything printed; under `preserve` a node name
  keeps its case in the vector name and the twins stop matching.
- Do not print with `vm(...)`/`vp(...)`/`vdb(...)` — `doc/codex/issues/0020`
  gap 2 — and do not reach a plot with `setplot <name>`: plot names are
  generated in lower case, so the sweep's uppercased copy fails on
  `SETPLOT OP1`. `setplot previous` survives uppercasing. A control-language
  function such as `mag(...)` echoes its own name as the printed label and
  breaks the uppercased copy too.
- A `.cir` absent from its directory's `TESTS` is silently never run.
- `tests/.gitignore` ignores `*.out`, so every reference needs `git add -f`.
  An `.inc` file is matched by neither `$(TESTS)` nor `$(TESTS:.cir=.out)` and
  needs its own `EXTRA_DIST` entry.
- Tests that write files need a `CLEANFILES` entry.
- CIDER is off in `build-ver_50`. Do not claim coverage of a `#ifdef CIDER`
  path.

## Build

```
make -C build-ver_50 -j14
make -C build-ver_50/tests/regression/case check TESTS=<name>.cir
make -C build-ver_50 check                       # full suite, currently 188
```

Adding a test file to an existing `Makefile.am` needs only
`make -C build-ver_50/tests/regression/<dir> Makefile`. A **new** directory
needs `./autogen.sh`, an `AC_CONFIG_FILES` entry in `configure.ac`, a
`SUBDIRS` entry in the parent `Makefile.am`, then
`cd build-ver_50 && ./config.status --recheck && ./config.status`.

Four case directories already exist and need no new plumbing:
`tests/regression/case/` (`-D casemode=preserve`),
`tests/regression/case-lt/` (`+ -D ngbehavior=lt`),
`tests/regression/case-pspice/` (`+ -D ngbehavior=ps`),
`tests/xspice/case/` (own `spinit`, `SPICE_SCRIPTS=.`).

Hazard: never pipe `make` through `head`. SIGPIPE kills make mid-build and
leaves a stale binary. Send build output to a file and grep the file.

Hazard: never interrupt `./autogen.sh` or `./config.status`.

Hazard: do not rebuild while the differential sweep is running.

Hazard: a shell `cd` persists between commands in this harness. Use absolute
paths or `cd` back to the repo root.

Hazard: do not write a background wait loop whose condition greps `ps` or
`pgrep` for a command string — the loop matches its own and its siblings'
command lines and never exits. Wait on a sentinel file instead.

## Deliberately out of scope — enumerate, do not fix

- Issue 0011 (the `gnd` rewrite runs over command text it should not touch).
- Issue 0014 (`numnodes()` dispatches on a lower-case device letter,
  `src/frontend/subckt.c`).
- Issue 0016 (`show`/`showmod` match instance names outside `DEVnameHash`).
- Issue 0020 gap 2, `PRINT VM(2)` — control-language surface; it takes a
  `distinguish` decision together with issue 0011.
- Issue 0021 (a stale `.cm` segfaults the loader) — unrelated to this series.
- Everything gated behind `distinguish`: `src/spicelib/parser/inpptree.c:1256`
  `mkvnode` create-on-miss, `src/frontend/vectors.c:59/71/184`,
  `src/xspice/evt/evtcheck_nodes.c:720`, `evttermi.c:304`.
- Phase 3 and its five RED decks. Do not start it.

Anything new you find that you do not fix goes in
`doc/codex/issues/NNNN-slug.md` with the Status / Summary / Impact / Root
Cause / Acceptance Criteria / Resolution structure. Next free number is
**0022**.

## Doc bookkeeping

- Update `doc/codex/issues/0015` in place as the criteria land, and revise its
  Status line if all five are met.
- Update `doc/codex/issues/0020`: close gap 1, leave gap 2 open, and correct
  its `DIFF` accounting.
- Update `doc/codex/issues/0010` where its `NUPA_SUBCKT` special case is
  deleted, and `doc/codex/issues/0013` with whatever the three exact-match
  sites decision turns out to be.
- Add a `doc/claude/checklists/phase2-differential-sweep.md` section for the
  re-run, in the same shape as the "Re-run after `doc/codex/issues/0019`" one.

## Stop and report

What changed per file, RED evidence per behavioural change, the before/after
sweep verdict counts compared per deck, which criteria of 0015 and which gap
of 0020 are still open, whether each change is fold-mode visible and the
argument either way, the decision taken on 0013's three exact-match sites and
why, anything deliberately left undone, and the full `make check` result.

---

## Why this step, and what follows it

It is the largest remaining `preserve` gap by deck count — `.param NAME` with
`{name}` is ordinary modern style — and it carries the last enumerated silent
wrong number, `doc/codex/issues/0020`'s `M=`. The plan names it as a Phase 3
prerequisite in its own words: "`.param VDD` with `{vdd}`: numparam is the
highest-likelihood break and has no test". And it lets issue 0010's
`NUPA_SUBCKT` special case be deleted, so the numparam tables end up with one
policy instead of two.

It is also the last piece that is *mechanical*. What remains after it —
issues 0011, 0014, 0016, 0020 gap 2 and all of Phase 3 — either needs the
`distinguish` design decision or is control-language surface, and both of
those want the name-space question settled first.

After this, in order: the four `distinguish` gates (`inpptree.c` `mkvnode`,
`vectors.c` aliasing, `evtcheck_nodes.c` auto-bridge, `evttermi.c` event
nodes), then issues 0011/0014/0016 which are cheap once that is settled, then
a build-enforced lint so the sweep does not decay, then Phase 3 and its five
RED decks. The plan's own prerequisites list — the `visualc/*.vcxproj`
checklist, `man/man1/ngspice.1` and `NEWS` for `-D`, the `sharedspice.h`
contract note, the OSDI decision record, the CIDER/`tclspice` scope paragraph
— is still entirely open and none of it needs code.
