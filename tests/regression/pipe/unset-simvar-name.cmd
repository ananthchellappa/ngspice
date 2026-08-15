* 'unset' of a simulator option the session set leaves the session alive
* doc/codex/issues/0067, acceptance criteria 1, 4 and 6, US_SIMVAR arm.
*
* A 'set' of a simulator option in a session that has a circuit is filed in
* ft_curckt->ci_vars by cp_vset() (src/frontend/variable.c).  cp_remvar()'s
* US_SIMVAR arm takes it back out of that list and used to free it there --
* and then the tail of the function wrote through the freed node and freed it
* a second time, so 'set temp=27' followed by 'unset temp' aborted the
* process, rc=134, on this build and on ngspice-46 as released.  That is the
* shape a generated deck writes, and the client that reported it as R6 hit it
* from one.  It needs no rawfile: a netlist and two commands are the whole of
* it.
*
* Four options, because the arm is reached by name and not by list: 'temp',
* 'gmin', 'trtol' and 'abstol' are each measured in the issue.  The deck runs
* an analysis afterwards, which is what says the circuit's own variable list
* survived rather than merely that the process did.

set prompt = ""

* No .control block: the circuit is loaded and nothing has run yet, which is
* the state the issue reproduces in.  Lower-case nets so every reference
* below resolves in all three case modes.
echo "unset-simvar-name inner deck" > uns_inner.cir
echo ".options noacct" >> uns_inner.cir
echo "vs in 0 dc 3" >> uns_inner.cir
echo "rl in mid 1k" >> uns_inner.cir
echo "rg mid 0 3k" >> uns_inner.cir
echo ".end" >> uns_inner.cir
source uns_inner.cir

set temp = 27
set gmin = 1e-11
set trtol = 5
set abstol = 1e-11

let n0 = $?temp
if n0 <> 1
  echo "ERROR: harness broken, temp does not answer after the set"
  quit 1
end
set c0 = 99
strcmp c0 "$temp" "27"
if $c0 <> 0
  echo "ERROR: harness broken, the set of temp was not recorded"
  quit 1
end

unset temp
unset gmin
unset trtol
unset abstol

* Both of these walk the circuit's variable list, and 'set' walks every other
* list too; either used to read what the unset had freed.
set
display

let n1 = $?temp
if n1 <> 0
  echo "ERROR: unset temp left the option in the circuit's variable list"
  quit 1
end
let n2 = $?gmin
if n2 <> 0
  echo "ERROR: unset gmin left the option in the circuit's variable list"
  quit 1
end
let n3 = $?trtol
if n3 <> 0
  echo "ERROR: unset trtol left the option in the circuit's variable list"
  quit 1
end
let n4 = $?abstol
if n4 <> 0
  echo "ERROR: unset abstol left the option in the circuit's variable list"
  quit 1
end

* The circuit still runs, which is the list itself and not just the names.
op
let probe = 99
let probe = v(mid)
if probe > 2.3
  echo "ERROR: the circuit did not survive the unsets"
  quit 1
end
if probe < 2.2
  echo "ERROR: the circuit did not survive the unsets"
  quit 1
end

* And the option can be set again afterwards, so the list is still linked and
* not merely readable.
set temp = 30
set c1 = 99
strcmp c1 "$temp" "30"
if $c1 <> 0
  echo "ERROR: a simulator option could not be set again after being unset"
  quit 1
end

echo "INFO: unset of a simulator option removes it and the session survives"
quit 0
