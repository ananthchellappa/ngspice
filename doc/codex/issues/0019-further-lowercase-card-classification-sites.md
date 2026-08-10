# Issue: Six More Lower-Case Card-Classification Sites, Four of Them Outside `inpcom.c`

## Status

Closed. All six sites are folded, and so is a seventh —
`src/frontend/inpcompat.c:1771` — that was a footnote when this issue was
written and is confirmed here. Every one of the seven carries a twin pair
whose upper-case deck was RED before the fix; see Resolution.

**One claim in the Summary below is wrong and is corrected in place.** The
two `pspice_compat()` area sites are *not* wrong-number defects in the shape
described: `INP2Q` and `INP2D` already apply an unlabelled trailing number to
`area` themselves, so an upper-case `Q1 c b e s qmod 2` does **not** lose its
area factor. What those two sites really cost is the substrate-bracket
rewrite (Q) and keyword precedence (D). The genuinely silent wrong-number
defects of this issue turned out to be the Q substrate rewrite and
`ltspice_compat()`'s `noiseless` translation.

## Summary

Every site below classifies a card by comparing a byte to a lower-case
character literal, and is correct only because `inp_read()` folded the card
first. That is `doc/codex/issues/0009`'s defect class exactly. None of them
appears in any of that issue's three follow-up tables, and its "Method note
on this enumeration" explains why: the enumeration was scoped to
`src/frontend/inpcom.c` and to a card's *first* character, and four of these
six are in other files.

Line numbers are against `b10fdafb4`.

**The two `pspice_compat()` sites.**

```c
/* src/frontend/inpcompat.c:1101, pspice_compat() */
        if (*cut_line == 'q') {
```
```c
/* src/frontend/inpcompat.c:1153, pspice_compat() */
        else if (*cut_line == 'd') {
```

~~These two append `area=<n>` to a BJT or diode instance when the token after
the model name is a bare positive number, which is how PSpice spells an area
factor. Under `-D casemode=preserve -D ngbehavior=ps`, `Q1 c b e s qmod 2`
and `D1 a k dmod 3` silently lose their area factor. The device still
parses, the simulation still runs, and the currents are wrong by the area
ratio. Nothing is printed.~~

**Struck: the area half of that claim is false, measured.** The parser
applies an unlabelled trailing number to `area` by itself —
`src/spicelib/parser/inp2q.c:113-124` and `src/spicelib/parser/inp2d.c:102-110`
call `INPpName("area", leadval)` whenever `PARSECALL` reports `waslead` — so
the appended keyword is redundant for the plain PSpice form. Measured under
`-D casemode=preserve -D ngbehavior=ps`: `Q1 2 1 0 qmod 4` and
`q1 2 1 0 qmod area=4` both give `i(V2) = -2.26814e-04`, and
`D1 1 2 dmod 3` matches `d1 1 2 dmod area=3` at `i(V2) = 1.701104e-02`.
The same holds for the `{...}` arm and for a numeric fourth node.

What the two sites really cost:

- **Q (`:1101`) — a wrong number, silently.** The same block strips the
  brackets PSpice requires around a fourth (substrate) node, so an upper-case
  `Q1 2 1 0 [3] qmod 4` binds its substrate terminal to a node literally
  named `[3]` and leaves node 3 unconnected. The run completes and prints a
  different voltage.
- **D (`:1153`) — keyword precedence.** The parser writes the positional
  value to `area` *after* the keyword loop has walked the card, whereas
  `area=` takes part in that loop. On a card that names the area twice the
  two spellings disagree; on the plain PSpice form they do not.

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

**A seventh site, a footnote when this issue was written, confirmed real.**

```c
/* src/frontend/inpcompat.c:1771, ltspice_compat() */
        else if (*cut_line == 'r') {
            char* noi = cistrstr(cut_line, "noiseless");
```

LTspice's `noiseless` resistor keyword is translated to ngspice's `noisy=0`
here. Under `-D casemode=preserve -D ngbehavior=lt` an upper-case `R` card
misses the translation, the keyword survives into the parser, and the
resistor keeps its thermal noise. Measured on a two-resistor divider:
`n = 9.103864e-08` against the correct `6.437404e-08`, with nothing on
stdout or stderr to say so. That makes it the second silent wrong-number
site of this issue, alongside the Q substrate rewrite.

## Impact

`doc/codex/issues/0009` and `doc/codex/issues/0013` between them brought the
differential sweep to `PARSE-FAIL` = 0 and `NUM-DIFF` = 0. None of the sites
above shows up there, and that is a coverage statement rather than a safety
one:

- the two `pspice_compat()` sites need `-D ngbehavior=ps`, which the sweep
  never passes;
- `ltspice_compat()`'s `noiseless` site needs `-D ngbehavior=lt`, likewise;
- `inp_savecurrents()` needs `.options savecurrents`, which no deck under
  `tests/` sets;
