# Next-session prompt — issues 0011, 0014 and 0016 class (a)

Paste everything below the line into a fresh session at
`/home/qflow/dev/ngspice_test`, branch `ver_50`.

---

Close `doc/codex/issues/0011`, `doc/codex/issues/0014` and the **class (a) half
only** of `doc/codex/issues/0016` on branch `ver_50`. All three are
decision-free: none of them needs the pending `distinguish` decision, and that
was checked rather than assumed — the argument for each is in this prompt and
must be restated, not merely cited. This is the last round before that decision
becomes the only thing left.

`doc/codex/issues/0011` first, and on its own merits: it is the only defect in
the list that is **wrong in the default mode with no `-D` flag at all**, so it
is the only one that is upstreamable today independently of the whole case
feature.

Read `doc/codex/issues/0011` first, then `0014`, then `0016`. Every one of the
three carries line numbers that have drifted; the corrected anchors are in this
prompt and were re-read at HEAD. Then read
`doc/claude/suggestions/case-sensitive-identifiers-plan.md` §2.2 (the "touch
`inpcom.c` control flow exactly once" rule, which 0011 has to respect) and
Phase 3's gate list (to confirm for yourself that none of these three is on
it).

Phases 0, 1 and 2 and issues 0009, 0010, 0013, 0015, 0017, 0018, 0019, 0020
gap 1, 0022 and 0023 are committed. Do not redo any of them. HEAD is
`63d2b6567`.

## Baseline, measured on this tree at HEAD

- full `make check` = **206 tests, 0 FAIL**. That is the acceptance bar.
- `python3 doc/claude/scripts/case_differential_sweep.py --jobs 12 --timeout 90`
  = **247 decks: OK=195, DIFF=49, PARSE-FAIL=0, NUM-DIFF=0, SKIP=3**.
  `PARSE-FAIL` and `NUM-DIFF` must both stay 0. The 3 SKIPs are stock-run
  timeouts (`tests/mesa/mesa-12.cir`, `tests/mesa/mesa12.cir`,
  `tests/vbic/FG.cir`) and stay. The sweep takes about 12 minutes.
  **Measure your own baseline and compare per deck, not against these totals.**
- **The sweep will not move this round.** All three issues were checked against
  the current non-OK list and none of them clears a single entry: no deck under
  `tests/` uses `show`, `showmod`, `save alli`, an upper-case `E`/`G`/`W`/`K`/`X`
  inside a `.subckt`, or a `gnd` token in command text. The sweep is therefore a
  **regression guard only** this round — its job is to prove nothing moved. Run
  it anyway, before and after, and say so in the report. `make check` plus
  hand-written twin pairs are the harness that decides.
- Beware `binning-1.cir`: its `DIFF` verdict is a false positive caused by
  `Reference value :` (`src/frontend/outitf.c:695`), a quarter-second progress
  print that appears or not depending on machine load and is absent from the
  sweep's `NOISE` regex. It may flip between your two runs for no reason. Adding
  `reference value` to that regex is a one-word change and is *worth making this
  round* — but make it **before** you measure your baseline, so both of your runs
  use the same tool, and commit it separately.

## The three pieces, in the order to take them

### 1. `doc/codex/issues/0011` — the `gnd` rewrite's scope.

Anchors verified at HEAD (**the issue's own citations are all ~+89 to +94 lines
stale**):

```c
/* src/frontend/inpcom.c:2468 */ static void inp_fix_gnd_name(struct card *c)   /* ends :2532 */
/* :2475 */        if (*gnd == '*')          /* the only UNCONDITIONAL exclusion */
/* :2480-2484 */   if (newcompat.ps) { ... found_subckt ... }   /* second exclusion */
/* :2481 */        if (ciprefix(".subckt", c->line) && search_plain_identifier(c->line, "gnd"))
/* :2488 */        if (found_subckt || (!cistrstr(gnd, "gnd") && !strstr(gnd, "/0")))
/* :2495 */        while ((gnd = cistrstr(gnd, "gnd")) != NULL) {
/* :2496-2497 */       /* the delimiter guard */
/* :2509 */        /* third scan, KiCad only, inside if (newcompat.ki) */
/* :2530 */        c->line = inp_remove_ws(c->line);   /* unconditional */
/* :1252-1253 */   if (!cp_getvar("no_auto_gnd", CP_BOOL, NULL, 0))
                       inp_fix_gnd_name(working);
```

