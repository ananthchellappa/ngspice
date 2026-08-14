# Issue: A Rawfile `Option:` Line Reconfigures the Next Netlist Read

## Status

Closed 2026-08-13 by the narrowing described under Resolution: a plot's
environment no longer answers a variable read taken while a netlist is being
read. The repo owner decided criterion 3 in favour of **shape A** — the
`Option:` pair keeps being filed and keeps being readable, and only the
reconfiguration stops — and that decision is recorded there, not re-opened
here. Everything above the Resolution is the measurement as it stood
*before* the fix and is left as it was written; each claim that has moved is
re-measured under the matching criterion below.

Found by the xschem client integration written up in
`doc/claude/feedback/ngspice_upstream/FINDINGS.md`, whose finding 1 asks for the
case mode to be recorded in the raw header and proposes `Option:` as the
carrier, on the correct observation that an *unmodified* `ngspice-46` parses
that key while a new `Casemode:` key aborts the load. The observation holds; the
conclusion does not, because `Option:` is not inert metadata. Everything below
was re-measured at `720c8743a` against
`/home/qflow/dev/ngspice_test/build-ver_50/src/ngspice` (`ngspice-46+`, build
stamp `Wed Aug 12 19:28:37 UTC 2026`) and, where the baseline is named,
`/usr/local/bin/ngspice` (`ngspice-46`, no `casemode` support).

Every input named below is in `doc/claude/feedback/ngspice_upstream/repro/`.
`mix.cir` is present there in the working tree. The spliced raw headers are
built by `repro/hdr_variants.sh`, which is present there too and takes each
one from `ascii_raw.cir` plus a single named line; the `.raw` products are not
committed, because that directory's `.gitignore` excludes `*.raw` and every
one of them carries a wall-clock `Date:` and a build stamp. So the `Date:` and
`Command:` lines in the header pastes below are from the run that produced the
paste beside them and will differ after a regeneration; nothing else in them
moves. Every command below is quoted with the filter it was run with, so each
paste is the whole output of the line above it.

Mode independent and pre-existing. The mechanism has nothing to do with
`casemode` — `casemode` is one of the variables it can reach, and finding 1 is
what made it visible.

This issue owns the mechanism for the batch. `doc/codex/issues/0060` keeps one
measurement of it — that a read of `$casemode` can be answered by a file — and
hands the mechanism here (its criterion 6); nothing below needs to be decided
there. Two other siblings are load-bearing for pastes below and are cited at
the point of use: `0058` for what counts as a netlist read, `0059` for what
plot a bare `write` writes.

## Summary

`raw_read()`'s header loop parses an `Option:` line into the plot's variable
list (`src/frontend/rawfile.c:472`):

```c
        } else if (ciprefix("option:", buf)) {          /* rawfile.c:472 */
            s = SKIP(buf);
            NONL(s);
            if (curpl) {
                wl = cp_lexer(s);
                for (vv = curpl->pl_env; vv && vv->va_next;
                     vv = vv->va_next)
                    ;
                if (vv)
                    vv->va_next = cp_setparse(wl);      /* :481 */
                else
                    curpl->pl_env = cp_setparse(wl);    /* :483 */
            } else {
                fprintf(cp_err, "Error: misplaced Option: line\n");
            }
```

`cp_getvar()` searches `plot_cur->pl_env` as the third link of its fallback
chain (`src/frontend/variable.c:703`):

```c
    for (v = variables; v; v = v->va_next)               /* variable.c:694 */
        if (eq(name, v->va_name))
            break;
    if (!v)
        for (v = uv1; v; v = v->va_next)                 /* :699  cp_usrvars() */
            ...
    if (!v && plot_cur)                                  /* :703 */
        for (v = plot_cur->pl_env; v; v = v->va_next)    /* :704 */
            if (eq(name, v->va_name))
                break;
```

So a `cp_getvar()` consumer can be answered by a line in a file the user merely
*loaded* — any consumer whose name is not already in the global `variables`
list or in `cp_usrvars()`, the two links ahead of the plot environment, which
is the defence property 3 measures. `set_case_mode()` is such a consumer
(`src/frontend/inpcom.c:1085`), and it is called from `inp_readall()`
(`:1178`), i.e. once per netlist read — `doc/codex/issues/0058` owns what
counts as a netlist read and measures the count, and its finding that a
`source` is one while an `.include` is not is what makes the two-command
measurement below a *second* read rather than a continuation of the first:

```c
static void set_case_mode(void)                          /* inpcom.c:1085 */
{
    char mode[64];

    ng_case_mode = NG_CASE_FOLD;

    if (!cp_getvar("casemode", CP_STRING, mode, sizeof(mode) - 1))   /* :1091 */
        return;
```

**Measured.** `repro/hdr_option.raw` is `repro/ascii_raw.raw` with one line
spliced in after `Plotname:`, exactly as `repro/run_all.sh` finding 1 generates
it and as `repro/hdr_variants.sh` regenerates it:

```
$ cat hdr_option.raw
Title: * writes an ascii raw, so finding 1's header experiments can splice a line in
Date: Thu Aug 13 00:25:44  2026
Command: ngspice-46+, Build Wed Aug 12 19:28:37 UTC 2026
Plotname: Operating Point
Option: casemode=preserve
Flags: real
No. Variables: 2
No. Points: 1
Variables:
	0	v(in)	voltage
	1	i(vs)	current
Values:
 0	3.000000000000000e+00
	-3.000000000000000e-03
```

