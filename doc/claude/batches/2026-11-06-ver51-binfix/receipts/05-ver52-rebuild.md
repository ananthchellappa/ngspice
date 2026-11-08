# 05 — rebuild and check of ver_52 at dff81c50e (build-ver_52)

Date: 2026-11-07. Worktree /home/analog/dev/ngspice-ver_52, branch ver_52, HEAD dff81c50e
(merge of ver_51 that adds only docs). No git writes. Worktree clean before and after.
Main checkout /home/analog/dev/ngspice, build-ver_50/ and build-ver_51/ were not touched (only read).

## Staleness before the rebuild
- Last commit touching src/: c85f0ac88 "VDMOS: read back ttt1 ttt2 tm1 tm2, ...", 2026-11-02 17:04:31 -0700.
- build-ver_52/stage/bin/ngspice was dated 2026-11-02 17:03:24, and src/ngspice 17:03:22. That is
  about one minute **older** than that commit by mtime.
- This was a false alarm. The vdmos sources (vdmosmask.c, vdmosmpar.c) are dated 17:00:21 and their
  objects 17:03:05. The binary was built from the working tree just before the commit was made.
  `make` found nothing to recompile or relink.

## Build
Both commands ran via ngheavy. Logs are in build-ver_52/claude-*.log.
- `ngheavy make -C build-ver_52 -j4`: rc 0 (claude-make.log). The only regeneration was automake
  re-running for tests/regression/model/Makefile. Nothing was compiled, and src/ngspice was not relinked.
- `ngheavy make -C build-ver_52 install`: rc 0 (claude-install.log). It copied the binary again,
  so stage/bin/ngspice now has mtime 2026-11-07 18:15.

## make -k check -j4 (claude-check.log)
rc 2. **395 PASS, 1 FAIL**, no XFAIL/XPASS/SKIP. 396 results in total, against 366 on ver_51,
because ver_52 has more tests. Every suite ran, including the model QA directories (bsim3, bsim4, bsimsoi,
hicum2, hisim, hisimhv1, hisimhv2) and tests/lint.

### FAIL: tests/regression/pipe/resource-keyword-case.cmd (expected)
Excerpt:
```
strstr sihit "$siline" "Nominal temperature"
if $sihit < 0
>   echo "ERROR: lowercase 'rusage task' printed no simulator stat, line = [$sil ine]"
>   quit 1
> end
ERROR: lowercase 'rusage task' printed no simulator stat, line = []
ngspice-46+ done
FAIL: resource-keyword-case.cmd
```
The cause is confirmed by a read-only diff against the owner's uncommitted
/home/analog/dev/ngspice/tests/regression/pipe/resource-keyword-case.cmd. The committed version
reads exactly one line after the frontend statistic and expects the simulator statistic there.
build-ver_52 is configured with `--enable-cider` (`#define CIDER 1`). In a CIDER build, printres()
prints a blank line and then every statistic, so that one line is empty (`line = []`). The owner's
edit replaces the fixed read with a `repeat 100` loop that scans the capture for
"Nominal temperature", at both check sites. ver_52 doesn't have that edit, so the test fails here.
It passed in build-ver_51 (receipt 04), which ran with the edit.

### Other `ERROR:` text in the log (not test results)
- "ERROR: (internal)  tried to destroy non-existent graph" is ngspice output inside a passing
  test (between vector-let-report.cir and vector-probe-report.cir). The same line appears in
  build-ver_51/claude-check.log.

## Binary
- sha256 build-ver_52/stage/bin/ngspice: `3cb7eee49e99567bf6e5cf50e46353328764a0b4ddb8f457ab5efee7f27c031c`
- build-ver_52/src/ngspice is identical (same sha256, and `cmp` reports no difference).
