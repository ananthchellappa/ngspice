* Plot keywords typed interactively must be recognised whatever their case.
*
* The grid-type and plot-type keywords in src/frontend/plotting/plotit.c are
* read out of the "gridstyle" and "plotstyle" shell variables and compared
* with eq(), and the "vs" separator of the plot command is compared with
* eq() against the name of a zero length vector. None of that can be
* exercised from a deck: inp_read() lowercases every raw deck line before
* the card structure exists, and .control blocks go the same way. The cp
* shell input path does not, so "set gridstyle = LOGLOG" typed at the prompt
* (or fed to "ngspice -p", as here) is the only way to put an uppercase
* keyword in front of those comparisons. "setcs" is the other such route.
*
* The circuit is built with circbyline so the test needs no companion .cir;
* those lines are deck lines and ARE folded, which keeps the case of the
* typed command as the only variable.
*
* Reading the choice back: plotit() has no shell-visible result, so every
* check renders the same data twice with "hardcopy" onto the built in svg
* device and compares a signature of the two files -
*     NR*1000000 + (#<text> lines)*1000 + (#<path> lines)
* - which is a fingerprint of the rendered picture. Each grid/plot style
* draws a visibly different picture, so the signature says which branch of
* plotit() ran. Every signature vector is seeded before it is filled in, and
* the lowercase and uppercase seeds differ (99 vs 98), so a hardcopy that
* produced no file at all leaves a mismatch rather than a silent pass.
*
* Each check is in two halves: the lowercase spelling must differ from the
* signature of an unrecognised value (proof the keyword does something at
* all), and the uppercase spelling must equal the lowercase one.

set prompt = ""
let fail_count = 0

circbyline * inline circuit for the plot keyword checks
circbyline .options noacct
circbyline V1 in 0 dc 1 ac 1
circbyline R1 in out 1k
circbyline C1 out 0 1n
circbyline .end

ac dec 10 1 1meg

* yv is real and strictly positive, so a log axis is legal for it
let yv = vm(out)
* big is complex and well outside the Smith unit circle, which is what makes
* the smith transform of the data visible against a bare smithgrid
let big = v(out) * 50
* a deliberately non-monotonic scale: only 'retraceplot' keeps drawing
* across the reversals
let idx = vector(61)
let xnm = cos(idx / 4)
let ynm = idx
* an alternative scale whose name appears as the x label of the picture
let zscale = frequency * 2

set hcopydevtype = svg

* ---------------------------------------------------------------- grids ---
* An unrecognised gridstyle falls back to a linear grid; that fallback is the
* signature every uppercase spelling is expected NOT to produce.
set gridstyle = notagridtype
hardcopy pkc_a0.svg yv
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_a0.svg`"
let gbase = 96
let gbase = $sg

* --- plotit.c eq(buf, "loglog") ---
set gridstyle = loglog
hardcopy pkc_ll_lo.svg yv
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_ll_lo.svg`"
let siglo = 99
let siglo = $sg
set gridstyle = LOGLOG
hardcopy pkc_ll_up.svg yv
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_ll_up.svg`"
let sigup = 98
let sigup = $sg
if siglo = gbase
   echo "ERROR: control case: lowercase gridstyle=loglog drew the fallback grid"
   let fail_count = fail_count + 1
end
if sigup <> siglo
   echo "ERROR: gridstyle=LOGLOG not matched case insensitively"
   let fail_count = fail_count + 1
end

* --- plotit.c eq(buf, "xlog") ---
set gridstyle = xlog
hardcopy pkc_xl_lo.svg yv
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_xl_lo.svg`"
let siglo = 99
let siglo = $sg
set gridstyle = XLOG
hardcopy pkc_xl_up.svg yv
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_xl_up.svg`"
let sigup = 98
let sigup = $sg
if siglo = gbase
   echo "ERROR: control case: lowercase gridstyle=xlog drew the fallback grid"
   let fail_count = fail_count + 1
end
if sigup <> siglo
   echo "ERROR: gridstyle=XLOG not matched case insensitively"
   let fail_count = fail_count + 1
end

* --- plotit.c eq(buf, "ylog") ---
set gridstyle = ylog
hardcopy pkc_yl_lo.svg yv
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_yl_lo.svg`"
let siglo = 99
let siglo = $sg
set gridstyle = YLOG
hardcopy pkc_yl_up.svg yv
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_yl_up.svg`"
let sigup = 98
let sigup = $sg
if siglo = gbase
   echo "ERROR: control case: lowercase gridstyle=ylog drew the fallback grid"
   let fail_count = fail_count + 1
end
if sigup <> siglo
   echo "ERROR: gridstyle=YLOG not matched case insensitively"
   let fail_count = fail_count + 1
end

* --- plotit.c eq(buf, "polar") ---
set gridstyle = polar
hardcopy pkc_po_lo.svg yv
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_po_lo.svg`"
let siglo = 99
let siglo = $sg
set gridstyle = POLAR
hardcopy pkc_po_up.svg yv
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_po_up.svg`"
let sigup = 98
let sigup = $sg
if siglo = gbase
   echo "ERROR: control case: lowercase gridstyle=polar drew the fallback grid"
   let fail_count = fail_count + 1
end
if sigup <> siglo
   echo "ERROR: gridstyle=POLAR not matched case insensitively"
   let fail_count = fail_count + 1
end

* --- plotit.c eq(buf, "nogrid") ---
set gridstyle = nogrid
hardcopy pkc_ng_lo.svg yv
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_ng_lo.svg`"
let siglo = 99
let siglo = $sg
set gridstyle = NOGRID
hardcopy pkc_ng_up.svg yv
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_ng_up.svg`"
let sigup = 98
let sigup = $sg
if siglo = gbase
   echo "ERROR: control case: lowercase gridstyle=nogrid drew the fallback grid"
   let fail_count = fail_count + 1
end
if sigup <> siglo
   echo "ERROR: gridstyle=NOGRID not matched case insensitively"
   let fail_count = fail_count + 1
end

* ------------------------------------------------------- smith / polar ---
* Smith needs complex data far from the unit circle: GRID_SMITH runs the data
* through the reflection coefficient transform, GRID_SMITHGRID only draws the
* grid, so the two produce different pictures and neither can be confused
* with the other or with the linear fallback.
set gridstyle = notagridtype
hardcopy pkc_b0.svg big
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_b0.svg`"
let sbase = 96
let sbase = $sg

* --- plotit.c eq(buf, "smith") ---
set gridstyle = smith
hardcopy pkc_sm_lo.svg big
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_sm_lo.svg`"
let smlo = 99
let smlo = $sg
set gridstyle = SMITH
hardcopy pkc_sm_up.svg big
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_sm_up.svg`"
let sigup = 98
let sigup = $sg
if smlo = sbase
   echo "ERROR: control case: lowercase gridstyle=smith drew the fallback grid"
   let fail_count = fail_count + 1
end
if sigup <> smlo
   echo "ERROR: gridstyle=SMITH not matched case insensitively"
   let fail_count = fail_count + 1
end

* --- plotit.c eq(buf, "smithgrid") ---
set gridstyle = smithgrid
hardcopy pkc_sg_lo.svg big
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_sg_lo.svg`"
let siglo = 99
let siglo = $sg
set gridstyle = SMITHGRID
hardcopy pkc_sg_up.svg big
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_sg_up.svg`"
let sigup = 98
let sigup = $sg
if siglo = sbase
   echo "ERROR: control case: lowercase gridstyle=smithgrid drew the fallback grid"
   let fail_count = fail_count + 1
end
if siglo = smlo
   echo "ERROR: control case: gridstyle=smithgrid drew the same picture as smith"
   let fail_count = fail_count + 1
end
if sigup <> siglo
   echo "ERROR: gridstyle=SMITHGRID not matched case insensitively"
   let fail_count = fail_count + 1
end

* ----------------------------------------------------------- plot types ---
* The grid is pinned to a recognised value here so that only plotstyle moves.
set gridstyle = lingrid
set plotstyle = notaplotstyle
hardcopy pkc_p0.svg yv
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_p0.svg`"
let pbase = 96
let pbase = $sg

* --- plotit.c eq(buf, "combplot") ---
set plotstyle = combplot
hardcopy pkc_cb_lo.svg yv
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_cb_lo.svg`"
let siglo = 99
let siglo = $sg
set plotstyle = COMBPLOT
hardcopy pkc_cb_up.svg yv
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_cb_up.svg`"
let sigup = 98
let sigup = $sg
if siglo = pbase
   echo "ERROR: control case: lowercase plotstyle=combplot drew the fallback style"
   let fail_count = fail_count + 1
end
if sigup <> siglo
   echo "ERROR: plotstyle=COMBPLOT not matched case insensitively"
   let fail_count = fail_count + 1
end

* --- plotit.c eq(buf, "pointplot") ---
set plotstyle = pointplot
hardcopy pkc_pt_lo.svg yv
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_pt_lo.svg`"
let siglo = 99
let siglo = $sg
set plotstyle = POINTPLOT
hardcopy pkc_pt_up.svg yv
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_pt_up.svg`"
let sigup = 98
let sigup = $sg
if siglo = pbase
   echo "ERROR: control case: lowercase plotstyle=pointplot drew the fallback style"
   let fail_count = fail_count + 1
end
if sigup <> siglo
   echo "ERROR: plotstyle=POINTPLOT not matched case insensitively"
   let fail_count = fail_count + 1
end

* --- plotit.c eq(buf, "retraceplot") ---
* retraceplot only shows up on a scale that reverses direction: with any
* other plot type the reversals break the curve into separate pieces.
set plotstyle = notaplotstyle
hardcopy pkc_c0.svg ynm vs xnm
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_c0.svg`"
let rbase = 96
let rbase = $sg
set plotstyle = retraceplot
hardcopy pkc_rt_lo.svg ynm vs xnm
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_rt_lo.svg`"
let siglo = 99
let siglo = $sg
set plotstyle = RETRACEPLOT
hardcopy pkc_rt_up.svg ynm vs xnm
set sg = "`awk '/<path/{p++} /<text/{t++} END{print NR*1000000+t*1000+p+0}' pkc_rt_up.svg`"
let sigup = 98
let sigup = $sg
if siglo = rbase
   echo "ERROR: control case: lowercase plotstyle=retraceplot drew the fallback style"
   let fail_count = fail_count + 1
end
if sigup <> siglo
   echo "ERROR: plotstyle=RETRACEPLOT not matched case insensitively"
   let fail_count = fail_count + 1
end

* ------------------------------------------------------ the vs separator ---
* "plot a vs b" is recognised by comparing the name of a zero length vector
* with "vs". When it is honoured the x axis is labelled with the name of the
* scale vector, so counting the label in the picture reads the decision back.
set plotstyle = linplot
hardcopy pkc_v0.svg yv
set sg = "`grep -cx zscale pkc_v0.svg`"
let vbase = 96
let vbase = $sg
hardcopy pkc_vs_lo.svg yv vs zscale
set sg = "`grep -cx zscale pkc_vs_lo.svg`"
let siglo = 99
let siglo = $sg
hardcopy pkc_vs_up.svg yv VS zscale
set sg = "`grep -cx zscale pkc_vs_up.svg`"
let sigup = 98
let sigup = $sg
if siglo = vbase
   echo "ERROR: control case: lowercase vs did not change the scale vector"
   let fail_count = fail_count + 1
end
if sigup <> siglo
   echo "ERROR: the plot separator VS not matched case insensitively"
   let fail_count = fail_count + 1
end

shell rm -f pkc_a0.svg pkc_ll_lo.svg pkc_ll_up.svg pkc_xl_lo.svg pkc_xl_up.svg
shell rm -f pkc_yl_lo.svg pkc_yl_up.svg pkc_po_lo.svg pkc_po_up.svg pkc_ng_lo.svg
shell rm -f pkc_ng_up.svg pkc_b0.svg pkc_sm_lo.svg pkc_sm_up.svg pkc_sg_lo.svg
shell rm -f pkc_sg_up.svg pkc_p0.svg pkc_cb_lo.svg pkc_cb_up.svg pkc_pt_lo.svg
shell rm -f pkc_pt_up.svg pkc_c0.svg pkc_rt_lo.svg pkc_rt_up.svg pkc_v0.svg
shell rm -f pkc_vs_lo.svg pkc_vs_up.svg

if fail_count > 0
  echo "ERROR: $&fail_count uppercase plot keyword cases failed"
  quit 1
else
  echo "INFO: all uppercase plot keyword cases passed"
  quit 0
end
