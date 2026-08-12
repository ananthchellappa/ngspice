# Issue: A `define` Whose Body Is a Single Constant Is Freed on Its First Call — Wrong Number, Then Segfault

## Status

Open, **not a `casemode` defect**, and the most severe thing currently open in
`src/frontend/define.c`. It reproduces in the shipped default mode with no
`casemode` set, from three lines of input, and it has two faces: a silently
wrong number in a batch deck and a `SIGSEGV` at the interactive prompt.

Found on 2026-08-12 while scoping `doc/codex/issues/0051`'s acceptance
criterion 4 — *"redefining an existing function frees the packed name and the
parse tree it replaces, or the issue records why `free_pnode()` cannot be
called there"*. The answer to that question turned out to be that
`free_pnode()` **is** safe there, and that something else is already calling it
too early.

**It blocks `0051` criterion 4.** Adding the missing `free_pnode(udf->ud_text)`
to `com_define()`'s replacement path today would free a body that this defect
has already handed out with no reference count, so the leak fix has to land
after this or with it.

## Summary

Measured against `build-ver_50/src/ngspice` at `528c3defd`, **no `casemode`**,
nothing else set.

### At the prompt — `SIGSEGV`

```
$ ngspice -p
ngspice 1 -> define c(x) 5
ngspice 2 -> print c(2)
c(2) = 5.000000e+00
ngspice 3 -> print c(2)
Segmentation fault
```

`echo $?` is **139**. The `SURVIVED` marker after it never prints.

### In a batch deck — a silently wrong number, and the run completes

```spice
.control
op
define c(x) 5
print c(2)
print c(2)
echo SURVIVED
.endc
```

```
c(2) = 5.000000e+00
c(2) = 2.000000e+00      <- the ARGUMENT, not the body
SURVIVED
```

Exit status 0. The second call returns `2`, which is the value of `x` at the
call site, because the freed node has been reused. A deck that calls a
constant-bodied function twice prints a wrong number with nothing on either
stream.

### A third face — the body is a vector name

```
ngspice 1 -> let a = 5
ngspice 2 -> define g(x) a
ngspice 3 -> print g(1)
g(1) = 5.000000e+00
ngspice 4 -> print g(1)
g(1) = 1.000000e+00      <- wrong, and stays wrong
ngspice 5 -> print g(1)
g(1) = 1.000000e+00
```

### What is *not* affected

```
define c(x) 5+0     three calls, 5.000000e+00 each time
define c(x) x       three calls, correct
```

So the hazard is exactly **a body that is a single node which is not a formal
parameter** — a bare constant, or a bare reference to a vector. Give the body
any operator and it is safe.

## Impact

The worst combination this tree has open: silent in batch, fatal at the prompt,
present in the default mode, and reachable by an ordinary line. `define k(x) 5`
is not an adversarial construction — a constant-bodied helper is the first
thing a user writes when they discover `define`.

It also makes `undefine` fatal on the same functions:
`define c(x) 5` / `print c(2)` / `undefine c` segfaults, with valgrind
reporting an `Invalid free()`.

`ft_cpinit()`'s shipped `udfs[]` (`src/frontend/cpitf.c:56-71`) is **not**
exposed: every one of its fourteen bodies has an operator or a function call at
the root — `mag(v(x))`, `(x gt y) * x + …` — so no shipped function has a
single-node body. That is luck rather than design, and it is why the defect has
survived.

## Root Cause

`trcopy()` (`src/frontend/define.c`) substitutes a call's arguments into a copy
of the stored body. Every branch that builds a node allocates a fresh one and
bumps the child's reference count — `pn->pn_left->pn_use++` and its siblings.
The `pn_value` branch does not:

```c
    if (tree->pn_value) {
        struct dvec *d = tree->pn_value;
        if ((d->v_length == 0) && strcmp(d->v_name, "list")) {
            /* ... formal-parameter substitution ... */
            return tree;            /* not a formal: the STORED node itself */
        }
        return tree;                /* likewise */
    }
```

Returning the stored node is correct and deliberate — the comment above
`trcopy()` says the tree is shared on purpose, and `free_pnode_x()`
(`src/frontend/parse.c:700-722`) has carried `pn_use` reference counting since
`cbdd811aa` (2005) precisely so that sharing is safe.

The counting is done by the **parent**, inside `trcopy()`. When the body's root
*is* that value node there is no parent to do it: `ft_substdef()` returns it
straight to `PP_mkfnode()` (`src/frontend/parse.c:517-523`), which returns `q`
without bumping anything, and the node reaches the caller with `pn_use == 0`.
`free_pnode_x()` then takes its `else` branch — `pn_use` is not `> 1` — and
frees the node, its `pn_name`, and its `pn_value` if that vector is not
`VF_PERMANENT`. The definition in `udfuncs` now points at freed memory.

The first call therefore works and destroys the definition; the second reads
it. Whether that is a wrong number or a fault is an allocator accident, which
is why the prompt and a batch deck disagree.

`doc/codex/issues/0051` is a different defect in the same function and neither
causes the other.

## Acceptance Criteria

1. `define c(x) 5` called three times returns `5` three times, in a batch deck
   and at the prompt, in all three case modes. Same for
   `let a = 5` / `define g(x) a`, and for `define c(x) 5` followed by
   `undefine c`.
2. No `SIGSEGV` and no valgrind error on any of those sequences. The exit
   status of the prompt run is 0.
3. The fix is a reference count taken where one is missing, not a copy. Two
   candidates, and whichever is chosen must say why the other was not:
   **(a)** `ft_substdef()` bumps `pn_use` on the tree it returns when that tree
   is the stored root, which is the narrow repair;
   **(b)** `trcopy()`'s `pn_value` branch bumps before returning, which is
   broader and has to be checked against the formal-substitution path, where
   `ntharg()` also returns a node the caller owns.
   Whether `PP_mkfnode()` is the right place instead is worth reading, because
   it is the one frame that knows the result is about to be handed to an
   evaluator.
4. A deck asserts it where `make check` can see it. The failure is a **value**
   in batch — `c(2) = 2.000000e+00` where `5.000000e+00` is right — so it does
   not need the capture-and-read-back shape, and it belongs in
   `tests/regression/misc/`, not in a `case` directory: nothing here is about
   case.
5. `doc/codex/issues/0051`'s criterion 4 is re-read afterwards. Adding
   `free_pnode(udf->ud_text)` to `com_define()`'s replacement path is safe only
   once this is fixed; the two should be sequenced and the sequencing recorded.
6. `make check` unchanged with `casemode` unset — 282 PASS / 0 FAIL — and the
   differential sweep's `NUM-DIFF` and `PARSE-FAIL` stay 0.

## Resolution

Open.
