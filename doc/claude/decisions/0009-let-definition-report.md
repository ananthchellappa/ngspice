# Decision 0009 — the near-miss report on a `let` that defines, `doc/codex/issues/0034`

## Status

Accepted, 2026-08-11, branch `ver_50`. Closes `doc/codex/issues/0034`. Applies
decision 2 of `doc/claude/decisions/0001-distinguish.md` — *silence when a name
is being defined, a warning when a name is being resolved and the resolution
fails* — to `vec_get()` and its forty-one callers, one layer above
`doc/claude/decisions/0004-unlet-vector-identity.md`, which did the same for
`vec_remove()` and its three.

Commits: `a94636777` the mechanism and the deck; this record and the three
issues its audit filed are the commit that adds this file.

## Context

`findvec()` (`src/frontend/vectors.c`) reports a case near miss whenever a
lookup fails under `distinguish` and the plot holds a permanent vector
differing from the name asked for only in case. `com_let()` reached it through
`vec_get()` to decide whether to overwrite an existing vector or create a new
one, so when it created — the ordinary outcome — the warning fired on a
*definition*. `0001` decision 2's table said so in one line: **"reports, and
should not."**

Re-measured at `abd0e2f17`, since `0034`'s Impact was taken at `496110f27`:

```
let TIME = time * 2   Warning: no vector named 'TIME'; 'time' differs only in case
let VAA = 2  (Vaa)    Warning: no vector named 'VAA'; 'Vaa' differs only in case
compose WBB values .. (Wbb)   silent
cross XCC 0 in        (Xcc)   silent
setscale wBB          (Wbb)   Warning: ... 'Wbb' differs ... , then Error: no such vector as wBB.
```

Both `let` shapes still fire and the second-order inconsistency `0034` records
is still live: `compose` and `cross` reach `vec_remove()` with
`report_case_miss` false and are deliberately silent, so before this record
three commands that all define a vector diagnosed two different ways.

Two things the audit added to that Impact, both measured rather than reasoned:

- **It fires in a session with no user vectors at all.** `vec_get()` retries
  the const plot (`vectors.c`), whose permanent vectors are the twelve
  `predefs[]` of `src/frontend/cpitf.c` — `yes`, `TRUE`, `no`, `FALSE`, `pi`,
  `e`, `c`, `i`, `kelvin`, `echarge`, `boltz`, `planck`, with `TRUE` and
  `FALSE` stored upper case. So `let PI = 3` warns about `pi`, and `let I = 1`
  about `i`. Single letters and `PI` are ordinary script-variable names. Before
  the first analysis it warns **twice**, because `plot_cur` is then the const
  plot itself and both scans read the same table; that duplication is
  `doc/codex/issues/0046` and is not this record's, but it is why the quoted
  measurement below shows one line where a run shows two.
- **It reaches `.meas` and `.csparam`.** `com_let()` is not only the `let`
  command: `measure.c:139` runs it for a `.meas` result vector and
  `inp.c:1049` for `.csparam name=value`, both with names the deck wrote. The
  fix covers them because it is at `com_let()`.

Nothing in this record changes a computed value in any mode.

---

## Decision 1 — a silent sibling of `vec_get()`, not a flag on it

**Decided: `vec_get()` keeps its name and its behaviour for all forty other
callers, and `vec_get_quiet()` is added beside it. The two share
`vec_get_maybe_report()`, which takes `report_case_miss` and threads it through
`vec_fromplot_maybe_report()` into `findvec()`.**

`0004` decision 3 chose the opposite shape one layer down — a `bool` on
`vec_remove()` itself, rejecting a second entry point because "it hides the
classification inside a name instead of stating it at each call site" — and
this record does not inherit that answer, because the thing that made it right
is inverted here.

