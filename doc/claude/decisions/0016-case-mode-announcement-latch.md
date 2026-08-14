# Decision 0016 — what the case-mode announcement is compared against, `doc/codex/issues/0058`

## Status

Accepted, 2026-08-13, branch `ver_50`. Settles the one question
`doc/codex/issues/0058` left open after it was closed: the issue's acceptance
criterion 1 and the code that satisfied it stated two different rules, which
agree on every row of the issue's own table and disagree on exactly one
sequence. This record picks the rule, records the measurement that separates
them, and names the deck cases that now pin it.

**No computed value moves.** The whole subject is which of two `Warning:`
lines is printed and when. `doc/claude/scripts/case_differential_sweep.py` is
the check that says so, and it was run for this record rather than argued
about; the result is in Evidence.

## Context

`set_case_mode()` (`src/frontend/inpcom.c:1164`) establishes the identifier
case policy for one netlist read and, in two of its four arms, announces it:
the experimental banner for `distinguish`, the unknown-value warning for
anything it does not recognise. Its only caller is `inp_readall()`, which runs
for every input file — the system `spinit`, a `.spiceinit`, the deck, every
`source`, every `altermod <model> file`, and the deck the XSPICE auto-bridge
writes itself — so before `0058` the same deck under the same flag announced
1, 2 or 3 times depending on which init files the invoking user happened to
have. `0058` fixed that by splitting establishment from announcement and
putting the announcement behind a latch.

The latch is where the two rules part. `0058` criterion 1 said the
announcement fires when the outcome

> differs from the outcome last **announced**, and not otherwise

while `case_outcome_is_new()` (`inpcom.c:1138`) re-latches on **every** call,
announcing or not, so what it compares against is the outcome of the previous
**read**. Both readings produce identical output on all seven rows of the
issue's assertion table, which is why the difference survived to here.

They diverge on one shape only: a run that leaves an announceable outcome for
a silent one and comes back. `repro/reads/switcher_r.cir` is that run —
`distinguish`, then `preserve`, then `distinguish` again, three reads in one
process:

```
$ SPICE_SCRIPTS=. ngspice -b -n -D casemode=distinguish switcher_r.cir 2>&1 \
    | grep -c "is experimental"
2
$ SPICE_SCRIPTS=. ngspice -b -n switcher_r.cir 2>&1 \
    | grep -c "is experimental"          # fold -> preserve -> distinguish
1
```

Two, where the criterion as written owes one. Nothing in the tree decided
which is right; the code comment stated its choice openly and the criterion
stated the other, and that is the gap this record closes.

---

## Decision 1 — the latch holds the outcome of the previous read

**Decided: `case_outcome_is_new()` compares the outcome of this
`set_case_mode()` call against the outcome of the previous call and re-latches
either way, announcing or not. The announcement therefore marks a *transition*
of the effective outcome, and a run that returns to `distinguish` from a
silent mode is told again.**

The code already does this; what changes is that it is now the rule rather
than an unexamined reading of one, and `0058` criterion 1 is reworded to say
so (decision 2).

Three reasons, in the order they decide it.

**It is the only one of the two that keeps the promise `0058` was filed to
make.** The finding was that the announcement count was set by the reader's
file layout rather than by the run. Under this rule the count is exactly the
number of times the run *enters* an announceable outcome — a property of what
the run did, invariant under how many init files, `.include`s or auto-bridge
decks happened to be read. Under the last-announced rule the count is the
number of entries not preceded by another announcement, which is neither a
property of the run nor a smaller number in any case anyone can name.

**The alternative makes the announcement depend on history the reader cannot
see.** A last-announced latch remembers only what was said, so a silent read
in between is invisible to it, while a *noisy* one in between is not.
Measured on a binary carrying that rule (Evidence, RED), the same return to
`distinguish` is silent when the intervening read was `preserve` and announced
when the intervening read was a misspelling — two runs that differ only in an
unrelated typo three reads earlier, and a warranty that appears in one and not
the other. There is no user-facing account of that difference, which is the
strongest thing that can be said against a rule.

