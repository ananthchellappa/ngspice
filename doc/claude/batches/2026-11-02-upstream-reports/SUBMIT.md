# Upstream reports: texts ready to paste

**Nothing has been posted.** Nothing was pushed or filed. All of this waits for the owner.

**Report 3 as a bug does not hold, and is withdrawn.** The upstream manual says `(` may not appear in a node name (2.1.3.4) and that a parenthesis separates fields (2.1.3.1). So `58e4cf26b` makes the parser follow the documented rule. What is left is an optional forum note below, which only asks whether a message is wanted. You can skip it without losing a bug report.

**Report 2 changed.** It now leads with the hsa multi-finger failure, which breaks the commit's own rule. The outermost-max case is now a question, not a claim of a legal size. The thread states the rule with no exception for the top bin, so upstream may say the error is right.

**Two decisions for you, in your fork. Nothing here changes them.**

- You declined `58e4cf26b` because of the `(` cases. The manual undercuts those grounds, and declining it leaves the OSDI no-space loss in place: with `.model pm prange(R=2)` the first parameter is lost.
- The fork's `0f763b135` calls a size on the top edge "legal" in a code comment and keeps it loading. Upstream may rule the other way.

Order:

1. Report 1, sky130: **do not open a new ticket.** Patch ticket #138 already reports it and carries a working fix, so this is a comment on #138.
2. Report 2, binning: new bug ticket.
3. Report 4, `.dc @m1[w]` and `scale`: new bug ticket.
4. Note 3, `(` in a node name: optional forum post, not a ticket.

Every number below was measured on upstream `pre-master-48` tip `3adcd96a0`, 2026-11-02, or on the build named beside it. That is still the tip by `git ls-remote` at the time of the correction. Receipt: `receipts/01-confirm.md`. Reports 1, 2 and 4 are not fixed at that tip.

Before posting:

- Run the decks yourself once, or decide that the receipt is enough: each text says "I".
- Decide whether to keep the AI sentence, the same choice as on the 2026-10-26 submission.
- Check that #138 is still open and whether anyone has answered, and that no ticket has appeared for 2 or 4. The check here covers what could be read on 2026-11-02; see the receipt, section 6.

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

Title: `MOS binning under hsa since 03ef19244: a multi-finger size on an inner bin edge finds no model, because the subckt.c pruner compares in float`

Attach: `decks/2-hsa-finger.cir`, `decks/2-outer-max.cir`, `decks/2-outer-max-hsa.cir`

Body:

```
Version: pre-master-48 3adcd96a0. The load stop starts at 03ef19244, built on each side (parent e73cbd263) with nothing else changed. The float comparison below is older; 03ef19244 is what turns it into an error.

03ef19244 makes the bin rule min <= size < max, from the thread https://sourceforge.net/p/ngspice/discussion/127605/thread/56ed12d6f3/. I am not asking to change that rule. Under ngbehavior=hsa a size that the rule puts in a bin now stops the load with "could not find a valid modelname", exit 1.

Here w=15 nf=5 is one finger of exactly 3 um. Under the rule it belongs in card.2. Needs a .spiceinit with set ngbehavior=hsa.

.option scale=1u
.subckt dut d g s b l=1 w=1 nf=1
mn d g s b card l={l} w={w} nf={nf}
.model card.2 nmos level=54 version=4.5 lmin=1.5e-7 lmax=2.02e-5 wmin=3e-6 wmax=1.01e-3
.model card.1 nmos level=54 version=4.5 lmin=1.5e-7 lmax=2.02e-5 wmin=4.2e-7 wmax=3e-6
.ends dut
vd d 0 0.9
vg g 0 0.9
x1 d g 0 0 dut l=0.5 w=15 nf=5

e73cbd263: loads on x1:card.1, the lower card. 03ef19244 and 3adcd96a0: could not find a valid modelname. The same happens with no .option scale and w=15u. Without hsa the deck loads on card.2 on all three, but there w is not divided by nf, so that run does not reach the edge.

The cause, from the code and a small C program using the same expressions: the hsa pruner in doit() in subckt.c compares in float. (float)scale * w / nf is 2.9999998787e-06 and (float)3e-6 is 3.0000001061e-06. So the pruner removes card.2 and keeps card.1. INPgetModBin() then works in double with is_equal() and rightly rejects card.1, and no card is left.

What I tried, on 3adcd96a0: the pruner compares in double and uses the same rule as INPgetModBin(), and w, l, nf in struct card become double. That was all. The deck then loads on x1:card.2.

On sky130 (nfet_01v8, tt, hsa), on a build that is 3adcd96a0 plus the inpcom.c hunk of patch #138, because sky130 does not load without it:
- l=1 w=15 nf=5 and l=1 w=49 nf=7: could not find a valid modelname.
- With the pruner change as well, both load with the i(vd) of a0999002f to the 6 digits printed: -1.86245e-04 and -6.37262e-04. For w=15 nf=5, a0999002f used nshort_model.28 (wmax=3E-6) and this build uses nshort_model.20 (wmin=3E-6).
a0999002f is older than e73cbd263, so this sky130 comparison spans more than one commit. I have not run make check with the change. I can send it as a patch against pre-master-48 if you want it.

A separate question, not a claim. A size exactly on the outermost max, with no bin above it, is also rejected now:

.model nn.1 nmos level=14 version=4.8.2 lmin=1u lmax=2u wmin=1u wmax=2u
.model nn.2 nmos level=14 version=4.8.2 lmin=2u lmax=20u wmin=1u wmax=2u
v1 d 0 1
m1 d d 0 0 nn l=20u w=1.5u

e73cbd263 and ngspice-45.2: nn.2. 03ef19244 and 3adcd96a0: could not find a valid modelname. The thread states the rule with no exception for the top bin, and the TSMC limits there have WMAX = 0.0009000001, just above a round value. Inside a subckt under hsa the pruner already rejected this on e73cbd263. So the error may be what you intend. On sky130, without hsa, l=20.2 w=2 and l=1 w=1010 load on a0999002f and not on the build above. Should the outermost edge stay rejected? I have not checked it on HSPICE or Spectre. The pruner change above does not alter this case.

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

---

## Note 3: optional forum post, not a bug ticket

Where: https://sourceforge.net/p/ngspice/discussion/127605/

Subject: `Node names with '(' since 58e4cf26b: would a message be wanted?`

Attach: `decks/3-paren-node-value.cir`, `decks/3-paren-node-stop.cir`, `decks/3-plain-two-values.cir`

Body:

```
This is a question, not a bug report. 58e4cf26b makes '(' end a token in INPgetNetTok(). That follows the manual: 2.1.3.1 says "Fields on a line are separated by one or more blanks, a comma, an equal (=) sign, or a left or right parenthesis", and 2.1.3.4 lists ( and ) among the characters not allowed in a node name. It also fixes the OSDI card with no space before the paren: with .model pm prange(R=2) and a 1 V source, i(v1) is -1 on 6a10f7af2 and -0.5 on 58e4cf26b, the same as the spaced card.

So the decks below are invalid, and the new reading of them is the documented one. The only thing I want to raise is what happens to old decks that broke the rule. They used to run with the name cut short, and one shape now runs with different connectivity.

v1 x 0 1
r3 a(x) 0 1k
r4 a 0 1k
6a10f7af2 and ngspice-45.2: nodes a, a(x and x, i(v1) = 0.
58e4cf26b and 3adcd96a0: read as r3 a x 0 1k, nodes a and x, i(v1) = -5.00000e-04, exit 0.

The other shape does stop:
V1 net(0) 0 1
R1 net(0) 0 1k
6a10f7af2 and ngspice-45.2: runs, net(0 = 1. 58e4cf26b and 3adcd96a0: "v1 net(0) 0 1 unknown parameter (1)", exit 1.

Would you want a message for a parenthesis inside a node name, or is an invalid name the user's own problem? I have no patch, and these decks are small constructions, not from a real design. Spaced parentheses such as r3 (a x) 1k give the same result on 45.2 and 3adcd96a0, so a message would have to leave those alone.

A smaller point, older than 58e4cf26b and independent of it. The first deck also prints "Warning: Value of resistor r3 is too small, set to 1.000000e-12". The plain card r3 a x 0 1k prints the same on 45.2, 6a10f7af2 and 3adcd96a0, with two numbers on the card, yet @r3[resistance] reads 1000 and the current fits 1k. The warning does not describe what is simulated.

I checked this with help from an AI coding assistant (Claude); I have reviewed the results.

Ananth
```
