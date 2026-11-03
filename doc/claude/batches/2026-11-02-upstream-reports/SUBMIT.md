# Upstream reports: texts ready to paste

**Nothing has been posted.** Nothing was pushed or filed. All of this waits for the owner.

Order:

1. Report 1, sky130: **do not open a new ticket.** Patch ticket #138 already reports it and carries a working fix, so this is a comment on #138.
2. Report 2, binning: new bug ticket.
3. Report 3, `(` in a node name: new bug ticket.
4. Report 4, `.dc @m1[w]` and `scale`: new bug ticket.

Every number below was measured on upstream `pre-master-48` tip `3adcd96a0`, 2026-11-02, or on the build named beside it. Receipt: `receipts/01-confirm.md`. None of the four is fixed at that tip.

Before posting:

- Run the decks yourself once, or decide that the receipt is enough: each text says "I".
- Decide whether to keep the AI sentence, the same choice as on the 2026-10-26 submission.
- Check that #138 is still open and whether anyone has answered, and that no ticket has appeared for 2, 3 or 4. The check here covers what could be read on 2026-11-02; see the receipt, section 6.

Decks are in `decks/`. Paths below are relative to this file.

---

## Report 1: comment on patch ticket #138

Where: https://sourceforge.net/p/ngspice/patches/138/ ("Accept a braced term in .param with agauss", Ethan Sifferman, open). Its other ticket #137 points to it for the same regression.

Attach: `decks/1-brace-sum.cir`, `decks/1-brace-sum-subckt.cir`, `decks/1-sky130-load.cir`

Body:

```
Thanks for this patch. I hit the same failure and had been narrowing it down, so here is independent confirmation and one more case it fixes.

First bad commit, built from git archive on each side with nothing else changed:
- a0999002f (parent of 940bb50fa): sky130 loads, i(vd) = -1.69329e-05
- 940bb50fa: "; sign expected.", exit 1
- 3adcd96a0 (tip today): the same

Extent, counted on my copy of sky130 (models/libs.tech/combined): 128 .param lines have this shape, in 11 parameters_*.spice files, and all 128 hold a statistical function. Each corner includes 36 of them. All 49 .lib sections fail on 3adcd96a0 with 36 errors each, and all 49 load on a0999002f. The first failing line is parameters_fet_tt.spice line 19:
.param sw_polycd = {0.0+corner_factor*0.0} + process_mc_factor*MC_PR_SWITCH*AGAUSS(0,1.5e-09,1)

Without a statistical function this form never worked: .param p = {1+0} + c*3 gives the same error on 45.2 and on a0999002f. These lines only loaded because the .func conversion came first.

One more case, which gives no error. Inside a .subckt the same form loads, and the part after the brace is dropped without a message:
.param c=2
.subckt s n
.param p = {1+0} + c*agauss(3,0,1)
r1 n 0 {p}
.ends
x1 1 s
i1 0 1 1m
print v(1) gives 7.000000e-03 on 45.2 and a0999002f, and 1.000000e-03 on 940bb50fa and 3adcd96a0. I did not trace why. No line in my sky130 copy has this form inside a subckt, so sky130 does not hit it.

I applied only the inpcom.c hunk of your patch to 3adcd96a0:
- All 49 sections load.
- In the 24 sections without mismatch (not _mm, not mc), an nfet_01v8 w=1 l=0.5 at vg=0.9, vd=1 gives the same i(vd) as a0999002f to the 6 digits printed. tt is -1.69329e-05.
- In the 25 _mm and mc sections i(vd) differs, for example tt_mm -1.72783e-05 before and -1.68423e-05 after. I expect that from the changed draws, but I have not checked it statistically.
- The subckt case gives 7.000000e-03.
- Two references to .param p = {0} + agauss(0,1,1) give the same value, -6.671019e-01 twice, which keeps the point of 940bb50fa. a0999002f gives two different values.
- agauss-braces.cir from your patch prints INFO: ok.
I did not run make check with it.

I am not asking for 940bb50fa to go. On the two references in its commit message it does what the manual says, and set stat_param_as_func also loads the tt corner with the same i(vd). I would just like sky130 to keep loading, and your patch does that here.

I checked this with help from an AI coding assistant (Claude); I have reviewed the results.

Ananth
```

