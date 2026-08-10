* XSPICE keyword comparisons that only the cp shell can reach.
*
* Three keyword sites live behind the XSPICE event/code-model layer:
*
*   src/xspice/mif/mifgetvalue.c  MIFget_boolean()  "t"/"true"/"f"/"false"
*   src/xspice/evt/evtprint.c     EVTsave()         "all"
*   src/xspice/evt/evtcheck_nodes.c find_bridge()   ".inc"
*
* None of them can be driven from an ordinary deck. inp_read() lowercases
* every raw deck line before the card structure exists, so a .model card or
* an auto-bridge setup string written in a file arrives already folded. The
* three surfaces that keep their case are used here:
*
*   - a .model card of a code model matched by is_xspice_model() keeps the
*     text of ONE pair of double quotes (keep_case_of_cider_param() bails
*     out as soon as a second pair appears). "filesource" is on that list
*     and is the only model there with a boolean parameter, so
*     timerelative="TRUE" is the only way to put an uppercase boolean
*     literal in front of MIFget_boolean(). The file name must therefore
*     stay unquoted, which numparam rejects, so the model's default file
*     name filesource.txt is used and written here.
*   - 'esave' is a nutmeg command; its argument is typed at the prompt and
*     is never folded.
*   - the auto-bridge setup card comes from a shell variable created with
*     'setcs', which preserves case by definition.
*
* Every check asserts on a simulation value, never on a message, and each
* circuit uses its own node names so that a plot left over from an earlier
* check can never satisfy a later assertion. The seed assigned just before
* each read-back is chosen so that it fails the following test: a command
* that aborts leaves the seed in place and the check still fails.

set prompt = ""

* --- availability ------------------------------------------------------
* Without XSPICE there is no MIF parser, no event layer and no 'esave',
* so there is nothing here to check. The pipe suite is not gated on
* --enable-xspice, so the test has to gate itself.
let have_xspice = 0
if $?xspice_enabled
  let have_xspice = 1
end
if have_xspice < 1
  echo "INFO: ngspice built without XSPICE - no code model keyword sites"
  quit 0
end

* tests/bin/spinit, which the pipe suite uses, loads no code models at all,
* so they are loaded here from the build tree. The tests run with the build
* directory tests/regression/pipe as working directory in both in-tree and
* out-of-tree builds, so the .cm files sit three levels up.
set cmdir = "../../../src/xspice/icm"
set silent_fileio
let have_cm = 0
fopen fh "$cmdir/analog/analog.cm"
if $fh >= 0
  fclose $fh
  let have_cm = have_cm + 1
end
fopen fh "$cmdir/digital/digital.cm"
if $fh >= 0
  fclose $fh
  let have_cm = have_cm + 1
end
unset silent_fileio
* This is a failure, not a skip. XSPICE is enabled in this build, so the code
* models exist; not finding them means the working directory is not the one
* the harness promises, and a 'quit 0' here would turn the whole file into a
* silent pass. The suite asserts on the exit code only, so an INFO line on a
* passing run is invisible.
if have_cm < 2
  echo "ERROR: XSPICE is enabled but no analog.cm/digital.cm under $cmdir"
  quit 1
end
codemodel $cmdir/analog/analog.cm
codemodel $cmdir/digital/digital.cm

* Data for the filesource model. Its 'file' parameter has to be left at its
* default value because a quoted file name would be the second quoted pair
* on the .model card and would switch the whole line back to lower case.
echo 0.0 0.0 > filesource.txt
echo 1e-9 1.0 >> filesource.txt
echo 2e-9 0.0 >> filesource.txt

* --- MIFget_boolean, mifgetvalue.c:286 strcmp(token, "true") -----------
* A rejected boolean aborts the model, the A device that uses it and with
* it the whole circuit, so v(nbt) does not exist and the seed survives.
circbyline * filesource boolean literal 'true'
circbyline v1 nbt 0 dc 1
circbyline r1 nbt 0 1k
circbyline a1 [obt] fsbt
circbyline r2 obt 0 1k
circbyline .model fsbt filesource(amploffset=[0] amplscale=[1] timerelative="TRUE")
circbyline .end
op
let bt = 99
let bt = v(nbt)
if abs(bt - 1) > 1e-6
  echo "ERROR: uppercase TRUE not accepted by MIFget_boolean, v(nbt) = $&bt"
  quit 1
end

* --- MIFget_boolean, mifgetvalue.c:286 strcmp(token, "t") --------------
circbyline * filesource boolean literal 't'
circbyline v1 nbs 0 dc 1
circbyline r1 nbs 0 1k
circbyline a1 [obs] fsbs
circbyline r2 obs 0 1k
circbyline .model fsbs filesource(amploffset=[0] amplscale=[1] timerelative="T")
circbyline .end
op
let bs = 99
let bs = v(nbs)
if abs(bs - 1) > 1e-6
  echo "ERROR: uppercase T not accepted by MIFget_boolean, v(nbs) = $&bs"
  quit 1