**The function has two card exclusions, not one**, and the second is
behaviourally live — do not collapse them while scoping the pass. `*` at `:2475`
is unconditional; the `newcompat.ps` block at `:2480-2484` sets `found_subckt`
from a `.subckt` header that names `gnd` and clears it at `.ends`, and `:2488`
skips every card in between, so under PSpice mode a subcircuit may have a *port*
called `gnd`. Measured on one deck whose `.subckt blk a b gnd` port is wired to
a 0.5 V node:

```
$ ngspice --batch ps.cir                   -> v(2) = 5.000000e-01   # gnd -> node 0
$ ngspice -D ngbehavior=ps --batch ps.cir  -> v(2) = 7.500000e-01   # gnd stays a port
```

`nexttok()` at `:2492`/`:2508`/`:2519` is a third, positional exclusion: the
first token of every line is spared.

Both exposures reproduce **in the default mode, no `-D` flag**:

```
.control
op
echo GNDCHECK Gnd rail
echo lower case: my gnd rail
.endc
```
```
$ ngspice --batch gnd.cir      ->  GNDCHECK 0 rail
                                   lower case: my 0 rail
```

and on a command with side effects, not only `echo`:
`shell echo "SH1 rail gnd here"` prints `SH1 rail 0 here`.

The lower-case half has been broken for as long as the function has existed.
The mixed-case half is reachable because the reader's case-preserving whitelist
(`inpcom.c:1912-1923`) deliberately spares `echo`/`shell`/`write`/`wrdata`/
`source`/`cd`/`load`/`setcs`/`strcmp`/`strstr` arguments from lowercasing, so
`Gnd` arrives at `:2495` with its case intact and `cistrstr` matches it.

**Two of the issue's own claims are wrong and must be corrected in the file, not
worked around:**

1. Its "wider exposure" is the narrower one. The unconditional
   `inp_remove_ws()` at `:2530` does fire without a substitution — measured, two
   `echo` lines identical except that one contains `mygndtag` (where `gnd` is not
   delimiter-bounded, so nothing is substituted) and only that one loses its
   whitespace runs and its spaces around `=`. But its **only** surface is `echo`
   lines inside `.control`, because `inp_remove_excess_ws()` already ran at
   `inpcom.c:1205`, 48 lines earlier, and normalised every other card. So the
   issue's claim that a `.lib` path with a `gnd` component "is still whitespace-
   mangled by `inp_remove_ws()`" is true of the deck and **false of this
   function** — that card was mangled at `:1205` regardless. Demote exposure two
   below exposure one and rewrite that paragraph.
2. Its Root Cause cites `inpcom.c:5954` as proof the PSPICE `.subckt` guard is
   case-sensitive. Stale: the helper is now at `inpcom.c:6103` with its wrapper
   at `:6131` passing `ci = !inp_case_folding()`.

Also verified and worth knowing before you design the fix: a card **ending** in
`gnd` is never rewritten, because the newline zap at `inpcom.c:1978` makes
`gnd[3]` a `'\0'` and the delimiter guard rejects it. `echo tail token is gnd`
comes out unchanged while `echo paren form v(gnd)` becomes `v( 0 )`. Do not
"fix" that asymmetry by accident.

Two precedents in the same file, both to be reused rather than reinvented:
`inp_stripcomments_line()` (`inpcom.c:3697`) skips quoted spans at `:3720-3735`,
and `inp_rem_unused_models()` (`:9742`) tracks `.control` nesting at
`:9752-9763`.

