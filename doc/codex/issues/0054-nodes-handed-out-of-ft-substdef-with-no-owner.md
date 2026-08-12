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

### (a) A body that is a bare formal, at arity ≥ 2 — use-after-free, then abort

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

It has the same two faces `0053` had, and the call count is what selects
between them:

| calls | `Internal Error` lines | `SURVIVED` | exit | valgrind |
| --- | --- | --- | --- | --- |
| 1 | 2 | printed | **0** | `Invalid write`, `Invalid read` — 0 bytes lost |
| 2 | 2 | never printed | **134** | — |
| 3 | 2 | never printed | **134** | 67 errors, 22 `Invalid read`/`write`, 0 bytes lost |

So a single call is a use-after-free that a batch deck survives with status 0,
and the second call aborts in glibc. The value is never produced at all — the
node is gone before `ft_evaluate()` reaches it — so unlike `0053` this one
prints no wrong number, only the two `Internal Error` lines.

Mode-independent: exit 0 on a one-call deck under `fold`, `preserve` and
`distinguish` alike, which is expected — nothing here is about case.

At arity 1 the same shape — `define p(x) x` / `print p(2)` — is correct, and
`tests/regression/misc/define-const-body.cir` asserts that it stays correct.
`define d(x,y) x+0`, an operator over the formal, is correct twice over
(`d(2,3) = 2.000000e+00`), which localises it to the bare-formal return.

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

### (b) 64 B per call of a one-argument function whose body ignores its formal

`PP_mkfnode()` frees its `arg` only when `arg` is a comma node. At arity 1 the
argument is not a comma node, so nothing frees it — unless the body used the
formal, in which case `ntharg()` handed that same node to `trcopy()`, a parent
linked it, and it is freed with the result tree.

Measured, three calls each:

| definition | call | lost |
| --- | --- | --- |
| `define f(x) x*3` | `f(2)` | 0 |
| `define f(x) x*x` | `f(2)` | 0 |
| `define f(x) 5+0` | `f(2)` | **192 bytes in 3 blocks** |
| `define f(x) 5` | `f(2)` | **192 bytes in 3 blocks** |
| `define f(x,y) x+y` | `f(2,3)` | 0 |
| `define f(x,y) x+0` | `f(2,3)` | 0 |
| `define f(x,y) 5+0` | `f(2,3)` | 0 |

64 B is `sizeof(struct pnode)`. So the leak needs **arity exactly 1** and a body
that never mentions the parameter, and at arity ≥ 2 the comma node's own free
collects the unused arguments — which is the same free that causes (a).

**No shipped function is exposed**: `vm`, `vp`, `vdb`, `vr`, `vi`, `vg`, `gd`,
`min` and `max` all use their formals. `print vm(1)` three times loses 0 bytes,
measured.

Identical at `1eb641b24` and after `0053`'s fix, on a shape `0053` does not
touch, which is what identifies it as pre-existing. It scales: 2000 calls of
`define c(x) 5` lose `128,000 bytes in 2,000 blocks`. The argument's dvec is
not lost with it — `PP_mknnode()` gave it to the current plot.

### (c) 160 B per `undefine` of a body whose root is a value node

```spice
define c(x) 5
undefine c          * no call in between
```

```
170 (160 direct, 10 indirect) bytes in 1 blocks are definitely lost
```

`free_pnode_x()` frees a node's `pn_value` only when `pn_use == 1`. A stored
body's root rests at 0, so `undefine`'s `free_pnode(udf->ud_text)` frees the
node and abandons the dvec `savetree()` copied for it.

It needs a **value node at the root**, which is the same shape `0053` was
about, so an operator root is unaffected. Measured, `undefine` with no call in
between:

| definition | lost |
| --- | --- |
| `define c(x) 5` | 160 B + 10 B indirect |
| `define c(x) x` | 160 B + 2 B indirect |
| `define c(x) 5+0` | 0 |

Identical before and after `0053`.

## Impact

(a) is an abort on a plausible line — a two-argument helper that returns one of
its arguments, `define pick(a,b) a`, is not an adversarial construction — and
it is loud, so it costs a user a session rather than a wrong answer. It is
strictly less severe than `0053` was, which was silent.

(b) and (c) are small and bounded per event, but (b) is per *call*, so a
control-language loop over a constant-bodied helper leaks without limit —
64 B × iterations, `128,000 bytes` over 2000. It is narrow: arity 1 and a body
that ignores its parameter, so no shipped function reaches it. (c) is once per
`undefine`, which is not a loop, and is the smallest of the three.

## Root Cause

All three are the same missing question, asked at three different frames: *who
owns this node after it leaves here?*

- (a) `ntharg()`'s return is owned by the caller's argument list, and
  `PP_mkfnode()` frees that list.
- (b) `PP_mkfnode()` frees `arg` on one branch only, so the other branches
  abandon it.

(a) and (b) are the same missing rule seen from both sides: freeing the comma
node is what destroys a *used* argument at arity ≥ 2, and it is also what
collects an *unused* one, which is why (b) needs arity 1 and (a) needs arity
≥ 2. Neither can be fixed by deleting or adding that free alone.
- (c) `free_pnode_x()`'s `pn_use == 1` guard on `vec_free()` means a root's
  vector is never freed, and `undefine` is the one caller that frees a root
  that owns its vector.

`doc/claude/decisions/0014-single-node-define-body.md` states the invariant
these violate: `pn_use` counts parent links, `free_pnode_x()` frees at the
floor, and a node may only be shared when some parent holds the count.

## Acceptance Criteria

1. `define d(x,y) x` / `print d(2,3)` returns `2`, three times, exit 0, with no
   valgrind error. The arity-1 shape stays correct. Note that a one-call deck
   already exits 0 today, so a deck asserting only the status is a vacuous
   test — it has to assert the value, or the absence of the `Internal Error`
   lines, or call twice.
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
