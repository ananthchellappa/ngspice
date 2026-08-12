# Decision 0012 — the subcircuit formal pin, `doc/codex/issues/0049`

## Status

Accepted, 2026-08-11, branch `ver_50`. Fixes `doc/codex/issues/0049`. It
applies decision 3 of `doc/claude/decisions/0001-distinguish.md` at a Class A
site that record's enumeration missed, and decision 2 of the same record at
the resolution beside it.

It clears the **subcircuit formal pin** half of the one bullet on
`doc/claude/specs/case-sensitive-identifiers.md`'s acceptance list that was
still marked untested. It does **not** close that bullet: the same sentence
also says the rawfile is untested, and it still is — no deck asserts that two
case-variant vectors are separately present in a `-r` rawfile under
`distinguish`. That is stated here rather than left to be inferred, because
the session that produced this record was asked to close the list and closed
one of its two halves.

## Context

Every `src/frontend/subckt.c` line number in this record is the **parent
commit's**, `b0ae03bc1`, because that is the tree the measurements are of. The
change moves them: after it, `eq_substr()` is at `:1654`, `eq_substr_ci()` at
`:1666`, `eq_substr_id()` at `:1683`, `report_pin_case_miss()` at `:1708`,
`gettrans()`'s fixed comparison at `:1756` and the `.subckt`-name comparison
at `:1787`.

`gettrans()` (`src/frontend/subckt.c`) resolves a node name inside a `.subckt`
body against the formal pin list. Measured at `b0ae03bc1`:

```c
    for (i = 0; table[i].t_old; i++)
        if (eq_substr(name, name_end, table[i].t_old)) {   /* :1705 */
```

Its sibling twenty-nine lines below, matching a `.subckt` **name**, uses the
case-aware predicate `0001` decision 3 introduced for exactly this:

```c
                if (eq_substr_id(xname, xname_e, subs->su_name))  /* :1734 */
```

A miss is not an error. `translate_node_name()` falls through to prefixing the
name with the instance scope, so the body's `a` becomes the subcircuit-local
net `x1.a` and the subcircuit is not wired to the outside. The only sign is
`Warning from checkvalid: vector b is not available or has zero length`, which
names neither pins nor case — and which `tests/bin/check.sh`'s `egrep -v`
filter removes from both sides of every comparison it makes.

(`0049` says the scoped name is `x1:in`. It is `x1.in`: `translate_node_name()`
writes `bxx_putc(buffer, '.')`. The issue's Impact paragraph is corrected with
its Resolution.)

Measured at `b0ae03bc1`, three decks differing only in which side is upper
case:

| deck | `fold` | `preserve` | `distinguish` |
| --- | --- | --- | --- |
| `.subckt` UPPER, body lower | 1.5 V | **no `v(b)`** | no `v(b)` |
| `.subckt` lower, body UPPER | 1.5 V | **no `v(b)`** | no `v(b)` |
| both lower | 1.5 V | 1.5 V | 1.5 V |

---

## Decision 1 — this is a `preserve` fix, and the reduction says so

**Decided: the edit is `eq_substr` → `eq_substr_id` and the mode it moves is
`preserve`. State that before writing it, because the series heading says
`distinguish`.**

`eq_substr_id()` is `eq_substr()` whenever `inp_case_exact_ids()` is true, and
`inp_case_exact_ids()` (`src/frontend/inpcom.c`) returns
`mode != NG_CASE_PRESERVE`. So `fold` and `distinguish` both take the
byte-exact branch the site already had, and `preserve` is the only mode the
edit can reach. That is a reduction of the predicate, not an argument about
what the reader lowercased, and it is the same one `0001` decision 3 makes for
every Class A row.

