# Issue: Script Parameters Are Discarded By Option Parsing

## Status

Open

## Summary

A `*ng_script_with_params` script receives its parameters from
`Copy_of_argv[optind ...]`, which is populated after `getopt_long()` has
already consumed the command line. Any parameter beginning with `-` that
ngspice does not recognise as one of its own options is reported on stderr and
then dropped: it never reaches the script, `$argc` does not count it, and the
script has no way to detect that an argument went missing.

## Impact

`vlnggen` is a `*ng_script_with_params` script whose whole interface is
"arguments acceptable to the Verilator compiler". The natural and documented
way to request a waveform-capable code model is therefore

```sh
ngspice vlnggen --trace foo.v
```

and that command silently produces a code model with tracing disabled. The run
returns 0, the script prints no error, the generated makefile carries
`-DVM_TRACE=0 -DVM_TRACE_VCD=0`, and the only sign of trouble is a line from
ngspice itself mentioning an option the script never asked about. A user who
believes they built a trace-enabled model gets one that cannot dump waveforms.

The same loss applies to every other Verilator option a user may need to pass
through `vlnggen` or `ghnggen`, for example `--timing`, `--top-module` or
`-Wno-...`.

## Reproduction

```sh
$ cat > p.sp <<'EOF'
*ng_script_with_params
echo argc=$argc
echo argv=$argv
quit
EOF

$ ngspice p.sp --trace counter.v
ngspice: unrecognized option '--trace'
argc=1
argv=counter.v

$ ngspice -- p.sp --trace counter.v
argc=2
argv=--trace counter.v
```

Argument order does not help: `ngspice p.sp counter.v --trace` also reports the
unrecognised option and yields `argc=1`.

Observed through `vlnggen` itself, on a tree whose `vlnggen` is otherwise
unmodified:

```sh
$ ngspice /usr/local/share/ngspice/scripts/vlnggen --trace counter.v
ngspice: unrecognized option '--trace'
...
g++ ... -DVM_TRACE=0 -DVM_TRACE_VCD=0 ... -c -o Vlng__ALL.o Vlng__ALL.cpp
$ echo $?
0
```

Measured on ngspice-46 and on the current master snapshot (46+), Ubuntu
24.04.4, x86-64.

## Root Cause

`src/main.c:1489` assigns `Copy_of_argv = argv` inside the
`*ng_script_with_params` branch, which is reached only after option parsing has
run. `src/frontend/inp.c:665-690` then builds the script's `argv` and `argc`
from `Copy_of_argv[optind + n]`.

GNU `getopt_long()` permutes the command line so that non-option arguments end
up at the tail, and it skips unrecognised options after reporting them. An
unrecognised option is therefore never inside the `[optind, argc)` range that
`inp.c` reads, so the loss is structural rather than incidental.

A bare `--` before the script name stops option parsing and makes the whole
remainder visible to the script, which is why the second command above works.
That behaviour is currently undocumented.

## Acceptance Criteria

- A parameter that ngspice does not recognise as one of its own options either
  reaches the script verbatim in `$argv`, or causes the run to fail with a
  diagnostic that names the discarded parameter.
- `$argc` counts every parameter supplied after the script file name.
- Invocations that pass ngspice's own options before the script name continue
  to work unchanged.
- `vlnggen` fails with an error rather than producing a code model when a
  requested Verilator option did not arrive.
- The escape used by any interim workaround is documented where the script
  interface is described.
- `git diff --check` passes.

## Implementation Plan

1. Add a regression under `tests/regression/misc/` that runs a
   `*ng_script_with_params` file with a parameter ngspice does not recognise
   and asserts it appears in `$argv`. Confirm it fails against the current
   code (RED).
2. Snapshot the raw `argv` before `getopt_long()` runs and hand the script
   everything following the script file name verbatim. This is the smallest
   change that makes the script interface mean what it says, and it does not
   alter how ngspice's own options are parsed.
3. If preserving the current parsing order is preferred instead, suppress
   permutation for this case (a leading `+` in the optstring, or `opterr = 0`
   with explicit handling) so that unrecognised options remain in the tail.
4. Make `vlnggen` verify that it received at least one argument beyond the
   Verilog source when the caller supplied one, so a silently truncated
   command line cannot yield a quietly misconfigured build.
5. Document the parameter-passing rule, including the `--` form, in the
   `vlnggen` and `ghnggen` header comments and in the manual section covering
   script files.
6. Re-run the new regression (GREEN), then `make -C tests check`, and finish
   with `git diff --check`.

## Design Note

The `--` escape makes this recoverable today without a code change, so the
urgency is documentation rather than correctness of the simulator core. What
makes it worth fixing rather than only documenting is the failure mode: the
build succeeds, exits 0, and produces an artifact that differs from the one
requested. A user has no signal to act on beyond a stderr line that appears to
be about ngspice's own command line.
