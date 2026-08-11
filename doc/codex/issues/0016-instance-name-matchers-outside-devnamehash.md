# Issue: Instance-Name Matchers Outside `DEVnameHash` Are Case Sensitive

## Status

**Open**, with class (a) fixed and class (b) outstanding. The three
device-letter comparisons are fixed and criterion 3 is met in full; criteria 1
and 2 are met only for the device-letter half of the `show` filter, and the two
instance- and model-name compares at `gens.c:289` and `:295` are untouched by
design. They belong to the `distinguish` design decision and must not be taken
before it. Found while verifying acceptance criterion 4 of
`doc/codex/issues/0010`.

## Summary

Issue 0010's criterion 4 — "instance-name lookup through `DEVnameHash` resolves
`alter r1` against a deck that declared `R1`" — is met, and the reason is that
`src/frontend/spiceif.c` calls `INPretrieve()` on the typed name before
`ft_sim->findInstance()` at every one of its six call sites (`:687`, `:751`,
`:794`, `:828`, `:866`, `:953`), so the hash is only ever probed with the
interned first-seen spelling.

Five matchers do **not** go through `DEVnameHash`. They split cleanly in two,
and the split is visible in the code itself: `gens.c:289` indexes
`dev_name + 1`, deliberately stepping over the device letter, so the letter and
the identity are already one pointer offset apart inside a single expression.

| Site | Class | Comparison | What it breaks |
| --- | --- | --- | --- |
| `src/frontend/gens.c:260` | **(a)** device letter | `type != *dev_name` | `show` / `showmod` device-letter filter |
| `src/frontend/device.c:1489` | **(a)** device letter | `dev[0] == 'm'` on the typed name | `alter M1 w=…` does not re-bin |
| `src/frontend/outitf.c:391` | **(a)** device letter | `tmpname[1] == 'd'` on a generated name | `save alli` loses diode branch currents |
| `src/frontend/gens.c:289` | (b) instance identity | `strcmp(device, dev_name + 1 + subckt_len)` | `show` instance filter |
| `src/frontend/gens.c:295` | (b) model identity | `strcmp(model, mod_name)` | `showmod` model filter |

`gens.c:276` in the same loop already uses `ciprefix()` for the subcircuit
component, so the file was internally inconsistent about this before the fix and
still is, now on the (b) line. `device.c:1489` had the same inconsistency inside
one expression: the *parameter* names on that line already used `eqc()` while
the device letter did not.

## Impact

Under `fold` none of it is reachable: the reader has lowercased the deck and
`inp_casefix()` has lowercased the typed command text.

Under `casemode=preserve` the stored spelling is the deck's, and the typed
spelling is the user's:

```spice
* show with a lower-case typed name against an upper-case declaration
.OPTIONS noacct
V1 1 0 dc 4
RSer 1 2 1k
RLoad 2 0 3k
.control
op
show rser
show RSer
.endc
.end
```

```
$ ngspice --batch showprobe.cir
      ... show rser  -> Resistor: device rser, resistance 1000
      ... show RSer  -> Resistor: device rser, resistance 1000

$ ngspice -D casemode=preserve --batch showprobe.cir
      ... show rser  -> No matching instances or models
      ... show RSer  -> Resistor: device RSer, resistance 1000
```

`alter rser resistance = 3k` against the same deck works in both modes, which is
the contrast that identifies `gens.c` rather than the device table as the
defect: `alter` goes through `INPretrieve()`, `show` does not.

`device.c:1489` is the quieter one and the only one that can change a number
without a diagnostic. Under `preserve`, `alter M1 w=2u` on a deck that declared
`M1` took `dev[0] == 'M'`, skipped `if_set_binned_model()`, and applied the new
geometry without re-selecting the model bin; `alter m1 w=2u` on the same deck
re-binned. Originally recorded as verified by reading; it has since been
reproduced on a binned BSIM3 deck, which is the RED evidence in the Resolution.

`outitf.c:391` is the `save alli` rewrite of `@D1#internal` to `[id]`: with a
deck-cased `D1` the test `tmpname[1] == 'd'` fails and the code falls to the
`"Debug: could output current for %s"` arm. With `save alli` as the only save
this does not merely lose the diode branch current — the run ends with zero data
descriptors and reports `Error: no data saved for D.C. Operating point analysis;
analysis not run`.

`src/frontend/device.c:1420` is the gate that makes all of this reachable: it
folds the typed name only under `inp_case_folding()`, which is correct — folding
it under `preserve` would make every deck-cased name unreachable — and is what
exposes the user's spelling to the sites above.

## Root Cause

`gens.c` is the legacy `show`/`showmod` walker; it scans the device list
directly rather than using the name tables the parser built, so it never sees
the interning that makes `alter` work. `device.c:1489` and `outitf.c:391` are
the same lower-case device-letter dispatch class as `doc/codex/issues/0009` and
`doc/codex/issues/0014`, in files those issues did not enumerate.

## Acceptance Criteria

1. Under `preserve`, `show r1` and `show R1` both match a deck that declared
   `R1`, as they do under `fold`.
2. `showmod` matches a model name regardless of spelling.
3. `alter M1 w=2u` re-bins a binned MOS model, and `save alli` emits the diode
   branch current, for an upper-case device letter.
