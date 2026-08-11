# Next-session prompt — the `distinguish` decision and Phase 3 gate 3 (`vectors.c`)

Paste everything below the line into a fresh session at
`/home/qflow/dev/ngspice_test`, branch `ver_50`.

---

Make the `distinguish` design decision, record it, and land the **`vectors.c`
gate** — Phase 3 gate 3 — on branch `ver_50`. Two decision-free riders come
first in the same session, as their own commits: `doc/codex/issues/0026`, and
the spec/plan anchor corrections listed below.

This is the step the whole feature is blocked on.
`doc/claude/suggestions/case-sensitive-identifiers-plan.md` Phase 3 lists four
gates; the `vectors.c` one has to be first because **until two case-variant nets
can be addressed independently the feature is unobservable and the other three
cannot be tested at all**. Every RED deck Phase 3 needs — `node-case-split.cir`,
`instance-case-split.cir`, `subckt-pin-case.cir`, `subckt-hier-case.cir`,
`xspice-event-case.cir` — asserts on a `print` or a `let`, and every one of
those goes through `findvec()`.

Read `doc/claude/specs/case-sensitive-identifiers.md` first — the whole file,
not just the gate table — then `doc/claude/suggestions/case-sensitive-identifiers-plan.md`
Phase 3 and its `## Prerequisites` section, then
`doc/claude/checklists/phase2-differential-sweep.md`'s last section for what the
sweep can and cannot see.

Phases 0, 1 and 2 and issues 0009, 0010, 0011, 0013, 0014, 0015, 0016 class (a),
0017, 0018, 0019, 0020 gap 1, 0022 and 0023 are committed. Do not redo any of
them. HEAD is `b60e40c2e`.

## Baseline, measured on this tree at HEAD

- full `make check` = **214 tests, 0 FAIL**. That is the acceptance bar.
- `python3 doc/claude/scripts/case_differential_sweep.py --jobs 12 --timeout 90`
  = **255 decks: OK=204, DIFF=48, PARSE-FAIL=0, NUM-DIFF=0, SKIP=3**.
  `PARSE-FAIL` and `NUM-DIFF` must both stay 0. The 3 SKIPs are stock-run
  timeouts (`tests/mesa/mesa-12.cir`, `tests/mesa/mesa12.cir`,
  `tests/vbic/FG.cir`) and stay. The sweep takes about 15 minutes.
  **Measure your own baseline and compare per deck, not against these totals.**
- Two of the 48 `DIFF`s are `tests/regression/case/alter-rebin-case{,-lower}.cir`
  and they are `doc/codex/issues/0026`, not noise. If you fix 0026 they should
  clear; say whether they did.
- `tests/bsim3soi{dd,fd}/ring51.cir` carry detail `stdout reordered only` and
  **flip between runs for no reason**. Do not read a single-deck flip on either
  of them as a fix or a regression without re-running `--filter ring51`.

## The pieces, in the order to take them

### 1. `doc/codex/issues/0026` — the binned-model re-selection letter.

Decision-free, one line, and it is the last open defect that does not need
`distinguish`. Take it first so the decision-free backlog is empty before the
design work starts.

Anchors verified at HEAD:

```c
/* src/frontend/device.c:1272 */  if (param[0] == 'w')
/* src/frontend/device.c:1489 */  if ((tolower_c(dev[0]) == 'm') && (eqc(param, "w") || eqc(param, "l")))
/* src/frontend/device.c:1421-1422 */  strtolower(param); strtolower(dev);   /* inside if (inp_case_folding()) */
```

The caller admits `param` case-insensitively with `eqc()` and the body then
tests `param[0] == 'w'` byte-exactly, so `alter M1 W=2u` under `preserve` takes
the `else`, assigns the new width to the local **length**, and bins on the
unchanged width. RED, both numbers already measured:

