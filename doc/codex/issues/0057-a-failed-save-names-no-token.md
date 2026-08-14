# Issue: A Failed `.save` Names No Token

## Status

Closed in the working tree **as a near-miss report and not as the general
unresolved-name report this issue was first fixed with**, with one clause of
acceptance criterion 8 owed at commit time: `0056` has still to move
`make check` on its own, which cannot be shown until the two are committed as
two commits in that order. Resolution 8 says what the two commits hold.

**The wider report was tried and withdrawn, and this is the most useful
paragraph in this file.** The first three rounds of this fix reported *every*
unresolved `.save` token, in every mode, whether or not a case variant existed
— criterion 1 as originally written. Each of three adversarial verifications
then found another **correct** deck it fired on, and each was answered with
another guard: a deck with two analyses; a `.noise` column that only the plot
the analysis opens *second* lacks (`onoise_total`); an XSPICE event node, which
is on a node list `CKTnodes` does not contain; a token `gettoks()` had already
reduced to a fragment (`dout(state)` arriving as `state`); and finally two
decks under `examples/` that are shipped, tracked and correct —
`examples/transient-noise/noi-ring51-demo.cir` printing three
`no vector named 't1'` lines and `examples/probe/F5TurboV2-Probe.cir` one for
`mq4:3`. A condition that needs a new guard per verification round is the wrong
condition. The right one was already in the tree and this issue had already
quoted it: decision 2 of `doc/claude/decisions/0001-distinguish.md` at `:76-78`
conditions this family of warning on two things **together** — the resolution
fails **and** a name differing only in case exists. The report is now exactly
that, and every false positive above goes silent for a structural reason
rather than by a guard: `onoise_total`, `t1`, `mq4:3`, `state`, `in)` and an
event node have no case variant, so the condition is not met and nothing in
the code has to know what any of them is. The case defect the issue was filed
for still reports.

What the withdrawal costs is stated rather than argued away: an unresolved
`.save` name with no case variant is silent again, which is the defect the
Summary below describes. It is silent in stock `ngspice-46` too. This issue
therefore closes on the *case* half of what it asked for and records the
generic half as not delivered; `doc/claude/decisions/0008-undefined-node-diagnostic.md`
decision 1 remains the precedent that would place it, and Resolution 1 says
why that precedent does not transfer to the save list unchanged.

Criteria 1, 2, 3, 4, 6, 7 and 9 were rewritten on 2026-08-13 across four
adversarial verifications. Criterion 9 is no longer a list of guards; it is
the measurement that the classes it names are silent without one.

Three gaps are recorded and not fixed, in Resolution 9: an event node's own
case near miss, which this site does not report because the twin scan walks
the analog node list only; the `.print` route's wording, which no deck in this
tree can assert because `ft_savedotargs()` runs only from `src/main.c`'s batch
path; and `.save @r99[i]` for a device that does not exist, which is silent in
stock too and belongs to its own issue.

Found by a client-program integration — xschem generating decks, reading
the raw file and showing the names back to a user — and written up as finding 4
of `doc/claude/feedback/ngspice_upstream/FINDINGS.md`. Every measurement below
was re-run at `720c8743a` against `build-ver_50/src/ngspice` (`ngspice-46+`,
build stamp `Wed Aug 12 19:28:37 UTC 2026`) from
`doc/claude/feedback/ngspice_upstream/repro/`, with `/usr/local/bin/ngspice`
(`ngspice-46`, no `casemode` support) as the baseline.

Every deck quoted below lives in that directory and is named at the point it is
used: `save_lower.cir`, `print_lower.cir`, `save_absent.cir`,
`save_partial_lower.cir`, `save_cmd_lower.cir`, `save_guard_probe.cir`, and the
four `asym_*.cir` decks of the asymmetry table. The one deck that is not a file
— the control-block-less variant in the Summary — is quoted in full where it is
used. `repro/run_all.sh`'s finding 4 section runs the lot.

FINDINGS presents it through a `preserve` deck, which is how the client reached
it. Measured, it is **mode independent and pre-dates `casemode` entirely**: the
same silence is on stock `ngspice-46` for a `.save` of a name no card defines.
`preserve` is what makes it easy to reach, not what causes it.

**The `720c8743a` pin is load-bearing and the transcripts below are historical.**
Two fixes have landed since it was written and both move decks quoted here, so
a reader running them at HEAD gets different output and should not read that as
this issue being wrong. `doc/codex/issues/0056` made the `.save` comparison
fold, so every `preserve` transcript of a *mis-cased* name — the Summary's
`save_lower.cir` run, Impact's partial-miss and `save_cmd_lower.cir` runs, and
acceptance criterion 5's `save_guard_probe.cir` — now resolves and succeeds
instead of failing silently. This issue's own fix then made the failure that
survives say the token, so the `distinguish` runs of the same decks no longer
show a silence either. Each of those places carries a one-line note saying what
it gives at HEAD. What is **not** historical, and was re-measured on
2026-08-13: the asymmetry table's `nosuchnode` rows, which are absent names in
every mode and reproduce unchanged, and the whole of the Resolution below.

## Summary

When `.save` cannot resolve a name, nothing anywhere in the output says which
name. **This issue closes half of that**: a `.save` token that misses *and has
a case variant among the names the run has* is now answered by name, in
decision 2's words, under `casemode=distinguish`. A token that misses with no
case variant is still not named, in any mode, and the rest of this Summary
describes a defect that is still there. The Status paragraph above says why the
wider report was withdrawn.

The distinction is not a hedge and it is not about how hard the wider report
is to write — it was written, and it worked. It is that the two halves are
different claims about the deck. "There is no name like this here, and one you
did write is one keystroke away" is a statement `beginPlot()` can make from
what it has in front of it, and it cannot be true of a correct deck: a name
with a case variant would have resolved in the two shipped modes, so reaching
the report at all means the deck spells one thing two ways. "There is no name
like this here" is a statement about the *whole simulation*, and `beginPlot()`
sees one plot of one analysis of it; every false positive in the Status
paragraph is a correct deck about which that second statement was made from a
column list that was never going to hold the name.

`repro/save_lower.cir` is a divider whose net is `MidNode` and whose card is
`.save v(midnode)`. Under `preserve` the card's spelling is left alone, the
compare is byte exact (`doc/codex/issues/0056`), and the resolution misses.
The complete output of the run, both streams, captured separately and pasted
whole:

```
$ ngspice -b -n -D casemode=preserve save_lower.cir >out.txt 2>err.txt ; echo rc=$?
rc=1

$ cat out.txt

Note: No compatibility mode selected!


Circuit: * folded .save spelling -- kills the run under preserve

Doing analysis at TEMP = 27.000000 and TNOM = 27.000000

Using SPARSE 1.3 as Direct Linear Solver
binary raw file "save_lower.raw"

$ cat err.txt
Error: no data saved for D.C. Operating point analysis; analysis not run
doAnalyses: not found

op simulation(s) aborted
Error: incomplete or empty netlist
       or no ".plot", ".print", or ".fourier" lines in batch mode;
no simulations run!

$ grep -in 'midnode' out.txt err.txt ; echo "rc=$?"
rc=1
```

