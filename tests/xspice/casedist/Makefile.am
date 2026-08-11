## Process this file with automake to produce Makefile.in

## Every deck in this directory is run with the identifier case mode set to
## 'distinguish', where two spellings of a name are two names.  It cannot
## share tests/xspice/case, which runs under 'preserve': there two
## case-variant names are correctly one name, so a deck asserting that they
## are two would be asserting the wrong thing.  That is the same split
## tests/regression/casedist makes against tests/regression/case.
##
## It cannot live in tests/regression/casedist either, because these decks
## instantiate XSPICE A devices and tests/bin/spinit loads no code models.
## Hence the local spinit.in and SPICE_SCRIPTS=., copied from
## tests/xspice/case.  check.sh takes the binary as an unquoted $1, so the
## extra flag rides on that argument.

TESTS = harness-alive.cir event-node-lookup-case-split.cir \
	event-node-case-split.cir event-node-dangling-no-variant.cir \
	auto-bridge-node-case-split.cir hand-bridge-node-case-split.cir \
	auto-bridge-vcc-param-case.cir auto-bridge-vcc-param-case-lower.cir \
	auto-bridge-family-param-case.cir \
	auto-bridge-family-param-case-lower.cir \
	auto-bridge-vcc-subckt-case.cir auto-bridge-vcc-subckt-case-lower.cir \
	auto-bridge-vcc-param-ambiguous.cir \
	auto-bridge-vcc-nested-subckt-case.cir \
	auto-bridge-vcc-param-subckt-name.cir \
	auto-bridge-node-case-report.cir event-node-case-report.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) SPICE_SCRIPTS=. $(SHELL) $(top_srcdir)/tests/bin/check.sh "$(top_builddir)/src/ngspice -D casemode=distinguish"

# auto-bridge-node-case-report.cir and event-node-case-report.cir assert on a
# diagnostic.  Both are written during the parse, before any .control block
# runs, so each deck writes the circuits it is about with echo and sources
# them with '>&' to capture cp_err.  The sub-decks and the captures land in
# the build directory.
CLEANFILES = abr_*.txt abr_*.cir enr_*.txt enr_*.cir

EXTRA_DIST = \
	$(TESTS) \
	$(TESTS:.cir=.out)

MAINTAINERCLEANFILES = Makefile.in
