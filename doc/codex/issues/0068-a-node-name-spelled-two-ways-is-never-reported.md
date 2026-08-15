# Issue: A Node Name The Deck Spells Two Ways Is Never Reported

## Status

Closed. Filed on the repo owner's decision, which is quoted verbatim in
"The decision" below. This is the definition-side diagnostic that
`doc/claude/decisions/0001-distinguish.md` decision 2 declined, and the
section "Why this is not what decision 2 rejected" is the argument for
re-opening it; decision 2 needs amending and this issue says how.

## Summary

A deck that writes one node name with two spellings is ambiguous, and every
`casemode` resolves the ambiguity silently:

```
V1 in 0 dc 1.5
R1 in Out 1k
R2 out 0 1k
```

- `casemode=fold` — the reader lowercases both cards, `Out` and `out` are one
  node, and `v(out)` is 7.500000e-01.
- `casemode=preserve` — `ent_eq()` (`src/spicelib/parser/inpsymt.c:33`) is
  `cieq`, so the two spellings intern to one entry. Same one node, same
  7.500000e-01.
- `casemode=distinguish` — `ent_eq()` is `strcmp`, so they intern to two
  entries. `R2`'s node is a dangling divider leg, `v(out)` is 0.

Measured at `58496a8dc` on that deck: three runs, three parses, not one word
about `Out` and `out` in any of them. The only line that moves between the
modes is the number.

The `distinguish` banner is not that word. It is printed by `set_case_mode()`
(`src/frontend/inpcom.c:1232`) once per run, before any deck is read, and it
says this happens *in general*:

```
Warning: casemode 'distinguish' is experimental. Identifier identity is case
sensitive, ... but a deck that spells one net two ways becomes a deck with two
nets and nothing says so, because both spellings are definitions.
```

It fires on every `distinguish` run whether or not the deck in hand has the
problem, and it is silent in the two modes that have shipped.

## Impact

**Silent, and in opposite directions, in all three modes.** The user is not
told which of the two readings the simulator took, and the two readings are
different circuits. Under `fold` and `preserve` a deck that meant two nets
gets one and the extra net's cards are quietly paralleled onto it; under
`distinguish` a deck that meant one net gets two and half its cards are
quietly disconnected. Neither is an error — a floating node is legal SPICE —
so nothing downstream reports it either.

This is the failure mode that keeps `distinguish` marked experimental.
`doc/claude/decisions/0001-distinguish.md` decision 6 item 1 names it as the
strongest reason the word stays, and names the reason it is undiagnosed in the
same sentence:

> decision 5's migration hazard is inherent to what `distinguish` means,
> because a deck that spells one net two ways becomes two nets through two
> *definitions*, and this record's decision 2 deliberately does not warn on a
> definition.

Measured on the tree's own corpus at `58496a8dc` — every `.cir`, `.sp`,
`.net`, `.ckt`, `.mod`, `.inc` and `.lib` under `tests/` and `examples/`, 935
files — a syntactic scan that reads each device card's node fields finds 44
files that spell some node name two ways. The scan over-reports (it counts
`GND`/`gnd`, which `inp_fix_gnd_name()` turns into `0` long before the parser,
and it mis-sizes the node lists of PSpice `U` primitives), so the number that
matters is the one the implementation measures, not this one; what it
establishes is that the shape is ordinary rather than exotic. `examples/soi/Inv_chain.sp`
writes `out2` on one card and `Out2` on the next, four times over.

## Root Cause

There is no check. The two spellings meet at exactly one place —
`term_insert()` in `src/spicelib/parser/inpsymt.c`, the node symbol table's
find-or-create — and that place has no reason of its own to notice: under
`preserve` `ent_eq()` says they are the same node and returns the existing
entry, and under `distinguish` `ent_eq()` says they are different nodes and
makes a second one. Both answers are correct for the mode. Nobody asks the
third question, which is whether the deck can have meant both.

Under `fold` the two spellings never meet at all. `inp_readall()`
(`src/frontend/inpcom.c:2168`) lowercases the card in place as it is read,
before comment stripping, before continuation stitching, before `.include`
expansion, before numparam substitution and before `inp_subcktexpand()`.
Nothing in the tree keeps the pre-fold text — that is what `preserve` exists
to change — so by the time any node is interned the second spelling is gone.
This is the structural reason the fold arm of the decision is harder than the
other two, and the Resolution says what was done about it.

