# Receipt: item 1 — the driver learns `<base>.invoke`

PLAN item **1 — The driver: `<base>.invoke`**, from
`doc/claude/batches/2026-08-15-harness-invocations/PLAN.md`.

Crew 01, 2026-08-15, branch `ver_50`. HEAD at start `54f3ac942`.

**Code was touched**, but no file under `src/` was: one shell driver
(`tests/bin/check_status.sh`), one new self-test deck with four sidecars, and
the directory's `Makefile.am`. Nothing under `doc/claude/upstream/` and nothing
under `doc/claude/batches/2026-08-15-exit-status-testing/upstream-0072/` was
opened, read or written, and nothing here states or implies that any upstream
material has been sent.

Binary: `build-ver_50/src/ngspice`, `ngspice-46+`. The build stamp moved three
times during the item, because `./autogen.sh` makes `config.status` regenerate
`src/include/ngspice/config.h` and the binary relinks:

| stamp | what was measured on it |
| --- | --- |
| `Sun Aug 16 00:53:25 UTC 2026` | the route probes, the discriminator table |
| `Sun Aug 16 02:51:52 UTC 2026` | the 332-pass run, the re-measured 331 baseline |
| `Sun Aug 16 03:01:15 UTC 2026` | the final targeted and full `make check` |

Every route measurement was repeated on the last stamp and gave the same
answer. Nothing in this item depends on which stamp a number came from.

Scratch: `/tmp/claude-1000/-home-qflow-dev-ngspice-test/535437bd-d68e-4ce6-af8c-934f0a7a07c3/scratchpad`.
Nothing was left in the repository or in
`build-ver_50/tests/regression/exitstatus/` beyond the generated `Makefile` and
the artefacts `CLEANFILES` now names.

### What the tree needed after `./autogen.sh`

