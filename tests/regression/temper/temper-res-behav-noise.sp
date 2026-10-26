netlist for temper-res-behav-noise.cir, sourced from its .control block

* Each output node has one resistor to ground and its own 1 uA source, so
* the output noise at a node is the thermal noise of that resistor alone.
*   a  converted R with a tnom=25 model     b  plain R, the same model
*   d  converted R, instance dtemp=10       e  plain R, dtemp=10
*   f  converted R, instance temp=50        g  plain R, temp=50
* 'r={73k+0*v(x)}' forces the R->B rewrite without changing the value.

.model rm r tc1=-2.47e-3 tc2=2.7e-6 tnom=25

ia 0 a dc 1u ac 1
ra a 0 rm r={73k+0*v(a)}
ib 0 b dc 1u ac 1
rb b 0 rm r=73k

id 0 d dc 1u ac 1
rd d 0 rm r={73k+0*v(d)} dtemp=10
ie 0 e dc 1u ac 1
re e 0 rm r=73k dtemp=10

iff 0 f dc 1u ac 1
rf f 0 rm r={73k+0*v(f)} temp=50
ig 0 g dc 1u ac 1
rg g 0 rm r=73k temp=50

.end
