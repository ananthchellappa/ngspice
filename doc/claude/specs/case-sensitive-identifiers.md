# Spec: Case-Sensitive Net, Instance and Pin Names

## Status

Phases 0, 1 and 2 are implemented and `preserve` has shipped; Phase 3
(`distinguish`) is experimental, with gates 1, 2 and 4 open. The design text
below is kept as written except where a line is marked **Done**. Companion
documents:

- `doc/claude/code_analysis/case-insensitivity-origins.md` — where the current
  behavior comes from.
- `doc/claude/suggestions/case-sensitive-identifiers-plan.md` — the RED-first
  delivery plan.
- `doc/claude/decisions/0001-distinguish.md` — the `distinguish` decision
  record, which closes Open decisions 1 and 2 below.

## Problem

Ngspice destroys the case of netlist text in the file reader
(`src/frontend/inpcom.c:1838-1840`), before any token is classified. Every
identifier table below that point is case-*sensitive* (`strcmp` over a
non-folding hash), so the folding is the only thing making the simulator appear
case-insensitive.

Three consequences:

1. `R1`, `r1` and `R_Big` are indistinguishable. Two instances differing only in
   case is a hard duplicate error.
2. Names never round-trip. What a user typed cannot be recovered from a
   rawfile, a plot label, a `listing`, or an error message.
3. Tools that generate ngspice decks from case-sensitive sources — Verilog
   co-simulation via `d_cosim`, Verilog-A/OSDI, VHDL-derived flows, schematic
   exporters carrying designer-chosen net names — must flatten their namespace
   before handing it over, and cannot map results back.

## Goal

Let a deck opt into identifiers that keep their case, and — as a separate,
later, explicitly gated step — into identifiers that are *distinguished* by
case. Default behavior must remain byte-identical.

## Modes

A single tri-state mode, named `casemode`, with three values:

| Value | Identity | Spelling | Default |
| --- | --- | --- | --- |
| `fold` | `R1` == `r1` | lowercased | yes |
| `preserve` | `R1` == `r1` | as typed, first occurrence wins | no |
| `distinguish` | `R1` != `r1` | as typed | no |

`preserve` and `distinguish` are two independent axes deliberately expressed as
one ordered mode: `preserve` changes what ngspice *shows*, `distinguish` changes
what ngspice *simulates*. Shipping `preserve` first puts mixed-case text through
every downstream matcher in production while topology is still protected by
case-insensitive comparison. That is the only way to discover the long tail
without risking wrong numbers.

### Canonical spelling under `preserve`

First occurrence in file order wins. This falls out of the interning tables for
free: on a hit, `INPtermInsert` and `INPinsert`
(`src/spicelib/parser/inpsymt.c:41-46`, `:158-163`) free the caller's token and
substitute the stored one. A deck writing `R1 IN OUT 1k` and then `.print
v(Out)` reports `V(OUT)`. This must be documented; it is not an implementation
detail the user can ignore.

## Switch mechanism

A control-language variable read once per deck read, not a `.option` and not a
new command-line flag.

`.option` is structurally impossible: `inp_getopts()` (`src/frontend/options.c`)
runs over an already-read, already-folded deck.

The read point is `src/frontend/inpcom.c:1057`, where `inp_readall()` already
calls `set_compat_mode()`; `inp_read()` — which contains the fold — is called
six lines later at `:1063`. `set_compat_mode()` itself resolves `ngbehavior`
through `cp_getvar` at `src/frontend/inpcompat.c:76`, and `inp_read()` calls
`cp_getvar` mid-loop at `:1173` and `:1939`. A cp variable is demonstrably
readable before the first character is folded.

Because the read is per-`inp_readall`, the mode re-establishes on every
`.include`, every `ngSpice_Circ()`, and survives `ngSpice_Reset()` with no
teardown code.

Three ways to set it, with no new argument parsing:

```sh
ngspice -D casemode=preserve deck.cir     # src/main.c already maps -D to cp_vset
```
```
set casemode = preserve                    # in .spiceinit, or spinit for a site default
```
```c
ngSpice_Command("set casemode=preserve");  /* before ngSpice_Circ / source */
```

