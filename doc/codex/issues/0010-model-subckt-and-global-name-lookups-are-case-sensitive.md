# Issue: Model, Subcircuit and Global-Node Lookups Are Case Sensitive

## Status

Fixed. All five acceptance criteria are met; see Resolution.

## Summary

Phase 2 of the case-sensitivity work made the parser's own interning tables
fold their lookup key, so nodes and instances keep one identity across
spellings (`src/spicelib/parser/inpsymt.c`). Three other name spaces were left
alone, and each still matches with `strcmp`:

| Name space | Site | Comparison |
| --- | --- | --- |
| `.model` name | `src/spicelib/parser/inplkmod.c:24` | `strcmp(i->INPmodName, name)` |
| `.model` name | `src/spicelib/parser/inpmkmod.c:38`, `:60` | nghash with `NGHASH_FUNC_STR`, i.e. `strcmp` |
| `.subckt` name | `src/frontend/subckt.c:604` | `eq(sss->su_name, s)` |
| `.subckt` translation table | `src/frontend/subckt.c:1610` | `eq(table[i].t_new, subname)` |
| `.global` node | `src/frontend/subckt.c:146`, `:1650` | nghash with `NGHASH_FUNC_STR` |
| instance name | `src/include/ngspice/cktdefs.h:322` `DEVnameHash`, used at `src/spicelib/analysis/cktdltm.c:36` | `strcmp` |

## Impact

Under `fold` none of this is reachable: the reader has already lowercased both
sides.

Under `casemode=preserve` the contract is that identity is unchanged — `R1` and
`r1` are the same device, `preserve` changes only what ngspice shows
(`doc/claude/specs/case-sensitive-identifiers.md`). These six sites break that
contract for a deck that spells one name two ways:

```spice
r3 1 2 RModel1 l=11u w=2u
.model rmodel1 r rsh=1000 narrow=1u
```

```
$ ngspice -D casemode=preserve --batch two-spellings.cir
Error on line 2 or its substitute:
  r3 1 2 RModel1 l=11u w=2u
  unknown parameter (RModel1)
```

The `.global` case is the quieter one, and the only one that can produce a
wrong answer rather than a diagnostic. `inpcom.c:1939` inserts a hard-coded
lower-case `.global gnd`, `collect_global_nodes()` stores the node name
verbatim into a `strcmp` hash (`subckt.c:146`, `:161`), and `gettrans()` probes
it with `nghash_find` (`subckt.c:1650`). A deck whose subcircuit port is
spelled `GND` therefore does not match the automatic global, so the port is
scope-renamed per instance instead of being shared. Note that
`inp_fix_gnd_name()` cannot rescue it: its delimiter test requires a character
after the token (`gnd[3]`), and the newline has already been zapped at
`inpcom.c:1889`, so a card that *ends* in `GND` — which is what a `.subckt`
header does — is never rewritten.

A user cannot see any of this in a rawfile, because a rawfile has no
case-policy field.

## Root Cause

The plan calls this the "fold-the-key refactor" and rates it separately from
the rest of Phase 2, at roughly 25% merge odds, because it "touches parser,
devices, XSPICE" with "no user-visible payoff the day it lands"
(`doc/claude/suggestions/case-sensitive-identifiers-plan.md`). It was scheduled
out of Phase 2 deliberately, not overlooked.

The constraint that makes it more than a one-line change is
`src/misc/hash.c:548`: `curTable->key = copy(user_key)` runs **only** when
`hash_func == NGHASH_DEF_HASH(NGHASH_FUNC_STR)`, and the frees at `:110`,
`:182`, `:393` and `:471` are gated the same way. Installing a custom
case-folding hash function silently flips every such table from owning its keys
to borrowing them, which is the exact class of defect commits `c5cd68015` and
`5ad395d5e` were written to fix. The key has to be folded at the call site
instead, in every insert and every probe, or `nghash` needs a first-class
case-insensitive string mode that keeps the copy-and-free behaviour.

