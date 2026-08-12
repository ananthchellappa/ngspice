# Decision 0013 — the user-defined function name, `doc/codex/issues/0020` gap 2

## Status

Accepted, 2026-08-11, branch `ver_50`. Closes gap 2 of
`doc/codex/issues/0020`, re-scoped: the defect is not the `vm()` prefix, it is
that a **user-defined function name** is matched byte-exactly, and
`vm`/`vp`/`vdb`/`vr`/`vi` are instances of that rather than its subject.
Files `doc/codex/issues/0051`. Applies decision 3's Class C rule and decision
2's definition/resolution split of `doc/claude/decisions/0001-distinguish.md`
to `src/frontend/define.c`, and Class A to the one site in the same file that
is not Class C.

## Context

### `0020`'s Root Cause paragraph for gap 2 is wrong, and this is its second correction

It says

> the control language's vector-function prefix is matched against lower-case
> literals on text the reader stopped folding

and there is no such site. `PP_mkfnode()` (`src/frontend/parse.c:506`) copies
the typed name into a buffer, `strtolower()`s the copy, and matches
`ft_funcs[]` with `eqc()` — case-insensitive twice over. Nothing there can
fail on case.

There is also no `vm` in `ft_funcs[]`. `vm`, `vi`, `vr`, `vp`, `vdb`, `vg`,
`gd`, `max` and `min` are **user-defined functions**: `ft_cpinit()`
(`src/frontend/cpitf.c:56`) holds them in a `udfs[]` table of source text and
installs them by calling `com_define()`, in lower case, at start-up. So a
`print VM(2)` reaches `PP_mkfnode()`, misses `ft_funcs[]`, and falls through to

```c
struct pnode *
ft_substdef(const char *name, struct pnode *args)
{
    ...
    for (udf = udfuncs; udf; udf = udf->ud_next)
        if (eq(name, udf->ud_name)) {          /* define.c:296 at 4d36cf61d */
```

`eq()` is `strcmp`. Under `fold` the reader had lower cased the control line,
so the typed name and the stored name were both lower case; under `preserve`
it does not, and the lookup is byte-exact.

`0020`'s gap 1 Root Cause paragraph was corrected once already, for naming
`found_mult_param()` when the site was in `inp_fix_inst_line()`. This is the
second correction, and it is the same failure: the paragraph named the place a
reader would guess from the symptom rather than the place the measurement
leads to.

### The scope `0020` does not state

Measured at `4d36cf61d` under `-D casemode=preserve`, on a deck whose control
block defines its own function and calls it back in the other case (the
diagnostic is two lines; it is joined here):

```
.control
op
define f(x) x*3
print F(2)      Error: no such function as F, / or F(2) is not available.
print f(2)      f(2) = 6.000000e+00
.endc
```

So **every** user-defined function in the control language answers only to the
spelling it was defined with. The shipped `vm` family is that rule applied to
nine functions ngspice defines for itself, which is why they are the visible
half.

### Re-scoped in place, not re-filed

**Decided: re-title and re-scope `0020` gap 2 where it is; do not open a new
issue and mark it superseded.**

`0020`'s acceptance criterion 2 already names `VM`, `VDB` and `VP` by name and
criterion 3 already counts gap 2's entries in the differential sweep. Moving
the defect to a new number would leave both criteria pointing at an issue that
no longer contains their subject, and `0020` is already a two-gap issue whose
gap 1 is closed with a Resolution in place — the file is the record of that
pair. What the Resolution does instead is state the correction, so a reader who
arrives at criterion 2 through the `vm()` wording finds the wider rule.

Nothing in this record changes a computed value under `distinguish`.

---

## Decision 1 — `ft_substdef()` is Class C, and the measurement is the argument

**Decided: Class C. The predicate is `udf_name_eq()`, whose exact arm is
`inp_case_mode() == NG_CASE_DISTINGUISH`, not `ng_ideq()`.**

This was genuinely open, and both candidates make `distinguish` exact — that
is not what separates them. The whole of the difference is `fold`:

| | `fold` | `preserve` | `distinguish` |
| --- | --- | --- | --- |
| Class A, `ng_ideq()` | exact | insensitive | exact |
| Class C, `udf_name_eq()` | **insensitive** | insensitive | exact |

