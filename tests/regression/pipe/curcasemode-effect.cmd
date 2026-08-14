* the effective identifier case mode is readable from a session that reads no deck
* doc/codex/issues/0060, acceptance criteria 3 (second and fourth bullets), 5
* and 6.  'casemode' is the request and 'curcasemode' is the effect; the
* request is a writable control variable and the effect is the static
* ng_case_mode (src/frontend/inpcom.c), latched once per netlist read.
*
* This is the shape the issue is about and the only place it is expressible.
* A deck run in batch has an answer available without any of this -- splice a
* three-way 'let CaseProbe / let caseprobe / print' probe into the .control
* block, doc/claude/casemode-distinguish-guide.md section 2 -- but a session
* that reads no deck at all has no probe: an ngspice -p co-process, a
* libngspice session between ngSpice_Circ() calls, a wrapper checking the
* binary before it commits to generating anything.  There the latch ran during
* start-up and never again, so everything typed at this prompt moves the
* request and nothing moves the effect.  tests/regression/case and
* tests/regression/casedist carry the deck halves; this is the prompt half,
* and the pipe harness (tests/regression/pipe/Makefile.am) asserts by exit
* code, so every check below is fail-fast with an explicit 'quit 1'.
*
* Every check is a comparison against a literal mode name rather than against
* $casemode, because the whole point is that the two differ; and every check
* that says "the effect did not move" is preceded by a control that says the
* request DID move, so a build in which neither variable existed could not
* pass the pair.  The strcmp result variables are preseeded, so a check whose
* strcmp never ran -- the shape a read that errors out produces, because the
* failed substitution drops the word and strcmp then has too few args -- fails
* rather than passing on a stale zero.

set prompt = ""

* --- 1. no casemode variable at all -----------------------------------------
* The effective mode here is the static initialiser, NG_CASE_FOLD, and the
* answer must be 'fold'.  It must NOT be 'no such variable': the absence of
* the request is itself one of the four divergence shapes -- a binary with the
* feature, defaulting to fold, where the enum is the only object there is --
* and reproducing that absence would leave the caller unable to tell it from
* criterion 4's featureless binary.
let n_req_unset = $?casemode
if n_req_unset <> 0
  echo "ERROR: harness broken, casemode is already set at session start"
  quit 1
end
set d1 = 99
strcmp d1 "$curcasemode" "fold"
if $d1 <> 0
  echo "ERROR: with no casemode set, curcasemode did not read back fold"
  quit 1
end

* --- 2. a request typed at the prompt, with no netlist read after it ---------
set casemode=distinguish
set d2req = 99
strcmp d2req "$casemode" "distinguish"
if $d2req <> 0
  echo "ERROR: harness broken, set casemode=distinguish did not take"
  quit 1
end
set d2 = 99
strcmp d2 "$curcasemode" "fold"
if $d2 <> 0
  echo "ERROR: a prompt set casemode=distinguish moved curcasemode"
  quit 1
end

* --- 3. a source, which IS a netlist read, moves it -------------------------
echo "* a sub-deck sourced from the prompt to re-latch the case mode" > ccp_inner.cir
echo ".OPTIONS noacct" >> ccp_inner.cir
echo "V1 In 0 dc 3" >> ccp_inner.cir
echo "R1 In Mid 1k" >> ccp_inner.cir
echo "R2 Mid 0 3k" >> ccp_inner.cir
echo ".end" >> ccp_inner.cir
source ccp_inner.cir
set d3 = 99
strcmp d3 "$curcasemode" "distinguish"
if $d3 <> 0
  echo "ERROR: sourcing a deck did not re-latch curcasemode to distinguish"
  quit 1
end

* --- 4. and the request moving back, with no read behind it, does not --------
* This is the direction in which the binary used to contradict itself two
* lines apart: the mode in force is distinguish, $casemode answers fold.
set casemode=fold
set d4req = 99
strcmp d4req "$casemode" "fold"
if $d4req <> 0
  echo "ERROR: harness broken, set casemode=fold did not take"
  quit 1
end
set d4 = 99
strcmp d4 "$curcasemode" "distinguish"
if $d4 <> 0
  echo "ERROR: a prompt set casemode=fold undid a latched distinguish"
  quit 1
end

* --- 5. the effect is not writable in its own name --------------------------
* A write is refused through cp_usrset()'s existing US_READONLY arm, the one
* 'plots' already uses.  The refusal is on stderr, which this harness does not
* read, so the assertion is on the value: had the write been recorded it would
* be in the shell's own variable list, which vareval() searches before
* cp_enqvar(), and the read below would answer 'preserve'.
set curcasemode=preserve
set d5 = 99
strcmp d5 "$curcasemode" "distinguish"
if $d5 <> 0
  echo "ERROR: set curcasemode=preserve was recorded and answered the read"
  quit 1
end

* --- 6. a loaded plot's environment must not answer it ----------------------
* A rawfile's 'Option:' line is parsed straight into plot_cur->pl_env
* (src/frontend/rawfile.c), and cp_enqvar() (src/frontend/options.c) searches
* that environment before anything else, so any key a file names can answer a
* read of that name.  For the mode in force that is an answer about some other
* run: the file below says preserve and this session is distinguish.
* The casemode line in the same Option: is the positive control, and it is
* what stops this check passing for free -- it proves the environment entry
* really did land and really would have answered.  It needs the request to be
* unset first, because vareval() searches the shell's own variables ahead of
* the plot environment.  doc/codex/issues/0061 is the mechanism.
unset casemode
let n_req_gone = $?casemode
if n_req_gone <> 0
  echo "ERROR: harness broken, unset casemode left the request behind"
  quit 1
end
echo "Title: hand built for the curcasemode shadow check" > ccp_shadow.raw
echo "Date: Tue Aug 11 00:00:00  2026" >> ccp_shadow.raw
echo "Plotname: Operating Point" >> ccp_shadow.raw
echo "Option: casemode=preserve curcasemode=preserve" >> ccp_shadow.raw
echo "Flags: real" >> ccp_shadow.raw
echo "No. Variables: 2" >> ccp_shadow.raw
echo "No. Points: 1" >> ccp_shadow.raw
echo "Variables:" >> ccp_shadow.raw
echo " 0 v(1) voltage" >> ccp_shadow.raw
echo " 1 OUT voltage" >> ccp_shadow.raw
echo "Values:" >> ccp_shadow.raw
echo " 0 3.0" >> ccp_shadow.raw
echo "1.0" >> ccp_shadow.raw
load ccp_shadow.raw
set d6ctl = 99
strcmp d6ctl "$casemode" "preserve"
if $d6ctl <> 0
  echo "ERROR: harness broken, the rawfile Option: line did not reach the plot env"
  quit 1
end
set d6 = 99
strcmp d6 "$curcasemode" "distinguish"
if $d6 <> 0
  echo "ERROR: a loaded rawfile's Option: line answered the curcasemode read"
  quit 1
end

quit 0
