# Plan: Deliver Case-Sensitive Identifiers, RED-First

Implements `doc/claude/specs/case-sensitive-identifiers.md`. Assumes
`doc/claude/code_analysis/case-insensitivity-origins.md` for the map.

Delivery order is chosen so that the work with independent value ships first and
the work that can produce silently wrong numbers ships last, or not at all.

## Ground rules

1. RED-first, per `AGENTS.md`. Every behavioral change begins with a test that
   fails for the expected reason, and the failure is recorded before production
   code moves.
2. **Evidence must be numeric.** `tests/bin/check.sh:21` filters both actual and
   expected output through an `egrep -v` that removes `Error`, `Warning`,
   `Note`, `Index`, `time`, `trans`, `urrent`, `Total`, `memory`, `Operating`
   and more. A test whose only evidence is a diagnostic passes whether or not
   the bug exists. Every RED deck below is built so that both interpretations
   are electrically solvable and the evidence is a node voltage.
3. No phase may weaken an assertion or regenerate a `.out` to hide a failure.
4. Each phase is a separate series of commits. Phases 0 and 1 are offered
   upstream immediately and independently of the feature.

## Test infrastructure, built before Phase 0

The harness cannot currently enable a mode. `check.sh` runs
`$SPICE --batch $testdir/$testname.cir` with `SPICE=$1`, unquoted, so the flag
rides on the binary argument. The precedent is `tests/xspice/digital/Makefile.am:9`,
which already passes `"$(top_builddir)/src/ngspice -r foobaz"`.

Create `tests/regression/case/` with:

```make
TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) SPICE_SCRIPTS=. $(SHELL) \
	$(top_srcdir)/tests/bin/check.sh \
	"$(top_builddir)/src/ngspice -D casemode=preserve"
EXTRA_DIST = $(TESTS) $(TESTS:.cir=.out)
```

Register the directory in `tests/regression/Makefile.am` `SUBDIRS` **and** in
`configure.ac` `AC_CONFIG_FILES`, then run `./autogen.sh`. A `.cir` absent from
`TESTS`, or a directory absent from `SUBDIRS`, is silently never executed — and
a RED test that never ran looks exactly like a passing one.

Every deck in this plan starts with `.OPTIONS noacct`. The rusage block is
**not** filtered (`Maximum ngspice program size`, `Shared ngspice pages`,
`Stack = 0 bytes.`) and is machine-dependent.

All name-bearing assertions use a single-point `op` inside `.control`. In a
sweep, the table header carrying the vector names is destroyed by the `Index`
and `time` filter entries.

## Phase 0 — Standalone defect fixes

Five verified defects, each independently wrong today, each shippable alone.
Three of them change the default fold surface, so they must land before any
gating work or the gating cannot be reasoned about.

**Order matters.** Fix the `keepquotes` polarity first; the later gating work is
untestable until `inp_casefix` behaves as documented.

| # | Site | Defect |
| --- | --- | --- |
| 0.1 | `src/frontend/inpcom.c:3545-3557` | `keepquotes` polarity inverted; also computed with `*string == 'x'`, lowercase-only |
| 0.2 | `src/frontend/inpcom.c:1857` | `$`-token fold on `echo` lines inside `.control` breaks `setcs`+`echo` |
| 0.3 | `src/frontend/inp.c:713` | `*#`-prefixed commands bypass the whitelist because `ciprefix` sees the prefix |
| 0.4 | `src/frontend/vectors.c:100` | `tolower(word[0] != 'a')` folds a boolean; `all`/`allv` wildcards are case-sensitive |
| 0.5 | `src/frontend/control.c` flow keywords | command names use `strcasecmp` at `:205` but `if`/`while`/`foreach`/`end` use `eq`, so `PRINT` works and `IF` does not |

Hazard for 0.2: `src/xspice/verilog/vlnggen` deliberately uses `setcs` at lines
34, 36, 44, 62, 134, 139, 143, 147, 170 *because* plain `set` is folded, and
plain `set` at lines 8, 24, 71, 84, 85, 133, 169 where folding is harmless.
Changing the fold changes that script, and **there is no test for `vlnggen`
anywhere in `tests/`**. Add one — a `*ng_script` deck asserting a `setcs` value
survives and a `set` value folds — before touching `:1857`.

