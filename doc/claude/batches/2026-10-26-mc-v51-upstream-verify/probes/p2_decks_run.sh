#!/bin/bash
# usage: p2run.sh <ngspice> <outdir>
S=/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad
NG=$(realpath $1); mkdir -p $2; O=$(realpath $2); cd $S/p2decks
for d in override random multiple unusable nosetseed setseedrandom nooption; do
  timeout 60 $NG --batch $d.cir > $O/$d.stdout 2> $O/$d.stderr; rc=$?
  echo "== $d rc=$rc  values: $(grep -h '@r1\[r\]' $O/$d.stdout | awk '{print $3}' | tr '\n' ' ')"
  echo "   stderr warnings: $(grep -c 'Warning' $O/$d.stderr); 'once per session': $(grep -c 'once per session' $O/$d.stderr); override: $(grep -c 're-seeds' $O/$d.stderr); multiple: $(grep -c 'Multiple' $O/$d.stderr); unusable: $(grep -c 'Cannot convert' $O/$d.stderr)"
done
