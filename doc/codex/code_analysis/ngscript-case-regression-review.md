# Ngscript Case-Preservation Regression Review

## Scope

This report reviews the current regression-only change for the ngspice script
case-preservation bug. The change adds `script-case.cir`, its expected output,
and registration in `tests/regression/misc/Makefile.am`. No production code is
changed because commit `e42a9e6b5c` already fixes the behavior.

## Findings

### Expected output is ignored

`tests/.gitignore` contains a repository-wide `*.out` rule. Consequently,
`tests/regression/misc/script-case.out` exists locally but does not appear in
normal `git status` output and will not be included by an ordinary `git add`.

This is a blocking packaging problem. `tests/regression/misc/Makefile.am`
derives expected-output files with:

```make
EXTRA_DIST = \
	$(TESTS) \
	$(TESTS:.cir=.out)
```

If `script-case.cir` is committed without its `.out` file, clean checkouts and
release archives will not have the expected result used by `check.sh`. The test
must therefore be force-added explicitly:

```sh
git add -f tests/regression/misc/script-case.out
```

This should be done only when staging is requested; the file should not be
removed or replaced with a weaker expectation.

### Literal `VL_DATA(...)` echo path is not covered

The current test assigns `VL_DATA` using `setcs` and prints it through `$data`:

```spice
setcs data="VL_DATA"
echo preserved: $option $prefix $in $out $inout $data
```

This verifies `setcs` value preservation and variable expansion, but it does
not directly exercise the literal echo form used by `vlnggen` when creating
port headers:

```spice
echo VL_DATA($size,$line >> inputs.h
```

The production script depends on the `echo` line itself retaining uppercase
`VL_DATA`. A regression could preserve `setcs` operands while corrupting an
echo literal, and the current test would not isolate that behavior.

Add a portable literal assertion such as:

```spice
echo generated: VL_DATA(8,Clk,0,0)
```

with the exact matching line in `script-case.out`. This keeps the regression
self-contained and does not require Verilator.

## Existing Coverage

The regression already checks exact preservation of all required strings:

- `--Mdir`
- `Vlng`
- `VL_IN`
- `VL_OUT`
- `VL_INOUT`
- `VL_DATA`

It also exercises case-sensitive `strcmp` and `strstr` command lines. Testing
against the `ngspice-46` tag produces the expected lowercase corruption, while
the current checkout passes because of `e42a9e6b5c`.

## Recommended Resolution

1. Add a literal `echo VL_DATA(...)` assertion to `script-case.cir` and its
   expected output.
2. Re-run the ngspice 46 RED check and current-checkout GREEN check.
3. Force-add `script-case.out` when staging the completed change.
4. Confirm the staged diff contains the `.cir`, `.out`, and `Makefile.am`
   registration before submission.

