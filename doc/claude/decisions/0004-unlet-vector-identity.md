# Decision 0004 — which vector `unlet` removes, `doc/codex/issues/0027`

## Status

Accepted, 2026-08-11, branch `ver_50`. Closes `doc/codex/issues/0027`, the last
item `set_case_mode()`'s experimental-mode warning named. Applies decision 3's
Class C rule and decision 2's definition/resolution split of
`doc/claude/decisions/0001-distinguish.md` to `vec_remove()`
(`src/frontend/vectors.c`) and its three callers. It re-decides nothing those
records settled.

## Context

`vec_remove()` picked the vector to drop with an unconditional `cieq()` over
`plot_cur->pl_dvecs`. Under `casemode=distinguish` two spellings are two
vectors, and `vec_new()` prepends to that list, so the first `cieq()` hit was
the most recently created case variant rather than the name the caller typed.

Measured at `9b6f1d318`, all three silent, all three under `distinguish`:

- two nets `Out` and `OUT`, list order `V1#branch, OUT, Out, in`;
  `unlet Out` cleared `VF_PERMANENT` on **`OUT`**.
- `compose Vaa values 1 2 3` beside an existing `VAA` left only `Vaa`.
- `cross Wbb 0 in` beside an existing `WBB` left only `Wbb`.

Nothing was printed on either stream in any of the three. `print all` then
reported the surviving twin's numbers, which is why this one *is* assertable
by a deck: unlike `0029` and `0030` the failure reaches stdout.

Nothing in this record changes a computed value under `fold` or `preserve`.

---

## Decision 1 — the predicate is `vec_name_eq()`, not `inp_case_exact_ids()`

**Decided: `vec_name_eq(ov->v_name, name)`, whose exact arm is
`inp_case_mode() == NG_CASE_DISTINGUISH`.**

`vec_remove()` is Class C, not Class A. Decision 3 draws that line at whether
the two operands are both names a deck wrote: `ent_eq()` compares two card
tokens and is an identity test, while the frontend vector table compares a
*stored* name against a *query*, and the query has never been a deck token.
`vec_remove()`'s query is the word the user typed at the control language, and
decision 3's Class C paragraph names exactly this hazard — in `fold` mode the
query may still arrive with upper case in it, because the reader's fold only
covers text that came through `inp_readall()`.

That is measured, not assumed: under default `fold`, through `ngspice -p`,
`let MyVec = 2` creates a vector whose `v_name` is `MyVec`, and `unlet OUT`
removes a vector named `out`. `ngSpice_Command()` from `libngspice` has the
same property, and so does the interactive prompt. Giving this site
`inp_case_exact_ids()` would therefore make the **default** mode exact and
break every one of those callers, which is the regression decision 3 refused
at `findvec()` for the same reason.

Rejected: `ng_ideq()`. It is `cieq` under `preserve` and `strcmp` under both
`fold` and `distinguish` — the exact opposite of what this site needs in
`fold`.

## Decision 2 — `vec_name_eq()` stays `static`

Nothing outside `src/frontend/vectors.c` needs it. All three callers reach the
list through `vec_remove()` rather than comparing names themselves, so the
predicate change is confined to the file that owns `pl_dvecs`.

The one site that *would* need it exported is
`is_scale_vec_of_current_plot()` (`src/frontend/postcoms.c:54`), which is
`cieq` and is `doc/codex/issues/0032`. Exporting `vec_name_eq()` is the shape
that issue's fix will take; it is deliberately not done here, because
exporting a predicate for a caller that does not yet exist would leave the
tree with a public function and no second user.

## Decision 3 — the three callers split by decision 2, at the call site

**Decided: `vec_remove()` takes `report_case_miss`; `com_unlet()` passes
`TRUE`, `com_compose()` and `com_cross()` pass `FALSE`.**

Making the compare exact converts a silent wrong removal into a silent no-op.
That is a smaller mistake and exactly as invisible, so decision 2 applies:
`unlet` *resolves* the name it is given, the resolution can now fail, and a
failure with a case variant present is the near miss that record asks to be
reported.

`compose` and `cross` must stay silent. The string each hands to
`vec_remove()` is the name it is about to allocate — `com_compose.c:116`
`cp_unquote`s it and `:637` passes that same allocation to `dvec_alloc()`;
`postcoms.c:958` takes the raw word and `:991` `copy()`s it — so a miss is the
ordinary case and not a failure. Warning there is the "warn on definition"
option decision 2 rejected by name, and it would fire on the legitimate deck:
under `distinguish`, `compose OUT` beside an `Out` is two vectors on purpose.
This is the trap the `0030` session hit with the auto-bridge diagnostic, where
a deliberately case-split hand-written bridge had to be suppressed with
`already_joined()`.

Rejected: **warn unconditionally inside `vec_remove()`**. One line shorter and
wrong for two of the three callers.

