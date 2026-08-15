# Running a simulation with arbitrary case — `casemode=distinguish`

How to use this repo's build to run an ngspice deck in which every identifier
may be written in whatever case you like. Everything below was measured
against `build-ver_50/src/ngspice`.

If you are a **client program** (Xschem, a test harness, a wrapper script)
rather than a person writing a deck, skip to §9 — the question you have is
capability detection, and it has a one-line answer.

## 1. The command

```sh
/home/qflow/dev/ngspice_test/build-ver_50/src/ngspice --batch -n \
    -D casemode=distinguish deck.cir > out.txt 2> err.txt ; echo rc=$?
```

Every part of that line is load-bearing:

- **The absolute path.** Plain `ngspice` on this machine is `ngspice-46` in
  `/usr/local/bin`, a build *without* this feature. It accepts
  `-D casemode=distinguish` without complaint — an unknown `-D name=value` is
  just a control variable nobody reads — folds everything, and exits 0. The
  repo build reports `ngspice-46+`. This is the most likely way to get a
  silently wrong answer.
- **`-D casemode=distinguish`.** The mode is a control variable, read once at
  the start of each netlist read. Spell the variable name in lower case and
  use `=` with no spaces; other spellings are ignored silently.
- **`-n`** (`--no-spiceinit`). A `.spiceinit` is sourced *after* the flag and
  can silently override it — a `.spiceinit` is searched for in the deck's own
  directory first, so a stale one next to a deck you were given will defeat
  you. `-n` also discards your own `.spiceinit` customisations; if you need
  them, drop `-n` and `grep -il casemode` the deck directory, `$SPICE_USERINIT_DIR`,
  the cwd and `$HOME` first.
- **Two separate redirections.** `rc` read through a pipe reports the pipe's
  status, and stderr is unbuffered, so `2>&1` scrambles warnings away from the
  output line that caused them.

**You cannot turn the mode on from inside the deck.** Neither
`.options casemode=distinguish` nor a `set casemode=distinguish` in the deck's
own `.control` block has any effect on that deck — it has already been read and
folded by then. Both fail silently. If the mode must travel with the deck, use
a wrapper that sets it and then `source`s the real file.

## 2. Confirm it actually took

```
echo $curcasemode
```

answers `fold`, `preserve` or `distinguish` — the mode actually **in force**,
read from the latch at the moment you ask, wherever you ask it: a `.control`
block, the interactive prompt, `ngspice -p`, `ngSpice_Command()`. It is
read-only; `set curcasemode=…` is refused with
`Error: curcasemode is a read-only variable.`

Do not trust `echo $casemode` — that is the *request*, an ordinary writable
variable, and it is set in exactly the cases that fail. The two are separate
objects and they drift in both directions: a `set casemode=fold` in a
`.control` block moves the request while the deck keeps running the way it was
read, and `-D casemode=distinguish` followed by a `set casemode=fold` at the
prompt leaves the run distinguishing while `$casemode` says `fold`. Measured,
in a session that reads no deck:

```
$ printf 'set casemode=distinguish\necho req $casemode eff $curcasemode\nquit\n' \
    | ngspice -p -n 2>/dev/null | grep '^req'
req distinguish eff fold
```

Do not count the `experimental` banner on stderr either; it is neither
necessary nor sufficient (a run that folds can still print two).

`$curcasemode` is also the capability probe: a binary without the feature has
no such variable, so the read *fails* rather than answering — `ngspice-46`
replies `Error: curcasemode: no such variable.` on stderr and echoes nothing,
where `$casemode` there happily reports back whatever `-D` put in it. That is
the one probe that separates all three modes *and* both binaries from a
session with no deck in it; see §9.

The deck probe below stays as the portable fallback — it works on binaries
that predate `$curcasemode`, and it tests the *behaviour* rather than the
binary's report of itself. Put it in the deck's `.control` block:

```
let CaseProbe = 1
let caseprobe = 2
print CaseProbe caseprobe
```

| output | mode |
| --- | --- |
| `CaseProbe = 1` and `caseprobe = 2` | **distinguish** — what you want |
| `CaseProbe = 2` and `caseprobe = 2` | preserve |
| `caseprobe = 2` twice | fold — the mode is off |

