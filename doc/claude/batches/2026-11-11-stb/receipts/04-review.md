# Receipt 04: after the review of the port

Tree: `ver_53` at `68f2a343d`. The review of rows 01 to 11 had five findings,
all minor. Logs are `build-ver_53/claude-check-<step>-<dir>.log` as before.

## 1. The letter of a flattened name (spec) - corrected, `68f2a343d`

Reproduced under `-D casemode=distinguish`, `Vprb` inside `X1`:

```
stb V.X1.Vprb ...   runs
stb v.X1.Vprb ...   Error: stb: probe v.X1.Vprb: no such instance in the circuit
alter v.X1.Vprb dc=0   Error: no such device or model name v.X1.Vprb
```

The code agrees with `alter`; section 10 of the spec claimed more. It now
limits "any case" to the places that ask for the kind of an element and says
that the letter in front of a flattened name is part of the stored name.

## 2. Row 07 of receipt 02 had no log behind it - re-run

The saved logs of row 07 show 121 PASS, 1 FAIL (`case`) and 32 PASS, 1 FAIL
(`casedist`), both `stb-term-hier-case.cir`; they predate the final `.out`.
The tree was checked out at `4f4d69344`, rebuilt and installed, and the
directories run again: case 122, casedist 33, stb 55, lint 2, 0 FAIL
(`claude-check-c7-rerun-*.log`). Receipt 02 row 07 carries the correction.

## 3. A miss by case under distinguish names the other spelling - `3e21745a5`

Decision 2 of `doc/claude/decisions/0001-distinguish.md` applies: a name is
looked up, the lookup fails, a name that differs only in case exists.

RED: `tests/regression/casedist/stb-near-miss-case.cir`, 13 verdicts, written
with its expected lines first. On the code of `5a40d5aaf`
(`claude-check-r1-red-regression_casedist.log`) the seven near-miss verdicts
fail, each because the error has no pointer:

```
near-miss-source  near-miss-flat  near-miss-path  near-miss-terminal
near-miss-element  near-miss-element-path  near-miss-localgnd
```

and the six others (four misses with no such name, two runs with the deck's
spelling) pass.

GREEN (`claude-check-r1-*.log`): casedist 35, case 123, stb 59, lint 2,
0 FAIL. `tests/regression/casedist/stb-term-hier-case.out`: three
`not inserted` lines gain the new text (`X4.xl`, `X6.XL.g1`, `m1`); the
verdicts of that deck are the same. Under `fold` and `preserve` no message
changes. valgrind (`--leak-check=full`) on the new deck: no invalid access
and no definite leak; one "possibly lost" block of 512 bytes, which
`stb-localgnd-case.cir` shows in the same place.

Not covered: a subcircuit instance given without a port (`stb x1` for `X1`)
and a port name that misses by case (that message lists the ports); spec
section 11.

## 4. What a jump leaves (spec) - corrected, `68f2a343d`

`inp_netlist_read_reset()` has three callers (`shared_exit()`,
`ft_sigintr_cleanup()`, `totalreset()`); `inp_stb_discard()` is called only
in the third. No call was added: `inp_stb_collect()` begins with
`inp_stb_discard()` in every read that goes on to make a circuit, and the
only use of the state outside `inpc_stb.c` (`inp_stb_pending()` in the
subcircuit expansion, which the XSPICE bridge expansion also runs) lies
inside a read, after that collection. The spec sentence now says this.

## 5. `tf` ends ngspice (outside the port) - not changed

Reproduced, and not a matter of the case mode or of this batch. In the
default mode:

```
* tf
V1 in 0 dc 1
Vprb in p2 dc 0
R1 p2 0 1k
.control
op
tf v(nosuch) vprb
.endc
.end
```

prints `Internal Error: incomplete CKTunsetup(), this will cause serious
problems` and `ERROR: fatal error in ngspice, exit(1)`. The same happens on a
build of upstream `pre-master-48` and on a released ngspice-45.2. Without the
`op` before it the `tf` does not fail. Under `distinguish` `tf v(p2) vprb`
is this case because `p2` is not the node `P2`. Left for a separate issue.

## Final state

`env -u DISPLAY make -C build-ver_53 -k check`,
`build-ver_53/claude-check-r1-full.log`: **435 PASS, 33 FAIL**; the 33 are the
33 of the baseline, line for line. stb 59, case 123, casedist 35 (one more
than receipt 03), lint 2 (`identity lint: 11 comparisons, baseline matches`;
`tests/lint/identity.baseline` unchanged).

`xvfb-run -a make -C build-ver_53/tests/regression/pipe -k check`: 32 PASS,
1 FAIL (`resource-keyword-case.cmd`). With it the tree stands at 467 PASS and
1 FAIL.
