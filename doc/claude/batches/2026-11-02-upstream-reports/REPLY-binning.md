# Reply for bugs/889

Attach: `patches/0001-subckt.c-bin-in-double-with-the-same-test-as-INPgetM.patch`

```
Patch attached, against pre-master-48 97f8de1a7 (applies with git am).

What changed: in_range() in inpgmod.c is now exported as INPbinInRange(), and the hsa pruner in doit() in subckt.c calls it, so the two stages use one test, as you preferred. The pruner compares in double, and w, l, nf in struct card become double. That makes struct card 16 bytes larger on a 64-bit build. Nothing else uses those fields except inp_get_w_l_x() and the deck copies in subckt.c.

Test: tests/regression/model/binning-hsa.cir, needs no PDK and no .spiceinit. It sets ngbehavior=hsa in .control and gives the circuit with circbyline. w=15u nf=5 and w=49u nf=7 sit on inner edges, two other sizes are off any edge, and it runs without and with .option scale. On 97f8de1a7 it fails with "could not find a valid modelname". With the patch each size lands on the expected card, checked against a plain model of the same vth0.

make -k check: 59 pass, 0 fail before; 60 pass, 0 fail after, the new one being the test. Nothing else moved.

The report's decks: 2-hsa-finger, with and without scale, now load on x1:card.2.

sky130 (nfet_01v8, tt, hsa) does not load on 97f8de1a7, so I checked it with only the inpcom.c hunk of patch #138 added, in a scratch copy. l=1 w=15 nf=5 gives -1.86245e-04 on nshort_model.20, and l=1 w=49 nf=7 gives -6.37262e-04 on nshort_model.4. Both stopped with no model before. On 200 sizes off any edge the card and i(vd) are the same before and after. On 144 sizes on an inner edge, 16 stopped before and none after. Those that loaded before kept their card, and all 144 land on the card min <= size < max gives.

The outer edge is untouched by this patch, as you suggested. A size exactly on the outermost lmax or wmax is still rejected, with and without hsa, the same as on 97f8de1a7. A switch for it would be a separate request.

One thing the patch does not change: the pruner always divides w by nf, while INPgetModBin() does not under .options wnflag=0. So with wnflag=0 and hsa, a size where w and w/nf fall in different bins finds no model, before and after. For example w=14.5u nf=5 with bins split at 3u and 7u. The one difference is a size where w/nf is on an edge and w is in the same upper bin: that now loads, for example w=15u nf=5 with one bin split at 3u. I left this alone. I can send it separately if you want it.

Not verified: I built on Linux only, with autotools. The default build ran make check. A build with --with-ngshared --enable-cider compiled and linked but I did not run it. I did not build with CMake or MSVC. On sky130 I checked nfet_01v8 tt only.

I checked this with help from an AI coding assistant (Claude); I have reviewed the results.

Ananth
```
