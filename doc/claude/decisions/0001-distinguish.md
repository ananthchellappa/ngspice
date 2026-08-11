# Decision 0001 — `casemode=distinguish`

## Status

Accepted, 2026-08-10, branch `ver_50`. Supersedes the "Open decisions" entries
in `doc/claude/specs/case-sensitive-identifiers.md` that this file names.

## Why this is a record and not a spec section

The spec describes the feature; this describes a choice made once, with the
losing options kept so nobody re-argues them. It also has to enumerate every
identity call site in the tree, which is a table that would swamp a spec whose
job is to be read end to end. `doc/codex/issues/NNNN-slug.md` already sets the
numbered-artifact precedent, so `doc/claude/decisions/NNNN-slug.md` follows it.
`doc/claude/decisions/` is new with this file. The spec's Open decisions section
now points here rather than repeating any of it.

## Context

`preserve` has shipped. It changes what ngspice *shows*; `distinguish` changes
what ngspice *simulates*. The whole of it is one function:

```c
/* src/frontend/inpcom.c:1055, before this decision */
bool ng_ideq(const char *a, const char *b)
{
    return inp_case_folding() ? (strcmp(a, b) == 0) : (cieq(a, b) != 0);
}
```

Two arms, because there were two modes. The moment a third mode exists, "am I
folding?" and "are these two names the same?" stop being the same question, and
every site that asked the first while meaning the second has to be re-read.
There are twenty-eight such sites; they are enumerated in decision 3.

---

## Decision 1 — `distinguish` is the third `casemode` value, not a separate flag

**Decided: third value of the existing `casemode` variable.**

The argument for a separate flag is real and is not "the enum already reserves
the value". It is that `preserve` and `distinguish` are two independent axes —
spelling and identity — and a deck could plausibly want identity without
preservation. The spec says as much at its Modes section.

It is rejected because that combination does not exist. Distinguishing `R1`
from `r1` while *displaying* both as `r1` is not a mode anyone can use: the two
nets would be numerically distinct and typographically identical, in the
rawfile, in `print`, and in every diagnostic. So of the 2 x 2 the flag would
create, one cell is incoherent, one is `fold`, one is `preserve` and one is
`distinguish`. An ordered tri-state expresses exactly the three that exist.

Three further arguments, in order of weight:

- The switch is read once per `inp_readall()` and is a control variable, not
  argument parsing. A second variable means a second read point, a second
  default, a second thing to get right across `.include`, `ngSpice_Circ()` and
  `ngSpice_Reset()`, and a second thing for `src/spinit.in` to document.
- `set_case_mode()` (`src/frontend/inpcom.c:1066`) already rejects the string
  with a named warning. Users who tried it will get the feature by changing
  nothing.
- The enum value `NG_CASE_DISTINGUISH` is already in
  `src/include/ngspice/fteext.h:212` and already ordered after `NG_CASE_PRESERVE`.
  That is weak evidence — an enum can be deleted — but it does mean the
  ordered-mode reading is the one every existing guard was written against.

**Consequence, and it is the cost of this choice**: `distinguish` is not
independently gateable. A deck cannot ask for exact identity and folded
spelling, and there is no place to hang a "strict identity, everything else as
before" opt-in. If that is ever wanted it will need a fourth value, not a
second variable.

## Decision 2 — the diagnostic for a case near-miss

**Decided: silence when a name is being *defined*, a warning when a name is
being *resolved*, the resolution fails, and a name differing only in case
exists.**

The question matters because silence is the current behaviour at three of the
four Phase 3 gates and is precisely what makes them gates: `mkvnode`
(`src/spicelib/parser/inpptree.c:1249`) mints a floating node on a miss, the
XSPICE auto-bridge (`src/xspice/evt/evtcheck_nodes.c:720`) omits a bridge on a
miss, and the frontend vector table returned whichever of two aliased vectors
hashed first.

