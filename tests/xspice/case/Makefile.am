## Process this file with automake to produce Makefile.in

## Case-mode decks whose rewrites emit an XSPICE A device, so they need code
## models loaded.  tests/bin/spinit loads none, which is why this directory
## carries its own spinit and sets SPICE_SCRIPTS=. the way
## tests/xspice/digital does.  Every deck is run with the identifier case
## mode set to 'preserve'; the flag rides on check.sh's unquoted $1.
##
## The auto-bridge and event-node decks write their A devices by hand rather
## than getting them from a rewrite, and they need adc_bridge, dac_bridge and
## d_buffer, which live in src/xspice/icm/digital.  That is the one extra
## codemodel line in spinit.in.

TESTS = freq-e-source-case.cir freq-e-source-case-lower.cir \
	freq-g-source-case.cir freq-g-source-case-lower.cir \
	freq-node-expr-case.cir freq-node-expr-case-lower.cir \
	freq-v-expr-case.cir freq-v-expr-case-lower.cir \
	poly-2g6-case.cir poly-2g6-case-lower.cir \
	auto-bridge-node-case.cir auto-bridge-node-case-lower.cir \
	event-node-case.cir event-node-case-lower.cir \
	event-node-lookup-case.cir event-node-lookup-case-lower.cir \
	multi-input-pwl-model-case.cir multi-input-pwl-model-case-lower.cir \
	auto-bridge-family-file-case.cir auto-bridge-family-file-case-lower.cir \
	auto-bridge-family-file-upper.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) SPICE_SCRIPTS=. $(SHELL) $(top_srcdir)/tests/bin/check.sh "$(top_builddir)/src/ngspice -D casemode=preserve"

## bridge_74hct_d_out.subcir is not a deck; it is the family bridge file that
## auto-bridge-family-file-case*.cir make the auto-bridge resolve, so it has
## to be distributed even though it is never a TEST.

EXTRA_DIST = \
	bridge_74hct_d_out.subcir \
	bridge_HCTB_d_out.subcir \
	$(TESTS) \
	$(TESTS:.cir=.out)

MAINTAINERCLEANFILES = Makefile.in