---

## Report 2: bug ticket

Where: https://sourceforge.net/p/ngspice/bugs/

Title: `MOS binning since 03ef19244: a size on the outermost lmax or wmax finds no model, and under hsa a multi-finger size finds none`

Attach: `decks/2-outer-max.cir`, `decks/2-hsa-finger.cir`

Body:

```
Version: pre-master-48 3adcd96a0. First bad commit: 03ef19244, built on each side (parent e73cbd263) with nothing else changed.

03ef19244 makes the bin rule min <= size < max, from the thread https://sourceforge.net/p/ngspice/discussion/127605/thread/56ed12d6f3/. For sizes on an inner edge that rule looks right to me, and I am not asking to change it there. Two kinds of legal size now stop the load with "could not find a valid modelname", exit 1.

1. A size on the outermost max, where no bin lies above it.

.model nn.1 nmos level=14 version=4.8.2 lmin=1u lmax=2u wmin=1u wmax=2u
.model nn.2 nmos level=14 version=4.8.2 lmin=2u lmax=20u wmin=1u wmax=2u
v1 d 0 1
m1 d d 0 0 nn l=20u w=1.5u

e73cbd263 and ngspice-45.2: nn.2, v(d) = 1. 03ef19244 and 3adcd96a0: could not find a valid modelname.

2. Under ngbehavior=hsa, a literal multi-finger size whose finger lands on an inner edge. Here w=15 nf=5 is one finger of exactly 3 um, on the edge between the two cards. Needs a .spiceinit with set ngbehavior=hsa.

.option scale=1u
.subckt dut d g s b l=1 w=1 nf=1
mn d g s b card l={l} w={w} nf={nf}
.model card.2 nmos level=54 version=4.5 lmin=1.5e-7 lmax=2.02e-5 wmin=3e-6 wmax=1.01e-3
.model card.1 nmos level=54 version=4.5 lmin=1.5e-7 lmax=2.02e-5 wmin=4.2e-7 wmax=3e-6
.ends dut
vd d 0 0.9
vg g 0 0.9
x1 d g 0 0 dut l=0.5 w=15 nf=5

e73cbd263: x1:card.1. 03ef19244 and 3adcd96a0: could not find a valid modelname. Without hsa the same deck loads on card.2 on all three. As far as I can read it, the hsa pruner in doit() in subckt.c compares in float: scale*w/nf comes out just under the float value of wmin, only the lower card stays, and the new rule rejects that one too.

On sky130 (nfet_01v8), on a build that is 3adcd96a0 plus the inpcom.c hunk of patch #138, because sky130 does not load without it:
- l=20.2 w=2 and l=1 w=1010, without hsa: could not find a valid modelname. a0999002f loads them.
- l=1 w=15 nf=5 and l=1 w=49 nf=7, with hsa: could not find a valid modelname. a0999002f loads them.
a0999002f is older than e73cbd263, so this sky130 comparison spans more than one commit. The commit itself comes from the two decks above.

What I tried, in my own fork, commit 0f763b135: if no bin holds the size, try again with the max included, so an inner edge still goes to the upper bin; and make the hsa pruner apply the same rule as INPgetModBin(), in double. On 3adcd96a0 it fixes both decks: nn.2 and card.2. It is not a clean apply: one hunk of subckt.c needed its context changed by hand, because my fork's line differs, and inpdefs.h needed stdbool.h. Its binning test passes in my fork, but I have not run make check with it on pre-master-48. I can send it as a patch against pre-master-48 if you want it, or you may prefer another way.

I do not know what HSPICE or Spectre do with a size on the outermost max.

I checked this with help from an AI coding assistant (Claude); I have reviewed the results.

Ananth
```

---

## Report 3: bug ticket

Where: https://sourceforge.net/p/ngspice/bugs/

Title: `INPgetNetTok since 58e4cf26b: '(' ends a token, so a node name with '(' changes the circuit or stops the load`

