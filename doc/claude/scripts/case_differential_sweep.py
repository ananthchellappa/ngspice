#!/usr/bin/env python3
"""Differential sweep for the casemode feature.

`make check` cannot see most of what `casemode=preserve` touches: it runs
only the directories listed in tests/Makefile.am SUBDIRS, it compares stdout
after an egrep -v filter that removes most diagnostics, and it never runs any
deck twice under two different modes.  This script does what the harness
cannot.

Every `.cir` under tests/ is run three ways:

  (a) stock          -- the case mode unset
  (b) preserve       -- `-D casemode=preserve` on the original deck
  (c) preserve UPPER -- `-D casemode=preserve` on a mechanically uppercased
                        copy of the deck

and each run also writes a rawfile with `-r`.  Two things are then compared
against run (a):

  * stdout, with case normalised on both sides and machine-dependent lines
    removed, so that a name printed as `OutB` or `OUTB` instead of `outb`
    is not reported while a changed number is;
  * the rawfile payload, byte for byte after the `Binary:`/`Values:` marker,
    which is the numeric result stripped of every name.

Usage:

    python3 doc/claude/scripts/case_differential_sweep.py \\
        [--ngspice build-ver_50/src/ngspice] [--spinit build-ver_50/src] \\
        [--timeout 120] [--jobs 8] [--filter SUBSTRING]

Exit status is 0 when every deck agrees on the numbers in all three runs.

The uppercasing is deliberately not a blind str.upper(): text inside double
quotes is left alone, and whole lines are left alone when their first token
names a file rather than an identifier (`.lib`, `.inc`, `.include`, `source`,
`shell`, `cd`, `load`, `codemodel`, `osdi`, `pre_osdi`).  Uppercasing a path
would test the filesystem, not ngspice.  Numeric scale suffixes are safe to
uppercase by language rule: `1M` is milli in either case.
"""

import argparse
import concurrent.futures
import os
import re
import shutil
import subprocess
import sys
import tempfile

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__)))))

# Lines whose first token names a file: uppercasing them would test the
# filesystem rather than ngspice.
FILE_CARDS = (".lib", ".inc", ".include", "source", "shell", "cd", "load",
              "codemodel", "osdi", "pre_osdi", "*ng_script")

# Machine- or run-dependent stdout that is not part of the answer.  The
# rawfile path differs between the three runs by construction, and the
# resource block is machine dependent, so both go.
NOISE = re.compile(
    r"(cpu time|elapsed time|total elapsed|memory|ngspice program size|"
    r"shared ngspice pages|stack =|dram|date|circuit:|"
    r"\b(mon|tue|wed|thu|fri|sat|sun)\b|"
    r"analysis time|raw file \"|shell cwd was reset|reference value|"
    r"pages =| mb\.|warning: can't find|"
    r"warning: can't find the initialization file)")

# A run that never simulated: the netlist did not survive parsing.
PARSE_FAIL = re.compile(
    r"(simulation interrupted due to error|incomplete or empty netlist|"
    r"could not find a valid modelname|unable to find definition of model|"
    r"unknown subckt|unknown parameter)")

# Absolute paths differ between the three scratch directories.
PATHS = re.compile(r"/tmp/casesweep-[a-z0-9_]+\S*")

# The one problem that makes a verdict NUM-DIFF rather than DIFF.  It is a
# constant so that the classification tests for the marker rather than for a
# word a deck's own output might contain.
RAWFILE_DIFFERS = "rawfile payload differs"


def upper_deck(text):
    """Mechanically uppercase a deck without touching file names."""
    out = []
    for line in text.splitlines(True):
        stripped = line.lstrip()
        first = stripped.split(None, 1)[0].lower() if stripped.split() else ""
        if first.startswith(FILE_CARDS):
            out.append(line)
            continue
        buf = []
        in_quotes = False
        for ch in line:
            if ch == '"':
                in_quotes = not in_quotes
                buf.append(ch)
            elif in_quotes:
                buf.append(ch)
            else:
                buf.append(ch.upper())
        out.append("".join(buf))
    return "".join(out)


def normalise(stdout):
    """Case-normalised, noise-free stdout for comparison."""
    keep = []
    for line in stdout.lower().splitlines():
        line = PATHS.sub("<path>", " ".join(line.split()))
        if not line or NOISE.search(line):
            continue
        keep.append(line)
    return keep


NVARS = re.compile(rb"No\. Variables:\s*(\d+)")
NPOINTS = re.compile(rb"No\. Points:\s*(\d+)")
FLAGS = re.compile(rb"Flags:\s*(\S+)")


