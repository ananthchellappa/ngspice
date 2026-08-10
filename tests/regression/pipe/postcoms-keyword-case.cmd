* Post-processing keywords typed interactively must be recognised whatever
* their case. A deck cannot express this: inp_read() (src/frontend/inpcom.c)
* lowercases every raw deck line in place before the card structure exists, so
* a ".save SPEEDCHECK" card already reaches outitf.c as "speedcheck" and passes
* today. The cp shell input path -- the prompt, "ngspice -p", ngSpice_Command()
* -- never goes through that fold, so stdin is the only way to put an uppercase
* post-processing keyword in front of src/frontend/postcoms.c and outitf.c.
*
* The circuits are built with circbyline so this test needs no companion .cir
* file; those lines are deck lines and are folded, which keeps the case of the
* typed command as the only variable.
*
* "print" writes to the terminal, so every print check redirects into a file
* and reads it back with fopen/fread: the assertions are on the shape of the
* data that was produced, never on a diagnostic message. Accumulator vectors
* are seeded first, so a command that failed and left nothing behind still
* fails its assertion instead of reading a stale value. Each check is fail-fast
* with an explicit "quit 1", and every check is done in the lowercase spelling
* first so that a check which fails for both spellings cannot hide here.

set prompt = ""
* keep the "print col" column layout independent of the plot's scale vector
set noprintscale

* --- com_print "col", postcoms.c:141 -----------------------------------
* A one-point vector prints in line mode by default (one single line), so a
* column listing in the file can only come from the leading word having been
* taken as the "col" option. If it is not taken as the option it is parsed as
* a vector name instead, the expression list fails and the file stays empty.
* Asserting that a fourth, non-empty line exists rules out both outcomes.
let pkc_scalar = 5

print col pkc_scalar > pkc_col_lo.txt
fopen pkc_fh pkc_col_lo.txt r
fread pkc_l $pkc_fh pkc_n1
fread pkc_l $pkc_fh pkc_n2
fread pkc_l $pkc_fh pkc_n3
fread pkc_l $pkc_fh pkc_n4
fclose $pkc_fh
let col_lo = 0
let col_lo = $pkc_n4
if col_lo < 1
  echo "ERROR: lowercase 'col' did not make print use column mode"
  quit 1
end

print COL pkc_scalar > pkc_col_up.txt
fopen pkc_fh pkc_col_up.txt r
fread pkc_l $pkc_fh pkc_n1
fread pkc_l $pkc_fh pkc_n2
fread pkc_l $pkc_fh pkc_n3
fread pkc_l $pkc_fh pkc_n4
fclose $pkc_fh
let col_up = 0
let col_up = $pkc_n4
if col_up < 1
  echo "ERROR: uppercase COL not recognised by print, no column listing produced"
  quit 1
end

* --- com_print "line", postcoms.c:145 ----------------------------------
* The mirror case: a three-point vector prints in column mode by default
* (eight lines), so exactly one line in the file can only come from the
* leading word having been taken as the "line" option. An unrecognised
* option word again leaves the file empty.
let pkc_vec3 = vector(3)

print line pkc_vec3 > pkc_line_lo.txt
fopen pkc_fh pkc_line_lo.txt r
fread pkc_l $pkc_fh pkc_m1
fread pkc_l $pkc_fh pkc_m2
fclose $pkc_fh
let lin1_lo = 0
let lin2_lo = 0
let lin1_lo = $pkc_m1
let lin2_lo = $pkc_m2
if lin1_lo < 1
  echo "ERROR: lowercase 'line' produced no output at all"
  quit 1
end
if lin2_lo <> -1
  echo "ERROR: lowercase 'line' did not make print use line mode"
  quit 1
end

print LINE pkc_vec3 > pkc_line_up.txt
fopen pkc_fh pkc_line_up.txt r
fread pkc_l $pkc_fh pkc_m1
fread pkc_l $pkc_fh pkc_m2
fclose $pkc_fh
let lin1_up = 0
let lin2_up = 0
let lin1_up = $pkc_m1
let lin2_up = $pkc_m2
if lin1_up < 1
  echo "ERROR: uppercase LINE not recognised by print, no output produced"
  quit 1