This is the only check that survives every failure route: wrong binary,
`.spiceinit` override, and setting the mode too late.

The three-way reading depends on the `.control` text having been through the
reader, so it holds **in a deck only**. Fed to `ngspice -p` on stdin the fold
and preserve rows collapse; see §9.

## 3. What the mode actually means

Distinguish does **not** make case irrelevant. It makes case *significant*:
you choose a spelling per identifier and are then obliged to repeat that
spelling byte-exactly at every use.

The sharp edge is that this applies to files you did not write. A library
defining `.SUBCKT NAND2` and `.MODEL NCH`, referenced from your deck as
`Nand2` and `Nch`, is a fatal run — `Error: unknown subckt`, `no simulations
run!`, rc=1. **Names you define are yours to case; names a library defines are
not.**

The gain is that identifiers differing only in case become distinct, so
`Out` and `OUT` are two nets, and `Rm`/`RM` are two devices where fold rejects
the deck outright with `device already exists, bail out`.

## 4. What splits by case, and what never does

| Case-sensitive under distinguish | Case-insensitive in every mode |
| --- | --- |
| node names | command names (`print`, `op`, `let`) |
| device instance names | dot-cards (`.TRAN`, `.SUBCKT`, `.END`) |
| `.model` / `.subckt` / `.global` names | device letter prefixes (`R` = `r`) |
| `.param` / `.func` names and `.func` formals | `.model` type names (`SW`, `ADC_BRIDGE`) |
| subcircuit formal pins | model and instance parameter names |
| **subcircuit parameters on the X card** | built-in functions (`sqrt`, `mag`, `sin`) |
| control-language vector names | numeric suffixes (`1K` = `1k`; `1M` = `1m` = milli) |
| `define` function names | `gnd` / `GND` / `0` |
| XSPICE event node names | the `v()` / `i()` wrapper letter |
| the device half of `@dev[param]` | the parameter half of `@dev[param]` |
| | `.lib` **section** names |

Two consequences worth internalising: dot-cards and commands may be typed in
any case freely, so `.PARAM`/`.CONTROL`/`OP`/`PRINT` all work; and
simulator-*constructed* names keep the capitals the simulator chose —
`V1#branch` must be typed that way.

## 5. The three silent traps

Most mis-cased names are reported, with a `... differs only in case
(casemode=distinguish)` warning on stderr. These three are not — no warning,
no error, rc=0:

1. **A mis-cased node just becomes a second net.** Nothing is wrong from the
   simulator's point of view; you get a different circuit.
2. **A mis-cased subcircuit parameter falls back to its default.**
   `X1 In N1 Div Rv=2k` binds against `.subckt Div A B Rv=1k`;
   `X2 In N2 Div RV=2k` does not, and silently uses `1k`. Measured:
   `v(N1) = 1.0` against `v(N2) = 1.5`, with zero diagnostics.
3. **A mis-cased `.global` reference floats.** The subcircuit body gets a new
   instance-local net instead of the global, and reads 0.

**Exit status will not save you.** rc=0 for all three of the above, for an
empty `.print` table, and for a mis-cased vector. rc=1 only for an unknown
subckt/model and a `.func` formal/body mismatch. A CI gate on `$?` passes every
silent corruption. Note also that stderr is *never* empty under distinguish —
the experimental banner is always there — so "stderr empty = pass" fails every
run.

## 6. Auditing your own deck

The only thing that catches the silent traps: run the deck both ways, fold the
*labels*, diff the *values*.

```sh
NG=/home/qflow/dev/ngspice_test/build-ver_50/src/ngspice
for m in fold distinguish; do
  $NG --batch -n -D casemode=$m deck.cir 2>/dev/null | grep -E '^v\(' \
    | awk '{print tolower($1), $NF}' > v_$m.txt
done
diff -u v_fold.txt v_distinguish.txt
```

A consistently-cased deck prints nothing. Anything it does print is a place
where case changed your answer — intended or not. `grep "differs only in case"`
on stderr finds none of these.

`display` inside `.control` shows the spellings the simulator is actually
holding, which is how you find a stray net like `X3.VDD`.

## 7. Two control-language gotchas

