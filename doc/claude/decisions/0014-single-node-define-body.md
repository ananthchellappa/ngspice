# Decision 0014 — a `define` body that is a single node, `doc/codex/issues/0053`

## Status

Accepted, 2026-08-12, branch `ver_50`. Resolves
`doc/codex/issues/0053`. Unblocks `doc/codex/issues/0051` criterion 4. Files
`doc/codex/issues/0054`.

**Not a `casemode` decision.** Nothing here is about case, and nothing here
disturbs the classification `doc/claude/decisions/0013-user-defined-function-identity.md`
gave the four comparisons in `src/frontend/define.c`. The identity lint is
unmoved at **268 comparisons, baseline matches**, which is the check that says
so.

## Context

`define c(x) 5` at the prompt, then `print c(2)` twice, was a `SIGSEGV`. The
same three lines in a batch deck printed `5` and then `2` — the *argument* —
and exited 0.

### What was measured

Instrumented at `1eb641b24` with a `printf` on `pn_use` at the three frames the
node passes through, on `define c(x) 5` / `print c(2)` × 3:

```
PROBE trcopy:      return STORED value node 0x…58f0 name=5 pn_use=0
PROBE ft_substdef: name=c ud_text=0x…58f0 returns=0x…58f0 same=1 pn_use=0
PROBE PP_mkfnode:  q=0x…58f0 pn_use=0
PROBE free_pnode_x: FREEING 0x…58f0 pn_use=0 value=0x…7ea0 vname=c(2)
PROBE trcopy:      return STORED value node 0x…58f0 name=2 pn_use=0
```

Three things in that trace, only the first of which `0053` had read:

1. `ft_substdef()` returns `udf->ud_text` itself (`same=1`) with `pn_use == 0`,
   and `free_pnode_x()` frees it. On the second call `trcopy()` reads the
   freed node and finds `name=2` — the argument's dvec, in the reused block.
2. The node is freed with `vname=c(2)`, not `vname=5`. Between the two,
   `ft_evaluate()` (`src/frontend/evaluate.c:79-83`) had renamed the stored
   dvec after the enclosing command.
3. `parse-bison.y:131`, `one_exp: exp { $1->pn_name = copy_substring(…) }`,
   had written `pn_name` on to the same stored node.

### The invariant

`pn_use` is a count of *parent links*. `alloc_pnode()` sets it to 0 and every
constructor that links a child bumps it, so a root rests at 0.
`free_pnode_x()` decrements while `pn_use > 1` and frees at the floor. Sharing
is therefore safe exactly when some parent holds the count — which is what
`trcopy()`'s `pn_func` and `pn_op` branches do with `pn->pn_left->pn_use++`,
added by `cbdd811aa` in 2005 together with the counting in `free_pnode_x()`.

The `pn_value` branch returns the stored node and leaves the count to the
parent `trcopy()` is about to build. When the body's root **is** that value
node, there is no parent. That is the whole defect, and it is why `5+0` and `x`
were never affected: `5+0` has an operator at the root that takes the count,
and `x` is substituted by `ntharg()` rather than shared.

The stale comment above `trcopy()` — *"we never free parse trees, so we needn't
worry that they aren't trees here"* — dates from the 2000 initial revision and
its premise was retired by `cbdd811aa`. It was not the cause, but it is why the
branch reads as deliberate.

## Decision

**`ft_substdef()` returns a node of its own, over a copy of the vector, when
`trcopy()` hands back the stored root.** `copy_value_node()`, new and static in
`src/frontend/define.c`; the test is `tree == udf->ud_text`, which is exact —
`trcopy()`'s other branches allocate, and its formal branch returns a node from
the caller's argument list, so neither can be `ud_text`.

### Why not the reference count, which is what `0053` criterion 3 asked for

Criterion 3 says the fix should be a reference count and not a copy, and names
`ft_substdef()` (a) and `trcopy()` (b) as the candidates, with `PP_mkfnode()`
(c) worth reading. That is a preference, and the prompt for this session
allowed overruling it with a proof. **A reference count is not enough, and the
reason is measured, not argued.**

Candidate (a) was implemented and run, in full, before it was rejected:
`names->pn_use++` in `com_define()` to make `udfuncs`' own hold explicit, plus
`result->pn_use++` in `ft_substdef()` when the stored root is returned. It
produces the right number on every shape. It also leaves this:

```
define c(x) 5
define        ->  c (x) = 5
print c(2)    ->  c(2) = 5.000000e+00
define        ->  c (x) = c(2)          <- the definition, rewritten
```

and, on `define c(x) 5` with three calls, valgrind reports `10 bytes in 2
blocks are definitely lost` against the copy's 0 — one displaced `pn_name` per
call after the first, because `parse-bison.y` assigns over the field without
freeing what was there.

