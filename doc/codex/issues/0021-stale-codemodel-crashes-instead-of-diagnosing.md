# Issue: A Stale `.cm` Crashes the Code-Model Loader Instead of Being Rejected

## Status

Open, not fixed. Found in passing while looking for a numeric witness for
`doc/codex/issues/0019`'s `cktislinear()` row, and recorded here so it is not
re-found as a mystery. It has nothing to do with the case-sensitivity series;
it is filed in this sequence only because that is where it turned up.

## Summary

`spinit` loads XSPICE code models by path. There is no version or ABI check
on what it loads. If the `.cm` on that path was built from a different
ngspice than the running binary, the run does not fail with a diagnostic —
it segfaults.

Reproduced on this tree with a deck that needs `spice2poly.cm`:

```spice
* t
.OPTIONS noacct
v1 1 0 dc 3
e1 3 0 poly(1) 1 0 1 2
r1 3 0 1k
.control
op
print v(3)
.endc
.end
```

```
$ ngspice --batch t.cir            # default spinit -> /usr/local/lib/ngspice/*.cm
Segmentation fault (core dumped)   # exit 139, nothing on stdout or stderr
```

The same deck, same binary, three other ways:

- with a `spinit` that loads **no** code models — clean diagnostic,
  `MIF-ERROR - unable to find definition of model a$poly$e1`, exit 1;
- with a `spinit` that loads the **freshly built**
  `build-ver_50/src/xspice/icm/spice2poly/spice2poly.cm` — correct answer,
  `v(3) = 7.000000e+00`;
- through `make check`, which uses `tests/xspice/case/spinit` and the build
  tree's own `.cm` files — passes.

So the crash is not in the POLY path. It is the loader accepting a shared
object it should have rejected. On this machine
`/usr/local/lib/ngspice/*.cm` dates from an earlier `compile_linux.sh` of
this same source; that is enough to produce it.

## Impact

Low frequency, high confusion. Anyone who has ever run
`./compile_linux.sh` and then works out of tree gets a binary whose default
`spinit` points at the installed `.cm` files, and any deck that touches a
code model can die with no message at all. The failure names nothing — not
the model, not the file, not the mismatch — so the natural conclusion is
that the deck or the tree is broken.

It does not affect `make check`: every test directory that needs code models
carries its own `spinit` pointing into the build tree.

## Root Cause

Not diagnosed beyond the above. `codemodel` dlopens the file and reads its
descriptor tables; nothing compares a build stamp, an interface version or a
symbol set first, so a layout change between the two builds is dereferenced
as if it were current.

## Acceptance Criteria

1. Loading a `.cm` whose interface does not match the running binary
   produces a diagnostic naming the file, and the run continues or exits
   cleanly.
2. A regression test that loads a deliberately mismatched `.cm` and asserts
   the diagnostic rather than a signal.

## Resolution

Not fixed. Recorded only.

Note for whoever picks it up: reproducing it needs two builds of ngspice
whose code-model interface differs, so the first job is to find what
actually changed between them — it may be that the two builds here differ by
more than time.