Recorded because the item asked. `./autogen.sh` from the repo root, then
**nothing else by hand**: `make -C build-ver_50/tests/regression/exitstatus`
re-runs `automake-1.16 --gnu tests/regression/exitstatus/Makefile` and then
`./config.status tests/regression/exitstatus/Makefile` from its own maintainer
rules. `config.status` also rewrites `src/include/ngspice/config.h`, so
`make -C build-ver_50 -j8` relinks the binary (1295 compile lines the first
time, because `config.h`'s timestamp moved, not its content). Running
`make -C build-ver_50 -j8` before `make check` is therefore the safe order and
is what was done.

## The RED failure, verbatim

The deck and its four sidecars were written and registered **first**, and run
through the real harness against the **unmodified** driver. The driver ignored
`selftest-invoke.invoke`, ran the default `--batch` line, and both of the
sidecar's witnesses collapsed — so `.mustfail`'s set equality reported the
extra failure and the entry failed:

```
check_status: selftest-invoke: self-test.  The failure reported below is
  the one this deck exists to produce; the verdict is inverted.
check_status: selftest-invoke: FAILED (status)
  expected exit status 3, got 0
  stderr was empty
  grep selftest-invoke.evidence 'batchmode=0': not found; the file holds:
      batchmode=1
  present selftest-invoke.raw: the run did not create it
check_status: selftest-invoke: FAILED (files)
check_status: selftest-invoke: SELF-TEST FAILED
  ../../../../tests/regression/exitstatus/selftest-invoke.mustfail requires these checks to fail: status 
  the checks that actually failed:               files status 
  The captured output is in selftest-invoke.stdout and selftest-invoke.stderr.
FAIL: selftest-invoke.cir
===========================================================
1 of 10 tests failed
```

`batchmode=1` and "the run did not create it" are the two halves of the
sidecar being ignored, printed by the driver itself. That is the RED.

### `selftest-status.cir` still fails when it should

Re-run after the change, unmodified, no `.invoke` of its own — the pre-existing
self-test still catches a driver that cannot report a status mismatch, and it
now prints the command line as well:

```
check_status: selftest-status: self-test.  The failure reported below is
  the one this deck exists to produce; the verdict is inverted.
check_status: selftest-status: FAILED (status)
  expected exit status 3, got 0
  invocation: ../../../src/ngspice --batch -n ../../../../tests/regression/exitstatus/selftest-status.cir
  stderr was empty
check_status: selftest-status: self-test passed; the driver reported: status 
PASS: selftest-status.cir
```

That `invocation:` line is also the proof of the "absent `.invoke` == today's
behaviour exactly" requirement, read off the driver rather than off the diff.

### The new mechanism, corrupted three ways

`selftest-invoke.invoke` was edited in place and restored (`cmp -s` against a
saved copy after each). All three corruptions move the result from pass to
`SELF-TEST FAILED`.

**A. the `route notty` line deleted** — falls back to `batch`:

```
check_status: selftest-invoke: FAILED (status)
  expected exit status 3, got 0
  invocation: ../../../src/ngspice --batch -n -r selftest-invoke.raw ../../../../tests/regression/exitstatus/selftest-invoke.cir
  stderr was empty
  grep selftest-invoke.evidence 'batchmode=0': not found; the file holds:
      batchmode=1
check_status: selftest-invoke: FAILED (files)
  invocation: ../../../src/ngspice --batch -n -r selftest-invoke.raw ../../../../tests/regression/exitstatus/selftest-invoke.cir
check_status: selftest-invoke: SELF-TEST FAILED
  the checks that actually failed:               files status 
rc=1
```

**B. the `flags` line deleted** — no `-r`, no rawfile. This is also item 1's
hard witness that `flags` reaches the command line:

```
check_status: selftest-invoke: FAILED (status)
  expected exit status 3, got 0
  invocation: ../../../src/ngspice -n ../../../../tests/regression/exitstatus/selftest-invoke.cir < /dev/null
  stderr was empty
  present selftest-invoke.raw: the run did not create it
check_status: selftest-invoke: FAILED (files)
check_status: selftest-invoke: SELF-TEST FAILED
  the checks that actually failed:               files status 
rc=1
```

**C. `route notty` changed to `route stdin`** — the rawfile still appears, so
`present` passes and only the route witness moves. This is the cleanest single
proof that the three routes are three different command lines and not one:

```
  invocation: ../../../src/ngspice -b -n -r selftest-invoke.raw < ../../../../tests/regression/exitstatus/selftest-invoke.cir
  grep selftest-invoke.evidence 'batchmode=0': not found; the file holds:
      batchmode=1
check_status: selftest-invoke: FAILED (files)
check_status: selftest-invoke: SELF-TEST FAILED
  the checks that actually failed:               files status 
rc=1
```

### The `spec_error()` cases, by hand

They cannot be committed as `TESTS`: they exit 1 and are deliberately not
invertible. Each was produced by rewriting `selftest-invoke.invoke` — **a deck
that has a `.mustfail`** — and running the driver with the same command line
`TESTS_ENVIRONMENT` builds. Every one exits 1 despite the `.mustfail`, which is
the rule the plan says must keep holding, measured rather than assumed. The
`check_status:` line is the whole of the driver's output in each case; the run
never happens.

```
1. unknown key            route notty / rawfile selftest-invoke.raw
check_status: selftest-invoke: broken test spec: .../selftest-invoke.invoke: unknown key 'rawfile'; the keys are route, flags
rc=1

2. unknown route          route pipe
check_status: selftest-invoke: broken test spec: .../selftest-invoke.invoke: unknown route 'pipe'; the routes are batch, stdin, notty
rc=1

3. repeated route         route notty / route batch
check_status: selftest-invoke: broken test spec: .../selftest-invoke.invoke: a second 'route' line; at most one of each key
rc=1

4. repeated flags         flags -r a.raw / flags -r b.raw
check_status: selftest-invoke: broken test spec: .../selftest-invoke.invoke: a second 'flags' line; at most one of each key
rc=1

5. flags with no words    route notty / flags
check_status: selftest-invoke: broken test spec: .../selftest-invoke.invoke: 'flags' with no words
rc=1

6. shell metacharacter    flags -r `id>pwned`
check_status: selftest-invoke: broken test spec: .../selftest-invoke.invoke: flags word '`id>pwned`' holds a character this driver will not put on a command line.  A flags word may hold only letters, digits and . _ / = + , : -
rc=1

7. absolute path          flags -r /tmp/pwned.raw
check_status: selftest-invoke: broken test spec: .../selftest-invoke.invoke: flags word '/tmp/pwned.raw' is an absolute path or reaches outside the build directory
rc=1

8. '..' component         flags -r ../pwned.raw
check_status: selftest-invoke: broken test spec: .../selftest-invoke.invoke: flags word '../pwned.raw' is an absolute path or reaches outside the build directory
rc=1

9. route with two words   route notty batch
check_status: selftest-invoke: broken test spec: .../selftest-invoke.invoke: 'route' takes exactly one word, got 'notty batch'
rc=1

10. a glob                flags -r *
check_status: selftest-invoke: broken test spec: .../selftest-invoke.invoke: flags word '*' holds a character this driver will not put on a command line.  A flags word may hold only letters, digits and . _ / = + , : -
rc=1
```

Case 6 and case 10 are the two that matter. `id>pwned` never ran and no
`pwned` file exists anywhere — checked in `/tmp`, in
`build-ver_50/tests/regression/` and in the deck's own build directory. Case 10
is only refused because the parse runs under `set -f`: without it the shell
would expand `*` to the build directory's contents *before* the check, and
every one of those words would then pass.

## The production change

Three files.

### `tests/bin/check_status.sh`

The header block gains a `<base>.invoke` entry beside the other four sidecars,
naming the two keys, the three command lines, the default, the four ways to
break the spec, and the character set a `flags` word is restricted to. One
sentence is added to the capture paragraph, saying that every FAILED report now
carries the command line.

The body gains one parse block, one dispatch, and four one-line prints:

- **Parse**, placed with the other spec validation, so a broken `.invoke`
  leaves through `spec_error()` before anything is deleted or run. Defaults are
  `route=batch` and empty `flags`, which is the old fixed command line
  verbatim, so a deck with no `.invoke` is untouched by any of it. `#` is
  stripped to end of line, including after a key (`route notty  # comment`
  works, measured). The read loop runs under `set -f`, and its condition is
  `while read -r key rest || [ -n "$key" ]` so a last line with no newline
  after it is not silently dropped — a dropped `route` line is exactly the
  invisible fallback this feature is supposed to prevent. The existing `.files`
  loop does **not** do this; see *For the owner*.
- **Word validation** is an allow-list, `[A-Za-z0-9._/=+,:-]`, not a list of
  metacharacters to refuse. These words go onto a command line unquoted, so the
  answerable question is which characters are known inert, not which are known
  dangerous. Quote, backtick, `$ ; & | < > ( ) { } ! ~ #`, whitespace and every
  glob character fall outside by construction. Then the same absolute-path and
  `..` refusal the `.files` loop applies at `:219`-`:222`, for the same reason.
- **Dispatch**, one `case` for the string printed on failure and one for the
  run itself, built from the same words:

  ```sh
  case $route in
      batch) $SPICE --batch -n $flags "$deck" > "$out" 2> "$err" ;;
      stdin) $SPICE -b -n $flags > "$out" 2> "$err" < "$deck" ;;
      notty) $SPICE -n $flags "$deck" > "$out" 2> "$err" < /dev/null ;;
  esac
  rc=$?
  ```

  `$flags` is unquoted, so it word-splits like `$SPICE` already does and an
  empty one contributes no argument. `rc=$?` after a `case` is the status of
  the branch's last command, which is the simulator's.
- **`invocation:`** is printed by all four failure paths — `signal`, `status`,
  `err` and `files` — not only by the two that print a status. The rule is
  simple enough to keep: every `FAILED (...)` block ends with the command line.

The driver still never executes a string a deck supplied. `flags` words are
arguments, never a command, never a template, never anything `eval` sees; the
only reason they are validated at all is that they are expanded unquoted.

### `tests/regression/exitstatus/selftest-invoke.{cir,invoke,status,mustfail,files}`

The second committed self-test. `.invoke` declares both keys:

```
route notty
flags -r selftest-invoke.raw
```

`.status` says `3` and the deck exits `0`, `.mustfail` says `status`, and
`.files` asserts one artefact per key: `grep selftest-invoke.evidence
batchmode=0` and `present selftest-invoke.raw`. Because `.mustfail` is **set
equality**, the `.files` check must *pass*, so the deck asserts that the
sidecar was honoured at the same time as it asserts that the driver can still
fail. The deck's `.control` block is a single line, `echo "batchmode=$?batchmode"
> selftest-invoke.evidence`.

### `tests/regression/exitstatus/Makefile.am`

`selftest-invoke.cir` into `TESTS`; its four sidecars into `EXTRA_DIST`; its two
captures, two filter scratch files, `selftest-invoke.raw` and
`selftest-invoke.evidence` into `CLEANFILES`. The header comment block gains a
paragraph describing `.invoke` beside the other four sidecars, with the
vocabulary and the `spec_error()` rule, and the `CLEANFILES` comment gains a
sentence naming the two artefacts a passing run now leaves. Then `./autogen.sh`
from the repo root.

`tests/.gitignore` ignores `*.test`, `*.log`, `*.out`, `*.vcd`; nothing there or
in the root `.gitignore` touches `.cir`, `.invoke`, `.status`, `.files` or
`.mustfail`. No `git add -f` was needed.

`tests/lint/identity.baseline` is **unchanged** — this item adds no C. Both
lint specs report `baseline matches`, 264 and 11 comparisons.

## The two `make check` counts

| run | result |
| --- | --- |
| `make -C build-ver_50/tests/regression/exitstatus check` | **10 PASS / 0 FAIL** — "All 10 tests passed" |
| `make check` from `build-ver_50` | **332 PASS / 0 FAIL**, `make` exit status 0 |

The baseline was **re-measured, not carried**: the two modified files were
reverted with `git checkout --`, the five new files moved out of the tree, the
directory `Makefile` regenerated, and the full suite run again — **331 PASS /
0 FAIL**, `make` exit status 0, zero lines mentioning `selftest-invoke`. Then
everything was restored and `./autogen.sh` re-run. So the one new deck is the
whole of the difference and no existing test moved. A full `make check` on this
machine takes 1 minute 9 seconds, which is why re-measuring was cheap enough to
be worth doing rather than citing.

## What was measured

### The build's own route matrix

One probe deck, `.op` plus a one-line `.control` block writing
`batchmode=$?batchmode inputdir=$inputdir` to a file, run six ways by hand:

| command line | rc | `$?batchmode` | `$inputdir` | rawfile |
| --- | --- | --- | --- | --- |
| `ngspice --batch -n D` | 0 | 1 | *deck's directory* | none |
| `ngspice -b -n < D` | 0 | 1 | `.` | none |
| `ngspice -n D < /dev/null` | 0 | 0 | *deck's directory* | none |
| `ngspice --batch -n -r F D` | 0 | 1 | *deck's directory* | 255 bytes |
| `ngspice -b -n -r F < D` | 0 | 1 | `.` | 255 bytes |
| `ngspice -n -r F D < /dev/null` | 0 | 0 | *deck's directory* | 255 bytes |

The pair `(batchmode, inputdir)` separates all three routes; `-r` separates
`flags` from no `flags`. Sources: `batchmode` is set in exactly one place,
`main.c:1032`, from `-b`/`--batch` — `grep '"batchmode"' src/` returns that one
line — and `--batch` is the long form of `-b` (`main.c:959`), so `route batch`
and `route stdin` are indistinguishable on it. `inputdir` is set in exactly one
place, `inp.c:591`, and only `if (fp)`, which is why the stdin route leaves the
default `.`. `notty` reaches batch mode at `main.c:1175`-`:1177`,
`if ((!iflag && !istty) || ft_servermode) ft_batchmode = TRUE;`, which sets the
C flag and no cp variable — that is the whole of the difference.

### Per-route discriminator, as asked

| route | discriminator | observable? | where it is used |
| --- | --- | --- | --- |
| `batch` | `$?batchmode` = 1 **and** `$inputdir` = deck's directory | artefact only | corruption A |
| `stdin` | `$?batchmode` = 1 **and** `$inputdir` = `.` | artefact only | corruption C |
| `notty` | `$?batchmode` = 0 | artefact only | **committed**, `selftest-invoke.files` |
| `flags` | `-r F` writes `F`; without it no rawfile exists | artefact | **committed**, `selftest-invoke.files` |

### The full `make check` counts and the identity lint

Reported above. `git status --porcelain tests/lint/` is empty.

## What I falsified

### 1. No route has a `stdout` or `stderr` discriminator. Not one.

PLAN item 1 says to find "a byte-level difference between running some deck the
declared way and the default way, on `stdout`, `stderr`, or an artefact". Two
of those three are empty sets, measured:

`selftest-invoke.cir` was run all three ways with stdout and stderr captured
and diffed. **stderr is byte-identical across all three routes** — `diff`
produces no output at all. **stdout differs only in these lines:**

```
73c73
< Total analysis time (seconds) = 8.633E-05
> Total analysis time (seconds) = 7.0755E-05
75c75
< Total elapsed time (seconds) = 0.004 
> Total elapsed time (seconds) = 0.003 
78,80c78,80
< DRAM currently available =  942.371 MB.
> DRAM currently available =  942.738 MB.
...
```

— which is the resource-usage block, the same noise two consecutive runs of the
*same* route produce, and which this driver's own `FILTER` already strips. The
same was measured on `op-empty-netlist.cir`, the six-byte reproducer: rc=1 on
all three routes, and stderr byte-identical, all three routes:

```
Error: incomplete or empty netlist
       or no node to report an operating point for;
no operating point printed!
```

So every discriminator that exists is an **artefact a deck must deliberately
write**, via a `.control` block that exports a simulator variable. That is what
`selftest-invoke.cir` does and why it needs a `.control` block at all. A crew
that goes looking for a route difference in a diagnostic will not find one.

This also answers a question PLAN item 2 raises in advance — *"If the
diagnostic or the capture differs between routes … measure it before assuming
the three `.err` files are identical."* Measured: for that deck's content they
are identical. Item 2 can write one `.err` and use it three times.

### 2. The `.files` link for a flags-written file is not enforced, and the plan's reason for enforcing it does not hold.

PLAN item 1: *"Any file the flags cause the run to write must also be named in
`<base>.files`, because that is what deletes it before the run; … enforcing it
is preferred if it costs a line."* Three measurements against that:

- **It cannot cost a line, because the driver cannot tell which words are
  filenames.** `-r foo.raw` and `-t xterm` (`main.c:969`, `--terminal`, takes a
  terminal type) are the same shape. Any rule of the form "a word that does not
  begin with `-` must appear in `.files`" is a heuristic that would reject
  `flags -t xterm` as a broken spec. A rule keyed on a list of the simulator's
  file-taking options — `-r`, `-o`, `-c`, `--soa-log` — is about twelve lines
  and rots the day that list changes.
- **Where it matters it is self-enforcing.** The consequence the plan names is
  a `present` or `grep` satisfied by a stale artefact. But a deck can only be
  exposed to that if it asserts on the file, and asserting on the file *is* a
  `.files` line. There is no reachable state in which a deck asserts on a
  flags-written file and has not named it.
- **Where it does not matter, `.files` is the wrong file anyway.** A deck that
  uses `flags -o run.log` and asserts nothing about `run.log` needs the name in
  `CLEANFILES`, so `make distclean` is quiet. Putting it in `.files` would only
  add a delete before a run that no assertion reads.

So it is **documented, not enforced**, and the driver header says so in one
sentence: *"A file the flags cause the run to write is cleaned by the
directory's CLEANFILES, and is deleted before the run only if some
`<base>.files` line also names it."* Both of this item's own flags-written
artefacts are in `CLEANFILES`, and one is in `.files` too because the deck
asserts on it.

### 3. "Split by the shell the way `$SPICE` itself already is" is not safe on its own.

PLAN item 1 describes what `flags` may contain as "Words, split by the shell the
way `$SPICE` itself already is", and separately asks for a metacharacter
refusal. Measured: splitting the way `$SPICE` is split also **globs**, and
globbing happens *before* any per-word check can run. `flags *` would silently
become the contents of the build directory and every resulting word would then
pass a metacharacter test cleanly. The parse therefore runs under `set -f`, and
demonstration 10 above is that measurement. This is a gap in the written rule,
not a disagreement with its intent.

Nothing else in item 1 was found to be wrong. The two line references it makes
were checked against `54f3ac942` and both are right: the fixed command line was
at `check_status.sh:235`, and the `.files` path refusal at `:219`-`:222`.

## What was left, deliberately

- **No deck asserting 0072 or 0069 behaviour.** Items 2 and 3 own those. The
  `stdin` route has no committed deck in this item at all; it is exercised only
  by corruption C above, which is a hand run. Item 2's `op-empty-stdin.cir` is
  the first committed user, and the discriminator it will need is `$inputdir`,
  recorded in the table above.
- **No load-back and no second-invocation concept**, per PLAN decision 3.
  Nothing in the driver runs the simulator more than once.
- **`tests/bin/check.sh` untouched.**
- **No `env` key, no `cwd` key, no `stdin <file>` key.** The three routes are
  what `main.c:1175` distinguishes; a fourth spelling of "run it differently"
  with no caller would be a guess, and the plan's decision 2 is that this
  vocabulary is closed.
- **`route` and `flags` are not made order-sensitive and not made
  mandatory-together.** `flags` alone means `batch` plus flags, which is the
  shape item 3 needs; that is measured (corruption A is exactly that file).
- **The `.files` reader was not given the same last-line-without-newline
  handling** the `.invoke` reader has. It is a pre-existing behaviour of a
  parser this item did not otherwise touch, and changing it would move an
  existing directory's behaviour outside the item's scope. Named below.

## For the owner

1. **`.mustfail` set equality is now doing real work, and it is the whole
   reason `selftest-invoke.cir` proves anything.** The deck's `.files` must
   *pass* for the entry to pass, so "the route was honoured" is asserted by
   the *absence* of `files` from the `.mustfail` set. A crew that later adds
   `files` to that file to quiet a failure would silently delete the
   assertion. The `.mustfail` file says so in a comment; there is nothing
   mechanical stopping it.
2. **`tests/bin/check_status.sh`'s `.files` reader drops a last line that has
   no newline after it.** `while read -r verb path rest ; do … done <
   "$files_spec"` — `read` sets the variables and then reports EOF, so the
   final line is never seen. The `.invoke` reader added here guards against it
   with `|| [ -n "$key" ]`; the `.files` reader was left alone because it is
   pre-existing and out of this item's scope. Every committed `.files` ends
   with a newline today, so nothing is failing. It is a one-token fix if you
   want it.
3. **A `flags` word is restricted to `[A-Za-z0-9._/=+,:-]`, which is stricter
   than "no shell metacharacters".** A future flag needing `%`, `@`, `[`, `]`
   or a space inside one word will be refused as a broken spec, with a message
   naming the set. That was the deliberate trade: the driver expands these
   words unquoted, so the allow-list is what makes "never executes a string a
   deck supplied" checkable by reading eight characters rather than by
   enumerating a shell's grammar. Widening it is a one-character edit and a
   re-run of demonstrations 6 and 10.
4. **Falsification 2 needs a ruling if you disagree.** The plan asked for the
   `flags`-writes-a-file ⇒ named-in-`.files` link to be enforced. It is
   documented instead, for the three measured reasons above. If you want it
   enforced anyway, the honest version is the twelve-line list of the
   simulator's file-taking options, not the one-line heuristic.

Nothing was pushed. One commit on `ver_50`, carrying the driver, the deck, the
sidecars, the `Makefile.am` and this receipt.