`repro/mix.cir` is an unrelated deck whose middle net is spelled `MidNode`:

```
* mixed-case net probe -- the net is spelled MidNode
Vs In 0 DC 3
Rl In MidNode 1k
Rg MidNode 0 3k
.control
op
display
.endc
.end
```

```
$ printf 'load hdr_option.raw\nsource mix.cir\nquit\n' \
      | build-ver_50/src/ngspice -p -n 2>&1 | grep -E 'Circuit:|: (voltage|current)'
    i(vs)               : current, real, 1 long
    v(in)               : voltage, real, 1 long [default scale]
Circuit: * mixed-case net probe -- the net is spelled MidNode
    In                  : voltage, real, 1 long [default scale]
    MidNode             : voltage, real, 1 long
    Vs#branch           : current, real, 1 long

$ printf 'source mix.cir\nquit\n' \
      | build-ver_50/src/ngspice -p -n 2>&1 | grep -E 'Circuit:|: (voltage|current)'
Circuit: * mixed-case net probe -- the net is spelled midnode
    in                  : voltage, real, 1 long [default scale]
    midnode             : voltage, real, 1 long
    vs#branch           : current, real, 1 long
```

The first two lines of the first paste are `load`'s own echo of the plot it
just read — `i(vs)` and `v(in)` are the raw file's own vectors, already folded
when *that* file was written, and not part of the claim. The claim is the four
lines after them, against the four lines of the second run. The deck is
byte-identical in both runs and no `casemode` was ever requested. The plot
title moves with the net names, which is the reader's fold and not just a
display choice.

`$casemode` reports it, on this build and on the featureless baseline alike:

```
$ printf 'echo before=$casemode\nload hdr_option.raw\necho after=$casemode\nquit\n' \
      | build-ver_50/src/ngspice -p -n 2>&1 | grep -E '^before=|^after='
before=
after=preserve
$ printf 'echo before=$casemode\nload hdr_option.raw\necho after=$casemode\nquit\n' \
      | /usr/local/bin/ngspice -p -n 2>&1 | grep -E '^before=|^after='
before=
after=preserve
```

Five properties of the mechanism, each measured:

**1. It is general, not `casemode`-specific.** `set_compat_mode()` sits on the
same two lines of `inp_readall()` (`inpcom.c:1176`) and reads the same way:

```
$ printf 'load hdr_ngb.raw\nsource mix.cir\nquit\n' \
      | build-ver_50/src/ngspice -p -n 2>&1 | grep -i compatibility
Note: Compatibility modes selected: hs
$ printf 'source mix.cir\nquit\n' \
      | build-ver_50/src/ngspice -p -n 2>&1 | grep -i compatibility
Note: No compatibility mode selected!
```

`repro/hdr_ngb.raw` differs from `hdr_option.raw` in one line:
`Option: ngbehavior = hs`.

**2. But only for options read *on demand*.** The `Option:` arm never calls
`cp_vset()` and therefore never reaches `cp_usrset()`'s dispatch chain, so an
option that `cp_usrset()` *latches* into a C global is out of reach. `numdgt`
is the example: `cp_usrset()` tests the name at `options.c:355` and copies the
value into `cp_numdgt` at `:357`/`:359`/`:361`, and `grep -rn 'cp_getvar("numdgt"' src/`
finds nothing — the global is the only reader. `repro/hdr_numdgt.raw` is
`hdr_option.raw` with `Option: numdgt = 12` in place of the `casemode` line,
and it measurably does nothing. The control is the same load with the same
value set through `set`, which does go through `cp_usrset()`:

```
$ printf 'load hdr_numdgt.raw\nprint v(in)\nquit\n' \
      | build-ver_50/src/ngspice -p -n 2>&1 | grep -E '^v\(in\)'
v(in) = 3.000000e+00
$ printf 'load hdr_option.raw\nset numdgt=12\nprint v(in)\nquit\n' \
      | build-ver_50/src/ngspice -p -n 2>&1 | grep -E '^v\(in\)'
v(in) = 3.000000000000e+00
```

The `Option:` line was nonetheless parsed and filed — it is only the latch it
cannot reach:

```
$ printf 'load hdr_numdgt.raw\necho dollar-numdgt=$numdgt\nquit\n' \
      | build-ver_50/src/ngspice -p -n 2>&1 | grep -E '^dollar-numdgt='
dollar-numdgt=12
```

So the set an `Option:` line can *act on* is "names some site passes to
`cp_getvar()` at the moment it needs them", which includes both of
`inp_readall()`'s two policy switches. The set it is *visible* in is larger:
`$name` substitution reaches the plot environment by a different route
(`cp_enqvar()`, `options.c:60`), which is what the `$numdgt` paste above and
every `$casemode` paste in this issue use, and `set` with no argument lists it
(`cp_vprint()`, `variable.c:1125` and `:1147`). Acceptance criterion 3 turns on
that difference.

**3. A pre-existing global wins, and that is the only defence.** `cp_getvar()`
walks `variables` first, and the `$` path does the same: `cp_variablesubst()`
(`variable.c:846`) evaluates each name through `vareval()` (called at `:866`),
which walks `variables` at `:1005` and only then falls back to `cp_enqvar()` at
`:1021`. So `-D` and `.spiceinit` are both above the plot environment:

```
$ printf 'load hdr_option.raw\necho dollar-casemode=$casemode\nsource mix.cir\nquit\n' \
      | build-ver_50/src/ngspice -p -n -D casemode=fold 2>&1 \
      | grep -E '^dollar-casemode=|Circuit:|(midnode|MidNode) +:'
dollar-casemode=fold
Circuit: * mixed-case net probe -- the net is spelled midnode
    midnode             : voltage, real, 1 long
```