The argument for Class A is that a `define` name is a name the deck author
chose, like a `.param` or a `.subckt` name, and `0001` decision 3 puts those
in Class A without hesitation.

It loses to a measurement, and the measurement is `0004` decision 1's,
repeated at a second site. `0004` argues that `vec_remove()` is Class C
because its query is a word typed at the control language, and the reader
folds such a word **only when it arrived through `inp_readall()`**. That is
exactly the situation here, and the consequence is visible in the **default**
mode. At `4d36cf61d`, through `ngspice -p`, with no `casemode` set at all:

```
ngspice 1 -> define f(x) x*3
ngspice 2 -> print F(2)

Error: no such function as F,
    or F(2) is not available.
ngspice 3 -> print VM(1)

Error: no such function as VM,
    or VM(1) is not available.
```

`ngSpice_Command()` from `libngspice` and the interactive prompt have the same
property. So Class A would have left the shipped default mode unable to call
its own shipped functions from three of its four entry points, and would have
done it in the name of a mode that does not ship. That is the regression
`0001` decision 3 refused at `findvec()` and `0004` decision 1 refused at
`vec_remove()`, for the same reason and on the same kind of evidence.

The second argument is what the list actually holds. Fourteen of its entries,
under nine names, are not deck text at all: they are `cpitf.c`'s `udfs[]`,
which is ngspice naming things for itself — `vm`, `vi`, `vr`, `vp` and `vdb`
each have a one- and a two-argument form, beside `vg`, `gd`, `max` and `min`. `0001` decision 3's closing rule is that `distinguish` must
be exact about names the deck chose and case-insensitive about names ngspice
constructs; a name space that mixes the two cannot be Class A wholesale. Class
C is the reading that gets `fold` right for both halves.

**Rejected: split the predicate per entry** — a bit on `struct udfunc` saying
"ngspice defined this one", so that the shipped `vm` stays case-insensitive
under `distinguish` while a user's `define` does not. It is the only answer
that would make `print VM(2)` work in all three modes. It is not taken because
it makes one name space answer identity two ways, and because `q1#collCX`
(`0001` decision 3) already sets the precedent for the cheaper answer: a name
ngspice constructs must be typed the way ngspice spells it, and the failure is
loud rather than silent.

### What Class C decides, stated for the two cases the choice is about

- **`define VM(x)` beside the shipped `vm`.** Under `fold` and `preserve` they
  are one identifier, so `vm(1)` and `VM(1)` both answer with the user's
  function afterwards — which they do, because `com_define()` prepends and
  `ft_substdef()` takes the first match. Under `fold` the reader also folds
  the card, so the shipped entry is *replaced* and `define vm` lists one
  one-argument entry; under `preserve` it is only shadowed, and `define vm`
  lists both. That difference is decision 2's residue and is
  `doc/codex/issues/0051`, not a property of Class C. Under `distinguish`
  they are two functions — `vm(1)` is `mag(v(1))` and `VM(1)` is the user's —
  and that is the mode's rule rather than a defect.
- **Two `define`s differing only in case, under `distinguish`.** Two
  functions. `define ff(x) x*3` beside `define FF(x) x*5` gives `ff(2) = 6`
  and `FF(2) = 10`, asserted by
  `tests/regression/casedist/udf-name-case.cir`. Under `fold` and `preserve`
  the second replaces the first and both spellings give 10.

## Decision 2 — the definition side does **not** take the same predicate, because case-blinding a prefix test widens what it destroys

**Decided: `com_define()`'s replace-or-prepend lookup stays byte-exact
`prefix()`. The case fix at that one site is blocked on
`doc/codex/issues/0051` and waits for it. This is the reverse of what this
record first said, and the reversal was forced by a measurement made while
reviewing it.**

The lookup is

```c
    for (udf = udfuncs; udf; udf = udf->ud_next)
        if (prefix(b, udf->ud_name) && (arity == udf->ud_arity))
            break;
```

and it carries two defects that turn out not to be separable.

**The case half looked like this record's.** Leaving it exact leaves the state
`0004` decision 4 argues against for `com_compose()` and `com_cross()`: a
table holding two entries under the name being defined. Measured at
`4d36cf61d` under `preserve`:

```
define VM(x) x*7
print vm(1)     vm(1) = 2.000000e+00     <- the shipped vm
print VM(1)     VM(1) = 7.000000e+00     <- the user's
```

Two functions under one identifier, in the mode whose rule is that two
spellings are one identifier.

**The prefix half is `doc/codex/issues/0051`.** `prefix(p, s)` is true when
`p` is a prefix of `s`, and `ud_name` is a packed `name\0arg1\0arg2\0`, so the
test compares the name being defined against a stored name's *leading
characters*. Measured at `4d36cf61d` under the default `fold`, no `casemode`
involved:

```
define foo(y) y*100
print foo(2)          foo(2) = 2.000000e+02
define f(x) x*3
print foo(2)          Error: no such function as foo
define                foo is gone; f (x) = (x)*(3) stands where it stood
```

`define f(x)` silently destroys a stored `foo(y)` of the same arity, in every
mode, and overwrites `ud_name` without freeing it.

**The two are not separable, and the first version of this record said they
were.** It replaced `prefix()` with a `ciprefix()`/`prefix()` pair and claimed
in three places — the source comment, this decision and `0051`'s Root Cause —
that "gating only the case sensitivity leaves it exactly where it was in all
three modes". That is false, and the falsehood is destructive. Case-blinding a
prefix test **widens the set of names it destroys**, because `ciprefix()`
matches pairs `prefix()` never did. Measured on the built binary, at the
`ngspice -p` prompt under the **default `fold`**, which is the path decision 1
itself identifies as unfolded:

```
define VD(x) x*7
define          before:  VD (x) = (x)*(7) / vdb (x, y) = ... / vdb (x) = db (v (x))
                after:   vdb (x, y) = ...  / VD (x) = (x)*(7)
```

`ciprefix("VD", "vdb")` is true where `prefix("VD", "vdb")` is false, so
defining `VD(x)` silently destroyed the shipped `vdb(x)`. A `libngspice` host
issuing `ngSpice_Command("define FO(x) ...")` destroyed a stored `foo(y)` the
same way, and a `preserve` deck did too. That is new data loss in a mode this
record says only gains resolutions.

Reverting the arm to `prefix()` is not the alternative it looks like: the
`ciprefix()` behaviour is exactly what "under `preserve`, `define VM(x)`
replaces the shipped `vm`" requires. The two wants are the same want. So the
site cannot be made case-blind until the comparison stops being a prefix test,
which is `0051` acceptance criterion 2, and this record does not do that —
`0051` is a mode-independent wrong answer that wants its own RED deck in
`tests/regression/misc/`, and folding it in here would put two mechanisms in
one commit for the second time.

**The residue, stated plainly and pinned by a deck.** Under `preserve`,
`define VM(x)` beside the shipped `vm` prepends a second entry instead of
replacing it. No number moves: `com_define()` prepends and `ft_substdef()`
takes the first match, so the newer definition answers *both* spellings —
`vm(1)` and `VM(1)` are both 7 after the fix, where at `4d36cf61d` they were
2 and 7. What remains is that `define vm` lists three entries where the
consistently-spelled twin lists two. `tests/regression/case/udf-name-case.cir`
asserts those three lines and its twin asserts the two, so `0051`'s eventual
fix has to come through both decks.

**How this was found, because the method is the point.** Not by reading. The
first version of this record shipped the `ciprefix()` arm with a GREEN test
suite, a passing lint and a Evidence table whose one `0051` row —
`define f(x)` beside a stored `foo(y)` — tested the pair that was *already*
destroyed at the parent commit and therefore could not see the widening. It
was found by an adversarial re-read of the change that ran the case the
Evidence section did not: a pair differing in case as well as in length.

## Decision 3 — all five `define.c` sites, classified in the source

`tests/lint/identity.baseline` held five `src/frontend/define.c` records and
had no opinion about any of them, which is what `0011` decision 2 says a
baseline entry claims. The classification is this decision.