## The decision

From the repo owner, and this issue records the reasoning as given:

> Warn when two identifiers differ only in case, in all three modes, with
> wording matched to what actually happened:
>
> - fold and preserve: the two spellings became one net
> - distinguish: the two spellings became two nets
>
> The deck is ambiguous whichever mode runs it, and today every mode is
> silent. Under fold/preserve it merges without a word; under distinguish it
> splits without a word — the experimental banner is generic, not about the
> deck. So the same detection serves all three and only the sentence changes.

## Why this is not what decision 2 rejected

This is the paragraph the issue exists for, because
`doc/claude/decisions/0001-distinguish.md` decision 2 rejected a warning on a
definition in as many words:

> Rejected: **warn on definition**, i.e. whenever a second name differing only
> in case from an existing one is created. It fires on the legitimate deck.
> Under `distinguish`, `Out` and `OUT` as two real nets is not a mistake, it
> is the feature; a warning there makes the feature unusable and trains users
> to ignore the warning that matters.

The predicate is the same one. It would be dishonest to claim otherwise: this
issue does not find a narrower condition that decision 2 overlooked, it
reverses decision 2's judgement of that condition. Four things changed, in
descending order of weight.

**1. Decision 2 was answering a `distinguish`-only question, and a
`distinguish`-only warning really is unusable.** Read the heading: "the
diagnostic for a case near-miss". Every row of its table is a *resolution*
site, and every one of them is silent under `fold` and `preserve` for a
structural reason it states — "a condition that cannot arise at all under
`fold` or `preserve`, where the lookup would have succeeded". A definition
warning bolted onto that table would have inherited its scope, and a warning
that appears only under `distinguish` can only be read as *you have made a
mistake*. In the one mode where two spellings are deliberate, that is exactly
the wrong sentence, and decision 2 is right to refuse it.

The owner's decision is not that warning. It is a three-mode report of an
outcome, and it says which outcome: `fold` and `preserve` are told the deck
became one node, `distinguish` is told it became two. A user who wanted two
nets under `distinguish` is told they got two nets. That is a confirmation,
not an accusation, and it cannot be got at all in the shape decision 2 was
considering, because two of the three sentences do not exist there.

**2. The silence decision 2 chose is not neutral between the modes, and it was
not costed as if it were.** Decision 2's whole justification for silence on a
definition is about `distinguish`: two real nets are the feature. It never
asks what silence costs under `fold` and `preserve`, because those modes were
not the subject. The cost is symmetric and it is larger, because those are the
modes with shipped decks: a deck that meant two nets silently gets one, and
the deck author has no mode in which the merge is visible. Decision 5's
withdrawn-guarantees list is written entirely from the `preserve` side —
"Under `preserve` these are one thing; under `distinguish` they are two" —
which is the same fact seen from the other end and left equally unreported.

**3. Decision 6 item 1 already names this as the record's own blind spot.**
The sentence quoted under Impact is decision 0001 saying that the reason
`distinguish` stays experimental is a failure decision 2 declines to
diagnose. A record that names its own gap in its closing section has left the
question open, not settled it; this issue is the answer to that sentence and
not a re-litigation of decision 2's.

**4. "Trains users to ignore the warning" is a frequency argument, and the
frequency is answerable.** The report is once per colliding pair per parse,
not once per occurrence, so a deck with `Out` on forty cards and `out` on
forty more gets one line. A deck that deliberately runs two case-variant nets
under `distinguish` gets one line per pair and can silence none of them —
that is this decision's real cost and it is stated as such below, not argued
away.

**What stays rejected.** Decision 2's other refusal — an *error* on a
resolution miss — is untouched; this is a warning and the parse continues.
And the scope stays at nodes: `.model`, `.subckt`, `.global` and `.param`
names spelled two ways stay silent, so the last row of decision 2's table
still reads as written. Those namespaces are not interned by
`term_insert()`, they have no single meeting point of the kind the Root Cause
describes, and nothing about this decision argues they should move; if they
should, that is a separate issue with its own measurement.

**How decision 2 must be amended.** One row changes:

| Site | Rule | State before | State after |
| --- | --- | --- | --- |
| `INPtermInsert()` from a device card | definition | silent, deliberately | **reported**, in all three modes, once per pair, by this issue |

