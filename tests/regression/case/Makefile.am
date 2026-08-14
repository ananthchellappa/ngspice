## Process this file with automake to produce Makefile.in

## Every deck in this directory is run with the identifier case mode set to
## 'preserve'.  check.sh takes the binary as an unquoted $1, so the extra
## flags ride on that argument; tests/xspice/digital/Makefile.am is the
## precedent.

TESTS = harness-alive.cir name-roundtrip.cir alter-case.cir \
	gnd-alias-case.cir option-card-case.cir node-case-alias.cir \
	write-roundtrip.cir case-flag-noop.cir \
	device-letter-model-case.cir device-letter-model-case-lower.cir \
	device-letter-case.cir device-letter-case-lower.cir \
	model-name-case.cir model-name-case-lower.cir \
	subckt-name-case.cir subckt-name-case-lower.cir \
	global-node-case.cir global-node-case-lower.cir \
	name-lookup-case.cir name-lookup-case-lower.cir \
	ac-value-case.cir ac-value-case-lower.cir \
	diode-off-case.cir diode-off-case-lower.cir \
	mos-off-case.cir mos-off-case-lower.cir \
	bjt-off-case.cir bjt-off-case-lower.cir \
	vdmos-pchan-case.cir vdmos-pchan-case-lower.cir \
	subckt-params-case.cir subckt-params-case-lower.cir \
	quote-params-model-case.cir quote-params-model-case-lower.cir \
	quote-params-subckt-case.cir quote-params-subckt-case-lower.cir \
	compat-e-value-case.cir compat-e-value-case-lower.cir \
	compat-g-value-case.cir compat-g-value-case-lower.cir \
	compat-f-temper-case.cir compat-f-temper-case-lower.cir \
	compat-h-temper-case.cir compat-h-temper-case-lower.cir \
	compat-r-behav-case.cir compat-r-behav-case-lower.cir \
	compat-c-behav-case.cir compat-c-behav-case-lower.cir \
	compat-l-behav-case.cir compat-l-behav-case-lower.cir \
	compat-k-mutual-case.cir compat-k-mutual-case-lower.cir \
	vdmos-type-suffix-case.cir vdmos-type-suffix-case-lower.cir \
	vdmos-thermal-nodes-case.cir vdmos-thermal-nodes-case-lower.cir \
	bsource-quote-case.cir bsource-quote-case-lower.cir \
	bsource-compat-case.cir bsource-compat-case-lower.cir \
	subckt-mult-skip-case.cir subckt-mult-skip-case-lower.cir \
	meas-current-vsource-case.cir meas-current-vsource-case-lower.cir \
	func-vnode-case.cir func-vnode-case-lower.cir \
	bsource-vnode-case.cir bsource-vnode-case-lower.cir \
	behav-r-vnode-case.cir behav-r-vnode-case-lower.cir \
	agauss-bsource-case.cir agauss-bsource-case-lower.cir \
	savecurrents-case.cir savecurrents-case-lower.cir \
	meas-when-vnode-case.cir meas-when-vnode-case-lower.cir \
	alterparam-x-case.cir alterparam-x-case-lower.cir \
	noopac-linear-case.cir noopac-linear-case-lower.cir \
	subckt-mult-param-case.cir subckt-mult-param-case-lower.cir \
	param-name-case.cir param-name-case-lower.cir \
	subckt-formal-name-case.cir subckt-formal-name-case-lower.cir \
	func-name-case.cir func-name-case-lower.cir \
	func-formal-case.cir func-formal-case-lower.cir \
	param-depend-case.cir param-depend-case-lower.cir \
	param-quote-name-case.cir param-quote-name-case-lower.cir \
	param-temper-func-case.cir param-temper-func-case-lower.cir \
	builtin-func-case.cir builtin-func-case-lower.cir \
	numnodes-letter-case.cir numnodes-letter-case-lower.cir \
	save-alli-case.cir save-alli-case-lower.cir \
	save-name-case.cir save-name-case-lower.cir \
	plot-scale-name-twin.cir \
	alter-rebin-case.cir alter-rebin-case-lower.cir \
	alter-rebin-param-case.cir alter-rebin-param-case-lower.cir \
	bsource-forward-ref-case.cir bsource-forward-ref-case-lower.cir \
	subckt-formal-pin-case.cir subckt-formal-pin-case-lower.cir \
	udf-name-case.cir udf-name-case-lower.cir \
	show-device-letter-case.cir curcasemode-effect.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) $(SHELL) $(top_srcdir)/tests/bin/check.sh "$(top_builddir)/src/ngspice -D casemode=preserve"

EXTRA_DIST = \
	$(TESTS) \
	$(TESTS:.cir=.out)

# write-roundtrip.cir writes a rawfile into the build directory, and
# plot-scale-name-twin.cir the capture it reads its assertion back out of.
# curcasemode-effect.cir writes the capture its read-only refusal is read back
# out of: the refusal is on cp_err, which check.sh does not capture.
CLEANFILES = casewrite.raw pst_*.txt cce_*.txt

MAINTAINERCLEANFILES = Makefile.in
