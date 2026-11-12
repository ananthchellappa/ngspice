# Spec: the `stb` loop-gain (stability) analysis

## Status

Implemented on `ver_53`, as a port of the nine commits of the `stb-upstream`
branch (base `8e03f9570`), one fork commit per upstream commit, each with
`(cherry picked from commit ...)` in its message. `stb_version` is 1.

This file describes the feature **as built**. It is written from the code and
from the user guide `README.stb` at the tree root, which is the text a user
reads; where the two could disagree the code decides and the difference is a
defect of the guide. Sections 1 to 9 hold for both branches. Section 10 is what
this fork adds: names follow the case mode. Section 11 lists what is not
covered.

Reference for the method: M. Tian, V. Visvanathan, J. Hantgan, K. Kundert,
"Striving for small-signal stability", IEEE Circuits and Devices Magazine,
vol. 17, no. 1, 2001.

Files:

| file | part |
| --- | --- |
| `src/spicelib/analysis/stban.c` | the analysis: checks, operating point, sweep, the two solves, the plots, the log lines |
| `src/spicelib/analysis/stbmarg.c` | the margins; arithmetic only |
| `src/spicelib/analysis/stbsetp.c`, `stbaskq.c` | the parameters of the job |
| `src/include/ngspice/stbdefs.h` | the job, the margins record, the plot titles |
| `src/spicelib/parser/inp2dot.c` (`dot_stb`) | the card and the command line |
| `src/frontend/inpc_stb.c`, `inpc_stb.h` | terminal probes: collection, insertion of the 0 V sources, lookup |
| `src/frontend/inp.c`, `subckt.c` | the hooks of the reader for the terminal probes |
| `src/frontend/outitf.c`, `typesdef.c`, `dotcards.c`, `com_measure2.c`, `runcoms.c` | plots, vector types, `.print`/`.plot`, `meas`, `.meas` under `run` |
| `src/maths/ni/niaciter.c` (`NIacFactor`) | load and factor of the AC matrix without a solve |

## 1. Purpose

The analysis gives the loop gain of a feedback loop over frequency and, from
it, the phase margin and the gain margin. The loop stays closed: the operating
point is that of the working circuit. The loop is named by a *probe*, a 0 V
voltage source in the loop, or a terminal of an instance at which the reader
inserts such a source.

## 2. Interface

```
.stb <probe> dec|oct|lin <points> <fstart> <fstop> [localgnd=<node>] [diag=1]
stb  <probe> dec|oct|lin <points> <fstart> <fstop> [localgnd=<node>] [diag=1]
```

- `<probe>`: a voltage source (section 4) or `<instance>:<terminal>`
  (section 5). Written without quotes.
- `dec`, `oct`: `<points>` per decade or octave; `lin`: `<points>` in all. The
  points are those `.ac` makes for the same words.
- `localgnd=<node>`: the reference node of the probe (section 7).
- `diag=1`: six more vectors (section 6). `diag=0` is accepted and is the same
  as no `diag`. Any other value is an error.
- `localgnd=` and `diag=` follow the sweep in either order; given twice, the
  last counts. Blanks around `=` are accepted. From the probe on, a comma
  separates words as a blank does.
- `stb_version` (number, 1) is set at start-up in the executable and in the
  shared library. A script tests for the feature with `$?stb_version`.

When it runs:

- The command runs at once.
- A `.stb` card is run by `run`; in batch mode it behaves as an `.ac` card
  does (`-r file`, or a `.print`/`.plot` card, or a `.control` section that
  runs it).
- The analysis is registered last in the table of analyses, so no existing
  analysis index changes, and `run` does the stb jobs after all others.
  Several `.stb` cards run last card first, the order the task list has for
  several cards of any analysis.
- A `.stb` card inside a `.subckt` definition is not collected and not run;
  the reader says so in one line per card and turns the card into a comment.

## 3. Semantics

Operating point: found as `.ac` finds it. `noopac`, `keepopinfo`, the XSPICE
event-driven operating point and the per-frequency operating point with
`hertz` apply. With KLU and `hertz` the complex factors are made anew at each
frequency instead of refreshed.

Per frequency the AC matrix is loaded and factored once (`NIacFactor`) and
solved for two right-hand sides. The right-hand side that the load made is
dropped: **AC values of all sources are ignored**, the probe's included; their
matrix entries stay, so a voltage source is a short and a current source is
open. With `x` the excited node of the probe, `y` its other node and `ref` the
reference node (ground, or the node of `localgnd=`):

1. a unit voltage in series with the probe (what `V x y ac 1` stamps) gives the
   probe current `i1` and `v1 = v(x) - v(ref)`;
2. a unit current into `x` that returns to `ref` (what `I ref x ac 1` stamps)
   gives `i2` and `v2`.

