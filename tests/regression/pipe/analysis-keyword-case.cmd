* Analysis keywords typed interactively must be recognised whatever their
* case. A deck cannot express this: an ordinary card is lowercased in place
* by inp_read() before the card structure exists, so ".TRAN 1n 10n UIC"
* already reaches dot_tran() as "uic" and passes today. The command line
* never goes through that fold, so "tran 1n 10n UIC" is the only way to put
* an uppercase analysis keyword in front of src/spicelib/parser/inp2dot.c.
*
* The circuit is built with circbyline so the test needs no companion .cir;
* those lines are deck lines and are folded, which keeps the case of the
* typed command as the only variable.
*
* Every check is fail-fast with an explicit "quit 1": a keyword that is not
* recognised aborts its analysis, so the vector it should have produced does
* not exist and the following "let" would leave a stale value behind.
set prompt = ""

circbyline * inline divider for the analysis keyword checks
circbyline .options noacct
circbyline V1 in 0 dc 1
circbyline R1 in out 1k
circbyline Rl out 0 3k
circbyline C1 out 0 1n ic=0
circbyline .end

* --- dot_tran, inp2dot.c strcmp(word, "uic") ---------------------------
* With UIC honoured the transient starts from the capacitor's ic=0, so the
* first point is ~0. Without it the operating point supplies 0.75 V, and if
* the card is rejected outright no v(out) exists at all. The seed of 99
* makes that third outcome fail too instead of reading a stale vector.
tran 1n 10n UIC
let v0 = 99
let v0 = v(out)[0]
if v0 > 0.5
  echo "ERROR: uppercase UIC not honoured by tran, v(out)[0] = $&v0"
  quit 1
end

* --- dot_tf, inp2dot.c *name == 'v' ------------------------------------
tf V(out) v1
let tfv = 99
let tfv = transfer_function
if abs(tfv - 0.75) > 1e-9
  echo "ERROR: uppercase V() not accepted by tf, transfer_function = $&tfv"
  quit 1
end

* --- dot_tf, inp2dot.c *name == 'i' ------------------------------------
tf I(v1) v1
let tfi = 99
let tfi = transfer_function
if abs(tfi + 2.5e-4) > 1e-12
  echo "ERROR: uppercase I() not accepted by tf, transfer_function = $&tfi"
  quit 1
end

* --- dot_sens, inp2dot.c *name == 'v' ----------------------------------
sens V(out)
let sv = 99
let sv = r1
* sensitivities are computed by perturbation, so the tolerance is loose
if abs(sv + 1.875e-4) > 1e-8
  echo "ERROR: uppercase V() not accepted by sens, r1 = $&sv"
  quit 1
end

* --- dot_sens, inp2dot.c strcmp(name, "ac") ---------------------------
* The mode word that ends the .sens filter list and switches the analysis
* from DC to AC. Unrecognised, it is taken for another filter name, the
* analysis stays DC and the frequency sweep never happens, so the result
* vector is one point long instead of three.
sens v(out) AC dec 1 1k 100k
let acn = 99
let acn = length(r1)
if acn <> 3
  echo "ERROR: uppercase AC not accepted by sens, length(r1) = $&acn"
  quit 1
end

echo "INFO: all uppercase analysis keyword cases passed"
quit 0
