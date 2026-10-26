# Receipt: follow-up 1 — P1 fixes (review issues 1a–1e, gaps B6–B7)

Follow-up crew **"P1 fixes"** for the ver_51 MC batch. The issues are item 1's
open review issues in `doc/claude/batches/2026-10-24-mc-v51/receipts/06-close.md`
section C (1a–1e), and the P1 gaps in section B (6, 7). The item's own receipt
is `doc/claude/batches/2026-10-24-mc-v51/receipts/01-p1-r-model-through-b.md`.
The spec is that batch's `PLAN.md`, item 1, which the owner approved.

Crew P1-fixes, 2026-10-26, branch `ver_51`. HEAD at start **`6df77429c`**
(`docs: fill in the ver_51 MC batch ledger`). The code commit is
**`82d8e7018`** (`Warn about a temper TC only when the R model adds it`).
This receipt is the docs commit on top of it.

**Code was touched.** There is one production file, `src/frontend/inpcom.c`
(+24/−5). The owner's uncommitted `CLAUDE.md` and
`tests/regression/pipe/resource-keyword-case.cmd` were not staged or
modified. No `LEDGER.md` or `PLAN.md` was edited, and neither was
`tests/lint/identity.baseline`. `build-ver_51-shared/` was not touched, built
in or deleted. Nothing was pushed or sent anywhere. Every heavy command went
through `ngheavy`, one at a time, with `-j4`.

Scratch: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/f1/`.
It holds `red-warn.stderr` (the RED capture for 1b), `autogen.log`,
`build.log`, `install.log`, `check.log`, the listing deck `lst.cir`, and the
1d probe decks `malformed.cir`, `typeless*.cir`.

## Issues taken

| issue | outcome | what |
| --- | --- | --- |
| 1a | fixed | `tests/regression/exitstatus/Makefile.am`: `TESTS =tran-…` is `TESTS = tran-…` again. That one line, nothing else. |
| 1b | fixed, RED first | The temper+TC warning fires only for a TC the model adds, and names it (`tc1`, `tc2` or `tc1/tc2`). An instance-only TC gets no warning, with or without a model. |
| 1c | corrected below | See *Correction to receipt 01*. Receipt 01 is not edited. |
| 1d | fixed | The rewrite's `.model` lists are freed in `inp_netlist_read_reset()`, which `shared_exit()` calls on every fatal exit. No RED test is possible in tree. The reason is given below. |
| 1e | fixed | New deck `tests/regression/temper/temper-res-behav-noise-subckt.cir` + `.sp` + `.out`. It fails on the pre-P1 binary and passes on this one. |
| B6 | declined | Each of its three parts is ruled out by a plan rule or an owner decision. See *B6 and B7*. |
| B7 | declined | The PLAN's identity-lint constraint forbids the comparison a fix needs. See *B6 and B7*. |

## 1b — the temper+TC warning

**Before.** In `behav_r_model_args()`, the test was `(tc1 || tc2)` after the
instance had overridden the model. It fired when only the instance gave the
TC, and its text always said `model %s adds tc1/tc2`. That text was also wrong
for the committed case r3: model `rok` has `tc1` only, yet the committed
`.err` asserted `adds tc1/tc2`.

**Decision: an instance-only `tc1`/`tc2` plus `temper` does not warn, on
either path.** The reasons:

- Decision 4's warning is there because P1 started carrying a TC the card
  does not show, the model's, onto an expression that may already model
  temperature (xschem's `fixp.cir` patch).
- An instance `tc1`/`tc2` has always been carried. Before the batch,
  `eval_tc()` put it on the B line both with and without a model token. It is
  written on the card next to the expression, so nothing is hidden and nothing
  changed.
- Warning on the no-model path would add a stderr line to cards whose
  behaviour this batch does not otherwise change. The PLAN's rule for those is
  that they keep "exactly today's line".

So the two paths are consistent: the warning means "the model added this
TC", and it fires only then.

**Test change.** `behav-r-model-warn.cir` gains three cards and one model:

- **r7:** the instance `tc1` overrides model `rok`'s only TC, on a temper
  expression. Expected: no warning.
- **r8:** the same, but model `rboth` also has `tc2`. Expected: `adds tc2`.
- **r9:** a temper expression with instance `tc1` and no model. Expected: no
  warning. This was already true, and the card pins it.

The `.err` changed in two places, and both changes are deliberate:

- r3's line now reads `adds tc1` instead of `adds tc1/tc2`. The old text was
  the false text this issue is about, so the assertion was correcting a wrong
  expectation, not weakening one.
- r8's line was added.

**RED**, `ngheavy make -C build-ver_50/tests/regression/exitstatus check TESTS=behav-r-model-warn.cir`,
against the unchanged binary `e53fb551360f6cea4f69`. Trimmed:

```
 Warning: behavioral R r1: model nosuch not resolved, tc1/tc2/tnom not applied
 Warning: behavioral R r2: model rt has tce, which a B source cannot express, model not applied