- **The shipped functions answer only to lower case.** `vm`, `vp`, `vdb`,
  `vr`, `vi`, `vg`, `gd`, `min` and `max` are installed by ngspice in lower
  case, so under distinguish `print VM(1)` fails. Either type them lower case,
  or alias one: `define VmAg(X) MaG(v(X))` works, because a `define` you write
  is yours to case.
- **Type option variable names in lower case** (`set numdgt=6`), for the same
  reason.

## 8. XSPICE footnote

Event node names are case-sensitive and mis-cased ones *are* reported. But an
XSPICE deck run outside a build test directory cannot find its code models and
may segfault. Run it with a local `spinit` holding absolute `codemodel` lines
and point at it:

```sh
SPICE_SCRIPTS=. $NG --batch -n -D casemode=distinguish deck.cir
```

Only the `codemodel` lines are needed in that file.

## 9. For a client program (Xschem, harnesses, wrappers)

### Ask the right question

Do not ask *"is this feature compiled in?"* Ask **"will the run I am about to
launch be case-sensitive?"** They differ — the mode can be present but
overridden by a `spinit`, a `.spiceinit`, or a missing flag — and only the
second question has an actionable answer. One probe answers it correctly
whatever the cause.

**The version string is not the signal.** This build reports `ngspice-46+`
and the featureless one `ngspice-46`, but `+` only means "a development
build"; it is not a capability claim, and no other build advertises the
feature in its banner.

### The probe: one spawn, no temp file, ~10 ms

Ask the binary directly. `$curcasemode` is read-only and computed from the
latch on every read, so it answers for the run this very process would do:

```sh
printf 'echo MODE $curcasemode\nquit\n' \
  | ngspice -p -n -D casemode=distinguish 2>/dev/null \
  | sed -n 's/^MODE //p'
```

| binary | flag | stdout | stderr |
| --- | --- | --- | --- |
| this build | `-D casemode=distinguish` | `distinguish` | — |
| this build | `-D casemode=preserve` | `preserve` | — |
| this build | no flag | `fold` | — |
| ngspice-46 | any or none | *(nothing)* | `Error: curcasemode: no such variable.` |

That is all three modes plus the featureless binary from one spawn, and it is
the only probe that sees `preserve` from a session with no deck in it. The
empty stdout on the last row is the capability answer: treat "no line" as "no
support", not as "fold".

### Run the probe from the deck's own directory — `cd` first

This is the one way to make the probe above lie, and it was found by a client
program, not by us. `.spiceinit` is searched for in the **deck's** directory;
a `-p` probe has no deck, so it searches **cwd**. Give the probe a different
cwd from the deck and it answers for a `.spiceinit` situation that is not the
one your real run will be in. Measured, with a `.spiceinit` holding
`set casemode=fold` sitting next to the deck and `-D casemode=preserve` on both
command lines:

| cwd | deck | probe says | the real run writes |
| --- | --- | --- | --- |
| `probe/` | `deck.cir` | `fold` | `v(in)`, `v(midnode)` — agree |
| `repro2/` | `probe/deck.cir` | `preserve` | `v(in)`, `v(midnode)` — **disagree** |

The second row is the trap: the probe is confidently wrong, and it is wrong in
the direction that matters, claiming capitals you will not get. So the rule is
not "probe with the real run's argv" but **"probe with the real run's argv
*and* its cwd"**.

Two things soften it, and one of them is worth building on:

- **The rawfile's own `Option: casemode=` line catches it afterwards**, if you
  asked for the line — `set casemodewrite`, see below; it is off by default.
  The disagreeing run above wrote `Option: casemode=fold` while the probe had
  said `preserve`. If you read that line, a wrong-cwd probe becomes a
  discrepancy you can detect rather than a silent mislabelling — which is the
  strongest reason to read the header even when you have probed, and the
  strongest reason to turn the writer on in your own generator.
- **`-n` makes the whole question go away**, at the price of the user's own
  `.spiceinit`. With `-n` on both the probe and the run, the first row's probe
  answers `preserve` and so does the run.

While you are here: **`write` inside a `.control` block resolves its filename
against cwd, not against the deck.** In the second row above, `deck.raw` landed
in `repro2/` and not beside the deck. Running from the deck's directory fixes
that too, which is one more reason to do it.

