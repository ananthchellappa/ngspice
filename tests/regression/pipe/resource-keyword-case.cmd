* Resource keywords typed interactively must be recognised whatever their
* case. "rusage" only ever reaches src/frontend/resource.c from a command
* wordlist, and a deck cannot express this: a .control line is lowercased in
* place by inp_read() before the card structure exists, so "RUSAGE SPACE" in
* a deck already arrives at printres() as "space" and passes today. The
* prompt / "ngspice -p" path never goes through that fold, so stdin is the
* only way to put an uppercase resource name in front of com_rusage().
*
* The circuit is built with circbyline so the test needs no companion .cir;
* those lines are deck lines and are folded, which keeps the case of the
* typed command as the only variable.
*
* printres() only prints - it leaves no vector behind - so every check
* captures the report with the '>' redirection (which redirects cp_out only,
* never cp_err) and reads the captured lines back with fopen/fread. The
* assertion is always on the report TEXT that the matched branch produces,
* never on the "Note: no resource usage information" diagnostic, which goes
* to cp_err and is not in the file at all. When a resource name is not
* matched, printres() writes nothing, so the captured file is empty and the
* marker search fails.
*
* Each check runs the lowercase spelling first and demands the same marker,
* so a check that would fail for both spellings is caught here rather than
* silently passing forever.
*
* Every check is fail-fast with an explicit "quit 1".

set prompt = ""

circbyline * inline divider for the resource keyword checks
circbyline .options noacct
circbyline V1 in 0 dc 1
circbyline R1 in out 1k
circbyline Rl out 0 3k
circbyline C1 out 0 1n ic=0
circbyline .end

* An active circuit with a completed run is needed for the "task" checks:
* printres() only reaches the ft_getstat()/if_getstat() blocks when
* ft_curckt and ft_curckt->ci_ckt exist.
op

* Prime the "called" static in printres() so that the "cputime" branch
* actually prints its line; it stays silent on its very first invocation.
rusage cputime > rusage-case.tmp

* --- resource.c:88, eq(wl->wl_word, "everything") -----------------------
* "everything" makes com_rusage() call printres(NULL), which prints the
* whole report; unmatched, printres("EVERYTHING") prints nothing at all.
rusage everything > rusage-case.tmp
fopen fh rusage-case.tmp r
set evline = "SEED"
fread evline $fh evlen
fclose $fh
strstr evhit "$evline" "Total"
if $evhit < 0
  echo "ERROR: lowercase 'rusage everything' printed no report, line = [$evline]"
  quit 1
end

rusage EVERYTHING > rusage-case.tmp
fopen fh rusage-case.tmp r
set evline = "SEED"
fread evline $fh evlen
fclose $fh
strstr evhit "$evline" "Total"
if $evhit < 0
  echo "ERROR: uppercase EVERYTHING not matched by rusage, line = [$evline]"
  quit 1
end

* --- resource.c:88, eq(wl->wl_word, "all") ------------------------------
rusage all > rusage-case.tmp
fopen fh rusage-case.tmp r
set alline = "SEED"
fread alline $fh allen
fclose $fh
strstr alhit "$alline" "Total"
if $alhit < 0
  echo "ERROR: lowercase 'rusage all' printed no report, line = [$alline]"
  quit 1
end

rusage ALL > rusage-case.tmp
fopen fh rusage-case.tmp r
set alline = "SEED"
fread alline $fh allen
fclose $fh
strstr alhit "$alline" "Total"
if $alhit < 0
  echo "ERROR: uppercase ALL not matched by rusage, line = [$alline]"
  quit 1
end

* --- resource.c:156 and :195, eq(name, "totalcputime") ------------------
* The only line this report can hold is "Total <elapsed|CPU> time
* (seconds) = ...", printed by the guard at :156 together with the inner
* test at :195. "Total" is the case-independent part of both spellings.
rusage totalcputime > rusage-case.tmp
fopen fh rusage-case.tmp r
set tcline = "SEED"
fread tcline $fh tclen
fclose $fh
strstr tchit "$tcline" "Total"
if $tchit < 0
  echo "ERROR: lowercase 'rusage totalcputime' printed no total, line = [$tcline]"
  quit 1
