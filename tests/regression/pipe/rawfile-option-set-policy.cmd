* a loaded rawfile's Option: line does not make a name read-only
* doc/codex/issues/0061, the write half of its rule; criterion 3's shape A.
*
* That issue settled the reading half: a plot's environment describes a plot
* and does not answer a question asked on behalf of a file being turned into
* cards.  The same principle has a write half, and until now cp_usrset()
* (src/frontend/options.c) broke it -- any name the current plot's
* environment carried was answered US_READONLY, so
*
*     load <any raw> ; set casemode=preserve
*     Error: casemode is a read-only variable.
*
* and the NEXT netlist read then parsed in the mode the file had named.  The
* only way to override the header was to setplot off the loaded plot.  Since
* this build records the case mode in every raw file it writes, that stopped
* being a hand-edited-header curiosity and became the ordinary case: load a
* waveform, and the session could no longer choose how it read its next deck.
*
* Nothing a simulation does fills a plot's environment -- the 'Option:' line
* of a raw header is its only writer (src/frontend/rawfile.c) -- so every
* name that arm could reach had arrived in a file the user merely loaded.  A
* file may not decide what the session is allowed to set.
*
* Stated as a rule and not as a name: check 3 below uses a key that is not
* 'casemode' and is not a policy switch at all, and asserts the same thing
* plus what 'unset' does with the two layers.  The deck is fail-fast by exit
* code, as everything in this directory is.

set prompt = ""

* The deck whose spelling makes the mode visible.  Its title is one word so
* that a '$curplottitle' substitution is one word, and it is mixed case so
* that a folding read and a preserving one give different answers.
echo "RSPInner" > rsp_inner.cir
echo ".options noacct" >> rsp_inner.cir
echo "vs in 0 dc 3" >> rsp_inner.cir
echo "rl in mid 1k" >> rsp_inner.cir
echo "rg mid 0 3k" >> rsp_inner.cir
echo ".control" >> rsp_inner.cir
echo "op" >> rsp_inner.cir
echo ".endc" >> rsp_inner.cir
echo ".end" >> rsp_inner.cir

* A raw file written by this build, so the key under test is the one an
* ordinary product of this simulator carries and not one spliced in by hand.
* 'casemodewrite' is what puts the key there: the option line is opt-in
* (tests/regression/pipe/rawfile-casemode-header.cmd asserts the gate itself),
* and a session that wants the header to record the mode says so.  This deck
* is about what a loaded key does to 'set' and 'unset', so it turns the
* writer on and leaves it on.
source rsp_inner.cir
set filetype = ascii
set casemodewrite
write rsp_written.raw

* --- 1. the session may still choose 'preserve' after a load ---------------
load rsp_written.raw
let k0 = $?casemode
if k0 <> 1
  echo "ERROR: harness broken, the raw this build wrote carries no casemode key"
  quit 1
end
set casemode = preserve
set c1 = 99
strcmp c1 "$casemode" "preserve"
if $c1 <> 0
  echo "ERROR: a loaded rawfile made casemode read-only for the session"
  quit 1
end
source rsp_inner.cir
set c2 = 99
strcmp c2 "$curcasemode" "preserve"
if $c2 <> 0
  echo "ERROR: the read after the load did not latch the mode the session asked for"
  quit 1
end
set c3 = 99
strcmp c3 "$curplottitle" "RSPInner"
if $c3 <> 0
  echo "ERROR: the read after the load folded a deck the session asked to preserve"
  quit 1
end

* --- 2. and 'distinguish', from a session that is already in 'preserve' ----
* Two modes and not one, so that whichever mode this session started in, at
* least one check is asking for a mode the loaded file did not name.
load rsp_written.raw
set casemode = distinguish
set c4 = 99
strcmp c4 "$casemode" "distinguish"
if $c4 <> 0
  echo "ERROR: a loaded rawfile made casemode read-only for the session"
  quit 1
end
source rsp_inner.cir
set c5 = 99
strcmp c5 "$curcasemode" "distinguish"
if $c5 <> 0
  echo "ERROR: the read after the load did not latch the second mode either"
  quit 1
end
set c6 = 99
strcmp c6 "$curplottitle" "RSPInner"
if $c6 <> 0
  echo "ERROR: the read after the load folded a deck read under distinguish"
  quit 1
end

* --- 3. the rule is about any key, and 'unset' takes one layer at a time ---
* The name here steers nothing and is not a control variable; it is here to
* say that the fix is not a special case for 'casemode'.  The rawfile is hand
* built, because no writer emits an arbitrary key unless it is round-tripping
* one it read.
echo "Title: hand built for doc/codex/issues/0061" > rsp_plain.raw
echo "Date: Tue Aug 11 00:00:00  2026" >> rsp_plain.raw
echo "Plotname: Operating Point" >> rsp_plain.raw
echo "Option: rspprobe=fromfile" >> rsp_plain.raw
echo "Flags: real" >> rsp_plain.raw
echo "No. Variables: 2" >> rsp_plain.raw
echo "No. Points: 1" >> rsp_plain.raw
echo "Variables:" >> rsp_plain.raw
echo " 0 v(1) voltage" >> rsp_plain.raw
echo " 1 OUT voltage" >> rsp_plain.raw
echo "Values:" >> rsp_plain.raw
echo " 0 3.0" >> rsp_plain.raw
echo "1.0" >> rsp_plain.raw
load rsp_plain.raw
set c7 = 99
strcmp c7 "$rspprobe" "fromfile"
if $c7 <> 0
  echo "ERROR: harness broken, the Option: key is not readable after the load"
  quit 1
end
set rspprobe = fromsession
set c8 = 99
strcmp c8 "$rspprobe" "fromsession"
if $c8 <> 0
  echo "ERROR: a loaded rawfile made an ordinary key read-only for the session"
  quit 1
end

* The session's value shadows the file's; it does not overwrite it.  So the
* first unset uncovers what the file said, and the second takes that away.
unset rspprobe
set c9 = 99
strcmp c9 "$rspprobe" "fromfile"
if $c9 <> 0
  echo "ERROR: unset of the session's value did not uncover the file's"
  quit 1
end
unset rspprobe
let k1 = $?rspprobe
if k1 <> 0
  echo "ERROR: unset could not remove a key a loaded rawfile carried"
  quit 1
end

echo "INFO: a loaded Option: line is readable and makes nothing read-only"
quit 0
