## Process this file with automake to produce Makefile.in

## Decks whose subject is the exit status of a batch run, and what the run
## said and left behind while returning it.  Unlike every other directory
## under tests/regression, these are not driven by tests/bin/check.sh: that
## script runs 'ngspice --batch' and discards the result.  '$?' appears
## nowhere in it, stderr is never redirected, and its only failure path is the
## diff of filtered stdout against a committed .out.  So a check.sh directory
## cannot see a status at all, and its FILTER strips 'Error' and 'Warning' as
## unanchored substrings, which would delete the subject of a deck that is
## about a diagnostic.  tests/regression/pipe/ is the one directory whose
## verdict is an exit status, but it drives 'ngspice -p', which never enters
## batch mode.  Hence tests/bin/check_status.sh, whose header documents the
## per-deck assertion files: .status (required), .err, .files, .mustfail.
##
## tests/lint/ is the in-tree precedent for a test directory that runs its own
## driver, and selftest-status.cir follows its selftest.lint: a deck whose
## .status is deliberately wrong and whose .mustfail names the check that has
## to report it.  It passes only while the driver can still fail, which is the
## one thing a harness directory owes its own suite.
##
## selftest-invoke.cir is the same idea for the fifth assertion file, .invoke,
## which says how the deck is invoked:
##
##     route  batch | stdin | notty     (batch is the default and the
##                                       behaviour of a deck with no .invoke)
##     flags  <words put on the command line before the deck>
##
## An unknown key, an unknown route, a repeated key or a 'flags' with no words
## is a broken test spec, not a test failure: the driver exits 1 through
## spec_error() and .mustfail never inverts it.  selftest-invoke.cir declares
## both keys and its .files asserts one artefact per key, so the entry fails
## -- as a self-test, loudly -- if the sidecar is ever read and dropped.
##
## The assertion files deliberately avoid the .out extension, which
## tests/.gitignore ignores globally.
##
## The four sim-status-* decks are doc/codex/issues/0069 criterion 3: the
## $sim_status guard, in the client's own deck shape (an analysis dot card
## AND a .control run, which is the combination that otherwise exits 0), plus
## its negative control and the three properties 0069's Resolution measures.
## They are the first decks anywhere under tests/ that mention sim_status.
##
## The three op-empty-* decks are doc/codex/issues/0072 criterion 6, which
## says in as many words that no existing harness can assert that fix.  They
## are the three deck shapes its criterion 1 names -- an .op dot card alone,
## with a .tran card, and with a .control run block -- each on a netlist with
## no non-ground node, which is the state that used to reach
## assert(plot_cur->pl_dvecs != NULL) in ft_cktcoms() and kill the process
## with SIGABRT.  op-empty-netlist.cir is that issue's reproducer byte for
## byte and carries no comment block for that reason; its .status sibling
## holds the commentary.  The criterion is "terminates normally, non-zero,
## with a diagnostic", so all three assert a status AND the stderr text, and
## the driver's signal check turns a regression back into two named failures.
##
## op-empty-stdin.cir and op-empty-notty.cir are the same six bytes again, and
## they are here for the other dimension of that criterion: not the three deck
## shapes but the three ROUTES into the one ft_cktcoms() call at main.c:1581.
## Each carries an .invoke -- 'route stdin' is 'ngspice -b -n < deck', 'route
## notty' is 'ngspice -n deck < /dev/null', the row 0072 calls the trap because
## no tty and no -i is batch mode whether or not anyone typed -b
## (main.c:1175).  The three shapes x three routes cross product is
## deliberately not built: the route dimension is what nothing asserted, the
## shape dimension is already covered above under the default batch route, and
## nine decks for one code path reached three ways is not worth the EXTRA_DIST.
##
## What these two decks prove, exactly.  They prove that the six-byte
## reproducer returns 1 with the three-line diagnostic and no signal when the
## driver is told to run it these two ways -- which is the property criterion 1
## asks for, and which used to be rc=134 on all three routes.  They do NOT
## witness the route from inside: the deck returns before any control language
## runs, so it cannot read $?batchmode or $inputdir, and the diagnostic is
## byte-identical on all three routes (measured, sha256 equal, stdout too).
## Measured consequence: deleting the 'route' line from either .invoke leaves
## the entry passing, because the default batch route produces the same verdict
## for this deck.  What holds the route honest is elsewhere -- the driver
## prints its exact command line on every failure, and selftest-invoke.cir
## asserts the two cp variables that do separate the routes.
##
## op-empty-raw.cir is those same six bytes a third time, and it is the one
## op-empty-* deck whose expected status is 0.  It is doc/codex/issues/0072
## criterion 5, which asks the opposite question from criterion 1: not "does
## this route report the mistake" but "is this route, which never reported it,
## unchanged by the fix, and is what it writes a valid raw file rather than a
## third thing".  Its .invoke carries no route -- the route is the default
## batch -- and one 'flags -r op-empty-raw.raw' line.  Under -r the run streams
## to a file: OUTpBeginPlot() calls fileInit() instead of plotInit()
## (src/frontend/outitf.c:494-:497), no plot is linked into plot_list,
## setcplot("op") in ft_cktcoms() answers NULL, and the dotcards.c:225 guard
## that makes the other five op-empty-* decks exit 1 is never reached.  It also
## puts this deck on the rflag arm of the batch epilogue, main.c:1561, and so
## on the other of ft_cktcoms()'s two call sites, main.c:1573 with terse=TRUE;
## every other deck about the simulator's behaviour here arrives at main.c:1581
## with terse=FALSE.  (selftest-invoke.cir carries a 'flags -r' line too and
## takes the same arm, but its subject is the driver, not the simulator.)  Zero
## is therefore the correct status and not a bug; the .status sibling says so
## at length so that a reader who finds one 0 among five 1s does not "fix" it.
##
## Unlike the two route decks above, this one DOES witness its own sidecar.
## Its .files asserts the rawfile's presence and two header lines that are
## neither clock- nor build-specific -- 'No. Variables: 0', which is the whole
## of criterion 5, and 'Plotname: Operating Point'.  Delete the flags line and
## no rawfile is written, so all three fail.  It is, with selftest-invoke.cir,
## one of the only two committed decks in this directory whose .invoke is
## load-bearing.

