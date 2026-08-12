# Decision 0010 — the probe, `doc/codex/issues/0045` and `doc/codex/issues/0044`

## Status

Accepted, 2026-08-11, branch `ver_50`. Closes `doc/codex/issues/0045` and
`doc/codex/issues/0044`. Writes the third row of decision 2 of
`doc/claude/decisions/0001-distinguish.md` — the row that record's table has
been carrying as *"reports, and the rule does not yet say whether it should"*
since `doc/claude/decisions/0009-let-definition-report.md`'s audit found the
category.

Commits: `ec050a9ff` the `0045` mechanism and its deck, `2aa8ca1cc` the `0044`
mechanism and its deck; this record and the two Resolutions are the commit that
adds this file.

## Context

`0009` classified all forty-one `vec_get()` call sites into **resolution** (38),
**definition** (1, fixed there) and **probe** (2, left). A probe asks *whether*
a name exists in order to decide what kind of token it is; a miss is the answer,
not a failure. Both probes reported a case near miss under `distinguish`, on
decks that were correct. Re-measured at `882377ee9`, on decks with nets `X`,
`VS` and `Out`:

```
define f(x) x*2        Warning: no vector named 'x'; 'X' differs only in case
print f(2)             f(2) = 4.000000e+00

hardcopy f.svg v(X) vs v(in)
                       Warning: no vector named 'vs'; 'VS' differs only in case
                       The file "f.svg" may be printed on a postscript printer.

pyplot out             Warning: no vector named 'out'; 'Out' differs only in case
                       and then nothing at all -- no file, no message
```

`pyplot out v(Out)` warns the same way and then writes `out.py` correctly. That
half is `0044`'s measurement at `abd0e2f17` and was **not** re-run here: it
starts a python interpreter, which is also why no deck asserts it.

Nothing in this record changes a computed value in any mode.

---

## Decision 1 — a probe is silent, and the unit is the **token**, not the parse

**Decided: silence, at both sites — but only for the exact identifiers the
caller already has another meaning for. Every other identifier in the same
expression is a resolution and keeps the report.**

This is the third row of `0001` decision 2's rule:

> *silence when a name is being defined; silence a probe on the token it is
> probing; a warning when a name is being resolved and the resolution fails.*

`0044` criterion 1 asks whether a probe is silent and says the argument against
is real. It is, and both halves of it are answered by making the unit the token:

- **For silence.** The miss is the answer the caller asked for. `define f(x)`
  and `plot a vs b` are ordinary lines, and warning on them is the failure
  `0001` decision 2 rejects by name for definitions — it trains users to ignore
  the warning that matters. A deck with a supply net called `VS` warned on
  *every* `plot a vs b`.
- **Against silence**, measured rather than argued: `pyplot out` with **no plot
  arguments** took `out` as the file name, found nothing left to plot, and
  returned having written nothing and said nothing. The near miss was the
  *entire* diagnostic a user who mistyped a vector's case got, so silencing it
  alone would have turned a typo into a silent no-op — the failure mode this
  series exists to remove, arriving from the other direction. Decision 4
  removes that objection at its source rather than paying it.

The two sites are **not** distinguished, and the candidate distinction the
prompt for this work offers — that `PP_mksnode()`'s callers have another name
for the token while `com_pyplot()`'s does not — does not survive contact with
the code. `com_pyplot()` does have another name for it: *output file name*. What
separates them is not whether the caller has another meaning but whether the
caller's path after the miss can be **silent**, and that is a property of the
caller, fixable there.

What the rule decides, token by token — which is where all of its content is:

| Lookup | Class | After this record |
| --- | --- | --- |
| `define f(x) x*2`, the formal `x` | probe — `trcopy()` (`define.c:343`) finds formals by matching this very placeholder | **silent** |
| `define g(y) y*Zzz`, the body's `Zzz` | resolution | **reports**, and must: the tree is frozen at definition time, so a body reference that misses now can never resolve later. Measured — `define h(y) y*Www`, then `let WWW = 7`, then `h(3)`: still `vector Www is not available` |
| `plot a vs b`, the separator | probe — `plotit.c:833` finds it by testing `v_length == 0 && eqc(v_name, "vs")` | **silent** |
| `plot a vs NNN`, the vector `NNN` | resolution — `ft_evaluate()` prints `no such vector NNN` | **reports** |
| `pyplot out v(Out)`, the first word | probe — the miss makes it the file name | **silent**, with decision 4 |
| `pyplot f OUt`, the plot argument | resolution | **reports** |
| `.meas ... = token`, `measure.c:83`/`:100` | probe in shape, resolution in fact | **reports**, decision 5 |

