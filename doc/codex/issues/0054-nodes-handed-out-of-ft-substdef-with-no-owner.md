# Issue: Three More Parse Nodes Handed Across `ft_substdef()` With Nobody Counting Them

## Status

Open. Found on 2026-08-12 while fixing `doc/codex/issues/0053`, which was the
fourth member of this family and is now closed. These three are the residue:
each was measured both before and after `0053`'s repair and is unchanged by it,
so none of them is a regression from it.

Class (a) is a fault and the other two are leaks.

## Summary

Measured against `build-ver_50/src/ngspice` at `1eb641b24` and again after
`0053`'s fix, `--batch`, no `casemode` set, each deck in its own process.

### (a) A body that is a bare formal, at arity ≥ 2 — `Internal Error`, exit 134

```spice
.control
op
define d(x,y) x
print d(2,3)
.endc
```

```
checkvalid: Internal Error: bad node
ft_evaluate: Internal Error: bad node
```

exit status **134**. At arity 1 the same shape — `define p(x) x` / `print p(2)`
— is correct, and `tests/regression/misc/define-const-body.cir` asserts that it
stays correct.

`trcopy()`'s formal-substitution branch returns `ntharg(i, args)`, a node from
the caller's argument list. At arity ≥ 2 that list is a comma tree, so the node
returned is `args->pn_left`, whose `pn_use` of 1 is held by the comma node —
and `PP_mkfnode()` (`src/frontend/parse.c:519-522`) then frees exactly that
comma node, `free_pnode()` walks into a child at the floor of its count, and
the node the caller is about to evaluate is freed underneath it.

This is the same shape of defect as `0053` — a node crosses `ft_substdef()`
with nobody holding a reference — but the node belongs to the *caller's
argument list* rather than to `udfuncs`, and the frame that frees it too early
is `PP_mkfnode()` rather than the evaluator. Different owner, different site,
so a different commit.

Unlike `0053` this one is loud: it prints two `Internal Error` lines and aborts.

### (b) 64 B per call of any user-defined function

`PP_mkfnode()` frees its `arg` only when `arg` is a comma node. An arity-1 call
whose body does not substitute the formal never frees the argument `pnode`.

```
define c(x) 5+0     3 calls  ->  definitely lost: 192 bytes in 3 blocks
```

64 B is `sizeof(struct pnode)`. Identical at `1eb641b24` and after `0053`'s
fix, on a shape `0053` does not touch, which is what identifies it as
pre-existing. It scales: 2000 calls lose `128,000 bytes in 2,000 blocks`. The
argument's dvec is not lost with it — `PP_mknnode()` gave it to the current
plot.

### (c) 160 B + 10 B per `undefine` of a single-node body

```spice
define c(x) 5
undefine c          * no call in between
```

```
170 (160 direct, 10 indirect) bytes in 1 blocks are definitely lost
```

`free_pnode_x()` frees a node's `pn_value` only when `pn_use == 1`. A stored
body's root rests at 0, so `undefine`'s `free_pnode(udf->ud_text)` frees the
node and abandons the dvec `savetree()` copied for it. Identical before and
after `0053`.

## Impact

(a) is an abort on a plausible line — a two-argument helper that returns one of
its arguments, `define pick(a,b) a`, is not an adversarial construction — and
it is loud, so it costs a user a session rather than a wrong answer. It is
strictly less severe than `0053` was, which was silent.

(b) and (c) are small and bounded per event, but (b) is per *call*, so a
control-language loop over a user-defined function leaks without limit: 64 B ×
iterations, on every shipped function including `vm()` and `min()`.

## Root Cause

All three are the same missing question, asked at three different frames: *who
owns this node after it leaves here?*

- (a) `ntharg()`'s return is owned by the caller's argument list, and
  `PP_mkfnode()` frees that list.
- (b) `PP_mkfnode()` frees `arg` on one branch only, so the other branches
  abandon it.
- (c) `free_pnode_x()`'s `pn_use == 1` guard on `vec_free()` means a root's
  vector is never freed, and `undefine` is the one caller that frees a root
  that owns its vector.

`doc/claude/decisions/0014-single-node-define-body.md` states the invariant
these violate: `pn_use` counts parent links, `free_pnode_x()` frees at the
floor, and a node may only be shared when some parent holds the count.

## Acceptance Criteria

1. `define d(x,y) x` / `print d(2,3)` returns `2`, three times, exit 0, with no
   valgrind error. The arity-1 shape stays correct.
2. `define c(x) 5+0` called N times loses no more than it loses at N = 1.
3. `define c(x) 5` / `undefine c` reports no `definitely lost` attributable to
   the definition.
4. A deck asserts (a) where `make check` can see it. Its face is an exit status
   and two `Internal Error` lines, so either `tests/regression/misc/` on the
   truncated output or `tests/regression/pipe/` on the status will do; prefer
   `misc/`, beside `define-const-body.cir`, since nothing here is about case.
5. `make check` unchanged — 285 PASS / 0 FAIL — and the differential sweep's
   `NUM-DIFF` and `PARSE-FAIL` stay 0.
6. Sequenced against `doc/codex/issues/0051`: (b) is in `PP_mkfnode()` and
   `0051` criterion 4 is in `com_define()`, so they do not collide, but both
   free parse trees and should be measured together.

## Resolution

Open.
