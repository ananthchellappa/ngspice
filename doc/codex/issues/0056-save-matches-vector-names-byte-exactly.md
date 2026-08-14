# Issue: `.save` Matches Vector Names Byte-Exactly In Every Case Mode

## Status

Closed in the working tree, with the commit sequencing `doc/codex/issues/0057`
criterion 8 asks for still owed: this issue has to move `make check` on its own,
which cannot be shown until it and `0057` are committed as two commits in that
order. `0057`'s Resolution 8 says what the two commits hold.

Criterion 2 and Resolution 2 were amended on 2026-08-13: the one comparison
this issue deliberately left byte exact had no deck, and a mutation of it
passed the whole suite. `tests/regression/case/plot-scale-name-twin.cir` is
that deck and belongs to this issue's commit.

**Every transcript in Impact and Root Cause below is `720c8743a`'s and is
historical.** The Status line already pinned them; this paragraph says it again
at the top because two fixes have landed on top and a reader running the decks
at HEAD gets different output. This issue's own fix makes every `preserve`
transcript of a mis-cased `.save` resolve and succeed; `0057`'s makes the
failure that survives under `distinguish` name its token; and
`doc/codex/issues/0059`'s makes `write` refuse rather than emit the constants
plot, which changes the "it still writes a plausible raw" transcript. Each of
those places carries a one-line note. The Resolution's numbers were
re-measured on 2026-08-13 and are HEAD's.

Found from outside the tree, which is new for this series: a client
program — xschem, adding case-preserving signal names to a schematic editor
that generates decks and reads the raw back — measured nine findings against
this build, and `doc/claude/feedback/ngspice_upstream/FINDINGS.md` finding 2 is
this defect. Every number below was **re-measured** at `720c8743a` against
`build-ver_50/src/ngspice` (`ngspice-46+`, build stamp
`Wed Aug 12 19:28:37 UTC 2026`) with `/usr/local/bin/ngspice` (`ngspice-46`, no
`casemode`) as the baseline; no number is copied from that report. One
*argument* is copied: the migration-hazard paragraph at the end of Impact is
FINDINGS' own (`FINDINGS.md:124-128`), kept rather than re-derived, and
attributed where it appears.

Every deck quoted below is present in the working tree under
`doc/claude/feedback/ngspice_upstream/repro/` and named where it is used —
`save_lower.cir`, `save_current_lower.cir`, `print_lower.cir`, `plain.cir`,
`time_node.cir` and `save_probe.cir`, plus `save_partial_lower.cir`, which
belongs to `doc/codex/issues/0057`.

Not a regression. The comparison predates `casemode` entirely, and the one
route that reaches it on `ngspice-46` fails there identically.

Three issues from the same report touch this one. `doc/codex/issues/0057` owns
every diagnostic a failed `.save` should emit, in all three modes;
`doc/codex/issues/0059` owns the raw file that failure leaves behind;
`doc/codex/issues/0058` owns the doubled banner visible in the `distinguish`
transcripts below. This issue owns one thing only: the comparison.

## Summary

`.save` names are matched against the run's variable names by `name_eq()`,
`src/frontend/outitf.c:1338`, whose final compare is a bare `strcmp`:

```c
/* This routine must match two names with or without a V() around them. */

static bool
name_eq(char *n1, char *n2)                                /* outitf.c:1338 */
{
    ...
    return (strcmp(n1, n2) ? FALSE : TRUE);                /* outitf.c:1358 */
}
```

Five callers, all inside `beginPlot()`:

| Site | Compare | Kind |
| --- | --- | --- |
| `outitf.c:286` | `name_eq(saves[i].name, refName)` | query against a stored name |
| `outitf.c:300` | `name_eq(saves[i].name, dataNames[j])` | query against a stored name |
| `outitf.c:324` | `name_eq(dataNames[i], refName)` | **stored against stored** — neither operand is a query |
| `outitf.c:427` | `name_eq(depbuf, run->data[j].name)` | query against a stored name |
| `outitf.c:432` | `name_eq(depbuf, dataNames[j])` | query against a stored name |

