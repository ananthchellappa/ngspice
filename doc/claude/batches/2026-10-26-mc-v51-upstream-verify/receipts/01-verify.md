# 01: Checking the four upstream patches against current upstream

Date: 2026-10-26. Patches checked:
`doc/claude/batches/2026-10-26-mc-v51-followup/upstream/{p1,p2,p3,p4-crash}/`.

## Short answer

- **Send three patches, in this order: `p1`, `p4-crash/on-p1`, `p2`.**
- **Do not send `p3`.** Upstream already fixed the same bug, in its own way.
- `p2` had to be rebased by hand. The file in `p2/` is now the rebased
  version.
- `p1`, `p4-crash` and `p4-crash/on-p1` apply as they are.

## 1. Upstream branch

The patches were made against `pre-master-47` at `c5cd68015`. Upstream has
since opened a new development branch:

- **`pre-master-48`, tip `a1148c9699f58c32680344ab876ac459d8bb126e`,
  2026-10-26 17:52:09 +0100.** This is also `origin/HEAD`. It has 262
  commits since `c5cd68015`.

The tip commit itself does not build from git. It adds
`tests/xspice/analog/` to `configure.ac` and to `tests/xspice/Makefile.am`,
but the directory was never committed, so `autogen.sh` fails. That is
upstream's bug, not ours. In each scratch checkout I reverse-applied only
those two build-file lines, without committing them, and built everything
on the tip's own code. None of the patches touch those files.

## 2. Does each patch apply?

| patch | alone on the tip | in the series |
| --- | --- | --- |
| `p1` | applies, line offsets only | 1st |
| `p4-crash/on-p1` | does not apply alone (by design: it is built on `p1`) | 2nd, after `p1`: applies, offsets only |
| `p4-crash` | applies, offsets only | only if `p1` is not sent |
| `p2` | **conflicted**, rebased by hand, now applies | 3rd |
| `p3` | **conflicts**, and is superseded | not sent |

The order the task gave, `p4-crash`, `p1`, `p2`, `p3`, cannot work, and the
README already said so: `p1` does not apply after `p4-crash`, because both
change the same "no model type given" check in `inpcom.c`. `p4-crash/on-p1`
exists for this reason. These combinations were applied with plain `git am`
from the final patch files: `p1 p4-crash/on-p1 p2`, `p2 p1 p4-crash/on-p1`
and `p4-crash p2`. The first gives exactly the tree that was built and
tested below.

**`p2`, what changed.** Upstream commit `cbb09dc25` split `com_sseed()` in
two. `com_sseed()` is now the `setseed` command, and it also refills the
Wallace pools. `set_sseed()` does the seeding, and `eval_opt()` now calls it.
In the rebase:
- `seed_from_option()` calls `set_sseed(NULL)` where it used to call
  `com_sseed(NULL)`. This matches what `eval_opt()` does on the tip. Keeping
  `com_sseed()` would have printed an extra "Note: Seed value" line and
  refilled the pools twice.
- The `setseed_used` flag moved into `set_sseed()`. Upstream added a new
  `setseed random` form, and the flag is set for it as well as for
  `setseed N`. Both are explicit choices that the option silently overrides.
  A bare `setseed` still does not set the flag.
- The commit message says this. The message, author and date are otherwise
  unchanged.

**`p3`, why it should not be sent.** Upstream fixed the same bug. Bugs #872
("rndseed is 1 on startup, but is actually process ID") and #873
(".spiceinit does not allow setting random seed") were both filed by Andrew
Sterian, and Holger Vogt fixed them in `059066b53` (2026-10-05), with
follow-ups in `628d84aa5`, `cbb09dc25` and `9ea3cfa84` (the
`sharedspice.c` side). On the tip:
- `initw()` no longer calls `srand(getpid())`.
- `main.c` and `ngSpice_Init()` seed from `rndseed=1` and then call `initw()`.
- `setseed 1` does the same, seed and then `initw()`.

So the first parse already draws what `setseed 1` draws. `p3`'s own test,
`startup-seed.cir`, **passes on the unpatched tip**. Only two things are left
that `p3` would add: a dead `getpid` include in `wallace.c`, and the test as
a guard. I did not rewrite `p3` into a test-only patch, because that would
change what it is. The file in `p3/` is unchanged. If you want to offer the
test, send it as a separate test-only patch that cites #872 and #873.

## 3. make check

Each tree was built out of tree with plain `../configure`, run through
`ngheavy`, with `-j4`.

| build | make check | counts |
| --- | --- | --- |
| tip, unpatched | exit 0 | 57 PASS, 0 FAIL, 0 ERROR, 0 SKIP |
| tip + `p1`, `p4-crash/on-p1`, `p2` | exit 0 | 62 PASS, 0 FAIL, 0 ERROR, 0 SKIP |

The 5 extra PASS lines are the patches' new tests. Every test that passed
before still passes. No test went from pass to fail. Both builds printed 328
compiler warnings.

**New tests, run by hand with the tip's `tests/bin/check.sh`:**

| test (patch) | unpatched tip | patched |
| --- | --- | --- |
| `temper-res-behav-1.cir` (p1) | FAIL, maxerr 0.314, "ERROR: test failed" | PASS |
| `temper-res-behav-noise.cir` (p1) | FAIL, maxerr 0.145 | PASS |
| `temper-res-behav-noise-subckt.cir` (p1) | FAIL, maxerr 0.156 | PASS |
| `temper-res-behav-dup.cir` (p1) | FAIL, maxerr 0.282 | PASS |
| `model-typeless-if-dead.cir` (p4-crash) | FAIL: SIGSEGV, exit 139, 3 of 3 runs | PASS, "INFO: success", 3 of 3 runs |
| `startup-seed.cir` (p3) | **PASS** (already fixed upstream) | PASS |

