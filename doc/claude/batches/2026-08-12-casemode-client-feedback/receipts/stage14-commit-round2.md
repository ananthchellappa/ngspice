# Stage 14 — round 2 committed, five commits, audited

Workflow `wf_77d8ce95-305` (task `wxbcmc0ow`), two agents, 0 errors, 353k
tokens, 280 tool calls, 48m54s. One committer, one independent auditor that
did not do the splitting. Verdict **SIGN**.

## The commits, on `ver_50` above `58496a8dc`

| SHA | subject | files |
| --- | --- | --- |
| `8c3fc233a` | docs: the client's round-2 reply and its repro set | 11, +491 |
| `ac1819ff8` | fix: cp_remvar frees a node only when nothing points at it | 9, +767/-28 |
| `4e738fc3e` | feat: report two spellings of one node name | 21, +2407/-37 |
| `9e341a8b7` | feat: record the case mode in the raw header, opt-in | 15, +2106/-25 |
| `f829c9191` | docs: round 2's issues, guide and client reply | 14, +2296/-17 |

Nothing pushed, nothing rewritten, no `git add -A`, no `git commit -a`.

## How each commit was measured

Not in `build-ver_50`. For each commit the committer materialised the
**staged** tree into a separate worktree — `git worktree add` at the previous
commit, then every staged blob written in with `git cat-file blob :path` and
verified byte-for-byte against the index — then `./autogen.sh` + `../configure`
+ `make -j12` + full `make check` in that build. After the last commit, all 67
files differing between `58496a8dc` and `f829c9191` were re-checked
byte-identical to what was built.

| tree | build | `make check` |
| --- | --- | --- |
| `58496a8dc` baseline | clean, rc=0 | 305 PASS, 0 FAIL, rc=0, 5m28s |
| `ac1819ff8` | clean, rc=0, no new warnings | 308 PASS, 0 FAIL, rc=0 |
| `4e738fc3e` | clean, rc=0 | 311 PASS, 0 FAIL, rc=0 |
| `9e341a8b7` | clean, rc=0 | 317 PASS, 0 FAIL, rc=0 |
| `f829c9191` | clean, rc=0 | 317 PASS, 0 FAIL, rc=0 |

Each step's PASS set is the previous set plus exactly the decks that step adds,
**diffed by name rather than by count**: R1 `+unset-{computed,readonly,simvar}-name.cmd`;
R2 `+node-case-collision-report.cir ×3`; R3
`+rawfile-casemode-{header,rewrite,gate-key}.cmd`,
`+rawfile-option-set-policy.cmd`, `+unset-rawfile-option-key.cmd`,
`+rawfile-casemode-header.cir`; R4 identical to R3. Identity lint: 265
comparisons at baseline, 264 from R1 on, "baseline matches" at every step.

Beyond `make check`: `--enable-oldapps` configured and built separately at R3 —
clean, `ngsconvert` links. RED-proved by relinking by hand without
`ngsconvert_stubs.o`: fails on exactly `inp_case_mode_name`,
`inp_reading_netlist`, `vec_name_eq`. This is the breakage that stood at
`58496a8dc` and it is closed by R3.

## The ordering constraint

Held. `0067` is `ac1819ff8`; the raw header line is `9e341a8b7`. Had they
landed the other way round, history would contain a commit at which every raw
file this build writes is a crash trigger for released `ngspice-46`, and a
bisect through that range would hit it.

## Two departures from the proposed split, both disclosed

1. **R2 needed four source files the brief did not list** — `inpcom.c`,
   `inpdefs.h`, `inppas2.c`, `evtcheck_nodes.c`. Without them R2 does not
   compile: `inpsymt.c` dereferences `INPcurrent_card->line_case`. Also placed
   in R2 and unlisted: `tests/regression/misc/Makefile.am` and decision `0001`,
   whose amendment is the record `0018` cites.