and the "Rejected: warn on definition" paragraph must be re-scoped rather than
deleted: what is rejected is a definition warning *as part of the near-miss
diagnostic*, i.e. one that appears only under `distinguish` and therefore
reads as an accusation. The paragraph is still the right answer to the
question decision 2 asked. The amendment belongs in decision 0001 itself,
with a `Corrected` note in its own style, and a new decision record carries
the three-mode argument.

The `distinguish` banner's last clause — "and nothing says so, because both
spellings are definitions" — becomes false with this issue and must be
rewritten, which is the fourth time that clause has changed subject and the
first time it can be deleted rather than replaced.

## Acceptance Criteria

1. A deck that writes one node name with two spellings produces exactly one
   warning per colliding pair per parse, in all three modes, naming both
   spellings. The sentence says the two spellings are one node under `fold`
   and `preserve` and two nodes under `distinguish`.
2. The warning goes to `cp_err`, not to `stderr`, so that a deck can capture
   it with `>&`; `doc/claude/decisions/0006-diagnostic-deck-coverage.md`.
3. One detection site. The check lives where node identity is decided and
   nowhere else, and no caller of `INPtermInsert()` acquires a check of its
   own.
4. It does not fire on a device instance name that collides with a node name,
   nor on a `.model`, `.subckt`, `.global` or `.param` name: those are other
   namespaces and two spellings there are not two nodes.
5. It does not double up with the reports already at this site. A node that a
   reference created and no card defined is `doc/codex/issues/0028`'s
   undefined-node report or decision 2's near miss, and stays exactly one of
   those; the new report is about two nodes both of which cards define.
6. Every deck under `tests/` and `examples/` runs in all three modes and every
   new line of output is accounted for: a line on a deck that genuinely spells
   one node two ways, and no line anywhere else.
7. Guarded by decks that assert the report **and** the silences, in the shape
   `doc/claude/decisions/0006-diagnostic-deck-coverage.md` decision 2
   establishes.
8. The `distinguish` banner no longer claims the split is unreported.

## Resolution

Implemented on branch `ver_50`. The decision record is
`doc/claude/decisions/0018-node-name-collision-report.md`, which also carries
the amendment to `doc/claude/decisions/0001-distinguish.md` decision 2 that
this issue's argument section asks for.

### What was built

One detection site, `src/spicelib/parser/inpsymt.c`. `term_insert()` records
the second spelling on the symbol-table entry; `INPtermCaseCheck()` reports it
once per colliding pair at the end of the parse, to `cp_err`:

```
Warning: node names 'Out' and 'out' differ only in case and name one node (casemode=fold)
Warning: node names 'Out' and 'out' differ only in case and name one node (casemode=preserve)
Warning: node names 'Out' and 'out' differ only in case and name two nodes (casemode=distinguish)
```

Under `preserve` the two spellings meet at `term_insert()` because `ent_eq()`
is `cieq`; under `distinguish` they are two entries in one bucket and the pair
is found by scanning the chain. Under `fold` neither happens, because the
reader lower cased both cards before either token was interned, so `struct
card` gains `line_case` — the card's own text as the deck wrote it, attached by
`inp_readall()` one statement before it folds the line, stitched across
continuation lines, and read back through `INPcurrent_card`, a global
`INPpas2()` sets and clears. `card_spelling()` finds the node token as a whole
word in that text, skipping the first field, which is what keeps an instance
`R1` from supplying a spelling for a node `r1`.

The `distinguish` banner's last clause is rewritten: it named this silence and
now names what is still silent, which is a `.model`, `.subckt`, `.global` or
`.param` name written two ways.

### Acceptance criteria

1. **Met.** One line per pair, all three modes, both spellings named, the
   sentence differing per mode. The line count is asserted, not just presence:
   the guard decks' PAIR case writes the same pair on four cards and prints
   `plines = 1.000000e+00`.
2. **Met.** `cp_err`. The three guard decks capture it with `>&` and read it
   back, which they could not do from a literal `stderr`.
3. **Met.** `src/spicelib/parser/inpsymt.c` and nowhere else. No caller of
   `INPtermInsert()` gained a check.
4. **Met, and measured.** `examples/hicum2/hic2_ft.sp` is the case in the
   tree: it has a capacitor instance named `c` on one card and nodes `c` and
   `C` on others, and the report names the two nodes. The INST case of each
   guard deck asserts the silence directly.