| Site | Shape | Class | Now |
| --- | --- | --- | --- |
| `ft_substdef()` `eq(name, udf->ud_name)` | resolution — the typed call | **C** | `udf_name_eq()` |
| `prdefs()` `eq(name, udf->ud_name)` | resolution — the word `define <name>` was given | **C** | `udf_name_eq()` |
| `com_undefine()` `eq(wlist->wl_word, udf->ud_name)` | resolution — the word `undefine` was given | **C** | `udf_name_eq()` |
| `trcopy()` `eq(s, d->v_name)` | the formal's spelling against the body's, both on one card | **A** | `ng_ideq()` |
| `com_define()` `eqc(ft_funcs[i].fu_name, tbuf)` | keyword — `ft_funcs[]` is the language's own table | keyword | unchanged, `/* case-lint: keyword - ... */` |

**The session prompt's list of the five is one substitution off**, and it
matters because it changes what leaves the baseline. It names
`prefix(b, udf->ud_name)` (`define.c:161` at `4d36cf61d`) as one of the five;
`prefix` is not on `identity.lint`'s `comparators` list, so the lint has never
seen that line. The fifth baseline record is the *second*
`eq(name, udf->ud_name)`, in `prdefs()` at `:227`.

`trcopy()` is the one Class A site and it is a `preserve` defect of its own,
found by reading rather than by the issue:

```
define g(X) x*2
print g(5)      fold  g(5) = 1.000000e+01
                preserve  -- no line: the body's x was never substituted
```

The formal is `X`, the body writes `x`, both on the same card. `0001` decision
3 already puts the `.func` formal (`inpcom.c:4787`) in Class A, and this is
its control-language twin. Under `distinguish` the miss is the right answer
and `ng_ideq()` keeps it. Both halves are in the decks: `hh(5)` has a value
line in `tests/regression/case/udf-name-case.cir` and none in
`tests/regression/casedist/udf-name-case.cir`, where it is pinned between
`FF(2)` and `vm(2)` so that its absence cannot pass by the deck stopping
early. This is a second mechanism and lands in its own commit; the deck lines
that assert it land with it.

`parse.c`'s `is_probe_name()` matches these same formals byte-exactly, and its
comment used to say `com_define()` matches them with `eq()`. That is now
`ng_ideq()`, and the comment says so and says why byte-exact is still right
there: the near-miss report the probe list suppresses is gated on
`distinguish`, which is the arm in which `ng_ideq()` is exact.

Two comparisons in the file are **not** in the baseline and are not touched,
because each has a string literal operand and the lint exempts those by
design: `savetree()`'s `eq(d->v_name, "list")` and `trcopy()`'s
`strcmp(d->v_name, "list")`. Both test the placeholder name the parser reserves,
which is a word this file defines and not a name anybody typed.

## Decision 4 — `undefine F` was broken, and it is `0027`'s shape at a second name space

**Finding, measured, not assumed.** At `4d36cf61d` under `preserve`:

```
define f(x) x*3
undefine F      silent
print f(2)      f(2) = 6.000000e+00     <- still there
define          f (x) = (x)*(3)         <- still listed
```

`doc/codex/issues/0027` is `unlet` removing the wrong vector; this is the same
command shape one name space over, failing in the opposite direction — a
removal that resolves exactly removes nothing, silently. `0004` decision 3
splits `vec_remove()`'s callers into a resolution that reports and two
definitions that stay silent. `com_undefine()` is unambiguously the
resolution half: the word it is given must already name something, and there
is nothing it is about to allocate.

**No near-miss diagnostic is added here**, and that is a deliberate omission
rather than an oversight. `0001` decision 2 asks for a warning where a
resolution fails beside a case variant, and this site qualifies. It is not
taken because `com_undefine()`'s miss is silent *in every mode and for every
reason* — `undefine nosuchfunction` has always said nothing — so a
`distinguish`-only near-miss report would be the only thing this command ever
says, and would arrive before the plain-miss report that
`doc/codex/issues/0028` established should come first. Filed as a criterion on
`0051` rather than done half way here.

## Decision 5 — what moves in each mode

`udf_name_eq()`'s insensitive arm covers `fold` and `preserve`, so unlike
every Class A edit in this series this change is **not** a no-op under `fold`.
That is the point of decision 1 and it is stated plainly:

- **`fold`** — a strict loosening on the **resolution** side only, and only
  where the reader never folded. Decision 2 is what keeps the definition side
  out of it, and that is the difference between this version of the record and
  its first. A deck's `.control` block is folded by `inp_readall()`, so no
  deck that resolves today resolves differently; what changes is that a call
  which *failed* now succeeds, at the interactive prompt, through
  `ngspice -p` and through `ngSpice_Command()`. This is the same trade `0001`
  decision 3 took for `evtprint.c:341`, where `eprint OUTQ` at the prompt
  answered `ERROR - Node OUTQ is not an event node` and now prints the table.
  `trcopy()`'s Class A edit cannot move `fold` either, and the reason is the
  predicate rather than the reader: `ng_ideq()` is `strcmp` under `fold`, so
  it is the comparison that was there — which matters, because at the prompt
  the reader has folded nothing and the "both operands were lower cased"
  argument does not hold.
- **`preserve`** — the mode the issue is filed under. `PRINT VM(2)`,
  `VDB(2)` and `VP(2)` resolve; a user's own `define` answers to either
  spelling; `undefine` finds what it names; and a body that spells a formal in
  the other case substitutes it. A `define` in the other case is *reached* by
  either spelling but still shadows rather than replaces — decision 2.

  One cost, measured rather than reasoned about. `trcopy()`'s fold makes two
  formals differing only in case collapse onto the first, so
  `define ff(X,x) X+x*10` then `ff(3,4)` moves from `4.3e+01` to `3.3e+01`.
  That is the answer `fold` has always given, because the reader lower cases
  both formals and `eq()` then collapses them there too, and under `preserve`
  two spellings of one formal are one formal by the mode's own rule. It is the
  malformed-deck case `0012` decision 3 settles for the `.subckt` card in the
  same words. `distinguish` keeps `4.3e+01`.
- **`distinguish`** — nothing moves. `udf_name_eq()`'s exact arm is the
  `eq()` that was there, `com_define()`'s `prefix()` is untouched, and
  `ng_ideq()` is exact in this mode. The guard deck
  `tests/regression/casedist/udf-name-case.cir` passes at the parent commit
  and after, which is what says so.

## Decision 6 — five baseline entries leave and none arrives

**Decided: delete all five `src/frontend/define.c` lines from
`tests/lint/identity.baseline` by hand, extend the header's *entries removed
since* list, and annotate rather than baseline the one comparison that stays.**

Four leave because their comparison left the tree: `udf_name_eq()`,
`udf_name_prefix()` and `ng_ideq()` are not comparators `identity.lint` looks
for. The fifth, `eqc(ft_funcs[i].fu_name, tbuf)`, leaves although its code is
**unchanged**: it is now annotated `/* case-lint: keyword - ft_funcs[] is the
language's own table */`, and an annotated call is not a reported site, so the
baseline entry would otherwise outlive its report. That is `0011` decision 4's
second way out used for the first time, and it is worth naming because it is a
third way for an entry to leave that decision 1 of `0011` does not enumerate:
not by the comparison leaving, but by the comparison being classified where it
stands.

`udf_name_eq()`'s own body is two comparator calls of two runtime operands, so
it is a site the scanner sees. It is annotated rather than baselined: the
baseline is for a genuine identity comparison that cannot be fixed yet and
must name the issue that will fix it, and this one is fixed. That keeps
`0012` decision 5's direction — the tree ends with fewer unclassified
comparisons, not more.

`273 → 269 → 268`, in two steps, because the two mechanisms land in two
commits and the baseline is a description of the tree at each of them. The
header carries one entry per commit and names which record left in which.

## Decision 7 — the spec's `## Status` paragraph, corrected in passing

It says Phase 3 is *"experimental, with gates 1, 2 and 4 open"*. All four
gates have been closed since `0001` decision 6 item 1, and the spec's own
acceptance bullet below it already says **Done** for the same sites. The
paragraph is corrected to say the gate list is closed and that the mode is
experimental for the reason `0004` decision 6 gives, which is not a gate.

The `preserve` acceptance bullet about a mechanically uppercased copy of every
deck is annotated rather than marked Done: the sweep's `NUM-DIFF` has been 0
throughout, because a `print` that fails writes no rawfile and the rawfile
payload is what that verdict compares, so what this work removes is a *missing
number on stdout* that the numeric comparison could not see. That is the
bullet's subject as written and not as the tool measures it, and saying so is
worth more than a check mark.

