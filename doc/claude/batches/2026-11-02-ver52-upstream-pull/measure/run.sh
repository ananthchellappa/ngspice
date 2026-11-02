#!/bin/bash
# run.sh <ngspice binary> <output dir>: run the measurement decks and write one
# normalised result file per deck, so compare.sh can diff two runs.
#
#   <deck>.txt        name = value lines, section labels (@@), bin choice, exit
#                     status and counted warnings. Deterministic.
#   mc_*.ref.txt      mc_r and mc_sky130 under the reference binary (REF, default
#                     /usr/bin/ngspice 45.2), for the bit-for-bit check
#   mc_identity.txt   yes or no for those two decks against REF
#   drawlog_*.tsv     the draw logs as ngspice wrote them
#   timing.txt        wall clock and analysis times. Not deterministic;
#                     compare.sh reports it separately and it never fails.
#
# Environment: REF, SKY130 (the combined sky130.lib.spice). The run is hermetic:
# HOME and the working directory are a fresh temp dir holding the .spiceinit the
# PDK asks for. Run it through ngheavy: ngheavy measure/run.sh BIN OUT
set -u
export LC_ALL=C
[ $# -eq 2 ] || { echo "usage: run.sh <ngspice binary> <output dir>" >&2; exit 2; }
BIN=$(readlink -f "$1"); OUT=$2
HERE=$(cd "$(dirname "$0")" && pwd)
REF=${REF:-/usr/bin/ngspice}
SKY130=${SKY130:-/home/analog/private/xschem-private/sky130A/models/libs.tech/combined/sky130.lib.spice}
[ -x "$BIN" ] || { echo "run.sh: no binary $BIN" >&2; exit 2; }
[ -r "$SKY130" ] || { echo "run.sh: no PDK $SKY130" >&2; exit 2; }
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
RUN=$(mktemp -d); trap 'rm -rf "$RUN"' EXIT
printf 'set ngbehavior=hsa\nset ng_nomodcheck\n' > "$RUN/.spiceinit"
: > "$OUT/timing.txt"
echo "version = $("$BIN" --version | sed -n 's/^\*\* \(ngspice-[^ ]*\).*/\1/p')" >> "$OUT/timing.txt"
fail=0

# Keep result lines only: "name = value" with no spaces in either, @@ labels, the
# bin showmod reports, the deterministic rusage counts. Warnings and errors go
# last, counted. Long instance paths are shortened to keep the files small.
normalise() {
    awk '
    /^@@ /                                  { print; next }
    /^[^ \t]+ = [^ \t]+$/                   { print; next }
    /^ +model +[^ ]+:[^ ]+$/                { split($2, a, ":"); print "bin." a[1] " = " a[2]; next }
    /^(Total iterations|Transient iterations|Circuit Equations|Circuit original non-zeroes|Transient timepoints|Accepted timepoints|Rejected timepoints) = / {
        n = $0; sub(/ = .*/, "", n); gsub(/ /, "_", n); print "rusage." n " = " $NF; next }
    ' "$1" | sed -E 's/@m\.([^.]+)\.msky130_fd_pr__[np]fet_01v8/\1/; s/^m\.xm1\.msky130_fd_pr__nfet_01v8:/xm1:/; s/^(op[0-9]+|noise[0-9]+|const)\.//'
    grep -E 'Warning|Error|ERROR|Fatal|error:' "$1" | sort | uniq -c | sed -E 's/^ *([0-9]+) /msg \1 /' | cut -c1-200
}

# time_and_keep <deck> <binary> <result file>: run in the temp dir
run_deck() {
    local deck=$1 bin=$2 res=$3 raw start end st
    sed "s|@SKY130@|$SKY130|" "$HERE/decks/$deck.cir" > "$RUN/$deck.cir"
    raw="$RUN/$deck.raw"
    start=$(date +%s.%N)
    ( cd "$RUN" && HOME="$RUN" "$bin" --batch "$deck.cir" ) > "$raw" 2>&1 < /dev/null
    st=$?
    end=$(date +%s.%N)
    { normalise "$raw"; echo "exit = $st"; } > "$res"
    if [ "$res" = "$OUT/$deck.txt" ]; then
        awk -v d="$deck" -v s="$start" -v e="$end" 'BEGIN { printf "%s.wall_seconds = %.3f\n", d, e - s }' >> "$OUT/timing.txt"
        sed -nE "s/^(Total analysis time|Transient analysis time) \(?.*= *([0-9.eE+-]+) *$/$deck.\1 = \2/p; s/^(Transient analysis time) = *([0-9.eE+-]+) *$/$deck.\1 = \2/p" "$raw" | sed -E 's/ time =/_time =/; s/^([^ ]+)\.Total analysis/\1.total_analysis/; s/^([^ ]+)\.Transient analysis/\1.transient_analysis/' >> "$OUT/timing.txt"
    fi
    [ "$st" -eq 0 ] || { echo "run.sh: $deck exit $st under $bin" >&2; fail=1; }
}

for f in "$HERE"/decks/*.cir; do
    run_deck "$(basename "$f" .cir)" "$BIN" "$OUT/$(basename "$f" .cir).txt"
done
for f in "$RUN"/drawlog_*.tsv; do cp "$f" "$OUT/"; done

# Bit for bit: the Monte Carlo decks under REF, all result lines but the exit
# and the messages (a warning text may differ between versions). numdgt=17 in
# the decks prints enough digits to round-trip a double.
: > "$OUT/mc_identity.txt"
for d in mc_r mc_sky130; do
    run_deck "$d" "$REF" "$OUT/$d.ref.txt"
    if diff <(grep -v '^msg ' "$OUT/$d.txt") <(grep -v '^msg ' "$OUT/$d.ref.txt") > /dev/null; then
        echo "$d.identical_to_ref = yes" >> "$OUT/mc_identity.txt"
    else
        echo "$d.identical_to_ref = NO" >> "$OUT/mc_identity.txt"
        echo "run.sh: $d differs from $REF" >&2; fail=1
    fi
done
exit $fail
