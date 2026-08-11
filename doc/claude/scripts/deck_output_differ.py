#!/usr/bin/env python3
"""Run every .cir under tests/ with two ngspice binaries and report every deck
whose stdout or stderr differs.

Neither of the tree's measuring tools can see a change to a diagnostic:
tests/bin/check.sh captures stdout only and filters every line containing
'Warning', and case_differential_sweep.py compares three case modes of ONE
binary against each other rather than two binaries against each other.  This
compares the two binaries directly, on both streams, in a named case mode.

Each deck is run the way its own directory's TESTS_ENVIRONMENT runs it, copied
from doc/claude/scripts/undefined_node_census.py: cwd is the build-tree
counterpart of the deck's directory, and SPICE_SCRIPTS is '.' for the xspice
dirs, which ship their own spinit -- without that every xspice deck loads the
installed /usr/local codemodels and segfaults (doc/codex/issues/0021).

Volatile text -- timings, memory figures, temp file names -- is masked before
comparing, with the same spirit as check.sh's egrep filter.  The mask has to be
earned rather than guessed: the first run of this script reported five moved
decks in the default mode, and every one of them differed between two runs of
ONE binary.  tests/general/mosamp.cir moves its 'Reference value' line and
tests/mesa/mesa12.cir its analysis banner, which carries a wall-clock stamp;
the four tests/regression/case/alter-rebin*.cir decks are the flap
doc/claude/decisions/0008-undefined-node-diagnostic.md already records.  Before
believing a MOVED line, run that deck twice against the SAME binary.

Added by doc/claude/decisions/0009-let-definition-report.md, whose subject was
a diagnostic being removed from one call site: make check cannot see a Warning
line and the sweep compares modes rather than binaries, so without this there
was no way to say that nothing else moved.
"""

import argparse, concurrent.futures, os, re, subprocess

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__)))))
BUILD = os.path.join(REPO, "build-ver_50")

VOLATILE = [
    re.compile(r"\b\d+\.\d+ *(seconds|s\b)"),
    re.compile(r"CPU time[^\n]*"),
    re.compile(r"Total (elapsed|analysis|DRAM|CPU)[^\n]*"),
    re.compile(r"\b(Date|Total memory|Current dynamic|Maximum ngspice)[^\n]*"),
    re.compile(r"\d+ kB|\d+ MB"),
    re.compile(r"/tmp/[A-Za-z0-9_.]+"),
    re.compile(r"\bsp[0-9a-fA-F]{4,}\b"),
    # the analysis banner carries a wall-clock stamp, and mosamp.cir's
    # 'Reference value' moves run to run on one unchanged binary
    re.compile(r"(Mon|Tue|Wed|Thu|Fri|Sat|Sun) +\w+ +\d+ +[\d:]+ +\d{4}"),
    re.compile(r"Reference value *:.*"),
]


def mask(s):
    for r in VOLATILE:
        s = r.sub("X", s)
    return s


def run_one(ngspice, deck, timeout, mode):
    srcdir = os.path.dirname(deck)
    builddir = os.path.join(BUILD, os.path.relpath(srcdir, REPO))
    if not os.path.isdir(builddir):
        builddir = srcdir
    env = dict(os.environ)
    if os.path.exists(os.path.join(builddir, "spinit")):
        env["SPICE_SCRIPTS"] = "."
    else:
        env["SPICE_SCRIPTS"] = os.path.join(BUILD, "src")
    env["ngspice_vpath"] = srcdir
    cmd = [ngspice]
    if mode:
        cmd += ["-D", "casemode=" + mode]
    cmd += ["--batch", deck]
    try:
        p = subprocess.run(cmd, cwd=builddir, env=env, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return (p.returncode,
                mask(p.stdout.decode("utf-8", "replace")),
                mask(p.stderr.decode("utf-8", "replace")))
    except subprocess.TimeoutExpired:
        return ("TIMEOUT", "", "")


def compare(a, b, deck, timeout, mode):
    ra = run_one(a, deck, timeout, mode)
    rb = run_one(b, deck, timeout, mode)
    if ra == rb:
        return deck, None
    what = []
    if ra[0] != rb[0]:
        what.append("rc %s -> %s" % (ra[0], rb[0]))
    if ra[1] != rb[1]:
        what.append("stdout")
    if ra[2] != rb[2]:
        sa = set(ra[2].splitlines())
        sb = set(rb[2].splitlines())
        gone = sorted(sa - sb)[:3]
        new = sorted(sb - sa)[:3]
        what.append("stderr -%r +%r" % (gone, new))
    return deck, "; ".join(what)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--before", required=True)
    ap.add_argument("--after", required=True)
    ap.add_argument("--timeout", type=int, default=120)
    ap.add_argument("--jobs", type=int, default=8)
    ap.add_argument("--mode", default="")
    a = ap.parse_args()

    decks = []
    for root, _d, files in os.walk(os.path.join(REPO, "tests")):
        for f in files:
            if f.endswith(".cir"):
                decks.append(os.path.join(root, f))
    decks.sort()
    print("decks: %d  mode: %s" % (len(decks), a.mode or "stock"))

    moved = 0
    with concurrent.futures.ThreadPoolExecutor(max_workers=a.jobs) as ex:
        futs = [ex.submit(compare, a.before, a.after, d, a.timeout, a.mode)
                for d in decks]
        for fu in concurrent.futures.as_completed(futs):
            deck, diff = fu.result()
            if diff:
                moved += 1
                print("MOVED  %s  %s" % (os.path.relpath(deck, REPO), diff))
    print("moved=%d of %d  mode=%s" % (moved, len(decks), a.mode or "stock"))


main()