`vec_remove()` has **three** callers and the split is 1 : 2, so every call site
carries an informative `TRUE` or `FALSE`. `vec_get()` has **forty-one** and the
split is 40 : 1. A flag on `vec_get()` would write `TRUE` at forty sites where
it says nothing, which is the churn `0034` criterion 2 rules out and which
`CLAUDE.md` rules out generally. The classification is instead stated where it
belongs, in a comment at the one call site that is not a resolution — exactly
as `com_compose.c:119` states it beside its `vec_remove(resname, FALSE)`.

Rejected: **splitting `findvec()` into a reporting and a silent form**, `0034`
criterion 2's third candidate. It does not reach the caller that needs it.
`com_let()` calls `vec_get()`, which calls `vec_fromplot()`, which calls
`findvec()`; a split at the bottom still has to be selected from the top, so
the parameter has to be threaded either way. It is threaded, and `findvec()`
does get the flag — but as an argument, not as a second function, because
`findvec()` is `static` and its two call sites are both inside
`vec_fromplot_maybe_report()`.

Rejected: **a file-static "next lookup is a definition" flag** set by
`com_let()` around its call. One line and no signature changes, and wrong:
`vec_get()` is re-entered from expression evaluation, and global simulator
state has to survive repeated `ngSpice_Reset` in the shared build, which
`CLAUDE.md` names as a class of bug this tree has already had twice.

**`vec_fromplot()` keeps its two-argument signature.** It is public
(`src/include/ngspice/fteext.h`) with **fourteen** callers outside `vectors.c`,
in five files — `linear.c` (7), `breakp.c` (2), `evaluate.c` (2), `graf.c` (2)
and `ciderlib/support/database.c` (1) — and every one of them is resolving a
name, so the public entry point passes `TRUE` and nothing outside the file
moves. The `ciderlib` one is not compiled in this configuration (CIDER needs
`--enable-cider`) and does not need to be: only the *body* of `vec_fromplot()`
moved, into `vec_fromplot_maybe_report()`, and the declaration every caller
compiles against is unchanged.

**What `vec_get_quiet()` silences is exactly one line.** The wildcard error
(`plot wildcard (name %s) matches nothing`) and the `@` diagnostics inside
`vec_get()` are unchanged in both forms. The name says "quiet" and the comment
above it says what that means, because "quiet" on its own would over-promise.

## Decision 2 — the indexed form keeps the report

**Decided: `com_let()` passes `vec_get_quiet()` only when `index_start` is
NULL; `let x[0] = 1` keeps `vec_get()`.**

This is the correction an adversarial review of the first version of the fix
produced, and it is the substantive half of this record.

`let x[0] = 1` **cannot create `x`**. `com_let()` refuses it at
`com_let.c:162`: *"When creating a new vector, it cannot be indexed."*
So the indexed form is not defining a name — it is *resolving* one that must
already exist, and its miss is a reported user error. That is decision 2's
resolution arm exactly, and the near miss is the only thing in the output that
explains why a vector the user can see on the screen was not found.

Measured, on a deck with a `Qqq`:

```
blanket suppression   let QQQ[0] = 9   When creating a new vector, it cannot be indexed.
this record           let QQQ[0] = 9   Warning: no vector named 'QQQ'; 'Qqq' differs only in case
                                       When creating a new vector, it cannot be indexed.
```

`index_start` is known at `com_let.c:75`, forty lines before the lookup at
`:115`, so the condition costs nothing.

The first version of this fix suppressed unconditionally and the deck as first
written did not notice, because it had no indexed case. It has one now, in the
same capture as the silence it bounds, and that case is RED against the blanket
version and GREEN against this one.

## Decision 3 — the audit: forty-one call sites, and a third category

`0034` criterion 3 asks for the other definition-shaped callers.
**There are none.** `com_let()`'s left-hand side is the only `vec_get()` call
site in the tree that creates the name it looks up. What the audit found
instead is a category `0001` decision 2 does not name.