def raw_payload(path):
    """The numeric content of a rawfile, with every name and title dropped.

    A rawfile may hold several plots back to back, so the bytes after the
    first `Binary:` marker still contain the *headers* of the later plots.
    Those headers carry variable names, which preserve mode is meant to
    change, so slicing at the first marker would report a name change as a
    numeric change.  Each plot is therefore measured from its own header:
    exactly nvars * npoints doubles, twice that when the plot is complex.
    """
    try:
        with open(path, "rb") as fh:
            blob = fh.read()
    except OSError:
        return None
    out = []
    pos = 0
    while True:
        bidx = blob.find(b"Binary:\n", pos)
        aidx = blob.find(b"Values:\n", pos)
        if bidx < 0 and aidx < 0:
            break
        binary = bidx >= 0 and (aidx < 0 or bidx < aidx)
        idx = bidx if binary else aidx
        head = blob[pos:idx]
        mv, mp, mf = NVARS.search(head), NPOINTS.search(head), FLAGS.search(head)
        if not (mv and mp):
            return None
        nvars, npoints = int(mv.group(1)), int(mp.group(1))
        complex_plot = bool(mf) and b"complex" in mf.group(1).lower()
        start = idx + len(b"Binary:\n" if binary else b"Values:\n")
        if binary:
            nbytes = nvars * npoints * 8 * (2 if complex_plot else 1)
            out.append(blob[start:start + nbytes])
            pos = start + nbytes
        else:
            # ASCII: take the numbers only, up to the next plot header
            nxt = blob.find(b"Title:", start)
            chunk = blob[start:] if nxt < 0 else blob[start:nxt]
            out.append(b" ".join(re.findall(rb"[-+0-9.eE]+", chunk)))
            pos = start + len(chunk)
    return out or None


def run(ngspice, spinit, deck, workdir, preserve, timeout):
    env = dict(os.environ)
    env["SPICE_SCRIPTS"] = spinit
    env["ngspice_vpath"] = os.path.dirname(deck) or "."
    os.makedirs(workdir, exist_ok=True)
    rawpath = os.path.join(workdir, "sweep.raw")
    cmd = [ngspice]
    if preserve:
        cmd += ["-D", "casemode=preserve"]
    cmd += ["-r", rawpath, "--batch", deck]
    try:
        proc = subprocess.run(cmd, cwd=workdir, env=env, timeout=timeout,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT)
        text = proc.stdout.decode("utf-8", "replace")
        rc = proc.returncode
    except subprocess.TimeoutExpired:
        return None, None, "timeout"
    return text, raw_payload(rawpath), "rc=%d" % rc


def masked_equal(ref, other, noise):
    """Compare two rawfile payloads, ignoring 8-byte words that already
    differ between two stock runs.

    An AC rawfile is not reproducible byte for byte on this tree: the
    imaginary component of the frequency scale vector is written from
    uninitialised memory, so two identical stock runs disagree there.  The
    sweep therefore runs stock twice and treats any word the two stock runs
    disagree on as noise rather than reporting it as a case-mode effect.
    """
    if ref is None or other is None:
        return ref == other
    if len(ref) != len(other):
        return False
    for i, (a, b) in enumerate(zip(ref, other)):
        if len(a) != len(b):
            return False
        bad = noise[i] if noise and i < len(noise) else set()
        for w in range(0, len(a), 8):
            if w // 8 in bad:
                continue
            if a[w:w + 8] != b[w:w + 8]:
                return False
    return True