## Evidence

### RED, at the parent commit

`tests/regression/case/udf-name-case.cir` under `-D casemode=preserve`,
against the `4d36cf61d` binary, through `tests/bin/check.sh`:

```
-VM(2) = 8.467330e-01
-VDB(2) = -1.44507e+00
-VP(2) = -5.60982e-01
-FF(2) = 6.000000e+00
-hh(5) = 1.000000e+01          <- 'define hh(X) x*2' never substituted its x
-vm(1) = 7.000000e+00
-VM (x) = (x)*(7)              <- 'define vm' could not see the VM entry
+gg(2) = 1.000000e+01          <- a second one: 'undefine GG' did nothing
+vm(1) = 1.000000e+00          <- and the call did not reach VM either
```

exit 1. Seven missing lines and two surplus ones, which is the whole of this
record in one diff — at least one line per site, including `trcopy()`'s and
`prdefs()`'s. The consistently spelled twin
`udf-name-case-lower.cir` **passes at the same commit**, which is what says
the assertion is about the case of the call and not about the circuit, the
analysis or the arithmetic.

The second mechanism has its own RED, against the **first commit's** binary
rather than the parent's, because that is the tree its own line was absent
from. Built and run rather than reasoned about, it is one line:

```
-hh(5) = 1.000000e+01
```

with the twin and the `casedist` deck passing there — the twin's formal and
body agree in case, so it never depended on `trcopy()`'s predicate.

`tests/regression/casedist/udf-name-case.cir` passes at the parent commit and
is not RED: it is the guard for decision 5's third row. It is not vacuous —
each of its four assertions fails if the fix reaches `distinguish`. `ff` and
`FF` carry different multipliers, so a run that collapsed them prints one
number twice instead of 6 and 10; `print VM(2)` has no line, so a run that
resolved it gains one; `define hh(X) x*2` has no line either, pinned between
`FF(2)` and `vm(2)`; and `undefine GG` leaves `gg` callable, so a run that
removed it loses a line.

### GREEN

All three decks pass against the built binary. The `.out` files predicted the
printed spellings as well as the numbers — `VM(2)`, `FF(2)` and
`VM (x) = (x)*(7)` — and were written before the fix was compiled.

The five hand measurements, re-run against the new binary:

| | at `4d36cf61d` | after |
| --- | --- | --- |
| `print F(2)` at the `ngspice -p` prompt, **fold** | `Error: no such function as F` | `F(2) = 6.000000e+00` |
| `print VM(1)` at the prompt, **fold** | `Error: no such function as VM` | resolves the function; fails on the vector, `no such vector 1`, because the prompt has no circuit |
| `define g(X) x*2`, `g(5)`, **preserve** | no line | `g(5) = 1.000000e+01` |
| `define g(X) x*2`, `g(5)`, **distinguish** | no line | no line — unchanged |
| `undefine F` after `define f(x)`, **preserve** | silent no-op, `f` still callable and still listed | `f` removed |
| `define f(x)` beside a stored `foo(y)`, **fold** | `foo` destroyed | `foo` destroyed — `0051`, deliberately unchanged |
| `define VD(x)` beside the shipped `vdb(x)`, **fold**, at the prompt | `vdb(x)` survives | `vdb(x)` survives — the row the first version of this record got wrong, and the reason decision 2 reversed |
| `define ff(X,x) X+x*10`, `ff(3,4)`, **preserve** | `4.3e+01` | `3.3e+01`, which is `fold`'s answer; `distinguish` keeps `4.3e+01` |

### `make check`

**282 PASS / 0 FAIL**, exit 0, against the recorded bar of 279 / 0 at
`4d36cf61d`. The three that appeared are the three decks this work adds:
`tests/regression/case` is 113 and `tests/regression/casedist` is 23, two and
one above the counts the bar implies. Nothing was lost, and the lint's own two
tests are in it and pass at 268 comparisons.

