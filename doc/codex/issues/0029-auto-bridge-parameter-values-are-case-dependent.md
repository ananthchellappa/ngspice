# Issue: The XSPICE Auto-Bridge Resolves Its Own Parameters by Byte-Exact Lower Case

## Status

Fixed on branch `ver_50`, all four defects, one commit per mechanism. Found
while closing Phase 3 gates 2 and 4, by an independent sweep of
`src/xspice/` for case dependencies that are not identifier comparisons. Four
defects, three of them in `src/xspice/evt/evtcheck_nodes.c` and one in a code
model. None is on any gate list; none is fixed by the gate commits, which
changed only the two identifier comparisons in that file's neighbourhood.

The reason they were not found by the identifier census is that none of them
is a `strcmp` between two identifiers. They are a symbol-table probe keyed by
a constructed literal, a parameter **value** embedded in a filename, and a
model **parameter value** compared against a lower-case literal — three shapes
that a `grep` for `strcmp`/`cieq` over identifier names cannot see.

## Summary

### (a) `vcc` and `family` are looked up under names ngspice spells in lower case

`find_bridge()` (`src/xspice/evt/evtcheck_nodes.c:475-528`) resolves the bridge
supply voltage and the logic family out of the numparam symbol table using
literals it builds itself:

```c
/* src/xspice/evt/evtcheck_nodes.c:482, :505, :525 */
        vcc_parm = "vcc";
...
            snprintf(dot + 1, sizeof buff - (size_t)(dot - buff), "family");
            family = nupa_get_string_param(buff);
...
        family = nupa_get_string_param("family");
```

`nupa_get_param()` and `nupa_get_string_param()` reach `entrynb()`
(`src/frontend/numparam/xpressn.c:413`) and thence `symbol_key()` (`:386`),
which is

```c
    if (inp_case_exact_ids())
        return s;
```

i.e. the key is folded only under `preserve`. Under `fold` the reader already
lower cased the deck, so a `.param VCC=5` arrives as `vcc` and the probe hits.
Under `preserve` the key is folded and the probe hits. Under `distinguish` the
table keeps `VCC` and the probe asks for `vcc`, so it misses.

### (b) the `family` parameter *value* becomes a filename and a subcircuit name

```c
/* src/xspice/evt/evtcheck_nodes.c:539, :589, :594 */
    snprintf(buff, sizeof buff, "bridge_%s_%s_%s.subcir", family, ...);
```

`family` here is a string **value** the deck wrote on a `.model` card. It is
embedded in a filename resolved by `inp_pathresolve()`, in an `.include`, and
in the generated `Xauto_bridge%d ... bridge_%s_%s_%s` subcircuit reference. A
POSIX filesystem is byte exact in every mode.

### (c) two casemode-independent defects in the same function

```c
/* src/xspice/evt/evtcheck_nodes.c:370-372, inside examine_device() */
                 *family = pdp->element->svalue; // May be NULL
                 if (family && !family[0])
                     family = NULL;     // Ignore empty string.
```

`family` is the `const char **` out-parameter and is never NULL there;
`family[0]` is `*family`; and the assignment clobbers the local
pointer-to-pointer instead of the caller's string. The empty-string guard is
therefore dead in both directions. The correct form is
`if (*family && !(*family)[0]) *family = NULL;`.

```c
/* src/xspice/evt/evtcheck_nodes.c:558 */
                if (!strcmp(family, bridge->family)) {
```

`bridge->family` is assigned from `family` at bridge creation (`:669`) and may
be NULL, while the enclosing `if (family)` guarantees only the left operand.
A family-bearing node reaching an existing family-less bridge of the same
`udn_index` and `direction` dereferences NULL.

### (d) `multi_input_pwl` exits on an upper-case model name

```c
/* src/xspice/icm/analog/multi_input_pwl/cfunc.mod:207-218 */
      if (!strcmp(model, "and"))
...
      else {
	  fprintf(stderr, "ERROR(cm_multi_input_pwl): unknown gate model type "
                  "'%s'; expecting 'and|or|nand|nor'.\n", model );
	  exit(-1);
```

`model` is a string-valued instance parameter. `and`/`or`/`nand`/`nor` are
keywords naming a behaviour, and keywords are case insensitive in every mode
by the spec's compatibility contract point 2, so `cieq` is the right
comparison here in all three modes.

## Impact

- (a) `distinguish` only, and it is a **silent wrong number**: with the `.param`
  invisible, `find_bridge()` falls through to `vcc = 3.3` for digital
  (`evtcheck_nodes.c:519`) and to no family, so a deck that set `.param VCC=5`
  gets a 3.3 V bridge and no diagnostic. This is the reason `set_case_mode()`
  still calls `distinguish` experimental after gates 2 and 4 closed.
