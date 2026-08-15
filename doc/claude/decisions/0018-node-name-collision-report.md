# Decision 0018 — reporting a node name the deck spells two ways

## Status

Accepted, 2026-08-14, branch `ver_50`. Implements `doc/codex/issues/0068` on
the repo owner's decision. **Amends `doc/claude/decisions/0001-distinguish.md`
decision 2**, which declined this diagnostic; the amendment is decision 1
below and 0001's own table now carries a pointer to it.

## Context

Three modes, one deck, three different circuits and no word from any of them:

```
V1 in 0 dc 1.5
R1 in Out 1k
R2 out 0 1k
```

`fold` lower cases both cards and gets one node. `preserve` interns both
spellings into one entry and gets one node. `distinguish` interns two entries
and gets two, one of which is a dangling divider leg. Measured at `58496a8dc`:
the only line that moved between the three runs was the number.

---

## Decision 1 — the report is owed in all three modes, and decision 2's refusal is re-scoped rather than reversed wholesale

**Decided: warn, in `fold`, `preserve` and `distinguish`, once per colliding
pair per parse, with the sentence naming what the mode did:**

```
Warning: node names 'Out' and 'out' differ only in case and name one node (casemode=fold)
Warning: node names 'Out' and 'out' differ only in case and name one node (casemode=preserve)
Warning: node names 'Out' and 'out' differ only in case and name two nodes (casemode=distinguish)
```

`doc/claude/decisions/0001-distinguish.md` decision 2 rejected exactly this
predicate — "warn on definition, i.e. whenever a second name differing only in
case from an existing one is created" — and the argument for reversing it is
in `doc/codex/issues/0068` under "Why this is not what decision 2 rejected".
It is not repeated here in full; what belongs in a decision record is the part
that survives as a rule:

**A `distinguish`-only warning on a definition would still be wrong.** That is
what decision 2 was asked about and its answer stands. Under `distinguish`,
two case-variant nets are the feature, and a diagnostic that fires only there
can only be read as *you have made a mistake*.

**A three-mode report of an outcome is a different object.** It is available
only because the same detection is owed in the two modes that merge, where
silence costs at least as much and where the decks that have shipped live. The
sentence differs per mode and names the circuit that was built, so the
`distinguish` user who wanted two nodes is told they got two. The three
sentences are the decision; a single sentence in a single mode is the thing
decision 2 refused.

**The cost is real and is not argued away.** A deck that deliberately runs two
case-variant nets under `distinguish` gets one line per pair on every parse and
cannot silence it. `tests/regression/casedist/node-case-collision-report.cir`
is such a deck on purpose, and so is
`tests/regression/casedist/node-case-split.cir`, the deck that demonstrates the
feature — it now warns about the very thing it exists to show. Once per pair
rather than once per occurrence is what keeps that bearable, and it is the
whole of the mitigation.

**Corrected 2026-08-14: the cost was under-counted, and this paragraph is not
the last word on it.** "Once per pair" is not once per mistake. After
subcircuit expansion each instantiation of a body carries its own pair, so one
two-way spelling inside one `.subckt` body is one line **per instance**.
Measured on shipped decks: spelling the `nand2` body's internal node `sig3` as
`SIG3` on a single card produces 416 lines on
`examples/klu/Circuits/85/c1355/c1355.net`, 454 on `c5315.net` and 1028 on
`c7552.net` — one per instantiation, exactly. `not1` is instantiated 1410
times in `c7552.net` and in each of its three PDK variants. Nothing above
considers a deck of that shape; the paragraph was
written about a top-level pair, where "once per pair" and "once per mistake"
are the same number. Whether the report should collapse per subcircuit
definition, per distinct unscoped pair, or stay as it is, is an open question
for the repo owner, written up with the cost of each option in
`doc/codex/issues/0068` under "Open question for the owner". **This decision
does not answer it**, and should be amended here when it is answered.

**Rejected: a variable to suppress it.** A second knob for a diagnostic that
fires on a deck shape this rare buys little and has to be documented,
defaulted, reset across `ngSpice_Reset` and explained in `src/spinit.in`. If
the noise turns out to matter, the answer is a general diagnostic-suppression
mechanism and not a name of this one.

