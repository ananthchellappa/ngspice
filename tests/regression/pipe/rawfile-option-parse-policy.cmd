* a loaded rawfile's Option: line does not steer the next netlist read
* doc/codex/issues/0061, acceptance criteria 1, 2 and 5.
*
* A raw header's 'Option:' line is parsed straight into the loaded plot's
* environment (src/frontend/rawfile.c), and cp_getvar()
* (src/frontend/variable.c) consulted that environment for any name at all
* -- including the two policy switches inp_readall() reads once per netlist
* read, set_case_mode() (src/frontend/inpcom.c) and set_compat_mode()
* (src/frontend/inpcompat.c).  So a file the user merely loaded decided how
* the next deck he sourced was parsed.
*
* The owner chose shape A of criterion 3: the pair is still filed and still
* readable -- '$casemode' below answers 'preserve' after the load, which is
* what the client that reported this asked a header for -- and what stops is
* the plot environment answering a read taken while a netlist is being read.
* Each half therefore carries a positive control that reads the value back
* first, so a narrowing that simply refused the key at the Option: arm would
* fail this deck rather than pass it for free.
*
* Criterion 2 is why 'ngbehavior' is here beside 'casemode': the two are one
* defect, and a narrowing that named 'casemode' would leave the other
* standing.  Neither name appears in the fix, and neither is what this deck
* keys on: the rule is that a read taken while a file is being turned into
* cards is not answered by a plot's environment.
*
* This directory is the shape doc/codex/issues/0048 gives: the sequence is
* 'load' then 'source', which is control language and not a netlist.  It has
* no reference output and asserts by exit code, so every check is fail-fast
* with an explicit 'quit 1'.  The rawfiles are hand built with echo and its
* redirect, as tests/regression/casedist/vector-rawfile-scale-case.cir does,
* because no ngspice writer emits an 'Option:' line unless it is
* round-tripping one it read.
*
* The case checks compare the second read of a deck against the FIRST read
* of the same deck rather than against a literal spelling, so this deck says
* the same thing in all three case modes: whatever this session parses
* 'MidNodeProbe' to with nothing in front of it, it must parse it to after a
* load.  Two rawfiles are loaded in turn, one asking for 'preserve' and one
* for 'distinguish', so that whichever of the three modes the session is in,
* at least one of them -- in a folding session, both -- is asking for a
* different one and is a live check.  'curcasemode' is the mode in force
* (doc/codex/issues/0060) and the title is its consequence; both are
* asserted, because a repair of the reader that left the title folded would
* satisfy neither on its own.
*
* The strcmp result variables are preseeded, so a check whose strcmp never
* ran -- the shape a failed substitution produces, since the dropped word
* leaves strcmp with too few arguments -- fails rather than passing on a
* stale zero.  The two 0**0 probes are preseeded for the same reason: a
* 'let' of a vector the source never created leaves the seed, which is not
* the value either check accepts.

set prompt = ""

* The deck both halves read.  Its title is one word so that a
* '$curplottitle' substitution is one word; its nets are mixed case so that
* the title and the vector spellings move together under a mode change; and
* its probe node is lower case in the deck so that the reference to it below
* resolves in all three modes.  0**0 is 1 everywhere except under hs, where
* PTpowerH() (src/spicelib/parser/ptfuncs.c) answers 0 -- that is the
* compatibility mode made visible as a number, with no output capture.
echo "MidNodeProbe" > rop_inner.cir
echo ".options noacct" >> rop_inner.cir
echo "vs In 0 dc 3" >> rop_inner.cir
echo "rl In MidNode 1k" >> rop_inner.cir
echo "rg MidNode 0 3k" >> rop_inner.cir
echo "b1 hsn 0 v = 0**0" >> rop_inner.cir
echo "rh hsn 0 1k" >> rop_inner.cir
echo ".control" >> rop_inner.cir
echo "op" >> rop_inner.cir
echo ".endc" >> rop_inner.cir
echo ".end" >> rop_inner.cir

* --- 1. the same deck, with nothing in front of it --------------------------
let hs0 = 99
source rop_inner.cir
set t0 = "$curplottitle"
set m0 = "$curcasemode"
let hs0 = v(hsn)
if hs0 <> 1
  echo "ERROR: harness broken, 0**0 is not 1 here, so ngbehavior is already set"
  quit 1
end

* --- 2. a rawfile whose Option: line asks for 'preserve' --------------------
echo "Title: hand built for doc/codex/issues/0061" > rop_pres.raw
echo "Date: Tue Aug 11 00:00:00  2026" >> rop_pres.raw
echo "Plotname: Operating Point" >> rop_pres.raw
echo "Option: casemode=preserve" >> rop_pres.raw
echo "Flags: real" >> rop_pres.raw
echo "No. Variables: 2" >> rop_pres.raw
echo "No. Points: 1" >> rop_pres.raw
echo "Variables:" >> rop_pres.raw
echo " 0 v(1) voltage" >> rop_pres.raw
echo " 1 OUT voltage" >> rop_pres.raw
echo "Values:" >> rop_pres.raw
echo " 0 3.0" >> rop_pres.raw
echo "1.0" >> rop_pres.raw
load rop_pres.raw
set c1 = 99
strcmp c1 "$casemode" "preserve"
if $c1 <> 0
  echo "ERROR: harness broken, the Option: pair is not readable after the load"
  quit 1
