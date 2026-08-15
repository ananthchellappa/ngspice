# Receipt: item 1 — the exit-status harness

PLAN item **1 — The harness: `check_status.sh` and `tests/regression/exitstatus/`**,
from `doc/claude/batches/2026-08-15-exit-status-testing/PLAN.md`.

Crew A, 2026-08-15, branch `ver_50`. HEAD at start `f14fc04aa`.

**Code was touched**, but no file under `src/` was: the change is one new test
driver, one new test directory with two decks, and the two lines of build
metadata that register it. Nothing under `doc/claude/upstream/` was opened or
written, and nothing here states or implies that any upstream submission has
been sent.

Binary: `build-ver_50/src/ngspice`, `ngspice-46+`, build stamp
**`Sat Aug 15 23:47:16 UTC 2026`**. The stamp moved during the item: re-running
`../configure` from `build-ver_50` regenerated `config.h`, so `make -j8`
relinked. The first measurement of the real deck was taken on the previous
binary (`Sat Aug 15 18:18:34 UTC 2026`) and gave the same answer.

Scratch: `/tmp/claude-1000/-home-qflow-dev-ngspice-test/d775575c-7554-4eec-9dca-17c55bd7d1cf/scratchpad`.
Nothing was left in the repository or in `build-ver_50/tests/regression/exitstatus/`
beyond the generated `Makefile`.

## The RED failure, verbatim

This item builds a harness, so RED-first means proving the driver can fail
before trusting it passing. Each assertion kind was corrupted in turn and the
corruption reverted. The first two were run through the real harness
(`make -C build-ver_50/tests/regression/exitstatus check`); the `files` ones
were run through the same command line `TESTS_ENVIRONMENT` builds, with a
temporary deck, because no committed deck writes a file yet.

### 1. Wrong `.status` — `tran-empty-netlist.status` set to `0`

```
check_status: tran-empty-netlist: FAILED (status)
  expected exit status 0, got 1
  stderr was:
    Error: incomplete or empty netlist
           or no ".plot", ".print", or ".fourier" lines in batch mode;
    no simulations run!
check_status: tran-empty-netlist: FAILED: status
  The captured output is in tran-empty-netlist.stdout and tran-empty-netlist.stderr.
FAIL: tran-empty-netlist.cir
```

### 2. Wrong `.err` — last line changed to `some simulations run!`

```
--- tran-empty-netlist.stderr.expected	2026-08-15 16:49:31.392371834 -0700
+++ tran-empty-netlist.stderr.filtered	2026-08-15 16:49:31.392371834 -0700
@@ -1,3 +1,3 @@
 Error: incomplete or empty netlist
        or no ".plot", ".print", or ".fourier" lines in batch mode;
-some simulations run!
+no simulations run!
check_status: tran-empty-netlist: FAILED (err)
  filtered stderr does not match ../../../../tests/regression/exitstatus/tran-empty-netlist.err (- expected, + actual)
check_status: tran-empty-netlist: FAILED: err
  The captured output is in tran-empty-netlist.stdout and tran-empty-netlist.stderr.
FAIL: tran-empty-netlist.cir
```

### 3. The `files` check — and a correction to the item's wording

The item asks for "an `absent` line for a file that exists". **That corruption
cannot fail, by construction, and the construction is requirement 6 of the same
item**: the driver deletes every path a `.files` line names *before* the run,
so a file that merely exists beforehand is gone by the time `absent` is
evaluated. Measured, not reasoned: a stale `redfiles.raw` in the build
directory, asserted `absent`, passes.

So the two corruptions that are actually load-bearing were run instead, and
together they cover the same ground plus requirement 6 itself.

**3a. `absent <path>` for a file the run creates** (temporary deck with a
`.control` block doing `op` / `write redfiles.raw`):

```
  absent redfiles.raw: the run created it
check_status: redfiles: FAILED (files)
check_status: redfiles: FAILED: files
  The captured output is in redfiles.stdout and redfiles.stderr.
rc=1
```

**3b. `present <path>` satisfied only by a file an earlier run left.** A
38-byte `redfiles.raw` was written into the build directory by hand, then a
deck that writes nothing was run against `present redfiles.raw`. This is the
direct proof of requirement 6 — without the pre-run delete this line would
have passed:

