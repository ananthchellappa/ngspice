# Receipt 03 — 0069's Resolution made reproducible

Item 3 of `doc/claude/batches/2026-08-15-xschem-open-items/PLAN.md`. Branch
`ver_50`, HEAD at start `731c01455` — item 2's commit, so both of today's
behaviour changes are in the binary everything below was measured against.

Files changed: `doc/codex/issues/0069-batch-exit-status-reports-the-last-thing-main-did.md`
and this receipt. Nothing under `src/` or `tests/` was opened for editing, no
`.cir` was committed anywhere, and the item needed no code. Scratch decks under
`/tmp/claude-1000/-home-qflow-dev-ngspice-test/.../scratchpad/i3`.

Binary: `/home/qflow/dev/ngspice_test/build-ver_50/src/ngspice` (`ngspice-46+`,
build stamp `Sat Aug 15 18:18:34 UTC 2026`). Baseline `/usr/local/bin/ngspice`
(`ngspice-46`).

## What was wrong

1. **The guard listing was not a deck.** `guard_both.cir` at the old `:232-244`
   was `.save` / `.op` / a `.control` block and nothing else — no `Vs`, no `Rl`,
   no `Rg`. A reader who copied it got the `dotcards.c:225` assertion abort
   (rc=134, the abort receipt 02 of the predecessor batch found and PLAN item 4
   is filing), not the three-row table printed under it.
2. **Three named decks did not exist.** `guard_both.cir`, `guard_absent.cir` and
   `ctl_noop.cir` are in no directory of the tree, so every number in the
   Resolution's table, and the `599`-byte figure quoted twice in *Summary* and
   *Impact*, was attributed to a file nobody could open.
3. **Two byte counts were stale** (below), and one *Impact* row overstated a
   silence.

## What I re-measured, and the output

Every deck in the corrected issue is printed complete. To prove the printed
version is the measured version, all four were extracted back **out of the
edited issue** with `awk` on its ```` ```spice ```` fences and run from a clean
directory. That is the output quoted here.

### The guard, as printed in 0069 (`ngspice -b -n -D casemode=<mode> guard.cir`)

```
 fold        rc=0 fired=0 raw=[Plotname: Operating Point 281B vars=[0 v(midnode) voltage]]
 preserve    rc=0 fired=0 raw=[Plotname: Operating Point 281B vars=[0 v(MidNode) voltage]]
 distinguish rc=1 fired=1 raw=[ABSENT]
```

Same deck with the four guard lines (`if` … `end`) deleted:

```
 UNGUARDED fold        rc=0 Plotname: Operating Point 281B No. Variables: 1
 UNGUARDED preserve    rc=0 Plotname: Operating Point 281B No. Variables: 1
 UNGUARDED distinguish rc=0 Plotname: constants       570B No. Variables: 12
```

Same deck with `.save v(nosuchnode)`, no `casemode` flag on either binary:

```
 guarded   stock  rc=1 fired=1 raw=ABSENT
 guarded   ver_50 rc=1 fired=1 raw=ABSENT
 unguarded stock  rc=0 Plotname: constants 569B
 unguarded ver_50 rc=0 Plotname: constants 570B
```

The three-row table's rc, guard and rawfile columns are therefore exactly what
0069 published on 2026-08-14. The decision it supports is untouched.

### The control-only deck (0069's old `ctl_noop.cir`), now printed complete

```
 fold        rc=0 analyses=1 nearmiss=0 raw=[Plotname: Operating Point 289B]
 preserve    rc=0 analyses=1 nearmiss=0 raw=[Plotname: Operating Point 289B]
 distinguish rc=1 analyses=1 nearmiss=1 raw=[Plotname: constants       570B]
```

The `distinguish` row is the "rc=1 does not mean nothing was written" row. The
`fold` and `preserve` rows are new and are worth having: the same block at rc=0,
which is arm 3 of the chain (`Note: Simulation executed from .control section`).

### The three `$sim_status` properties, now one deck plus one deck

`props.cir` as printed, under `distinguish`, rc=0:

```
 HAVE-BEFORE=0
 VALUE-BEFORE=
 HAVE-AFTER=1
 AFTER-BAD=1
 AFTER-GOOD=0
 stderr: Error: sim_status: no such variable.
```

`noanalysis.cir` as printed: rc=0, `AFTER-RUN=0`,
`Note: Simulation executed from .control section`.

### The Summary's four-run table, re-run from the committed `repro2/` decks

```
 A -r plain_fail  rc=1 analyses=1 nearmiss=1 raw=none
 B    plain_fail  rc=0 analyses=1 nearmiss=1 raw=none
 C    ctl_fail    rc=0 analyses=2 nearmiss=2 raw=ctl_fail.raw constants 570B
 D -r ctl_fail    rc=1 analyses=2 nearmiss=2 raw=ctl_fail.raw constants 570B
```

Unchanged, and rows C and D are also criterion 4's two-simulation evidence
re-measured (two `Doing analysis` lines, two near-miss warnings for one
mistake).

### The Impact table's `absent.cir` rows

```
 ver_50 fold/preserve/distinguish  rc=0  Plotname: constants 570B  mentions of 'nosuchnode': 0
 stock  no flag                    rc=0  Plotname: constants 569B
