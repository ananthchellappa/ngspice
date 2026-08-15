* a plot loaded from a file that recorded no mode is not stamped with the
* reading session's mode
* doc/codex/issues/0070.
*
* With 'casemodewrite' set, raw_write() (src/frontend/rawfile.c) records the
* mode in force under the 'Plotname:' line -- but only for a plot this
* session produced, because what a header records is a property of the plot
* and the session's mode describes nobody else's data.  The provenance test
* used to be an empty 'pl_env': an environment is filed by raw_read() and by
* nothing else, so a plot that HAS one came out of a file.  The converse is
* what the writer needed and it is false.  A file with no 'Option:' lines
* leaves the loaded plot's environment empty, and an empty environment is
* what a plot this session simulated has too, so the two origins were
* indistinguishable at that test.
*
* Every raw file every released ngspice has written has no 'Option:' lines,
* and so does every file this build writes with the gate unset, which is the
* default.  The mis-identified case is therefore not a corner: it is
* essentially every raw file that exists.  Loading one and writing it out
* stamped the copy with the copying session's mode, and under 'fold' that
* claim is not merely unsupported but self-contradictory -- the file says a
* folding run produced names that no folding run can produce.
*
* The file under test here is written under 'preserve' with the gate SHUT,
* so it carries no 'Option:' line of any kind: that is the premise, and
* check 0b asserts it by scanning the whole file rather than one line,
* because the claim is about the file and not about a position in it.  The
* session that then loads it is a folding one, so a wrong answer is
* distinguishable from a right one -- the two modes name themselves
* differently, and the loaded plot's own spelling (check 2) is the control
* that the plot really is foreign.
*
* Check 4 is the same claim after the plot has stopped being the one the
* load left current: another netlist is read, the loaded plot is selected
* again by name and renamed, and only then written.  A provenance record
* that lives anywhere but the plot -- a flag on the last load, say -- passes
* check 3 and fails here.
*
* This directory compares no output and asserts by exit code, so every check
* is fail-fast with an explicit 'quit 1'.  The strcmp and strstr result
* variables are preseeded, so a check whose comparison never ran -- the
* shape a failed substitution produces, since the dropped word leaves the
* command with too few arguments -- fails rather than passing on a stale
* zero.

set prompt = ""
set filetype = ascii

* --- 0. a preserve run of a mixed-case deck, with the gate shut -----------
* The nets are mixed case so that the file's own spelling is evidence: under
* preserve they stay In and MidNode, which a folding read cannot produce.
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
echo "CaseForeignProbe" > rcf_inner.cir
echo ".options noacct" >> rcf_inner.cir
echo "vs In 0 dc 3" >> rcf_inner.cir
echo "rl In MidNode 1k" >> rcf_inner.cir
echo "rg MidNode 0 3k" >> rcf_inner.cir
echo ".control" >> rcf_inner.cir
echo "op" >> rcf_inner.cir
echo ".endc" >> rcf_inner.cir
echo ".end" >> rcf_inner.cir
set casemode = preserve
source rcf_inner.cir
set b0 = 99
strcmp b0 "$curcasemode" "preserve"
if $b0 <> 0
  echo "ERROR: harness broken, a source did not latch preserve"
  quit 1
end
write rcf_old.raw

* --- 0b. and the file it wrote says nothing at all about any option -------
* This is the premise of the whole deck, so it is measured and not assumed:
* the file is the file every ngspice has always written.
let nopt = 0
set n0 = 1
set optline = "no_hit_line"
fopen f0 rcf_old.raw r
while $n0 >= 0
  set capline = "no_line_read"
  set p0 = -1
  fread capline $f0 n0
  strstr p0 "_$capline" "Option:"
  if $p0 >= 0
    let nopt = nopt + 1
    set optline = "$capline"
  end
end
fclose $f0
if nopt <> 0
  echo "ERROR: harness broken, the gate-shut file carries $&nopt Option: lines, last <$optline>"
  quit 1
end

* --- 1. the session is now a folding one ----------------------------------
* The request is dropped and the deck re-read, because a source is what
* re-latches the mode (doc/codex/issues/0058).
unset casemode
source rcf_inner.cir
set b1 = 99
strcmp b1 "$curcasemode" "fold"
if $b1 <> 0
  echo "ERROR: harness broken, the session did not go back to folding"
  quit 1