RED for each: a targeted `.cir`/`.cmd` under `tests/regression/misc/` (or
`tests/regression/pipe/` for 0.5, which needs interactive dispatch) whose
evidence is an echoed uppercase sentinel or a voltage. Avoid any marker
containing lowercase `est` — the filter's `est` entry eats the word "test".

## Phase 1 — Make language keywords explicitly case-insensitive

The largest genuinely valuable piece, and the only phase that is unambiguously
good independent of the feature. Convert every site that matches a *language
literal* — keyword, port qualifier, parameter keyword, `gnd`, `null`/`true`/
`false`, model type — from `strcmp`/`strstr`/`eq` to a case-insensitive
comparison.

### 1.0 Census first

Three independent estimates of the surface disagree by an order of magnitude:
642, ~304, ~120 sites. **The phase is not costed until a mechanical census
exists.** First task: a scripted enumeration of every `strcmp`/`strncmp`/
`strstr`/`eq(` whose other operand is a lowercase string literal, under
`src/frontend`, `src/spicelib/parser`, `src/xspice`, with each hit classified as
keyword (convert), identifier (leave), or filename (leave). Commit the census as
a checklist; it is the phase's unit of progress.

### 1.1 Known members

- `src/spicelib/parser/inp2dot.c:442,695,793` — `strcmp(word,"uic")`; `.TRAN 1n 10n UIC` silently drops UIC and simulates a different circuit.
- `src/spicelib/parser/inp2dot.c:366,387,489,512` — `.SENS`/`.TF` accept only lowercase `v`/`i`. **Done**, now `cieq`.
- `src/xspice/mif/mif_inp2.c:757` — `%`-qualifier table.
- `src/xspice/mif/mifgetmod.c:206` — code-model parameter keywords.
- `src/xspice/mif/mifutil.c:198` — `null`, `t`/`true`, `f`/`false`.
- `src/xspice/evt/evttermi.c:270` — UDN type names.
- `src/spicelib/parser/inpgmod.c:52`, `inpdpar.c:30`, `inpdomod.c:46` — model and instance parameter keywords.
- `src/frontend/numparam/spicenum.c:241,245,254,256,258` — the numparam line
  categorizer uses case-sensitive `prefix()` for `.param`, `.subckt` and
  friends. This contradicts the common assumption that numparam needs no work.
  **Stale as written**: re-checked at `a1161ccdc`, every one of those five is
  already `ciprefix()`, and the `params:` split beside them is already
  `cistrstr()`. Nothing to do there.
- `src/frontend/numparam/xpressn.c:640` — `keyword()` matched an identifier
  against the built-in function list `fmathS` with a raw byte compare, so
  `{SQRT(x)}` was not a function under `preserve`. **Done**,
  `doc/codex/issues/0022`; the deck side now folds with `tolower_c()`,
  ungated, and the list is untouched. The census missed it because it compares
  against a `static const char *` list rather than an individual literal.
- `src/frontend/inpcom.c:2400-2407` — `gnd` rewrite, converted to a delimiter-guarded case-insensitive scan.
- `src/frontend/subckt.c:659,675,692,708` — MOS bin selection `strstr(" wmin=")`. **Done**, now `cistrstr`.
- `src/frontend/inpcom.c:5986`, `:6039` — `search_identifier()` and
  `search_plain_identifier()`, the two whole-token searches the Phase 1 census
  left `unclear` because their `strstr` hides behind a helper. **Done**,
  `doc/codex/issues/0013`; they now fold the literal unless
  `inp_case_folding()`. `ya_search_identifier()` (`:6012`) is the third helper
  and is deliberately *not* folded: its single caller passes a user parameter
  name, not a keyword.
- `src/frontend/spiceif.c:1115` — model parameters use `eq` where instance parameters at `:1101` use `cieq`.

### 1.2 The no-op argument, and its limit

In `fold` mode the input side is already lowercase and the literal side is
lowercase, so case-insensitive and case-sensitive are the same predicate. That
makes the sweep reviewable without running anything — **except** on lines that
are not folded today: `.lib`/`.inc` are exempt *unconditionally* at
`inpcom.c:1826`, and the 13-prefix control whitelist, the print-redirection tail
(`:1806`) and the plot title spans reach downstream matchers case-preserved. The
sweep can therefore find genuinely new matches in `fold` mode on those lines.
State this in the commit message rather than claiming a pure no-op.

### 1.3 RED for this phase

`tests/regression/misc/keyword-case.cir`, run under the **default** environment
with all identifiers lowercase and only keywords varying in case:

