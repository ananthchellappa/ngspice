# Spec: terminal currents with `.probe I(...)`

## Status

Implemented on `ver_53`. This file describes the feature as built. It is
written from the code; where it and the code could disagree the code decides
and the difference is a defect of this file.

Before, `.probe I(...)` reached only the instances of the top level. A
terminal inside a subcircuit instance had no current of its own: what a user
could save were the quantities of a device model (`@m.x1.m3[id]`), whose names
depend on the device, which exist only in some analyses, and which are taken
at the iterate before the last. This adds the current into any terminal of
any instance at any depth of the hierarchy, in every analysis, under one rule
of names, with no code per device.

Files:

| file | part |
| --- | --- |
| `src/frontend/inpc_probe.c` (`inp_probe`, `probe_handover_for`) | the `.probe` card; what it inserts itself at the top level as before, and what it hands over as `.iprobe` cards |
| `src/frontend/inpc_stb.c`, `inpc_stb.h` | the current probes: collection of the `.iprobe` cards, the terminal, the 0 V source, the name, the `.save`, the rename after parsing |
| `src/frontend/inp.c` | the two hooks: `inp_stb_saves()` behind the `.save` cards, `inp_stb_rename()` behind `modprobenames()` |
| `src/frontend/subckt.c` | the hook of the expansion that `stb` has already (`inp_stb_xcard()`) |
| `src/frontend/cpitf.c` (`ft_cpinit`) | `probe_version` |
| `tests/regression/probe/` | the tests; also `tests/regression/case/probe-term-case.cir`, `tests/regression/casedist/probe-term-case.cir` |

## 1. Interface

```
.probe I(<instance>[,<terminal>]) ...
.probe I(<instance>:<terminal>) ...
.probe alli [I(...) ...]
```

- `<instance>`: the instance as the hierarchy names it, `x1.x2.m3`, or as the
  flat circuit names it, `m.x1.x2.m3`. One of the top level is `m1`.
- `<terminal>`: a position counted from 1; a letter of the device (MOS `d g s
  b`, BJT `c b e s`, JFET and MESFET `d g s`, diode `a`, and `c` or `k` for
  the cathode); the name of a port of a subcircuit instance; the name an OSDI
  (Verilog-A) device gives the terminal in its model. A letter, and the name
  of a terminal of an OSDI device, is taken in any case; the name of a port
  follows the case mode (section 8). A port can also be given by its
  position.
- Without a terminal: every terminal of the instance, each as its own probe
  (section 2). An instance with two terminals is one probe (section 3).
- `alli`: as before, a probe on every terminal of every instance of the top
  level. An OSDI device of the top level gets its probes from here. A probe
  written next to `alli` that is inside an instance is inserted as well.

Two spellings of one terminal, `x1.x2.m3,1` and `x1.x2.m3:d`, are one probe
and one vector.

`probe_version` (number, 1) is set at start-up in `ft_cpinit()`, which the
executable and the shared library both run. A script, or a tool that writes
decks, tests for the feature with `$?probe_version`. The number goes up when
the names or the sign of the vectors change. An ngspice without the feature
does not set it, and drops a probe inside an instance with `Warning: Could not
find the instance line for <instance>`.

## 2. Names of the vectors

The vector is `<path>:<terminal>#branch`. `<path>` is the instance as the
hierarchy names it: `x1.x2.m3`, also when the probe was written as
`m.x1.x2.m3`. `<terminal>` is the name that `.probe` has always given that
terminal at the top level (`get_terminal_name()` in `inpc_probe.c`), so that
a terminal has one name at every depth: moving a block into a subcircuit only
puts the path in front.

| device | terminals, in order |
| --- | --- |
| M | `d g s b_tj tc n6 n7` |
| Q | `c b e s t` |
| J, Z | `d g s` |
| D | `a c` |
| X | the names of the ports of the subcircuit |
| N (OSDI) | the names of its model, from the device table of the loaded `.osdi` |
| any other | `n1 n2 ...` |