## Decision 2 — `check == FALSE` is not the probe bit, and `0045` says it is

**Decided: the `check` flag of `ft_getpnames()` is not threaded down. A separate
list of probe names is, and `src/frontend/device.c:1433` — one of the three
`check == FALSE` callers `0045` names — is left alone.**

This is the substantive half of this record and it is a correction to the issue
it closes. `0045`'s Summary says *"three callers ask for a parse with `check`
false and use the placeholder as a name rather than as a failure"*, and names
`device.c:1433` on the strength of a grep, recording that it *"was not
measured"*. Criterion 3 then asks for exactly that flag to be plumbed down.

Measured, `com_alterparam()`'s right-hand side on a deck with a vector `Rval`:

```
alter R1 = Rval      @R1[resistance] = 2.000000e+03
alter R1 = RVAL      Warning: no vector named 'RVAL'; 'Rval' differs only in case
                     Error: no such vector RVAL
```

The placeholder is **not** used as a name there. It is passed to `ft_evaluate()`
(`device.c:1483`), which fails and says so, and the near miss is the half of
that pair which explains *why*. `check == FALSE` at that call site buys the
fall-back to the `alter @vin[pulse] = [ ... ]` numeric-list path when the parse
fails outright, not a name. So `check` separates *when* the failure is reported,
not *whether* the miss is a failure — threading it down would have silenced a
resolution and thrown away the informative half of a real error message.

The same measurement disposes of the idea one layer down. Inside the two callers
that *are* probes, `check == FALSE` covers the whole parse while only one token
in it is a probe: `hardcopy f.svg NNN vs v(in)` on a deck with an `Nnn` must
report `NNN` and stay quiet about `vs`, and both come out of one parse. A
per-parse bit cannot say that. A per-token list can, and the caller is the only
thing in the tree that knows which token it means.

## Decision 3 — the probe list is a parser parameter, not a flag and not a static

**Decided: a third `%parse-param` on the grammar, `const char * const *probes`,
a NULL terminated array of names matched byte exactly by `is_probe_name()`
(`parse.c:607`), consulted by `PP_mksnode()` (`parse.c:621`) to choose between
`vec_get()` and `0009`'s `vec_get_quiet()`.**

`vec_get_quiet()` is the lookup either way, which is what `0045` criterion 3
asks for; this is its second user and the test `0009` decision 1 set for whether
it was named well. It was: the call site reads
`is_probe_name(string, probes) ? vec_get_quiet(string) : vec_get(string)`.

The signature change was counted before it was chosen, which is what `0045`
criterion 3 asks for. Outside `parse.c` the three entry points have, at
`882377ee9`, **19 call sites in 11 files** — `ft_getpnames()` 5 in 3,
`ft_getpnames_quotes()` 11 in 8, `ft_getpnames_from_string()` 3 in 1 — of which
**two** want a probe list. That is `0009` decision 1's 40 : 1 again in
miniature, so it gets `0009` decision 1's answer rather than
`doc/claude/decisions/0004-unlet-vector-identity.md` decision 3's: the existing
three entry points keep their names and their behaviour and become one-line
wrappers, and `ft_getpnames_probe()` (`parse.c:73`) and
`ft_getpnames_quotes_probe()` (`parse.c:124`) are added beside them. Both funnel
into the static `getpnames_from_string()` (`parse.c:40`), which is the old body
of `ft_getpnames_from_string()` with the parameter added. Every existing caller
passes NULL, `is_probe_name()` returns FALSE on the first line, and the lookup
is `vec_get()` exactly as before.

Rejected: **a file-static "the next lookups are probes" in `parse.c`**, set
around the `PPparse()` call. It is smaller and needs no grammar change, and
`0009` decision 1 rejected the same shape for `com_let()` for a reason that
applies here too — global state has to survive repeated `ngSpice_Reset` in the
shared build, which `CLAUDE.md` names as a class of bug this tree has had twice.
The grammar already carries one such static, `keepline`, and it is not a
precedent worth extending. A `%parse-param` is checked by the compiler at every
action, costs one argument, and cannot leak between parses.