4. `tests/regression/case/` gains a deck whose evidence is a voltage rather than
   a `show` table, because `tests/bin/check.sh` filters the diagnostics and much
   of the table text; the `alter`-re-binning witness is the usable one.
5. `make check` unchanged with `casemode` unset.

## Resolution

**Class (a) fixed; class (b) deliberately not.** Three one-character
comparisons, one line each:

```c
/* src/frontend/gens.c:260 */
-                } else if (type != *dev_name) {
+                } else if (tolower_c(type) != tolower_c(*dev_name)) {

/* src/frontend/device.c:1489 */
-    if ((dev[0] == 'm') && (eqc(param, "w") || eqc(param, "l")))
+    if ((tolower_c(dev[0]) == 'm') && (eqc(param, "w") || eqc(param, "l")))

/* src/frontend/outitf.c:391 */
-                    } else if (strstr(ch, "#internal") && (tmpname[1] == 'd')) {
+                    } else if (strstr(ch, "#internal") && (tolower_c(tmpname[1]) == 'd')) {
```

`tolower_c()` rather than `elem_letter()` at all three: `elem_letter()` is for a
card's leading device letter, which is the rule `doc/codex/issues/0018` set and
`doc/codex/issues/0014` follows. None of these three is handed a card — two read
a user-typed instance name and one a generated vector name — so the named
helper does not apply and the fold is written out.

### Why splitting (a) from (b) is safe

Argued rather than assumed. `gens.c:260` is a **pre-filter**: it rejects a
candidate before the name compares at `:289` and `:295` ever run. Folding it can
therefore only *widen* the candidate set those two compares see, and any
instance that then fails a byte-exact name compare is rejected exactly as it is
today. The resulting intermediate state — `show r` works, `show Rser` against a
deck that declared `RSer` still does not — is coherent, not half-broken.

Class (b) is the `distinguish` decision's territory: whether two names differing
only in case are one identity or two is precisely the question that decision
settles, and answering it inside `gens.c` would pre-empt it. Criteria 1 and 2
stay open on that account, and this issue stays **Open**.

### RED evidence

`tests/regression/case/alter-rebin-case.cir` and its `-lower` twin,
`device.c:1489`. Two BSIM3 bins differing only in `vth0` and in which width band
they claim, so crossing the boundary is a drain voltage:

```
                       op                alter M1 w=2u
stock, upper           1.432887e+00      1.481980e+00
preserve, lower twin   1.432887e+00      1.481980e+00
preserve, upper  RED   1.432887e+00      1.340324e+00
```

The RED number is the device keeping `nch.1`'s `vth0=0.4` at the new width and
drawing more current. The committed `.out` also carries
`Notice: model has changed from nch.1 to nch.2.`, which survives `check.sh`'s
filter and is absent from the RED run. This closes the half of criterion 3 that
had only ever been verified by reading, and it satisfies criterion 4: the
evidence is a voltage, not a `show` table.

`tests/regression/case/save-alli-case.cir` and its `-lower` twin,
`outitf.c:391`. `save alli` is the only save, so dropping the diode's internal
node leaves the run with no data descriptors and the operating point is never
solved. The diagnostic is on stderr, which `check.sh` does not capture, so the
deck reads the diode current back out of the circuit into a fixed-name vector —
fixed-name so that the printed label does not carry the deck's spelling and the
two twins' `.out` files stay byte identical:

```
preserve, lower twin   zprobe = 3.281336e-03
preserve, upper  RED   zprobe = 0.000000e+00      analysis not run
```

`rs` on the diode model is load-bearing: without a series resistance there is no
internal node and nothing for `save alli` to convert.

`tests/regression/case/show-device-letter-case.cir`, `gens.c:260`. One deck, no
twin, because what is under test is the typed letter rather than the deck's
spelling: it declares `RSer` and `RLoad` and runs `show r` and then `show R`,
asserting the same table twice. RED printed `No matching instances or models`
for the first of the two. The deck deliberately passes no instance name, so it
exercises the pre-filter and nothing of class (b).

### Fold-mode visibility

**None of the three changes is fold-mode visible.** Under `fold` the reader has
lowercased the deck and `com_alter_common()` lowercases the typed name at
`device.c:1420`, so every character these three sites test is already lower case
and `tolower_c()` is the identity on it. `outitf.c:391` reads a generated
internal node name, which is built from the stored instance name and is
therefore lower case under `fold` for the same reason.

`make check` with `casemode` unset is 214 tests, 0 FAIL, with no committed
`.out` touched. The differential sweep does not move: no deck under `tests/`
uses `show`, `showmod` or `save alli`.

### Still open

- Class (b), `gens.c:289` and `gens.c:295`. Criteria 1 and 2.
- `doc/claude/suggestions/case-sensitive-identifiers-plan.md` Phase 3 lists
  `outitf.c:391` among the items to close before Phase 3 starts; that entry can
  now be marked done, and the `gens.c` (b) pair joins the `distinguish` gate
  list alongside `inpptree.c:1256`, `vectors.c:58,71,184`,
  `evtcheck_nodes.c:720` and `evttermi.c:304`.
