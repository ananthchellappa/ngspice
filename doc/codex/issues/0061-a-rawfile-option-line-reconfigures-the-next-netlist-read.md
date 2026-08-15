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
- ~~`set casemode=…` is still refused while a plot carrying that key is
  current, because `cp_usrset()`'s `US_READONLY` arm (`options.c:462`-`:465`
  after this change, `:415`-`:417` as quoted above it) is untouched — property 4, unchanged and still measured
  (`Error: casemode is a read-only variable.`). It no longer matters for
  parsing, and it is still a nuisance: the one way to override the header is
  to `setplot` off that plot first. Narrowing that arm is a change to what
  `set` may write and wants its own argument.~~ — **superseded 2026-08-14.**
  That argument was made and the arm was narrowed; the second addendum below
  is the decision and the measurement. It stopped being a nuisance and became
  a defect the day `raw_write()` started putting the key in every file, which
  is the first addendum's change.
- The round trip of property 5 is unchanged: `raw_write()` still re-emits
  `Option: casemode = preserve`, and the reload still reads it back into the
  new plot's environment. Only the reconfiguration is gone, measured —
  after `load rt.raw`, `$casemode` is `preserve` and `source mix.cir` still
  gives `midnode`. (Since 2026-08-14 `raw_write()` also writes a `casemode`
  line of its own, so that round trip now produces two of them; the
  addendum below measures which one a reader gets back and why.)
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

## Addendum, 2026-08-14 — the ask this issue kept alive shipped on this carrier

`doc/claude/feedback/ngspice_upstream/FINDINGS.md` finding 1 asked for the case
mode to be recorded in the raw header and proposed `Option:` as the carrier.
The Impact section above says that disqualifies the carrier, and it did — for
as long as an `Option:` line reconfigured the reading session. The narrowing
under Resolution is what removed that, so the ask has now been implemented on
exactly the carrier it proposed. The repo owner decided it; this addendum
records what was written, what was measured, and what is left. Nothing above
it is restated or amended.

**The change.** One `fprintf` in `raw_write()` (`src/frontend/rawfile.c`),
immediately after the `Plotname:` line:

```c
    fprintf(fp, "Option: casemode=%s\n", inp_case_mode_name());
```

Three things about it are load-bearing, and each one is a measurement in this
issue or in the finding:

- **The key.** `Option:` is a key the reader already parses. A new `Casemode:`
  key aborts the load — the header loop's closing `else` prints
  `Error: strange line in rawfile` and `raw_read()` returns NULL — on stock
  `ngspice-46` and on this build alike, re-measured 2026-08-14.
- **The place.** After `Plotname:`, because the `Option:` arm quoted at the top
  of this issue needs `curpl` and prints `Error: misplaced Option: line` when
  there is none. That diagnostic marks the case where nothing is filed, which
  the Impact section already measured; here it is the reason the line cannot
  move up.
- **The value.** `inp_case_mode_name()` (`src/frontend/inpcom.c`), the mode in
  force — what `curcasemode` answers — and not `cp_getvar("casemode")`, which
  is the request and stops describing the file being written the moment a
  `.control` block writes it. `doc/codex/issues/0060` is that split, and this
  is a second consumer of the read side it added.

**Measured, on the unmodified `/usr/local/bin/ngspice` (`ngspice-46`).** A
divider written by this build under `-D casemode=preserve`, in both formats:

```
$ head -6 new_pres.raw
Title: * a divider written by the new build, nets In / MidNode
Date: Fri Aug 14 12:04:21  2026
Command: ngspice-46+, Build Thu Aug 13 22:49:54 UTC 2026
Plotname: Operating Point
Option: casemode=preserve
Flags: real

$ printf 'load new_pres.raw\ndisplay\necho casemode-from-file=$casemode\nquit 0\n' \
      | /usr/local/bin/ngspice -p -n 2>&1 | grep -E 'done|Error|casemode-from-file|v\(MidNode\)'
done.
    v(MidNode)          : voltage, real, 1 long
casemode-from-file=preserve
```

and byte for byte the same three lines for `new_bin.raw`, the binary write —
the header is text in both formats, so the line travels in both. No diagnostic
in either, which is the whole compatibility claim.

**Criterion 1 re-measured after the change**, because this is the change that
made it matter: as this addendum was written every raw file carried the line,
so a reader that steered on it would steer on every file. It does not, on
either binary. (Since the third addendum a file carries the line only if the
writing session set `casemodewrite`; the measurement below was taken when
every file did, and it holds for every file that has the line.)

```
$ printf 'load new_pres.raw\necho casemode=$casemode\necho effect=$curcasemode\nsource mix.cir\nquit 0\n' \
      | build-ver_50/src/ngspice -p -n 2>&1 | grep -E '^casemode=|^effect=|Circuit:|(midnode|MidNode) +:'
casemode=preserve
effect=fold
Circuit: * mixed-case net probe -- the net is spelled midnode
    midnode             : voltage, real, 1 long
```

`preserve` is what the file recorded and is readable, `fold` is what this
session is doing, and the deck read after the load is folded. On stock
`ngspice-46` the same sequence also folds, for the different reason that it has
no case mode at all.