The identity probe below predates `$curcasemode` and remains useful against
older binaries, and because it tests behaviour rather than the binary's own
report. Pipe mode takes commands on stdin and needs no netlist at all:

```sh
printf 'let CaseProbe = 1\nlet caseprobe = 2\nprint CaseProbe\nquit\n' \
  | ngspice -p -n -D casemode=distinguish 2>/dev/null \
  | grep -q '^CaseProbe = 1' && echo distinguish || echo folded
```

Two vectors are created whose names differ only in case. Under `distinguish`
they are two vectors and `CaseProbe` still holds 1; otherwise the second
assignment overwrote the first and the reply is `caseprobe = 2`. The `grep`
is anchored because pipe mode echoes its own input back to stdout — an
unanchored match would find the probe's own text and always succeed.

Measured, using the same flags you will use for the real run:

| binary | flag | result |
| --- | --- | --- |
| this build | `-D casemode=distinguish` | `distinguish` |
| this build | `-D casemode=fold` or `preserve` | `folded` |
| this build | no flag | `folded` |
| ngspice-46 | `-D casemode=distinguish` | `folded` |

The probe tests *identity* — whether two names differing only in case are two
vectors — which is why it works in pipe mode at all, and why it is unaffected
by how output is labelled. It answers exactly one question: **is `distinguish`
in effect.** `preserve` reports `folded`, correctly, because `preserve` does
not change identity.

**It cannot distinguish `fold` from `preserve`,** and no *identity*-based
pipe-mode probe can: `print` labels its output with the name as *typed*, and
stdin is never folded because it is not a netlist read, so both modes reply
`CaseProbe = 2`. (§2's three-way table works only from inside a deck, where
the `.control` text has itself been through the reader.) `$curcasemode` above
does see `preserve`, because it reports the latch instead of testing a
behaviour; on a binary too old to have it, run one throwaway deck and look at
the raw file — see the table below, which is the property you actually care
about anyway.

The featureless binary accepts the flag and reports `folded`, which is the
correct answer to the question as posed. Run the probe with the *exact*
argument vector you intend to use for simulations, so it inherits the same
`spinit`/`.spiceinit` situation; then the answer is about your real runs and
not about the binary in the abstract.

### What actually changes for a schematic tool: the raw file

This is the payoff, and it does not need `distinguish`. Net `MidNode`,
instance `Vs`:

| mode | raw-file `Variables:` |
| --- | --- |
| fold *(default, and ngspice-46)* | `v(in)`, `v(midnode)`, `i(vs)` |
| **preserve** | `v(In)`, `v(MidNode)`, `i(Vs)` |
| **distinguish** | `v(In)`, `v(MidNode)`, `i(Vs)` |

So back-annotation labels match the schematic as drawn under either `preserve`
or `distinguish`. If your back-annotation currently lower-cases names to match,
that step has to become conditional — under both non-fold modes the raw file
carries the capitals.

### The raw file can say which mode wrote it — ask for it with `casemodewrite`

Since 2026-08-14 the header written by the `write` command can carry the mode
in force, on the line after `Plotname:`. It is **off by default**; one word in
the `.control` block turns it on:

```
.control
  op
  set casemodewrite
  write out.raw
.endc
```

```
Plotname: Operating Point
Option: casemode=preserve
```

It is there in both the ASCII and the binary format — the header is text in
both — and the value is the *effective* mode, the one `curcasemode` reports,
so a `set casemode=` typed in a `.control` block after the deck was read
cannot make it lie (§2). Unset — the default — the header is byte for byte
the header every ngspice has ever written, which is what makes turning it on
your decision and not ours.

**Know this before you hand such a file to somebody else.** Loading it puts a
`casemode` key into that session's loaded-plot environment, and a *released*
`ngspice-46` that then does `unset casemode` frees a node it leaves linked:
the `unset` returns cleanly and the next command — `set`, `display`, `print`,
`echo $casemode`, another `unset` — segfaults, `rc=139`, in both formats. That
is `doc/codex/issues/0067`, fixed in this tree and in nothing released, and it
is the whole reason the line is opt-in. Within one shop it is a non-issue: you
control the writer and the reader, a reader that parses the key never unsets
it, and this build does not have the defect. The default flips once the fix
has been in a release.

