* Option-variable keywords typed interactively must be recognised whatever
* their case. src/frontend/options.c compares the name of every built-in
* shell option (cp_usrset) and every built-in plot pseudo-variable
* (cp_enqvar) with eq()/strncmp(), i.e. case sensitively.
*
* A deck cannot express this: inp_read() lowercases each raw line before the
* card structure exists, so a ".control" section can only ever hand options.c
* a lowercased name. The cp shell input path - the prompt, "ngspice -p", and
* ngSpice_Command() - does not go through that fold, so a typed
* "set NOACCT" or a typed "echo @CURPLOT" keeps its case and is the only way
* to put an uppercase option keyword in front of options.c.
*
* Every check below asserts on a value or on state read back from the
* session, never on a diagnostic message, and every check is paired with the
* lowercase spelling so that a check which fails for both spellings is
* visible as a harness fault rather than as a keyword-case fault.
set prompt = ""
set silent_fileio
let fail_count = 0

* --- cp_enqvar: the built-in plot pseudo-variables --------------------
* "curplot", "curplotname", "curplottitle", "curplotdate" (options.c:70,
* 76, 80, 84) and "plots" (options.c:90). The dollar-question notation
* yields 1 when the reference resolves and 0 when it does not, so the
* existence of the built-in under an uppercase spelling is directly a value.
let ep_lo = $?curplot
if ep_lo <> 1
   echo "ERROR: harness: lowercase curplot reference does not resolve"
   let fail_count = fail_count + 1
end
let ep_up = $?CURPLOT
if ep_up <> 1
   echo "ERROR: uppercase CURPLOT not recognised as a built-in plot variable"
   let fail_count = fail_count + 1
end

let en_lo = $?curplotname
if en_lo <> 1
   echo "ERROR: harness: lowercase curplotname reference does not resolve"
   let fail_count = fail_count + 1
end
let en_up = $?CURPLOTNAME
if en_up <> 1
   echo "ERROR: uppercase CURPLOTNAME not recognised as a built-in plot variable"
   let fail_count = fail_count + 1
end

let et_lo = $?curplottitle
if et_lo <> 1
   echo "ERROR: harness: lowercase curplottitle reference does not resolve"
   let fail_count = fail_count + 1
end
let et_up = $?CURPLOTTITLE
if et_up <> 1
   echo "ERROR: uppercase CURPLOTTITLE not recognised as a built-in plot variable"
   let fail_count = fail_count + 1
end

let ed_lo = $?curplotdate
if ed_lo <> 1
   echo "ERROR: harness: lowercase curplotdate reference does not resolve"
   let fail_count = fail_count + 1
end
let ed_up = $?CURPLOTDATE
if ed_up <> 1
   echo "ERROR: uppercase CURPLOTDATE not recognised as a built-in plot variable"
   let fail_count = fail_count + 1
end

let es_lo = $?plots
if es_lo <> 1
   echo "ERROR: harness: lowercase plots reference does not resolve"
   let fail_count = fail_count + 1
end
let es_up = $?PLOTS
if es_up <> 1
   echo "ERROR: uppercase PLOTS not recognised as a built-in plot variable"
   let fail_count = fail_count + 1
end

* --- cp_usrset "curplot" (options.c:378) ------------------------------
* Setting curplot switches the current plot. Two throwaway plots are made
* first so that the switch back to the constants plot is a real change of
* state; the switch is read back through the lowercase reference, which
* already works today, so the case of the typed "set" is the only variable.
setplot new
setplot new
set CURPLOT = const
let sp_up = 99
strcmp sp_r1 "$curplot" "const"
let sp_up = $sp_r1
if sp_up <> 0
   echo "ERROR: uppercase CURPLOT not honoured by set, current plot unchanged"
   let fail_count = fail_count + 1
end
set curplot = const
let sp_lo = 99
strcmp sp_r2 "$curplot" "const"
let sp_lo = $sp_r2
if sp_lo <> 0
   echo "ERROR: harness: lowercase curplot not honoured by set"
   let fail_count = fail_count + 1