**Two decks assert it**, and between them they cover all three modes:

- `tests/regression/pipe/rawfile-casemode-header.cmd` — one session, three
  modes, moved with `set casemode` plus a `source` (`doc/codex/issues/0058`:
  a `source` re-latches). Per mode: the header's line 5 is exactly
  `Option: casemode=<mode>` and line 4 is the `Plotname:` line, so the
  position is the assertion; the load's own diagnostics are captured and
  scanned for `misplaced`; the request is unset before the load so the
  read-back can only be answered by the file; and the `source` after the load
  must still latch `fold` and spell the deck's title accordingly.
- `tests/regression/casedist/rawfile-casemode-header.cir` — the same round
  trip in the batch harness, where the mode comes from `-D` at start-up
  instead of from a `set`, with the line and the read-back in the committed
  `.out`.

RED before the change: `the header line after Plotname: is <Flags: real> and
not <Option: casemode=fold>`, exit 1, and `HDR-LINE5 Flags: real` with an empty
`CASEMODE-AFTER-LOAD` in the batch deck. GREEN after it, and `make check` 307
PASS / 0 FAIL from cleared caches — the 305 this batch had been running plus
these two. Identity lint 265, baseline matches: the change adds no string
comparison.

One committed deck moved, and it moved because the header is one line longer:
`tests/regression/pipe/postcoms-keyword-case.cmd` reads a fixed number of lines
off an ASCII raw and asserts the next one is `Values:`, to prove `filetype` was
honoured. Eleven reads became twelve and its comment now enumerates the
`Option` line. The assertion is the same assertion — an exact line number, not
a search.

**Left, and each is measured.**

- **The `-r` path does not carry the line.** A batch run writing through
  `-r file.raw` is served by `fileInit_pass1()` (`src/frontend/outitf.c:945`
  onwards), which builds its own header and is not `raw_write()`. Measured:
  the file it writes has `Flags:` on line 5. The `write` command, which is
  what the client's decks and every deck in `repro/` use, is `raw_write()` and
  does carry it. Extending it is a second, larger change: that writer streams
  its header before the analysis runs and shares no code with this one.
- ~~**A re-written loaded plot records the writing session's mode.** The mode is
  a property of the session, not of the plot, so a `load` of a `preserve` file
  followed by a `write` from a folding session produces a header saying
  `casemode=fold` above variables still spelled `v(In)` and `v(MidNode)`. The
  same write also re-emits the loaded plot's own `Option: casemode = preserve`
  from the `pl_env` loop, so that file carries two `casemode` lines; the
  reader's own lookup answers with the first, which is `raw_write()`'s. A
  writer that preferred the plot's recorded value would fix the provenance and
  is not what the owner asked for — the value is defined here as the effective
  mode, from the same function `curcasemode` reads.~~ — **superseded
  2026-08-14** by the fourth addendum, which is where the owner did ask: the
  two lines were the defect its verifier reported, and the mode in force is now
  written only onto a plot this session produced. The `ngsconvert` half below
  was already superseded by the third addendum's gate.
  ~~The same is true of
  `ngsconvert`, which converts a file with `raw_write()` and reads no netlist,
  so it stamps `casemode=fold` on whatever it converts and the original's own
  line survives below it as a second `Option:` pair. Measured 2026-08-14 on a
  `--enable-oldapps` build; it is this bullet's phenomenon and not a new one.~~
- Everything under **Left** above this addendum is unchanged by it.

## Addendum, 2026-08-14 — the write half of the same rule

The narrowing under Resolution is about reads: a plot's environment may not
answer a question asked on behalf of a file being turned into cards. It has a
write half, and until now `cp_usrset()` broke it. Any name the current plot's
environment carried was answered `US_READONLY`
(`src/frontend/options.c:462`-`:465`), so for as long as that plot was current
the session could not set the name and could not unset it. That was recorded
above as property 4 and left as a nuisance; the first addendum's change is
what made it a defect, because from then on **every raw file this build wrote
carried `Option: casemode=<mode>`**, so an ordinary `load` disarmed the
session's control of its own case mode:

> Written the day the line was unconditional, and left in the past tense it was
> measured in. Since the third addendum the line is behind `casemodewrite` and
> a file carries it only if the writing session asked, so what is now reachable
> from an ordinary `load` is any key a header happens to carry — which is the
> same defect and the same deletion, reached by a hand-edited header or by a
> file somebody wrote with the gate open rather than by every file this build
> writes. Nothing else in this addendum moves: the rule, the deletion and every
> measurement below were taken with the key in the file.

```
$ printf 'load new.raw\nset casemode=preserve\necho ask=$casemode\nsource mix.cir\necho effect=$curcasemode\nquit 0\n' \
      | build-ver_50/src/ngspice -p -n 2>&1 | grep -E '^Error|^ask=|^effect='
Error: casemode is a read-only variable.
ask=fold
effect=fold
```

against `ask=preserve`, `effect=preserve` for the same two commands with no
`load` in front of them. The next netlist read parsed in the mode the *file*
named, and the only way out was to `setplot` off the loaded plot.