```
  present redfiles.raw: the run did not create it
check_status: rednowrite: FAILED (files)
check_status: rednowrite: FAILED: files
  The captured output is in rednowrite.stdout and rednowrite.stderr.
rc=1
```

**3c. `grep <path> <text>` that misses** (the same writing deck, asserting a
transient plotname on an operating-point rawfile):

```
  grep redfiles.raw 'Plotname: Transient Analysis': not found; the file holds:
      Title: * temporary red deck: writes a rawfile
      Date: Sat Aug 15 16:50:23  2026
      Command: ngspice-46+, Build Sat Aug 15 23:47:16 UTC 2026
      Plotname: Operating Point
      Flags: real
      No. Variables: 2
      No. Points: 1
      Variables:
      	0	v(in)	voltage
      	1	i(v1)	current
      Binary:
      ????????????MbP?check_status: redfiles: FAILED (files)
check_status: redfiles: FAILED: files
rc=1
```

### 4. The signal check, which is the one item 4 will lean on

Not asked for, but the property PLAN item 4 needs, so it was demonstrated on
`printf '*\n.op\n'` with a `.status` of `1`:

```
check_status: redsignal: FAILED (signal)
  the simulator did not terminate normally: rc=134 is 128+6, SIGABRT.
  A run that cannot proceed has to say so and return a status.
check_status: redsignal: FAILED (status)
  expected exit status 1, got 134
  stderr was:
    ngspice: ../../../src/frontend/dotcards.c:225: ft_cktcoms: Assertion `plot_cur->pl_dvecs != NULL' failed.
    Aborted (core dumped)
check_status: redsignal: FAILED: signal status
```

The signal check is independent of `.status`: it fires on any `rc >= 128`
whatever the file says, so "terminated normally rather than on a signal" is
named in the failure text and not inferred from a number.

### 5. The committed self-test, which is the RED evidence that stays

`selftest-status.cir` runs cleanly and `selftest-status.status` says `3`.
It passes because the driver reports the mismatch, and it will stop passing
the day the driver stops being able to:

```
check_status: selftest-status: self-test.  The failure reported below is
  the one this deck exists to produce; the verdict is inverted.
check_status: selftest-status: FAILED (status)
  expected exit status 3, got 0
  stderr was empty
check_status: selftest-status: self-test passed; the driver reported: status
PASS: selftest-status.cir
```

## The production change

Six new files, two edited.

**`tests/bin/check_status.sh`** (new, 300 lines, plain POSIX sh, mode 644 like
`check.sh` — it is invoked as `$(SHELL) <path>`). Mirrors `check.sh`'s
`SPICE_SCRIPTS`/`ngspice_vpath` preamble so spinit is found. Takes the
simulator as `$1` and uses it **unquoted** so a multi-word string word-splits,
the way `tests/regression/casedist/Makefile.am` needs; takes the deck as the
**last** argument rather than as `$2`, because the simulator argument may carry
flags. Runs `$SPICE --batch -n <deck>` with stdout to `<base>.stdout` and
stderr to `<base>.stderr` in the build directory, and then:

- **status** — compares `$?` with the required `<base>.status`. Mismatch prints
  expected, actual and the raw stderr.
- **signal** — `rc >= 128` is always a failure, reported as `128+<n>` with the
  signal named from a table. A `.status` holding `>= 128` is refused as a
  broken spec, since a shell cannot tell `exit(134)` from `SIGABRT` and this
  driver has already ruled on which one it calls it.
- **err** — optional `<base>.err`, filtered and `diff -B -w -u`'d. Both sides
  go through the same filter, so a reference may be pasted straight out of a
  run.
- **files** — optional `<base>.files`: `absent <path>`, `present <path>`,
  `grep <path> <text>` (BRE), `#` comments, blank lines ignored. Every path is
  `rm -f`'d before the run. Paths that are absolute or contain `..` are refused
  as a broken spec, because the driver deletes them.
- **mustfail** — optional `<base>.mustfail` names the checks that must fail.
  The verdict inverts and requires **set equality**: exit 0 iff exactly those
  checks failed. Unknown names are refused.

