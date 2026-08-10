## Process this file with automake to produce Makefile.in

## Every deck in this directory is run with the identifier case mode set to
## 'preserve' AND the PSpice dialect selected, because pspice_compat() is
## called only under newcompat.ps.  check.sh takes the binary as an
## unquoted $1, so the extra flags ride on that argument.
##
## The device cards under test live in .inc files rather than in the decks:
## pspice_compat() runs on the card list inp_read() returns for an INCLUDED
## file (src/frontend/inpcom.c:1754), not on the top-level deck.

TESTS = pspice-table-e-case.cir pspice-table-e-case-lower.cir \
	pspice-tc-r-case.cir pspice-tc-r-case-lower.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) $(SHELL) $(top_srcdir)/tests/bin/check.sh "$(top_builddir)/src/ngspice -D casemode=preserve -D ngbehavior=ps"

EXTRA_DIST = \
	pspice-table-e-case.inc \
	pspice-table-e-case-lower.inc \
	pspice-tc-r-case.inc \
	pspice-tc-r-case-lower.inc \
	$(TESTS) \
	$(TESTS:.cir=.out)

MAINTAINERCLEANFILES = Makefile.in
