## Process this file with automake to produce Makefile.in


TESTS = temper-1.cir temper-2.cir temper-3.cir temper-res-1.cir \
	temper-res-behav-1.cir temper-res-behav-noise.cir \
	temper-res-behav-noise-subckt.cir temper-res-behav-dup.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) $(SHELL) $(top_srcdir)/tests/bin/check.sh $(top_builddir)/src/ngspice

EXTRA_DIST = \
	$(TESTS) \
	$(TESTS:.cir=.out) \
	temper-res-behav-1.lib \
	temper-res-behav-noise.sp \
	temper-res-behav-noise-subckt.sp

MAINTAINERCLEANFILES = Makefile.in
