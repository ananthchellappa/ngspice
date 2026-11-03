# Receipt 01: confirming the four upstream reports

Date 2026-11-02. Nothing posted, pushed or filed. All configure, make and PDK runs went through `ngheavy`, `-j4`. No exit 137.
Scratch: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/rep`. Not committed there; `tipfix.diff` and `final.sh` live there.

## Results first

- **Sky130: first bad commit is `940bb50fa`.** Confirmed by experiment: its only parent `a0999002f` loads and it does not. No bisect was needed.
- **Report 1 is a duplicate.** Patch ticket #138 (Ethan Sifferman, hours old on 2026-11-02) gives the same cause and a fix. So SUBMIT.md has a comment on #138, not a new ticket.
- **Reports 2, 3, 4: named commit confirmed first bad each**, open at tip, no duplicate found in what could be read.
- **Correction to our own ledger.** Receipt 07 and receipt 12 item 3 say `r3 gnd(x) 0 1k` gets value 0 / 1e-12. The deck does not show that. The warning prints, but `@r3[resistance]` reads 1000 and the solve fits 1k. What changes is the nodes. See section 4. Those receipts were not edited.
- **Report 4 was not reverted upstream.** Only the fork reverted it (`fa5e0691f`).

## 1. Latest upstream

`git fetch https://git.code.sf.net/p/ngspice/ngspice pre-master-48:refs/upstream/pre-master-48`: `ac2798364..3adcd96a0`. Tip **`3adcd96a02ff5aa408e5bc63d8e93dcfd84a14e3`**.

Since `ac2798364`, one commit: `3adcd96a0` "CMake: no ctest with the Windows GUI binary, document it", `README-CMAKE.md` and `tests/CMakeLists.txt` only. It touches none of the four.

Still at tip, by log and source: `git log 940bb50fa..tip -- src/frontend/inpcom.c src/frontend/numparam` lists only `6e6d91f6b Remove linker warnins`; `58e4cf26b` is the last commit on `inpgtok.c`; `af240ba02` is the last on `dctrcurv.c`; `in_range()` in `inpgmod.c` is the `03ef19244` text. No `Revert` of any of them in the log. Each also reproduces on the tip build below.

## 2. Builds

Each: `git archive <commit> | tar -x`, `ngheavy ./autogen.sh`, `ngheavy ../configure --prefix=<dir>/stage --with-readline=no`, `ngheavy make -j4`, `ngheavy make -j4 install` (script `rep/b.sh`). All rc 0. None needed the `tests/xspice/analog` revert: the directory is present where `configure.ac` names it.

| label | commit | what |
|---|---|---|
| tip | `3adcd96a0` | latest |
| p940, c940 | `a0999002f`, `940bb50fa` | report 1 |
| p03, c03 | `e73cbd263`, `03ef19244` | report 2 |
| p58, c58 | `6a10f7af2`, `58e4cf26b` | report 3 |
| paf, caf | `81f5b2c6d`, `af240ba02` | report 4 |
| tip138 | tip + `inpcom.c` hunk of #138's `agauss-brace.patch`, read first, applied with `patch` | scratch only |
| tip0f | tip + `src/` of `0f763b135`, by hand where needed | scratch only |
| tipfix | tip + my own rewrite of `{a} + b` to `{(a) + b}` in the keep branch | scratch only |
| v45 | `/usr/bin/ngspice` 45.2 | system |

Each named commit has one parent (`git log -1 --format=%p`), so parent good and commit bad is first bad.

tip0f adaptations: hunk 2 of `subckt.c` rejected because the fork's line has `cistrstr` and upstream's `strstr`; those float-to-double edits were made with `sed`. `inpdefs.h` needed `#include <stdbool.h>`: the first make failed with `unknown type name 'bool'`.

tipfix behaved the same as tip138 on every run here: same i(vd) on all 49 corners (`join` found 0 differences). #138 got there first and its change is smaller, so tipfix is not offered.

## 3. Report 1: sky130

Deck: `upstream-decks/u28-sky130-load.cir` with its `.spiceinit` (`set ngbehavior=hsa`, `set ng_nomodcheck`), PDK `/home/analog/private/xschem-private/sky130A/models/libs.tech/combined/sky130.lib.spice`.