`b_tj` is the name the top level has had for the fourth node of an M line,
which is the bulk of most models. The letters that are typed (`b`, `k`, a
position) are spellings of the terminal; the name of the vector follows the
table.

An instance of two terminals without a terminal gives one vector named after
the instance: `x1.r1#branch`, `x1#branch` (section 3).

## 3. Sign

The current is the current *into* the terminal, from the net into the
instance. The four terminals of a MOS sum to zero; the port of a subcircuit
instance carries what flows into the instance there.

Without a terminal, an instance of two terminals is one probe: the current
out of its second terminal, into the net. That is where `.probe` has put it at
the top level. For an element of two terminals it is the current through the
element from its first terminal to its second, the same as the current into
the first. For a subcircuit instance of two ports it is not, when something
inside leads to ground or to a global node: the current into the first port
is `x1:<first port>`, one word away.

The source is a 0 V voltage source in series with the terminal, so the value
is what the solution gives, in every analysis that solves the circuit: `op`,
`dc`, `ac` (a complex current), `tran`. The tests cover `op`, `ac` and `tran`;
a `dc` sweep was checked by hand and has no test.

## 4. Saving

Each probe adds `.save <name>#branch`. When the deck has no `.save`, `.probe`
adds `.save all` as before, and then all node voltages are saved too. With a
`.save` of its own, the deck gets what it saves and the probes.

The new node between the source and the terminal has `#probe_int_` in its
name, which `.save all` does not save (`outitf.c`), so the list of vectors has
the probes and not the nodes they need.

## 5. How it is built

1. `inp_probe()` (reader, before the subcircuits are expanded) inserts
   what it can serve at the top level, as before. It hands over, as
   `.iprobe <text>` cards right after the title and in the order of the deck:
   a probe of an instance that is not one of the top level; any probe of an
   OSDI device; a probe of a port of a subcircuit instance of the top level,
   by name or by position (so that two spellings of one port are one source).
   With `alli`, the OSDI devices of the top level as every terminal.
2. The `.iprobe` cards are in the copy of the deck that `reset` reads again,
   where `inp_probe()` does not run, so the probes survive `reset`.
3. `inp_stb_collect()` collects the `.iprobe` cards next to the probes of
   `stb`, and comments them out.
4. A port of a subcircuit instance is put on a new node when the instance is
   expanded (`inp_stb_xcard()`); a terminal of an element in the flat deck
   (`stb_element()`). A probe without a terminal becomes one probe per
   terminal at that point, when the number of terminals and the names of the
   ports are known.
5. `inp_stb_insert()` writes the 0 V sources, `v.<new node>`, with the net
   as their positive node, so that the branch is the current into the
   terminal; the source of section 3 the other way round.
6. `inp_stb_saves()` adds the `.save` cards.
7. After the circuit is made, `inp_stb_rename()` renames each source to
   `<path>:<terminal>` (or `<path>`), in the circuit and in its hash table of
   instances, so that its branch vector has the name of section 2.

## 6. With `stb`

A terminal can be the probe of `stb` and a current probe at once. They do not
share a source: on the same terminal the two sources are in series, and a
0 V source changes nothing. The stb names and results are those without the
current probe (`probe-stb-shared.cir`).

## 7. Refusals

A probe that cannot be inserted prints one line, and the deck is read and run
without it:

```
probe: I(x1.nope:d) not inserted: no instance named x1.nope
probe: I(x1.x2.m3:q) not inserted: x1.x2.m3 has no terminal named q (1 to 4, or d g s b)
probe: I(x1.x2.m3:7) not inserted: x1.x2.m3 has 4 terminals
probe: I(x1.x2:zz) not inserted: x1.x2 has no terminal named zz (1 to 3, the ports of stage: i d s)
probe: I(x1.k1:1) not inserted: x1.k1 has no terminals
probe: I(x1.a1:1) not inserted: x1.a1 is a code model instance; place a voltage source
```

