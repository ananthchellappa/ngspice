# Issue: Pipe Mode Segfaults Without A Display

## Status

Open

## Summary

With no X display available, `ngspice -p` and `ngspice -i` print the
"no graphics interface" diagnostic and then terminate with SIGSEGV. No circuit
is required; a command stream consisting of `quit` alone is enough. The batch
and default modes are unaffected and exit cleanly with status 1.

This is not a regression: ngspice-46 and the current master snapshot (46+)
behave identically.

## Impact

`tests/regression/pipe` invokes the simulator as
`$(top_builddir)/src/ngspice -p < $1`, so the crash takes out ngspice's own
regression suite whenever no display is present:

```sh
$ env -u DISPLAY make -C tests/regression check
... Segmentation fault (core dumped) ../../../src/ngspice -p < $1
make: *** [check] Error 2
```

With a display the same suite is 59/59 PASS. A continuous-integration or
container consumer therefore cannot run `make check` at all, and the result
presents as a failing test rather than as a crash in the simulator.

Beyond the suite, `-p` is the interface external tools use to drive ngspice
over a pipe, which is exactly the context least likely to have a display.

## Reproduction

```sh
$ echo quit | env -u DISPLAY ngspice -p
...
 please check if X-server is running,
 or ngspice is compiled properly (see INSTALL)
Segmentation fault (core dumped)
$ echo $?
139
```

Scope, same binary and environment, varying only the flag:

| invocation        | DISPLAY unset  | DISPLAY set |
| ----------------- | -------------- | ----------- |
| `ngspice -p`      | SIGSEGV (139)  | 0           |
| `ngspice -i`      | SIGSEGV (139)  | 0           |
| `ngspice -b`      | 1 (clean)      | -           |
| `ngspice` (bare)  | 1 (clean)      | -           |

Measured on ngspice-46 and 46+, Ubuntu 24.04.4, x86-64, X11 build.

## Root Cause

Not established; recorded here as the area rather than as a proven cause,
because the binaries to hand carry no symbols and no symbolised frame could be
obtained.

`src/frontend/display.c:203-215` handles the no-device case. It emits the
observed message through `externalerror()` and then falls back with

```c
dispdev = FindDev("error");
```

The `"error"` row of the device table at `src/frontend/display.c:44` is

```c
{ "error", 0, 0, 0, 0, 0, 0, ...
```

that is, its entries are null. A later call through one of those entries, or a
read of a structure that was never initialised because the real device never
came up, would match what valgrind reports:

```
Invalid read of size 4
 Access not within mapped region at address 0x10
```

The offset is consistent with a member access through a null pointer.

## Acceptance Criteria

- `echo quit | ngspice -p` with no display available terminates without a
  signal, and with a status distinguishable from success.
- `ngspice -i` under the same conditions behaves the same way.
- `make -C tests check` completes with no display present, or
  `tests/regression/pipe` is skipped with a stated reason rather than
  crashing.
- Behaviour with a display present is unchanged.
- `git diff --check` passes.

## Implementation Plan

1. Reproduce under a build carrying symbols and record the faulting frame, so
   the fix targets the actual dereference rather than the fallback that
   precedes it.
2. Add a regression that runs `ngspice -p` with `DISPLAY` unset and asserts a
   clean exit. Confirm it fails against the current code (RED).
3. Give the `"error"` device real no-op entries, so that every call through
   `dispdev` after the fallback is safe by construction. This is preferable to
   guarding each call site, because the fallback exists precisely to be a
   safe stand-in.
4. Where a graphics-dependent operation genuinely cannot proceed, report it
   through the normal error path instead of continuing, matching the way `-b`
   already declines.
5. Re-run the new regression (GREEN), then `make -C tests check` both with and
   without a display, and finish with `git diff --check`.

## Design Note

The diagnostic that precedes the crash is already correct and already tells the
user what is wrong. Only the continuation past it is at fault, which is why the
fallback device rather than the display-detection logic is the place to look
first.
