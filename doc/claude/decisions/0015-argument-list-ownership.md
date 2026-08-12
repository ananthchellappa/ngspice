# Decision 0015 — who owns a user-defined function's argument list, `doc/codex/issues/0054`

## Status

Accepted, 2026-08-12, branch `ver_50`. Resolves `doc/codex/issues/0054`, all
three classes. Files `doc/codex/issues/0055`. Completes the family
`doc/claude/decisions/0014-single-node-define-body.md` opened.

**Not a `casemode` decision.** Class (a) was measured identical under `fold`,
`preserve` and `distinguish`, and nothing here is about case. The identity
lint is unmoved at **268 comparisons, baseline matches**, which is the check
that says so.

## Context

Three defects, all in the two functions `0053` had open, all measured both
before and after `0053`'s repair and unchanged by it:

- **(a)** `define d(x,y) x` / `print d(2,3)` printed no value and two
  `Internal Error: bad node` lines. One call exited **0**; two aborted at
  **134**.
- **(b)** `define f(x) 5` leaked **64 B per call** — `128,000 bytes in 2,000
  blocks` over 2000 calls.
- **(c)** `define c(x) 5` / `undefine c`, no call between, lost
  `170 (160 direct, 10 indirect) bytes`.

### What was measured

Instrumented at `6ac4dc733` with a `printf` on `pn_use` at the four frames a
substituted node crosses, on `define d(x,y) x` / `print d(2,3)`:

```
PROBE trcopy FORMAL x -> node 0x…dedb0 pn_use=1 args=0x…decd0 args_is_comma=1 args_left=0x…dedb0
PROBE ft_substdef name=d arity=2 returns=0x…dedb0 pn_use=1 args_pn_use=0
PROBE PP_mkfnode  q=0x…dedb0 q_pn_use=1 arg=0x…decd0 arg_is_comma=1 arg_left=0x…dedb0
PROBE free_pnode_x t=0x…decd0 pn_use=0 value=(nil)          act=FREE   <- the comma node
PROBE free_pnode_x t=0x…dedb0 pn_use=1 value=0x…1850 vname=2 act=FREE  <- q, with its dvec
PROBE free_pnode_x t=0x…ded40 pn_use=1 value=0x…1900 vname=3 act=FREE
checkvalid: Internal Error: bad node
ft_evaluate: Internal Error: bad node
PROBE free_pnode_x t=0x…dedb0 pn_use=1 value=(nil)          act=FREE   <- q again
```

The inference `0054` recorded was that `ntharg()` returns `args->pn_left` and
`PP_mkfnode()` frees the comma node that holds its only count. That is
confirmed, and the trace adds two things the reading did not have: the node's
**dvec** is freed with it, because `pn_use == 1` crosses `free_pnode_x()`'s
second threshold; and the node is then freed a **second** time by the caller,
which is why glibc says `double free or corruption (fasttop)` rather than
merely returning rubble.

The other two, on the same instrumentation:

```
(b)  PP_mkfnode arg=0x…5880 pn_use=0 arg_is_comma=0      <- never appears again
(c)  free_pnode_x t=0x…dd7a0 pn_use=0 value=0x…06b0 vname=5 act=FREE
```

(b)'s argument node is reached by no `free_pnode_x()` call at all. (c)'s
stored root is freed at `pn_use == 0`, below the `vec_free()` guard.

### The invariant

`0014`'s, unchanged: `pn_use` counts *parent links*, `alloc_pnode()` sets 0,
every constructor that links a child bumps it, `free_pnode_x()` decrements
while `> 1` and frees at the floor. Two thresholds, not one — the node is
freed at 0 **or** 1, and its `pn_value` only at 1.

That second threshold is the design's statement about roots: **a root's vector
is not the tree's to free**, because `ft_evaluate()` hands a `pn_value` node's
dvec to its caller. It is correct, and it is why (c) cannot be repaired inside
`free_pnode_x()`.

