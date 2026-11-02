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
## Two more from the ver_51 follow-up (P2 fixes):
##
##   option-seed-spelling.cir        the warning quotes '.option seed=05' as
##                                   written, not as the parsed 5
##   option-seed-multiple.cir        two '.option seed=' cards: the existing
##                                   'Multiple' warning, once over four parses
##
## One more from the ver_51 follow-up batch 2 (item 3):
##
##   option-seed-unusable.cir        '.option seed=abc': the 'Cannot convert'
##                                   warning, once over four parses, and the
##                                   skipped option leaves 'setseed' alone
##
## ver_52 group 8, from upstream 95d7f2c2b's idea and not its code:
##
##   option-seed-empty.cir           '.option seed=' with no value: the same
##                                   warning, once, where eval_opt() crashed
##   option-seed-control.cir         'option seed=5' in .control draws what
##                                   'setseed 5' draws, wherever seed= sits in
##                                   the command, and is logged src=option
##   option-seed-control-random.cir  'option seed=random': the same, from the
##                                   clock, once and not on every parse
##   option-seed-control-unusable.cir  a value that is not a seed warns each
##                                   time and changes nothing
##   option-seed-control-netlist.cir '.option seed=' still wins at the next
##                                   parse, and says so once (group 11)
##   option-seed-control-netlist-random.cir  the same, seed=random wording
##   option-seed-control-netlist-unusable.cir  a value that seeded nothing is
##                                   not overridden, so no such warning
##
## The next two are ver_51 batch item 3 (P3).  initw() used to re-seed the
## generator from the process id after startup had seeded it from
## rndseed=1; now startup re-seeds from $rndseed after initw():
##
##   startup-equals-setseed1.cir     a deck's first draw equals the draw
##                                   after 'setseed 1', in one process
##   setseed7-bit-identity.cir       'setseed 7' still draws the 45.2 value
##                                   (glibc rand(); skipped on another libc)
##
## The rnddraws-* decks are ver_51 batch item 4 (P4), the draw log.
## 'set rnddraws=<file>' writes a tab-separated record set per parse, one row
## per random draw; the .files beside each deck assert its structure with
## grep, because the values are glibc rand() values:
##
##   rnddraws-mm.cir      two subckt instances over setseed 1-3, a reset
##                        without setseed (offset != 0), an unset (nothing
##                        logged), and a second set (a fresh file)
##   rnddraws-order.cir   a third instance in a nested subckt ahead of the
##                        two: the rows are keyed by instance path
##   rnddraws-funcs.cir   one draw from every draw path, a repeated key
##                        (r#2), a global .param ('?'); and the guard: the
##                        draws do not change when the log is on (any libc)
##   rnddraws-gold.cir    the same circuit: with the log off, the draws equal
##                        the values from before the log existed (glibc
##                        rand(); skipped on another libc)
##
## Two more from the ver_51 follow-up (P4 fixes).  rnddraws-mm.cir asserts
## only the src=setseed header; these assert the other three seed sources:
##
##   rnddraws-src.cir     src=startup (the initial load, through pre_set) and
##                        src=checkseed offset=1 ('set rndseed=11' and one
##                        control-language draw before the reset)
##   rnddraws-option.cir  src=option, on the initial load and on a reset
##
## A deck with a .requires sibling ('libc glibc') is skipped, exit 77, when
## the platform does not provide it; check_status.sh's header has the format.
##
## An empty .err is an assertion that stderr is empty, not a missing file.

TESTS = option-seed-setseed.cir option-seed-random-setseed.cir \
	option-seed-nosetseed.cir option-seed-absent.cir \
	option-seed-spelling.cir option-seed-multiple.cir \
	option-seed-unusable.cir option-seed-empty.cir \
	option-seed-control.cir option-seed-control-random.cir \
	option-seed-control-unusable.cir option-seed-control-netlist.cir \
	option-seed-control-netlist-random.cir option-seed-control-netlist-unusable.cir \
	startup-equals-setseed1.cir setseed7-bit-identity.cir \
	rnddraws-mm.cir rnddraws-order.cir rnddraws-funcs.cir \
	rnddraws-gold.cir rnddraws-src.cir rnddraws-option.cir

TESTS_ENVIRONMENT = ngspice_vpath=$(srcdir) \
	$(SHELL) $(top_srcdir)/tests/bin/check_status.sh $(top_builddir)/src/ngspice

