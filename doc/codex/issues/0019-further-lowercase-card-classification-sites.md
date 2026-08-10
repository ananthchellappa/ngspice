# Issue: Six More Lower-Case Card-Classification Sites, Four of Them Outside `inpcom.c`

## Status

Open. Enumerated while closing `doc/codex/issues/0009`'s follow-up table and
deliberately not fixed there: that issue's scope was its own table, and each
of these needs its own deck under its own dialect. Two of the six are
wrong-number defects, which makes them higher priority than most of what the
table contained.

## Summary

Every site below classifies a card by comparing a byte to a lower-case
character literal, and is correct only because `inp_read()` folded the card
first. That is `doc/codex/issues/0009`'s defect class exactly. None of them
appears in any of that issue's three follow-up tables, and its "Method note
on this enumeration" explains why: the enumeration was scoped to
`src/frontend/inpcom.c` and to a card's *first* character, and four of these
six are in other files.

Line numbers are against `b10fdafb4`.

**Wrong-number defects — no diagnostic at all.**

```c
/* src/frontend/inpcompat.c:1101, pspice_compat() */
        if (*cut_line == 'q') {
```
```c
/* src/frontend/inpcompat.c:1153, pspice_compat() */
        else if (*cut_line == 'd') {
```

These two append `area=<n>` to a BJT or diode instance when the token after
the model name is a bare positive number, which is how PSpice spells an area
factor. Under `-D casemode=preserve -D ngbehavior=ps`, `Q1 c b e s qmod 2`
and `D1 a k dmod 3` silently lose their area factor. The device still
parses, the simulation still runs, and the currents are wrong by the area
ratio. Nothing is printed.

**Silent loss of output vectors.**

```c
/* src/frontend/inp.c:2454, inp_savecurrents() */
        switch (devline[0]) {
        case 'm': ... case 'j': ... case 'q': ... case 'd':
        case 'r': case 'c': case 'l': case 'b': case 'f':
        case 'g': case 'w': case 's': ... case 'i': ...
        default:
            continue;
```

Structurally identical to the `switch` at `inpcom.c:9802` that
`doc/codex/issues/0009` did fix, in a different file. With
`.options savecurrents` and upper-case device letters no `.save @dev[...]`
line is generated at all, so every terminal-current vector is missing from
the saved plot.

**Silent misclassification of the circuit.**

```c
/* src/frontend/inp.c:2030, cktislinear() */
            firstchar = *dd->line;
            switch (firstchar) {
                case 'r': case 'l': case 'c': case 'i': case 'v':
                case '*': case '.': case 'e': case 'g': case 'f':
                case 'h': case 'k':
                    continue;
                default:
                    ckt->CKTisLinear = 0;
```

An all-upper-case RLC deck is declared non-linear under `preserve`.

**A command that silently does nothing.**

```c
/* src/frontend/inp.c:1828, com_alterparam() */
                            if (*xline == 'x') {
```

`alterparam` walks `ft_curckt->ci_mcdeck` for the `X` instance whose
subcircuit parameter is being altered. Under `preserve` an upper-case `X1`
card is never found and the command is a no-op.

**One more keyword-search asymmetry of the `doc/codex/issues/0013` shape.**

```c
/* src/frontend/inpcom.c:5108, inp_fix_param_values() */
            if (ciprefix(".meas", line))
                if (((equal_ptr[1] == 'v') || (equal_ptr[1] == 'i')) &&
                        (equal_ptr[2] == '(')) {
```

The outer gate is case-insensitive and the inner one is not, so
`.MEAS TRAN t1 WHEN V(1)=V(2)` loses the exemption that stops
`inp_fix_param_values()` brace-quoting the right-hand side. It is the same
shape as `doc/codex/issues/0018` in the same file; `doc/codex/issues/0009`'s
table lists only this function's `*line == 'b'` row.

**And one more first-character site in the same family.**

```c
/* src/frontend/inpcom.c:2755, replace_freq() */
    if (expr[0] == 'v' && expr[1] == '(' && expr_e[-1] == ')') {
```

