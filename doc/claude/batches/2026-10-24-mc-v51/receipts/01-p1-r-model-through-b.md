# Receipt: item 1 — P1, carry the R model through the R→B rewrite

PLAN item **1 — P1: carry the R model through the R→B rewrite**, from
`doc/claude/batches/2026-10-24-mc-v51/PLAN.md`. The owner approved every
recommendation under *Decisions for the owner*. For this item that means
decision 2 (noise stays behind the switch, and the noise branch gets the
model), decision 3 (ASRC's `300.15` is left alone) and decision 4 (a TC on a
`temper` expression is applied, with a warning).

Crew P1, 2026-10-26, branch `ver_51`. HEAD at start **`ae2223e41`**
(`docs: record the ver_51 batch baseline`). The code commit is **`4e614b4da`**
(`Carry the R model through the R->B rewrite`). This receipt is the docs
commit on top of it.

**Code was touched.** There is one production file, `src/frontend/inpcom.c`.
The ASRC device is unchanged. Nothing else under `src/` was opened for
editing. The owner's uncommitted `CLAUDE.md` and
`tests/regression/pipe/resource-keyword-case.cmd` were not staged or
modified. `PLAN.md`, `LEDGER.md` and `tests/lint/identity.baseline` were not
edited.

Scratch: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/p1/`.
It holds the RED captures (`red-*.stdout`, `red-*.log`), `makecheck.log`,
`build*.log`, the `pairb.listing` and `noise.listing` captures, the
`valgrind` logs (`vg*.stderr`), the before/after comparisons (`cmp/`), the
static scanner (`scan.py`) and a copy of the before binary (`old/ngspice`).

## What changed, file by file

| file | change |
| --- | --- |
| `src/frontend/inpcom.c` | +366/−10 lines. See *The production change*. |
| `tests/regression/temper/temper-res-behav-1.cir`, `.out`, `.lib` | The acceptance deck: eight pairs, at 0/25/125 °C, every ratio held to 1e-9. The `.lib` supplies pair C. |
| `tests/regression/temper/temper-res-behav-noise.cir`, `.out`, `.sp` | The noise deck. The switch is read while the netlist is parsed, so this deck sources `.sp` twice, once with `enable_noisy_r` off and once with it on. |
| `tests/regression/temper/Makefile.am` | Two `TESTS`. The `.lib` and `.sp` go in `EXTRA_DIST`. |
| `tests/regression/exitstatus/behav-r-model-warn.cir`, `.status`, `.err` | Asserts the warning text on stderr, through `check_status.sh`. |
| `tests/regression/exitstatus/Makefile.am` | One `TESTS`, two `EXTRA_DIST` and four `CLEANFILES` entries, plus a header paragraph saying why the deck is here. |

`./autogen.sh` was re-run after the two `Makefile.am` edits, then the tree
was rebuilt and `make install` was run.

## The RED failure, verbatim

All three decks were written and run before the fix. The two `temper` decks
ran against the baseline binary `2976d0243ee800c7f461`, which is the hash in
`00-baseline.md`. `behav-r-model-warn.cir` was written after the fix had been
coded. For its RED step, `inpcom.c` was put back to `HEAD`, the tree was
rebuilt (`7e7b5610a68aa28a079e`, which differs from the baseline only because
`autogen.sh` re-ran `configure`) and the deck was run. The fix was then
restored and the tree rebuilt.

`make -C build-ver_50/tests/regression/temper check`, before:

```
PASS: temper-1.cir
PASS: temper-2.cir
PASS: temper-3.cir
PASS: temper-res-1.cir
--- temper-res-behav-1.out_tmp
+++ temper-res-behav-1.test_tmp
@@ -1 +1,16 @@
-INFO: success
+
 (… blank lines only …)
FAIL: temper-res-behav-1.cir
--- temper-res-behav-noise.out_tmp
+++ temper-res-behav-noise.test_tmp
@@ -1 +1,75 @@
-INFO: success
 (… blank lines only …)
