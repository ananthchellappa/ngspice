* Simulator-interface keywords typed interactively must be recognised
* whatever their case. src/frontend/spiceif.c compares three families of
* keyword with case sensitive eq()/strcmp():
*
*   - the .options names handled directly by if_option()
*   - the "unsupported"/"obsolete" .options name tables
*   - the analysis parameter keywords looked up by ft_find_analysis_parm()
*   - the "all" wildcard and the model parameter keywords used by
*     spif_getparam_special()/parmlookup()
*
* A deck cannot express this: inp_read() lowercases every raw card before
* the card structure exists, so ".OPTIONS ITL1=200" already reaches
* if_option() as "itl1". The cp shell input path does not go through that
* fold, so "set ITL1 = 200" and "let x = @rmod[TC1]" typed on stdin are the
* only way to put an uppercase keyword in front of spiceif.c.
*
* The circuit is built with circbyline so the test needs no companion .cir;
* those lines are deck lines and are folded, which keeps the case of the
* typed command as the only variable.
*
* Every check is fail-fast with an explicit "quit 1", and every assertion is
* on a value or on a state that can be read back ("$?name" for the cp
* variable list, a vector value for everything else). Vector reads are
* seeded immediately before the read, in the plot they are read from, so a
* command that produced nothing fails the check instead of reading a stale
* value from an earlier plot.
set prompt = ""

* --- the readback mechanism itself ------------------------------------
* "$?name" must be 1 for a recorded cp variable and 0 for an absent one,
* otherwise the if_option checks below would pass whatever happens.
set spiceif_ctl = 1
if $?spiceif_ctl <> 1
  echo "ERROR: '$?' does not report a variable that was just set"
  quit 1
end
unset spiceif_ctl
if $?spiceif_ctl <> 0
  echo "ERROR: '$?' does not report an absent variable"
  quit 1
end

* The three if_option() checks below all assert an ABSENCE -- the name was
* swallowed by the simulator interface, so it is not in the cp variable
* list. An absence is only evidence if a name that if_option() does NOT
* claim is still recorded when it is spelled in upper case; otherwise any
* future change that drops or folds upper case variable names would turn
* those checks green without spiceif.c having been touched at all.
set SPICEIF_NOTANOPTION = 5
if $?SPICEIF_NOTANOPTION <> 1
  echo "ERROR: an upper case name that is not an .options keyword was not recorded"
  quit 1
end
unset SPICEIF_NOTANOPTION

* --- if_option(), unsupported[] -- spiceif.c eq(name, *vv) -------------
* No circuit is loaded yet, so an .options name that if_option() claims is
* consumed by the simulator interface (return 1) is dropped by cp_vset()
* through US_NOSIMVAR and never reaches the cp variable list. A name that
* if_option() does not recognise falls through as US_OK and IS recorded.
* "itl3" is in the unsupported[] table, so it must be consumed either way.
set itl3 = 5
if $?itl3 <> 0
  echo "ERROR: lowercase itl3 was recorded as a plain cp variable"
  quit 1
end
set ITL3 = 5
if $?ITL3 <> 0
  echo "ERROR: uppercase ITL3 not matched against the unsupported .options table"
  quit 1
end

* --- if_option(), obsolete[] -- spiceif.c eq(name, *vv) ----------------
set limpts = 5
if $?limpts <> 0
  echo "ERROR: lowercase limpts was recorded as a plain cp variable"
  quit 1
end
set LIMPTS = 5
if $?LIMPTS <> 0
  echo "ERROR: uppercase LIMPTS not matched against the obsolete .options table"
  quit 1
end

* --- ft_find_analysis_parm() -- spiceif.c strcmp(...keyword, name) -----
* "itl1" is a real settable option parameter, so with no circuit loaded
* if_option() reports it as a simulation variable and it is dropped. An
* unmatched keyword is recorded instead.
set itl1 = 200
if $?itl1 <> 0
  echo "ERROR: lowercase itl1 was recorded as a plain cp variable"
  quit 1
end
set ITL1 = 200
if $?ITL1 <> 0
  echo "ERROR: uppercase ITL1 not matched against the options parameter keywords"
  quit 1
end

* --- the circuit for the value based checks ----------------------------
* R1 carries a model with tc1, so its value tracks the operating
* temperature: 1k at 27 C, 2k at 127 C, and i(v1) tracks with it.
circbyline * inline divider for the simulator interface keyword checks
circbyline .options noacct
circbyline .model rmod r (tc1=0.01)
circbyline V1 in 0 dc 1
circbyline R1 in 0 1k rmod
circbyline .end

* --- ft_find_analysis_parm(), value path -------------------------------
* "set temp" is the interactive spelling of ".options temp"; it reaches
* ft_find_analysis_parm() with exactly the case that was typed.
set temp = 127
op
let tlo = -1
let tlo = i(v1)
if abs(tlo + 5.0e-4) > 1e-9
  echo "ERROR: lowercase temp did not reach the options parameters, i(v1) = $&tlo"
  quit 1
end
set temp = 27
op
set TEMP = 127
op
let tup = -1
let tup = i(v1)
if abs(tup + 5.0e-4) > 1e-9
  echo "ERROR: uppercase TEMP did not reach the options parameters, i(v1) = $&tup"
  quit 1
end
set temp = 27

* --- instance parameter keyword, already case insensitive --------------
* parmlookup() matches instance parameters with cieq(), so the uppercase
* @dev[PARAM] syntax itself works today. This control proves the model
* parameter check below is not just testing the bracket syntax.
let ilo = -1
let ilo = @r1[RESISTANCE]
if abs(ilo - 1000) > 1e-6
  echo "ERROR: uppercase instance parameter RESISTANCE not read back, got $&ilo"
  quit 1
end

* --- spif_getparam_special() -- eq(param, "all") -----------------------
* "@dev[all]" asks for every parameter of the device; the first one handed
* back is the principal value, the resistance. Without the wildcard the
* lookup falls through to parmlookup(), which has no parameter called
* "ALL", so nothing is produced and the seed survives.
let alo = -1
let alo = @r1[all]
if alo < 0
  echo "ERROR: lowercase @r1[all] produced no value"
  quit 1
end
let aup = -1
let aup = @r1[ALL]
if aup <> alo
  echo "ERROR: uppercase @r1[ALL] wildcard not honoured, got $&aup instead of $&alo"
  quit 1
end

* --- parmlookup() -- eq(dev->modelParms[i].keyword, param) -------------
* rmod is a model name, so the lookup goes through the modelParms branch,
* which is the case sensitive one.
let mlo = -1
let mlo = @rmod[tc1]
if abs(mlo - 0.01) > 1e-12
  echo "ERROR: lowercase model parameter tc1 not read back, got $&mlo"
  quit 1
end
let mup = -1
let mup = @rmod[TC1]
if abs(mup - 0.01) > 1e-12
  echo "ERROR: uppercase model parameter TC1 not matched, got $&mup"
  quit 1
end

echo "INFO: all uppercase simulator interface keyword cases passed"
quit 0