First, a correction to the number. The prompt for this work says `vec_get()`
has **"45 call sites in 19 files"**. It has **41 call sites in 17 files**.
`grep -rn vec_get src/` returns 47 hits in 20 files; six are not calls — the
definition at `vectors.c`, two `fprintf` format strings inside it that mention
the name, a comment at `parse.c:599`, the declaration in
`src/include/ngspice/fteext.h`, and the no-op stub `struct dvec
*vec_get(const char *word) { NG_IGNORE(word); return (NULL); }` at
`src/ngsconvert.c:536`, which is the `ngsconvert` link target's replacement and
can never reach `findvec()`. 45/19 is 47 minus the declaration and the comment.
The sub-claim that `com_measure2.c` alone has 18 is correct.

The three categories:

- **Resolution** — the caller treats a miss as a failure, usually by printing
  one. The report is right and nothing changes. **38 sites.**
- **Definition** — the caller creates the name. The report is wrong.
  **1 site**, fixed here, less its indexed form.
- **Probe** — the caller is asking *whether* the name exists in order to decide
  what kind of token it is. A miss is the answer, not a failure. **2 sites**,
  not fixed; `doc/codex/issues/0044` and `0045`.

| Site | Enclosing function | What it looks up | Class | On a miss | Near miss reachable |
| --- | --- | --- | --- | --- | --- |
| `com_let.c:101` | `com_let()` | the LHS the user typed | **definition** | creates the vector | yes — the whole issue |
| `com_let.c:101`, indexed | `com_let()` | the same, with `[...]` | resolution | *"cannot be indexed"* | yes, decision 2 |
| `com_let.c:164` | `com_let()`, `plainlet` | the RHS | resolution | `Can't evaluate "%s"` | yes |
| `com_pyplot.c:53` | `com_pyplot()` | first word: file name or vector | **probe** | word becomes the file name | yes — `0044` |
| `parse.c:575` | `PP_mksnode()` | any identifier in any expression | **mixed** | zero-length placeholder | yes — resolution for `check == TRUE` callers, **probe** for the three `check == FALSE` ones; `0045`. **Corrected by `0010`:** the split is not `check`. `device.c:1433` parses with `check == FALSE` and is a resolution, and within the other two only the *token* the caller has another meaning for is a probe |
| `parse.c:500` | `PP_mkfnode()` | a synthesised `func(arg)` | resolution | `no such function as %s` | unlikely — needs a permanent vector literally named `Foo(bar)`. **Corrected by `0010`:** measured, and **reachable** — `let foo(bar) = 1` makes one, and `print FOO(BAR)` then prints the near miss before `no such function as FOO`. It is a resolution and the report is right, so nothing moves |
| `com_setscale.c:19` | `find_vec()`, both operands | a word the user typed | resolution | `no such vector as %s.` | yes — asserted by the deck |
| `com_display.c:41` | `com_display()` | a word the user typed | resolution | `no such vector as %s.` | yes |
| `com_compose.c:233` | `com_compose()`, `device` | `resname`, always `@...` | resolution | loop skipped, silent | no — compose renames its own output `@r1_resistance`, and `vec_get()`'s `@` dvecs are allocated without `VF_PERMANENT`, so neither can be a twin |
| `com_measure2.c:400`, `:403`, `:683`, `:791` | `com_measure_when()`, `measure_at()`, `measure_minMaxAvg()` | `meas->m_vec`, `m_vec2` | resolution | `no such vector as %s.` | yes |
| `com_measure2.c:811`, `:818`, `:825`, `:827`, `:829`, `:831` | `measure_minMaxAvg()` | the literals `frequency`, `time`, `v-sweep`, `i-sweep`, `temp-sweep`, `res-sweep` | resolution | the cascade's collective miss errors | no — the analysis writes those scales lower case, and a twin needs a user vector spelled `Time` or `V-Sweep`, which `let` cannot name because of the hyphen |
| `com_measure2.c:1009`, `:1016`, `:1023`, `:1030`, `:1032`, `:1034`, `:1036` | `measure_rms_integral()` | the same six literals and `meas->m_vec` | resolution | as above | as above — this block is byte identical to the one above but for the local name, and gets the same answer, which is the point of saying so |
| `com_measure2.c:1275` | `measure_valid_vector()` | `varname` | resolution | three of its four callers print `no such vector as '%s'` | the fourth caller, `:1518`, is a probe, but only numbers and expressions reach it and neither can have a twin |
| `measure.c:83`, `:100` | `com_meas()` | the token after `=` in a `.meas` line | resolution | token left alone, and `com_measure2.c:400` reports it | yes — and the near miss is the *earlier* and more informative of the two |
| `newcoms.c:153`, `:172` | `com_reshape()` | a word the user typed | resolution | `'%s' vector not found` | yes |
| `numparam/xpressn.c:1113`, `:1124` | `formula()`, `vec()` in a `.param` | a word the deck wrote | resolution | `u = 0`, **silently** | yes — the near miss is the only diagnostic `vec()` has, which is a reason to keep it. `:1124` re-probes the const plot that `vec_get()` already probed; see `doc/codex/issues/0046` |
| `options.c:136` | `cp_enqvec_as_var()`, i.e. `$&name` | a word the user typed | resolution | `%s: no such variable.` | yes. The `$?&` and `$#&` forms do not reach here at all — the lexer splits the `&` — so there is no probe caller |
| `plotit.c:744` | `plotit()`, `plainplot` | a word the user typed | resolution | error, `goto quit` | yes |
| `postcoms.c:638` | `com_write()`, `plainwrite` | a word the user typed | resolution | error | yes |
| `postcoms.c:809` | `com_write_sparam()` | the literal `"Rbase"` | resolution | `No Rbase vector given` | yes, and usefully: the literal is mixed case, so a deck that did `let rbase = 50` is told why |
| `postcoms.c:930` | `com_transpose()` | a word the user typed | resolution | `no such vector as %s.` | yes |
| `typesdef.c:372` | `com_stype()` | a word the user typed | resolution | `no such vector %s.` | yes |
| `sharedspice.c:1215` | `ngGet_Vec_Info()` | the caller's string, public API | resolution | returns NULL | yes |
| `tclspice.c:536` | `vectoblt()` | the Tcl caller's string | resolution | error | yes |
| `xspice/evt/evtprint.c:520` | `get_real()`, `eprvcd` | the literal `"time"` | resolution | `ERROR - No vector 'time' in current plot` | unlikely, and identical in shape to `com_measure2.c:818`, so it gets the identical answer |