TESTS = tran-empty-netlist.cir selftest-status.cir selftest-invoke.cir \
	sim-status-guard.cir sim-status-guard-ok.cir \
	sim-status-properties.cir sim-status-unset.cir \
	op-empty-netlist.cir op-empty-tran.cir op-empty-control.cir \
	op-empty-stdin.cir op-empty-notty.cir op-empty-raw.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) \
	$(SHELL) $(top_srcdir)/tests/bin/check_status.sh $(top_builddir)/src/ngspice

EXTRA_DIST = \
	$(TESTS) \
	tran-empty-netlist.status \
	tran-empty-netlist.err \
	tran-empty-netlist.files \
	selftest-status.status \
	selftest-status.mustfail \
	selftest-invoke.status \
	selftest-invoke.mustfail \
	selftest-invoke.invoke \
	selftest-invoke.files \
	sim-status-guard.status \
	sim-status-guard.files \
	sim-status-guard-ok.status \
	sim-status-guard-ok.files \
	sim-status-properties.status \
	sim-status-unset.status \
	sim-status-unset.err \
	op-empty-netlist.status \
	op-empty-netlist.err \
	op-empty-netlist.files \
	op-empty-tran.status \
	op-empty-tran.err \
	op-empty-control.status \
	op-empty-control.err \
	op-empty-stdin.status \
	op-empty-stdin.err \
	op-empty-stdin.invoke \
	op-empty-notty.status \
	op-empty-notty.err \
	op-empty-notty.invoke \
	op-empty-raw.status \
	op-empty-raw.invoke \
	op-empty-raw.files

# The driver captures each run's stdout and stderr into the build directory
# and filters the latter in place; tran-empty-netlist.files asserts that the
# default rawfile was not written, and the driver deletes that path before the
# run, so a stale one has to be cleanable too.  sim-status-guard-ok.cir writes
# its rawfile from inside its .control block and sim-status-guard.cir would if
# its guard ever stopped firing, so both names are listed here whether or not
# the run that produced them passed.  selftest-invoke.cir leaves two artefacts
# behind on a passing run, one per .invoke key: selftest-invoke.raw, which the
# 'flags -r' line causes, and selftest-invoke.evidence, which its .control
# block writes so that the route can be read off a file.  op-empty-raw.cir
# leaves one for the same reason -- op-empty-raw.raw, which its own 'flags -r'
# line causes and which its .files asserts on.
#
# 'core' and 'core.*' are here for five of the six op-empty-* decks -- every
# one but op-empty-raw.cir, whose whole subject is that its route never
# aborted.  No committed deck dumps one now, and that is the point of them:
# the state the other five assert used to abort, and an abort leaves a core
# file in whichever directory the run was made from, which is this one.  A
# regression that fails those decks should not also leave an unlisted artefact
# behind.
CLEANFILES = \
	tran-empty-netlist.stdout tran-empty-netlist.stderr \
	tran-empty-netlist.stderr.filtered tran-empty-netlist.stderr.expected \
	selftest-status.stdout selftest-status.stderr \
	selftest-status.stderr.filtered selftest-status.stderr.expected \
	selftest-invoke.stdout selftest-invoke.stderr \
	selftest-invoke.stderr.filtered selftest-invoke.stderr.expected \
	selftest-invoke.raw selftest-invoke.evidence \
	sim-status-guard.stdout sim-status-guard.stderr \
	sim-status-guard.stderr.filtered sim-status-guard.stderr.expected \
	sim-status-guard.raw \
	sim-status-guard-ok.stdout sim-status-guard-ok.stderr \
	sim-status-guard-ok.stderr.filtered sim-status-guard-ok.stderr.expected \
	sim-status-guard-ok.raw \
	sim-status-properties.stdout sim-status-properties.stderr \
	sim-status-properties.stderr.filtered sim-status-properties.stderr.expected \
	sim-status-unset.stdout sim-status-unset.stderr \
	sim-status-unset.stderr.filtered sim-status-unset.stderr.expected \
	op-empty-netlist.stdout op-empty-netlist.stderr \
	op-empty-netlist.stderr.filtered op-empty-netlist.stderr.expected \
	op-empty-tran.stdout op-empty-tran.stderr \
	op-empty-tran.stderr.filtered op-empty-tran.stderr.expected \
	op-empty-control.stdout op-empty-control.stderr \
	op-empty-control.stderr.filtered op-empty-control.stderr.expected \
	op-empty-stdin.stdout op-empty-stdin.stderr \
	op-empty-stdin.stderr.filtered op-empty-stdin.stderr.expected \
	op-empty-notty.stdout op-empty-notty.stderr \
	op-empty-notty.stderr.filtered op-empty-notty.stderr.expected \
	op-empty-raw.stdout op-empty-raw.stderr \
	op-empty-raw.stderr.filtered op-empty-raw.stderr.expected \
	op-empty-raw.raw \
	rawspice.raw core core.*

MAINTAINERCLEANFILES = Makefile.in