end

* --- cp_usrset "curplotname" (options.c:384) --------------------------
* Renaming is done on a throwaway plot so the constants plot keeps its
* special-cased name and title.
setplot unknown1
set CURPLOTNAME = qqname
let nm_up = 99
strcmp nm_r1 "$curplotname" "qqname"
let nm_up = $nm_r1
if nm_up <> 0
   echo "ERROR: uppercase CURPLOTNAME not honoured by set, plot name unchanged"
   let fail_count = fail_count + 1
end
set curplotname = qqname
let nm_lo = 99
strcmp nm_r2 "$curplotname" "qqname"
let nm_lo = $nm_r2
if nm_lo <> 0
   echo "ERROR: harness: lowercase curplotname not honoured by set"
   let fail_count = fail_count + 1
end

* --- cp_usrset "curplottitle" (options.c:393) -------------------------
set CURPLOTTITLE = qqtitle
let ti_up = 99
strcmp ti_r1 "$curplottitle" "qqtitle"
let ti_up = $ti_r1
if ti_up <> 0
   echo "ERROR: uppercase CURPLOTTITLE not honoured by set, plot title unchanged"
   let fail_count = fail_count + 1
end
set curplottitle = qqtitle
let ti_lo = 99
strcmp ti_r2 "$curplottitle" "qqtitle"
let ti_lo = $ti_r2
if ti_lo <> 0
   echo "ERROR: harness: lowercase curplottitle not honoured by set"
   let fail_count = fail_count + 1
end

* --- cp_usrset "curplotdate" (options.c:402) --------------------------
set CURPLOTDATE = qqdate
let da_up = 99
strcmp da_r1 "$curplotdate" "qqdate"
let da_up = $da_r1
if da_up <> 0
   echo "ERROR: uppercase CURPLOTDATE not honoured by set, plot date unchanged"
   let fail_count = fail_count + 1
end
set curplotdate = qqdate
let da_lo = 99
strcmp da_r2 "$curplotdate" "qqdate"
let da_lo = $da_r2
if da_lo <> 0
   echo "ERROR: harness: lowercase curplotdate not honoured by set"
   let fail_count = fail_count + 1
end

* --- cp_usrset "plots" (options.c:410) --------------------------------
* "plots" is the one variable cp_usrset refuses to overwrite. When the name
* is not matched the assignment is recorded as an ordinary shell variable
* instead, and the built-in list of plot names disappears behind it: the
* element count of the reference drops from the number of plots to 1.
* Three plots exist at this point, so a count below 2 means the uppercase
* spelling was accepted as writable. The seed is 0, a failing value, so a
* reference that does not resolve at all still fails the check.
let pl_ref = 0
let pl_ref = $#plots
if pl_ref < 2
   echo "ERROR: harness: fewer than two plots exist for the plots readonly check"
   let fail_count = fail_count + 1
end
set plots = zzz
let pl_lo = 0
let pl_lo = $#plots
if pl_lo < 2
   echo "ERROR: harness: lowercase plots was not rejected as read-only"
   let fail_count = fail_count + 1
end
set PLOTS = zzz
let pl_up = 0
let pl_up = $#PLOTS
if pl_up < 2
   echo "ERROR: uppercase PLOTS accepted by set instead of being read-only"
   let fail_count = fail_count + 1
end

* --- cp_usrset "units" (options.c:373) --------------------------------
* units=degrees makes cx_degrees TRUE, which turns the result of ph() from
* radians into degrees. The RC divider below is at its corner frequency, so
* the phase of v(out) is -0.785398 rad, i.e. -45 degrees. The default is
* radians, and this is the first place in the file that touches units.
circbyline * inline RC divider for the units check
circbyline .options noacct
circbyline V1 in 0 dc 1 ac 1
circbyline R1 in out 1k
circbyline C1 out 0 159.155n
circbyline .end
ac lin 1 1000 1000
set UNITS = degrees
let un_up = 99
let un_up = ph(v(out))
if abs(un_up + 45) > 0.5
   echo "ERROR: uppercase UNITS not honoured by set, ph() still in radians"
   let fail_count = fail_count + 1