5. **Met, and guarded from 2026-08-14.** Both sides of a pair must be claimed,
   so a node only a reference named is still `doc/codex/issues/0028`'s
   undefined-node report or decision 2's near miss and is not also reported
   here.

   It was **marked Met with nothing testing it** when this section was first
   written, which the batch's verifier found: deleting both `t_unclaimed`
   guards from `INPtermCaseCheck()` made a node collect two diagnostics and
   the whole suite stayed green — measured, with both guards deleted and the
   tree rebuilt, `tests/regression/case` passed all 118 and
   `tests/regression/casedist` all 30. Three cases were added:

   | case | deck | what it holds |
   | --- | --- | --- |
   | NMDEF | `tests/regression/casedist/node-case-collision-report.cir` | `In` defined, `in` created by a later `V()` — near miss ×1, pair ×0 |
   | NMREF | same deck | the same two names in the other interning order, so the bucket scan starts from the **defined** entry — near miss ×1, pair ×0 |
   | NMPAIR | `tests/regression/case/node-case-collision-report.cir` | two spellings of a node **no** card defines, both `V()` references — undefined-node ×1, pair ×0 |
   | NMPAIR | `tests/regression/misc/node-case-collision-report.cir` | the same shape under `fold`, reached through two `.tf` cards — undefined-node ×1, pair ×0. Added 2026-08-14 by the correction below |

   Each guard was then deleted **on its own**, rebuilt, and the four cases
   re-run, which is what shows that no case is redundant:

   | mutation | NMDEF | NMREF | NMPAIR (`case`) | NMPAIR (`misc`) |
   | --- | --- | --- | --- | --- |
   | shipped code | pass | pass | pass | pass |
   | outer `if (t->t_unclaimed) continue;` deleted | **FAIL** | pass | **FAIL** | **FAIL** |
   | inner `!u->t_unclaimed` deleted | pass | **FAIL** | pass | pass |
   | both deleted | **FAIL** | **FAIL** | **FAIL** | **FAIL** |

   Two cases rather than one under `distinguish` because the chain is
   prepended to: which of the two guards is asked depends on which spelling
   was interned first, so a deck in only one order leaves the other guard free
   to be deleted — the middle two rows are that fact. Both NMPAIRs are the
   single-entry shape, where one entry can be unclaimed and still hold a case
   twin, and they track the outer guard only, which is why the inner-guard row
   leaves them passing. Each case asserts a **count**, not a presence, and the
   count of the report that *is* owed is what proves the capture was read.

   **Correction, 2026-08-14.** The first version of this paragraph said there
   was nothing for `tests/regression/misc/` to add, on the ground that
   "`t_unclaimed` is set only by `mkvnode()`, which is only reached from a B
   source expression, and a B card carries no `line_case` — so under `fold` an
   unclaimed entry can never hold a recovered spelling and neither guard is
   reachable". **That is false**, and it was checked by running this deck:

   ```
   .options noacct rshunt=1e9
   V1 in 0 dc 0 ac 1
   R1 in 0 1k
   .noise v(MISS) V1 dec 10 1 100
   .tf v(Miss) V1
   ```

   No card defines the node, so the entry stays unclaimed; both cards are the
   deck's own text, so both carry `line_case` and both yield a spelling, and
   under `fold` and `preserve` the entry ends the parse holding
   `t_spelling = MISS` and `t_casetwin = Miss` with `t_unclaimed` still set
   (under `distinguish` they are two entries — see the paragraph after next).
   `mkvnode()` is not the
   only caller of `INPtermInsertRef()` — `INPgetValue()`
   (`src/spicelib/parser/inpgval.c`, an `IF_NODE` value) and `dot_noise()`,
   `dot_tf()`, `dot_sens()`, `dot_pss()` and `dot_hb()`
   (`src/spicelib/parser/inp2dot.c`) all call it, and a dot card is not
   preprocessor-built. On the shipped binary that deck prints one
   undefined-node line; with the outer guard deleted it prints that line **and**
   `node names 'MISS' and 'Miss' differ only in case and name one node
   (casemode=fold)`, which is the double diagnostic criterion 5 forbids.
   `tests/regression/misc` passed in full with both guards deleted — so `fold`
   was the one mode where the criterion had no guard at all.

   **That outer-guard result is `fold` and `preserve` only** (re-measured
   2026-08-14; `doc/claude/decisions/0018` decision 3 briefly generalised it to
   all three modes and now carries the three-mode table). This deck is the
   one-entry shape: under `fold` and `preserve` the two spellings share an
   entry, so `t_casetwin` holds them and the outer guard is all that stands
   between that entry and the pair line. Under `distinguish` they are two
   entries, neither carries a `t_casetwin`, and the pair is reachable only
   through the chain scan — whose own `!u->t_unclaimed` test rejects the other
   unclaimed entry. Shipped, `distinguish` prints the plain undefined-node line
   twice (not the near miss: the first scan's twin lookup is `!u->t_unclaimed`
   too); with the outer guard deleted it prints those same two lines and
   nothing more; only with both guards deleted does it add
   `… and name two nodes (casemode=distinguish)`. That is why the four-column
   table above pairs `NMDEF` and `NMREF`, both `distinguish`, with the two
   `NMPAIR`s, one `preserve` and one `fold`: no single deck holds both guards,
   and the two-entry shape and the one-entry shape are guarded separately.

   The `misc` NMPAIR case above closes it. It uses two `.tf` cards rather than
   the falsifier's `.noise` + `.tf`, because a `.noise` analysis whose output
   node does not exist aborts the run and prints a table the deck has nothing
   to say about; the mechanism under test — an unclaimed entry carrying a
   recovered spelling and a case twin — is the same either way. Only the outer
   guard is reachable under `fold`: the inner one sits in the chain scan, which
   `INPtermCaseCheck()` returns from before it starts in any mode but
   `distinguish`, which is why the `misc` column tracks the `case` column
   exactly.