```
K        = 2 (i1 v2 - v1 i2) + v1 + i2
T        = K / (1 - K)          the return ratio
loopgain = -T
```

The sign is fixed: there is no option for the other one. A loop with negative
feedback has `T` positive at low frequency, so `loopgain` starts with a phase
of 180 degrees.

The excited node is the positive node of the source. If that node is the
reference node the source is taken the other way round (the first log line
says `source reversed`). If both nodes are the reference node the run is an
error.

The first line of a run names what was resolved:

```
stb: probe <probe> (voltage source <name>, x = <node>, y = <node>[, localgnd = <node>][, source reversed]), <n> points, <fstart> Hz to <fstop> Hz
```

## 4. A voltage source as the probe

The probe is found, in this order:

1. by the name the source has in the circuit (`vprb`, `v.x1.vprb`);
2. by the path of its instances (`x1.vprb` for `v.x1.vprb`). In an instance of
   a subcircuit that is defined inside another one the name in the circuit
   repeats the device letter, and `x3.xi.vprb` is `v.x3.v.xi.vprb`. A name is
   compared with a path only if one of the parts of the path after the first
   begins with the letter of the device: a word without a dot is no path, and
   `x1.vprb` is not taken for a resistor `r.x1.vprb` of the top level.

Only a voltage source is a probe by itself. Any other instance is an error
whose text names the terminal form that could be meant (section 8).

## 5. A terminal of an instance as the probe

`<instance>:<terminal>` names one terminal of an element or one port of a
subcircuit instance. The reader takes the terminal off its net, puts it on a
new node `x`, and places a 0 V source between `x` (its positive node) and the
net `y`. For a port only that one instance is opened.

The instance: an element or instance of the top level (`r1`, `x1`); an element
or instance inside an instance, by the path of the instances (`x1.r3`,
`xamp.xm4`); an element by the name it has in the circuit (`r.x1.r3`).

The terminal:

- a number: the position on the instance line, counted from 1. A number is
  always a position; if a port has that number as its name the reader says
  which was taken.
- the name of a port, for a subcircuit instance.
- a letter, for a transistor, in the order of the nodes on its line: M `d g s
  b`, Q `c b e s`, J and Z `d g s`. A letter beyond the nodes the line has is
  refused. No other device has letters.

**Read-time insertion.** The source is made while the netlist is read, so the
reader has to find the probe in the deck. It collects the probes of

- the `.stb` cards at the top level, and
- the `stb` lines of the deck's `.control` section in which the probe is
  written out literally, also inside `if`, `while`, `repeat`, `dowhile` and
  `foreach`. A word with `$`, a quote, a backquote or a brace is not collected.

A port is handled when its instance line is expanded (`inp_stb_xcard`, called
from `doit()` in `subckt.c` for the instance lines of the deck itself); an
element is handled in the flat deck, after `.if` is evaluated
(`inp_stb_insert`). One line is printed per terminal:

```
stb: probe x1:inn inserted as v.x1#stb2 (x = x1#stb2, y = mid)
stb: probe <probe> not inserted: <reason>
```

Names made: the node is the name the instance has in the circuit, `#stb` and
the position (`x1#stb2`, `r.x1.r3#stb1`); the source is `v.` and the node.
**Spellings of one terminal share one source.** If an element line of the deck
already contains `#stb`, the mark is lengthened (`#stb#`, ...) until no line
has it. The same instance name on a second card (another `.if` branch) gets a
node with the suffix `_2`.

`reset` and `mc_source`: the collected probes of the `.control` section are
kept with the stored deck as marker cards, known by the address of their line
source and not by their text, so a reload inserts the sources again.

A command whose terminal was not collected (it comes from a variable or was
typed at the prompt and is nowhere in the deck) finds the source if the same
terminal has one under another spelling (`x1:2` for `x1:inn`); otherwise it is
an error that tells the user to add a `.stb` card.

Effects on the circuit:

- each inserted source adds its branch current and the new node to the plots
  of every analysis (`save nointernals` leaves out the node);
- results of other analyses are equal to rounding, not bit for bit;
- an `.ic` for the net is repeated for the new node, and the other way round.
  A new node with an `.ic` also gets a capacitor of 0 F to ground
  (`c.<node>`), because with KLU an `.ic` needs a matrix element on its node.

Refused probes (the deck still loads; the analysis with that probe fails with
the same reason):

- an instance that does not exist or is not unique, a position the instance
  does not have, an unknown port name or letter, a malformed word;
- R, C and L with a behavioural value, E and G with an expression, POLY
  sources: the reader replaces these by other elements;
- position 3 and up of an F or H element, a coupling K, a code model instance;
- an element whose terminals cannot be counted (the word after the nodes of a
  D, M, Q, P or N line is not a `.model` of the deck);
- a port that is a global node; a port with the same name as an earlier port
  of the same `.subckt` line; a port with nothing connected in the instance;