| build | exit | result |
|---|---|---|
| p940 | 0 | `i(vd) = -1.69329e-05` |
| c940 | 1 | 36 errors |
| tip | 1 | 36 errors, output `cmp` equal to c940 |

Exact error, first of 36 and the end:

```
Error in netlist line no. 1890, new internal line no. 105:

 ; sign expected.
...
Error in netlist line no. 1961, new internal line no. 212:

 ; sign expected.

ERROR: fatal error in ngspice, exit(1)
```

The numbers are lines of the expanded deck; the message names no file. With `set ngdebug` the card text prints. The first is `.param sw_polycd={0.0+corner_factor*0.0} + process_mc_factor*mc_pr_switch*agauss(0,1.5e-09,1)`, source `continuous/parameters_fet_tt.spice:19`.

**Why.**

- `nupa_assignment()` (`src/frontend/numparam/xpressn.c`, message at line 1530 at tip): `getexpress()` takes a `{...}` group as the whole expression and stops at `}`. Anything after it that is not `;` gets ` ; sign expected.`. `nupa_done()` in `spicenum.c` then calls `controlled_exit(EXIT_FAILURE)` in batch mode.
- `inp_fix_agauss_in_param()` used to turn every such `.param` into a `.func`, whose body has its braces stripped by `inp_strip_braces()`, so `{a} + b` worked there.
- `940bb50fa` keeps a top-level `.param` whose identifiers are all top-level (`stat_expr_is_toplevel`). So these lines now reach numparam.

**Without a PDK**, `decks/1-brace-sum.cir`:

| deck | v45 | p940 | c940 | tip | tip138 |
|---|---|---|---|---|---|
| `.param p = {1+0} + c*agauss(3,0,1)` | 7 | 7 | exit 1 | exit 1 | 7 |
| `.param p = {1+0} + c*3` (no statistical function) | exit 1 | exit 1 | exit 1 | exit 1 | exit 1 |
| `.param p = {1+0 + c*agauss(3,0,1)}` | 7 | 7 | 7 | 7 | not run |
| same as the first, inside a `.subckt`: `r1 n 0 {p}`, 1 mA | 7e-3 | 7e-3 | **1e-3, exit 0, no message** | **1e-3** | 7e-3 |

The second row is why sky130 only loaded through the `.func` path. The fourth is new, and I did not trace why the tail is dropped there.

**Lines affected, measured** (`rep/scan.py` over `sky130A/models`, 92133 lines; `grep -E '^\.param .*\} *\+'` agrees): **128 lines in 11 files**, all `continuous/parameters_*.spice`, all holding a statistical function. Per file: `fet_{ff,fs,sf,ss,tt}` 10 each, `res_{high,low,nom}` 21 each, `cap_{high,low,nom}` 5 each. Those files have no `.subckt`. A corner includes 10+21+5 = **36**, which matches the 36 errors.

**All corners**, u28 with each `.lib` section (`corn/loop.sh`, 49 sections):

| build | result |
|---|---|
| p940 | 49 of 49 exit 0 |
| tip | 49 of 49 exit 1, 36 errors each |
| tip138 | 49 of 49 exit 0. 24 sections equal p940's i(vd) to 6 digits; the 25 `_mm` and `mc` differ, e.g. `tt_mm` -1.72783e-05 to -1.68423e-05, `mc` -1.72876e-05 to -1.73474e-05 |

The differing 25 are the sections that draw. I read that as the changed draws, not checked statistically.

**Upstream's purpose is real.** The commit message cites thread `c13bbd10c6`. `.param aga = agauss(1,2,3)` referenced twice, `numdgt=7`: p940 5.5526541e-01 and -3.320390e-01; c940 and tip 5.5526541e-01 twice. Same on tip138 for `{0} + agauss(0,1,1)`: -6.671019e-01 twice; p940 -6.671019e-01 and -1.998058e+00. `set stat_param_as_func` on tip loads tt with -1.69329e-05 (tt only).

## 4. Reports 2, 3, 4