At `720c8743a` only. At HEAD the same command gives rc=0 and a plot holding
`MidNode`, because `doc/codex/issues/0056` made this comparison fold under
`preserve`; the mode this deck still misses in is `distinguish`, and there it
now answers `Warning: no vector named 'midnode'; 'MidNode' differs only in case
(casemode=distinguish)` before the same `no data saved` error. Both halves of
that sentence are the point of this issue and neither existed when the
transcript above was taken.

The load-bearing line is `src/frontend/outitf.c:465`. It names the *analysis*.
It does not name the card, the token, or the count of tokens that missed.

Meanwhile the message this case wants is already in the tree, already fires for
this exact identifier, and is already well worded — from the `print` path, on
`repro/print_lower.cir`, same net, same spelling:

```
$ ngspice -b -n -D casemode=distinguish print_lower.cir 2>&1 | grep 'differs only in case'
Warning: no vector named 'midnode'; 'MidNode' differs only in case (casemode=distinguish)
```

`src/frontend/vectors.c:478-481`, in `vec_warn_case_near_miss()`
(`src/frontend/vectors.c:464`).

The sharpest form of the defect is that **the same deck-level mistake is
diagnosed if the user writes it as `.print` and silent if they write it as
`.save`**. Both dot cards funnel into the same save list. The four decks
`repro/asym_save_mid.cir`, `asym_save_nosuch.cir`, `asym_print_mid.cir` and
`asym_print_nosuch.cir` are one divider with `.dc Vs 1 3 1`, `MidNode` as the
net, and exactly one card varied:

| card | deck | mode | rc | does stderr name the token? |
| --- | --- | --- | --- | --- |
| `.print dc v(midnode)` | `asym_print_mid.cir` | `fold` | 0 | n/a — resolves |
| `.print dc v(midnode)` | `asym_print_mid.cir` | `preserve` | 1 | **yes** — `Warning: can't parse 'midnode': ignored` |
| `.print dc v(nosuchnode)` | `asym_print_nosuch.cir` | `fold` | 1 | **yes** — `Warning: can't parse 'nosuchnode': ignored` |
| `.save v(midnode)` | `asym_save_mid.cir` | `fold` | 0 | n/a — resolves |
| `.save v(midnode)` | `asym_save_mid.cir` | `preserve` | 1 | no |
| `.save v(nosuchnode)` | `asym_save_nosuch.cir` | `fold` | 1 | no |

Every row of that table is `720c8743a`'s; the twelve-row version of it in
Resolution 7 is HEAD's, re-measured on 2026-08-13, and the two are meant to be
read against each other. Two rows moved for `0056` rather than for this issue
and it is worth saying which: both `preserve` rows of a mis-cased name are
rc=0 at HEAD, so the `preserve` column no longer has a failure in it to be
silent about. Every remaining `no` in the last column became a token.

All four decks carry a `.control` / `run` / `.endc` block, and that is not
decoration: a `.dc` card with no `.print` beside it never runs in batch mode at
all. Measured on the `.save` deck with the control block deleted —

```
* the same deck with the control block deleted
Vs In 0 DC 3
Rl In MidNode 1k
Rg MidNode 0 3k
.dc Vs 1 3 1
.save v(midnode)
.end
```

— both `fold` and `preserve` give rc=1 and

```
Error: incomplete or empty netlist
       or no ".plot", ".print", or ".fourier" lines in batch mode;
no simulations run!
```

which is rc=1 for a reason that has nothing to do with the save, with
`beginPlot()` never reached. With the control block the six rows above are what
the runs produce:

```
$ for d in asym_save_mid asym_save_nosuch asym_print_mid asym_print_nosuch; do
    for m in fold preserve; do
      ngspice -b -n -D casemode=$m $d.cir >o 2>e; rc=$?
      tok=$(grep -ihoE "can't parse '[a-z]+'" e | head -1)
      echo "$d $m rc=$rc token=[${tok:-none}]"
    done
  done
asym_save_mid fold rc=0 token=[none]
asym_save_mid preserve rc=1 token=[none]
asym_save_nosuch fold rc=1 token=[none]
asym_save_nosuch preserve rc=1 token=[none]
asym_print_mid fold rc=0 token=[none]
asym_print_mid preserve rc=1 token=[can't parse 'midnode']
asym_print_nosuch fold rc=1 token=[can't parse 'nosuchnode']
asym_print_nosuch preserve rc=1 token=[can't parse 'nosuchnode']
```

The `.save v(nosuchnode)` rows are the ones that say this is not a `casemode`
defect. On the baseline binary, `repro/save_absent.cir`, no flag at all:

```
$ /usr/local/bin/ngspice --version | sed -n 2p
** ngspice-46 : Circuit level simulation program
$ /usr/local/bin/ngspice -b -n save_absent.cir >b.out 2>b.err ; echo rc=$?
rc=1
$ cat b.err
Error: no data saved for D.C. Operating point analysis; analysis not run
doAnalyses: not found

op simulation(s) aborted
Error: incomplete or empty netlist
       or no ".plot", ".print", or ".fourier" lines in batch mode;
no simulations run!
$ cat b.out b.err | grep -ic nosuchnode
0
```

The same deck on this build gives rc=1 and the same zero count in each of
`fold`, `preserve` and `distinguish`.

**Which record this issue is owed under.** It has two halves and they rest on
different precedents.

- The *reporting* half — an unresolved `.save` token is named, in every mode,
  whether or not a case variant exists — is **not** decision 2 of
  `doc/claude/decisions/0001-distinguish.md`. That decision conditions its
  warning on "the resolution fails, **and** a name differing only in case
  exists" (`:76-78`), and explicitly rejects turning an unresolved name into an
  error (`:93-96`). The precedent that does place it is
  `doc/codex/issues/0028` and `doc/claude/decisions/0008-undefined-node-diagnostic.md`
  decision 1: one scan, two reports, and *the generic half is mode
  independent*, because it names one spelling — the one the deck wrote — and
  the deck wrote it in every mode. This issue is that decision applied at the
  save list instead of at the parser's term table.

  **This half is not delivered, and the paragraph above is why it took three
  rounds to see that it could not be.** `0028`'s scan runs once over a
  complete table after the whole deck is read; the save list is walked once
  per plot with one analysis's column list in hand. The precedent transfers
  the *wording* of a two-report scan and not its *ground*. Criterion 1 has the
  full argument and the Status paragraph has the short one.
- The *near-miss* half — naming the case variant when there is one — is
  decision 2's, at a resolution site its table does not list. Decision 2's
  table enumerates the sites the decision was written against; this one is
  absent from it, and absent from decision 3's Class A and Class C tables too,
  because it reaches identity through `name_eq()` (`src/frontend/outitf.c:1338`)
  rather than through `ng_ideq()` or `vec_name_eq()`. Making that comparison
  fold is `doc/codex/issues/0056`.

## Impact

Two failure shapes, and the second is worse than the one the client reported.

**Total miss — every `.save` name unresolved.** rc=1, so a consumer that waits
on the process has a signal, but it is the only signal: nothing names the
token, and `write` still produces a well-formed raw file of the constants plot.
Measured on `repro/save_lower.cir` under `preserve`:

