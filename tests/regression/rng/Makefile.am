## Process this file with automake to produce Makefile.in

## Decks about the random number generator's seeding.  They are driven by
## tests/bin/check_status.sh, not check.sh, for the same reason as
## tests/regression/exitstatus/: their subject is a 'Warning' on stderr, and
## check.sh's FILTER deletes every line containing 'Warning' and never
## captures stderr at all.  check_status.sh keeps both, and its header
## documents the per-deck .status and .err files.
##
## The four option-seed-* decks are ver_51 batch item 2 (P2).  eval_opt()
## re-seeds from '.option seed=' on every parse, and 'reset' is a parse, so
## the option silently overrides a 'setseed N' made before the reset.  The
## option keeps winning (warn-only), and after an explicit 'setseed N' the
## simulator says so once per session:
##
##   option-seed-setseed.cir         the warning, exactly once over three
##                                   resets, and the option still wins
##   option-seed-random-setseed.cir  the seed=random wording, exactly once
##   option-seed-nosetseed.cir       no setseed: stderr is empty
##   option-seed-absent.cir          setseed without the option: stderr is
##                                   empty, and setseed is honoured
##
## The next two are ver_51 batch item 3 (P3).  initw() used to re-seed the
## generator from the process id after startup had seeded it from
## rndseed=1; now startup re-seeds from $rndseed after initw():
##
##   startup-equals-setseed1.cir     a deck's first draw equals the draw
##                                   after 'setseed 1', in one process
##   setseed7-bit-identity.cir       'setseed 7' still draws the 45.2 value
##                                   (glibc rand(), so Linux-only)
##
## An empty .err is an assertion that stderr is empty, not a missing file.

TESTS = option-seed-setseed.cir option-seed-random-setseed.cir \
	option-seed-nosetseed.cir option-seed-absent.cir \
	startup-equals-setseed1.cir setseed7-bit-identity.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) \
	$(SHELL) $(top_srcdir)/tests/bin/check_status.sh $(top_builddir)/src/ngspice

EXTRA_DIST = \
	$(TESTS) \
	option-seed-setseed.status option-seed-setseed.err \
	option-seed-random-setseed.status option-seed-random-setseed.err \
	option-seed-nosetseed.status option-seed-nosetseed.err \
	option-seed-absent.status option-seed-absent.err \
	startup-equals-setseed1.status startup-equals-setseed1.err \
	setseed7-bit-identity.status setseed7-bit-identity.err

# The driver captures each run's stdout and stderr into the build directory
# and leaves them there when a deck fails.
CLEANFILES = \
	option-seed-setseed.stdout option-seed-setseed.stderr \
	option-seed-setseed.stderr.filtered option-seed-setseed.stderr.expected \
	option-seed-random-setseed.stdout option-seed-random-setseed.stderr \
	option-seed-random-setseed.stderr.filtered option-seed-random-setseed.stderr.expected \
	option-seed-nosetseed.stdout option-seed-nosetseed.stderr \
	option-seed-nosetseed.stderr.filtered option-seed-nosetseed.stderr.expected \
	option-seed-absent.stdout option-seed-absent.stderr \
	option-seed-absent.stderr.filtered option-seed-absent.stderr.expected \
	startup-equals-setseed1.stdout startup-equals-setseed1.stderr \
	startup-equals-setseed1.stderr.filtered startup-equals-setseed1.stderr.expected \
	setseed7-bit-identity.stdout setseed7-bit-identity.stderr \
	setseed7-bit-identity.stderr.filtered setseed7-bit-identity.stderr.expected \
	rawspice.raw

MAINTAINERCLEANFILES = Makefile.in
