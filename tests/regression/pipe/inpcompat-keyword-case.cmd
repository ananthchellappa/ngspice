* The compatibility-mode keywords of the 'ngbehavior' shell variable must be
* recognised whatever their case. set_compat_mode() in src/frontend/inpcompat.c
* picks the modes apart with a chain of case sensitive strstr() calls:
*
*     strstr(behaviour, "hs")  strstr(behaviour, "ps")  strstr(behaviour, "xs")
*     strstr(behaviour, "lt")  strstr(behaviour, "ki")  strstr(behaviour, "a")
*     strstr(behaviour, "ll")  strstr(behaviour, "s3")  strstr(behaviour, "eg")
*     strstr(behaviour, "spe") strstr(behaviour, "mc")
*
* A deck cannot express this. 'ngbehavior' is a shell variable, and the only
* two places it is normally written are spinit and .spiceinit - both of which
* are read as decks and lowercased in place by inp_read() before anything sees
* them. The cp shell input path does not go through that fold, so a value
* typed at the prompt (or piped into 'ngspice -p', or handed to
* ngSpice_Command) keeps its case and is the only way to put an uppercase
* compatibility keyword in front of set_compat_mode(). 'setcs' is used here
* because it is the spelling that is guaranteed never to fold its value.
*
* set_compat_mode() itself is called from inp_readall(), so every check below
* sets 'ngbehavior' first and then builds a fresh circuit with circbyline.
* circbyline lines are deck lines and are folded, which keeps the case of the
* typed 'setcs' value as the only variable in the experiment.
*
* Every check comes in pairs or triples: the lowercase spelling establishes
* what the mode does, and the uppercase spelling must reproduce it. Node and
* device names are unique per block on purpose - when a mode change makes a
* deck unparsable the previous circuit stays current, and a reused name would
* silently answer from the stale circuit.
*
* Every check is fail-fast with an explicit "quit 1". Accumulators are seeded
* with 99 before the value that should overwrite them is read, so a check that
* produced no vector at all fails instead of reading something stale.

set prompt = ""
set filetype = ascii

* --- inpcompat.c:77   strstr(behaviour, "hs")  HSPICE --------------------
* PTpowerH() (src/spicelib/parser/ptfuncs.c) computes a negative base raised
* to a non-integer power as pow(base, round(exp)) in HSPICE mode and as 0
* otherwise, so (v(in)-2)^1.5 is pow(-2,2) = 4 with hs and 0 without it.
setcs ngbehavior = hs
circbyline * hs lowercase control
circbyline vhsl hsl_in 0 dc 0
circbyline bhsl hsl_out 0 v = (v(hsl_in)-2)^1.5
circbyline rhsl hsl_out 0 1k
circbyline .end
op
let hsl = 99
let hsl = v(hsl_out)
if abs(hsl - 4) > 1e-9
  echo "ERROR: lowercase hs control failed, v(hsl_out) = $&hsl expected 4"
  quit 1
end

setcs ngbehavior = HS
circbyline * hs uppercase
circbyline vhsu hsu_in 0 dc 0
circbyline bhsu hsu_out 0 v = (v(hsu_in)-2)^1.5
circbyline rhsu hsu_out 0 1k
circbyline .end
op
let hsu = 99
let hsu = v(hsu_out)
if abs(hsu - 4) > 1e-9
  echo "ERROR: uppercase HS ngbehavior not honoured, v(hsu_out) = $&hsu expected 4"
  quit 1
end

* --- inpcompat.c:79   strstr(behaviour, "ps")  PSPICE --------------------
* PTexp() continues linearly above EXPARGMAX (14) in PSPICE mode, so exp(20)
* is EXPMAX*(20-14+1) = 8.41823e6 with ps and exp(20) = 4.85165e8 without.
setcs ngbehavior = ps
circbyline * ps lowercase control
circbyline vpsl psl_in 0 dc 20
circbyline bpsl psl_out 0 v = exp(v(psl_in))
circbyline rpsl psl_out 0 1k
circbyline .end
op
let psl = 99
let psl = v(psl_out)
if abs(psl / 8.41823e6 - 1) > 1e-4
  echo "ERROR: lowercase ps control failed, v(psl_out) = $&psl expected 8.41823e6"
  quit 1