Ordering rule, which must be documented: the mode is *read* when the netlist is
read, so the last writer before that point wins. `.spiceinit` is sourced after
the `-D` getopt loop, so a `set casemode` in `.spiceinit` overrides `-D`.

`casemode` stays orthogonal to `ngbehavior`. Folding a case policy into a vendor
dialect letter would make `set ngbehavior=hs` silently imply a case policy, and
`set_compat_mode()` matches dialect letters with `strstr`, which makes new
letters collision-prone.

**Known cost of that orthogonality**, stated because it removes most of the
obvious user base: vendor decks assume the fold. If `casemode` defaults to
`fold` whenever `ngbehavior` names a vendor dialect, the feature is unavailable
in exactly the flows `examples/SkywaterOpenSourcePDK/readme.txt:20` and
`examples/IHPOpenSourcePDK/IHP-PDK-howto.txt:14` instruct users to configure
(`set ngbehavior=hsa`). This spec accepts that: a PDK-consuming deck cannot use
case-sensitive names at the top level until the PDK is known to be safe. There is
no proposal here for mixed-mode decks, and `.include`-ing a fold-assuming library
from a `distinguish` top level is out of scope — see Open Decisions.

## Scope

### In scope

- Net (node) names, including hierarchical names built by subcircuit flattening
  (`src/frontend/subckt.c:1123-1125`) and model renaming (`:1761`).
- Instance names, including `GENname` interning (`src/spicelib/devices/cktcrte.c:58`)
  and `ckt->DEVnameHash`.
- Subcircuit formal and actual pin names, and `.subckt` names.
- `.model` names.
- The output path, so that a preserved name actually reaches the user. This is
  larger than it looks — see "The output path is part of the deliverable".
- Making language keywords explicitly case-insensitive wherever they are
  currently matched with `strcmp` against a lowercase literal.

### Out of scope

- **Numeric semantics.** Scale suffixes stay case-insensitive and `1M` stays
  milli, not mega (`src/spicelib/parser/inpeval.c:143-186`). No case mode may
  ever change how a number parses.
- **Device letters.** `R1` and `r1` dispatch to the same device type in every
  mode. Already non-destructive (`src/spicelib/parser/inppas2.c:92-94`).
- **Ground.** `gnd`, `GND` and `Gnd` remain aliases for node `0` in every mode.
  Ground is a reserved word, not an identifier. The rewrite at
  `src/frontend/inpcom.c:2400-2407` becomes a case-insensitive,
  delimiter-guarded scan.
- **XSPICE code-model port names.** Binding is positional by index
  (`src/xspice/mif/mif_inp2.c:632`); port-name case has no effect and needs no
  work.
- **CIDER.** `--enable-cider` is off by default and CIDER reaches identifiers
  only through *filenames* — `numdset.c:164` builds `ic.<instname>`, mirrored in
  `numddump.c:70`, `nbjtdump.c`, `nbt2dump.c`, `nud2dump.c`, `nummdump.c`. Under
  `preserve`, instance `D1` looks for `ic.D1` where every existing setup has
  `ic.d1`. Declared broken-under-`preserve`, not fixed.
- **`tclspice`.** A third name-crossing API surface, unexamined.
- **The user manual.** Not in this repository; see Documentation.

## The output path is part of the deliverable

**Done, in commit `47c52c7dd`.** The reasoning below is kept as written; the
line numbers and the tense are corrected.

`preserve` does not deliver preservation by fixing the reader alone.
`vec_basename()` used to call `strtolower(buf)` at
`src/frontend/vectors.c:1129`; that line is now the comment recording its
removal. It sits on the output path of:

| Consumer | Site |
| --- | --- |
| `print` | `src/frontend/postcoms.c:207` |
| `write` | `src/frontend/postcoms.c:661` — *replaces* `v_name` before writing the rawfile |
| `write_sparam` | `src/frontend/postcoms.c:826` |
| `spec` | `src/frontend/spec.c:213` |
| `fft`, `psd` | `src/frontend/com_fft.c:165`, `:398` |