end

rusage TOTALCPUTIME > rusage-case.tmp
fopen fh rusage-case.tmp r
set tcline = "SEED"
fread tcline $fh tclen
fclose $fh
strstr tchit "$tcline" "Total"
if $tchit < 0
  echo "ERROR: uppercase TOTALCPUTIME not matched by rusage, line = [$tcline]"
  quit 1
end

* --- resource.c:156 and :200, eq(name, "cputime") -----------------------
* "cputime" prints only the "<elapsed|CPU> time since last call" line, so
* "since last call" separates it from the totalcputime branch above.
rusage cputime > rusage-case.tmp
fopen fh rusage-case.tmp r
set cpline = "SEED"
fread cpline $fh cplen
fclose $fh
strstr cphit "$cpline" "since last call"
if $cphit < 0
  echo "ERROR: lowercase 'rusage cputime' printed no delta, line = [$cpline]"
  quit 1
end

rusage CPUTIME > rusage-case.tmp
fopen fh rusage-case.tmp r
set cpline = "SEED"
fread cpline $fh cplen
fclose $fh
strstr cphit "$cpline" "since last call"
if $cphit < 0
  echo "ERROR: uppercase CPUTIME not matched by rusage, line = [$cpline]"
  quit 1
end

* --- resource.c:228, eq(name, "space") ----------------------------------
rusage space > rusage-case.tmp
fopen fh rusage-case.tmp r
set spline = "SEED"
fread spline $fh splen
fclose $fh
strstr sphit "$spline" "Total DRAM available"
if $sphit < 0
  echo "ERROR: lowercase 'rusage space' printed no memory report, line = [$spline]"
  quit 1
end

rusage SPACE > rusage-case.tmp
fopen fh rusage-case.tmp r
set spline = "SEED"
fread spline $fh splen
fclose $fh
strstr sphit "$spline" "Total DRAM available"
if $sphit < 0
  echo "ERROR: uppercase SPACE not matched by rusage, line = [$spline]"
  quit 1
end

* --- resource.c:293 and :349, eq(name, "task") --------------------------
* "task" is what turns the per-name lookup into "give me everything":
* :293 makes ft_getstat() report the frontend statistics and :349 makes
* if_getstat() report the simulator statistics. The report is then two
* lines, the frontend one first. Unmatched, "TASK" is looked up as the
* name of a single statistic, no such statistic exists, and both lines
* disappear - so the two sites are checked separately from one capture.
rusage task > rusage-case.tmp
fopen fh rusage-case.tmp r
set feline = "SEED"
set siline = "SEED"
fread feline $fh felen
fread siline $fh silen
fclose $fh
strstr fehit "$feline" "Number of lines in the deck"
if $fehit < 0
  echo "ERROR: lowercase 'rusage task' printed no frontend stat, line = [$feline]"
  quit 1
end
strstr sihit "$siline" "Nominal temperature"
if $sihit < 0
  echo "ERROR: lowercase 'rusage task' printed no simulator stat, line = [$siline]"
  quit 1
end

rusage TASK > rusage-case.tmp
fopen fh rusage-case.tmp r
set feline = "SEED"
set siline = "SEED"
fread feline $fh felen
fread siline $fh silen
fclose $fh
strstr fehit "$feline" "Number of lines in the deck"
if $fehit < 0
  echo "ERROR: uppercase TASK not matched for the frontend stats, line = [$feline]"
  quit 1
end
strstr sihit "$siline" "Nominal temperature"
if $sihit < 0
  echo "ERROR: uppercase TASK not matched for the simulator stats, line = [$siline]"
  quit 1
end

echo "INFO: all uppercase resource keyword cases passed"
quit 0
