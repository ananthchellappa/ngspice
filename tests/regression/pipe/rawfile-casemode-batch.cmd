* the batch raw writer records the case mode too, when the session asks it to
* doc/codex/issues/0071, which is the coverage gap left by the feature filed
* as doc/codex/issues/0061 and asserted for the other writer by
* tests/regression/pipe/rawfile-casemode-header.cmd.
*
* ngspice has two writers for one raw format.  raw_write()
* (src/frontend/rawfile.c) serialises a plot that is already in memory and is
* what 'write' calls; fileInit() (src/frontend/outitf.c) streams a run to a
* file as the analysis produces it and is what 'ngspice -b -r out.raw' and
* 'run <file>' call.  They share their grammar and no code, so the option line
* added to the first one did not appear in the second, and a consumer could
* not tell a '-r' file written by a session that asked for the mode from a
* file written by a release that has no such feature.  This deck asserts the
* second writer, with the same three properties the first one is asserted for:
*
*   - the KEY is 'Option:', the key every existing reader already parses.
*   - the PLACE is the line after 'Plotname:', because the reader's Option:
*     arm needs a current plot to file the pair into and prints 'Error:
*     misplaced Option: line' when there is none.  Five more header lines are
*     read here than the claim strictly needs, so a line that landed anywhere
*     else -- or a header that grew a second copy -- fails on a position and
*     not on a search.
*   - the VALUE is the mode in force, inp_case_mode_name()
*     (src/frontend/inpcom.c), which is what 'curcasemode' answers.  Check 4
*     is that half on its own: it moves the 'casemode' request without a
*     netlist read, so a writer that quoted the request rather than the mode
*     would write a mode this run's names were never spelled under
*     (doc/codex/issues/0060).
*
* This directory runs 'ngspice -p < deck' and so cannot pass '-r'.  It does
* not need to: 'run <file>' from the control language reaches fileInit()
* through the same dosim()/ft_getOutReq() route (src/frontend/runcoms.c:289
* and :421), which is measured in doc/codex/issues/0071 and is why one arm in
* that function covers both entry points.  What this deck cannot reach is the
* third one, 'ngspice -s', whose raw goes to stdout; that path is the reason
* the new line is written in this function's own idiom -- sprintf into buf,
* n += strlen(buf), fputs -- because 'n' is the fallback for the seek position
* fileEnd() backfills the point count into when ftell() is unusable.  Check 5
* is the half of that trap this harness can see: it compares the count in the
* file against the count of the same run held in memory, so a header whose
* reserved field was overwritten in the wrong place fails here.
*
* The line is OPT-IN, and the gate is asserted in both directions in one
* session by checks 0b and 6.  Unset -- the default -- this writer's header is
* the header every ngspice back to spice3 has written, which check 0b asserts
* by position in both the ASCII and the binary format; a consumer that counts
* lines off a batch raw is a consumer this repository contains
* (postcoms-keyword-case.cmd), so that half is the one to fail loudly.
*
* This directory compares no output and asserts by exit code, so every check
* is fail-fast with an explicit 'quit 1'.  The strcmp result variables are
* preseeded, so a check whose strcmp never ran -- the shape a failed
* substitution produces, since the dropped word leaves strcmp with too few
* arguments -- fails rather than passing on a stale zero.

set prompt = ""
set filetype = ascii

* --- 0. the baseline: a session that asked for nothing is folding ----------
* The inner deck has no .control block, so a 'source' loads it and runs
* nothing; the analysis happens when this session says 'run <file>', which is
* the writer under test.  It is a .tran and not an .op because a single point
* cannot show a point count that moved.
let n_req_unset = $?casemode
if n_req_unset <> 0
  echo "ERROR: harness broken, casemode is already set at session start"
  quit 1
end
echo "BatchHdrProbe" > rcb_inner.cir
echo ".options noacct" >> rcb_inner.cir
echo "vs In 0 dc 3" >> rcb_inner.cir
echo "rl In MidNode 1k" >> rcb_inner.cir
echo "rg MidNode 0 3k" >> rcb_inner.cir
echo ".tran 1u 10u 0 1u" >> rcb_inner.cir
echo ".end" >> rcb_inner.cir
source rcb_inner.cir
set b0 = 99
strcmp b0 "$curcasemode" "fold"
if $b0 <> 0
  echo "ERROR: harness broken, a session with no casemode set is not folding"
  quit 1
end