Without this in scope, `print v(OutB)` would still print `v(outb)` and `write
foo.raw` would still write `outb`, while batch `-r`
(`src/frontend/outitf.c:532`) and `listing`/`show` preserve. Case would land on
two surfaces and be discarded on five. With it in scope all seven agree, and
`tests/regression/case/name-roundtrip.cir` asserts on `print` as well as on
`show`.

Removing that `strtolower` was **not a no-op in `fold` mode**, and the spec must
say so. `CKTmkVolt(ckt, &tmp, here->BJTname, "collCX")`
(`src/spicelib/devices/bjt/bjtsetup.c:433`) generates node names containing
uppercase regardless of any deck fold, so before the change `ngspice -r` emitted
`q1#collCX` while interactive `write` emitted `q1#collcx`. Correcting
`vec_basename` changed existing `write`/`print`/`fft` output and churned
committed `.out` files. That is a deliberate, documented correction, and it is
the one place where "the default is provably byte-identical" does not hold.

## Compatibility contract

1. With `casemode` unset, every code path is byte-identical to today, with the
   single exception of the `vec_basename` correction above.
2. Language keywords are case-insensitive in **all** modes. `.TRAN`, `PULSE`,
   `PARAMS:`, `UIC`, `%VD`, `TRUE`/`FALSE`/`NULL`, `.model` type names, B-source
   function names. Many already are (`ciprefix` for card dispatch,
   `strcasecmp` at `src/spicelib/parser/inptyplk.c:37` and
   `src/frontend/inpcom.c:542`); the rest is the Phase 1 sweep.
3. The frontend vector table must not silently alias. **Done**, Phase 3 gate 3.
   `src/frontend/vectors.c:61` sets `nghash_unique(..., FALSE)` and `:71`/`:184`
   fold key and query, so under `distinguish` two case-variant nets would
   collide and `print` would return whichever hashed first — a silent wrong
   answer. All three lines stay; the duplicate chain they produce is now
   filtered on the spelling the caller typed, which is a tautology under `fold`
   and `preserve` and exact under `distinguish`.
   `tests/regression/casedist/node-case-split.cir` is the assertion. See
   `doc/claude/decisions/0001-distinguish.md` Class C for why this site does not
   use the same predicate as the identity sites.
4. The shared library contract must be stated. `ngGet_Vec_Info`
   (`src/sharedspice.c:1194` → `findvec`) is case-insensitive; its event twin
   `ngGet_Evt_NodeInfo` (`src/sharedspice.c:1441` → `EVTshareddata`, whose
   `strcmp` is in `get_index()` at `src/xspice/evt/evtshared.c:253`) is exact.
   **The two halves of the public API already disagree.** Any mode change makes
   the analog half ambiguous and the event half stricter, so this needed a
   header contract note and not just an implementation change. **Done**: the
   note is `src/include/ngspice/sharedspice.h:65-84`.
5. Rawfiles are not self-describing. Three writers use three naming policies
   (batch `-r` verbatim, `write` lowercased, `wrdata`), and the reader binds
   `scale=` with `cieq` (`src/frontend/rawfile.c:611`). A consumer cannot tell
   whether `Out` and `out` are two nets or an artifact of which writer produced
   the file. The format has no case-policy field; this spec does not add one.

## Silent-failure sites that gate `distinguish`

These produce wrong numbers with no diagnostic, and each must be resolved before
`distinguish` can ship. They are listed here because they define the gate, not
because the fix is specified.