```
$ ngspice -D casemode=preserve --batch rebin.cir   # alter M1 w=2u
v(3) = 1.432887e+00 / Notice: model has changed from nch.1 to nch.2. / v(3) = 1.481980e+00
$ ngspice -D casemode=preserve --batch rebinW.cir  # alter M1 W=2u
v(3) = 1.432887e+00 / v(3) = 1.340324e+00
```

`tests/regression/case/alter-rebin-case.cir` is the deck to copy; the twin pair
for this issue varies the **typed parameter letter**, not the device letter.
Not fold-mode visible: `device.c:1421` folds `param` under `inp_case_folding()`.

### 2. Spec and plan anchor corrections — no code.

Every one of these was re-read at HEAD. Fix them in place before you start
relying on the documents.

`doc/claude/specs/case-sensitive-identifiers.md`:

- The gate table's `src/frontend/outitf.c:391` row is now **Done**, commit
  `f38570c43`. Mark it, the way the `measure.c` and `subckt.c` rows are marked.
- `src/frontend/device.c:1417-1418` → `:1421-1422`, and the fold is now gated
  on `inp_case_folding()` at `:1420`.
- The claimed `spiceif.c:1101` `cieq` / `:1115` `eq` asymmetry **no longer
  exists**: `:1115` is `eqc(dev->modelParms[i].keyword, param)`. Delete the
  claim rather than re-citing it.
- `inp_casefix` `:3544-3557` → the function is at `inpcom.c:3653`.
- `ngGet_Vec_Info` `src/sharedspice.c:1215` → `:1194`. `ngGet_Evt_NodeInfo` is
  **not** in `evtshared.c:253`; it is `src/sharedspice.c:1441`.
- `src/spicelib/parser/inpptree.c:1256` → `mkvnode` is at `:1249`; the
  create-on-miss is `INPtermInsert(circuit, &name, tables, &temp)` at `:1256`,
  so that number is right for the call and wrong for the function.

`doc/claude/suggestions/case-sensitive-identifiers-plan.md` `## Prerequisites`:

- "A contract note in `src/include/ngspice/sharedspice.h` covering the
  `ngGet_Vec_Info` / `ngGet_Evt_NodeInfo` case asymmetry" is **already done** —
  `sharedspice.h:58-70` says exactly that, including "it compares with strcmp,
  so it only accepts the". Mark it done rather than writing it again.

The remaining prerequisites are still open and still need no decision, so they
can ride along or wait: the `visualc/*.vcxproj` (×3) checklist item,
`man/man1/ngspice.1` plus a `NEWS` bullet for `-D`, the OSDI decision record
(`src/osdi/osdiinit.c:74`, `strtolower(para_name)`), and the CIDER/`tclspice`
out-of-scope paragraph. Say which you took.

### 3. The `distinguish` decision itself, written down before any code.

The mode value **is rejected today**:

```c
/* src/include/ngspice/fteext.h:210-212 */
    NG_CASE_FOLD = 0,     /* R1 == r1, spelling lowercased (default) */
    NG_CASE_PRESERVE,     /* R1 == r1, spelling as first typed */
    NG_CASE_DISTINGUISH   /* R1 != r1, spelling as typed; not implemented */

/* src/frontend/inpcom.c:1079-1082 */
    else if (cieq(mode, "distinguish"))
        fprintf(stderr,
                "Warning: casemode 'distinguish' is not implemented yet, "
                "using 'fold'\n");
```

so `set_case_mode()` (`inpcom.c:1066`) never sets `NG_CASE_DISTINGUISH` and
`ng_ideq()` (`inpcom.c:1055-1058`) has only two arms:

```c
bool ng_ideq(const char *a, const char *b)
{
    return inp_case_folding() ? (strcmp(a, b) == 0) : (cieq(a, b) != 0);
}
```

**`ng_ideq()` is the decision, expressed in one function.** Under `preserve` it
returns `cieq`; under `distinguish` it has to return `strcmp`, and every caller
that relies on the `cieq` arm to make `preserve` work then changes meaning. The
decision record must enumerate those callers, not gesture at them —
`inp_case_folding()` has call sites all over `inpcom.c` and each one is a
separate question, because "am I folding?" and "are these two names the same?"
stop being the same question the moment a third mode exists.

