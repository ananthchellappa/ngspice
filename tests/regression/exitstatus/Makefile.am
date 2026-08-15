## Process this file with automake to produce Makefile.in

## Decks whose subject is the exit status of a batch run, and what the run
## said and left behind while returning it.  Unlike every other directory
## under tests/regression, these are not driven by tests/bin/check.sh: that
## script runs 'ngspice --batch' and discards the result.  '$?' appears
## nowhere in it, stderr is never redirected, and its only failure path is the
## diff of filtered stdout against a committed .out.  So a check.sh directory
## cannot see a status at all, and its FILTER strips 'Error' and 'Warning' as
## unanchored substrings, which would delete the subject of a deck that is
## about a diagnostic.  tests/regression/pipe/ is the one directory whose
## verdict is an exit status, but it drives 'ngspice -p', which never enters
## batch mode.  Hence tests/bin/check_status.sh, whose header documents the
## per-deck assertion files: .status (required), .err, .files, .mustfail.
##
## tests/lint/ is the in-tree precedent for a test directory that runs its own
## driver, and selftest-status.cir follows its selftest.lint: a deck whose
## .status is deliberately wrong and whose .mustfail names the check that has
## to report it.  It passes only while the driver can still fail, which is the
## one thing a harness directory owes its own suite.
##
## The assertion files deliberately avoid the .out extension, which
## tests/.gitignore ignores globally.

TESTS = tran-empty-netlist.cir selftest-status.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) \
	$(SHELL) $(top_srcdir)/tests/bin/check_status.sh $(top_builddir)/src/ngspice

EXTRA_DIST = \
	$(TESTS) \
	tran-empty-netlist.status \
	tran-empty-netlist.err \
	tran-empty-netlist.files \
	selftest-status.status \
	selftest-status.mustfail

# The driver captures each run's stdout and stderr into the build directory
# and filters the latter in place; tran-empty-netlist.files asserts that the
# default rawfile was not written, and the driver deletes that path before the
# run, so a stale one has to be cleanable too.
CLEANFILES = \
	tran-empty-netlist.stdout tran-empty-netlist.stderr \
	tran-empty-netlist.stderr.filtered tran-empty-netlist.stderr.expected \
	selftest-status.stdout selftest-status.stderr \
	selftest-status.stderr.filtered selftest-status.stderr.expected \
	rawspice.raw

MAINTAINERCLEANFILES = Makefile.in
