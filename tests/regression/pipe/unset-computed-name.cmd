* 'unset' of a name the current plot computes leaves the session alive
* doc/codex/issues/0067, acceptance criteria 1, 4, 5 and 6, US_DONTRECORD arm.
*
* 'curplot', 'curplotname', 'curplottitle' and 'curplotdate' are computed by
* cp_enqvar() (src/frontend/options.c) from the current plot on every read;
* nothing stores them.  cp_usrvars() manufactures them into a temporary chain
* which cp_remvar() (src/frontend/variable.c) searches and then frees at the
* end of the call, and cp_usrset() answers US_DONTRECORD for all four -- an
* arm that deliberately leaves the node where it found it.  The tail of
* cp_remvar() freed it anyway, so the chain was freed twice one line later:
* 'unset curplot' alone was an immediate abort, rc=134, on this build and on
* ngspice-46 as released.
*
* This directory is the shape the issue's criterion 6 asks for: no netlist is
* needed, and it is the one place in the suite that can see an exit status.
* A crash therefore fails this deck by itself -- but only if the process gets
* that far, so every check below also reads the name back afterwards, which
* is what says the list the arm declined to unlink is still whole.
*
* Two rounds, because the two are not the same memory.  The first runs before
* any analysis, when the current plot is the built-in 'constants' one whose
* three strings are static initialisers (src/frontend/plotting/plotting.c) --
* 'unset curplotdate' handed Spice_Build_Date to free() there, which the two
* neighbouring arms of cp_usrset() had always guarded against and that one had
* not.  The second runs on a plot an analysis made, whose strings are
* malloced.
*
* Criterion 4's behaviour question is decided here as an assertion and not
* just as an absence of a crash: the four names are refused and survive.  The
* refusal text is on stderr, which this harness does not capture; what the
* deck can see is that the value is unchanged afterwards, and it checks that.

set prompt = ""

* --- 1. before any analysis: the built-in 'constants' plot -----------------
set p0 = "$curplot"
set n0 = "$curplotname"
set t0 = "$curplottitle"
set d0 = "$curplotdate"

unset curplot
unset curplotname
unset curplottitle
unset curplotdate

* 'set' with no argument walks every variable list there is, including the
* chain cp_usrvars() rebuilds for the listing; it is the shortest command
* that reads what the free above had handed back.
set
display

set c1 = 99
strcmp c1 "$curplot" "$p0"
if $c1 <> 0
  echo "ERROR: unset curplot moved the current plot on the constants plot"
  quit 1
end
set c2 = 99
strcmp c2 "$curplotname" "$n0"
if $c2 <> 0
  echo "ERROR: unset curplotname changed the plot name on the constants plot"
  quit 1
end
set c3 = 99
strcmp c3 "$curplottitle" "$t0"
if $c3 <> 0
  echo "ERROR: unset curplottitle changed the plot title on the constants plot"
  quit 1
end
set c4 = 99
strcmp c4 "$curplotdate" "$d0"
if $c4 <> 0
  echo "ERROR: unset curplotdate changed the plot date on the constants plot"
  quit 1
end

* --- 2. and on a plot an analysis made -------------------------------------
* The nets are spelled in lower case so that every reference below resolves
* in all three case modes.
echo "unset-computed-name inner deck" > unc_inner.cir
echo ".options noacct" >> unc_inner.cir
echo "vs in 0 dc 3" >> unc_inner.cir
echo "rl in mid 1k" >> unc_inner.cir
echo "rg mid 0 3k" >> unc_inner.cir
echo ".control" >> unc_inner.cir
echo "op" >> unc_inner.cir
echo ".endc" >> unc_inner.cir
echo ".end" >> unc_inner.cir
source unc_inner.cir

set c5 = 99
strcmp c5 "$curplot" "$p0"
if $c5 = 0
  echo "ERROR: harness broken, the analysis did not make a new current plot"
  quit 1
end

set p1 = "$curplot"
set n1 = "$curplotname"
set t1 = "$curplottitle"
set d1 = "$curplotdate"

unset curplot
unset curplotname
unset curplottitle
unset curplotdate

set
display

set c6 = 99
strcmp c6 "$curplot" "$p1"
if $c6 <> 0
  echo "ERROR: unset curplot moved the current plot"
  quit 1
end
set c7 = 99
strcmp c7 "$curplotname" "$n1"
if $c7 <> 0
  echo "ERROR: unset curplotname changed the plot name"
  quit 1
end
set c8 = 99
strcmp c8 "$curplottitle" "$t1"
if $c8 <> 0
  echo "ERROR: unset curplottitle changed the plot title"
  quit 1
end
set c9 = 99
strcmp c9 "$curplotdate" "$d1"
if $c9 <> 0
  echo "ERROR: unset curplotdate changed the plot date"
  quit 1
end

* The vector the analysis made is still readable, which is the plot itself
* and not just the names it computes.
let probe = 99
let probe = v(mid)
if probe > 2.3
  echo "ERROR: the plot lost its vectors across the unsets"
  quit 1
end
if probe < 2.2
  echo "ERROR: the plot lost its vectors across the unsets"
  quit 1
end

echo "INFO: unset of a computed plot name is refused and the session survives"
quit 0