Write the record as `doc/claude/decisions/0001-distinguish.md` (new directory;
say so in the commit) or as a new section of the spec — pick one, say why, do
not do both. It must answer, each with an argument and not an assertion:

1. Does `distinguish` ship as a third `casemode` value, or behind a separate
   flag? The enum already reserves the value, which is an argument but not the
   answer.
2. What is the diagnostic when a deck under `distinguish` references a name
   that differs only in case from one that exists? Silence is the current
   behaviour at three of the four gates and is what makes them gates.
3. `ng_ideq()`'s third arm, and which existing `inp_case_folding()` call sites
   must become `inp_case_mode() == NG_CASE_FOLD` instead because they are asking
   about the *fold*, not about identity.
4. The OSDI question the spec calls out as "decision required, single answer,
   not three" (`src/osdi/osdiinit.c:74`).
5. What `preserve` guarantees that `distinguish` withdraws, stated as a
   migration note, since `preserve` is the mode that has shipped decks.

### 4. The `vectors.c` gate.

Anchors verified at HEAD:

```c
/* src/frontend/vectors.c:48  */ static void vec_rebuild_lookup_table(struct plot *pl)
/* :61  */    nghash_unique(pl->pl_lookup_table, FALSE);     /* multiple entries allowed */
/* :71  */    if (ds_cat_str_case(&dbuf, d->v_name, ds_case_lower) != DS_E_OK)   /* key folded */
/* :77  */    nghash_insert(lookup_p, lower_name, d);        /* add lower-cased name */
/* :152 */ static struct dvec *findvec(char *word, struct plot *pl)
/* :184 */    if (ds_cat_str_case(&dbuf, word, ds_case_lower) != DS_E_OK)        /* query folded */
/* :190 */    struct dvec *d = find_permanent_vector_by_name(pl_lookup_table, lower_name);
/* :196-206 */ the "v(<lowercased word>)" retry, same fold
/* :326-353 */ static struct dvec *find_permanent_vector_by_name(...)
```

Three things are folded, not one: the **key** at `:71`, the **query** at `:184`,
and the **`v()` retry's query** at `:198`. `nghash_unique(..., FALSE)` at `:61`
then permits two entries under one key, so under `distinguish` two case-variant
nets collide and `findvec()` returns whichever hashed first — a silent wrong
answer, which is exactly what the spec says at its point 3.

Note before designing: the fold at `:71`/`:184` is **load-bearing for `fold` and
`preserve` both**, and `doc/claude/suggestions/case-sensitive-identifiers-plan.md`
§2.5 (commit `47c52c7dd`) deliberately removed the `strtolower` from
`vec_basename` so the *stored* name carries the deck's spelling while the
*lookup* stays folded. Under `preserve` that pairing is the feature. Whatever
you do here must leave `preserve` byte-identical; the `case/` suite is 105
decks and every one of them is the assertion.

Also relevant, and read before you touch the hash: §2.3 of the plan —
`src/misc/hash.c:549` copies the key **only** when
`hash_func == NGHASH_DEF_HASH(NGHASH_FUNC_STR)`, and the frees at `:108`,
`:182`, `:393` are gated the same way. Installing a custom hash silently flips
the table from owning to borrowing its keys. "Fold the key, never swap the
comparator" is the rule the plan sets; if you break it, say why in the commit.

RED deck, from the plan: `tests/regression/case/node-case-split.cir` — two nets
differing only by case, both interpretations solvable. RED today prints one
value for both `v(Out)` and `v(OUT)`; GREEN prints two different voltages. §2.5
has landed, so the labels are safe to assert on as well as the values.