end

setcs ngbehavior = PS
circbyline * ps uppercase
circbyline vpsu psu_in 0 dc 20
circbyline bpsu psu_out 0 v = exp(v(psu_in))
circbyline rpsu psu_out 0 1k
circbyline .end
op
let psu = 99
let psu = v(psu_out)
if abs(psu / 8.41823e6 - 1) > 1e-4
  echo "ERROR: uppercase PS ngbehavior not honoured, v(psu_out) = $&psu expected 8.41823e6"
  quit 1
end

* --- inpcompat.c:81   strstr(behaviour, "xs")  XSPICE --------------------
* The 8th PULSE parameter is a phase in XSPICE mode (VSRCload() shifts the
* waveform by it and the source keeps repeating) but a pulse count otherwise
* (the source clamps to V1 after one period). At 13ns that is 1 V versus 0 V.
setcs ngbehavior = xs
circbyline * xs lowercase control
circbyline vxsl xsl_in 0 pulse(0 1 0 1n 1n 5n 10n 1)
circbyline rxsl xsl_in 0 1k
circbyline .end
tran 0.1n 20n
let mxsl = 99
meas tran mxsl find v(xsl_in) at=13n
let xsl = 99
let xsl = mxsl
if abs(xsl - 1) > 1e-6
  echo "ERROR: lowercase xs control failed, v(xsl_in) at 13ns = $&xsl expected 1"
  quit 1
end

setcs ngbehavior = XS
circbyline * xs uppercase
circbyline vxsu xsu_in 0 pulse(0 1 0 1n 1n 5n 10n 1)
circbyline rxsu xsu_in 0 1k
circbyline .end
tran 0.1n 20n
let mxsu = 99
meas tran mxsu find v(xsu_in) at=13n
let xsu = 99
let xsu = mxsu
if abs(xsu - 1) > 1e-6
  echo "ERROR: uppercase XS ngbehavior not honoured, v(xsu_in) at 13ns = $&xsu expected 1"
  quit 1
end

* --- inpcompat.c:83   strstr(behaviour, "lt")  LTSPICE ------------------
* INP2R() reads RKM values like 4k7 only in LTSPICE mode; otherwise 4k7 is
* evaluated as 4k. The parsed value is read straight back off the device.
setcs ngbehavior = lt
circbyline * lt lowercase control
circbyline vltl ltl_in 0 dc 1
circbyline rltl ltl_in ltl_out 4k7
circbyline rltl2 ltl_out 0 1k
circbyline .end
op
let ltl = 99
let ltl = @rltl[resistance]
if abs(ltl - 4700) > 1e-9
  echo "ERROR: lowercase lt control failed, @rltl[resistance] = $&ltl expected 4700"
  quit 1
end

setcs ngbehavior = LT
circbyline * lt uppercase
circbyline vltu ltu_in 0 dc 1
circbyline rltu ltu_in ltu_out 4k7
circbyline rltu2 ltu_out 0 1k
circbyline .end
op
let ltu = 99
let ltu = @rltu[resistance]
if abs(ltu - 4700) > 1e-9
  echo "ERROR: uppercase LT ngbehavior not honoured, @rltu[resistance] = $&ltu expected 4700"
  quit 1
end

* --- inpcompat.c:85   strstr(behaviour, "ki")  KiCad --------------------
* inp_fix_gnd_name() replaces a local "/gnd" node by 0 only in KiCad mode.
* With the replacement the 3k leg is shorted out and the divider reads 0.5 V,
* without it "/gnd" stays a real node and the divider reads 0.8 V.
setcs ngbehavior = ki
circbyline * ki lowercase control
circbyline vkil kil_in 0 dc 1
circbyline rkil kil_in kil_mid 1k
circbyline rkil2 kil_mid /gnd 1k
circbyline rkil3 /gnd 0 3k
circbyline .end
op
let kil = 99
let kil = v(kil_mid)
if abs(kil - 0.5) > 1e-9
  echo "ERROR: lowercase ki control failed, v(kil_mid) = $&kil expected 0.5"
  quit 1
