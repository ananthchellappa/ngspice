# Where Ngspice's Case Insensitivity Actually Comes From

## Summary

Ngspice is usually described as a case-insensitive simulator. The code does not
implement case insensitivity. Almost every identifier comparison in the
simulator core is case-**sensitive**: plain `strcmp` over a hash function that
does not fold case. The apparent insensitivity is produced entirely by the
frontend destroying case in the input text before the parser ever sees it.

That distinction matters, because it determines what a case-sensitivity feature
would have to change. There is no lookup layer to make case-aware. There is a
destructive text transformation to remove, and roughly twenty hard-coded
lowercase string literals downstream that silently depend on it having happened.

The folding is not confined to net names. It applies to every token on an
ordinary card: nets, instance names, subcircuit pin names, `.subckt` names,
`.model` names, parameter names, brace expressions and quoted strings.

This document records the audited source positions. Line numbers are from branch
`ver_50` at commit `2133ad4c2`.

## The primary site

`inp_read()` in `src/frontend/inpcom.c` lowercases the whole raw line in place,
per line, before the card structure is allocated:

```c
/* src/frontend/inpcom.c:1838-1840 */
/* lower case for all other lines */
for (s = buffer; *s && (*s != '\n'); s++)
    *s = tolower_c(*s);
```

This is the default arm of a long `if`/`else if` chain. Tracing an ordinary card
such as `R1 NodeA NodeB 1k` through the chain: it is not a `plot`/`gnuplot`/
`hardcopy` line, not a `print`/`eprint`/`eprvcd`/`asciiplot` line, not a CIDER
model line, does not contain `ic.file`, and is not an XSPICE `.model` line, so it
falls through to line 1839 and is folded. The card is only created afterwards, at
`inpcom.c:1918-1919`.

The original text is not retained anywhere. `inp_deckcopy()` runs at
`src/frontend/inp.c:613`, inside `inp_spsource()` — that is, after `inp_read()`
has already folded the buffer — so `card->actualLine` holds folded text too.

## The second destructive pass

`inp_casefix()` (`src/frontend/inpcom.c:3502`, folding at `3556-3557`) is a
separate in-place lowercasing pass applied later:

```c
/* src/frontend/inpcom.c:3556-3557 */
if (isupper_c(*string))
    *string = tolower_c(*string);
```

Call sites:

| Caller | Scope |
| --- | --- |
| `src/frontend/inp.c:814` | first token of every circuit card, unconditionally |
| `src/frontend/inp.c:817` | whole card, unless the first token is `.plot` or `.print` |
| `src/frontend/options.c:229` | `.option` lines, skipped when the line contains `{` |
| `src/frontend/nutinp.c:134-135` | nutmeg deck reader — token **and** whole line, with no `.plot`/`.print` exemption |
| `src/frontend/device.c:1156` | user-typed device names in legacy `show -v` |
| `src/frontend/inp.c:227` | `upper()` helper for the `listing` command; folds a copy, non-destructive |

## The sibling arms fold too

The earlier arms of the `inp_read()` chain are not escapes. They are alternative
folds with their own carve-outs:

- `plot` / `gnuplot` / `hardcopy` (`inpcom.c:1730-1789`): folds the line except
  the single token following `title`, `xlabel` or `ylabel`, which may be
  delimited by `'`, `"` or whitespace. Folding at `1739`, `1744`, `1770`.
- `print` / `eprint` / `eprvcd` / `asciiplot` (`inpcom.c:1793-1807`): folds up to
  the first `>` and preserves everything after it. The vector names are
  destroyed; the output file path survives.
- `keep_case_of_cider_param()` (`inpcom.c:222-252`): preserves case **only when
  the line contains exactly two double-quote characters** (`numq == 2`, line
  238). With zero, one, three, four or more quotes the `else` at `248-252` folds
  the entire line, quoted paths included.

Selection into the CIDER/XSPICE arms is by substring content, not prefix:
`is_xspice_model()` requires `.model` plus one of the literals `filesource`,
`table2d`, `table3d`, `d_state`, `d_source`, `d_process`, `d_cosim` on the same
physical line (`inpcom.c:415-437`). Every other code-model `.model` card is
folded in full. The CIDER arm is sticky across `+` continuation lines.

## Nothing gates any of it

No `.option`, no compatibility mode (`ps`, `hs`, `lt`, `spice3`, `ki`, `spe`,
`eg`, `xs`), and no `set` variable conditions the folding anywhere in the reader.
The `cp_getvar` calls in `inpcom.c` cover `wnflag`, `no_auto_gnd`, `addcontrol`,
`mingwpath`, `sourcepath`, `rawfile`, `soacheck`, `enable_noisy_r` and
`no_auto_braces` — none is case-related.