**That deck cannot live in `tests/regression/case/`** as it stands: everything
there runs under `-D casemode=preserve`, where the two nets are *correctly* one
net. It needs its own directory with its own `TESTS_ENVIRONMENT`, following the
`tests/regression/case/Makefile.am` pattern (which puts the flag on the unquoted
`$1`), registered in `configure.ac`'s `AC_CONFIG_FILES` and in
`tests/Makefile.am`. Do that first and prove the harness runs an empty-ish deck
before writing the real one.

## Conventions

- `elem_letter()` (`src/frontend/inpcom.c:930`) is for a card's **leading device
  letter** only, the rule `doc/codex/issues/0018` set. `tolower_c()` / `cieq()`
  / `cistrstr()` elsewhere as the site demands.
- `ng_ideq()` (`inpcom.c:1055`) is for identifier identity in the name spaces
  that are never interned — `.model`, `.subckt`, `.global`.
- Match the style of the file being edited: four-space indent, same-line opening
  brace, no reflowing, no whitespace churn.
- One commit per mechanism. Expected: 0026, the doc corrections, the decision
  record, the new test directory, the `vectors.c` change.
- Commit messages: short imperative summary, then problem, mechanism, and the
  RED evidence. State whether the change is fold-mode visible, with the
  argument, and state what it does to `preserve`.

## Method — RED-first per AGENTS.md, non-negotiable

1. write or extend the test, 2. run it and show the failure is for the expected
reason, 3. smallest production change, 4. re-run the targeted test then the
directory's `make check`, 5. full `make check` before the commit. Never weaken
an assertion or regenerate a `.out` to hide a failure.

Twin-pair pattern: an upper/lower pair with byte-identical `.out` files, so the
assertion is "the spelling cannot change the number". Change **only the bytes
under test** between the twins. `node-case-split.cir` is the exception — it is a
single deck whose whole point is that the spelling *does* change the number.

## Test harness hazards — most obvious tests are invalid without these

- `tests/bin/check.sh:20` filters BOTH sides through a large `egrep -v` that
  removes `Error`, `Warning`, timings, memory, index columns, any line containing
  `---`, and the words `nalysis`, `oise`, `urrent`, `est`, `time`, `trans`,
  `Data`, `Total`, `ole`. **Run every new deck against an empty `.out` first; if
  it passes, its evidence is not a number and the deck is worthless.**
- `check.sh` diffs with `diff -B -w`. **Whitespace differences are invisible**,
  so no deck in this harness can assert on spacing.
- `check.sh` captures **stdout only**. `Error:` diagnostics go to stderr, so an
  aborting deck asserts a *missing* number.
- **`@D1[id]` and friends resolve straight from the circuit, not from the saved
  plot.** A deck that prints one is not evidence that a vector was saved. Learned
  the hard way while writing `save-alli-case.cir`; that deck works because
  `save alli` is its *only* save, so a dropped descriptor means the analysis
  never runs and the value reads back as 0.
- **The first line of a deck is its title**, and it is consumed. Start every deck
  with a `*` comment and put `.OPTIONS noacct` on line 2.
- Use numeric node names for anything printed, except where the deck is
  specifically about node-name case.
- Do not print with `vm(...)`/`vp(...)`/`vdb(...)` — `doc/codex/issues/0020`
  gap 2 — and do not reach a plot with `setplot <name>`.
- A `.cir` absent from its directory's `TESTS` is silently never run.
- `tests/.gitignore` ignores `*.out`, so every reference needs `git add -f`.
- CIDER is off in `build-ver_50`. Do not claim coverage of a `#ifdef CIDER` path.

## Build

```
make -C build-ver_50 -j14
make -C build-ver_50/tests/regression/case check TESTS=<name>.cir
make -C build-ver_50 check                       # full suite, currently 214
```

Adding a test file to an existing `Makefile.am` needs only
`make -C build-ver_50/tests/regression/<dir> Makefile` — touch the `Makefile.am`
first or make will call it up to date. A **new** test directory needs
`./autogen.sh` and a re-`configure`.