`doc/codex/issues/0009`'s row 2711 covers the *identifier* branch of the same
`if`/`else` pair, which is fixed; this is its `v(...)` sibling, so
`E1 2 0 FREQ {V(1)} = DB ...` still takes neither branch. The
`tests/xspice/case/` harness added for row 2711 is where its deck belongs.

## Impact

`doc/codex/issues/0009` and `doc/codex/issues/0013` between them brought the
differential sweep to `PARSE-FAIL` = 0 and `NUM-DIFF` = 0. None of the sites
above shows up there, and that is a coverage statement rather than a safety
one:

- the two `inpcompat.c` area sites need `-D ngbehavior=ps`, which the sweep
  never passes;
- `inp_savecurrents()` needs `.options savecurrents`, which no deck under
  `tests/` sets;
- `cktislinear()` changes an internal flag, not a printed number;
- `com_alterparam()` needs an `alterparam` command;
- `inpcom.c:5108` needs a `.meas` card with `V(` on the right of a `WHEN`;
- `inpcom.c:2755` needs the XSPICE FREQ table form, which needs a code model.

The two area factors are the ones that matter: they are wrong numbers with
no diagnostic, which is the class this whole series has been trying to rule
out.

## Root Cause

The same load-bearing reader fold recorded in
`doc/claude/code_analysis/case-insensitivity-origins.md`, and the same
enumeration gap recorded three times now:
`doc/claude/checklists/phase1-keyword-case-census.md` does not count
character-literal comparisons, `doc/codex/issues/0009` scoped itself to a
card's first character in one file, and `doc/codex/issues/0013` scoped itself
to searches hidden behind a helper.

`src/frontend/inp.c` and `src/frontend/inpcompat.c` were swept for the first
time in the commit series that closed `doc/codex/issues/0009`'s table, and
only for the four sites `doc/codex/issues/0013` had already named. These six
are what a full sweep of those two files finds.

## Acceptance Criteria

1. Each site folds the byte it tests. `elem_letter()` is the in-tree helper
   and is now declared in `src/frontend/inpcom.h`, so `inp.c` and
   `inpcompat.c` can use it.
2. `tests/regression/case-pspice/` gains a twin pair per area site, with the
   device card in an `.inc` file — `pspice_compat()` runs on the card list of
   an included file, not on the deck — asserting a printed current that
   depends on the area factor.
3. `tests/regression/case/` gains a twin pair for `inp_savecurrents()`, using
   `.options savecurrents` and printing a terminal current vector, and one
   for `inpcom.c:5108`, printing a `.meas` result.
4. `tests/xspice/case/` gains a twin pair for `inpcom.c:2755`.
5. `com_alterparam()` and `cktislinear()` are folded with an explicit
   argument if no numeric witness can be built;
   `tests/regression/case/alter-case.cir` is the precedent for driving a
   shell command from a deck in that directory.
6. `make check` unchanged with `casemode` unset, and the differential sweep
   shows no deck's verdict getting worse.

## Resolution

Not fixed.

### One site that looks like this and is dead

```c
/* src/frontend/inp.c:2784, rem_unused_mos_models() */
        if (*curr_line == 'm') {
```

The function, its prototype and its only call are all inside `#ifdef
REM_UNUSED` (`inp.c:61`, `:1084`, `:2682`), and `REM_UNUSED` is defined
nowhere in `src/`. Do not fold it; it is recorded here so the next sweep
does not re-find it.

### One site that was fixed rather than recorded

`inp_modify_exp()` (`inpcom.c:7804`) is the same class and was found the same
way, and it is fixed in `48b72c7b2` because it blocked the decks for two rows
of `doc/codex/issues/0009`'s own table. It has its own write-up as
`doc/codex/issues/0018`.

### How this list was produced

An adversarial completeness pass over `src/frontend/inpcom.c`,
`src/frontend/inp.c` and `src/frontend/inpcompat.c`, run against the combined
per-site analyses of the commit series that closed
`doc/codex/issues/0009`'s follow-up table, scanning for `X == '<lower>'`,
`X != '<lower>'`, `switch` on a character with lower-case case labels,
`strchr`/`strpbrk`/`strspn` literal sets and `'a'..'z'` range tests. Every
line quoted above was re-read by hand at `b10fdafb4` before being written
down.