Rejected: **a second entry point**, `vec_unlet()` beside `vec_remove()`. It
needs `vec_remove()` to return a bool as well, so it is two API changes rather
than one, and it hides the classification inside a name instead of stating it
at each call site. The flag follows the pattern decision 3 chose for the Class
A edits — make the classification visible in the source — and matches the
file's existing style of bare `bool` arguments
(`find_permanent_vector_by_name()`, `clookup(word, dd, FALSE, TRUE)`).

The report reuses `vec_warn_case_near_miss()`, `findvec()`'s own helper, so one
sentence covers every vector lookup in the frontend. It rebuilds the plot's
lookup table first, because `unlet` can be the first command in a `.control`
block and `vec_remove()` otherwise never touches that table.

Measured, one deck with two nets `Out` and `OUT`, stderr:

```
distinguish
  unlet oUt           Warning: no vector named 'oUt'; 'Out' differs only in case (casemode=distinguish)
  unlet nosuchvector  silent
  compose oUt ...     silent
  cross OuT 0 in      silent
  unlet OUT           silent, and OUT is removed
fold, preserve        nothing on stderr for any of the five
```

`unlet nosuchvector` is silent by design: no case variant exists, so it is
`doc/codex/issues/0028`'s class — a reference that misses with nothing near it
— and not this one.

All four of those lines are asserted by a deck, not only measured by hand.
`756112c46` claimed they could not be — "no `.out` changes and none can" — on
the strength of decision 2's own paragraph saying no deck in this harness can
assert on a warning. Both were wrong, and an adversarial review of that commit
is what found it. `>&` redirects `cp_err` as well as `cp_out`
(`src/frontend/streams.c:156`), so the warning can be captured to a file and
read back with `fopen`/`fread`/`strstr`, exactly as
`tests/regression/pipe/shell-keyword-case.cmd:79` already does for output it
has to take off disk, and reduced to one upper-case token that the filter
keeps. `tests/regression/casedist/vector-unlet-report.cir` does that four
times: reported for the `unlet` miss, silent for the miss with no twin, silent
for `compose`, silent for `cross`. Its RED, measured with the `if (!ov)` block
reverted and the binary rebuilt, is one line — `-UNLET-MISS-REPORTED`,
`+UNLET-MISS-SILENT` — with the three silence assertions passing either way,
so the deck fails on precisely the behaviour this decision adds.

The silence assertions are the valuable half. A future change that made
`compose` report would be a regression against decision 2 and is otherwise
invisible: it writes to a stream no deck compares, on a deck that still
produces every correct number.

## Decision 4 — exactness in `compose` and `cross` is a fix, not a behaviour change

The `vec_remove()` call in each is the guard for one invariant: the plot must
not end up holding two permanent vectors under **the name being defined**,
because that puts two identical names on one folded lookup key and makes
`find_permanent_vector_by_name()`'s pick arbitrary. `vec_name_eq()` enforces
exactly that invariant, on exactly the string about to be allocated. `cieq()`
enforced something strictly larger, and the surplus was another vector's
destruction.

The residue is the second limb and it is not new. After the fix,
`compose Vaa` beside a `VAA` leaves both permanent on the folded key `vaa`.
`let VAA = 2` beside a `Vaa` already produces that state under `distinguish` —
measured, both remain in `display` and `print Vaa` / `print VAA` each report
their own value — and the lookup filters the chain with this same predicate.
So the fix does not invent an unreachable state; it stops `compose` and
`cross` being the only two commands that silently delete a vector the user
never named.

Blast radius: none outside `distinguish`. The mode enum has three members
(`src/include/ngspice/fteext.h`), `vec_name_eq()` returns `cieq` for two of
them, and `cieq` is symmetric, so the swapped argument order is the same
comparison. Under `distinguish` the new predicate is strictly *narrower* than
`cieq` — `strcmp` equality implies it, and `vec_wrapped_name_eq()` permits a
difference only at index 0 of a `v()`/`i()` wrapper — so `vec_remove()` can
only ever match fewer vectors than before, never more, in any mode.

## Decision 5 — `cp_remkword()` keeps the caller's spelling

**Decided: leave `cp_remkword(CT_VECTOR, name)` as it is, and file the
mismatch as `doc/codex/issues/0033`.**

`0027`'s acceptance criterion 2 asks for an audit of the line, and the audit's
answer is that the two halves genuinely disagree: vectors are registered under
their own spelling (`vectors.c:597`, `com_let.c:239`, `com_compose.c:658`,
`postcoms.c:1018`), `clookup()` (`src/frontend/parser/complete.c`) walks the
trie byte for byte with no case folding in any mode, and `vec_remove()` is
handed the spelling the user typed. So whenever the two differ the removal
finds nothing and leaves a stale completion entry — in `fold` and `preserve`
too, independent of this issue. `com_remzerovec()` (`postcoms.c:94`) already
passes `ov->v_name` and is the in-tree precedent for the one-word fix.

It is not taken here, for three reasons and not one:

1. **It is unfalsifiable.** `keywords[CT_VECTOR]` is read only by `cp_ccom()`,
   and `cp_ccom()` has no callers anywhere in the tree — upstream deleted them
   from `src/frontend/parser/lexical.c` in `4feb0c3cc`. The trie is write-only
   dead data, so no `.out`, exit status, stdout or stderr can distinguish the
   two spellings of that line. A change no test can see cannot be landed
   RED-first, and `AGENTS.md` makes RED-first non-negotiable for behavioural
   change.