```
$ ls -la save_lower.raw
-rw-r--r-- 1 qflow qflow 570 Aug 13 00:24 save_lower.raw
$ sed -n '1,13p' save_lower.raw
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
	3	boltz	notype
	4	c	notype
```

(cut at thirteen lines: eight header lines plus variables 0-4 of the twelve, so
the remaining seven variables and the binary payload follow.)

The twelve constants, what a reader does with them, and why the header's tells
are ones no raw reader has prior reason to test, are
`doc/codex/issues/0059`'s subject and are not restated here.

That is the combination the client actually hit: **no token named, no usable
exit signal for a streaming consumer, and a plausible-looking raw file.** A
tool that reads the raw as it appears rather than waiting on `rc` attaches a
database and shows the user `yes`, `FALSE`, `boltz` as signals, with nothing in
the log to grep for.

`720c8743a`'s combination, and two of its three parts survive at HEAD for this
deck. `preserve` resolves the name (`0056`); under `distinguish`, where it
still misses, the token is named by this issue's fix. **The plausible-looking
raw file is still written**: `doc/codex/issues/0059`'s fix landed beside this
one and was withdrawn on 2026-08-13, so `write` emits the constants plot again
and `0059` is Open. The paragraph is kept because it is the shape the client
reported and the reason all three issues were filed.

**Partial miss — one `.save` name resolves and another does not.** There is no
error at all. Measured on `repro/save_partial_lower.cir`, same circuit,
`.save v(In) v(midnode)`, `preserve`:

```
$ ngspice -b -n -D casemode=preserve save_partial_lower.cir >t3.out 2>t3.err ; echo rc=$?
rc=0
$ wc -c < t3.err
0
$ sed -n '/vectors currently active/,$p' t3.out
Here are the vectors currently active:

Title: * partial .save under preserve: v(In) resolves, the folded net spelling does not
Name: op1 (Operating Point)
Date: Thu Aug 13 00:18:41  2026

    In                  : voltage, real, 1 long [default scale]
binary raw file "save_partial_lower.raw"
Note: Simulation executed from .control section 
$ grep -in midnode t3.out t3.err ; echo "rc=$?"
rc=1
```

rc=0, stderr empty, the analysis ran, the raw is written — 316 bytes, and
`load` plus `display` on it lists `v(In)` and `v(all)` and nothing else — and a
requested signal is simply not in it. Every channel a consumer has reports
success. This is the shape a schematic tool hits first, because a schematic
emits many `.save` cards and only the mis-cased ones miss; the "no data saved"
error the client saw is the degenerate case where *all* of them miss.

`720c8743a` again: at HEAD the same run lists `In` **and** `MidNode`, because
`0056` resolves the folded spelling under `preserve`. The shape is not gone,
only this deck's route to it — `repro/save_partial_miss.cir`, `.save v(In)
v(nosuchnode)`, is the same rc=0-with-a-missing-signal in every mode, and it is
what Resolution 2 and the PARTIAL case of
`tests/regression/misc/save-undef-report.cir` use instead.

The control-language `save` command is the same path and the same silence:
`repro/save_cmd_lower.cir` puts `save v(midnode)` inside a `.control` block,
and under `preserve` at `720c8743a` it gave rc=1, byte-identical stderr to
`save_lower.cir`'s above, and `grep -in midnode` over both streams exited 1.
At HEAD it is rc=0 with an empty stderr, for `0056`'s reason and not this
one's; `distinguish` is where the route still misses, and there it names the
token like every other route.

**What 0056 leaves behind.** `doc/codex/issues/0056` makes `name_eq()` fold
under `preserve`, so the specific miss the client hit — a stored `.save` card
written in the folded spelling, run under `preserve` — stops missing. It
reduces the blast radius; it does not remove the defect and does not supersede
this issue. What survives:

- Under `distinguish`, `.save v(midnode)` against a net `MidNode` still misses
  by design, and still names nothing. Measured above: rc=1, no token.
- Under `preserve` *and* `fold`, a `.save` of a name that is genuinely absent
  rather than merely mis-cased still misses and still names nothing. Measured
  on the baseline `ngspice-46` above, with no `casemode` in the picture. **This
  bullet is unchanged by the fix as it finally landed**, and is the withdrawal
  of criterion 1: the first of these two is what this issue closes and the
  second is what it does not.

So `0056` is the identity fix and this is the diagnostic fix, and they are
independent in both directions: `0056` landing alone leaves a silent failure
for every deck with a typo in a `.save`, and this landing alone leaves the
client's decks failing loudly instead of silently — which is an improvement,
but not the one they need. **This issue owns the near-miss-on-a-failed-`.save`
prescription**, and `0056` now hands it over rather than specifying it a second
time: its criterion 3 opens *"Deferred to `doc/codex/issues/0057`"* and keeps
for itself only that the comparison it chooses leaves this report writable,
while its Resolution opens *"No diagnostic"* and points the token-naming here.
`0056` is left with the comparison and its decks; the prescription is stated
once, below.

**`0059` is what makes the silence expensive.** It is a separate defect —
`write` emitting the constants plot when no analysis ran — and it is
`casemode`-independent, as its own issue records. The interaction is the
client's actual failure and neither issue describes it alone: this issue means
the log has no token to grep for, `0059` means the artefact on disk looks like
a successful run, and in the partial-miss shape above rc is 0 as well, so all
three of the channels a consumer has agree that nothing went wrong. Fixing
either one alone leaves a consumer able to detect the failure; fixing this one
gives them the name they need to *report* it, which is what a schematic editor
has to do with it.

**Barely reachable from any committed deck.** No deck under
`tests/regression/case`, `tests/regression/casedist` or `tests/xspice/case*`
carries a `.save` card; the one `.save` card in the whole of `tests/` is
`tests/bsim3soipd/RampVg2.cir:15`, in a model-QA directory `make check` runs
through `runQaTests.pl`. What those directories do exercise is the same save
machinery by other routes — `save alli` in the control language
(`tests/regression/case/save-alli-case.cir:34` and its `-lower` twin at `:13`),
and the `.save @dev[...]` cards `inp_savecurrents()` generates for
`savecurrents-case.cir`. Neither reaches a *named-vector* miss, and neither can
assert on this diagnostic: `save-alli-case.cir:13` already quotes the exact
run-level line this issue is about,
`Error: no data saved for D.C. Operating point analysis; analysis not run`, in
a header comment explaining that `tests/bin/check.sh` cannot see it and that
the deck therefore asserts on a number instead. The narrow claim is what
matters and it holds: no committed deck asserts anything about what a failed
`.save` says.

## Root Cause

`.save` names never enter the machinery that carries the near-miss diagnostic,
and the one diagnostic that *is* on their path is gated shut for exactly the
card that produces them.

**Where they go.** `ft_dotsaves()` (`src/frontend/dotcards.c:58`) collects
every `.save` card out of `ci_commands` after the parse and hands the tokens to
`com_save()` (`:74`). `com_save()` is `settrace(wl, VF_ACCUM, NULL)`
(`src/frontend/breakp2.c:34`), and `settrace()` runs each token through
`copynode()` (`src/frontend/breakp2.c:161`), which strips the wrapper —
`v(midnode)` becomes `midnode`, `i(vs)` becomes `vs#branch` — and files the
bare string in the global `dbs` list. `ft_getSaves()`
(`src/frontend/breakp2.c:126`) copies that list into the array that
`beginPlot()` reads at `src/frontend/outitf.c:222` (`beginPlot()` is the static
worker at `:168`; `OUTpBeginPlot()`, `:110`, calls it at `:124`). Resolution is
Pass 0 (`:286`) and Pass 1 (`:300`), both `name_eq(saves[i].name,
dataNames[j])`, and `name_eq()` (`src/frontend/outitf.c:1338`) ends in a bare
`strcmp` (`:1358`) — `tests/lint/identity.baseline:196` carries it as an
un-migrated comparison, and making it fold is `doc/codex/issues/0056`.

