## Process this file with automake to produce Makefile.in
## regression first, then xspice, then the model qua tests, then the lint

SUBDIRS = regression

if XSPICE_WANTED
SUBDIRS += xspice
endif

if SHORT_CHECK_WANTED
SUBDIRS +=		\
	bsim3		\
	bsim4
endif

if !SHORT_CHECK_WANTED
SUBDIRS +=		\
	bsim3		\
	bsim4		\
	bsimsoi		\
	hisim		\
	hicum2		\
	hisimhv1	\
	hisimhv2
endif

## Last, and unconditional: it runs no binary, so no configure flag gates it.
## A failing directory aborts the recursion, so from here a lint failure costs
## the suite nothing that has not already run.
SUBDIRS += lint

DIST_SUBDIRS =		\
	bsim1		\
	bsim2		\
	bsim3		\
	bsim3soidd	\
	bsim3soifd	\
	bsim3soipd	\
	bsim4		\
	bsimsoi		\
	filters		\
	general		\
	hfet		\
	hicum2		\
	hisim		\
	hisimhv1	\
	hisimhv2	\
	jfet		\
	lint		\
	mes		\
	mesa		\
	mos6		\
	polezero	\
	regression	\
	resistance	\
	sensitivity	\
	transient	\
	transmission	\
	vbic		\
	xspice


TESTS_ENVIRONMENT = \
	$(SHELL) $(top_srcdir)/tests/bin/check.sh $(top_builddir)/src/ngspice

EXTRA_DIST = \
	README bin .gitignore

MAINTAINERCLEANFILES = Makefile.in
