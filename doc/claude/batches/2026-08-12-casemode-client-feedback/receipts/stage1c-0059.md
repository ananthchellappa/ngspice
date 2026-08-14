# Stage 1c receipt — issue 0059

Binary `build-ver_50/src/ngspice` (`ngspice-46+`, build stamp
`Wed Aug 12 19:28:37 UTC 2026`), baseline `/usr/local/bin/ngspice`, decks copied
out of `doc/claude/feedback/ngspice_upstream/repro/` and run in a scratch dir.

**1. FIXED, and the count was four, not three.** Ran all nine decks both ways
(`-b -n` vs `-b -n -D casemode=preserve`). Six need the flag to produce the
quoted output — `save_lower` (570 vs 296 bytes, `Plotname: constants` vs
`Operating Point`), `seq` (341 vs 317, 7-line stderr vs empty), `rc0` (570 vs
303), `status_probe` (`1 const` / `0 op1` vs `0 op1` / `0 op2`), `write_named`
(rc=1 no file vs rc=0 324 bytes), `write_named_capture` (`captured: Warning from
checkvalid: …` vs `captured: binary raw file "write_named.raw"`). Two already
named it (lines 39, 222 of the old text); **four** did not — the checker's three
plus `seq.cir`, whose block quoted a 341-byte raw and a 7-line stderr that only
exist under `preserve`. Flag now named at every point of use: the `seq` block,
the `status_probe` block (with its `grep`), matrix rows 1/2/6 (deck column is now
"deck (flag)", rows 3 and 7 marked `no -D`), criterion 3 for both `write_named`
decks, and the `source rc0.cir` pipe in the `rc` section — that one also only
exits 0 over a bogus raw under the flag (570-byte constants raw with it,
303-byte `Operating Point` without). A new Status paragraph lists which six take
`preserve` and which three take no `-D`. Three unmarked measurements were
re-run and re-quoted while there: `seq.raw`'s `Date:` (its run time), the
`divider.raw` half of the two-tells comparison (now names `repro/divider.cir`,
325 bytes and a run-time `Date:` under both flags), and `write_named`'s stderr
(nine lines, not the two shown — the `grep` that cuts it is now in the block).

**2. FIXED.** `grep -rn 'plot_cur =' src/` returns 23, the 23rd being
`src/frontend/vectors.c:1117` `if (plot_cur == NULL)`; `grep -rn 'plot_cur = '
src/` returns 22. Root Cause now quotes the trailing-space form and says in-line
that it is what excludes `vectors.c:1117`. The enumeration and the load-bearing
"20 in-process sites" are unchanged.

**3. FIXED by wording, not by a commit.** Nothing under `doc/claude/feedback/`
is committed (the eight decks are `git add`-staged; `save_lower.cir`,
`divider.cir` and `run_all.sh` are untracked; the six issue files are untracked
too), so the Status claim now reads "Every deck quoted below **lives under**
`doc/claude/feedback/ngspice_upstream/repro/`" and makes no claim about git
state — true whether or not the batch commits the tree later.

Six headings unchanged and in order, no seventh section. `src/` and `tests/`
untouched.
