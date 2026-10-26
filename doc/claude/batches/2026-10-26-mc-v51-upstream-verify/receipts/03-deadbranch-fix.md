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
inside `inp_collect_models()`. See section 6, which is added with the
upstream commit.
