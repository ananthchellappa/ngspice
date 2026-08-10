* Device query / device alter keywords typed at the cp shell prompt must be
* recognised whatever their case.  A deck cannot express any of this: an
* ordinary card is lowercased in place by inp_read() before the card
* structure exists, and the words below are command words, not card words,
* so the only way to put an uppercase spelling in front of
* src/frontend/device.c and src/spicelib/parser/inpaname.c is to type it -
* which is exactly what this suite does (ngspice -p reading stdin).
*
* The circuits are built with circbyline so the test needs no companion
* .cir; those lines are deck lines and are folded, which keeps the case of
* the typed command as the only variable.
*
* devhelp and show have no return value, so their assertions are made on
* the text they actually produced: the output is captured with the '>'
* redirection tail and read back line by line with fopen/fread.  Each check
* compares the uppercase run against the lowercase run of the same command
* and additionally pins the lowercase run to a marker that only the
* intended code path produces, so no check can pass vacuously.
*
* Failures are accumulated rather than fail-fast so that the scratch files
* are removed on every path.

set prompt = ""
let fail_count = 0

* ==================================================================
* devhelp option words -type / -flags / -csv
*   src/frontend/device.c eq(wlist->wl_word, "-type") and friends
* ==================================================================
* devhelp prints the device description, a blank line, "Model Parameters"
* and then a column header, so the header is always the 4th line and it is
* the option word that decides what that header looks like.  When the
* option word is not recognised the option loop breaks and the word is
* taken as the device name instead, so the whole output collapses to the
* single line "Error: Device -TYPE not found" and the 4th line is missing.

* --- -type ---------------------------------------------------------
devhelp -type resistor > devkc_lo.txt
devhelp -TYPE resistor > devkc_up.txt

fopen fh devkc_lo.txt r
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread lo_line $fh lo_len
fclose $fh

fopen fh devkc_up.txt r
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread up_line $fh up_len
fclose $fh

strstr mark "$lo_line" "Type"
if $mark < 0
  echo "ERROR: lowercase control failed, devhelp -type gave no Type column"
  let fail_count = fail_count + 1
end
strcmp diff "$lo_line" "$up_line"
if $diff <> 0
  echo "ERROR: devhelp -TYPE was not taken as an option word"
  let fail_count = fail_count + 1
end

* --- -flags --------------------------------------------------------
devhelp -flags resistor > devkc_lo.txt
devhelp -FLAGS resistor > devkc_up.txt

fopen fh devkc_lo.txt r
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread lo_line $fh lo_len
fclose $fh

fopen fh devkc_up.txt r
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread up_line $fh up_len
fclose $fh

strstr mark "$lo_line" "Flags"
if $mark < 0
  echo "ERROR: lowercase control failed, devhelp -flags gave no Flags column"
  let fail_count = fail_count + 1
end
strcmp diff "$lo_line" "$up_line"
if $diff <> 0
  echo "ERROR: devhelp -FLAGS was not taken as an option word"
  let fail_count = fail_count + 1
end

* --- -csv ----------------------------------------------------------
devhelp -csv resistor > devkc_lo.txt
devhelp -CSV resistor > devkc_up.txt

fopen fh devkc_lo.txt r
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread lo_line $fh lo_len
fclose $fh

fopen fh devkc_up.txt r
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread up_line $fh up_len
fclose $fh

strstr mark "$lo_line" "id#,"
if $mark <> 0
  echo "ERROR: lowercase control failed, devhelp -csv gave no comma header"
  let fail_count = fail_count + 1
end
strcmp diff "$lo_line" "$up_line"
if $diff <> 0
  echo "ERROR: devhelp -CSV was not taken as an option word"
  let fail_count = fail_count + 1
end

* ==================================================================
* A circuit for the show / save checks
* ==================================================================
circbyline * inline divider for the device keyword checks
circbyline .options noacct
circbyline V1 in 0 dc 1
circbyline R1 in out 1k
circbyline Rl out 0 3k
circbyline .end
op

* ==================================================================
* show -v  in all_show_old()   src/frontend/device.c eq(wl->wl_word, "-v")
* ==================================================================
* Without the "altshow" variable com_show() calls all_show_old(), which
* hands the wordlist to old_show() as soon as its first word is "-v".
* The two printers are trivial to tell apart: old_show() heads its output
* with "r1:", all_show_old() heads it with the device class line
* " Resistor: Simple linear resistor".  devkc_oldshow.txt is kept: it is
* the reference copy of old_show() output used again further down.

show -v r1 > devkc_oldshow.txt
show -V r1 > devkc_up.txt

fopen fh devkc_oldshow.txt r
fread lo_line $fh lo_len
fread lo_line2 $fh lo_len2
fclose $fh

