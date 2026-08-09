# Issue: Ngscript Case Regression Is Incomplete

## Status

Resolved

## Summary

The new `script-case.cir` regression had two submission gaps:

1. `tests/regression/misc/script-case.out` is ignored by
   `tests/.gitignore` (`*.out`) and will be omitted unless explicitly
   force-added.
2. The test printed `VL_DATA` through a `setcs` variable but did not cover the
   literal `echo VL_DATA(...)` form used by `src/xspice/verilog/vlnggen`.

## Impact

A commit can register the regression without shipping its expected output,
causing failures in clean checkouts or distribution archives. Separately, a
future regression affecting case preservation of echo literals could escape
the current test.

## Acceptance Criteria

- `script-case.cir` contains an exact literal `echo VL_DATA(...)` case.
- `script-case.out` contains the corresponding case-sensitive expected line.
- The corrected test fails against `ngspice-46` and passes against the current
  checkout.
- The staged changes include `script-case.out` despite the `*.out` ignore rule.
- `git diff --check` passes.

## Suggested Commands

```sh
git add -f tests/regression/misc/script-case.out
git diff --cached --name-status
git diff --check
```

## Resolution

Added the literal `echo generated: VL_DATA(8,Clk,0,0)` path to
`script-case.cir` and the exact case-sensitive expected line to
`script-case.out`. No production code or existing assertion was changed.

Verification on 2026-08-08:

- Corrected targeted test against separately built ngspice 46: failed as
  expected, emitting `generated: vl_data(8,clk,0,0)`.
- Corrected targeted test against the separately built current checkout
  (ngspice 46+): passed.
- `make -C tests/regression/misc check` from the out-of-tree current build:
  all 15 tests passed.
- Broader `tests/regression` check: all completed groups passed. The `pipe`
  group could not run because this environment could not open an X display,
  including under `xvfb-run`; the remaining `pz` group passed when run
  separately.
- `git diff --check`: passed.

The expected-output file remains ignored by `tests/.gitignore` and was
included with `git add -f tests/regression/misc/script-case.out`.
