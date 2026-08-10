* Keywords of the expression parser and of the vector building commands,
* typed at the cp shell. A deck cannot express any of these: an ordinary
* card is lowercased in place by inp_read() before the card structure
* exists, so only the shell input path (the interactive prompt, "ngspice -p"
* reading stdin, ngSpice_Command) can put an uppercase keyword in front of
* parse.c, com_let.c, com_compose.c or com_setscale.c.
*
* The circuit is built with circbyline so the test needs no companion .cir;
* those lines are deck lines and are folded, which keeps the case of the
* typed command as the only variable. The KiCad style node /out and the
* '-' in the source name V1-A are what make two of the keywords reachable
* at all, and both survive the fold because they carry no case.
*
* Every check is fail-fast with an explicit "quit 1", and every uppercase
* check is preceded by the same command spelled in lower case, so a check
* that would fail for both spellings cannot pass unnoticed. Each probe is
* seeded with 99 first: a rejected command leaves no vector behind, so the
* seed has to survive and fail the comparison in that case as well.
set prompt = ""

circbyline * inline network for the expression keyword checks
circbyline .options noacct
circbyline V1-A na 0 dc 1
circbyline R1 na /out 1k
circbyline R2 /out nb 2k
circbyline R3 nb 0 1k
circbyline .end
op
set opplot = $curplot

* --- com_let.c:93, eq(vec_name, "all") ---------------------------------
* "all" is the reserved wildcard and may not name the vector being
* assigned. When the reserved word is missed, vec_get() expands the name to
* the whole vector list and the assignment overwrites a circuit vector
* instead of being rejected, which the node voltage sum detects.
let sum_ref = 99
let sum_ref = na + nb + v(/out)
if abs(sum_ref - 2) > 1e-9
  echo "ERROR: node voltage sum is $&sum_ref, expected 2 - vehicle broken"
  quit 1
end
let all = 3
let sum_lo = 99
let sum_lo = na + nb + v(/out)
if abs(sum_lo - 2) > 1e-9
  echo "ERROR: lowercase 'all' accepted as a let target, sum = $&sum_lo"
  quit 1
end
let ALL = 3
let sum_up = 99
let sum_up = na + nb + v(/out)
if abs(sum_up - 2) > 1e-9
  echo "ERROR: uppercase ALL accepted as a let target, sum = $&sum_up"
  quit 1
end

* --- parse.c:912, prefix("i(v", sbuf) -----------------------------------
* The expression lexer keeps i(vname) as a single token only when it reads
* a lower case "i(v". The source name carries a '-', so the fallback in
* PP_mkfnode() cannot rescue the uppercase spelling: the token breaks at
* '(' and the expression is parsed as a call of an unknown function I.
let cur_lo = 99
let cur_lo = i(v1-a)
if abs(cur_lo + 2.5e-4) > 1e-12
  echo "ERROR: lowercase i(v1-a) not evaluated, got $&cur_lo"
  quit 1
end
let cur_up = 99
let cur_up = I(V1-A)
if abs(cur_up + 2.5e-4) > 1e-12
  echo "ERROR: uppercase I(V1-A) not kept as one token, got $&cur_up"
  quit 1
end

* --- com_let.c:172 and kivec(), com_let.c:869 and com_let.c:876 ---------
* A KiCad node name starts with '/', so v(/out) is only parseable after
* kivec() has quoted the node name. All three strstr(..., "v(/") have to
* agree: the gate at line 172 picks the branch and the two inside kivec()
* do the rewrite, so a converted gate with an unconverted kivec() returns
* NULL and the assignment fails just the same.
let kv_lo = 99
let kv_lo = v(/out)
if abs(kv_lo - 0.75) > 1e-9
  echo "ERROR: lowercase v(/out) not evaluated, got $&kv_lo"
  quit 1
end
let kv_up = 99
let kv_up = V(/out)
if abs(kv_up - 0.75) > 1e-9
  echo "ERROR: uppercase V(/out) not quoted by kivec, got $&kv_up"
  quit 1
end

* --- com_compose.c:121, eq(wl->wl_word, "values") -----------------------
compose cv_lo values 5 6 7 8
let cvl = 99
let cvl = cv_lo[2]
if cvl <> 7
  echo "ERROR: lowercase compose values failed, cv_lo[2] = $&cvl"
  quit 1
end
compose cv_up VALUES 5 6 7 8
let cvu = 99
let cvu = cv_up[2]
if cvu <> 7
  echo "ERROR: uppercase compose VALUES not recognised, cv_up[2] = $&cvu"
  quit 1
end

* --- com_compose.c:229, eq(wl->wl_word, "device") -----------------------
* The device form renames @r1[resistance] to @r1_resistance, so the new
* name is the readback. Two different resistors keep the two spellings
* from sharing a vector.
compose @r1[resistance] device
let cdl = 99
let cdl = @r1_resistance
if abs(cdl - 1000) > 1e-6
  echo "ERROR: lowercase compose device failed, @r1_resistance = $&cdl"
  quit 1
