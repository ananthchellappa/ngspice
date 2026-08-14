# Issue: The `casemode` Diagnostics Fire Once Per Input File Read, Not Once Per Run

## Status

Closed on `ver_50`, 2026-08-13; see Resolution. Found from outside the tree: a client program — xschem, adding
case-preserving signal names to a schematic editor that generates decks and
reads back the raw file — hit it while wiring `preserve` in, and handed it over
as finding 7 of `doc/claude/feedback/ngspice_upstream/FINDINGS.md`. Every
number below was **re-measured** 2026-08-13 at `720c8743a` against
`build-ver_50/src/ngspice` (`ngspice-46+`, build stamp
`Wed Aug 12 19:28:37 UTC 2026`) rather than copied from that file; where the
re-measurement went further than the finding, it is said so. Every deck quoted
below is present in the working tree under
`doc/claude/feedback/ngspice_upstream/repro/` and is named at the point it is
used; the new ones for this issue are in
`repro/reads/` and `repro/spiceinit-fold/`.

Pre-existing, and it arrived in two commits rather than one: the unknown-value
warning came in with `set_case_mode()` itself (`424ebaf75`), the `distinguish`
banner later (`d06326d05`). Not a regression.

Visible in two of `set_case_mode()`'s four arms — the `distinguish` arm, and
the fall-through arm that any unrecognised `casemode` value takes. `fold` and
`preserve` are silent.

## Summary

`set_case_mode()` (`src/frontend/inpcom.c:1085`) does two jobs in one function:
it establishes the case policy for a netlist read, and in two of its four arms
it announces it.

```c
    else if (cieq(mode, "distinguish")) {
        ng_case_mode = NG_CASE_DISTINGUISH;
        /* ...17 lines of comment elided, inpcom.c:1100-1116... */
        fprintf(stderr,                                  /* inpcom.c:1117 */
                "Warning: casemode 'distinguish' is experimental. ...");
    }
    else
        fprintf(stderr,                                  /* inpcom.c:1126 */
                "Warning: unknown casemode '%s', using 'fold'\n", mode);
```

Its only caller is `inp_readall()` (`src/frontend/inpcom.c:1178`), which reads
*every* input file, not only the deck. The system `spinit` is one such read and
a `.spiceinit` is another, so both diagnostics repeat once per read, and the
count is not a fixed duplicate — it is a function of which files this
particular user has.

Measured, from `doc/claude/feedback/ngspice_upstream/repro/`, same flag
throughout, on the working-tree `divider.cir` and `count/deck.cir`:

```
$ SPICE_SCRIPTS=. ngspice -b -n -D casemode=bogus divider.cir 2>&1 \
      | grep -c 'unknown casemode'
1
$ ngspice -b -n -D casemode=bogus divider.cir 2>&1 | grep -c 'unknown casemode'
2
$ ngspice -b    -D casemode=bogus count/deck.cir 2>&1 | grep -c 'unknown casemode'
3
```

| run | files read | count |
| --- | --- | --- |
| `SPICE_SCRIPTS` at a directory with no `spinit`, `-n` | deck | **1** |
| normal, `-n` | `spinit` + deck | **2** |
| `count/deck.cir`, no `-n`, with a `.spiceinit` beside it | `spinit` + `.spiceinit` + deck | **3** |

All three rows of the finding's table reproduce exactly. The third row's
`.spiceinit` (`repro/count/.spiceinit`) sets nothing at all; its only job is to
be a third file, and adding `-n` to that same command drops the count back to
2, which is what attributes the third line to it and to nothing else.

The `distinguish` banner behaves identically on the same three
configurations — 1, 2, 3 — and it is the expensive one: 320 bytes per
emission, measured, of which the run below prints 640.

```
$ ngspice -b -n -D casemode=distinguish case_collision.cir 2>&1 >/dev/null | wc -l
2
$ ngspice -b -n -D casemode=distinguish case_collision.cir 2>&1 >/dev/null | wc -c
640
```

### Which reads, and what does not count

There are exactly four `inp_readall()` call sites in the tree — `grep -rn
'inp_readall' src/` returns these four plus the prototype
(`src/include/ngspice/fteext.h:199`), the definition (`inpcom.c:1168`) and
comments:

| `inp_readall()` call site | `comfile` | `intfile` | what it reads | contributes |
| --- | --- | --- | --- | --- |
| `src/frontend/inp.c:532`, inside `inp_spsource()` | caller's | caller's | every deck and every command file — see the second table | yes, once per call |
| `src/frontend/device.c:1598` | FALSE | FALSE | `altermod <model> file <f>`'s model file | yes |
| `src/xspice/evt/evtcheck_nodes.c:232`, in `expand_deck()` | FALSE | TRUE | the synthetic auto-bridge deck | yes, when a bridge card was generated |
| `src/frontend/nutinp.c:38` | caller's | FALSE | `ngnutmeg` only | not exercised here |