Hazard: never pipe `make` through `head`. SIGPIPE kills make mid-build and
leaves a stale binary. Send build output to a file and grep the file.

Hazard: do not rebuild while the differential sweep is running.

Hazard: `git stash` + `make` to measure a pre-fix number leaves the binary built
from the stashed tree. Rebuild after `git stash pop` before you trust anything
else you measure.

Hazard: a shell `cd` persists between commands in this harness. Use absolute
paths or `cd` back to the repo root.

Hazard: do not write a background wait loop whose condition greps `ps` or
`pgrep` for a command string — it matches its own command line and never exits.
Wait on a sentinel file instead.

## Deliberately out of scope — enumerate, do not fix

- Phase 3 gates 1, 2 and 4: `src/spicelib/parser/inpptree.c:1256` `mkvnode`
  create-on-miss, `src/xspice/evt/evtcheck_nodes.c:720` auto-bridge `strcmp`,
  `src/xspice/evt/evttermi.c:304`. They are unblocked *by* this session, not
  taken in it.
- `doc/codex/issues/0016` class (b), `gens.c:289` and `:295`.
- `doc/codex/issues/0024` sites 1 and 2 (`var()` → `cp_getvar`, `vec()`'s plot
  qualifier). Site 3, `spicenum.c:367`, was examined and declined: it is **not**
  a one-line fold — `cp_getvar()` compares the stored name with `strcmp` inside
  `src/frontend/variable.c`, so the fold belongs there — and `ft_batchmode`
  calls `controlled_exit()` before the value is read, so it is unreachable under
  `--batch` and untestable in `check.sh`. Re-decide it only with that in hand.
- `doc/codex/issues/0025` — a parameter named after a built-in evaluates to 0
  with no diagnostic. Changes the **default** mode's numbers; needs its own
  design decision.
- `doc/codex/issues/0020` gap 2, `PRINT VM(2)`, and `doc/codex/issues/0021`.
- The build-enforced lint that stops a new `strcmp` against a lower-case literal
  from re-entering the tree. Still wanted, still needs no decision, still a
  session of its own.

Anything new you find that you do not fix goes in
`doc/codex/issues/NNNN-slug.md` with the Status / Summary / Impact / Root Cause /
Acceptance Criteria / Resolution structure. Next free number is **0027**.

## Stop and report

What changed per file, RED evidence per behavioural change, the before/after
sweep verdict counts compared per deck, what the decision record decided and
what it explicitly left open, which Phase 3 gates are now unblocked, whether
each change is fold-mode visible and the argument either way, what `preserve`
does before and after (it must be byte-identical, and `tests/regression/case/`
is the proof), anything deliberately left undone, and the full `make check`
result.

---

## Why this step, and what follows it

Because nothing else can proceed. `preserve` shipped and is closed out: as of
`b60e40c2e` there is exactly one open defect that does not need this decision
(`doc/codex/issues/0026`, one line), and everything else on the list —
`0016` class (b), `0020` gap 2, `0024` sites 1 and 2, and the four Phase 3
gates — is waiting on the same answer.

Among the four gates the `vectors.c` one is first for a mechanical reason, not a
preference: it is the only one whose failure mode is *unobservability*. The
other three produce wrong numbers, which a deck can catch — but only if the deck
can name two case-variant nets separately, and `findvec()` is what decides that.
Take it in the wrong order and the RED decks for the other three cannot be
written at all.

After this: gate 1 (`inpptree.c` `mkvnode`) is the next most valuable, because
its failure manufactures a floating node and runs to completion, which is the
worst failure mode in the list. Gates 2 and 4 are XSPICE and want the
`xspice-event-case.cir` harness with its own `spinit`, following
`tests/xspice/digital/spinit.in`, which is a session's work before any
production change.

The two things that still need no code and no decision, and could fill a short
session at any point, are the remaining plan prerequisites (§2 above, minus the
`sharedspice.h` note which is done) and the `strcmp` lint.