## Decision

**`PP_mkfnode()` owns the argument list, and `ft_substdef()` returns a tree
whose root is nobody else's.** One rule, two halves, neither sufficient alone:

1. `PP_mkfnode()` takes a reference on `arg` and frees it on the substitution
   path at every arity, not only when `arg` is a comma node.
2. `ft_substdef()` copies the result root when `trcopy()` substituted an
   argument into the root position — `node_of_arglist()` and
   `copy_arg_root()`, new and static in `src/frontend/define.c`.

The interior needs no copy: `trcopy()`'s parents already bump what they link,
so half 1's release decrements those nodes rather than freeing them. Only the
root is special, and for the reason `0014` found — a root is written to after
it is handed out, by `parse-bison.y:131`'s `pn_name` and by `ft_evaluate()`'s
rename of the vector it carries — and, additionally here, because a root is
owned *implicitly*, at `pn_use == 0`, so two owners cannot be expressed for
one. `copy_arg_root()` duplicates the node and shares the subtree beneath it
under a count; for a value root it delegates to `0053`'s `copy_value_node()`,
which is the same operation one level down.

Class (c) is the same sentence at a third frame: **`com_define()` takes the
reference, because `udfuncs` is a parent too.** `names->pn_use++`. The stored
root then reaches `com_undefine()`'s `free_pnode()` at the count of a node one
owner holds, and its vector is released.

### Why (a) and (b) cannot be two commits

They are one free seen from both sides, and every candidate that treats them
separately was **built and run** rather than argued about. Each row is the
eleven-shape set, each deck in its own process, `--batch`, no `casemode`:

| candidate | (a) | (b) | what it costs |
| --- | --- | --- | --- |
| **L1** delete `free_pnode(arg)` | fixed | **unfixed** | leaks 64 B per call at *every* arity: `d(x,y) x` ×3 → 192 B, `d(x,y) x+0` ×2 → 128 B, `t(x,y,z) y` ×1 → 64 B, all 0 today |
| **L2** unconditional `free_pnode(arg)`, no count | unfixed | fixed | `p(x) x` → **134**, `sq(x) x*x` → **SIGSEGV 139**, `print vm(1)` → **SIGSEGV 139**, and the `0053` chain deck prints `4` where `16` is right |
| **L3** = candidate (i), `trcopy()` bumps `ntharg()`'s return | fixed | **unfixed** | leaks on every shape that *uses* its formals: `d(x,y) x+0` ×2 → 128 B, `sq(x) x*x` ×3 → 128 B (that shape leaks 64×(N−1), not 64×N: measured 0, 64, 128, 192 at N = 1, 2, 3, 4), **`print vm(1)` ×3 → 192 B**, all 0 today |
| **L4** half 1 without half 2 | **unfixed** | fixed | `p(x) x` → **134**, newly broken |
| **chosen** = half 1 + half 2 | fixed | fixed | see Evidence |

L2 and L3 are the decisive ones. L2 segfaults `print vm(1)` — the release
without the count destroys an argument the result tree kept. L3 leaks on
`print vm(1)` — the count without the release over-counts, and since `vm`,
`vp`, `vdb`, `vr`, `vi`, `vg`, `gd`, `min` and `max` all use their formals,
candidate (i) would have leaked on every call of every function ngspice
ships. `0054` named no preferred candidate; this is why.

L4 is the measurement that says the two halves are one rule: taking the
reference and releasing it fixes (b) outright and *newly aborts* `define p(x) x`,
because at arity 1 the argument is also the result root.

### The arithmetic, for each shape

`arg->pn_use++` before the release is what makes it exact. Writing *h* for
half 1's reference and *p* for a parent link in the result tree:

| shape | counts | at the release | at the caller's free |
| --- | --- | --- | --- |
| `vm(1)`, `f(x) x*3` | h + p = 2 | 2 > 1 → decrement to 1 | 1 → node **and** dvec freed |
| `f(x) 5` (b) | h = 1 | 1 → node and dvec freed | copy is independent |
| `sq(x) x*x` | h + p + p = 3 | 3 > 1 → 2 | 2 → 1 → freed once |
| `p(x) x` | h = 1, root copied | 1 → node and dvec freed | copy, root at 0 |
| `d(x,y) x+0` | comma h = 1; child p+comma = 2 | comma freed, child 2 → 1, unused arg 1 → freed | child 1 → freed |
| `d(x,y) x` (a) | comma h = 1, root copied | comma freed, both args 1 → freed | copy, root at 0 |

## Evidence

Eleven shapes -- the acceptance table's ten plus `define d(x,y) x` ×2 -- each in its own process
because class (a) corrupts the heap and a second probe after it measures
rubble. `HEAD` is `6ac4dc733`. Both binaries were run from the **same** path,
`build-ver_50/src/ngspice`, because a binary copied elsewhere resolves a
different `spinit`.

| shape | HEAD | fixed |
| --- | --- | --- |
| `define d(x,y) x` ×1 | no value, 2 `Internal Error`, exit 0 — 23 errors, `Invalid free()` | `2` — 1 error, 0 lost |
| `define d(x,y) x` ×2 | exit **134** | `2, 2` — 1 error, 0 lost |
| `define d(x,y) x` ×3 | exit **134**, 67 errors | `2, 2, 2` — 1 error, 0 lost |
| `define t(x,y,z) y` ×1 | no value, exit 0 — 23 errors | `3` — 1 error, 0 lost |
| `define d(x,y) x+0` ×2 | `2, 2` — 1 error, 0 lost | **identical** |
| `define p(x) x` ×3 | `2, 2, 2` — 1 error, 0 lost | **identical** |
| `define sq(x) x*x` ×3 | `9, 9, 9` — 1 error, 0 lost | **identical** |
| `define f(x) 5+0` ×3 | `5, 5, 5` — **192 B in 3 blocks** | `5, 5, 5` — 1 error, **0 lost** |
| `define c(x) 5` / `undefine c` | **170 (160 direct, 10 indirect) B** | 1 error, **0 lost** |
| `print vm(1)` ×3 | `1, 1, 1` — 1 error, 0 lost | **identical** |
| `define f(x) x*3` / `define g(y) f(y)+1` / `undefine f` / `g(5)` | `1.6e+01` — 230 B in 3 blocks | `1.6e+01` — **identical**, see `0055` |

Every `Invalid read`, `Invalid write` and `Invalid free()` is gone. The four
shapes that were already correct are byte-identical, valgrind included, which
is the check that no parent's count was disturbed.

At scale, 2000 events in a `while` loop:

| deck | HEAD | fixed |
| --- | --- | --- |
| `define c(x) 5` called ×2000 | `128,000 bytes in 2,000 blocks`, RSS 15,084 kB | **0 lost**, RSS 15,088 kB |
| `define d(x,y) x` called ×2000 | exit **134** | exit 0, 0 lost, RSS 15,092 kB |
| `define c(x) 5` / `undefine c` ×2000 | `319,840 bytes in 1,999 blocks`, RSS 15,312 kB | **0 lost**, RSS 15,060 kB |

The class (c) row is **path sensitive**, which is worth knowing before it is
re-measured: the same pre-fix binary run from a scratch directory instead of
`build-ver_50/src/ngspice` reports `320,000 bytes in 2,000 blocks`, one dvec
more, because it resolves a different `spinit`. Both figures are stable across
three runs; the one tabulated is the sanctioned path. The class (b) row is not
sensitive -- 128,000 either way.

The per-call leak is gone at no measurable RSS cost — the copy is one node,
and for a value root one `vec_copy()`, against an argument evaluation that
already allocates.

