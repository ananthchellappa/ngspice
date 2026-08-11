## Process this file with automake to produce Makefile.in


TESTS = bugs-1.cir bugs-2.cir dollar-1.cir empty-1.cir resume-1.cir log-functions-1.cir alter-vec.cir test-noise-2.cir test-noise-3.cir ac-zero.cir asrc-tc-1.cir asrc-tc-2.cir if-elseif.cir convergence.cir script-case.cir control-setcs.cir starhash-case.cir all-wildcard-case.cir \
	fold-mixed-case.cir print-tail-case.cir case-flag-noop.cir \
	device-letter-case.cir keyword-fold-guard.cir \
	gnd-command-text.cir diff-pairing-case.cir \
	bsource-undef-node-report.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) $(SHELL) $(top_srcdir)/tests/bin/check.sh $(top_builddir)/src/ngspice

EXTRA_DIST = \
	$(TESTS) \
	$(TESTS:.cir=.out) \
	MixedLib.lib

# print-tail-case.cir redirects print output into the build directory.
# diff-pairing-case.cir writes the two rawfiles it loads, because the second
# plot of a cross-plot diff has to spell a name the reader would have folded,
# and captures load's own output so the reference holds diff's answer alone.
# bsource-undef-node-report.cir writes the sub-decks it sources, because the
# diagnostic it asserts is written during the parse and 'source' is the only
# command a redirect can be attached to.
CLEANFILES = PrintTail.txt dpc_*.raw dpc_*.txt bun_*.cir bun_*.txt

MAINTAINERCLEANFILES = Makefile.in