A malformed assertion file exits 1 through a separate path that `.mustfail`
never inverts, so a broken test can never be read as a passing self-test.
Captures are removed on success and left in place on failure.

**The filter, entry by entry.** Four things, and only these:

| entry | what it does | why |
| --- | --- | --- |
| `sed s,[^ \t]*/\(src/[^ \t:]*\),\1,g` | strips the directory prefix from a C source path | an assertion carries `__FILE__` as the compiler saw it. This build says `../../../src/frontend/dotcards.c:225`; an in-tree build says `src/frontend/dotcards.c:225`. The `../` run counts build-directory levels, not anything the simulator said. **Measured**, on the `.op` deck above. |
| `sed s,[^ \t]*/\(<base>\.cir\),\1,g` | strips the directory prefix from the deck's own name | automake hands the deck over as `$(srcdir)/<base>.cir`; `$(srcdir)` is `.` in a source-tree build and a relative or absolute path in a VPATH build. |
| `grep -v -E '^(Aborted\|Killed\|Segmentation fault\|…)( \(core dumped\))?$'` | drops the shell's own report of a child killed by a signal | **Measured, and a real trap for item 4.** With `$(SHELL)` = dash, `Aborted (core dumped)` is written into the *redirected* stderr of the command and lands in the capture; with `$(SHELL)` = bash it goes to the shell's stderr and does not. Which shell configure picked is not a property of ngspice. Nothing is lost — the death is reported from `$?` by the signal check. |
| `grep -v -E '^\*\*\|^CPU time\|^Total (analysis\|elapsed) time\|^(Total DRAM\|DRAM currently\|Maximum ngspice\|…)'` | drops the banner and the resource-usage block | version string, build Creation Date, wall-clock durations, free-memory figures. **Insurance, not measured**: no deck here puts these on stderr today, and they are listed so a deck that redirects them cannot freeze a build stamp or a machine's spare RAM into a reference file. |

Deliberately **not** filtered: anything matching `Error` or `Warning`.
`check.sh`'s `FILTER` strips both as unanchored substrings, which is right for
a table of numbers and would delete the entire subject here.

**`tests/regression/exitstatus/Makefile.am`** (new). `TESTS`,
`TESTS_ENVIRONMENT` naming the new driver and `$(top_builddir)/src/ngspice`,
`EXTRA_DIST` listing the decks and all five sibling assertion files,
`CLEANFILES` for both captures per deck, the two filter scratch files and
`rawspice.raw`, and `MAINTAINERCLEANFILES = Makefile.in`. The header block, in
`tests/lint/Makefile.am`'s voice, says what the directory is for and why it
does not use `check.sh`.

**`tests/regression/exitstatus/tran-empty-netlist.{cir,status,err,files}`**
(new). Title line, three comment lines, `.tran 1n 10n`, `.end`. The real
assertion: `rc=1`, the three-line `incomplete or empty netlist` diagnostic on
stderr, and no `rawspice.raw`.

**`tests/regression/exitstatus/selftest-status.{cir,status,mustfail}`** (new).
A deck that runs, a `.status` of `3`, a `.mustfail` of `status`.

**`tests/regression/Makefile.am`** — `exitstatus` appended to `SUBDIRS`.
**`configure.ac`** — `tests/regression/exitstatus/Makefile` added beside the
other `tests/regression/*/Makefile` entries. Then `./autogen.sh` from the repo
root and a bare `../configure` from inside `build-ver_50`.

`tests/.gitignore` ignores `*.test`, `*.log`, `*.out`, `*.vcd`; the root
`.gitignore` adds nothing that touches `.cir`, `.status`, `.err`, `.files` or
`.mustfail`. No `git add -f` was needed.

`tests/lint/identity.baseline` is **unchanged** — this item adds no C at all.
Both lint specs report `baseline matches` (264 and 11 comparisons).

## The two `make check` counts

| run | result |
| --- | --- |
| `make -C build-ver_50/tests/regression/exitstatus check` | 2 PASS / 0 FAIL — "All 2 tests passed" |
| `make check` from `build-ver_50` | **324 PASS / 0 FAIL**, `make` exit status 0 |

Baseline was 322 PASS / 0 FAIL; the two new decks account for the whole
difference, and no existing test moved.

