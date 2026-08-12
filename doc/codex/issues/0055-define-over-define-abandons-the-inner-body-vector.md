# Issue: Defining a Function Over Another One Abandons the Inner Body's Saved Vector

## Status

Open. Found on 2026-08-12 while fixing `doc/codex/issues/0054`, whose chain
control deck — `define f(x) x*3` / `define g(y) f(y)+1` / `undefine f` /
`g(5)` — was the one shape of eleven still losing bytes after that repair.
Measured identically at `6ac4dc733` and after, so it is neither caused nor
fixed by `0054`, and it is why that deck's `230 bytes in 3 blocks` is
unchanged in decision 0015's Evidence table.

The fifth member of the family `doc/codex/issues/0053` opened, and the first
of them that is about `savetree()` rather than `pn_use`.

## Summary

Measured against `build-ver_50/src/ngspice`, `--batch`, no `casemode` set,
each deck in its own process, `valgrind --leak-check=full`.

**A `define` whose body calls another user-defined function loses 160 B for
every non-formal value node in the inner function's stored body.** No call is
needed — the loss is at *definition* time.

| deck | lost |
| --- | --- |
| `define g(y) y*3+1` | 70 B in 2 blocks |
| `define f(x) x` + `define g(y) f(y)+1` | 70 B in 2 blocks |
| `define f(x) x*x` + `define g(y) f(y)+1` | 70 B in 2 blocks |
| `define f(x) x*3` + `define g(y) f(y)+1` | **230 B in 3 blocks** |
| `define f(x) x+0` + `define g(y) f(y)+1` | **230 B in 3 blocks** |
| `define f(x) x*3` + `define g(y) f(y)` | **230 B in 3 blocks** |
| `define f(x) x*3` + `define g(y) f(y)+1` + `define h(z) f(z)+2` | **390 B in 4 blocks** |

Subtracting the 70 B residue below, that is exactly **160 B per outer
definition**, once per non-formal value node in the inner body: `f(x) x*3` and
`f(x) x+0` each have one such node and cost 160; `f(x) x` and `f(x) x*x` have
none and cost nothing; a third definition over the same `f` costs 160 again.
160 B is `sizeof(struct dvec)`.

`undefine` does not reclaim it: the same deck with `undefine f` and
`undefine g` appended loses the same 230 B.

### The 70 B residue, separately

A run that uses `define` at all loses a fixed `70 bytes in 2 blocks`,
independent of how many functions it defines — 1, 2 and 3 definitions all
cost 70 — and `undefine` does not reclaim it. A deck with no user `define`
(`let q = 1`) loses 0. It is **not** attributed here; it is recorded so the
figures above can be read, and so a session that fixes the 160 B does not
mistake the remainder for a regression.

## Impact

Small and bounded per event, and it needs a control deck that defines helpers
in terms of other helpers — a real style, but not a loop. Nothing scales it:
the loss is per `define` card, and decks do not define functions in a `while`.

Its second effect is the more interesting one and has no measured cost yet:
`savetree()` **writes to a node another `udfunc`'s stored body owns**. After
`define g(y) f(y)+1`, the `3` node inside `f`'s stored body carries the vector
`g`'s `savetree()` allocated for it, not the one `f`'s own `savetree()` made.
The contents and the name are equal, so no number moves and `define f` still
lists `f (x) = (x)*(3)`, but stored state that every other frame in
`src/frontend/define.c` treats as frozen is being mutated by a later,
unrelated command. That is the same class of assumption
`doc/claude/decisions/0014-single-node-define-body.md` and `0015` were about.

## Root Cause

Confirmed by instrumentation, not by reading. A `printf` in `savetree()` on
`define f(x) x*3` then `define g(y) f(y)+1`:

```
PROBE savetree node=0x…5080 pn_use=1 dvec=0x…1b60 name=3 len=1 plot=0x…1110
PROBE savetree node=0x…5080 pn_use=2 dvec=0x…1c10 name=3 len=1 plot=(nil)
```

The **same node**, `0x…5080`, is visited twice.

1. `define f(x) x*3` parses `3` into a node whose dvec `PP_mknnode()` gave to
   the current plot (`plot=0x…1110`). `savetree()` replaces it with a
   `dvec_alloc()`ed copy deliberately kept out of any plot, so garbage
   collection cannot take it — that is the function's whole purpose.
2. `define g(y) f(y)+1` expands `f(y)` through `ft_substdef()`. `trcopy()`
   *shares* interior value nodes and leaves the count to the parent it builds,
   which is correct and is what `0015` relies on — so `g`'s tree contains
   `f`'s `3` node, at `pn_use=2`.
3. `savetree()` then walks `g`'s tree, reaches that shared node, and
   overwrites `pn_value` again. The copy step 1 made is in no plot
   (`plot=(nil)` on the second line is the *new* one; the old one is likewise
   plot-less), so nothing owns it and nothing collects it. 160 B + 10 B
   indirect, definitely lost.

`savetree()` has no test for a node it has already saved, and no test for a
node it does not own. It was written when a stored tree could only have come
from one `define` card; `ft_substdef()` returning a tree that shares the
interior of another stored body is what makes that untrue, and that sharing is
load-bearing — removing it would cost a deep copy per expansion.

## Acceptance Criteria

1. `define f(x) x*3` / `define g(y) f(y)+1` loses no more than
   `define g(y) y*3+1` loses, at `definitely lost`.
2. It stays true for three definitions over the same `f`, and after
   `undefine f` / `undefine g` in either order — including the order that
   frees the *inner* function first while the outer body still shares its
   nodes. `free_pnode_x()` decrements a shared node rather than freeing it, so
   that order is already safe today; a repair must not change it.
3. `savetree()` does not overwrite a `pn_value` it did not allocate. Stating
   the rule that way, rather than "does not visit a node twice", is
   deliberate: the sharing itself is correct.
4. The eleven shapes in `doc/claude/decisions/0015-argument-list-ownership.md`
   are unmoved, valgrind included — in particular `define sq(x) x*x` and
   `print vm(1)` at 0 bytes, which is where a `savetree()` change would show
   up first.
5. `make check` unchanged at 286 PASS / 0 FAIL, identity lint unmoved at 268,
   and the sweep's `NUM-DIFF` and `PARSE-FAIL` still 0.
6. Whether the 70 B residue above is the same defect or another one is
   answered, and if another, filed separately rather than folded in.

## Resolution

Open.