So the query is matched against `dataNames[]`, the run's own variable-name
array. That is not the frontend vector table, not the parser's terminal symbol
table, not the subcircuit formal-pin list and not the XSPICE event node list —
it is none of the search sets any existing near-miss diagnostic walks.

**Why none of the existing emitters reach it.** There are five sites in the
tree that emit this message or a sibling of it, over those four search sets:

| Emitter | Message | What it resolves |
| --- | --- | --- |
| `vec_warn_case_near_miss()`, `src/frontend/vectors.c:464`, message at `:478-481` | `no vector named '%s'; '%s' differs only in case` | a frontend vector name, from `findvec()` (`:246`) and `vec_remove()` (`:661`) |
| `INPtermCaseCheck()`, `src/spicelib/parser/inpsymt.c:143`, messages at `:165` and `:169` | `no node named '%s'; ...` | a node name a device card or dot card referenced |
| `report_pin_case_miss()`, `src/frontend/subckt.c:1708`, message at `:1722` | `no subcircuit pin named '%.*s'; ...` | a `.subckt` formal pin |
| `report_bridge_case_miss()`, `src/xspice/evt/evtcheck_nodes.c:910`, message at `:935` | `no analog node named '%s'; ...` | the analog side of an XSPICE auto-bridge |
| `EVTnode_case_check()`, `src/xspice/evt/evttermi.c:222`, message at `:240` | `no event node named '%s'; ...` | an XSPICE event node nothing drives |

A `.save` argument is none of those five things. It is not a node reference —
`copynode()` puts it in `dbs`, and nothing hands it to `INPtermInsert()`, which
is why the `distinguish` run of `save_lower.cir` produces the experimental
banner and the "no data saved" error and **no** `no node named 'midnode'` line:

```
$ ngspice -b -n -D casemode=distinguish save_lower.cir 2>&1 | grep -c "no node named"
0
```

And it is not a frontend vector lookup: `vec_warn_case_near_miss()` takes an
`NGHASHPTR pl_lookup_table`, and at the moment of the failure there is no plot
to supply one. `plotInit(run)` is `src/frontend/outitf.c:479`; the error is
`:465-466` and the `return E_NOTFOUND` that follows it is `:467`, twelve lines
before `plotInit()`. The only lookup table in scope belongs to the *constants*
plot, which is the wrong search set and is also the plot `write` then writes
(`doc/codex/issues/0059`).

**The diagnostic that is on the path, and its gate.** Pass 2
(`src/frontend/outitf.c:411`) sweeps the saves that Passes 0 and 1 did not
consume and re-reads each as a device-parameter special. `parseSpecial()`
returns `FALSE` for anything not beginning with `@`, so a plain node name
always falls into this arm:

```c
            if (!parseSpecial(saves[i].name, namebuf, parambuf, depbuf)) {
                if (saves[i].analysis)
                    fprintf(cp_err, "Warning: can't parse '%s': ignored\n",
                            saves[i].name);
                continue;
            }
```

`src/frontend/outitf.c:417-422`. The message names the token. It is gated on
`saves[i].analysis`, which is set only by `com_save2()`
(`src/frontend/breakp2.c:42`) — that is, only for saves synthesised from
`.print`, `.plot`, `.sndparam`, `.sndprint` (`src/frontend/dotcards.c:138`),
`.four` (`:147`), `.op` (`:156`), `.tf` (`:159`) and `.meas`
(`src/frontend/com_measure2.c:355`, `:363`). A `.save` card goes through
`com_save()` with `NULL`, so `saves[i].analysis` is `NULL` and the message is
suppressed. That single `if` is the whole of the asymmetry in the table above:
`.print dc v(midnode)` prints `Warning: can't parse 'midnode': ignored` and
`.save v(midnode)` prints nothing, for the same miss on the same name in the
same run.

The gated message is also badly worded, which matters because the obvious fix
is to ungate it. Nothing failed to parse: `midnode` parsed fine and then failed
to *resolve*. Ungating it as written would name the token and misdescribe the
failure in every case it covers, including the ones it already covers today.

## Acceptance Criteria

1. ~~An unresolved `.save` name is reported, and the report contains the
   token. In **all three modes** — the defect is mode independent and the
   baseline binary reproduces it — and for a name with no case variant as well
   as one with.~~ **WITHDRAWN 2026-08-13, fourth round.** The criterion as
   struck through was implemented, shipped in this working tree for three
   rounds, and falsified by every one of them; the crossed-out text is kept
   because a reader has to be able to see what was asked for and what replaced
   it. What replaced it:

   > An unresolved `.save` name is reported **when, and only when, a name
   > differing from it only in case is among the names this run has.** That is
   > decision 2 of `doc/claude/decisions/0001-distinguish.md` at `:76-78`,
   > applied at a resolution site its table does not list, and it is criterion
   > 3 below, which is now the whole of what this issue delivers.

   The precedent this criterion cited for the generic half —
   `doc/codex/issues/0028` and
   `doc/claude/decisions/0008-undefined-node-diagnostic.md` decision 1, one
   scan producing two reports of which the generic one is mode independent —
   is a good precedent for the parser and does **not** transfer to the save
   list, which is the thing this criterion got wrong and the reason it took
   three rounds to see. `INPtermCaseCheck()` scans the parser's terminal
   symbol table once, after the whole deck is read, over a set that is
   complete and a property — "referenced and never defined" — that is
   determinate for the deck. `beginPlot()` walks the save list **once per
   plot**, with the column list of *one* plot of *one* analysis in hand;
   `.save` is a global card that applies to every analysis; the analyses do
   not produce the same columns and one analysis's plots do not either; and a
   save token need not be a node at all — `onoise_spectrum` is a column the
   analysis computes and an XSPICE event node is on a list `CKTnodes` does not
   contain. So at the point of the report "this name is not in this circuit"
   is answerable and "this name is nowhere in this simulation" is not, and the
   generic report is the second question. Decision 2's rejection of
   abort-on-miss (`0001:93-96`) is respected either way — what survives is a
   warning beside an error the run already emits, not a new error.

   The cost, plainly: `.save v(nosuchnode)` is silent in every mode, exactly
   as it is in stock `ngspice-46`, and the Impact section's two failure shapes
   are undiminished for a name that is simply absent. Fixing that properly
   needs the question asked where it can be answered — after the simulation,
   not inside `beginPlot()` — which is a different change with a different
   risk, and is recorded as such rather than attempted again here.
