# Issue: Instance-Name Matchers Outside `DEVnameHash` Are Case Sensitive

## Status

Open. Found while verifying acceptance criterion 4 of
`doc/codex/issues/0010`.

## Summary

Issue 0010's criterion 4 — "instance-name lookup through `DEVnameHash`
resolves `alter r1` against a deck that declared `R1`" — is met, and the reason
is that `src/frontend/spiceif.c` calls `INPretrieve()` on the typed name before
`ft_sim->findInstance()` at every one of its six call sites (`:687`, `:751`,
`:794`, `:828`, `:866`, `:953`), so the hash is only ever probed with the
interned first-seen spelling.

Three instance-name matchers do **not** go through `DEVnameHash` and are still
byte-exact:

| Site | Comparison | What it breaks |
| --- | --- | --- |
| `src/frontend/gens.c:289` | `strcmp(device, dev_name + 1 + subckt_len)` | `show` / `showmod` instance filter |
| `src/frontend/gens.c:260` | `type != *dev_name` | `show` device-letter filter |
| `src/frontend/gens.c:295` | `strcmp(model, mod_name)` | `showmod` model filter |
| `src/frontend/device.c:1489` | `dev[0] == 'm'` on the typed name | `alter M1 w=…` does not re-bin |
| `src/frontend/outitf.c:391` | `tmpname[1] == 'd'` on a generated name | `save alli` loses diode branch currents |

`gens.c:276` in the same loop already uses `ciprefix()` for the subcircuit
component, so the file is internally inconsistent about this.

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

`alter rser resistance = 3k` against the same deck works in both modes, which
is the contrast that identifies `gens.c` rather than the device table as the
defect: `alter` goes through `INPretrieve()`, `show` does not.

`device.c:1489` is the quieter one and the only one that can change a number
without a diagnostic. Under `preserve`, `alter M1 w=2u` on a deck that declared
`M1` takes `dev[0] == 'M'`, skips `if_set_binned_model()`, and applies the new
geometry without re-selecting the model bin; `alter m1 w=2u` on the same deck
re-bins. Verified by reading, not reproduced — it needs a binned BSIM deck.

`outitf.c:391` is the `save alli` rewrite of `@D1#internal` to `[id]`: with a
deck-cased `D1` the test `tmpname[1] == 'd'` fails and the code falls to the
`"Debug: could output current for %s"` arm. Verified by reading.

## Root Cause

`gens.c` is the legacy `show`/`showmod` walker; it scans the device list
directly rather than using the name tables the parser built, so it never sees
the interning that makes `alter` work. `device.c:1489` and `outitf.c:391` are
the same lower-case device-letter dispatch class as
`doc/codex/issues/0009` and `doc/codex/issues/0014`, in files those issues did
not enumerate.

`src/frontend/device.c:1420` gates its `strtolower()` of the typed name on
`inp_case_folding()`, which is correct — folding the typed name under
`preserve` would make every deck-cased name unreachable — and is what exposes
`gens.c` to the user's spelling.

## Acceptance Criteria

1. Under `preserve`, `show r1` and `show R1` both match a deck that declared
   `R1`, as they do under `fold`.
2. `showmod` matches a model name regardless of spelling.
3. `alter M1 w=2u` re-bins a binned MOS model, and `save alli` emits the diode
   branch current, for an upper-case device letter.
4. `tests/regression/case/` gains a deck whose evidence is a voltage rather
   than a `show` table, because `tests/bin/check.sh` filters the diagnostics
   and much of the table text; the `alter`-re-binning witness is the usable
   one.
5. `make check` unchanged with `casemode` unset.

## Resolution

Not fixed. `doc/codex/issues/0010`'s scope is the `.model`, `.subckt` and
`.global` name spaces plus the confirmation that `DEVnameHash` needs no change;
this is the residue of that confirmation.