**The rule.** Nothing a simulation does fills a plot's environment — the
`Option:` line of a raw header is its only writer, which is the grep and the
two-function read recorded under property 5 — so every name that arm could
reach is a name that arrived in a file the user merely loaded. **A file the
user loaded may not decide what the session is allowed to set.** The scan is
deleted. No name appears in the predicate, for the same reason the read half
names none: `casemode` is one of the names it could reach and `ngbehavior`,
`sourcepath` and `numdgt` are others.

**`unset` gets the same treatment, deliberately, and the judgement is worth
writing down.** `cp_usrset()` is called by both `cp_vset()` and `cp_remvar()`,
so the deletion covers both by construction — but the two could have been
split, and keeping the refusal on the `unset` side was the alternative
considered. It was rejected: `Error: casemode is read-only.` on an `unset` is
the same sentence, from the same file, about the same session, and a user who
may set the name and may not unset it has been given a worse rule rather than
a safer one. So a key a loaded plot carries is removed by `unset`, from that
plot's environment, and the session's own value shadows rather than replaces
it while both exist — which means an `unset` takes one layer at a time:

```
load rsp_plain.raw     # Option: rspprobe=fromfile
set rspprobe=fromsession   -> $rspprobe = fromsession
unset rspprobe             -> $rspprobe = fromfile     (the file's value again)
unset rspprobe             -> $?rspprobe = 0
```

That is asserted in `tests/regression/pipe/rawfile-option-set-policy.cmd`,
which is this addendum's deck: five checks, fail-fast by exit code, the first
four on `casemode` through a rawfile *this build wrote* and the last on a
plain key through a hand-built one, so the rule is asserted as a rule.
RED at `58496a8dc` plus the deck:
`ERROR: a loaded rawfile made casemode read-only for the session`, `rc=1`.
GREEN after, `rc=0`, and `rc=0` under each of `-D casemode=fold`,
`-D casemode=preserve` and `-D casemode=distinguish`.

**What does not change.** The pair is still parsed, still filed into
`plot_cur->pl_env`, still listed by `set`, still answered through `$`, and
still re-emitted by `raw_write()`. The read narrowing is untouched: a loaded
plot still answers no read taken inside `inp_readall()`, which is what keeps
criterion 1 true now that a `set` can reach the same name.
`tests/regression/pipe/rawfile-option-parse-policy.cmd` still passes, so the
two halves are asserted independently.

**It closes the live defect this issue's second behaviour note left standing.**
That note ends: *a rawfile's `Option: sourcepath=…` still makes `cp_usrset()`
call the variable read-only, and the reader's own attempt to extend
`sourcepath` still fails with an internal-error message, on every binary
tested*. Re-measured 2026-08-14 with a deck in a directory that is genuinely
new, which is what that note established as the condition for reaching
`cp_vset()` at all:

```
$ printf 'load hdr_sourcepath.raw\nsource subdir/inner.cir\necho sp=$sourcepath\nquit 0\n' | ngspice -p -n
  /usr/local/bin/ngspice   Error: sourcepath is a read-only variable.
                           cp_vset: Internal Error: it was already there too!!
  build-ver_50/src/ngspice sp=. /usr/local/share/ngspice/scripts subdir
```

Two lines on the released binary, none on this one, and the reader's
extension now takes. It was the `US_READONLY` arm from the write side, as that
note said, and it is gone with the arm.

**The memory defect behind the other side of this arm is
`doc/codex/issues/0067`**, closed the same day. `unset` of a `pl_env` key used
to leave the plot holding a freed node and the next command that walked the
environment was a `SIGSEGV`; that was the `US_READONLY` arm of `cp_remvar()`
and it is fixed there, by an ownership rule that does not depend on this
deletion. Both were needed and each was taken RED and GREEN on its own.

**Left.**

- The decision is recorded as
  `doc/claude/decisions/0019-a-loaded-plot-makes-nothing-read-only.md`,
  because it changes what `set` and `unset` may do to a name and reverses an
  upstream behaviour that has been in place since the plot environment was
  written.
- A session that sets a name a loaded plot carries now has two values for it,
  one shadowing the other, and `set` lists both — the plot's under its own
  heading, the session's under the global one. That is what the plot
  environment has always looked like when a global shadows it; nothing new is
  displayed, and no diagnostic is printed when the shadowing begins.
- `US_READONLY` still means what it meant for the two names that are genuinely
  computed, `plots` and `curcasemode`.

### `--enable-oldapps` had stopped linking, and the first addendum is half of why

`ngsconvert` links a hand-picked list of front-end objects rather than
`libfte` (`ngsconvert_LDADD`, `src/Makefile.am`) and supplies the rest itself,
as stubs at the bottom of `ngsconvert.c` — `cp_usrset()`, `cp_usrvars()` and a
dozen others, since long before any of this. Two of the objects on that list
acquired references the list could not satisfy:

```
$ ../configure --enable-oldapps && make
/usr/bin/ld: frontend/rawfile.o: in function `raw_write':
     undefined reference to `inp_case_mode_name'
/usr/bin/ld: frontend/rawfile.o: in function `raw_read':
     undefined reference to `vec_name_eq'
/usr/bin/ld: frontend/variable.o: in function `cp_getvar':
     undefined reference to `inp_reading_netlist'
```

