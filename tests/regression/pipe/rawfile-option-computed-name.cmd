* a rawfile's Option: line cannot shadow a variable the session computes
* doc/codex/issues/0065, acceptance criteria 1, 2, 4 and 7.
*
* cp_enqvar() (src/frontend/options.c) searched the current plot's
* environment before the arms that compute curplot, curplotname,
* curplottitle, curplotdate and plots, and cp_usrvars() asks it for exactly
* those five names and writes va_next into whatever comes back.  A raw
* header's 'Option:' line files an arbitrary name=value pair into that
* environment (src/frontend/rawfile.c, doc/codex/issues/0061), so a file
* could hand a borrowed node to a caller that frees it: 'load' alone was a
* SIGSEGV, on this build and on stock ngspice-46 alike.
*
* The assertion has to be a marker reached after the load and an exit code,
* not the absence of a message: the process died silently and this directory
* compares no output.  Each check is fail-fast with an explicit 'quit 1'.
*
* The rawfiles are hand built with echo and its redirect, as
* tests/regression/casedist/vector-rawfile-scale-case.cir does, because no
* ngspice writer emits an 'Option:' line unless it is round-tripping one it
* read.  'SHADOW' is the value throughout, and it appears nowhere else in
* any of these files -- not in a title, a plot name or a date -- so the one
* scan at the end can say that no computed read answered with the file's
* string.
*
* Mode independent: no name here is looked up across a case boundary, and
* the containment is that the environment scan is byte exact, so only the
* lower case spelling ever shadowed.

set prompt = ""

* --- 1. one load per computed name -----------------------------------------
* Each of these five was a fault inside 'load' itself, because com_load()
* calls com_display() (src/frontend/postcoms.c), which reads two variables
* in a row: the first read freed the plot's own node and the second walked
* the environment that still pointed at it.  Reaching the marker below is
* the whole of this check.
foreach nm curplot curplotname curplottitle curplotdate plots
  echo "Title: hand built for doc/codex/issues/0065" > rcn_one.raw
  echo "Date: Tue Aug 11 00:00:00  2026" >> rcn_one.raw
  echo "Plotname: Operating Point" >> rcn_one.raw
  echo "Option: $nm=SHADOW" >> rcn_one.raw
  echo "Flags: real" >> rcn_one.raw
  echo "No. Variables: 2" >> rcn_one.raw
  echo "No. Points: 1" >> rcn_one.raw
  echo "Variables:" >> rcn_one.raw
  echo " 0 v(1) voltage" >> rcn_one.raw
  echo " 1 OUT voltage" >> rcn_one.raw
  echo "Values:" >> rcn_one.raw
  echo " 0 3.0" >> rcn_one.raw
  echo "1.0" >> rcn_one.raw
  load rcn_one.raw
end
echo "INFO: the five poisoned loads survived"

* --- 2. and the computed answer is the one that comes back ------------------
* One rawfile naming all five at once, so that the reads which follow are a
* single capture.  The five values are echoed to a file rather than compared
* with strcmp because four of them are several words long.
echo "Title: hand built for doc/codex/issues/0065" > rcn_all.raw
echo "Date: Tue Aug 11 00:00:00  2026" >> rcn_all.raw
echo "Plotname: Operating Point" >> rcn_all.raw
echo "Option: curplot=SHADOW curplotname=SHADOW curplottitle=SHADOW curplotdate=SHADOW plots=SHADOW" >> rcn_all.raw
echo "Flags: real" >> rcn_all.raw
echo "No. Variables: 2" >> rcn_all.raw
echo "No. Points: 1" >> rcn_all.raw
echo "Variables:" >> rcn_all.raw
echo " 0 v(1) voltage" >> rcn_all.raw
echo " 1 OUT voltage" >> rcn_all.raw
echo "Values:" >> rcn_all.raw
echo " 0 3.0" >> rcn_all.raw
echo "1.0" >> rcn_all.raw
load rcn_all.raw
echo $curplot > rcn_cap.txt
echo $curplotname >> rcn_cap.txt
echo $curplottitle >> rcn_cap.txt
echo $curplotdate >> rcn_cap.txt
echo $plots >> rcn_cap.txt

* --- 3. the poison is per plot, so it also fires after the load -------------
* A two plot rawfile whose FIRST plot carries the line.  The load leaves the
* clean plot current and returns 0 even unrepaired; it is the setplot back
* onto the poisoned plot, and then any command that reads two variables,
* that faulted.  'tran1' is deterministic here because this is the only
* transient plot the deck creates.
echo "Title: hand built for doc/codex/issues/0065" > rcn_two.raw
echo "Date: Tue Aug 11 00:00:00  2026" >> rcn_two.raw
echo "Plotname: Transient Analysis" >> rcn_two.raw
echo "Option: curplot=SHADOW" >> rcn_two.raw
echo "Flags: real" >> rcn_two.raw
echo "No. Variables: 2" >> rcn_two.raw
echo "No. Points: 1" >> rcn_two.raw
echo "Variables:" >> rcn_two.raw
echo " 0 time time" >> rcn_two.raw
echo " 1 OUT voltage" >> rcn_two.raw
echo "Values:" >> rcn_two.raw
echo " 0 0.0" >> rcn_two.raw
echo "1.0" >> rcn_two.raw
echo "Title: hand built for doc/codex/issues/0065" >> rcn_two.raw
echo "Date: Tue Aug 11 00:00:00  2026" >> rcn_two.raw
echo "Plotname: Operating Point" >> rcn_two.raw
echo "Flags: real" >> rcn_two.raw
echo "No. Variables: 2" >> rcn_two.raw
echo "No. Points: 1" >> rcn_two.raw
echo "Variables:" >> rcn_two.raw
echo " 0 v(1) voltage" >> rcn_two.raw
echo " 1 OUT voltage" >> rcn_two.raw
echo "Values:" >> rcn_two.raw
echo " 0 3.0" >> rcn_two.raw
echo "1.0" >> rcn_two.raw
load rcn_two.raw
setplot tran1
display
print OUT
set filetype = ascii
write rcn_round.raw
echo "INFO: display, print and write on the poisoned plot survived"
echo $curplot >> rcn_cap.txt

* --- 4. no computed read answered with the file's string --------------------
* The lines are counted as well as scanned, so that a capture which never
* arrived -- an fopen that failed, a redirect that wrote nothing -- fails
* here instead of passing for free with nothing to find.  Six: the five
* reads of check 2 and the one of check 3.
set found = 0
let nlines = 0
set n1 = 1
fopen f1 rcn_cap.txt r
while $n1 >= 0
  set capline = "no_line_read"
  set p1 = -1
  fread capline $f1 n1
  if $n1 >= 0
    let nlines = nlines + 1
  end
  strstr p1 "_$capline" "SHADOW"
  if $p1 >= 0
    set found = 1
  end
end
fclose $f1
if nlines < 6
  echo "ERROR: harness broken, the capture holds $&nlines lines and not six"
  quit 1
end
if $found > 0
  echo "ERROR: a rawfile Option: key answered a read of a computed variable"
  quit 1
end

echo "INFO: every computed variable answered for itself"
quit 0
