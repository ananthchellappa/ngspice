# Receipt: follow-up 2, item 2 — P1 hardening

Crew **"P1 hardening"**, ver_51 follow-up batch 2, 2026-10-26, branch
`ver_51`. HEAD at start **`8a671e1c0`** (`docs: receipt for ver_51 follow-up
2, item 1, the typeless model crash`). The code commit is **`3a904f87d`**
(`Carry an R model whose duplicate cards in one scope agree`). This receipt
is the docs commit on top of it.

The three parts are follow-ups to P1 (the R model carried through the R→B
rewrite, `doc/claude/batches/2026-10-24-mc-v51/PLAN.md` item 1):

- (a) is follow-up 1's declined gap **B7**. The batch driver lifted the "no
  new runtime-vs-runtime comparison" constraint for this one comparison.
- (b) is the double-free window that review noted on follow-up 1's 1d fix.
- (c) is a mutation proof for follow-up 1's deck
  `temper-res-behav-noise-subckt.cir`.

**Code was touched**, in one production file, `src/frontend/inpcom.c`
(+10/−4). The owner's uncommitted `CLAUDE.md` and
`tests/regression/pipe/resource-keyword-case.cmd` were not staged or
modified. No `LEDGER.md`, `PLAN.md` or `tests/lint/identity.baseline` was
edited. No shared-library tree was created and no worktree was used. Nothing
was pushed. Every heavy command went through `ngheavy`, one at a time, with
`-j4` at most, and every build of `build-ver_50` was followed by `ngheavy make
install`. That includes the mutant build, which was installed and then
replaced.

Scratch: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/h2/`.
It holds:

- `mutation.patch`, `build-mut.log`, `install-mut.log`, `mut-check.log`,
  `mut-strong.log` (the mutant runs);
- `build-revert.log`, `install-revert.log`, `revert-check.log`,
  `revert-strong.log` (after the revert);
- `strong/` (the strengthened netlist, as tried before it was committed);
- `red-warn.log`, `red-dup.log` (RED for (a));
- `autogen.log`, `build.log`, `install.log`, `green-exit.log`,
  `green-temper.log`, `vg.log`, `check.log`;
- `lst.cir`, `lst2.cir` (listing decks).

## (a) — identical duplicate `.model` cards

**Before.** `behav_r_model_args()` counted the exact matches in the first
scope that had any, and any count other than 1 meant "not resolved". The
rewrite runs before `inp_dot_if()`, so a model written in both branches of an
`.if` is two matches. The R then lost its TC even when the two cards were the
same.

**After.** The first match is kept. Each later match in the same scope is
compared with it, and the comparison is the card's text after `.model
<name>`, byte for byte. The model is resolved when every card agrees. When any
card differs, the old warning is printed and nothing is carried. The
comparison is annotated:

```c
                /* case-lint: stored - two .model cards of this deck, text after the name */
                else if (strcmp(skip_token(skip_token(m->model->line)),
                        skip_token(skip_token(p->model->line))) != 0)
```

The names were already matched by `model_name_match()`, so only the text
after them is compared. That text includes the type token, so `r` and `R`
count as different. A card that differs only in the case of a key, or in
spacing, also counts as different. This errs towards "not resolved", which
is the old behaviour, and it always prints the warning.

**Tests.**

- New `tests/regression/temper/temper-res-behav-dup.cir`, registered in
  `TESTS`, with `.out` `INFO: success` (added with `git add -f`, as
  `tests/.gitignore` ignores `*.out`). It has two pairs:
  - **A:** the same `.model rm` in `.if` and `.else` at the top level.
  - **B:** the same `.model rs` in both branches inside a subckt, after the
    instance.

  Each converted R must match a plain R with the same model within 1e-9 at
  0, 25 and 125 °C, and it must move with temperature (spread ≥ 0.1).
- `tests/regression/exitstatus/behav-r-model-warn.cir` gains two cards:
  - **r10:** identical cards in `.if`/`.else`. Expected: no warning.
  - **r11:** three cards in `.if`/`.elseif`/`.else`, where the first two
    agree and the third differs. Expected: `not resolved`.

  r5, the existing two-different-cards case, still warns. The `.err` gained
  only r11's line, and no expected line was removed.

**RED**, against the unchanged binary `4b677f485e4dcdeed58d`. The
exitstatus deck (trimmed):

```
 Warning: behavioral R r5: model ramb not resolved, tc1/tc2/tnom not applied
 Warning: behavioral R r8: the expression uses temper and model rboth adds tc2, both are applied
+Warning: behavioral R r10: model rsame not resolved, tc1/tc2/tnom not applied
 Warning: behavioral R r11: model rthree not resolved, tc1/tc2/tnom not applied