fopen fh devkc_up.txt r
fread up_line $fh up_len
fclose $fh

strcmp mark "$lo_line" "r1:"
if $mark <> 0
  echo "ERROR: lowercase control failed, show -v r1 did not reach old_show"
  let fail_count = fail_count + 1
end
strcmp diff "$lo_line" "$up_line"
if $diff <> 0
  echo "ERROR: show -V r1 did not reach old_show, all_show_old kept the -V"
  let fail_count = fail_count + 1
end

* ==================================================================
* show all  (device position) in all_show_old()
*   src/frontend/device.c eq(w->wl_word, "all")
* ==================================================================
* "all" in the device position sets DGEN_ALLDEVS and every device of the
* circuit is listed.  Unrecognised, "ALL" stays an ordinary device name,
* matches no instance, and the command prints nothing at all.

show all > devkc_lo.txt
show ALL > devkc_up.txt

fopen fh devkc_lo.txt r
fread lo_line $fh lo_len
fclose $fh

fopen fh devkc_up.txt r
fread up_line $fh up_len
fclose $fh

if $lo_len <= 0
  echo "ERROR: lowercase control failed, show all printed nothing"
  let fail_count = fail_count + 1
end
strcmp diff "$lo_line" "$up_line"
if $diff <> 0
  echo "ERROR: show ALL was not taken as the all-devices wildcard"
  let fail_count = fail_count + 1
end

* ==================================================================
* show dev : all  (parameter position) in all_show_old()
*   src/frontend/device.c eq(w->wl_word, "all")
* ==================================================================
* Behind the ':' the same word sets DGEN_ALLPARAMS and every parameter of
* r1 is listed.  Unrecognised, "ALL" is looked up as a parameter name of
* r1, is not one, and its value is printed as a single row of question
* marks, so the whole table is four or five lines long.  The comparison is
* made on the 8th line: that is well inside the parameter list of the
* working spelling and past the end of the broken one, which keeps the
* check independent of how the first rows of the table are laid out.

show r1 : all > devkc_lo.txt
show r1 : ALL > devkc_up.txt

fopen fh devkc_lo.txt r
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread lo_line $fh lo_len
fclose $fh

fopen fh devkc_up.txt r
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread up_line $fh up_len
fclose $fh

if $lo_len <= 0
  echo "ERROR: lowercase control failed, show r1 : all listed no parameters"
  let fail_count = fail_count + 1
end
strstr mark "$lo_line" "?????"
if $mark >= 0
  echo "ERROR: lowercase control failed, show r1 : all found no parameters"
  let fail_count = fail_count + 1
end
strcmp diff "$lo_line" "$up_line"
if $diff <> 0
  echo "ERROR: show r1 : ALL was not taken as the all-parameters wildcard"
  let fail_count = fail_count + 1
end

* ==================================================================
* The same three words in all_show(), reached with "altshow" set
*   src/frontend/device.c eq(wl->wl_word, "-v"), eq(w->wl_word, "all")
* ==================================================================
set altshow

* --- -v ------------------------------------------------------------
* all_show() also heads its output with "r1:", so the first line cannot
* separate it from old_show().  The second line can: old_show() must
* produce exactly what it produced above in the default mode, because it
* is the same function printing the same parameters of the same device.

show -v r1 > devkc_lo.txt
show -V r1 > devkc_up.txt

fopen fh devkc_lo.txt r
fread junk $fh jlen
fread lo_line $fh lo_len
fclose $fh

fopen fh devkc_up.txt r
fread junk $fh jlen
fread up_line $fh up_len
fclose $fh

strcmp mark "$lo_line" "$lo_line2"
if $mark <> 0
  echo "ERROR: lowercase control failed, altshow show -v r1 did not reach old_show"
  let fail_count = fail_count + 1
end
strcmp diff "$lo_line2" "$up_line"
if $diff <> 0
  echo "ERROR: altshow show -V r1 did not reach old_show, all_show kept the -V"
  let fail_count = fail_count + 1
end

* --- all, device position ------------------------------------------
show all > devkc_lo.txt
show ALL > devkc_up.txt

fopen fh devkc_lo.txt r
fread lo_line $fh lo_len
fclose $fh

fopen fh devkc_up.txt r
fread up_line $fh up_len
fclose $fh

if $lo_len <= 0
  echo "ERROR: lowercase control failed, altshow show all printed nothing"
  let fail_count = fail_count + 1
end
strcmp diff "$lo_line" "$up_line"
if $diff <> 0
  echo "ERROR: altshow show ALL was not taken as the all-devices wildcard"
  let fail_count = fail_count + 1
end