* --- 0b. with the gate unset it writes the header it always wrote ----------
* The claim is a position, so it is read as one: line 5 of a batch header
* written with 'casemodewrite' unset is 'Flags:', the line that has followed
* 'Plotname:' in this writer since spice3.  Both formats, because the header
* is the same text in both and a build that wrote the line unconditionally
* would have to fail in both.
let n_gate_unset = $?casemodewrite
if n_gate_unset <> 0
  echo "ERROR: harness broken, casemodewrite is already set at session start"
  quit 1
end
run rcb_off.raw
set n0 = 1
set o4 = "no_line_read"
set o5 = "no_line_read"
fopen f0 rcb_off.raw r
fread o1 $f0 n0
fread o2 $f0 n0
fread o3 $f0 n0
fread o4 $f0 n0
fread o5 $f0 n0
fclose $f0
set q4 = -1
strstr q4 "_$o4" "Plotname:"
if $q4 < 0
  echo "ERROR: harness broken, line 4 of the batch header is not Plotname:, it is <$o4>"
  quit 1
end
set q5 = -1
strstr q5 "_$o5" "casemode"
if $q5 >= 0
  echo "ERROR: the batch header carries <$o5> after Plotname: with casemodewrite unset"
  quit 1
end
set b0b = 99
strcmp b0b "$o5" "Flags: real"
if $b0b <> 0
  echo "ERROR: the line after Plotname: is <$o5> and not <Flags: real> when unset"
  quit 1
end
set filetype = binary
run rcb_offb.raw
set n0b = 1
set p4b = "no_line_read"
set p5b = "no_line_read"
fopen f0b rcb_offb.raw r
fread p1b $f0b n0b
fread p2b $f0b n0b
fread p3b $f0b n0b
fread p4b $f0b n0b
fread p5b $f0b n0b
fclose $f0b
set q4b = -1
strstr q4b "_$p4b" "Plotname:"
if $q4b < 0
  echo "ERROR: harness broken, line 4 of the binary batch header is not Plotname:, it is <$p4b>"
  quit 1
end
set b0c = 99
strcmp b0c "$p5b" "Flags: real"
if $b0c <> 0
  echo "ERROR: binary, the line after Plotname: is <$p5b> and not <Flags: real> when unset"
  quit 1
end
set filetype = ascii

* and from here on the session asks for the line
set casemodewrite

foreach m fold preserve distinguish

* --- 1. run this circuit into a file under this mode -----------------------
* 'set casemode' plus a 'source' is how one session moves the mode: a source
* is a netlist read and re-latches it (doc/codex/issues/0058).
  set casemode = $m
  source rcb_inner.cir
  set c1 = 99
  strcmp c1 "$curcasemode" "$m"
  if $c1 <> 0
    echo "ERROR: harness broken, a source did not latch the mode $m"
    quit 1
  end
  run rcb_hdr.raw

* --- 2. the header carries the mode, on the line after Plotname: -----------
* Eight reads and no search.  Lines 6, 7 and 8 are read as well, so a header
* that carried the option line twice, or that lost a line to make room for
* it, fails here rather than passing on line 5 alone.
  set n2 = 1
  set l4 = "no_line_read"
  set l5 = "no_line_read"
  set l6 = "no_line_read"
  set l7 = "no_line_read"
  set l8 = "no_line_read"
  fopen f2 rcb_hdr.raw r
  fread l1 $f2 n2
  fread l2 $f2 n2
  fread l3 $f2 n2
  fread l4 $f2 n2
  fread l5 $f2 n2
  fread l6 $f2 n2
  fread l7 $f2 n2
  fread l8 $f2 n2
  fclose $f2
  set p4 = -1
  strstr p4 "_$l4" "Plotname:"
  if $p4 < 0
    echo "ERROR: harness broken, line 4 of the batch header is not Plotname:, it is <$l4>"
    quit 1
  end
  set expect = "Option: casemode=$m"
  set c2 = 99
  strcmp c2 "$l5" "$expect"
  if $c2 <> 0
    echo "ERROR: the batch header line after Plotname: is <$l5> and not <$expect>"
    quit 1
  end
  set c2b = 99
  strcmp c2b "$l6" "Flags: real"
  if $c2b <> 0
    echo "ERROR: the line after the casemode line is <$l6> and not <Flags: real>"
    quit 1
  end
  set c2c = 99
  strcmp c2c "$l7" "No. Variables: 4"
  if $c2c <> 0
    echo "ERROR: the batch header line 7 is <$l7> and not <No. Variables: 4>"
    quit 1
  end
  set p8 = -1
  strstr p8 "_$l8" "No. Points:"
  if $p8 < 0
    echo "ERROR: the batch header line 8 is <$l8> and not the No. Points: line"
    quit 1
  end