2. The **partial** miss reports too, where it reports at all. Only the
   all-saves-missed case produces any output today, from
   `src/frontend/outitf.c`, and it is the partial case that is silent end to
   end with rc=0. So the report belongs at the per-save site and not beside
   the run-level error, and it is at the per-save site.

   **Amended 2026-08-13, fourth round.** With criterion 1 withdrawn the
   partial shape reports only when the missing name is a near miss — which is
   the shape the client hit, since a schematic tool emits many `.save` cards
   and the mis-cased ones are the ones that miss. A partial miss on a name
   with no case variant is silent, and that is criterion 1's cost, not a
   separate one.
3. Under `distinguish`, and under `preserve` if `0056` leaves any miss
   reachable there, the near-miss twin is named, reusing decision 2's wording
   verbatim: `no vector named '%s'; '%s' differs only in case
   (casemode=distinguish)`. **This criterion is this issue's, not `0056`'s**,
   and `0056` reads that way: its criterion 3 opens by deferring here and its
   Resolution opens "No diagnostic", so the prescription is not stated twice and
   `0056` keeps the identity fix. The search set is `dataNames[]`, which Pass 1
   already walks — not a plot lookup table, which does not exist at
   `src/frontend/outitf.c:465` because `plotInit()` is `:479`. So
   `vec_warn_case_near_miss()` cannot be called and its wording has to be
   duplicated, the same conclusion `doc/codex/issues/0032` reached for
   `rawfile.c`'s `scale=` reference and for the same reason. `.save` then joins
   `doc/codex/issues/0049`'s subcircuit pins as a resolution site that decision
   2's table did not list.

   **Amended 2026-08-13, third round.** "The search set is `dataNames[]`" was
   written before criterion 9 existed, and criterion 9 is what makes it too
   narrow. `dataNames[]` is the column list of the analysis opening this plot;
   for `op`, `dc`, `ac` and `tran` that is the circuit's node list entire, so
   the twin was always found there and the criterion held. `noise`, `disto`,
   `sens` and `tf` build their own, holding none of the circuit's node
   voltages, and in a deck whose only analysis is one of those the scan found
   nothing and the deck got the generic line — true, and not the reason. The
   reason is the case, and criterion 9 had already made `ckt_knows_name()` ask
   the circuit rather than the analysis, so the twin scan asks the same list
   as a fallback: `dataNames[]` first, so no message that was already right
   can move, then `ckt->CKTnodes`. Measured in
   `tests/regression/casedist/save-near-miss-node-list.cir`.

   The **event node table is deliberately not** in the fallback. An event node
   with a case variant has its own diagnostic, `EVTnode_case_check()`
   (`src/xspice/evt/evttermi.c`, `doc/claude/decisions/0003`), and reaching
   the event table here would need a second lookup shape — the twin scan folds,
   `Evt_Node_Name_Eq()` does not.

   **Amended 2026-08-13, fourth round**, and this criterion is now the whole
   of what the issue delivers. Three clauses are added to it, each measured
   rather than reasoned:

   - The token is excluded from its own twin scan. "Differs only in case" has
     to mean *differs*, and with `.save all` in force a named save entry is
     never marked used while its column is in the run, so the scan meets the
     token itself and without the exclusion answers `'onoise_spectrum'
     differs only in case from 'onoise_spectrum'`. `case_twin_p()`.
   - A name the **circuit** has exactly is never a near miss, however few of
     its names this analysis's columns include. `ckt_knows_name()` asks both
     of a mixed-signal circuit's node lists, the event one through
     `Evt_Ckt_Has_Node()` (`src/xspice/evt/evtplot.c`), which is
     `Evt_Node_Name_Eq()`'s case rule. Without it a deck with nodes `out` and
     `OUT`, saving `out`, is offered `OUT`; and a deck with the event node
     `dout` beside an analog `DOUT`, saving `dout`, is offered `DOUT`.
   - Both search sets are needed and neither subsumes the other.
     `dataNames[]` holds a noise analysis's `onoise_RLoad`, which carries the
     deck's spelling of a device name and is on no node list; `ckt->CKTnodes`
     holds the node voltages a noise analysis has no column for. `dataNames[]`
     is searched first, so the twin named is the one the run would have
     produced when it has one.

   The event-node exclusion above is now a **stated gap and not a trade**:
   with the generic line withdrawn, `.save v(DOUT)` against the event node
   `dout` under `distinguish` is answered by nothing at all.
   `tests/xspice/casedist/save-event-node-case-split.cir` asserts that silence
   so that closing it later is a visible change; Resolution 9 says what
   closing it would take.
4. `src/frontend/outitf.c`'s `can't parse '%s': ignored` is left exactly as it
   was found — on its original `saves[i].analysis` gate, with its original
   text — and the two cards do not describe one failure two ways.

   **Amended 2026-08-13, fourth round.** This criterion originally asked for
   the message to be "fixed rather than merely ungated", because it
   misdescribes a name that parsed and did not resolve. The first three rounds
   did ungate it, in the narrow form of a parenthesis test that sent an
   expression fragment to it, and that was itself a behaviour change nobody
   asked for: `.save v(out)-v(in)` acquired `Warning: can't parse 'in)':
   ignored` where stock `ngspice-46` is silent, because a `.save` card's
   entries carry no analysis name. With criterion 1 withdrawn there is no
   second message on that path to be confused with, so the gate goes back and
   the `.save` route says nothing about an expression argument again. The
   near-miss report returns TRUE when it speaks and the `can't parse` line is
   skipped for that token, so a `.print`-derived token that near-misses gets
   one message and not two. The wording defect the criterion names is
   untouched and pre-existing, and Resolution 9 records why no deck in this
   tree can assert a change to it.
5. The report goes to `cp_err`, per decision 2's correction of 2026-08-11 and
   `doc/claude/decisions/0006-diagnostic-deck-coverage.md`. Measured with
   `repro/save_guard_probe.cir`, the read-it-back guard shape reaches it: `op
   >& cap.txt` followed by `fopen`/`fread`/`strstr`, the shape of
   `tests/regression/casedist/vector-unlet-report.cir`, recovers
   `no data saved` from inside the failing deck itself and echoes
   `GUARD-CAPTURED`. So a guard deck is writable without `tests/bin/check.sh`
   changing. (A `shell cat` of the same capture in the same block prints
   nothing — the file is not flushed until later — so the `fopen`/`fread` form
   is the one that works, not a shell read.)

   That probe echoes `GUARD-NOT-CAPTURED` at HEAD in all three modes, and the
   criterion it was written for is met anyway — the probe is what stopped
   reproducing, not the shape. Its `.save v(midnode)` resolves under `fold`
   and `preserve` now, so there is no error to capture at all; under
   `distinguish` the capture is four lines beginning
   `Warning: no vector named 'midnode'; 'MidNode' differs only in case
   (casemode=distinguish)`, with `Error: no data saved ...` on the line below
   it, and the probe reads exactly one line and tests it for `no data saved`.
   The standing evidence for this criterion is therefore the two decks the
   Resolution names, which read the whole capture in a loop; that they pass is
   the proof that `cp_err` is reachable from inside a deck.
