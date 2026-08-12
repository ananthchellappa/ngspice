# Issue: A Control Variable Is Set Case-Insensitively and Read Case-Sensitively

## Status

Open. Found by the census behind
`doc/claude/decisions/0011-identity-lint.md`, which had to classify every
comparison of two runtime strings in `src/`; `src/frontend/variable.c` is the
single densest file in that census, with 19 of the 274 sites. Mode
independent and pre-existing. Named as unfiled in `0009` deferral 5 and in
`0048`'s session prompt; this is the write-up.

## Summary

The name of a control-language variable is compared two different ways
depending on which half of the front end is asking.

**Dispatch** — `cp_usrset()` (`src/frontend/options.c`) decides what a `set`
means by comparing the stored name against a literal with `eqc()`, which is
`cieq()`:

```c
    } else if (eqc(var->va_name, "numdgt")) {          /* options.c:355 */
```

**Storage and read-back** — `cp_vset()` (`src/frontend/variable.c:100`) and
`cp_getvar()` (`:695`, `:700`, `:705`, `:710`) compare the name against the
stored `va_name` with `eq()`, which is `strcmp()`:

```c
        if (eq(copyvarname, v->va_name)) {             /* variable.c:100 */
```

So a variable set as `NUMDGT` reaches its dispatch arm and works, and a
variable set as `WIDTH` is stored under that spelling and is then invisible to
every `cp_getvar("width", ...)` in the tree — `postcoms.c:206` and `:282`
(`print`), `device.c:404` and `:566`, `agraf.c:70`, `com_ghelp.c:87`,
`terminal.c:103`.

Measured at `6e2dca123` through `ngspice -p` on stdin:

```
--- default ---
in = 1.000000e+00
--- after set NUMDGT=12 ---
in = 1.000000000000e+00          <- honoured, through options.c's eqc()
```

```
--- after set WIDTH=200 ---
Index   time            in              mid             time
--------------------------------------------------------------------------------
                                                        (80 columns, split in two)
--- after set width=200 ---
Index   time            in              mid             time      v1#branch
------------------------------------------------------------------------------ ...
                                                        (200 columns, one block)
```

`set` with no arguments lists `WIDTH` and `width` as two entries after both
have been set, which is the same fact stated by the front end itself.

## Impact

An option the user sets in the wrong case is **silently** ignored, and which
options those are is not predictable from anything the user can see: it
depends on whether the option is implemented in `cp_usrset()`'s `eqc()` chain
or read back with `cp_getvar()`. `numdgt` works, `width` does not.

Not reachable from a deck. The reader lower-cases a `.control` line before
`com_set()` sees it, so `set WIDTH=200` inside a deck arrives as
`set width=200`. It is reachable from the **interactive prompt**, from
`ngspice -p` on stdin, and from `ngSpice_Command("set ...")` — the same three
routes `doc/codex/issues/0033` names, and for the same reason: the reader's
fold covers only text that arrives through `inp_readall()`.

Mode independent. `fold`, `preserve` and `distinguish` behave identically,
because no `casemode` predicate is on any of these paths.

## Root Cause

Two name spaces were conflated. A control variable can be either

- an **option**, a name the language defines — `width`, `numdgt`, `filetype`,
  `sourcepath` — which the spec's compatibility contract point 2 makes
  case-insensitive in all three modes, and which `options.c` already treats
  that way; or
- a **user variable**, a name the user invented for `$foo` substitution, which
  is an identifier and belongs to whatever rule `casemode` sets.

`cp_vset()` and `cp_getvar()` serve both and were written for the second, with
`eq()`. `cp_usrset()` serves only the first and was written with `eqc()`.
Nothing reconciles them, and the front end's own option table
(`src/frontend/miscvars.c:99`, which lists `numdgt` among the names `set` will
complete and describe) is not consulted by either.

## Acceptance Criteria

1. An option name is matched case-insensitively wherever it is stored and read
   — at minimum `cp_getvar()` finds a variable set under any spelling of an
   option name — so that `set WIDTH=200` and `set width=200` are the same
   request in all three modes.
2. Whether a *user* variable name stays byte-exact is decided rather than
   inherited. It is an identifier by `doc/claude/decisions/0001-distinguish.md`
   decision 3's rule, so under `preserve` two spellings should arguably be one
   variable; today they are two in every mode. Deciding it needs the list of
   which names are options, which `miscvars.c` has and neither comparison
   reads.
3. A deck asserts it. `tests/regression/pipe/` is the shape: the defect is not
   reachable from a `.cir` because the reader folds the control line, and
   `tests/regression/pipe/shell-keyword-case.cmd` already feeds `ngspice -p`
   on stdin.
4. `make check` unchanged in all three modes; the fix is mode independent and
   must move none of them.

## Resolution

Not fixed. The lint's census is what found it and the lint does not fix what
it finds; `src/frontend/variable.c`'s nineteen sites are in
`tests/lint/identity.baseline` with this issue named in that file's header.

Of the nineteen, eight were read as comparisons of a *user-chosen* name, eight
as comparisons of a *language-defined* one and three as neither — and the two
independent reads the census ran disagreed on eight of them, every
disagreement being in this file. That disagreement is the issue restated: a
reader cannot tell which name space a `cp_getvar()` call is in, because the
same `eq()` serves both.