end
if lin2_up <> -1
  echo "ERROR: uppercase LINE not recognised by print, output is not line mode"
  quit 1
end

* --- com_write "ascii" filetype, postcoms.c:593 ------------------------
* setcs keeps the case of the value, so "filetype" can carry an uppercase
* ASCII into com_write. The rawfile itself is the read-back: its eleventh
* line is the "Values:" marker of an ASCII rawfile or the "Binary:" marker
* of a binary one (ten header lines: Title, Date, Command, Plotname, Flags,
* No. Variables, No. Points, Variables:, and one line per variable, here the
* scale and pkc_vec3).
setcs filetype = ascii
write pkc_ft_lo.raw pkc_vec3
fopen pkc_fh pkc_ft_lo.raw r
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
strcmp pkc_rc "$pkc_l" "Values:"
fclose $pkc_fh
let ftype_lo = 99
let ftype_lo = $pkc_rc
if ftype_lo <> 0
  echo "ERROR: lowercase filetype 'ascii' did not produce an ASCII rawfile"
  quit 1
end

setcs filetype = ASCII
write pkc_ft_up.raw pkc_vec3
fopen pkc_fh pkc_ft_up.raw r
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
fread pkc_l $pkc_fh pkc_k
strcmp pkc_rc "$pkc_l" "Values:"
fclose $pkc_fh
let ftype_up = 99
let ftype_up = $pkc_rc
if ftype_up <> 0
  echo "ERROR: uppercase filetype ASCII not recognised by write, rawfile is not ASCII"
  quit 1
end
unset filetype

* --- com_print reserved "frequency" vector, postcoms.c:349 and 390 -----
* The AC scale is complex with a zero imaginary part; com_print special-cases
* the name "frequency" so that such a vector takes one 16-wide column instead
* of two 32-wide ones. A vector name is not folded when it is created at the
* prompt, so "FREQUENCY" is what a case-sensitive eq() fails on. The read-back
* is the width of the two lines that the special case controls:
*   header line 4 : "Index   " + %-16.15s = 24, against %-32.31s = 40
*   value line 6  : one printed number = 15, against "real,<tab>imag" = 29
circbyline * inline divider for the frequency column checks
circbyline .options noacct
circbyline V1 in 0 ac 1
circbyline R1 in out 1k
circbyline C1 out 0 1n
circbyline .end
ac dec 1 1k 1k

print col frequency > pkc_freq_lo.txt
fopen pkc_fh pkc_freq_lo.txt r
fread pkc_l $pkc_fh pkc_f1
fread pkc_l $pkc_fh pkc_f2
fread pkc_l $pkc_fh pkc_f3
fread pkc_l $pkc_fh pkc_f4
fread pkc_l $pkc_fh pkc_f5
fread pkc_l $pkc_fh pkc_f6
fclose $pkc_fh
let fhead_lo = 0
let fdata_lo = 0
let fhead_lo = $pkc_f4
let fdata_lo = $pkc_f6
if fhead_lo < 1
  echo "ERROR: lowercase 'frequency' produced no column header"
  quit 1
end
if fhead_lo > 30
  echo "ERROR: lowercase 'frequency' header used the wide complex column"
  quit 1
end
if fdata_lo < 1
  echo "ERROR: lowercase 'frequency' produced no data row"
  quit 1
end
if fdata_lo > 20
  echo "ERROR: lowercase 'frequency' data row printed both complex parts"
  quit 1
end

let FREQUENCY = frequency
print col FREQUENCY > pkc_freq_up.txt
fopen pkc_fh pkc_freq_up.txt r
fread pkc_l $pkc_fh pkc_f1
fread pkc_l $pkc_fh pkc_f2
fread pkc_l $pkc_fh pkc_f3
fread pkc_l $pkc_fh pkc_f4
fread pkc_l $pkc_fh pkc_f5
fread pkc_l $pkc_fh pkc_f6
fclose $pkc_fh
let fhead_up = 0
let fdata_up = 0
let fhead_up = $pkc_f4
let fdata_up = $pkc_f6
if fhead_up < 1
  echo "ERROR: uppercase FREQUENCY produced no column header"
  quit 1