Rejected: **warn on definition**, i.e. whenever a second name differing only in
case from an existing one is created. It fires on the legitimate deck. Under
`distinguish`, `Out` and `OUT` as two real nets is not a mistake, it is the
feature; a warning there makes the feature unusable and trains users to ignore
the warning that matters.

Rejected: **error and abort on a resolution miss**. An unresolved node is not
an error in SPICE — a node mentioned once is a legal floating node, and decks
rely on that. Turning every miss into an abort changes the language, not the
case policy, and would break decks that have nothing to do with case.

Accepted, because it is the only rule that separates the two: the near-miss is
diagnosable exactly where a *lookup* fails and a case-variant of the looked-up
name is present. That is a condition that cannot arise by accident and cannot
arise at all under `fold` or `preserve`, where the lookup would have succeeded.
Its text names both spellings, because the deck author's problem is almost
always that they cannot see the difference:

```
Warning: no vector named 'OUT'; 'Out' differs only in case (casemode=distinguish)
```

Where it applies, and its state:

| Site | Rule | State |
| --- | --- | --- |
| `findvec()`, `src/frontend/vectors.c:152` | resolution | **implemented** with this decision |
| `mkvnode`, `src/spicelib/parser/inpptree.c:1249` | resolution — it creates on miss, so the miss was invisible | Phase 3 gate 1, **closed** by `0002-deferred-node-resolution-check.md`. It still creates, because a forward reference depends on it; the miss is reported after the parse instead of at the reference |
| auto-bridge, `src/xspice/evt/evtcheck_nodes.c:720` | resolution | Phase 3 gate 2, **closed**: the comparator is `ng_ideq()`. The `distinguish` near-miss warning is **implemented** with `doc/codex/issues/0030`, in `report_bridge_case_miss()`, once an event node's scan of `CKTnodes` has finished without an exact match |
| `src/xspice/evt/evttermi.c:304` | resolution | Phase 3 gate 4, **closed**, same comparator. The near-miss warning is **implemented** with `doc/codex/issues/0030`, deferred to `EVTnode_case_check()` and narrowed there from the issue's wording to an event node that nothing drives; see `doc/claude/decisions/0003-event-node-near-miss.md` |
| `vec_remove()` from `com_unlet()`, `src/frontend/vectors.c:613` | resolution | **implemented** with `doc/codex/issues/0027`, behind `vec_remove()`'s `report_case_miss`, reusing `vec_warn_case_near_miss()` so the wording is `findvec()`'s. See `doc/claude/decisions/0004-unlet-vector-identity.md` |
| `vec_remove()` from `com_compose()` and `com_cross()` | definition — each passes the name it is about to allocate | silent, deliberately, via the same flag |
| `vec_get()` on `com_let()`'s left-hand side, `src/frontend/com_let.c:101` | definition | **reports, and should not.** It reaches `findvec()`, so `let TIME = time * 2` warns and then correctly succeeds. This is the rejected option arriving by accident; `doc/codex/issues/0034` |
| `INPtermInsert()` from a device card | definition | silent, deliberately |
| `.model` / `.subckt` / `.global` declaration | definition | silent, deliberately |

The warning goes to `stderr`. That is not a preference: `tests/bin/check.sh:29`
captures stdout only and its `egrep -v` filter drops any line containing
`Warning` from both sides anyway.

**Corrected 2026-08-11:** this paragraph used to continue "so no deck in this
harness can assert on it either way", and that was wrong. A deck *can* assert
on a diagnostic, and `doc/codex/issues/0027`'s
`tests/regression/casedist/vector-unlet-report.cir` does. The control language
redirects `cp_err` as well as `cp_out` with `>&`
(`src/frontend/streams.c:153`), so `unlet oUt >& capture.txt` puts the warning
in a file; `fopen`/`fread`/`strstr` then reduce it to one upper-case token on
stdout, which the filter keeps because it contains neither `Warning` nor
`Error`. `tests/regression/pipe/shell-keyword-case.cmd:79` was already using
that read-it-back shape for output it had to take off disk. Preseeding the
variable makes the *silent* cases assertable too, which matters more than the
warning: silence on a definition is the half of this decision a diagnostic gets
wrong.