Commit messages:

- `03ef19244`: "Correct the limits for binning of MOS model. Make results equal to other simulators. Patch provided by Vilem Kledrowetz." From thread `56ed12d6f3`, where he states the standard rule is min <= value < max and that foundry limits are shifted by picometers. The thread does not discuss a size on the top-most max.
- `58e4cf26b`: "E-8: In INPgetNetTok() '(' will terminate a token". No ticket. Its code comment states the purpose: the OSDI `.model` with no space before `(`. The `E-` numbers belong to the author's enhancement list; thread `851d765954` mentions one and an external GitHub repo, which I did not read.
- `af240ba02`: "E-62: Sweep type code for a generic instance-parameter sweep". No ticket.

Every deck from `decks/`, run by `rep/final.sh` (`ngheavy bash final.sh`; its rc 1 is the last `[ ] &&` test in the script, not a deck). `HOME` was a directory with an empty `.spiceinit`, or `set ngbehavior=hsa` for the hsa row, or both hsa and `ng_nomodcheck` for sky130.

```
1-brace-sum.cir v45 exit=0
    v(1) = 7.000000e+00
1-brace-sum.cir p940 exit=0
    v(1) = 7.000000e+00
1-brace-sum.cir c940 exit=1
    ERROR: fatal error in ngspice, exit(1)
1-brace-sum.cir tip exit=1
    ERROR: fatal error in ngspice, exit(1)
1-brace-sum.cir tip138 exit=0
    v(1) = 7.000000e+00
1-brace-sum-subckt.cir v45 exit=0
    v(1) = 7.000000e-03
1-brace-sum-subckt.cir p940 exit=0
    v(1) = 7.000000e-03
1-brace-sum-subckt.cir c940 exit=0
    v(1) = 1.000000e-03
1-brace-sum-subckt.cir tip exit=0
    v(1) = 1.000000e-03
1-brace-sum-subckt.cir tip138 exit=0
    v(1) = 7.000000e-03
1-sky130-load.cir p940 exit=0
    i(vd) = -1.69329e-05
1-sky130-load.cir c940 exit=1
    ERROR: fatal error in ngspice, exit(1)
1-sky130-load.cir tip exit=1
    ERROR: fatal error in ngspice, exit(1)
1-sky130-load.cir tip138 exit=0
    i(vd) = -1.69329e-05
2-outer-max.cir v45 exit=0
    v(d) = 1.000000e+00
     model nn.2
2-outer-max.cir p03 exit=0
    v(d) = 1.000000e+00
     model nn.2
2-outer-max.cir c03 exit=1
    could not find a valid modelname
2-outer-max.cir tip exit=1
    could not find a valid modelname
2-outer-max.cir tip0f exit=0
    v(d) = 1.000000e+00
     model nn.2
2-hsa-finger.cir p03 exit=0
    i(vd) = -2.43486e-03
     model x1:card.1
2-hsa-finger.cir c03 exit=1
    could not find a valid modelname
2-hsa-finger.cir tip exit=1
    could not find a valid modelname
2-hsa-finger.cir tip0f exit=0
    i(vd) = -2.43486e-03
     model x1:card.2
2-hsa-finger.cir p03 exit=0
    i(vd) = -2.43486e-03
     model x1:card.2
2-hsa-finger.cir c03 exit=0
    i(vd) = -2.43486e-03
     model x1:card.2
2-hsa-finger.cir tip exit=0
    i(vd) = -2.43486e-03
     model x1:card.2
3-paren-node-value.cir v45 exit=0
    a = 0.000000e+00
    a(x = 0.000000e+00
    v1#branch = 0.000000e+00
    x = 1.000000e+00
    @r3[resistance] = 1.000000e+03
3-paren-node-value.cir p58 exit=0
    a = 0.000000e+00
    a(x = 0.000000e+00
    v1#branch = 0.000000e+00
    x = 1.000000e+00
    @r3[resistance] = 1.000000e+03
3-paren-node-value.cir c58 exit=0
    Warning: Value of resistor r3 is too small, set to 1.000000e-12
    a = 5.000000e-01
    v1#branch = -5.00000e-04
    x = 1.000000e+00
    @r3[resistance] = 1.000000e+03
3-paren-node-value.cir tip exit=0
    Warning: Value of resistor r3 is too small, set to 1.000000e-12
    a = 5.000000e-01
    v1#branch = -5.00000e-04
    x = 1.000000e+00
    @r3[resistance] = 1.000000e+03
3-paren-node-gnd.cir v45 exit=0
    v(1) = 1.000000e+00
    gnd(x = 0.000000e+00
    v1#branch = -5.00000e-04
    x = 5.000000e-01
    @r3[resistance] = 1.000000e+03
3-paren-node-gnd.cir p58 exit=0
    v(1) = 1.000000e+00
    gnd(x = 0.000000e+00
    v1#branch = -5.00000e-04
    x = 5.000000e-01
    @r3[resistance] = 1.000000e+03
3-paren-node-gnd.cir c58 exit=0
    Warning: Value of resistor r3 is too small, set to 1.000000e-12
    v(1) = 1.000000e+00
    gnd = 5.000000e-01
    v1#branch = -5.00000e-04
    x = 5.000000e-01
    @r3[resistance] = 1.000000e+03
3-paren-node-gnd.cir tip exit=0
    Warning: Value of resistor r3 is too small, set to 1.000000e-12
    v(1) = 1.000000e+00
    gnd = 5.000000e-01
    v1#branch = -5.00000e-04
    x = 5.000000e-01
    @r3[resistance] = 1.000000e+03
3-paren-node-stop.cir v45 exit=0
    net(0 = 1.000000e+00
    v1#branch = -1.00000e-03
3-paren-node-stop.cir p58 exit=0
    net(0 = 1.000000e+00
    v1#branch = -1.00000e-03
3-paren-node-stop.cir c58 exit=1
    Warning: Value of resistor r1 is too small, set to 1.000000e-12
     unknown parameter (1) 
3-paren-node-stop.cir tip exit=1
    Warning: Value of resistor r1 is too small, set to 1.000000e-12
     unknown parameter (1) 
3-osdi-model-nospace.cir p58 exit=0
    i(v1) = -1.00000e+00
    i(v2) = -5.00000e-01
3-osdi-model-nospace.cir c58 exit=0
    i(v1) = -5.00000e-01
    i(v2) = -5.00000e-01
3-osdi-model-nospace.cir tip exit=0
    i(v1) = -5.00000e-01
    i(v2) = -5.00000e-01
4-dc-inst-param-scale.cir v45 exit=0
    i(vd) = -1.25000e-04
    @m1[w] = 1.000000e-06
    i(vd) = -1.25000e-04
    @m1[w] = 1.000000e-06
    i(vd) = -1.25000e-04
4-dc-inst-param-scale.cir paf exit=0
    i(vd) = -1.25000e-04
    @m1[w] = 1.000000e-06
    i(vd) = -1.25000e-04
    @m1[w] = 1.000000e-06
    i(vd) = -1.25000e-04
4-dc-inst-param-scale.cir caf exit=0
    i(vd) = -1.25000e-04
    @m1[w] = 1.000000e-06
    0	1.000000e+00	-1.25000e-04	
    1	2.000000e+00	-2.50000e-04	
    2	3.000000e+00	-3.75000e-04	
    @m1[w] = 1.000000e-12
    i(vd) = -1.26010e-10
4-dc-inst-param-scale.cir tip exit=0
    i(vd) = -1.25000e-04
    @m1[w] = 1.000000e-06
    0	1.000000e+00	-1.25000e-04	
    1	2.000000e+00	-2.50000e-04	
    2	3.000000e+00	-3.75000e-04	
    @m1[w] = 1.000000e-12
    i(vd) = -1.26010e-10
4-dc-rebin-sweep.cir tip exit=0
    0	2.000000e-06	-1.21063e-04	
    1	5.000000e-06	-3.21578e-04	
4-dc-rebin-op.cir tip exit=0
    i(vd) = -7.35168e-05
```