## Acceptance Criteria

1. Under `preserve`, a `.model`, `.subckt` or `.global` name matches its
   references regardless of spelling, with the first-seen spelling retained for
   reporting.
2. No `nghash` table changes from owning its keys to borrowing them. Either the
   key is folded by the caller before `nghash_insert`/`nghash_find`, or
   `NGHASH_FUNC_STR` gains a case-insensitive sibling that keeps the
   `copy(user_key)` and the matching frees.
3. `tests/regression/case/` gains a deck that spells one model name, one
   subcircuit name and one global node two ways each and produces the same
   voltages as its consistently-spelled twin.
4. Instance-name lookup through `DEVnameHash` resolves `alter r1` against a
   deck that declared `R1`.
5. `make check` unchanged with `casemode` unset.

## Resolution

Fixed in three commits, one per name space. Every comparison is gated on
`inp_case_folding()`, so the default mode is the same predicate on the same
bytes, and no `nghash` table changes from owning its keys to borrowing them.

Two shared helpers were added:

```c
/* src/frontend/inpcom.c, next to inp_case_folding(); declared in fteext.h */
bool ng_ideq(const char *a, const char *b)
{
    return inp_case_folding() ? (strcmp(a, b) == 0) : (cieq(a, b) != 0);
}
```

and `model_name_match()` (`src/misc/string.c`) gained a `bool ci` parameter.
The policy is **passed in** rather than read inside the function, because
`src/misc` has no frontend dependency today — `grep -rl 'cp_getvar\|ft_curckt\|
fteext.h' src/misc/*.c` is empty — and `ngsconvert` links `misc/string.lo`
without `libfte` under `--enable-oldapps`.

`ng_ideq()` is deliberately gated rather than spelled `cieq` outright. The gate
costs nothing, it matches the in-tree precedent at
`src/spicelib/parser/inpsymt.c:33`, and `q1#collCX`
(`src/spicelib/devices/bjt/bjtsetup.c:433`) proves the codebase does mint
upper-case identifiers of its own, so any future name space that reaches these
comparisons inherits the protection.

### The three decisions the issue left open

**1. Is `INPlookMod()` enough for the `.model` half? No — three layers, and the
one that matters most is upstream of the parser entirely.**

`INPlookMod()` (`inplkmod.c:24`) is the gate that `INP2R`/`C`/`L`/`D`/`Q`
consult *before* interning the token, so it is the only site that sees the raw
deck spelling for those devices; once it passes, `INPinsert()` canonicalises
the token and `INPgetMod()` is probed with the interned name. But `INP2M`, the
OSDI `INP2N` and XSPICE's `MIFgetMod()` have **no** `INPlookMod()` gate — they
hand the raw token straight to the `modtabhash` probe at `inpgmod.c:364` — so
folding `INPlookMod()` alone leaves every MOSFET deck failing with `could not
find a valid modelname`. `modtabhash`'s probe (`inpmkmod.c:38`) and insert
(`:60`) therefore fold too, and they fold **together**: a folded probe against
unfolded keys would be a new miss. The key is built once, above the `if`/`else`
in `INPmakeMod()`, because the shape of that `if`/`else` skips the duplicate
check for the first model of every circuit and would otherwise leave exactly
one model per deck keyed unfolded.

Upstream of all of that, `inp_rem_unused_models()` (`inpcom.c:9528`, `:9648`)
matches instance references against `.model` names with `model_name_match()`,
and a miss makes `rem_unused_xxx()` comment the `.model` card out before
`INPpas1()` runs. Nothing below can recover from that, so it is the first thing
that had to fold.

`inpgmod.c:398` and `inpcom.c:3254` are inside `#if 0` blocks and are dead;
`inpcom.c:3254` was updated only to keep the disabled block compiling against
the new signature, and is not coverage.

