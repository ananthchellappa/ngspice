# Receipt 01 — the wildcard rename, fixed in the narrow scope

Item 1 of `doc/claude/batches/2026-08-15-xschem-open-items/PLAN.md`. Branch
`ver_50`, HEAD at start `16a0b3156`. `doc/codex/issues/0064`, option (i): the
`ft_evaluate()` rename only. `src/frontend/postcoms.c` was not opened.

## The RED failure, verbatim

`tests/regression/misc/wildcard-rename.cir` and its `.out` were written first
and run against the unmodified binary. From
`make -C build-ver_50/tests/regression/misc check TESTS=wildcard-rename.cir`:

```
make[1]: Entering directory '/home/qflow/dev/ngspice_test/build-ver_50/tests/regression/misc'
--- wildcard-rename.out_tmp	2026-08-15 10:58:35.905559007 -0700
+++ wildcard-rename.test_tmp	2026-08-15 10:58:35.901558991 -0700
@@ -5,15 +5,15 @@
 
 
 WCR-ONE-PRINT-ALL
-in = 1.000000e+00
+all = 1.000000e+00
 WCR-ONE-PRINT-ALLV
-in = 1.000000e+00
+allv = 1.000000e+00
 ASCII raw file "wcr_one.raw"
 WCR-ONE-WRITE-HOLDS-NET
-WCR-ONE-WRITE-NO-WILDCARD
+WCR-ONE-WRITE-HOLDS-WILDCARD
 ASCII raw file "wcr_allv.raw"
 WCR-ALLV-WRITE-HOLDS-NET
-WCR-ALLV-WRITE-NO-WILDCARD
+WCR-ALLV-WRITE-HOLDS-WILDCARD
 WCR-TWO-PRINT-ALL
 in = 1.000000e+00
 wcr_b = 2.000000e+00
FAIL: wildcard-rename.cir
===========================================================
1 of 1 test failed
Please report to http://ngspice.sourceforge.net/bugrep.html
===========================================================
```

It failed for the expected reason and for no other: four lines, all four the
rename. The two controls in the same deck — the same wildcard on a two-vector
plot (`WCR-TWO-PRINT-ALL`) and an expression (`WCR-EXPR-PRINT`,
`v(in)+wcr_b = 3.000000e+00`) — matched before the change and match after it.
That is the deck's evidence that the rename was withheld and not removed.

## The production change

`ft_evaluate()` (`src/frontend/evaluate.c:80`) gains one conjunct,
`!vec_is_all_wildcard(node->pn_name)`, so it stops copying a parse node's text
over the name of the single unchained vector that text produced when the text
is a wildcard; `vec_is_all_wildcard()` is a three-line publication of the
existing `get_all_type()` added beside it in `src/frontend/vectors.c` and
declared in `src/include/ngspice/fteext.h`, which is the whole of the point —
the exempt set is by construction the same set `findvec()` intercepts before
any name lookup and therefore the same set whose results are chained through
`v_link2` at two or more matches, so `all`, `allv`, `alli`, `ally` and `alle`
are exempt together and the match is case-insensitive because `get_all_type()`
is. 31 insertions and 1 deletion across three files, most of them comment; the
only line that changes behaviour is the conjunct. No `strcmp`-family comparison
was added — `get_all_type()` tests characters, not strings — so
`tests/lint/identity.baseline` is untouched (`identity lint: 264 comparisons,
baseline matches`).

**`alle` is in the set, and the reason is in the callers, not in the name.**
`get_all_type()` returns `ALL_TYPE_ALLE` for it (`src/frontend/vectors.c:143`),
`findvec()` dispatches it to `findvec_alle()` before any name lookup
(`:184-186`, under `XSPICE`), and `findvec_alle()` (`:299-343`) builds its
result list by writing `v_link2` exactly as `FINDVEC_ALL_GEN` does — so one
event node reaches `ft_evaluate()` with `v_link2 == NULL` and had precisely the
exposure `all` had. Leaving it out would have made one wildcard of five behave
differently for no reason a reader could find in the code.

## The two `make check` counts

| what | result |
|---|---|
| `make -C build-ver_50/tests/regression/misc check` | **All 36 tests passed** (35 before this item; `wildcard-rename.cir` is the 36th) |
| `make check` from `build-ver_50` | **321 PASS, 0 FAIL**, `rc=0` |

`LEDGER.md` records the batch's starting point as 320 PASS, 0 FAIL at
`eb0b96c8c` with nothing under `src/` or `tests/` changed since. 321 is that
number plus this item's one new deck: no test changed state.

