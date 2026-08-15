# Why the raw-header default cannot flip yet: `cp_remvar()` and the release cycle

A tutorial report on one sentence from the round-2 status summary:

> because a released ngspice that reads one of our files and then clears that
> setting crashes — we fixed that crash here, but nobody else has our fix. So
> the default can't flip until our crash fix has been in a public release,
> which we don't control.

Everything below was measured 2026-08-15 against three binaries: stock
`/usr/local/bin/ngspice` (`ngspice-46`), upstream `master` at `2f9c8ad47`
("Update the news for ngspice-47") built here, and this branch's
`build-ver_50/src/ngspice`.

---

## 1. What we shipped, and the one string that makes it dangerous

`doc/codex/issues/0061` added a line to the raw-file header:

```
Title: * my circuit
Date: Sat Aug 15 00:10:39  2026
Plotname: Operating Point
Option: casemode=preserve          <-- this line
Flags: real
```

The purpose is simple. A raw file records signal names, and under `preserve`
those names keep their capitals — `v(MidNode)` rather than `v(midnode)`. A
program reading the file later has no way to know which convention produced
those names, because that is a property of the *run*, not of the reader's
binary. Without the line, every consumer has to launch a throwaway simulation
just to find out. With it, the file describes itself.

We deliberately chose the key `Option:` rather than inventing `Casemode:`. That
choice came from the client's own measurement: ngspice's raw-header parser is a
chain of `else if` tests over known prefixes, ending in an `else` that declares
any unrecognised line "strange" and aborts the load. A new key would break
every existing reader. `Option:` is a key every ngspice already parses.

And that is exactly what makes the line dangerous. An `Option:` line is not
inert metadata. The reader **files it into the loaded plot's environment** as a
live control variable:

```c
/* src/frontend/rawfile.c:562-570, on reading an "Option:" line */
wl = cp_lexer(s);
...
curpl->pl_env = cp_setparse(wl);
```

So a file written by us, opened by any ngspice, puts a variable named
`casemode` into that session's variable space. Nothing is wrong yet. The
trouble starts when the user removes it.

## 2. The bug: `cp_remvar()` frees a node it chose not to unlink

`unset <name>` reaches `cp_remvar()` in `src/frontend/variable.c`. The function
does three things in order.

**First, it searches four lists** for the name, stopping in whichever one holds
it, and keeps a pointer to the *slot* so the node can be unlinked later:

```c
for (p = &variables;            *p; p = &(*p)->va_next)  /* global set vars */
for (p = &uv1;                  *p; p = &(*p)->va_next)  /* computed vars   */
for (p = &plot_cur->pl_env;     *p; p = &(*p)->va_next)  /* the plot's env  */
for (p = &ft_curckt->ci_vars;   *p; p = &(*p)->va_next)  /* circuit options */
```

The third list is the one a raw-file `Option:` line writes into.

**Second, it asks `cp_usrset()` what kind of variable this is** and runs one of
five arms. The arms do genuinely different things:

| arm | what it does with the node |
| --- | --- |
| `US_OK` | unlinks it — the list gives it up |
| `US_DONTRECORD` | nothing; the node stays linked |
| `US_READONLY` | refuses; the node stays linked |
| `US_SIMVAR` | unlinks the *circuit's* copy, which may be a different node |
| `US_NOSIMVAR` | error message only |

**Third — and this is the defect — it frees the node unconditionally:**

```c
    v->va_next = NULL;
    free_struct_variable(v);
```

Three of the five arms deliberately left that node linked into a list. Freeing
it anyway does two things at once: it truncates the list at a node that is
still referenced, and it leaves a dangling pointer where a live variable used
to be. The next walk of that list reads freed memory, or frees it a second
time. Under glibc the second free trips the allocator's own consistency check
and calls `abort()`; the dangling read is an ordinary segfault.

The bug is one line of *missing* condition, and it is old. It is present in
released `ngspice-46` and in current upstream `master`.

## 3. Six ways to reach it, none of them exotic

Each line below is a complete session. `-p` is pipe mode, `-n` skips
`.spiceinit`. Measured on stock `ngspice-46` and on unpatched upstream
`master` (`2f9c8ad47`) — identical results on both:

| # | session | rc | signal |
| --- | --- | --- | --- |
| 1 | `set curplotdate=hello` | 134 | `SIGABRT` |
| 2 | `unset curplotdate` | 134 | `SIGABRT` |
| 3 | `unset curplot` | 134 | `SIGABRT` |
| 4 | `unset plots` | 134 | `SIGABRT` |
| 5 | `source deck.cir` / `set temp=27` / `unset temp` | 134 | `SIGABRT` |
| 6 | `load f.raw` / `unset <any Option key>` / `display` | 139 | `SIGSEGV` |

Numbers 1 and 2 are a second, independent defect that travels with the first
and is fixed alongside it: the `curplotdate` arm of `cp_usrset()` calls
`FREE(plot_cur->pl_date)`, and before any analysis has run the current plot is
the built-in `constants` plot whose date field is the static string
`Spice_Build_Date`. The two neighbouring arms — `curplotname`, `curplottitle` —
have always guarded against exactly this; this one does not. Handing a string
constant to `free()` aborts.

Number 5 is the shape that matters for tool authors: a generated `.control`
block that sets a simulator variable and unsets it afterwards. The xschem
integration hit it independently, on both binaries, and adopted a client-side
rule — never emit a `set` and an `unset` of the same simulator variable — as a
mitigation.

**Number 6 is the one that governs the header default.** Note what it is *not*
about: the key does not have to be `casemode`. Any `Option:` key in any raw
file will do:

```
$ printf 'load f.raw\nunset myopt\ndisplay\nquit 0\n' | ngspice -p -n
  stock ngspice-46          rc=139 (segfault, core dumped)
  upstream master 2f9c8ad47 rc=139 (segfault, core dumped)
```

*A measurement note, because it changed the conclusion.* The first version of
this test injected an `Option:` line into a **binary** raw file with `awk`.
Every variant returned rc=0, which looked like evidence that the crash was
specific to our `casemode` key. It was not evidence of anything: `awk`
line-processes the binary data section too, so the file was corrupt, the `load`
failed, no plot existed, and there was nothing to crash on. Re-run against
ASCII raw files, all three variants — a generic name, a spaced value, an
unspaced value — segfault. The crash has nothing to do with casemode.

## 4. Why this blocks the default, specifically

Put the two halves together.

Our build writes `Option: casemode=<mode>` into raw files. Any ngspice that
loads such a file gets a `casemode` variable in its plot environment. If that
session then unsets it — or unsets anything else the file carried — released
ngspice dies. Not with a diagnostic: `SIGSEGV`, core dumped, on the *next*
command of any kind.

So a raw file written by this build is a latent crash trigger for every
released ngspice in the field. That is an unusual thing to ship. A data file
should not be able to kill the program that opens it, and "only if the user
unsets a variable" is not much of a defence when the variable arrived without
the user asking for it.

This is why the line is **opt-in**, behind `set casemodewrite`, default off.
With it unset, the header this build writes is byte-for-byte the header every
ngspice has ever written. Nobody's files become anybody's crash unless they
asked.

The condition for flipping the default is therefore not "is our fix good" — it
is **"has the fix reached the binaries that will read our files"**. Those are
other people's installed ngspice releases. Fixing `cp_remvar()` on this branch
protects exactly one binary: ours. Every other ngspice on every other machine
still has the old `cp_remvar()` and still dies on our files.

That is the whole content of the sentence this report is about. The gap is not
technical debt and there is no work item that closes it here. It closes when a
public ngspice release contains the fix, and releases are the upstream
maintainers' to make.

## 5. The fix

The rule is ownership, not a list of arms: **free the node only when nothing
points at it any more.** Three conditions make that true — the function
manufactured the node itself, the arm unlinked it, or the arm intended the tail
to dispose of it. A `bool v_free` carries the answer:

```c
    bool v_free = FALSE;    /* TRUE when v is the tail's to free */
...
    if (!v) {
        v = var_alloc_num(copy(varname), 0, NULL);
        v_free = TRUE;      /* nothing but this call ever saw it */
    }
...
    case US_OK:
        if (vp) {
            *vp = v->va_next;
            v_free = TRUE;
        }
        break;
...
    if (v_free) {
        v->va_next = NULL;
        free_struct_variable(v);
    }
```

A second variable, `vp`, saves the slot the search stopped in. The `US_SIMVAR`
arm reuses `p` to walk the circuit's list, so without `vp` the function would
later unlink through the wrong list — a bug the unconditional free was masking.

Two arms also stop lying to the user. `US_DONTRECORD` used to print
`cp_remvar: Internal Error: var %d` on the ordinary path, rendering the first
byte of the name as an integer; it now says `Error: <name> cannot be unset.`

## 6. What is actually being sent upstream, and what is not

The upstream submission is deliberately narrower than our commit.

**Sent** — two patches, `doc/claude/upstream/`:

- `0001-cp_remvar-ownership.patch` — `src/frontend/variable.c`, the ownership
  rule. Fixes repros 2–6.
- `0002-curplotdate-static.patch` — `src/frontend/options.c`, the
  `Spice_Build_Date` guard. Fixes repro 1.

**Not sent** — the deletion of the `pl_env` read-only scan in `cp_usrset()`.
That is our own policy decision (`doc/claude/decisions/0019`): we hold that a
file the user merely opened may not decide what the session is allowed to set.
It is a behaviour change, it is tied to the casemode feature, and it is not
needed to fix any crash.

That last claim is measured rather than assumed, because it decides what may
honestly be sent. A build was made with the ownership rule applied and the
read-only scan **restored**, and repro 6 returns rc=0 there. The crash fix
stands on its own; with the scan intact, `unset` on a loaded key is refused
with `Error: <name> is read-only.` instead of crashing, which is upstream's
existing intent, correctly executed for the first time.

On that same build, three of our branch's 29 pipe-suite decks fail — the three
that encode the policy decision, not the crash fix. All three
`unset-*-name.cmd` crash guards pass. That is the expected split and it is the
evidence that the two changes are independent.

## 7. Validation of the exact artifact

Both patches were applied to a clean worktree of upstream `master`
(`2f9c8ad47`), not to our branch:

| step | result |
| --- | --- |
| `git apply --check`, both patches | clean |
| `./autogen.sh && ../configure && make -j12` | exit 0, no new warnings in either file |
| all six repros | rc=0, every one |
| upstream's own `make check` | 58 tests, **0 failures**, exit 0, 42 directories |

Patch 1 applied to upstream unmodified. Patch 2 needed regenerating: our tree
had changed the surrounding `eq()` to `eqc()` as part of casemode work, so the
context did not match. The version in `doc/claude/upstream/` is the
upstream-context one, and it is the one validated above.

## 8. What to do while the gap is open

- Keep `casemodewrite` off by default. Revisit only when the fix is in a
  released ngspice.
- Fix `doc/codex/issues/0070` before the default flips. A file that carries the
  line must not carry a *wrong* mode, and today a plot loaded from a file that
  recorded nothing is stamped with the copying session's mode.
- Clients that need the mode today should read `$curcasemode` from a probe
  process rather than the header. It needs no file, and its absence on an older
  build is a clean negative rather than a crash.