6. The report fires once per unresolved token, not once per
   `OUTpBeginPlot()` call. `doc/codex/issues/0046` is the precedent for a
   near-miss diagnostic that doubles, and `doc/codex/issues/0058` is the same
   hazard from the other end — a `casemode` diagnostic whose count tracks
   `inp_readall()` calls rather than runs. Finding 7 of
   `doc/claude/feedback/ngspice_upstream/FINDINGS.md` is the client's complaint
   about exactly that: a consumer counting lines is what this diagnostic is
   for.

   **Amended 2026-08-13, after a first implementation was measured against
   it and failed.** "Once per token" was written without a unit, and the unit
   is the whole of it. The unit is **one simulation**: the report is emitted
   at most once for a given token between one `dosim()`
   (`src/frontend/runcoms.c`) and the next, so a `run` reports each mistake
   once however many plots and however many analyses it opens, and a second
   `run` of an unfixed deck reports again. One analysis opening several plots
   is not hypothetical and is what falsified the first implementation:
   `noisean.c` calls `OUTpBeginPlot()` twice for one `.noise` card and
   `distoan.c` up to six times for one `.disto`, so a report emitted from
   `beginPlot()` without a memory answers one mistake with two lines or six.
   Anything coarser than one simulation is wrong in the other direction — a
   memory that outlived the run would silence the second and third sub-deck
   of `tests/regression/misc/save-undef-report.cir`, which report the same
   token from three separate runs and are each correct to.

   **Amended again 2026-08-13, fourth round: the unit is unchanged and the
   deck that pins it moved.** With criterion 1 withdrawn, the only report
   there is to count is the near miss, so the counting deck has to be one that
   produces one — `tests/regression/casedist/save-near-miss-node-list.cir`,
   whose NOISE case saves a mis-cased node beside `.noise … 1`, which opens
   two plots, and asserts `tlines = 1`. The card carries `.save all` as well,
   so the run survives the miss and the second plot is really opened;
   without that the first plot ends the analysis and the count would be 1 for
   the wrong reason. Removing `save_miss_first_time()` makes it 2. The three
   sub-decks that report the same token from three separate runs are gone with
   criterion 1, so the "not coarser than one simulation" half is now argued
   and not measured — recorded as such in Resolution 6.
7. Decks split by mode, and the split is now the other way up from what this
   criterion first asked for. `tests/regression/casedist/` and
   `tests/xspice/casedist/` carry the report, because criterion 3 is the whole
   of it and `distinguish` is the only mode it exists in.
   `tests/regression/misc/`, `tests/xspice/digital/` and — through the whole
   of `tests/regression/case/` and `tests/xspice/case/` — `preserve` carry the
   *silences*: a correct deck and a deck with an unresolvable `.save` both say
   nothing, which is what re-widening the condition would break.
   `doc/claude/decisions/0008` decision 6 is the precedent for splitting by
   mode, and its decision 5 is the reason a census by hand is worth more here
   than a `make check` count: this harness cannot see the diagnostic being
   added, and it cannot see it being taken away either.

   **Amended 2026-08-13, fourth round.** The `.print` contrast is not a deck
   and cannot be one, for the reason Resolution 7 gives; that is unchanged.
   What changed is that the total miss and the partial miss are no longer
   *reports* to assert but silences, and `tests/regression/misc/`'s three
   decks now assert them as such. The census is over `examples/` as well as
   `tests/`, because the third round's verifier found the two decks that
   falsified an earlier claim of "no committed deck acquired noise" under
   `examples/`, which the census had not covered.
8. `make check` unchanged in all three modes apart from the new decks, and
   `tests/regression/case/` untouched. **Sequenced after `0056`**, for two
   reasons. `0056` decides what "the resolution missed" means, and criterion
   3's near-miss clause can only be written once the compare it qualifies is
   settled; written first, it would have to fire under `preserve` for a name
   that `0056` is about to make resolve. And `0056` is a behaviour change to a
   shipped mode, so it wants to move `make check` on its own, without a new
   diagnostic in the same commit range confusing which deck moved why.
9. **Added 2026-08-13; rewritten in the fourth round, when the guards it
   describes stopped being guards.** Five classes of correct deck must not
   acquire a warning, every one of them found by measuring an implementation
   rather than by reading it, and each one of them found *after* the previous
   had been fixed. That is the shape of the finding, and it is why this
   criterion no longer reads as a list of things the code must check:

   | class | the deck | what the wide report said |
   | --- | --- | --- |
   | a column this analysis has no place for | `.save v(out)` beside `.noise` | `no vector named 'out'` — twice, one per noise plot |
   | a column only the *first* plot of an analysis has | `.save onoise_spectrum` or `onoise_total` beside `.noise … 1` | `no vector named 'onoise_total'` |
   | an XSPICE event node | `.save v(dout)` on an `adc_bridge`/`d_inverter`/`dac_bridge` chain | `no vector named 'dout'` |
   | a token `gettoks()` reduced to a fragment | `.save dout(state)`, `.save v(out)-v(in)` | `no vector named 'state'`, `can't parse 'in)'` |
   | a shipped, tracked deck with a stale token | `examples/transient-noise/noi-ring51-demo.cir`, `examples/probe/F5TurboV2-Probe.cir` | `no vector named 't1'` ×3, `no vector named 'mq4:3'` |

   **All five are now silent because the report's condition is not met, and
   not because anything checks for them.** `out` and `dout` are names the
   circuit has; `onoise_total`, `t1`, `mq4:3`, `state` and `in)` are names
   nothing has — and none of the seven has a case variant anywhere in its run,
   so criterion 3's second half fails and no line is printed. Nothing in
   `report_save_case_miss()` knows what a noise column is, what an event node
   is, or what `gettoks()` does to an expression. That is the property this
   criterion now asserts, and the way to check it is the way the false
   positives were found: run the decks.

   Two conditions survive as real guards, and both are guards against a **false
   twin** rather than against a false miss, so neither can grow with the next
   analysis somebody adds:

   - `ckt_knows_name()` — a name the circuit has exactly is not a near miss,
     even when this analysis has no column for it. Both node lists, the event
     one through `Evt_Ckt_Has_Node()`.
   - `case_twin_p()`'s exclusion of the token from its own twin scan — "differs
     only in case" must differ.

   Each has a deck and each fails one when it is disabled; Resolution 9's
   matrix is the measurement.

   The **last two false positives of the wide report were only reachable
   because it fired on decks nobody had run**, which is the reason the census
   for this issue covers all 626 `.cir` files under `tests/` and `examples/`
   and not `make check` alone: `tests/bin/check.sh` captures stdout, and this
   diagnostic goes to `cp_err`, so a deck can acquire a warning and stay green
   forever.

## Resolution

**Fixed in four rounds, and the fourth undoes most of the first three.** The
first landed the report and was then measured against its own criteria by an
adversarial verifier, which falsified criterion 6 and found two behaviour
changes nothing in the criteria had asked for; the second round fixed those,
added criterion 9, and amended criterion 6 to name its unit. That round's
verifier found a third false positive of the same class, on XSPICE event nodes;
the third round fixed it, found a fourth by looking for the same shape
elsewhere, and widened criterion 3's search set. The third round's verifier
then found two more — a token `gettoks()` had reduced to `state`, and two
shipped decks under `examples/` — and at that point the pattern rather than the
individual defects was the finding: **each round removed one class of false
positive and revealed another, and a guard list that grows once per
verification round is the wrong shape.**