**Rejected: widening it past nodes.** `.model`, `.subckt`, `.global` and
`.param` names spelled two ways stay silent. They are not interned by
`term_insert()`, they have no single place where both spellings meet, and each
would need its own detection with its own false-positive surface. The last row
of decision 2's table therefore still reads as written, and the `distinguish`
banner names this as the silence that remains.

## Decision 2 — the detection lives at the node symbol table, and nowhere else

**Decided: `term_insert()` records, `INPtermCaseCheck()` reports, both in
`src/spicelib/parser/inpsymt.c`.** No caller of `INPtermInsert()` acquires a
check of its own.

That is the one place node identity is decided, and in two of the three modes
it is also the one place both spellings are in scope:

- **`preserve`** — `ent_eq()` is `cieq`, so the second spelling arrives at an
  entry that holds the first. `t_casetwin` keeps it.
- **`distinguish`** — `ent_eq()` is `strcmp`, so the two spellings are two
  entries, necessarily in one bucket because `hash()` folds the bucket key
  whenever the reader is not folding. The pair is found by scanning the chain.
- **`fold`** — neither. See decision 3.

Reporting at the end of the parse rather than at the insert is what makes it
once per pair: `Out` on forty cards and `out` on forty more is one line. It
also puts the new report beside the two `INPtermCaseCheck()` already makes, so
the three can be kept from saying the same thing twice — a node that no card
defines is `doc/codex/issues/0028`'s undefined-node report or decision 2's
near miss, and the pair report skips any node with `t_unclaimed` set on either
side.

**Both of those skips are load-bearing and both are now guarded** (added
2026-08-14; they were not when this record was written, and with both deleted
and the tree rebuilt all 148 decks in `tests/regression/case` and `casedist`
still passed). Which of the two is asked depends on the order the two
spellings were interned, because the bucket chain is prepended to, so one deck
cannot reach both: `NMDEF` and `NMREF` in
`tests/regression/casedist/node-case-collision-report.cir` are the two orders,
and deleting one guard at a time fails exactly one of them. `NMPAIR` in the
`tests/regression/case` deck is the single-entry shape, where one entry can be
unclaimed and still hold a case twin because `term_insert()` keeps the second
spelling whether the caller was defining or resolving.

`NMPAIR` in `tests/regression/misc` is that same shape under `fold`, added
2026-08-14. This paragraph first said the opposite — "under `fold` neither
guard is reachable at all: `t_unclaimed` is set only by `mkvnode()`, only
reachable from a B source expression, and a B card has no `line_case`" — and
that is **false**. `mkvnode()` is one of seven callers of
`INPtermInsertRef()`; `INPgetValue()` and `dot_noise()`, `dot_tf()`,
`dot_sens()`, `dot_pss()` and `dot_hb()` are the others, and a dot card is the
deck's own text and carries `line_case` like any other card. `.tf v(MISS)` on
one card and `.tf v(Miss)` on the next therefore leave an entry that is
unclaimed and holds both spellings, and with the outer guard deleted it
collects the undefined-node line and the pair line at once. The outer guard is
the only one `fold` can reach — the inner one is in the chain scan, which
`INPtermCaseCheck()` returns from before it starts in any mode but
`distinguish` — so the `misc` case tracks the `case` case exactly, and the
table in `doc/codex/issues/0068` criterion 5 has the four columns.

## Decision 3 — `fold` is served by keeping the card's pre-fold text, not by a witness of the deck's words

This is the decision with the most alternatives, because under `fold` the
symbol table genuinely cannot see the second spelling. `inp_readall()`
(`src/frontend/inpcom.c`) lower cases each card in place as it is read, before
comment stripping, before continuation stitching, before `.include`, before
numparam and before `inp_subcktexpand()`. Nothing in the tree keeps the
pre-fold text — that is what `preserve` exists to change.

**Decided: `struct card` gains `line_case`, the card's own text as the deck
wrote it, attached by the reader one statement before it folds the line;
`INPcurrent_card` (set by `INPpas2`) carries the card to `term_insert()`,
which recovers the node's spelling by finding the token as a whole word in
that text, skipping the first field.**

Three things make that sound rather than approximate:

1. **It is the card's own text.** A card the preprocessor built has none —
   `insert_new_line()` and the hand-rolled `TMALLOC(struct card, …)` sites all
   leave it NULL, and `tmalloc()` is `calloc`, so that is the default rather
   than a thing each site must remember. A subcircuit body copied for an
   instantiation therefore answers NULL rather than answering from the body's
   text, which describes formal pins and not the node being interned.
2. **The first field is skipped.** A device card's nodes are fields 2..n+1,
   immediately after the instance name, so the first whole-word match at or
   after field 2 is the node's own spelling. Skipping field 1 is precisely
   what keeps an instance `R1` from supplying a spelling for a node `r1`,
   which is `0068`'s acceptance criterion 4 and the case the corpus warns
   about.
3. **The first match wins, and a disagreement later on the card is ignored.**
   `C1 Out 0 1p ic=OUT` gives `Out`, not a collision.

**Rejected: a deck-global witness of pre-fold spellings**, i.e. recording at
the reader, for every identifier-shaped token, the set of spellings the deck
used, and reporting when a node name has two. It is the obvious cheap answer
and it is unsound in two independent ways, both of which the corpus exhibits:

- **Namespace.** The witness cannot know which occurrences were nodes. A node
  `out` beside a `.model OUT`, a `.param Out`, a subcircuit named `OUT` or a
  bare keyword spelled two ways all produce a pair the node table then
  confirms is a node name. Filtering by "is it also an instance or model
  name?" removes the loudest cases and not the tail; `.model dmod d` beside a
  subcircuit pin `D` is a false line, and so is a node named `dc`.
- **Scope.** The witness is per deck and node names are per subcircuit.
  `examples/p-to-n-examples/switch-oscillators.cir` writes `.subckt invertern
  In Out VDD DGND` with a body that uses `out`; a top-level node `out`
  spelled consistently would be reported on the strength of a spelling from
  inside a subcircuit it has nothing to do with.

**Rejected: mapping an expanded card back to the body card it came from.**
Same scope failure from the other end: `.subckt div a b` with body `R1 a b 1k`
instantiated as `X1 A b div` would report `A` against `a`, which are an actual
and a formal and were never two spellings of one node.

**One card ngspice writes itself had to be excluded, and it was found by
measurement.** The XSPICE auto-bridge builds a deck of bridge instance cards
and re-enters `inp_readall()` to parse it (`create_deck()`,
`src/xspice/evt/evtcheck_nodes.c`), so those cards were handed a `line_case`
like any others — and a bridge card spells its node the way the node is
*already interned*, which under `fold` is lower case. That let a generated line
stand as evidence that the user had written a second spelling.
`examples/xspice/verilator/adc.cir` reported a pair on the strength of
`auto_adc4 [ start ] [ start ] auto_adc`, and it was the only line in the whole
corpus that `fold` produced and `preserve` did not — which is exactly what a
three-mode sweep is for. `Evtcheck_nodes()` now clears `line_case` on the deck
it generates. This is decision 3 of `0001` again: exact about names the deck
chose, indifferent to names ngspice constructs, applied to a spelling rather
than to an identity.

**The gaps this leaves, stated rather than discovered later.** Under `fold` a
spelling can only come from a card the parser reads as the deck wrote it, and
four shapes fall outside that. All four are silences, not wrong lines, and all
four were measured across `tests/` and `examples/`:

1. **A subcircuit body.** Two spellings of one internal node are reported under
   `preserve` and `distinguish` as `x1.Mid` and `x1.mid` — the expander
   prefixes the scope and the spellings survive it — and not under `fold`,
   where the expanded card is built by the preprocessor and carries no
   pre-fold text. `examples/xspice/see/CMOSComparator/Fig27_12_see.sp`
   (`X1.Vr`/`x1.vr`), `tests/regression/case/global-node-case.cir` and
   `name-lookup-case.cir` (`vss`/`VSS`). All three decks guard this: the
   `tests/regression/misc` BODY case asserts the silence and the other two
   assert the report, so the asymmetry is frozen and visible.
2. **An `X` card's actual.** The `X` card is consumed by the expander, so a
   node whose second spelling appears only there gets none.
   `examples/tclspice/tcl-testbench3/FB14.cir` (`Vcc`/`vcc`, `Vimage`/`vimage`).
