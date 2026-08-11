## Process this file with automake to produce Makefile.in

## Every deck in this directory is run with the identifier case mode set to
## 'distinguish', where two spellings of a name are two names.  It cannot
## share tests/regression/case, which runs under 'preserve': there two
## case-variant nets are correctly one net, so a deck asserting that they are
## two would be asserting the wrong thing.  check.sh takes the binary as an
## unquoted $1, so the extra flag rides on that argument;
## tests/regression/case and tests/regression/case-lt are the precedents.

TESTS = harness-alive.cir node-case-split.cir \
	instance-case-split.cir model-case-split.cir param-case-split.cir \
	bsource-node-case.cir bsource-forward-ref.cir \
	vector-unlet-case.cir vector-compose-case.cir vector-unlet-report.cir \
	vector-scale-case.cir vector-diff-case.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) $(SHELL) $(top_srcdir)/tests/bin/check.sh "$(top_builddir)/src/ngspice -D casemode=distinguish"

# vector-unlet-report.cir and vector-scale-case.cir redirect cp_err to a file
# and read it back, which is the only way a deck in this harness can assert on
# a diagnostic: tests/bin/check.sh captures stdout only and filters every line
# containing 'Warning'.  The captures land in the build directory.
CLEANFILES = vuc_*.txt vsc_*.txt

EXTRA_DIST = \
	$(TESTS) \
	$(TESTS:.cir=.out)

MAINTAINERCLEANFILES = Makefile.in