end
set own = "$curplot"

* --- 2. it loads the old file, and asks for the line ----------------------
set casemodewrite
load rcf_old.raw >& rcf_load.txt
set lp = "$curplot"
set b2 = 99
strcmp b2 "$lp" "$own"
if $b2 = 0
  echo "ERROR: harness broken, the load left this session's own plot current"
  quit 1
end
* the loaded plot really is foreign: it is spelled the preserving way, which
* the session doing the writing below cannot produce
display >& rcf_disp.txt
set found2 = 0
set n2 = 1
fopen f2 rcf_disp.txt r
while $n2 >= 0
  set capline = "no_line_read"
  set p2 = -1
  fread capline $f2 n2
  strstr p2 "_$capline" "MidNode"
  if $p2 >= 0
    set found2 = 1
  end
end
fclose $f2
if $found2 <> 1
  echo "ERROR: harness broken, the loaded plot is not spelled the preserved way"
  quit 1
end
* and the file told it no mode, so nothing but the writer can invent one
let n_ans2 = $?casemode
if n_ans2 <> 0
  echo "ERROR: harness broken, the loaded plot answers a casemode it cannot have"
  quit 1
end

* --- 3. writing the loaded plot out claims nothing about its mode ---------
write rcf_copy.raw
let nhits = 0
set n3 = 1
set hitline = "no_hit_line"
fopen f3 rcf_copy.raw r
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
  echo "ERROR: the copy of a file that recorded no mode carries $&nhits casemode lines, last <$hitline>"
  quit 1
end

* --- 4. and it still claims nothing after the plot has been left ----------
* Another netlist is read, so the loaded plot stops being current; then it
* is selected again by name, renamed, and written from a later command.
source rcf_inner.cir
set b4 = 99
strcmp b4 "$curplot" "$lp"
if $b4 = 0
  echo "ERROR: harness broken, the second source did not make its own plot current"
  quit 1
end
setplot $lp
set c4 = 99
strcmp c4 "$curplot" "$lp"
if $c4 <> 0
  echo "ERROR: harness broken, setplot did not go back to the loaded plot"
  quit 1
end
set curplotname = "RenamedForeignPlot"
set d4 = 99
strcmp d4 "$curplotname" "RenamedForeignPlot"
if $d4 <> 0
  echo "ERROR: harness broken, the loaded plot was not renamed"
  quit 1
end
write rcf_late.raw
let nlate = 0
set n4 = 1
set lateline = "no_hit_line"
fopen f4 rcf_late.raw r
while $n4 >= 0
  set capline = "no_line_read"
  set p4 = -1
  fread capline $f4 n4
  strstr p4 "_$capline" "casemode"
  if $p4 >= 0
    let nlate = nlate + 1
    set lateline = "$capline"
  end
end
fclose $f4
if nlate <> 0
  echo "ERROR: a renamed foreign plot written later carries $&nlate casemode lines, last <$lateline>"
  quit 1
end

* --- 5. the control: this session's own plot still gets the line ----------
* Without it, a build that simply stopped writing the line would pass every
* check above.
source rcf_inner.cir
set e5 = 99
strcmp e5 "$curplot" "$lp"
if $e5 = 0
  echo "ERROR: harness broken, the third source did not make its own plot current"
  quit 1
end
write rcf_own.raw
let nown = 0
set n5 = 1
set ownline = "no_hit_line"
fopen f5 rcf_own.raw r
while $n5 >= 0
  set capline = "no_line_read"
  set p5 = -1
  fread capline $f5 n5
  strstr p5 "_$capline" "casemode"
  if $p5 >= 0
    let nown = nown + 1
    set ownline = "$capline"
  end
end
fclose $f5
if nown <> 1
  echo "ERROR: harness broken, this session's own plot carries $&nown casemode lines"
  quit 1
end
set b5 = 99
strcmp b5 "$ownline" "Option: casemode=fold"
if $b5 <> 0
  echo "ERROR: this session's own plot records <$ownline> and not fold"
  quit 1
end

echo "INFO: a plot loaded from a file that recorded no mode is written out claiming none"
quit 0