def noise_mask(ref, again):
    """Word indices per plot on which two stock runs already disagree."""
    if ref is None or again is None or len(ref) != len(again):
        return None
    mask = []
    for a, b in zip(ref, again):
        bad = set()
        if len(a) == len(b):
            for w in range(0, len(a), 8):
                if a[w:w + 8] != b[w:w + 8]:
                    bad.add(w // 8)
        mask.append(bad)
    return mask


def sweep_one(ngspice, spinit, cir, timeout):
    """Return (relpath, verdict, detail)."""
    rel = os.path.relpath(cir, REPO)
    srcdir = os.path.dirname(cir)
    base = os.path.basename(cir)
    tmp = tempfile.mkdtemp(prefix="casesweep-")
    try:
        plain = os.path.join(tmp, "plain")
        upper = os.path.join(tmp, "upper")
        shutil.copytree(srcdir, plain)
        shutil.copytree(srcdir, upper)
        with open(os.path.join(upper, base), "r", errors="replace") as fh:
            text = fh.read()
        with open(os.path.join(upper, base), "w") as fh:
            fh.write(upper_deck(text))

        a_out, a_raw, a_rc = run(ngspice, spinit,
                                 os.path.join(plain, base),
                                 os.path.join(tmp, "a"), False, timeout)
        a2_out, a2_raw, _a2_rc = run(ngspice, spinit,
                                     os.path.join(plain, base),
                                     os.path.join(tmp, "a2"), False, timeout)
        b_out, b_raw, b_rc = run(ngspice, spinit,
                                 os.path.join(plain, base),
                                 os.path.join(tmp, "b"), True, timeout)
        c_out, c_raw, c_rc = run(ngspice, spinit,
                                 os.path.join(upper, base),
                                 os.path.join(tmp, "c"), True, timeout)

        if a_out is None:
            return rel, "SKIP", "stock run timed out"
        noise = noise_mask(a_raw, a2_raw)
        a_broke = bool(PARSE_FAIL.search(a_out.lower()))
        problems = []
        for tag, out in (("preserve", b_out), ("preserve-UPPER", c_out)):
            if out is not None and not a_broke and PARSE_FAIL.search(out.lower()):
                first = [l for l in out.splitlines()
                         if PARSE_FAIL.search(l.lower())]
                return rel, "PARSE-FAIL", "%s: %s" % (
                    tag, first[0].strip() if first else "?")
        for tag, out, raw, rc in (("preserve", b_out, b_raw, b_rc),
                                  ("preserve-UPPER", c_out, c_raw, c_rc)):
            if out is None:
                problems.append("%s: %s" % (tag, rc))
                continue
            if not masked_equal(a_raw, raw, noise):
                problems.append("%s: %s" % (tag, RAWFILE_DIFFERS))
            na, nb = normalise(a_out), normalise(out)
            if na != nb:
                problems.append("%s: stdout %s" % (tag, describe(na, nb)))
        if problems:
            # Match the marker, not the word.  A stdout problem quotes the
            # deck's own output, and a deck whose output contains the word
            # rawfile -- 'OP information in rawfile.', src/frontend/dotcards.c:228
            # -- was reported as a numeric difference when its numbers were
            # identical and only its stdout had moved.
            numeric = [p for p in problems if RAWFILE_DIFFERS in p]
            return rel, "NUM-DIFF" if numeric else "DIFF", "; ".join(problems)
        return rel, "OK", a_rc
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def describe(a, b):
    """Symmetric difference of two filtered stdouts, as a short string."""
    from collections import Counter
    ca, cb = Counter(a), Counter(b)
    only_a = list((ca - cb).elements())
    only_b = list((cb - ca).elements())
    if not only_a and not only_b:
        return "reordered only"
    parts = []
    for tag, lines in (("-", only_a), ("+", only_b)):
        for line in lines[:3]:
            parts.append("%s%r" % (tag, line[:100]))
        if len(lines) > 3:
            parts.append("%s(%d more)" % (tag, len(lines) - 3))
    return "; ".join(parts)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ngspice",
                    default=os.path.join(REPO, "build-ver_50/src/ngspice"))
    ap.add_argument("--spinit",
                    default=os.path.join(REPO, "build-ver_50/src"))
    ap.add_argument("--timeout", type=int, default=120)
    ap.add_argument("--jobs", type=int, default=8)
    ap.add_argument("--filter", default="")
    args = ap.parse_args()

    decks = []
    for root, _dirs, files in os.walk(os.path.join(REPO, "tests")):
        for name in sorted(files):
            if name.endswith(".cir") and args.filter in os.path.join(root, name):
                decks.append(os.path.join(root, name))
    decks.sort()

    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futs = [pool.submit(sweep_one, args.ngspice, args.spinit, d,
                            args.timeout) for d in decks]
        for fut in concurrent.futures.as_completed(futs):
            results.append(fut.result())

    results.sort()
    bad = 0
    for rel, verdict, detail in results:
        if verdict != "OK":
            print("%-8s %-60s %s" % (verdict, rel, detail))
        if verdict in ("DIFF", "NUM-DIFF", "PARSE-FAIL"):
            bad += 1
    counts = {}
    for _rel, verdict, _detail in results:
        counts[verdict] = counts.get(verdict, 0) + 1
    print("\n%d decks: %s" % (len(results),
                              ", ".join("%s=%d" % kv
                                        for kv in sorted(counts.items()))))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