3. **A card the preprocessor rebuilds — every B source, and every `E`, `G`,
   `R`, `C` or `L` card carrying an expression.**

   **Corrected 2026-08-14.** This item used to read "a spelling inside an
   arithmetic expression", and blamed `card_word_boundary()` for leaving `+`,
   `-`, `*` and `/` inside words: "`V(VFreq)*V(VTime)` is one word and the
   `VFreq` in it is not found". Both halves are wrong. The named cause is not
   the cause, and the gap is not the expression — it is the whole card, and
   every card of several kinds rather than the two decks the item cited.

   The attribution is falsified by the function itself. `(` and `)` **are**
   boundaries, so `V(VFreq)*V(VTime)` is not one word: `VFreq` starts after a
   `(` and ends before a `)`, which is exactly the shape `card_spelling()`
   looks for. With `card_word_boundary()` and `card_spelling()` copied
   verbatim into a standalone harness and handed the offending line of
   `examples/various/FFT_Leakage.cir`, it returns `VFreq` — and `VTime`:

   ```
   card_spelling("BVAC IN 0 V = sin( 6.283185307179586*V(VFreq)*V(VTime))", "vfreq") = VFreq
   ```

   It is falsified a second time by a deck with no arithmetic in it at all.
   `V1 Mid 0 dc 1` / `R1 Mid 0 1k` / `B1 mid 0 I = 1m` puts the second spelling
   in the B card's own **node field**, where no boundary question arises, and
   `fold` is still silent while `preserve` reports. Nothing about expressions
   is involved.

   The real cause is one line up the pipeline. `inp_bsource_compat()`
   (`src/frontend/inpcom.c`) comments the B card out — `*(card->line) = '*'` —
   and inserts a **replacement** built by `insert_new_line()`, which sets
   `line_case` to NULL. The card the parser reaches is therefore not the card
   the deck wrote, and by rule 1 of this decision it answers NULL rather than
   answering from someone else's text. That is the design working as intended;
   what was wrong was the account of which cards it applies to. It applies to
   **every node named anywhere on a B card**: both terminals, and every `V()`
   or `I()` inside the expression. `inp_compat()`, which runs immediately
   before it, uses the same two steps — `*(card->line) = '*'` then
   `insert_new_line()` — on an `E`, `G`, `R`, `C` or `L` card whose value is an
   expression, whatever it rewrites that card into. Measured, one deck per row:

   | card | second spelling | `preserve` | `fold` |
   | --- | --- | --- | --- |
   | `B1 mid 0 I = 1m` | node field | reported | **silent** |
   | `B1 out 0 V={V(mid)*2}` | in the expression | reported | **silent** |
   | `E1 o 0 vol='V(mid)*2'` | in the expression | reported | **silent** |
   | `G1 o 0 cur='V(mid)*1m'` | in the expression | reported | **silent** |
   | `R2 o 0 R='1k+V(mid)'` | in the expression | reported | **silent** |
   | `E1 o 0 mid 0 2` | controlling node field | reported | reported |

   The last row is the control: a plain linear `E` card is rewritten by
   nothing, keeps its own text, and reports.

   **Size.** 236 B cards in 53 of the 875 files under `tests/` and
   `examples/` — a syntactic count of cards whose first token begins with `b`,
   outside `.control` blocks and excluding each file's title line, so it is
   the right order rather than exact. In those 53 decks no node mentioned on a
   B card can contribute
   a spelling under `fold`, which is the mode ngspice ships in. Three of the
   ten `preserve` reports that have no `fold` twin are this gap:
   `examples/various/FFT_Leakage.cir` (`Vfreq`/`VFreq`),
   `tests/regression/casedist/bsource-node-case.cir` and `bnr_miss.cir`
   (`In`/`in`, the second an untracked run artifact of
   `bsource-node-case-report.cir`).

   The old item's closing sentence — "the trade is deliberate and it is the
   safe direction" — was defending a trade that was never made. The boundary
   set is still deliberately narrow and the reasoning for it stands; it simply
   costs nothing that is otherwise recoverable, and no measured line in the
   corpus is lost to it.

