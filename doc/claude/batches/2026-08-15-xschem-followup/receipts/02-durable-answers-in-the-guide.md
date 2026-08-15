# Item 2 receipt — questions 2, 3 and 4 carried into the guide

Crew for `PLAN.md` item 2. Branch `ver_50`, HEAD at start `4a042f0f4` — item 1's
commit, so this builds on the corrected guide rather than on the pre-0070 one.

Files edited (only this one, plus this receipt):

- `doc/claude/casemode-distinguish-guide.md`

No `src/` or `tests/` change was needed, and nothing in the item turned out to
require code. `RESPONSE.md` was **not** touched — item 1's crew rewrote it
yesterday and this item is a carry-across, not a second edit of the
correspondence.

Every claim written into the guide was re-measured 2026-08-15 against
`/home/qflow/dev/ngspice_test/build-ver_50/src/ngspice` (`ngspice-46+`), with
`/usr/local/bin/ngspice` (`ngspice-46`) as the featureless baseline. Nothing was
carried over from `RESPONSE.md` or from an issue on trust. Scratch decks under
`/tmp/claude-1000/-home-qflow-dev-ngspice-test/.../scratchpad/m2`; nothing was
left in the repo.

## What was added, and where

Five edits, all in the guide's own voice.

1. **Intro pointer (`:7-12`).** The "skip to §9" line said §9 answers
   capability detection. It now also names the four other things §9 carries, so
   a client arriving at the top is sent to the right subsection.
2. **§5, after "Exit status will not save you" (`:162-165`).** That paragraph
   named no substitute. It now names `$sim_status` and `quit <n>` and points at
   §9's guard block. This is one of the two places the item flagged.
3. **New §9 subsection "Guard the run with `$sim_status`, not with the exit
   status" (`:338`).** Q4. Placed after the two probe subsections and before
   the raw-file subsections, which is the client's own order: probe the mode,
   run the deck, check the run ran, read the file.
4. **New §9 subsection "A node the deck spells two ways is reported, in all
   three modes" (`:427`).** Q3, with the three-mode table, four properties, the
   stderr/parse-time note, and the silence table.
5. **New §9 subsection "Under `distinguish`, `.save` stays byte-exact —
   permanently" (`:646`).** Q2. Placed immediately after "Which mode a
   schematic tool should ask for", because that is where a UI author decides
   how to word the warning, and immediately before "If you offer it as a
   setting", whose "Do not gate on exit status" sentence (the item's second
   flagged place) now names `$sim_status` as the replacement.

The guide had zero occurrences of `sim_status`, `.save`, "contract" and the
collision warning before this. It now has 11 `sim_status` mentions, the `.save`
contract with its decision-5 quotation, and the collision warning with its
silences.

## Measurements

Build: `ngspice-46+` at `build-ver_50/src/ngspice`. Baseline
`/usr/local/bin/ngspice` = `ngspice-46`.

### Q4 — the guard deck, all three modes

The deck **as printed in the guide** was extracted back out of the guide with
`sed` and run, so the version that landed is the version measured:

```
fold         rc=0 fired=0 raw=Plotname: Operating Point
preserve     rc=0 fired=0 raw=Plotname: Operating Point
distinguish  rc=1 fired=1 raw=ABSENT
```

Same deck with the four guard lines deleted:

```
UNGUARDED fold        rc=0 raw=[Plotname: Operating Point size=294]
UNGUARDED preserve    rc=0 raw=[Plotname: Operating Point size=294]
UNGUARDED distinguish rc=0 raw=[Plotname: constants size=570]
```

On stock, guard plus `.save v(nosuchnode)`, no `casemode` flag:

```
/usr/local/bin/ngspice          rc=1 fired=1 raw=ABSENT
build-ver_50/src/ngspice        rc=1 fired=1 raw=ABSENT
--- same deck unguarded ---
/usr/local/bin/ngspice          rc=0 raw=[Plotname: constants size=569]
build-ver_50/src/ngspice        rc=0 raw=[Plotname: constants size=570]
```

### Q4 — `$?sim_status` before any run

```
HAVE=0                                     <- $?sim_status, before op
VALUE=                                     <- $sim_status, empty
Doing analysis at TEMP = 27.000000 ...
HAVE-AFTER=1
VALUE-AFTER=0
stderr: Error: sim_status: no such variable.
```

### Q4 — the other two properties

```
AFTER-BAD=1        save v(midnode) / op    (distinguish)
AFTER-GOOD=0       save all        / op
AFTER-RUN=0        a deck with no analysis card at all, .control with only `run`
```

