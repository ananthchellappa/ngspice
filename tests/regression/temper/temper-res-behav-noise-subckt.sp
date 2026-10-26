netlist for temper-res-behav-noise-subckt.cir, sourced from its .control block

* The noise branch of a converted R inside a subcircuit.  Its 1 ohm
* resistor names the model as the card wrote it, and subcircuit expansion
* has to translate that name to the subckt's own model (xs:rm), not to the
* global one of the same name, whose TC and tnom are different on purpose.
* The local model also comes after the instances that use it, as sky130's do.
*   a  converted R, local model           b  plain R, the same model
*   d  converted R, instance dtemp=10     e  plain R, dtemp=10
* 'r={73k+0*v(x)}' forces the R->B rewrite without changing the value.

.model rm r tc1=-2.47e-3 tc2=2.7e-6 tnom=25

.subckt sr a b d e
ra a 0 rm r={73k+0*v(a)}
rb b 0 rm r=73k
rd d 0 rm r={73k+0*v(d)} dtemp=10
re e 0 rm r=73k dtemp=10
.model rm r tc1=4e-3 tc2=-1e-6 tnom=40
.ends

ia 0 a dc 1u ac 1
ib 0 b dc 1u ac 1
id 0 d dc 1u ac 1
ie 0 e dc 1u ac 1
xs a b d e sr

.end
