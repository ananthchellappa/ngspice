# Next-session prompt — Phase 3 gate 1, `mkvnode`'s create-on-miss

Paste everything below the line into a fresh session at
`/home/qflow/dev/ngspice_test`, branch `ver_50`.

---

Close **Phase 3 gate 1** on branch `ver_50`: `mkvnode()`
(`src/spicelib/parser/inpptree.c:1249`) creates a node on a miss, so a B source
whose `V()` reference differs only in case from a real net manufactures a new
node instead of reporting anything. Under `casemode=distinguish` that is the
worst failure mode on the gate list, because in the common case it runs to
completion and prints a wrong number.

Read `doc/claude/decisions/0001-distinguish.md` first — the whole file, and
decision 2 twice, because it already fixes the *rule* this gate has to
implement and this session's job is to implement it, not to re-decide it. Then
`doc/claude/specs/case-sensitive-identifiers.md`'s "Silent-failure sites that
gate `distinguish`" table, then
`doc/claude/suggestions/case-sensitive-identifiers-plan.md` Phase 3.

Phases 0, 1 and 2 and issues 0009, 0010, 0011, 0013, 0014, 0015, 0016 class (a),
0017, 0018, 0019, 0020 gap 1, 0022, 0023 and 0026 are committed, as is Phase 3
gate 3. Do not redo any of them. HEAD is `bba506257`.

## Baseline, measured on this tree at HEAD

- full `make check` = **221 tests, 0 FAIL**. That is the acceptance bar.
- `python3 doc/claude/scripts/case_differential_sweep.py --jobs 12 --timeout 90`
  = **262 decks: OK=208, DIFF=51, PARSE-FAIL=0, NUM-DIFF=0, SKIP=3**.
  `PARSE-FAIL` and `NUM-DIFF` must both stay 0. The 3 SKIPs are stock-run
  timeouts (`tests/mesa/mesa-12.cir`, `tests/mesa/mesa12.cir`,
  `tests/vbic/FG.cir`) and stay. About 15 minutes.
  **Measure your own baseline and compare per deck, not against these totals.**
- Of the 51 `DIFF`s, six are known and explained in
  `doc/claude/checklists/phase2-differential-sweep.md`'s last section: the two
  `harness-alive.cir` probes echo `$casemode` and must differ, and the four
  `alter-rebin*.cir` decks are the `stdout reordered only` class, which is a
  line-ordering artefact and not a numeric disagreement. **Read the detail
  column, not the verdict.** `ring51.cir` still flips between runs; re-run with
  `--filter ring51` before calling a single-deck flip anything.

## The defect, both of its shapes, measured at HEAD

```spice
* B source referencing a node that differs only in case
.OPTIONS noacct
V1 In 0 dc 1.5
R1 In 0 1k
B1 out 0 V={V(in)*2}
R2 out 0 1k
```

Under `-D casemode=preserve`, `in` and `In` are one net and `v(out)` is
`3.000000e+00`. Under `-D casemode=distinguish` the deck asks for a net named
`in`, there is none, and `mkvnode` mints one.

**Shape 1, silent, and it is the one the spec describes.** Give the deck any
shunt that puts the manufactured node on a DC path — `.OPTIONS noacct
rshunt=1e9` is enough — and it runs to completion:

```
preserve      v(out) = 3.000000e+00
distinguish   v(out) = 0.000000e+00
```

No warning, no error, a number that is wrong by a factor of infinity.

**Shape 2, loud but misleading, and the spec does not mention it.** Without a
shunt the manufactured node is genuinely isolated — an expression reference
contributes no conductance to that node's own row — so the matrix is singular
and the run aborts:

```
Warning: singular matrix:  check node in
...
Error: Transient op failed, timestep too small
op simulation(s) aborted
```

The deck never wrote `in`. A user reading that message goes looking for a node
that does not exist in their netlist. Both shapes are the same defect and the
same fix; **correct the spec's "manufactures a floating node at 0 V and runs to
completion" to say that this holds only when the manufactured node has a DC
path, and that otherwise the diagnostic names a node the deck never wrote.**

## The design problem, which is the whole session

**`mkvnode`'s create-on-miss is load-bearing and you cannot simply refuse it.**
Measured at HEAD, in `fold` and in `distinguish` alike:

```spice
* forward reference: B source names a node defined by a later card
.OPTIONS noacct
B1 out 0 V={V(mid)*2}
R2 out 0 1k
V1 mid 0 dc 1.5
R1 mid 0 1k
```

`v(out) = 3.000000e+00`. The B source is parsed before the cards that define
`mid`, and it works precisely because `INPtermInsert` creates the node. Replace
that call with `INPtermSearch` (`src/spicelib/parser/inpsymt.c:308`, which
already does exactly the non-creating probe and returns `E_EXISTS` or 0) and
every forward reference in the tree breaks. Confirm that for yourself before
designing anything: it is the single fact the design turns on.