The four query rows are Class C by `doc/claude/decisions/0001-distinguish.md`
decision 3 — a name the caller typed, resolved against a name the run stores —
so the rule is `cieq()` under `fold` and `preserve` and exact only under
`distinguish`. That rule is `vec_name_eq()` (`src/frontend/vectors.c:413`,
exported in `src/include/ngspice/fteext.h:366`), which
`doc/codex/issues/0032` made the frontend's one answer to "are these two vector
names the same name?". `name_eq()` is exact in all three modes instead.

`:324` is not that question and is treated separately throughout this issue;
see acceptance criterion 2, which has a measured reason it must stay exact.

Both operands reach `name_eq()` without a `v()`/`i()` wrapper. `copynode()`
(`src/frontend/breakp2.c:161`) has already reduced the card's text —
`v(midnode)` to `midnode`, `i(vs)` to `vs#branch`, folding the accessor letter
itself with `*(l - 1) == 'i' || *(l - 1) == 'I'` — before `settrace()` stores
it. So the comparison that fires is `strcmp("midnode", "MidNode")` and
`strcmp("vs#branch", "Vs#branch")`, and `name_eq()`'s own parenthesis-stripping
arms are not on this path. (Corroboration rather than inference: if they were,
`.save i(Vs)` could never match, because that arm reduces `i(Vs)` to `(Vs` and
would compare it against `Vs#branch`. It matches.)

## Impact

Under `preserve` the miss has two shapes, and the loud one is the less
dangerous.

**When every `.save` name misses, the run ends.** Deck
`doc/claude/feedback/ngspice_upstream/repro/save_lower.cir`, net `MidNode`,
card `.save v(midnode)`:

```
$ build-ver_50/src/ngspice -b -n -D casemode=preserve save_lower.cir 2>&1 >/dev/null
Error: no data saved for D.C. Operating point analysis; analysis not run
doAnalyses: not found

op simulation(s) aborted
Error: incomplete or empty netlist
       or no ".plot", ".print", or ".fourier" lines in batch mode;
no simulations run!
$ echo rc=$?
rc=1
```

That is stderr entire. Stdout is the ordinary banner, `Circuit:`, the TEMP and
solver lines, and `binary raw file "save_lower.raw"` — the last of which is
quoted again below, because it is the run reporting success while failing.

At `720c8743a`. At HEAD this run is rc=0 with `MidNode` in the plot, which is
this issue's fix; the mode the deck still misses in is `distinguish`, where it
is correct to miss and where the run now says which name missed
(`doc/codex/issues/0057`).

**When only some of them miss, nothing happens at all.**
`repro/save_partial_lower.cir` — `0057`'s deck, since `0057` owns this shape —
is the same divider with `.save v(In) v(midnode)`, one spelling the run stores
and one it does not:

```
$ build-ver_50/src/ngspice -b -n -D casemode=preserve save_partial_lower.cir \
      >p.out 2>p.err ; echo rc=$?
rc=0
$ wc -c p.err
0 p.err
$ sed -n '/vectors currently active/,$p' p.out
Here are the vectors currently active:

Title: * partial .save under preserve: v(In) resolves, the folded net spelling does not
Name: op1 (Operating Point)
Date: Thu Aug 13 00:26:04  2026

    In                  : voltage, real, 1 long [default scale]
binary raw file "save_partial_lower.raw"
Note: Simulation executed from .control section
$ grep -in midnode p.out p.err || echo "(no match)"
(no match)
```

rc=0, stderr empty, a well-formed raw, and one requested signal simply absent.
Every cell of the matrix below is a **single**-`.save` deck, so the matrix shows
only the loud shape; the quiet one is what a schematic editor hits first,
because it emits many `.save` cards and only the mis-cased ones miss. Reporting
either shape is `doc/codex/issues/0057` — criterion 2 there is this partial case
by name — and this issue prescribes no diagnostic of its own.

`print` with the same identifier in the same mode is correct
(`repro/print_lower.cir`): `v(midnode) = 2.250000e+00`. `print` resolves
through `findvec()`, which took the Class C predicate; `.save` did not.

**The full matrix**, net `MidNode` and source `Vs`. Each cell is
`ngspice -b -n -D casemode=<mode>` over `repro/save_lower.cir` (the three node
rows) or `repro/save_current_lower.cir` (the three current rows), with the
card's spelling substituted and the deck's `write` line replaced by `display`,
reading off rc and the vector that reached the plot:

| `.save` card | `fold` | `preserve` | `distinguish` |
| --- | --- | --- | --- |
| `.save v(MidNode)` | rc=0 → `midnode` | rc=0 → `MidNode` | rc=0 → `MidNode` |
| `.save v(midnode)` | rc=0 → `midnode` | **rc=1, no vectors** | rc=1, no vectors |
| `.save v(MIDNODE)` | rc=0 → `midnode` | **rc=1, no vectors** | rc=1, no vectors |
| `.save i(Vs)` | rc=0 → `vs#branch` | rc=0 → `Vs#branch` | rc=0 → `Vs#branch` |
| `.save i(vs)` | rc=0 → `vs#branch` | **rc=1, no vectors** | rc=1, no vectors |
| `.save i(VS)` | rc=0 → `vs#branch` | **rc=1, no vectors** | rc=1, no vectors |

The matrix is `720c8743a`'s and the Resolution's re-measurement of it is
HEAD's; the two are meant to be read against each other, and the four bold
cells are what moved. Every rc=1 cell leaves the session on the `constants`
plot; the four `preserve` rc=1 cells are the defect. **The `distinguish` column is correct as it stands**
— two spellings are two names there, the lookup genuinely misses, and rc=1 is
the right answer. The `fold` column is correct too, and for a reason that is
not the comparison: measured, all eighteen cells run again with `save v(...)`
inside a `.control` block instead of on a `.save` card come back identical,
cell for cell, because the reader lowercases the control block exactly as it
lowercases the card. Under `fold` both operands are already lower case, so
`strcmp` and `cieq` cannot disagree.

**That last claim is testable rather than assumed, and testing it finds a
second reachable failure.** Take the fold away from the card by typing the
same `save` at the prompt, against `repro/plain.cir` — the same divider with no
`.save` and no `.control` in it. The transcripts below are filtered by the
`grep` shown, because `display` prints all twelve constants; nothing else is
cut:

```
$ printf 'source plain.cir\nsave v(MIDNODE)\nop\ndisplay\nquit\n' \
      | build-ver_50/src/ngspice -p -n -D casemode=fold 2>&1 \
      | grep -E 'Error|Name:|midnode *:'
Error: no data saved for D.C. Operating point analysis; analysis not run
Name: const (constants)

$ printf 'source plain.cir\nsave v(midnode)\nop\ndisplay\nquit\n' \
      | build-ver_50/src/ngspice -p -n -D casemode=fold 2>&1 \
      | grep -E 'Error|Name:|midnode *:'
Name: op1 (Operating Point)
    midnode             : voltage, real, 1 long [default scale]
```

`save v(MidNode)` gives the first transcript verbatim. `print V(MIDNODE)` after
an `op` at the same prompt in the same mode answers
`v(MIDNODE) = 2.250000e+00`. So the defect is live in the **default** mode on
every route the reader's fold does not cover — the same route class
`doc/codex/issues/0048` and `0033` name — and it is not a `casemode` regression
at all:

```
$ printf 'source plain.cir\nsave v(MIDNODE)\nop\ndisplay\nquit\n' \
      | /usr/local/bin/ngspice -p -n 2>&1 | grep -E 'Error|Name:|midnode *:'
Error: no data saved for D.C. Operating point analysis; analysis not run
Name: const (constants)
```

`ngspice-46`, no `casemode` support, same failure. Only `-p` on stdin was
measured; the interactive prompt and `ngSpice_Command("save ...")` reach
`com_save()` (`src/frontend/breakp2.c:34`) by the same path and are read from
the source, not measured.

**Three things make the `preserve` failure worse than a lost vector.**

*It is silent about the token.* Grepping the entire stdout **and** stderr of
the failing `repro/save_lower.cir` run for `midnode` gives zero hits, in every
mode; under `distinguish` stderr is the experimental-mode banner twice — that
doubling is `doc/codex/issues/0058` — and then only `Error: no data saved for
D.C. Operating point analysis; analysis not run`, `doAnalyses: not found` and
the batch-mode `incomplete or empty netlist` pair. **The silence belongs to
`doc/codex/issues/0057`**, which owns it in all three modes and for names with
no case variant at all; it is recorded here only because it is what makes the
comparison expensive, and criterion 3 below hands it over rather than
prescribing a second version of the same message.