-Warning: behavioral R r3: the expression uses temper and model rok adds tc1, both are applied
+Warning: behavioral R r3: the expression uses temper and model rok adds tc1/tc2, both are applied
 Warning: behavioral R r4: model rtemp uses temper, tc1/tc2/tnom not applied
 Warning: behavioral R r5: model ramb not resolved, tc1/tc2/tnom not applied
-Warning: behavioral R r8: the expression uses temper and model rboth adds tc2, both are applied
+Warning: behavioral R r7: the expression uses temper and model rok adds tc1/tc2, both are applied
+Warning: behavioral R r8: the expression uses temper and model rboth adds tc1/tc2, both are applied
check_status: behav-r-model-warn: FAILED (err)
FAIL: behav-r-model-warn.cir
```

All three wrong behaviours fail as expected: the false `tc1/tc2` for r3, the
spurious r7 warning, and the false `tc1/tc2` for r8. r9 is silent before and
after the fix.

**Fix.** The fix computes `mtc1 = mk.tc1 && !ik.tc1` and
`mtc2 = mk.tc2 && !ik.tc2`, warns on `(mtc1 || mtc2)`, and prints
`tc1/tc2`, `tc1` or `tc2`. What is carried onto the B line does not change.
After the fix the deck passes.

## 1d — the model lists on a fatal exit

**The leak is reachable.** Any `controlled_exit()` between
`inp_collect_models()` and `inp_free_models()` reaches it, and so does one
inside `inp_collect_models()`. For example, with the standalone binary:

```
$ ngspice --batch malformed.cir      # .model rok r tc1=1e-3 / r1 1 0 rok 1k time
ERROR: mal formed resistor instance R: r1 1 0 rok 1k time
    in line no. 4 of file malformed.cir
    {...} or '...' around equation's right hand side are missing!
