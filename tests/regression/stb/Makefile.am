## Process this file with automake to produce Makefile.in

TESTS = stb-version.cir stb-flat.cir stb-bilateral.cir stb-grounded.cir stb-sweeps.cir stb-sources.cir stb-names.cir stb-card-run.cir stb-repeat.cir stb-klu.cir stb-klu-hertz.cir stb-save.cir stb-dotmeas.cir stb-fail.cir stb-errors.cir stb-card-bad.cir stb-card-subckt.cir stb-opfail.cir stb-singular.cir stb-plots.cir stb-twopole.cir stb-threepole.cir stb-margin-cases.cir stb-rawfile.cir stb-rawfile-keepop.cir stb-card-print.cir stb-card-print-keepop.cir stb-card-plot.cir stb-meas.cir stb-keepop.cir stb-keepop-klu.cir term-prim.cir term-title.cir term-title-card.cir term-port.cir term-scope.cir term-if.cir term-collect.cir term-reset.cir term-forged.cir term-uic.cir term-uic-klu.cir term-dotcard.cir term-uncollected.cir term-hints.cir term-reserved.cir term-loopgain-port.cir term-loopgain-res.cir term-loopgain-klu.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) $(SHELL) $(top_srcdir)/tests/bin/check.sh $(top_builddir)/src/ngspice

EXTRA_DIST = \
	$(TESTS) \
	$(TESTS:.cir=.out)

CLEANFILES = *.stblog *.stberr *.raw

MAINTAINERCLEANFILES = Makefile.in