FAIL: temper-res-behav-noise.cir
```

The missing success line is the expected failure, as the plan says. The
deck's own `ERROR: test failed` line is deleted by `check.sh`'s FILTER,
because the FILTER entry `est` matches inside "test". The deck's `Note:`
lines, captured before filtering, give the reason. The pairs are A B C D E F
G H, as listed in the deck header:

```
Note: temp = 0 err = 0.0642472
Note: ratio = 0.940347 0.940347 0.940347 0.963709 1.0639 0.996224 0.935753 0.940347
Note: temp = 25 err = 0.0639005
Note: ratio = 1 1 1 1.02504 1.0639 0.998 0.995074 1
Note: temp = 125 err = 0.314112
Note: ratio = 1.28205 1.28205 1.28205 1.31411 1.0639 0.974268 1.27572 1.28205
Note: maxerr = 0.314112 spread = 0
```

At 125 °C the ratio is `1.28205`, and 0.073 / 0.05694 = 1.28205. That is the
measurement in the plan. `spread = 0` shows that the converted R is flat
with temperature. Pair E reads `1.0639` at every temperature, which is
`temp=50` being dropped: (1−2.47e-3·25+2.7e-6·625)⁻¹ = 1.0639. Pair D at
25 °C reads `1.02504`, which is `dtemp=10` being dropped.

Noise deck, before (each value is a ratio of integrated noise voltages, so
it is the square root of the resistance ratio):

```
Note: temp = 0 err = 0.0516928
Note: ratio = 0.969715 0.964196 0.948307
Note: temp = 25 err = 0.00924599
Note: ratio = 1 0.99588 0.990754
Note: temp = 125 err = 0.144911
Note: ratio = 1.13228 1.13222 1.14491
Note: maxerr = 0.144911 quiet = 0
```

`quiet = 0` is the gate: with `enable_noisy_r` unset, a converted R is
noiseless. That was true before the fix and is still true after it, and the
deck pins it.

`make -C build-ver_50/tests/regression/exitstatus check TESTS=behav-r-model-warn.cir`, before:

```
--- behav-r-model-warn.stderr.expected
+++ behav-r-model-warn.stderr.filtered
@@ -1,5 +0,0 @@
-Warning: behavioral R r1: model nosuch not resolved, tc1/tc2/tnom not applied
-Warning: behavioral R r2: model rt has tce, which a B source cannot express, model not applied
-Warning: behavioral R r3: the expression uses temper and model rok adds tc1/tc2, both are applied
-Warning: behavioral R r4: model rtemp uses temper, tc1/tc2/tnom not applied
-Warning: behavioral R r5: model ramb not resolved, tc1/tc2/tnom not applied
check_status: behav-r-model-warn: FAILED (err)
FAIL: behav-r-model-warn.cir
```

Stderr was empty before the fix. Every one of these cases was silent.

After the fix, the same three decks pass. `temper-res-behav-1` reads `err =
2.22045E-16` at every temperature, `spread = 0.22`, and all eight ratios are
`1`. The noise deck reads `maxerr = 2.22045E-16` and `quiet = 0`.

## The production change

All of it is in `src/frontend/inpcom.c`.

- **Model table.** The `.model` collection loop is factored out of
  `inp_rem_unused_models()` into `inp_collect_models()`. That function
  returns the final `.control` nesting count, which the caller carries into
  its second walk as before, so even an unterminated `.control` behaves as it
  did. `inp_free_models()` is the new free-only walk. Nothing is commented
  out. In `inp_readall`, `behav_r_models_wanted()` gates the work. When some
  R card both needs the rewrite and names a model token, the lists are built
  just before `inp_compat()` and freed just after it. No `return` lies
  between those two points, so the lists cannot leak on an early exit.
  Decks with `.if` and decks without it are handled the same way.
- **Token.** `behav_r_model_token()` reads the card after `node2`. It
  tokenizes brace-aware, skips one optional value token (a number or a whole
  `{…}`), and then accepts the next token if `is_a_modelname()` does. When
  no model token is found, the line is built by exactly the old code, from
  `tcrstr`.
- **Lookup.** `behav_r_model_args()` walks `card->level` outwards. It counts
  only `elemb == 'r'` models with `model_name_match(…) == 1`, so a binned
  `name.N` does not count. The first scope with any hit wins. If that scope
  has other than exactly one hit, or no scope has one, the function warns
  `not resolved` and nothing is carried.
- **Extraction.** `behav_r_kv_scan()` collects `key=value` pairs. Tokens are
  split at blanks, parentheses and commas outside braces, so `r(tc1=…)`
  model cards work. Each key is compared with a literal using `cieq`. Model
  keys are `tc1|tc1r`, `tc2|tc2r`, `tce` and `tnom`. Instance keys are
  `tc|tc1`, `tc2`, `dtemp` and `temp`. When a key repeats, the later pair
  wins. Instance values override model values one parameter at a time.
- **Temperature reference.** If the model has a tnom, the B line gets
  `dtemp={(<dt|0>)+27-(<tnom>)}`. If the instance gives `temp=`, it gets
  `temp={(<T>)+27-(<tnom>)}` instead. Without a model tnom, the instance's
  `temp=` or `dtemp=` is passed through, and `temp` wins as it does in the
  device.
- **Noise.** The gate is unchanged. When the gate is on and a model
  resolved, the noise branch's resistor is
  `r<name>_2 <name>_3 0 1.0 <model> [tc1=] [tc2=] [temp=|dtemp=]`, using the
  instance's own values, and the resistor device applies the model itself.
- **Warnings**, all to stderr: not resolved (also used for two or more exact
  matches), `tce`, a model that uses `temper` (nothing carried), and a TC on
  a `temper` expression (carried, decision 4).
- **`get_model_name()`** for R cards now skips a leading `{…}` value as well
  as a numeric one. See *What was falsified*.

`listing e` of pair B, showing the `tc1`/`tc2`/`dtemp` text on the B line
(after the fix, then before it):

```
     9 : b.xb.br1 b 0 i= v(b,0) / (   1.4600000000e+05 * (    5.000000000000000e-01    ) * (   1.0000000000e+00 +   1.0000000000e-09 * abs ( v(b) ) ) ) tc1=   -2.470000000000000e-03     tc2=2.7e-6 dtemp=    2.000000000000000e+00     reciproctc=1 reciprocm=0
    10 : .model xb:rs r tc1=   -2.470000000000000e-03     tc2=2.7e-6 tnom=25
