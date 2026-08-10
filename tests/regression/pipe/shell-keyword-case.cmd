* Shell keywords typed interactively must be recognised whatever their case.
* A deck cannot express this: inp_read() (src/frontend/inpcom.c) lowercases
* every raw deck line in place before the card structure exists, so an option
* word written "-N" or "ALL" or "PARAM" on a card has already become "-n",
* "all", "param" long before the command sees it. The cp shell input path --
* the interactive prompt, "ngspice -p" reading stdin, ngSpice_Command() --
* never goes through that fold, so the case of a typed word survives. Typing
* the word is the only way to put an uppercase shell keyword in front of
* com_echo(), com_history(), com_help(), com_listing(), cp_doalias(),
* ft_getstat(), FindDev() and the "filetype" reader in dosim().
*
* The circuit is built with circbyline so the test needs no companion .cir.
* Those lines are deck lines and ARE folded, which is deliberate: it keeps
* the case of the typed command as the only variable in the session.
*
* None of these commands returns a value; they only print. The printed text
* is therefore captured with '>' and read back with fopen/fread, so every
* assertion below is on a value the session can read, never on a diagnostic
* message. Where a command can fail without writing anything, the variable
* that holds the answer is seeded first, so the seed survives a failure and
* the assertion still fires. Every check also runs the lowercase spelling and
* demands the same value from it, so a check that is simply wrong fails for
* both spellings instead of passing silently.
*
* All loops are counted and contain no 'break', so no construct can leave the
* session waiting for a line that never comes.
set prompt = ""
set silent_fileio

circbyline * inline deck for the shell keyword case checks
circbyline .param foo = 42
circbyline .options noacct
circbyline V1 in 0 dc 1
circbyline R1 in out 1k
circbyline Rl out 0 3k
circbyline .tran 1n 5n
circbyline .end
run

* --- com_echo.c:18, eq(wlist->wl_word, "-n") ---------------------------
* With the flag honoured the file holds exactly "abc" and no newline. When
* the flag word is not recognised it is printed as ordinary text, so the
* first line reads "-N abc" instead.
echo -n abc > shkc_echo_lo.txt
echo -N abc > shkc_echo_up.txt
set el = "no_line_read"
set eu = "no_line_read"
fopen fh shkc_echo_lo.txt r
if $fh < 0
  echo "ERROR: cannot reopen shkc_echo_lo.txt"
  quit 1
end
fread el $fh en
fclose $fh
fopen fh shkc_echo_up.txt r
if $fh < 0
  echo "ERROR: cannot reopen shkc_echo_up.txt"
  quit 1
end
fread eu $fh en
fclose $fh
strcmp ec "_$el" "_abc"
if $ec <> 0
  echo "ERROR: lowercase control 'echo -n' is broken, first line = <$el>"
  quit 1
end
strcmp ec "_$eu" "_abc"
if $ec <> 0
  echo "ERROR: uppercase -N not taken as the echo flag, first line = <$eu>"
  quit 1
end

* --- com_history.c:534, eq(wl->wl_word, "-r") --------------------------
* "history -r 2" lists the two most recent entries newest first. The two
* lines are read back and both are checked, so a listing that is merely
* non-empty, or that came out in forward order, still fails.
echo shkc_mark_one
echo shkc_mark_two
history -R 2 > shkc_hist_up.txt
echo shkc_mark_three
echo shkc_mark_four
history -r 2 > shkc_hist_lo.txt
set h1 = "no_line_read"
set h2 = "no_line_read"
fopen fh shkc_hist_lo.txt r
if $fh < 0
  echo "ERROR: cannot reopen shkc_hist_lo.txt"
  quit 1
end
fread h1 $fh hn
fread h2 $fh hn
fclose $fh
strstr hp "_$h1" "shkc_mark_four"
if $hp < 0
  echo "ERROR: lowercase control 'history -r' is broken, line 1 = <$h1>"
  quit 1
end
strstr hp "_$h2" "shkc_mark_three"
if $hp < 0
  echo "ERROR: lowercase control 'history -r' is broken, line 2 = <$h2>"
  quit 1
end
set h1 = "no_line_read"
set h2 = "no_line_read"
fopen fh shkc_hist_up.txt r
if $fh < 0
  echo "ERROR: cannot reopen shkc_hist_up.txt"
  quit 1
end
fread h1 $fh hn
fread h2 $fh hn
fclose $fh
strstr hp "_$h1" "shkc_mark_two"
if $hp < 0
  echo "ERROR: uppercase -R did not reverse the history listing, line 1 = <$h1>"
  quit 1
end
strstr hp "_$h2" "shkc_mark_one"
if $hp < 0
  echo "ERROR: uppercase -R did not reverse the history listing, line 2 = <$h2>"
  quit 1
end