```spice
* uppercase keyword guard
.OPTIONS noacct
V1 in 0 DC 1
R1 in out 1k
C1 out 0 1n
.TRAN 1n 10n UIC
.control
op
print v(out)
.endc
.end
```

RED because `UIC` is dropped today. Evidence is the voltage difference between a
UIC and a non-UIC start, not a diagnostic.

Submit per file. A single ~300-site patch will not be reviewed.

## Phase 2 — Flag plumbing and `preserve`

### 2.1 Switch

Read the mode at `src/frontend/inpcom.c:1057`, immediately after
`set_compat_mode()`. Store in a file-scope tri-state. Do not add a new source
file unless unavoidable: each new `.c` must be hand-added to three
`visualc/*.vcxproj` files of ~1480 entries with no CI to catch the omission.

### 2.2 Guard shape

Wrap the **whole** fold chain (`inpcom.c:1713`-`:1852`) in one conditional whose
else-branch byte-copies the existing terminal else at `:1848-1851`, so `s` still
lands on end-of-line for the continuation logic:

```c
if (ng_case_mode == NG_CASE_FOLD) {
    /* ... existing chain, byte-identical ... */
} else {
    for (s = buffer; *s && (*s != '\n'); s++)
        ;
}
```

Three lines at the most dangerous site in the tree. Do not guard each arm
individually and do not edit the `:1826` predicate; both leave the plot, print,
CIDER and XSPICE arms partially live in non-fold modes for no benefit.

Touch `inpcom.c` control flow exactly once. That file churns constantly upstream
and every structural edit there is a recurring rebase cost.

### 2.3 Identity discipline

Fold the key, never swap the comparator. `src/misc/hash.c:549` does
`curTable->key = copy(user_key)` **only** when
`hash_func == NGHASH_DEF_HASH(NGHASH_FUNC_STR)`, and the frees at `:108`,
`:182`, `:393` are gated the same way. Installing a custom hash silently flips
every such table from owning to borrowing its keys — given commits `c5cd68015`
and `5ad395d5e`, not a trade worth making.

Under `preserve`, identity is unchanged, so the interning tables fold the lookup
key while `t_ent` keeps the first-seen spelling
(`src/spicelib/parser/inpsymt.c:41-46`, `:158-163`).

### 2.4 The other fold sites

- `inp_casefix` — split so only the fold is gated; quote handling at `:3544-3553`
  and non-printable substitution at `:3555` keep running unconditionally. In the
  carried patch, split only where the fold is gated and leave the surrounding
  lines byte-identical.
- `src/frontend/options.c:229` — `.option` extraction folds the line; gating it
  changes `.option` parsing and must be reasoned about explicitly, not
  incidentally.
- `src/frontend/device.c:1417-1418` — `com_alter_common`, on the shared-library
  path.
- `src/frontend/measure.c:236` — `.measure` analysis name.

### 2.5 The output path — **done, commit `47c52c7dd`**

Not optional. The `strtolower` was removed from `vec_basename`
(`src/frontend/vectors.c:1138`, now the comment recording the removal) so
`print` (`postcoms.c:212`), `write` (`postcoms.c:666`, which *replaces*
`v_name`), `write_sparam` (`:826`), `spec` (`spec.c:213`) and `fft`/`psd`
(`com_fft.c:165`, `:398`) carry the preserved name. Without it, `preserve`
delivered case on `listing`/`show`/batch-`-r` and discarded it on five other
surfaces.

This changed `fold`-mode output for generated internal node names such as
`q1#collCX` (`src/spicelib/devices/bjt/bjtsetup.c:433`) and churned committed
`.out` files. It was taken as the correction rather than as
bug-compatibility, and the decision is recorded in that commit message.

### 2.6 RED tests

`tests/regression/case/name-roundtrip.cir` — identical electrical answer under
both modes, casing as the only variable, asserted on `show` (live today) and on
`print`/`display` (live only after 2.5 lands):

```spice
* name round trip
.OPTIONS noacct
V1 InA 0 dc 1.5
Rser InA OutB 1k
Rload OutB 0 1k
.control
op
print v(OutB)
show Rser
.endc
.end
```

RED today: `v(outb) = 7.500000e-01` and `device  rser`. GREEN demands
`v(OutB)` and `Rser` with the value unchanged.

`tests/regression/case/case-flag-noop.cir` — an all-lowercase legacy deck run
with the flag ON, byte-identical to a copy in `tests/regression/misc/` run with
the flag OFF, with identical `.out`. Proves the switch is a strict no-op.