- `cktislinear()` changes an internal flag, and the flag reaches stdout only
  through `.options noopac` plus `.options keepopinfo`;
- `com_alterparam()` needs an `alterparam` command;
- `inpcom.c:5108` needs a `.meas` card with `V(` on the right of a `WHEN`;
- `inpcom.c:2755` needs the XSPICE FREQ table form, which needs a code model.

Two of the seven are wrong numbers with no diagnostic, which is the class
this whole series has been trying to rule out: the Q substrate rewrite
(`inpcompat.c:1101`) and the `noiseless` translation (`inpcompat.c:1771`).
The area factor, which this issue originally named as the wrong-number
mechanism, is not one of them — see the correction in the Summary.

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
2. `tests/regression/case-pspice/` gains a twin pair per `pspice_compat()`
   site, with the device card in an `.inc` file — `pspice_compat()` runs on
   the card list of an included file, not on the deck — asserting a printed
   number that depends on the rewrite. (Written as "depends on the area
   factor"; that is not what either rewrite actually moves, see the Summary
   correction.)
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

Fixed. Seven sites folded, seven twin pairs, one commit per mechanism. Line
numbers below are *at the fix*; every single-token edit left the line number
unchanged, and `inpcom.c:5108` grew by one line without moving its own head.

| site at the fix | edit | deck that witnesses it |
| --- | --- | --- |
| `inpcompat.c:1101` | `elem_letter(cut_line) == 'q'` | `tests/regression/case-pspice/pspice-q-substrate-case.cir` + twin |
| `inpcompat.c:1153` | `elem_letter(cut_line) == 'd'` | `tests/regression/case-pspice/pspice-diode-area-case.cir` + twin |
| `inpcompat.c:1771` | `elem_letter(cut_line) == 'r'` | `tests/regression/case-lt/noiseless-r-case.cir` + twin |
| `inp.c:1828` | `elem_letter(xline) == 'x'` | `tests/regression/case/alterparam-x-case.cir` + twin |
| `inp.c:2030` | `firstchar = elem_letter(dd->line)` | `tests/regression/case/noopac-linear-case.cir` + twin |
| `inp.c:2454` | `switch (elem_letter(devline))` | `tests/regression/case/savecurrents-case.cir` + twin |
| `inpcom.c:2755` | `tolower_c(expr[0]) == 'v'` | `tests/xspice/case/freq-v-expr-case.cir` + twin |
| `inpcom.c:5108` | `tolower_c(equal_ptr[1])` ×2 | `tests/regression/case/meas-when-vnode-case.cir` + twin |

`elem_letter()` for a card's leading device letter, bare `tolower_c()` for
the two bytes that are not one: `inpcom.c:2755`'s `expr` points into the
middle of a FREQ source's control expression and `inpcom.c:5108`'s
`equal_ptr[1]` is the byte after an `=` in the middle of a `.meas` card.
`elem_letter()` would be a lie about what is being read in both, which is the
convention `doc/codex/issues/0018` set.

### RED evidence, per site

Every upper-case deck was run through `tests/bin/check.sh` before the fix and
failed, and every lower-case twin passed. Every deck was also run against a
zero-byte reference and failed, which is the check the filter makes
necessary — none of the seven asserts a diagnostic.

| deck | before the fix | reference |
| --- | --- | --- |
| `pspice-q-substrate-case` | `v(3) = 2.000000e+00` — substrate bound to a node named `[3]` | `v(3) = 1.595830e+00` |
| `pspice-diode-area-case` | `v(2) = 3.971803e-01` — positional area 3 overrode `area=5` | `v(2) = 4.095966e-01` |
| `noiseless-r-case` | `n = 9.103864e-08` — R1 still noisy | `n = 6.437404e-08` |
| `alterparam-x-case` | `3.000000e+00` twice — the command was a no-op | `3.000000e+00` then `2.000000e+00` |
| `noopac-linear-case` | `v(2) = 6.666667e-01` — the OP ran despite `noopac` | `v(2) = 0.000000e+00` |
| `savecurrents-case` | `rlen = 1`, `rmin = 1e-3`, `dlen = 1` — no `.save` emitted | `rlen = 6`, `rmin = 0`, `dlen = 6` |
| `freq-v-expr-case` | two extra vectors, `b_gen_1#branch` and `gen_node_1` | four vectors, direct connection |
| `meas-when-vnode-case` | nothing on stdout; numparam died on `{V(2)}` | `tx = 4.00000e-01` |

Three of the eight rows deserve their own note.

- **`pspice-diode-area-case` needs the area named twice.** With only the bare
  number on the card the two spellings agree to every printed digit, because
  the parser applies it either way. The deck therefore carries
  `D1 1 2 dmod 3 area=5`: without the rewrite the positional 3 is written to
  `area` after the keyword loop and wins; with it the card becomes
  `D1 1 2 dmod  area=3 area=5` and the later keyword wins. That precedence is
  the whole observable consequence of this site.