So a diagnostic under this decision should be verified by hand, quoted in the
commit that adds it, **and** guarded by a deck. The three diagnostics that
shipped before this was noticed — `0002`'s parser check and `0003`'s two
XSPICE event checks — are quoted but unguarded; that is
`doc/codex/issues/0036`.

## Decision 3 — `ng_ideq()`'s third arm, and the twenty-eight call sites

**Decided**:

```c
bool ng_ideq(const char *a, const char *b)
{
    /* preserve is the only mode in which two spellings are one identifier */
    return inp_case_mode() == NG_CASE_PRESERVE ? (cieq(a, b) != 0)
                                               : (strcmp(a, b) == 0);
}
```

The shape matters more than the arm. Written as a switch on `inp_case_mode()`
with `NG_CASE_PRESERVE` as the *only* case-insensitive arm, `fold` and
`distinguish` share the exact-comparison branch and the default mode keeps
comparing exactly the bytes it always did. Written the other way — fold ?
strcmp : (distinguish ? strcmp : cieq) — it is the same function and a worse
statement of the rule.

Every site below was re-read at commit `ff1b9955a`. The classification is the
whole point: a site asking **"is the reader folding?"** is already correct for
all three modes, because `preserve` and `distinguish` both leave the card's
spelling alone. A site asking **"are these two names the same?"** while spelled
`inp_case_folding()` is wrong under `distinguish` and must become
`inp_case_mode() == NG_CASE_PRESERVE`.

### Class A — identity. Must be exact under `distinguish`.

| Site | Today | Under `distinguish` before this decision |
| --- | --- | --- |
| `src/frontend/inpcom.c:1055` `ng_ideq()` | `fold ? strcmp : cieq` | `cieq` — wrong |
| `src/spicelib/parser/inpsymt.c:31` `ent_eq()` — node and token interning | same shape | `cieq` — wrong; this is the one that decides whether two nets exist at all |
| `src/frontend/inpcom.c:4334` `user_ident_eq()` — `.param` and `.func` names | same shape | `cieq` — wrong |
| `src/frontend/inpcom.c:4787` — `.func` formal, fixed length | `fold ? strncmp : cieqn` | `cieqn` — wrong |
| `src/frontend/subckt.c:1665` `eq_substr_id()` — subcircuit names | `fold ? eq_substr : ci` | ci — wrong |
| `src/frontend/subckt.c:1853` `wl_find_id()` — `.model` name translation | `fold ? wl_find : ci` | ci — wrong |
| `src/frontend/subckt.c:154` `glo_key()` — `.global` hash key | folds the key when not folding | folded key aliases two globals — wrong |
| `src/spicelib/parser/inpmkmod.c:35` `INPmodKey()` — model table hash key | same shape | folded key aliases two models — wrong |
| `src/frontend/numparam/xpressn.c:386` `symbol_key()` — numparam symbol table key | same shape | folded key aliases two `.param`s — wrong |
| `src/frontend/subckt.c:2162`, `src/spicelib/parser/inpgmod.c:306`, `src/frontend/inpcom.c:9677`, `:9797` | `model_name_match(..., !inp_case_folding())` | case-insensitive — wrong |
| `src/frontend/numparam/xpressn.c:1602` `search_isolated_identifier()` — the identifier is a `.subckt` formal | `fold ? strstr : cistrstr` | `cistrstr` — wrong |
| `src/frontend/inpcom.c:4718` — doubles each `.func` formal's initial into an accept set | doubles when not folding | doubles — wrong, though only a filter widening |
| `src/frontend/inpcom.c:6136` `ya_search_identifier()` — user-chosen names, `doc/codex/issues/0015` | `ci = !inp_case_folding()` | ci — wrong |

Every Class A site is now the single token `inp_case_exact_ids()`
(`src/frontend/inpcom.c:1060`) in place of `inp_case_folding()`, on an
otherwise unchanged line. That is deliberate: it makes the classification
visible in the source — a line that says `inp_case_folding()` is asking about
the fold, a line that says `inp_case_exact_ids()` is asking about identity —
and it is the single-token edit shape the plan's fallback strategy asks for,
because a token swap on an untouched line rarely conflicts on rebase.