Each failure is the one the patch fixes. The `p1` maxerr numbers are the
same as the fork's pre-patch numbers in the README.

**`p2` has no upstream test**, because its output goes to stderr, which
`check.sh` does not capture. I checked it with 7 small decks, saved in
`../probes/p2decks/` with their runner `../probes/p2_decks_run.sh` (the
runner uses scratch paths and needs editing before it is reused). The deck
runs `setseed 1/2/3`, each followed by `reset` and `op`:

| deck | unpatched tip | patched |
| --- | --- | --- |
| `.option seed=5` | same value 3 times, **no warning** | same values, override warning printed **once** |
| `.option seed=random` | no warning | override warning (random wording) once |
| two `seed=` options | "Multiple" warning 4 times | "Multiple" once, then the override once |
| `.option seed=abc` | "Cannot convert" 4 times | once, with "(printed once per session)" |
| `seed=5`, only a bare `setseed` | silent | silent |
| `seed=5` + `setseed random` | silent | override warning once |
| no option | silent | silent |

Stdout was byte-identical before and after for every deck except
`setseed random`, whose "Note" line prints the process id. So the sequence
of draws did not change.

## 4. libngspice check

I configured a second build of tip + `p1`, `p4-crash/on-p1`, `p2` with
`--with-ngshared` and built `libngspice.so.0.0.16`. The original probe
(`2026-10-24-mc-v51/probes/rnddraws_reset_probe.c`) tests the fork-only
draw log, so I wrote a scratch adaptation that covers only the init, reset,
re-seed and warning-flag paths. It is saved as
`../probes/seed_reset_probe.c`. It runs three sessions:
init, `ngSpice_Reset`, init, `ngSpice_Reset`, init. Result: **PASSED, 0
failures.**

| check | result |
| --- | --- |
| session 1: `$rndseed` reads 1 after `ngSpice_Init` | pass |
| session 1: the first parse draws what `setseed 1` draws (9.332898e+02 both) | pass |
| session 1: override warning once over four parses | pass |
| session 1: "Multiple" and "Cannot convert" once each over three parses | pass |
| session 2: `$rndseed` reads 1 again after `ngSpice_Reset` + `ngSpice_Init` | pass |
| session 2: startup draw equals session 1's | pass |
| session 2: `setseed 1` draws what it drew in session 1 | pass |
| session 2: override warning printed again (flags cleared by `totalreset()`) | pass |
| session 2: "Multiple" and "Cannot convert" printed again | pass |
| session 2: and not repeated on the next `reset` | pass |
| session 3: no `setseed N` yet (a bare `setseed` only): silent | pass |
| session 3: the first `setseed N` makes it warn | pass |

My first version of the probe had 2 failures. Both were my mistake: it ran
`setseed 1` before testing the "silent without setseed" case. I moved that
case into a fresh session 3. I did not run the probe against an unpatched
library, so the warning checks are shown to pass but not shown to fail
before. The CLI decks above show the "before" behaviour.

## 5. Overlap upstream (read only)

**Commits since `c5cd68015` that touch the same code:**
- `059066b53`, `628d84aa5`, `cbb09dc25` and `9ea3cfa84` rework the startup
  seed, `.option seed` in comfiles, and `setseed`. `p3` is superseded by
  these, and `p2` was rebased onto them. Neither of them adds the
  override, once-only or reset behaviour that `p2` adds.
- `940bb50fa` ("evaluate .param with agauss etc. once") and `3ef069fb1`
  (bug #876) change `inpcom.c`. Neither touches the R to B rewrite or the
  model-type check. `p1` and `p4-crash` apply cleanly, and their tests pass
  on top of them.
- `b2a86d38c` and `dd3a8dd15` change `eval_opt()` for cshunt and rshunt,
  next to `p2`'s hunk. There is no conflict.

**SourceForge tickets** (bugs and patches pages, read only):
- Bugs [#872](https://sourceforge.net/p/ngspice/bugs/872/) and
  [#873](https://sourceforge.net/p/ngspice/bugs/873/), both open-fixed, are
  the `p3` topic, fixed by `059066b53`.
- [#871](https://sourceforge.net/p/ngspice/bugs/871/) (a function argument
  is replaced by a same-named random parameter) is near the Monte Carlo area,
  but not the same bug.
- No open bug or patch ticket covers R to B-source conversion and tc,
  `.option seed` overriding `setseed`, or the typeless `.model` crash. The
  nearest one, bug [#862](https://sourceforge.net/p/ngspice/bugs/862/)
  (a segfault on an invalid OSDI `.model` card), is a different path.

Nothing was posted, filed, mailed or pushed.

## 6. What changed in this repo

- `doc/claude/batches/2026-10-26-mc-v51-followup/upstream/p2/0001-randnumb.c-warn-once-when-.option-seed-overrides-set.patch`:
  regenerated with `git format-patch` against `pre-master-48`.
- `.../upstream/README.md`: a new "Re-verified on pre-master-48" paragraph,
  and the `p3` table row marked as superseded. Nothing else was changed, and
  `git diff` shows no section dropped.
- This receipt, and `../probes/`.

## 7. Clean-up

The clone, its three worktrees, the three build trees (unpatched, patched,
`--with-ngshared`), the run outputs and the scratch logs were all deleted
from the scratchpad. Nothing was written to `/home/analog/dev/ngspice`
outside the paths listed above. No remote was added, and the repo's config
was not changed.
