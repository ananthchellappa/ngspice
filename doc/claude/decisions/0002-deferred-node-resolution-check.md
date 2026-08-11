# Decision 0002 — the deferred node-resolution check, Phase 3 gate 1

## Status

Accepted, 2026-08-10, branch `ver_50`. Implements decision 2 of
`doc/claude/decisions/0001-distinguish.md` at
`src/spicelib/parser/inpptree.c:1249`; it does not re-decide it. Closes Phase 3
gate 1. Gates 2 and 4 are untouched.

## Context

`mkvnode()` resolves a `V()` reference inside a B-source expression with
`INPtermInsert()`, which **creates the node on a miss**. Under
`casemode=distinguish` a reference whose spelling differs only in case from a
real net is a miss, so the parser mints a node no card defines.

Measured at `191e45c3a`, the defect has two shapes and the spec described only
the first:

```spice
* .OPTIONS noacct rshunt=1e9
V1 In 0 dc 1.5
R1 In 0 1k
B1 out 0 V={V(in)*2}
R2 out 0 1k
```

- **With a DC path to the manufactured node** (the `rshunt` above, or any card
  that reaches it) the run completes: `preserve` prints `v(out) =
  3.000000e+00`, `distinguish` prints `0.000000e+00`, with no diagnostic.
- **Without one** the node is genuinely isolated — an expression reference
  contributes no conductance to the referenced node's own row — the matrix is
  singular, and the run aborts with

  ```
  Warning: singular matrix:  check node in
  ...
  Error: Transient op failed, timestep too small
  op simulation(s) aborted
  ```

  naming a node the deck never wrote. The `Last Node Voltages` dump lists
  `In`, `out` **and** `in`, which is the manufactured node made visible.

The spec is corrected accordingly; both shapes are one defect and take one fix.

## The constraint that shapes everything: create-on-miss is load-bearing

```spice
* forward reference: B source names a node defined by a later card
.OPTIONS noacct
B1 out 0 V={V(mid)*2}
R2 out 0 1k
V1 mid 0 dc 1.5
R1 mid 0 1k
```

`v(out) = 3.000000e+00` in `fold`, `preserve` and `distinguish` alike. The B
card is parsed before the cards that define `mid`; the reference works
*because* `INPtermInsert()` creates the node and the later cards then find it.

This was tested, not assumed. Replacing the call with `INPtermSearch()`
(`src/spicelib/parser/inpsymt.c:385`), which is the obvious non-creating probe
and already returns `E_EXISTS` or `0`, and building it:

```
EXPERIMENT: no node named 'mid'
Segmentation fault (core dumped)
```

on the **all-lower-case** twin under `preserve` — that is, the naive fix breaks
forward references in every mode, not only in the mode being added, and
`values[i].nValue` becomes `NULL` for a node the evaluator later dereferences.
All four decks added with this change fail under it.

A diagnostic therefore cannot be raised at the reference, because at the
reference a miss is indistinguishable from a forward reference. It has to be
**deferred to the end of the parse**, where "a node that a reference created
and that no card ever defined" is a decidable question.

---

## Decision 1 — the bit lives on `struct INPnTab`, not on `CKTnode`

**Decided: `bool t_unclaimed` on `struct INPnTab`
(`src/include/ngspice/inpdefs.h`).**

`CKTnode` (`src/include/ngspice/cktdefs.h`) was the other candidate and is
rejected on four counts, in order of weight:

- **Lifetime.** The bit answers a question that only exists while the netlist
  is being read. `INPtables` is allocated in `if_inpdeck()` and destroyed by
  `INPtabEnd()`; `CKTnode` outlives the parse, the analysis and — in the shared
  build — the `ngSpice_Reset()` that is supposed to clear everything. `CLAUDE.md`
  calls out simulator state that must survive repeated `ngSpice_Reset` as a
  live bug class (`c5cd68015`, `5ad395d5e`). A parse-scoped bit on a
  simulation-scoped struct is exactly how that class of bug is created.