Three symbols and not one. `inp_case_mode_name()` is the first addendum's
`raw_write()` line; `inp_reading_netlist()` is this issue's own narrowing in
`cp_getvar()`; `vec_name_eq()` is `doc/codex/issues/0032`'s scale-reference
rule in `raw_read()`. The flag is off by default, which is why the batch got
this far without noticing.

Adding the defining objects is not the fix: `frontend/inpcom.lo` and
`frontend/vectors.lo` pull some fifty further symbols behind them — the
netlist reader, the plot list, the hash tables — into a program that reads no
netlist and keeps no plots. Measured by attempting exactly that link. So
`src/ngsconvert_stubs.c` joins `ngsconvert_SOURCES` with the three
definitions, in the shape the program has always used for this.

**They are not placeholders that lie.** `ngsconvert` never reads a netlist, so
`ng_case_mode` never leaves its initial `NG_CASE_FOLD`, and the real functions
linked in would answer exactly what these do: `"fold"`, `FALSE`, and the
case-insensitive comparison that fold and preserve share. The last is marked
`/* case-lint: helper */` because it *is* `vec_name_eq()`, which is what that
marker is for; the lint reports 264 sites and its baseline is unchanged by it.

Confirmed by configuring a scratch tree with `--enable-oldapps` and building:
the link error above, then `EXIT=0` and four `bin_PROGRAMS` where there were
none. Two residues, both measured and neither this fix's:

- `ngsconvert` segfaults converting the ASCII raw an `op` writes, with or
  without the `Option:` line. So does a build of `pre-master-47`
  (`c5cd68015`), which has none of this work in it — pre-existing and
  upstream. It converts the small hand-built rawfile in
  `rawfile-option-set-policy.cmd` fine, from both builds, and the only
  difference between the two outputs is the `Option: casemode=fold` line the
  new one writes.
- Which is the second: `ngsconvert` now stamps the case mode on everything it
  converts, and a file that recorded `preserve` comes out carrying
  `casemode=fold` above its own round-tripped line. That is the bullet above
  about a re-written loaded plot, reached through a second program.

## Addendum, 2026-08-14 — the line is behind a variable, off by default

The repo owner's decision: the first addendum's `Option: casemode=` line ships
**opt-in**, and the default flips once `doc/codex/issues/0067` has been in a
release. Nothing above is withdrawn. The line is written in the same place,
with the same key, valued from the same function; what changed is that
`raw_write()` asks first, and that a file this build writes with the variable
unset is byte for byte the file `58496a8dc` wrote.

**Why, and it is not about this tree.** The line is safe to *read* on every
binary that exists — that is the first addendum's measurement and it still
holds. The hazard is one step past the read. Loading such a file puts a
`casemode` key into that session's loaded-plot environment, and on
`/usr/local/bin/ngspice` (`ngspice-46`, as released) `unset casemode` frees a
node it leaves linked: 0067's defect, fixed in this tree by that issue and in
nothing released. The `unset` returns cleanly, so the crash lands on the next
command, whatever it is. Measured 2026-08-14 on a file written by this build
under `-D casemode=preserve` with the variable set, in both formats:

```
$ printf 'load new_pres_bin.raw\nunset casemode\n<CMD>\necho STILL-ALIVE\nquit 0\n' \
      | /usr/local/bin/ngspice -p -n ; echo rc=$?
  <CMD> = set                rc=139     no STILL-ALIVE
  <CMD> = echo $casemode     rc=139     no STILL-ALIVE
  <CMD> = display            rc=139     no STILL-ALIVE
  <CMD> = print v(In)        rc=139     no STILL-ALIVE
  <CMD> = unset casemode     rc=139     no STILL-ALIVE
```

and `rc=0`, `STILL-ALIVE`, on all four of those on `build-ver_50/src/ngspice`.
`new_pres_ascii.raw` gives the same five `139`s. (An earlier report had the
binary format dying on a *single* `unset` and the ASCII one surviving it; the
distinction does not reproduce — both formats survive the `unset` itself and
neither survives the command after it. The header is text in both, which is
why they behave alike.) A raw file this build writes must not become a crash
trigger for a simulator nobody can fix, unless somebody asked for the line.

**The change.** `src/frontend/rawfile.c`, the first addendum's `fprintf`
gains a condition:

```c
    if (cp_getvar("casemodewrite", CP_BOOL, NULL, 0))
        fprintf(fp, "Option: casemode=%s\n", inp_case_mode_name());
```

The name sits with the write-path options `raw_write()` and its caller already
read — `nopadding` and `keep#branch` in this function, `appendwrite` and
`plainwrite` in `com_write()` (`src/frontend/postcoms.c`), `filetype` beside
them — lowercase, no underscore, formed like `appendwrite`/`plainwrite`, and
read exactly the way they are. It is a boolean: `set casemodewrite` before the
`write`. No entry was added to `ft_setkwords[]` (`src/frontend/miscvars.c`),
which is the `set`-completion list and does not carry `plainwrite` either.