**One earlier full run reported 1 FAIL and it was the environment**, recorded
because a reader who finds it in a log should not have to re-derive it. Two
runs made while the differ was using the machine each failed one
`tests/regression/pipe` deck — a different one each time — immediately after
`X connection to :0 broken (explicit kill or server shutdown)`. That is Xlib's
fatal handler killing the process when the WSL X server exits under a
connection this suite holds open. Each deck passes on its own, at the same
commit and with the same binary, and the clean run above was made with the X
server down rather than dying.

### The sweep, compared per deck rather than by its total

`case_differential_sweep.py --jobs 8 --timeout 90`, run twice over a frozen
tree — once against the snapshot taken before any of this and once against the
rebuilt binary. Both runs see the three decks this work adds, so the deck count
is 321 rather than the recorded 318.

```
before   321 decks: DIFF=74, OK=244, SKIP=3     PARSE-FAIL=0  NUM-DIFF=0
after    321 decks: DIFF=61, OK=257, SKIP=3     PARSE-FAIL=0  NUM-DIFF=0
```

Thirteen decks clear and **none newly differs**:

```
DIFF only before:  tests/regression/case/compat-c-behav-case.cir
                   tests/regression/case/compat-c-behav-case-lower.cir
                   tests/regression/case/compat-l-behav-case.cir
                   tests/regression/case/compat-l-behav-case-lower.cir
                   tests/regression/case/compat-k-mutual-case.cir
                   tests/regression/case/compat-k-mutual-case-lower.cir
                   tests/regression/case-lt/rkm-c-case.cir
                   tests/regression/case-lt/rkm-c-case-lower.cir
                   tests/regression/case-lt/rkm-l-case.cir
                   tests/regression/case-lt/rkm-l-case-lower.cir
                   tests/regression/casedist/udf-name-case.cir
                   tests/regression/case/alter-rebin-case.cir
                   tests/bsim3soidd/ring51.cir
DIFF only after:   (none)
```

**`0020`'s criterion 3 predicted eight; the ten decks it names cleared, and
the criterion undercounted its own list.** It names
`compat-c-behav-case.cir`, `compat-l-behav-case.cir`,
`compat-k-mutual-case.cir`, `rkm-c-case.cir` and `rkm-l-case.cir` *"with their
twins"* — five decks with twins is ten, not eight, and
`tests/regression/case-lt/` does carry `rkm-c-case-lower.cir` and
`rkm-l-case-lower.cir`. So the deck list was right and the arithmetic on it
was not; the same mis-split is in
`doc/claude/checklists/phase2-differential-sweep.md`, where it originates, and
is corrected there. This is the only numeric prediction this series has made
in advance that a later session could check, and checking it per deck rather
than by the total is what shows which half was wrong.

Of the other three, one is this work's `casedist` deck, `DIFF` at the parent
for exactly the behaviour being fixed. The other two are not this work's:
`alter-rebin-case.cir` is the recorded flapper — that family gives `DIFF=4`
before and 3 after, inside its recorded ±2, and is also why the before run's
74 sits two above the 72 the recorded 69-at-318 predicts once three decks are
added — and `tests/bsim3soidd/ring51.cir` reports `stdout reordered only`,
which is thread-ordering nondeterminism rather than a comparison.

**Two decks stay `DIFF` after, by design, and they are this work's own.**
`case/udf-name-case.cir` and its twin report
`preserve-UPPER: stdout +'vm (x) = mag (v (x))'`, and that one line is
decision 2's residue seen by a second tool: under `fold` the reader folds the
`define VM(x)` card so the shipped `vm(x)` is *replaced* and `define vm` lists
two entries, while under `preserve` it is only shadowed and three are listed.
The deck is on the sweep's by-design `DIFF` list beside `harness-alive.cir`
and `write-roundtrip.cir` until `doc/codex/issues/0051` lands, at which point
it should clear — which makes it a live check on that issue rather than noise.

`PARSE-FAIL` and `NUM-DIFF` are 0 in both runs, which is the pair that must
not move.

### The differ, all three modes

`deck_output_differ.py --before <the snapshot> --after
build-ver_50/src/ngspice --jobs 8 --timeout 200`, which compares two binaries
on **both** streams and on exit status:

```
decks: 321  mode: stock          moved=3   alter-rebin{,-lower}, alter-rebin-param
decks: 321  mode: preserve       moved=4   2 alter-rebin + two of this work's three decks
decks: 321  mode: distinguish    moved=0
```

