## Process this file with automake to produce Makefile.in


TESTS = bugs-1.cir bugs-2.cir dollar-1.cir empty-1.cir resume-1.cir log-functions-1.cir alter-vec.cir test-noise-2.cir test-noise-3.cir ac-zero.cir asrc-tc-1.cir asrc-tc-2.cir if-elseif.cir convergence.cir script-case.cir control-setcs.cir starhash-case.cir all-wildcard-case.cir \
	fold-mixed-case.cir print-tail-case.cir case-flag-noop.cir \
	device-letter-case.cir keyword-fold-guard.cir \
	gnd-command-text.cir diff-pairing-case.cir \
	bsource-undef-node-report.cir dotcard-undef-node-report.cir \
	define-const-body.cir define-const-body-listing.cir \
	define-formal-body.cir save-undef-report.cir \
	save-multianalysis-report.cir save-crossplot-report.cir \
	write-let-session.cir node-case-collision-report.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) $(SHELL) $(top_srcdir)/tests/bin/check.sh $(top_builddir)/src/ngspice

EXTRA_DIST = \
	$(TESTS) \
	$(TESTS:.cir=.out) \
	MixedLib.lib

# print-tail-case.cir redirects print output into the build directory.
# diff-pairing-case.cir writes the two rawfiles it loads, because the second
# plot of a cross-plot diff has to spell a name the reader would have folded,
# and captures load's own output so the reference holds diff's answer alone.
# bsource-undef-node-report.cir and dotcard-undef-node-report.cir write the
# sub-decks they source, because the diagnostic they assert is written during
# the parse and 'source' is the only command a redirect can be attached to.
# save-undef-report.cir writes sub-decks for a related reason: its diagnostic
# is written during the run, so the redirect has to be in force across the
# 'source' that runs the sub-deck whose .save cards are the subject.
# save-multianalysis-report.cir is the same shape and asserts the cases that
# report must stay silent about, one of which needs a noise analysis and so
# needs a sub-deck of its own.
# save-crossplot-report.cir is the same shape again for the silence one plot
# of an analysis owes a name an earlier plot of it already resolved.
# write-let-session.cir writes the three rawfiles it asserts on and reads one
# of them back, so the files it wrote have to survive the run.
# node-case-collision-report.cir writes sub-decks for bsource-undef-node-report.cir's
# reason, and is the fold-mode half of doc/codex/issues/0068: its BODY case is
# the one the other two modes report and this one cannot, so the deck freezes
# a silence that is a stated gap rather than an accident.
CLEANFILES = PrintTail.txt dpc_*.raw dpc_*.txt bun_*.cir bun_*.txt \
	dun_*.cir dun_*.txt sur_*.cir sur_*.txt smr_*.cir smr_*.txt \
	scr_*.cir scr_*.txt wls_*.raw ncc_*.cir ncc_*.txt

MAINTAINERCLEANFILES = Makefile.in
