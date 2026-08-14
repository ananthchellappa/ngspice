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
	vector-scale-case.cir vector-diff-case.cir \
	vector-rawfile-scale-case.cir vector-diff-pairing-case.cir \
	vector-diff-wrapper-case.cir vector-diff-branch-case.cir \
	bsource-node-case-report.cir sens-node-case-report.cir \
	vector-let-report.cir vector-probe-report.cir \
	vector-pyplot-probe-report.cir subckt-formal-pin-case.cir \
	udf-name-case.cir save-name-case.cir save-undef-report.cir \
	save-near-miss-node-list.cir \
	casemode-announce-report.cir curcasemode-relatch.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) $(SHELL) $(top_srcdir)/tests/bin/check.sh "$(top_builddir)/src/ngspice -D casemode=distinguish"

# vector-unlet-report.cir and vector-scale-case.cir redirect cp_err to a file
# and read it back, which is the only way a deck in this harness can assert on
# a diagnostic: tests/bin/check.sh captures stdout only and filters every line
# containing 'Warning'.  The captures land in the build directory.
# vector-rawfile-scale-case.cir also writes the rawfile it loads, because the
# raw writer never emits the scale= parameter that deck is about.
# bsource-node-case-report.cir writes the sub-decks it sources as well as the
# captures, because the diagnostic it asserts on is written during the parse
# and 'source' is the only command a redirect can be attached to.
# vector-diff-pairing-case.cir writes the rawfile it loads for the same reason
# as vector-rawfile-scale-case.cir: the second plot of a cross-plot diff has
# to spell a name the way another tool would.  vector-diff-wrapper-case.cir
# writes both of its plots, and vector-diff-branch-case.cir the one it pairs
# against a live run, because only a file can spell a current as i(name):
# ngspice stores its own branch currents as name#branch.
# sens-node-case-report.cir writes its sub-decks and captures for
# bsource-node-case-report.cir's reason.  vector-let-report.cir writes both for
# a different reason: 'let' is on control.c's noredirect[] list, because
# 'let a = b > c' needs '>' as an operator, so the only way to put a 'let'
# under a redirect is to source a deck whose control block runs it.
# vector-probe-report.cir writes sub-decks for the same reason -- 'define' is
# on that list too -- and one of its cases writes an SVG, because the only way
# to reach plotit()'s 'vs' separator in batch mode is 'hardcopy': 'plot' is
# refused there.  Its sub-decks are named in UPPER CASE, which the deck's own
# header explains: it is the only spelling the differential sweep's uppercaser
# leaves agreeing with the 'source' line that reads them.
# vector-pyplot-probe-report.cir needs no sub-deck: 'pyplot' takes the redirect
# directly, and its cases stop before any python is started.
# subckt-formal-pin-case.cir writes its sub-decks and captures for
# bsource-node-case-report.cir's reason: the near-miss it asserts on is
# written during inp_subcktexpand(), before any control line runs.
# save-undef-report.cir writes its sub-decks and captures for
# bsource-node-case-report.cir's reason, except that its diagnostic is written
# during the run rather than during the parse: the redirect has to be in force
# across the 'source' that runs the sub-deck carrying the .save card.
# save-near-miss-node-list.cir has save-undef-report.cir's shape and its
# reason, for the twin scan's fallback to the circuit's node list: the deck it
# needs has an analysis that builds its own column list, so the miss cannot be
# reached from the outer deck's own circuit.
# casemode-announce-report.cir writes its sub-decks and captures for
# bsource-node-case-report.cir's reason as well: set_case_mode() announces the
# case policy during inp_readall(), so 'source' is the only command a redirect
# can be attached to.  It is also the deck that asserts how many times the
# announcement is made, which is doc/codex/issues/0058.
# curcasemode-relatch.cir writes the sub-deck it sources for a different
# reason again: what it asserts is that a 'source' re-latches the case mode,
# so it needs a second netlist read and not a redirect.
CLEANFILES = vuc_*.txt vsc_*.txt vsr_*.txt vsr_*.raw bnr_*.txt bnr_*.cir \
	sur_*.txt sur_*.cir snm_*.txt snm_*.cir \
	vdp_*.txt vdp_*.raw vdw_*.txt vdw_*.raw vdb_*.txt vdb_*.raw \
	snr_*.txt snr_*.cir vlr_*.txt vlr_*.cir cma_*.txt cma_*.cir \
	vpr_*.txt vpr_*.svg VPR_*.CIR vpp_*.txt sfp_*.txt sfp_*.cir \
	ccr_*.cir

EXTRA_DIST = \
	$(TESTS) \
	$(TESTS:.cir=.out)

MAINTAINERCLEANFILES = Makefile.in