* --- com_help.c:85, eq(wl->wl_word, c->co_comname) ---------------------
* The help topic is a command name, so it has to be looked up the same way
* the dispatcher looks up the command itself. Both spellings must produce
* the identical help line, and the lowercase one must really carry the ac
* help text so that two empty files cannot satisfy the comparison.
oldhelp ac > shkc_help_lo.txt
oldhelp AC > shkc_help_up.txt
set p1 = "no_line_read"
set p2 = "no_line_read"
fopen fh shkc_help_lo.txt r
if $fh < 0
  echo "ERROR: cannot reopen shkc_help_lo.txt"
  quit 1
end
fread p1 $fh pn
fclose $fh
fopen fh shkc_help_up.txt r
if $fh < 0
  echo "ERROR: cannot reopen shkc_help_up.txt"
  quit 1
end
fread p2 $fh pn
fclose $fh
strstr hq "_$p1" "Do an ac analysis"
if $hq < 0
  echo "ERROR: lowercase control 'oldhelp ac' is broken, line 1 = <$p1>"
  quit 1
end
strcmp hq "_$p1" "_$p2"
if $hq <> 0
  echo "ERROR: uppercase topic AC not matched against the command table, line 1 = <$p2>"
  quit 1
end

* --- com_help.c:19, eq(wl->wl_word, "all") -----------------------------
* "oldhelp all" prints one line per command. Without the keyword the word
* is taken as a topic and nothing but the two-line no-topic reply appears,
* so the alias entry is missing. The scan is counted and breakless.
oldhelp all > shkc_helpall_lo.txt
oldhelp ALL > shkc_helpall_up.txt
set fnd = 0
fopen fh shkc_helpall_lo.txt r
if $fh < 0
  echo "ERROR: cannot reopen shkc_helpall_lo.txt"
  quit 1
end
let sc = 0
while sc < 40
  let sc = sc + 1
  fread al $fh an
  strstr ap "_$al" "Define an alias"
  if $ap >= 0
    set fnd = 1
  end
end
fclose $fh
if $fnd = 0
  echo "ERROR: lowercase control 'oldhelp all' is broken, no alias entry listed"
  quit 1
end
set fnd = 0
fopen fh shkc_helpall_up.txt r
if $fh < 0
  echo "ERROR: cannot reopen shkc_helpall_up.txt"
  quit 1
end
let sc = 0
while sc < 40
  let sc = sc + 1
  fread al $fh an
  strstr ap "_$al" "Define an alias"
  if $ap >= 0
    set fnd = 1
  end
end
fclose $fh
if $fnd = 0
  echo "ERROR: uppercase ALL not taken as the help keyword, no alias entry listed"
  quit 1
end

* --- inp.c:160, strcmp(s, "param") -------------------------------------
* "listing param" prints the global symbol definitions, so the .param value
* appears as "---> foo = 42". Uppercase PARAM falls through to the switch on
* the first letter and silently becomes a physical listing, whose deck line
* reads ".param foo=42" and therefore never matches the spaced form.
listing param > shkc_list_lo.txt
listing PARAM > shkc_list_up.txt
set fnd = 0
fopen fh shkc_list_lo.txt r
if $fh < 0
  echo "ERROR: cannot reopen shkc_list_lo.txt"
  quit 1
end
let sc = 0
while sc < 20
  let sc = sc + 1
  fread ll $fh lln
  strstr lp "_$ll" "foo = 42"
  if $lp >= 0
    set fnd = 1
  end
end
fclose $fh
if $fnd = 0
  echo "ERROR: lowercase control 'listing param' is broken, no symbol value printed"
  quit 1
end
set fnd = 0
fopen fh shkc_list_up.txt r
if $fh < 0
  echo "ERROR: cannot reopen shkc_list_up.txt"
  quit 1
end
let sc = 0
while sc < 20
  let sc = sc + 1
  fread ll $fh lln
  strstr lp "_$ll" "foo = 42"
  if $lp >= 0
    set fnd = 1
  end
end
fclose $fh
if $fnd = 0
  echo "ERROR: uppercase PARAM not taken as the listing keyword, no symbol value printed"
  quit 1
end

* --- ftesopt.c:41, eq(name, FTEOPTtbl[i].keyword) ----------------------
* rusage looks its argument up in FTEOPTtbl. Both spellings must report the
* same deck line count; the lowercase spelling must really report one, so
* two empty files cannot satisfy the comparison.
rusage decklineno > shkc_rusage_lo.txt
rusage DECKLINENO > shkc_rusage_up.txt
set r1 = "no_line_read"
set r2 = "no_line_read"
fopen fh shkc_rusage_lo.txt r
if $fh < 0
  echo "ERROR: cannot reopen shkc_rusage_lo.txt"
  quit 1
end
fread r1 $fh rn
fclose $fh
fopen fh shkc_rusage_up.txt r
if $fh < 0
  echo "ERROR: cannot reopen shkc_rusage_up.txt"
  quit 1
end
fread r2 $fh rn
fclose $fh
strstr rp "_$r1" "Number of lines in the deck"
if $rp < 0
  echo "ERROR: lowercase control 'rusage decklineno' is broken, line 1 = <$r1>"
  quit 1
