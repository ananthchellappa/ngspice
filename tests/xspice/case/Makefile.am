## Process this file with automake to produce Makefile.in

## Case-mode decks whose rewrites emit an XSPICE A device, so they need code
## models loaded.  tests/bin/spinit loads none, which is why this directory
## carries its own spinit and sets SPICE_SCRIPTS=. the way
## tests/xspice/digital does.  Every deck is run with the identifier case
## mode set to 'preserve'; the flag rides on check.sh's unquoted $1.
##
## A deck that writes its A device by hand rather than getting it from a
## rewrite can need a code model this spinit did not load.  adc_bridge,
## dac_bridge, bidi_bridge and the d_* gates all live in
## src/xspice/icm/digital, which is the one extra codemodel line in spinit.in.

TESTS = freq-e-source-case.cir freq-e-source-case-lower.cir \
	freq-g-source-case.cir freq-g-source-case-lower.cir \
	freq-node-expr-case.cir freq-node-expr-case-lower.cir \
	freq-v-expr-case.cir freq-v-expr-case-lower.cir \
	poly-2g6-case.cir poly-2g6-case-lower.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) SPICE_SCRIPTS=. $(SHELL) $(top_srcdir)/tests/bin/check.sh "$(top_builddir)/src/ngspice -D casemode=preserve"

EXTRA_DIST = \
	$(TESTS) \
	$(TESTS:.cir=.out)

MAINTAINERCLEANFILES = Makefile.in