- a port of an instance of a subcircuit that is defined inside another one;
- a port that leads to a digital or mixed port of a code model.

## 6. Output

A run makes two plots, in this order; the loop-gain plot is the current plot
afterwards:

| plot | type name | vectors |
| --- | --- | --- |
| `STB Analysis` | `stb<n>` | `frequency` (scale), `loopgain` (complex) |
| `STB Margins` | `stbmargin<n>` | `phasemargin` (deg), `phasemarginfreq` (Hz), `gainmargin` (dB), `gainmarginfreq` (Hz); one real value each, always present |

With `keepopinfo` a third plot, `STB Operating Point`, is written before the
other two, when the sweep is done.

- `stb_marginplot` (string) is set to the name of the margins plot after every
  successful run. The two plot numbers are equal only while plots are
  destroyed in pairs. A run that writes to a file makes no plots; the plots
  are in the file and `stb_marginplot` is removed.
- Nothing is written before the sweep is complete: a run that fails makes no
  plot, and the current plot and `stb_marginplot` stay as they were.
- `save` and `.save` do not apply to the two plots. For the operating point
  plot a save that names nothing in it is ignored, and with no other save the
  plot keeps all its vectors.
- Vector types: `loopgain` and the two margins have no type, the two margin
  frequencies are frequencies, `stb_i*` are currents.
- `diag=1` adds `loopgain_normal` and `loopgain_reverse`, which sum to
  `loopgain`, and the four responses `stb_i1`, `stb_v1`, `stb_i2`, `stb_v2`.
- `.print stb` and `.plot stb` select the loop-gain plot, not the margins plot.
- `meas` takes a loop-gain plot with the meaning it has for an ac plot (written
  `meas ac ...`), not the margins plot. `phase_margin` and `gain_margin` expect
  `T`, that is `-loopgain`. There is no `.meas stb` card.
- `.meas` cards of the other analyses are evaluated by `run` on the plot of
  their own analysis, as without a `.stb` card.

## 7. `localgnd=<node>`

Without it the injected current returns to ground and `v(x)` is taken against
ground. With it both use the named node. `localgnd=0` and `localgnd=gnd` give
the results of no `localgnd`. A node that does not exist is an error; no node
is created by the lookup. Checked against closed forms only.

## 8. Margins

From the points of the sweep; between two points `|T|` in dB and the phase of
`T` in degrees are straight lines over `log10(f)`. The phase of `T` (not of
`-T`) is made continuous along the sweep, starting in `(-180, 180]`.

Decided in this order (`STBmargin`):

1. `T` not finite at some point: one warning naming the first such frequency
   and the number of points; no margins. The IEEE values are written.
2. `max|T| < 1e-12`: "the loop gain is zero at every frequency"; no margins.
3. Fewer than two points: no margins.
4. A frequency that is not positive or not rising (a `lin` sweep from 0 Hz):
   no margins.
5. `T` exactly zero at some point: no margins, also if the crossings lie below
   that frequency.
6. **Positive feedback at the start**: the real part of `T` is negative at the
   first point; no margins. The line says that the phase lag of the return
   ratio exceeds 90 degrees there and that a loop with an ideal integrator
   triggers this.
7. Phase margin: `180 + phase(T)` at the **first falling** 0 dB crossing of
   `|T|`. If there is none: `|T|` is still above 0 dB at the last point, or it
   is below 0 dB at every point (the line gives the largest value).
8. Gain margin: `-|T|` in dB at the first frequency at which the phase of `T`
   falls through -180 degrees. If there is none the line gives the phase at the
   last point.
9. One line if the loop gain crosses 0 dB more than once, and one line giving
   the frequency of a rising crossing that lies before the one used.

A margin that does not exist is NaN in the margins plot and has its reason in
the log. The exact log lines are listed in `README.stb` section 5.

## 9. Errors

One line starting `Error: stb:`, naming the probe where one was given; no
plot. Nothing is created or changed before the line, the sweep and the probe
are known to be good.

- The line: no probe; a step word that is not `dec`, `oct` or `lin`; a missing
  or unreadable number; an unknown argument (the text lists the known ones);
  `localgnd` without a node; `diag` not 0 or 1; a number of points out of
  range. A bad `.stb` card does not stop the deck from loading: the problem is
  stored with the job and reported when it runs, and the other analyses run.
- The sweep, checked more strictly than `.ac`: points below 1; a start or stop
  that is not finite; a start that is not positive for `dec`/`oct` or negative
  for `lin`; a stop that is not positive or below the start; a step too small
  to change the frequency; too many points; a sweep too wide to count. One
  point and a `lin` sweep from 0 Hz are allowed and give no margins.