The cause is that **a parse tree's root is not read-only the way its interior
is**. `parse-bison.y:131` writes `pn_name` on to whatever node an expression
reduces to, and `ft_evaluate()` renames the dvec that node carries. A reference
count keeps the node alive; it cannot keep it unwritten. Sharing an *interior*
node is safe precisely because nothing writes to one — which is why the 2005
design works everywhere else and fails only here.

So the losers' costs:

- **(a) `ft_substdef()` bumps the stored root.** Narrow and correct as far as
  lifetime goes. Rejected because it blesses the sharing of a node the parser
  and the evaluator both write to: the definition is rewritten to `c (x) = c(2)`
  and a `pn_name` leaks per call. Measured above.
- **(b) `trcopy()`'s `pn_value` branch bumps before returning.** Everything in
  (a), plus it fires for every *interior* value node, where the parent already
  bumps — so it double-counts unless the three parent `pn_use++` lines are
  removed in the same edit. That is two changes wearing one hat, and it would
  rewrite the mechanism `cbdd811aa` installed rather than complete it.
- **(c) `PP_mkfnode()` bumps `q`.** Everything in (a), plus it bumps the fresh
  node `trcopy()` built for every operator-rooted body — a node the caller
  genuinely owns, whose count would never return to the floor. That trades the
  crash for a leak on the shapes that work today.

### What `copy_value_node()` copies

The vector, not just the node. A node over the *shared* dvec would fix the
crash and the `pn_name` leak and still let `ft_evaluate()` rename the
definition, so the copy has to reach the dvec.

`vec_copy()` then `vec_new()`, which is what `PP_mksnode()` does for a name and
`PP_mknnode()` for a constant: the vector goes to the current plot and is
garbage collected like any the parser makes. A zero-length dvec — a name the
body never resolved, `define c(x) nosuchvec` — is instead `dvec_alloc()`ed and
not given to a plot, which is exactly what `PP_mksnode()` leaves behind on a
miss. At `1eb641b24` that deck printed `trcopy: Internal Error: bad parse node`
and `Error: no such function as c`; it now reports `Warning from checkvalid:
vector nosuchvec is not available or has zero length.`

This is the contract the file's own header states — *"return what the parse
tree would have been had the entire thing been typed"*. For a single-node body
the tree the parser would have built is a fresh node over a fresh vector, and
that is now what it gets.

The cost is one `vec_copy()` per call of a single-node-bodied function.
Measured over 200→2000 calls in a `while` loop, peak RSS grows **128 kB** for
`define c(x) 5` against **260 kB** for `define c(x) 5+0` on the same range —
the copy path grows less than the operator path already did, because evaluating
an operator allocates a result vector per call anyway.

## Evidence

Seven shapes, each in its own process, `--batch`, no `casemode` set. `HEAD` is
`1eb641b24`. Valgrind was run with both binaries installed at the **same**
path, `build-ver_50/src/ngspice`, because a binary copied elsewhere resolves a
different `spinit` and loads a different set of code models — the first pass at
this comparison was wrong for exactly that reason.

| shape | HEAD | fixed |
| --- | --- | --- |
| `define c(x) 5` ×3 | `5, 2, 2` — 42 valgrind errors, `Invalid free()` | `5, 5, 5` — 2 errors |
| `let a = 5` / `define g(x) a` ×3 | `5, 1, 1` — 44 errors, `Invalid free()` | `5, 5, 5` — 4 errors |
| `define c(x) 5+0` ×3 | `5, 5, 5` — 2 errors | `5, 5, 5` — 2 errors, **identical** |
| `define c(x) x` ×3 | `2, 2, 2` — 1 error | `2, 2, 2` — 1 error, **identical** |
| `define sq(x) x*x` ×3 | `9, 9, 9` — 1 error | `9, 9, 9` — 1 error, **identical** |
| `define f(x) x*3` / `define g(y) f(y)+1` / `undefine f` / `g(5)` | `1.6e+01` — 4 errors | `1.6e+01` — 4 errors, **identical** |
| `define c(x) 5` / `print c(2)` / `undefine c` | `SIGSEGV`, exit 139, `Invalid free()` | exit 0 — 3 errors |

Every `Invalid read`, `Invalid write` and `Invalid free()` is gone. The four
shapes that were already correct are byte-identical, valgrind included, which
is the check that the parent's count was not disturbed.

The bytes that remain on the fixed side are all pre-existing, and each was
tied to a control deck that isolates it:

- **64 B per call of a one-argument function whose body ignores its formal.**
  `PP_mkfnode()` frees its `arg` only when it is a comma node; at arity 1
  nothing frees it, unless the body used the formal and a parent linked it.
  Present at `HEAD` on `define c(x) 5+0`, which this change does not touch:
  3 calls → `192 bytes in 3 blocks`, both before and after. Narrow —
  `define f(x) x*3`, `define f(x,y) x+y` and `print vm(1)` all lose 0, so no
  shipped function is exposed.
