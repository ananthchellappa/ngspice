* Breakpoint sub-keywords typed interactively must be recognised whatever
* their case. These words are not command names -- the dispatcher never
* sees them -- they are the private syntax of "stop" and "delete", matched
* with eq()/strstr() inside src/frontend/breakp.c.
*
* A deck cannot express this. inp_read() lowercases every raw deck line in
* place before the card structure exists, so ".control ... stop WHEN ..."
* already reaches com_stop() as "when". The cp shell input path -- the
* prompt, "ngspice -p" on stdin, ngSpice_Command() -- skips that fold, so
* driving stdin is the only way to put an uppercase sub-keyword in front of
* com_stop() and com_delete().
*
* The circuit is built with circbyline so the test needs no companion .cir.
* Those lines are deck lines and ARE folded, which keeps the case of the
* typed command as the only variable.
*
* Every assertion reads a value back, never a message. A "stop" whose
* sub-keyword is not recognised takes the "goto bad" path and installs no
* breakpoint at all, so the following transient runs to completion: the
* observable is the length of the time vector (108 rows for the full
* 100 ns run) or the last time point (1e-07 for the full run). Each read
* is seeded first, so a transient that produced no data at all fails too
* instead of leaving a stale value behind.
*
* Failures are accumulated in a shell variable rather than a vector,
* because every "tran" makes a new plot and a vector accumulator would not
* survive the plot switch.

set prompt = ""

circbyline * inline divider for the breakpoint keyword checks
circbyline .options noacct
circbyline V1 in 0 dc 1
circbyline R1 in out 1k
circbyline Rl out 0 3k
circbyline C1 out 0 1n ic=0
circbyline .end

set nbad = 0

* --- breakp.c:72  eq(wl->wl_word, "after") ------------------------------
* "stop AFTER 5" must halt the run on the 5th data row.
destroy all
delete all
stop AFTER 5
tran 1n 100n
let n1 = 999
let n1 = length(time)
if n1 <> 5
  echo "ERROR: uppercase AFTER not matched by stop, length(time) = $&n1"
  let nb = $nbad + 1
  set nbad = "$&nb"
end

* --- breakp.c:90  eq(wl->wl_word, "when") -------------------------------
* "stop WHEN time gt 20n" must halt just past 20 ns. The relational word
* is lowercase here so the only uppercase token is the "when" itself.
destroy all
delete all
stop WHEN time gt 20n
tran 1n 100n
let t2 = 99
let t2 = time[length(time)-1]
if (t2 <= 2e-8) | (t2 > 4e-8)
  echo "ERROR: uppercase WHEN not matched by stop, last time = $&t2"
  let nb = $nbad + 1
  set nbad = "$&nb"
end

* --- breakp.c:95  strstr(wl->wl_next->wl_next->wl_word, "when") ---------
* "vec=val" is one lexer word, so com_stop splices it into "vec eq val"
* -- but only when the word that follows it is another "when" or "after".
* With "WHEN" the lookahead misses, nothing is spliced, "time=0" is taken
* for a node name and the next word has to be a relational operator, which
* "WHEN" is not, so the whole command is thrown away. Both conditions of
* the conjunction hold on the very first data row, so the run must stop
* there with a single row.
destroy all
delete all
stop when time=0 WHEN time ge 0
tran 1n 100n
let n3 = 999
let n3 = length(time)
if n3 <> 1
  echo "ERROR: uppercase WHEN lookahead did not split vec=val, length(time) = $&n3"
  let nb = $nbad + 1
  set nbad = "$&nb"
end

* --- breakp.c:96  strstr(wl->wl_next->wl_next->wl_word, "after") --------
* Same splice, reached through the other lookahead word. "after 1" is
* satisfied on data row 1, which is also where time equals 0.
destroy all
delete all
stop when time=0 AFTER 1
tran 1n 100n
let n4 = 999
let n4 = length(time)
if n4 <> 1
  echo "ERROR: uppercase AFTER lookahead did not split vec=val, length(time) = $&n4"
  let nb = $nbad + 1
  set nbad = "$&nb"
end

* --- breakp.c:131  eq(wl->wl_word, "eq") --------------------------------
* time equals 0 on the first data row, so the run stops with one row.
destroy all
delete all
stop when time EQ 0
tran 1n 100n
let n5 = 999
let n5 = length(time)
if n5 <> 1
  echo "ERROR: uppercase EQ not matched by stop, length(time) = $&n5"
  let nb = $nbad + 1
  set nbad = "$&nb"
end

* --- breakp.c:133  eq(wl->wl_word, "ne") --------------------------------
* time differs from 0 from the second data row on.
destroy all
delete all
stop when time NE 0
tran 1n 100n
let n6 = 999
let n6 = length(time)
if n6 <> 2
  echo "ERROR: uppercase NE not matched by stop, length(time) = $&n6"
  let nb = $nbad + 1
  set nbad = "$&nb"
end

* --- breakp.c:135  eq(wl->wl_word, "gt") --------------------------------
destroy all
delete all
stop when time GT 20n
tran 1n 100n
let t7 = 99
let t7 = time[length(time)-1]
if (t7 <= 2e-8) | (t7 > 4e-8)
  echo "ERROR: uppercase GT not matched by stop, last time = $&t7"
  let nb = $nbad + 1
  set nbad = "$&nb"
end

* --- breakp.c:137  eq(wl->wl_word, "lt") --------------------------------
* time is below 20 ns already on the first data row.
destroy all
delete all
stop when time LT 20n
tran 1n 100n
let n8 = 999
let n8 = length(time)
if n8 <> 1
  echo "ERROR: uppercase LT not matched by stop, length(time) = $&n8"
  let nb = $nbad + 1
  set nbad = "$&nb"
end

* --- breakp.c:150  eq(wl->wl_word, "ge") --------------------------------
destroy all
delete all
stop when time GE 30n
tran 1n 100n
let t9 = 99
let t9 = time[length(time)-1]
if (t9 < 3e-8) | (t9 > 5e-8)
  echo "ERROR: uppercase GE not matched by stop, last time = $&t9"
  let nb = $nbad + 1
  set nbad = "$&nb"
end

* --- breakp.c:152  eq(wl->wl_word, "le") --------------------------------
destroy all
delete all
stop when time LE 20n
tran 1n 100n
let n10 = 999
let n10 = length(time)
if n10 <> 1
  echo "ERROR: uppercase LE not matched by stop, length(time) = $&n10"
  let nb = $nbad + 1
  set nbad = "$&nb"
end

* --- breakp.c:457  eq(wl->wl_word, "all") in com_delete -----------------
* A lowercase breakpoint is installed, then removed with the uppercase
* wildcard. If "ALL" is not taken for the wildcard it is parsed as a
* breakpoint number instead, the breakpoint survives and the run stops on
* row 5 rather than completing all 108 rows. The seed of 1 makes a run
* that produced no data fail as well.
destroy all
delete all
stop after 5
delete ALL
tran 1n 100n
let n11 = 1
let n11 = length(time)
if n11 < 100
  echo "ERROR: uppercase ALL not matched by delete, length(time) = $&n11"
  let nb = $nbad + 1
  set nbad = "$&nb"
end

delete all

if $nbad > 0
  echo "ERROR: $nbad uppercase breakpoint keyword cases failed"
  quit 1
else
  echo "INFO: all uppercase breakpoint keyword cases passed"
  quit 0
end
