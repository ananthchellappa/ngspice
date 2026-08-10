# Issue: Verilator Shim Frees Context While Model Still Uses It

## Status

Resolved

## Summary

`Cosim_setup()` in `src/xspice/verilog/verilator_shim.cpp` creates its
`VerilatedContext` as a local `std::unique_ptr`, then passes only the raw
pointer to the generated `Vlng` model. When `Cosim_setup()` returns, the local
owner destroys the context even though the model remains stored in
`pinfo->handle` and continues to use it.

On the `--timing` path, a later call to `step()` obtains the dangling pointer
with `topp->contextp()` and dereferences it to read the time precision and
advance simulation time. This is a use-after-free in the shipped Verilator
shim.

## Impact

Timing-enabled Verilator code models can read freed memory after setup. The
observed behavior is allocator- and build-dependent: a simulation may appear
to work, produce incorrect timing behavior, crash, or be diagnosed by a
memory sanitizer. Trace setup also needs the same context after
`Cosim_setup()` returns.

The defect is independent of downstream trace changes; those changes merely
make the context lifetime requirement more visible.

## Root Cause

The setup code currently has two different lifetimes:

- `contextp` is a local `std::unique_ptr<VerilatedContext>` whose lifetime
  ends when `Cosim_setup()` returns.
- `topp` is allocated with `new`, returned through `pinfo->handle`, and used by
  subsequent co-simulation callbacks.

Constructing `topp` with `contextp.get()` does not transfer ownership. The
generated model retains a non-owning pointer, so its context must outlive the
model.

## Acceptance Criteria

- The `VerilatedContext` remains alive for every later use by the generated
  model, including timing and trace operations.
- `Cosim_setup()` does not leave the model holding a pointer owned by an
  automatic local object.
- A timing-enabled model can return from `Cosim_setup()` and execute `step()`
  without an AddressSanitizer use-after-free report.
- Non-timing Verilator co-simulation behavior remains unchanged.
- The ownership decision and its process-lifetime tradeoff are documented.
- The targeted test and relevant Verilator/XSPICE regression tests pass, and
  `git diff --check` passes.

## Implementation Plan

1. Add a RED regression that constructs a timing-enabled Verilator model,
   returns from `Cosim_setup()`, and invokes its step callback under
   AddressSanitizer. Confirm the unmodified shim reports the context
   use-after-free. Keep the test conditional on the required Verilator and
   sanitizer support so configurations without them are not broken.
2. In `Cosim_setup()`, explicitly release the context from the local
   `std::unique_ptr` after the model has been constructed successfully. This
   gives the context process lifetime, matching the current Verilator shim's
   process-lived model.
3. Add a short comment at the ownership transfer explaining why the apparent
   leak is intentional and preventing a future cleanup from restoring the
   dangling pointer.
4. Re-run the targeted sanitizer test to demonstrate GREEN, then exercise a
   non-timing model and a `--timing` model through the existing Verilator
   example flow. Include trace-enabled coverage when available because trace
   setup also retains the context.
5. Run the relevant XSPICE regression suite, followed by the broader enabled
   test suite, and finish with `git diff --check`.

## Design Note

Releasing the `unique_ptr` is the smallest safe change under the shim's current
lifecycle. `struct co_info` has a teardown callback, but this shim does not yet
use it. A future change could introduce a joint model/context owner and destroy
both from that callback; adding that lifecycle mechanism is broader than the
use-after-free fix and should not be required to make current callers safe.

## Resolution

Removed the `const` qualifier from the local `std::unique_ptr` and called
`release()` immediately after successful model construction. The generated
model therefore retains a valid context for its process lifetime. A nearby
comment documents the intentional lifetime choice.

No automated Verilator test was added because the existing test hierarchy has
no Verilator integration or configure-time handling for the external compiler
and sanitizer requirements. The checked-in timing example was used as the
focused behavioral regression instead.

## Verification

- Built `examples/xspice/verilator/delay.v` with Verilator 5.020 and
  `--timing`, recompiled the generated shim object directly from the changed
  source, linked `delay.so`, and ran `delay.cir` successfully.
- Built `examples/xspice/verilator/555.v` without timing, recompiled the
  generated shim object directly from the changed source, linked `555.so`, and
  ran `555.cir` successfully.
- `git diff --check` passes.
