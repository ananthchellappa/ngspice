# Response to the casemode findings — round 2

For whoever picks up the xschem side. Round 1 of this file replied to
`FINDINGS.md`; this is the reply to `REPLY.md`, which came back with six new
findings and four questions. **All four questions now have answers**, two of
them shipped as code, and **one piece of advice in round 1 was wrong and is
corrected in §1**.

Every line below was **re-measured 2026-08-14** against
`build-ver_50/src/ngspice` (`ngspice-46+`, build stamp
`Fri Aug 14 20:52:09 UTC 2026`) with `/usr/local/bin/ngspice` (`ngspice-46`) as
the featureless baseline. Round 1's work is committed through `58496a8dc`;
round 2's two shipped changes — the header line and the collision warning — are
in the same working tree and are not yet committed.
All six of your findings reproduce here — we ran your
`repro2/run_round2.sh` unmodified before touching anything.

Round 1's text is not preserved below. Where a round-1 claim still holds it is
restated in its round-2 form; where it does not, §1 says so by name.

---

## 1. Correction: we told you to keep `rc`. For your decks that was wrong.

Round 1 of this file said, of the constants-plot artefact, *"`rc` and a
vector-count sanity check are still worth having"*, and called `rc` *"the
obvious defence"*. **The first half of that is wrong for the deck shape a
schematic tool generates, and R1 is right that it is wrong.** We are sorry it
went out that way; it was written from decks that all had one shape, which is
exactly the method note you added at the end of your reply.

Your diagnosis needs one correction of its own, and it makes the problem
slightly worse rather than better. **The variable is `-r`, not `.control`.**
Your two commands differ in two ways, and it is the flag that decides:

```
$ ngspice -b -n -D casemode=distinguish -r plain_fail.raw plain_fail.cir
rc=1     no raw written                        <- your first command
$ ngspice -b -n -D casemode=distinguish        plain_fail.cir
rc=0                                           <- same deck, -r removed
$ ngspice -b -n -D casemode=distinguish        ctl_fail.cir
rc=0     ctl_fail.raw: Plotname: constants     <- your second command
$ ngspice -b -n -D casemode=distinguish -r x.raw ctl_fail.cir
rc=1     ctl_fail.raw: Plotname: constants     <- same deck, -r added
```

And a deck whose analysis lives *only* in a `.control` block — no analysis dot
card at all — exits **1**:

```
.control
save v(midnode)
op
write ctl_noop.raw
.endc
                      rc=1, and ctl_noop.raw is 599 bytes of constants anyway.
```

So "a `.control` deck has no rc" is not the rule, and the rule it replaces is
worse: `rc` reports whichever of `main()`'s four batch-epilogue arms ran last,
and which arm that is depends on `-r` and on whether the deck carries an
analysis dot card. A deck author cannot read it off the deck. The mechanism is
`src/main.c:1533-1596`; with no `-r`, `ft_savedotargs()` re-runs your `.op`
card *after* the control block with a save list of its own, that run succeeds,
and the exit status is its. Filed as `doc/codex/issues/0069`, with the full
chain.

Note the last column above as well: **rc=1 does not mean nothing was written.**
The fourth row exits 1 having already written `ctl_fail.raw` from inside the
control block, and `ctl_noop.cir` exits 1 leaving 599 bytes of constants. Only
the `-r` row with no `.control` writes nothing, and that is because `-r`'s own
writer never opened the file. Whatever `rc` says, the content checks are still
owed.

### The answer: `$sim_status`, and it needs no ngspice change

**Question 4 — is exit status inside `.control` fixable?** Decided: **no, and
deliberately not.** `run` inside `.control` is the scripting interface, and
decks retry, sweep and deliberately continue past a failed run; turning that
into a process failure would be a behaviour change to every such script, with
no opt-out and no way for a deck to say "I meant that". Your reply offered us
that answer as acceptable and it is the one we are giving.

What you get instead is better than the exit status, because it fires *before*
the artefact exists. `$sim_status` is the per-analysis outcome, readable from
the control language, and `quit <n>` gives the deck the exit status of its
choice. Measured in **your** deck shape — analysis dot card, `.control run`, no
`-r`, i.e. the shape that gives rc=0:

```
.save v(midnode)
.op
.control
run
if $sim_status ne 0
  echo RUN-FAILED
  quit 1
end
write guard_both.raw
.endc
```