| Site | Failure |
| --- | --- |
| `src/spicelib/parser/inpptree.c:1249`, call at `:1256` | `mkvnode` uses `INPtermInsert`, which **creates on miss**, so a `V()` reference that misses by case manufactures a node no card defines. The manufactured node has **two shapes**, not one. *(a)* With a DC path to it — an `rshunt`, or any other card that reaches it — the deck runs to completion and prints a wrong number: `B1 out 0 V={V(in)*2}` against net `In` gives `v(out) = 0.000000e+00` where `preserve` gives `3.000000e+00`, with no diagnostic. *(b)* Without one it is genuinely isolated, because an expression reference contributes no conductance to the referenced node's own row; the matrix is singular and the run aborts with `Warning: singular matrix:  check node in`, naming a node **the deck never wrote**. Both shapes are the same defect and take the same fix. Note that create-on-miss cannot simply be refused: a B source is parsed before the cards below it, so `V(mid)` on the first card of a deck is a legal forward reference that only works because the node is created. **Done**, Phase 3 gate 1: the call is now `INPtermInsertRef`, which marks the created node unclaimed, and `INPtermCaseCheck()` reports the near-miss at the end of the parse. The node is still created and the numbers are unchanged — `doc/claude/decisions/0002-deferred-node-resolution-check.md` — so what closed the gate is that the failure became audible, not that it became correct. |
| `src/xspice/evt/evtcheck_nodes.c:720` | **Done**, Phase 3 gate 2, and the row was misfiled: this is a **`preserve`** failure, not a `distinguish` one. Auto-bridge insertion was `strcmp(event_node->name, analog_node->name)`, so under `preserve` — where two spellings are one identifier — digital `A` and analog `a` were not recognised as the same mixed-type node, no bridge was inserted, the analog net was driven by nothing and no diagnostic fired. Under `distinguish` the `strcmp` was already the correct answer. The comparator is now `ng_ideq()`; `tests/xspice/case/auto-bridge-node-case.cir` moved `v(a)` from `0.000000e+00` to `3.300000e+00`. What is still missing is `distinguish`'s near-miss warning, `doc/codex/issues/0030`. |
| `src/xspice/evt/evttermi.c:304` | **Done**, Phase 3 gate 4, and misfiled in the same way. `EVTnode_insert()`'s find-or-create was `strcmp(node_name, node->name)`, so under `preserve` two spellings of one event node became two event nodes and silently split a net — the interning site, and the reason gate 4 had to land before gate 2. Now `ng_ideq()`; `tests/xspice/case/event-node-case.cir` moved `v(aout)` from `0.000000e+00` to `5.000000e+00`. Same open diagnostic, `doc/codex/issues/0030`. |
| `src/frontend/measure.c:236` | **Done.** `strtolower(an_name)` at `:237` is now gated on `inp_case_folding()` at `:236` and its consumers at `:351` and `:446` compare with `cieq`. A case-mixed `.MEAS TRAN` used to be silently skipped with no output line. |
| `src/frontend/subckt.c:659`, `:675`, `:692`, `:708` | **Done.** MOS bin selection now uses `cistrstr(curr_line, " wmin=")` and the `wmax`/`lmin`/`lmax` siblings. A card written `WMIN=` used to select the wrong bin, silently. |
| `src/frontend/outitf.c:391` | **Done**, commit `f38570c43`. The internal-node classifier is now `tolower_c(tmpname[1]) == 'd'`. A diode named `D1` used to lose its terminal current under `.save alli`; `doc/codex/issues/0016` class (a). |
| `src/spicelib/parser/inp2dot.c:366`, `:387`, `:489`, `:512` | **Done.** `.SENS`/`.TF` now match `v`/`i` with `cieq`, as `.NOISE` already did. |

## Fold sites outside the reader

Three destructive folds exist outside `inp_read()` and must be handled:

- `src/frontend/device.c:1421-1422` — `com_alter_common` does
  `strtolower(param); strtolower(dev);`, on the shared-library command path.
  The pair is now gated on `inp_case_folding()` at `:1420`. What that gate
  exposes one call level deeper is `doc/codex/issues/0026`.
- `src/frontend/measure.c:236` — as above.
- `src/frontend/inp_casefix()` (`src/frontend/inpcom.c:3653`) — three unrelated
  jobs in one function (quote handling at `:3697-3703`, non-printable-to-`_` at
  `:3704-3705`, folding at `:3706-3707`). Only the fold may be gated; gating the
  whole function would disable input sanitisation.

## OSDI

`src/osdi/osdiinit.c:74` stores a `strtolower`'d copy of each Verilog-A
parameter name as the `IFparm` keyword. Verilog-A is case-sensitive, so two
distinct parameters `Vth` and `VTH` silently collapse and the second shadows the
first in `find_model_parameter`'s linear scan
(`src/spicelib/parser/inpgmod.c:52`). This is a latent correctness bug
independent of any deck.

