# 03: A typeless .model in a dead .if branch stops a deck with an R->B rewrite

Date: 2026-10-26. Branch: `ver_51`. Build tree: `build-ver_50/`.

## 1. The defect

An independent check of the upstream series found it, and the fork has the
same code. In a deck with `.if`, `inp_readall()` builds the `.model` lists
with `inp_collect_models()` for the R->B rewrite when some R that becomes a B
source names a `.model`. That happens before `dotifeval()` decides the
branches. `inp_collect_models()` calls `inp_chk_model_type()` on every
`.model` card, and that function exits. So a typeless card in a branch that
is not taken stopped a valid deck with "no model type given" and 1:

```
.model rm r tc1=-2.47e-3 tnom=25
.if (0)
.model bad
.endif
i1 0 1 1u
r1 1 0 rm r={1k+0*v(1)}
```

`model-typeless-if-dead.cir` did not catch this, because it has no R that is
rewritten, so the lists are never built before `.if` is resolved.

## 2. RED

Two new `check_status.sh` decks in `tests/regression/exitstatus/`, next to
the other typeless-model decks (the fork has none in `tests/regression/model`):

- `model-typeless-if-dead-behav.cir`: the deck above. Status 0, empty `.err`.
- `model-typeless-if-live-behav.cir`: the same with `.if (1)`. Status 1, and
  the `.err` holds the diagnostic. This is the guard that the fix does not
  lose the error. It passes before and after.

Before the fix (`make -C build-ver_50/tests/regression/exitstatus check
TESTS="model-typeless-if-dead-behav.cir model-typeless-if-live-behav.cir"`):

```
check_status: model-typeless-if-dead-behav: FAILED (status)
  expected exit status 0, got 1
  invocation: ../../../src/ngspice --batch -n ../../../../tests/regression/exitstatus/model-typeless-if-dead-behav.cir
  stderr was:
    Error: no model type given in line .model bad!
        line no. 12 from file ../../../../tests/regression/exitstatus/model-typeless-if-dead-behav.cir!
    ERROR: fatal error in ngspice, exit(1)
--- model-typeless-if-dead-behav.stderr.expected
+++ model-typeless-if-dead-behav.stderr.filtered
@@ -0,0 +1,4 @@
+Error: no model type given in line .model bad!
+    line no. 12 from file model-typeless-if-dead-behav.cir!
+
+ERROR: fatal error in ngspice, exit(1)
check_status: model-typeless-if-dead-behav: FAILED: status err
FAIL: model-typeless-if-dead-behav.cir
check_status: model-typeless-if-live-behav: exit status 1 as expected
PASS: model-typeless-if-live-behav.cir
1 of 2 tests failed
```

It fails for the expected reason: the exit comes from the collection pass,
for a card that is not part of the circuit.

## 3. The fix

`src/frontend/inpcom.c`, `inp_collect_models()`: when `has_if` is set and
`get_model_type()` returns NULL, the card is skipped (`continue`) before
`inp_chk_model_type()`. It is not added to the lists, so no consumer sees a
NULL type. `inp_chk_model_types()`, which `inp_spsource()` runs right after
`dotifeval()`, still reports the card if its branch is taken. Without `.if`,
nothing changes: `inp_rem_unused_models()` still exits on the card. The
comment above `inp_chk_model_types()` now says the collection pass skips the
check in a deck with `.if`.

## 4. GREEN

After `ngheavy make -j4` and `ngheavy make install`:

```
check_status: model-typeless-if-dead-behav: exit status 0 as expected
PASS: model-typeless-if-dead-behav.cir
check_status: model-typeless-if-live-behav: exit status 1 as expected
PASS: model-typeless-if-live-behav.cir
```

- `tests/regression/exitstatus`: all 23 tests passed.
- `tests/regression/temper`: all 8 passed.
- `tests/regression/model`: all 3 passed.
- Full `ngheavy make -j4 check`: exit 0, 364 PASS, 0 FAIL, including
  `identity.lint` and `selftest.lint`.

The new `if` makes no string comparison, so the identity lint has nothing to
report, and `identity.baseline` is unchanged.

## 5. Upstream

The same fix goes into `p4-crash/on-p1`, the patch that adds the check
inside `inp_collect_models()`. See section 6.