end
source rop_inner.cir
set d1 = 99
strcmp d1 "$curcasemode" "$m0"
if $d1 <> 0
  echo "ERROR: a loaded Option: casemode=preserve moved the mode the next read latched"
  quit 1
end
set d2 = 99
strcmp d2 "$curplottitle" "$t0"
if $d2 <> 0
  echo "ERROR: a loaded Option: casemode=preserve changed the spelling of the next deck"
  quit 1
end

* --- 3. and one asking for 'distinguish' -----------------------------------
echo "Title: hand built for doc/codex/issues/0061" > rop_dist.raw
echo "Date: Tue Aug 11 00:00:00  2026" >> rop_dist.raw
echo "Plotname: Operating Point" >> rop_dist.raw
echo "Option: casemode=distinguish" >> rop_dist.raw
echo "Flags: real" >> rop_dist.raw
echo "No. Variables: 2" >> rop_dist.raw
echo "No. Points: 1" >> rop_dist.raw
echo "Variables:" >> rop_dist.raw
echo " 0 v(1) voltage" >> rop_dist.raw
echo " 1 OUT voltage" >> rop_dist.raw
echo "Values:" >> rop_dist.raw
echo " 0 3.0" >> rop_dist.raw
echo "1.0" >> rop_dist.raw
load rop_dist.raw
set c2 = 99
strcmp c2 "$casemode" "distinguish"
if $c2 <> 0
  echo "ERROR: harness broken, the Option: pair is not readable after the second load"
  quit 1
end
source rop_inner.cir
set d3 = 99
strcmp d3 "$curcasemode" "$m0"
if $d3 <> 0
  echo "ERROR: a loaded Option: casemode=distinguish moved the mode the next read latched"
  quit 1
end
set d4 = 99
strcmp d4 "$curplottitle" "$t0"
if $d4 <> 0
  echo "ERROR: a loaded Option: casemode=distinguish changed the spelling of the next deck"
  quit 1
end

* --- 4. the same rule, the other policy switch ------------------------------
echo "Title: hand built for doc/codex/issues/0061" > rop_ngb.raw
echo "Date: Tue Aug 11 00:00:00  2026" >> rop_ngb.raw
echo "Plotname: Operating Point" >> rop_ngb.raw
echo "Option: ngbehavior=hs" >> rop_ngb.raw
echo "Flags: real" >> rop_ngb.raw
echo "No. Variables: 2" >> rop_ngb.raw
echo "No. Points: 1" >> rop_ngb.raw
echo "Variables:" >> rop_ngb.raw
echo " 0 v(1) voltage" >> rop_ngb.raw
echo " 1 OUT voltage" >> rop_ngb.raw
echo "Values:" >> rop_ngb.raw
echo " 0 3.0" >> rop_ngb.raw
echo "1.0" >> rop_ngb.raw
load rop_ngb.raw
set c3 = 99
strcmp c3 "$ngbehavior" "hs"
if $c3 <> 0
  echo "ERROR: harness broken, the Option: ngbehavior pair is not readable after the load"
  quit 1
end
let hs1 = 99
source rop_inner.cir
let hs1 = v(hsn)
if hs1 <> 1
  echo "ERROR: a loaded Option: ngbehavior=hs changed how the next deck was parsed"
  quit 1
end

* --- 5. an ordinary name is still answered, which is shape A ----------------
* The narrowing is about which reads the plot environment may answer, not
* about which keys may be filed: a name no netlist read consults is readable
* exactly as before, and a fix that refused keys at the Option: arm would
* fail here.
echo "Title: hand built for doc/codex/issues/0061" > rop_plain.raw
echo "Date: Tue Aug 11 00:00:00  2026" >> rop_plain.raw
echo "Plotname: Operating Point" >> rop_plain.raw
echo "Option: ropprobe=readback" >> rop_plain.raw
echo "Flags: real" >> rop_plain.raw
echo "No. Variables: 2" >> rop_plain.raw
echo "No. Points: 1" >> rop_plain.raw
echo "Variables:" >> rop_plain.raw
echo " 0 v(1) voltage" >> rop_plain.raw
echo " 1 OUT voltage" >> rop_plain.raw
echo "Values:" >> rop_plain.raw
echo " 0 3.0" >> rop_plain.raw
echo "1.0" >> rop_plain.raw
load rop_plain.raw
set c4 = 99
strcmp c4 "$ropprobe" "readback"
if $c4 <> 0
  echo "ERROR: a plot environment key is no longer readable after a load"
  quit 1
end

echo "INFO: a loaded Option: line stayed readable and steered no netlist read"
quit 0
