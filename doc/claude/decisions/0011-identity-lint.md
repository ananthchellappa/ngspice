# Decision 0011 — the build-enforced identity lint

## Status

Accepted, 2026-08-11, branch `ver_50`. Closes item 7 of `0005`, item 7 of
`0006`, item 6 of `0007`, item 6 of `0009` and item 5 of `0010` — the same
deferral carried by every record in the series since `0005`. Files
`doc/codex/issues/0048`.

## Context

Ten records precede this one, and five of them — `0005`, `0006`, `0007`,
`0009`, `0010` — close by deferring the same thing: the lint that would stop
the next `strcmp` on a name from arriving. They do not word it the same way.
`0005` item 7 says the lint goes *"last, after there is nothing left for it to
flag"*; `0009` item 6 says it waits for `doc/codex/issues/0044` and `0045` and
*"names nothing else"*, which `0010` satisfied. Those two are not the same
instruction, and deciding between them is this record's first job.

The three helpers the lint would push callers toward, measured at `6e2dca123`:

- `ng_ideq()`, `src/frontend/inpcom.c:1074` — deck identifiers.
- `vec_name_eq()`, `src/frontend/vectors.c:413` — vector names.
- `Evt_Node_Name_Eq()`, `src/xspice/evt/evtplot.c:63` — event nodes.

And the scale of the problem, measured with a scanner rather than a grep,
because `eq()` and `eqc()` are macros over `strcmp()` and `cieq()` that a grep
for the underlying name never sees:

```
1541 calls to a string comparator in src/
1267 of them have a string literal operand
 274 of them do not
```

---

## Decision 1 — it ships now, as a ratchet, with `0033` and `0035` open

**Decided: ship. `0005`'s "after there is nothing left for it to flag" is not
achievable in principle, and that is a measured statement about these two
issues, not an impatient one.**

`doc/codex/issues/0033` is not a comparison. It is

```c
    cp_remkword(CT_VECTOR, name);            /* src/frontend/vectors.c:669 */
```

— a call that passes the caller's spelling where the vector's own spelling
belongs. There are not two operands to compare, so no lint of this shape will
ever report it, and waiting for it is waiting for something the lint has no
opinion about. Two of `0035`'s three items are the same: `(b)` is a missing
NULL check and `(c)` is a missing guard and a missing NULL check. Neither is a
comparison either.

That leaves exactly one row of the two open issues that the lint can see,
`0035` item (a):

```c
        if (cieq(v->v_plot->pl_typename, v->v_name))     /* vectors.c:1310 */
```

(the issue says `:1266`; it has moved.) One site, in the frozen set, named in
`tests/lint/identity.baseline`'s header with its issue number. Holding a lint
that guards 274 sites for one of them is the wrong trade.

**Rejected: advisory-only until they close.** There is nowhere for advice to
go. `tests/bin/check.sh:20`'s `egrep -v` filter drops every line containing
`Warning` from both sides of the comparison, so a warning printed during
`make check` is invisible by construction; and `0006` is the record of what
happened the last time three diagnostics shipped with nothing asserting on
them. An advisory lint would be a file that nobody runs and a report that
nobody sees.

**Rejected: fix `0033` and `0035` first.** `0033`'s own acceptance criterion 2
says it cannot be landed RED-first without first building a reader for
`keywords[CT_VECTOR]`, because `cp_ccom()` has had no callers since
`4feb0c3cc` — a larger change than the line it would test. `0035` (a)'s
resolution says it is *"not a typo with an obvious repair but an open question
about what `vec_basename()` should return for a dotted name"*. Both are
sessions of their own and neither is about case.

**The cost, stated plainly.** A baseline entry is a place for a defect to live
quietly. Two things pay for it:

1. **An entry can only leave by its comparison leaving the tree.** The lint
   fails on a baseline line it can no longer find, in the same run and with
   the same exit status as a new comparison. So a rewrite of a baselined site
   is forced through the baseline file and through review, and `make check`
   prints the exact lines to delete. There is no expiry, no `--update` flag
   and no regeneration target — a lint that can be silenced by regenerating
   its baseline is a lint that will be.
2. **The baseline names what is known to be in it**, in its own header:
   `0035` (a) and `doc/codex/issues/0048`. It also names its permanent
   residents — the comparators' own definitions and the bodies of the six
   approved helpers — so it is not read as 274 units of debt.

## Decision 2 — argument shape, whole tree, and no classification at all

**Decided: report every call to a string comparator, anywhere in `src/`, whose
operands are both runtime expressions. Do not scope by file. Do not attempt to
say which of them are identity comparisons.**