The second `2-hsa-finger.cir` block is without hsa. Its first block is with hsa.

Reading these:

- **Report 2:** p03 fine, c03 fails, on both decks. tip0f passes both.
  Sky130 sizes, nfet_01v8, tt, p940 against tip138 (tip138 because tip cannot load the PDK; this spans more than one commit):

  | size | mode | p940 | tip138 |
  |---|---|---|---|
  | l=20.2 w=2 | no hsa | -1.47582e-06 | no modelname |
  | l=1 w=1010 | no hsa | -1.32937e-02 | no modelname |
  | l=20.2 w=2, l=1 w=1010 | hsa | no modelname | no modelname |
  | l=1 w=15 nf=5 | hsa | -1.86245e-04 | no modelname |
  | l=1 w=49 nf=7 | hsa | -6.37262e-04 | no modelname |
  | l=1 w=15 nf=5, w=49 nf=7 | no hsa | -1.91929e-04, -6.37262e-04 | same |

- **Report 3:** the OSDI fix is real, p58 -1 to c58 -0.5. `3-paren-node-gnd`: the printed v(x) does not change, only the node `gnd(x` becomes `gnd`. So the value deck uses `a(x)`, where the current moves from 0 to -5e-4. `@r3[resistance]` is 1000 on every build while the warning says 1e-12.
- **Report 4:** paf has no such sweep; caf and tip give the bad second op. Without scale (scratch `w/4-noscale.cir`) the op after the sweep stays -1.25000e-04 on caf and tip. `git log --grep=E-62` on upstream lists only `af240ba02`. On the fork: `92511d009`, reverted `fa5e0691f`.

