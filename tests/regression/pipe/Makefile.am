## Process this file with automake to produce Makefile.in

TESTS = alter-1.cmd flow-keyword-case.cmd \
	analysis-keyword-case.cmd options-keyword-case.cmd \
	resource-keyword-case.cmd spiceif-keyword-case.cmd \
	inpcompat-keyword-case.cmd plot-keyword-case.cmd \
	breakp-keyword-case.cmd postcoms-keyword-case.cmd \
	variable-keyword-case.cmd spec-keyword-case.cmd \
	device-keyword-case.cmd exprcoms-keyword-case.cmd \
	xspice-keyword-case.cmd shell-keyword-case.cmd \
	define-const-body.cmd save-name-case.cmd \
	curcasemode-effect.cmd \
	rawfile-option-computed-name.cmd

TESTS_ENVIRONMENT = \
	SPICE_SCRIPTS=$(top_srcdir)/tests/bin \
	ngspice_vpath=$(srcdir) \
	$(SHELL) -c '$(top_builddir)/src/ngspice -p < $$1' --

EXTRA_DIST = \
	$(TESTS) \
    alter-1.cir

# The keyword-case tests assert on values they read back off disk, so they
# write rawfiles, plot output and scratch markers into the build directory.
CLEANFILES = \
	rawspice.raw \
	devkc_*.txt devkc_*.mod \
	exprcoms-pn-*.raw \
	ic_eg_*.raw \
	optcase-*.raw optcase-*.tmp optcase-*.txt optcase-*.m0 optcase-*.m1 \
	optcase-*.m2 \
	pkc_*.svg pkc_*.txt pkc_*.raw \
	rusage-case.tmp \
	shkc_*.txt shkc_*.raw shkc_*.svg \
	variable-keyword-case*.raw variable-keyword-case*.tmp \
	ccp_*.cir ccp_*.raw \
	rcn_*.raw rcn_*.txt \
	filesource.txt

MAINTAINERCLEANFILES = Makefile.in
