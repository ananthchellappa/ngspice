# Ledger: the stb loop-gain analysis ported to ver_53 (2026-11-11)

**What:** the nine commits of the branch `stb-upstream` (on upstream
`pre-master-48`, base `8e03f9570`) cherry-picked onto `ver_53` (off `ver_52`
at `5a5251547`), one fork commit per upstream commit, each with `-x`. Where the
fork version differs, the commit message has a `Fork port:` paragraph.
The feature as built is specified in `doc/claude/specs/stb-analysis.md`; the
user guide is `README.stb`.

**Build:** `build-ver_53/`, `../configure --prefix=$PWD/stage
--with-readline=yes --enable-pss --enable-cider` (the flags of `build-ver_52`).

## Rows

| # | upstream | fork | what the fork changes | receipt |
|---|----------|------|-----------------------|---------|
| 00 | - | `5a5251547` | baseline before the port: 363 PASS, 33 FAIL with `DISPLAY` unset; all 33 are in `tests/regression/pipe`. Under a virtual X server that directory gives 32 PASS and 1 FAIL (`resource-keyword-case.cmd`), which is the known state of 395 PASS and 1 FAIL | receipts/01-baseline.md |
| 01 | `7c68fb832` | `af1c1b90e` | nothing | receipts/02-port.md |
| 02 | `ec1d4e137` | `1c0d71374` | probe word folded only under `fold`; instances and path parts compared under the case mode; no optimizer guard; `.stb` dispatch with `cieq()`; save exemption around `ft_savenone()`; 2 new case decks | receipts/02-port.md |
| 03 | `66d8731f2` | `41bcf5b21` | lint comments on the title compares; no `EVTswitch_plot()` call | receipts/02-port.md |
| 04 | `a353c4c52` | `f290dc49c` | nothing | receipts/02-port.md |
| 05 | `bc9f63414` | `d428e339a` | saves matched to the operating point plot with `name_eq_query()`; 1 new case deck | receipts/02-port.md |
| 06 | `455a566a6` | `3a3d7f53f` | `inpc_stb.c` under the case mode (`ng_ideq()` and its mode arms, fold only under `fold`, device letters in any case); `inp_stb_discard()` in `totalreset()`; 2 new case decks | receipts/02-port.md |
| 07 | `5a36ce8e0` | `4f4d69344` | paths, terminal letters, refusals and code model port types under the case mode; `glo_key()` exported from `subckt.c`; 2 new case decks | receipts/02-port.md |
| 08 | `469b9f016` | `0817a5329` | the `localgnd` node through `ng_ideq()`, `gnd` in any case; 2 new case decks | receipts/02-port.md |
| 09 | `3af952a7e` | `c600cedfe` | `README.stb` states the case-mode rule; NEWS entry under this tree's top heading | receipts/02-port.md |
| 10 | - | `e96524b85` | `doc/claude/specs/stb-analysis.md` | (the file) |
| 11 | - | this commit | final check: **434 PASS, 33 FAIL** with `DISPLAY` unset, the same 33 as row 00; with the pipe directory under a virtual X server 466 PASS and 1 FAIL (`resource-keyword-case.cmd`, unchanged). 71 new PASS: 59 in `tests/regression/stb`, 5 in `case`, 4 in `casedist`, 2 in `tests/xspice/analog`, 1 in `tests/xspice/digital`. `tests/lint` passes; `tests/lint/identity.baseline` is unchanged | receipts/03-final.md |

| 12 | - | `3e21745a5` | review: under `distinguish` the error of a probe, of the instance of a terminal probe or of a `localgnd` node that misses only by case names the other spelling; 1 new casedist deck | receipts/04-review.md |
| 13 | - | `68f2a343d` | review: spec section 10 corrected on the letter of a flattened name and on what a read left by a jump leaves behind | receipts/04-review.md |
| 14 | - | this commit | review: receipt 02 row 07 corrected from a re-run at `4f4d69344`; final check **435 PASS, 33 FAIL** with `DISPLAY` unset, the same 33; 467 PASS and 1 FAIL with the pipe directory under a virtual X server | receipts/04-review.md |

Row 11 names the commit it is part of, `5a40d5aaf`.

## Identity lint

No entry was added to `tests/lint/identity.baseline`. On the code as picked the
lint reported one comparison at row 05, 17 at row 06 (in `inpc_stb.c` and
`stban.c`), 12 at row 07 and one at row 08; each is now an `ng_ideq()` call, a
call of a mode arm that carries `case-lint: helper`, or a marked site:

- `case-lint: keyword`: five compares of a plot title with a `STB_*_TITLE`
  macro in `outitf.c` (four marked lines; a macro is not a literal to the
  lint); the port types of a code model line in `inpc_stb.c`.
- `case-lint: helper`: the two arms each of `stb_ideqn()` and
  `stb_find_name()` in `inpc_stb.c`.
- `case-lint: stored`: the `<name>_int1` node of an E or G source with an
  expression in `inpc_stb.c`.

Row 12 adds five marked sites and no baseline entry: `case-lint: helper` on
the preserve arm of the instance scan in `stban.c` (it was an `ng_ideq()`
call), and `case-lint: neither` on the four scans for a name that differs
only in case (the nodes in `stban.c`; the subcircuit instances in
`inp_stb_xcard()` and the cards by name and by path in `stb_why_missing()`,
all in `inpc_stb.c`).

## Where the fork behaves differently from the upstream branch

1. Names follow the case mode (spec section 10). Under `fold`, the default,
   the behaviour is the upstream one. Under `preserve` and `distinguish` the
   probe word is not folded; under `distinguish` `X1` and `x1` are two
   instances.
2. The names made for a terminal probe carry the spelling of the deck under
   `preserve` and `distinguish` (`v.X1#stb1`).
3. Under `preserve` a message names the probe in the spelling the symbol table
   saw first.
4. The lines of a run are printed always; upstream leaves them out while its
   optimizer runs, which this tree does not have.
5. `ngSpice_Reset` also frees what an abandoned read had collected.
6. No CMake lists in this tree.
7. Under `distinguish` a miss by case names the other spelling (row 12).

## Open

- Under `distinguish` a subcircuit instance given without a port and a port
  name that miss only by case get no pointer to the other spelling (row 12
  covers the probe, the instance of a terminal probe and the `localgnd` node).
- `tf v(<a node the circuit does not have>) <source>` after an `op` ends
  ngspice with a fatal error, in every case mode and on upstream as well;
  under `distinguish` a node that misses by case is such a node. Not part of
  this batch (receipt 04, item 5).
- `CLAUDE.md` and `AGENTS.md` do not mention `tests/regression/stb` or the
  `stb-*-case.cir` decks; left to the owner.
- Not tested: the interrupt path.
- Nothing was pushed.