Rejected: **removing `plotit()`'s "gross hack"** — splitting the wordlist at
`vs` before parsing so the separator never reaches a lookup. That is the real
fix for the comment at `plotit.c:783-796` that calls itself one, and it is not
a case-mode change: it rewrites the argument handling of every `plot`,
`hardcopy` and `pyplot` in the tree. Filed as `doc/codex/issues/0047`.

**`PP_mkfnode()` does not take the list.** Its own `PP_mksnode()` call
(`parse.c:543`) is reached only after `vec_get(buf)` a few lines above it has
**succeeded**, so the lookup it makes cannot miss and cannot report; it passes
NULL, with a comment saying that.

## Decision 4 — `pyplot`'s silent return is the price of decision 1, and it is paid

**Decided: `com_pyplot.c:61` probes with `vec_get_quiet()`, and the path that
used to return in silence now says what it did.**

```
Error: no vectors given; 'out' was taken as the output file name
```

Without this, decision 1 would be a straight trade of a false warning for a
silent no-op. With it, the only path where `pyplot`'s probe could have hidden a
mistyped vector name reports on its own account, in every case mode, and the
probe can be silent for the reason every other probe is.

The check moves **above** the file-name munging, so it names the word the user
typed rather than the temp name `pyplot temp` would have made of it or the deck
directory `ci_filename` would have prefixed. The old `if (!wl) goto done;` at
the bottom of the function is removed rather than left: after this it cannot be
reached — the only word `com_pyplot()` consumes is the file name — and dead code
that implies otherwise is worse than no code.

This is a **mode independent** change, the only one in this record, and the deck
asserts it under `distinguish` only because that is where its sibling assertion
lives.

## Decision 5 — what does not move, and why each is a reading rather than a count

- **`measure.c:83` and `:100`**, which `0044` criterion 2 names as probes of the
  same shape. They keep the report. `com_meas()` substitutes the value when the
  token names a single-valued vector and otherwise **leaves the token alone**
  for `com_measure2.c` to interpret — it acquires no other meaning for it, and
  `com_measure2.c:400`/`:403` then report `no such vector as %s`. So the near
  miss is the earlier and more informative half of a diagnosed failure, which is
  what `0009`'s table already said. `0044` criterion 2 is right that a rule
  should cover three sites and a patch one; the rule does cover them, and its
  answer for these two is *report*.
- **`PP_mkfnode()`'s `vec_get()`** (`parse.c:531`). `0009`'s table calls its near
  miss *"unlikely — needs a permanent vector literally named `Foo(bar)`"*.
  Measured, and it is **reachable**:

  ```
  let foo(bar) = 1
  print FOO(BAR)   Warning: no vector named 'FOO(BAR)'; 'foo(bar)' differs only in case
                   Error: no such function as FOO, or FOO(BAR) is not available.
  ```

  It is a resolution and the report is right: it is the line that explains why
  the name the user can see in `display` was not found. `0009`'s table row is
  corrected to say so.
- **`device.c:1433`**, decision 2.
- The wildcard error and the `@dev[param]` diagnostics inside `vec_get()`, which
  `vec_get_quiet()` does not touch — `0009` decision 1.

## Decision 6 — mode independence, stated, proved cheaply and measured anyway

The near-miss report is inside
`if (!d && report_case_miss && inp_case_mode() == NG_CASE_DISTINGUISH)`
(`vectors.c:245`), so choosing `vec_get_quiet()` for a probe cannot move `fold` or
`preserve`: the guard is already false there. That is the cheap proof and it is
one `if`.

It is not the whole proof, because the *mechanism* touches code every mode uses,
and more of it than `0009`'s did: `parse.c` is on every expression path in the
frontend and the grammar itself changed. What has to be shown is that no lookup
changed its answer. The Evidence section is that, and the differ is the tool
that can see it.

Decision 4's message is the one thing here that is mode independent, and it is
new output on a path that produced none.

## Decision 7 — what each case of the two decks pins

`tests/regression/casedist/vector-probe-report.cir`, six assertions in three
captures, plus the value of the function the silence let through:

| Case | Silent | Reported in the same capture | What only this case pins |
| --- | --- | --- | --- |
| DEFN | `define f(x) x*2` beside a net `X` | `define g(y) y*Zzz` beside a `ZZZ` **and** `print OUt` beside an `Out` | decision 1's token rule and decision 2's boundary at once: the formal is silent while a body reference in the *same command* and a resolution in the same capture both report. `print f(2)` = 4 is echoed by the wrapper *after* the sourced deck returned, which is also what says the definition survived the silence |
| VS | `hardcopy vpr_ok.svg v(X) vs v(in)` and `hardcopy vpr_bad.svg NNN vs v(in)`, both beside a net `VS` | `NNN` beside an `Nnn`, from the second of those two commands | the per-token rule inside **one parse**: the separator is silent and the vector beside it reports, from the same `ft_getpnames_quotes_probe()` call. The first command really writes its SVG, which is what says the silence did not cost the plot |
| ALTER | — | `alter R1 = RVAL` beside a vector `Rval` | decision 2, in the harness rather than only in prose: the third `check == FALSE` caller is a resolution and keeps its near miss. Nothing else in either deck notices RED 3b |

`tests/regression/casedist/vector-pyplot-probe-report.cir`, three assertions in
two captures:

| Case | Silent | Reported in the same capture | What only this case pins |
| --- | --- | --- | --- |
| NOARG | `pyplot out` beside an `Out` | decision 4's own message | the case the argument against silence is about, pinned by a deck rather than only argued: the probe is quiet **and** the command is not. No python interpreter is started, because `com_pyplot()` returns before `plotit()` |
| ARG | — | `pyplot vpp_none OUt` beside an `Out` | that the silence is the probe's alone and does not reach the vectors being plotted, through `com_pyplot()` rather than through `hardcopy` |

Every silence is asserted beside something in the **same capture** that must be
reported, which is the strong shape `0008` and `0009` ended with, and no
`CAPTURE-READ` fallback is needed: a phrase that is found proves its own capture
was read. `doc/claude/decisions/0006-diagnostic-deck-coverage.md` decision 5.

`define` is on `noredirect[]` (`src/frontend/control.c:62`) beside `if`, `let`,
`stop` and `circbyline`, so `define f(x) x*2 >& f.txt` parses the redirect into
the function body and dies with a `PPerror`, writing no file; the DEFN and VS
cases therefore use the sourced sub-deck of `vector-let-report.cir`, for
`0009` decision 5's reason. `hardcopy` and `pyplot` are not on the list. `plot`
is refused in batch mode, so the `vs` assertion goes through `hardcopy`, which
reaches the same `plotit()`.

## Evidence

Every RED is a `make check` failure against a binary built before the change,
and each deck was run against an **empty** `.out` first so that its evidence is
output rather than an absence.

| Step | Binary | Result |
| --- | --- | --- |
| empty `.out`, `vector-probe-report.cir` | either | FAIL, **22 added lines** |
| empty `.out`, `vector-pyplot-probe-report.cir` | either | FAIL, **9 added lines** |
| RED 1 | `882377ee9` | FAIL, exactly two lines: `PROBE-DEFN-` and `PROBE-VS-` `SILENT` → `REPORTED` |
| RED 2 | the `0045` commit | FAIL, exactly two lines: `PYPLOT-PROBE-SILENT` → `REPORTED` and `PYPLOT-NOARG-REPORTED` → `SILENT` |
| RED 3a | a **per-parse** variant of this fix — `is_probe_name()` replaced by `return probes != NULL` | FAIL, exactly two lines: `PROBE-BODY-` and `PROBE-ARG-` `REPORTED` → `SILENT` |
| **RED 3b** | **`0045` criterion 3 as written** — `check` threaded down, so every `check == FALSE` parse is silent | FAIL, exactly three lines: `PROBE-BODY-`, `PROBE-ARG-` and `PROBE-ALTER-` `REPORTED` → `SILENT` |
| GREEN | this record | PASS, both |

In RED 1 `PROBE-BODY-REPORTED`, `PROBE-RESOLVE-REPORTED`, `PROBE-ARG-REPORTED`,
`PROBE-ALTER-REPORTED` and `f(2) = 4` all pass unchanged, which is what says the
deck asserts the report's *absence* on two tokens and not a side effect of
removing it from the parse. In RED 2 `PYPLOT-ARG-REPORTED` passes unchanged, for
the same reason.

