#!/bin/sh
#
# check_status.sh -- driver for tests/regression/exitstatus/.
#
# Invoked by automake the way tests/bin/check.sh is: the simulator first --
# possibly a multi-word string, so it is used unquoted and word-splits the way
# tests/regression/casedist/Makefile.am relies on -- and the deck appended by
# automake as the last argument.
#
# What it adds to check.sh, and the whole reason it exists: check.sh runs
# ngspice --batch and throws the exit status away.  '$?' appears nowhere in
# that file and stderr is never redirected, so no directory driven by it can
# say what a failing run returned or what it said while failing.  This driver
# asserts on both, plus on the files the run did or did not leave behind.
#
# Per deck <base>.cir, in the deck's own directory:
#
#   <base>.status    required.  One integer: the exit status the run must
#                    return.  A value >= 128 is refused -- see the signal
#                    check below.
#   <base>.err       optional.  The filtered stderr of the run, compared with
#                    diff -B -w.  Both sides are put through the same filter,
#                    so a reference file may be pasted straight from a run.
#   <base>.files     optional.  Assertions about the files the run left in the
#                    build directory, one per line:
#                        absent  <path>
#                        present <path>
#                        grep    <path> <text>      (<text> is a BRE)
#                    '#' starts a comment and blank lines are ignored.  Every
#                    <path> named here is deleted BEFORE the run, so a file
#                    left by an earlier run can never satisfy a 'present' or
#                    a 'grep' line.  Paths are relative to the build directory
#                    and may not be absolute or contain '..', because the
#                    driver deletes them.
#   <base>.mustfail  optional, and this directory's self-test hook.  Names the
#                    checks -- from 'status', 'signal', 'err', 'files' -- that
#                    this deck exists to make fail.  With it present the
#                    verdict is inverted: the driver exits 0 only if exactly
#                    those checks failed, and 1 if everything passed.  That is
#                    how a permanently committed test proves the driver can
#                    still fail, under the same TESTS_ENVIRONMENT as every
#                    other entry.
#
# The run's stdout and stderr are captured to <base>.stdout and <base>.stderr
# in the current directory, which is the build directory.  They are removed on
# success and left in place on failure.
#
# Exit 0 if every check passed (or, under .mustfail, if exactly the named
# checks failed), 1 otherwise.

if [ -z "$SPICE_SCRIPTS" ] ; then
    SPICE_SCRIPTS=`dirname $0`
    export SPICE_SCRIPTS
    if [ -z "$ngspice_vpath" ] ; then
        ngspice_vpath=.
        export ngspice_vpath
    fi
fi