- **The check needs the table, not the node list.** `hash()`
  (`inpsymt.c:364`) folds the bucket key whenever the reader is not folding, so
  every name differing only in case is *already* in one bucket. The sibling
  scan is a walk of the chain the entry is on. Starting from `CKTnode` the
  check would have to re-enter the symbol table anyway.
- **Blast radius.** `struct INPnTab` is touched by exactly one file,
  `src/spicelib/parser/inpsymt.c`; `t_ent` and `t_node` appear nowhere else in
  `src/`. `CKTnode` is in a public header included by every device directory
  and by `libngspice` consumers.
- **Initialisation.** Not every `CKTnode` comes through the term table —
  internal device nodes go through `CKTmkVolt` directly — so a `CKTnode` field
  would need a correct default on paths that have nothing to do with parsing.
  Every `INPnTab` is created in one of four adjacent functions in one file.

Cost of the choice: the bit is unavailable to anything after `INPtabEnd()`. No
current or planned consumer needs it — the check runs before then — but a
future "which nodes did the deck actually define" query from the frontend would
have to be answered some other way.

## Decision 2 — the sweep runs in every mode; the diagnostic fires only under `distinguish`

**Decided: maintain the bit unconditionally, and gate the report on
`inp_case_mode() == NG_CASE_DISTINGUISH`.**

Keeping the bookkeeping mode-independent means the `fold` path through
`term_insert()` is the same path, one store different, rather than a second
path. Gating the *report* is the substantive half:

- Under `preserve` the condition is **unreachable**. `ent_eq()` is
  case-insensitive there, so two entries differing only in case cannot both be
  in the table, and the sibling scan can never find anything. The guard is
  belt-and-braces in that mode, not a policy.
- Under `fold` it is reachable, and that is the argument that had to be had.
  The reader lowercases every card, so the only table entries carrying upper
  case are the ones **ngspice constructs for itself** — `q1#collCX`
  (`src/spicelib/devices/bjt/bjtsetup.c:433`) is the canonical one. A deck that
  writes `V(q1#collcx)` would get a true positive. But every true positive in
  `fold` is about a name the user did not choose, and decision 3 of
  `0001-distinguish.md` states the rule for exactly this case: distinguish is
  exact about names the deck chose and case-insensitive about names ngspice
  constructs. Firing in the default mode, on the one class of name the user
  cannot control, is the wrong trade — and it is a behaviour change for every
  existing deck, which would need its own evidence.

**What `distinguish`-only costs, stated plainly.** A reference that misses with
**no** case variant present — `V(mdi)` against `mid`, an ordinary typo — is
still silent, in all three modes, and still manufactures a node. That is the
generic *undefined-node* diagnostic, and it is a different change with a
different risk profile: a node mentioned once is a legal floating node in
SPICE, so it would fire on correct decks and needs the full suite and the
differential sweep behind it. It is filed as `doc/codex/issues/0028` rather
than folded in here. This was the only chance in the feature to catch a plain
typo cheaply, and it is deliberately not taken in this commit.

Measured consequence: the differential sweep runs stock, `preserve` and
`preserve`-UPPER only — it never runs `distinguish` — so this change cannot
move a sweep verdict by construction, and did not.

## Decision 3 — a warning, not an error

**Decided: `Warning:` on `stderr`, run continues.**

Decision 2 of `0001-distinguish.md` rejects abort-on-miss for the general case.
That argument was re-tested against this specific site rather than inherited,
because this site is worse than the vector table: it changes what is
*simulated*, not what is *displayed*. It survives, for three reasons:

- The check is deferred, so by the time it fires the circuit is built and the
  matrix is set up. Aborting there is a much larger hammer than refusing at the
  reference would have been, and it would turn every `distinguish` deck that
  today runs with a wrong number into a deck that does not run at all — a
  strictly bigger behaviour change than making the failure audible.
- The immediate neighbours already warn and continue. `.nodeset` and `.ic` on a
  non-existent node (`inppas3.c:94`, `:143`) print `Warning : Nodeset on
  non-existent node - %s, ignored` and carry on. Aborting here would make the
  parser inconsistent with itself one file away.