6. **Met.** See the sweep below.
7. **Met.** Three decks, one per mode:
   `tests/regression/misc/node-case-collision-report.cir` (fold),
   `tests/regression/case/node-case-collision-report.cir` (preserve),
   `tests/regression/casedist/node-case-collision-report.cir` (distinguish).
   Each asserts one report, the line count, and three silences, and each also
   carries criterion 5's cases — the fold deck from 2026-08-14, when the claim
   that it owed none was falsified.

   **All three `.out` files are invisible to `git add`.** `tests/.gitignore`
   line 3 is `*.out`, and every `.out` already tracked under `tests/` was
   force-added. `git add -f` is required for
   `tests/regression/misc/node-case-collision-report.out`,
   `tests/regression/case/node-case-collision-report.out` and
   `tests/regression/casedist/node-case-collision-report.out`, or the commit
   ships three tests with no reference output and every one of them fails on
   a fresh clone.
8. **Met.**

### The sweep

Every `.cir`, `.sp`, `.net` and `.ckt` under `tests/` and `examples/` — 875
files — run in each of the three modes, and every line of the new diagnostic
accounted for.

| Mode | Lines | Files |
| --- | --- | --- |
| `fold` | 28 | 19 |
| `preserve` | 38 | 25 |
| `distinguish` | 29 | 16 |

**`fold` and `distinguish` are each a strict subset of `preserve`.** There is
no deck on which `fold` or `distinguish` reports a pair that `preserve` does
not, which is the property that matters: `preserve` is the mode where the
detection is exact by construction, so it is the oracle, and a line the oracle
does not produce is the defect shape this issue had to avoid.

Every line was read against its deck. They fall into three groups:

- **Shipped `examples/` decks that genuinely spell one node two ways** —
  `hicum2/hic2_ft.sp` and `hic2_gain.sp` (`b`/`B`, `c`/`C`; the nodes of a
  transistor card written upper case and of the sources written lower),
  `noise/baker-252-gain.cir` (`Vplus`/`vplus`, `Vplusa`/`vplusa`),
  `osdi/hicuml0/ECL-RO-5.cir` (`vee`/`VEE`),
  `p-to-n-examples/switch-oscillators.cir` and `switch-oscillators_inc.cir`
  (`VDD`/`vdd`), `soi/Inv_chain.sp` (`out0`..`out3` against `Out0`..`Out3`,
  four pairs), `tclspice/tcl-testbench3/FB14.cir` (six pairs),
  `various/FFT_Leakage.cir` (`Vfreq`/`VFreq`),
  `xspice/see/CMOSComparator/Fig27_12_see.sp` (`VDD`/`vdd` and two
  subcircuit-internal pairs).