The four candidates were measured before choosing, and three of them lost to
the data.

**By file — rejected, and the data reverses the guess.** The suggestion was
that `src/frontend/vectors.c` and `src/spicelib/parser/` are suspect and the
rest of the tree is not. Classifying all 274 sites says the opposite:

| Scope | sites | identity | not identity | wrong if flagged |
| --- | --- | --- | --- | --- |
| whole tree | 266 | 95 | 171 | 64% |
| `src/frontend/` | 169 | 69 | 100 | 59% |
| `src/spicelib/parser/` | 15 | 1 | 14 | 93% |
| `src/xspice/evt/` | 10 | 2 | 8 | 80% |
| the eight name-owning frontend files | 33 | 9 | 24 | 73% |

(266 rather than 274 because eight lines carry two comparisons each. The
counts are after the adversarial second read described under Evidence; the
first read gives 113 identity for the whole tree, 57%, and reorders no row.)
The two directories named as suspect are the two *worst* —
`src/spicelib/parser/` is almost entirely device parameter keywords, which
`0001` decision 5 puts explicitly outside this feature's scope. No file
scoping improves anything, so none is taken.

**By argument shape beyond "no literal" — rejected, measured.** Narrowing the
comparator set to the exact whole-string forms (`strcmp`, `strcasecmp`,
`cieq`, `eq`, `eqc`) moved 64% to 65%, i.e. the wrong way. Dropping sites with
an indexed-table operand (`plist[i].keyword`, `cp_coms[i].co_comname`) moved
it to 61% and cost twelve real identity comparisons to remove thirty-seven
others. Neither buys enough to pay for the coverage.

**By allowlist — this is what shipped**, as the baseline, per decision 1.

**By lower-case string literal, which is `0001` decision 6 item 5's own
wording — rejected.** It is inverted: 1267 of the tree's 1541 comparison sites
have a literal operand and almost all of them are keyword tests. A literal
operand is the one signal that reliably means *not* an identity comparison,
so the lint uses it as the exemption rather than the trigger.

**What the lint therefore claims.** Not that a reported site is a bug. That it
is **unclassified** — and about two thirds of the time the answer is a comment
rather than a helper call (100 keyword and 71 neither, against 95 identity).
Both answers are the point: `0001` decision 3 is a classification of
twenty-eight sites made once by hand, and this is what makes the
classification mandatory for the twenty-ninth instead of optional.

**The false-positive question a contributor actually asks** is not "what
fraction of flags are wrong" but "how often will this stop me". Measured by
running the scanner over historical trees:

| interval | commits | new sites | rate |
| --- | --- | --- | --- |
| HEAD~2000 → HEAD~1000 (2022-04 → 2024-02) | 1000 | 32 | 1 per 31 commits |
| HEAD~1000 → HEAD~500 (2024-02 → 2026-02) | 500 | 19 | 1 per 26 commits |
| HEAD~500 → HEAD~250 (2026-02 → 2026-04) | 250 | 0 | — |

Removals over the same two quiet intervals are 7 and 6, about one per 100
commits, which is the cost of decision 1's "an entry can only leave by its
comparison leaving". A contributor meets this lint a couple of times a year.

## Decision 3 — `tests/lint/`, last in `SUBDIRS`, wired into `make check`

**Decided: a new `tests/lint/` directory whose `TESTS_ENVIRONMENT` runs
`tests/bin/identity_lint.sh` instead of `tests/bin/check.sh`, added
unconditionally and *last* to `tests/Makefile.am`'s `SUBDIRS`. A failing lint
fails `make check`.**

The generated `Makefile` settles where it can go. `$(am__recursive_targets)`
recurses with

```
	  ($(am__cd) $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
	  || eval $$failcom; \
```

and `failcom` is `exit 1` unless make is in keep-going mode. So a failing
directory costs the suite every directory *after* it. From last position a
lint failure costs nothing that has not already run, and the contributor gets
the full deck result and the lint report from one `make check`.

**The cost of last position**, since it is not free: the model QA suites run
first, so a contributor who wants only the lint's answer waits minutes for it
inside `make check`. They should not — `make -C build-ver_50/tests/lint check`
runs both specs in about 1.6 s. That is the same relationship
every other directory here has with the suite, and it is why last position is
the right trade: the fast path exists and is one command.

