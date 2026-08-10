## Process this file with automake to produce Makefile.in

## Every deck in this directory is run with the identifier case mode set to
## 'preserve' AND the LTspice dialect selected, because the sites it covers
## are gated on newcompat.lt.  check.sh takes the binary as an unquoted $1,
## so the extra flags ride on that argument; tests/regression/case and
## tests/xspice/digital are the precedents.

TESTS = rkm-r-case.cir rkm-r-case-lower.cir \
	rkm-c-case.cir rkm-c-case-lower.cir \
	rkm-l-case.cir rkm-l-case-lower.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) $(SHELL) $(top_srcdir)/tests/bin/check.sh "$(top_builddir)/src/ngspice -D casemode=preserve -D ngbehavior=lt"

EXTRA_DIST = \
	$(TESTS) \
	$(TESTS:.cir=.out)

MAINTAINERCLEANFILES = Makefile.in