The defence is only available to a caller that thought to set the variable
*before* the load. There is no way to set it afterwards, which is property 4.

**4. Once loaded it is read-only.** `cp_usrset()` returns `US_READONLY` for any
name present in `plot_cur->pl_env` (`options.c:417`), so the user cannot
override or remove it while that plot is current:

```
$ printf 'load hdr_option.raw\nset casemode=fold\necho now=$casemode\nsource mix.cir\nquit\n' \
      | build-ver_50/src/ngspice -p -n 2>&1 | grep -E '^Error|^now=|(midnode|MidNode) +:'
Error: casemode is a read-only variable.
now=preserve
    MidNode             : voltage, real, 1 long

$ printf 'load hdr_option.raw\nunset casemode\nquit\n' \
      | build-ver_50/src/ngspice -p -n 2>&1 | grep -E '^Error|^cp_remvar'
Error: casemode is read-only.
cp_remvar: Internal Error: var 99
```

The messages are `variable.c:188` and `:644`. The `99` on the last line is
incidental and worth naming so the paste is not mistaken for a second defect:
`variable.c:646` prints `*varname` — the first *byte* of the name, `'c'` — with
`%d`.

**5. It is plot-scoped, it re-arms, and it round-trips.** `plot_cur` moves to
the new plot when the sourced deck runs an analysis, so `$casemode` empties;
`setplot` back to the loaded plot restores it, and a second read is folded or
preserved accordingly:

```
$ printf 'load hdr_option.raw\necho after-load=$casemode\nsource mix.cir\necho after-source=$casemode\nsetplot op1\necho back-on-op1=$casemode\nquit\n' \
      | build-ver_50/src/ngspice -p -n 2>&1 \
      | grep -E '^after-load=|^after-source=|^back-on-op1='
after-load=preserve
after-source=
back-on-op1=preserve
```

And the raw *writer* re-emits every `pl_env` entry (`rawfile.c:132`-`:148`), so
the setting propagates to any file written from a plot that carried it — with
the key moved after `No. Points:` and re-spaced, and still live on reload.
`write` with no vector list writes whatever plot is current, which here is the
plot the `load` made current; that rule is `doc/codex/issues/0059`'s subject
and this paste depends on it:

```
$ printf 'load hdr_option.raw\nset filetype=ascii\nwrite rt.raw\nquit\n' \
      | build-ver_50/src/ngspice -p -n >/dev/null 2>&1
$ sed -n '1,9p' rt.raw          # the first 9 of 15 lines; Variables:/Values: follow
Title: * writes an ascii raw, so finding 1's header experiments can splice a line in
Date: Thu Aug 13 00:25:44  2026
Command: ngspice-46+, Build Wed Aug 12 19:28:37 UTC 2026
Plotname: Operating Point
Flags: real
No. Variables: 2
No. Points: 1
Option: casemode = preserve
Variables:
$ printf 'load rt.raw\nsource mix.cir\nquit\n' \
      | build-ver_50/src/ngspice -p -n 2>&1 | grep -E '(midnode|MidNode) +:'
    MidNode             : voltage, real, 1 long
```

`rawfile.c:481`/`:483` is the only code in the tree that puts anything *into* a
plot's environment. The grep that shows it has to be read with care, so here it
is with its whole output:

```
$ grep -rn "pl_env *=" src/
src/frontend/postcoms.c:1128:        printf("va: killplot should tfree pl->pl_env=(%p)\n", pl->pl_env);
src/frontend/postcoms.c:1157:        printf("va: killplot should tfree pl->pl_env=(%p)\n", pl->pl_env);
src/frontend/rawfile.c:483:                curpl->pl_env = cp_setparse(wl);
```

Three lines, and neither of the first two is an assignment: `postcoms.c:1128`
is a debug `printf` in `killplot()` (`:1072`) and `:1157` is the same `printf`
text in `destroy_const_plot()` (`:1136`) — only the string says `killplot`.
The pattern also cannot see `:481`, which appends to the list through
`vv->va_next = cp_setparse(wl)` rather than through `pl_env` itself, nor the
one indirect writer: `cp_remvar()` takes `p = &plot_cur->pl_env`
(`variable.c:600`) and unlinks with `*p = v->va_next` (`:630`) in its `US_OK`
arm. That arm is unreachable for a name that is in `pl_env`, because
`cp_usrset()` returns `US_READONLY` for exactly those names
(`options.c:415`-`:417`) — property 4 — so the conclusion stands, but it stands
on the read of the two functions and not on the grep alone. Nothing a normal
ngspice run does puts anything in a plot's environment, which is why the writer
at `:132` has been dead code in practice and why the round trip above is only
reachable from a hand-edited header.

### The adjacent `Command:` path, and why it is a separate issue

The same header loop executes a non-ngspice `Command:` line
(`src/frontend/rawfile.c:456`):

```c
        } else if (ciprefix("command:", buf)) {          /* rawfile.c:456 */
            /* Note that we reverse these commands eventually... */
            s = SKIP(buf);
            /* Exec command only if not ngspice simulator info */
            if (!ciprefix(ft_sim->simulator, s)) {
                ...                                      /* :461-:468 file the
                                                            line in pl_commands
                                                            or complain */
                /* Now execute the command if we can. */
                (void)cp_evloop(s);                      /* :470 */
            }
```

