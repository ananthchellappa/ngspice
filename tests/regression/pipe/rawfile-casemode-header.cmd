* the raw header records the case mode the file was written under, when the
* session asks it to
* doc/claude/feedback/ngspice_upstream/FINDINGS.md finding 1, and
* doc/codex/issues/0061, which is the issue that ask kept alive.
*
* With 'casemodewrite' set, raw_write() (src/frontend/rawfile.c) emits
* 'Option: casemode=<mode>' immediately after the 'Plotname:' line, so a
* consumer that opens a file written last year can tell how the names in it
* were spelled without spawning a probe simulation.  Three properties are
* asserted here and each one of them is why the line has the shape it has:
*
*   - the KEY is 'Option:', which every existing reader already parses.  A
*     new 'Casemode:' key aborts the load on ngspice-46 and on this build
*     alike -- the header loop ends in an else that prints 'Error: strange
*     line in rawfile' and returns NULL.
*   - the PLACE is after 'Plotname:', because the reader's Option: arm needs
*     a current plot to file the pair into and prints 'Error: misplaced
*     Option: line' when there is none.  Check 3 below reads the load's own
*     diagnostics back and asserts that word is absent, so a line that moved
*     above 'Plotname:' would fail here rather than pass quietly.
*   - the VALUE is the mode in force, inp_case_mode_name()
*     (src/frontend/inpcom.c), which is what 'curcasemode' answers -- not the
*     'casemode' variable, which holds what was requested and stops being
*     authoritative the moment the deck is read (doc/codex/issues/0060).
*
* The mode is moved with 'set casemode' plus a 'source', because a source is
* a netlist read and re-latches (doc/codex/issues/0058), so one session can
* assert the round trip in all three modes.  tests/regression/casedist
* carries the deck half, where the mode comes from -D at start-up instead.
*
* The line is OPT-IN and the gate is asserted in both directions, by checks
* 0b and 6, which bracket the three modes.  A file that carries it can crash
* a released ngspice-46 -- that binary's 'unset casemode' frees a node it
* left linked (doc/codex/issues/0067, fixed in this tree and not in anything
* released), and the key has to be there for the unset to reach it -- so a
* file this build writes must not become a crash trigger for somebody else's
* simulator unless the session asked for the line.  Unset, the header this
* build writes is the header every ngspice has ever written, which is what
* check 0b asserts by position; the variable name is in the neighbourhood of
* 'appendwrite' and 'plainwrite', the write-path options it sits beside in
* raw_write().
*
* Check 5 is the other half of doc/codex/issues/0061 and the reason this
* line is safe to write at all: the pair is filed into the loaded plot's
* environment and stays readable there, but it does not steer the next
* netlist read.  Writing the mode into every raw file would otherwise mean
* every loaded raw file silently reconfigured the session that read it.  The
* request is unset before the load so that both of those checks are live:
* the read-back in check 4 can only be answered by the file, and the read in
* check 5 has nothing but the file to be wrongly answered by.
*
* This directory compares no output and asserts by exit code, so every check
* is fail-fast with an explicit 'quit 1'.  The strcmp result variables are
* preseeded, so a check whose strcmp never ran -- the shape a failed
* substitution produces, since the dropped word leaves strcmp with too few
* arguments -- fails rather than passing on a stale zero.

set prompt = ""
set filetype = ascii

* --- 0. the baseline: a session that asked for nothing is folding ----------
* The title is one word so that a '$curplottitle' substitution is one word,
* and it is mixed case so that it moves with the mode: 'casehdrprobe' under
* fold, 'CaseHdrProbe' under preserve and distinguish.  It is the spelling
* check 5 compares against.
let n_req_unset = $?casemode
if n_req_unset <> 0
  echo "ERROR: harness broken, casemode is already set at session start"
  quit 1
end
echo "CaseHdrProbe" > rch_inner.cir
echo ".options noacct" >> rch_inner.cir
echo "vs In 0 dc 3" >> rch_inner.cir
echo "rl In MidNode 1k" >> rch_inner.cir
echo "rg MidNode 0 3k" >> rch_inner.cir
echo ".control" >> rch_inner.cir
echo "op" >> rch_inner.cir
echo ".endc" >> rch_inner.cir
echo ".end" >> rch_inner.cir
source rch_inner.cir
set t_fold = "$curplottitle"
set b0 = 99
strcmp b0 "$curcasemode" "fold"
if $b0 <> 0
  echo "ERROR: harness broken, a session with no casemode set is not folding"
  quit 1
end

* --- 0b. and with the gate unset it writes the header it always wrote ------
* The claim is a position, so it is read as one: line 5 of a header this
* build writes with 'casemodewrite' unset is 'Flags:', the line that has
* followed 'Plotname:' since spice3.  A build that wrote the option line
* unconditionally fails here.
let n_gate_unset = $?casemodewrite
if n_gate_unset <> 0
  echo "ERROR: harness broken, casemodewrite is already set at session start"
  quit 1