Add a **mixed-case** `fold`-mode guard too: the all-lowercase noop deck cannot
catch a Phase 1 sweep that accidentally changed `fold`-mode behavior on the
`.lib`/`.inc`/whitelist lines that are not folded today.

### 2.7 Differential sweep, outside the harness

`make check` cannot see most of this. Run, as a separate script, every `.cir`
under `tests/`: (a) stock, (b) `preserve` on the original, (c) `preserve` on a
mechanically uppercased copy — comparing raw stdout and rawfile numerics with
case normalised in the comparison. This is the only mechanism that exercises the
long tail, and it is unbudgeted infrastructure; treat it as a deliverable, not a
convenience.

## Phase 3 — `distinguish`, gated and unscheduled

Do not schedule until every item below is closed. Each produces wrong numbers
with no diagnostic.

1. `src/spicelib/parser/inpptree.c:1249`, whose `INPtermInsert` call is at
   `:1256` — **done**. `mkvnode` created on miss, so a case-mismatched `V(IN)`
   in a B-source manufactured a node no card defines. It still creates, because
   that is what makes a forward reference to a node defined by a later card
   work; the call is `INPtermInsertRef` and `INPtermCaseCheck()` reports the
   near-miss after the parse, when a miss can finally be told from a forward
   reference. `doc/claude/decisions/0002-deferred-node-resolution-check.md`;
   `tests/regression/casedist/bsource-node-case.cir` is the deck and
   `bsource-forward-ref.cir` plus the `tests/regression/case` twins are the
   guard.
2. `src/xspice/evt/evtcheck_nodes.c:720` — auto-bridge `strcmp`; mixed-signal
   decks silently lose their bridges.
3. `src/frontend/vectors.c:60,71,184` — **done**. `nghash_unique(..., FALSE)`
   plus folded key and query meant two case-variant nets aliased and `print`
   returned whichever hashed first. Until this was exact, `distinguish` could
   create two nets that the user could not address independently and the
   feature was unobservable, which is why this gate went first. The fold and
   the duplicate entries stay; the chain is filtered on the typed spelling.
   `tests/regression/casedist/` is the new directory that can assert it, and
   `node-case-split.cir` is the deck.
4. `src/frontend/measure.c:236` (done), `src/frontend/outitf.c:391`
   (open, `doc/codex/issues/0016`), `src/frontend/subckt.c:659` (done).
5. A source lint that fails the build on a new `strcmp` against a lowercase
   literal in the affected directories, so the sweep does not decay.

### RED tests for Phase 3

`tests/regression/case/node-case-split.cir` — two nets differing only by case,
both interpretations solvable:

RED today prints one value for both `v(Out)` and `v(OUT)`; GREEN prints two
different voltages. 2.5 has landed, so the labels are safe to assert on as
well as the values.

`tests/regression/case/instance-case-split.cir` — `Rab` and `RAB` coexisting,
each independently addressable via `@name[resistance]` and `alter`. Today the
deck does not run at all (`device already exists, bail out`), so the `.out`
contains numeric lines the current binary cannot produce. The second-order
assertion — that `alter RAB` moves only one of the two — catches a
half-implementation.

`tests/regression/case/subckt-pin-case.cir` — two `.subckt` formals differing
only by case. Today both fold to one formal and the second actual is silently
dropped; a 1 MΩ keeper resistor to ground keeps both interpretations
non-singular so the evidence is a voltage rather than a filtered warning.

`tests/regression/case/subckt-hier-case.cir` — case survives flattening
(`subckt.c:1123-1125`), including an internal node whose name differs only by
case from a formal pin. `display` is usable here because all vector names are
distinct under both modes; note that `com_display` sorts with `strcmp`
(`com_display.c:20`), so mixed-case names re-sort ASCII-wise.

`tests/regression/case/xspice-event-case.cir` — event node split plus a
`%`-qualified port and a code-model parameter. Needs its own `spinit` following
the `tests/xspice/digital/spinit.in` pattern, registered in `AC_CONFIG_FILES`.
This test demands a production change — `evtcheck_nodes.c:720` — that no design
currently scopes.

### Missing coverage to add before Phase 3 starts

- `.param VDD` with `{vdd}`: numparam is the highest-likelihood break and has no
  test.
- `.MEAS` with mixed-case analysis name: silent skip, invisible to `check.sh`.
- Shared library: `tests/` has no shared-lib harness at all. Record as a known
  gap rather than omitting it.