Why it does not need `distinguish`: ground is a reserved word, not an
identifier — `gnd`/`GND`/`Gnd` alias node 0 in every mode by specification — so
the identity question has no jurisdiction here. And the fix is *containment*, a
predicate over which cards the pass may look at, not a change to what `gnd`
matches. State that argument in the commit; do not assume it.

Respect §2.2: `inpcom.c` churns constantly upstream. Add the scoping as a guard
on the pass, not as a restructure of the caller.

### 2. `doc/codex/issues/0014` — `numnodes()`'s device letter.

Anchors verified at HEAD (**the issue cites `subckt.c:1673`; that has drifted
+44, and every `inpcom.c` citation in it is stale too**):

```c
/* src/frontend/subckt.c:1716 */ static int
/* :1717 */ numnodes(const char* line, struct subs* subs)
/* :1719 */     switch (*line) {          <- the only line that needs editing
/* :1720-1726 */    case 'e': 'g': 'w':  -> 2 ;  case 'k': -> 0 ;  case 'x':
/* :1731 */         if (eq_substr_id(xname, xname_e, subs->su_name))
/* :1735 */     return get_number_terminals((char *)line);
```

Call sites: `subckt.c:1390` (inside `translate()`'s `case 'e'/'f'/'g'/'h'` arm,
head at `:1366-1369`) and `subckt.c:1480` (the `default:` arm, head at `:1467`).

It reproduces as a **hard abort**, not a wrong number:

```spice
* upper-case E inside a subckt body
.OPTIONS noacct
v1 1 0 dc 1
x1 1 2 amp
.subckt amp a b
E1 b 0 a 0 2.0
.ends
.control
op
print v(2)
.endc
.end
```
```
$ ngspice --batch ecase.cir                       -> v(2) = 2.000000e+00
$ ngspice -D casemode=preserve --batch ecase.cir  -> Error: too few devs: E1 b 0 a 0 2.0
```

The lower-case `e1` twin, every other byte identical, runs under `preserve`.
`K` aborts the same way. The mechanism is worth stating in the commit because
the diagnostic misdirects: `translate()` folds its own local copy at `:1202`, so
it *does* enter `case 'e'`; `numnodes()` then returns 4 instead of 2 and eats
the control nodes, `numdevs()` (`:1744-1774`, which enumerates both cases
already) returns 2, and the abort surfaces one loop later as `too few devs`
rather than as a node-count error.

The rest of `subckt.c` is already clean — `translate()`'s fold at `:1202`,
`translate_inst_name()` at `:1159`, `finishLine()` at `:1561-1562`/`:1583`,
`numdevs()` at `:1750-1772`, `devmodtranslate()` at `:1906-1908`. `numnodes()`
is the last one.

**The one real decision here is a header, not a fold.** `doc/codex/issues/0018`
set the rule that `elem_letter()` is the helper for a card's leading device
letter, and this is exactly that case. It is declared `extern` at
`src/frontend/inpcom.h:19` and defined at `src/frontend/inpcom.c:930` — but
`subckt.c` does not include `inpcom.h` (its include block is `:58-81`). Adding
that include pulls a large header into a large file; folding with `tolower_c()`
in place does not, at the cost of not using the named helper. Pick one, say
which and why, and do not do both.

`get_number_terminals()` (`inpcom.c:5400`, dispatch at `:5409`) is already
folded by issue 0009, which is why the fallback silently returns the wrong
count instead of failing: K gives 2 where `numnodes()` wants 0 (`:5413`/`:5419`),
W gives 3 where it wants 2 (`:5456`/`:5458`), E/G give 4 where they want 2
(`:5462-5463`/`:5466`), X gives token-count-minus-2 where it wants `su_numargs`
(`:5440-5452`). The style precedent issue 0014's own criterion 1 names,
`src/spicelib/parser/inppas2.c:92-94`, has **not** drifted.

