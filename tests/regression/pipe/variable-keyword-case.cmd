* The built-in option variables (noglob, noclobber, ...) and the vector
* type names used by settype must be recognised whatever their case.
* A deck cannot express this: every card is lowercased in place by
* inp_read() before the card structure exists, so ".control / set NOGLOB"
* already reaches cp_vset() as "noglob". The cp shell input path used here
* is never folded, so a typed "set NOGLOB" is the only way to put an
* uppercase option name in front of update_option_variables()
* (src/frontend/variable.c) or an uppercase type name in front of
* ft_typenum_x() (src/frontend/typesdef.c).
*
* Every check below reads back a value - a variable that is or is not set,
* or a vector that is or is not reachable - never a diagnostic message,
* and every check is repeated in lower case so that a check which fails
* for both spellings is caught here rather than mistaken for a real find.
set prompt = ""
let fail_count = 0

* --- variable.c update_option_variables(), eq(sz_rest, "glob") ---------
* cp_doglob() strips a single-term brace group, so "set {gvar}" really
* sets a variable called gvar. With noglob in force the braces survive and
* the variable that gets set is the one literally called "{gvar}", so gvar
* stays unset. $?gvar reports that as 1 or 0, which is a value.
* Positive control first: with globbing on "set {gvar}" really does set
* gvar. Both checks below pass when gvar is NOT set, so without this
* control they would both pass for free if brace globbing ever stopped
* setting gvar for a reason that has nothing to do with case.
unset gvar
set {gvar}
let n_glob_on = $?gvar
unset gvar
if n_glob_on <> 1
  echo "ERROR: harness broken, brace globbing does not set gvar"
  let fail_count = fail_count + 1
end

unset gvar
set NOGLOB
set {gvar}
let n_glob = $?gvar
unset NOGLOB
unset gvar
if n_glob <> 0
  echo "ERROR: uppercase NOGLOB did not turn brace globbing off"
  let fail_count = fail_count + 1
end

set noglob
set {gvar}
let n_glob_lc = $?gvar
unset noglob
unset gvar
if n_glob_lc <> 0
  echo "ERROR: lower case noglob did not turn brace globbing off"
  let fail_count = fail_count + 1
end

* --- variable.c update_option_variables(), eq(sz_rest, "clobber") ------
* With noclobber in force cp_redirect() refuses to open an output file
* that already exists and the whole command is dropped, so the variable
* the command would have set is never set. The redirection that creates
* the file has to run while noclobber is still off.
echo seed > variable-keyword-case.tmp
* Positive control first, for the same reason as above: with clobbering
* allowed the redirection succeeds, the command behind it runs and ncvar
* really does get set. Both checks below pass when ncvar is not set.
unset ncvar
set ncvar = 1 > variable-keyword-case.tmp
let n_clob_on = $?ncvar
unset ncvar
if n_clob_on <> 1
  echo "ERROR: harness broken, redirection to an existing file was refused"
  let fail_count = fail_count + 1
end

unset ncvar
set NOCLOBBER
set ncvar = 1 > variable-keyword-case.tmp
unset NOCLOBBER
let n_clob = $?ncvar
unset ncvar
if n_clob <> 0
  echo "ERROR: uppercase NOCLOBBER did not protect an existing file"
  let fail_count = fail_count + 1
end

set noclobber
set ncvar = 1 > variable-keyword-case.tmp
unset noclobber
let n_clob_lc = $?ncvar
unset ncvar
if n_clob_lc <> 0
  echo "ERROR: lower case noclobber did not protect an existing file"
  let fail_count = fail_count + 1
end

* --- variable.c vareval(), eq(v->va_name, "argv") ----------------------
* $1 is argv[1]. The scan that looks for argv walks the user's own
* variable list, whose names keep whatever case they were typed with, so
* a list set as ARGV has to be found too. When it is not found $1 expands
* to nothing, "let n_argv =" is rejected and the seeded 99 survives.
unset argv
unset ARGV
set ARGV = ( 11 22 33 )
let n_argv = 99
let n_argv = $1
unset ARGV
if n_argv <> 11
  echo "ERROR: positional parameter did not find the list set as ARGV"
  let fail_count = fail_count + 1
end

unset argv
set argv = ( 11 22 33 )
let n_argv_lc = 99
let n_argv_lc = $1
unset argv
if n_argv_lc <> 11
  echo "ERROR: positional parameter did not find the list set as argv"
  let fail_count = fail_count + 1
end

* --- typesdef.c ft_typenum_x(), eq(type, types[i].t_name) --------------
* The type a vector carries is read back through the raw file: rawfile.c
* writes a vector of type current as "i(name)", so after a write/load
* round trip the typed vector answers to i(name) and carries its value,
* while an untyped companion vector keeps its plain name. Both reads are
* positive - a wrong or missing type makes the read fail and the seeded
* 99 survive - so nothing here passes just because a command failed.
setplot new
let st_typed = 5
let st_plain = 3
settype CURRENT st_typed
write variable-keyword-case-1.raw st_typed st_plain
load variable-keyword-case-1.raw
let st_check = 99
let st_check = st_plain
if st_check <> 3
  echo "ERROR: settype harness broken, reloaded plot has no st_plain"
  let fail_count = fail_count + 1
end
let st_named = 99
let st_named = i(st_typed)
if st_named <> 5
  echo "ERROR: uppercase CURRENT not accepted as a settype vector type"
  let fail_count = fail_count + 1
end

setplot new
let st2_typed = 5
let st2_plain = 3
settype current st2_typed
write variable-keyword-case-2.raw st2_typed st2_plain
load variable-keyword-case-2.raw
let st2_check = 99
let st2_check = st2_plain
if st2_check <> 3
  echo "ERROR: settype harness broken, reloaded plot has no st2_plain"
  let fail_count = fail_count + 1
end
let st2_named = 99
let st2_named = i(st2_typed)
if st2_named <> 5
  echo "ERROR: lower case current not accepted as a settype vector type"
  let fail_count = fail_count + 1
end

shell rm -f variable-keyword-case*.tmp variable-keyword-case*.raw

if fail_count > 0
  echo "ERROR: $&fail_count uppercase option/type keyword cases failed"
  quit 1
else
  echo "INFO: all uppercase option/type keyword cases passed"
  quit 0
end