This matters because the parent series is called `distinguish` and this issue
is on a `distinguish` acceptance list. `0001` decision 3's last two Class A
rows are the precedent and say it in so many words: Phase 3 gates 2 and 4 were
filed under `distinguish`, were **bare comparators** rather than sites asking
`inp_case_folding()`, and turned out to be `preserve` defects — under
`preserve` two spellings are one identifier, so `strcmp` refused to bridge and
refused to intern a net that was one net. This is the third instance of that
shape and the first outside XSPICE.

**Why `0001` decision 3's enumeration missed it.** That list was built by
re-reading the sites that already asked `inp_case_folding()`, because the
question it was answering was "which of these are asking about identity?". A
site that asked *nothing* could not appear in it. `gettrans()` asked nothing;
so did `evttermi.c:304` and `evtcheck_nodes.c:720`. Three of the tree's four
worst identity defects were bare comparators, and the enumeration's method
could not see any of them.

That is the measured reason `doc/codex/issues/0048`'s lint exists, and it is
worth recording that the lint **did** see this site.
`tests/lint/identity.baseline` has carried
`src/frontend/subckt.c eq_substr(name, name_end, table[i].t_old)` since
`0011` landed. The lint froze it and had no opinion about it, which is exactly
what `0011` decision 2 says a report claims: not that a site is a bug, but
that it is unclassified. The classification is what was missing, and this
record is it.

## Decision 2 — the `distinguish` half gets the diagnostic, at the miss, in `gettrans()`

**Decided: implement decision 2 of `0001-distinguish.md` here rather than
defer it. `report_pin_case_miss()` writes to `cp_err` when the formal-pin
lookup misses and a pin differing only in case is in the table, gated on
`inp_case_mode() == NG_CASE_DISTINGUISH`.**

```
Warning: no subcircuit pin named 'b'; 'B' differs only in case (casemode=distinguish)
```

The site is a **resolution**: `gettrans()` looks a body token up in the pin
list, and on a miss `translate_node_name()` creates a scoped name for it. That
is `mkvnode()`'s shape — resolve, and on a miss create — which is Phase 3 gate
1 and `doc/claude/decisions/0002-deferred-node-resolution-check.md`. Under
`distinguish` the *result* is defensible, because two spellings are two names
and the body's `b` really is a different net from the pin `B`; only the
silence is wrong.

**Rejected: defer it to the parser's end-of-parse scan.** This was the
proposal, and it is not merely more work — it cannot be made to work. By the
time `INPtermCaseCheck()` (`src/spicelib/parser/inpsymt.c`) runs, the body's
`b` is the node `x1.b`; it is *defined* by the card that mentions it, so
`t_unclaimed` is false, and no case variant of `x1.b` exists anywhere in the
table. The scan is looking for a name that no card defines beside a spelling
that some card does, and after expansion this defect presents as neither. The
near miss is visible only while `table[]` is, and `table[]` lives inside one
call to `translate()`.

**Rejected: silence, recorded.** The argument for it is that the `distinguish`
answer is already right, so nothing is being printed wrongly. It loses to
decision 4 below: the `.subckt` *name* lookup on the same card fails **loudly**
under `distinguish`, with `Error: unknown subckt`, and the run stops. A deck
whose instance card names an unknown subcircuit is told; a deck whose body
misspells that subcircuit's pin is not, and gets a disconnected subcircuit and
a completed run. Two lookups twenty-nine lines apart in one file should not
answer the same question two ways.

**The silence this keeps, which is the half that makes the mode usable.** A
miss with *no* case variant in the pin list is an ordinary internal node.
Every internal node of every subcircuit in every deck takes that path, so a
report there would fire on correct decks and on almost nothing else. The
near-miss test is what separates them, and it is the same test decision 2 of
`0001` names: a resolution that fails *beside a case variant*.

### Decision 2a — reported once per formal pin per expansion

**Decided: a `t_reported` bit on `struct tab`, set when a pin's near miss is
reported.**

