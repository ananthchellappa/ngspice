#!/usr/bin/env python3
"""Run every .cir under tests/ and report which ones trip the undefined-node
diagnostic of doc/codex/issues/0028.

Neither of the two measuring tools this tree already has can see that
diagnostic.  tests/bin/check.sh captures stdout only and filters every line
containing 'Warning', and case_differential_sweep.py compares three runs that a
mode-independent diagnostic fires identically in.  So the decks are run
directly and stderr is grepped for the text.

Each deck is run the way its own directory's TESTS_ENVIRONMENT runs it:
cwd is the build-tree counterpart of the deck's directory (so that a dir with
its own spinit -- the xspice dirs -- loads the codemodels it was built with),
SPICE_SCRIPTS is '.' for those dirs and the src spinit otherwise, and
ngspice_vpath is the deck's source directory.  Without that, every xspice deck
loads the stale /usr/local codemodels and segfaults (doc/codex/issues/0021),
and a census over crashed runs measures nothing.

rc and whether the run got as far as printing its 'Circuit:' line are recorded
for every deck, so that a zero-hit result can be told from a zero-run one.
"""

import argparse, concurrent.futures, os, subprocess, sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__)))))
BUILD = os.path.join(REPO, "build-ver_50")
PHRASE = "it is referenced but no card defines it"
NEARMISS = "differs only in case (casemode=distinguish)"


def run_one(ngspice, deck, timeout, mode):
    srcdir = os.path.dirname(deck)
    reldir = os.path.relpath(srcdir, REPO)
    builddir = os.path.join(BUILD, reldir)
    if not os.path.isdir(builddir):
        builddir = srcdir
    env = dict(os.environ)
    # the xspice dirs ship their own spinit beside the Makefile
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
        err = p.stderr.decode("utf-8", "replace")
        out = p.stdout.decode("utf-8", "replace")
        rc = p.returncode
    except subprocess.TimeoutExpired:
        return deck, "TIMEOUT", False, [], []
    hits = [l.strip() for l in err.splitlines() if PHRASE in l]
    near = [l.strip() for l in err.splitlines() if NEARMISS in l]
    return deck, "rc=%d" % rc, ("Circuit:" in out), hits, near


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ngspice", default=os.path.join(BUILD, "src/ngspice"))
    ap.add_argument("--timeout", type=int, default=120)
    ap.add_argument("--jobs", type=int, default=8)
    ap.add_argument("--mode", default="")
    a = ap.parse_args()

    decks = []
    for root, _dirs, files in os.walk(os.path.join(REPO, "tests")):
        for f in files:
            if f.endswith(".cir"):
                decks.append(os.path.join(root, f))
    decks.sort()
    print("decks: %d  mode: %s  binary: %s" % (len(decks), a.mode or "stock", a.ngspice))

    tripped = notparsed = crashed = timedout = 0
    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=a.jobs) as ex:
        futs = [ex.submit(run_one, a.ngspice, d, a.timeout, a.mode) for d in decks]
        for fu in concurrent.futures.as_completed(futs):
            results.append(fu.result())
    for deck, status, parsed, hits, near in sorted(results):
        rel = os.path.relpath(deck, REPO)
        if hits:
            tripped += 1
            for h in sorted(set(hits)):
                print("HIT    %s [%s] %s" % (rel, status, h))
        for h in sorted(set(near)):
            print("NEAR   %s [%s] %s" % (rel, status, h))
        if status == "TIMEOUT":
            timedout += 1
            print("TMO    %s" % rel)
        elif not status.startswith("rc=0"):
            crashed += 1
            print("RC     %s [%s] parsed=%s" % (rel, status, parsed))
        if status != "TIMEOUT" and not parsed:
            notparsed += 1
            print("NOPARSE %s [%s]" % (rel, status))
    print("tripped=%d  nonzero-rc=%d  timeout=%d  no-Circuit-line=%d  of %d"
          % (tripped, crashed, timedout, notparsed, len(decks)))


main()