`inp.c:532` is guarded by `if (fp || intfile)` (`inp.c:531`), so the
`inp_spsource()` callers that reach it are the ones that pass an open file or
set `intfile`:

| `inp_spsource()` caller | `comfile` | what it reads | measured here |
| --- | --- | --- | --- |
| `src/frontend/cpitf.c:291` | TRUE | the system `spinit` | yes |
| `src/frontend/cpitf.c:306` | TRUE | `./spinit` — Windows builds only (`#if HAS_WINGUI \|\| __MINGW32__ \|\| _MSC_VER`) | no |
| `src/frontend/inp.c:1985` | TRUE | `com_source()` on a name containing `.spiceinit`; this is the path `main.c:820`'s `inp_source()` takes | yes |
| `src/frontend/inp.c:1995` | FALSE | `com_source()` on an ordinary deck | yes, one per `source` |
| `src/main.c:1508` | FALSE | the command-line deck, or in `-p` mode the temp file even when it is empty | yes |
| `src/main.c:1524` | FALSE | the batch fallback, taken only when nothing was sourced above | not reached in any run below |
| `src/frontend/inp.c:1678`, `:1718` | FALSE | `com_edit()` re-sourcing the edited deck | not exercised here |
| `src/frontend/inp.c:2105` | FALSE, `intfile` TRUE | `ngSpice_Circ()`'s card array — the `libngspice` entry | not measured; this is the binary build |

Two `inp_spsource()` callers do **not** reach `inp_readall()`:
`src/frontend/inp.c:415` (`inp_source_recent()`, behind `com_rset`) and
`src/frontend/inp.c:1644` (`com_mc_source()`). Both pass `fp == NULL` with
`intfile == FALSE` and fall into the reload branch at `inp.c:544`, which
re-uses a saved deck instead of reading one.

The finding stopped at the three-row table; the re-measurement asked what else
pushes it higher, and the answers split two ways.

**`.include` and `.lib` do not.** Both are handled by recursion into
`inp_read()` — `inpcom.c:1741` for `.include`, `inpcom.c:599` for `.lib` — one
level *below* `inp_readall()`, so they add cards without adding a call.
`repro/reads/inc_deck.cir` is `divider.cir`'s circuit with its last resistor
moved into `inc_tail.inc`; `repro/reads/lib_deck.cir` reaches the same resistor
through a `.lib` section of `lib_tail.lib`:

```
$ ngspice -b -n -D casemode=bogus inc_deck.cir 2>&1 | grep -c 'unknown casemode'
2
$ ngspice -b -n -D casemode=bogus lib_deck.cir 2>&1 | grep -c 'unknown casemode'
2
```

Same as `repro/reads/plain.cir`, the same circuit in one file, which also
gives 2. The included and library cards really do land — all three decks print
`v(midnode) = 2.250000e+00`, the 3k/4k divider, which they cannot do without
the third resistor. The finding did not claim otherwise; this is recorded
because it is the first thing a reader will guess. `doc/codex/issues/0060`
acceptance criterion 2 re-measures the same distinction and defers to this
issue for the count.

**Four other things do.** An `ngspice -p` session grows by one per `source`,
without bound (`repro/reads/plain.cir` again, run from that directory):

```
$ printf 'quit\n' | ngspice -p -n -D casemode=bogus 2>&1 \
      | grep -c 'unknown casemode'
2
$ printf 'source plain.cir\nquit\n' | ngspice -p -n -D casemode=bogus 2>&1 \
      | grep -c 'unknown casemode'
3
$ printf 'source plain.cir\nsource plain.cir\nquit\n' \
      | ngspice -p -n -D casemode=bogus 2>&1 | grep -c 'unknown casemode'
4
```

A `.control` block that `source`s a second deck does the same in batch:
`repro/reads/outer.cir` sources `plain.cir` and measures 3.

One of those two `-p` baseline emissions has no user file behind it at all.
With `SPICE_SCRIPTS` at a `spinit`-less directory and nothing sourced, `-p -n`
still prints one, and `spinit` is provably not it, because the
"can't find the initialization file" warning precedes it:

```
$ SPICE_SCRIPTS=. ngspice -p -n -D casemode=bogus </dev/null 2>&1 \
      | grep -nE 'unknown casemode|initialization file'
1:Warning: can't find the initialization file spinit.
2:Warning: unknown casemode 'bogus', using 'fold'
```