**2. Is `subckt.c:1610` reachable independently of `:604`? No, but it is a
distinct second failure that `:604`'s fix exposes.**

`doit() -> translate() (:742) -> settrans() (:1164)` is the only call chain;
neither `translate()` nor `settrans()` has a second caller. On `settrans()`'s
final iteration the formals are exhausted and the remaining actual is the
subcircuit name as the *instance* spelled it, compared against `subname =
sss->su_name`, the *definition*'s spelling. With only `:604` folded, the deck
gets past "unknown subckt" and dies at `Too many parameters for subcircuit
type "divider"`. The two must ship together, and they do.

**3. Do `collect_global_nodes()`'s hard-coded `"0"` and `"null"` inserts need
anything? Not on the insert side. The probe side changes one behaviour, on
purpose.**

Both are already lower-case literals, so folding the key leaves them alone.
Folding the *probe* means a node spelled `NULL` or `Null` inside a subcircuit
body now matches the `"null"` global under `preserve` and stops being scope
renamed — for every device, not only XSPICE `A` cards, because `gettrans()` is
probed from `translate()`'s `default:` arm. That is exactly what `fold` mode
already does for `null`, and it agrees with `src/xspice/mif/mifutil.c:198`,
which classifies the null port with `cieq`. Recorded here rather than
suppressed, because the alternative — a second, unfolded probe for the literal
`"null"` — would leave `preserve` disagreeing with `mifutil.c`.

### Option (a) versus option (b)

Option (a), fold at the call site. Option (b), a first-class case-insensitive
`nghash` string mode, would touch roughly 80 lines across `src/misc/hash.c` and
`src/include/ngspice/hash.h`: 6 `switch (hash_func)` dispatch sites
(`hash.c:242, 354, 428, 502, 590, 823`), 7 comparison gates (`:259, 309, 373,
447, 521, 607, 845`) and 7 ownership gates (`:110, 182, 393, 471, 548, 787,
866`), every one of them on the hot path of the 14 `nghash_init()` call sites in
the tree. It would also add a fourth sentinel to a scheme whose stated safety
property is that a mis-set `hash_func` segfaults immediately
(`hash.h:62-70`) — miss one `switch` site and `default:` calls the sentinel as
a function pointer. Option (a) touches no shared code and cannot perturb the
`.param`, vector or device-name tables.

### Sites changed

`.model` name space — commit "fold the key for .model name lookup":

| File | Site |
| --- | --- |
| `src/frontend/inpcom.c` | `inp_find_model_1()`, `mark_all_binned()` |
| `src/spicelib/parser/inplkmod.c` | `INPlookMod()` |
| `src/spicelib/parser/inpmkmod.c` | new `INPmodKey()`; `INPmakeMod()`'s probe and insert |
| `src/spicelib/parser/inpgmod.c` | `INPgetMod()`'s `modtabhash` probe; `INPgetModBin()`'s binning match |
| `src/xspice/mif/mifgetmod.c` | `MIFgetMod()`'s `modtab` walk |
| `src/frontend/subckt.c` | new `wl_find_id()`; `translate_mod_name()` and the three `devmodtranslate()` probes; the `'m'`-arm `model_name_match()` |
| `src/misc/string.c`, `src/include/ngspice/stringutil.h` | `model_name_match()` gains `bool ci` |
| `src/frontend/inpcom.c`, `src/include/ngspice/fteext.h` | `ng_ideq()` |

`src/misc/wlist.c:436` (`wl_find()`) was **not** changed: it is a generic
wordlist helper with callers all over the tree, so `subckt.c` got a local
policy-aware find instead.

`.subckt` name space — commit "fold the key for .subckt name lookup":

