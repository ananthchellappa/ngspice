* a re-written loaded plot's header carries one casemode line, and it is the
* mode that plot was produced under
* doc/codex/issues/0061, the fourth addendum.
*
* With 'casemodewrite' set, raw_write() (src/frontend/rawfile.c) records the
* case mode in force under the 'Plotname:' line.  It also re-emits, further
* down, every 'Option:' pair the plot carries in its environment -- and for a
* plot that came out of a file, that environment is the file's own header,
* filed there by raw_read().  So a plot loaded from a file written under
* 'preserve' and written out again by a folding session came out with two
* casemode lines: the copying session's 'fold' first, the file's own
* 'preserve' second.  The reader's own lookup answers with the first, so the
* header described the session that copied the file rather than the names
* inside it -- v(In) and v(MidNode), which no folding run can produce.
*
* The rule that fixes it names no variable: what a header records is a
* property of the plot, so the session's mode is stamped only on a plot the
* session produced.  A plot that arrived carrying an environment of its own
* is described by that environment and by nothing else, and the loop that
* re-emits it is left alone -- a pair the file carried is data the user may
* want kept, and it is the only record of that file's mode there is.
*
* Both assertions here are about the file on disk.  Exactly one of its lines
* mentions the mode: counting is what makes that live, because the defect
* was a second line and not a wrong one.  And a reader that loads the file
* back is told 'preserve': the load is what makes that live, because then it
* is the reader's own lookup, and not this deck's parse, that has to pick
* the surviving line.
*
* Check 2 is the control that the plot under test really is foreign: its
* vectors are still spelled the way the preserving run spelled them, which
* is a spelling the session doing the writing here cannot produce.
*
* This directory compares no output and asserts by exit code, so every check
* is fail-fast with an explicit 'quit 1'.  The strcmp and strstr result
* variables are preseeded, so a check whose comparison never ran -- the
* shape a failed substitution produces, since the dropped word leaves the
* command with too few arguments -- fails rather than passing on a stale
* zero.

set prompt = ""
set filetype = ascii

* --- 0. a file written under preserve, by a session that asked for the line
* The nets are mixed case so that the file's own spelling is evidence: under
* preserve they stay In and MidNode, and a folding read of the same deck
* cannot produce them.
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
echo "CaseRewriteProbe" > rcw_inner.cir
echo ".options noacct" >> rcw_inner.cir
echo "vs In 0 dc 3" >> rcw_inner.cir
echo "rl In MidNode 1k" >> rcw_inner.cir
echo "rg MidNode 0 3k" >> rcw_inner.cir
echo ".control" >> rcw_inner.cir
echo "op" >> rcw_inner.cir
echo ".endc" >> rcw_inner.cir
echo ".end" >> rcw_inner.cir
set casemodewrite
set casemode = preserve
source rcw_inner.cir
set b0 = 99
strcmp b0 "$curcasemode" "preserve"
if $b0 <> 0
  echo "ERROR: harness broken, a source did not latch preserve"
  quit 1
end
write rcw_pres.raw

* and the file really does record it, on the line after Plotname:
set n0 = 1
set o4 = "no_line_read"
set o5 = "no_line_read"
fopen f0 rcw_pres.raw r
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
strcmp b1 "$o5" "Option: casemode=preserve"
if $b1 <> 0
  echo "ERROR: harness broken, the preserve file records <$o5>"
  quit 1
end

* --- 1. and now the session is a folding one ------------------------------
* The request is dropped and the deck re-read, because a source is what
* re-latches the mode (doc/codex/issues/0058).
unset casemode
source rcw_inner.cir
set b2 = 99
strcmp b2 "$curcasemode" "fold"
if $b2 <> 0
  echo "ERROR: harness broken, the session did not go back to folding"
  quit 1
end

* --- 2. load the preserve file: the plot is foreign and says so -----------
load rcw_pres.raw >& rcw_load1.txt
display >& rcw_disp.txt
set found2 = 0
set n2 = 1
fopen f2 rcw_disp.txt r
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

* --- 3. write it out again: one casemode line, and it is the file's -------
write rcw_again.raw
let nhits = 0
set n3 = 1
set hitline = "no_hit_line"
fopen f3 rcw_again.raw r
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
if nhits > 1
  echo "ERROR: the re-written plot carries $&nhits casemode lines, last <$hitline>"
  quit 1
end
if nhits < 1
  echo "ERROR: the re-written plot carries no casemode line at all"
  quit 1
end
set q3 = -1
strstr q3 "_$hitline" "preserve"
if $q3 < 0
  echo "ERROR: the one casemode line is <$hitline> and does not record preserve"
  quit 1
end
set r3 = -1
strstr r3 "_$hitline" "fold"
if $r3 >= 0
  echo "ERROR: the one casemode line is <$hitline> and records the copying session"
  quit 1
end

* --- 4. and a reader is told preserve when it loads the copy --------------
* The request is unset in this session, so nothing but the file can answer.
* The load's own output is captured and scanned for the two complaints the
* header loop makes -- a line it will not parse, and an Option: line with no
* plot to file it into -- so a copy that moved the line somewhere illegal
* fails here instead of passing quietly.
load rcw_again.raw >& rcw_load2.txt
set found4 = 0
set n4 = 1
fopen f4 rcw_load2.txt r
while $n4 >= 0
  set capline = "no_line_read"
  set p4 = -1
  set s4 = -1
  fread capline $f4 n4
  strstr p4 "_$capline" "misplaced"
  if $p4 >= 0
    set found4 = 1
  end
  strstr s4 "_$capline" "strange"
  if $s4 >= 0
    set found4 = 1
  end
end
fclose $f4
if $found4 > 0
  echo "ERROR: the reader complained about the re-written header"
  quit 1
end
let n_ans4 = $?casemode
if n_ans4 <> 1
  echo "ERROR: harness broken, the loaded copy answers no casemode at all"
  quit 1
end
set b4 = 99
strcmp b4 "$casemode" "preserve"
if $b4 <> 0
  echo "ERROR: the re-written file reads back as <$casemode> and not preserve"
  quit 1
end

echo "INFO: a copied plot's header records the mode the plot was produced under"
quit 0