* --- all, parameter position ---------------------------------------
show r1 : all > devkc_lo.txt
show r1 : ALL > devkc_up.txt

fopen fh devkc_lo.txt r
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread lo_line $fh lo_len
fclose $fh

fopen fh devkc_up.txt r
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread junk $fh jlen
fread up_line $fh up_len
fclose $fh

if $lo_len <= 0
  echo "ERROR: lowercase control failed, altshow show r1 : all listed no parameters"
  let fail_count = fail_count + 1
end
strstr mark "$lo_line" "?????"
if $mark >= 0
  echo "ERROR: lowercase control failed, altshow show r1 : all found no parameters"
  let fail_count = fail_count + 1
end
strcmp diff "$lo_line" "$up_line"
if $diff <> 0
  echo "ERROR: altshow show r1 : ALL was not taken as the all-parameters wildcard"
  let fail_count = fail_count + 1
end

unset altshow

* ==================================================================
* save @dev[param]   src/spicelib/parser/inpaname.c
*   strcmp(parm, sim->devices[*dev]->instanceParms[i].keyword)
* ==================================================================
* A saved "special" vector is filled by getSpecial() -> INPaName(), which
* matches the text between the brackets against the instance parameter
* table.  The bracket text of a typed "save" never passes inp_read(), so
* it arrives with the case the user gave it.  When the match fails the
* vector is still created but no point is ever appended to it, so it ends
* up zero long and any expression over it is rejected - which is why every
* result below is seeded with 99 first.

save @r1[i]
op
let n_lo = 99
let n_lo = length(@r1[i])
let i_lo = 99
let i_lo = @r1[i][0]
if n_lo <> 1
  echo "ERROR: lowercase control failed, save @r1[i] produced no data point"
  let fail_count = fail_count + 1
end
if abs(i_lo - 2.5e-4) > 1e-9
  echo "ERROR: lowercase control failed, save @r1[i] gave the wrong current"
  let fail_count = fail_count + 1
end

* A fresh circuit, so that the uppercase spelling is the only special
* vector in the plot and cannot be answered by the lowercase one.
circbyline * inline divider for the uppercase save check
circbyline .options noacct
circbyline V1 in 0 dc 1
circbyline R1 in out 1k
circbyline Rl out 0 3k
circbyline .end

save @r1[I]
op
let n_up = 99
let n_up = length(@r1[I])
let i_up = 99
let i_up = @r1[I][0]
if n_up <> 1
  echo "ERROR: save @r1[I] produced no data point, INPaName rejected the parameter"
  let fail_count = fail_count + 1
end
if abs(i_up - 2.5e-4) > 1e-9
  echo "ERROR: save @r1[I] gave no current, INPaName rejected the parameter"
  let fail_count = fail_count + 1
end

* ==================================================================
* altermod ... file <name>   src/frontend/device.c strstr(input, "file")
* ==================================================================
* The word that introduces the model file is accepted case insensitively
* by the ciprefix() scan that stops the model-name loop, but the pointer
* into the flattened command text is then found with a case sensitive
* strstr().  With "FILE" that strstr() cannot see the keyword, so the file
* name is cut out of the wrong place and the whole altermod is dropped.
* The scratch file name deliberately contains a lowercase "file", so the
* mis-parse lands inside the file name instead of on a null pointer.

echo "* device-keyword-case model file" > devkc_modfile.mod
echo ".model dmod d (is=3e-14)" >> devkc_modfile.mod

circbyline * inline divider for the altermod file check
circbyline .options noacct
circbyline V1 in 0 dc 0.6
circbyline D1 in 0 dmod
circbyline .model dmod d (is=1e-14)
circbyline .end
op

altermod dmod file devkc_modfile.mod
let is_lo = 99
let is_lo = @dmod[is]
if abs(is_lo - 3e-14) > 1e-16
  echo "ERROR: lowercase control failed, altermod dmod file did not load the model"
  let fail_count = fail_count + 1
end

echo "* device-keyword-case model file" > devkc_modfile.mod
echo ".model dmod d (is=7e-14)" >> devkc_modfile.mod

altermod dmod FILE devkc_modfile.mod
let is_up = 99
let is_up = @dmod[is]
if abs(is_up - 7e-14) > 1e-16
  echo "ERROR: altermod dmod FILE did not load the model, strstr missed the keyword"
  let fail_count = fail_count + 1
end

* ==================================================================
shell rm -f devkc_lo.txt devkc_up.txt devkc_oldshow.txt devkc_modfile.mod

if fail_count > 0
  echo "ERROR: $&fail_count uppercase device keyword cases failed"
  quit 1
else
  echo "INFO: all uppercase device keyword cases passed"
  quit 0
end