2. **It is not free.** Every case-mismatched remove that returns `NULL` today
   would newly reach `cdelete()`, hand-rolled trie surgery that recurses into
   parents and frees nodes. That path is currently reached only by exact
   matches. Zero observable benefit does not justify newly exercising it.
3. **It would record something false.** "Add and remove now use the same
   string" is true; "the keyword is now always removed" is not, because
   `plot_setcur()` deliberately does not swap the tree (`vectors.c:1347`), a
   rawfile `load` leaves `keywords[CT_VECTOR]` NULL (`vectors.c:614`), and
   simulation vectors are never added at all. A one-word change here would be
   read as "`CT_VECTOR` is now consistent" when three structural mismatches
   remain.

The honest disposition is an issue that carries the audit, which is
`doc/codex/issues/0033`.

## Decision 6 — `distinguish` stays experimental

**Decided: keep the word, and retarget the clause rather than emptying it.**

`0027` was the last thing the clause named, so closing it could have emptied
the sentence. It does not, because auditing `vec_remove()`'s neighbourhood
turned up `doc/codex/issues/0032`: the current plot's scale vector is still
identified with `cieq()` in three places, so under `distinguish` a vector
whose name differs from the scale's only in case is taken *for* the scale by
`print`, dropped from `ally`, and refused by `unlet`. Two of those three are
silent wrong output, which is the same shape the clause has always named, so
the clause keeps its shape and changes its subject.

The word would stay even if the clause emptied, and this is the part that
needs writing down rather than assuming:

- `doc/codex/issues/0028` is open on both sides of the mixed-signal boundary.
- A name ngspice constructs with upper case of its own — `q1#collCX`
  (`src/spicelib/devices/bjt/bjtsetup.c:433`) — must be typed as the simulator
  spells it. Documented in decision 3, not a silent wrong answer, but a real
  limitation.
- Decision 5 of `0001-distinguish.md`, the migration hazard, is **inherent to
  the mode**: a deck that spells one net two ways becomes two nets, silently,
  because both spellings are *definitions* and decision 2 deliberately does
  not warn on a definition. That cannot be fixed without withdrawing the
  feature, only documented, and it is the strongest single reason for the
  word.
- The differential sweep never runs `distinguish`, so the mode's collateral
  coverage is the nine decks in `tests/regression/casedist/` and the four in
  `tests/xspice/casedist/`, and nothing else.

## What this decision does not decide

1. **`doc/codex/issues/0032`**, the vector name comparators outside
   `findvec()` — `is_scale_vec_of_current_plot()`, `findvec_ally()`,
   `vec_eq()` and its six callers, `rawfile.c:618`, `diff.c:95`. All Class C
   sites by decision 3's rule, none fixed here. It is what the experimental
   warning now names.

   **Closed since**, on `ver_50`, by
   `doc/claude/decisions/0005-scale-vector-identity.md`. Decision 2 above
   predicted the shape correctly: `vec_name_eq()` was exported, and all six
   sites call it. What decision 2 did *not* settle, and `0005` decision 1
   does, is that a `vec_is_plot_scale()` helper is not the alternative it
   looked like — the three scale rows have three different signatures.

   Decision 6's expectation is the part that did not survive. It kept the word
   `experimental` **and** retargeted the clause to `0032`, arguing that
   auditing `vec_remove()`'s neighbourhood had found a replacement subject.
   Closing `0032` emptied the clause again with no defect left to name, and
   `0005` decision 5 declines to retarget it a third time: the obvious
   candidate, `doc/codex/issues/0034`, is a false positive rather than a
   silence, and this clause has always promised a silence. The clause now
   names decision 5's migration hazard instead, which is inherent to the mode
   and cannot be closed. The word stays, on decision 6's own reasoning.
2. **`doc/codex/issues/0033`**, `cp_remkword()`'s spelling, per decision 5.
3. **`doc/codex/issues/0034`**, `findvec()`'s near-miss warning firing on a
   *definition* when it is reached through `com_let()`'s left-hand side. That
   is decision 2's rejected option arriving by accident, and it is noise on
   the legitimate deck rather than a wrong number. It predates this work.
4. **`doc/codex/issues/0035`**, three latent defects found in the same
   functions and unrelated to case: `vec_basename()`'s read past the
   terminator, `findvec_ally()`'s unguarded `pl_scale`, and
   `com_remzerovec()`'s missing scale guard.
5. **Whether `unlet` should refuse or report on the `cp_unquote` asymmetry** —
   `com_compose` unquotes its result name and `com_cross` does not, and
   `vec_remove()` has none of `find_permanent_vector_by_name()`'s
   quote-stripping retry, so a quoted name never matches in any mode.
   Pre-existing, orthogonal, and not filed: it needs a decision about what
   quoting means in a vector name before it needs a fix.
