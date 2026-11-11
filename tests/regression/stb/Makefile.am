## Process this file with automake to produce Makefile.in

TESTS = stb-version.cir stb-flat.cir stb-bilateral.cir stb-grounded.cir stb-sweeps.cir stb-sources.cir stb-names.cir stb-card-run.cir stb-repeat.cir stb-klu.cir stb-klu-hertz.cir stb-save.cir stb-dotmeas.cir stb-fail.cir stb-opfail.cir stb-singular.cir stb-plots.cir stb-twopole.cir stb-threepole.cir stb-margin-cases.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) $(SHELL) $(top_srcdir)/tests/bin/check.sh $(top_builddir)/src/ngspice

EXTRA_DIST = \
	$(TESTS) \
	$(TESTS:.cir=.out)

CLEANFILES = *.stblog *.stberr *.raw

MAINTAINERCLEANFILES = Makefile.in
