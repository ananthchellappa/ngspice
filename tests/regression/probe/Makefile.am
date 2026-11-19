## Process this file with automake to produce Makefile.in

## .probe I(...): the current into a terminal of an instance at any depth
## of the hierarchy, or into a port of a subcircuit instance.
## doc/claude/specs/terminal-current-probes.md

TESTS = probe-hier.cir probe-names.cir probe-ac.cir probe-tran.cir \
	probe-refuse.cir probe-reset.cir probe-save.cir probe-saveall.cir \
	probe-stb-shared.cir probe-alli.cir probe-legacy.cir \
	probe-subckt-ports.cir probe-depth-names.cir probe-version.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) $(SHELL) $(top_srcdir)/tests/bin/check.sh $(top_builddir)/src/ngspice

EXTRA_DIST = \
	$(TESTS) \
	$(TESTS:.cir=.out)

MAINTAINERCLEANFILES = Makefile.in