end

* --- MIFget_boolean, mifgetvalue.c:288 strcmp(token, "false") ----------
circbyline * filesource boolean literal 'false'
circbyline v1 nbf 0 dc 1
circbyline r1 nbf 0 1k
circbyline a1 [obf] fsbf
circbyline r2 obf 0 1k
circbyline .model fsbf filesource(amploffset=[0] amplscale=[1] timerelative="FALSE")
circbyline .end
op
let bf = 99
let bf = v(nbf)
if abs(bf - 1) > 1e-6
  echo "ERROR: uppercase FALSE not accepted by MIFget_boolean, v(nbf) = $&bf"
  quit 1
end

* --- MIFget_boolean, mifgetvalue.c:288 strcmp(token, "f") --------------
circbyline * filesource boolean literal 'f'
circbyline v1 nbg 0 dc 1
circbyline r1 nbg 0 1k
circbyline a1 [obg] fsbg
circbyline r2 obg 0 1k
circbyline .model fsbg filesource(amploffset=[0] amplscale=[1] timerelative="F")
circbyline .end
op
let bg = 99
let bg = v(nbg)
if abs(bg - 1) > 1e-6
  echo "ERROR: uppercase F not accepted by MIFget_boolean, v(nbg) = $&bg"
  quit 1
end

* --- EVTsave, evtprint.c:1001 strcmp("all", wl->wl_word) ---------------
* 'esave none' switches event logging off for every node; 'esave all' has
* to switch it back on before the run. An unrecognised argument falls
* through to the per node loop, which clears every save flag first and
* then rejects the word, so an unmatched "ALL" leaves the node with no
* logged events. The event vector then holds only the closing point that
* EVTfindvec() always appends, i.e. length 2 instead of 16. The seed is 0
* so that a failed run fails this check too.
circbyline * event node for esave
circbyline v1 evin 0 dc 0 pulse(0 1 0 1n 1n 5n 10n)
circbyline r1 evin 0 1k
circbyline a1 [evin] [evout] abr
circbyline .model abr adc_bridge(in_low=0.4 in_high=0.6)
circbyline .end
esave none
esave ALL
tran 0.5n 20n
let ev = 0
let ev = length(evout)
if ev < 8
  echo "ERROR: uppercase ALL not honoured by esave, length(evout) = $&ev"
  quit 1
end

* --- find_bridge, evtcheck_nodes.c:657 strncmp(setup, ".inc", 4) -------
* The first element of an auto_bridge_* variable is the setup card. When
* it is not an .include card it is run through snprintf() with copies of
* vcc, which also collapses "%%" to "%". So a setup card that is spelt
* ".INCLUDE" is formatted when it should not be, and pulls in a file with
* a different name. Both candidate names exist here and define the same
* subcircuit with a different dac_bridge output level, so the check is a
* voltage and neither spelling can abort the run on a missing file.
echo .subckt auto_buf dig ana vcc=5 > xkcbridge%%.sub
echo .model adacok dac_bridge(out_low=0 out_high=3.3) >> xkcbridge%%.sub
echo adacok [dig] [ana] adacok >> xkcbridge%%.sub
echo .ends >> xkcbridge%%.sub
echo .subckt auto_buf dig ana vcc=5 > xkcbridge%.sub
echo .model adacno dac_bridge(out_low=0 out_high=1.0) >> xkcbridge%.sub
echo adacno [dig] [ana] adacno >> xkcbridge%.sub
echo .ends >> xkcbridge%.sub
setcs auto_bridge_d_out = ( ".INCLUDE xkcbridge%%.sub" "xauto_buf%d %s %s auto_buf vcc=%g" 1 )
circbyline * mixed analog/event node needing an auto bridge
circbyline v1 abin 0 dc 0 pulse(0 1 0 1n 1n 5n 10n)
circbyline r1 abin 0 1k
circbyline a1 [abin] [abmix] abr2
circbyline .model abr2 adc_bridge(in_low=0.4 in_high=0.6)
circbyline rl abmix 0 1k
circbyline .end
tran 0.5n 20n
let ab = 99
let ab = vecmax(v(abmix))
unset auto_bridge_d_out
if abs(ab - 3.3) > 0.01
  echo "ERROR: uppercase .INCLUDE reformatted by find_bridge, v(abmix) peak = $&ab"
  quit 1
end

echo "INFO: all uppercase XSPICE keyword cases passed"
quit 0