* --- 3. and the same in the binary format ----------------------------------
* The two formats share this header text and differ only in the marker that
* ends it, so a line that reached one and not the other would be a writer
* that grew a second branch.
  set filetype = binary
  run rcb_bin.raw
  set n3 = 1
  set k4 = "no_line_read"
  set k5 = "no_line_read"
  set k6 = "no_line_read"
  fopen f3 rcb_bin.raw r
  fread k1 $f3 n3
  fread k2 $f3 n3
  fread k3 $f3 n3
  fread k4 $f3 n3
  fread k5 $f3 n3
  fread k6 $f3 n3
  fclose $f3
  set p4b2 = -1
  strstr p4b2 "_$k4" "Plotname:"
  if $p4b2 < 0
    echo "ERROR: harness broken, line 4 of the binary batch header is not Plotname:, it is <$k4>"
    quit 1
  end
  set c3 = 99
  strcmp c3 "$k5" "$expect"
  if $c3 <> 0
    echo "ERROR: binary, the header line after Plotname: is <$k5> and not <$expect>"
    quit 1
  end
  set c3b = 99
  strcmp c3b "$k6" "Flags: real"
  if $c3b <> 0
    echo "ERROR: binary, the line after the casemode line is <$k6> and not <Flags: real>"
    quit 1
  end
  set filetype = ascii

end

* --- 4. the value is the mode in force and not the request -----------------
* doc/codex/issues/0060.  The loop left the session reading under
* 'distinguish'; moving the request without a netlist read leaves the names
* in the next run spelled the way they always were, so a header that quoted
* the request would describe a run that never happened.
set casemode = fold
set c4a = 99
strcmp c4a "$curcasemode" "distinguish"
if $c4a <> 0
  echo "ERROR: harness broken, setting the request moved the mode in force to $curcasemode"
  quit 1
end
run rcb_eff.raw
set n4 = 1
set e4 = "no_line_read"
set e5 = "no_line_read"
fopen f4 rcb_eff.raw r
fread e1 $f4 n4
fread e2 $f4 n4
fread e3 $f4 n4
fread e4 $f4 n4
fread e5 $f4 n4
fclose $f4
set c4 = 99
strcmp c4 "$e5" "Option: casemode=distinguish"
if $c4 <> 0
  echo "ERROR: with the request moved to fold the batch header says <$e5>"
  quit 1
end

* --- 5. and the point count survived the extra line ------------------------
* fileInit() writes its header before the first data point exists and
* reserves eight characters for a count fileEnd() seeks back to, using the
* length it accumulated as the fallback position.  The independent count is
* the same circuit run into memory: 'run' with no argument takes plotInit()
* instead of this writer, so the two numbers come from different code and a
* header whose reserved field moved gives a file that reads back short.
run
let n_mem = length(time)
if $&n_mem < 2
  echo "ERROR: harness broken, the in-memory run has $&n_mem points"
  quit 1
end
load rcb_eff.raw
let n_file = length(time)
if $&n_file <> $&n_mem
  echo "ERROR: the batch file holds $&n_file points and the same run in memory holds $&n_mem"
  quit 1
end

* --- 6. the gate closes again, in the same session -------------------------
* The other direction of check 0b, and a separate check because a gate that
* only ever opened would pass 0b on start-up state alone.
unset casemodewrite
run rcb_back.raw
set n6 = 1
set s4 = "no_line_read"
set s5 = "no_line_read"
fopen f6 rcb_back.raw r
fread s1 $f6 n6
fread s2 $f6 n6
fread s3 $f6 n6
fread s4 $f6 n6
fread s5 $f6 n6
fclose $f6
set r4 = -1
strstr r4 "_$s4" "Plotname:"
if $r4 < 0
  echo "ERROR: harness broken, line 4 of the batch header is not Plotname:, it is <$s4>"
  quit 1
end
set c6 = 99
strcmp c6 "$s5" "Flags: real"
if $c6 <> 0
  echo "ERROR: after unset casemodewrite the batch header line after Plotname: is <$s5>"
  quit 1
end

echo "INFO: the batch raw header carried the effective mode in fold, preserve and distinguish"
quit 0