- **This batch's own case-test decks**, which spell one node two ways on
  purpose: `tests/regression/case/node-case-alias.cir` and
  `curcasemode-effect.cir`, `tests/regression/casedist/node-case-split.cir`,
  `vector-diff-case.cir`, `vector-unlet-case.cir`, `vector-unlet-report.cir`,
  `bsource-node-case.cir`, and `tests/vbic/noise_scale_test.cir` (`VOUT` on
  the device cards, `v(vout)` on the `.NOISE` card).
- **Sub-decks other guard decks write into the build tree** — `bnr_defn.cir`,
  `bnr_fwdref.cir`, `bnr_miss.cir`, `snr_miss.cir`, `snr_only.cir`. These are
  untracked run artifacts that happened to be on disk, not committed decks.

**No committed `.out` file changed, and none should have.** The orchestrating
note expected about eighteen to gain a line; they do not, and the reason is
structural rather than lucky. `tests/bin/check.sh` redirects stdout only
(`>$testname.test`) and then drops every line containing `Warning` from both
sides of its diff, so a diagnostic on `cp_err` is invisible to the harness
twice over. That is also why the three new guard decks have to capture the
warning with `>&` and echo an upper-case token derived from it.

**Which means `make check` is not evidence about this warning either way.**
A green suite does not say the report is right and it would not have gone red
if the report were wrong, on any deck the suite runs. Everything the suite
does say about this diagnostic comes from the three guard decks, which reach
it only by re-entering the parse under a `>&` redirect. A reviewer reading
"`make check` passes" as coverage of the sweep below would be reading it as
something it cannot be.

### The nine tracked decks that now emit a line in the default mode

`casemode=fold` is what ngspice ships with and what every deck in `tests/` and
`examples/` is run under unless a `Makefile.am` says otherwise. Nine tracked
decks gained a line there. Every one of the lines is true — each deck really
does spell one node two ways — and a reviewer should still see the list rather
than infer it from a count:

| Deck | Pair(s) |
| --- | --- |
| `examples/hicum2/hic2_ft.sp` | `c`/`C`, `b`/`B` |
| `examples/hicum2/hic2_gain.sp` | `c`/`C`, `b`/`B` |
| `examples/noise/baker-252-gain.cir` | `Vplus`/`vplus`, `Vplusa`/`vplusa` |
| `examples/p-to-n-examples/switch-oscillators.cir` | `VDD`/`vdd` |
| `examples/p-to-n-examples/switch-oscillators_inc.cir` | `VDD`/`vdd` |
| `examples/soi/Inv_chain.sp` | `out0`/`Out0` … `out3`/`Out3` |
| `examples/tclspice/tcl-testbench3/FB14.cir` | `Imbat`/`imbat`, `iimage`/`Iimage`, `vmbatm`/`Vmbatm`, `vmbat`/`Vmbat` |
| `examples/xspice/see/CMOSComparator/Fig27_12_see.sp` | `VDD`/`vdd` |
| `tests/vbic/noise_scale_test.cir` | `VOUT`/`vout` |

`tests/vbic/noise_scale_test.cir` is the only one of the nine that `make check`
runs, and it passes — because of the filter above, not because the line is
absent. The other `fold` lines in the sweep's 28 come from
`tests/regression/case` and `tests/regression/casedist` decks that spell a node
two ways on purpose, and from untracked sub-decks other guard decks write into
the build tree.

### One false positive, found by the sweep and fixed

The first sweep produced exactly one line that `fold` reported and `preserve`
did not: `examples/xspice/verilator/adc.cir`, `Start`/`start`. The pair is real
in that deck, but the evidence was not. The XSPICE auto-bridge builds a deck of
bridge cards and re-enters `inp_readall()` to parse it (`create_deck()`,
`src/xspice/evt/evtcheck_nodes.c`), so those cards were given a `line_case`
like any others — and a bridge card spells its node the way the node is already
interned, which under `fold` is lower case. The offending line was
`auto_adc4 [ start ] [ start ] auto_adc`: a card ngspice wrote, standing as
evidence that the user had written a second spelling. `Evtcheck_nodes()` now
clears `line_case` on the deck it generates. This is
`doc/claude/decisions/0001-distinguish.md` decision 3's rule — exact about
names the deck chose, indifferent to names ngspice constructs — applied to a
spelling rather than to an identity.