Why it does not need `distinguish`: `numnodes()` reads no identifier. It reads
one byte of card *syntax* to decide how many tokens are node names, and there is
no dialect in which `E1` and `e1` are different device types. It is not on the
plan's Phase 3 gate list.

### 3. `doc/codex/issues/0016`, class (a) only.

The five sites split cleanly in two, and the split is visible in the code
itself: `gens.c:289` indexes `dev_name + 1`, deliberately stepping over the
device letter, so the letter and the identity are already one pointer offset
apart in the same expression.

| site | class | verified at HEAD |
| --- | --- | --- |
| `src/frontend/gens.c:260` | **(a)** device letter | `} else if (type != *dev_name) {` |
| `src/frontend/device.c:1489` | **(a)** literal `'m'` = MOSFET | `if ((dev[0] == 'm') && (eqc(param, "w") \|\| eqc(param, "l")))` |
| `src/frontend/outitf.c:391` | **(a)** literal `'d'` = diode | `} else if (strstr(ch, "#internal") && (tmpname[1] == 'd')) {` |
| `src/frontend/gens.c:289` | (b) instance identity | `strcmp(device, dev_name + 1 + subckt_len)` |
| `src/frontend/gens.c:295` | (b) model identity | `strcmp(model, mod_name)` |

**Fix class (a) only.** Class (b) is the `distinguish` decision's territory and
must not be touched. That the split is safe was argued rather than assumed:
`gens.c:260` is a *pre-filter*, so folding it can only widen the candidate set
that reaches `:289`, and any instance that then fails the byte-exact name
compare is rejected exactly as today. The resulting intermediate state — `show r`
works, `show Rser` still does not — is coherent, not half-broken. `gens.c:276`
already uses `ciprefix()` for the subcircuit component, so the file is already
internally inconsistent about this.

`outitf.c:391` is the one worth a deck: under `preserve`, a deck with `D1` and
`save alli` does not merely lose the diode branch current, it ends with zero
data descriptors and reports `Error: no data saved for D.C. Operating point
analysis; analysis not run`. The same deck with `d1` and everything else still
upper case runs. `device.c:1420` is the gate that makes all of this reachable:
the typed `alter`/`altermod` name is lowercased only under `fold`.

Note `device.c:1489`'s own internal inconsistency — the *parameter* names on
that line already use `eqc()` while the device letter does not.

## Conventions

- Fold with `tolower_c()` / `cieq()` / `cistrstr()` as the site demands.
  `elem_letter()` (`src/frontend/inpcom.c:930`) is for a card's **leading device
  letter** only — the rule `doc/codex/issues/0018` set — which is why piece 2
  raises the header question rather than silently answering it.
- These changes are **not** gated on `inp_case_folding()`: a device letter and a
  reserved word are language syntax, which is the Phase 1 keyword class. Issue
  0011's scoping guard is a different thing again — it is not a case predicate at
  all and must not acquire one.
- Match the style of the file being edited: four-space indent, same-line opening
  brace, no reflowing, no whitespace churn.
- One commit per mechanism. Four commits are expected: the sweep `NOISE`
  one-liner, 0011, 0014, and 0016 class (a).
- Commit messages: short imperative summary, then problem, mechanism, and the
  RED evidence. State whether the change is fold-mode visible, with the
  argument. 0011 **is** fold-mode visible — that is the point of it — and the
  other two are not.

## Method — RED-first per AGENTS.md, non-negotiable

1. write or extend the test, 2. run it and show the failure is for the expected
reason, 3. smallest production change, 4. re-run the targeted test then the
directory's `make check`, 5. full `make check` before the commit. Never weaken an
assertion or regenerate a `.out` to hide a failure.

Twin-pair pattern: an upper/lower pair with byte-identical `.out` files, so the
assertion is "the spelling cannot change the number". Change **only the bytes
under test** between the twins.