check_status: behav-r-model-warn: FAILED (err)
FAIL: behav-r-model-warn.cir
```

The temper deck, run directly to show the numbers, then under `make check`:

```
Warning: behavioral R ra: model rm not resolved, tc1/tc2/tnom not applied
Warning: behavioral R rb: model rs not resolved, tc1/tc2/tnom not applied
Note: temp = 0 err = 0.192748
Note: temp = 125 err = 0.282051
Note: maxerr = 0.282051 spread = 0
ERROR: test failed
FAIL: temper-res-behav-dup.cir
```

Both decks fail for the expected reason: identical cards are "not resolved",
and the converted R is flat. r11 is the same before and after the fix.

**GREEN** on the new binary: `All 21 tests passed` (exitstatus) and `All 8
tests passed` (temper).

## (b) — the double-free window

There are two changes, and neither changes behaviour:

- On the normal path in `inp_readall_cards()`, `behav_r_models_root =
  NULL;` now comes before `inp_free_models(root)`. So
  `inp_netlist_read_reset()` can never see a root whose lists are being
  freed, or have been.
- `inp_free_models()` sets `level->models = NULL` before it walks the list,
  not after. A level whose walk is under way no longer points at freed
  nodes.

There is no RED test. The window needs a fatal exit inside
`inp_free_models()`, which has none, and the hook only runs in the shared
build, which is not built (the reason is in follow-up 1's receipt, 1d).
`valgrind --leak-check=full` on `behav-r-model-warn.cir` reports
`definitely lost: 0 bytes in 0 blocks` and `ERROR SUMMARY: 0 errors`
(`vg.log`).

## (c) — does the subckt noise deck catch a wrong-scope pick?

**Mutation** (`mutation.patch`, never committed): `behav_r_model_args()`
first walked `scope` to the outermost level and searched only there. It was
built (`bda1891b2f74786b75f0`) and installed.

**First finding: the deck as committed fails, but for the wrong reason.**

```
Warning: behavioral R ra: model rm not resolved, tc1/tc2/tnom not applied
Warning: behavioral R rd: model rm not resolved, tc1/tc2/tnom not applied
FAIL: temper-res-behav-noise-subckt.cir
```

Nothing at the top level used the global `.model rm`. The deck has no
`.if`, so `inp_rem_unused_models()` deletes that card before the rewrite
runs. The mutant therefore found *no* model, not the wrong one. The deck was
catching "nothing carried". It could not have told a wrong-scope pick from
that, and follow-up 1's receipt claimed that "if the noise resistor picked up
the global `rm`, the ratios would move away from 1". With the deck as it
stood, that pick was impossible.

**Fix to the deck (committed).** `temper-res-behav-noise-subckt.sp` gains a
top-level `ig 0 g dc 1u` / `rg g 0 rm r=73k`, with a comment saying why it is
there. That keeps the global card. Nothing the deck measures uses node `g`.
Under the mutant, the strengthened deck shows a real wrong-scope pick. The
listing shows the B line carrying the global TC and tnom offset:

```
    15 : b.xs.bra a 0 i= ... tc1=-2.47e-3 tc2=2.7e-6 dtemp= 2.0e+00 reciproctc=1 ...
    18 : r.xs.rra_2 xs.ra_3 0 1.0 xs:rm
Note: temp = 0 err = 0.268413
Note: temp = 25 err = 0.0640845
Note: temp = 125 err = 0.444942
Note: maxerr = 0.444942 spread = -0.297652
ERROR: test failed
```

**Revert.** The patch was removed with `git apply -R`, and the tree was
rebuilt and installed. The rebuilt binary is `4b677f485e4dcdeed58d`, byte
for byte HEAD's binary. Both the committed deck and the strengthened one
pass:

```
PASS: temper-res-behav-noise-subckt.cir
PASS: temper-res-behav-1.cir
Note: maxerr = 3.33067E-16 spread = 0.522213      (strengthened deck)
INFO: success
```

The mutant also failed `temper-res-behav-1.cir`, pair B (`model rs not
resolved`). That deck has no global `rs` at all.

## `make check`

| run | binary | PASS | FAIL | XFAIL | XPASS | SKIP | `make` exit |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `tests/regression/exitstatus` + `tests/regression/temper` | `ff62bd5239326574ff8f` | 21 + 8 | 0 | 0 | 0 | 0 | 0 |
| full `make check` from `build-ver_50/` | `ff62bd5239326574ff8f` | **361** | 0 | 0 | 0 | 0 | **0** |

361 is item 1's 360 plus `temper-res-behav-dup.cir`. One log line starts
with `ERROR:`. It is `ERROR: (internal)  tried to destroy non-existent
graph`, the known output of `vector-probe-report.cir`, which passes.
`./autogen.sh` was re-run for the two `Makefile.am` edits. The build's only
`inpcom.c` warning is the pre-existing `'/*' within comment` at line 1823,
which is not in this change.

**Lint**, from the log:

```
identity lint: 264 comparisons, baseline matches
identity lint: 11 comparisons, baseline matches
```

These counts are unchanged. The new `strcmp` is annotated, so it is
suppressed and does not enter the found set. The baseline was not edited.

**Binary:** `sha256sum build-ver_50/src/ngspice | cut -c1-20` →
**`ff62bd5239326574ff8f`** (was `4b677f485e4dcdeed58d`).

## For the owner

1. **A model written the same way in both branches of an `.if` now keeps
   its temperature coefficients** when the resistor is turned into a
   behavioral source. If the copies differ in any way, the old warning
   appears and nothing is carried. The test is strict: even a difference in
   letter case or spacing between the cards counts as a difference. That
   only ever produces the old warning, never a wrong value.
2. **The subcircuit noise test was weaker than claimed, and it is fixed.**
   The global model it meant to compete with was being deleted as unused
   before the rewrite. So a lookup that looked in the wrong place found
   nothing, instead of finding the wrong model. The test now uses that
   global model at the top level. The deliberate mutation then fails it on
   the numbers, and the real code passes it.
3. **The freeing-order change has no test of its own.** It only matters in
   the shared library, which is not built here. Valgrind on the
   standalone binary is clean.

Nothing was pushed. Two commits on `ver_51`: `3a904f87d` (code and tests)
and this receipt.