### Q4 — the two-simulation deck shape

One deck per row, `-D casemode=distinguish`, `.save v(midnode)` against net
`MidNode`:

```
shape1  .op card, no .control          rc=0  analyses=1  near-miss=1
shape1  .op card, -r x.raw             rc=1  analyses=1  near-miss=1  raw ABSENT
shape3  no dot card, op in .control    rc=1  analyses=1  near-miss=1  raw=constants 570 bytes
shape4  .op card AND .control run      rc=0  analyses=2  near-miss=2
```

`shape3` is the row behind the guide's "rc=1 does not mean nothing was
written": it exits 1 and still leaves a 570-byte constants file, because the
`write` inside the block already ran.

### Q3 — the collision warning, three modes

```
-D casemode=fold          Warning: node names 'Out' and 'out' differ only in case and name one node (casemode=fold)
-D casemode=preserve      ... name one node (casemode=preserve)
-D casemode=distinguish   ... name two nodes (casemode=distinguish)
no flag at all            ... name one node (casemode=fold)
stock ngspice-46          (nothing)
```

Deck as printed in the guide, extracted and re-run: identical.

### Q3 — across an `.include` boundary

`R1 in Out 1k` in the deck, `R2 out 0 1k` in `pdk/lib.inc`:

```
fold           ... 'Out' and 'out' ... name one node (casemode=fold)
preserve       ... name one node (casemode=preserve)
distinguish    ... name two nodes (casemode=distinguish)
```

### Q3 — counts

```
Out on 3 cards, out on 2 more     1 line in each mode
Out / out / OUT                   1 line (fold), 1 line (preserve), 3 lines (distinguish)
one pair in a .subckt body,       0 (fold), 3 (preserve), 3 (distinguish)
  3 instantiations
two simulations, pair + near miss 1 pair line, 2 near-miss lines, analyses=2
```

### Q3 — the silences, one deck each

| second spelling only in | fold | preserve | distinguish |
| --- | --- | --- | --- |
| `.subckt` body | silent | `X1.Mid`/`X1.mid` reported | reported |
| `X` card actual | silent | reported | reported |
| B card node field | silent | reported | reported |
| expression on an `E` card | silent | reported | near-miss line instead |
| `.model` name | silent | silent | silent |
| `.param` name | silent | silent | silent |

### Q2 — the `.save` contract

`.save v(midnode)` against net `MidNode`, `-r sc.raw`:

```
fold         rc=0  No. Variables: 1   0 v(midnode) voltage
preserve     rc=0  No. Variables: 1   0 v(MidNode) voltage
distinguish  rc=1  no rawfile
             Warning: no vector named 'midnode'; 'MidNode' differs only in case (casemode=distinguish)
             Error: no data saved for D.C. Operating point analysis; analysis not run
             run simulation(s) aborted
```

Plus the `.print` twin, which the item did not ask for and which is what makes
the contract read as a rule rather than as one card's quirk — `.dc Vs 1 3 1` /
`.print dc v(midnode)`:

```
fold         rc=0  Index  v-sweep  v(midnode)
preserve     rc=0  Index  v-sweep  v(midnode)
distinguish  rc=1  near-miss warning, then
             Error: no data saved for D.C. Transfer curve analysis; analysis not run
```

And the quiet one, `print v(midnode)` inside `.control` under `distinguish`:
the near-miss warning, no value printed, **rc=0**.

## Three things I measured that differ from what the sources say

1. **The collision warning cannot be captured with `>&` from a `.control`
   block.** `RESPONSE.md` §4 and decision 0018 decision 4 say it goes to
   `cp_err` "so a deck can capture it with `>&`". That is true of the stream
   and misleading in practice: the line is emitted at **parse** time, before
   any `.control` block runs. Measured — a deck whose block opens with
   `echo start >& cap.log` gets `start` in `cap.log` and the warning on the
   process's stderr. The guide says so; neither source document was edited.
2. **Under `distinguish`, a rebuilt card's expression reference gets the
   near-miss line, not the pair line.** `E1 o 0 vol='V(mid)*2'` beside
   `V1 Mid 0 dc 1` reports the pair under `preserve` and reports
   `Warning: no node named 'mid'; 'Mid' differs only in case` under
   `distinguish` — the unresolved reference is caught by a different report.
   The item's summary ("`preserve` and `distinguish` report the first two")
   holds; this is the third shape, and the guide's table states it exactly
   rather than rounding it to "reported".