4. **An XSPICE `a` card whose node list has no space inside the brackets.**
   `[` and `]` are not in `card_word_boundary()`, so `a1 [in1 in2] [out] m`
   yields no spelling for `in1`, while `a1 [ in1 in2 ] [ out ] m` — the
   spacing every shipped example uses — yields one. Measured both ways on a
   deck with `V1 In 0 dc 1.5` above it: the spaced form reports under `fold`
   and the tight form does not. No deck in the corpus is affected, which is
   why the sweep did not find it; it is listed because it is a silence of the
   same kind and a reader looking for the boundary set's real cost should find
   it here rather than the sentence corrected above. Adding `[` and `]` to
   `card_word_boundary()` would close it, and is safe in the way `(` and `)`
   already are, but it is a change with no measured line behind it and it is
   not made here.

Closing 1 and 2 means giving the subcircuit expander a pre-fold text of its
own, which is a change to `src/frontend/subckt.c` and a separate issue.
Closing 3 means giving `insert_new_line()` a way to inherit the pre-fold text
of the card being replaced, which is a change to `src/frontend/inpcom.c` and a
narrower one than it sounds — `inp_bsource_compat()` and `inp_compat()` both
already hold the card they are replacing — but it has to be shown not to hand
a rewritten card the text of a card it is not a rewrite of, which is the
failure mode rule 1 exists to prevent.

**A consequence gap 3 does NOT have, corrected 2026-08-14, then corrected
again the same day.** This paragraph first read: "`t_unclaimed` is set only by
`mkvnode()`, which is only reached from a B source expression, and a B card has
no `line_case`. So under `fold` an unclaimed entry can never carry a recovered
spelling, and the pair report's `t_unclaimed` guards are unreachable in that
mode … there is nothing for `tests/regression/misc/` to assert." Both halves
are false. `mkvnode()` is one of seven **calling functions** of
`INPtermInsertRef()` — `INPgetValue()` (`src/spicelib/parser/inpgval.c`) and
`dot_noise()`, `dot_tf()`, `dot_sens()`, `dot_pss()` and `dot_hb()`
(`src/spicelib/parser/inp2dot.c`) are the rest; ten call sites in all, because
the first three of those `inp2dot.c` functions call it twice each — and a dot
card is a card the deck wrote, so it carries `line_case` and answers
`card_spelling()` normally. The falsifying deck:

```
.options noacct rshunt=1e9
V1 in 0 dc 0 ac 1
R1 in 0 1k
.noise v(MISS) V1 dec 10 1 100
.tf v(Miss) V1
```

**What that correction then got wrong.** The sentence written to replace the
false one — "with the outer guard deleted it prints that line and the pair
line both, in `fold` as in the other two modes" — is itself false, for
`distinguish`. Re-measured on this deck, one build per column, the three modes
selected with `-D casemode=`:

| mode | shipped | outer guard deleted | inner guard deleted | both deleted |
| --- | --- | --- | --- | --- |
| `fold` | 1 undefined-node line | that line **and the pair line** | 1 line, unchanged | that line and the pair line |
| `preserve` | 1 undefined-node line | that line **and the pair line** | 1 line, unchanged | that line and the pair line |
| `distinguish` | **2** undefined-node lines | 2 lines, **unchanged** | 2 lines, unchanged | 2 lines and the pair line |

The reason is the shape decision 2 already states from the other end. Under
`fold` and `preserve` the two spellings land on **one** entry, which ends the
parse unclaimed with `t_spelling = MISS` and `t_casetwin = Miss`, and the outer
guard is the only thing between that entry and the pair line. Under
`distinguish` they are **two** entries, so neither carries a `t_casetwin` at
all and the pair can only be found by the chain scan — whose own
`!u->t_unclaimed` test rejects the other entry for exactly the reason the outer
one rejects this entry. Both guards have to go before `distinguish` says
anything, which is why its row moves only in the last column. The same
asymmetry is why `distinguish` prints the plain undefined-node line twice
rather than the near-miss line once: the first scan's twin lookup is
`!u->t_unclaimed` too, and here neither side is claimed.