`gettrans()` runs once per node token, not once per pin. Without the bit, a
body that spells one pin wrongly on ten cards prints ten identical lines, and
`doc/codex/issues/0046` is already the tree's record of a near-miss report
arriving more than once. Adding a third instance of that shape to fix a
silence is a poor trade.

The bit rather than a set of (pin, body spelling) pairs, and the cost stated
plainly: a pin spelled two wrong ways in one body — `IN` written as both `in`
and `In` — reports the first and not the second, so the second surfaces only
after the first is fixed. That is a ratchet, not a loss.

**Not deduplicated across instances.** `table[]` is rebuilt by `settrans()` per
`translate()` call, so a hundred instances of a broken subcircuit warn a
hundred times. Left alone deliberately: each expansion is a separate
resolution against a separate table, and suppressing across them needs state
that outlives the table, which is `0002` decision 1's argument about
parse-scoped bits run backwards.

### Decision 2b — the canonical sentence, with the noun that names the side that failed

**Decided: the series form, and no instance name in it.**

`scname` is one frame up, in `translate_node_name()`, and threading it down
would have let the message say which instance. It is not taken. Every
diagnostic in this series is one sentence of one shape and `0003` decision 4
settled that the noun is the only thing that varies — `no node named`,
`no analog node named`, `no event node named`, and now
`no subcircuit pin named`. A deck asserting on the shared tail would pass on
the wrong warning, which is why the guarding deck asserts the noun; adding a
clause here would make this the one message whose shape a reader cannot
predict.

Cost, stated plainly: in a large hierarchical deck the warning names the pin
and its twin but not the instance, so the reader greps the `.subckt` card
rather than the invocation. The pin name and its case variant are enough to
find that card, which is where the fix goes anyway.

### Decision 2c — the mode guard is belt and braces, in both other modes

- Under **preserve** the condition is unreachable. `eq_substr_id()` is the
  case-insensitive arm there, so a name that matches when case is ignored has
  already matched and returned from the loop above.
- Under **fold** the reader has lowercased every card. The residual that made
  the parser's guard a real decision — names ngspice constructs for itself,
  `q1#collCX` — does reach `translate()` on a nested subcircuit's second pass
  as `x1.a`, but every such name is built from a name the reader had already
  lowercased, so no two of them can differ only in case.

Same conclusion as `0003` decision 4, reached at a third site.

## Decision 3 — `settrans()` needs no edit, and the reason is its shape

**Decided: unchanged. It cannot put two spellings of one pin into two rows.**

`settrans()` (`:1621`) is positional. It calls `gettok(&formal)` and
`gettok(&actual)` in lockstep and writes one row per token position; nothing
in it compares a formal against a formal, so no comparison decides which row a
pin lands in. Two spellings of one pin occupy two rows only if the `.subckt`
card itself writes the same pin twice, and that is two pins positionally in
every mode — under `fold` the reader lowercases both and the two rows become
byte identical, which is a malformed deck's problem and not a case one.

Its one comparison is `ng_ideq(table[i].t_new, subname)` at `:1636`. That is
the row that ends the loop when the formals run out and the remaining actual
is the subcircuit name, and it is already the case-aware predicate. So the
table was right and only the lookup was wrong, which is why this is a one-line
fix and not a two-line one.

## Decision 4 — the `.subckt` name row is symmetric, and its miss is loud

**Finding, not a decision: `:1734` is already correct in both directions, and
no issue is filed.**

Measured at `b0ae03bc1`, a divider whose `.subckt` name is spelled one way on
the definition and the other on the invocation:

| deck | `fold` | `preserve` | `distinguish` |
| --- | --- | --- | --- |
| `.subckt DIV`, `x1 a b div` | 1.5 V | 1.5 V | `Error: unknown subckt` |
| `.subckt div`, `x1 a b DIV` | 1.5 V | 1.5 V | `Error: unknown subckt` |

`preserve` matches either way round, which is what `eq_substr_id()` at `:1734`
and `ng_ideq()` at `:630` are for, and `distinguish` refuses — loudly, at
`subckt.c:391`, and the run stops with `Error: incomplete or empty netlist`.

