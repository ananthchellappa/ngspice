# Issue: `define` Silently Overwrites Any Function Whose Name It Is a Prefix Of

## Status

Open, and **blocking one line of a shipped fix**. Found by
`doc/claude/decisions/0013-user-defined-function-identity.md` while
classifying the five `src/frontend/define.c` comparisons that
`tests/lint/identity.baseline` had frozen. Items 1 and 3 are not `casemode`
defects — both reproduce under the default `fold` with no `casemode` set —
and item 2 is the reason `0013` could not make `com_define()`'s
replace-or-prepend lookup case-insensitive along with the three resolutions
beside it.

## Summary

### 1. `define f(x)` destroys a stored `foo(y)`

`com_define()` decides whether to replace an existing definition or prepend a
new one with

```c
    for (udf = udfuncs; udf; udf = udf->ud_next)
        if (prefix(b, udf->ud_name) && (arity == udf->ud_arity))
            break;
```

`prefix(p, s)` is true when `p` is a prefix of `s`. So the test is not
"is this the same function", it is "does this name begin the stored one".

```spice
.control
define foo(y) y*100
print foo(2)          foo(2) = 2.000000e+02
define f(x) x*3
print foo(2)          Error: no such function as foo, or foo(2) is not available.
define                foo is gone; f (x) = (x)*(3) stands where it stood
.endc
```

Measured at `4d36cf61d` under the default `fold`. `f` and `foo` are unrelated
one-argument functions and defining the shorter name destroys the longer one,
silently.

Two allocations go with it. `udf->ud_name = b` and `udf->ud_text = names`
overwrite the fields without freeing what was there, so both the old packed
name and the old parse tree leak. That is the ordinary redefinition path too —
`define f(x) 1` followed by `define f(x) 2` leaks the same two — so the leak is
not caused by the prefix test, only made reachable by names the user never
redefined.

The name does not have to be shorter by much, and it does not have to be the
user's. `define m(a,b) a+b` destroys the shipped `min(x,y)`, measured at
`4d36cf61d` under `fold` at the prompt:

```
define m(a,b) a+b
define          m (a, b) = (a)+(b)   stands where min stood; max survives
```

### 2. The case fix cannot reach this line until it is repaired

`doc/claude/decisions/0013-user-defined-function-identity.md` makes the three
*resolutions* of a user-defined function name case-insensitive outside
`distinguish`. Under `preserve` the definition side should agree — a
`define VM(x)` beside the shipped `vm` should replace it, because two
spellings are one identifier there — and it cannot, because **case-blinding a
prefix test widens the set of names it destroys**. Measured on a build that
tried it, at the `ngspice -p` prompt under the default `fold`:

```
define VD(x) x*7
define          vdb (x) = db (v (x))   is gone
```

`ciprefix("VD", "vdb")` is true where `prefix("VD", "vdb")` is false. So the
case-insensitive arm this line needs is exactly the arm that turns item 1 from
a defect about *length* into a defect about length **and** case, and `0013`
decision 2 leaves the line byte-exact instead. Its residue is that under
`preserve` `define VM(x)` prepends beside the shipped `vm` rather than
replacing it: no number moves, because `com_define()` prepends and
`ft_substdef()` takes the first match, but `define vm` lists two entries under
one identifier.

That residue is asserted by two decks, so this issue's fix has to come through
both: `tests/regression/case/udf-name-case.cir` ends with three `define vm`
lines and `udf-name-case-lower.cir`, where the definition really does replace,
ends with two.

### 3. `undefine` and `define <name>` say nothing when they find nothing

`com_undefine()` walks the list and removes every match. On no match it
returns, in every mode, for every reason:

```
undefine nosuchfunction        silent
undefine VM                    silent under distinguish, where VM is not vm
```

`prdefs()` has the same shape: `define nosuchfunction` prints nothing rather
than saying the name is not defined. `doc/codex/issues/0028` established for
node names that a reference which misses with **no** case variant present is
an ordinary typo and should be reported mode-independently; this is the same
shape at the user-defined function table, and it is why
`doc/claude/decisions/0013` decision 4 declined to add a `distinguish`-only
near-miss warning here. A near-miss report would be the only thing either
command ever says, and it would arrive before the plain-miss report that
should come first.

## Impact

Item 1 is a silent wrong answer, and it is not exotic: `min`, `max`, `vm`,
`vi`, `vr`, `vp`, `vdb`, `vg` and `gd` are all in the table at start-up
(`ft_cpinit()`, `src/frontend/cpitf.c:56`), so `define m(a,b)` destroys `min`
without inventing an adversarial name. Not every short name reaches it —
`define v(x)` is refused earlier by `com_define()`'s `ft_funcs[]` check,
`Error: v is a predefined function.` — so the reachable set is the names that
prefix a *user-defined* one and match its arity.