*It still writes a plausible raw.* The failing run leaves a 570-byte
`save_lower.raw` that parses cleanly:

```
$ head -11 save_lower.raw
Title: Constant values
Date: Wed Aug 12 19:28:37 UTC 2026
Command: ngspice-46+, Build Wed Aug 12 19:28:37 UTC 2026
Plotname: constants
Flags: complex
No. Variables: 12
No. Points: 1
Variables:
	0	yes	notype
	1	FALSE	notype
	2	TRUE	notype
```

A consumer testing only for a well-formed raw shows the user `yes`, `FALSE`,
`boltz` as signals. The tells are `Plotname: constants` and a `Date:` that is
the build stamp; neither is something a raw reader has prior reason to test.
(`720c8743a`. At HEAD no such file is produced: `write` answers `Error: no
plot has been selected, "save_lower.raw" not written.` instead, which is
`doc/codex/issues/0059`'s fix and not this one's.)
That half is independent of `casemode` and **is `doc/codex/issues/0059`**,
filed from the same report as finding 3; `preserve`'s strict `.save` merely
makes it trivial to reach, which is the interaction `0057` records.

*It hits hardest the tools that were already doing the right thing.* (This
paragraph is FINDINGS' "Why this is the biggest migration hazard" argument,
`FINDINGS.md:124-128`, kept as written because it is right.) A program
that lower-cased names to cope with `fold` — the natural thing to do, and what
the reporting client did — has every stored `.save` card become fatal the
moment a user selects `preserve`. `preserve`'s contract is "labels preserved,
identity folds"; `print`, `findvec()`, `unlet`, `diff` and the vector table all
honour it after `doc/codex/issues/0027`, `0032` and `0037`. `.save` is the
carve-out, and it is undocumented.

## Root Cause

