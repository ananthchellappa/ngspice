# Phase 2 prerequisites — decision record

`doc/claude/suggestions/case-sensitive-identifiers-plan.md` closes with six
prerequisites, "absent from every design reviewed", to be opened before code is
written. This file closes or explicitly defers each one. Nothing here is left
implicit; a deferral says what breaks.

## 1. `visualc/*.vcxproj` (x3) — CLOSED, no action needed

All thirteen MSVC project files enumerate their sources explicitly. There are
no wildcards anywhere; `vngspice.vcxproj` carries 1481 `ClCompile` entries and
542 `ClInclude` entries, `vngspice-fftw.vcxproj` the same, `sharedspice.vcxproj`
1477 and 539.

Phase 2 added **no new `.c` file**, which is why the mode lives in
`src/frontend/inpcom.c` next to the fold it gates rather than in a file of its
own. Every source it touches is already listed. The enum and the two accessors
went into the existing `src/include/ngspice/fteext.h`; a `ClInclude` entry is a
passive IDE listing and compilation resolves through
`AdditionalIncludeDirectories`, which already contains `..\src\include`, so an
addition to an existing header needs no project edit either.

The obligation stands for anyone who later adds a file: it must go into all
three top-level projects by hand, independently, and there is no CI in the tree
to catch the omission.

## 2. `man/man1/ngspice.1`, `NEWS`, and the manual

**Man page — CLOSED.** `-D`/`--define` was in `src/main.c` (option table at
`:954`, handler at `:984`, help text at `:747`) and undocumented. It is the sole
command-line entry point for `casemode`, so it is now documented in the OPTIONS
section, between `-o` and `-p`, in the existing `.TP` style.

Not fixed, and noted here so it is not rediscovered as a regression: the same
page still omits `-f/--version-full`, `--version-small`, `-q/--completion` and
`--soa-log`, and documents `-t`'s long form as `--term=` where `src/main.c:969`
registers `--terminal`.

**NEWS — CLOSED.** A bullet was added to the Ngspice-47 block in the existing
4-space `+ ` / 6-space continuation style. For a feature whose only interface is
a variable, the NEWS bullet is the discovery mechanism.

**`src/spinit.in` — CLOSED.** Ships a commented `*set casemode=preserve` as the
site-default surface, next to the other commented settings.

**The user manual — DEFERRED, with a stated consequence.** It is not in this
repository. `doc/` contains only analysis artifacts and is not referenced by any
`Makefile.am`; there is no `.tex`, `.texi` or `.dbk` anywhere in the tree, and
`man/man1/ngspice.1:28` points at an externally hosted PDF. The manual chapter
has to be filed separately upstream, which means the feature ships undocumented
in the manual for at least one release cycle. That is a limitation, not a task
anyone in this repository can schedule.

## 3. `sharedspice.h` contract note — CLOSED

Added to `src/include/ngspice/sharedspice.h`, in the comment block that already
documents each entry point.

The asymmetry it records was already true before this work and documented
nowhere. `ngGet_Vec_Info` (`sharedspice.c:1194` -> `vec_get` -> `findvec`)
lowercases both the query and the table key (`vectors.c:183`, `:71`), and
`ngSpice_Raw_Evt` folds too (`evtplot.c:83`). `ngGet_Evt_NodeInfo`
(`sharedspice.c:1441` -> `EVTshareddata` -> `get_index`) compares with plain
`strcmp` at `evtshared.c:253`. So one half of the public API accepts any case
and the other half accepts exactly one. A caller's only safe rule is to use the
string `ngSpice_AllEvtNodes` returned.

The note also states how libngspice selects the mode, which the plan lists as an
open item. There is no argv: the `*ng_script_with_params` block at
`src/frontend/inp.c:648-703` is `#ifndef SHARED_MODULE`, and `Copy_of_argv` is
only ever assigned in `src/main.c:1489`. The mechanism is therefore

```c
ngSpice_Command("set casemode=preserve");   /* before ngSpice_Circ / source */
```

and it is guaranteed to be seen, because `ngSpice_Command` returns immediately
unless `is_initialized` (`sharedspice.c:1141`), which is set at `:1090` after
both `ft_cpinit()` and the `.spiceinit` block; `runc()` executes a non-`bg_`
command synchronously (`sharedspice.c:723`); and `set_case_mode()` runs at the
top of every `inp_readall()`, which `ngSpice_Circ` and `source` both reach later.
One caveat, which the note states: control-variable names are matched with `eq`
(`variable.c:695`), so the name must be sent in lower case.

## 4. OSDI — CLOSED, decision: keep the fold, keep case-insensitive matchers

`src/osdi/osdiinit.c:74` stores a `strtolower`'d copy of every Verilog-A
parameter name and alias as the `IFparm` keyword. Verilog-A is case sensitive,
so two parameters differing only in case collapse to one keyword and the second
shadows the first in the linear scan. That is a latent correctness bug
independent of any deck, and it is not introduced or worsened by `casemode`.