That contrast is the substance of decision 2. The same instance card carries
two lookups: its trailing name token, which fails loudly, and, one expansion
later, its subcircuit's pins, which failed silently and completed the run with
a disconnected subcircuit. Fixing the pin comparator makes the two agree under
`preserve`; the diagnostic makes them agree under `distinguish`.

## Decision 5 — the lint's baseline entry left with the comparison

**Decided: delete `identity.baseline`'s line 181 by hand. No new entry, and no
`case-lint:` comment.**

`tests/lint/identity.baseline` had three `src/frontend/subckt.c` records: the
definition line of `eq_substr()`, its call from `eq_substr_id()`, and
`eq_substr(name, name_end, table[i].t_old)` — the call this change replaces.
The third leaves the tree with the fix, so `make check` fails with

```
In .../tests/lint/identity.baseline but no longer in the tree:

  src/frontend/subckt.c	eq_substr(name, name_end, table[i].t_old)
```

until the line is deleted, which is `0011` decision 1's second failure
direction working as designed and the first time it has fired on a real
change. The baseline was not regenerated; there is no target that would.

Nothing is added. `eq_substr_id` and the new `eq_substr_ci` are not on
`identity.lint`'s `comparators` list, so neither the changed call site nor the
new helper's body is a site the scanner sees — the scanner matches comparator
names on a word boundary, which `identity.baseline` already demonstrated by
never having listed `eq_substr_id(xname, xname_e, subs->su_name)` at `:1734`.
That is `0011` decision 4's first way out, "call a helper", and it is the one
that leaves the tree with fewer unclassified comparisons rather than more:
274 → 273.

`eq_substr_ci()` is a factoring, not a new predicate. `eq_substr_id()`'s
`tolower_c` loop moved into it verbatim so that the near-miss test in decision
2 has a case-insensitive comparator to call; under `distinguish`
`eq_substr_id()` is exact, so it cannot answer "would this have matched if
case were ignored?".

**The baseline's own header was stale and is corrected.** It read "measured
over these 274 records, 95 are identity comparisons", which was a claim about
*this file* and became false the moment a record left it — the failure mode
`0011` decision 1 spends its length guarding the other half of. It now says
the count is the measurement's, at `6e2dca123`, and carries a short list of
entries removed since with the issue that removed each. That list is the
provenance a reader needs to reconcile 273 against 274 without re-running the
scanner, and this is the first entry on it: `0011` decision 1 predicted an
entry would leave only by its comparison leaving, and this is that happening.

## Evidence

### RED, at the parent commit

`tests/regression/case/subckt-formal-pin-case.cir` under
`-D casemode=preserve`, at `b0ae03bc1`:

```
Error: no such vector 2
Error: no such vector 4
Error: no such vector 6
```

All three voltages **absent**, not merely different: the top-level nodes are
not vectors at all, because the subcircuit bodies were wired to `x1.a`,
`x1.b`, `x2.A`, `x2.B` and so on instead. `check.sh`'s filter drops every
`Error` line, so what the harness sees is an empty `.test` file against a
three-line `.out`. The consistently-spelled twin
`subckt-formal-pin-case-lower.cir` prints `3.0`, `6.0` and `9.0` at the same
commit, which is what makes the assertion about the case of the pins and not
about the deck.

The third witness is a **nested** subcircuit with the pins of both levels
spelled in the other case by the body that uses them. It was added after the
first two passed, on the adversarial reading that `translate()` runs once per
expansion and the inner one resolves against a `table[]` whose `t_new` column
holds names the outer expansion has already rewritten — a different path, not
a second dose of the same one. It is RED at the parent with the same shape and
GREEN after, and `fold` gives `9.0` for it too.