Measured, on both binaries — `repro/hdr_cmd.raw` is `hdr_option.raw` with the
`Option:` line replaced by `Command: echo COMMAND-LINE-EXECUTED`:

```
$ printf 'load hdr_cmd.raw\nquit\n' | build-ver_50/src/ngspice -p -n 2>&1 | grep COMMAND
COMMAND-LINE-EXECUTED
$ printf 'load hdr_cmd.raw\nquit\n' | /usr/local/bin/ngspice   -p -n 2>&1 | grep COMMAND
COMMAND-LINE-EXECUTED
```

It reaches the same variable by a stronger route, because `cp_evloop("set ...")`
writes the *global* `variables` list rather than `pl_env`, so the effect
survives the plot change that property 5 shows empties the environment.
`repro/hdr_cmdset.raw` is `hdr_option.raw` with the `Option:` line replaced by
`Command: set casemode=preserve`:

```
$ printf 'load hdr_cmdset.raw\nsource mix.cir\necho after-source=$casemode\nquit\n' \
      | build-ver_50/src/ngspice -p -n 2>&1 \
      | grep -E '^after-source=|(midnode|MidNode) +:'
    MidNode             : voltage, real, 1 long
after-source=preserve
```

**This path is deliberate and it is documented in this tree.**
`man/man1/ngsconvert.1:109`:

> Any number of **Command:** lines may appear between the **No. Points:**
> and the **Variables:** lines, and whenever the plot is loaded into
> **nutmeg** they will be executed.

That is the whole of the documentation — the manual page lists `Command:` in
the ASCII header grammar at `:89` and says it is executed on load. It says
nothing about `Option:`, which appears in no man page and in no `doc/` file
outside `doc/claude/` and `doc/codex/`. So the two lines are not symmetric: one
is a documented feature whose surprise is the user's to accept, the other is an
undocumented side effect of a key whose name promises inertness.

The two are kept separable here on purpose. Narrowing `Option:` needs no
decision about `Command:`, and revisiting `Command:` is a change to a
thirty-year-old documented behaviour that this issue does not ask for and must
not block on.

## Impact

**For the client that reported this, the consequence is specific: it
disqualifies `Option:` as the carrier for finding 1's ask.** Recording the case
mode as `Option: casemode=<mode>` after `Plotname:` was the cheap
implementation — one `fprintf` in `raw_write()`, parsed by every existing
reader including `ngspice-46`'s, no format break. It cannot be taken. A raw file
written that way would silently set the case mode of the *next* deck any session
that loaded it went on to read, would refuse to be overridden by a subsequent
`set` (property 4), and would propagate itself into every raw written from that
plot (property 5). A schematic editor that loads a year-old waveform before
re-simulating would get a different netlist than the one it asked for.
`doc/claude/batches/2026-08-12-casemode-client-feedback/LEDGER.md` already
carries F1 as deferred on exactly this ground; this issue is the measurement
behind that deferral.

Beyond that ask, the general exposure:

- Any `cp_getvar()` consumer whose name the session has not already set, not a
  fixed list — the qualification is property 3, and it is the whole of the
  defence. `set_case_mode()` and `set_compat_mode()` are the two consumers that
  change how a *netlist is parsed*, which is the sharp end; the rest change
  display and file handling.
- Silent. No diagnostic is printed when an `Option:` line is filed, in either
  binary. The only line the reader ever emits about `Option:` is
  `Error: misplaced Option: line`, and that fires when the line appears
  **before** `Plotname:` — so the diagnostic marks the case where nothing
  happens. `repro/hdr_option_early.raw` is `hdr_option.raw` with the same line
  moved one line up:

  ```
  $ printf 'load hdr_option_early.raw\necho cm=$casemode\nsource mix.cir\nquit\n' \
        | build-ver_50/src/ngspice -p -n 2>&1 \
        | grep -E 'misplaced|^cm=|Circuit:|(midnode|MidNode) +:'
  Error: misplaced Option: line
  cm=
  Circuit: * mixed-case net probe -- the net is spelled midnode
      midnode             : voltage, real, 1 long
  ```
- Reachable from `load` (`src/frontend/postcoms.c:107`, `:112`) and from the
  asynchronous-run result load (`src/frontend/aspice.c:238`). A bare `load` with
  no argument reads `ft_rawfile`, the default name, so the file need not even be
  named on the command line. In the `ngspice` binary a rawfile cannot be given
  on the command line — `main.c:1612`'s `ft_loadfile()` loop is inside the
  `#else` of `SIMULATOR` and belongs to `nutmeg`.
- Mode independent: `fold`, `preserve` and `distinguish` all read the mode
  through `cp_getvar()`, so the mechanism is the same in each. What differs is
  only which value the line can install.

**Class, for the taxonomy in `doc/claude/decisions/0001-distinguish.md`.** This
is not an identifier comparison and so is in none of the three classes; it is a
scoping defect. The two comparisons it *does* involve are each already
classified, and both behave correctly:

| what | comparator | measured |
| --- | --- | --- |
| the option **name** on the `Option:` line, against `"casemode"` | `eq()` = `strcmp`, called at `variable.c:705` inside the `pl_env` loop opened at `:704` | byte-exact: `Option: CaseMode=preserve` and `Option: CASEMODE=preserve` both fold; only `casemode` preserves |
| the option **value**, against `"preserve"` | `cieq()` (`inpcom.c:1096`) | case-insensitive: `casemode=PRESERVE` and `casemode=Preserve` both preserve |