**Byte identity, measured with `cmp` and not by eye.** A worktree at
`58496a8dc` was configured with `SOURCE_DATE_EPOCH` set to the epoch of
`build-ver_50`'s own `NGSPICEBUILDDATE`, so the two binaries stamp the same
`Command:` line, and the probe deck fixes the other volatile field with `set
curplotdate`. Same deck, both binaries, both formats:

```
$ cmp old/bi_ascii.raw new/bi_ascii.raw   &&  echo IDENTICAL
IDENTICAL
$ cmp old/bi_bin.raw   new/bi_bin.raw     &&  echo IDENTICAL
IDENTICAL
$ cmp old/bi_ascii.raw on/bi_ascii.raw    # the same build with the variable set
old/bi_ascii.raw on/bi_ascii.raw differ: byte 160, line 5
$ diff old/bi_ascii.raw on/bi_ascii.raw
4a5
> Option: casemode=fold
```

One line, at line 5, and nothing else — in the binary format the same `cmp`
reports the same first-differing byte 160.

**`ngsconvert` is fixed by the same gate, which is the second residue of the
section above.** That program stamped `casemode=fold` on everything it
converted, so a `preserve` file came out carrying two `casemode` lines with
the wrong one first. It has no way to set a variable: the global list is
empty, `cp_usrvars()` is its own stub, `plot_cur` is NULL (`ngsconvert.c:38`)
and `ft_curckt` is NULL, so every link of `cp_getvar()`'s chain answers FALSE
and the line is never written. What the file said survives instead, through
`raw_read()` filing the pair and `raw_write()`'s `pl_env` loop re-emitting it.
`src/ngsconvert_stubs.c` keeps `inp_case_mode_name()` — the reference still
has to resolve — and its comment now says why it is unreachable rather than
merely what it answers. Measured on an ASCII raw this build wrote under
`-D casemode=preserve`, with its `Command:` line stripped so that the *before*
column could be measured at all — the crash in the next section is what stood
in the way, and it is the same file either way:

```
before:  Plotname: Operating Point        after:  Plotname: Operating Point
         Option: casemode=fold                    Flags: real
         Flags: real                              No. Variables: 3
         No. Variables: 3                         No. Points: 1
         No. Points: 1                            Option: casemode = preserve
         Option: casemode = preserve              Variables:
```

The *after* column is the same on the untouched file, once the crash below is
out of the way: one `Option: casemode = preserve`, the file's own, and none of
the converter's.