end

setcs ngbehavior = KI
circbyline * ki uppercase
circbyline vkiu kiu_in 0 dc 1
circbyline rkiu kiu_in kiu_mid 1k
circbyline rkiu2 kiu_mid /gnd 1k
circbyline rkiu3 /gnd 0 3k
circbyline .end
op
let kiu = 99
let kiu = v(kiu_mid)
if abs(kiu - 0.5) > 1e-9
  echo "ERROR: uppercase KI ngbehavior not honoured, v(kiu_mid) = $&kiu expected 0.5"
  quit 1
end

* --- inpcompat.c:87   strstr(behaviour, "a")   whole netlist -----------
* 'a' only ever acts together with lt or ps: inp_readall() runs
* ltspice_compat_a() over the whole deck when both lt and a are set, and
* limits ltspice_compat() to .include'd cards otherwise. One thing
* ltspice_compat() does is rewrite the LTSPICE resistor flag 'noiseless'
* into 'noisy=0'; without the rewrite 'noiseless' is taken for a model name
* and the deck does not parse at all. So the flag is read back off the
* device, and the seed of 99 covers the unparsable case.
* The letters are concatenated ("lta") because 'setcs ngbehavior = lt a'
* would assign only "lt" and then set a second variable named 'a'.
setcs ngbehavior = lt
circbyline * a: lt without a, control
circbyline vanone anone_in 0 dc 1
circbyline ranone anone_in anone_out 1k noiseless
circbyline ranone2 anone_out 0 1k
circbyline .end
op
let anone = 99
let anone = @ranone[noisy]
if abs(anone - 99) > 1e-9
  echo "ERROR: 'a' control failed, lt alone should not rewrite noiseless, @ranone[noisy] = $&anone"
  quit 1
end

setcs ngbehavior = lta
circbyline * a lowercase control
circbyline valo alo_in 0 dc 1
circbyline ralo alo_in alo_out 1k noiseless
circbyline ralo2 alo_out 0 1k
circbyline .end
op
let alo = 99
let alo = @ralo[noisy]
if abs(alo) > 1e-9
  echo "ERROR: lowercase lta control failed, @ralo[noisy] = $&alo expected 0"
  quit 1
end

setcs ngbehavior = ltA
circbyline * a uppercase
circbyline vaup aup_in 0 dc 1
circbyline raup aup_in aup_out 1k noiseless
circbyline raup2 aup_out 0 1k
circbyline .end
op
let aup = 99
let aup = @raup[noisy]
if abs(aup) > 1e-9
  echo "ERROR: uppercase A in ngbehavior not honoured, @raup[noisy] = $&aup expected 0"
  quit 1
end

* --- inpcompat.c:91   strstr(behaviour, "s3")  spice3 -------------------
* s3 switches the whole compatibility block in inp_readall() off, inp_compat()
* included. inp_compat() is what strips the 'vcvs' keyword out of an
* "Exxx n1 n2 vcvs nc1 nc2 gain" line; left in place the E line has one token
* too many and the deck does not parse. So s3 is visible as the disappearance
* of a value that is definitely there without it - the first block below is
* the positive control that proves the deck itself is good.
unset ngbehavior
circbyline * s3: no mode, control
circbyline vs3n s3n_in 0 dc 1
circbyline es3n s3n_out 0 vcvs s3n_in 0 2
circbyline rs3n s3n_out 0 1k
circbyline .end
op
let s3n = 99
let s3n = v(s3n_out)
if abs(s3n - 2) > 1e-9
  echo "ERROR: s3 control failed, without a mode v(s3n_out) = $&s3n expected 2"
  quit 1
end