`-p` sets `istty` (`src/main.c:1073`), so `src/main.c:1423` skips the stdin
append and leaves the temp file empty, but `src/main.c:1508` sources it
anyway, and by the tables above it is the only `inp_readall()` reachable in
that configuration.

`altermod` reading a model file is a third. `repro/reads/am.cir` runs
`altermod dmod file am.mod` from its `.control` block:

```
$ ngspice -b -n -D casemode=bogus am.cir 2>&1 | grep -c 'unknown casemode'
3                       # spinit + deck + the .mod file
$ ngspice -b -n -D casemode=bogus am.cir 2>&1 | grep -i 'is_now'
is_now = 3.000000e-14
```

The `is_now` line is there because a silent `altermod` failure would give the
same count as a deck with no `altermod` at all: 3e-14 is `am.mod`'s value, not
the deck's 1e-14, so the file really was read.

And the XSPICE auto-bridge is the fourth, which is the worst of them because
the extra file is one ngspice wrote itself. The committed deck
`tests/xspice/casedist/auto-bridge-node-case-split.cir` isolates it, because
it is the same deck either way and only the flag changes whether a bridge card
is generated — under `distinguish` the digital node `A` and the analog net `a`
are two names and nothing is bridged; under `bogus`, which establishes `fold`,
they are one net and a bridge is generated. Run by hand from the build test
directory:

```
$ SPICE_SCRIPTS=. ../../../src/ngspice -D casemode=distinguish --batch \
      auto-bridge-node-case-split.cir 2>&1 >/dev/null | grep -c 'experimental'
2
$ SPICE_SCRIPTS=. ../../../src/ngspice -D casemode=bogus --batch \
      auto-bridge-node-case-split.cir 2>&1 >/dev/null | grep -c 'unknown casemode'
3
$ SPICE_SCRIPTS=. ../../../src/ngspice -D casemode=distinguish --batch \
      auto-bridge-node-case-split.cir 2>/dev/null | grep -i 'v(a)'
v(a) = 0.000000e+00
$ SPICE_SCRIPTS=. ../../../src/ngspice -D casemode=bogus --batch \
      auto-bridge-node-case-split.cir 2>/dev/null | grep -i 'v(a)'
v(a) = 3.300000e+00
```

`v(a) = 0` is the unbridged arm and `3.3` the bridged one, so the extra read
tracks the bridge and not the flag: `expand_deck()` runs only when at least
one bridge card was generated (`evtcheck_nodes.c:1068` returns before `:1080`
when `head` is NULL). A mixed-signal deck that needs a bridge announces once
more than the same deck that does not, for a file the user never wrote and
cannot see.

## Impact

Cosmetic on a terminal. Not cosmetic to the consumer this came from, for three
reasons, in increasing order of weight.

**1. The number tracks the user's init files, not the run.** A tool that
scrapes the log to learn what mode is in effect gets 1, 2 or 3 for
byte-identical decks and flags, depending on whether the invoking user happens
to have a `.spiceinit`. A fixed duplicate would be worse output and a better
contract. Log-scraping is what is left because the two obvious channels are
themselves defective: `$casemode` reports the request rather than the effect
(`doc/codex/issues/0060`), and the raw header carries no case field at all,
which is finding 1's ask and `doc/codex/issues/0061`'s subject. `0056` and
`0057` quote transcripts in which this doubling appears and point here for it;
this issue owns the count.

**2. The banner is a contract statement, not a progress message.** Its 320
bytes say that identity is case sensitive, that three classes of near miss are
reported, and that one class — a deck spelling one net two ways — is not. That
is the mode's warranty, and repeating it *n* times where *n* is set by the
reader's file layout makes it read as noise. `make check` already demonstrates
the end state. `tests/regression/casedist/Makefile.am:23` and
`tests/xspice/casedist/Makefile.am:28` run 23 + 17 decks under
`-D casemode=distinguish`. Counting the banner for each deck, with the
harness's own environment (`ngspice_vpath=$(srcdir)`, and `SPICE_SCRIPTS=.`
for the XSPICE directory), from the corresponding build directories:

```
$ SPICE_SCRIPTS=…/tests/bin ngspice_vpath=$SRC …/src/ngspice \
      -D casemode=distinguish --batch $SRC/vector-unlet-report.cir 2>&1 >/dev/null \
      | grep -c 'experimental'
2
```

but 2 is only the floor, and 16 of the 40 decks are above it:

| directory | decks | banners | bytes |
| --- | --- | --- | --- |
| `tests/regression/casedist` | 23 | **62** | 19840 |
| `tests/xspice/casedist` | 17 | **51** | 16320 |
| total | 40 | **113** | **36160** |

24 decks emit 2. Nine emit 3 — every one of them generates an auto-bridge
(`auto-bridge-vcc-param-case` and `-lower`, `auto-bridge-family-param-case`
and `-lower`, `auto-bridge-vcc-subckt-case` and `-lower`,
`auto-bridge-vcc-param-ambiguous`, `auto-bridge-vcc-nested-subckt-case`,
`auto-bridge-vcc-param-subckt-name`). Seven emit 4 to 6, because they write
sub-decks with `echo` and `source` them to reach the parse with a redirect in
force, and each `source` is a read — the arithmetic is exactly 2 plus
`grep -c '^source ' <deck>`:

| deck | `source` lines | banners |
| --- | --- | --- |
| `vector-probe-report` | 2 | 4 |
| `vector-let-report` | 3 | 5 |
| `subckt-formal-pin-case` | 3 | 5 |
| `bsource-node-case-report` | 4 | 6 |
| `sens-node-case-report` | 4 | 6 |
| `auto-bridge-node-case-report` | 3 | 6 |
| `event-node-case-report` | 3 | 6 |

The two XSPICE rows are one over 2 + `source` count, and that one is a
generated bridge inside a sub-deck. Each of those two decks writes three
sub-decks into the build directory; run standalone, exactly one of each three
costs 3 rather than 2, and it is the bridged one:

```
$ for f in abr_*.cir enr_*.cir; do
      echo "$f $(SPICE_SCRIPTS=. …/src/ngspice -D casemode=distinguish --batch $f \
                 2>&1 >/dev/null | grep -c experimental)"; done
abr_exact.cir 3
abr_hand.cir 2
abr_miss.cir 2
enr_bridged.cir 3
enr_miss.cir 2
enr_notwin.cir 2
```

So the mechanisms this issue's Summary identifies — `source` and the generated
bridge — account for every deck above the floor, and
36160 bytes of a 320-byte contract statement go to the terminal on every
`make check`. Nobody has noticed because `tests/bin/check.sh:29` captures
stdout only.

**3. The count is not even monotone in how much of the run was experimental.**
This is the part the finding did not reach, and it is what makes the defect
more than duplication. The mode is re-established at *every* read, so it can
genuinely change inside one process. `repro/reads/switcher.cir`:

```
* issue 0058: deck read under fold, then a control block sets preserve
* and sources a second deck, so two case modes are live in one process
Vs In 0 DC 3
Rl In MidNode 1k
Rg MidNode 0 3k
.control
  op
  echo FIRST-DECK
  display
  set casemode=preserve
  source mid.cir
.endc
.end
```

and `repro/reads/mid.cir` is the same circuit with `op`, `echo SECOND-DECK`
and its own `display`. Two displays, because each deck can only show its own
plot:

```
$ ngspice -b -n -D casemode=fold switcher.cir 2>&1 \
      | grep -iE '^-DECK|DECK$|^Name:|midnode'
FIRST-DECK
Name: op1 (Operating Point)
    midnode             : voltage, real, 1 long
SECOND-DECK
Name: op2 (Operating Point)
    MidNode             : voltage, real, 1 long
```

Two modes, one process, two plots: `op1`'s net was folded and `op2`'s was
preserved. Now count the banner across two runs of that shape:

```
$ ngspice -b -n -D casemode=distinguish switcher.cir 2>&1 | grep -c experimental
2                       # distinguish for the whole run except the sourced deck
$ ngspice -b -n -D casemode=fold switcher_d.cir 2>&1 | grep -c experimental
1                       # fold, then distinguish for the sourced deck only
```

`repro/reads/switcher_d.cir` is `switcher.cir` with `set casemode=distinguish`
in place of `preserve` and nothing else changed but the title comment — `diff`
shows exactly two hunks, the title and that one word. The run that was under
`distinguish` from the start announces twice; the run that switched *into* it
halfway announces once. A
consumer reading the count as "how experimental was this run" reads it
backwards.

**Class.** None of `doc/claude/decisions/0001-distinguish.md` decision 3's
Class A / B / C applies, and saying so is part of the write-up: that taxonomy
classifies *comparisons of two names*, and this is an emission, not a
comparison. Decision 2 does not govern it either — it is scoped to "the
diagnostic for a case near-miss", and a mode banner is a statement about the
mode rather than a report of a miss. What does carry over is the reason behind
that decision's second 2026-08-11 correction
(`doc/claude/decisions/0001-distinguish.md:143-160`): `>&` retargets the
*variable* `cp_err`, so a diagnostic written to a literal `stderr` cannot be
guarded by a deck at all. Both `fprintf`s here are literal `stderr`, so the
consequence is the same whether or not the decision reaches them. Measured
with `repro/reads/capture.cir`, which uses the `echo`-and-`source` shape
`doc/claude/decisions/0006-diagnostic-deck-coverage.md` decision 2 prescribes:

```
.control
* --- inner deck 1: its own inp_readall() re-announces the mode
echo "* inner deck: read under the mode the outer run set" > inner.cir
...
source inner.cir >& cap.txt
* --- inner deck 2: a vector near miss, which goes through cp_err
...
source inner2.cir >& c2.txt
.endc
```

```
$ ngspice -b -n -D casemode=distinguish capture.cir >/dev/null 2>&1
$ cat cap.txt

Circuit: * inner deck: read under the mode the outer run set

$ grep -c -i casemode cap.txt
0
$ grep -c -i experimental cap.txt
0
$ grep -c -i warning cap.txt
0
```

The `Circuit:` line is there deliberately: it proves the redirect was in force
and the file did receive the sourced deck's output, so the three zeros are a
silence and not a vacuum. The
banner still lands on the terminal. For contrast, on the same build and in the
same run, `vec_warn_case_near_miss()` (`src/frontend/vectors.c:464`, writing to
`cp_err` at `:478`) *is* captured by the same redirect:

```
$ cat c2.txt

Circuit: * inner deck: v(midnode) misses, MidNode is what the deck defines

Warning: no vector named 'midnode'; 'MidNode' differs only in case (casemode=distinguish)
Warning from checkvalid: vector midnode is not available or has zero length.
```

So this issue arrives with the same defect `doc/codex/issues/0036` closed for
three other case diagnostics, and a fix cannot be guarded until that is fixed
too.

## Root Cause

`set_case_mode()` establishes and announces in one function, and the function
is called once per file read.

**The establishment cannot move.** `inp_readall()` calls `set_case_mode()` at
`:1178` and then `inp_read()` at `:1184`, and `inp_read()` is where the fold
happens — `inpcom.c:1842` asks `inp_case_folding()` to decide whether to
lowercase the card. That runs for a command file too, because the `!comfile`
early return is not until `:1204`, so a command file's own cards are folded or
not according to the mode. Measured in `repro/spiceinit-fold/`, where the whole
of the `.spiceinit` is `set MixedVar = 1` and `d.cir` prints the variable list:

```
$ ngspice -b -D casemode=fold     d.cir 2>&1 | grep -i mixedvar
  mixedvar	1
$ ngspice -b -D casemode=preserve d.cir 2>&1 | grep -i mixedvar
  MixedVar	1
```

That is real behaviour and not an accident, so the *call* has to stay where it
is on every read. Only the two `fprintf`s are movable.

**`inp_readall()` already carries the flag that would gate them**, and the
tree already uses it for exactly this split two lines away: `set_compat_mode()`
is called unconditionally at `:1176`, and its announcement
`print_compat_mode()` (`src/frontend/inpcompat.c:115`) is called from inside
the `if (!comfile && cc)` block at `:1214`. The compat pair is the shape this
pair should have had.

**Gating on `comfile` is necessary and not sufficient.** It fixes the measured
table — `spinit` and `.spiceinit` are the two `comfile == TRUE` readers, so the
1 / 2 / 3 rows all become 1, and the count stops tracking the user's init
files, which is the harm the finding names. It does nothing for the other
`comfile == FALSE` readers: a `source`d deck, an `altermod` model file, the
auto-bridge's synthetic deck, and `-p`'s empty temp file. Measured on
`repro/reads/outer.cir`, `print_compat_mode()` is not once per run either, and
for the same reason:

```
$ ngspice -b -n -D casemode=bogus -D ngbehavior=ps outer.cir 2>&1 \
      | grep -c 'Compatibility modes selected'
2
$ ngspice -b -n -D casemode=bogus -D ngbehavior=ps outer.cir 2>&1 \
      | grep -c 'unknown casemode'
3
```

Adopting the compat shape buys once-per-*deck*, which is one better than
once-per-*file* and still not the ask. Gating on `!comfile && !intfile` adds
the auto-bridge read to the silenced set and still leaves `source` and
`altermod`.