The damage is confined to a session's own function table — no simulation
result is computed from it directly — but a `.control` block that loses a
function it defined earlier prints an error and skips a `print`, which is a
missing number rather than a wrong one.

Item 2 is why this issue blocks a line of a shipped fix rather than only
waiting for one. Item 3 is a missing diagnostic.

## Root Cause

Item 1: `prefix()` where an equality test belongs. The comparison has to be
against the *name* rather than the whole packed `name\0arg1\0arg2\0` buffer,
which is presumably why a prefix test looked right — `b`'s first NUL ends the
name, so `prefix(b, ud_name)` does compare only the name. What it does not do
is require the stored name to end there too.

The line is `prefix(b, udf->ud_name)` and `doc/claude/decisions/0013` leaves it
exactly there, with a comment saying why. The repair is to end the comparison
at the *stored* name's terminator as well as the typed one's — at which point
the same line can take `udf_name_eq()`, because `strcmp()` and `cieq()` on
these packed buffers already stop at the first NUL and therefore compare the
name alone.

Item 2 has no separate cause: it is item 1's cause meeting a mode in which two
spellings are one identifier.

Item 3: no miss path. Both loops fall out of the bottom with nothing to say.

## Acceptance Criteria

1. `define foo(y) y*100` followed by `define f(x) x*3` leaves both functions
   callable, and `define m(a,b)` leaves `min` callable, in all three case
   modes, with a deck in `tests/regression/misc/` asserting the values — the
   defect is mode-independent, so its deck does not belong in
   `tests/regression/case/`.
2. The name comparison ends at the stored name's terminator as well as at the
   typed name's, and then takes `udf_name_eq()`, so that
   `src/frontend/define.c` resolves and defines a name by one rule.
3. With criterion 2 met, under `preserve` `define VM(x)` beside the shipped
   `vm` **replaces** it. `tests/regression/case/udf-name-case.cir`'s last
   three lines become the two its `-lower` twin already asserts, and both
   decks are updated in the commit that does it. That deck change is the RED
   for this half.
4. Redefining an existing function frees the packed name and the parse tree it
   replaces, or the issue records why `free_pnode()` cannot be called there —
   `trcopy()`'s comment says parse trees are never freed, so this needs
   checking rather than assuming.

   **Unblocked on 2026-08-12 by `doc/codex/issues/0053`, and the sequencing was
   the reason that issue went first.** Until then `ft_substdef()` handed a
   single-node body's stored root out to the evaluator with no reference at
   all and the evaluator freed it, so adding `free_pnode(udf->ud_text)` here
   would have freed it a second time. That is now closed —
   `copy_value_node()` gives the caller a node of its own — so nothing escapes
   `udf->ud_text` uncounted and `free_pnode()` **is** safe on this path. The
   comment that prompted the caveat is stale: `cbdd811aa` (2005) retired its
   premise when it added `pn_use` counting to `free_pnode_x()` and the three
   `pn_use++` lines to `trcopy()`. See
   `doc/claude/decisions/0014-single-node-define-body.md`, which also records
   that `undefine` on a single-node body still abandons that body's dvec —
   `doc/codex/issues/0054` class (c) — which this criterion's repair should be
   measured against.

   **`0054` closed on 2026-08-12, and this criterion should land after it
   rather than with it.** Class (c)'s repair is precisely the reference this
   criterion needs: `com_define()` now does `names->pn_use++`, so a stored body
   is a tree with an owner, and the `free_pnode(udf->ud_text)` added here
   releases the node, its children **and the root's vector**, the same way
   `com_undefine()` already does. Landing this first would have added a call
   that leaked 160 B per redefinition of a value-rooted body — `define c(x) 5`
   twice — which is exactly the loss `0054` class (c) measured at `undefine`.
   The two edits do not collide: `0054` touched `com_define()`'s assignment,
   `ft_substdef()` and `PP_mkfnode()`, while this touches the `prefix()` test
   and the replacement path. `0054` criterion 6 asked that both be measured
   together, so this session should re-run the eleven-shape set in
   `doc/claude/decisions/0015-argument-list-ownership.md`: a redefinition free
   is the one event that can reach a tree `0015` made owned. Note also
   `doc/codex/issues/0055` — a body that calls another user-defined function
   shares that function's interior nodes, so a redefinition free walks into
   nodes a second `udfunc` still holds. `free_pnode_x()` decrements those
   rather than freeing them, which is what makes it safe; that is worth
   asserting with a deck rather than assuming.
5. `undefine nosuchfunction` and `define nosuchfunction` report the miss, on
   `cp_err`, mode-independently, in `doc/codex/issues/0028`'s wording family.
   Only after that is a `distinguish` near-miss report worth adding, per
   `doc/claude/decisions/0001-distinguish.md` decision 2.
6. `make check` unchanged with `casemode` unset, and the differential sweep's
   `NUM-DIFF` and `PARSE-FAIL` stay 0.

## Resolution

Open.