**The first residue of that section is fixed too, and it was not this batch's
bug.** `ngsconvert` segfaulted on any raw an `op` writes, with or without the
`Option:` line, on `pre-master-47` as well. The cause: `raw_read()`'s
`Command:` arm dereferences `ft_sim->simulator` (`src/frontend/rawfile.c`) to
recognise ngspice's own provenance stamp, and `ft_sim` is NULL in that program
(`ngsconvert.c:44`) — where the *writer* has always guarded on it, one hundred
lines above, and the reader never did. Both the stamp and the deref arrived
together in upstream `123ed0aad`, 2024-08-18 (*"Add simulator version info to
raw file"*), which is the commit that made every raw ngspice writes carry a
`Command:` line and therefore the commit that broke the converter for all of
them. Every file ngspice writes carries a
`Command: ngspice-<v>, Build <date>` line, so the crash was reachable from any
of them, and only a hand-built header without one got through. The guard is
`if (!ft_sim || !ciprefix(ft_sim->simulator, s))`: with no simulator to
compare against, the line cannot be recognised as our own and is treated as a
user command like any other — filed if there is a plot to file it into,
`cp_evloop()`d, which in `ngsconvert` does nothing. It is inert in `ngspice`,
where `ft_sim` is set at `main.c:926` before any `load` can run.

```
$ ngsconvert a pres.raw a conv.raw          # before
Segmentation fault (core dumped)
$ ngsconvert a pres.raw a conv.raw          # after
Error: misplaced Command: line
ASCII raw file "conv.raw"                   rc=0, header intact
```

The remaining diagnostic is the reader's existing message for a `Command:`
line that arrives before any `Plotname:` — which ngspice's own provenance line
does, in every file — and it is accurate: there is no plot to file it into, so
it is dropped. `ngspice` never prints it for that line because it recognises
the name and skips the arm. Making a converter recognise it would mean
hardcoding the simulator's name in the reader, or giving `ngsconvert` a
non-NULL `ft_sim`, which would make its own writer emit a `Command:` line
built from a `Spice_Build_Date` that is `" date is not available\n"` —
a newline in the middle of a header. Neither was taken.

**Tests.** Both decks of the first addendum now assert the gate from both
sides, and neither lost an assertion:

- `tests/regression/pipe/rawfile-casemode-header.cmd` — a new check 0b before
  the three-mode loop writes a raw with the variable unset and asserts line 5
  is exactly `Flags: real` and holds no `casemode`, by position and not by
  search; the loop sets it; a new check 6 unsets it again and re-asserts line
  5, so the gate is measured in both directions in one session.
- `tests/regression/casedist/rawfile-casemode-header.cir` — writes the same
  plot twice, once each side of the gate, and both headers' line 5 is in the
  committed `.out`.
- `tests/regression/pipe/rawfile-option-set-policy.cmd` and
  `tests/regression/pipe/unset-rawfile-option-key.cmd` each set
  `casemodewrite` before the `write` whose output they load: both are about
  what a loaded key does, so they need the key to be there.
- `tests/regression/pipe/postcoms-keyword-case.cmd` is back to the committed
  eleven reads. It counts header lines to prove `filetype` was honoured, and
  the header is ten lines again by default.

RED, before the production change: `ERROR: the header carries <Option:
casemode=fold> after Plotname: with casemodewrite unset`, `rc=1`, and the
`casedist` deck failing on the one line `HDR-OFF-LINE5 Option:
casemode=distinguish` against `HDR-OFF-LINE5 Flags: real`. GREEN after it.

**Left.**

- The default is off *for now*. Flipping it is a one-word change in
  `raw_write()` plus the two decks' check 0b and 6, and the condition is that
  0067 has been in a release.
- Everything under **Left** in the first addendum is unchanged: the `-r` path
  still does not carry the line, and ~~a loaded-then-rewritten plot still
  records the writing session's mode~~ — **superseded 2026-08-14** by the
  fourth addendum, which stopped that. The `ngsconvert` half of it was already
  gone with this gate: that program records nothing.
- **The gate itself could be opened by a file, and that was found here.** A
  header carrying `Option: casemodewrite` was parsed into the loaded plot's
  environment and answered `raw_write()`'s own `cp_getvar()`, so a `load`
  turned this session's writer on — a default a data file can flip is not a
  default, and this one exists to keep our files out of somebody else's crash.
  Closed by the fourth addendum below.

## Addendum, 2026-08-14 — a policy read, and a header that describes its own plot

The third addendum's verifier found two defects in the shipped line, and both
come from the same place: `plot_cur->pl_env` is the last link of
`cp_getvar()`'s chain outside a netlist read, and `raw_write()` re-emits every
pair it finds there. Neither is a new mechanism. Both are this issue's own rule
not reaching far enough, and the fix is the rule applied where it was missing.

**1. A re-written loaded plot recorded the wrong mode and shadowed the right
one.** A raw written under `preserve`, loaded in a folding session and written
out again with `casemodewrite` set, came out carrying two `casemode` lines —
the copying session's `fold` under `Plotname:`, the file's own `preserve`
further down — and the reader's own lookup answers with the first, because
`raw_read()` files the pairs in header order and `cp_enqvar()` takes the first
match. So the header said `fold` about a plot whose variables are `v(In)` and
`v(MidNode)`, which no folding run can produce:

```
$ head -9 again.raw                                    # before
Title: CaseRewriteProbe
Date: Fri Aug 14 18:55:01  2026
Command: ngspice-46+, Build Fri Aug 14 22:52:38 UTC 2026
Plotname: Operating Point
Option: casemode=fold
Flags: real
No. Variables: 3
No. Points: 1
Option: casemode = preserve
$ grep -c casemode again.raw                           2
$ ... unset casemode ; load again.raw ; echo READBACK=$casemode
READBACK=fold
```

**2. A data file opened the gate.** A header carrying `Option: casemodewrite`
is parsed into the loaded plot's environment, and `raw_write()` asked
`cp_getvar()`, whose chain reads through that link — so loading somebody's
waveform switched this session's writer on:

```
$ printf 'set filetype=ascii\nsource inner.cir\nload gate.raw\nwrite out.raw op1.all\nquit 0\n' \
      | build-ver_50/src/ngspice -p -n ; head -5 out.raw     # before
Title: caserewriteprobe
Date: Fri Aug 14 18:55:13  2026
Command: ngspice-46+, Build Fri Aug 14 22:52:38 UTC 2026
Plotname: Operating Point
Option: casemode=fold
```

`gate.raw` is eleven hand-written lines whose header holds
`Option: casemodewrite` and nothing else unusual; the plot written is `op1`,
this session's own, so nothing but the gate can explain the line. That defeats
the third addendum's decision, which exists so that a file this build writes
cannot become a crash trigger for a released `ngspice-46`: a default a data
file can flip is not a default.

### The rule, and it names no variable

The Resolution above says a plot's environment may answer a question about the
session and may not answer a question asked on behalf of a file being turned
into cards. The window it guards is `inp_readall()` — chosen because that is
where the reads were, and because a reader with dozens of call sites cannot be
asked one site at a time. The general statement is one step out:

> A **policy** read — one the code takes on its own behalf, about how to read a
> file or about what this build is allowed to do — never consults a loaded
> plot's environment, whenever it happens. A read about the session still may.

The gate is a policy read. It decides what this build writes; the answer must
come from the session, and a loaded plot's answer is a fact about some other
run. So `cp_getvar()` keeps its meaning — the session's own read, narrowed only
inside a netlist read — and a second entry point carries the declaration:

- `src/frontend/variable.c` — the body becomes
  `static bool getvar_chain(..., bool policy)`, with the plot-environment link
  reading `if (!v && plot_cur && !policy)`. `cp_getvar()` passes
  `inp_reading_netlist()`, which is exactly what that link tested before, so
  every existing caller is unchanged by construction. `cp_getvar_policy()`
  passes `TRUE`.
- `src/frontend/variable.h` — the declaration. That is `variable.c`'s own
  header and it includes `ngspice/cpextern.h`, where `cp_getvar()` is declared;
  putting it here keeps the change inside the two files that need it, and
  `rawfile.c` already includes it, as ten other front-end files do.
- `src/frontend/rawfile.c` — the gate read becomes `cp_getvar_policy()`.

Two ways a read is known to be policy, then: it is taken while a netlist is
being read, when every read is policy and the caller cannot be asked, or its
caller says so. Neither predicate knows a variable name, which is the same
reason criterion 2 gives: a predicate that knew `casemode` would have had to
learn `casemodewrite` the next day, and did not have to.

### Decision — the file's pair is kept, and our line is not written over it

The question the owner posed is whether `raw_write()` should re-emit a loaded
plot's `casemode` pair at all. **It should, and this build's own line is the
one that stops being written for such a plot.**

- *For suppressing the file's pair:* this build writes its own line, so the
  file's is redundant and its spelling is not ours (`Option: casemode = preserve`
  from the `pl_env` loop against `Option: casemode=preserve` from the writer).
