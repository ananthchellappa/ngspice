# Issue: Vlnggen Trace Link Omits VCD Runtime

## Status

Resolved

## Summary

`vlnggen` fails during its final shared-library link when Verilator is invoked
with `--trace`. Verilator builds the waveform-dumping runtime as the separate
global object `verilated_vcd_c.o`, but `vlnggen` did not pass that object to
the linker.

## Impact

Code models generated without tracing continue to link normally. A
trace-enabled code model can compile successfully through Verilator and then
fail when `vlnggen` invokes `g++ --shared`, because symbols supplied by the VCD
runtime are unresolved. This prevents users from building a loadable code
model with waveform tracing enabled.

## Root Cause

The Unix-like final-link path in `src/xspice/verilog/vlnggen` constructs
`v_objs` from a fixed list:

- `verilator_shim.o`
- `verilated.o`
- `verilated_threads.o`
- `verilated_timing.o`, when timing is enabled

It then links those objects with `Vlng__ALL.a`. Verilator does not place the
global VCD runtime object in `Vlng__ALL.a`; for a `--trace` build it writes
`verilated_vcd_c.o` alongside the other global objects in the generated object
directory. Because the hardcoded list did not include it, the final link was
incomplete.

The VCD object is conditional on Verilator options, so adding it
unconditionally would break ordinary non-trace builds where the file does not
exist.

## Acceptance Criteria

- When `$objdir/verilated_vcd_c.o` exists, the final `g++ --shared` command
  includes it.
- When the object does not exist, the existing non-trace link command remains
  unchanged.
- Existing timing-object selection remains unchanged.
- `git diff --check` passes.

## Resolution

Before constructing the final link command, `vlnggen` now probes for
`$objdir/verilated_vcd_c.o` with `fopen`. If the file exists, the script closes
the handle and appends the pathname to `v_objs`. If it is absent, no object is
added.

This keeps the behavior driven by Verilator's generated output and avoids
duplicating or attempting to infer every option that may cause the VCD runtime
to be built.

## Verification

- `git diff --check` passes.
- Inspection confirms that the object is appended before the final
  `g++ --shared` invocation and only after a successful existence check.
- `make -j8` completes successfully in the out-of-tree `build-ver_50` build.
- The rebuilt ngspice successfully runs `vlnggen -- --trace` against the
  `examples/xspice/verilator/555.v` example. Verilator generates
  `verilated_vcd_c.o`, and the final step produces a valid `555.so` shared
  object containing `Cosim_setup` and the expected `VerilatedVcd` symbols.
