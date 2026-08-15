* 'unset' of a read-only name leaves the session alive
* doc/codex/issues/0067, acceptance criteria 1, 4, 5 and 6, US_READONLY arm.
*
* cp_usrset() (src/frontend/options.c) answers US_READONLY for two names it
* computes and never stores: 'plots', and 'curcasemode', which
* doc/codex/issues/0060 added.  The arm refuses the removal and leaves the
* node where the search found it -- 'plots' on the temporary chain
* cp_usrvars() built for this call, which cp_remvar() frees at the end of it.
* The tail freed that node as well, so 'unset plots' alone aborted the
* process, rc=134, on this build and on ngspice-46 as released.
*
* The third source of US_READONLY was every key a loaded rawfile's 'Option:'
* line put in the current plot's environment.  That is gone: a file the user
* merely loaded no longer decides what the session may set or unset
* (doc/codex/issues/0061), so the two computed names below are now the whole
* of this arm.  tests/regression/pipe/unset-rawfile-option-key.cmd is where
* the rawfile shape is asserted, and it asserts the other outcome -- the key
* is removed -- for the same reason.
*
* What the deck can see is the exit status and the value afterwards; the
* refusal itself goes to stderr, which this harness does not capture.

set prompt = ""

set m0 = "$curcasemode"
let n0 = $?plots
if n0 <> 1
  echo "ERROR: harness broken, plots does not answer before the unset"
  quit 1
end

unset plots
unset curcasemode

* 'set' with no argument and 'display' both walk the lists the arm declined
* to unlink from; either used to read freed memory.
set
display

let n1 = $?plots
if n1 <> 1
  echo "ERROR: unset plots removed the computed plot list"
  quit 1
end
set c1 = 99
strcmp c1 "$curcasemode" "$m0"
if $c1 <> 0
  echo "ERROR: unset curcasemode moved the case mode in force"
  quit 1
end

* The plot list is still walkable and still names the plot we are on: this
* reads the list itself rather than the fact that a name answers.
setplot $curplot
set c2 = 99
strcmp c2 "$curplot" "const"
if $c2 <> 0
  echo "ERROR: the plot list no longer names the current plot"
  quit 1
end

echo "INFO: unset of a read-only name is refused and the session survives"
quit 0