## 5. What tip0f shows

Both report 2 decks pass on 3adcd96a0 with `0f763b135` adapted. `make check` was not run on tip0f or tip138.

## 6. Duplicate check

Read with WebFetch on 2026-11-02, which returns a model's summary of the page and not the raw text, except the attachment below, fetched with curl and read in full.

| page | read | found |
|---|---|---|
| `patches/` pages 1, 2 (all 40) | titles | **#138 report 1.** #137 points to it. Nothing for 2, 3, 4 |
| `patches/138/`, `patches/137/`, `patches/130/` | ticket | #138 as above. #130 has a `.dc` on `.param` diagnostic, not instance parameters |
| `patches/138/attachment/agauss-brace.patch` | full | from commit `3d83d98bc`, dated 2026-11-02 10:36 -0800, on `3adcd96a0` |
| `bugs/` pages 1, 2 of 7 (tickets 888 to 803) | titles | nothing for the four |
| `bugs/869/` | ticket | `INPgetStr` and quoted strings, not `INPgetNetTok` |
| `discussion/` index; ngspice-devel page 1 of 21; ngspice-users page 1 | titles | nothing direct |
| threads `c13bbd10c6`, `56ed12d6f3`, `851d765954`, `5d3537b2` | thread | `c13bbd10c6` has sky130 only as a sharing concern, no load failure. `56ed12d6f3` no top-edge discussion. Others nothing |
| `search/?q=INPgetNetTok` | page 1 of 74 | fuzzy, 1850 results, not useful |

**Not read:** bugs pages 3 to 7, older than the four commits (page 2 ends at 2025-09, and the commits are 2026-07 or later), so unlikely but not excluded; ticket bodies other than those above, so a mention inside one is possible; forum pages after the first; the mailing lists.

## 7. Still unverified

- What HSPICE or Spectre do with a size on the outermost max.
- Why a `.param {a} + b` in a `.subckt` silently drops the tail: measured, not traced.
- That the 25 drawing corners differ only because of the draws.
- `DCTsetInstParam()` and `mos1par.c` as the path for report 4: read from code, with only the deck's result measured.
- Whether `(` in a node name was ever meant to be legal.
- `make check` on the scratch builds with #138's change or `0f763b135`. #138's own deck was run and prints `INFO: ok`.
- Whether my sky130 copy equals the one #138 used. It is not a stock release I identified: it is the copy under `xschem-private`, the one the earlier measure runs used, and receipt 01-triage records the owner patched `nfet_20v0_nvt` in it. The `parameters_*` line count could differ on another copy.