Attach: `decks/3-paren-node-value.cir`, `decks/3-paren-node-stop.cir`, `decks/3-paren-node-gnd.cir`, `decks/3-osdi-model-nospace.cir`, `decks/prange.va`

Body:

```
Version: pre-master-48 3adcd96a0. First bad commit: 58e4cf26b, built on each side (parent 6a10f7af2) with nothing else changed.

58e4cf26b has a real purpose. Its comment says an OSDI card with no space before the paren loses its first parameter, and I confirm that fix: with .model pm prange(R=2) and a 1 V source, i(v1) is -1 on 6a10f7af2 and -0.5 on 58e4cf26b, the same as the spaced card. The commit message names no ticket.

But INPgetNetTok also reads node names on instance cards, so a node with '(' in its name now splits there.

Changes the circuit, exit 0:
v1 x 0 1
r3 a(x) 0 1k
r4 a 0 1k
6a10f7af2 and ngspice-45.2: nodes a, a(x and x, i(v1) = 0.
58e4cf26b and 3adcd96a0: nodes a and x, i(v1) = -5.00000e-04, because r3 now joins a and x. They also print "Warning: Value of resistor r3 is too small, set to 1.000000e-12", yet @r3[resistance] reads 1000 and the current fits 1k, so the warning looks misleading.

Stops, exit 1:
V1 net(0) 0 1
R1 net(0) 0 1k
6a10f7af2 and ngspice-45.2: runs, net(0 = 1. 58e4cf26b and 3adcd96a0: "v1 net(0) 0 1 unknown parameter (1)".

To be fair about the old behaviour: it was not clean either. The node is printed as net(0 with the ')' lost. I also do not know whether '(' in a node name was ever meant to be legal, and these decks are small constructions, not from a real design. If it is not legal, an error would still be better than the case above, which runs with different connectivity.

I have no tested alternative. In my fork I declined this commit because of the cases above, so there the OSDI card with no space still loses its first parameter.

I checked this with help from an AI coding assistant (Claude); I have reviewed the results.

Ananth
```

---

## Report 4: bug ticket

Where: https://sourceforge.net/p/ngspice/bugs/

Title: `.dc @inst[param] since af240ba02: the restore applies .option scale again, so the next op is wrong`

Attach: `decks/4-dc-inst-param-scale.cir`, `decks/4-dc-rebin-sweep.cir`, `decks/4-dc-rebin-op.cir`

Body:

```
Version: pre-master-48 3adcd96a0. First bad commit: af240ba02, which adds the sweep, built on each side (parent 81f5b2c6d) with nothing else changed. Before it the same deck stops the dc with "not in the circuit" and the second op is untouched.

The sweep itself is a useful addition, and its rows are right here. The problem comes after it ends.

.option scale=1e-6
.model nn nmos level=1 vto=0.5 kp=1e-3
vd d 0 1
vg g 0 1
m1 d g 0 0 nn w=1 l=1
.control
op
print i(vd)
print @m1[w]
dc @m1[w] 1 3 1
print i(vd)
print @m1[w]
op
print i(vd)
.endc

Expected: the op after the sweep equals the op before it.
3adcd96a0:
- before the sweep: i(vd) = -1.25000e-04, @m1[w] = 1.000000e-06
- sweep rows: -1.25000e-04, -2.50000e-04, -3.75000e-04
- after the sweep: @m1[w] = 1.000000e-12
- next op: i(vd) = -1.26010e-10
There is no message and the exit code is 0. Without .option scale the op after the sweep is right.

From reading the code, which I have not stepped through: the sweep saves the instance value, already scaled, and restores it through DCTsetInstParam() in dctrcurv.c, which calls DEVparam. mos1par.c multiplies a new w by scale there. The deck shows the result; the path is my reading.

A second point: the sweep does not rebin. With two cards split at w=3u, row w=5u gives -3.21578e-04 from a sweep starting at w=2u, and the same device written at w=5u gives -7.35168e-05. The two cards differ only in vth0, 0.4 and 0.7. The decks are attached.

I have not tested a fix. In my fork I reverted af240ba02 for these two reasons, so I have no patch to offer.

I checked this with help from an AI coding assistant (Claude); I have reviewed the results.

Ananth
```