## What was measured

**The real deck's numbers, re-measured rather than copied from 0072.**

```
$ printf '* …\n.tran 1n 10n\n.end\n' > tran-empty.cir
$ ngspice --batch -n tran-empty.cir >so 2>se ; echo rc=$?
rc=1
$ cat se
Error: incomplete or empty netlist
       or no ".plot", ".print", or ".fourier" lines in batch mode;
no simulations run!
$ ls rawspice.raw
ls: cannot access 'rawspice.raw': No such file or directory
```

Same on both binaries involved (stamps `18:18:34` and `23:47:16`). 0072's
table said `rc=1` and the same wording; confirmed independently.

**The self-test deck** (`v1`/`r1`/`.op`) exits **0** with empty stderr and
writes no rawfile.

**Where the shell's job message goes**, since it lands in a stderr capture and
would otherwise have to be frozen into a reference file:

| shell | capture holds | shell's own stderr holds |
| --- | --- | --- |
| dash | the assertion line **and** `Aborted (core dumped)` | nothing |
| bash | the assertion line only | `/bin/bash: line 1: … Aborted (core dumped) …` |

`build-ver_50/tests/regression/exitstatus/Makefile` has `SHELL = /bin/bash`,
so today's harness sees the bash column; `/bin/sh` on this machine is dash, so
another configure would see the other. The filter covers both.

**automake hands the deck over as `$(srcdir)/<deck>`** in a VPATH build —
`build-ver_50/tests/regression/misc/Makefile`'s `check-TESTS` recipe, the
`else dir="$(srcdir)/"` arm. `AM_INIT_AUTOMAKE` carries `serial-tests`, so
there is no per-test `.log` and the driver's output goes straight to the
console.

**The `.op` abort still reproduces** on this binary: `rc=134`,
`dotcards.c:225`. Untouched by this item; it is PLAN item 4's.

## What was left, deliberately

- **Issues 0069 and 0072 are not touched.** No `$sim_status` deck, no guard, no
  edit to either issue file. Items 2 and 4 own those. The directory has exactly
  two decks, as the item specifies.
- **`tests/bin/check.sh` is unmodified**, as is every existing test directory.
- **No `nogrep` verb.** The item asks for "at least" the three, and the three
  cover what items 2 and 4 were costed for (`absent` for a rawfile that must
  not appear, `present` + `grep` for one that must, with its header). A fourth
  verb with no caller would be a guess.
- **No `-r` in the driver's command line.** `--batch -n <deck>` and nothing
  else, so a deck controls its own rawfile through a `.control` block and the
  `.files` assertions stay about what the deck asked for. 0072 criterion 5 is
  about the `-r` route specifically; if item 4 needs it, the driver will need a
  per-deck flag file and that is a change, not a configuration.
- **Captures are not compared to a `.out`.** stdout is captured only so it can
  be looked at after a failure. A stdout comparison is what `check.sh` already
  does, and duplicating it here would put two harnesses on the same job.
- **No `core`/`core.*` in `CLEANFILES`.** Item 4's RED deck will dump one in
  the build directory. Left to the crew that produces it, since `core*` in a
  `CLEANFILES` is a wide glob and should be added by whoever needs it.

## For the owner

Three things the next crews need from the driver's contract:

1. **`.status` may not hold a value `>= 128`.** The driver refuses it as a
   broken spec, and the refusal is deliberate: the signal check owns that
   range and reports it by name. So item 4's RED deck asserts the status it
   *should* return — `1` — and today fails **twice**, `signal` then `status`,
   which is exactly the evidence 0072 criterion 1 wants. The verbatim output is
   in section 4 above; it need not be re-derived.
2. **`.mustfail` requires set equality, not containment.** A self-test deck
   that dies on a signal must list `signal status`, not just `status`, because
   both checks fire. Only `selftest-status.cir` uses it today.
3. **The pre-run delete makes a bare `absent` line weaker than it looks.** It
   proves the run did not create the file — which is what 0069 criterion 3
   wants — and it proves nothing about what was there before. Section 3b is the
   corruption that shows the delete working; a crew that doubts it can re-run
   the same two lines.

Nothing was pushed. One commit on `ver_50`.