| File | Site |
| --- | --- |
| `src/frontend/inpcom.c` | `nlist_find()`, `get_subckts_for_subckt()`, `find_name()`, `find_subckt_1()` |
| `src/frontend/subckt.c` | `doit()`'s definition lookup; `settrans()`; new `eq_substr_id()` for `numnodes()` |
| `src/frontend/numparam/xpressn.c` | `defsubckt()` insert key, `findsubckt()` probe, `search_isolated_identifier()` |
| `src/frontend/numparam/spicenum.c` | `findsubname()` probe |

The numparam half is folded at the `NUPA_SUBCKT` call sites only, not inside
`entrynb()`, which is the single probe for every numparam symbol. Folding
`entrynb()` would silently extend this change to the `.param`/`.func` name
space; that gap is `doc/codex/issues/0015`.

`.global` name space — commit "fold the key for .global node lookup":
`src/frontend/subckt.c` gains `glo_key()`, used by `collect_global_nodes()`'s
probe and insert and by `gettrans()`'s probe. `gettrans()` probes with a
separate temporary because `newgl` is the value returned to the caller and its
ownership is signalled by `*isglobal`.

### Evidence

Four RED decks under `tests/regression/case/`, each with a
consistently-spelled twin carrying a byte-identical `.out`, so the assertion is
"the spelling cannot change the number" rather than "this run printed this".

| Deck | RED before |
| --- | --- |
| `model-name-case.cir` | no output; `warning, can't find model 'RMod'` then `unknown parameter (RMod)` |
| `subckt-name-case.cir` | no output; `Error: unknown subckt: x1 1 2 Divider` |
| `global-node-case.cir` | `v(2) = 4.000000e+00` against an expected `3.000000e+00` |
| `name-lookup-case.cir` | criterion 3: all three name spaces in one deck |

`global-node-case.cir` is the one that matters, and the only one
`tests/bin/check.sh` could have caught unaided: its failure is a number, and
the filter at `check.sh:20` removes `Error` and `Warning` from both sides of
the comparison. It is also the shape that no diagnostic would ever have
surfaced — the port silently becomes per-instance and the divider stops
dividing.

The per-mechanism split was necessary: with only `subckt.c:604` folded, the
subcircuit deck died at `:1610`; with `:604` and `:1610` folded it died in
numparam with `Error, illegal subckt call.` A single combined deck aborts on
the first error and hides the rest, which is what happened on
`doc/codex/issues/0009`.

### No fold-mode guard is possible, and the issue-0009 argument had to be redone

Issue 0009's Resolution argued that its change was invisible in `fold` mode
because the reader's exemptions "all cover dot cards and control lines rather
than device cards". That argument does not transfer: `.lib`, `.inc` and the
`.control` whitelist *are* dot cards and control lines, and this change is in
the `.subckt` and `.global` paths, which dot cards feed. It was redone per
exemption, all in `src/frontend/inpcom.c`:

- `.lib` / `.inc`, whole-card exempt at `:1898`. The `.include` card itself is
  commented out at `:1703` and a `.lib` reference at `:4133`, so neither
  supplies a name to any of these lookups. The *contents* of both are read by
  the same recursive `inp_read()` — `:1685` for `.include`, `:599` for
  `read_a_lib()` — and are folded identically. The `.lib` section name is
  already matched case-insensitively at `:542`.
- The 13-command `.control` whitelist at `:1899-1910`. None of `write wrdata
  codemodel osdi pre_osdi echo shell source cd load setcs strcmp strstr`
  supplies a model, subcircuit or global name; `source` re-enters
  `inp_readall()` and is folded from scratch. `circbyline`, `alter` and
  `altermod` are *not* on the whitelist and are folded.
- The `print`/`eprint`/`eprvcd`/`asciiplot` redirection tail at `:1866-1880`
  spares only text after the first `>`.
- The `plot`/`gnuplot`/`hardcopy` arm lowercases every character at `:1810`
  except a token immediately following `title`, `xlabel` or `ylabel`.
- `keep_case_of_cider_param()` and `is_xspice_model()` spare only text between
  exactly two `"` characters; the `.model` keyword, the model name and the type
  are all outside the quotes.