**So the announcement must be latched — and the latch key is the whole
outcome of `set_case_mode()`, not the mode enum.** "Announce once and never
again" is the wrong rule, because the `switcher.cir` measurement above shows a
single process legitimately holding two modes, and `switcher_d.cir` is the
version of it that reads a `fold` deck and then `source`s a `distinguish`
one — that run has something new to say and must be allowed to say it. But
"announce when the mode differs from the mode last
announced" is wrong too, and `repro/reads/switcher_b.cir` is the case that
kills it. It is `switcher.cir` again, with `set casemode=bogus` in place of
`preserve` before the `source`, so the mid-run value is a misspelling:

```
$ ngspice -b -n -D casemode=fold switcher_b.cir 2>&1 | grep 'unknown casemode'
Warning: unknown casemode 'bogus', using 'fold'
$ ngspice -b -n -D casemode=fold switcher_b.cir 2>&1 | grep -c 'unknown casemode'
1
```

That warning prints exactly once today and must keep printing. A mode-keyed
latch would lose it: the misspelled arm establishes `NG_CASE_FOLD`
(`inpcom.c:1089` sets it before the parse and the `else` arm at `:1126`
changes nothing), which is byte-identical to the `fold` already latched by the
two reads before it, so a mode-difference test never fires.

What survives every measurement in this issue is a latch on the *outcome* of
the call — the mode it established **together with the unrecognised value
string, when there was one** — initialised to the pair
(`NG_CASE_FOLD`, no unrecognised value), which is the state `ng_case_mode`'s
static initialiser at `inpcom.c:1034` already describes. A read announces only
when its outcome differs from the latched one. Checked against each
measurement above:

| run | outcomes, in read order | announcements now | under the rule |
| --- | --- | --- | --- |
| three-row table, `-D casemode=bogus` | (FOLD,"bogus") ×1, ×2, ×3 | 1 / 2 / 3 | 1 / 1 / 1 |
| `case_collision.cir`, `distinguish` | (DISTINGUISH,–) ×2 | 2 | 1 |
| `am.cir`, `-D casemode=bogus` | (FOLD,"bogus") ×3 | 3 | 1 |
| auto-bridge deck, `-D casemode=bogus` | (FOLD,"bogus") ×3 | 3 | 1 |
| `switcher.cir`, `distinguish` | (DISTINGUISH,–), (DISTINGUISH,–), (PRESERVE,–) | 2 | 1 |
| `switcher_d.cir`, `fold`→`distinguish` | (FOLD,–), (FOLD,–), (DISTINGUISH,–) | 1 | 1 |
| `switcher_b.cir`, `fold`→`bogus` | (FOLD,–), (FOLD,–), (FOLD,"bogus") | 1 | 1 |

The last two rows are the ones that constrain the rule: `switcher_d` is why it
cannot be "say it once", and `switcher_b` is why the key cannot be the mode
alone.

The latch is new static state in `src/frontend/inpcom.c`, beside
`ng_case_mode` (`:1034`), and CLAUDE.md's rule applies to it: it must survive
repeated `ngSpice_Reset()` in the shared build. `ng_case_mode` itself needs no
reset because every `inp_readall()` rewrites it from `cp_getvar()`; a latch
does, or a host program that resets and re-loads a `distinguish` circuit gets
the mode's warranty exactly once per process lifetime.

## Acceptance Criteria

1. The two diagnostics are emitted when the outcome of `set_case_mode()` — the
   mode established, together with the unrecognised value string if there was
   one — differs from the outcome of the **previous read**, and not otherwise.
   The latch's initial value is (`NG_CASE_FOLD`, none), which is the outcome
   `ng_case_mode`'s own initialiser describes, so a plain `fold` run is
   silent. The seven-row table in Root Cause is the assertion set: the
   three-row table becomes 1 / 1 / 1, a run that changes mode mid-process
   still announces at the change (`switcher_d.cir`), and a mid-run
   misspelling still announces even though it establishes the mode already in
   force (`switcher_b.cir`). The last two are the halves a "say it once" fix
   and a mode-keyed fix respectively would break.

   *Reworded 2026-08-13, after the fix and against a measurement; the rule the
   tree implements is unchanged by the rewording.* This criterion first read
   "differs from the outcome last **announced**". That reading and the one
   above agree on all seven rows of the table, and diverge on exactly one
   sequence: a run that leaves `distinguish` for a silent mode and returns to
   it. Measured on the fixed binary, `repro/reads/switcher_r.cir` under
   `-D casemode=distinguish` — `distinguish`, `preserve`, `distinguish` in one
   process — announces **2** times; under the original wording the second
   return is owed nothing and the count would be 1. The rule above is the one
   the tree implements and the one
   `doc/claude/decisions/0016-case-mode-announcement-latch.md` accepts, with
   the last-announced reading rejected there and its incoherence measured: it
   would leave that run silent while announcing the same return when the
   intervening read was a *misspelling* rather than `preserve`, so the count
   would depend on unrelated history the reader cannot see. The sequence that
   discriminates the two is now asserted by criterion 4's deck, cases `VIA`
   and `RETURN`.