end
set units = radians
let un_rad = 99
let un_rad = ph(v(out))
if abs(un_rad + 0.785398) > 0.001
   echo "ERROR: harness: units=radians does not give ph() in radians"
   let fail_count = fail_count + 1
end
set units = degrees
let un_lo = 99
let un_lo = ph(v(out))
if abs(un_lo + 45) > 0.5
   echo "ERROR: harness: lowercase units not honoured by set"
   let fail_count = fail_count + 1
end
set units = radians

* --- cp_usrset "strictnumparse" (options.c:327) -----------------------
* With ft_strictnumparse set, a number followed by anything other than '_'
* is rejected, so the right hand side below fails to parse and the seed
* survives. Without it the trailing letters are simply eaten and the value
* becomes 1.5.
set STRICTNUMPARSE
let sn_up = 99
let sn_up = 1.5xyz
unset STRICTNUMPARSE
if sn_up <> 99
   echo "ERROR: uppercase STRICTNUMPARSE not honoured by set, 1.5xyz still parsed"
   let fail_count = fail_count + 1
end
set strictnumparse
let sn_lo = 99
let sn_lo = 1.5xyz
unset strictnumparse
if sn_lo <> 99
   echo "ERROR: harness: lowercase strictnumparse did not reject 1.5xyz"
   let fail_count = fail_count + 1
end
let sn_off = 99
let sn_off = 1.5xyz
if abs(sn_off - 1.5) > 0.001
   echo "ERROR: harness: 1.5xyz is rejected even with strictnumparse unset"
   let fail_count = fail_count + 1
end

* --- cp_usrset "rawfile" (options.c:303) ------------------------------
* ft_rawfile is the file that "load" reads when no name is given
* (postcoms.c com_load). The probe value is written to a named file, then
* changed in place, then the bare "load" has to bring the written value
* back as the current plot. If the name was not honoured the bare load
* looks for rawspice.raw and the changed value stays current.
shell rm -f optcase-keyword.raw optcase-keyword.tmp
set filetype = ascii
setplot new
let rp = 1234
write optcase-keyword.raw
let rp = 5678
set RAWFILE = optcase-keyword.raw
load
let rf_up = 99
let rf_up = rp
if abs(rf_up - 1234) > 0.5
   echo "ERROR: uppercase RAWFILE not honoured by set, bare load read another file"
   let fail_count = fail_count + 1
end
set rawfile = optcase-keyword.raw
load
let rf_lo = 99
let rf_lo = rp
if abs(rf_lo - 1234) > 0.5
   echo "ERROR: harness: lowercase rawfile not honoured by bare load"
   let fail_count = fail_count + 1
end

* --- cp_usrset "rawfileprec" (options.c:336) --------------------------
* raw_prec is the number of digits an ascii rawfile is written with, so a
* value written with rawfileprec=1 and read back again is 1.2 instead of
* 1.23456789.
setplot new
let pv = 1.23456789
set RAWFILEPREC = 1
write optcase-keyword.raw
load optcase-keyword.raw
let pr_up = 99
let pr_up = pv
if abs(pr_up - 1.2) > 0.001
   echo "ERROR: uppercase RAWFILEPREC not honoured by set, full precision written"
   let fail_count = fail_count + 1
end
setplot new
let pw = 1.23456789
set rawfileprec = 1
write optcase-keyword.raw
load optcase-keyword.raw
let pr_lo = 99
let pr_lo = pw
if abs(pr_lo - 1.2) > 0.001
   echo "ERROR: harness: lowercase rawfileprec did not truncate the written value"
   let fail_count = fail_count + 1
end