**Rejected: a separate `make lint` target that `make check` does not depend
on.** The deferral asks for a *build-enforced* lint, and this tree's `make
check` is the thing people run. A target nobody invokes is the advisory lint
of decision 1 under another name.

**Rejected: a `configure`-time check.** It would run once per configure, not
once per change, and it would put a source-tree scan in the path of every
build of every user who is not editing the tree.

It is unconditional because it links nothing: no `if XSPICE_WANTED`, no
`--enable` flag, and `tests/bin` is already whole-directory `EXTRA_DIST`, so
the scanner and driver are distributed with no further edit.

**Written in `awk`, not in `python3` or `perl`.** `configure.ac` never calls
`AC_PROG_AWK`, but it does not have to: `AM_INIT_AUTOMAKE` requires it, the
generated `configure` searches `gawk mawk nawk awk`, and every generated
`Makefile` in this tree already carries `AWK = @AWK@` — `tests/lint/Makefile`
included, at line 158, before this work touched anything. So the lint adds no
build dependency and cannot be skipped for want of an interpreter. The model
QA suites need `perl` and are gated behind a configure flag for it; a lint
that is gated is a lint that is off. The scanner is 238 lines, the driver
174, and scanning all 2141 files under `src/` takes about 1.8 s.

## Decision 4 — the escape hatch is a comment that names the reason

**Decided: `/* case-lint: <reason> */`, honoured on the line the call is on
and on the line directly above it, and nothing else. Documented in `AGENTS.md`
with the two questions spelled out, and in `CLAUDE.md` in one sentence.**

The reason is required rather than a bare suppression token because the lint
cannot check the classification and a reviewer can. `case-lint: keyword -
both operands are command names` is a claim someone can be wrong about in a
diff; `NOLINT` is not.

The one-line scope is deliberate. A marker that covered a function would
silence the next comparison added to it, which is the failure this whole
series is about. A call wrapped across lines may carry the marker on any line
it spans, so a long argument list can put it where it reads best.

Three ways out, and the driver's failure message prints all three: call a
helper, annotate with a reason, or add a baseline entry naming the issue that
will fix it. The message is 26 lines and is the documentation a contributor
actually reads, because it is the one that appears at the moment they need it.

## Decision 5 — the baseline is keyed on the call text, not on the line number

**Decided: `<path><TAB><the call, whitespace collapsed>`, taken from the
masked source, one line per occurrence.**

A line number in the key would make every edit *above* a site look like a new
comparison. That is the failure mode that teaches a contributor to regenerate
the baseline rather than read it, and decision 1 depends on nobody ever doing
that. The line number is still printed in the report, where it helps and
cannot rot.

The key is taken from the text with comments and string-literal interiors
blanked, so a comment inside an argument list cannot change a key —
`strcasecmp(a /* the stored spelling */, b)` keys as `strcasecmp(a , b)`, with
the two blanks collapsed to one. `tests/lint/selftest.baseline` says so, since
it is the one place the shape is visible.

## Decision 6 — mode independence, proved by the diff and measured anyway

**Decided: it is mode independent, and the cheap proof is that the change
touches no file under `src/`.**

`git show --stat` for the two commits lists `configure.ac`,
`tests/Makefile.am`, `tests/bin/identity_lint.{sh,awk}`, `tests/lint/*`,
`AGENTS.md`, `CLAUDE.md` and `doc/`. Nothing is compiled, nothing is linked,
no `casemode` predicate is read, and `tests/lint/`'s `TESTS_ENVIRONMENT` is
the only one under `tests/` that does not name `$(top_builddir)/src/ngspice`.

There is a sharper demonstration than the diff, and it was run: `configure` in
an empty directory outside the source tree, then `make -C tests/lint check`
with **no `make` at all** beforehand. Both specs pass. Nothing under `src/` had
been compiled, so there was no binary for a case mode to be given to.

That is the argument. It was measured anyway, because `0010`'s review found a
commit message asserting a measurement made against a different binary:

- `make check` 274 PASS / 0 FAIL before, **276 PASS / 0 FAIL** after, compared
  per test rather than by the total: `identity.lint` and `selftest.lint` gained
  and nothing lost. Re-run at the commit itself after the last edits to the
  lint's own files, not only at the state that first passed.
- `case_differential_sweep.py --jobs 8 --timeout 90`: see Evidence.
- `deck_output_differ.py --mode {,preserve,distinguish}`: see Evidence.

## Evidence

### The scanner agrees with an independent implementation

The site set was produced twice, once by the shipped `awk` scanner and once by
an unrelated `python3` one written first for the measurement, with different
masking and different argument splitting. The two site sets are byte identical
at 274 records. That is the only check available for a scanner whose output is
its own baseline.

### RED 1 — the detector, against the real tree

`doc/codex/issues/0027` reintroduced at `src/frontend/vectors.c:654`, where
`vec_name_eq()` now stands in `vec_remove()`:

```
-        if (vec_name_eq(ov->v_name, name) && (ov->v_flags & VF_PERMANENT))
+        if (cieq(ov->v_name, name) && (ov->v_flags & VF_PERMANENT))
```

```
identity lint: FAILED

Unclassified comparisons of two runtime strings, not in .../identity.baseline:

  src/frontend/vectors.c:654  cieq(ov->v_name, name)
```
exit 1. Reverted:
```
identity lint: 274 comparisons, baseline matches
```
exit 0.

### RED 2 — the escape hatch, both placements, against the real tree

The same reintroduced comparison, annotated on its own line:

```c
        if (cieq(ov->v_name, name) && (ov->v_flags & VF_PERMANENT))   /* case-lint: RED for the escape hatch */