`inp_case_exact_ids()` returns `mode != NG_CASE_PRESERVE`. Both `fold` and
`distinguish` therefore take the byte-exact branch, for opposite reasons: fold
because the reader lowercased both sides, distinguish because two spellings
are two names. The default mode compares exactly the bytes it always did, and
every Class A edit is provably a no-op under `fold` and under `preserve`.

Two further Class A sites were added with Phase 3 gates 2 and 4. They call
`ng_ideq()` itself rather than swapping a single token, because unlike the
rows above they were **bare `strcmp`** and had no `inp_case_folding()` to
replace:

| Site | Today | Before |
| --- | --- | --- |
| `src/xspice/evt/evttermi.c:304` `EVTnode_insert()` — the event-node find/create, i.e. the interning site | `ng_ideq(node_name, node->name)` | `strcmp` — wrong under **preserve**: two spellings of one node became two event nodes and silently split a net |
| `src/xspice/evt/evtcheck_nodes.c:720` `Evtcheck_nodes()` — auto-bridge insertion | `ng_ideq(event_node->name, analog_node->name)` | `strcmp` — wrong under **preserve**: digital `A` and analog `a` are one mixed-type net there, and no bridge was inserted |

Both operands are names the deck wrote on a card, which is what makes them
Class A: the event side is an A card's port token (`src/xspice/mif/mif_inp2.c:943`
hands it to `EVTtermInsert` **without** interning it, so it keeps the card's
spelling), and the analog side is the interned first spelling of a device
card's node. No ngspice-constructed name can reach either — `Evtcheck_nodes`
runs at `src/frontend/spiceif.c:185`, before `CKTsetup`, so no `q1#collCX` is
on `CKTnodes` yet.

These two are the exception to "every Class A edit is a no-op under
`preserve`": that claim holds for the rows *above*, which were already
`ng_ideq`-shaped. These two were `strcmp`, so `preserve` is the only mode they
move, and moving it is the point — they are `preserve` defects filed under an
unshipped mode. `fold` and `distinguish` are byte identical by the reduction
of `ng_ideq()`, with no argument about what the reader lowercased needed.

The generated bridge card keeps `analog_node->name` on both port lists
(`evtcheck_nodes.c:777`; `flush_card()` at `:254` feeds one buffer into both
`%s` slots, so a per-side spelling was never available). That is correct once
both gates are closed: the card's analog slot re-resolves through `ent_eq()`
and its digital slot through `ng_ideq()`, both `cieq` under `preserve`, so
either spelling lands on the same pair of objects; under `fold` and
`distinguish` a match at `:720` implies the two spellings are byte identical.
It is **not** correct with only gate 2 closed — the digital slot then re-enters
an unfixed `strcmp` and mints a third event node — which is why the two gates
land in that order.

### Class C — the frontend vector lookup, which is neither

`src/frontend/vectors.c:61`, `:71`, `:184` — duplicates permitted, key and
query folded, so two case-variant nets aliased and `findvec()` returned
whichever hashed first. **Phase 3 gate 3, closed with this decision.**

It is listed apart because its predicate is
`inp_case_mode() == NG_CASE_DISTINGUISH`, not `inp_case_exact_ids()`, and the
difference is not cosmetic. This is a *lookup*, not an identity test between
two deck tokens. In `fold` mode the query still arrives with upper case in it
— from `ngGet_Vec_Info()`, from the interactive prompt, from a generated name
such as `q1#collCX` — and `findvec()` has always matched it without regard to
case. Giving this site the Class A predicate would make the default mode
exact and break every one of those callers. Only `distinguish` makes it exact.