**RED 3b is the one that matters most**, and it is decision 2 made assertable:
the deck rejects the fix `0045` criterion 3 asks for, by name, on the three
tokens that separate a probe from a resolution — one inside a `define`, one
inside a `plot` argument list, and one in the caller the issue misclassified.
Both variants were built and run rather than reasoned about. It is the analogue
of `0009` decision 2's second RED, and like that one it exists because the
classification was read back against the code after the deck had already gone
green.

- `make check`: **274 PASS, 0 FAIL**, up from 272 at `882377ee9` by exactly the
  two decks this work adds. `tests/regression/casedist` 19 → 21;
  `tests/regression/misc` 27, `tests/regression/case` 109, `tests/xspice/case`
  21 and `tests/xspice/casedist` 17 unchanged.
- `python3 doc/claude/scripts/case_differential_sweep.py --jobs 8 --timeout 90`
  on snapshots of the binary before and after: **315 decks, DIFF=71, OK=241,
  SKIP=3** on both, `PARSE-FAIL=0` and `NUM-DIFF=0` on both. Compared per deck
  rather than by totals, the non-`OK` lines differ only by which of the two
  `alter-rebin` decks reports `stdout reordered only` in which mode, which is
  the known flap. The two decks this work adds are a `preserve-UPPER` `DIFF`
  and an `OK` respectively, both identical before and after.
- `python3 doc/claude/scripts/deck_output_differ.py --before X --after Y
  --jobs 8 --mode {,preserve,distinguish}`, every `.cir` under `tests/` — 315 —
  run with both binaries and compared on stdout, stderr and exit status:
  **`fold` 3 of 315, `preserve` 2, `distinguish` 2.** Every `fold` and
  `preserve` move is an `alter-rebin` deck, the documented flap. **In
  `distinguish` the moved decks are this work's two and nothing else**, which
  is the strongest single statement in this record: `parse.c` is on every
  expression path in the frontend, and no other deck in the tree changed a byte
  of either stream.

  The same three runs against **`ec050a9ff` alone**, the first of the two
  mechanisms, move 1 deck in `fold`, 3 in `preserve` and 4 in `distinguish` —
  all `alter-rebin` but the fourth, which is that commit's own deck. That
  measurement was made because the commit message first claimed the pair's
  numbers as its own, which they are not.

  **A `--timeout` shorter than the slowest deck manufactures a MOVED line.**
  The first `distinguish` run of that pair reported `tests/mesa/mesa12.cir` as
  `rc TIMEOUT -> 1`. It takes **~143 s on both binaries** — over the script's
  120 s default — and run serially its two outputs differ only by the
  wall-clock stamp in its own analysis banner. At `--timeout 200` it does not
  appear at all. This belongs beside the flap warning in the script's header:
  a timeout is a second way for the tool to report a difference that is not
  one, and it does not look like a flap, because re-running the deck *serially*
  makes it go away rather than reproducing it.
- The three shapes, before and after, on one deck with nets `X`, `VS`, `Out`,
  `A`, `B` and vectors `CCC`, `Rval`, `Nnn`, `ZZZ`:

  | Shape | Before | After |
  | --- | --- | --- |
  | `define f(x) x*2`, `define h(a,b) a*b` | 3 near misses | none; `h(3,4)` = 12 |
  | `define m(a) a*Ccc` | near miss on `a` and on `Ccc` | near miss on `Ccc` only |
  | `hardcopy f.svg v(X) vs v(in)` | near miss on `vs` | none; SVG written |
  | `hardcopy f.svg NNN vs v(in)` | near miss on `vs` and `NNN` | near miss on `NNN` only |
  | `alter R1 = RVAL` | near miss, then `no such vector RVAL` | unchanged |
  | `print OUt`, `let A = v(OUt)` | near miss | unchanged |
  | `print FOO(BAR)` after `let foo(bar) = 1` | near miss, then `no such function` | unchanged |
  | `pyplot out v(Out)` | near miss, then writes `out.py` | *not re-run — it starts a python interpreter; `0044`'s measurement at `abd0e2f17`, and the reason no deck asserts it* |
  | `pyplot out` | near miss, then **nothing** | `Error: no vectors given; 'out' was taken as the output file name` |
  | `pyplot f OUt` | near miss, then `no such vector OUt` | unchanged |

## Corrections found by reviewing this work

The step `0027`'s session added and every session since has repeated.