The four rows of that matrix are `repro/hdr_optname_mixed.raw`,
`hdr_optname_upper.raw`, `hdr_optval_upper.raw` and `hdr_optval_mixed.raw`,
each `hdr_option.raw` with the one spelling substituted, run as
`load <file> ; source mix.cir`:

```
$ for f in hdr_optname_mixed hdr_optname_upper hdr_optval_upper hdr_optval_mixed; do
>   printf "%-20s -> " "$f"
>   printf "load $f.raw\nsource mix.cir\nquit\n" \
>     | build-ver_50/src/ngspice -p -n 2>&1 | grep -oE '(MidNode|midnode) +:' | head -1
> done
hdr_optname_mixed    -> midnode             :
hdr_optname_upper    -> midnode             :
hdr_optval_upper     -> MidNode             :
hdr_optval_mixed     -> MidNode             :
```

The value is a language-defined word and so is case-insensitive in all three
modes by the spec's compatibility contract point 2 — Class B, correct as
written. The name is `doc/codex/issues/0048`'s territory: a control variable
whose name `cp_usrset()` would have dispatched with `eqc()` is read back with
`eq()`, so the spelling that works is not predictable from anything the user can
see. That is 0048's defect reached through a new door, not a new one, and this
issue does not restate it — but it does mean a fix here must not "helpfully"
fold the name and thereby widen 0048.

Not reachable from any committed deck. No test in `tests/` contains an `Option:`
line; `grep -rln "Option:" tests/` is empty.

## Root Cause

`pl_env` is a nutmeg-era per-plot environment, and the raw header's `Option:`
key was its serialisation. In that design the two halves are coherent: the
options that mattered were about how *this plot* is displayed, the plot carried
them, and `raw_write()`'s loop at `rawfile.c:132` wrote them back out. Nothing
in the mechanism distinguishes "an option describing this plot" from "an option
controlling the simulator", because when it was written there was no second
category.

Two things then grew past it, independently:

1. `cp_getvar()` acquired a fallback chain in which `plot_cur->pl_env` is
   consulted for *any* name, not for a known set of display options. The plot
   environment thereby became a general variable namespace, session-wide in
   reach and file-supplied in origin, with no marker on the values saying where
   they came from.
2. `inp_readall()` acquired two policy switches read through that same chain —
   `set_compat_mode()` at `inpcom.c:1176` and `set_case_mode()` at `:1178` —
   whose subject is not a plot at all but the *next file to be parsed*.

The join of the two is the defect: a per-plot, file-supplied variable now
configures a global, one-shot decision about a different file. The `Option:` arm
also bypasses `cp_vset()`, so none of the validation, dispatch or read-only
policy that a `set` would go through applies on the way in — which is what
makes property 2's carve-out (latched options unreachable) and property 4's
asymmetry (unsettable on the way out) both true at once.

`set_case_mode()`'s own comment (`inpcom.c:1079`-`:1083`) frames the precedence
question as "the last writer before the deck is read wins", and enumerates the
writers it considered: the `-D` getopt loop, `.spiceinit` (which is sourced
after that loop and therefore overrides it), and libngspice callers going
through `ngSpice_Command("set casemode=...")`. A loaded raw file is a fourth
writer, later than all three, and it is not in that list.

## Acceptance Criteria

1. Loading a raw file does not change how the next netlist is parsed. The
   measurement at the top of this issue — same `repro/mix.cir`, with and
   without a preceding `load` — gives the same vector spellings in both runs.
2. Whatever narrowing is chosen is stated as a rule about *which* variables a
   file may set, not as a special case for `casemode`. `ngbehavior` is measured
   above and is the same defect; a fix that names only `casemode` leaves it.
3. Backward compatibility is decided explicitly, because `Option:` round-trips
   through `raw_write()` (`rawfile.c:132`) and a hand-written header may rely on
   it. The two shapes are: keep filing the pair but stop letting
   `cp_getvar()`'s plot-environment link answer the policy reads, or refuse the
   name at the `Option:` arm and say so. The first leaves `echo $casemode`
   answering `preserve` after a `load`, which is finding 1's *stated* ask —
   a consumer wants to read the mode out of the header — and only removes the
   reconfiguration; the second is louder and breaks the read-back too. This
   issue does not choose, but whoever does should read
   `doc/codex/issues/0060` first: the first shape manufactures a fresh instance
   of that issue's split, because after such a `load` the variable would answer
   `preserve` while the mode in effect is `fold`. On this one path the two
   agree today — the plot environment both reports the mode and sets it — so
   the divergence is a cost the first shape introduces and the second does not.
4. The `Command:` path is untouched by the fix. It is documented at
   `man/man1/ngsconvert.1:109` and executes on `ngspice-46` as well as on this
   build, both measured above; if it is to be revisited that is a separate
   issue with its own compatibility argument, and criterion 1 must be satisfied
   without waiting for it. Note that a fix satisfying criterion 1 through the
   `Option:` arm alone leaves `Command: set casemode=preserve` working, which is
   measured above — so criterion 1 should be read as "no *`Option:`* line
   changes the next netlist read", and the residue named in the issue that
   closes this one.
5. A deck asserts it. `tests/regression/pipe/` is the shape, for the same reason
   `doc/codex/issues/0048` gives: the sequence is `load` then `source`, which is
   two control-language commands and not a netlist. `tests/regression/casedist/vector-rawfile-scale-case.cir`
   is the precedent for a deck that writes the hand-built rawfile it then loads,
   which is needed here because no ngspice writer emits `Option:` unless it
   round-trips one. The evidence in this issue is not that deck yet: it is
   `repro/mix.cir` plus `repro/hdr_variants.sh`, which is enough to re-run every
   paste above by hand and is not enough to fail in `make check`.