EXTRA_DIST = \
	$(TESTS) \
	option-seed-setseed.status option-seed-setseed.err \
	option-seed-random-setseed.status option-seed-random-setseed.err \
	option-seed-nosetseed.status option-seed-nosetseed.err \
	option-seed-absent.status option-seed-absent.err \
	option-seed-spelling.status option-seed-spelling.err \
	option-seed-multiple.status option-seed-multiple.err \
	option-seed-unusable.status option-seed-unusable.err \
	option-seed-empty.status option-seed-empty.err \
	option-seed-control.status option-seed-control.err option-seed-control.files \
	option-seed-control-random.status option-seed-control-random.err option-seed-control-random.files \
	option-seed-control-unusable.status option-seed-control-unusable.err \
	option-seed-control-netlist.status option-seed-control-netlist.err \
	option-seed-control-netlist-random.status option-seed-control-netlist-random.err \
	option-seed-control-netlist-unusable.status option-seed-control-netlist-unusable.err \
	startup-equals-setseed1.status startup-equals-setseed1.err \
	setseed7-bit-identity.status setseed7-bit-identity.err \
	setseed7-bit-identity.requires \
	rnddraws-mm.status rnddraws-mm.err rnddraws-mm.files \
	rnddraws-order.status rnddraws-order.err rnddraws-order.files \
	rnddraws-funcs.status rnddraws-funcs.err rnddraws-funcs.files \
	rnddraws-gold.status rnddraws-gold.err rnddraws-gold.files \
	rnddraws-gold.requires \
	rnddraws-src.status rnddraws-src.err rnddraws-src.files \
	rnddraws-option.status rnddraws-option.err rnddraws-option.files

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
	option-seed-spelling.stdout option-seed-spelling.stderr \
	option-seed-spelling.stderr.filtered option-seed-spelling.stderr.expected \
	option-seed-multiple.stdout option-seed-multiple.stderr \
	option-seed-multiple.stderr.filtered option-seed-multiple.stderr.expected \
	option-seed-unusable.stdout option-seed-unusable.stderr \
	option-seed-unusable.stderr.filtered option-seed-unusable.stderr.expected \
	option-seed-empty.stdout option-seed-empty.stderr \
	option-seed-empty.stderr.filtered option-seed-empty.stderr.expected \
	option-seed-control.stdout option-seed-control.stderr \
	option-seed-control.stderr.filtered option-seed-control.stderr.expected \
	option-seed-control-random.stdout option-seed-control-random.stderr \
	option-seed-control-random.stderr.filtered option-seed-control-random.stderr.expected \
	option-seed-control-unusable.stdout option-seed-control-unusable.stderr \
	option-seed-control-unusable.stderr.filtered option-seed-control-unusable.stderr.expected \
	option-seed-control-netlist.stdout option-seed-control-netlist.stderr \
	option-seed-control-netlist.stderr.filtered option-seed-control-netlist.stderr.expected \
	option-seed-control-netlist-random.stdout option-seed-control-netlist-random.stderr \
	option-seed-control-netlist-random.stderr.filtered option-seed-control-netlist-random.stderr.expected \
	option-seed-control-netlist-unusable.stdout option-seed-control-netlist-unusable.stderr \
	option-seed-control-netlist-unusable.stderr.filtered option-seed-control-netlist-unusable.stderr.expected \
	startup-equals-setseed1.stdout startup-equals-setseed1.stderr \
	startup-equals-setseed1.stderr.filtered startup-equals-setseed1.stderr.expected \
	setseed7-bit-identity.stdout setseed7-bit-identity.stderr \
	setseed7-bit-identity.stderr.filtered setseed7-bit-identity.stderr.expected \
	rnddraws-mm.stdout rnddraws-mm.stderr \
	rnddraws-mm.stderr.filtered rnddraws-mm.stderr.expected \
	rnddraws-order.stdout rnddraws-order.stderr \
	rnddraws-order.stderr.filtered rnddraws-order.stderr.expected \
	rnddraws-funcs.stdout rnddraws-funcs.stderr \
	rnddraws-funcs.stderr.filtered rnddraws-funcs.stderr.expected \
	rnddraws-gold.stdout rnddraws-gold.stderr \
	rnddraws-gold.stderr.filtered rnddraws-gold.stderr.expected \
	rnddraws-src.stdout rnddraws-src.stderr \
	rnddraws-src.stderr.filtered rnddraws-src.stderr.expected \
	rnddraws-option.stdout rnddraws-option.stderr \
	rnddraws-option.stderr.filtered rnddraws-option.stderr.expected \
	rnddraws-mm.tsv rnddraws-mm-2.tsv rnddraws-order.tsv rnddraws-funcs.tsv \
	rnddraws-gold.tsv rnddraws-src.tsv rnddraws-option.tsv \
	option-seed-control.tsv option-seed-control-random.tsv \
	rawspice.raw

MAINTAINERCLEANFILES = Makefile.in