`tests/regression/casedist/subckt-formal-pin-case.cir` under
`-D casemode=distinguish`, at `b0ae03bc1`: `PIN-NEARMISS-SILENT`, where the
`.out` says `PIN-NEARMISS-REPORTED`. Its `v(2) = 2.000000e+00` and
`v(x1.b) = 0.000000e+00` half is **not** RED and is not meant to be — it is a
guard that the fix leaves `distinguish` alone, which decision 1's reduction
says it must.

Its PART 1 circuit is also the shortest statement of the whole change,
because all three modes disagree about it and the fix is what makes two of
them agree:

| | `fold` | `preserve` | `distinguish` |
| --- | --- | --- | --- |
| at `b0ae03bc1` | 1.714286 | **2.0** | 2.0 |
| after | 1.714286 | **1.714286** | 2.0 |

`preserve` moves onto `fold`'s answer, which is what "two spellings of one pin
are one pin" means, and `distinguish` does not move, which is what "two
spellings are two names" means. `x1.b` exists as a vector of its own only in
the column that did not move.

### GREEN, after

Both decks pass. The diagnostic, from the capture the deck reads back:

```
Warning: no subcircuit pin named 'b'; 'B' differs only in case (casemode=distinguish)
```

once, not twice, although `rb b 0 3k`'s `b` is one of three tokens the body
resolves — decision 2a's bit.

### The three silences

`PIN-INTERNAL-SILENT` is the one that matters: `.subckt div A B` with a body
node `m` that is simply internal reports nothing. `PIN-EXACT-SILENT` covers a
body that spells every pin exactly. Both carry a `CAPTURE-READ` token taken
from the sub-deck's own `Circuit:` line, so neither can pass by never having
opened the capture. `PIN-NEARMISS-PIN-NOUN-ONLY` asserts that the parser's
noun `no node named` is *not* also in the capture, so the two diagnostics
cannot be confused for one another.

### The dedupe bit, and the three shapes it was checked against

Decision 2a's claim is that the bit collapses per pin and not per token, so it
was run rather than asserted. `.subckt div IN OUT` with a body of
`r1 in out 1k` / `r2 out 0 1k` — `out` on two cards, `in` on one:

```
Warning: no subcircuit pin named 'in'; 'IN' differs only in case (casemode=distinguish)
Warning: no subcircuit pin named 'out'; 'OUT' differs only in case (casemode=distinguish)
```

Two lines for three tokens. The same subcircuit instantiated twice gives four
lines, which is decision 2a's "not deduplicated across instances" measured
rather than claimed.

Three false-positive shapes were checked and all are silent under
`distinguish`:

- A `.subckt` with a `params:` tail. `settrans()` would put a row per token if
  the tail reached it, and a `params:` row is a pin that no body can spell, so
  it was worth confirming that nothing fires. Nothing does, and the deck's
  behaviour is byte identical to the parent's in both modes — its
  `Undefined parameter [rtop]` under `distinguish` is
  `doc/codex/issues/0015`'s subject and predates this change.
- A nested subcircuit whose *outer* pins are spelled exactly and whose inner
  pins are not: only the inner pair is reported.
- An ordinary internal node, which is the deck's `PIN-INTERNAL-SILENT` case.

### `make check`

**279 PASS / 0 FAIL**, against the recorded bar of 276 / 0 at `b0ae03bc1`.
The three that appeared are the three decks added here; nothing was lost, and
the count was compared per test rather than by the total. The lint's own two
tests are in it and pass, at 273 comparisons.

One line in the log reads `ERROR: (internal)  tried to destroy non-existent
graph`. It is not a test verdict — automake's are the `PASS:`/`FAIL:` lines
and there are no `FAIL:` — it is `vector-probe-report.cir`'s `hardcopy` on
`stderr`, and it is present at the parent commit under the parent binary. It
is noted because a grep for `ERROR` finds it and a reader should not have to
re-derive that it is pre-existing.

### The sweep, compared per deck rather than by its total