6. `make check` unchanged in all three modes. No committed test contains an
   `Option:` line, so the fix should move no deck.

## Resolution

Closed 2026-08-13, **shape A**, as the repo owner decided it: the `Option:`
pair is still parsed, still filed into `plot_cur->pl_env`, and still readable
— `echo $casemode` after a `load` answers what the header recorded, which is
finding 1's actual ask — and what stops is that environment answering the
reads that steer parsing.

**The rule, and it is a rule about reads and not about names.** A plot's
environment describes a plot. It may answer a question about the session; it
may not answer a question asked on behalf of a file that is being turned into
cards. Every variable the reader consults inside that window is policy for
*the file in hand* — which case mode this deck is parsed under, which
compatibility dialect, where an `.include` comes from — and a loaded plot
answering any of them is answering about some other run. So the window is the
whole of `inp_readall()`, and no variable name appears in the predicate.
`casemode` and `ngbehavior` are both fixed by the same line, as are the
reads made further down (`no_auto_gnd`, `addcontrol`, `sourcepath`) and the
next policy read somebody adds; a predicate that knew the name `casemode`
would have had to learn `ngbehavior` next.

Five production edits:

- `src/frontend/inpcom.c` — `inp_readall()` becomes a five-line wrapper that
  raises a depth count around the read and lowers it after; the body is now
  `static struct card *inp_readall_cards(...)` with the same signature and no
  other change. Beside it, `inp_reading_netlist()` — the predicate — and
  `inp_netlist_read_reset()`. A count and not a flag so that a nested read
  cannot lower the guard on its way out; there is no nesting today (an
  `.include` is read by `inp_read()`, and a `.control` block is executed by
  `inp_spsource()` *after* `inp_readall()` returns, so nothing a user types
  is ever inside the window) but `inp_readall()` has three callers and one of
  them, the XSPICE auto-bridge, reads a deck ngspice writes itself.
- `src/frontend/variable.c`, `cp_getvar()` — the plot-environment link gains
  `&& !inp_reading_netlist()`. That link only; the `variables`, `cp_usrvars()`
  and `ft_curckt->ci_vars` links are untouched, and so is `cp_enqvar()`, which
  is what `$name` and `set`'s listing take.
- `src/include/ngspice/fteext.h` — the two declarations, beside `inp_readall()`.
- `src/frontend/signal_handler.c`, `ft_sigintr_cleanup()` — `inp_netlist_read_reset()`,
  because an interrupt during a `source` longjmps out of `inp_readall()`,
  which therefore never lowers its own guard, and this is where that jump
  lands.
- `src/sharedspice.c`, `totalreset()` — the same call, one line after the
  `inp_case_announce_reset()` that `doc/codex/issues/0058` put there, for a
  host that abandons a netlist read by the longjmp at `sharedspice.c:2193`.

One test file, `tests/regression/pipe/rawfile-option-parse-policy.cmd`, plus
its `TESTS` and `CLEANFILES` entries in that directory's `Makefile.am`. No
`.out` file was added or edited — this directory has none — and
`identity.baseline` is unchanged: the change adds no string comparison of any
kind (`make check` reports the same 265).

**Criterion by criterion.**

1. *Loading a raw file does not change how the next netlist is parsed.* The
   measurement at the top of this issue, re-run on the fixed binary with the
   same `repro/mix.cir` and the same `repro/hdr_option.raw`:

   ```
   $ printf 'load hdr_option.raw\nsource mix.cir\nquit\n' \
         | build-ver_50/src/ngspice -p -n 2>&1 | grep -E 'Circuit:|: (voltage|current)'
       i(vs)               : current, real, 1 long
       v(in)               : voltage, real, 1 long [default scale]
   Circuit: * mixed-case net probe -- the net is spelled midnode
       in                  : voltage, real, 1 long [default scale]
       midnode             : voltage, real, 1 long
       vs#branch           : current, real, 1 long

   $ printf 'source mix.cir\nquit\n' \
         | build-ver_50/src/ngspice -p -n 2>&1 | grep -E 'Circuit:|: (voltage|current)'
   Circuit: * mixed-case net probe -- the net is spelled midnode
       in                  : voltage, real, 1 long [default scale]
       midnode             : voltage, real, 1 long
       vs#branch           : current, real, 1 long
   ```

   The four lines that are the claim are now identical in both runs; the two
   lines above them are still the loaded file's own vectors.
2. *A rule about which variables a file may set, not a special case for
   `casemode`.* Stated above. `ngbehavior` is fixed by it with no mention:

   ```
   $ printf 'load hdr_ngb.raw\nsource mix.cir\nquit\n' \
         | build-ver_50/src/ngspice -p -n 2>&1 | grep -i compatibility
   Note: No compatibility mode selected!
   $ printf 'source mix.cir\nquit\n' \
         | build-ver_50/src/ngspice -p -n 2>&1 | grep -i compatibility
   Note: No compatibility mode selected!
   ```

   The deck asserts the same thing as a number rather than as a message,
   through `0**0`, which `PTpowerH()` (`src/spicelib/parser/ptfuncs.c:91`)
   evaluates to 1 in every mode but `hs`, where it is 0.
