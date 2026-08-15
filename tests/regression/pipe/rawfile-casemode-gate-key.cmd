* a rawfile header does not open this build's own writer gate
* doc/codex/issues/0061, the fourth addendum.
*
* The 'Option: casemode=' line is opt-in: raw_write() (src/frontend/rawfile.c)
* records the mode only when 'casemodewrite' is set, because a file that
* carries the key can crash a released ngspice-46 -- that binary's 'unset
* casemode' frees a node it left linked (doc/codex/issues/0067, fixed here
* and in nothing released) -- and the third addendum is that decision.
*
* The gate is a question this build asks about itself, and it used to ask
* cp_getvar(), whose chain ends in the current plot's environment.  A raw
* header carrying 'Option: casemodewrite' is parsed straight into that
* environment, so loading somebody's waveform switched this session's writer
* on for the rest of the run: a default a data file can flip is not a
* default, and the whole point of this one being off is that our files must
* not become somebody else's crash.
*
* The rule is doc/codex/issues/0061's, one step further out.  A plot's
* environment describes a plot: it may answer a question about the session,
* and it may not answer a policy question -- one the code asks on its own
* behalf about what it may do.  0061 applied that to every read taken while
* a netlist was being read; a write is not one, so this read declares itself
* instead, through cp_getvar_policy() (src/frontend/variable.c).  No variable
* name appears in either predicate.
*
* The plot written here is a plot of this session and not the loaded one, so
* that the deck fails on the gate and on nothing else: a loaded plot's own
* re-write stopped carrying the line for a different reason, which is what
* tests/regression/pipe/rawfile-casemode-rewrite.cmd covers.  Check 3 is the
* control that the writer is still alive -- the same write, of the same
* plot, with the same plot current, after the session itself asks for the
* line.
*
* This directory compares no output and asserts by exit code, so every check
* is fail-fast with an explicit 'quit 1'.  The strcmp and strstr result
* variables are preseeded, so a check whose comparison never ran fails
* rather than passing on a stale zero.

set prompt = ""
set filetype = ascii

* --- 0. a plot of this session, and the gate is shut ----------------------
let n_gate_unset = $?casemodewrite
if n_gate_unset <> 0
  echo "ERROR: harness broken, casemodewrite is already set at session start"
  quit 1
end
echo "CaseGateProbe" > rcg_inner.cir
echo ".options noacct" >> rcg_inner.cir
echo "vs in 0 dc 3" >> rcg_inner.cir
echo "rl in mid 1k" >> rcg_inner.cir
echo "rg mid 0 3k" >> rcg_inner.cir
echo ".control" >> rcg_inner.cir
echo "op" >> rcg_inner.cir
echo ".endc" >> rcg_inner.cir
echo ".end" >> rcg_inner.cir
source rcg_inner.cir
* The written plot is named explicitly below, because the plot that is
* current at that point is the loaded one.  A change in plot numbering
* therefore has to fail here, as a broken harness, and not further down as a
* missing line.
set b0 = 99
strcmp b0 "$curplot" "op1"
if $b0 <> 0
  echo "ERROR: harness broken, the session's own plot is <$curplot> and not op1"
  quit 1
end

* with nothing set, the header is the header every ngspice has ever written
write rcg_shut.raw op1.all
set n0 = 1
set o4 = "no_line_read"
set o5 = "no_line_read"
fopen f0 rcg_shut.raw r
fread o1 $f0 n0
fread o2 $f0 n0
fread o3 $f0 n0
fread o4 $f0 n0
fread o5 $f0 n0
fclose $f0
set q4 = -1
strstr q4 "_$o4" "Plotname:"
if $q4 < 0
  echo "ERROR: harness broken, line 4 of the header is not Plotname:, it is <$o4>"
  quit 1
end
set b1 = 99
strcmp b1 "$o5" "Flags: real"
if $b1 <> 0
  echo "ERROR: harness broken, the shut gate wrote <$o5> after Plotname:"
  quit 1