The fourth round therefore did not add a sixth guard. It re-conditioned the
report on what the tree had already decided — decision 2 of
`doc/claude/decisions/0001-distinguish.md` at `:76-78`, a warning when the
resolution fails **and** a name differing only in case exists — and every one
of the six false positives went silent without any code knowing what it was.
All four rounds are in one uncommitted working tree, so **criterion 8's
sequencing clause is still owed** — see 8 below.

The report is `report_save_case_miss()` at `src/frontend/outitf.c`'s Pass 2. It
is *beside* the `saves[i].analysis` gate rather than in place of it: the gate
and the `can't parse '%s': ignored` it guards are back exactly as they were
found, and the new report returns TRUE when it speaks so that a token it named
does not also get `can't parse`.

What the fourth round changed, all in `src/frontend/outitf.c` except the decks:

- `report_save_miss()` -> `report_save_case_miss()`: the generic line is gone,
  the near-miss line is all that is left, and the function returns `bool`.
- the `saves[i].analysis` gate on `can't parse '%s': ignored` restored, and
  with it the parenthesis test that had replaced it. An expression argument to
  `.save` is silent again, as in stock.
- `case_twin_p()`, one predicate for both twin scans, requiring `cieq` **and**
  `!eq` — the exclusion of the token from its own scan, which `.save all`
  makes reachable.
- `save_in_run()`, `save_resolved_note()` and the `save_miss_resolved`
  wordlist deleted. Both were suppressions of the generic report and neither
  can change the near-miss report's answer: a token that resolved in an
  earlier plot is a name the run has exactly, and criterion 3's condition asks
  for one that differs. `save_miss_reported` and `OUTsaveMissClear()` stay —
  criterion 6 is unchanged.
- eight decks rewritten or added; `tests/xspice/case/save-event-node-case.cir`
  deleted, because under the narrowed contract `preserve` cannot produce this
  report at all and the deck could no longer fail under any single mutation of
  the mechanism it named.

1. **Withdrawn, and this is the substantive change of the fourth round.** An
   unresolved `.save` name with no case variant is not reported, in any mode.
   The measurement that closed the argument is the pair of shipped decks:

   ```
   $ ngspice -b -n examples/transient-noise/noi-ring51-demo.cir 2>&1 >/dev/null | grep -c "no vector named"
   3                                          # third-round binary
   0                                          # this binary, and stock ngspice-46
   $ ngspice -b -n examples/probe/F5TurboV2-Probe.cir 2>&1 >/dev/null | grep -c "no vector named"
   1                                          # third-round binary
   0                                          # this binary, and stock ngspice-46
   ```

   Both are true positives — `.save in bufout v(t1)` names a net the deck no
   longer has, and `.probe V(MQ4:3)` passes `mq4:3` through verbatim — and
   both are decks this repository ships, has always run silently, and did not
   ask to have re-audited by a `casemode` issue. That is the argument in one
   line: the wide report was right about the tokens and wrong about whose
   problem they are.
2. Met where the report exists. The near miss is emitted from the per-save
   site, so a partial miss — one name resolving beside one that does not, rc 0,
   empty stderr — is named when the missing name is a case variant, which is
   the client's shape. `tests/regression/casedist/save-undef-report.cir`'s NODE
   and BRANCH cases are partial misses in that sense: the run completes and one
   token is named. A partial miss on an absent name is silent, which is
   criterion 1's cost.
3. **Met, and it is now the whole of the issue.** Under `distinguish`:

   ```
   Warning: no vector named 'midnode'; 'MidNode' differs only in case (casemode=distinguish)
   Warning: no vector named 'vs#branch'; 'Vs#branch' differs only in case (casemode=distinguish)
   Warning: no vector named 'onoise_rload'; 'onoise_RLoad' differs only in case (casemode=distinguish)
   ```

   Decision 2's wording verbatim, one line per token per simulation, to
   `cp_err`. The three clauses added by this round are each pinned by a deck
   and each falsified by a mutation; the matrix is under 9 below.

   The mode test at the head of `report_save_case_miss()` is **provably
   redundant and kept anyway**, which is worth stating because no deck can
   pin it: removing it flips nothing in the suite, measured (mutation M2).
   Under `fold` and `preserve` the resolution in Pass 1 folds
   (`name_eq_query()`, `doc/codex/issues/0056`) and so does
   `ckt_knows_name()`, so a token with a case variant among the run's names
   has already resolved or already been recognised, and the twin scan cannot
   be reached with one. The test is kept because it turns "the two shipped
   modes print exactly what they printed before" from a three-step argument
   into one line of code, and because the message it guards names the mode in
   its own text.
4. Met by leaving it alone. `can't parse '%s': ignored` keeps its gate and its
   text, so the `.print` route says what it has always said and the `.save`
   route says nothing about an expression argument — which is stock's
   behaviour and was not, for three rounds. The asymmetry table's rows,
   re-measured on this binary:

   ```
   asym_save_mid      fold         rc=0 token=[none]
   asym_save_mid      preserve     rc=0 token=[none]          <- 0056 resolves it
   asym_save_mid      distinguish  rc=1 token=[no vector named 'midnode']
   asym_save_nosuch   fold         rc=1 token=[none]
   asym_save_nosuch   preserve     rc=1 token=[none]
   asym_save_nosuch   distinguish  rc=1 token=[none]
   asym_print_mid     fold         rc=0 token=[none]
   asym_print_mid     preserve     rc=0 token=[none]
   asym_print_mid     distinguish  rc=1 token=[no vector named 'midnode']
   asym_print_nosuch  fold         rc=1 token=[can't parse 'nosuchnode']
   asym_print_nosuch  preserve     rc=1 token=[can't parse 'nosuchnode']
   asym_print_nosuch  distinguish  rc=1 token=[can't parse 'nosuchnode']
   ```

   Read against the six-row table in the Summary, which is `720c8743a`'s: the
   `distinguish` rows are new and are this issue; the `asym_print_nosuch` rows
   are stock's and are unchanged; and the three `asym_save_nosuch` rows are
   the withdrawal — they say `none` here and said `no vector named
   'nosuchnode'` for three rounds. **The asymmetry this issue was filed for
   therefore survives for a name with no case variant**: `.print dc
   v(nosuchnode)` names the token and `.save v(nosuchnode)` does not. What is
   closed is the case half of it, in the mode where case can be the reason.
5. Met: `cp_err`, and the read-it-back guard shape reaches it. Every deck
   below is built on it. `repro/save_guard_probe.cir`, the probe this
   criterion was argued from, no longer echoes `GUARD-CAPTURED`; the note
   under the criterion says why and why the criterion holds anyway.