if [ $# -lt 2 ] ; then
    echo "usage: check_status.sh <simulator> [simulator-args...] <deck.cir>" >&2
    exit 1
fi

SPICE=$1
shift

# automake appends the deck as the last argument, and the simulator may have
# carried flags of its own in $1, so take the last one rather than the second.
for arg ; do
    TEST=$arg
done

testname=`basename $TEST .cir`
testdir=`dirname $TEST`

status_spec=$testdir/$testname.status
err_spec=$testdir/$testname.err
files_spec=$testdir/$testname.files
mustfail_spec=$testdir/$testname.mustfail

out=$testname.stdout
err=$testname.stderr
errfilt=$testname.stderr.filtered
errwant=$testname.stderr.expected

# The checks that failed, as a space-separated list of their names.  .mustfail
# is compared against exactly this.
failed=

fail () {
    failed="$failed $1"
}

# A malformed assertion file is not a test failure, it is a broken test, and
# it must never be inverted into a pass by .mustfail.  So it leaves here.
spec_error () {
    echo "check_status: $testname: broken test spec: $1" >&2
    exit 1
}

# The filter, applied to both sides of the <base>.err comparison.  Only these
# things are touched, and each of them differs between two runs of the same
# deck on the same machine or between two builds of the same tree:
#
#   1. the directory prefix on a C source file name.  An assertion message
#      carries __FILE__ as the compiler saw it, so this build says
#      '../../../src/frontend/dotcards.c:225' where an in-tree build says
#      'src/frontend/dotcards.c:225'.  The '../' run counts build-directory
#      levels and is not part of what the simulator said.
#   2. the directory prefix on the deck's own name.  automake hands the deck
#      to this script as $(srcdir)/<base>.cir, and $(srcdir) is '.' in a
#      source-tree build and a relative or absolute path in a VPATH build.
#   3. the shell's own report of a child that died on a signal.  Measured:
#      with $(SHELL) = dash, 'Aborted (core dumped)' is written into the
#      redirected stderr of the command and so lands in the capture; with
#      $(SHELL) = bash the same message goes to the shell's stderr and does
#      not.  It is the shell speaking, not the simulator, and which shell
#      configure picked is not a property of ngspice.  Nothing is lost: the
#      death by signal is reported from $?, by the signal check below.
#   4. the startup banner and the resource-usage block, which carry the
#      version string, the build's Creation Date, wall-clock durations and
#      free-memory figures.  No deck here puts them on stderr today; the
#      entries are here so that a deck which redirects them cannot freeze a
#      build stamp or a machine's spare RAM into a reference file.
#
# Deliberately NOT filtered: anything matching 'Error' or 'Warning'.
# check.sh strips both as unanchored substrings, which is right when the
# subject is a table of numbers and would delete the entire subject here.
FILTER='^(Aborted|Quit|Killed|Terminated|Hangup|Bus error|Segmentation fault|Floating point exception|Illegal instruction|Trace/breakpoint trap)( \(core dumped\))?$|^\*\*|^CPU time|^Total (analysis|elapsed) time|^(Total DRAM|DRAM currently|Maximum ngspice|Current ngspice|Shared ngspice|Text \(code\)|Stack =|Library pages)'

filter () {
    sed -e 's,[^ 	]*/\(src/[^ 	:]*\),\1,g' \
        -e "s,[^ 	]*/\($testname\.cir\),\1,g" | \
    grep -v -E "$FILTER"
}

# 128+signo is all a shell can see of a death by signal; it cannot tell that
# apart from a deliberate exit(134).  Naming the signal is the point -- issue
# 0072's first criterion is that the run terminates normally rather than on a
# signal, and a bare 'expected 1, got 134' does not say that.
signal_name () {
    case $1 in
        1)  echo SIGHUP ;;
        2)  echo SIGINT ;;
        3)  echo SIGQUIT ;;
        4)  echo SIGILL ;;
        6)  echo SIGABRT ;;
        7)  echo SIGBUS ;;
        8)  echo SIGFPE ;;
        9)  echo SIGKILL ;;
        11) echo SIGSEGV ;;
        13) echo SIGPIPE ;;
        14) echo SIGALRM ;;
        15) echo SIGTERM ;;
        24) echo SIGXCPU ;;
        25) echo SIGXFSZ ;;
        *)  echo "signal $1" ;;
    esac
}

# Turn free-form whitespace into a sorted, space-separated set, so that
# .mustfail may be written one word per line or all on one.
words () {
    tr -s ' 	\n' '\n\n\n' | grep -v '^$' | sort | tr '\n' ' '
}

# ---------------------------------------------------------------- the spec

if [ ! -f "$testdir/$testname.cir" ] ; then
    spec_error "no such deck: $testdir/$testname.cir"
fi

if [ ! -f "$status_spec" ] ; then
    spec_error "no $status_spec.  Every deck in this directory needs a
    .status sibling holding the exit status the run must return."
fi

expected=`sed -e 's/#.*//' -e 's/[ 	]//g' "$status_spec" | grep -v '^$' | head -1`

case $expected in
    '')          spec_error "$status_spec holds no integer" ;;
    *[!0-9]*)    spec_error "$status_spec holds '$expected', not an integer" ;;
esac

if [ "$expected" -ge 128 ] ; then
    spec_error "$status_spec asks for $expected.  A status >= 128 is how a
    shell reports a death by signal, which this driver always calls a failure;
    it cannot be an expected result."
fi

if [ -f "$mustfail_spec" ] ; then
    mustfail=`sed 's/#.*//' "$mustfail_spec" | words`
    if [ -z "$mustfail" ] ; then
        spec_error "$mustfail_spec names no check"
    fi
    for name in $mustfail ; do
        case $name in
            status|signal|err|files) ;;
            *) spec_error "$mustfail_spec names '$name'; the checks are status, signal, err, files" ;;
        esac
    done
fi

# ------------------------------------------------- clear the ground, then run