end
compose @r2[resistance] DEVICE
let cdu = 99
let cdu = @r2_resistance
if abs(cdu - 2000) > 1e-6
  echo "ERROR: uppercase compose DEVICE not recognised, @r2_resistance = $&cdu"
  quit 1
end

tran 0.1 3

* --- com_setscale.c:60, strcmp(wl->wl_word, "none") ---------------------
* "none" clears the scale of a single vector so that the plot default (the
* time vector) is used again. The range operator counts the points whose
* scale value falls into [0,3]: with the artificial scale 0,100,200,... only
* the first point qualifies, with time every point does. An unrecognised
* "none" is looked up as a vector name instead, the scale keeps pointing at
* the artificial vector and the count stays 1.
let ys = nb
let ny = length(ys)
let sc = 100*vector(ny)
setscale ys sc
let n_sc = 99
let n_sc = length(ys[[0,3]])
if n_sc <> 1
  echo "ERROR: setscale to an explicit vector failed, count = $&n_sc"
  quit 1
end
setscale ys none
let n_lo = 99
let n_lo = length(ys[[0,3]])
if n_lo <> ny
  echo "ERROR: lowercase setscale none did not clear the scale, count = $&n_lo"
  quit 1
end
setscale ys sc
setscale ys NONE
let n_up = 99
let n_up = length(ys[[0,3]])
if n_up <> ny
  echo "ERROR: uppercase setscale NONE did not clear the scale, count = $&n_up"
  quit 1
end

* --- parse.c:252, eq(pn->pn_value->v_name, "list") ----------------------
* checkvalid() lets a zero length vector through only when it is named
* "list"; any other zero length name makes ft_getpnames_quotes() return
* NULL and the whole vector list of the command is thrown away. fft is the
* readback: with "list" tolerated the remaining vector is transformed and a
* spectrum plot with a frequency vector appears, otherwise nothing is
* computed at all and the seed survives. Seed and readback are always taken
* in the plot fft has just left, so that the check never straddles a plot
* switch: a vector of another plot is simply not visible and would make the
* comparison evaluate to false.
linearize nb
set tplot = $curplot
setplot $tplot
fft list nb
let nf_lo = 99
let nf_lo = length(frequency)
if nf_lo = 99
  echo "ERROR: lowercase 'list' was not tolerated by checkvalid - vehicle broken"
  quit 1
end
setplot $tplot
fft LIST nb
let nf_up = 99
let nf_up = length(frequency)
if nf_up = 99
  echo "ERROR: uppercase LIST discarded the whole vector list of fft"
  quit 1
end

* --- parse.c:214, strstr(pn_name, 'v(' quote) and strstr(pn_name, 'i(' quote) -
* ft_getpnames_quotes() puts a node or device name that carries an
* arithmetic character in quotes before handing the line to the parser and
* strips those quotes off pn_name again afterwards. pn_name is not only a
* display name: ft_evaluate() (evaluate.c:83) copies it into v_name, so the
* strip decides the name the vector is stored and written under. The whole
* name is folded to lower case further downstream, so the only difference
* the two spellings can leave behind is the pair of quotes. Writing the
* vectors out and loading them back turns that name into a readable value:
* a still quoted name means the loaded plot has no v(/out) or i(v1-a) at
* all, the readback let fails and the seed survives. Both readbacks are
* spelled in lower case, so the case of the written expression stays the
* only variable.
setplot $opplot
write exprcoms-pn-lo.raw v("/out") i("v1-a")
load exprcoms-pn-lo.raw
let pnv_lo = 99
let pnv_lo = length(v(/out))
if pnv_lo <> 1
  echo "ERROR: lowercase quoted v of the KiCad node kept its quotes, len = $&pnv_lo"
  quit 1
end
let pni_lo = 99
let pni_lo = length(i(v1-a))
if pni_lo <> 1
  echo "ERROR: lowercase quoted i of the hyphenated source kept its quotes, len = $&pni_lo"
  quit 1
end
setplot $opplot
write exprcoms-pn-up.raw V("/out") I("v1-a")
load exprcoms-pn-up.raw
let pnv_up = 99
let pnv_up = length(v(/out))
if pnv_up <> 1
  echo "ERROR: uppercase V of the quoted KiCad node kept its quotes in the vector name, len = $&pnv_up"
  quit 1
end
let pni_up = 99
let pni_up = length(i(v1-a))
if pni_up <> 1
  echo "ERROR: uppercase I of the quoted hyphenated source kept its quotes in the vector name, len = $&pni_up"
  quit 1
end

echo "INFO: all uppercase expression and vector command keyword cases passed"
quit 0