* s3 is the one mode whose effect is the disappearance of a value, so each s3
* setting first builds a plain deck that must succeed. Without that companion
* the "no vector" assertion below would be satisfied by any breakage at all.
setcs ngbehavior = s3
circbyline * s3 lowercase, plain deck must still build
circbyline vs3ls s3ls_in 0 dc 1
circbyline rs3ls s3ls_in s3ls_out 1k
circbyline rs3ls2 s3ls_out 0 1k
circbyline .end
op
let s3ls = 99
let s3ls = v(s3ls_out)
if abs(s3ls - 0.5) > 1e-9
  echo "ERROR: s3 lowercase plain deck did not build, v(s3ls_out) = $&s3ls expected 0.5"
  quit 1
end

circbyline * s3 lowercase control
circbyline vs3l s3l_in 0 dc 1
circbyline es3l s3l_out 0 vcvs s3l_in 0 2
circbyline rs3l s3l_out 0 1k
circbyline .end
op
let s3l = 99
let s3l = v(s3l_out)
if abs(s3l - 99) > 1e-9
  echo "ERROR: lowercase s3 control failed, vcvs should not be stripped, v(s3l_out) = $&s3l"
  quit 1
end

setcs ngbehavior = S3
circbyline * s3 uppercase, plain deck must still build
circbyline vs3us s3us_in 0 dc 1
circbyline rs3us s3us_in s3us_out 1k
circbyline rs3us2 s3us_out 0 1k
circbyline .end
op
let s3us = 99
let s3us = v(s3us_out)
if abs(s3us - 0.5) > 1e-9
  echo "ERROR: s3 uppercase plain deck did not build, v(s3us_out) = $&s3us expected 0.5"
  quit 1
end

circbyline * s3 uppercase
circbyline vs3u s3u_in 0 dc 1
circbyline es3u s3u_out 0 vcvs s3u_in 0 2
circbyline rs3u s3u_out 0 1k
circbyline .end
op
let s3u = 99
let s3u = v(s3u_out)
if abs(s3u - 99) > 1e-9
  echo "ERROR: uppercase S3 ngbehavior not honoured, v(s3u_out) = $&s3u expected no vector"
  quit 1
end

* --- inpcompat.c:95   strstr(behaviour, "spe") Spectre ------------------
* Spectre (and HSPICE) mode defaults 'wnflag' to 1, so INPgetModBin() divides
* the instance width by nf before picking the bin. With w=2u and nf=4 that
* selects the 0.5u bin (vth0 = 0.4) instead of the 2u bin (vth0 = 1.4), which
* is a drain current of ~1.4mA instead of ~1uA.
setcs ngbehavior = spe
circbyline * spe lowercase control
circbyline vspelg spel_g 0 dc 1.5
circbyline vspeld spel_d 0 dc 1.0
circbyline mspel spel_d spel_g 0 0 nchspel w=2u l=0.1u nf=4
circbyline .model nchspel.1 nmos level=14 version=4.5 vth0=0.4 lmin=1n lmax=1 wmin=1n wmax=1u
circbyline .model nchspel.2 nmos level=14 version=4.5 vth0=1.4 lmin=1n lmax=1 wmin=1u wmax=10u
circbyline .end
op
let spel = 99
let spel = -i(vspeld)
* the seed of 99 is caught by the upper guard, so a deck that does not build
* fails this check instead of sliding past the 'not too small' test
if spel > 0.1
  echo "ERROR: lowercase spe control produced no current, -i(vspeld) = $&spel"
  quit 1
end
if spel < 1e-4
  echo "ERROR: lowercase spe control failed, drain current = $&spel expected > 1e-4"
  quit 1
end

setcs ngbehavior = SPE
circbyline * spe uppercase
circbyline vspeug speu_g 0 dc 1.5
circbyline vspeud speu_d 0 dc 1.0
circbyline mspeu speu_d speu_g 0 0 nchspeu w=2u l=0.1u nf=4
circbyline .model nchspeu.1 nmos level=14 version=4.5 vth0=0.4 lmin=1n lmax=1 wmin=1n wmax=1u
circbyline .model nchspeu.2 nmos level=14 version=4.5 vth0=1.4 lmin=1n lmax=1 wmin=1u wmax=10u
circbyline .end
op
let speu = 99
let speu = -i(vspeud)
if speu > 0.1
  echo "ERROR: uppercase SPE check produced no current, -i(vspeud) = $&speu"
  quit 1