### Stated gaps

Under `fold` a spelling can only come from a card the parser reads as the deck
wrote it. Four shapes fall outside that, all of them silences and all of them
measured; `doc/claude/decisions/0018-node-name-collision-report.md` decision 3
carries the detail.

1. A subcircuit body. Reported under `preserve` and `distinguish` as `x1.Mid`
   and `x1.mid`, not under `fold`. Guarded in both directions by the three
   decks' BODY case.
2. An `X` card's actual, which the expander consumes.
3. **A card the preprocessor rebuilds: every B source, and every `E`, `G`,
   `R`, `C` or `L` card whose value is an expression.**
   **Corrected 2026-08-14** — this item used to say "a spelling inside an
   arithmetic expression", and blamed `card_word_boundary()` for leaving `+`,
   `-`, `*` and `/` inside words. That was wrong twice. `(` and `)` **are**
   boundaries, so `V(VFreq)` yields `VFreq` — the function, compiled out of
   `inpsymt.c` and handed the offending line of `FFT_Leakage.cir`, returns it
   — and the silence survives on `B1 mid 0 I = 1m`, where the second spelling
   is a plain node field and no expression is involved. The cause is
   `inp_bsource_compat()` (`src/frontend/inpcom.c`): it comments every B card
   out and inserts a **replacement** built by `insert_new_line()`, which has
   no `line_case`. So no node named anywhere on a B card can supply a
   spelling. `inp_compat()` does the same to an expression-valued `E`, `G`,
   `R`, `C` or `L`. **236 B cards in 53 of the 875 decks.**
4. An XSPICE `a` card whose bracketed node list has no spaces — `[` and `]`
   are not boundaries either. No deck in the corpus is affected; it is listed
   so that the boundary set's real cost is on the record next to the sentence
   that used to misstate it.

**Ten** `preserve` reports have no `fold` twin — the earlier "nine" was a
miscount — and the ten divide cleanly among the first three gaps:

| Gap | Lines | Decks |
| --- | --- | --- |
| 1, subcircuit body | 4 | `Fig27_12_see.sp` (`X1.Vr`, `X1.Vsur`), `tests/regression/case/global-node-case.cir`, `name-lookup-case.cir` |
| 2, `X` card actual | 3 | `osdi/hicuml0/ECL-RO-5.cir` (`vee`/`VEE`), `FB14.cir` (`Vcc`, `Vimage`) |
| 3, rebuilt card | 3 | `various/FFT_Leakage.cir`, `tests/regression/casedist/bsource-node-case.cir`, `bnr_miss.cir` |