rm -f "$out" "$err" "$errfilt" "$errwant"

if [ -f "$files_spec" ] ; then
    while read -r verb path rest ; do
        case $verb in
            ''|\#*) continue ;;
            absent|present|grep) ;;
            *) spec_error "$files_spec: unknown verb '$verb'; the verbs are absent, present, grep" ;;
        esac
        if [ -z "$path" ] ; then
            spec_error "$files_spec: '$verb' with no path"
        fi
        case $path in
            /*|../*|*/../*|..|*/..)
                spec_error "$files_spec: '$path' is outside the build directory, and this driver deletes the paths it asserts on" ;;
        esac
        if [ "$verb" = grep ] && [ -z "$rest" ] ; then
            spec_error "$files_spec: 'grep $path' with no text"
        fi
        rm -f "$path"
    done < "$files_spec"
fi

if [ -f "$mustfail_spec" ] ; then
    echo "check_status: $testname: self-test.  The failure reported below is"
    echo "  the one this deck exists to produce; the verdict is inverted."
fi

$SPICE --batch -n "$testdir/$testname.cir" > "$out" 2> "$err"
rc=$?

# ------------------------------------------------------------- the checks

if [ $rc -ge 128 ] ; then
    signo=`expr $rc - 128`
    echo "check_status: $testname: FAILED (signal)"
    echo "  the simulator did not terminate normally: rc=$rc is 128+$signo, `signal_name $signo`."
    echo "  A run that cannot proceed has to say so and return a status."
    fail signal
fi

if [ $rc -ne "$expected" ] ; then
    echo "check_status: $testname: FAILED (status)"
    echo "  expected exit status $expected, got $rc"
    if [ -s "$err" ] ; then
        echo "  stderr was:"
        sed 's/^/    /' "$err"
    else
        echo "  stderr was empty"
    fi
    fail status
fi

if [ -f "$err_spec" ] ; then
    filter < "$err" > "$errfilt"
    filter < "$err_spec" > "$errwant"
    if diff -B -w -u "$errwant" "$errfilt" ; then
        :
    else
        echo "check_status: $testname: FAILED (err)"
        echo "  filtered stderr does not match $err_spec (- expected, + actual)"
        fail err
    fi
fi

if [ -f "$files_spec" ] ; then
    files_ok=yes
    while read -r verb path rest ; do
        case $verb in
            ''|\#*) continue ;;
        esac
        case $verb in
            absent)
                if [ -e "$path" ] ; then
                    echo "  absent $path: the run created it"
                    files_ok=no
                fi ;;
            present)
                if [ ! -f "$path" ] ; then
                    echo "  present $path: the run did not create it"
                    files_ok=no
                fi ;;
            grep)
                if [ ! -f "$path" ] ; then
                    echo "  grep $path: no such file"
                    files_ok=no
                elif grep -q -e "$rest" "$path" ; then
                    :
                else
                    echo "  grep $path '$rest': not found; the file holds:"
                    # a rawfile is the expected subject and its body is
                    # binary, so bound the dump and make it printable
                    head -c 2000 "$path" | head -20 | \
                        LC_ALL=C tr -c '\11\12\40-\176' '?' | sed 's/^/      /'
                    files_ok=no
                fi ;;
        esac
    done < "$files_spec"
    if [ "$files_ok" = no ] ; then
        echo "check_status: $testname: FAILED (files)"
        fail files
    fi
fi

rm -f "$errfilt" "$errwant"

# ------------------------------------------------------------- the verdict

if [ -f "$mustfail_spec" ] ; then
    got=`echo "$failed" | words`
    if [ "$got" = "$mustfail" ] ; then
        echo "check_status: $testname: self-test passed; the driver reported: $got"
        rm -f "$out" "$err"
        exit 0
    fi
    echo "check_status: $testname: SELF-TEST FAILED"
    echo "  $mustfail_spec requires these checks to fail: $mustfail"
    echo "  the checks that actually failed:               ${got:-<none>}"
    echo "  The captured output is in $out and $err."
    exit 1
fi

if [ -n "$failed" ] ; then
    echo "check_status: $testname: FAILED:$failed"
    echo "  The captured output is in $out and $err."
    exit 1
fi

echo "check_status: $testname: exit status $rc as expected"
rm -f "$out" "$err"
exit 0