end
if speu < 1e-4
  echo "ERROR: uppercase SPE ngbehavior not honoured, drain current = $&speu expected > 1e-4"
  quit 1
end

* --- inpcompat.c:99   strstr(behaviour, "mc")  make check ---------------
* 'mc' is the odd one out: it clears every other mode that was matched before
* it. So it is checked by pairing it with lt, whose effect (4k7 = 4700) the
* lt block above already pinned: "ltmc" must undo that and leave 4000.
setcs ngbehavior = ltmc
circbyline * mc lowercase control
circbyline vmcl mcl_in 0 dc 1
circbyline rmcl mcl_in mcl_out 4k7
circbyline rmcl2 mcl_out 0 1k
circbyline .end
op
let mcl = 99
let mcl = @rmcl[resistance]
if abs(mcl - 4000) > 1e-9
  echo "ERROR: lowercase mc control failed, @rmcl[resistance] = $&mcl expected 4000"
  quit 1
end

setcs ngbehavior = ltMC
circbyline * mc uppercase
circbyline vmcu mcu_in 0 dc 1
circbyline rmcu mcu_in mcu_out 4k7
circbyline rmcu2 mcu_out 0 1k
circbyline .end
op
let mcu = 99
let mcu = @rmcu[resistance]
if abs(mcu - 4000) > 1e-9
  echo "ERROR: uppercase MC ngbehavior not honoured, @rmcu[resistance] = $&mcu expected 4000"
  quit 1
end

* --- inpcompat.c:93   strstr(behaviour, "eg")  EAGLE --------------------
* raw_write() names a node vector 'out' instead of 'v(out)' in EAGLE mode.
* Reading the name back needs care, because both spellings resolve in an
* expression. 'unlet' does not: it matches the stored name exactly. So the
* file is written, loaded, and 'unlet v(egX_out)' is attempted - it only bites
* in the non-EAGLE spelling, and whether the vector survived is then a plain
* value question.
*
* This group is deliberately last, and its uppercase check deliberately runs
* before its lowercase control. set_compat_mode() clears hs/ps/xs/lt/ki/a/s3/
* spe/mc on entry but not eg (nor ll), so eg latches: the first deck read with
* eg set leaves it set for the rest of the session. Running the lowercase
* control first would make the uppercase check pass on the latched flag rather
* than on its own spelling.
unset ngbehavior
circbyline * eg: no mode, control
circbyline vegn egn_in 0 dc 1
circbyline regn egn_in egn_out 1k
circbyline regn2 egn_out 0 3k
circbyline .end
op
write ic_eg_no.raw egn_out
destroy all
load ic_eg_no.raw
unlet v(egn_out)
let egn = 99
let egn = egn_out
if abs(egn - 99) > 1e-9
  echo "ERROR: eg control failed, without a mode the vector should be named v(egn_out), got $&egn"
  quit 1
end

setcs ngbehavior = EG
circbyline * eg uppercase
circbyline vegu egu_in 0 dc 1
circbyline regu egu_in egu_out 1k
circbyline regu2 egu_out 0 3k
circbyline .end
op
write ic_eg_up.raw egu_out
destroy all
load ic_eg_up.raw
unlet v(egu_out)
let egu = 99
let egu = egu_out
if abs(egu - 0.75) > 1e-9
  echo "ERROR: uppercase EG ngbehavior not honoured, egu_out = $&egu expected 0.75"
  quit 1
end

setcs ngbehavior = eg
circbyline * eg lowercase control
circbyline vegl egl_in 0 dc 1
circbyline regl egl_in egl_out 1k
circbyline regl2 egl_out 0 3k
circbyline .end
op
write ic_eg_lo.raw egl_out
destroy all
load ic_eg_lo.raw
unlet v(egl_out)
let egl = 99
let egl = egl_out
if abs(egl - 0.75) > 1e-9
  echo "ERROR: lowercase eg control failed, egl_out = $&egl expected 0.75"
  quit 1
end

echo "INFO: all uppercase ngbehavior compatibility keyword cases passed"
quit 0