6. Met, and pinned by one deck instead of four. The unit is one simulation.
   `tests/regression/casedist/save-near-miss-node-list.cir`'s NOISE case saves
   a mis-cased node beside `.noise … 1`, which calls `OUTpBeginPlot()` twice,
   and asserts `tlines = 1`; with `save_miss_first_time()` disabled it is 2
   (mutation M7, measured). The `.disto` row of the third round's table is no
   longer reachable by a deck — a `.disto` deck cannot produce a near miss
   without a node whose case variant is also a node — so the six-plot case is
   argued from the same mechanism and not measured. The other direction, "not
   coarser than one simulation", is also argued rather than measured now: the
   three sub-decks that reported one token from three separate runs were
   sub-decks of the withdrawn report. `OUTsaveMissClear()` is still called
   from `dosim()` and nowhere else.
7. Met, with the split inverted. `tests/regression/casedist/save-undef-report.cir`
   (NODE, BRANCH near misses; NOTWIN silent; OK silent) and
   `tests/regression/casedist/save-near-miss-node-list.cir` (NOISE, COLUMN
   near misses; NOTWIN, BOTH, SELF silent) carry the report;
   `tests/xspice/casedist/save-event-node-case-split.cir` carries it for a
   mixed-signal circuit. `tests/regression/misc/save-undef-report.cir`,
   `save-multianalysis-report.cir`, `save-crossplot-report.cir` and
   `tests/xspice/digital/save-event-node.cir` carry the silences in the
   default mode. Every silence assertion sits beside a positive read of the
   same capture.

   **The `.print` contrast is still not a deck and cannot be one**: the save
   entries that carry an analysis name are made by `ft_savedotargs()`
   (`src/frontend/dotcards.c:93`), called from exactly one place —
   `src/main.c:1577`, the top-level batch deck only, after the control block
   has ended. A deck sourced from inside a control block gets `ft_dotsaves()`
   and nothing else, so no sub-deck can carry a `.print` that produces a save
   at all, and the top-level run that could is past the last line a deck can
   redirect. (`ft_savemeasure()`, `dotcards.c:167`, would be a second producer
   and is dead code: nothing in the tree calls it.) The table under 4 above is
   the by-hand measurement instead, which is
   `doc/claude/decisions/0008-undefined-node-diagnostic.md` decision 5's point.
8. **Half met, and the unmet half is owed rather than argued away.**
   `make check` passes whole — EXIT=0, 302 PASS, 0 FAIL, with cached `*.log`
   and `*.trs` deleted first — and identity lint is 265 comparisons, baseline
   matches. The **sequencing** clause is not met: this issue and `0056` are
   both in one uncommitted working tree, so `0056` has not moved `make check`
   on its own. What is owed is two commits in this order — `0056` (the
   comparison, `name_eq_query()`, `tests/regression/case/`'s `save-name-case`
   pair **and `plot-scale-name-twin.cir`**, the `casedist` and `pipe` decks,
   the baseline note) and then this issue (`report_save_case_miss()` and its
   helpers, `outitf.h`, `runcoms.c`, `evtplot.c`, `evtproto.h`, and the seven
   decks) — with `make check` run between them so the `case` decks are seen to
   move on `0056` alone.

   `tests/regression/case/` is **no longer untouched by this series**:
   `plot-scale-name-twin.cir` is new there. It belongs to `0056`, whose
   criterion 2 had no deck until the third round's verifier measured the
   mutation and found the whole suite passing; the file is listed with `0056`
   above so the commit split stays clean.
9. Met, and met structurally. Nine mutations were applied to
   `src/frontend/outitf.c` in place, each rebuilt (mtime confirmed moved every
   time, no `cp -p`) and the targeted decks re-run, then restored `md5sum -c`
   clean:

   | mutation | decks that fail |
   | --- | --- |
   | M1 `name_eq()` -> `vec_name_eq()` | `case/plot-scale-name-twin` |
   | M2 the `distinguish` mode test removed | **none** — provably redundant, see 3 above |
   | M3 `ckt_knows_name()` removed | `casedist/save-near-miss-node-list` (BOTH), `xspice/casedist/save-event-node-case-split` (TWIN) |
   | M4 `case_twin_p()` without its `!eq` | `casedist/save-near-miss-node-list` (SELF) |
   | M5 the `dataNames[]` scan skipped | `casedist/save-near-miss-node-list` (COLUMN) |
   | M6 the `ckt->CKTnodes` fallback removed | `casedist/save-near-miss-node-list` (NOISE) |
   | M7 `save_miss_first_time()` removed | `casedist/save-near-miss-node-list` (`tlines` = 2) |
   | M8 `Evt_Ckt_Has_Node()` removed | `xspice/casedist/save-event-node-case-split` (TWIN) |
   | M9 the wide report restored | **all seven decks of this issue** |

   M9 is the row that matters most: the withdrawal is not a deletion nobody
   would notice, it is asserted by every deck this issue owns. M1 is `0056`'s
   criterion 2 and is here because the same verification found it.

   The five classes of criterion 9 were re-measured one deck at a time on this
   binary: `noi-ring51-demo.cir` 3 lines -> 0, `F5TurboV2-Probe.cir` 1 -> 0,
   `.save onoise_spectrum onoise_total` beside `.noise … 1` 1 -> 0 with
   `onoise_total = 2.864464e-08` still printed, `.save all onoise_total` 1 ->
   0, `.save dout(state)` on an event chain 1 -> 0, and `.save v(midnode)`
   against `MidNode` under `distinguish` 1 -> 1.

   **Three gaps are recorded rather than fixed.**

   *An event node's own case near miss is not reported here.* Under
   `distinguish`, `.save v(DOUT)` against the event node `dout` is silent:
   the twin scan walks `ckt->CKTnodes` and `dataNames[]`, and an event node is
   on neither. Reaching it would need a second lookup shape over the event
   table — the twin scan folds, `Evt_Node_Name_Eq()` does not — which is a
   `Evt_Ckt_Case_Twin()` beside `Evt_Ckt_Has_Node()`, perhaps fifteen lines,
   plus a deck. It was not taken in this round because the round's whole
   subject was narrowing, and because the gap is stated by a deck:
   `tests/xspice/casedist/save-event-node-case-split.cir`'s SPLIT case asserts
   the silence, so closing it later is a visible change and not a surprise.
   `EVTnode_case_check()` (`doc/claude/decisions/0003`) covers the neighbouring
   case — an event node with a case variant that nothing drives — and does not
   cover this one, because the node here is driven.

   *The `.print` route to `can't parse` is worded wrongly and no deck in this
   tree can assert a change to it.* `ft_savedotargs()` runs only from
   `src/main.c`'s batch path, after every `.control` block has ended, so no
   redirect can be in force over it and `tests/bin/check.sh` captures stdout
   only. This round leaves the message exactly as stock has it, so the gap is
   now purely inherited: three rounds of this issue made it worse and this one
   does not. Fixing it properly needs a harness decision — a deck shape that
   can capture a batch run's stderr — which is a contract call, not an
   implementation one.

   *`.save @r99[i]` for a device that does not exist is silent, in stock and
   here.* `parseSpecial()` succeeds on the `@dev[param]` shape without asking
   whether `dev` exists, so the miss never reaches the report site; it
   surfaces later, or not at all. Same family, different code path, and
   `.save @r1[i]` against a real device is measured correct in all three
   modes. It deserves its own issue rather than an amendment here.