Full-suite detail, from `make check`'s own log: 321 `PASS:` lines and no
`FAIL:`, `XFAIL:`, `XPASS:` or `SKIP:`. The single line matching `^ERROR:` is
`ERROR: (internal)  tried to destroy non-existent graph`, emitted inside the
output of `vector-probe-report.cir`, which passes; it is pre-existing and
unrelated. `tests/lint` passes (`identity lint: 264 comparisons, baseline
matches`; `selftest.lint`, 11 comparisons). The XSPICE digital suite, which the
previous batch's receipt recorded as segfaulting on stale `*.cm` models, is
green here — the `make -j8` this item ran rebuilt them, as that receipt
predicted.

## What was measured against the fixed binary

`build-ver_50/src/ngspice`, rebuilt this session, `set filetype=ascii`, default
`casemode=fold`, scratch directory outside the repo. Same netlist family as the
predecessor batch's table.

| deck | before | after |
|---|---|---|
| `.op`, `.save v(in)`, bare `write` | `No. Variables: 2`, `v(in)` `v(all)` | `No. Variables: 1`, `v(in)` |
| `.op`, `.save v(in)`, `print all` | `all = 1.000000e+00` | `in = 1.000000e+00` |
| `.op`, `.save i(v1)`, `write f alli` | `No. Variables: 2`, `i(v1)` `i(alli)` | `No. Variables: 1`, `i(v1)` |
| `.op`, `.save i(v1)`, `print alli` | `alli = -1.00000e-03` | `v1#branch = -1.00000e-03` |
| `.tran`, `.save v(in)`, `write f allv` | `time` `v(allv)` | `time` `v(in)` |
| `.tran`, `.save v(in)`, `write f v(In)` | `time` `v(In)` | `time` `v(In)` — unchanged |
| `.tran`, `.save v(in)`, `write f v(In)+0` | `time` `v(In)+0` | `time` `v(In)+0` — unchanged |
| `.op`, `.save v(in)`, `write f v(In)` | `v(in)` `v(In)` | `v(in)` `v(In)` — unchanged |

The last three rows are the ones that had to *not* move. The fifth and sixth
are the client-visible behaviour scope option (ii) would have taken away — a
`fold` run still hands back the deck's own spelling when the deck names the
vector — and the seventh is the mechanism left in place, below.

**Every "before" above was run, not carried over from the predecessor batch's
table.** Two rows — `alli`, and `v(In)+0` on `.tran` — were not in that table,
so the pre-fix binary was rebuilt (`git checkout src/frontend/evaluate.c`,
`make -j8`), measured, and the fix restored and re-verified. `alli` is the one
that matters: nothing had ever measured it, and it behaves exactly as `all` and
`allv` do, which is the fourth of 0064's acceptance criteria.

## What was left, deliberately

- **`com_write()`'s scale prepend is untouched.** `src/frontend/postcoms.c` was
  not edited. A *partial* write of an `.op` plot still gains a column: after
  the fix, `.op` + `.save v(in)` + `write f.raw v(In)` writes
  `No. Variables: 2`, `v(in)` and `v(In)`, and under `preserve`, `distinguish`
  and stock the two carry a byte-identical name. That is item 5's issue.
  It is why `wildcard-rename.cir` asserts **names and not column counts**: a
  count assertion in that deck would have been an assertion about
  `com_write()`, and it would have been asserting the wrong thing.
- **`alle` was decided from the code and not from a run.** A live
  single-event-node measurement was attempted and produced nothing to report:
  `print alle` on a one-event-node deck says *"Warning from checkvalid: vector
  alle is not available or has zero length"*, and on a two-event-node deck
  (`din`, `dout`) prints nothing at all, both before the change and after it.
  Something on the `alle` path does not work in this tree; it is unrelated to
  the rename, it was **not** investigated, and it is not filed. Whoever wants
  `alle` covered by a deck has to settle that first. Flagged here so the next
  crew is not surprised, exactly as the predecessor receipt flagged the stale
  code models.
- **No client-facing document was touched.** 0064's fix has to reach the round-3
  reply (item 6), together with the fact that the second mechanism is filed and
  not fixed.

## For the owner

Acceptance criteria 1, 2, 3 and 5 of 0064 are met by measurement and by the
deck. Criterion 4's `alle` half is met by code reading only, for the reason
above; the other three wildcards in it are measured. If that is not good
enough, the prerequisite is an `alle` that produces output, which is a new
investigation and not this item's.