Read it however you already read the header — but read it as an `Option:`
*key*, not as a line number. Scan the header for lines beginning `Option:`,
split the rest on the first `=`, and trim the spaces around both halves; that
is what ngspice's own reader does, and it is what makes the paragraph below
work. The line the writer puts under `Plotname:` is spelled
`Option: casemode=preserve`; a value that came out of a file the session
loaded is re-emitted lower down in the header and spelled
`Option: casemode = preserve`. Same key, same value, two spellings and two
places.

Inside ngspice, `load` files the pair into the loaded plot's environment, so
`echo $casemode` after a `load` answers what the file recorded, and an
unmodified `ngspice-46` does that too: `Option:` is a key its reader already
parses, which is why the mode did not get a header key of its own. A key it
does *not* know aborts the whole load with `Error: strange line in rawfile`,
so do not invent one either.

Three caveats. **Absence is not `fold`** — a file written by any older ngspice
has no such line, nor has one written by the `-r` batch path, which is a
different writer that does not carry it yet, nor has one this build wrote
with `casemodewrite` unset, which is every file until somebody sets it; treat
a missing line as unknown and fall back to the probe below.

**A plot that was loaded and then written back out keeps the mode its own file
recorded**, and does not acquire the re-writing session's. That is what you
want from a tool that loads, tidies and re-writes: the header goes on
describing the names under it. The writer never adds a line of its own over the
file's, so a file that came in with one `casemode` line goes out with one. The
corollary is the previous paragraph:
in a re-written file the line sits where the re-emitted options sit, after
`No. Points:`, so a reader that looked at line 5 alone would miss it. And if
the file it was loaded from recorded *nothing* — the default, and every older
ngspice — then there is nothing to keep, and a re-write with `casemodewrite`
set stamps the re-writing session's mode; that case is still "absence is not
fold" one step on, so if provenance matters to you, write the original with
the variable set.

**`Option: casemodewrite` in a header does nothing to the session that reads
it.** The variable is a request this session makes about what it writes, not
a property a file can carry, and a file the user loaded is not allowed to
answer it — so putting the key in a header you generate will not make the next
ngspice record the mode. Say `set casemodewrite` in the deck, which is the
only thing that turns the writer on.

### Which mode a schematic tool should ask for

**`preserve` is very probably what you want.** It is the safe half of the
feature: labels keep their capitals everywhere, while *identity* still folds,
so none of §5's silent traps can fire. Measured on a deck with nets `Out`/`OUT`
and devices `Rm`/`RM`:

| mode | result |
| --- | --- |
| fold | rc=1, `device already exists, bail out` |
| **preserve** | rc=1, `device already exists, bail out` — identical to fold |
| distinguish | rc=0, two nets and two devices |

Choose `distinguish` only when the user genuinely wants `Out` and `OUT` to be
*different nets*. It is the mode that lets a schematic contain two nets
differing only in case — and it is also the mode in which a mis-cased
subcircuit parameter silently takes its default and a mis-cased `.global`
silently floats, with rc=0 either way. A GUI that emits netlists
programmatically is well placed to be consistent, but it inherits the user's
libraries, and library names are not yours to case (§3).

### If you offer it as a setting

Probe once at startup with the user's configured ngspice command; if the
result is `NGCASE=folded` when they asked for `distinguish`, say so rather
than proceeding — that is the state in which everything looks fine and the
numbers are quietly folded. Do not gate on exit status: rc=0 covers every
silent trap in §5.

## Worked example

```spice
* Arbitrary case throughout, under casemode=distinguish.
* Out and OUT are two nets; Rm and RM are two devices.
Vs In 0 DC 3
Rm In Out 1k
RM In OUT 1k
Rn Out 0 1k
Ro OUT 0 3k
.CONTROL
op
let CaseProbe = 1
let caseprobe = 2
print CaseProbe caseprobe
print v(Out) v(OUT)
.ENDC
.END
```

```
CaseProbe = 1.000000e+00
caseprobe = 2.000000e+00
v(Out) = 1.500000e+00
v(OUT) = 2.250000e+00
```

Under `-D casemode=fold` the same deck does not run at all: `Rm` and `RM`
collide and it exits 1.