The SPICE3-era `INPcaseFix()` in `src/spicelib/parser/inpcfix.c` still exists but
is dead with respect to the netlist path; its only caller is the standalone
`ngproc2mod` utility.

## Downstream is case-sensitive

Every identifier table relies on the input already being folded.

| Subsystem | Mechanism | Site |
| --- | --- | --- |
| Node symbol table | djb2 hash, no folding, plus `strcmp` | `src/spicelib/parser/inpsymt.c:51`, `:166`, `:280` |
| Node storage | interned pointer assigned directly to `CKTnode->name` | `src/spicelib/analysis/cktnewn.c:36` |
| Node output | handed to plots/rawfiles verbatim | `src/spicelib/analysis/cktnames.c:28`, `src/frontend/outitf.c:532` |
| Instance names | `GENname` = interned token; `DEVnameHash` is `strcmp`-compare | `src/spicelib/devices/cktcrte.c:58` |
| Model names | `strcmp` | `src/spicelib/parser/inpdomod.c:40`, `inpgmod.c:364`, `src/misc/hash.c:374` |
| Subckt name match | `eq()` i.e. `strcmp` | `src/frontend/subckt.c:604` |
| XSPICE event nodes | `strcmp` | `src/xspice/evt/evttermi.c:304` |
| XSPICE model params | `strcmp` against the ifspec keyword | `src/xspice/mif/mifgetmod.c:206` |
| `.save` matching | `strcmp` | `src/frontend/outitf.c:1358` |

The parser itself never changes case: `INPgetTok`/`INPgetNetTok` copy the
substring (`src/spicelib/parser/inpgtok.c:111`, `:197`), `INPmkTemp` uses
`strcpy` (`inpmktmp.c:22`), and MIF takes the instance name verbatim from
`MIFgettok` (`src/xspice/mif/mif_inp2.c:206`).

## Lowercase literals that depend on the fold

These comparisons are case-sensitive against hard-coded lowercase text. They
work only because the input was folded first, and each is a breakage point for
any change that stops folding.

- `gnd` → `0` rewrite: `strstr(gnd, "gnd")` at `src/frontend/inpcom.c:2400-2407`.
- XSPICE port-type qualifiers (`%vd`, `%d`, …): `strcmp` against the
  cmpp-generated all-lowercase table at `src/xspice/mif/mif_inp2.c:757`.
- MIF value keywords `null`, `t`/`true`, `f`/`false`: `strcmp` at
  `src/xspice/mif/mifutil.c:198`.
- UDN type names: `strcmp` at `src/xspice/evt/evttermi.c:270`.
- XSPICE auto-bridge include filename, built from the lowercased `family`
  parameter: `src/xspice/evt/evtcheck_nodes.c:367`. Bridge subcircuit files must
  therefore exist in lowercase on case-sensitive filesystems.
- OSDI parameter keywords: `osdiinit.c:70-78` stores a `strtolower`'d copy of
  each Verilog-A parameter name as the `IFparm` keyword; the live matchers are
  `strcmp` in `find_model_parameter`/`find_instance_parameter`
  (`src/spicelib/parser/inpgmod.c:51`, `:65`). Terminal names are aliased
  verbatim at `osdiinit.c:104`.

## What is genuinely case-insensitive

Independent of the fold, and therefore behavior a case-sensitivity feature must
preserve:

- Device model **type** name on a `.model` card: `strcasecmp` at
  `src/spicelib/parser/inptyplk.c:37`. This is the one XSPICE-facing identifier
  with a real case-insensitive comparison.
- `.lib` section names: `strcasecmp` at `src/frontend/inpcom.c:542`. Whole `.lib`
  lines are exempt from folding, so both path and section name keep their case.
- Numeric scale suffixes: explicit dual `switch` cases for `t/T`, `g/G`, `k/K`,
  `u/U`, `n/N`, `p/P`, `f/F`, `a/A`, `m/M`, with `MEG`/`MIL` sub-checks, at
  `src/spicelib/parser/inpeval.c:143-186`. numparam's `parseunit` uses
  `toupper_c` plus `ciprefix` (`xpressn.c:612`). The `1M == 1m` milli trap is a
  language rule, not a consequence of the fold.
- Leading device letter: `inppas2.c:92-94` copies it into a local and
  upper-cases the copy for dispatch; `devmodtranslate` (`subckt.c:1836-1839`)
  lower-cases a local for the same purpose. Neither mutates the card.