`vec_remove()` (`:613`) is the same class and was missed by this decision,
because it predates the lookup table and walks `pl_dvecs` directly rather than
querying the table. It took the same `vec_name_eq()` predicate with
`doc/codex/issues/0027`; `doc/claude/decisions/0004-unlet-vector-identity.md`
decision 1 argues why it is Class C and not Class A, and the argument is this
section's verbatim — its query is a control-language word, which the reader
folds only when the word arrived through `inp_readall()`. The name matchers
*outside* `findvec()` that this decision also did not reach —
`is_scale_vec_of_current_plot()`, `findvec_ally()`, `vec_eq()` and its six
callers, `rawfile.c:611`, `diff.c:89` — are `doc/codex/issues/0032` and are
what `set_case_mode()`'s experimental warning now names.

The mechanism keeps the plan's rule intact. The fold at `:71` and `:184` and
the `nghash_unique(pl_lookup_table, FALSE)` at `:61` are all unchanged; what
is new is that the duplicate chain those three lines already produced is
filtered on the spelling the caller typed. Under `fold` and `preserve` every
candidate on a folded-key chain satisfies `cieq()` by construction, so the
filter is a tautology and the lookup is byte identical to the historical one.

One exception inside the exception, found by running the filter rather than
by reading it. `src/frontend/outitf.c:1164` stores a node name that begins
with a digit as `V(<name>)`, with that upper-case `V`, **in every mode**. A
strict `strcmp` therefore made `print v(1)` find nothing under `distinguish`
while working under both other modes, for a spelling no deck chose. The
wrapper is language syntax and not part of the identifier — `v` and `i` are
the voltage and current accessors, and compatibility contract point 2 keeps
language keywords case-insensitive in **all** modes — so the wrapper letter is
folded and only the name inside it is compared exactly
(`vec_wrapped_name_eq()`). This is the same distinction as Class A versus
Class B, applied inside a single string.

The general form of the hazard is worth stating, because it will recur at
gates 1, 2 and 4: **`distinguish` must be exact about names the deck chose and
must stay case-insensitive about names ngspice constructs.** `V(1)` is the
first instance. `q1#collCX` (`src/spicelib/devices/bjt/bjtsetup.c:433`) is the
next one, and it is not handled: its upper case is inside the constructed
part, so a user must type it as the simulator spells it. That is a documented
limitation and not a silent wrong answer — the lookup fails and decision 2's
warning fires.

#### The four XSPICE event-node lookups, added with gates 2 and 4

The event node list is read by four lookups that are **not** identity tests
between two deck tokens, and they were failing in two opposite directions —
two too strict, two too loose. All four now share one predicate,
`Evt_Node_Name_Eq()` (`src/xspice/evt/evtplot.c`, declared in
`src/include/ngspice/evtproto.h`), which is `findvec()`'s rule verbatim:
exact only under `inp_case_mode() == NG_CASE_DISTINGUISH`.

| Site | Before | Wrong under |
| --- | --- | --- |
| `src/xspice/evt/evtshared.c:253` `get_index()` — `ngGet_Evt_NodeInfo()` | `strcmp` | `fold` and `preserve`: the query is the host program's, and the analog half of the same API accepted any case |
| `src/xspice/evt/evtprint.c:341` `get_index()` — `eprint`, `esave`, `eprvcd` | `strcmp` | `fold` and `preserve`: the query is a word typed at the control language, which the reader never folded |
| `src/xspice/evt/evtplot.c:110` `Evt_Parse_Node()` — `print`/`plot` via `findvec()` | `cieq`, over a query force-lowercased at `:83` | `distinguish`: an event node answered to a spelling no card wrote |
| `src/xspice/evt/evtaccept.c:342` `EVTcancel_value_call()` | `cieq` | `distinguish`, and it must move with `evtplot.c:110` or a callback registered case-insensitively becomes uncancellable |

`Evt_Parse_Node()`'s `strtolower()` is narrowed rather than deleted: it now
folds only the `member` qualifier of `node(member)`. The member is a keyword
named by the user-defined node type — `state`, `strength` — and keywords are
case insensitive in **all** modes by compatibility contract point 2. This is
the same distinction as `vec_wrapped_name_eq()`'s `V(...)` wrapper above,
applied inside a single string, and it is load-bearing rather than cosmetic:
`EVTnew_value_call()` **stores** the member (`evtaccept.c:313`) and replays it
for the life of an `iplot`, so unfolding it would change a persisted string.

