#!/bin/bash
# compare.sh <dir A> <dir B> [rel tol, default 1e-9] [abs floor, default 1e-30]
# Diff two run.sh output dirs and list every changed quantity. Exit 1 if any
# changed. mc_* files and drawlog_* files are compared exactly (tolerance 0, as
# text): their contract is bit for bit. timing.txt is reported, never fails.
[ $# -ge 2 ] || { echo "usage: compare.sh <dirA> <dirB> [rel tol] [abs floor]" >&2; exit 2; }
exec python3 -I - "$@" <<'PY'
import os, sys, math
a, b = sys.argv[1], sys.argv[2]
tol = float(sys.argv[3]) if len(sys.argv) > 3 else 1e-9
floor = float(sys.argv[4]) if len(sys.argv) > 4 else 1e-30

def num(s):
    try:
        return float(s)
    except ValueError:
        return None

def parse(path):
    """Ordered {key: (section, line)}; a key repeats with a running count."""
    out, seen, sec = {}, {}, ""
    for line in open(path, errors="replace"):
        line = line.rstrip("\n")
        if not line:
            continue
        if line.startswith("@@ "):
            sec = line[3:]
            continue
        if path.endswith(".tsv"):
            f = line.split("\t")
            key = line if line.startswith("#") or len(f) < 3 else f[1] + "\t" + f[2]
        elif line.startswith("msg "):
            key = "msg " + line.split(" ", 2)[2]
        else:
            key = line.split(" = ")[0]
        seen[key] = seen.get(key, 0) + 1
        out[(key, seen[key])] = (sec, line)
    return out

def value(line):
    if line.startswith("msg "):
        return line.split(" ")[1]
    return line.split(" = ", 1)[1] if " = " in line else line

names = sorted(set(os.listdir(a)) | set(os.listdir(b)))
changed = 0
for name in names:
    if name == "timing.txt":
        continue
    pa, pb = os.path.join(a, name), os.path.join(b, name)
    if not (os.path.exists(pa) and os.path.exists(pb)):
        print(f"{name}: only in {a if os.path.exists(pa) else b}")
        changed += 1
        continue
    exact = name.startswith(("mc_", "drawlog_"))
    da, db = parse(pa), parse(pb)
    n, worst = 0, 0.0
    for k in list(da) + [k for k in db if k not in da]:
        if k not in db:
            print(f"{name} [{da[k][0]}] removed: {da[k][1]}"); n += 1; continue
        if k not in da:
            print(f"{name} [{db[k][0]}] added: {db[k][1]}"); n += 1; continue
        la, lb = da[k][1], db[k][1]
        if la == lb:
            continue
        if name.endswith(".tsv"):
            print(f"{name} [{da[k][0]}] {k[0]}#{k[1]}: {la}  ->  {lb}"); n += 1; continue
        va, vb = value(la), value(lb)
        x, y = num(va), num(vb)
        if exact or x is None or y is None or math.isnan(x) or math.isnan(y):
            print(f"{name} [{da[k][0]}] {k[0]}: {va}  ->  {vb}"); n += 1; continue
        d = abs(x - y)
        m = max(abs(x), abs(y))
        if d <= floor or d <= tol * m:
            continue
        rel = d / m
        worst = max(worst, rel)
        print(f"{name} [{da[k][0]}] {k[0]}: {va}  ->  {vb}  rel {rel:.3e}"); n += 1
    if n:
        print(f"== {name}: {n} of {len(da)} changed" + (f", worst rel {worst:.3e}" if worst else ""))
    changed += n

ta, tb = os.path.join(a, "timing.txt"), os.path.join(b, "timing.txt")
if os.path.exists(ta) and os.path.exists(tb):
    da, db = parse(ta), parse(tb)
    print("-- timing (informational, A -> B):")
    for k in da:
        if k in db and k[0].endswith(("wall_seconds", "transient_analysis_time")):
            print(f"   {k[0]}: {value(da[k][1])} -> {value(db[k][1])}")
print(f"TOTAL changed: {changed}  (rel tol {tol:g}, abs floor {floor:g}, exact for mc_* and drawlog_*)")
sys.exit(1 if changed else 0)
PY