- The user-facing goal is "do not print a wrong number silently", and a warning
  achieves it. Shape (b) shows why it matters that the warning is *first*: it
  now precedes `singular matrix: check node in`, so the misleading message
  arrives already explained.

Rejected: promoting it under `-strict` or `ft_stricterror`. That is a fourth
policy axis for one diagnostic, and decision 1 of `0001-distinguish.md` already
says a fourth axis needs a fourth mode value, not a second variable.

Rejected: emitting it on `stdout` as `Notice:`, which `check.sh`'s filter would
let through and a deck could then assert. It contradicts `0001`'s decision 2,
which puts these diagnostics on `stderr`, and ngspice's `stdout` is where the
answer goes. Testability is not worth putting a diagnostic in the data stream.

Text, matching decision 2's form for the vector table so the two read as one
rule:

```
Warning: no node named 'in'; 'In' differs only in case (casemode=distinguish)
```

**Guarded since 2026-08-11**, by
`tests/regression/casedist/bsource-node-case-report.cir`: the warning, and the
three silences this decision owes — a definition, a forward reference claimed
by a later card with a case variant also defined, and a miss with no variant
at all. The deck sources each circuit from inside a `.control` block so that
`>&` can reach the parse, which needed this diagnostic to write to `cp_err`
rather than to `stderr`. `doc/codex/issues/0036`,
`doc/claude/decisions/0006-diagnostic-deck-coverage.md`.

## Decision 4 — `INPtermSearch()` is not put on any new path

**Decided: `mkvnode()` keeps an inserting call; a new non-mutating sibling scan
is written instead.**

The question was whether `INPtermSearch()`'s token substitution is safe on the
paths this change adds. It is not put on any, and that is not incidental:

- On a **hit** it does `FREE(*token); *token = t->t_ent;`. `mkvnode()`'s caller
  hands it a fresh `copy_substring()` and `INPtermInsert()` has the same
  consuming contract, so a hit would be fine.
- On a **miss** it returns `0` having done neither, leaving `mkvnode()` with an
  untouched token it now owns and — the real hazard — an untouched `CKTnode
  *temp`. `temp` is uninitialised in the tree as written, so the literal
  one-line swap reads a garbage pointer into `values[i].nValue`; the
  experiment above initialised it to `NULL` first, to make the failure
  deterministic rather than undefined, and still segfaults.

So `mkvnode()` calls `INPtermInsertRef()`, which is `INPtermInsert()` with the
created entry marked. The end-of-parse scan (`INPtermCaseCheck()`) walks the
bucket chain directly and **borrows** `t_ent` for the message: it frees
nothing, substitutes nothing, and mutates nothing.

## What this decision does not decide

1. **Phase 3 gates 2 and 4** — `src/xspice/evt/evtcheck_nodes.c:720` and
   `src/xspice/evt/evttermi.c:304`. The auto-bridge has the same shape as this
   gate, a resolution that silently does nothing on a miss, and should reuse
   the collect-then-diagnose structure; it needs an XSPICE event harness with
   its own `spinit` first.
2. **The generic undefined-node diagnostic**, `doc/codex/issues/0028`.
3. **The other create-on-miss reference sites**, also `0028`:
   `inp2dot.c:53`/`:59` (`.NOISE` output), `:371`/`:376` (`.SENS`),
   `:495`/`:501` (`.TF`), `:677`/`:775` (`.PSS` oscnode) and `inpgval.c:90` all
   resolve a node name with `INPtermInsert()`. They are references, so they
   should be `INPtermInsertRef()` too; each also *clears* the bit today, which
   makes them a false-negative source — a `.SENS v(in)` on the manufactured
   node suppresses this warning. Not fixed here because each needs its own deck
   and none is on the gate list.
4. **`I()` references**, `mkinode()` (`inpptree.c:1279`). It interns an
   instance name into `INPsymtab` with `INPinsert()`, a different table with no
   node behind it, and instance resolution fails loudly elsewhere. Out of the
   gate's scope.