end
if fhead_up > 30
  echo "ERROR: uppercase FREQUENCY not matched by print, header used the wide complex column"
  quit 1
end
if fdata_up < 1
  echo "ERROR: uppercase FREQUENCY produced no data row"
  quit 1
end
if fdata_up > 20
  echo "ERROR: uppercase FREQUENCY not matched by print, data row printed both complex parts"
  quit 1
end

* --- .save reserved names, outitf.c:307 and 313 ------------------------
* With ngdebug set, "save speedcheck" / "save deltacheck" make dctran hand
* back a vector of wall-clock timings. The saved names come straight from the
* typed save command, so uppercase reaches the eq() in outitf.c untouched. If
* the name is not recognised no such vector is created at all, so the seeded
* length of 0 survives.
set ngdebug
circbyline * inline divider for the save keyword checks
circbyline .options noacct
circbyline V1 in 0 dc 1
circbyline R1 in out 1k
circbyline C1 out 0 1n
circbyline .end
save SPEEDCHECK DELTACHECK v(out)
tran 1n 5n
let speed_up = 0
let speed_up = length(speedcheck)
if speed_up < 1
  echo "ERROR: uppercase SPEEDCHECK not recognised by save, no speedcheck vector"
  quit 1
end
let delta_up = 0
let delta_up = length(deltacheck)
if delta_up < 1
  echo "ERROR: uppercase DELTACHECK not recognised by save, no deltacheck vector"
  quit 1
end

circbyline * inline divider for the lowercase save control
circbyline .options noacct
circbyline V1 in 0 dc 1
circbyline R1 in out 1k
circbyline C1 out 0 1n
circbyline .end
save speedcheck deltacheck v(out)
tran 1n 5n
let speed_lo = 0
let speed_lo = length(speedcheck)
if speed_lo < 1
  echo "ERROR: lowercase speedcheck not recognised by save, no speedcheck vector"
  quit 1
end
let delta_lo = 0
let delta_lo = length(deltacheck)
if delta_lo < 1
  echo "ERROR: lowercase deltacheck not recognised by save, no deltacheck vector"
  quit 1
end
unset ngdebug

* --- com_destroy "all", postcoms.c:1030 --------------------------------
* "destroy all" wipes every plot but the constants plot. If the sub-keyword
* is not recognised it is taken as a plot name instead and nothing is
* destroyed, so the marker vector planted in the operating-point plot is
* still readable afterwards. This check runs last because it removes the
* plots the earlier checks built. The leading lowercase "destroy all" clears
* the board and resets plot_num, which is what makes the plot the following
* "op" creates deterministically named op1.
circbyline * inline divider for the destroy keyword checks
circbyline .options noacct
circbyline V1 in 0 dc 1
circbyline R1 in out 1k
circbyline Rl out 0 3k
circbyline .end
* This check asserts an ABSENCE, so the seeded 0 is what a passing run reads.
* That only means something once the marker has been shown to be readable
* under the very name the assertion uses, so read it back before destroying:
* if op1.pkc_marker cannot be reached even now, the check below would pass
* without proving anything and must fail loudly instead.
destroy all
op
let pkc_marker = 42
let destr_pre = 0
let destr_pre = op1.pkc_marker
if destr_pre <> 42
  echo "ERROR: op1.pkc_marker unreadable before destroy, the destroy check is void"
  quit 1
end
destroy ALL
let destr_up = 0
let destr_up = op1.pkc_marker
if destr_up = 42
  echo "ERROR: uppercase ALL not recognised by destroy, plot op1 survived"
  quit 1
end

destroy all
op
let pkc_marker2 = 43
let destr_pre2 = 0
let destr_pre2 = op1.pkc_marker2
if destr_pre2 <> 43
  echo "ERROR: op1.pkc_marker2 unreadable before destroy, the destroy control is void"
  quit 1
end
destroy all
let destr_lo = 0
let destr_lo = op1.pkc_marker2
if destr_lo = 43
  echo "ERROR: lowercase 'all' not recognised by destroy, plot op1 survived"
  quit 1
end

echo "INFO: all uppercase postcoms keyword cases passed"
quit 0