The spec asks for a single answer rather than three. The answer is: **keep the
fold for both `fold` and `preserve`, and rely on case-insensitive matching**,
which is already in place — Phase 1 converted `find_model_parameter`
(`inpgmod.c:52`), `find_instance_parameter` (`inpgmod.c:66`) and
`inpdpar.c:30` from `strcmp` to `cieq` in commit `9ab04184b`. Consequently
`.model … Vth0=0.5` matches an OSDI model under `preserve`: `INPgetTok` does no
folding, so `parm` is literally `Vth0`, and `cieq("Vth0", "vth0")` is true.

Reasons for that choice, in order:

- Removing the fold would silently change behaviour for every existing OSDI
  deck, because the keyword table is the only thing those decks match against.
- `tests/` has no OSDI regression directory, so neither resolution is testable
  in-tree, and parameter case originates in a third-party compiled `.osdi`
  artifact the deck author does not control. An untestable change that alters
  existing behaviour is the worst of the three options.
- Terminal names are not affected either way: `osdiinit.c:102-104` aliases the
  descriptor string without copying or folding, and OSDI instance nodes bind
  positionally at `inp2n.c:108-116`, so terminal-name case is display-only.

The shadowing bug is left open deliberately. Closing it means detecting the
collision at load time, which is a diagnostic change with no deck to test it
against, and it belongs with `distinguish`, where Verilog-A's own case
sensitivity would start to matter.

## 5. CIDER and tclspice — OUT OF SCOPE, with what breaks

**CIDER** is not compiled here: `build-ver_50/src/include/ngspice/config.h:11`
has `/* #undef CIDER */`. Everything below is source reasoning, not measurement,
and cannot be exercised without reconfiguring with `--enable-cider`.

CIDER's own card, parameter and enumerated-value matching is already case
insensitive — `INPfindCard` and `INPfindParm` use `cimatch` (`inpgmod.c:614`,
`:642`), the parameterless card words and the value words use `cinprefix`
(`inpgmod.c:517-521`, `ciderlib/input/method.c:93,96`,
`ciderlib/input/mobility.c:169-196`) — so `preserve` breaks none of it and
arguably improves it: an unquoted `infile=/Path/To/x.dop`, which
`keep_case_of_cider_param()` fails to protect today, starts working.

What does break is filenames CIDER **constructs** from identifiers.
`numdset.c:164` builds `ic.<instname>` and `numddump.c:70`, `nbjtdump.c`,
`nbt2dump.c`, `nud2dump.c` and `nummdump.c` mirror it. Under `preserve`,
instance `D1` looks for `ic.D1` where every existing setup has `ic.d1`, and on
a case-sensitive filesystem it will not find it. This is declared
broken-under-`preserve` rather than fixed. The rule if it is ever fixed is the
one the spec gives for the XSPICE auto-bridge: fold the identifier once at the
point it is captured for filename construction, not at each `snprintf`.

One caveat that is **not** hypothetical: `keep_case_of_cider_param()` is guarded
`#if defined(CIDER) || defined(XSPICE)` (`inpcom.c:222`) and is also the reader
for XSPICE `.model` cards of `filesource`, `table2d`, `table3d`, `d_state`,
`d_source`, `d_process` and `d_cosim` (`inpcom.c:1824-1830`). XSPICE is compiled
here. Any future change to that helper has to be justified against the XSPICE
callers, not only the CIDER ones. Phase 2 does not touch it: the fold guard
wraps the whole chain, so under `preserve` the helper is simply not called.

**tclspice** is out of scope and needs no work, which is a stronger statement
than it sounds. `src/tclspice.c` has no argv path, no `cp_getvar` and no
`cp_setvar`; every command it exposes is a `cp_coms[]` entry registered as
`spice::<name>` and dispatched through `cp_evloop` (`tclspice.c:2563-2568`,
`:818`), and it reaches the reader only via `spice::source` ->
`com_source` -> `inp_readall`. So `spice::set casemode = preserve` works by the
same mechanism as the shared library, with the same two caveats: the variable
name must be lower case, and `_run` concatenates into a fixed `char buf[1024]`
(`tclspice.c:744`).

The reason tclspice appears on the prerequisite list at all is that a mode
implemented as a `main.c` command-line flag would have been invisible to it, and
to `sharedspice.c`, both of which link `main.c` but never call `main()`. A cp
variable is the only mechanism all three front ends share, which is the
argument for the design the spec chose rather than an argument about tclspice.

## Not on the plan's list, but closed here anyway

- **`.option` is structurally impossible as the switch.** `inp_getopts()` runs
  over an already-read, already-folded deck. Restated because it is the first
  thing a reviewer proposes.
- **Deck-local opt-in is impossible as specified.** The mode has to be known
  before the first line is folded, and the first line is read inside
  `inp_read()`'s loop. A title-line pre-scan would work for a file but not for
  `ngspice -p` on stdin nor for the shared library's `circarray` path.
- **Ordering rule, which needs documenting wherever the feature is
  documented.** The mode is read when the netlist is read, so the last writer
  before that point wins. `.spiceinit` is sourced after the `-D` getopt loop, so
  a `set casemode` there overrides `-D casemode`.
