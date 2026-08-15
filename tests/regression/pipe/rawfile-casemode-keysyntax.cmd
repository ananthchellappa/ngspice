* a raw file that records a mode is a fixed point of load-and-write-again
* doc/codex/issues/0070, the secondary finding.
*
* Two writers in raw_write() (src/frontend/rawfile.c) put an 'Option:' line
* in a header.  One records the mode of the plot this session produced and
* spells the pair 'casemode=preserve'.  The other re-emits every pair the
* plot's environment carries -- which for a loaded plot is the file's own
* header -- and spelled it 'casemode = preserve'.  So the same claim came
* out of the two paths written two ways, and a file that carried the line
* and was copied came back re-spaced.
*
* ngspice reads either spelling, because the reader hands the text to
* cp_lexer() and cp_setparse(), which is the control language's own 'set'
* parser.  The exposure is to the third-party consumer this line exists to
* serve: the ask and the answer both quote the key as 'casemode=<mode>'
* (doc/claude/feedback/ngspice_upstream/FINDINGS.md finding 1), and a reader
* that matches that literally fails on a file ngspice itself wrote.
*
* The branch that re-emits is the CORRECT half of the provenance logic -- a
* pair the file carried is the only record of that run's mode there is, and
* re-emitting it unchanged is the point -- so what is asserted here is the
* format and not the branch: a copy is byte-identical, in this line, to the
* file it came from.  Check 3 copies the copy, because a fixed point that
* only holds once is not one.
*
* Check 4 is the same claim for a key that is not casemode, hand-spliced so
* that it cannot have come from this build's own writer.  The two writers
* agreeing is a property of the format, not of one key, and a fix that
* special-cased 'casemode' would pass checks 2 and 3 and fail here.
*
* This directory compares no output and asserts by exit code, so every check
* is fail-fast with an explicit 'quit 1'.  The strcmp and strstr result
* variables are preseeded, so a check whose comparison never ran -- the
* shape a failed substitution produces, since the dropped word leaves the
* command with too few arguments -- fails rather than passing on a stale
* zero.

set prompt = ""
set filetype = ascii

* --- 0. a file this session wrote, recording preserve ----------------------
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
echo "CaseKeySyntaxProbe" > rck_inner.cir
echo ".options noacct" >> rck_inner.cir
echo "vs In 0 dc 3" >> rck_inner.cir
echo "rl In MidNode 1k" >> rck_inner.cir
echo "rg MidNode 0 3k" >> rck_inner.cir
echo ".control" >> rck_inner.cir
echo "op" >> rck_inner.cir
echo ".endc" >> rck_inner.cir
echo ".end" >> rck_inner.cir
set casemodewrite
set casemode = preserve
source rck_inner.cir
set b0 = 99
strcmp b0 "$curcasemode" "preserve"
if $b0 <> 0
  echo "ERROR: harness broken, a source did not latch preserve"
  quit 1
end
write rck_first.raw

let nfirst = 0
set n0 = 1
set line1 = "no_hit_line"
fopen f0 rck_first.raw r
while $n0 >= 0
  set capline = "no_line_read"
  set p0 = -1
  fread capline $f0 n0
  strstr p0 "_$capline" "casemode"
  if $p0 >= 0
    let nfirst = nfirst + 1
    set line1 = "$capline"
  end
end
fclose $f0
if nfirst <> 1
  echo "ERROR: harness broken, the first file carries $&nfirst casemode lines"
  quit 1
end
set b1 = 99
strcmp b1 "$line1" "Option: casemode=preserve"
if $b1 <> 0
  echo "ERROR: harness broken, the first file records <$line1>"
  quit 1
end

* --- 1. load it and write it out again ------------------------------------
load rck_first.raw >& rck_load1.txt
let nsec = 0
set n1 = 1
set line2 = "no_hit_line"
write rck_second.raw
fopen f1 rck_second.raw r
while $n1 >= 0
  set capline = "no_line_read"
  set p1 = -1
  fread capline $f1 n1
  strstr p1 "_$capline" "casemode"
  if $p1 >= 0
    let nsec = nsec + 1
    set line2 = "$capline"
  end
end
fclose $f1
if nsec <> 1
  echo "ERROR: the copy carries $&nsec casemode lines, last <$line2>"
  quit 1
end

* --- 2. and the line is the line it came from, byte for byte --------------
set b2 = 99
strcmp b2 "$line2" "$line1"
if $b2 <> 0
  echo "ERROR: the copy re-spelled the line: <$line1> became <$line2>"
  quit 1
end

* --- 3. copying the copy changes nothing either ---------------------------
load rck_second.raw >& rck_load2.txt
let nthird = 0
set n2 = 1
set line3 = "no_hit_line"
write rck_third.raw
fopen f2 rck_third.raw r
while $n2 >= 0
  set capline = "no_line_read"
  set p2 = -1
  fread capline $f2 n2
  strstr p2 "_$capline" "casemode"
  if $p2 >= 0
    let nthird = nthird + 1
    set line3 = "$capline"
  end