Independently of all of that, `src/frontend/inp.c:817` runs `inp_casefix()`
over every non-`.plot`/`.print` card a second time — after the
`pspice_compat`/`ltspice_compat`/`udevices` passes that *synthesize* `.model`
and `.subckt` cards, and before `inp_subcktexpand()` at `inp.c:936` and
`if_inpdeck()` at `:1429`. So even a synthesized or straggling card is lower
case before it can reach any of these comparisons.

A `tests/regression/misc/` deck would therefore pass identically with and
without this change, which is the definition of a test that cannot be RED. The
invariant it would have pinned is instead recorded here: **any future
card-synthesis pass moved after `inp_spsource()`'s `inp_casefix()` loop breaks
the fold-mode identity argument for all three name spaces.**

### Acceptance criteria

1. **Met.** All three name spaces resolve across spellings under `preserve`,
   and the first-seen spelling is what is stored and reported: `INPmodName` is
   left aliased to the interned token, `t_ent` keeps the deck's spelling, and
   `gettrans()` returns the body's spelling untouched.
2. **Met.** No table changes ownership. Every fold produces a temporary key
   that the caller frees; `hash_func` stays `NGHASH_FUNC_STR` everywhere, so
   `src/misc/hash.c`'s `copy(user_key)` at `:548` and the frees at `:110`,
   `:182`, `:393`, `:471` and `:866` are untouched.
3. **Met.** `tests/regression/case/name-lookup-case.cir` plus its twin.
4. **Met, and confirmed by running it rather than by inspection.** A deck
   declaring `RSer`/`RLoad` with `alter rser resistance = 3k` moves `v(2)` from
   `3.000000e+00` to `2.000000e+00` identically under `fold` and under
   `preserve`. The mechanism is `src/frontend/spiceif.c`, which calls
   `INPretrieve()` on the typed name before `ft_sim->findInstance()` at all six
   of its call sites (`:687`, `:751`, `:794`, `:828`, `:866`, `:953`), and
   `INPretrieve()` goes through `inpsymt.c`'s folding `ent_eq()`, so
   `DEVnameHash` is only ever probed with the interned spelling.
   `src/spicelib/devices/cktcrte.c:66` inserts that same interned token, and
   every `newInstance` call site in the tree passes a token that has just been
   through `INPinsert()`, so the table holds exactly one spelling per instance.
   No change was needed. Two caveats are recorded rather than fixed: the return
   value of `INPretrieve()` is discarded at all six sites, so a future device
   created without `INPinsert()` would silently reach the hash with raw user
   text; and `show`/`showmod` resolve instance names through a separate
   `strcmp` scan in `src/frontend/gens.c` that never touches `DEVnameHash` —
   that is `doc/codex/issues/0016`.
5. **Met.** `make check` is 101 tests, 0 FAIL with `casemode` unset, against a
   baseline of 93; the eight decks added here account for the difference. The
   suite was run to completion at each of the three commits: 95, 97, 101.

### Differential sweep

`python3 doc/claude/scripts/case_differential_sweep.py --jobs 12 --timeout 90`,
measured on this tree immediately before and after the series:

| verdict | before | after |
| --- | ---: | ---: |
| OK | 66 | 96 |
| DIFF | 40 | 41 |
| PARSE-FAIL | 25 | 2 |
| NUM-DIFF | 0 | 0 |
| SKIP | 3 | 3 |
| **total** | **134** | **142** |

Compared per deck rather than on the totals: **no deck's verdict got worse, and
no deck that was OK became anything else.** 20 decks went `PARSE-FAIL` to `OK`,
2 went `DIFF` to `OK` (`lib-processing/ex1b.cir` and `model/binning-1.cir`, the
latter being the binning fold at `inpgmod.c:306`), and 3 went `PARSE-FAIL` to
`DIFF` (`bsim3soi{dd,fd,pd}/ring51.cir`). The 8 extra decks are the tests added
here; all 8 report OK.