Two of these four are visible under `fold`, which the Class A rows are not.
Both are strict loosenings on the query side only — under `fold` the reader
lower cased every A card, so no two event nodes can differ only in case and no
query that resolves today can resolve differently. What changes is that a
query which failed now succeeds: `eprint OUTQ` at the prompt against a node
interned as `outq` answered `ERROR - Node OUTQ is not an event node.` and now
prints the table, which is what `print v(...)` has always done on the analog
side.

Sites that reach identity only through `ng_ideq()` and therefore need no edit of
their own: `subckt.c:628`, `:1634`, `:1860`, `inpcom.c:3318`, `:3396`, `:3892`,
`:9627`, `inplkmod.c:25`, `mifgetmod.c:134`.

`src/frontend/inpcom.c:3329` `nlist_model_find()` is inside `#if 0` and is left
exactly as it is; it is listed so that whoever re-enables it reads this table.

### Class B — the fold. Correct as written, in all three modes.

| Site | Question it asks |
| --- | --- |
| `src/frontend/inpcom.c:1800` | should the reader lowercase this card |
| `src/frontend/inpcom.c:3706` (`inp_casefix()`) | should this character be lowercased |
| `src/frontend/device.c:1420` | was the typed `alter` name lowercased upstream |
| `src/frontend/measure.c:236` | was the `.measure` analysis name lowercased upstream |
| `src/frontend/inpcom.c:6120` `search_identifier()`, `:6193` `search_plain_identifier()` | is the card folded, for a **language keyword** search |
| `src/spicelib/parser/inpsymt.c:292` `hash()` | which bucket — folding the bucket key is correct under `distinguish` too, because `ent_eq()` decides identity and a folded bucket only guarantees the two candidates meet |

Keywords are case-insensitive in **all** modes, per the spec's compatibility
contract point 2, so `!inp_case_folding()` is the right predicate for a keyword
search and stays.

### The rule this decision does not break

"Fold the key, never swap the comparator", `doc/claude/suggestions/case-sensitive-identifiers-plan.md`
section 2.3. `src/misc/hash.c:549` copies the key on insert only when
`hash_func == NGHASH_DEF_HASH(NGHASH_FUNC_STR)`, and the frees at `:108`,
`:182` and `:393` are gated identically, so installing a case-insensitive hash
function silently flips a table from owning its keys to borrowing them. Under
`distinguish` the three Class A key-folding sites stop folding rather than
start comparing differently, and `vectors.c` keeps its folded key and filters
the duplicate chain that `nghash_unique(pl->pl_lookup_table, FALSE)` has always
permitted. No comparator is swapped anywhere.

## Decision 4 — OSDI

`src/osdi/osdiinit.c:74` stores a `strtolower`'d copy of every Verilog-A
parameter name as the `IFparm` keyword, so two parameters differing only in
case collapse and the second shadows the first in `find_model_parameter`'s
linear scan (`src/spicelib/parser/inpgmod.c:52`).
`doc/claude/checklists/phase2-prerequisites.md` section 4 already decided
`fold` and `preserve`: keep the fold, match with `cieq`. This is the
`distinguish` half of the same question, and the spec asks for one answer.

**Decided: keep the fold and keep the case-insensitive matchers under
`distinguish` as well. Do not make OSDI parameter names case-sensitive.**

The tempting answer is the other one — Verilog-A is case-sensitive, so
`distinguish` is exactly the mode in which `Vth` and `VTH` should be two
parameters. It is rejected on three grounds:

- **Parameter names are not identifiers in this feature's sense.** The spec's
  scope is nets, instances, pins, `.model` and `.subckt` names — things the
  deck author names. An OSDI parameter name comes out of a third-party
  compiled `.osdi` artifact the deck author did not write and cannot change.
  Making the deck's spelling of someone else's parameter significant hands the
  user a failure they cannot fix.