**Class (c) scales, which `0054` did not expect.** Its Impact section reads
"(c) is once per `undefine`, which is not a loop, and is the smallest of the
three". A control-language loop that redefines a helper *is* a loop over
`undefine` — `com_define()`'s replacement path reaches the same free once
`doc/codex/issues/0051` criterion 4 lands — and 2000 cycles lose 320 kB, more
than either of the other two classes at the same count. It is the largest of
the three at scale, not the smallest. The repair recovers 252 kB of peak RSS
there rather than costing any.

### The adversarial check the new rule needed

Half 1 newly crosses `free_pnode_x()`'s **second** threshold at arity 1: an
argument the body ignored now reaches the release at `pn_use == 1`, so its
dvec is freed where before it lingered. The question is whether that dvec can
be one the user owns or one `ft_evaluate()` already handed out. It cannot —
`PP_mknnode()` and `PP_mksnode()` both `vec_copy()`/`vec_new()` a *fresh*
vector for the argument node, and a body that ignores its formal never
evaluates it. Measured, with named vectors and node voltages as arguments:

```
let a = 5 / let b = vector(3)
define ig(x) 7      print ig(a) ig(b) ig(v(1)) ig(dbl(a))
define pick(p,q) p  print pick(a,b) pick(b,a)
then print a, b, v(1)
```

exit 0, **1 valgrind error, 0 bytes lost**, and `a`, `b` and `v(1)` are intact
afterwards. The same deck at `HEAD` is exit **134**,
`malloc_consolidate(): unaligned fastbin chunk detected`, 48 errors and 320 B
lost.

### What the copy got wrong the first time

The root copy shipped in `00446226e` was **not faithful**, and an adversarial
review of that commit caught it. `copy_value_node()` builds the copy with
`vec_copy()`, which clears `v_link2` on what it returns
(`src/frontend/vectors.c`). But a name can resolve to a *list* of vectors —
`all`, `@dev[all]`, a cross-plot wildcard like `all.v(1)` — and `PP_mksnode()`
deliberately links that list through `v_link2`. Returning the argument node
had carried the whole chain; copying it took only the head:

| deck | `6ac4dc733` | `00446226e` | repaired |
| --- | --- | --- | --- |
| `define p(x) x` / `print p(all)` | 2 vectors | **1** | 2 |
| `define p(x) x` / `print p(@r1[all])` | 21 vectors | **1** | 21 |
| `define d(x,y) x` / `print d(all,1)` | abort 134 | **1** | 2 |

Silent, exit 0, no diagnostic — 20 of 21 values simply vanished. At arity 1
that is a straight regression on a shape the deck's own header calls "always
worked, must stay working"; at arity ≥ 2 it turned the abort into a wrong
answer rather than a right one. The repair walks the chain and rebuilds it the
way `PP_mksnode()` does. 200 calls with a 21-vector argument lose 0 bytes, so
the rebuilt chain is collected by the plot exactly as the parser's is.

The lesson is narrow and worth keeping: **`vec_copy()` is not a copy of a
dvec, it is a copy of one link of one.** The `0053` repair has the same call
and was never exposed, because a *stored body* cannot hold a chain — measured,
`define q() all` prints nothing on both binaries — so this only became
reachable when the same helper started serving arguments.

### Classes (b) and (c) get no deck, deliberately

Neither has an observable face: the deck prints the same bytes before and
after, and `make check` has no leak harness. Peak RSS is the only thing a deck
could look at, and it will not carry an assertion — for (b) it moves 4 kB over
2000 calls, inside noise, and while (c) moves 252 kB over 2000 cycles, RSS is
not something `check.sh` captures or that would be stable across platforms.
A deck that passed on both sides would assert nothing, so the valgrind figures
above and in `0054`'s Resolution are the assertion.
Class (a)'s face *is* observable, and
`tests/regression/misc/define-formal-body.cir` asserts it.

## Consequences