`case_differential_sweep.py --jobs 8 --timeout 90`, run twice over a frozen
tree, once against the snapshot taken before any of this and once against the
rebuilt binary:

```
before   318 decks: DIFF=71, OK=244, SKIP=3     PARSE-FAIL=0  NUM-DIFF=0
after    318 decks: DIFF=69, OK=246, SKIP=3     PARSE-FAIL=0  NUM-DIFF=0
```

318 rather than the recorded 315 because this change adds three decks. The
totals moved in the direction this fix should move them — *fewer* DIFFs — and
the per-deck comparison says exactly which:

```
DIFF only before:  tests/regression/case/subckt-formal-pin-case.cir
                   tests/regression/case/alter-rebin-case.cir
DIFF only after:   (none)
```

The first is this change's own RED deck: under the sweep's `preserve` run it
disagreed with the stock run and now agrees, which is the fix seen by a second
tool. The second is one of the four `alter-rebin` decks the series has
recorded as flapping — the family gives DIFF=4 in the before run and 3 in the
after, inside its own recorded ±2. **No deck newly differs**, which is the
number this change could have broken.

`tests/regression/casedist/subckt-formal-pin-case.cir` is DIFF in both runs
and it is worth saying why, since it did not clear. Its report has two rows
before and one after: the `preserve` row — `v(2) = 2.000000e+00` and an
`x1.b`, against the stock run's `1.714286e+00` — is gone, which is again the
fix. What remains is the `preserve-UPPER` row, and that is the collateral the
deck's own header describes: the sweep's uppercaser rewrites its `echo` lines
and leaves its `source` lines alone, so the copy writes `SFP_NEAR.CIR` and
sources `sfp_near.cir`. `bsource-node-case-report.cir`,
`sens-node-case-report.cir` and the two XSPICE report decks are DIFF for the
same reason and were before this change.

`PARSE-FAIL` and `NUM-DIFF` are 0 in both runs, which is the pair that must
not move.

### The differ, all three modes

`deck_output_differ.py --before <the snapshot> --after build-ver_50/src/ngspice
--jobs 8 --timeout 200`, which compares two binaries on **both** streams and
is therefore the only tool here that can see a new diagnostic:

```
decks: 318  mode: stock          moved=2   alter-rebin-case{,-lower}.cir
decks: 318  mode: preserve       moved=5   3 alter-rebin + this change's two decks
decks: 318  mode: distinguish    moved=4   2 alter-rebin + this change's two decks
```

Eleven MOVED lines across three modes. Seven name one of the four
`alter-rebin` decks, which is the no-change baseline this differ has always
had. The other four are the two decks added here, and what they say is the
decomposition of this change into its two commits:

- **`fold`: nothing outside the flapper family moves at all.** Decision 1's
  reduction says the comparator edit cannot reach `fold`, and this is that
  measured over 318 decks.
- **`preserve`: the only non-flapper moves are the two new decks, and both
  move on `stdout`.** `case/subckt-formal-pin-case.cir` gains its three
  voltages and loses the `in term: v(2)` / `v(4)` lines from `stderr`;
  `casedist/subckt-formal-pin-case.cir` goes from `2.0` to `1.714286` and
  gains a `checkvalid` warning, because `x1.b` has stopped existing. That is
  the fix, twice.
- **`distinguish`: the only non-flapper moves are the same two decks, and the
  numbers do not move.** `case/subckt-formal-pin-case.cir` moves on `stderr`
  **only** — the new warning, and no `stdout` at all, which is the sharpest
  available statement that this mode's arithmetic is untouched.
  `casedist/subckt-formal-pin-case.cir` moves on `stdout` too, and the moving
  line is its own `PIN-NEARMISS-SILENT` → `PIN-NEARMISS-REPORTED` token,
  which is the RED closing.

