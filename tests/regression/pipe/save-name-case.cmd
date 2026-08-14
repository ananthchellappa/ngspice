* A 'save' typed at the prompt must resolve a name whose case differs from the
* spelling the run stores.  doc/codex/issues/0056.
*
* This is the half of that defect that is live in the DEFAULT case mode, and
* it is only reachable here.  inp_read() (src/frontend/inpcom.c) lower cases
* every raw deck line in place before the card structure exists, so under
* casemode=fold a ".save v(MIDNODE)" card and the net it names have both been
* folded by the time outitf.c compares them and no card can disagree with
* itself.  The cp shell input path -- the prompt, "ngspice -p",
* ngSpice_Command() -- never goes through that fold, so stdin is the only way
* to put a name in front of beginPlot()'s save resolution in a case the deck
* did not write.  tests/regression/case/save-name-case.cir is the same
* comparison on the card route, in the mode where the card route reaches it.
*
* The comparison is name_eq(), src/frontend/outitf.c, whose final compare was
* a bare strcmp.  "print v(MIDNODE)" at this same prompt has always answered,
* because print resolves through findvec(); this deck is what makes save agree
* with it.
*
* The circuits are built with circbyline so this test needs no companion .cir
* file; those lines are deck lines and are folded, which keeps the case of the
* typed save command as the only variable.  Each check builds its own circuit,
* because loading one resets the global save list dbs (src/frontend/inp.c) and
* a save left over from the check above would resolve the analysis for the
* check below and hide its miss.  Each check also destroys the plots first:
* a run that ends in "no data saved" creates no plot at all, so the plot the
* check above left behind would still be current and would answer the read --
* which is how the first draft of this deck passed against the defect.
*
* The assertions are on the shape of the data produced, never on a diagnostic:
* if the save does not resolve, beginPlot() ends the run with "no data saved
* for D.C. Operating point analysis" and no vector reaches the plot at all, so
* the seeded 0 survives the read.  Each check is fail-fast with an explicit
* "quit 1", and the check is done in the spelling the deck wrote first, so
* that a check which fails for both spellings cannot hide here.

set prompt = ""

* --- save of the stored spelling: the case that has always worked ----------
circbyline * inline divider for the save name checks, stored spelling
circbyline .options noacct
circbyline Vs In 0 dc 3
circbyline Rl In MidNode 1k
circbyline Rg MidNode 0 3k
circbyline .end
destroy all
save v(midnode)
op
let snc_lo = 0
let snc_lo = length(v(midnode))
if snc_lo < 1
  echo "ERROR: save of the stored spelling v(midnode) did not resolve"
  quit 1
end

* --- save of the same node in upper case ----------------------------------
circbyline * inline divider for the save name checks, upper case query
circbyline .options noacct
circbyline Vs In 0 dc 3
circbyline Rl In MidNode 1k
circbyline Rg MidNode 0 3k
circbyline .end
destroy all
save v(MIDNODE)
op
let snc_up = 0
let snc_up = length(v(midnode))
if snc_up < 1
  echo "ERROR: uppercase save v(MIDNODE) did not resolve against the stored midnode"
  quit 1
end

* --- and in mixed case, which is what a schematic editor emits -------------
circbyline * inline divider for the save name checks, mixed case query
circbyline .options noacct
circbyline Vs In 0 dc 3
circbyline Rl In MidNode 1k
circbyline Rg MidNode 0 3k
circbyline .end
destroy all
save v(MidNode)
op
let snc_mx = 0
let snc_mx = length(v(midnode))
if snc_mx < 1
  echo "ERROR: mixed case save v(MidNode) did not resolve against the stored midnode"
  quit 1
end

* --- the branch current, which copynode() rewrites to vs#branch ------------
* i(VS) reaches name_eq() as "VS#branch" against the stored "vs#branch": the
* accessor letter is folded by copynode() (src/frontend/breakp2.c) and the
* name inside the wrapper is not, so this is the same comparison on a name
* the simulator half constructed.
circbyline * inline divider for the save current checks, stored spelling
circbyline .options noacct
circbyline Vs In 0 dc 3
circbyline Rl In MidNode 1k
circbyline Rg MidNode 0 3k
circbyline .end
destroy all
save i(vs)
op
let sni_lo = 0
let sni_lo = length(vs#branch)
if sni_lo < 1
  echo "ERROR: save of the stored spelling i(vs) did not resolve"
  quit 1
end

circbyline * inline divider for the save current checks, upper case query
circbyline .options noacct
circbyline Vs In 0 dc 3
circbyline Rl In MidNode 1k
circbyline Rg MidNode 0 3k
circbyline .end
destroy all
save i(VS)
op
let sni_up = 0
let sni_up = length(vs#branch)
if sni_up < 1
  echo "ERROR: uppercase save i(VS) did not resolve against the stored vs#branch"
  quit 1
end

quit 0