Two recommendations that came out of the audit are **rejected**, and both were
rejected by reading the code rather than by counting votes:

- **Silencing `com_measure2.c`'s scale cascade.** The argument for it is that a
  miss on `v-sweep` is expected when the run was a temperature sweep. True, and
  irrelevant: those six names are written by the analysis in lower case and no
  user vector can collide with them by case alone, so the report is not
  reachable and silencing it would be churn with no observable effect. The same
  argument disposes of `evtprint.c:520`.
- **Silencing `com_compose.c:233`.** `compose @r1[resistance] device` reads a
  device parameter; the *name* claim was already made silently by
  `vec_remove(resname, FALSE)` fifteen lines earlier, and the lookup that
  precedes the `@` branch cannot find a twin for the reason in the table.

## Decision 4 — mode independence, stated and measured

The report is inside `if (!d && report_case_miss && inp_case_mode() ==
NG_CASE_DISTINGUISH)`, so removing it for one caller cannot move `fold` or
`preserve`: the guard was already false there. That is the cheap proof and it
is one `if`.

The sweep was still run, because the *mechanism* touches code every mode uses:
`findvec()` gained a parameter, `vec_fromplot()` became a wrapper, and
`vec_get()` became one. What has to be shown is that no lookup changed its
answer, not merely that no warning moved. `make check` (272 decks, default
`fold`), `tests/regression/case` (109 decks, `preserve`) and the differential
sweep all say it did not; the Evidence section has the numbers.

## Decision 5 — `let` cannot take a redirect, so the deck sources a sub-deck

**Decided: `tests/regression/casedist/vector-let-report.cir` writes each
circuit with `echo` and sources it with `>&`, the shape
`bsource-undef-node-report.cir` uses.**