**Issue 0011 does not take a twin pair**, and that is the trap. It is wrong in
the default mode, so its deck belongs in `tests/regression/misc/`, not in
`tests/regression/case/` — everything in the latter runs under
`-D casemode=preserve` and would prove nothing about the mode the bug lives in.
Its evidence is an echoed sentinel, so pick one that survives the filter: no
`est`, no `time`, no `trans`, no `Data`, no `Total`, no `---`.

Suggested decks:

- `tests/regression/misc/gnd-command-text.cir` — default mode, a `.control`
  block echoing a sentinel around both `gnd` and `Gnd`, asserting the words come
  back unchanged. Add a second line proving a real netlist `gnd` still becomes
  node 0, or the fix could pass by disabling the feature.
- `tests/regression/case/numnodes-letter-case.cir` and its `-lower` twin — an
  upper-case `E` inside a `.subckt`, one node voltage. Add `K` if you want the
  zero-node arm covered; they abort identically today, so one deck per arm keeps
  the evidence separable.
- `tests/regression/case/save-alli-case.cir` and its `-lower` twin — `D1` plus
  `save alli`, evidence a printed current.

## Test harness hazards — most obvious tests are invalid without these

- `tests/bin/check.sh:20` filters BOTH sides through a large `egrep -v` that
  removes `Error`, `Warning`, timings, memory, index columns, any line containing
  `---`, and the words `nalysis`, `oise`, `urrent`, `est`, `time`, `trans`,
  `Data`, `Total`. **Run every new deck against an empty `.out` first; if it
  passes, its evidence is not a number and the deck is worthless.**
- Note `urrent` in that filter: a deck whose evidence is a *branch current* label
  can lose it. Print into a node voltage where you can.
- `check.sh` captures **stdout only**. `Error:` diagnostics go to stderr, so an
  aborting deck asserts a *missing* number and the upper deck will pass against
  an empty `.out` while the lower twin fails. That asymmetry is the proof the
  pair is valid.
- **The first line of a deck is its title**, and it is consumed. Start every deck
  with a `*` comment and put `.OPTIONS noacct` on line 2.
- Use numeric node names for anything printed.
- Do not print with `vm(...)`/`vp(...)`/`vdb(...)` — `doc/codex/issues/0020`
  gap 2 — and do not reach a plot with `setplot <name>`.
- A `.cir` absent from its directory's `TESTS` is silently never run.
- `tests/.gitignore` ignores `*.out`, so every reference needs `git add -f`.
- CIDER is off in `build-ver_50`. Do not claim coverage of a `#ifdef CIDER` path.

## Build

```
make -C build-ver_50 -j14
make -C build-ver_50/tests/regression/case check TESTS=<name>.cir
make -C build-ver_50/tests/regression/misc check TESTS=<name>.cir
make -C build-ver_50 check                       # full suite, currently 206
```

Adding a test file to an existing `Makefile.am` needs only
`make -C build-ver_50/tests/regression/<dir> Makefile` — touch the `Makefile.am`
first or make will call it up to date.

Hazard: never pipe `make` through `head`. SIGPIPE kills make mid-build and leaves
a stale binary. Send build output to a file and grep the file.

Hazard: do not rebuild while the differential sweep is running.

Hazard: a shell `cd` persists between commands in this harness. Use absolute
paths or `cd` back to the repo root.

Hazard: do not write a background wait loop whose condition greps `ps` or
`pgrep` for a command string — it matches its own command line and never exits.
Wait on a sentinel file instead.

## Deliberately out of scope — enumerate, do not fix

- `doc/codex/issues/0016` **class (b)**, `gens.c:289` and `:295`.
- `doc/codex/issues/0024` sites 1 and 2 (`var()` → `cp_getvar`, `vec()`'s plot
  qualifier). Site 3, `spicenum.c:367`, is a one-line Phase 1 fold and may ride
  along if you want it; say which you did.
