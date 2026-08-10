# Plan: Fix Case Loss in Ngspice Script Files

## Goal and Acceptance Criteria

Preserve case-sensitive literal values in `*ng_script` and `*ng_script_with_params` files so the shipped `vlnggen` script emits valid Verilator arguments and C++ identifiers. Ordinary netlists and `.control` blocks must retain their current case-normalization behavior.

The fix is complete when:

- A regression fails with an ngspice 46 binary for the expected lowercase corruption.
- The same regression passes with the patched binary.
- `--Mdir`, `Vlng`, `VL_IN`, `VL_OUT`, `VL_INOUT`, and `VL_DATA` retain exact spelling.
- The full relevant regression suite passes.

## Implementation Plan

### 1. Establish the Baseline

Build the current checkout out-of-tree and record the exact configure flags. Use an ngspice 46 binary or a separate worktree at the parent of `e42a9e6b5c` as the known-bad baseline. Do not alter the main worktree merely to manufacture RED.

Trace a minimal script through `inp_spsource()`, `inp_read()` in `src/frontend/inpcom.c`, and `cp_lexer()`. Confirm that corruption occurs in `inp_read()` before lexical parsing.

### 2. Add the RED Regression First

Create `tests/regression/misc/script-case.cir` and `script-case.out`, then register the test in `tests/regression/misc/Makefile.am`. Begin the input with `*ng_script` and cover:

- `setcs` values `--Mdir` and `Vlng`.
- Case-sensitive `strcmp` and `strstr` patterns for `VL_IN`, `VL_OUT`, and `VL_INOUT`.
- An `echo`-generated `VL_DATA(...)` line.
- A harmless `shell echo` command containing mixed-case arguments, if portable across supported test hosts.

Run the test with the known-bad binary and retain the failure diff as RED evidence. Then run it against the current checkout to determine whether commit `e42a9e6b5c` already makes it GREEN.

### 3. Select the Narrowest Production Change

If the current code passes every acceptance case, keep its existing change and add only regression coverage. If it fails, modify the case-normalization decision near `src/frontend/inpcom.c:1825`.

Prefer a rule based on script context and command semantics over adding ad hoc exceptions. Preserve literal operands for case-sensitive commands, but do not globally disable normalization until tests demonstrate that variable names, vector names, control flow, and command lookup remain compatible. Do not patch `vlnggen` to compensate for corrupted input.

### 4. Verify `vlnggen`

Run `src/xspice/verilog/vlnggen` with Verilator when available. Verify the first invocation contains `--Mdir` and `--prefix Vlng`, generated-header scans find all three `VL_*` port forms, and generated files contain uppercase `VL_DATA(...)`. If Verilator is unavailable, use a temporary fake executable that records arguments and supplies a minimal `Vlng.h`; do not commit generated artifacts.

### 5. Regression and Review

Run the targeted test, `make -C tests/regression/misc check`, and `make check`. Review the diff for changes outside test files and the focused preprocessing logic. Document the RED and GREEN commands and results in the final handoff.

## Constraints

- Follow RED-first TDD from `AGENTS.md`.
- Preserve existing user changes and avoid unrelated formatting.
- Do not weaken expected output or broaden netlist behavior without explicit evidence.
- Use `ananth.chellappa@outlook.com` if creating commits.