So the diagnostic has to be **deferred to the end of the parse**, not raised at
`mkvnode`. Roughly: a node that was created by `mkvnode` and was never
subsequently claimed by a device terminal is a node no card defines, and if a
name differing from it only in case *is* defined, that is decision 2's
resolution miss and must be reported. Decide and record:

1. Where the "created by a reference, never defined" bit lives. `CKTnode` is
   the obvious home (`src/include/ngspice/cktdefs.h`); `INPnTab`
   (`src/spicelib/parser/inpsymt.c`) is the other candidate and is parser-local,
   which may be the better fence. Say why you chose one.
2. Where the end-of-parse sweep runs, and whether it runs in **all** modes or
   only under `distinguish`. Argue it. A node no card defines is suspicious in
   `fold` too, and this is the only chance in the whole feature to catch a plain
   typo — but a diagnostic that fires in the default mode is a behaviour change
   for every existing deck and needs the full `make check` and sweep to back it.
   If you make it `distinguish`-only, say what it costs.
3. Whether it is a warning or an error. Decision 2 says an unresolved node is
   not an error in SPICE and rejects abort-on-miss; that argument was made for
   the general case and you should test it against this specific one rather
   than inherit it.
4. Whether `INPtermSearch`'s token substitution (`FREE(*token)` then
   `*token = t->t_ent`) is safe on the paths you add it to. It frees the
   caller's string on a hit, which `mkvnode`'s caller does not currently expect.

Anchors verified at HEAD:

```c
/* src/spicelib/parser/inpptree.c:1249 */ static INPparseNode *mkvnode(char *name)
/* :1256 */    INPtermInsert(circuit, &name, tables, &temp);
/* callers at :1549, :1550 (the two-node V(a,b) form) and :1564 */
/* src/spicelib/parser/inpsymt.c:308 */ int INPtermSearch(CKTcircuit*, char**, INPtables*, CKTnode**)
/* src/spicelib/parser/inpsymt.c:31  */ static bool ent_eq(...)  /* now inp_case_exact_ids() */
```

## The RED deck

`tests/regression/casedist/bsource-node-case.cir`, in the directory that
already exists and already runs every deck under `-D casemode=distinguish`.
Add it to that directory's `TESTS` and run
`make -C build-ver_50/tests/regression/casedist Makefile` first — touch
`Makefile.am` or make will call it up to date. The directory is registered in
`configure.ac` and `tests/regression/Makefile.am` already; no `autogen.sh`
needed unless you add another directory.

Its evidence must be a **number**, because `tests/bin/check.sh:20` filters every
line containing `Error` or `Warning` out of both sides. So the deck asserts the
voltage the correct interpretation produces, and the diagnostic you add is
verified by hand and quoted in the commit message. Use the `rshunt` shape: the
un-shunted shape asserts a *missing* number, which is weaker evidence and
couples the deck to the singular-matrix path.

Second deck worth having, and it is the regression guard for the design
problem: a forward-reference deck whose B source names a node defined by a
later card, in the *same* case. It must keep working. That one belongs in
`tests/regression/case/` as a twin pair, because it has to hold under
`preserve` too.

## Conventions

- `ng_ideq()` (`src/frontend/inpcom.c:1076`) is identifier identity;
  `inp_case_exact_ids()` (`:1060`) is the predicate behind it and is what a new
  identity site should ask. `inp_case_folding()` asks whether the reader
  lowercased the card and is for language keywords. Decision 3 of the record
  classifies all twenty-eight existing sites; put any new one in the table.
- Match the style of the file being edited: four-space indent, same-line opening
  brace, no reflowing, no whitespace churn.
- One commit per mechanism. Expected: the spec correction, the deck plus
  harness, the production change.
- Commit messages: short imperative summary, then problem, mechanism, and the
  RED evidence. State whether the change is fold-mode visible, with the
  argument, and state what it does to `preserve`.

## Method — RED-first per AGENTS.md, non-negotiable

1. write or extend the test, 2. run it and show the failure is for the expected
reason, 3. smallest production change, 4. re-run the targeted test then the
directory's `make check`, 5. full `make check` before the commit. Never weaken
an assertion or regenerate a `.out` to hide a failure.

## Test harness hazards — most obvious tests are invalid without these

- `tests/bin/check.sh:20` filters BOTH sides through a large `egrep -v` that
  removes `Error`, `Warning`, timings, memory, index columns, any line
  containing `---`, and the words `nalysis`, `oise`, `urrent`, `est`, `time`,
  `trans`, `Data`, `Total`, `ole`. **Run every new deck against an empty `.out`
  first; if it passes, its evidence is not a number and the deck is worthless.**
  Note that `Notice:` survives — `Note` is not a prefix of `Noti` — so a
  `Notice:` line is a real assertion.