2. **Two of `0067`'s five decks moved R1 → R3.** `unset-rawfile-option-key.cmd`
   and `rawfile-option-set-policy.cmd` both `set casemodewrite`, write a raw
   file and load it back; they cannot pass before the writer exists. They would
   have been a red R1. The code, `identity.baseline` and decision `0019` stayed
   in R1 as proposed, so R1 still carries its own record.

## Force-added `.out` (4)

`tests/.gitignore:3` is `*.out`; each added by name with `git add -f` —
`node-case-collision-report.out` in `regression/{misc,case,casedist}` (R2) and
`casedist/rawfile-casemode-header.out` (R3). Afterwards
`git status --ignored -uall -- tests/` lists no untracked `.out`.

**No committed `.out` moved**, contrary to the brief's expectation:
`git diff --name-only -- '*.out'` was empty at the start. The collision warning
goes to `cp_err`, and `tests/bin/check.sh` captures stdout only and filters
warning lines from both sides. R2's commit message says so.

## Left uncommitted

134 byproducts under `tests/` (none in any `TESTS` list; each matched by its
directory's `CLEANFILES`), 16 under `examples/`, 2 capture files under
`feedback/ngspice_upstream/repro/`, 11 `doc/claude/suggestions/next-session-*`
predating the batch. `build-shared/` and `build-ver_50/` are `.gitignore:95`.

The brief's figure of 146 was the session-start count and included the 12
intentional new decks (4 `.cir`, 8 `.cmd`), which are committed. The byproduct
remainder is 134. Zero tracked modifications left after the last commit.

Backup of the pre-commit state — full `git diff` plus a tarball of the
untracked files — and the five `make check` logs are in this session's
scratchpad at `scratchpad/backup/`, as `check-{base,r1,r2,r3,r4}.log`.

## The auditor's three problems, and what happened to each

1. **Stale rationale in a committed test comment.**
   `tests/regression/casedist/rawfile-casemode-header.cir:37` (added at
   `9e341a8b7`) said the unset must precede the load because the loaded plot's
   environment makes the name read-only while that plot is current — true of
   `0061`'s first shape, false after `ac1819ff8` deleted that scan from
   `cp_usrset()`. **Fixed** in a following commit, not by rewriting
   `9e341a8b7`. The replacement was measured rather than asserted: a file
   written under `preserve`, read by a session requesting `distinguish`, reads
   `preserve` in **both** orders — the unset takes the name off the global list
   where `-D` put it and leaves the plot environment holding the file's key.
   The deck's second reason for the order is the true one and was always
   stated on the same line: with the request still set, `$casemode` is answered
   by the session ahead of the file. Deck re-run: PASS.
2. **`ac1819ff8` is not purely `0067`.** `options.c`'s deletion of the `pl_env`
   `US_READONLY` scan is `0061`/decision-`0019` work riding in the
   `0067`-titled commit. Accepted as committed: the commit message body
   explains it and `decisions/0019` ships in the same commit, so the record
   travels with the code. Splitting it further would have put the deletion in a
   commit whose own decision doc lived elsewhere.
3. **The pre-commit backup omits the four gitignored `.out` files**, so their
   byte-identity to the pre-commit working tree cannot be checked from the
   backup. They are instead pinned by the R2/R3 suites passing with those files
   as the frozen expectations. Noted, not repaired — the backup is scratchpad
   state, not a deliverable.

## Two honesty notes the committer volunteered

- `ERROR: (internal) tried to destroy non-existent graph` appears in all five
  logs **including the baseline**. Deck output, not an automake result.
- The 5m28 → 1m04 drop after the first run is `runQaTests.pl` reusing its
  `results/` directory: the model-QA devices were simulated once and re-compared
  four times. Comparison output was identical every run (541 `Checking test`,
  39 `qaSpec` PASS). No commit in this round touches a device model.
- `serial-tests` means there were no per-test `.log`/`.trs` to clear, and none
  were cleared. The instruction to clear them, which this batch's briefs
  repeated for stages, was vacuous throughout.
