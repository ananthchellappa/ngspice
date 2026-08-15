* a plot derived from a loaded one is not stamped with the deriving
* session's mode either
* doc/codex/issues/0070, the copy paths its resolution left to be settled by
* measurement.
*
* raw_write() (src/frontend/rawfile.c) records the case mode in force only
* for a plot this session produced, and 'produced' has to survive the
* commands that make a new plot out of an old one.  'linearize' and 'cutout'
* (src/frontend/linear.c), 'fft' and 'psd' (src/frontend/com_fft.c) and
* 'spec' (src/frontend/spec.c) each allocate a fresh plot and fill it with
* vectors named after the ones they were given.  The names are therefore
* spelled the way the source spelled them, and a source that came out of a
* file was spelled by a run this session never saw -- so the derived plot is
* described by that run's mode and not by this session's, exactly as the
* plot it came from is.
*
* Both directions are asserted, because a build that simply stopped writing
* the line for derived plots would pass the first half for the wrong reason:
* check 3 derives from a foreign plot and must claim nothing, check 4
* derives from a plot this session simulated and must claim 'fold'.
*
* This directory compares no output and asserts by exit code, so every check
* is fail-fast with an explicit 'quit 1'.  The strcmp and strstr result
* variables are preseeded, so a check whose comparison never ran fails
* rather than passing on a stale zero.

set prompt = ""
set filetype = ascii

* --- 0. a transient run under preserve, with the gate shut ---------------
let n_req_unset = $?casemode
if n_req_unset <> 0
  echo "ERROR: harness broken, casemode is already set at session start"
  quit 1
end
let n_gate_unset = $?casemodewrite
if n_gate_unset <> 0
  echo "ERROR: harness broken, casemodewrite is already set at session start"
  quit 1
end
echo "CaseDerivedProbe" > rcd_inner.cir
echo ".options noacct" >> rcd_inner.cir
echo "vs In 0 pulse(0 1 0 1n 1n 5u 10u)" >> rcd_inner.cir
echo "rl In MidNode 1k" >> rcd_inner.cir
echo "cg MidNode 0 1n" >> rcd_inner.cir
echo ".control" >> rcd_inner.cir
echo "tran 0.2u 20u" >> rcd_inner.cir
echo ".endc" >> rcd_inner.cir
echo ".end" >> rcd_inner.cir
set casemode = preserve
source rcd_inner.cir
set b0 = 99
strcmp b0 "$curcasemode" "preserve"
if $b0 <> 0
  echo "ERROR: harness broken, a source did not latch preserve"
  quit 1
end
write rcd_old.raw

* --- 1. a folding session loads it and asks for the line -----------------
unset casemode
source rcd_inner.cir
set b1 = 99
strcmp b1 "$curcasemode" "fold"
if $b1 <> 0
  echo "ERROR: harness broken, the session did not go back to folding"
  quit 1
end
set casemodewrite
load rcd_old.raw >& rcd_load.txt
set lp = "$curplot"

* the loaded plot is spelled the preserving way: the control that it really
* is foreign, and that the names below are not this session's
display >& rcd_disp.txt
set found1 = 0
set n1 = 1
fopen f1 rcd_disp.txt r
while $n1 >= 0
  set capline = "no_line_read"
  set p1 = -1
  fread capline $f1 n1
  strstr p1 "_$capline" "MidNode"
  if $p1 >= 0
    set found1 = 1
  end
end
fclose $f1
if $found1 <> 1
  echo "ERROR: harness broken, the loaded plot is not spelled the preserved way"
  quit 1
end

* --- 2. linearize it: a new plot, out of foreign names -------------------
linearize
set dp = "$curplot"
set b2 = 99
strcmp b2 "$dp" "$lp"
if $b2 = 0
  echo "ERROR: harness broken, linearize did not make a new plot current"
  quit 1
end

* --- 3. and the derived plot claims no mode ------------------------------
write rcd_lin.raw
let nhits = 0
set n3 = 1
set hitline = "no_hit_line"
fopen f3 rcd_lin.raw r
while $n3 >= 0
  set capline = "no_line_read"
  set p3 = -1
  fread capline $f3 n3
  strstr p3 "_$capline" "casemode"
  if $p3 >= 0
    let nhits = nhits + 1
    set hitline = "$capline"
  end
end
fclose $f3
if nhits <> 0
  echo "ERROR: a plot linearized from a loaded one carries $&nhits casemode lines, last <$hitline>"
  quit 1
end

* --- 3b. and so does a spectrum taken off it ------------------------------
* 'fft' allocates its own plot and does not copy the one it read, so it is a
* separate path from linearize's: it takes the answer from the vectors it
* was handed (src/frontend/com_fft.c).  The names it gives its output are
* the input names, which here are still the preserving run's.
fft v(MidNode)
write rcd_fft.raw
let nfft = 0
set n3b = 1
set fftline = "no_hit_line"
fopen f3b rcd_fft.raw r
while $n3b >= 0
  set capline = "no_line_read"
  set p3b = -1
  fread capline $f3b n3b
  strstr p3b "_$capline" "casemode"
  if $p3b >= 0
    let nfft = nfft + 1
    set fftline = "$capline"
  end
end
fclose $f3b
if nfft <> 0
  echo "ERROR: an fft of loaded data carries $&nfft casemode lines, last <$fftline>"
  quit 1
end

* --- 4. the control: a plot derived from this session's own run does -----
source rcd_inner.cir
linearize
write rcd_own.raw
let nown = 0
set n4 = 1
set ownline = "no_hit_line"
fopen f4 rcd_own.raw r
while $n4 >= 0
  set capline = "no_line_read"
  set p4 = -1
  fread capline $f4 n4
  strstr p4 "_$capline" "casemode"
  if $p4 >= 0
    let nown = nown + 1
    set ownline = "$capline"
  end
end
fclose $f4
if nown <> 1
  echo "ERROR: harness broken, a plot linearized from this session's own run carries $&nown casemode lines"
  quit 1
end
set b4 = 99
strcmp b4 "$ownline" "Option: casemode=fold"
if $b4 <> 0
  echo "ERROR: a plot linearized from this session's own run records <$ownline>"
  quit 1
end

* --- 4b. and so does a spectrum taken off this session's own run ---------
fft v(midnode)
write rcd_ownfft.raw
let nownfft = 0
set n4b = 1
set ownfftline = "no_hit_line"
fopen f4b rcd_ownfft.raw r
while $n4b >= 0
  set capline = "no_line_read"
  set p4b = -1
  fread capline $f4b n4b
  strstr p4b "_$capline" "casemode"
  if $p4b >= 0
    let nownfft = nownfft + 1
    set ownfftline = "$capline"
  end
end
fclose $f4b
if nownfft <> 1
  echo "ERROR: harness broken, an fft of this session's own run carries $&nownfft casemode lines"
  quit 1
end
set b4b = 99
strcmp b4b "$ownfftline" "Option: casemode=fold"
if $b4b <> 0
  echo "ERROR: an fft of this session's own run records <$ownfftline>"
  quit 1
end

echo "INFO: a plot derived from a loaded one inherits the loaded plot's silence"
quit 0