3. *Backward compatibility decided explicitly.* Shape A, the owner's
   decision. The pair is still filed by `rawfile.c:472`-`:483`, still listed
   by `set`, still answered through `$`, and still round-tripped by
   `raw_write()`. Measured, on the fixed binary:

   ```
   $ printf 'echo before=$casemode\nload hdr_option.raw\necho after=$casemode\necho effect=$curcasemode\nquit\n' \
         | build-ver_50/src/ngspice -p -n 2>&1 | grep -E '^before=|^after=|^effect='
   before=
   after=preserve
   effect=fold
   ```

   Those two answers are the cost this issue warned of, and it is **settled,
   not outstanding**: the two readings are two different questions and both
   are now answerable in the same session. `casemode` is the *request* — what
   somebody asked for, here what the header recorded — and `curcasemode` is
   the *effect*, the mode `inp_case_mode()` has in force, which
   `doc/codex/issues/0060` added and which is answered ahead of the plot
   environment for exactly this reason. Before `curcasemode` existed, shape A
   would have left a session that could report `preserve` with no way to
   discover it was folding; it no longer can. 0060's criterion 6 needed the
   same ordering and is where that half is asserted.
4. *The `Command:` path is untouched by the fix,* and its residue is named.
   Both lines still execute, and `Command: set casemode=preserve` still
   reconfigures the next read, because `cp_evloop("set …")` writes the global
   `variables` list, which is the first link of `cp_getvar()`'s chain and is
   not narrowed:

   ```
   $ printf 'load hdr_cmd.raw\nquit\n' | build-ver_50/src/ngspice -p -n 2>&1 | grep COMMAND
   COMMAND-LINE-EXECUTED
   $ printf 'load hdr_cmdset.raw\nsource mix.cir\necho after-source=$casemode\nquit\n' \
         | build-ver_50/src/ngspice -p -n 2>&1 | grep -E '^after-source=|(midnode|MidNode) +:'
       MidNode             : voltage, real, 1 long
   after-source=preserve
   ```

   So criterion 1 is to be read as this issue said it should: *no `Option:`
   line changes the next netlist read.* A `Command:` line still can, it is
   documented at `man/man1/ngsconvert.1:109`, it does the same on stock
   `ngspice-46`, and revisiting it is a separate issue with its own
   compatibility argument.
5. *A deck asserts it.* `tests/regression/pipe/rawfile-option-parse-policy.cmd`,
   fail-fast by exit code, building each rawfile with `echo` and its redirect
   as `tests/regression/casedist/vector-rawfile-scale-case.cir` does. Five
   checks: a baseline read of a deck it writes itself; the same deck after a
   `load` of `Option: casemode=preserve`; after one of
   `Option: casemode=distinguish`; after one of `Option: ngbehavior=hs`; and a
   plain key read back through `$`. The case checks compare the second read
   against the *first read of the same deck* rather than against a literal
   spelling, so the deck says the same thing in all three modes, and the two
   rawfiles ask for different modes so that at least one of them is a live
   check whichever mode the session is in. Each check that says "nothing
   moved" is preceded by a positive control that reads the value back, so a
   narrowing that refused the key at the `Option:` arm would fail this deck
   rather than pass it. RED at `720c8743a` + this deck, before the production
   change:

   ```
   $ ngspice -p < tests/regression/pipe/rawfile-option-parse-policy.cmd ; echo rc=$?
   ERROR: a loaded Option: casemode=preserve moved the mode the next read latched
   rc=1
   ```

   and, with the earlier checks removed so the later ones are reached,
   `ERROR: a loaded Option: casemode=distinguish moved the mode the next read latched`
   and `ERROR: a loaded Option: ngbehavior=hs changed how the next deck was parsed`,
   each `rc=1`. GREEN after it, `rc=0`. Re-measured with only
   `!inp_reading_netlist()` removed from `cp_getvar()` and everything else in
   place: this deck fails again at the first check and
   `rawfile-option-computed-name.cmd` still passes, so neither deck is
   passing on the other's mechanism.
6. *`make check` unchanged in all three modes.* 305 passed, 0 failed, with
   every cached `*.log`/`*.trs` deleted first — the 303 this batch had been
   running, plus this deck and `doc/codex/issues/0065`'s. No existing `.out`
   file moved; no committed deck contains an `Option:` line.

**Left.**

- The `Command:` residue of criterion 4, deliberately.
- `set casemode=…` is still refused while a plot carrying that key is
  current, because `cp_usrset()`'s `US_READONLY` arm (`options.c:462`-`:465`
  after this change, `:415`-`:417` as quoted above it) is untouched — property 4, unchanged and still measured
  (`Error: casemode is a read-only variable.`). It no longer matters for
  parsing, and it is still a nuisance: the one way to override the header is
  to `setplot` off that plot first. Narrowing that arm is a change to what
  `set` may write and wants its own argument.
- The round trip of property 5 is unchanged: `raw_write()` still re-emits
  `Option: casemode = preserve`, and the reload still reads it back into the
  new plot's environment. Only the reconfiguration is gone, measured —
  after `load rt.raw`, `$casemode` is `preserve` and `source mix.cir` still
  gives `midnode`.
- The window is `inp_readall()`, which is the reader; the elaboration that
  follows it in `inp_spsource()` is outside it. No policy read of this shape
  was found there — the two the issue names are both inside — but a future
  one placed after the read would not be covered by this guard.