## 6. Upstream: `p4-crash/on-p1` amended

Scratch clone of `https://git.code.sf.net/p/ngspice/ngspice`, tip
`a1148c969` (`pre-master-48`), with the local `tests/xspice/analog` revert
from `01-verify.md` (not committed). Built out of tree with plain
`../configure`, `ngheavy make -j4`. The clone, its worktrees and builds
were deleted at the end.

**Change.** `git am` of `p1` and `p4-crash/on-p1`, then the commit was
amended: the same three-line skip and comment in `inp_collect_models()` as
the fork, the reworded comment above `inp_chk_model_types()`, and a new
`tests/regression/model/model-typeless-if-dead-behav.cir` + `.out`
(check.sh; the deck of section 1 with `print` replaced by a check that
v(1) is 0.99506 mV, so it also asserts that `rm` is carried). Message
changes, so every claim is true:
- "factored out of inp_collect_models()" (it said
  `inp_rem_unused_models()`, which is true only of the version without
  `p1`);
- a new Fix bullet for the skip;
- "Tests" lists both decks;
- the recheck paragraph says both decks crash (exit 139, 3 of 3) without
  the fix and pass with it;
- the last paragraph says this version adds the skip and its test, which
  only a tree with `p1` needs.

`p2` was then applied with `git am`. Regenerated, `p1` and `p2` differ from
the committed files only in the `From <sha>` line, so only the `on-p1`
`.patch` was overwritten.

**RED on the upstream tree** (`p1` + the old `on-p1`):

```
Error: no model type given in line .model bad!
    line no. 14 from file ../../../../tests/regression/model/model-typeless-if-dead-behav.cir!

ERROR: fatal error in ngspice, exit(1)
--- model-typeless-if-dead-behav.out_tmp
+++ model-typeless-if-dead-behav.test_tmp
@@ -1 +1,2 @@
-INFO: success
+
+
FAIL: model-typeless-if-dead-behav.cir
PASS: model-typeless-if-dead.cir
```

**Before and after**, the new deck:

| tree | result |
| --- | --- |
| ngspice-45.2 (`/usr/bin/ngspice`) | exit 139 |
| `a1148c969` unpatched | exit 139, 3 of 3; check.sh FAIL |
| `a1148c969` + `p1` + old `on-p1` | exit 1, "no model type given"; FAIL |
| `a1148c969` + `p1` + new `on-p1` | exit 0, 3 of 3; check.sh PASS |

The `.if (1)` variant exits 1 with the error on the patched tree (139
unpatched). `model-typeless-if-dead.cir` is 139, 3 of 3, unpatched.

**Full make check**, `a1148c969` + `p1`, new `on-p1`, `p2`: exit 0,
63 PASS, 0 FAIL (62 before, plus the new deck).

**Apply check.** `git am` of the three committed `.patch` files on a fresh
worktree of `a1148c969` applies with no fuzz, and its tree is identical to
the built one. Standalone `p1`, `p4-crash` and `p2` still pass
`git apply --check` on the tip.

**Numbers re-measured for the drafts** (45.2 / unpatched tip / patched):
- p1 deck at 125 C, v(a): 7.300000e-02 / 7.300000e-02 / 5.694000e-02;
  v(b) 5.694000e-02 everywhere.
- p4 deck: `.if (0)` 139 / 139 / v(1) = 1.000000e-03, exit 0; `.if (1)`
  139 / 139 / error, exit 1.
- section 1's deck: 139 / 139 / v(1) = 9.950600e-04, exit 0.
- seed=5: 9.312134e+02 / 7.974070e+02 / 7.974070e+02 on both samples; the
  override warning 0 / 0 / 1 time.
- seed=abc, `setseed 3 / reset / op` three times: 8.392277e+02 /
  8.432359e+02 / 8.432359e+02; "Cannot convert" 4 / 4 / 1.
- Two `.option seed=5` lines, the same loop: "Multiple" 4 / 4 / 1. A
  single `seed=abc` line prints no "Multiple" at all, because
  `eval_opt()` counts only seeds it could use, so the draft's "Multiple
  goes from 4 to 1" belongs to this deck, not to the seed=abc one.
- p1 tests on the unpatched tip: maxerr 0.314112, 0.144911, 0.156474,
  0.282051.