`0034`'s prompt expects the simpler shape — *"`let` is a command, so the
redirect attaches to it directly and no `source` wrapper is needed"* — and that
is wrong. `let` is on `noredirect[]` at `src/frontend/control.c:62`, beside
`if`, `stop`, `define` and `circbyline`, and for a good reason: `let a = b > c`
needs `>` as an operator. So `let VAA = 2 >& f` parses the redirect tokens into
the right-hand side and dies with `Error: RHS "2 > & cap.txt" invalid`, having
printed the warning to the real `stderr` where no deck can see it.

`source` is the way in. `inp_spsource()` sets `cp_curout`/`cp_curerr` to the
redirect targets for the duration (`src/frontend/inp.c:629-634`), so
`cp_ioreset()` at the top of each command inside the *sourced* deck's own
`.control` block restores to the capture file rather than to the terminal. That
is one step further than `0006` decision 2 needed — that record only had to get
the redirect as far as the sourced *parse* — and it is what makes a command on
`noredirect[]` assertable at all.

`setscale` is **not** on the list, so the deck's fourth case attaches `>&`
directly and is the shorter shape for comparison.

## Decision 6 — what each case of the deck pins

Every silence is asserted beside something in the **same capture** that must be
reported, which is the stronger of the two shapes `0008`'s review ended with;
no `CAPTURE-READ` token is needed, because a phrase that is found proves its
own capture was read. In each case the thing that must be reported is a
right-hand side, so one capture carries both arms of decision 2.

| Case | Silent | Reported in the same capture | What only this case pins |
| --- | --- | --- | --- |
| PLOT | `let op1.NNN = 8` beside an `Nnn` in `op1`, run while the const plot is current | `let Aaa = v(OUt)` | `vec_get()`'s plot-prefix branch, a second path into the same lookup. It **cannot pass vacuously**: had the `op1.` prefix not resolved to a plot inside `vec_get()`, the lookup would have been for `op1.NNN` in the const plot, which has no twin, and the case would have been silent against the *unfixed* binary too. It is not — that is the RED |
| SCALE | `let TIME = time * 2` beside a tran run's own scale | `let Zzz = v(OUt)` | the twin is a **simulation** vector rather than one a `let` made, and this is the shape `0034`'s Impact measured |
| LET | `let VAA = 4` beside a `Vaa` | `let Aaa = v(OUt)` **and** `let QQQ[0] = 9` beside a `Qqq` | the plain case, its two values, and decision 2's boundary in the same capture as the silence it bounds |
| SCALECMD | — | `setscale wBB` beside a `Wbb` | a **second caller** of `vec_get()`, resolving rather than defining, still reports. This is the audit's answer made assertable: the fix is narrow |

The value half `0034` criterion 4 asks for is asserted twice, in the wrapper's
own plot (`Vaa = 1`, `VAA = 2`) and again from the sourced run (`Vaa = 3`,
`VAA = 4`), the second of which also proves the sub-deck's control block really
executed.

## Evidence

Every RED is a `make check` failure against a binary built before the change,
not a hand run, and the deck was run against an **empty** `.out` first so that
its evidence is output rather than an absence.

| Step | Binary | Result |
| --- | --- | --- |
| empty `.out` | either | FAIL, **30 added lines** |
| RED 1 | `abd0e2f17` | FAIL, exactly three lines: `LET-PLOT-`, `LET-SCALE-` and `LET-DEFN-` `SILENT` → `REPORTED` |
| RED 2 | blanket suppression, no `index_start` guard | FAIL, exactly one line: `LET-INDEXED-REPORTED` → `SILENT` |
| GREEN | this record | PASS |

In RED 1 the three `-RHS-REPORTED` lines, `LET-INDEXED-REPORTED`,
`SETSCALE-MISS-REPORTED` and all six value lines pass unchanged, which is what
says the deck asserts the report's absence and not a side effect of removing
it. In RED 2 everything except the indexed line passes, which is what says
decision 2 has its own RED and is not carried by the rest.