ERROR: fatal error in ngspice, exit(1)
```

`behav_r_models_wanted()` finds `rok`, so the lists are built. `inp_compat()`
then exits from inside the window. In the shared build that exit is
`shared_exit()`, and before this fix the lists were never freed.

**Fix.** A file static, `behav_r_models_root`, is set to `root` just before
`inp_collect_models()` and cleared just after `inp_free_models(root)`.
`inp_netlist_read_reset()` frees the lists and clears the pointer when the
static is non-NULL. All three ways out of a read that does not return already
call that function:

- `shared_exit()` (`sharedspice.c:2196`), which also covers the
  `pthread_exit()` route;
- `ft_sigintr_cleanup()` (`signal_handler.c:72`);
- `totalreset()` (`sharedspice.c:2638`).

So the new static is also cleared by `ngSpice_Reset`, as the PLAN's reset rule
requires. On the normal path the pointer is NULL again before `inp_compat()`
returns to the caller, so the hook does nothing then. The scope tree it walks
is never freed on those exit paths (it leaks as it did before, together with
the deck), so its memory is still valid when the hook runs.

**Why there is no RED test.** In the standalone binary, `controlled_exit()`
calls `exit()`. The hook is not on that path, and the process is gone, so the
binary under `make check` cannot show the leak or its fix. The only build
where the leak exists is the shared library. The task forbids
`build-ver_51-shared/`, and the PLAN forbids a second configured tree. The fix
was compiled into `build-ver_50` (no warnings from `inpcom.c` in
`build.log`), but it was **not run** on the shared path. `valgrind
--leak-check=full` on `behav-r-model-warn.cir` gives `definitely lost: 0
bytes` and `ERROR SUMMARY: 0 errors`, which shows that the normal path does
not double-free.

**Side finding, not fixed (pre-existing).** The review named one exit:
`inp_collect_models()`'s `controlled_exit(EXIT_BAD)` on a typeless `.model`,
in a deck with `.if`. That exit is not reached by the obvious deck, because a
typeless `.model bad` in a `.if` deck **segfaults earlier**:

- It segfaults in `strchr`, on both the current binary and the pre-P1 binary
  (`scratchpad/p1/old/ngspice`).
- It does so with a converted R and without one (`typeless.cir`,
  `typeless2.cir`: exit 139).
- Without `.if`, both binaries print `Error: no model type given` and exit 1
  (`typeless3.cir`).

So the crash is upstream behaviour that predates this batch. It is outside
P1's scope and is recorded for the owner. The binary is stripped (`-O2 -s`),
so the backtrace has no symbols.

## 1e — the subckt noise branch

New deck `tests/regression/temper/temper-res-behav-noise-subckt.cir`, with its
netlist in `.sp`, registered in `TESTS` and `EXTRA_DIST`. Its `.out` is
`INFO: success`, like its sibling. Since `tests/.gitignore` ignores `*.out`,
it was added with `git add -f`, as the existing `.out` files are tracked.
`./autogen.sh` was re-run.

The deck sets `enable_noisy_r` and sources the `.sp`. The `.sp` has a global
`.model rm` (`tc1=-2.47e-3 tc2=2.7e-6 tnom=25`) and, inside `.subckt sr`, a
local `.model rm` with a different TC and tnom (`tc1=4e-3 tc2=-1e-6
tnom=40`). The local model is written *after* the instances, as in sky130. It
has two pairs: converted against plain, and converted `dtemp=10` against plain
`dtemp=10`. At 0, 25 and 125 °C each ratio must be within 1e-9, and the
converted R's noise must change by more than 10 % from 0 to 125 °C, so that a
noiseless or flat branch cannot pass. If the noise resistor picked up the
global `rm`, the ratios would move away from 1.

`listing e` shows the translation (trimmed):

```
    17 : b.xs.bra_1 a 0 i= i(v.xs.vra_3) / sqrt ( … )
    18 : r.xs.rra_2 xs.ra_3 0 1.0 xs:rm
    20 : r.xs.rb b 0 xs:rm r=73k
    26 : r.xs.rrd_2 xs.rd_3 0 1.0 xs:rm dtemp=10
    29 : .model xs:rm r tc1=4e-3 tc2=-1e-6 tnom=40