end
strcmp rp "_$r1" "_$r2"
if $rp <> 0
  echo "ERROR: uppercase DECKLINENO not found in FTEOPTtbl, line 1 = <$r2>"
  quit 1
end

* --- runcoms.c:232, eq(buf, "binary") ----------------------------------
* dosim() reads the "filetype" variable to choose the rawfile format, and
* setcs stores the value with its case intact. A value it does not match
* falls back to ascii, so an unrecognised "BINARY" writes a text rawfile
* whose header line reads "Values:" instead of "Binary:". The scan stops
* consuming lines once the marker is found, so the binary payload is never
* read into a variable.
setcs filetype = binary
run shkc_ft_lo.raw
setcs filetype = BINARY
run shkc_ft_up.raw
setcs filetype = ascii
set fnd = 0
fopen fh shkc_ft_lo.raw r
if $fh < 0
  echo "ERROR: cannot reopen shkc_ft_lo.raw"
  quit 1
end
let sc = 0
while sc < 20
  let sc = sc + 1
  if $fnd = 0
    fread bl $fh bn
    strstr bp "_$bl" "Binary:"
    if $bp >= 0
      set fnd = 1
    end
  end
end
fclose $fh
if $fnd = 0
  echo "ERROR: lowercase control 'filetype = binary' is broken, no Binary: header"
  quit 1
end
set fnd = 0
fopen fh shkc_ft_up.raw r
if $fh < 0
  echo "ERROR: cannot reopen shkc_ft_up.raw"
  quit 1
end
let sc = 0
while sc < 20
  let sc = sc + 1
  if $fnd = 0
    fread bl $fh bn
    strstr bp "_$bl" "Binary:"
    if $bp >= 0
      set fnd = 1
    end
  end
end
fclose $fh
if $fnd = 0
  echo "ERROR: uppercase filetype BINARY ignored, rawfile has no Binary: header"
  quit 1
end

* --- display.c:153, strcmp(name, device[i].name) -----------------------
* hardcopy passes the "hcopydevtype" value to DevSwitch(), which asks
* FindDev() for the device of that name. A name FindDev cannot match leaves
* the hardcopy unwritten, so the file still holds the single seeded line.
echo shkc_seed_only > shkc_hc_lo.svg
echo shkc_seed_only > shkc_hc_up.svg
setcs hcopydevtype = svg
hardcopy shkc_hc_lo.svg v(out)
setcs hcopydevtype = SVG
hardcopy shkc_hc_up.svg v(out)
setcs hcopydevtype = svg
let nlo = 0
fopen fh shkc_hc_lo.svg r
if $fh < 0
  echo "ERROR: cannot reopen shkc_hc_lo.svg"
  quit 1
end
let sc = 0
while sc < 30
  let sc = sc + 1
  fread hl $fh hln
  if $hln >= 0
    let nlo = nlo + 1
  end
end
fclose $fh
if nlo < 10
  echo "ERROR: lowercase control 'hcopydevtype = svg' is broken, $&nlo lines written"
  quit 1
end
let nup = 0
fopen fh shkc_hc_up.svg r
if $fh < 0
  echo "ERROR: cannot reopen shkc_hc_up.svg"
  quit 1
end
let sc = 0
while sc < 30
  let sc = sc + 1
  fread hl $fh hln
  if $hln >= 0
    let nup = nup + 1
  end
end
fclose $fh
if nup < 10
  echo "ERROR: uppercase hcopydevtype SVG not found by FindDev, $&nup lines written"
  quit 1
end

* --- com_alias.c:89, eq(nwl->wl_word, comm->wl_word) -------------------
* asubst() finds an alias by its name without regard to case, so an alias
* whose replacement starts with its own name substitutes once and stops --
* but only when the guard recognises the head word it just produced as the
* one the user typed. Typed in uppercase the guard misses and the alias is
* applied a second time, so the replacement text appears twice. The alias
* is removed before anything is asserted, so the ERROR lines below are
* plain echo output again.
alias echo echo shkc_xx
echo abc > shkc_alias_lo.txt
ECHO abc > shkc_alias_up.txt
unalias echo
set a1 = "no_line_read"
set a2 = "no_line_read"
fopen fh shkc_alias_lo.txt r
if $fh < 0
  echo "ERROR: cannot reopen shkc_alias_lo.txt"
  quit 1
end
fread a1 $fh an
fclose $fh
fopen fh shkc_alias_up.txt r
if $fh < 0
  echo "ERROR: cannot reopen shkc_alias_up.txt"
  quit 1
end
fread a2 $fh an
fclose $fh
strcmp aq "_$a1" "_shkc_xx abc"
if $aq <> 0
  echo "ERROR: lowercase control self-alias is broken, line 1 = <$a1>"
  quit 1
end
strcmp aq "_$a2" "_shkc_xx abc"
if $aq <> 0
  echo "ERROR: uppercase alias head word not recognised by the self-alias guard, line 1 = <$a2>"
  quit 1
end

echo "INFO: all uppercase shell keyword cases passed"
quit 0