end
write rch_off.raw
set n0 = 1
set o4 = "no_line_read"
set o5 = "no_line_read"
fopen f0 rch_off.raw r
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
set q5 = -1
strstr q5 "_$o5" "casemode"
if $q5 >= 0
  echo "ERROR: the header carries <$o5> after Plotname: with casemodewrite unset"
  quit 1
end
set b0b = 99
strcmp b0b "$o5" "Flags: real"
if $b0b <> 0
  echo "ERROR: the line after Plotname: is <$o5> and not <Flags: real> when unset"
  quit 1
end

* and from here on the session asks for the line
set casemodewrite

foreach m fold preserve distinguish

* --- 1. write a raw under this mode ----------------------------------------
  set casemode = $m
  source rch_inner.cir
  set c1 = 99
  strcmp c1 "$curcasemode" "$m"
  if $c1 <> 0
    echo "ERROR: harness broken, a source did not latch the mode $m"
    quit 1
  end
  write rch_hdr.raw

* --- 2. the header carries the mode, on the line after Plotname: -----------
* Five reads and no search: the position is the claim, so a line found
* anywhere else must fail.  Line 4 is checked as well, so a header that
* lost its 'Command:' line fails as a broken harness and not as a missing
* option.
  set n2 = 1
  set l1 = "no_line_read"
  set l2 = "no_line_read"
  set l3 = "no_line_read"
  set l4 = "no_line_read"
  set l5 = "no_line_read"
  fopen f2 rch_hdr.raw r
  fread l1 $f2 n2
  fread l2 $f2 n2
  fread l3 $f2 n2
  fread l4 $f2 n2
  fread l5 $f2 n2
  fclose $f2
  set p4 = -1
  strstr p4 "_$l4" "Plotname:"
  if $p4 < 0
    echo "ERROR: harness broken, line 4 of the header is not Plotname:, it is <$l4>"
    quit 1
  end
  set expect = "Option: casemode=$m"
  set c2 = 99
  strcmp c2 "$l5" "$expect"
  if $c2 <> 0
    echo "ERROR: the header line after Plotname: is <$l5> and not <$expect>"
    quit 1
  end

* --- 3. and the reader takes it without a complaint ------------------------
* 'misplaced' is the word the Option: arm prints when the line arrives with
* no plot to file it into, which is what a line written above 'Plotname:'
* would do.  The capture is scanned rather than read at a fixed line
* because 'load' writes several lines of its own first.
  unset casemode
  load rch_hdr.raw >& rch_load.txt
  set found3 = 0
  set n3 = 1
  fopen f3 rch_load.txt r
  while $n3 >= 0
    set capline = "no_line_read"
    set p3 = -1
    fread capline $f3 n3
    strstr p3 "_$capline" "misplaced"
    if $p3 >= 0
      set found3 = 1
    end
  end
  fclose $f3
  if $found3 > 0
    echo "ERROR: the reader called the casemode line misplaced in mode $m"
    quit 1
  end

* --- 4. a consumer reads the mode back out of the file ---------------------
* This is the whole of the ask: the loaded plot's environment answers
* '$casemode' with what the header recorded.  The request was unset above,
* so the answer can have come from nowhere else.
  set c4 = 99
  strcmp c4 "$casemode" "$m"
  if $c4 <> 0
    echo "ERROR: after loading a raw written under $m, casemode reads <$casemode>"
    quit 1
  end

* --- 5. and reading it does not reconfigure the reading session ------------
* doc/codex/issues/0061.  With the request unset the next read must latch
* the default, fold, and spell the title accordingly -- not the mode the
* loaded file was written under.
  source rch_inner.cir
  set c5 = 99
  strcmp c5 "$curcasemode" "fold"
  if $c5 <> 0
    echo "ERROR: a loaded raw written under $m latched $curcasemode for the next read"
    quit 1
  end
  set c6 = 99
  strcmp c6 "$curplottitle" "$t_fold"
  if $c6 <> 0
    echo "ERROR: a loaded raw written under $m changed the spelling of the next deck"
    quit 1
  end

end

* --- 6. the gate closes again, in the same session -------------------------
* The other direction of check 0b, and it is a separate check because a gate
* that only ever opened would pass 0b on start-up state alone.  The last
* 'source' of the loop left a fresh plot current, so this writes the same
* kind of file the loop wrote, with only the variable moved.
unset casemodewrite
write rch_back.raw
set n6 = 1
set s4 = "no_line_read"
set s5 = "no_line_read"
fopen f6 rch_back.raw r
fread s1 $f6 n6
fread s2 $f6 n6
fread s3 $f6 n6
fread s4 $f6 n6
fread s5 $f6 n6
fclose $f6
set r4 = -1
strstr r4 "_$s4" "Plotname:"
if $r4 < 0
  echo "ERROR: harness broken, line 4 of the header is not Plotname:, it is <$s4>"
  quit 1
end
set c7 = 99
strcmp c7 "$s5" "Flags: real"
if $c7 <> 0
  echo "ERROR: after unset casemodewrite the line after Plotname: is <$s5>"
  quit 1
end

echo "INFO: the raw header carried the effective mode in fold, preserve and distinguish"
quit 0