- The probe: no such instance; a subcircuit instance (the text suggests a
  port); a transistor or a controlled source (the text suggests a terminal); a
  coupling or a code model instance; any other device (the text suggests
  `<name>:1`); a refused terminal probe (the reason of section 5); a terminal
  that was not collected; both nodes on the reference node; a `localgnd` node
  that does not exist.
- The run: the operating point does not converge; the AC matrix cannot be
  solved at some frequency; an interrupt (no plot, no resume).

As with any analysis that fails, `run` then reports the run as aborted and
does not evaluate the `.meas` cards.

## 10. Names and the case mode (this fork)

`doc/claude/specs/case-sensitive-identifiers.md` defines the three case modes.
The analysis follows them; the identity lint of
`doc/claude/decisions/0011-identity-lint.md` passes with no baseline entry
added.

**Names the deck wrote** — an instance, every part of a path, the name an
element has in the circuit, a port of a subcircuit, a node (also the node of
`localgnd=` and a node on an `.ic` card), a model, and the names the reader
made from them — are compared with `ng_ideq()` or one of its mode arms:

| mode | a probe word | `X1` and `x1` |
| --- | --- | --- |
| `fold` | folded to lower case (the deck is lower case) | one instance |
| `preserve` | kept; compared without regard to case | one instance |
| `distinguish` | kept; compared byte for byte | two instances; a spelling the deck does not have names nothing |

- The fold of a probe word, of a `localgnd` node and of a marker card happens
  only under `fold` (`inp_case_folding()`), as `com_alter()` does for a device
  name.
- The mode arms in `inpc_stb.c` are `stb_ch_eq()` (a character),
  `stb_ideqn()` (a prefix) and `stb_find_name()` (a search); in `stban.c`
  `stb_ch_eq()` and `stb_begins()`. Each raw comparator inside them carries
  `case-lint: helper`.
- The instance table is looked up byte for byte (`CKTfndDev`); under
  `preserve` `stb_find()` then compares the instances one by one before it
  tries a path.
- A port is looked up in the table of the global nodes with `glo_key()` of
  `subckt.c`, the key the expansion stores a global node under.
- A save is matched against the operating point plot with `name_eq_query()`,
  the helper `beginPlot()` uses for every save.
- One compare stays byte-exact with `case-lint: stored`: the `<name>_int1`
  node of an E or G source with an expression, where both sides are made from
  the name on the same card.

**Words the language defines** are taken in any case in every mode: `.stb`,
`stb`, `dec`/`oct`/`lin`, `localgnd`, `diag`, the letter of a terminal, the
letter of a device (also the `x` of an instance and the hints of the error
messages), the port types of a code model line, `v(` on an `.ic` card, `gnd`.
The compares of a plot title with the `STB_*_TITLE` macros carry
`case-lint: keyword`.

**Made names** carry the spelling of the deck: for `X1:In` under `preserve`
the node is `X1#stb1` and the source `v.X1#stb1`. The `v.` and `c.` prefixes
and `#stb` are lower case in every mode.

Under `preserve` a message prints the probe in the spelling the symbol table
saw first, which is the deck's for a name of the deck.

**Shared library.** The file-scope state of `inpc_stb.c` (the probes of the
deck being read, the sources made for them, the texts of the marker cards of a
reload, the mark, the flag that the deck's own expansion is going on) is
cleared at the start of every collection, handed to the circuit when the deck
has become one, and freed by `inp_stb_discard()` on both failure returns of
`inp_spsource()`. `totalreset()` in `sharedspice.c` calls `inp_stb_discard()`
as well, for a read that was abandoned by a jump. The probes of a circuit are
freed with it (`com_remcirc`). `stb_version` is set in `ft_cpinit()`.

Tests of this section: `tests/regression/case/stb-*-case.cir` (preserve) and
`tests/regression/casedist/stb-*-case.cir` (distinguish, with instances and
nodes that differ only by case).

## 11. Limits and what is not covered

- No device as the probe: the loop gain "of" a transistor is an error that
  names a terminal.
- No differential or common-mode loops, no sweep of anything but the
  frequency, no periodic operating point.
- No pause and resume: an interrupted run makes no plot.
- No declare-only form: a terminal probe has to be written in a `.stb` card or
  literally in the `.control` section of the deck.
- Ports of an instance of a subcircuit defined inside another subcircuit
  cannot be probes; the terminals of its elements can.
- A position is read only if it has fewer than nine digits.
- The command line is assembled in the buffer all analysis commands share
  (`BSIZE_SP`); a probe path longer than that is not supported.
- `show` and other commands that give `#` a meaning do not find an inserted
  source by its name; vectors are not affected.
- Under `distinguish` a probe that misses only by case is an error like any
  other miss; the message does not point at the case variant.
- The margins and `localgnd` have been checked against closed forms of
  textbook loops, not against another simulator.
- Not tested: the interrupt path.