- `make check`: **272 PASS, 0 FAIL**, up from 271 at `abd0e2f17` by exactly the
  one deck this work adds. `tests/regression/casedist` 18 → 19;
  `tests/regression/misc` 27, `tests/regression/case` 109,
  `tests/xspice/case` 21 and `tests/xspice/casedist` 17 unchanged.
- `python3 doc/claude/scripts/case_differential_sweep.py --jobs 8 --timeout 90`
  on snapshots of the binary before and after: **312 decks, DIFF=68, OK=241,
  SKIP=3** before and **313 decks, DIFF=67, OK=243, SKIP=3** after,
  `PARSE-FAIL=0` and `NUM-DIFF=0` in both. Compared per deck rather than by
  totals: the non-`OK` lines differ by the one deck this work adds — a
  `preserve-UPPER` `DIFF` of the class the six existing `*-report` decks
  already have, because the sweep uppercases the `echo > vlr_plot.cir` line and
  not the `source vlr_plot.cir` that reads it — and by two `alter-rebin` lines,
  which are the known flap and were re-measured rather than assumed:
  `--filter alter-rebin`, three runs on each binary, gave DIFF = 3, 4, 4 on the
  before binary and 4, 4, 4 on the after one, so it moves on an unchanged
  binary.
- `python3 doc/claude/scripts/deck_output_differ.py`, added by this work: every
  `.cir` under `tests/` — 313 — run directly with the before and after
  binaries under each of `fold`, `preserve` and `distinguish`, comparing
  stdout, stderr and exit status: **`fold` 0 of 313, `preserve` 1, and
  `distinguish` 5**. `make check` cannot see a `Warning` line and the sweep
  compares three case modes of one binary rather than two binaries, so neither
  could have said this.

  Of the six, four are the `alter-rebin` flap — one in `preserve` and three in
  `distinguish` — and the flap was **reproduced on one unchanged binary rather
  than asserted**: eight concurrent runs of
  `tests/regression/case/alter-rebin-case-lower.cir` with the *before* binary
  in `preserve` gave two distinct outputs, five of one and three of the other,
  differing by whether a ` Reference value :  0.00000e+00` line is emitted at
  all. Run serially, before and after are byte identical.

  The fifth is this work's own deck. **The sixth is the interesting one**, and
  it is the fix confirmed from outside its own deck:
  `tests/regression/casedist/vector-scale-case.cir` has `let TIME = time * 2`
  at line 56 and a `print TIME` at line 73, after an `unlet TIME`. It used to
  print the near miss **twice** and now prints it once. The one that went is
  the `let` — a definition. The one that stayed is the `print` after the
  `unlet` — a resolution whose miss is real. Its stdout is byte identical, and
  `make check` passes it before and after, which is exactly why a tool was
  needed to see it.

  The mask that makes that number meaningful had to be earned. The first run
  reported five moved decks in the default mode and **every one of them
  differed between two runs of the same binary**: `tests/general/mosamp.cir`
  moves its `Reference value` line, `tests/mesa/mesa12.cir` its analysis
  banner, which carries a wall-clock stamp, and three were `alter-rebin`. The
  script says so in its own header, because the next reader will meet it.

## Corrections found by reviewing this work

The step `0027`'s session added and every session since has repeated. Four of
these were found before anything was committed and one after.

**The fix was wrong for the indexed form**, and the deck as first written could
not see it. That is decision 2 and it is the largest of the five. The order in
which it was found matters: the deck went GREEN, `make check` went 272, and
only then did reading the classification back against the code turn up a branch
of `com_let()` that resolves. A deck that passes is not the end of the review.

**Three factual claims in this record's first commit message were wrong**, all
found by re-grepping rather than by re-reading. `vec_fromplot()` has
**fourteen** callers outside `vectors.c`, not twelve. The deck adds **30** lines
against an empty `.out`, not 29 — 29 was measured before decision 2 added the
indexed case, and re-measuring after changing the deck is the step that was
skipped. `let PI = 3` prints the warning **twice**, not once, which the quoted
measurement flattened. The message was amended before anything was built on it.