* --- cp_usrset "unixcom" (options.c:364) ------------------------------
* With cp_dounixcom set, a word that is not an ngspice command is run as an
* operating system command (control.c cp_unixcom), so "touch" creates a
* file. The file is the read-back state: fopen returns its descriptor, or
* -1 when the command never ran. The marker is removed through the "shell"
* command, which does not depend on unixcom, so the starting state is the
* same on every run.
shell rm -f optcase-keyword.tmp
set UNIXCOM
touch optcase-keyword.tmp
unset UNIXCOM
fopen ux_fh1 optcase-keyword.tmp r
let ux_up = -1
let ux_up = $ux_fh1
if ux_up < 0
   echo "ERROR: uppercase UNIXCOM not honoured by set, touch did not run"
   let fail_count = fail_count + 1
end
shell rm -f optcase-keyword.tmp
set unixcom
touch optcase-keyword.tmp
unset unixcom
fopen ux_fh2 optcase-keyword.tmp r
let ux_lo = -1
let ux_lo = $ux_fh2
if ux_lo < 0
   echo "ERROR: harness: lowercase unixcom did not let touch run"
   let fail_count = fail_count + 1
end
* --- cp_usrset "numdgt" (options.c:355) -------------------------------
* cp_numdgt is the digit count printnum() uses, and wrdata writes its
* columns through printnum(), so the digits in the written file are data
* and not a diagnostic. The file is written at four digits first: that
* proves wrdata works and leaves content which does not match the twelve
* digit pattern, so a wrdata that never runs cannot make either check
* below pass.
shell rm -f optcase-numdgt.txt optcase-numdgt.m0 optcase-numdgt.m1 optcase-numdgt.m2
setplot new
let nd_v = 1.23456789012
set numdgt = 4
wrdata optcase-numdgt.txt nd_v
shell sh -c "grep -q 1.2346e optcase-numdgt.txt && touch optcase-numdgt.m0"
fopen nd_fh0 optcase-numdgt.m0 r
let nd_base = -1
let nd_base = $nd_fh0
if nd_base < 0
   echo "ERROR: harness: wrdata did not write the four digit reference file"
   let fail_count = fail_count + 1
end
set NUMDGT = 12
wrdata optcase-numdgt.txt nd_v
shell sh -c "grep -q 234567890 optcase-numdgt.txt && touch optcase-numdgt.m1"
fopen nd_fh1 optcase-numdgt.m1 r
let nd_up = -1
let nd_up = $nd_fh1
if nd_up < 0
   echo "ERROR: uppercase NUMDGT not honoured by set, wrdata kept four digits"
   let fail_count = fail_count + 1
end
set numdgt = 4
wrdata optcase-numdgt.txt nd_v
set numdgt = 12
wrdata optcase-numdgt.txt nd_v
shell sh -c "grep -q 234567890 optcase-numdgt.txt && touch optcase-numdgt.m2"
fopen nd_fh2 optcase-numdgt.m2 r
let nd_lo = -1
let nd_lo = $nd_fh2
if nd_lo < 0
   echo "ERROR: harness: lowercase numdgt did not widen the written value"
   let fail_count = fail_count + 1
end
set numdgt = 6

* --- cp_usrset "debug" (options.c:284) --------------------------------
* "set debug" in its boolean form sets every debug flag at once, ft_evdb
* among them, and ft_evdb stops ft_evaluate() from naming an expression
* result after the expression itself (evaluate.c:80). The second vector
* of the rawfile below is therefore called "abs(db_a)" while the flag is
* clear and "-(db_a)" while it is set, so the name inside the file is
* state and not a diagnostic. The boolean form can be cleared again with
* "unset", which the string form cannot, so this check runs before the
* "eval" class check below and leaves ft_evdb clear for it.
shell rm -f optcase-debug.raw optcase-debug.m0 optcase-debug.m1 optcase-debug.m2
set filetype = ascii
setplot new
let db_a = 2
write optcase-debug.raw abs(db_a)
shell sh -c "grep -q abs optcase-debug.raw && touch optcase-debug.m0"
fopen db_fh0 optcase-debug.m0 r
let db_base = -1
let db_base = $db_fh0
if db_base < 0
   echo "ERROR: harness: expression vector is not named after the expression"
   let fail_count = fail_count + 1
