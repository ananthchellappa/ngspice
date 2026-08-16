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
##
## The four sim-status-* decks are doc/codex/issues/0069 criterion 3: the
## $sim_status guard, in the client's own deck shape (an analysis dot card
## AND a .control run, which is the combination that otherwise exits 0), plus
## its negative control and the three properties 0069's Resolution measures.
## They are the first decks anywhere under tests/ that mention sim_status.

TESTS = tran-empty-netlist.cir selftest-status.cir \
	sim-status-guard.cir sim-status-guard-ok.cir \
	sim-status-properties.cir sim-status-unset.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) \
	$(SHELL) $(top_srcdir)/tests/bin/check_status.sh $(top_builddir)/src/ngspice

EXTRA_DIST = \
	$(TESTS) \
	tran-empty-netlist.status \
	tran-empty-netlist.err \
	tran-empty-netlist.files \
	selftest-status.status \
	selftest-status.mustfail \
	sim-status-guard.status \
	sim-status-guard.files \
	sim-status-guard-ok.status \
	sim-status-guard-ok.files \
	sim-status-properties.status \
	sim-status-unset.status \
	sim-status-unset.err

# The driver captures each run's stdout and stderr into the build directory
# and filters the latter in place; tran-empty-netlist.files asserts that the
# default rawfile was not written, and the driver deletes that path before the
# run, so a stale one has to be cleanable too.  sim-status-guard-ok.cir writes
# its rawfile from inside its .control block and sim-status-guard.cir would if
# its guard ever stopped firing, so both names are listed here whether or not
# the run that produced them passed.
CLEANFILES = \
	tran-empty-netlist.stdout tran-empty-netlist.stderr \
	tran-empty-netlist.stderr.filtered tran-empty-netlist.stderr.expected \
	selftest-status.stdout selftest-status.stderr \
	selftest-status.stderr.filtered selftest-status.stderr.expected \
	sim-status-guard.stdout sim-status-guard.stderr \
	sim-status-guard.stderr.filtered sim-status-guard.stderr.expected \
	sim-status-guard.raw \
	sim-status-guard-ok.stdout sim-status-guard-ok.stderr \
	sim-status-guard-ok.stderr.filtered sim-status-guard-ok.stderr.expected \
	sim-status-guard-ok.raw \
	sim-status-properties.stdout sim-status-properties.stderr \
	sim-status-properties.stderr.filtered sim-status-properties.stderr.expected \
	sim-status-unset.stdout sim-status-unset.stderr \
	sim-status-unset.stderr.filtered sim-status-unset.stderr.expected \
	rawspice.raw

MAINTAINERCLEANFILES = Makefile.in