- **It is untestable in this tree.** `tests/` has no OSDI regression directory
  and there is no `.osdi` artifact in the repository, so neither the fix nor
  its absence can be exercised by `make check`. Landing an untestable change
  that alters existing behaviour, in the mode that is already the riskiest, is
  the worst of the available trades.
- **The shadowing bug is orthogonal.** It exists in `fold` today, with no deck
  and no `casemode` involved. Fixing it means detecting the collision at
  `.osdi` load time and diagnosing it — one diagnostic, in one place, for all
  three modes. That is the right fix and it is not a `casemode` change.

**Left open, deliberately, and this is the one place where `distinguish` is
knowingly not case-sensitive**: an OSDI model whose Verilog-A source declares
both `Vth` and `VTH` still loses one of them, in every mode. Filed as the
follow-up in decision 6 below.

## Decision 5 — migration note: what `preserve` guarantees that `distinguish` withdraws

`preserve` is the mode that has shipped decks, so this is stated as a list of
withdrawn guarantees rather than as a list of new behaviour.

**Withdrawn.** Under `preserve` these are one thing; under `distinguish` they
are two:

- Two spellings of a node name. `R1 In Out 1k` and `.print v(OUT)` print
  nothing useful under `distinguish`; `Out` and `OUT` are two nets.
- Two spellings of an instance name, a `.model` name, a `.subckt` name, a
  `.global` name, a `.param` name and a `.func` formal.
- Two spellings of a name typed at the control language: `alter`, `show`,
  `@dev[param]`, `let`, `print`, `save`. The typed name must now match the
  stored spelling exactly.
- The shared-library contract. `ngGet_Vec_Info()` accepted a name in any case
  in both shipped modes; under `distinguish` it does not.
  `src/include/ngspice/sharedspice.h` is updated by the commit that lands the
  vector-table change, because a header that says "matched without regard to
  case" becomes false.

**Not withdrawn.** These are case-insensitive in all three modes and stay that
way — the spec's compatibility contract point 2 and its Out of scope section:

- Language keywords: `.TRAN`, `PULSE`, `PARAMS:`, `UIC`, `%VD`, `.model` type
  names, B-source function names.
- Device letters. `R1` and `r1` still dispatch to the same device type.
- Ground. `gnd`, `GND` and `Gnd` are node `0` in every mode.
- Numeric semantics. `1M` is still milli and scale suffixes stay
  case-insensitive.
- XSPICE code-model port names, which bind positionally.
- Instance and model *parameter* names — `w`, `L`, `vth0`. These are keywords
  naming a parameter, not identifiers; `doc/codex/issues/0026` is the most
  recent site where that distinction was the fix.

**The migration hazard that has no answer yet**, restated from the spec's Open
decisions 2 because `distinguish` is what makes it live: a `distinguish` top
level that `.include`s a fold-assuming PDK or library fails *silently*, because
an unresolved node is not an error — the parser mints a new floating node
(`src/spicelib/parser/inpptree.c:1249`, Phase 3 gate 1). Until that gate closes,
the only safe rule is that a `distinguish` deck and everything it includes must
be case-consistent. Decision 2's resolution warning is what will make this
audible; it is implemented at the vector table and not yet at the parser.

## Decision 6 — what this record does not decide

Enumerated so they are not read as oversights:

1. **Phase 3 gates 1, 2 and 4.** `mkvnode` create-on-miss, the XSPICE
   auto-bridge `strcmp`, and `src/xspice/evt/evttermi.c:304`. This record fixes
   the rule they must implement (decision 2); it does not implement it. Gate 1
   closed with `doc/claude/decisions/0002-deferred-node-resolution-check.md`,
   which is decision 2 applied at the parser and answers the questions this
   record left open there: where the "created by a reference, never defined"
   bit lives, in which modes the check reports, and why it is a warning and not
   an error. **Gates 2 and 4 have since closed too**, and with them the Phase 3
   gate list. Both turned out to be `preserve` defects rather than `distinguish`
   ones — under `preserve` two spellings are one identifier, so `strcmp`
   refused to bridge and refused to intern a net that was one net — so what
   they moved is the mode that has shipped, and neither delivered anything to
   `distinguish`, in which `strcmp` was already the right answer.
   `set_case_mode()` no longer names them. It still says `distinguish` is
   experimental, because closing the gate list did not exhaust the silent
   failures. `doc/codex/issues/0029`, the silent wrong bridge voltage under
   `distinguish`, has since closed as well, and with it that clause of the
   warning; so has `doc/codex/issues/0030`, the near-miss diagnostic these two
   gates did not implement, which moved the XSPICE event node from the silent
   half of that warning's sentence to the reported half — see
   `doc/claude/decisions/0003-event-node-near-miss.md`, which also records why
   the interner reports a node nothing drives rather than the near-miss
   definition `0030`'s acceptance criterion asked for. `doc/codex/issues/0027`,
   `unlet` matching a vector name case insensitively, has closed too — see item
   4 below — so what the warning names now is `doc/codex/issues/0032`, the
   vector name matchers outside `findvec()`, whose scale-vector rows confuse a
   case variant of the current plot's scale with the scale itself. The word
   `experimental` stays for the reasons in
   `doc/claude/decisions/0004-unlet-vector-identity.md` decision 6, of which
   the strongest is not an open issue at all: decision 5's migration hazard is
   inherent to what `distinguish` means, because a deck that spells one net two
   ways becomes two nets through two *definitions*, and this record's decision
   2 deliberately does not warn on a definition.

   `0029`'s (a) is worth reading beside decision 3, though it is not an
   identifier comparison and so is not in its table. It is Class C's rule
   applied to a *probe*: the query was a lower-case literal ngspice built and
   the table held deck text, so under `distinguish` the tolerance had to go on
   the query side. It went into a separate numparam entry point,
   `entrynb_constructed()`, which retries an exact miss case insensitively on
   the final dot-separated component only — `symbol_key()` stays exact, so
   nothing of Phase 2 is undone, and the deck's own subcircuit instance path
   before the last `.` still matches exactly. The split inside one string is
   the same one `vec_wrapped_name_eq()` makes for the `V` of `V(1)` and
   `Evt_Parse_Node()` makes for the member of `node(member)`.
2. **Whether `distinguish` should be refused in combination with
   `ngbehavior=hs*`.** The spec's Open decision 4. A PDK-consuming deck under
   `distinguish` is the hazard in decision 5 with a vendor library attached.
3. **The OSDI duplicate-parameter diagnostic** (decision 4). A load-time
   collision check in `src/osdi/osdiinit.c`, mode-independent.
4. **`vec_remove()`, `src/frontend/vectors.c:613`,** which found the vector to
   `unlet` with `cieq` unconditionally. Under `distinguish` `unlet Out` removed
   `OUT`. Not on any gate list; filed as `doc/codex/issues/0027`. It is the
   same shape as `evtaccept.c:342` and `evtplot.c:110`, which gates 2 and 4
   fixed with `Evt_Node_Name_Eq()`; `0027` takes the same predicate and was
   deliberately not folded in. **Closed** at `756112c46` with
   `doc/claude/decisions/0004-unlet-vector-identity.md`, which took
   `vec_name_eq()` rather than `Evt_Node_Name_Eq()` — the two are the same rule
   on opposite sides of the mixed-signal boundary, and this is the analog side.
   It reached further than `unlet`: `com_compose()` and `com_cross()` call
   `vec_remove()` to clear a name before defining it and were destroying case
   variants too. Three things `0027` did not decide came out of it —
   `doc/codex/issues/0032`, `0033` and `0034` — and `0034` is this record's
   decision 2 being violated in the tree today, by `let`.
5. **The build-enforced lint** that would stop a new `strcmp` against a
   lower-case literal from re-entering the tree. Still wanted, still needs no
   decision.
