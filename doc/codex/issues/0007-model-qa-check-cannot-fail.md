# Issue: Model QA Check Cannot Fail

## Status

Open

## Summary

Thirteen of the fifty-nine tests run by `make -C tests check` are model-QA
cases (bsim3, bsim4, bsimsoi, hisim, hicum2, hisimhv1, hisimhv2). They report
PASS unconditionally, for two independent reasons:

1. The comparator has no failing exit path.
   `tests/bin/compareSimulationResults.pl` contains exactly three `exit`
   calls, all `exit(0)` in usage and info handling. It prints `DIFFER` lines
   and returns success regardless of how large the deviation is.
2. Results are re-used rather than re-simulated. `tests/bin/run_cmc_check:73`
   passes `-r` to `runQaTests.pl`, documented as "re-use previously simulated
   results if they exist (default is to resimulate, even if results exist)".
   After the first `make check`, those thirteen tests no longer invoke ngspice.

## Impact

`make check` cannot detect a model regression in any of the seven device
models covered by that arm, and a maintainer reading the summary sees an
all-green result while the same log reports comparisons that failed by up to
100 percent.

The second reason compounds the first: even once the comparator is taught to
fail, a subsequent `make check` would compare results produced by an earlier
build unless `-r` is reconsidered. A rebuild is currently never re-tested by
this arm.

## Reproduction

A full run from a clean out-of-tree build directory, configured with no
options:

```sh
$ make -C tests check
$ echo $?
0
```

```sh
$ grep -c '^PASS:' check.log          # 59
$ grep -cE '^(FAIL|ERROR|XPASS):' check.log   # 0
$ grep -c 'DIFFER' check.log          # 515
$ grep -cE 'DIFFER.*max rel error is [1-9][0-9]*\.[0-9]+%' check.log   # 189
$ grep -c 'max rel error is 100%' check.log   # 9
```

A representative pair of adjacent lines from that log:

```
variant: standard   (compared to: reference) DIFFER  (max rel error is 99.1%)
...
All 2 tests passed
```

The source facts behind it:

```sh
$ grep -n 'exit' tests/bin/compareSimulationResults.pl
116:        &usage();exit(0);
118:        &usage();&info();exit(0);
130:    &usage();exit(0);

$ grep -n '\-r' tests/bin/run_cmc_check
73:                -r -t ${test} \
```

Measured on the current master snapshot (46+), Ubuntu 24.04.4, x86-64. The two
scripts are unchanged in ngspice-46.

## Root Cause

`compareSimulationResults.pl` reports differences as output rather than as
status. `run_cmc_check` invokes it and does not derive its own exit status from
the comparison outcome, so the automake test driver sees success for every
case. The `-r` flag then removes the remaining connection between a `make
check` invocation and the binary being tested.

## Acceptance Criteria

- `compareSimulationResults.pl` exits non-zero when any comparison exceeds its
  tolerance, and zero otherwise.
- `run_cmc_check` propagates that status so the automake driver records FAIL.
- A deliberately perturbed reference file makes the corresponding model-QA
  test fail, and restoring it makes the test pass again.
- The tolerance each model is held to is stated somewhere a maintainer can
  find it, so that a `DIFFER` line is either a failure or is explained.
- `make check` re-simulates by default, or the reuse behaviour is documented
  along with how to force a fresh run.
- The existing pass/fail behaviour of the other forty-six tests is unchanged.
- `git diff --check` passes.

## Implementation Plan

1. Add a targeted check that perturbs one reference result and asserts the
   corresponding model-QA test fails. Confirm it does not fail against the
   current code (RED).
2. Give `compareSimulationResults.pl` a non-zero exit when a comparison
   exceeds tolerance. Keep the printed `DIFFER` output as it is; only the
   status changes.
3. Have `run_cmc_check` collect the comparator's status and exit non-zero if
   any comparison failed.
4. Establish what the existing 189 above-tolerance comparisons represent
   before turning them into failures. They may be genuine model deviations,
   stale reference data, or tolerances that were never set. Whichever it is,
   the arm cannot be made meaningful without deciding it, and turning on
   failure first would simply paint `make check` red.
5. Reconsider `-r`. Re-simulating by default costs run time but restores the
   property that `make check` tests the build in front of it.
6. Re-run the targeted check (GREEN), then `make -C tests check`, and finish
   with `git diff --check`.

## Design Note

Step 4 is the substance of this issue rather than a preliminary to it. The
mechanical part -- making a comparator return a status -- is small. What it
exposes is a body of comparisons nobody has had to adjudicate, because until
now nothing forced the question. Filing the mechanism and the data separately
would understate the work.