**Decision required, single answer, not three**: keep the fold and make the
matchers `strcasecmp` (safe, keeps the shadowing bug), or remove the fold (fixes
shadowing, silently changes behavior for every existing OSDI deck). This spec
recommends the former for `fold`/`preserve` and the latter only as part of
`distinguish`, but the decision is open. Note that `tests/` has **no OSDI
regression directory**, so neither resolution is testable in-tree, and parameter
case originates in a third-party compiled `.osdi` artifact the deck author does
not control.

## Platform and packaging obligations

- **MSVC.** `visualc/vngspice.vcxproj`, `visualc/vngspice-fftw.vcxproj` and
  `visualc/sharedspice.vcxproj` each list roughly 1480 explicit `<ClCompile>`
  entries. Any new `.c` file must be added to all three by hand; there is no CI
  in the tree to catch the omission. This argues for putting new helpers in an
  existing translation unit unless a new file is clearly warranted.
- **Filenames.** In `preserve`/`distinguish`, every place a user identifier
  becomes a filename behaves differently on NTFS than on ext4: CIDER's
  `ic.<instname>`, the XSPICE auto-bridge include
  (`src/xspice/evt/evtcheck_nodes.c:538`), `.include` paths. Rule: fold any
  identifier at the point it is captured for filename construction — for the
  auto-bridge, fold `family` once at `evtcheck_nodes.c:367` rather than gating
  the five downstream `snprintf` sites.

  **Done for the auto-bridge**, `doc/codex/issues/0029` defect (b), with one
  deviation from the line named above: the fold is at the *convergence* point,
  after `evtcheck_nodes.c:530`, not at the `examine_device()` capture. `family`
  has three sources — a model card's `family` parameter, a scoped `.param` and
  a global `.param` — and only the first passes through `examine_device()`, so
  a fold at `:367` would have left the two `.param` sources unfolded. The fold
  is unconditional: under `fold` the reader has already lower cased the value,
  so all three modes now build the same file name. The folded string is owned
  by the `struct bridge` and released by `free_bridges()`.

  The same rule reaches one site that is a *lookup* rather than a filename and
  is listed here because it is the same "fold what ngspice constructs" move:
  the auto-bridge's scoped parameter probe is built from the MIF instance
  name, which keeps the deck's spelling, while numparam stores a scoped
  parameter under a path whose device letter has been forced lower case at
  `src/frontend/numparam/spicenum.c:721`. `fold_path_device_letters()` folds
  that one character per path component before the probe. Device letters are
  keywords, so this is compatibility contract point 2 and not an identity
  loosening.

  Still open: CIDER's `ic.<instname>` and `.include` paths, neither of which
  this work touched.

## Documentation

- `man/man1/ngspice.1` documents options at lines 30-88 and **does not document
  `-D`/`--define` at all**. Since `-D` is the sole command-line entry point for
  this feature, adding it is a prerequisite, not a nicety.
- `NEWS` carries the per-release user-visible feature list (`NEWS:1-22`). For a
  feature whose only interface is a variable, the `NEWS` bullet is the discovery
  mechanism.
- `src/spinit.in` should ship a commented `*set casemode = preserve` line as the
  site-default surface.
- The user manual is **not in this repository** — `doc/` contains only agent
  artifacts, and there are no `.tex`/`.texi` sources. The manual chapter has to
  be filed separately upstream, which means the feature ships undocumented in
  the manual for at least one release cycle. That is a stated limitation, not a
  schedulable task.

## Open decisions

Decisions 1 and 2 are **closed** by
`doc/claude/decisions/0001-distinguish.md`, which also answers the three
questions this section never asked: whether `distinguish` is a `casemode` value
or a flag, what the diagnostic is for a case near-miss, and which
`inp_case_folding()` call sites were asking about identity rather than about the
fold. It is not repeated here.

