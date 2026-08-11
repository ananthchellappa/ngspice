# 0043 — an analysis typed as a command resolves its node without the end-of-parse check

## Status

Open. Found by the adversarial review of `doc/codex/issues/0028`'s two
commits, `17e429993` and `7b18264c4`, on branch `ver_50`.

## Summary

`sens`, `tf`, `noise` and `pz` exist both as dot cards and as control-language
commands. The dot card goes through `INPpas2()` from `if_inpdeck()`
(`src/frontend/spiceif.c`), which ends with `INPtermCaseCheck()`. The command
form builds a synthetic card and runs `INPpas2()` on it from `if_run()`
(`src/frontend/spiceif.c:357`) — and **never calls `INPtermCaseCheck()`**.

So `0028`'s report fires for

```spice
.sens v(sou)
```

and is silent for the same miss written as

```spice
.control
sens v(sou)
.endc
```

`7b18264c4` made all nine reference sites mark the node they create, which is
correct on both paths; nothing reads the mark on the command path.

## Impact

The same silent wrong answer `0028` closed, reachable through the interactive
and control-language form of four analyses. It is a diagnostic gap, not a
wrong number in itself: the analysis runs against a manufactured node and
reports about a node the deck never defined.

Worse on the `sens` command specifically, and this part is **pre-existing and
not caused by `0028`**: the synthetic card is parsed after `CKTsetup()`, so
the manufactured node is created on a circuit that is already set up, and the
run dies with

```
Internal Error: incomplete CKTunsetup(), this will cause serious problems, please report this issue !
ERROR: fatal error in ngspice, exit(1)
```

## Root Cause

`if_inpdeck()` and `if_run()` both parse cards, and only the first asks the
end-of-parse question. The check is written for a whole netlist —
`INPtermCaseCheck()` walks the table and reports what no card claimed — and on
the command path the "whole netlist" is one synthetic card against a table
that already holds every node of a circuit that has been built.

That is not a reason it cannot be answered there; it is a reason the answer
has to be scoped differently — to the nodes this card created — which is a
different mechanism from the one `0002` built, not a call added in one place.

## Acceptance Criteria

- `sens v(sou)` typed in a `.control` block, against a deck that never wrote
  `sou`, produces the same report the dot card produces.
- A node created by a command-form analysis is not reported when the deck does
  define it, including when the command names it before... any later
  `source` defines it, if that is possible on this path.
- The `CKTunsetup` fatal above is decided one way or the other: either the
  command-form analysis refuses to create a node after setup, or it creates
  one the rest of the run can survive.
- A deck guards both, in a directory whose harness can capture `cp_err`.

## Resolution

None yet.