The three `ring51.cir` decks land in DIFF rather than OK for a defect that is
neither new nor numeric. Their card `cout  buf ss 1pF` carries an upper-case
`F`, and `is_a_modelname()`'s unit-suffix test at `inpcom.c:3206` is
case-sensitive, so under `preserve` `1pF` is not recognised as a number and is
reported as `warning, can't find model '1pF'`. That is already on issue 0009's
follow-up list; it is a warning only, and the rawfile numerics are identical.
The remaining lines in their diff are transient `reference value` timing
measurements, which differ between two consecutive *stock* runs by more than
they differ between modes.

The two surviving `PARSE-FAIL`s are `tests/polezero/pz2.cir` and
`tests/polezero/pzt.cir`, which are `doc/codex/issues/0013`
(`search_plain_identifier()` is `strstr`-based, so the `ac`-with-no-value fixup
does not fire on `AC`). The three `SKIP`s are unchanged and are stock-run
timeouts.

The baseline measured here — OK=66, DIFF=40 — differs by one deck from the
figure recorded after `doc/codex/issues/0009` (OK=67, DIFF=39). Both numbers
were produced by the same command at the same commit; the difference is
run-to-run variation in one of the timing-sensitive decks, which is the same
effect the sweep already documents for the AC rawfile
(`doc/codex/issues/0012`).

### Found while fixing this, deliberately not fixed

- `doc/codex/issues/0014` — `numnodes()` (`src/frontend/subckt.c`) dispatches on
  a lower-case device letter. Issue-0009 class, in a file issue 0009 did not
  touch. RED: an upper-case `E1` inside a subcircuit body gives `Error: too few
  devs` under `preserve`. It also makes this issue's own `numnodes()` name fold
  dead code for an upper-case `X`.
- `doc/codex/issues/0015` — numparam `.param`/`.func` symbol names are matched
  byte-exactly. RED: `.param RVal=1k` with `{rval}` gives `Undefined parameter
  [rval]` and a fatal exit under `preserve`.
- `doc/codex/issues/0016` — `show`/`showmod` (`src/frontend/gens.c:289`), the
  binned-MOS re-selection in `com_alter_common()`
  (`src/frontend/device.c:1489`) and the `save alli` diode rewrite
  (`src/frontend/outitf.c:391`) match instance names outside `DEVnameHash`.
  RED: `show rser` finds nothing under `preserve` on a deck declaring `RSer`,
  while `show RSer` and `alter rser` both work.

Two smaller observations, recorded here rather than as issues because neither
is a live defect:

- `free_global_nodes()` (`subckt.c:183`) frees `glonodes` without nulling the
  file-scope pointer. There is no dereference after the free today —
  `gettrans()` is reachable only from `translate()`, which is reachable only
  from `doit()`, which runs between the init and the free — so this is hygiene
  in the `c5cd68015`/`5ad395d5e` class rather than a bug, and it was left alone
  to keep the diff to the name lookups.
- Folding `subckt.c:604` means that under `preserve`, `.subckt Foo` and
  `.subckt foo` at the same level stop being two independent definitions and
  the later one silently shadows the earlier one — which is what `fold` mode
  already does. `inpcom.c:9500`'s `Warning: redefinition of .subckt %s,
  ignored` is a different, scope-based check and now fires across spellings
  too.

One claim from the analysis pass was checked and **not** reproduced:
`IFnewUid()` (`src/spicelib/parser/ifnewuid.c:31`) mints upper-case default
model UIDs such as `"R"` and pushes them through `INPinsert()`, which folds
under `preserve`, so a deck containing both a default-model resistor and a
`.model R` was predicted to collide in `ckt->MODnameHash`. A deck of exactly
that shape produces identical voltages under `fold` and `preserve`, so nothing
is written up.
