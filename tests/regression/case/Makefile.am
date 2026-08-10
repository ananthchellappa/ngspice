## Process this file with automake to produce Makefile.in

## Every deck in this directory is run with the identifier case mode set to
## 'preserve'.  check.sh takes the binary as an unquoted $1, so the extra
## flags ride on that argument; tests/xspice/digital/Makefile.am is the
## precedent.

TESTS = harness-alive.cir name-roundtrip.cir alter-case.cir \
	gnd-alias-case.cir option-card-case.cir node-case-alias.cir \
	write-roundtrip.cir case-flag-noop.cir \
	device-letter-model-case.cir device-letter-model-case-lower.cir \
	device-letter-case.cir device-letter-case-lower.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) $(SHELL) $(top_srcdir)/tests/bin/check.sh "$(top_builddir)/src/ngspice -D casemode=preserve"

EXTRA_DIST = \
	$(TESTS) \
	$(TESTS:.cir=.out)

# write-roundtrip.cir writes a rawfile into the build directory
CLEANFILES = casewrite.raw

MAINTAINERCLEANFILES = Makefile.in