**The prompt for this work states two things that are not so**, and both were
load-bearing. `vec_get()` has 41 call sites in 17 files, not "45 in 19";
decision 3 shows how 45 is reachable by counting four non-calls. And `let`
**cannot** take a redirect, so the deck needs the `source` wrapper the prompt
says it does not; decision 5 is that, and `src/frontend/control.c:62` is why.

**The differ's first answer was five moved decks in the default mode and every
one of them was a flap.** Believing it would have produced a record claiming
this change moves `tests/general/mosamp.cir`. The check that disposed of it —
run the deck twice against the *same* binary — cost one command, and for
`alter-rebin-case-lower.cir` it took eight *concurrent* runs of one binary to
reproduce, because that deck's flap only appears under load. Any tool that
compares two runs needs that check wired into how its output is read; the
script's header says so.

## What this decision does not decide

1. **`doc/codex/issues/0044`**, `com_pyplot()`'s file-name probe. It is the
   audit's third category and it needs a rule rather than a patch, because the
   argument against silence is real and measured: `pyplot out` with no plot
   arguments returns before `plotit()` without writing or saying anything, so
   the near miss is the *entire* diagnostic a user who mistyped a vector's case
   would get. `measure.c:83`/`:100` are probes of the same shape.

   **Decided 2026-08-11 by `doc/claude/decisions/0010-probe-category.md`:**
   silent, and the objection above is removed rather than paid — that return
   now reports on its own account. `measure.c:83`/`:100` **keep** the report:
   `com_meas()` acquires no other meaning for the token, and
   `com_measure2.c:400`/`:403` report the same name afterwards.
2. **`doc/codex/issues/0045`**, `PP_mksnode()`. `define f(x) x*2` on a deck with
   a node `X`, and `plot a vs b` on a deck with a node `VS`, both warn and both
   then work. The fix threads `ft_getpnames()`'s existing `check` flag down to
   the lookup; three callers pass it false. `vec_get_quiet()` is the lookup it
   will call, which is the second user this record's new function will get.

   **Decided 2026-08-11 by `doc/claude/decisions/0010-probe-category.md`:**
   silent, and `vec_get_quiet()` is indeed the lookup — but **not** through the
   `check` flag, and this paragraph's last sentence is the thing that record
   had to correct. The third `check == FALSE` caller, `device.c:1433`, is a
   *resolution* whose miss `ft_evaluate()` reports, and even within the two
   that are probes only one token of the parse is. The flag threaded is a
   per-token list of probe names, not `check`.
3. **`doc/codex/issues/0046`**, the duplicate report. `vec_get()` scans
   `plot_cur` and then the const plot, so before the first analysis — when
   `plot_cur` **is** the const plot — the same line prints twice.
4. **`com_let()`'s `plainlet` leading space.** `wl_flatten()` rejoins words with
   single blanks and nothing left-trims the right-hand side, so at the
   interactive prompt `set plainlet` + `let a = Foo` looks up `" Foo"` and
   fails whatever the case mode; inside a deck the reader has already removed
   the spaces around `=`, so it works. Pre-existing, orthogonal to case, and
   not filed: it wants a decision about where `com_let()` should trim.
5. **The `@dev[param]` paths**, which `0034` criterion 3 names as candidates.
   They are resolutions, and the table above says why their near miss is not
   reachable rather than why it would be wrong.
6. **The build-enforced lint.** It still goes last. With this record
   `vec_get()` has one reporting entry point and one silent one, which is the
   state a lint over `findvec()`'s report would freeze — but `0044` and `0045`
   are two more callers that will move through the silent one, so the lint
   waits for them. **Both are closed by
   `doc/claude/decisions/0010-probe-category.md`, 2026-08-11**, and
   `vec_get_quiet()` has three users; nothing now stands before the lint.