- **`distinguish`: `moved=0`. Nothing in the tree moves at all**, over 321
  decks and both streams. That is decision 5's third row measured rather than
  reduced, and it is the strongest statement available about a change in
  `src/frontend/define.c`, which is on the path of every `print`, `let` and
  `meas` in the frontend. The flapper family happens not to flap in this run,
  which is inside its recorded ±2 and is why the number is 0 rather than 2.
- **`stock`: no deck outside the flapper family moves**, including this work's
  own three. That is decision 5's first row: a deck's `.control` block is
  folded by `inp_readall()`, so nothing a deck can express reaches the
  loosening, which only exists where the reader never folded — the prompt,
  `ngspice -p` and `ngSpice_Command()`, none of which a deck is.
- **`preserve`**: `case/udf-name-case.cir` and `casedist/udf-name-case.cir`
  move on `stdout`, which is the fix, and their `stderr` reports show it
  precisely — the upper deck loses `or VM(2) is not available.`,
  `or VDB(2)…` and `or FF(2)…` and gains the `gg` pair that
  `undefine GG` now causes; the `casedist` deck loses its `VM(2)` error and
  its `checkvalid: vector x` warning and gains nothing.
  **`case/udf-name-case-lower.cir` does not move**, in any mode, which is what
  makes it a control rather than a second copy of the assertion.

The four `alter-rebin` decks are the family this series has recorded as
flapping, with a recorded no-change baseline of fold 2 / preserve 3 /
distinguish 2 and a recorded ±2. All three modes sit inside it once this
work's own decks are set aside.

`--timeout 200` rather than the default, for `tests/mesa/mesa12.cir`'s ~143 s,
which at 120 reports as `rc TIMEOUT -> 1` — a MOVED line that is not a
difference.

## What this decision does not decide

1. **`doc/codex/issues/0051`**, filed by this work and now **blocking** one
   line of it: `com_define()`'s `prefix()` match, which lets `define f(x)`
   silently overwrite a stored `foo(y)` of the same arity in every mode and
   leak the name it overwrites. Decision 2 leaves that line byte-exact and
   states why it cannot be made case-blind first. `0051` also carries
   decision 4's deferred diagnostic — `undefine` says nothing on a miss, for
   any reason, in any mode — and the `preserve` residue decision 2 describes,
   with the deck lines that will have to change when it lands.
2. **A near-miss diagnostic for a function name under `distinguish`.**
   `0001` decision 2's rule applies to all three resolutions in this file and
   none of them implements it. `ft_substdef()`'s miss is at least loud —
   `Error: no such function as VM` — which is the state `0012` decision 2 was
   arguing *against* for the subcircuit pin, where the miss was silent. Here
   the report would name the twin rather than break a silence, which is a
   smaller gain; and `prdefs()` and `com_undefine()` are silent for every
   reason, which is `0051`'s criterion rather than this one's.
3. **Whether `cpitf.c`'s `udfs[]` should be exempt from `distinguish`.**
   Decision 1 rejects the per-entry bit and accepts that `print VM(2)` fails
   under `distinguish` for the same reason `q1#collCX` must be typed as the
   simulator spells it. If that is ever revisited, the place is `udfs[]` and a
   flag on `struct udfunc`, not the comparator.
4. **`cp_addkword(CT_UDFUNCS, b)` and `cp_remkword(CT_UDFUNCS, ...)`.**
   `com_define()` registers the spelling it stored and `com_undefine()`
   removes the spelling the user typed, so the two disagree exactly as
   `doc/codex/issues/0033` describes for `CT_VECTOR` — and for the same reason
   it is unfalsifiable: `cp_ccom()` has had no callers since `4feb0c3cc`.
   Recorded here so that `0033`'s eventual fix knows there are two of them.
5. **`doc/codex/issues/0047`, `0048`, `0046`, `0043`, `0042`, `0041`, `0039`,
   `0038`, `0035`, `0033`, `0031`, `0021`, `0050`, `0024`, `0025`**, `0016`
   class (b), `0005`, and `com_let()`'s `plainlet` leading space per `0009`
   deferral 4. Enumerated in this session's prompt as out of scope and
   untouched.