end

* --- 1. a file whose header carries the gate key --------------------------
* Hand built, because no writer emits this pair: it is the shape somebody
* else's file, or a hand-edited header, can have.  The key is a bare
* boolean, which is how 'set casemodewrite' would spell it.
echo "Title: hand built for doc/codex/issues/0061" > rcg_gate.raw
echo "Date: Tue Aug 11 00:00:00  2026" >> rcg_gate.raw
echo "Plotname: Operating Point" >> rcg_gate.raw
echo "Option: casemodewrite" >> rcg_gate.raw
echo "Flags: real" >> rcg_gate.raw
echo "No. Variables: 1" >> rcg_gate.raw
echo "No. Points: 1" >> rcg_gate.raw
echo "Variables:" >> rcg_gate.raw
echo " 0 v(probe) voltage" >> rcg_gate.raw
echo "Values:" >> rcg_gate.raw
echo " 0 1.0" >> rcg_gate.raw
load rcg_gate.raw

* The key really did reach the plot, so the check below is live: if it had
* not, a build that read the gate through the plot would pass for free.
let n_key = $?casemodewrite
if n_key <> 1
  echo "ERROR: harness broken, the loaded header's key does not answer at all"
  quit 1
end
set b2 = 99
strcmp b2 "$curplot" "op1"
if $b2 = 0
  echo "ERROR: harness broken, the load left this session's own plot current"
  quit 1
end

* --- 2. writing a plot of this session, with that file's plot current -----
write rcg_out.raw op1.all
let nhits = 0
set n2 = 1
set hitline = "no_hit_line"
fopen f2 rcg_out.raw r
while $n2 >= 0
  set capline = "no_line_read"
  set p2 = -1
  fread capline $f2 n2
  strstr p2 "_$capline" "casemode"
  if $p2 >= 0
    let nhits = nhits + 1
    set hitline = "$capline"
  end
end
fclose $f2
if nhits > 0
  echo "ERROR: a loaded header opened the writer gate: <$hitline>"
  quit 1
end
set n3 = 1
set s4 = "no_line_read"
set s5 = "no_line_read"
fopen f3 rcg_out.raw r
fread s1 $f3 n3
fread s2 $f3 n3
fread s3 $f3 n3
fread s4 $f3 n3
fread s5 $f3 n3
fclose $f3
set q5 = -1
strstr q5 "_$s4" "Plotname:"
if $q5 < 0
  echo "ERROR: harness broken, line 4 of the header is not Plotname:, it is <$s4>"
  quit 1
end
set b3 = 99
strcmp b3 "$s5" "Flags: real"
if $b3 <> 0
  echo "ERROR: after loading a header carrying the key, line 5 is <$s5>"
  quit 1
end

* --- 3. and the session can still open the gate itself --------------------
* Same write, same plot, same current plot: only the session's own request
* is new, so a fix that closed the gate by breaking the writer fails here.
set casemodewrite
write rcg_open.raw op1.all
set n4 = 1
set t4 = "no_line_read"
set t5 = "no_line_read"
fopen f4 rcg_open.raw r
fread t1 $f4 n4
fread t2 $f4 n4
fread t3 $f4 n4
fread t4 $f4 n4
fread t5 $f4 n4
fclose $f4
set q6 = -1
strstr q6 "_$t4" "Plotname:"
if $q6 < 0
  echo "ERROR: harness broken, line 4 of the header is not Plotname:, it is <$t4>"
  quit 1
end
set expect = "Option: casemode=$curcasemode"
set b4 = 99
strcmp b4 "$t5" "$expect"
if $b4 <> 0
  echo "ERROR: with casemodewrite set by the session, line 5 is <$t5> and not <$expect>"
  quit 1
end

echo "INFO: the writer gate is answered by the session and not by a loaded file"
quit 0