- `check.sh` diffs with `diff -B -w`: whitespace and blank lines are invisible.
- `check.sh` captures **stdout only**. Your new diagnostic will go to `stderr`
  and no deck can assert it. Verify it by hand and quote it.
- **The first line of a deck is its title**, and it is consumed. Start every
  deck with a `*` comment and put `.OPTIONS noacct` on line 2.
- Use numeric node names for anything printed, except where the deck is
  specifically about node-name case — which this one is, so use named nodes and
  remember that `src/frontend/outitf.c:1164` stores a digit-leading node name as
  `V(<name>)` with an upper-case V in every mode.
- Do not print with `vm(...)`/`vp(...)`/`vdb(...)` — `doc/codex/issues/0020`
  gap 2 — and do not reach a plot with `setplot <name>`.
- A `.cir` absent from its directory's `TESTS` is silently never run.
- `tests/.gitignore` ignores `*.out`, so every reference needs `git add -f`.
- CIDER is off in `build-ver_50`. Do not claim coverage of a `#ifdef CIDER` path.

## Build

```
make -C build-ver_50 -j14
make -C build-ver_50/tests/regression/casedist check TESTS=<name>.cir
make -C build-ver_50 check                       # full suite, currently 221
```

Hazard: never pipe `make` through `head`. SIGPIPE kills make mid-build and
leaves a stale binary. Send build output to a file and grep the file.

Hazard: **do not rebuild while the differential sweep is running.** Relinking
`src/ngspice` under a running sweep invalidates it; if you do it by accident,
kill the sweep and start it again rather than trusting the result.

Hazard: `git stash` + `make` to measure a pre-fix number leaves the binary built
from the stashed tree. Rebuild after `git stash pop`.

Hazard: a shell `cd` persists between commands in this harness. Use absolute
paths or `cd` back to the repo root.

Hazard: do not write a background wait loop whose condition greps `ps` or
`pgrep` for a command string — it matches its own command line and never exits.
Wait on a sentinel file instead.

## Deliberately out of scope — enumerate, do not fix

- Phase 3 gates 2 and 4: `src/xspice/evt/evtcheck_nodes.c:720` auto-bridge
  `strcmp` and `src/xspice/evt/evttermi.c:304`. Both are XSPICE and both want
  an `xspice-event-case.cir` harness with its own `spinit`, following
  `tests/xspice/digital/spinit.in` and registered in `AC_CONFIG_FILES`. That
  harness is a session's work before any production change, which is why they
  come after this one.
- `doc/codex/issues/0027`, `vec_remove()`'s unconditional `cieq`.
- `doc/codex/issues/0016` class (b), `gens.c:289` and `:295`.
- `doc/codex/issues/0024` sites 1 and 2. Site 3 was examined and declined.
- `doc/codex/issues/0025`, `0020` gap 2, `0021`.
- Subcircuit formal pins under `distinguish`, and whether two case-variant nets
  are independently present in the **rawfile**. Both are listed as untested in
  the spec's acceptance criteria and neither is a gate.
- The OSDI duplicate-parameter diagnostic, decision 4 of the record.
- The build-enforced lint that stops a new `strcmp` against a lower-case
  literal from re-entering the tree. Still wanted, still needs no decision,
  still a session of its own.

Anything new you find that you do not fix goes in
`doc/codex/issues/NNNN-slug.md` with the Status / Summary / Impact / Root Cause /
Acceptance Criteria / Resolution structure. Next free number is **0028**.

## Stop and report

What changed per file, RED evidence per behavioural change, the before/after
sweep verdict counts compared per deck, what the deferred-diagnostic design
decided and what it left open, which Phase 3 gates remain, whether each change
is fold-mode visible and the argument either way, what `preserve` does before
and after (`tests/regression/case/` is the proof), what forward references do
before and after (they must be unchanged), anything deliberately left undone,
and the full `make check` result.

---

## Why this step, and what follows it

Gate 1 is next because its failure mode is the worst of the three that remain
and because it is the only one testable with the harness that already exists.
Gate 3 shipped `tests/regression/casedist/`, so a deck can now name two
case-variant nets separately and assert on both — which is exactly what gate 1
needs and what nothing before it could do. Gates 2 and 4 need an XSPICE event
harness with its own `spinit` that does not exist yet.

It is also the only gate whose fix teaches something the other two will reuse:
the deferred end-of-parse check. The auto-bridge at `evtcheck_nodes.c:720` has
the same shape — a resolution that silently does nothing on a miss — and will
want the same "collect, then diagnose after everything is parsed" structure.

After this: build the XSPICE event harness, then gates 2 and 4 together. The
two things that still need no code and no decision, and could fill a short
session at any point, are `doc/codex/issues/0027` and the `strcmp` lint.