- `tests/regression/misc/define-formal-body.cir`, six shapes. Its assertion
  is **not** the diagnostic: the two `Internal Error` lines go to stderr,
  which `tests/bin/check.sh` does not capture, and its `egrep -v` filter drops
  every line matching `Error` in any case. What the harness sees is the
  *missing* `d(2,3) = …` value. At `6ac4dc733` the deck aborts at exit 134
  with **empty stdout**; the reference holds 16 lines through the filter.
- `make check`: **286 PASS, 0 FAIL**, exit 0, against the 285 baseline plus
  this deck. Every pre-existing directory unchanged: `regression/case` 113,
  `casedist` 23, `misc` 29→30, `pipe` 17, `xspice/case` 21, `xspice/casedist`
  17, `lint` 2.
- Identity lint **268 comparisons, baseline matches** — unmoved, as an
  ownership change that adds no string comparison should be.
- Differential sweep: **324 decks, `DIFF=62, OK=259, SKIP=3`**, with
  `PARSE-FAIL` and `NUM-DIFF` absent. Against the 323-deck baseline of
  `DIFF=62, OK=258, SKIP=3` the new deck lands in `OK`. A second run gave
  `DIFF=63, OK=258`; the extra deck is `bsim3soidd/ring51.cir`, which run in
  isolation gives `DIFF=2, OK=1` under **both** binaries, twice each — a
  load-induced flap, not a movement.
- `deck_output_differ.py`, `6ac4dc733` → fixed, all three modes:

  | mode | moved | which |
  | --- | --- | --- |
  | fold | 3 of 324 | `alter-rebin-case-lower`, `alter-rebin-param-case`, + the new deck |
  | preserve | 5 of 324 | `alter-rebin-case-lower`, `alter-rebin-param-case{,-lower}`, `binning-1`, + the new deck |
  | distinguish | 2 of 324 | `alter-rebin-param-case`, + the new deck |

  The new deck moves `rc -6 -> 0`, gains its value lines and loses the two
  `Internal Error` lines, in all three modes — which is the intended
  movement. Every other deck is in the `alter-rebin`/`binning` set the
  differ's own docstring and `doc/claude/decisions/0008` record as
  self-flapping, and the check the docstring prescribes was run: the **fixed
  binary against itself** moves `alter-rebin-case` and `alter-rebin-param-case`
  in fold and `alter-rebin-param-case` in preserve. **No deck outside that
  set moved in any mode**, and the new deck never appears in a
  self-comparison.
- One shipped deck does define an arity-2 function with a bare formal body:
  `examples/control_structs/new-check-4.sp:102`,
  `define foo(a,b) a > unwanted_output_file_2`, which exists to demonstrate
  that csh semantics swallow the `>`. It is never *called*, and the fault is
  at call time, so it is unaffected — run both ways from the same path, its
  stdout and stderr are identical and it still prints its own `INFO: ok`. It
  is an `examples/*.sp`, so neither measuring script had it in scope.

## Sequencing against `doc/codex/issues/0051`

`0051` criterion 4 wants `free_pnode()` added to `com_define()`'s replacement
path, where a redefinition currently overwrites `ud_text` and abandons the old
tree. **It should land after this, and it is now easier than it was.**

Class (c)'s repair is exactly the reference `0051` needs: with
`names->pn_use++` in place, a stored body is a tree with an owner, so the
`free_pnode(udf->ud_text)` that `0051` wants to add to the replacement path
frees the same way `com_undefine()` already does — node, children and the
root's vector — rather than leaving the vector behind as it would have before.
Landing `0051` first would have added a call that leaked 160 B per
redefinition of a value-rooted body.

They do not collide: `0051` edits `com_define()`'s `prefix()` test and its
replacement path, this edits `com_define()`'s assignment, `ft_substdef()` and
`PP_mkfnode()`. `0054` criterion 6 asked that both be measured together;
`0051`'s session should re-run the eleven-shape set above, because a
redefinition free is the one event that can reach a tree this decision made
owned.