- *Against:* the pair is data the file carried. It is also, for a plot that came
  out of a file, the **only** record of the mode that plot was produced under —
  this session knows nothing about it. Dropping it to write our own value would
  keep the tidy spelling and lose the fact, which is the defect the verifier
  reported, not a fix for it.

So the value written is chosen by provenance, and the test is provenance and
not a name:

```c
    if (!pl->pl_env &&
            cp_getvar_policy("casemodewrite", CP_BOOL, NULL, 0))
        fprintf(fp, "Option: casemode=%s\n", inp_case_mode_name());
```

A plot's environment is filled by `raw_read()` and by nothing else in the tree
— property 5's grep, unchanged since — so a plot that carries one came out of a
file, and what that file said about it is re-emitted unchanged by the loop
below. A plot this session produced carries no environment at all, and gets the
mode in force. One `casemode` line either way, and it describes the plot:

```
$ head -9 again.raw                                    # after
Title: CaseRewriteProbe
Date: Fri Aug 14 19:07:13  2026
Command: ngspice-46+, Build Sat Aug 15 02:03:55 UTC 2026
Plotname: Operating Point
Flags: real
No. Variables: 3
No. Points: 1
Option: casemode = preserve
Variables:
$ grep -c casemode again.raw                           1
$ ... unset casemode ; load again.raw ; echo READBACK=$casemode
READBACK=preserve
```

and the same file in the binary format, where the header is text too:
`Option: casemode = preserve`, once, and no line under `Plotname:`.

The gate, after, with the same eleven-line `gate.raw` and the same write of
`op1`: `$?casemodewrite` still answers `1`, because the pair is still filed and
still readable — nothing about the read-back moved — and `out.raw` line 5 is
`Flags: real`. The session can still open the gate itself, in the same session,
with that plot still current: `set casemodewrite`, and the next write of `op1`
carries `Option: casemode=fold`.

### Tests

Two decks in `tests/regression/pipe/`, which compares no output and asserts by
exit code, plus their `TESTS` and `CLEANFILES` entries. Both are pipe shapes
because both are sequences of control-language commands and neither is a
netlist.

- `rawfile-casemode-rewrite.cmd` — writes a `preserve` file with the gate set,
  becomes a folding session, loads it and writes it out again. The assertion is
  a **count**: exactly one line of the copy mentions `casemode`, because the
  defect was a second line and not a wrong one. Then that line must hold
  `preserve` and must not hold `fold`, and the copy is loaded back with the
  request unset so that the reader's own lookup — not this deck's parse — has
  to answer `preserve`. Two controls keep it live: the `preserve` file's line 5
  is asserted before anything is loaded, and the loaded plot is checked to be
  spelled `MidNode`, a spelling the session doing the writing cannot produce.
- `rawfile-casemode-gate-key.cmd` — writes a plot of its own, then loads a
  hand-built header carrying `Option: casemodewrite` and writes **that plot,
  not the loaded one**, so the deck fails on the gate alone and not on the
  provenance test. Controls on both sides: the key must answer `$?casemodewrite`
  after the load (or a build that read the gate through the plot would pass for
  free), the header written with nothing set must have `Flags: real` on line 5,
  and after `set casemodewrite` the same write of the same plot must carry the
  line — so a fix that closed the gate by breaking the writer fails here.

**RED**, at the state before this addendum's production change:

```
$ ngspice -p < rawfile-casemode-rewrite.cmd ; echo rc=$?
ERROR: the re-written plot carries 2 casemode lines, last <Option: casemode = preserve>
rc=1
$ ngspice -p < rawfile-casemode-gate-key.cmd ; echo rc=$?
ERROR: a loaded header opened the writer gate: <Option: casemode=fold>
rc=1
```

**GREEN** after it, `rc=0` and the `INFO:` line from each. And each deck fails
on its own mechanism, measured by putting back one half at a time:

| build | rewrite deck | gate deck |
| --- | --- | --- |
| both fixes | 0 | 0 |
| `cp_getvar()` restored, `!pl->pl_env` kept | 0 | **1** |
| `cp_getvar_policy()` kept, `!pl->pl_env` removed | **1** | 0 |