- **160 B on `undefine` of a value-rooted body.** `free_pnode()` on a root with
  `pn_use == 0` does not free the node's dvec. `define c(x) 5` / `undefine c`
  with **no call at all** loses `170 (160 direct, 10 indirect) bytes`
  identically at `HEAD` and after; `define c(x) 5+0` / `undefine c` loses 0.

The one shape where this change could have *introduced* a leak is the
zero-length branch, whose dvec is not given to a plot and so is not collected
with one. It does not, and the adversarial check is `define c(x) nosuchvec`
called 0, 1 and 20 times:

| calls | HEAD | fixed |
| --- | --- | --- |
| 0 | 0 lost, 1 error | 0 lost, 1 error, **identical** |
| 1 | 74 B in 2 blocks, 3 errors | 64 B in 1 block, 2 errors |
| 20 | 1,290 B in 21 blocks, **877 errors**, `Invalid free()` ×3 | 1,280 B in 20 blocks, 2 errors |

64 B × calls exactly, with no dvec among them — the per-call `pnode` of (b)
below and nothing else, the same figure the resolvable body gives. The vector
is reclaimed because `ft_evaluate()` returns a `pn_value` node's dvec to the
caller and the caller frees it, which is also why the copy of a *resolvable*
body does not accumulate in the plot. At `HEAD` the same twenty calls are 877
valgrind errors, because the dvec being freed there belonged to the definition.

Both remaining leaks are filed as `doc/codex/issues/0054`, with a third: `define d(x,y) x` —
arity ≥ 2 with a bare formal as the body — prints `checkvalid: Internal Error:
bad node` and reads freed memory, at `HEAD` and still after. One call exits 0,
two abort at 134. `ntharg()` returns the caller's
argument node and `PP_mkfnode()` then frees the comma node that holds its only
count. It is the same *family* — a node handed out with nobody counting it —
but a different site and a different owner, so it is a different commit.

## Consequences

- `tests/regression/misc/define-const-body.cir` asserts the batch face, which
  is a wrong number at exit 0. At `1eb641b24`:

  ```
  -c(2) = 5.000000e+00      +c(2) = 2.000000e+00
  -g(1) = 5.000000e+00      +g(1) = 1.000000e+00
  ```

- `tests/regression/misc/define-const-body-listing.cir` asserts that a call
  does not rewrite the definition. It is the deck that rejects candidate (a),
  which would pass everything else. At `1eb641b24` it is a `SIGSEGV` in
  `prtree1()` and the whole reference output is missing.
- `tests/regression/pipe/define-const-body.cmd` asserts the fatal face.
  **`tests/regression/pipe` can see a non-zero exit status**: its
  `TESTS_ENVIRONMENT` is `sh -c '… ngspice -p < $1'`, the shell exec's ngspice,
  and a fault arrives at automake as 139 — measured, not assumed.
- `make check`: **285 PASS, 0 FAIL**, against a 282 baseline plus these three.
  Every pre-existing directory is unchanged: `regression/case` 113,
  `casedist` 23, `misc` 27→29, `pipe` 16→17, `xspice/case` 21,
  `xspice/casedist` 17, `lint` 2.
- Identity lint **268 comparisons, baseline matches** — unmoved, as a change
  that adds no string comparison should be.
- The differential sweep is **`DIFF=62, OK=258, SKIP=3` of 323 decks**, with
  `NUM-DIFF` and `PARSE-FAIL` both 0. The DIFF *set* is identical before and
  after — no deck moved in either direction. Note that the sweep at
  `1eb641b24` **with these two decks removed**, which is the tree the
  `0053` prompt's baseline describes, is `321 decks: DIFF=62, OK=256, SKIP=3`
  and not the recorded `DIFF=61, OK=257`. The recorded figure is off by one,
  at `HEAD`, before this change; it is not a movement caused here.
- `deck_output_differ.py`, all three modes, `1eb641b24` → fixed:

  | mode | moved | which |
  | --- | --- | --- |
  | fold | 5 of 323 | `alter-rebin-param-case{,-lower}`, `binning-1`, + the 2 new decks |
  | preserve | 3 of 323 | `alter-rebin-param-case-lower`, + the 2 new decks |
  | distinguish | 4 of 323 | `alter-rebin-case-lower`, `alter-rebin-param-case-lower`, + the 2 new decks |

  Discounting the two new decks, which are supposed to move, that is fold 3,
  preserve 1, distinguish 2 against the recorded fold 3, preserve 4,
  distinguish 0 — every one of them an `alter-rebin`/`binning` deck flapping
  within the recorded ±2, and **no other deck moved in any mode**, which is
  what a change with no shipped single-node body to touch should do.
  `define-const-body-listing.cir` moves `rc -11 -> 0` in all three.