- `doc/codex/issues/0025` — a parameter named after a built-in evaluates to 0
  with no diagnostic. It changes the **default** mode's numbers and needs a
  design decision.
- `doc/codex/issues/0020` gap 2, `PRINT VM(2)`, and `doc/codex/issues/0021`.
- Everything gated behind `distinguish`: `src/spicelib/parser/inpptree.c:1256`
  `mkvnode` create-on-miss, `src/frontend/vectors.c:59/71/184`,
  `src/xspice/evt/evtcheck_nodes.c:720`, `evttermi.c:304`.
- Phase 3 and its five RED decks. Do not start it.

Anything new you find that you do not fix goes in
`doc/codex/issues/NNNN-slug.md` with the Status / Summary / Impact / Root Cause /
Acceptance Criteria / Resolution structure. Next free number is **0026**.

## Doc bookkeeping

- Update `doc/codex/issues/0011` in place, **including the two corrections to its
  own premise** described above, and revise its Status line.
- Update `doc/codex/issues/0014` and `0016` in place. `0016` stays Open with
  class (b) outstanding; say so explicitly rather than closing it.
- Fix the stale line numbers in all three issue files while you are in them.
  `0011` is ~+89/+94 out, `0014` is +44 out in `subckt.c` and wholly stale in
  `inpcom.c`, `0016` has not drifted at all.
- Add a `doc/claude/checklists/phase2-differential-sweep.md` section for the
  re-run, in the same shape as the "Re-run after `doc/codex/issues/0022` and
  `0023`" one, and record that the sweep was a pure regression guard this round.
  If you took the `NOISE` one-liner, say that the two runs used the amended tool.

## Stop and report

What changed per file, RED evidence per behavioural change, the before/after
sweep verdict counts compared per deck, which criteria of 0011, 0014 and 0016
are still open, whether each change is fold-mode visible and the argument either
way, which of the two options you took for `elem_letter()` in `subckt.c` and
why, anything deliberately left undone, and the full `make check` result.

---

## Why this step, and what follows it

Because it is everything that is left that does **not** need the `distinguish`
decision, and one third of it is a bug in the shipping default. `0011` is the
only remaining defect a user hits with no flag set, which makes it the only one
worth offering upstream on its own; `0014` is the last unfolded device-letter
dispatch in the tree, in the one file `doc/codex/issues/0009` did not reach; and
`0016` class (a) is three one-character comparisons that were only ever bundled
with class (b) by the accident of living in the same table.

None of the three moves the sweep, which is worth saying plainly: the sweep has
given what it can. From here the harness that decides is `make check` plus
hand-written pairs, exactly as it was for `0015`, `0019` and `0022`.

After this, the `distinguish` design decision is unavoidable and nothing else
can proceed without it. It governs the four Phase 3 gates (`inpptree.c`
`mkvnode` create-on-miss, `vectors.c` aliasing, `evtcheck_nodes.c` auto-bridge,
`evttermi.c` event nodes), plus `0016` class (b), `0020` gap 2, and
`doc/codex/issues/0024` sites 1 and 2. The `vectors.c` gate is the one to take
first, because until two case-variant nets can be addressed independently the
feature is unobservable and the other three cannot be tested at all.

Two things remain that need no code and no decision, and could fill a short
session at any point: the plan's own prerequisites list — the `visualc/*.vcxproj`
(×3) checklist, `man/man1/ngspice.1` and a `NEWS` bullet for `-D`, the
`sharedspice.h` contract note on the `ngGet_Vec_Info`/`ngGet_Evt_NodeInfo` case
asymmetry, the OSDI decision record (`src/osdi/osdiinit.c:74`), and the
CIDER/`tclspice` out-of-scope paragraph — and the build-enforced lint that stops
a new `strcmp` against a lower-case literal from re-entering the tree.