- ~~The shared build is not built in `build-ver_50`, so the `totalreset()` line
  is compile-checked only~~ — **superseded 2026-08-13.** A
  `--with-ngshared` tree was built and the guard was measured in it, both
  halves: a loaded `Option: casemode=preserve` does not reconfigure the next
  `ngSpice_Command("source …")`, which is criterion 1 measured through the API
  for the first time, and it is the `HELD` assertion of
  `doc/claude/feedback/ngspice_upstream/repro/shared/netlist_guard_probe.c`.
  That build also found the defect below.
- **The guard did not survive a fatal netlist read, and this was a real bug in
  the fix above.** `inp_readall()` raises the count and lowers it on the way
  out; a read that ends fatally does not come out that way. In the standalone
  binary that is harmless, because `controlled_exit()` calls `exit()`. In a
  shared build it calls `shared_exit()`, the stack is discarded, and the count
  stays at 1 for the rest of the host process — from which point
  `plot_cur->pl_env` answers no `cp_getvar()` at all, which is this issue's
  narrowing applied permanently and to everything. The two landing sites this
  Resolution named (`ft_sigintr_cleanup()`, `totalreset()`) do not cover it,
  and a third route out — a `bg_` netlist read leaving through `pthread_exit()`
  — lands at no site at all. Filed and closed as `doc/codex/issues/0066`, one
  line in `shared_exit()`, with all three routes measured before and after.

- The `Option:` node is still leaked when the process exits. Measured under
  `valgrind` as `24 direct + 9 indirect bytes` for `Option: zzz=HAHA`, and
  byte-identically on stock `ngspice-46` — pre-existing, upstream, and
  untouched by this change.

**Two behaviour changes this batch made, or was thought to have made.** Both
were reported by the verifier and neither had been written down; both were
re-measured 2026-08-13 against three binaries, so that attribution is a
measurement and not a guess. **BASE** is this batch's own starting point —
`720c8743a` with all twelve modified files under `src/` reverted, configured
and built in a scratch copy outside the repo; **NEW** is `build-ver_50`;
**STOCK** is `/usr/local/bin/ngspice`, `ngspice-46`. BASE was checked against
two known markers first, to prove it really is pre-batch: it still shows this
issue's defect (`load hdr_option.raw`; `source mix.cir` gives `MidNode` where
NEW gives `midnode`) and it has no `curcasemode` (`echo $curcasemode` is empty
where NEW answers `fold`).

1. **Confirmed, and it is ours.** After `Option: CurPlot=HAHA`, `set` lists the
   file's key with the session's value:

   ```
   BASE   * CurPlot	HAHA      $CurPlot = HAHA
   NEW    * CurPlot	op1       $CurPlot = op1
   STOCK  * CurPlot	HAHA      $CurPlot = HAHA
   ```

   The cause is not this issue's guard — `set` is not a netlist read, and with
   `!inp_reading_netlist()` removed from `cp_getvar()` the listing still reads
   `op1`. It is `doc/codex/issues/0065`'s reordering of `cp_enqvar()`: the
   `curplot` arm now precedes the scan of `plot_cur->pl_env` and matches with
   `cieqn()`, so it claims the mixed-case spelling. `cp_vprint()`
   (`variable.c:1119`) prints `va_name` straight from `pl_env` but re-resolves
   the value through `vareval()`, so the two halves of the line now come from
   two different places. 0065 recorded the `$CurPlot` half of this under its
   criterion 1 and called it that arm's own case rule, which it is; the `set`
   listing is the same change seen one step further on and had not been
   written down anywhere.

2. **Falsified. Not ours, and not gone.** The report was that
   `Option: sourcepath=( subdir )` followed by a `source` used to emit
   `Error: sourcepath is a read-only variable.` and
   `cp_vset: Internal Error: it was already there too!!`, and no longer does.
   Re-measured, it fails twice over.

   It is not this batch's: BASE is silent on the same input, exactly as NEW is,
   and only STOCK speaks. The key reaches `pl_env` identically on all three —
   `write` re-emits `Option: sourcepath = ( subdir )` from every one of them —
   so nothing about the parse or the filing moved.

   And it has not gone away. The deck in the original measurement lives in the
   current directory, and `.` is already on `sourcepath` in both 46+ builds, so
   `add_to_sourcepath()` (`inpcom.c:10441`, called from the reader at `:1671`)
   drops every candidate path and returns at `:10488` without ever calling
   `cp_vset()`. Source a deck from a directory that is genuinely new and all
   three binaries print both lines:

   ```
   $ printf 'load hdr_sourcepath.raw\nsource subdir/inner.cir\nquit 0\n' | ngspice -p -n
   Error: sourcepath is a read-only variable.
   cp_vset: Internal Error: it was already there too!!
                                       BASE 2   NEW 2   STOCK 2
   ```

   against `0` for `source mix.cir` in the cwd on BASE and NEW, and `2` on
   STOCK — whose `sourcepath` carries `/usr/local/share/ngspice/scripts`
   twice, which is why its dedup misses where the others hit. So the
   difference the verifier saw was between two *installations*, not two
   builds of this tree.

   What is left standing is a live defect that this issue did not cause and
   does not fix: a rawfile's `Option: sourcepath=…` still makes
   `cp_usrset()` call the variable read-only, and the reader's own attempt to
   extend `sourcepath` still fails with an internal-error message, on every
   binary tested. It is the `US_READONLY` arm again, from the write side —
   `doc/codex/issues/0067` owns that arm's memory bug from the `unset` side.
   Whether the reader should be extending a variable a loaded plot has
   shadowed is a question for the `Option:` arm's own issue and not for this
   one, whose claim is about reads.