end
set DEBUG
write optcase-debug.raw abs(db_a)
unset DEBUG
shell grep -q abs optcase-debug.raw || touch optcase-debug.m1
fopen db_fh1 optcase-debug.m1 r
let db_up = -1
let db_up = $db_fh1
if db_up < 0
   echo "ERROR: uppercase DEBUG not matched by set, debug flags unchanged"
   let fail_count = fail_count + 1
end
set debug
write optcase-debug.raw abs(db_a)
unset debug
shell grep -q abs optcase-debug.raw || touch optcase-debug.m2
fopen db_fh2 optcase-debug.m2 r
let db_lo = -1
let db_lo = $db_fh2
if db_lo < 0
   echo "ERROR: harness: lowercase debug boolean did not set the debug flags"
   let fail_count = fail_count + 1
end

* --- setdb "eval" (options.c:477) -------------------------------------
* The debug class name of "set debug = <class>" reaches setdb() straight
* from the typed line, so it keeps its case. The "eval" class sets
* ft_evdb, and ft_evdb stops ft_evaluate() from naming the result of an
* expression after the expression itself (evaluate.c:80): the second
* vector of the rawfile below is called "abs(ev_a)" while ft_evdb is
* clear and "-(ev_a)" once it is set, so the name inside the file is
* state and not a diagnostic. The first grep is the baseline: it proves
* the file was written and that "abs" is there to begin with, so a write
* that never happened cannot make the two later checks pass. ft_evdb can
* never be cleared again - "unset debug" runs setdb() a second time - so
* this is the last check in the file.
shell rm -f optcase-evdb.raw optcase-evdb.m0 optcase-evdb.m1 optcase-evdb.m2
set filetype = ascii
setplot new
let ev_a = 2
write optcase-evdb.raw abs(ev_a)
shell sh -c "grep -q abs optcase-evdb.raw && touch optcase-evdb.m0"
fopen ev_fh0 optcase-evdb.m0 r
let ev_base = -1
let ev_base = $ev_fh0
if ev_base < 0
   echo "ERROR: harness: expression vector is not named after the expression"
   let fail_count = fail_count + 1
end
set debug = EVAL
write optcase-evdb.raw abs(ev_a)
shell grep -q abs optcase-evdb.raw || touch optcase-evdb.m1
fopen ev_fh1 optcase-evdb.m1 r
let ev_up = -1
let ev_up = $ev_fh1
if ev_up < 0
   echo "ERROR: uppercase EVAL debug class not matched by setdb"
   let fail_count = fail_count + 1
end
set debug = eval
write optcase-evdb.raw abs(ev_a)
shell grep -q abs optcase-evdb.raw || touch optcase-evdb.m2
fopen ev_fh2 optcase-evdb.m2 r
let ev_lo = -1
let ev_lo = $ev_fh2
if ev_lo < 0
   echo "ERROR: harness: lowercase eval debug class not honoured"
   let fail_count = fail_count + 1
end

shell rm -f optcase-keyword.raw optcase-keyword.tmp
shell rm -f optcase-evdb.raw optcase-evdb.m0 optcase-evdb.m1 optcase-evdb.m2
shell rm -f optcase-numdgt.txt optcase-numdgt.m0 optcase-numdgt.m1 optcase-numdgt.m2
shell rm -f optcase-debug.raw optcase-debug.m0 optcase-debug.m1 optcase-debug.m2

if fail_count > 0
  echo "ERROR: $&fail_count uppercase option keyword cases failed"
  quit 1
else
  echo "INFO: all uppercase option keyword cases passed"
  quit 0
end
