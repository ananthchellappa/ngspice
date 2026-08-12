## Process this file with automake to produce Makefile.in

## The build-enforced identifier-comparison lint.  Unlike every other
## directory under tests/, nothing here runs ngspice: the driver reads the
## source tree with $(AWK) and compares what it finds against a frozen list.
## So this directory is mode independent by construction -- it does not link
## against the simulator and no casemode reaches it -- and it is the reason
## TESTS_ENVIRONMENT does not name $(top_builddir)/src/ngspice the way
## tests/bin/check.sh's callers do.
##
## It is last in tests/Makefile.am's SUBDIRS on purpose.  A failing directory
## aborts make check's recursion, so a directory that fails costs the suite
## every directory after it; from last position a lint failure costs nothing
## and the contributor sees the full deck result and the lint report in one
## run.  See doc/claude/decisions/0011-identity-lint.md decision 3.

TESTS = identity.lint selftest.lint

TESTS_ENVIRONMENT = ngspice_srcdir=$(top_srcdir) AWK="$(AWK)" \
	$(SHELL) $(top_srcdir)/tests/bin/identity_lint.sh

EXTRA_DIST = \
	$(TESTS) \
	identity.baseline \
	selftest.baseline \
	selftest/flagged.c \
	selftest/silent.c

MAINTAINERCLEANFILES = Makefile.in