======  before
     9 : b.xb.br1 b 0 i= v(b,0) / ( … ) reciproctc=1 reciprocm=0
```

`tc1={-4.94e-3*rsq}` was resolved against the global `rsq=0.5` by numparam,
and `dtemp={(0)+27-(25)}` became `2`. Noise branch under `set
enable_noisy_r`, for `ra a 0 rm r={73k+0*v(a)} dtemp=10 tc1=1e-3`:

```
     7 : bra a 0 i= v(a, 0) / ( … ) tc1=1e-3 tc2=2.7e-6 dtemp=    1.200000000000000e+01     reciproctc=1 reciprocm=0
     9 : bra_1 a 0 i= i(vra_3) / sqrt ( … )
    10 : rra_2 ra_3 0 1.0 rm tc1=1e-3 dtemp=10
    11 : vra_3 ra_3 0 0
```

## The two `make check` counts

| run | binary | PASS | FAIL | XFAIL | XPASS | SKIP | ERROR | `make` exit |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| targeted: `tests/regression/temper` + `tests/regression/exitstatus` | `53e6e42b7a4ed38ca28a` | 6 + 14 | 0 | 0 | 0 | 0 | 0 | 0 |
| full `make check` from `build-ver_50/` | `53e6e42b7a4ed38ca28a` | **338** | 0 | 0 | 0 | 0 | 0 automake results | **0** |

338 = the baseline's 335 + the three new decks. No existing test moved. One
line matches `^ERROR:`: `ERROR: (internal)  tried to destroy non-existent
graph`. It is the same pre-existing simulator output from
`vector-probe-report.cir` that the baseline recorded, and that deck passes.

**Lint:** `identity lint: 264 comparisons, baseline matches` and `identity
lint: 11 comparisons, baseline matches`. These are the baseline's counts.
Every new comparison has a literal operand: `cieq(key, "tc1")`, and
`search_plain_identifier(…, "temper")`, which is not a comparator. Model
names go through `model_name_match()`.

**Binary:** `sha256sum build-ver_50/src/ngspice | cut -c1-20` →
`53e6e42b7a4ed38ca28a`. `make install` was run after the build.

## What was measured

- **Which existing decks the change perturbs: none.** `scan.py` scans
  `tests/` and `examples/` for R cards outside `.control` whose text after
  the nodes needs the rewrite and that carry a model token. It finds no
  instance cards. Its only hits are the title lines of `temper-2.cir` and
  `temper-3.cir`, which are not cards. A second grep looks for value-first
  `{…} model` R cards, the shape the `get_model_name()` change affects. It
  finds only the new pair H. The plan's named R→B decks (`asrc-tc-2`, the
  four `compat-r-behav-case`/`behav-r-vnode-case` variants) and
  `examples/various/tmp-noise.cir` were run on the before and after
  binaries. Their stdout and stderr are **byte-identical** once the
  date/time/memory lines are removed. `git status` shows no tracked `.out`
  modified, and none was regenerated.
- **Memory.** `valgrind --leak-check=full` on `temper-res-behav-1.cir` and
  `behav-r-model-warn.cir` reports `ERROR SUMMARY: 0 errors` and `definitely
  lost: 0 bytes` for both.
- **What the pairs cover:**
  - the model's TC: A
  - a subckt, with the model after the instance and a global `.param` in the
    model: B
  - `.lib file section` with the `tc1r`/`tc2r` aliases: C
  - instance `dtemp`: D
  - instance `temp=`: E
  - an instance `tc1` override that keeps the model's `tc2`: F
  - a model without `tnom`: G
  - value-first `{eq} model`: H

  The warnings deck covers unresolved, `.if`/`.else` duplicates, `tce`, a
  model that uses `temper`, `temper`+TC, and no warning for a model that
  resolves. The noise deck covers the gate off, then the gate on, with plain,
  `dtemp` and `temp` resistors at three temperatures.

## What was falsified in the plan

1. **"Token: skip one optional value token" was not enough on its own.** A
   value-first card `R n 0 {eq} m`, whose model is used only there, lost the
   model before the rewrite ever ran. `get_model_name()` skipped a numeric
   value but not a `{…}`, so `inp_rem_unused_models()` and
   `comment_out_unused_subckt_models()` saw `{eq}` as the model name and
   commented the real `.model` out as unused. Measured with the
   `get_model_name()` hunk removed: `Warning: behavioral R rh: model rq not
   resolved`. Measured on the same binary: a **plain** R `r2 b 0 {73k} rq2`
   failed with `unknown parameter (rq2)`, `Simulation interrupted due to
   error!`. That is a pre-existing bug outside the rewrite, and the same hunk
   fixes it. With the hunk, both read `5.496900e-02` at 125 °C, which is
   correct.
2. **"If several exact matches in the winning scope disagree"** cannot be
   tested without a runtime-against-runtime string comparison of the two
   cards, and the identity lint forbids that. The rule shipped is "more than
   one exact match → not resolved", with the plan's warning text. Identical
   duplicate `.model` cards in one scope are therefore not carried either.
3. **`tce` needed a decision the plan did not make.** In the real device,
   `tce` replaces tc1/tc2 (`restemp.c:102-105`). Carrying tc1/tc2 for a `tce`
   model would give a value that matches neither the real R nor the old
   result, so the shipped behaviour is warn and carry nothing.
4. **"Without tnom, pass the instance dtemp=/temp= through. That fixes
   today's silent drop, which also happens with no model."** This conflicts
   with "a card without a model token gets exactly today's line, byte for
   byte", which is also in the plan's acceptance. The byte-for-byte rule was
   kept. The pass-through applies only on the model path. With no model, a
   converted R still drops `dtemp=`/`temp=`.
5. **Size.** The plan estimated about 130–180 lines. The change is +366/−10,
   and about a third of that is comments.

Confirmed, not falsified: the per-scope lists are NULL when the rewrite runs,
both with `.if` and without it (`rem_unused_xxx` NULLs them). `reset` does
not re-run the rewrite, so `enable_noisy_r` has to be set before the netlist
is read, which is why the noise deck sources its netlist.

## What was left, deliberately

- **Instance `tc=` on a card without a model** is still dropped, because
  `eval_tc` matches only `tc1=`/`tc2=`. Fixing it would change the no-model
  line. On the model path, `tc=` is honoured as `tc1`.
- **Instance `tce`** on a converted R is ignored, as before.
- **Under `.option tnom≠27`**, a model without `tnom` keeps the ASRC's fixed
  27 °C reference, while a real R uses `CKTnomTemp`. This follows
  decision 3 and is documented in the code comment.
- **Flicker noise** of the model (`kf`/`af`) is not reproduced. The noise
  branch's resistor carries no DC current, so its flicker term is 0.
- **Scope shadowing.** An outer-scope model's `{expr}` that names a parameter
  the subckt shadows binds to the inner one, because the text is copied into
  the instance's scope. This is rare and is noted, not handled.
- **`eval_tc`'s whole-line `cistrstr`** was not widened. It still runs and is
  used on the no-model path only.

## For the owner

1. **Results change for every converted R that names a model**, sky130
   `xhigh_po` among them. That is the purpose of the change. The release note
   (item 6) must say so, and that xschem's `fixp.cir`-style temper patch has
   to be reverted, or the new `temper`+TC warning will fire and the TC will
   be applied twice.
2. **The warnings deck is in `tests/regression/exitstatus/`**, because that
   is the only directory driven by `check_status.sh`, which keeps `Warning`
   lines. A Makefile.am paragraph says why. Item 2 creates
   `tests/regression/rng/` with the same driver. If you would rather the
   deck lived in its own directory, the move costs a `configure.ac` entry.
3. **The `get_model_name()` hunk also fixes plain value-first R cards**
   (`R n1 n2 {expr} model`), which used to fail with `unknown parameter`.
   No in-tree deck uses that shape. The fix is in the same commit because
   the rewrite needs it.
4. **Duplicate exact `.model` cards in one scope now carry nothing**, even
   when they are identical (falsified item 2).

Nothing was pushed. Two commits on `ver_51`: `4e614b4da` (code and tests)
and this receipt.