3. **A `.subckt`-body pair is one line per instantiation**, so "once per pair
   per parse" is not "once per mistake" for a body. Three instances gave three
   identical lines. Decision 0018 decision 1's 2026-08-14 correction says this;
   `RESPONSE.md` §4 does not, and a UI that relays warnings has to know it.
   The guide carries it with the "deduplicate on the quoted pair" advice.

## What was left in `RESPONSE.md` rather than moved

Everything round-scoped. Specifically:

- §1's correction of round 1's own advice ("we told you to keep `rc`"), the
  apology, and the four-command `-r` vs `.control` transcript. The durable half
  of that — that rc reports an epilogue arm and cannot be read off the deck — is
  in the guide in one sentence with the issue number.
- §1's byte-count calibration note (570/592/569) beyond the two numbers the
  guard table needs.
- §2's whole `casemodewrite` treatment: already in the guide from item 1, and I
  did not re-derive it, per item 1's receipt point 3.
- §5 (`doc/codex/issues/0064`, the phantom `v(all)`) and §6's five-row deck
  shape table. Item 5 of this batch owns 0064's scope; the guide gets §6's
  durable half — do not carry both a dot card and a `.control run` — inside the
  `$sim_status` subsection, without the table.
- §7's replies about R2, R5 and R6, and §8's re-run instructions. All
  correspondence.
- The `doc/codex/issues/0067` crash story, which is already in the guide's
  `casemodewrite` subsection from before this item.

## For the driver or the owner

1. **`doc/codex/issues/0069` does need correcting, and I did not edit it**, as
   instructed. Two things are wrong with its Resolution:
   - The `guard_both.cir` listing at `:232-244` has **no netlist lines** —
     `.save v(midnode)` and `.op` and a control block, with no `Vs`, no `Rl`,
     no `Rg`. It cannot have produced the table underneath it. Run verbatim it
     does not produce any of that table's three rows; it **aborts** (see
     point 4). The measured table is right; the listing above it is not the
     deck that produced it.
   - `guard_both.cir`, `guard_absent.cir` and `ctl_noop.cir` do not exist
     anywhere in the tree (`find` across the repo returns nothing), so the
     listing is the only record of the deck and there is nothing to recover it
     from. The version now in the guide is complete, was extracted back out of
     the guide with `sed` and run, and reproduces 0069's table row for row —
     so a correction to 0069 can simply take the guide's block.
   Worth noting that this is exactly 0069's own acceptance criterion 3, which
   asks for a deck asserting the guard shape in `tests/regression/pipe/`. No
   such deck exists. Filing or writing it is `tests/` work and outside this
   item.
2. **Criterion 2 of 0069 is now satisfied for two of its three places.** It
   requires `$sim_status` documented at the guide's §9, at `RESPONSE.md`, and
   in the issue, with all three properties stated. The guide now has all three
   properties, measured; `RESPONSE.md` §1 already had them. Whoever closes 0069
   can tick that criterion.
3. **Nothing in the guide now contradicts `RESPONSE.md`.** The two overlap on
   Q2, Q3 and Q4 by design — the guide is the survivor when the correspondence
   is archived, and it is the fuller of the two on Q3 (the silence table and
   the per-instantiation count) and on Q2 (the `.print` twin).
4. **An unrelated abort, found by running 0069's listing verbatim, and it is
   upstream.** A deck with an analysis dot card and **no netlist at all** kills
   ngspice with a failed assertion rather than with a diagnostic:

   ```
   $ cat minimal.cir
   * no netlist
   .op
   .end
   $ ngspice --batch -n minimal.cir ; echo rc=$?
   ngspice: ../../../src/frontend/dotcards.c:225: ft_cktcoms:
       Assertion `plot_cur->pl_dvecs != NULL' failed.
   rc=134   (Aborted, core dumped)
   ```

   Three lines are the whole repro. It is **not** this branch: measured
   identically on `/usr/local/bin/ngspice` (`ngspice-46`, rc=134, same
   assertion) and it is mode-independent — `fold`, `preserve`, `distinguish`
   and no flag all abort. 0069's verbatim listing hits it because it has no
   netlist; on that deck the abort happens *after* `write` has already put a
   569/570-byte constants file on disk, so it is also another instance of
   0069's own "rc is not the signal" point, reached through a fourth route the
   issue does not describe.

   **This needs code and I wrote none**, per the item's instruction. There is
   no issue for it in `doc/codex/issues/`. It is nothing to do with casemode
   and nothing to do with this batch's client, so I did not widen the item to
   file one; the owner may want it filed, and it is small — the assertion is a
   missing empty-plot guard in `ft_cktcoms()`, on the same path as the fourth
   epilogue arm 0069 already dissects.