### Criteria, and what did not move

1. *Loading a raw file does not change how the next netlist is parsed.* Untouched:
   the netlist-read narrowing is the same predicate, now passed as an argument.
   `rawfile-option-parse-policy.cmd` still passes.
2. *A rule and not a special case.* Stated above; the predicate takes a `bool`
   and no name. `casemodewrite` is the second name the same rule has covered
   without being taught it.
3. *Backward compatibility decided explicitly.* Decided above: the file's pair
   survives, this build's line yields to it. A file written by any ngspice ever
   released reads the same as it did, and a file this build writes with the
   gate shut is unchanged in every byte.
4. *The `Command:` path is untouched.* Nothing here goes near it.
5. *A deck asserts it.* Two, below, in the directory this criterion names and
   for the reason it gives.
6. *`make check` unchanged.* 317 passed, 0 failed, from cleared `*.log`/`*.trs`,
   with these two decks among them — `tests/regression/pipe` 29 and
   `tests/regression/casedist` 30. The tree is shared with another crew this
   round, so that total also carries whatever they added; no directory
   regressed and no `.out` file moved. `identity.baseline` is untouched by this
   change and both lint specs match it (`identity lint: 264 comparisons,
   baseline matches`, and 11 for the self-test): the change adds no string
   comparison and moves none, and `!pl->pl_env` is a pointer test.

**And what did not move.**

- The default-off path is byte for byte what it was, for the session that
  neither sets the variable nor loads a header carrying it: no line is written
  at all, which is the header every ngspice has ever written. Both header decks
  assert that by position, on line 5. What did move is the session that *does*
  load such a header — it used to get the line and now does not, which is
  defect 2 and the point.
- `unset-rawfile-option-key.cmd` and `rawfile-option-set-policy.cmd` are
  unaffected and unedited. The first is the one that could have moved: it
  unsets the file's `casemode` key from the loaded plot and re-writes it,
  asserting that the copy still records the mode. It does — that `unset` empties
  the plot's environment, so the plot no longer carries a record, and the
  session's mode is written as it always was.

**Left.**

- **A loaded plot whose file recorded nothing still takes the writing session's
  mode.** Measured: a `preserve` file written with the gate shut (the default,
  so this is the common file) carries no `Option:` line at all, so the plot it
  loads into carries no environment, and a folding session that re-writes it
  with the gate open stamps `Option: casemode=fold` above `v(In)` and
  `v(MidNode)`. The provenance test cannot see that plot's origin, because the
  environment is the only mark of it there is. Closing it needs the mode
  recorded on the plot itself — one field in `struct plot`
  (`src/include/ngspice/plot.h`), set by `raw_read()` and by the writer of a
  simulation plot — which is a header this change does not own and a change
  worth its own argument. Until then the rule is: a header records the mode of
  the run that produced the data whenever the file it came from said so, and
  the copying session's otherwise.
- **A file that already carries two `casemode` lines round-trips both.** They
  are two pairs in one environment, and the loop re-emits what it is given —
  measured, both come out, in the `= ` spelling. Only a file written by this
  tree between the first addendum and this one can be in that state, and none
  has been released.
- **The copied line is not on line 5 and is not spelled the way ours is.** A
  plot's own record comes out of the `pl_env` loop, which writes
  `Option: <key> = <value>` after the `No. Points:`/`Command:` lines. A consumer
  must therefore parse `Option:` lines wherever they appear in the header and
  split on `=` with the spaces trimmed, which is what ngspice's own reader does
  (`cp_lexer()` then `cp_setparse()`). Recorded for the client in
  `doc/claude/casemode-distinguish-guide.md` §9 and
  `doc/claude/feedback/ngspice_upstream/RESPONSE.md` §2.
- **`cp_getvar_policy()` has one caller, and the sites beside it were left
  alone deliberately.** `raw_write()` reads `nopadding` and `keep#branch` two
  lines above, and `com_write()` reads `filetype`, `appendwrite` and
  `plainwrite`; all five are write-path options and all five can be reached
  from a loaded header the same way, so by the definition above they are policy
  reads too. They are not converted here for two reasons: no defect was
  reported against any of them, and each has a compatibility argument this
  addendum did not make — `Option: nopadding` in a header plausibly describes
  the plot it arrived with, `Flags: … unpadded` being the reader's other record
  of exactly that, so whether the session or the file should win is a question
  about padding and not about case. Whoever answers it has the entry point and
  needs no new mechanism. `casemodewrite` had no such argument: it exists to
  keep our files out of a released simulator's crash, which a file cannot be
  allowed to decide.
- **The rest of `cp_getvar_policy()`'s intended callers are future ones.** The
  reads inside `inp_readall()` keep being narrowed without asking their
  callers, which is the right shape for a reader with dozens of sites; a read
  of the form "may this build do X", taken anywhere else, is expected to use
  the new entry point, and the comment above the two says which is which.
- The `Command:` residue of criterion 4, the `-r` path, and the leak of the
  `Option:` node at exit are all unchanged by this addendum.
