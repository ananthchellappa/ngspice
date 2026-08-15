# Upstream submission: the `cp_remvar()` crash

What is here, and what it is for.

| file | what it is |
| --- | --- |
| `EMAIL-cp_remvar.txt` | the message to send to `ngspice-devel@lists.sourceforge.net`, ready to paste |
| `0001-cp_remvar-ownership.patch` | `src/frontend/variable.c` — the ownership rule. Fixes five of the six reproducers |
| `0002-curplotdate-static.patch` | `src/frontend/options.c` — the `Spice_Build_Date` guard. Fixes the sixth |
| `repro-option-line.raw` | a 377-byte ASCII raw file carrying `Option: myopt = seven`, for the loaded-plot reproducer |
| `cp-remvar-crash-and-the-header-default.md` | the long-form report: the bug, the six shapes, and why it gates our raw-header default |

## Both patches are against upstream `master`, not this branch

They were generated and validated against a clean worktree of
`remotes/upstream/master` at `2f9c8ad47` ("Update the news for ngspice-47"):

- `git apply --check` — both clean
- `./autogen.sh && ../configure && make -j12` — exit 0, no new warnings
- all six reproducers — rc=0
- upstream's own `make check` — 58 tests, 0 failures, exit 0, 42 directories

`0002` needed regenerating from our commit because this branch had changed the
surrounding `eq()` to `eqc()` for casemode work; the version here matches
upstream's context. Both patches were re-validated after a late comment edit,
so the numbers above describe the files as they stand.

## Deliberately not included

The deletion of the `pl_env` read-only scan in `cp_usrset()`
(`doc/claude/decisions/0019`) is ours, not upstream's business: it is a
behaviour change tied to the casemode feature, and it is not needed to fix any
crash. That independence is measured, not assumed — a build with the ownership
rule applied and the scan restored returns rc=0 on the loaded-plot reproducer,
refusing the `unset` with `Error: <name> is read-only.` instead of crashing.

## Reproducing, quickly

```sh
printf 'unset curplot\nquit 0\n'                     | ngspice -p -n ; echo $?
printf 'set curplotdate=x\nquit 0\n'                 | ngspice -p -n ; echo $?
printf 'load repro-option-line.raw\nunset myopt\ndisplay\nquit 0\n' \
                                                     | ngspice -p -n ; echo $?
```

Released `ngspice-46` and unpatched `master` give 134, 134, 139. Patched gives
0, 0, 0.

Capture the exit status straight into a variable. Writing
`echo "... rc=$?"` with a command substitution earlier in the same line reports
the substitution's status instead of ngspice's, which made a first pass at
these measurements read 0 everywhere.

## Why it matters here

`doc/codex/issues/0061` writes `Option: casemode=<mode>` into the raw header.
Any ngspice that loads such a file gets a `casemode` variable in the loaded
plot's environment, and a released ngspice that unsets it dies. That is why the
line is opt-in behind `casemodewrite`, default off, and the default cannot flip
until this fix is in a public release. See the report for the full argument.
