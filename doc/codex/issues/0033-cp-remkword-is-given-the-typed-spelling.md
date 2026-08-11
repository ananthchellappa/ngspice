# Issue: `cp_remkword(CT_VECTOR, ...)` Is Given the Typed Spelling, Not the Vector's

## Status

Open. This is `doc/codex/issues/0027`'s acceptance criterion 2, written up as
its own issue rather than folded into that fix.
`doc/claude/decisions/0004-unlet-vector-identity.md` decision 5 is the record
of why. Mode-independent and pre-existing.

## Summary

`vec_remove()` (`src/frontend/vectors.c:627`) deregisters the completion
keyword with the name the **caller** passed:

```c
    /* Remove from the keyword list. */
    cp_remkword(CT_VECTOR, name);
```

Every producer registers it with the **vector's own** spelling:
`cp_addkword(CT_VECTOR, v->v_name)` at `vectors.c:588`, `com_let.c:239`,
`com_compose.c:658`, `postcoms.c:1013`.

`cp_remkword()` reaches `clookup()` (`src/frontend/parser/complete.c:485`),
which walks the trie byte for byte — `place->cc_name[ind] < word[ind]` and
`place->cc_name[ind] > word[ind]`, with `create == FALSE` returning `NULL` on
either — and the file contains no case-folding primitive at all, in any mode.
So an add of `Out` and a remove of `OUT` diverge at the second character and do
not meet.

The pair is therefore internally inconsistent: the vector is selected
case-insensitively (`fold`, `preserve`) and deregistered case-sensitively.
`com_remzerovec()` (`postcoms.c:89`) already passes `ov->v_name` and is the
in-tree precedent for the one-word fix.

## Impact

A stale completion entry for a vector that no longer exists, plus the node
chain it holds — freed only at `cp_destroy_keywords()` or when the plot is
destroyed.

It is reachable in the **default** mode, not only under `distinguish`. The
reader's fold only covers text that arrives through `inp_readall()`, so a
control-language word can carry upper case in `fold` mode when it comes from
the interactive prompt, from `ngspice -p` on stdin, or from
`ngSpice_Command()`. Measured under default `fold` through `ngspice -p`:
`unlet OUT` removes a vector whose `v_name` is `out`, and `let MyVec = 2`
creates a vector named `MyVec`. Both leave a mismatch for the next `unlet`.
Simulator-built names make it reachable from a deck too: `q1#collCX`
(`src/spicelib/devices/bjt/bjtsetup.c:433`) and `V1#branch` keep their upper
case in every mode, so `unlet q1#collcx` de-permanents the vector and removes
no keyword.

**No user-visible consequence in this tree**, and that is the whole
difficulty. `keywords[CT_VECTOR]` is read only by `cp_ccom()`
(`src/frontend/parser/complete.c:98`), and `cp_ccom()` has no callers anywhere
— upstream deleted them from `src/frontend/parser/lexical.c` in `4feb0c3cc`,
"Remove function cp_ccon() and related code". `cp_comlook()` is callerless for
the same reason, `-q` is a no-op that prints "Command completion is not
supported", and `libngspice` forces `cp_nocc = TRUE` (`sharedspice.c:886`) so
the tree is never even built there. The keyword database is write-only in this
build.

## Root Cause

`cp_addkword()` is called from the site that owns the vector, which has
`v_name` to hand. `cp_remkword()` is called from `vec_remove()`, which
received a query. Nobody reconciled the two, and once completion lost its
readers there was nothing left to notice.

## Acceptance Criteria

1. `vec_remove()` passes `ov->v_name`, matching `com_remzerovec()`.
2. The change is landed as a consistency fix with the add sites named as its
   justification, **not** as a bug fix with a test, because no test can
   distinguish the two spellings while `cp_ccom()` is unreachable. If it is to
   be landed RED-first instead, the prerequisite is a reader — a `debug`
   command or a unit hook that dumps `keywords[CT_VECTOR]` — and that is a
   larger change than the line it would test.
3. `cdelete()` (`complete.c:601`) is read before landing it. Every
   case-mismatched remove that returns `NULL` today would newly reach it, and
   it is hand-rolled trie surgery that recurses into parents and frees nodes;
   one `valgrind` run of a `let`/`unlet` sequence with mixed case under
   `preserve` is the cheap guard.
4. The commit does not claim `CT_VECTOR` is now consistent. Three structural
   mismatches would remain: `plot_setcur()` deliberately does not swap the tree
   (`vectors.c:1338`, with the reason in a comment), a rawfile `load` leaves
   `keywords[CT_VECTOR]` NULL (`vectors.c:605`) so every later remove is a
   guaranteed no-op, and `vec_new()` never registers simulation vectors at all.

## Resolution

Not fixed. `doc/codex/issues/0027`'s criterion 2 asked for an audit and this is
its outcome: the mismatch is real, mode-independent, unobservable in this
build, and not free to change, because it newly exercises `cdelete()`. Filed
rather than folded in, per `doc/claude/decisions/0004-unlet-vector-identity.md`
decision 5. Anyone reviving command completion must fix this line **and** the
three items in criterion 4.
