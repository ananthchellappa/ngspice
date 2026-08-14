#!/bin/sh
#
# identity_lint.sh -- driver for the build-enforced identifier-comparison lint.
#
# Invoked by automake from tests/lint/Makefile.am with one argument, the path
# to a spec file (tests/lint/*.lint).  The spec names the tree to scan, the
# comparators to look for, the escape-hatch marker and the baseline; the
# scanner is tests/bin/identity_lint.awk.
#
# The lint is a ratchet.  It reports every call to a string comparator whose
# operands are both runtime expressions -- a comparison against a string
# literal is asking about a keyword and is never reported -- and then compares
# that set against the baseline.  It fails on a set that differs in either
# direction: a new comparison has to be classified before it lands, and a
# comparison that has left the tree has to leave the baseline with it, or the
# baseline stops being a description of the tree.
#
# Environment, set by tests/lint/Makefile.am:
#   ngspice_srcdir  top of the source tree
#   AWK             the awk automake configured
#
# Exit 0 if the tree matches the baseline, 1 otherwise.  Nothing is compiled
# and no deck is run, so the result does not depend on casemode.

set -e

spec=$1

if [ -z "$spec" ] ; then
    echo "usage: identity_lint.sh <spec>" >&2
    exit 1
fi
if [ ! -f "$spec" ] ; then
    echo "identity lint: no such spec file: $spec" >&2
    exit 1
fi

: ${ngspice_srcdir:=`dirname $0`/../..}
: ${AWK:=awk}

specdir=`dirname "$spec"`

# The scanner is run from inside the tree being scanned, so that the paths it
# prints are the ones the baseline holds; its own path therefore has to be
# absolute before that cd.
scannerdir=`cd "$ngspice_srcdir/tests/bin" && pwd` || exit 1
scanner=$scannerdir/identity_lint.awk

# The spec is read with a here-shell so that a value may contain spaces; keys
# are the first word of a line, '#' starts a comment and blank lines are
# ignored.
root=`sed -n 's/^root[ 	][ 	]*//p' "$spec"`
baseline=`sed -n 's/^baseline[ 	][ 	]*//p' "$spec"`
marker=`sed -n 's/^marker[ 	][ 	]*//p' "$spec"`
comparators=`sed -n 's/^comparators[ 	][ 	]*//p' "$spec" | tr '\n' ' '`
skip=`sed -n 's/^skip[ 	][ 	]*//p' "$spec"`

if [ -z "$root" ] || [ -z "$baseline" ] || [ -z "$marker" ] || [ -z "$comparators" ] ; then
    echo "identity lint: $spec is missing root, baseline, marker or comparators" >&2
    exit 1
fi

case $baseline in
    /*) ;;
    *) baseline=$specdir/$baseline ;;
esac
if [ ! -f "$baseline" ] ; then
    echo "identity lint: no such baseline: $baseline" >&2
    exit 1
fi

case $root in
    /*) scanroot=$root ; scanbase=/ ;;
    *) scanroot=$root ; scanbase=$ngspice_srcdir ;;
esac

tmp=identity-lint.$$
trap 'rm -f $tmp.*' 0 1 2 13 15

# Byte semantics: the tree has source files with non-UTF-8 bytes in comments,
# and a multibyte locale makes gawk warn about them.
LC_ALL=C
export LC_ALL

(
    cd "$scanbase" || exit 1
    find "$scanroot" \( -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.cc' \) -print
) | sort > $tmp.files

for s in $skip ; do
    grep -v "^$s\$" $tmp.files > $tmp.files.new || true
    mv $tmp.files.new $tmp.files
done

if [ ! -s $tmp.files ] ; then
    echo "identity lint: no source files under $scanroot" >&2
    exit 1
fi

( cd "$scanbase" && xargs "$AWK" \
    -v comparators="$comparators" \
    -v marker="$marker" \
    -f "$scanner" ) < $tmp.files > $tmp.raw

# The baseline is keyed on the path and the call text only.  A line number
# would make every edit above a site look like a new comparison, which is the
# failure mode that teaches a contributor to regenerate the baseline instead
# of reading it.
cut -f1,3 $tmp.raw | sort > $tmp.found
grep -v '^#' "$baseline" | grep -v '^[ 	]*$' | sort > $tmp.want

comm -13 $tmp.want $tmp.found > $tmp.new
comm -23 $tmp.want $tmp.found > $tmp.gone

if [ ! -s $tmp.new ] && [ ! -s $tmp.gone ] ; then
    echo "identity lint: `wc -l < $tmp.found` comparisons, baseline matches"
    exit 0
fi

echo "identity lint: FAILED"
echo

if [ -s $tmp.new ] ; then
    echo "Unclassified comparisons of two runtime strings, not in $baseline:"
    echo
    # re-attach the line numbers, which the key deliberately drops
    while IFS='	' read -r f t ; do
        awk -F'	' -v f="$f" -v t="$t" \
            '$1 == f && $3 == t { printf "  %s:%s  %s\n", $1, $2, $3 }' $tmp.raw
    done < $tmp.new | sort -u
    echo
    cat <<EOF
Each one is asking one of two questions, and no regular expression can tell
them apart, so you have to say which:

  identity   -- at least one operand is a name a deck wrote (a node, a vector,
                an instance, a .model, a .subckt, a .param, an event node).
                Call ng_ideq(), vec_name_eq() or Evt_Node_Name_Eq() instead,
                or the mode predicate inp_case_exact_ids(); see
                doc/claude/decisions/0001-distinguish.md decision 3.

  keyword    -- both operands are words the language defines (a command, an
                option, a device letter, an analysis or model type, a
                parameter keyword).  These stay byte-exact in all three case
                modes.  Say so on the line, with the reason:

                    if (eq(a, b))   /* $marker keyword - both are command names */

The marker suppresses the call on its own line and the call on the line
directly below it, and nothing else.  Nothing checks the reason word; it is
written for the reviewer, so it has to say which question the site is asking
and the answer has to be readable beside the call.  The reason words the
tree uses, beyond 'keyword':

  helper     -- the call IS one of the identity helpers, or the mode arm of
                one.  src/frontend/define.c.
  stored     -- both operands are names the simulator itself holds for one
                run, so the question is which column this is and not whether
                two spellings are one name.  These stay byte-exact for a
                measured reason, which the marker has to name.
                src/frontend/outitf.c.
  neither    -- a comparator's own definition, a sort, a deliberate
                case-insensitive scan, free text.
                tests/lint/selftest/silent.c.

Adding the line to the baseline instead is for a comparison that is a
genuine identity test and cannot be fixed yet; such an entry must name the
issue that will fix it.  The baseline is
    $baseline
EOF
fi

if [ -s $tmp.gone ] ; then
    if [ -s $tmp.new ] ; then
        echo
    fi
    echo "In $baseline but no longer in the tree:"
    echo
    sed 's/^/  /' $tmp.gone
    echo
    echo "Delete these lines.  The baseline is a description of the tree, and"
    echo "an entry that outlives its comparison is how a stale exemption hides"
    echo "the next one."
fi

exit 1