Nine `preserve` reports have no `distinguish` twin, because under
`distinguish` one of the two spellings is a node no card defines: six of those
nine get decision 2's near-miss report instead (`Warning: no node named 'in';
'In' differs only in case`) and the other three are a subcircuit formal pin
against its body, which under `distinguish` becomes a scoped internal node
with a different name — the `distinguish` half of `doc/codex/issues/0049`, and
a different diagnostic's business.

### Open question for the owner: one line per instantiation

Under `preserve` and `distinguish` the report is once per colliding **pair**,
and after subcircuit expansion each instantiation of a body carries its own
pair. One two-way spelling inside one `.subckt` body is therefore one line per
instance:

```
Warning: node names 'X13.sig3' and 'X13.SIG3' differ only in case and name one node (casemode=preserve)
Warning: node names 'X142.sig3' and 'X142.SIG3' differ only in case and name one node (casemode=preserve)
Warning: node names 'X18.sig3' and 'X18.SIG3' differ only in case and name one node (casemode=preserve)
...
```

**Measured, on shipped `examples/` decks.** Spelling the `nand2` body's one
internal node `sig3` as `SIG3` on a single card — one character, one line of
one `.subckt` body — and running under `-D casemode=preserve`:

| deck | `nand2` instances | warning lines |
| --- | --- | --- |
| `examples/klu/Circuits/85/c1355/c1355.net` | 416 | **416** |
| `examples/klu/Circuits/85/c5315/c5315.net` | 454 | **454** |
| `examples/klu/Circuits/85/c7552/c7552.net` | 1028 | **1028** |

One line per instantiation, exactly, each naming a different scoped pair —
`X3344.sig3`/`X3344.SIG3`, `X3393.sig3`/`X3393.SIG3`, and so on. The last row
is the largest multi-instance deck in `examples/` that this tree can parse;
the corpus goes one step further still, because the same netlist instantiates
`not1` 1410 times, as do the three PDK variants of it under
`examples/SkywaterOpenSourcePDK/c7552_ann_skywater.net`,
`examples/IHPOpenSourcePDK/c7552_ann_IHP.net` and
`examples/osdi/psp103/c7552_ann_psp.net` (those three need a PDK this tree
does not carry, so their number is the instance count rather than a measured
line count). The measurement is reproducible: copy the deck, change the one
card, run under `-D casemode=preserve`. All 1028 lines are written during the
parse, before any analysis starts.

A body pin does **not** do this. `not1`'s `z` is a formal pin, and spelling it
`Z` on one body card is silent in every mode, because the expander substitutes
the caller's actual for both spellings — the FORMAL case the three guard decks
already assert. It is specifically a body's *internal* node, the thing the
expander scopes rather than substitutes, that multiplies.

This is not a defect in the implementation — every line is true, the pairs
really are distinct nodes, and under `distinguish` they really are 416
distinct two-node splits. It is a question about what the diagnostic is *for*,
and the batch's decision record does not answer it: decision 1 of `0018` costs
"one line per pair" against a deck that deliberately uses case-variant nets,
and never considers that one *mistake* can be many pairs.

Three options, with what each costs:

1. **Leave it as is.** Cost: a single-character typo in a library cell floods
   the log of every netlist that instantiates it, and the flood is worst
   exactly where it is least readable — a 7000-instance ISCAS netlist. The
   user cannot suppress it (`0018` decision 1 rejected a variable) and cannot
   tell from the log whether they have one problem or 416. Benefit: nothing
   changes, no new state, and the report keeps its current property that every
   line names a node the user can address by name.
2. **Report once per `.subckt` definition** — collapse `x1.Mid`/`x1.mid`,
   `x2.Mid`/`x2.mid` … to one line naming the body. Cost: the symbol table
   does not know what a subcircuit is. `term_insert()` sees `x1.mid`, a flat
   name the expander built, and recovering "this is `mid` inside `div`" means
   either keeping the mapping the expander threw away (a change to
   `src/frontend/subckt.c`, the same file gaps 1 and 2 need) or stripping a
   scope prefix by string surgery, which is guesswork on a name a deck is
   allowed to write with dots in it. It also loses the top-level case: a pair
   with no scope has no definition to name. Benefit: one mistake, one line,
   and the line names the thing the user must edit.
3. **Report once per distinct pair after stripping the scope** — keep the
   flat-name detection, deduplicate on the pair of *unscoped* spellings, and
   say how many instances carried it. Cost: the same string surgery on the
   scope prefix, but only for grouping, so a wrong split degrades to a
   duplicate line rather than to a wrong name; plus a per-parse set to
   deduplicate against, which is new state to reset across `ngSpice_Reset`
   (`src/sharedspice.c`). It also stops naming any single addressable node,
   which is a real loss: `v(x13.sig3)` is a thing the user can print, and
   `sig3` on its own is not. Benefit: one line per authoring mistake, with the
   count as evidence of scale, and no dependency on the expander.

A fourth possibility — cap the report at N lines per pair-shape and say "…and
412 more" — is a variant of 3 with the same scope-stripping problem and is not
listed separately.

The measurement above is what the decision needs and did not have. Which of
the three is right is the owner's call, and `0018` decision 1 should be
amended with the answer rather than left reading as though the cost had been
counted.

### Not fixed here

- `.model`, `.subckt`, `.global` and `.param` names spelled two ways. Decision
  1 of `0018`, and the `distinguish` banner names them.
- XSPICE event nodes. `EVTnode_insert()` is the same shape in the other table
  and would have to be reconciled with `EVTnode_case_check()`.
- Closing gaps 1 and 2 above, which means giving `src/frontend/subckt.c` a
  pre-fold text of its own.
- Closing gap 3, which means giving `insert_new_line()` a way to inherit the
  pre-fold text of the card it replaces, and showing that no rewrite inherits
  the text of a card it is not a rewrite of.
- Closing gap 4, which is two characters in `card_word_boundary()` and no
  measured line asking for them.
- The one-line-per-instantiation question above. It is open, and the answer
  belongs in `0018` decision 1.