| mode | rc | guard fired | rawfile |
| --- | --- | --- | --- |
| `fold` | 0 | no | written, `Plotname: Operating Point` |
| `preserve` | 0 | no | written, `Plotname: Operating Point` |
| `distinguish` | **1** | **yes** | **absent** |

The failing run is now rc=1 *and* the constants artefact is never created,
because the guard quits ahead of the `write`.

**It works on stock.** Same guard, `.save v(nosuchnode)`, no `casemode` flag
anywhere:

```
stock ngspice-46   rc=1  RUN-FAILED  raw ABSENT   (unguarded: rc=0, 569-byte constants)
ver_50, no flag    rc=1  RUN-FAILED  raw ABSENT   (unguarded: rc=0, 592-byte constants)
```

So you can emit this into every deck you generate, against every ngspice you
support, today.

One byte-count note while you are calibrating: the constants file on this build
is round 1's 570 bytes by default and 592 with `casemodewrite` set, the
difference being the 22 bytes of the §2 header line — `Option: casemode=fold`,
written on the bogus file exactly as on a good one. The header records the
mode, not the health of the run, and it is not a signal that anything went
right. If you size-check the artefact, size-check it against the setting you
run with.

Three properties to build against, each measured:

1. **Per analysis, last writer wins.** A deck that fails one analysis and then
   succeeds at another reads 0 at the end (`AFTER-BAD=1`, `AFTER-GOOD=0`).
   Read it after *each* run, not once at the end of the block.
2. **It does not exist before the first analysis.** Reading it in a block that
   has not run anything gives an empty string and
   `Error: sim_status: no such variable.` on stderr. Test `$?sim_status` first
   if your block can reach the guard without a run.
3. **A `run` with no analysis to do reads 0.** `sim_status == 0` means "the
   last analysis did not report a failure", not "an analysis produced data".
   For the second question you still have to ask the rawfile.

### What survives from round 1's advice