2. `set_case_mode()` itself still runs on every `inp_readall()`, including
   command files. `inpcom.c:1842` is why; a fix that gates the *call* changes
   whether a `spinit` or `.spiceinit` card is lowercased under `preserve`, and
   the `MixedVar` measurement above (`repro/spiceinit-fold/`) is the assertion
   for that.
3. Both `fprintf(stderr, ...)` at `inpcom.c:1117` and `:1126` become
   `fprintf(cp_err, ...)`. This is `doc/codex/issues/0036`'s move applied to
   two diagnostics it did not cover, it is a no-op in every binary that
   parses a deck, and without it criterion 4 is not reachable —
   measured above, `>&` does not capture either line today.
4. A deck asserts it. The diagnostic fires during the deck's own
   `inp_readall()`, before any `.control` block runs, so the redirect cannot be
   attached to the deck itself; the shape is
   `doc/claude/decisions/0006-diagnostic-deck-coverage.md` decision 2 —
   `repro/reads/capture.cir` is a working example of it. The assertion is a
   **count**, not a presence: **zero** announcements for an inner deck that
   inherits the outer deck's outcome, **one** for an inner deck whose
   `.control` block changed it first, whether the change is to another mode or
   to a misspelling, and — added 2026-08-13 with criterion 1's rewording —
   **one** for an inner deck whose read returns to `distinguish` after a
   silent `preserve` read has intervened, which is the pair (`VIA`, `RETURN`)
   that pins which of the two latch rules is in force. Both halves matter, and
   the silent half matters more, exactly as in
   `tests/regression/casedist/vector-unlet-report.cir`.
5. `make check` unchanged in all three modes. It should be, without argument:
   no committed `.out` contains either string (`grep -rl 'is experimental'` and
   `grep -rl 'unknown casemode'` over `tests/` each find nothing),
   `tests/bin/check.sh:29` redirects stdout only, and its `FILTER` at `:20`
   drops every line containing `Warning` from both sides anyway. The visible
   effect on the suite is that `tests/{regression,xspice}/casedist` stop
   writing 113 copies of a 320-byte banner — 36160 bytes — to the terminal,
   and write 40 instead, one per deck.

## Resolution

Fixed on `ver_50`, 2026-08-13, in the shape this section sketched while the
issue was open, and in one change rather than two, because criterion 4's deck
cannot be written until criterion 3's move has happened.

`set_case_mode()` (`src/frontend/inpcom.c`) is split the way
`set_compat_mode()` / `print_compat_mode()` already is, except that the gate is
not `comfile` — which would have bought once per *deck* — but a file-static
latch, `ng_case_last_outcome` plus `ng_case_last_unknown`, holding the whole
outcome of the last call: the mode established together with the unrecognised
value string when there was one. `case_outcome_is_new()` compares the new
outcome against the latch and re-latches either way, so the comparison is
always against the previous read; the initial value is (`NG_CASE_FOLD`, none),
which is what `ng_case_mode`'s own initialiser already describes. The mode
values keep their `cieq()`, and the two unrecognised values are compared byte
for byte with a `case-lint: keyword` annotation — they are casemode values and
the question is whether the warning would print the same text, so `bogus` and
`Bogus` are two messages and each is owed once. `tests/lint/identity.baseline`
is unchanged: the new comparison is annotated in place, not baselined.

The establishment did not move: `set_case_mode()` still runs on every
`inp_readall()`, command files included, which criterion 2 requires and the
`MixedVar` measurement re-confirms (`fold` → `mixedvar`, `preserve` →
`MixedVar`). Both `fprintf`s are now `cp_err`, which is criterion 3 and is
`doc/codex/issues/0036`'s move applied to the two diagnostics it did not
cover. `inp_case_announce_reset()` (declared in `src/include/ngspice/fteext.h`)
clears the latch and is called from `totalreset()` (`src/sharedspice.c`), so a
host program that resets and re-loads a `distinguish` circuit is told the
mode's warranty again rather than once per process lifetime.

The seven-row table in Root Cause is the assertion set and every row was
re-measured on the fixed binary; all seven now read the "under the rule"
column:

| run | before | after |
| --- | --- | --- |
| three-row table, `-D casemode=bogus` | 1 / 2 / 3 | **1 / 1 / 1** |
| `case_collision.cir`, `distinguish` | 2 | 1 |
| `am.cir` (`altermod ... file`), `bogus` | 3 | 1, and `is_now = 3e-14` still says the model file was read |
| auto-bridge deck, `bogus` | 3 | 1, and `v(a)` is still 3.3 bridged / 0 unbridged |
| `switcher.cir`, `distinguish` | 2 | 1 |
| `switcher_d.cir`, `fold`→`distinguish` | 1 | 1 — the change is still announced |
| `switcher_b.cir`, `fold`→`bogus` | 1 | 1 — the misspelling is still announced |

`outer.cir` (a `.control` block that `source`s) goes 3 → 1 and the unbounded
`ngspice -p` growth goes 2 / 3 / 4 → 1 / 1 / 1. `.include` and `.lib` were
already 2 and are now 1, still with `v(midnode) = 2.25`. `switcher.cir` still
holds two modes in one process — `op1`'s net folded, `op2`'s preserved — which
is the behaviour that rules out "say it once".

Criterion 4 is `tests/regression/casedist/casemode-announce-report.cir`, in
`doc/claude/decisions/0006-diagnostic-deck-coverage.md` decision 2's shape: it
writes each sub-deck with `echo` and `source`s it under `>&`. Its four cases
are counts, not presences — INHERIT 0, BOGUS 1, REPEAT 0, BACK 1 — with a
`CAPTURE-READ` token on each silence. It was proved RED twice: against the
shipped binary, where BOGUS and BACK read 0 because a literal `stderr` is
invisible to `>&`, and against a binary carrying only the `cp_err` move, where
INHERIT and REPEAT read `ANNOUNCED` — the two halves a `stderr`-only fix and a
latch-less fix respectively would leave broken.

Criterion 5 holds: `make check` is **293 PASS, 0 FAIL**, up by exactly this
deck, with `tests/lint` still reporting `265 comparisons, baseline matches`.
The visible effect on the suite is the one predicted, one banner per deck:
`tests/regression/casedist` now writes **26** for its 26 decks, where the
census above measured 62 for the 23 decks it then had, and
`tests/xspice/casedist` **17** for 17, where it measured 51. 43 decks at 320
bytes is 13760, against the 36160 the census counted for 40.

### Amended 2026-08-13, same day, after verification

Four things the first pass left open or unmeasured, all now closed. Nothing
about the fix's behaviour changed; the binary that closed this issue is the
binary that is here, plus a rename.

- **Which rule the latch implements** was decided rather than left to two
  readings. Criterion 1's wording is corrected above, and
  `doc/claude/decisions/0016-case-mode-announcement-latch.md` carries the
  argument, the counterfactual binary that was built to measure the rejected
  rule, and the sequence that separates them
  (`repro/reads/switcher_r.cir`, `switcher_rb.cir` and `switcher_bb.cir`, all
  three new).
- **The deck now discriminates it.** `casemode-announce-report.cir` gains
  `VIA` and `RETURN`; against a binary carrying the rejected rule exactly one
  line moves, `CASEMODE-RETURN-ONCE` → `-NONE`, and every other case still
  passes.
- **The statics are renamed** `ng_case_announced` → `ng_case_last_outcome`
  and `ng_case_announced_unknown` → `ng_case_last_unknown`, because a latch
  named for the announcement is the reading decision 0016 rejects and is how
  the code and the criterion got out of step. No behaviour change; re-measured
  green.
- **The two unrun checks were run.**
  `doc/claude/scripts/case_differential_sweep.py` over all 332 decks, on this
  tree and on a binary built from `HEAD`'s `inpcom.c`: `DIFF=67/NUM-DIFF=1` vs
  `DIFF=66/NUM-DIFF=1`, differing in one `stdout reordered only` row that is
  flaky on the *unchanged* binary (3 repeats: DIFF=3, 4, 4), so nothing
  computed moves. And `inp_case_announce_reset()`, previously compile-checked
  only, is now measured in a real `--with-ngshared` build:
  `READ1 1 READ2 0 READ3-AFTER-CLEAR 1`
  (`repro/shared/casemode_reset_probe.c`). That probe also records a
  pre-existing shared-build defect it ran into, unrelated to this issue and
  owed one of its own: after `ngSpice_Reset()`, `ngSpice_Command()` runs
  nothing and the next `ngSpice_Circ()` segfaults in `CKTmodCrt()`, with or
  without this issue's line compiled in.

Re-measured after the amendment: `make check` **295 PASS, 0 FAIL** (the count
moved with other crews' decks, not with this one), `tests/lint`
`265 comparisons, baseline matches`, and the banner census is unchanged at 26
for 26 and 17 for 17 — the new deck cases write their banners into the deck's
own captures, not to the terminal.