A probe that is inserted prints nothing, as `.probe` does. The refusals of the
top level are those `.probe` has printed before.

## 8. Names and the case mode

The names follow the case mode of the netlist as the probes of `stb` do
(`stb-analysis.md`, section 10). Under `fold` the names are in lower case.
Under `preserve` they keep the spelling of the deck, and two spellings of one
name are one probe: `I(X1,O)` and `I(x1,o)` are one source, `X1:O#branch`.
Under `distinguish` they are two names: a probe of an instance the deck spells
otherwise is refused, and the reason names the spelling the deck has; a port
spelled otherwise is no port of the subcircuit.

## 9. Accuracy

- The value is exact to the solution: for a resistor between two nodes it
  agrees with `(v(a) - v(b))/R` to rounding (1e-15 A in the tests).
- `@`-parameters of a device (`@m.x1.m3[id]`) are taken at the iterate before
  the last, so they agree with the probe only within the tolerance of the
  solution (0.1 % in the tests).
- A terminal current includes what the device stamps at that terminal: a MOS
  bulk carries the gmin of its junctions (about 1.7e-12 A here), a diode in
  reverse its gmin. The terminals of a BJT on its line sum to the gmin current
  of the substrate node when that node is not on the line (1.8e-12 A).
- Two copies of one circuit at different places in the matrix agree to the
  tolerance of the solution, not to rounding (`probe-depth-names.cir`).

## 10. Limits and what is not covered

- `vd(...)` and `p(...)` of `.probe` are still for the top level only.
- `alli` reaches the top level only; a terminal inside an instance needs its
  own `I(...)`.
- The probes are made when the deck is read. A terminal cannot be added from
  the command line after that; `save` can only name vectors that exist.
- A code model (XSPICE `a`) instance, a coupling (`K`), and an `E`/`G` source
  with an expression are refused; a 0 V source in the netlist is the way
  there.
- At the top level the code of `.probe` is unchanged. Two spellings of one
  terminal of an element there, `I(m1,1) I(m1,d)`, still give two sources of
  one name, as before; inside an instance, and for the ports of a subcircuit
  instance at the top level, they give one.
- The shared library (`--with-ngshared`) has not been built in this work; the
  probes are made again after `reset` in the executable
  (`probe-reset.cir`). The state of `inpc_stb.c` is freed with that of `stb`
  (`inp_stb_discard()`).

## 11. Tests

`tests/regression/probe/`:

| deck | what |
| --- | --- |
| `probe-hier.cir` | the terminals of a MOS two levels down, a port of a nested instance, an element of two terminals, a position, a port of the top level by name; against the hand solution |
| `probe-names.cir` | spellings: a position and a letter as one vector, the name in the circuit, all terminals, a BJT, the diode letters |
| `probe-depth-names.cir` | one circuit at the top level and inside an instance: the same names with the path in front, the same values |
| `probe-subckt-ports.cir` | a subcircuit instance of two ports without a port, at the top level and inside; two spellings of one port at both depths |
| `probe-ac.cir`, `probe-tran.cir` | a capacitor, a port and a MOS drain in AC; a capacitor charging through a resistor in TRAN, against the exponential |
| `probe-refuse.cir` | the refusals |
| `probe-reset.cir` | the probes after `reset` |
| `probe-save.cir`, `probe-saveall.cir` | what is saved with and without a `.save` |
| `probe-stb-shared.cir` | an `stb` probe and a current probe on one terminal |
| `probe-alli.cir` | `alli` with a probe inside an instance |
| `probe-legacy.cir` | the top level: the names and values of before |
| `probe-version.cir` | `probe_version` is set, and is 1 |

The two `probe-term-case.cir` decks check `preserve` and `distinguish`.