```

**RED.** The behaviour already shipped in `4e614b4da`, so the failing run is
against the pre-P1 binary kept by item 1's crew
(`scratchpad/p1/old/ngspice`):

```
Note: temp = 0 err = 0.0921301
Note: ratio = 1.09213 1.04755
Note: temp = 125 err = 0.156474
Note: ratio = 0.866207 0.843526
Note: maxerr = 0.156474 spread = 0.207321
ERROR: test failed
```

On the current binary:

```
Note: temp = 0 err = 2.22045E-16
Note: temp = 125 err = 3.33067E-16
Note: maxerr = 3.33067E-16 spread = 0.522213
INFO: success
```

## B6 and B7

**B6, declined, every part.**

- *No-model converted R drops instance `dtemp`/`temp`/`tc=`.* This is
  forbidden by PLAN item 1's rule that "a card without a model token gets
  **exactly today's line**, byte for byte". It is repeated in *Acceptance*,
  which relies on the committed R→B decks being unchanged. Fixing it would also
  change upstream behaviour for every model-less converted R, which is outside
  this batch's scope. On the merits, the fix is worth doing, but as its own
  item, with that rule lifted by the owner.
- *Flicker noise not reproduced.* This cannot be done inside the rewrite's
  topology. The noise branch's 1 Ω resistor carries no DC current, so its
  `kf`/`af` term is 0. Reproducing it needs a different noise network, which
  is a design change the plan does not ask for ("Flicker noise cannot be
  reproduced. Document that.").
- *`.option tnom≠27` keeps ASRC's 27 °C reference.* This is owner decision 3,
  approved as **decline**.

**B7, declined.** Identical duplicate `.model` cards in one scope still carry
nothing. On the merits the plan's own rule is better: carry nothing only when
the duplicates *disagree*. The fix is to compare the two cards' parameter text
byte for byte. That is a new runtime-vs-runtime comparison, and PLAN's
standing constraint says: "**Identity lint.** No new runtime-vs-runtime
`strcmp`/`cieq`/`eq`/`eqc`." The lint would accept an annotated call (see the
correction below), but the plan rule forbids the call itself. A `memcmp`, or
comparing the extracted values one by one, would only dodge the rule's wording.
If the owner lifts that constraint for this one site, the fix is small: in
`behav_r_model_args()`, compare `skip_token(skip_token(line))` of each further
exact match with the first, under `/* case-lint: neither - two .model cards'
parameter text, byte for byte */`, and treat equal texts as one match.

## The two `make check` counts

| run | binary | PASS | FAIL | XFAIL | XPASS | SKIP | ERROR | `make` exit |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| targeted: `tests/regression/exitstatus` + `tests/regression/temper` | `0d61507c992f9ecfd8be` | 14 + 7 | 0 | 0 | 0 | 0 | 0 | 0 |
| full `make check` from `build-ver_50/` | `0d61507c992f9ecfd8be` | **348** | 0 | 0 | 0 | 0 | 0 automake results | **0** |

348 is the close receipt's 347 plus the new subckt noise deck. One log line
starts with `ERROR:`. It reads `ERROR: (internal)  tried to destroy
non-existent graph`, the known simulator output from
`vector-probe-report.cir`, which passes.

**Lint**, both lines from the log:

```
identity lint: 264 comparisons, baseline matches
identity lint: 11 comparisons, baseline matches
```

These are unchanged. The fix adds no comparison.

**Binary:** `sha256sum build-ver_50/src/ngspice | cut -c1-20` →
**`0d61507c992f9ecfd8be`** (was `e53fb551360f6cea4f69`). `./autogen.sh`,
`ngheavy make -j4` and `ngheavy make install` were run, in that order.

## Correction to receipt 01 (issue 1c)

Receipt 01, *What was falsified in the plan*, item 2, says that the
"disagreeing duplicates" rule "cannot be tested without a
runtime-against-runtime string comparison of the two cards, and **the identity
lint forbids that**". The close receipt repeats review's correction: "a
`/* case-lint: stored */` annotation would allow it, and **the frozen baseline**
is what actually stopped the crew."

Both reasons are imprecise. Read from `tests/bin/identity_lint.sh` and
`identity_lint.awk`:

- The lint does not forbid such a comparison. A call with `case-lint:` on its
  own line or the line above is suppressed (`annotated[...]` in the awk).
- A suppressed call never enters the found set, so it does not touch
  `tests/lint/identity.baseline` either. An annotated comparison leaves the
  baseline as it is and the lint passes, so the frozen baseline did not stop
  anything.
- The rule that does forbid it is **PLAN's standing constraint**: "No new
  runtime-vs-runtime `strcmp`/`cieq`/`eq`/`eqc`". That is a plan rule, not a
  tool limit. The owner can lift it, and doing so would cost one annotated
  line (see B7).
- The best-fitting reason word would be `neither` (free text: two cards'
  parameter text) rather than `stored`, which the lint's help reserves for
  names the simulator itself holds.

Receipt 01 is left as written.

## For the owner

1. **The temper+TC warning now means "the model added this TC".** An
   instance's own `tc1`/`tc2` on a temper expression is never warned about,
   with or without a model. If you want those flagged too, that is a new
   warning on the no-model path, outside this batch's byte-for-byte rule.
2. **1d's fix has not been run on the shared path.** It needs a shared build,
   and `build-ver_51-shared/` was off limits. Once you decide about that tree,
   running `malformed.cir` through `ngSpice_Circ` under valgrind would confirm
   the fix.
3. **New pre-existing bug:** a typeless `.model` in a deck with `.if`
   segfaults (exit 139) in 45.2-derived code as well as on `ver_51`. It is not
   from this batch. It would need its own issue under `doc/codex/issues/`.
4. **B6 and B7 are declined** under the plan's byte-for-byte rule and its
   identity-lint constraint. Both are worth doing if you lift those rules; B7
   then costs one annotated comparison.

Nothing was pushed. Two commits on `ver_51`: `82d8e7018` (code and tests)
and this receipt.