- (b) `preserve` and `distinguish`, and `preserve` has shipped decks. A deck
  writing `family='74HCT'` finds no `bridge_74HCT_d_out.subcir`, so the family
  bridge silently degrades to the built-in default bridge — a different device
  with different levels, with no message. Under `fold` the same deck works,
  because the reader lower cased the card.
- (c) both are mode independent. The clobber is a dead guard; the missing NULL
  check is a crash on a reachable combination.
- (d) `preserve` and `distinguish`, and it is a hard `exit(-1)` rather than a
  wrong answer: `model=OR` runs under `fold` and kills the process in the two
  other modes.

## Root Cause

The spec's identifier scope is nets, instances, pins, `.model` and `.subckt`
names. Every defect above is a name **outside** that scope — a parameter name
ngspice constructs, a parameter value, a filename, a behaviour keyword — and
each was written when `fold` was the only mode, so "the deck is already lower
case" was a safe assumption at the point of use. `preserve` and `distinguish`
withdrew that assumption without these sites being re-read, because they are
not identifier comparisons and no census that greps for identifier comparisons
finds them.

(a) is the mirror image of the Class C rule in
`doc/claude/decisions/0001-distinguish.md` decision 3: there the query comes
from outside and the table holds deck text, so the query must be tolerant.
Here the query is a lower-case literal ngspice constructed and the table holds
deck text, so the same rule — "distinguish must be exact about names the deck
chose and case insensitive about names ngspice constructs" — says the probe
must be tolerant, which under an exact `symbol_key()` it cannot be.

## Acceptance Criteria

- A deck under `casemode=distinguish` that writes `.param VCC=5` and relies on
  the auto-bridge gets a 5 V bridge, or a diagnostic; not a silent 3.3 V.
- A deck under `casemode=preserve` that writes `family='74HCT'` selects the
  same bridge it selects under `fold`, or says why it cannot.
- `examine_device()`'s empty-`family` guard actually clears the caller's
  string, with a test that a `.model` carrying `family=""` behaves as if the
  parameter were absent.
- `find_bridge()` does not dereference a NULL `bridge->family`.
- `A1 %vd(...) ... model=OR` under `preserve` runs and behaves as `model=or`.
- Decks for each, under the mode that fails: `tests/xspice/case/` for the
  `preserve` items and `tests/xspice/casedist/` for the `distinguish` one.
  Both directories exist.

## Resolution

Fixed, four commits, in the order (d), (c), (b), (a) — cheapest evidence
first, and the one needing a decision last. Recorded here rather than in
decision 3 of `doc/claude/decisions/0001-distinguish.md`, whose table is for
identifier comparisons: none of these four is one.

- **(d)** `fix: compare multi_input_pwl's gate-model keyword without regard to
  case`. The comparison is the C library's `strcasecmp`/`_stricmp`, not
  ngspice's `cieq`, and that is not a style choice: a code model is dlopened
  as a `.cm` and reaches the simulator only through the `coreInfo_t` function
  table of `src/xspice/icm/dlmain.c`. `cieq` is not in
  `src/include/ngspice/dllitf.h` and the binary exports no dynamic symbol for
  it, so a `cfunc.mod` cannot call it.
  `src/xspice/icm/digital/d_cosim/cfunc.mod` already carries the same pair.
  RED: `model="OR"` under `preserve` produced no stdout and exit 255.
  Decks: `tests/xspice/case/multi-input-pwl-model-case{,-lower}.cir`.

- **(c)** `fix: repair the auto-bridge's dead empty-family guard and its NULL
  strcmp`. The two one-line corrections. Both are mode independent and both
  move a number, which the issue did not claim: `family=""` selected the
  default bridge (3.3 V) where an absent `family` selected the `.param`
  family's bridge (1.65 V), and the NULL `strcmp` was a SIGSEGV, not a
  latent guard. Decks are in `tests/xspice/digital/`, which runs in the
  default mode, rather than in a case directory.
  Running the NULL deck with its two nodes in the other order exposed a third
  defect, filed as `doc/codex/issues/0031` and deliberately not fixed here.

- **(b)** `fix: fold the auto-bridge's family value once, where it becomes a
  file name`, corrected by the follow-up commit below. The spelling is chosen
  at the convergence of `family`'s three sources, after
  `evtcheck_nodes.c:530`, rather than at the `examine_device()` capture the
  spec names — the two `.param` sources do not pass through that function.
  The chosen string is now owned by the `struct bridge` and released by
  `free_bridges()`; it used to be a borrowed pointer that was never freed.
  RED: `family="74HCT"` printed 3.3 under `preserve` and `distinguish` against
  2.64 under `fold`; all six cells of that matrix are now 2.64.
  Decks: `tests/xspice/case/auto-bridge-family-file-case{,-lower}.cir` with
  the support file `tests/xspice/case/bridge_74hct_d_out.subcir`.

