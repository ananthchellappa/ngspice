## Process this file with automake to produce Makefile.in

TESTS = \
	d_ram.cir          \
	d_source.cir       \
	d_state.cir        \
	auto-bridge-family-null.cir \
	auto-bridge-family-empty.cir \
	auto-bridge-node-case-fold.cir \
	event-node-case-fold.cir \
	save-event-node.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) SPICE_SCRIPTS=. $(SHELL) $(top_srcdir)/tests/bin/check.sh "$(top_builddir)/src/ngspice -r foobaz"

# save-event-node.cir asserts on a diagnostic that goes to cp_err, which
# check.sh neither captures nor would let through its filter, so it writes the
# circuits it is about with echo and sources them with '>&'.  The sub-decks
# and the captures land in the build directory.
CLEANFILES = sen_*.cir sen_*.txt

EXTRA_DIST = \
	d_ram-stimulus.txt \
	d_source-stimulus.txt \
	d_state-stimulus.txt \
	d_state-updn.txt \
	$(TESTS) \
	$(TESTS:.cir=.out)

MAINTAINERCLEANFILES = Makefile.in
