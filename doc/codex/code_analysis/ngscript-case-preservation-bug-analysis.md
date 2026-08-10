# Ngspice Script Case-Preservation Bug Analysis

Start in `src/frontend/inpcom.c`, not in `vlnggen` or the command lexer.

The preprocessing path lowercases sourced lines before `cp_lexer()` sees them:

```c
/* lower case for all other lines */
for (s = buffer; *s && (*s != '\n'); s++)
    *s = tolower_c(*s);
```

This checkout already contains commit `e42a9e6b5c`, titled:

> Fix excessive forcing to lower-case in pure script files.

It added:

- Recognition of `*ng_script` files around `src/frontend/inpcom.c:1406`.
- Case-preserving exceptions for `setcs`, `shell`, `strcmp`, and `strstr`.
- Existing protection for `echo`.

Those exceptions correspond closely to the four `vlnggen` failures. However, the commit added no regression test and relies on a growing command whitelist, which is fragile.

## Recommended RED-First Path

1. Add `tests/regression/misc/script-case.cir` and its expected `.out`.
2. Start it with `*ng_script`.
3. Exercise mixed-case literals without requiring Verilator:

```spice
*ng_script
setcs option="--Mdir"
setcs prefix="Vlng"
setcs pattern="VL_INOUT"
echo $option $prefix $pattern
strcmp result "$prefix" "Vlng"
echo comparison=$&result
quit
```

4. Register it in `tests/regression/misc/Makefile.am`.
5. Run it against ngspice 46 and confirm RED: `--mdir`, `vlng`, and `vl_inout`.
6. Run it against this checkout to evaluate the existing fix.

## Production-Fix Scope

Avoid modifying `src/xspice/verilog/vlnggen`. Decide whether pure `*ng_script` files should bypass whole-line lowercasing entirely. That is cleaner than adding every case-sensitive command to the whitelist, but it needs regression coverage for legacy case-insensitive variable and vector behavior.

Keep `.control` blocks and ordinary netlist canonicalization out of the initial change. They share this preprocessing code but may depend on lowercasing.