- **`noopac-linear-case` needs `keepopinfo` as well as `noopac`.** The flag
  `cktislinear()` sets has exactly one consumer,
  `CKTnoopac = TSKnoopac && CKTisLinear` (`cktdojob.c:109`), and its only
  effect is to skip `CKTop()` in `acan.c:133`, `noisean.c:199` and
  `span.c:454`. For a deck `cktislinear()` accepts, the AC solution does not
  depend on the operating point, so `noopac` alone asserts nothing —
  measured, identical `mag(v(2))` in all four runs. `keepopinfo` makes `acan`
  dump the AC operating-point plot whether or not `CKTop()` ran, which turns
  the skipped OP's zeroed `CKTrhsOld` into a printed float. The one line the
  flag prints by itself, `" Linear circuit, option noopac given: no OP
  analysis"` (`acan.c:146`), contains `nalysis` and is deleted by
  `tests/bin/check.sh`'s filter.
- **`freq-v-expr-case` asserts a netlist shape, not a wrong number, and no
  wrong number is reachable at that site.** Missing the `v(` branch makes
  `replace_freq()` take its B-source input arm instead of connecting the A
  device's port to the node directly. That buffer is ideal and the analog
  input port draws no current, so the node voltages are identical; what
  changes is that two extra vectors exist. `print all` on a deck whose every
  printed name is case-free (numeric nodes, a current-source drive so there
  is no `V1#branch`, a numeric element suffix so the generated names are
  literal lower case) is what makes that visible and keeps the twins
  byte-identical. Single-node, differential, named-node, high-impedance and
  in-subcircuit variants were all measured and all agree between spellings.

### Not fold-mode visible, and the argument is per site

None of the seven changes anything in the default mode, and the argument is
the same one `doc/codex/issues/0009` made, checked again per site: an
ordinary device card matches none of `inp_read()`'s fold exemptions — the
`.lib`/`.inc` card itself (not the cards read out of the included file), the
`.control`/comfile command whitelist, the `print`/`plot` redirection and
title tails, and the CIDER/XSPICE `.model` arms — so it reaches all seven
passes already lower case. `cktislinear()` additionally runs on a deck from
which the control section has been removed. Measured rather than asserted:
every upper-case deck above passes against its committed reference with no
`-D casemode` flag, which is the same thing said as a number.

### Acceptance criteria

1. **Met** for all seven sites.
2. **Met**, with the correction above: the numbers those two decks assert
   depend on the substrate rewrite and on keyword precedence, not on the area
   factor being lost.
3. **Met.** `savecurrents-case` and `meas-when-vnode-case`, with twins.
   `savecurrents-case` prints `length()` and `minimum()` of the saved vector
   rather than the vector itself — `print @R1[i]` answers from a live
   instance-parameter query even when nothing was saved, so an op-only deck
   would have asserted nothing, and the vector's own name carries the
   device's spelling and could not appear in a shared reference.
4. **Met.** `freq-v-expr-case` and twin.
5. **Exceeded.** Both have numeric witnesses; neither needed the argument the
   criterion allowed for.
6. **Met.** See the report below.

### `make check` and the differential sweep

`make check` with `casemode` unset: **188 tests, 0 FAIL**, against a
baseline of 172 at `62d5d15b7`. The difference is exactly the sixteen decks
this issue adds — eight twin pairs, four under `tests/regression/case/`, two
under `tests/regression/case-pspice/`, one under `tests/regression/case-lt/`
and one under `tests/xspice/case/`.

`doc/claude/scripts/case_differential_sweep.py --jobs 12 --timeout 90`:

| verdict | before | after |
| --- | ---: | ---: |
| OK | 157 | 173 |
| DIFF | 53 | 53 |
| PARSE-FAIL | 0 | 0 |
| NUM-DIFF | 0 | 0 |
| SKIP | 3 | 3 |
| **total** | **213** | **229** |

Compared per deck rather than on the totals — a straight `diff` of the two
runs' non-OK lists — **no pre-existing deck moved at all**, and all sixteen
new decks report OK. `PARSE-FAIL` and `NUM-DIFF` are still 0, and the twelve
new-case decks that report `DIFF` for `doc/codex/issues/0020`'s two gaps are
still exactly twelve.

Two of the eight pairs had to be written around `doc/codex/issues/0020`'s
control-language gap to get there: `setplot noise2` and `setplot op1` do not
resolve in the sweep's uppercased copy, because plot names are generated in
lower case. The noise pair drops `setplot` (the analysis already leaves
`noise2` current) and the `noopac` pair uses `setplot previous`. That is
recorded in `doc/claude/checklists/phase2-differential-sweep.md`.

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