## Regression guards, all phases

- `tests/regression/misc/scale-suffix-case.cir` — `1MEG`/`1meg`/`1M` semantics,
  device names all lowercase so the `.out` is mode-independent.
- `tests/regression/misc/device-letter-case.cir` — `R1` versus `r1` as the
  leading letter, evidence is voltages only.
- `tests/regression/misc/gnd-alias-case.cir` — `GND`/`gnd` both reaching node 0.
- `tests/xspice/digital/xspice-keyword-case.cir` — `%VD`, uppercase model
  parameters, all identifiers lowercase.
- Whole suite: `make check` green with `casemode` unset at every phase. Every
  existing deck is an all-lowercase legacy deck, so the existing suite is the
  broadest available guard.

## Upstream strategy and fallback

Merge odds, from this repository's own evidence:

| Component | Odds | Reason |
| --- | --- | --- |
| Phase 0 | high | Each standalone, each demonstrably wrong today |
| Phase 1, split per file | 60–70% | Correct on its own merits; near zero as one large patch |
| Phase 2 switch + guard | ~50% | Tiny diff, `newcompat` is precedent, default unchanged |
| Fold-the-key refactor | ~25% | No user-visible payoff the day it lands; touches parser, devices, XSPICE |
| Phase 3 | 10–15% | Silent wrong answers are the one thing a simulator must not ship |

Structural obstacles that are not generic pessimism: there is no CI in the tree
and three hand-maintained MSVC project files; the manual is not in this
repository; and `make check` filters out precisely the diagnostics this change
produces.

If the feature is declined:

1. Upstream Phase 0 and Phase 1 anyway, separately, per file. They shrink the
   fork and they are the majority of the actual engineering.
2. Carry Phase 2 locally as single-token edits — `strcmp(` → `ng_ideq(` on an
   otherwise-untouched line rarely conflicts on rebase; a restructured function
   always does.
3. Do not reach for a shadow case-restoration table as a cheap fallback. It
   cannot cover names ngspice *constructs* rather than reads: subcircuit
   flattening (`subckt.c:1123-1125`, `:1761`), numparam substitution, and the
   pspice/ltspice/udevice rewrites all run before interning and destroy
   token-position mapping. Stashing the original line on the card struct fails
   for the same reason. There is no cheap fallback.

## Prerequisites, absent from every design reviewed

**All six are closed.** `doc/claude/checklists/phase2-prerequisites.md` takes
them one at a time and says for each whether it needed action, what was done,
and what was deferred with what consequence. The list below is kept for the
record with its outcome marked; do not re-open one of these without reading
that file first.

Open these before writing code:

- A scope entry for `src/frontend/vectors.c:1138` plus
  `postcoms.c:212,661,826`, `com_fft.c:165,398`, `spec.c:213`. **Done**, 2.5.
- A `visualc/*.vcxproj` (×3) checklist item. **Done**, checklist section 1: no
  action was needed because Phase 2 added no new `.c` file, and the standing
  obligation for anyone who does is stated there.
- `man/man1/ngspice.1` documentation for `-D`, plus a `NEWS` bullet and an
  out-of-tree manual plan. **Done**, checklist section 2. `-D`/`--define` is in
  the man page's OPTIONS section between `-o` and `-p`, the `casemode` bullet is
  in `NEWS`'s Ngspice-47 block, and `src/spinit.in:17` ships a commented
  `*set casemode=preserve`. The manual itself is deferred with a stated
  consequence: it is not in this repository.
- A contract note in `src/include/ngspice/sharedspice.h` covering the
  `ngGet_Vec_Info` / `ngGet_Evt_NodeInfo` case asymmetry. **Done**: the note is
  `src/include/ngspice/sharedspice.h:65-84`, and it says in as many words that
  `ngGet_Evt_NodeInfo` "compares with strcmp, so it only accepts the exact
  string that `ngSpice_AllEvtNodes` returned".
- A single decision record for OSDI (`src/osdi/osdiinit.c:74`). **Done**,
  checklist section 4 for `fold` and `preserve` and
  `doc/claude/decisions/0001-distinguish.md` decision 4 for `distinguish`. One
  answer in all three modes: keep the fold, keep the case-insensitive matchers.
- An explicit paragraph stating that CIDER and `tclspice` are out of scope, and
  what breaks in them. **Done**, checklist section 5.