**The unknown-value arm needs the transition reading, and `0058` already knew
it.** The issue rejected a mode-keyed latch because a misspelling establishes
`NG_CASE_FOLD` — byte identical to the fold already in force — and the warning
must still print (`switcher_b.cir`, criterion 1's last row). That is an
argument that the announcement tracks the *outcome as read* and not the
message history, and on the rejected rule it comes apart. Measured on both
binaries, `repro/reads/switcher_bb.cir` — `bogus`, `preserve`, `bogus`, three
reads, two files misspelling the mode:

```
this tree      : grep -c "unknown casemode"  ->  2
last-announced : grep -c "unknown casemode"  ->  1
```

The second misspelling is a real user error in a second file, and the rejected
rule does not report it, because the silent `preserve` read between them
cannot update a latch that only announcements write.

Rejected: **the last-announced latch**, for the three reasons above. It is a
defensible reading of the English and it is what criterion 1 literally said,
so it is rejected on the merits and not on a technicality; the criterion moves
to the rule, not the other way round.

Rejected: **say it once per process.** `0058` rejected it and the reason
stands: one process can hold two modes in turn — `repro/reads/switcher.cir`
has `op1`'s net folded and `op2`'s preserved — and the read that changes into
`distinguish` owes the banner.

Rejected: **gate on `comfile`, the way `print_compat_mode()` does.** That buys
once per deck, not once per run, and it would still announce twice for a deck
that `source`s another.

Rejected: **suppressing the banner behind a variable** (`set nocasewarn` or
similar). Nothing has asked for it, an experimental-mode warranty is the last
diagnostic that should be silenceable, and once the count is one per entry
there is nothing to silence.

## Decision 2 — the criterion is reworded, and the sequence that discriminates is asserted

**Decided: `0058` acceptance criterion 1 is reworded from "the outcome last
announced" to "the outcome of the previous read", dated and marked as a
rewording in place; and criterion 4's deck,
`tests/regression/casedist/casemode-announce-report.cir`, gains the two cases
that tell the rules apart — `VIA` (a `preserve` read announces nothing) and
`RETURN` (the `distinguish` read after it announces once).**

A criterion that no test discriminates is a criterion two implementations
satisfy, which is how the tree arrived here. `VIA` and `RETURN` are the
smallest pair that fixes that: run against a binary carrying the rejected
rule, `RETURN` reads `NONE` and every other case in the deck still passes
(Evidence, RED), so the deck now fails for exactly the difference this record
decides and for nothing else.

The rewording adds an obligation and removes none. Under the old wording the
second banner in `switcher_r.cir` was owed nothing; under the new one it is
owed, and a deck asserts it. The issue keeps its `Closed` status: the code it
closed on is the code that stays.

Rejected: **changing the code to the last-announced rule.** That is the same
question as decision 1 and loses for the same reasons; it would also have to
add a "nothing announced yet" state, since criterion 1's own initial value
(`NG_CASE_FOLD`, none) is an outcome that was never announced.

## Decision 3 — the latch is per process and is cleared on `ngSpice_Reset()`

**Decided: the latch is two file statics in `inpcom.c`
(`ng_case_last_outcome`, `ng_case_last_unknown`), and
`inp_case_announce_reset()` clears them from `totalreset()`
(`src/sharedspice.c`), so a host program that resets the simulator and
re-loads a `distinguish` circuit is told the mode's warranty again rather than
once per process lifetime.**

This is `0058`'s Root Cause requirement and CLAUDE.md's rule that anything
touching global state must survive repeated `ngSpice_Reset` in the shared
build. It is stated here because the statics are the only new global state the
fix introduces, and because the standalone binary can never exercise it:
`build-ver_50` is not `--with-ngshared`, so no deck in `make check` reaches
that line. Evidence records the shared-build run that does.

`ng_case_mode` itself is deliberately **not** reset there, and needs no reset:
every `inp_readall()` rewrites it from `cp_getvar()`. `totalreset()` does not
touch `cp_err`, which `ngSpice_Init()` set to `stderr`
(`src/sharedspice.c:921`), so the announcement still reaches the host's
`SendChar` callback after a reset, prefixed `stderr ` by `sh_fputsll()`.

The statics are named for what they hold — the outcome of the last call — and
not `ng_case_announced`, which is what they were called when the fix first
landed and is precisely the reading decision 1 rejects. A latch named for the
announcement invites the code to be read as implementing the other rule, which
is how the two got out of step.

## Evidence

All measurements at `720c8743a` plus the working tree's uncommitted work, on
`build-ver_50/src/ngspice`, rebuilt between every arm — never restored with
`cp -p`, whose preserved mtime makes `make` skip the rebuild and the next
measurement report the previous binary.

### The two rules, measured against each other

The rejected rule was built and run, not argued about. It is **not** obtained
by latching on `case_outcome_is_new()`'s return: `preserve` differs from
`distinguish` and so is "new", yet announces nothing, so a latch keyed on
newness still records it and behaves exactly like the accepted rule. That was
measured first, and it is the reason the counterfactual has to move the latch
into the two printing arms:

```
  first attempt: if (is_new) { latch }      switcher_r.cir -> 2   (no change)
  the real rule: latch inside the two fprintf arms
                                            switcher_r.cir -> 1
```

That is worth recording on its own: the last-announced rule cannot be
expressed as a property of the outcome alone, because the latch has to know
which arm printed, so it duplicates the arm structure it is meant to gate.

With it built, from `repro/reads/`, all under
`SPICE_SCRIPTS=. ngspice -b -n -D casemode=distinguish`:

| run | reads | this tree | last-announced |
| --- | --- | --- | --- |
| `plain.cir` | dist | 1 | 1 |
| `switcher_d.cir` | fold → dist | 1 | 1 |
| `switcher_b.cir` (unknown-value count) | fold → bogus | 1 | 1 |
| `switcher_r.cir` | **dist → preserve → dist** | **2** | **1** |
| `switcher_rb.cir` | **dist → bogus → dist** | **2** | **2** |
| `switcher_bb.cir` (unknown-value count) | **bogus → preserve → bogus** | **2** | **1** |

Rows four and five are decision 1's second reason, measured: under the
rejected rule the same return to `distinguish` is announced or not according
to whether the read three lines earlier was a typo or a legal silent mode.
Under the accepted rule both are 2, because both are a change of outcome. Row
six is decision 1's third reason on the other arm, and is the one with a cost
a user pays rather than a tidiness argument: a second file that misspells the
mode is not warned about at all.

### The deck

`tests/regression/casedist/casemode-announce-report.cir` gains `VIA` and
`RETURN`. RED, against the last-announced binary — one line moves, and every
other case in the deck still passes, so the deck fails for this difference and
for nothing else:

```
@@ -22,7 +22,7 @@
 CASEMODE-VIA-CAPTURE-READ
-CASEMODE-RETURN-ONCE
+CASEMODE-RETURN-NONE
FAIL: casemode-announce-report.cir
1 of 1 test failed
```

GREEN, same deck, same `.out`, never edited after it was written:

```
PASS: casemode-announce-report.cir
1 test passed
```

### The shared build

`build-shared/` was configured `--with-ngshared` for this record, because no
deck in `make check` can reach `inp_case_announce_reset()` and the previous
receipt had only compile-checked it.
`doc/claude/feedback/ngspice_upstream/repro/shared/casemode_reset_probe.c` is
the probe, and its header carries the build line. The mode variable is set
once and the same deck is read three times:

```
READ1 1  READ2 0  READ3-AFTER-CLEAR 1
```

`READ2` is what the latch buys, `READ3` is what `totalreset()`'s call buys,
and phase 1's banner arrives on the host's `SendChar` callback as
`stderr Warning: casemode 'distinguish' is experimental. …` — criterion 3's
`cp_err` move working in the shared build, where `cp_err` is the `stderr`
`ngSpice_Init()` set and `sh_fputsll()` routes by comparing against it.

The clear is called directly rather than through `ngSpice_Reset()` because the
path `totalreset()` sits on **does not work on this tree**, and does not work
in a way that has nothing to do with `casemode`. The same probe's `--reset`
mode, measured with the `inp_case_announce_reset()` line compiled in and again
with it removed, behaves identically both ways: after `ngSpice_Reset()`,
`ngSpice_Command("echo … $casemode")` produces no output at all — the command
does not run — and the next `ngSpice_Circ()` segfaults:

```
Program received signal SIGSEGV
#0  CKTmodCrt ()      #4  inp_dodeck ()
#1  INP2V ()          #5  inp_spsource ()
#2  INPpas2 ()        #6  create_circbyline ()
#3  if_inpdeck ()     #7  ngSpice_Circ ()
```

So the line in `totalreset()` is right, is exercised by the probe, and is not
what crashes; what a host cannot do today is observe the re-announcement end
to end. That is a pre-existing defect in the shared build's reset path and is
owed an issue of its own — it is recorded here because it is the reason this
decision's evidence takes the shape it does, and not as a finding about
`casemode`.

### Nothing computed moves

`doc/claude/scripts/case_differential_sweep.py`, all 332 decks under `tests/`,
run twice: once on this tree and once on a binary built from `HEAD`'s
`inpcom.c`, which is the tree without any of `0058`'s work. Same machine, same
flags, rebuilt between the two — never restored with `cp -p`.

```
this tree : 332 decks: DIFF=67, NUM-DIFF=1, OK=261, SKIP=3
pre-0058  : 332 decks: DIFF=66, NUM-DIFF=1, OK=262, SKIP=3
```

The two verdict sets differ by exactly one row,
`tests/regression/case/alter-rebin-case-lower.cir`, whose detail on both sides
of the family is `stdout reordered only` — the sweep's name for two stdouts
with the same lines in a different order. It is **flaky**, measured on the
unchanged pre-`0058` binary:

```
--filter alter-rebin, pre-0058 binary : DIFF=3 OK=1, then DIFF=4, then DIFF=4
--filter alter-rebin, this tree       : DIFF=4, then DIFF=4
```

so the row that separates the two full sweeps is run-to-run noise on a binary
that was not changed, and the fixed tree is the *stabler* of the two. No other
row moves, no `NUM-DIFF` appears or disappears, and the one `NUM-DIFF`
(`save-undef-report.cir`, another crew's deck) is identical in both.

The result is not surprising and the sweep says why: `grep -rl 'set casemode'
tests/` finds exactly one deck, the one this issue added, so no other deck in
the tree can print either diagnostic under the sweep's arms at all. The
sweep's own DIFF row for that deck is the documented lower-case-sub-deck
collateral `bsource-node-case-report.cir`'s header describes, shared with five
sibling report decks, and is a `no such file or directory` shape rather than a
numeric one.

## What this decision does not decide

- **The text of either diagnostic.** Unchanged, byte for byte.
- **Which arms announce.** `fold` and `preserve` stay silent; that is
  `0001-distinguish.md`'s and is not reopened here.
- **Whether the mode should be readable as a variable.**
  `doc/codex/issues/0060` is open on exactly that and is deferred on the
  variable's name, which no record in the tree has yet chosen.
- **Anything about `.include` / `.lib`.** They recurse into `inp_read()`
  (`inpcom.c:1821`, `:599`) and not into `inp_readall()`, so they never
  reached `set_case_mode()` at all; their count was 2 before the fix for the
  `spinit`/deck reason and is 1 after, like every other run.