- Card keyword recognition uses `ciprefix()` throughout, which folds nothing.
- Frontend vector lookup: `src/frontend/vectors.c:70-76` inserts a **lowercased
  copy** of `v_name` as the hash key and `findvec()` lowercases the query into a
  scratch buffer. `v_name` itself keeps the original case. This is the one place
  in the codebase that already implements preserve-the-name, fold-the-key, and
  it is the natural model for a case-aware identifier layer.
- cp shell command names: `strcasecmp` at `src/frontend/control.c:205`.

## Pin and port names specifically

The question of pin-name case has three different answers depending on the pin.

1. **Subcircuit formal and actual pins** are ordinary text on a `.subckt` or `X`
   card. They are folded by the read-time site and matched with `eq()`.
   `settrans()` (`subckt.c:1595-1617`) uses only `gettok`/`eq`; there is no
   `strcasecmp` or `cieq` anywhere in `subckt.c`.
2. **XSPICE code-model port names** from `ifspec.ifs` are never matched against
   netlist text at all. Connections bind **positionally by index**
   (`src/xspice/mif/mif_inp2.c:632`). Port-name case is irrelevant to
   simulation.
3. **Verilog signal names** reaching `d_cosim` never enter ngspice's case
   machinery. `vlnggen` extracts them from Verilator's generated C++ header and
   emits them into `VL_DATA` macro invocations; the C++ compiler binds them.
   Ngspice only sees the `simulation=` path.

## Round-trip verdict

Instance and net names do not round-trip. `R1`, `r1` and `R_Big` become `r1`,
`r1` and `r_big`; the original spelling is unrecoverable from the deck, the
rawfile or an error message. `R1` and `r1` on two cards is a hard
duplicate-instance error.

## Defects found during the audit

These are live and independent of any case-sensitivity work.

1. `inpcom.c:1857` lowercases `$`-prefixed tokens on `echo` lines *after* the
   whitelist has spared the rest of the line, gated on `is_control` only. So
   `setcs MyVar=X` followed by `echo $MyVar` fails with "no such variable" inside
   a `.control` block, but works in a `*ng_script` file.
2. In a `*ng_script` file the reverse mismatch occurs: `echo` is whitelisted but
   plain `set` is not, so `set MyVar = Hello` folds to `set myvar = hello` while
   `echo $MyVar` keeps its capitals.
3. `set` and `setcs` are the same C function (`src/frontend/commands.c:133`). The
   entire difference is whitelist membership at `inpcom.c:1835`, so the
   distinction exists only for text read from a file and vanishes for text
   entered any other way.
4. `inp_casefix()`'s `keepquotes` polarity is inverted (`inpcom.c:3545-3557`). On
   `keepquotes` lines (`.param`, `.subckt` containing `="`, `X` lines containing
   a quote) the quoted text **is** folded; on other lines the quoted text is
   preserved but the quote characters are blanked. With an odd number of quotes
   the line tail escapes both the fold and the non-printable substitution.
5. `keep_case_of_cider_param()` requires exactly two double quotes
   (`inpcom.c:238`); four or more folds the whole line including paths. This is
   the defect recorded in `doc/codex/issues/0005`.
6. `src/frontend/vectors.c:100` reads `if (tolower(word[0] != 'a'))` — the
   parenthesis encloses the comparison, so `tolower` folds a boolean. The
   `all`/`allv`/`alli`/`ally`/`alle` wildcards are therefore matched
   case-sensitively.
7. `src/frontend/device.c:1504-1507` compares a device name with `strcmp`
   against a pattern already known to contain `*`, `[` or `?`, which can
   essentially never match.
8. A `*#`-prefixed command in a script or `.control` block loses the whitelist,
   because `ciprefix("echo", buffer)` is tested against the buffer including the
   `*#` prefix (`src/frontend/inp.c:713`).
9. The cp dispatcher looks up command names with `strcasecmp`
   (`src/frontend/control.c:205`) but control-flow keywords with `strcmp`, so
   `PRINT` works interactively while `IF`, `WHILE`, `FOREACH` and `END` do not.

## Method

Twelve agents across six lenses (input path, node table, instances and models,
XSPICE, control language, exceptions), each finding independently and then
adversarially re-verified against the source; 115 claims survived verification,
92 confirmed as stated and 23 corrected in scope or mechanism. The correction
rate is concentrated in exactly one confusion — reporting a case-insensitive
*comparison* as destructive *folding* — which is the distinction this document
exists to make.