Unchanged and still correct: **the vector-count sanity check and the two header
checks.** `Plotname: constants` and a `Date:` equal to the build stamp are
still the only two signals a rawfile carries for this failure, they are still
necessary and still not sufficient (the `appendwrite` and `wrdata` shapes
defeat them, round 1's §3), and your plan to implement both plus a
vector-count floor is the right one. One adjustment to the floor: see §5 — a
deck that saves exactly one signal writes **two** variables today.

---

## 2. `Option: casemode=<mode>` ships — question 1, answered yes, opt-in

It is in the working tree and measured. **You have to ask for it: `set
casemodewrite` before the `write`.** One word in the `.control` block your
generator already writes:

```
.control
  op
  set casemodewrite          <- the header records the mode
  set filetype=ascii
  write $outfile
.endc
```

With it unset, the header this build writes is byte for byte the header every
ngspice has ever written — `cmp`-identical, both formats, against a binary
built from the commit before the change. Why it is off by default is the last
part of this section, and it is worth your reading before you hand one of
these files to anybody else.

With it set, `raw_write()` writes the line immediately
after `Plotname:`, valued from the mode **in force** (the one `curcasemode`
reports), so a `set casemode=` typed in a `.control` block after the deck was
read cannot make it lie:

```
Title: * hdr -- a clean run whose raw header should carry the mode
Date: Fri Aug 14 15:10:16  2026
Command: ngspice-46+, Build Fri Aug 14 20:52:09 UTC 2026
Plotname: Operating Point
Option: casemode=preserve
Flags: real
No. Variables: 3
```

Measured across all three modes and with no flag at all (→
`Option: casemode=fold`), in the ASCII and the binary format alike — the header
is text in both. Stock `ngspice-46` writes no such line, which is your
"absence is not fold" case.

Your `read_dataset` analysis was right and is now the shipped shape: it is one
added branch on a key your reader already ignores. Confirming the two round-1
claims it rests on, re-measured:

```
# the file's header carries: Option: casemode=preserve

$ printf 'load h.raw\necho READBACK=$casemode\nquit\n' | ngspice-46 -p -n
READBACK=preserve                     <- an unmodified ngspice-46 parses it

$ printf 'load h.raw\necho READBACK=$casemode\necho EFFECTIVE=$curcasemode\nquit\n' \
    | build-ver_50/src/ngspice -p -n
READBACK=preserve                     <- the file's record
EFFECTIVE=fold                        <- the session's truth, separately labelled
```

So the file can be read back by the binary you already ship against, and a file
cannot steer the session that reads it (`doc/codex/issues/0061`).

**Three caveats, all of them ours to state rather than yours to discover.**

- **The `-r` batch path does not carry it.** `ngspice -r out.raw deck.cir` is a
  different writer and writes no `Option:` line. Measured. If your generated
  decks name their rawfile from `.control write`, you have the line; if you
  ever switch to `-r`, you lose it. (You have another reason not to use `-r`:
  §1.)
- **Absence is not `fold`.** Any older ngspice, and the `-r` path above. Treat
  a missing line as *unknown* and fall back to the probe.
- **Match the `Option:` key, not the line number.** The writer's own line is
  under `Plotname:` and reads `Option: casemode=preserve`. A value that came
  out of a file the session had loaded is re-emitted further down the header
  and reads `Option: casemode = preserve` — same key, same value, spaces
  around the `=` because that is the spelling the option round-trip has always
  used. Scan every `Option:` line, split on the first `=`, trim both halves;
  that is what ngspice's own reader does. Your `read_dataset` branch is on the
  key already, so this costs you nothing, but a line-5 check would miss the
  second shape.
- **A loaded-then-rewritten file keeps the mode its own file recorded**, and
  does not take the re-writing session's — corrected 2026-08-14, and the
  opposite of what the first version of this section said. Load a `preserve`
  file in a folding session, `write` it back out, and the copy carries one
  `casemode` line saying `preserve`, above variables still spelled `v(In)` and
  `v(MidNode)`. That is the case a "load, tidy, re-write" tool hits, and it now
  behaves. Two edges remain: a file that recorded *nothing* (the default, and
  every older ngspice) leaves nothing to keep, so a re-write with
  `casemodewrite` set records the re-writing session — "absence is not fold",
  one step on; and if you want provenance to survive a copy, write the
  original with the variable set.
- **`Option: casemodewrite` in a header does not turn the writer on.** The
  variable is this session's request about what it writes, and a file the user
  loaded does not get to answer it — `doc/codex/issues/0061`, whose rule is
  that a loaded plot may answer a question *about* the session and may not
  decide what it does. `set casemodewrite` in the deck is the only thing that
  turns it on. (It was reachable from a header for one day; it is not now, and
  a deck asserts it.)

You said that if the answer were "not deciding yet" you would ship your
heuristic off by default. The answer is yes, so: you can drop the Xyce-style
capital-hunting heuristic for files that carry the line, and keep it only for
files that do not — which, until the default flips, means files your own
generator wrote with `casemodewrite` set.

### Why it is opt-in, and the one thing to know before you share such a file

The line is safe to *read* on every ngspice that exists — that is the
measurement above, and it has not moved. The hazard is one step further on,
and it is in the **released** binary, not in ours.

Loading a file that carries the line puts a `casemode` key into that session's
loaded-plot environment. On `ngspice-46` as released, `unset casemode` in that
session frees a node it leaves linked, and the `unset` itself returns
cleanly — so the crash lands on **the next command, whatever it is**. Measured
on `/usr/local/bin/ngspice`, ASCII and binary alike:

```
$ printf 'load withline.raw\nunset casemode\n<CMD>\nquit 0\n' | ngspice-46 -p -n
    <CMD> = set                  rc=139
    <CMD> = echo $casemode       rc=139
    <CMD> = display              rc=139
    <CMD> = print v(In)          rc=139
    <CMD> = unset casemode       rc=139
```

The same five sequences are `rc=0` on this build: the defect is
`doc/codex/issues/0067`, fixed here and in nothing released.

**For you this is a non-issue and you should turn the variable on.** You
control both ends — your generator writes the file and your reader reads it —
your reader parses the key rather than unsetting it, and the ngspice you ship
against will carry the fix. Nothing in the sequence above happens by accident:
it takes an explicit `unset casemode`.

It matters when a file leaves your hands. A raw you wrote with the line and
gave to somebody running a stock `ngspice-46` is a file that will kill their
session if their script unsets `casemode` after loading it — which is exactly
what a script written against *our* documentation of `casemode` might do. Say
so if you publish such files, or ship them without the line until the fix is
in a release. That is the whole reason we did not simply put the line in every
file: a file we write must not become a crash trigger for a simulator that
cannot be fixed unless somebody asked for it. The default flips once 0067 has
shipped.

---

## 3. `distinguish` keeps `.save` byte-exact, permanently — question 2

**It is a contract, not an interim state.** You can word your UI warning as
permanent.

The place it is written down is `doc/claude/decisions/0001-distinguish.md`
decision 5, and the form matters for how you word the warning: that decision is
a list of guarantees **`preserve` makes that `distinguish` deliberately
withdraws**. It is not a list of things `distinguish` has not got round to. The
withdrawn list names, among others:

> Two spellings of a name typed at the control language: `alter`, `show`,
> `@dev[param]`, `let`, `print`, **`save`**. The typed name must now match the
> stored spelling exactly.

A stored folded `.save` card being fatal under `distinguish` is that clause
doing its job. The mode exists so that `Out` and `OUT` can be two nets; a
`.save` that folded would be asking for both of them and being given one, which
is the thing the mode is for refusing. Making it fold would not be a bug fix,
it would be a withdrawal of the mode.

Re-measured, so the contract has a number beside it:

| mode | `.save v(midnode)` vs net `MidNode` |
| --- | --- |
| `fold` | rc=0 → `v(midnode)` |
| `preserve` | rc=0 → `v(MidNode)` — `doc/codex/issues/0056` |
| `distinguish` | rc=1 — the contract |

Your plan — select `preserve`, make `distinguish` opt-in per simulator profile
with a permanent warning — is the plan we would have recommended.

---

## 4. A case collision is now reported, in all three modes — question 3

Shipped, in the working tree, and it is the answer to the finding you called
*"the one signal a schematic editor could relay to a user who drew `Out` and
`OUT` and got one net"*. Deck:

```
V1 in 0 dc 1.5
R1 in Out 1k
R2 out 0 1k
```

Measured, one line per colliding pair, on `cp_err` so a deck can capture it
with `>&`:

```
-D casemode=fold          Warning: node names 'Out' and 'out' differ only in case and name one node (casemode=fold)
-D casemode=preserve      Warning: node names 'Out' and 'out' differ only in case and name one node (casemode=preserve)
-D casemode=distinguish   Warning: node names 'Out' and 'out' differ only in case and name two nodes (casemode=distinguish)
no flag at all            Warning: node names 'Out' and 'out' differ only in case and name one node (casemode=fold)
stock ngspice-46          (nothing)
```

Four things about it that a consumer wants:

- **All three modes**, which is why it is useful to you: the modes you will
  actually ship under are `fold` and `preserve`, and those are the two where
  the collision silently merges. A `distinguish`-only diagnostic would have
  been useless here and was rejected for that reason.
- **The sentence names the outcome, not the mistake.** `one node` / `two
  nodes`. Under `distinguish` two case-variant nets are the feature, so the
  line tells that user what they got rather than telling them off.
- **Once per colliding pair per parse.** `Out` on forty cards and `out` on
  forty more is one line. Note the contrast with the near-miss warning in §6:
  measured on the two-simulation deck shape, the near-miss doubles and this one
  does not.
- **It answers your `.include`d-PDK problem**, which is the half your own
  netlister could never have covered: the detection is at the parser's node
  symbol table, so it sees every card the deck reads, including the ones a
  library brought.

**What it does not cover, stated so you do not over-trust it.** `.model`,
`.subckt`, `.global` and `.param` names spelled two ways stay silent — they are
not interned in one place the way node names are. Under `fold` a collision
inside a **subcircuit body** is also silent, and so is one whose second
spelling appears only on an `X` card's actual or only inside an arithmetic
expression; under `preserve` and `distinguish` the first of those three does
report. `doc/claude/decisions/0018-node-name-collision-report.md` enumerates
them.

**One thing to check on your side:** the `distinguish` experimental banner's
*text* changed with this work, because it names which silences remain. If
anything of yours matches on that string rather than on its prefix, it will
stop matching.

---

## 5. R3 is already filed — `doc/codex/issues/0064`, with a root cause

You found it independently; we had filed it on 2026-08-13 and had not told you,
which is our miss. Your reproduction is now in that issue, and your isolation
of the trigger is sharper than ours was, so it is the one the file now uses.

**The cause.** `ft_evaluate()` (`src/frontend/evaluate.c:80-84`) renames its
result to the parse node's own text whenever the result is a **single** vector.
That is right for an expression — `print v(a)+v(b)` should be labelled
`v(a)+v(b)` — and wrong for a wildcard, whose text is not a name for what it
matched. The size condition is `!d->v_link2`: `findvec_all()` chains two or
more matches together, so two or more are exempt and exactly one is not. A bare
`write` substitutes `all`, so your deck need never type a wildcard to get here.

The second column is `com_write()` behaving correctly given the corrupted name:
it looks for the plot's scale by name, no longer finds it, and prepends a
second copy under the real name. Hence one vector, two columns, the same
numbers twice.

Re-measured with your decks, plus one row you did not have:

```
.save v(In)                      -> 0 v(In) voltage      | 1 v(all) voltage
.save v(In) / .save v(MidNode)   -> 0 v(In) voltage      | 1 v(MidNode) voltage
.save v(In) v(MidNode)           -> 0 v(In) voltage      | 1 v(MidNode) voltage
stock ngspice-46, .save v(In)    -> 0 v(in) voltage      | 1 v(all) voltage
```

`No. Variables:` is 2 in every row, which is the sharp form of it.

**Two things this means for you now**, since it is open and not fixed:

- Your filter-by-name is, unfortunately, the right defence for the moment. In
  practice you will only ever see `v(all)`, because a bare `write` substitutes
  the token `all` for itself and that is what a generated deck does; if you
  ever write an explicit wildcard, the rename follows the token you typed —
  measured, `write f.raw allv` on the same plot gives `v(allv)`. The recognised
  set is `all`, `allv`, `alli`, `ally`.
- **Your vector-count floor has to account for it.** A deck that saves exactly
  one signal writes two variables. Expect n, or n+1 when n is 1.

Not fixed in this round because `ft_evaluate()` is on the path of every
expression in the control language, so it wants the whole regression suite
behind it and a deck of its own. The intended fix is to withhold the rename
from the wildcard tokens rather than to change `com_write()`.

---

## 6. R4: the warning is per *simulation*, and your deck is two simulations

`doc/codex/issues/0057`'s contract is "at most one line per token per
simulation", and it is holding exactly as written. Your deck runs the analysis
twice. Measured, one deck per row, `-D casemode=distinguish`:

| deck shape | `Doing analysis` | near-miss lines | rc |
| --- | --- | --- | --- |
| `.op` card, no `.control`, no `-r` | 1 | 1 | 0 |
| `.op` card, no `.control`, `-r out.raw` | 1 | 1 | 1 |
| no analysis card; `op` inside `.control` | 1 | 1 | 1 |
| **`.op` card *and* `.control run`** — your `ctl_fail.cir` | **2** | **2** | 0 |
| `.op` card and `.control` with two `run`s | 3 | 3 | 0 |

The line count equals the simulation count in every row. The second simulation
in row 4 is the same `ft_savedotargs()` re-run that §1 is about — R1 and R4 are
one deck shape seen from two sides.

**How to avoid it, in one edit:** do not carry both an analysis dot card and a
`.control run`. Either put the analysis command inside the `.control` block
(row 3) or keep the dot card and drop the block (row 1). **Row 3 is the one to
pick**, because the same edit gets you rc=1 back as well — it is the row where
one mistake produces one warning *and* a failing exit status.

We are not making the memory outlive the simulation. It would silence your
second line, and it would also silence the second *run* of a deck deliberately
re-run after an `alter`, which is a correct deck asking the same question twice
and entitled to the same answer twice.

For a consumer, the durable rule is that "one line per mistake" is not
something the simulator can promise, because the deck decides how many
simulations it runs. Deduplicate on the quoted token — the message carries it
in single quotes for exactly this reason.

---

## 7. R5 is yours, and R2 and R6 are recorded

**R5 — the probe must run from the deck's directory.** You found it, we did not,
and it is now in two places on our side: a new subsection of
`doc/claude/casemode-distinguish-guide.md` §9, and an addendum to
`doc/codex/issues/0060`. Your two properties are **intended, not incidental**,
and the issue now says so: `curcasemode` reads the latch, and the latch is
written after the `-D` getopt loop and after `.spiceinit` is sourced, so
reporting the effect rather than the request is the whole reason the variable
exists. The clean negative on an old binary is its acceptance criterion 4.

The rule we should have written is not "probe with the real run's argv" but
**"probe with the real run's argv *and* its cwd"**. Re-measured:

```
cwd = probe/  (holds .spiceinit saying fold, and deck.cir)
  probe says fold        the real run writes v(in)        agree
cwd = repro2/ (no .spiceinit), deck at probe/deck.cir
  probe says preserve    the real run writes v(in)        DISAGREE
```

One thing worth adding, because it is the best argument this batch has produced
for reading the header even when you have probed: **the disagreeing run above
wrote `Option: casemode=fold`** — with `casemodewrite` set, which is the
argument for setting it. The header caught the wrong-cwd probe. If you compare
the two, a silent mislabelling becomes a detectable discrepancy — and that is a
check we would not have thought to suggest without R5.

Both your notes are in the guide: the cwd rule, and that `write` inside
`.control` is cwd-relative rather than deck-relative.

**R2 — recorded as corroboration in `doc/codex/issues/0059`,** with your
four-row table. It does not change the withdrawal, which you were not asking it
to, and it does move the priority: your measurement is what settles that the
failing shape needs no `casemode` flag and no unusual command, and that the two
header tells are — from a consumer's position — the only two signals in
existence for it. One footnote: your table's rc=0 rows and 0059's rc=1
transcripts are not in conflict; your `absent.cir` carries an `.op` card, so it
is §1's second epilogue arm.

**R6 — noted in `doc/codex/issues/0067` as independent confirmation.** Round 1
flagged that issue as the newest thing in the batch and therefore the
least-scrutinised item in it; an outside party reproducing it on a released
binary from their own deck is the scrutiny that flag was asking for. Your
client-side rule — never emit a `set` and an `unset` of the same simulator
variable into a generated `.control` block — is a good mitigation, and the
issue now records it, because its other two arms (`unset curplot`, and `load`
followed by `unset <key>`) are also shapes a *tool* writes rather than a person.

**Finding 6** (a misspelled `-D` *name* is a silent no-op) stays unfiled, as you
asked. It is `doc/codex/issues/0048`'s neighbourhood if it ever matters again.

---

## 8. Re-running the evidence

`./repro/run_all.sh [case-capable-ngspice] [baseline-ngspice]` is round 1's and
still runs. Your `repro2/run_round2.sh` runs unmodified against this tree and
reproduces all six of your findings; we ran it before writing anything above,
and every number in your reply came back identical.

Two decks in `repro2/` now measure things they did not set out to: `ctl_fail.cir`
is the R1/R4 two-simulation shape, and `absent.cir` is 0059's Impact reached
through the rc=0 arm. Both are cited by issue number from our side, so they
will not drift apart from the issues.

---

## Summary for a client program

| do | why |
| --- | --- |
| **guard with `$sim_status`, not `rc`** | rc reports the last epilogue arm, not the analysis; the guard fires before the artefact exists, and works on stock |
| read `$sim_status` after *each* run | per analysis, last writer wins; absent before the first analysis |
| **probe with the real run's argv *and* its cwd** | `.spiceinit` is searched beside the deck, a `-p` probe searches cwd |
| probe with `echo $curcasemode` | the only thing that sees `preserve`; fails loudly on old binaries |
| read `Option: casemode=` from the raw header | now written; also cross-checks the probe. Absent ≠ `fold`, and the `-r` path does not write it |
| match it as an `Option:` **key**, anywhere in the header | the writer's line is under `Plotname:`; a value kept from a loaded file is re-emitted lower down, spelled `key = value` |
| `set casemodewrite` in the deck, never as an `Option:` in a file | the gate is the session's request; a loaded header cannot open it |
| keep `Plotname: constants` and build-stamp-`Date:` checks | still the only two signals for the artefact, still not sufficient alone |
| expect n+1 variables when the deck saves exactly one | `doc/codex/issues/0064`, open |
| do not carry an analysis dot card *and* a `.control run` | two simulations: two warnings, and rc reports the second |
| select `preserve`; warn permanently if you offer `distinguish` | the folded-`.save` contract is permanent by decision, not interim |
| relay the new collision warning to the user | fires in all three modes, once per pair, including for `.include`d libraries |
| stop reading `$casemode` | reports the request; lies on a featureless build |
| avoid `set`/`unset` of simulator variables in generated control blocks | aborts, on released ngspice too |