```

## Did today's two commits move a number

**`25e891ec3` (wildcard rename) — yes, one, and it is an improvement.** The
guard deck's `fold`/`preserve` rawfile is now 281 bytes holding **one** variable
under the net's own name. The predecessor crew measured the same deck at 294
bytes this morning, before the fix, when a bare `write` of a one-vector plot
labelled the match `all` and thereby also tripped `com_write()`'s scale prepend
into a second column — receipt 01's own before/after table records that shape
(`2 vars v(in) v(all)` → `1 var v(in)`). 0069 printed no byte count for that
file, so no published number was falsified; the issue now states the variable
name and count, and its *Related* bullet on 0064 no longer tells a reader to
expect n+1 columns unconditionally. The surviving half is measurable on the same
deck: `write guard.raw` gives one column `v(midnode)`, `write guard.raw
v(midnode)` gives two of that name. That is 0064's second mechanism, PLAN
item 5's issue.

**`731c01455` (the `-r` writer's case-mode line) — no.** 0069 quotes no `-r`
header content. The issue now says the `-r` writer emits the same gated line, so
a reader who follows it to 0071 is not surprised.

**The stale numbers were not from today.** 0069's `592` (here) against `569`
(stock), and its `599` for the control-only deck, were measured against the
20:52 build of 2026-08-14, which wrote the raw header's `Option: casemode=…`
line unconditionally. As committed the line is opt-in behind `casemodewrite`
(`9e341a8b7`, 23:38 the same evening), so the default file is 570 bytes today.
Both old numbers come back exactly with the gate on:

```
 casemodewrite ON, fold        592B  [Plotname: constants / Option: casemode=fold]
 casemodewrite ON, distinguish 599B  [Plotname: constants / Option: casemode=distinguish]
 casemodewrite OFF             570B
```

22 and 29 bytes, which is the whole of the difference. The issue records this
rather than silently swapping the numbers.

## What changed in the issue

- **Status** — the measurement provenance now names both builds, and a
  *Corrected 2026-08-15* paragraph states what was wrong and that the decision
  is unchanged.
- **Summary** — `ctl_noop.cir` replaced by a complete inline `ctl_only.cir` with
  its measured rc for all three modes; the `repro2/` decks given their full path
  and marked re-measured.
- **Impact** — the stderr row corrected. It said "**nothing at all** if the name
  is simply absent"; measured, stderr carries `Error: no data saved …; analysis
  not run` and `run simulation(s) aborted`, and what it never carries is the
  token. The row now says that, which is still 0057's point. `599` → `570`, and
  the `absent.cir` rows carry sizes.
- **Root Cause** — arm 4's example renamed to the control-only deck, plus its
  measured rc=0 twin.
- **Acceptance Criteria** — a new *Where the criteria stand, checked 2026-08-15*
  subsection. The criteria themselves are unchanged.
- **Resolution** — rewritten: four complete decks, the three-mode table with the
  variable names, the unguarded and stock blocks, the byte-count drift
  paragraph, and the three properties each attached to a deck that produces
  them.
- **Related** — the 0064 bullet updated for today's fix and its surviving half.

## Where the criteria stand

1. **Met.** `src/main.c` has no commit on `ver_50` since 0069 was filed; every
   line number the Root Cause quotes still lands (`:1534`, `:1559`, `:1561`,
   `:1577`, `:1584`, `:1588`), as do `runcoms.c:329/:352/:358`.
2. **Met at all three places** — guide §9, `RESPONSE.md`, this issue, each with
   all three properties.
3. **NOT MET.** `grep -rl sim_status tests/` returns nothing. There is no deck
   anywhere under `tests/` that mentions `sim_status`, so the guard shape is
   documented and measured in three places and asserted in none. **I did not
   write it — out of this batch's scope by instruction** — and the issue now
   says so plainly instead of leaving the criterion looking open by accident.
   The criterion's own specification still stands: the client's deck shape,
   rc=1 and no rawfile, negative control with a resolvable `.save`, in
   `tests/regression/pipe/`.
4. **Met** — guide §9's "Do not carry both an analysis dot card and a
   `.control run`", plus this issue's Impact, re-measured today.
5. **Met** — both documents state the reverse hazard; 570-byte constants file at
   rc=1 re-measured.

## For the driver or the owner

1. **The missing test is the one real gap.** It is `tests/` work, one deck plus
   an `.out`, in `tests/regression/pipe/`. Nothing else in 0069 is owed.
2. **No `.cir` was committed and I recommend none.** Every deck in 0069 is
   short, is printed inline, and was verified by extraction from the file
   itself, so there is nothing a committed copy would add that the criterion-3
   test will not add properly. The one exception, if the owner wants it, would
   be the guard deck arriving as the fixture of that test — which is where it
   belongs rather than beside an issue.
3. **`dotcards.c:225` is still unfiled at the time of writing** (PLAN item 4
   owns it). 0069 now refers to that abort by file and line as the thing a
   reader of the old listing hit; if item 4's number lands, that sentence is the
   one place to cross-reference.
4. The upstream submission has **not** been sent; nothing in this item says or
   implies otherwise.