**The guards are load-bearing in every mode; this deck is not what shows it
under `distinguish`.** `tests/regression/casedist`'s `NMDEF` and `NMREF` are.
Measured, each guard deleted on its own and the three guard decks re-run:
outer-only fails all three directories — `misc` and `case` on `NMPAIR`,
`casedist` on `NMDEF`'s `pair5` — and inner-only fails `casedist` alone, on
`NMREF`'s `pair6`. That is the mutation table in `doc/codex/issues/0068`
criterion 5, reproduced row for row. `tests/regression/misc/` owes exactly the
case it now has, `NMPAIR`, built from two `.tf` cards. Gap 3 is real and
unchanged: it is about the B card, not about `t_unclaimed`.

**A field appended, not inserted.** `line_case` goes at the end of
`struct card`. `src/xspice/icm/dlmain.c` is compiled into every `.cm`, and a
field ahead of `w`/`l`/`nf`/`compmod` moves them for a code model built against
an older header. This was found by measurement, not by reading: the first
attempt put the field after `level` and the tree's own `examples/` decks
segfaulted inside the installed `spice2poly.cm`. The crash turned out to
pre-date this work — the same decks crash at `58496a8dc` with the codemodels
installed in this environment — but the hazard the investigation surfaced is
real and the field stays at the end.

## Decision 4 — wording

**Decided:**

```
Warning: node names '%s' and '%s' differ only in case and name %s (casemode=%s)
```

with `one node` / `two nodes` and the mode's own name from
`inp_case_mode_name()`, so the three spellings of the mode live in one file.

- **`node names` leads the line** so that neither a reader nor a deck's
  `strstr()` can confuse it with the two reports `INPtermCaseCheck()` already
  makes, both of which begin `no node named` and are about a name that failed
  to resolve. `tests/regression/casedist/bsource-node-case-report.cir` scans
  for `no node named` and for `differs only in case`; this line contains
  neither, and that is checked rather than assumed.
- **The spellings are named in the order the deck wrote them.** Under
  `preserve` and `fold` that is the entry's spelling then `t_casetwin`; under
  `distinguish` the bucket chain is prepended to, so an entry further along it
  was interned earlier and is named first.
- **`name one node` rather than `are one net`.** The owner's decision says
  "net"; ngspice's own diagnostics say "node" everywhere, including the two
  lines directly above this one in the same function, and a report that
  switched vocabulary mid-file would read as a different subsystem talking.
- **Three spellings are one line under `fold` and `preserve` and three under
  `distinguish`, and the asymmetry is the truth rather than an oversight.**
  `Out`, `out` and `OUT` are one node in the merging modes, so one line naming
  the first two says everything there is to say; under `distinguish` they are
  three nodes and every one of the three pairs is a distinct ambiguity. The
  entry holds one `t_casetwin` in the first case and the bucket scan walks
  every pair in the second, so the code says the same thing the sentence does.
- **It goes to `cp_err`**, not to `stderr`, per
  `doc/claude/decisions/0006-diagnostic-deck-coverage.md`: a literal
  `fprintf(stderr, …)` cannot be captured with `>&` and therefore cannot be
  guarded by a deck.

## Decision 5 — what this record does not decide

1. **The other namespaces.** `.model`, `.subckt`, `.global` and `.param`
   names spelled two ways. Decision 1 says why not, and the `distinguish`
   banner now names them as the silence that remains.
2. **The subcircuit-body gap under `fold`.** Decision 3.
3. **A suppression variable.** Decision 1.
4. **XSPICE event nodes.** `EVTnode_insert()`
   (`src/xspice/evt/evttermi.c`) is the event side's interning site and has
   the same shape, so an event node written two ways is the same defect in the
   other table. It is not fixed here: `EVTnode_case_check()` already reports a
   near miss there and the two would have to be reconciled, which is its own
   measurement.
5. **Whether the word `experimental` can now come off `distinguish`.** It
   cannot, and this record does not argue about it:
   `doc/claude/decisions/0004-unlet-vector-identity.md` decision 6 is still
   the answer. What changes is only which silence the banner names.
6. **Whether one authoring mistake should be one line.** Added 2026-08-14,
   after the other five, so that the numbering above stays as four records
   cite it. The report is once per pair, and one two-way spelling in a
   `.subckt` body is one pair per instantiation — 416, 454 and 1028 lines from
   one character on three shipped `examples/klu` netlists. Decision 1 costed
   the wrong thing and now says so. The options and their costs are in
   `doc/codex/issues/0068` under "Open question for the owner"; the answer is
   the owner's, and it amends decision 1.