`doc/codex/issues/0032` swept `src/frontend/` for exactly this class and closed
six sites, but it swept for `cieq()` — its own table has `cieq` in every row —
because the question it was asking was "what is too **loose** under
`distinguish`". This site fails the other way: too **strict** under `preserve`.
A bare `strcmp` looks like nothing when the grep is for the case-insensitive
comparator, which is the same reason `doc/codex/issues/0049` was missed
(`gettrans()`'s `eq_substr` "asked nothing") and the same reason Phase 3 gates
2 and 4 were bare `strcmp`s that turned out to move `preserve` rather than
`distinguish`.

The tree already records the site. `tests/lint/identity.baseline:196` carries

```
src/frontend/outitf.c	strcmp(n1, n2)
```

untriaged, as one of the 274 comparisons the lint froze at `6e2dca123` — the
lint's own header says it "has no opinion about which is which", and this is a
case where the opinion was the whole finding.

Two neighbours are worth reading with it. `tests/lint/identity.baseline:197`
and `:198` are `strcmp(varName, d->v_name)` and
`strcmp(varName, run->data[i].name)`, `src/frontend/outitf.c:883` and `:875`,
inside `OUTattributes()`. Same file, and arguably the same class — a name
handed in from an analysis, resolved against the run's stored names. They are
**not** reachable today: every caller in the tree passes `varName` as `NULL`
(`acan.c:178`, `span.c:568`, `dcpss.c:947`, `cktsens.c:297`, `noisean.c:285`,
`noisesp.c:195`, `distoan.c:522`), so both arms are dead by construction. The
interface is public (`SPfrontEnd->OUTattributes`, `src/include/ngspice/ifsim.h:460`),
so they should be triaged rather than either fixed blindly or ignored.

**And nothing was going to catch it.** `tests/regression/case/` holds 113
decks and `.save` appears in exactly one
pair — `save-alli-case.cir:34` and `save-alli-case-lower.cir:13` — in both
cases as the bare word `save alli`, which `beginPlot()` consumes at
`outitf.c:244` through `cieq(saves[i].name, "alli")` before any of the five
`name_eq()` callers runs. The only other `save`-adjacent pair is
`savecurrents-case.cir` / `-lower`, which uses `.options savecurrents`; the
`.save @dev[...]` cards it generates reach `name_eq()` only in its
**non-matching** arm and are then resolved by `parseSpecial()` in Pass 2, so
nothing there depends on a match. `tests/regression/casedist/` has 23 decks
and no `save` card at all.

**No deck anywhere saves a named vector by a spelling other than the stored
one.** Measured over the whole tree rather than the two case directories —
every `save`/`.save` with an argument in any `.cir`, `.cmd`, `.sp` or `.net`
under `tests/`, minus the bare `all`/`alli`/`allv` forms — there are six sites:

```
$ grep -rniE '^[[:space:]]*\.?save[[:space:]]+[^[:space:]]' \
      --include='*.cir' --include='*.cmd' --include='*.sp' --include='*.net' tests/ \
  | grep -viE 'save[[:space:]]+(all|alli|allv)[[:space:]]*$'
tests/bsim3soipd/RampVg2.cir:15:.save @m1[Vbs], V(g)/10
tests/regression/pipe/postcoms-keyword-case.cmd:248:save SPEEDCHECK DELTACHECK v(out)
tests/regression/pipe/postcoms-keyword-case.cmd:269:save speedcheck deltacheck v(out)
tests/regression/pipe/device-keyword-case.cmd:362:save @r1[i]
tests/regression/pipe/device-keyword-case.cmd:386:save @r1[I]
tests/regression/misc/bugs-2.cir:10:save @v1[dc]
```

Four of the six are `@dev[param]` specials, which reach `name_eq()` only in its
non-matching arm and are then resolved by `parseSpecial()` in Pass 2
(`RampVg2.cir:15` also carries an expression, `V(g)/10`). The two
`postcoms-keyword-case.cmd` lines *do* vary the case of a save argument, on the
prompt route at that — but `SPEEDCHECK` and `DELTACHECK` are consumed *inside*
Pass 1 by `eqc()` (`src/frontend/outitf.c:307` and `:313`) before `name_eq()`
decides anything, and the vectors that arm creates are synthesised rather than
looked up in `dataNames[]`. Their third argument, `v(out)`, does reach
`name_eq()` and does match — in the spelling the run stores, against the
lower-case `out` of the `circbyline` divider beside it, which is the case that
already works in every mode.

So not one deck in the tree resolves a `.save` argument against a stored name
in a *different* spelling: the whole matrix above is uncovered.

The shape an acceptance deck must take is already the directory's convention —
56 `*-case.cir` files with 52 `*-case-lower.cir` twins, plus five decks
(`case-flag-noop`, `harness-alive`, `name-roundtrip`, `node-case-alias`,
`write-roundtrip`) outside it. The twin is what makes the evidence a number
rather than a diagnostic: `tests/bin/check.sh:20`'s filter drops every line
containing `Error`, and it captures stdout only, so the failure cannot be
asserted directly. Measured, the candidate deck `repro/save_probe.cir` —
`.save v(midnode)` against net `MidNode`, then `let vprobe = v(midnode)` and
`print vprobe` inside `.control` — gives a clean RED:

```
$ for m in preserve fold distinguish ; do
>   ngspice -b -n -D casemode=$m save_probe.cir 2>&1 | grep -E 'vprobe|Error' ; done
--- preserve, rc=1
Error: no data saved for D.C. Operating point analysis; analysis not run
Error: RHS "v(midnode)" invalid
Warning from checkvalid: vector vprobe is not available or has zero length.
Error: incomplete or empty netlist
--- fold, rc=0
vprobe = 2.250000e+00
--- distinguish, rc=1
Error: no data saved for D.C. Operating point analysis; analysis not run
Error: RHS "v(midnode)" invalid
Warning from checkvalid: vector vprobe is not available or has zero length.
Error: incomplete or empty netlist
```

(The `--- mode, rc=` lines are labels added here, with the rc of that same run;
every other line is verbatim, and the `grep` is the only thing cut.)

That is the RED, at `720c8743a`. At HEAD `preserve` and `fold` both print
`vprobe = 2.250000e+00` and `distinguish` prints the same four lines as above
with `Warning: no vector named 'midnode'; 'MidNode' differs only in case
(casemode=distinguish)` in front of them, which is `doc/codex/issues/0057`.

The missing line is the assertion, exactly as `save-alli-case.cir`'s header
comment argues for its own `zprobe`.

## Acceptance Criteria

1. The four query callers — `outitf.c:286`, `:300`, `:427`, `:432` — compare
   with the Class C rule. `vec_name_eq()` is that rule and is already exported,
   so the edit is `name_eq()`'s final line, or `name_eq()` per caller if `:324`
   is left on the exact compare. The reduction is the proof for `fold`:
   `vec_name_eq()` is `cieq()` there, and under `fold` both operands are lower
   case, so no `.save` **card** can change. The `fold` **prompt** route does
   change, from rc=1 to rc=0 — a query-side loosening of the same kind
   `doc/claude/decisions/0001-distinguish.md` decision 3 accepted for
   `evtprint.c:341` and `evtshared.c:253`, and the change that makes `save` and
   `print` agree at the prompt.

2. `outitf.c:324` is decided, not swept in. It compares two simulator-side
   names — is this data name the reference vector's name — and giving it
   `cieq()` under `preserve` **deletes a vector**. Measured on
   `repro/time_node.cir` — a divider whose mid node is literally called `Time`,
   `.control tran 1u 3u` / `display` / ASCII `write`:

   ```
   $ ngspice -b -n -D casemode=preserve time_node.cir 2>&1 \
         | grep -E "^ +(In|Time|Vs#branch|time) +:"
       In                  : voltage, real, 59 long
       Time                : time, real, 59 long
       Vs#branch           : current, real, 59 long
       time                : time, real, 59 long [default scale]

   $ sed -n "/^Variables:/,/^ 1/p" time_node.raw
   Variables:
           0       time    time
           1       v(In)   voltage
           2       Time    time
           3       i(Vs)   current
   Values:
    0      0.000000000000000e+00
           3.000000000000000e+00
           1.500000000000000e+00
          -1.500000000000000e-03

    1      3.000000000000000e-10
   ```

   Index 0 is the scale at t=0; index 2, the node `Time`, is 1.5 — half of
   `v(In)` = 3.0 — and survives beside it. Under `fold` the node is already
   gone: the same deck, same greps, yields only `in`, `time` and `vs#branch`,
   because there the two names really are one string. `:324`'s exactness is
   what keeps the node under `preserve`, and it must stay.

   **Amended 2026-08-13.** This criterion rested on a measurement and on
   nothing a test could re-run: the mutation `name_eq() -> vec_name_eq()`
   passed the whole suite, so the one comparison this issue deliberately did
   **not** move was the one comparison nothing guarded. A deck is therefore
   owed as well, in `tests/regression/case/` because `preserve` is the only
   mode that can see it, and it has to assert on the plot's **contents**
   rather than on a value: `findvec()` folds its query under `preserve`, so
   reading the node back by name answers with a number whichever of the two
   vectors it reaches. `tests/regression/case/plot-scale-name-twin.cir`.

3. **Deferred to `doc/codex/issues/0057`.** What a failed `.save` should say —
   decision 2 of `doc/claude/decisions/0001-distinguish.md` at a resolution
   site, the near-miss wording, the `cp_err` requirement, the partial miss —
   is that issue's whole subject and its criteria 1-6; this issue does not
   restate it. What is owed *here* is only that the comparison chosen under
   criterion 1 leaves that report writable: it does, because the Pass 2 sweep
   that must carry it (`src/frontend/outitf.c:412`, `if (savesused[i])
   continue;` at `:414`) is driven by the `savesused[]` flags rather than by
   `name_eq()`, so loosening the comparator shrinks the set that reaches Pass 2
   without changing how Pass 2 sees it. `0057` is sequenced after this issue,
   for the reason its criterion 8 gives.

4. A `tests/regression/case/` deck asserts the `preserve` fix, with its
   `-case-lower.cir` twin, RED-first: the probed value **absent** at the parent
   commit, not merely different. Both the node spelling (`v(midnode)` against
   `MidNode`) and the branch-current spelling (`i(vs)` against `Vs`) are
   witnesses, and a fix in one does not clear the other only if both are in the
   deck — they share `name_eq()`, so one deck with both is enough to document
   the pair.

5. A `tests/regression/casedist/` deck asserts that `distinguish` still
   refuses, so criterion 1 is not read as licence to loosen the mode where the
   current rc=1 is right. The diagnostic that deck may eventually also guard is
   `0057`'s, not this issue's.

6. The prompt route is covered where it lives. `tests/regression/pipe/` is the
   shape, and it already holds typed `save` commands with the case varied
   deliberately — `postcoms-keyword-case.cmd:248`/`:269`
   (`save SPEEDCHECK ...` beside `save speedcheck ...`) and
   `device-keyword-case.cmd:362`/`:386` (`save @r1[i]` beside `save @r1[I]`).
   Neither pair varies the case of a name that `name_eq()` matches, for the
   reasons under Root Cause, so the new deck is the third such pair rather than
   the first `save` in the directory — and the prompt is the only route on
   which the `fold`-mode half of this defect is reachable.

7. `tests/lint/identity.baseline:196` is removed by hand with the note the
   file's header convention asks for, and `:197`/`:198` are triaged in the same
   pass: either classified in place with a `case-lint:` comment or moved with
   the rest.

8. `make check` unchanged in all three modes apart from the decks added, and
   `tests/regression/case/` unchanged.

9. `doc/claude/decisions/0001-distinguish.md:314-316` is corrected in the same
   pass. It records that after `0037` the frontend has "two folded-key tables
   filtered by the same predicate and no vector-name matcher outside them".
   `name_eq()` is exactly such a matcher — in `src/frontend/`, outside both
   tables, deciding whether two vector names are one name — so that sentence is
   false as written, and it is the sentence the build-enforced lint of item 7
   of `0005`'s closing list would be freezing. Nothing else in the record moves:
   the four query callers are Class C by decision 3, their `fold`/`preserve`
   loosening has the precedent the record itself lists at lines 357-358
   (`evtprint.c:341`, `evtshared.c:253`), and criterion 2's case for keeping
   `outitf.c:324` byte-exact is decision 3's stored-against-stored rule
   unchanged.

## Resolution

Fixed at `720c8743a`'s child. Every numbered criterion above is met, and every
number below was re-measured on 2026-08-13, after a second round of work on
`doc/codex/issues/0057` had touched the same file; none of them moved. The one
thing that is not yet true is not a criterion of this issue but of `0057`'s —
criterion 8 there asks that this issue move `make check` on its own, and both
issues are still in one uncommitted working tree. `0057`'s Resolution 8 says
what the two commits hold.

1. The four query callers call `name_eq_query()`, which is `vec_name_eq()`
   after the same `V()` unwrapping `name_eq()` did — the unwrapping is now
   `name_unwrap()`, shared by both, so the two differ in their final compare
   and in nothing else. The matrix was re-measured cell for cell on the
   fixed binary with the same decks: the four `preserve` rc=1 cells are now
   rc=0 with `MidNode` and `Vs#branch` in the plot, the `fold` column is
   unchanged, and the `distinguish` column is unchanged including both rc=1
   rows. The `fold` prompt route changes as this criterion said it would:
   `save v(MIDNODE)` at `ngspice -p` now resolves.
2. `outitf.c`'s reference-vector compare keeps `strcmp`, in `name_eq()`,
   which is now that one caller's function alone and carries the reason
   above it. `repro/time_node.cir` re-measured under `preserve` after the
   fix still lists `In`, `Time`, `Vs#branch` and `time`: the node survives
   beside the scale.

   **Guarded from 2026-08-13, and it was not before.** An adversarial
   verification of `0057` measured the mutation this criterion is about —
   `name_eq()`'s body replaced by `vec_name_eq()` — and the entire suite
   passed, so the claim above was true and untested.
   `tests/regression/case/plot-scale-name-twin.cir` is the deck: a divider
   whose mid node is `Time`, a `.tran`, no `.save` card so that beginPlot()
   takes the save-everything branch where `name_eq()` is, and `display`
   captured with `>&` and read back line by line. The capital T appears in
   exactly one vector name and nowhere else in the capture, so the assertion
   is the node's presence in the plot and not a value — which matters,
   because `findvec()` folds under `preserve` and answers a read of `Time`
   with a number either way. RED under the mutation (`SCALE-TWIN-DROPPED`),
   GREEN as shipped (`SCALE-TWIN-KEPT`), beside a positive read of
   `V1#branch` from the same capture so it cannot pass on a capture that was
   never written.
3. Delivered by `doc/codex/issues/0057`, which landed after this and is
   closed in the same working tree. Nothing here prescribes a diagnostic.
   That issue's report went through four rounds after this one closed, the
   last of which **narrowed it to a case near miss**; none of them changes
   the comparison, so nothing here moved with them. What did change is that
   `0057` no longer reports an unresolved `.save` name at all unless a case
   variant of it exists, so the `distinguish` rows of criterion 1's matrix
   are still rc=1 and are now the only rows that say anything.
4. `tests/regression/case/save-name-case.cir` with its
   `save-name-case-lower.cir` twin. Both witnesses are in the one deck —
   `.save v(midnode) i(vs)` against `MidNode` and `Vs` — probed by `vprobe`
   and `iprobe`. RED at the parent commit with **both** probe lines absent
   from stdout (the run ends in "no data saved" and `let` has nothing to
   read), against a two-line `.out`; the twin passes at the same commit.
5. `tests/regression/casedist/save-name-case.cir`. It saves one name the
   deck wrote and two it did not, so the analysis runs and the two misses
   have to be read as absences: `vin = 3`, `nmid = 0`, `nbr = 0`. It passes
   at the parent commit too, which is the point of it.
6. `tests/regression/pipe/save-name-case.cmd`, the third `save` pair in that
   directory and the first that varies the case of a name `name_eq()`
   matches. Five checks, the stored spelling first each time, each on its own
   `circbyline` circuit and behind a `destroy all` — without which a run that
   ends in "no data saved" leaves the previous check's plot current and the
   read answers from it, which is how the first draft of that deck passed
   against the defect.
7. `tests/lint/identity.baseline` loses all three `outitf.c` entries, by the
   file's second route: the comparisons are classified where they stand.
   `strcmp(n1, n2)` is `name_eq()`'s, kept for criterion 2's reason.
   `OUTattributes()`'s two are triaged with it and kept byte exact for the
   same reason rather than migrated: `varName` is an `IFuid`, so both arms
   compare the analysis's own name for a column against the name the run
   stored for it — this file's stored-against-stored question, not the save
   list's query — and the search includes the scale, which is where the
   `Time` hazard lives. Both remain unreachable (every caller passes NULL);
   they are classified rather than left frozen because the interface is
   public. 265 records, unchanged by the second round.

   The three markers read `/* case-lint: stored - both operands are names
   this run holds, see above */`. `stored` is a third reason word beside
   `keyword`, and the reason it is not `keyword` is that these are not
   keywords: by the lint driver's own taxonomy both operands are names a deck
   wrote, which would send them to an identity helper, and criterion 2 is the
   measured argument for why they must not go there. Nothing in the scanner
   reads the reason word — `tests/bin/identity_lint.awk` tests only for the
   literal `case-lint:`, so `keyword`, `bananas` and no word at all are
   accepted alike — so the word is written for the reviewer, and `stored` is
   the one that describes this class. `tests/bin/identity_lint.sh`'s failure
   text and `AGENTS.md` now list it beside `keyword`, `helper` (already in
   `src/frontend/define.c`) and `neither` (already in
   `tests/lint/selftest/silent.c`). `CLAUDE.md`'s sentence still names only
   `keyword` and wants the same one-line addition.
8. `make check` passes whole, and `tests/regression/case/` is unchanged apart
   from the pair added. Measured over the entire suite's stderr, the only
   new diagnostic lines anywhere are the two the new `casedist` deck emits —
   `casedist/save-name-case.cir`, which asserts on numbers rather than on
   those lines; the deck that asserts the near-miss wording,
   `casedist/save-undef-report.cir`, redirects it into a capture file, so it
   never reaches the suite log. The count is the first round's and is right;
   the attribution is corrected here.
9. `doc/claude/decisions/0001-distinguish.md`'s "no vector-name matcher
   outside them" sentence is corrected in place, with what was false about
   it, what is true now, and why the fifth caller is not covered by the rule.