1. **OSDI**: fold-and-`strcasecmp` versus remove-the-fold. Untestable in-tree
   either way. **Closed**: keep the fold in all three modes,
   `doc/claude/decisions/0001-distinguish.md` decision 4, which extends
   `doc/claude/checklists/phase2-prerequisites.md` section 4 to `distinguish`.
2. **Mixed-mode decks**: a `distinguish` top level `.include`-ing a
   fold-assuming PDK. The failure is silent — an unresolved node is not an
   error, the parser mints a new floating node. **Closed as far as a rule
   goes**, `doc/claude/decisions/0001-distinguish.md` decisions 2 and 5: the
   miss is diagnosable wherever a *resolution* fails while a case-variant
   exists, and that is what Phase 3 gate 1 has to implement. Until it does, a
   `distinguish` deck and everything it includes must be case-consistent.
3. **`vec_basename` churn**: accept the `#collCX` output change as a correction,
   or preserve bug-compatibility in `fold` mode with a mode check.
4. **Vendor-dialect gating**: default to `fold` when `ngbehavior` names a vendor
   dialect (safe, excludes the PDK population) or allow the combination and
   document the risk.
5. **Deck-local opt-in**: not possible as specified. The mode must be known
   before the first line is folded, and the first line is read inside
   `inp_read()`'s loop. A title-line pre-scan would work for real files but not
   for `ngspice -p` on stdin nor the shared-library `circarray` path.

## Acceptance criteria

`preserve` is complete when:

- A deck under `casemode=preserve` reports node and instance names with the
  spelling first used in the deck, on `print`, `write`, `show`, `listing`,
  `display`, batch `-r` rawfiles and `fft`/`psd`/`spec` derived vectors.
- Identity is unchanged: every existing test produces identical results under
  `fold` and under `preserve` when the deck is all-lowercase.
- A mechanically uppercased copy of every deck in `tests/` produces numerically
  identical results to the original under `preserve`.
- Uppercase keywords (`.TRAN`, `UIC`, `PULSE`, `PARAMS:`, `%VD`) work in all
  three modes.
- `make check` is green with `casemode` unset.

`distinguish` is complete when, in addition:

- Two nets, two instances, and two subcircuit formal pins differing only by case
  are distinct, independently addressable from the control language, and
  independently present in the rawfile. **Partly done**: nets and instances are,
  asserted by `tests/regression/casedist/node-case-split.cir` and
  `instance-case-split.cir`; `.model` names and `.param` names are too, asserted
  by `model-case-split.cir` and `param-case-split.cir`. Subcircuit formal pins
  and the rawfile are untested.
- Every site in "Silent-failure sites that gate `distinguish`" either diagnoses
  or behaves correctly. **Done**: gates 2 and 4 closed the last two,
  `src/xspice/evt/evtcheck_nodes.c:720` and `src/xspice/evt/evttermi.c:304`,
  both with `ng_ideq()`, and the four event-node *lookups* around them with
  `Evt_Node_Name_Eq()`. The harness is `tests/xspice/case/` under
  `-D casemode=preserve` and the new `tests/xspice/casedist/` under
  `-D casemode=distinguish`; both carry their own `spinit` because
  `tests/bin/spinit` loads no code models. `set_case_mode()` still calls the
  mode experimental, but no longer names these sites: what it names now is
  `doc/codex/issues/0029` and `doc/codex/issues/0027`, neither of which is a
  gate.
- The frontend vector table no longer aliases case-variant names. **Done**,
  Phase 3 gate 3.
- A B source `V()` reference that misses by case is diagnosed rather than
  silently manufacturing a node. **Done**, Phase 3 gate 1,
  `doc/claude/decisions/0002-deferred-node-resolution-check.md`. The node is
  still created, because create-on-miss is what makes forward references work;
  what changed is that a node no card defines, whose name differs only in case
  from one that is defined, is reported on `stderr` at the end of the parse,
  asserted by `tests/regression/casedist/bsource-node-case.cir` for the number
  and verified by hand for the text. A reference that misses with **no** case
  variant present — a plain typo — is still silent in every mode, as are the
  `.NOISE`/`.SENS`/`.TF`/`.PSS` node references, which have the same
  create-on-miss shape; `doc/codex/issues/0028`.