```
→ `identity lint: 274 comparisons, baseline matches`, exit 0.

And with the marker on the line above it:

```c
        /* case-lint: RED, marker on the line above */
        if (cieq(ov->v_name, name) && (ov->v_flags & VF_PERMANENT))
```
→ `identity lint: 274 comparisons, baseline matches`, exit 0.

### RED 3 — the other direction, which is what decision 1 rests on

Decision 1's whole answer to "how does an entry leave the baseline" is that
the lint fails on an entry it can no longer find, so that was run too.
`vec_wrapped_name_eq()`'s `strncmp` replaced by a helper call:

```
-    return strncmp(v_name + 2, typed + 2, n - 2) == 0;
+    return ng_ideq(v_name + 2, typed + 2);
```

```
identity lint: FAILED

In .../tests/lint/identity.baseline but no longer in the tree:

  src/frontend/vectors.c	strncmp(v_name + 2, typed + 2, n - 2)

Delete these lines.  The baseline is a description of the tree, and
an entry that outlives its comparison is how a stale exemption hides
the next one.
```
exit 1. Reverted, exit 0. (The replacement is not a change anyone should
make — `ng_ideq()` is the wrong predicate there — it is the cheapest edit that
removes a baselined comparison.)

### A scanner that dies does not pass

`tests/bin/identity_lint.awk` replaced with `BEGIN { exit 3 }`: the driver
exits 123 rather than reporting an empty set as a match. `set -e` and `xargs`
carry that, and without it every other assertion here could pass vacuously.

### Both REDs, made permanent

`identity.lint` freezes the tree, so on its own it can only ever say "nothing
changed" — a scanner that reported nothing at all would pass it, and so would
one whose literal exemption or escape hatch had stopped working, because the
frozen set was built by the same code. `selftest.lint` points the same driver
at two fixtures instead:

- `tests/lint/selftest/flagged.c` — eleven comparisons, all in
  `selftest.baseline`, covering the plain shape, the `eq`/`eqc` macros, a call
  wrapped across lines, a comment inside the argument list, a *different*
  call's literal on the same line, the length-taking and substring forms, and
  a suppression marker belonging to a different tool.
- `tests/lint/selftest/silent.c` — fifteen comparator-shaped constructs, none
  in the baseline: comparisons against a literal (including a literal
  containing a parenthesis and one containing an escaped quote), the three
  annotated forms, a comparator named inside a comment, one named inside a
  string, `my_strcmp`, `strcmp_wrapper` and `xeq`, which are not comparators,
  and the one accepted false negative — a literal buried in a nested call,
  which exempts an outer comparison of two runtime operands. No call in `src/`
  has that shape; every exempted call there has a literal as a top-level
  operand.

Each of `silent.c`'s exemptions was checked for vacuity by removing the thing
that exempts it and confirming the site is then reported: literal → variable
for each of the five keyword shapes, marker deleted for each of the three
annotated ones, uncommented and unquoted for the two textual ones, and the
nested literal replaced by a variable. Every one of the twelve mutations moved
the count, so none of them is passing because the scanner never looked.

So the driver's two failure directions are the two REDs: a detector that stops
seeing a raw comparison fails with *no longer in the tree*, and an exemption
that stops exempting fails with *unclassified comparison*.

### The sweep, compared per deck rather than by its total

`case_differential_sweep.py --jobs 8 --timeout 90` was run twice, once against
the snapshot of the binary taken before any of this and once against the
rebuilt one:

```
before   315 decks: DIFF=70, OK=242, SKIP=3     PARSE-FAIL=0  NUM-DIFF=0
after    315 decks: DIFF=69, OK=243, SKIP=3     PARSE-FAIL=0  NUM-DIFF=0
```

Compared per deck, exactly one differs, `tests/regression/case/alter-rebin-param-case-lower.cir`,
and it is one of the four `alter-rebin` decks the series has recorded as
flapping. Re-run three times against a *single* binary, that family gives
DIFF=1, DIFF=4 and DIFF=3, and the deck in question flips inside those three
runs. So no deck moves; the totals differ by one because the totals include a
deck that cannot be measured to ±1 in the first place. The `315` and the two
zeroes are the numbers this change could have broken and did not.

### The differ, all three modes

`deck_output_differ.py --before <the snapshot taken before this work>
--after build-ver_50/src/ngspice --jobs 8 --timeout 200`:

```
decks: 315  mode: stock          moved=2   alter-rebin-param-case{,-lower}.cir
decks: 315  mode: preserve       moved=2   alter-rebin-case.cir, alter-rebin-param-case.cir
decks: 315  mode: distinguish    moved=1   alter-rebin-param-case-lower.cir
```

Five MOVED lines across three modes, and all five name one of the four
`alter-rebin` decks — which is what the no-change baseline for this differ has
always been. The per-mode counts sit one below the recorded 3 / 2 / 2 in two
of the three modes, and that is the same ±1 the family shows against a single
binary. No deck outside that family moved in any mode, which for a change that
touches no compiled file is the expected answer and the reason it was worth
the two hours to have.

`--timeout 200` rather than the default: `tests/mesa/mesa12.cir` takes ~143 s
on either binary and reports as `rc TIMEOUT -> 1` at 120, which is a MOVED
line that is not a difference.

### The measurement behind decision 2

The 274 sites were classified by ten independent readers of the source, each
reading the function around every site rather than the line. Every verdict of
"this is an identity comparison" was then handed to a second reader told to
refute it and to default to refuting. The second pass overturned 18 of 113,
necessarily all in one direction, giving 95 identity / 100 keyword / 71
neither.

**Which way the error runs, stated because it does not run the way that would
flatter this decision.** Only the identity verdicts were challenged, and the
challenger was told to default to refuting, so both biases push the identity
count down: 95 is the low end and the 64% in decision 2's table is the high
end. The unchallenged first pass gives 113 identity and 57%. The decision does
not turn on which is right — a classifying lint would need to be wrong about
something like one site in ten, and both readings are five to six times that.

Eight of the eighteen sites the two passes disagreed about are in
`src/frontend/variable.c`, more than in any other file, and that disagreement
is `doc/codex/issues/0048`: a control variable is dispatched by `cp_usrset()`
with `eqc()` and stored and read back by `cp_vset()`/`cp_getvar()` with
`eq()`, so `set NUMDGT=12` is honoured and `set WIDTH=200` is silently
ignored, in the same session, in every mode. Two careful readers could not
agree whether those nineteen sites compare a name or a keyword because the
code does not distinguish them.

## What this decision does not decide

1. **`doc/codex/issues/0048`**, filed by this work and not fixed by it. The
   lint's census found it; the lint does not fix what it finds.
2. **`doc/codex/issues/0033` and `0035`.** Unchanged. `0035` (a) is in the
   baseline with its number; the other three items are not comparisons and are
   invisible to this lint in principle, which is decision 1.
3. **Whether the baseline should carry a class per line.** It does not. The
   classification behind decision 2 exists and is quoted in aggregate, but
   putting 274 individual verdicts in a file that `make check` enforces would
   assert 274 things this record cannot stand behind one at a time. The
   baseline asserts one thing: that this was the set.
4. **Annotating the permanent residents in `src/`.** The comparators' own
   definitions and the six approved helpers' bodies would read better with a
   `case-lint:` comment than with a baseline line, and doing it would cost the
   "touches no file under `src/`" property of decision 6. Left for a session
   that is changing those files for another reason.
5. **A lint for the other half of the rule.** `0001` decision 3's Class B is
   sites that ask `inp_case_folding()` and are correct; nothing stops a new
   site from asking that question while meaning the identity one, and this
   lint sees only the comparison, not the predicate above it.
6. **`doc/codex/issues/0047`, `0046`, `0043`, `0042`, `0041`, `0039`, `0038`,
   `0031`, `0021`**, and `com_let()`'s `plainlet` leading space per `0009`
   deferral 4. Enumerated in `0048`'s session prompt as out of scope and
   untouched here.
7. **The spec's own acceptance list.** `doc/claude/specs/case-sensitive-identifiers.md`
   still marks subcircuit formal pins and the rawfile untested under
   `distinguish`, and its last bullet still reads as though
   `doc/codex/issues/0028` were open.