- **(a)** `fix: let the auto-bridge find a .param whose case it did not
  choose`. The tolerance is at the caller, not the table: `symbol_key()` stays
  exact, and a new `entrynb_constructed()`
  (`src/frontend/numparam/xpressn.c`) retries an exact miss with a scan that
  compares **only the final dot-separated component** without regard to case.
  Everything before the last `.` is a subcircuit instance path the deck wrote
  and is still matched exactly — the same split `vec_wrapped_name_eq()` makes
  for the `V` of `V(1)`. The scan runs only under `distinguish` and only after
  a miss, so the two shipped modes do exactly the work they did before.
  Two case-variant candidates and no exact one is a genuine ambiguity: it is
  reported on stderr and neither is used.
  `auto_bridge_parm_<type>`'s value is **not** given the tolerant probe: that
  name was typed by the user at the control language, so decision 5 of
  `0001-distinguish.md` makes it exact.
  A second half of (a), found by testing rather than by reading: the scoped
  probe is built from the MIF instance name, which keeps the deck's spelling,
  while numparam stores a scoped parameter under a path whose device letter
  has been forced lower case (`spicenum.c:721`) — probe `X1.vcc` against key
  `x1.VCC`. `fold_path_device_letters()` folds that one character per
  component, which is compatibility contract point 2 and not an identity
  loosening. This is the case that matters in practice, since
  `examples/digital/auto_bridge/vcc.cir` is built on a per-subcircuit `vcc`.
  Decks: `tests/xspice/casedist/auto-bridge-vcc-param-case{,-lower}.cir`,
  `auto-bridge-family-param-case{,-lower}.cir`,
  `auto-bridge-vcc-subckt-case{,-lower}.cir` and
  `auto-bridge-vcc-param-ambiguous.cir`.

- **follow-up**, `fix: choose the auto-bridge's family spelling instead of
  imposing one, and fold one path letter`. An adversarial review of the four
  commits above found five defects, two of them regressions the commits
  themselves introduced, and this commit fixes all five. In order of weight:

  1. (b)'s unconditional fold is wrong in **every** mode. It breaks the deck
     whose bridge file or `auto_bridge_*` variable is named the way the deck
     spells the family — the only naming that worked under `preserve` — and it
     is not a `fold`-mode no-op either, because a quoted `.model` value
     survives the reader's fold whenever `is_xspice_model()` matches the card,
     which is a substring test over the whole line and fires on a trailing
     comment (`doc/codex/issues/0005`). Measured: a `fold`-mode deck that
     printed 2.64 at `3dbd6ba95` printed 3.3 at `4caf2ff35`. Now the deck's
     own spelling is tried first and the folded one second, which is a
     superset of both behaviours. Guard:
     `tests/xspice/case/auto-bridge-family-file-upper.cir`.
  2. (a)'s `fold_path_device_letters()` folded the leading letter of every
     dot-separated path component, but `spicenum.c:721` folds only the first
     character of the whole path, so a nested instance is stored as
     `x1.XIn.vcc`. The probe became `x1.xIn.vcc` and a nested-subcircuit
     lookup that had always worked under `distinguish` broke: 6.0 became 3.3.
     Guard: `tests/xspice/casedist/auto-bridge-vcc-nested-subckt-case.cir`.
  3. `Evtcheck_nodes()` has three exits and only two called `free_bridges()`;
     the `expand_deck()` failure path leaked the whole bridge list, and (b)
     added the owned family string to what it leaks. Pre-existing, made
     slightly worse, now fixed.
  4. The tolerant scan was type-blind, so a `.subckt` whose name differs from
     the probe only in case counted as a second candidate and vetoed the one
     real `.param` through the ambiguity branch. It now skips entries that are
     neither `NUPA_REAL` nor `NUPA_STRING`. Guard:
     `tests/xspice/casedist/auto-bridge-vcc-param-subckt-name.cir`.
  5. The two `snprintf()`s in the scoped-probe loop passed a size one byte
     larger than the space at `dot + 1`. Pre-existing, one byte of stack, and
     inside the loop this work rewrote.

With (a) closed, `set_case_mode()` no longer names the auto-bridge. The word
"experimental" stayed for `doc/codex/issues/0027` (`unlet` still removes a
vector whose name differs only in case) and `doc/codex/issues/0030` (an XSPICE
event node that misses by case is still not diagnosed). `0030` has since
closed too, so `0027` is the only issue that clause still names.