end
fclose $f2
if nthird <> 1
  echo "ERROR: the second copy carries $&nthird casemode lines, last <$line3>"
  quit 1
end
set b3 = 99
strcmp b3 "$line3" "$line1"
if $b3 <> 0
  echo "ERROR: the second copy re-spelled the line: <$line1> became <$line3>"
  quit 1
end

* --- 4. a key that is not casemode round-trips the same way ---------------
* Hand built, so the pair cannot have come from this build's own writer.
* The header is the minimum a load accepts, and the Option: line sits after
* Plotname: because the reader's arm needs a plot to file the pair into.
echo "Title: hand built for doc/codex/issues/0070" > rck_hand.raw
echo "Date: Sat Aug 15 00:00:00  2026" >> rck_hand.raw
echo "Plotname: Operating Point" >> rck_hand.raw
echo "Option: rckprobe=fromfile" >> rck_hand.raw
echo "Flags: real" >> rck_hand.raw
echo "No. Variables: 1" >> rck_hand.raw
echo "No. Points: 1" >> rck_hand.raw
echo "Variables:" >> rck_hand.raw
echo " 0 v(probe) voltage" >> rck_hand.raw
echo "Values:" >> rck_hand.raw
echo " 0 1.0" >> rck_hand.raw
load rck_hand.raw >& rck_load3.txt
let n_key = $?rckprobe
if n_key <> 1
  echo "ERROR: harness broken, the hand-built Option: key did not reach the plot"
  quit 1
end
write rck_hand2.raw
let nhand = 0
set n4 = 1
set line4 = "no_hit_line"
fopen f4 rck_hand2.raw r
while $n4 >= 0
  set capline = "no_line_read"
  set p4 = -1
  fread capline $f4 n4
  strstr p4 "_$capline" "rckprobe"
  if $p4 >= 0
    let nhand = nhand + 1
    set line4 = "$capline"
  end
end
fclose $f4
if nhand <> 1
  echo "ERROR: the copy carries $&nhand rckprobe lines, last <$line4>"
  quit 1
end
set b4 = 99
strcmp b4 "$line4" "Option: rckprobe=fromfile"
if $b4 <> 0
  echo "ERROR: the copy re-spelled a hand-written pair as <$line4>"
  quit 1
end

* --- 5. a value that cannot be closed up keeps its spaces --------------
* The closed form is the default and not a rule: cp_lexer() drops a ','
* that ends a word and keeps one that starts a word, so 'k = ,b' reads back
* as ',b' while 'k=,b' reads back as 'b', and a value that is a bare ','
* takes the whole line down with it.  '<=' and '>=' hold together only at
* the start of a word.  So the writer leaves those three shapes spaced, and
* what is asserted is the property rather than the spelling: the value a
* reader gets out of the copy is the value it got out of the original.
echo "Title: hand built for doc/codex/issues/0070" > rck_odd.raw
echo "Date: Sat Aug 15 00:00:00  2026" >> rck_odd.raw
echo "Plotname: Operating Point" >> rck_odd.raw
echo "Option: rckcomma = ,b" >> rck_odd.raw
echo "Option: rckbare = ," >> rck_odd.raw
echo "Option: rckle = <=y" >> rck_odd.raw
echo "Flags: real" >> rck_odd.raw
echo "No. Variables: 1" >> rck_odd.raw
echo "No. Points: 1" >> rck_odd.raw
echo "Variables:" >> rck_odd.raw
echo " 0 v(probe) voltage" >> rck_odd.raw
echo "Values:" >> rck_odd.raw
echo " 0 1.0" >> rck_odd.raw
load rck_odd.raw >& rck_load4.txt
set b5a = 99
strcmp b5a "_$rckcomma" "_,b"
if $b5a <> 0
  echo "ERROR: harness broken, the loaded comma value reads <$rckcomma>"
  quit 1
end
set b5b = 99
strcmp b5b "_$rckbare" "_,"
if $b5b <> 0
  echo "ERROR: harness broken, the loaded bare comma reads <$rckbare>"
  quit 1
end
set b5c = 99
strcmp b5c "_$rckle" "_<=y"
if $b5c <> 0
  echo "ERROR: harness broken, the loaded <= value reads <$rckle>"
  quit 1
end
write rck_odd2.raw
load rck_odd2.raw >& rck_load5.txt
set b5d = 99
strcmp b5d "_$rckcomma" "_,b"
if $b5d <> 0
  echo "ERROR: a copy turned the value ,b into <$rckcomma>"
  quit 1
end
set b5e = 99
strcmp b5e "_$rckbare" "_,"
if $b5e <> 0
  echo "ERROR: a copy turned the value , into <$rckbare>"
  quit 1
end
set b5f = 99
strcmp b5f "_$rckle" "_<=y"
if $b5f <> 0
  echo "ERROR: a copy turned the value <=y into <$rckle>"
  quit 1
end

echo "INFO: a header's Option: line survives a copy byte for byte"
quit 0
