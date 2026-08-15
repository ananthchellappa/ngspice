* 'unset' of a key a loaded rawfile carried, on a rawfile this build wrote
* doc/codex/issues/0067, acceptance criteria 2 and 6; doc/codex/issues/0061.
*
* A raw header's 'Option:' line is parsed straight into the loaded plot's
* environment (src/frontend/rawfile.c), and this build can record the case
* mode there: with 'casemodewrite' set, every raw file it writes carries one.
* That turned a defect which had needed a hand-edited header into one
* reachable from any file the simulator writes -- which is also why the
* writer is opt-in, since a released ngspice-46 has the defect and cannot be
* fixed (tests/regression/pipe/rawfile-casemode-header.cmd is the gate).
* 'unset <key>' found the node in that environment, an arm
* of cp_remvar() (src/frontend/variable.c) declined to unlink it, and the tail
* freed it anyway -- leaving the plot holding freed memory.  The 'unset'
* itself returned cleanly; the next command that walked the environment did
* not.  'display', 'set' and 'write' were each a SIGSEGV, rc=139, on this
* build and on ngspice-46 as released.
*
* So this deck asserts the exit status of the commands AFTER the unset, which
* is the whole point of the shape: a deck written around the unset alone
* passes without the fix.  It does not run under valgrind, and must not be
* read as if it did -- that allocator keeps the freed block readable and
* answers rc=0 on the sequence that faults under glibc, which is how the arm
* was mistaken for a latent one for two drafts of the issue.
*
* The file is written by this build rather than hand built with echo, because
* what is being asserted is that an ordinary product of this simulator is
* safe to load and tidy up after.  The mode the header records is the mode in
* force at the write, so the read-back is compared against '$curcasemode' and
* the deck says the same thing in all three case modes.
*
* The outcome asserted for the unset is removal, not refusal: a key that
* arrived in a file the user merely loaded does not make a name read-only for
* the session (doc/codex/issues/0061).  The plot keeps everything else the
* file said, which is checked below.

set prompt = ""

set m0 = "$curcasemode"

* Lower-case nets, so that every reference below resolves in all three modes.
echo "unset-rawfile-option-key inner deck" > uro_inner.cir
echo ".options noacct" >> uro_inner.cir
echo "vs in 0 dc 3" >> uro_inner.cir
echo "rl in mid 1k" >> uro_inner.cir
echo "rg mid 0 3k" >> uro_inner.cir
echo ".control" >> uro_inner.cir
echo "op" >> uro_inner.cir
echo ".endc" >> uro_inner.cir
echo ".end" >> uro_inner.cir
source uro_inner.cir
set filetype = ascii
set casemodewrite
write uro_written.raw

* Nothing in the session may answer 'casemode', so that the read-back after
* the load can only have come out of the file.
unset casemode
let k0 = $?casemode
if k0 <> 0
  echo "ERROR: harness broken, casemode still answers before the load"
  quit 1
end

load uro_written.raw
set p1 = "$curplot"
set t1 = "$curplottitle"

let k1 = $?casemode
if k1 <> 1
  echo "ERROR: harness broken, the raw this build wrote carries no readable Option: key"
  quit 1
end
set c1 = 99
strcmp c1 "$casemode" "$m0"
if $c1 <> 0
  echo "ERROR: harness broken, the header did not record the mode in force"
  quit 1
end

unset casemode

* Each of these three walks the environment the unset took the key out of,
* and each was a SIGSEGV before the ownership rule in cp_remvar().
display
set
write uro_rewritten.raw

let k2 = $?casemode
if k2 <> 0
  echo "ERROR: unset left the key in the loaded plot's environment"
  quit 1
end

* The rest of the loaded plot is untouched: the key went, the plot stayed.
set c2 = 99
strcmp c2 "$curplot" "$p1"
if $c2 <> 0
  echo "ERROR: the unset moved the current plot"
  quit 1
end
set c3 = 99
strcmp c3 "$curplottitle" "$t1"
if $c3 <> 0
  echo "ERROR: the unset changed the loaded plot's title"
  quit 1
end
let probe = 99
let probe = v(mid)
if probe > 2.3
  echo "ERROR: the loaded plot lost its vectors across the unset"
  quit 1
end
if probe < 2.2
  echo "ERROR: the loaded plot lost its vectors across the unset"
  quit 1
end

* The file written after the unset is still a rawfile the reader accepts, and
* it records no mode at all.  Both halves matter.  The plot being written is
* a loaded one, so the mode in force describes the session and not the data,
* and the unset took away the one record of that data's mode there was: a
* writer that filled the gap from the session would be claiming, about names
* it never spelled, whatever this session happens to be set to
* (doc/codex/issues/0070).  Silence is the answer, and a tool that loads,
* tidies and re-writes gets a file that asserts nothing rather than one that
* asserts something untrue.
*
* This deck asserted the opposite until 0070, on the reasoning that the line
* under 'Plotname:' is the writing session's and written afresh every time --
* which is a description of the defect.  The claim kept here is the one that
* belongs to 0067: the file survives the unset, is readable, and still
* carries the data.
load uro_rewritten.raw
let k3 = $?casemode
if k3 <> 0
  echo "ERROR: the rewritten copy of a loaded plot records a mode it cannot know"
  quit 1
end
let probe2 = 99
let probe2 = v(mid)
if probe2 > 2.3
  echo "ERROR: the rewritten file did not read back with its vectors"
  quit 1
end
if probe2 < 2.2
  echo "ERROR: the rewritten file did not read back with its vectors"
  quit 1
end

echo "INFO: a key from a loaded rawfile can be unset and the plot survives"
quit 0