The differ truncates its stderr list to three entries; run directly, the
`case` deck emits **eight** warnings under `distinguish`, one per formal pin
of each of its four subcircuits, each exactly once. That was checked rather
than inferred from the three the report shows, because a report that lists
three where eight are expected is what an over-eager dedupe would also look
like.

`--timeout 200` rather than the default, for `tests/mesa/mesa12.cir`'s ~143 s,
which at 120 reports as `rc TIMEOUT -> 1` — a MOVED line that is not a
difference.

**A false start worth recording, because it produced a plausible empty
result.** The first measurement run passed `--ngspice build-ver_50/src/ngspice`
as a *relative* path. Both scripts `chdir` per deck — the sweep into a
tempdir, the differ into the build-tree counterpart of the deck's directory —
so the path resolved against the wrong directory and the run died with
`FileNotFoundError`. It was caught because the output file was 1714 bytes
instead of tens of kilobytes. Every binary path in a measurement of this tree
has to be absolute.

## What this decision does not decide

1. **The global-node near miss — `doc/codex/issues/0050`, filed by this work.**
   `gettrans()`'s probe above the pin loop folds its key through `glo_key()`,
   which under `distinguish` does not fold at all — correctly, since two
   spellings are two globals there. A body reference that misses a `.global`
   by case alone therefore falls through to the pin loop and then to scoping,
   exactly as a pin miss did.

   It was measured rather than assumed, and it is worse than "says nothing":
   a subcircuit whose body pulls a resistor up to `vdd` beside a
   `.global VDD` prints `v = 2.498751e+00` under `fold` and under `preserve`
   and `0.000000e+00` under `distinguish`, with an empty `stderr`. That is a
   silently wrong number, the thing this whole series exists to remove.

   It is not fixed here because the fix is not the same size as the fix this
   record is about. `report_pin_case_miss()` re-scans `table[]`, a small array
   the function already holds; `glonodes` is a hash whose keys are unfolded
   under `distinguish`, so the equivalent question needs a walk of the whole
   table or a second folded index beside it, and `0001` decision 3's closing
   section forbids the cheap answer of installing a case-insensitive hash
   function. `0050` carries the deck and the criteria.
2. **The rawfile half of the spec's acceptance bullet.** Two case-variant
   vectors being separately present in a `-r` rawfile under `distinguish` is
   the other thing that bullet marked untested, and it is untested still. It
   is not this issue: `0049` is about the netlist, and the rawfile half wants
   a deck of its own that reads a `-r` file back.
3. **`set_case_mode()`'s experimental-mode warning is left as it is, and this
   change makes its enumeration incomplete.** It says a vector, a B source
   `V()` reference or an XSPICE node whose resolution misses by case is
   reported; a subcircuit formal pin now is too, and the sentence does not say
   so. That is deliberate and it is a trade rather than an oversight. The
   string is on `stderr` and is printed by **every** `distinguish` run, so
   editing it moves the `stderr` of every deck in
   `tests/regression/casedist/` and `tests/xspice/casedist/` in
   `deck_output_differ.py --mode distinguish` — which is the one tool that can
   see this change's diagnostic, and whose value here is that it reports
   exactly four moved decks. Trading that evidence for one more noun in a
   sentence that already runs to five lines is the wrong way round. The
   clause the word `experimental` actually rests on is the second half, "a
   deck that spells one net two ways becomes a deck with two nets and nothing
   says so", and this change does not touch it. The next session that edits
   that string for another reason should add the noun.

4. **`doc/codex/issues/0046`**, the duplicate near-miss report. Decision 2a
   avoids adding to it within one expansion and deliberately does not solve it
   across expansions.
5. **`doc/codex/issues/0047`, `0048`, `0043`, `0042`, `0041`, `0039`, `0038`,
   `0035`, `0033`, `0031`, `0021`**, and `com_let()`'s `plainlet` leading
   space per `0009` deferral 4. Enumerated in `0049`'s session prompt as out
   of scope and untouched.