**`0045`'s third caller is not a probe**, and the fix its own criterion 3
prescribes would have been a regression. That is decision 2. It was found by
running the deck the issue says was never measured, which cost one command, and
it changed the mechanism from *thread the existing flag* to *thread a new list* —
a difference of one bit in the design and of an error message in the output.

**The candidate distinction between the two issues does not exist.** The prompt
for this work offers, to test rather than assume, that `PP_mksnode()`'s callers
have another name for the probed token while `com_pyplot()`'s does not. Read,
`com_pyplot()` has one — *file name* — and the real difference is that its
miss-path was silent. Decision 1 records the reading and decision 4 fixes the
difference rather than legislating around it.

**The first version of the `pyplot` deck forked a python interpreter and wrote
into the source tree**, and only the differ saw it. Under any mode but
`distinguish`, `out` *is* the vector `Out`, so the probe succeeds, `pyplot`
plots it, and Enhancement-183 writes the `.py` and `.data` next to the **circuit
file** — which in this harness is `tests/regression/casedist/`, not the build
directory. A `fold` run left `pyplot.py`, `pyplot.data`, `vpp_none.py` and
`vpp_none.data` in the source tree and printed two matplotlib tracebacks.
`make check` could not see this, because it runs the deck under `distinguish`
only; the differ could, because it runs every deck under every mode, and it
reported the deck as moved in `fold` on `stderr`. The deck now gates its cases
on `$casemode` and is byte identical between the two binaries in `fold` and
`preserve`. A deck that litters the source tree makes the tools that would have
caught the *next* regression unusable, which is why this is a defect and not
untidiness.

**The first version of the same pair produced a `NUM-DIFF`**, the one sweep
verdict that must never appear for a reason that is not a number. The sweep's
uppercaser rewrites an `echo ... > f` line and leaves the `source f` that reads
it alone, so the source failed, and *that* failure is fatal and truncated the
rawfile the sweep compares. Naming the sub-decks in upper case makes both lines
fixed points of the uppercaser. The captures cannot be spelled the same way,
because `fopen` is not on the reader's fold-exemption list
(`doc/codex/issues/0038`) and a lower-case name is the fixed point of *that* —
so the deck carries one convention for files it sources and another for files
it reads, and says why.

**`tests/regression/model/binning-1.cir` is a flapper**, and is not on the
known-flapper list any prompt in this series carries. It moved in the differ's
`fold` and `preserve` runs; it is byte identical between the two binaries run
serially, four times each, and **differs between a serial run and an eight-way
concurrent run of the one unchanged binary** — by a ` Reference value :
0.00000e+00` line, which is `tests/general/mosamp.cir`'s flap exactly. The
differ's mask replaces that line's text but cannot replace its existence, so it
survives masking as an empty line against a masked one.

**Two degenerate `plot` shapes lose a diagnostic**, and this is a cost accepted
with open eyes rather than an oversight. `hardcopy f.svg vs` and
`hardcopy f.svg v(X) vs` on a deck with a net `VS` used to print the near miss
before `Error: misplaced vs arg` / `Error: missing vs arg`; they now print only
the error. `plotit()`'s own rule for the separator is positional-agnostic, so a
probe list that matched its rule exactly cannot be narrower than this. The loud
error remains in both.

## What this decision does not decide

1. **`doc/codex/issues/0047`**, `plotit()`'s separator hack. Splitting the
   wordlist before the parse would make the probe list unnecessary at that call
   site and would fix the two degenerate shapes above. It is an argument-handling
   change to every plotting command and it is not about case.
2. **`doc/codex/issues/0046`**, the duplicate report before the first analysis.
   Untouched; it shows up in these captures and no assertion depends on a count.
3. **`doc/codex/issues/0039`**, the `.param` ambiguity diagnostic still on
   `stderr` and so unguardable. Cheapest remaining item.
4. **Whether a `define` body should resolve its vectors at definition time at
   all.** It does — `define h(y) y*Www` can never see a `WWW` created
   afterwards — and that is why decision 1 keeps the report there. Changing it
   would be a language change, not a case-mode one, and is not filed.
5. **The build-enforced lint.** It goes last, and after this record nothing
   stands before it: `0009` names `0044` and `0045` and nothing else, and both
   are closed here. `vec_get()` now has one reporting entry point and one silent
   one with three users — `com_let()`, `PP_mksnode()`'s probes and
   `com_pyplot()` — which is the state the lint would freeze.
