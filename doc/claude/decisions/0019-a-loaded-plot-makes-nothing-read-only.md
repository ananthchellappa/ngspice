# Decision 0019 — a loaded plot's environment makes nothing read-only

## Status

Accepted, 2026-08-14, branch `ver_50`. Implements the write half of
`doc/codex/issues/0061`, whose second addendum carries the measurements; the
memory defect on the same arm is `doc/codex/issues/0067`. Reverses an upstream
behaviour that has been in `cp_usrset()` since the plot environment was
written, so it is recorded here rather than only in the issue.

## Context

A raw file's header may carry `Option: <name>=<value>` lines. `raw_read()`
parses them into the loaded plot's environment, `plot_cur->pl_env`
(`src/frontend/rawfile.c`), and that is the only code in the tree that puts
anything there — nothing a simulation does fills it. `cp_usrset()`
(`src/frontend/options.c`) then scanned that environment on every `set` and
every `unset`, and answered `US_READONLY` for any name it found:

```c
    if (plot_cur)
        for (tv = plot_cur->pl_env; tv; tv = tv->va_next)
            if (eq(tv->va_name, var->va_name))
                return (US_READONLY);
```

For thirty years that was nearly unreachable — no ngspice writer emitted an
`Option:` line unless it was round-tripping one it had read, so only a
hand-edited header could arm it. On 2026-08-14 `raw_write()` began recording
the identifier case mode in the files it writes
(`doc/codex/issues/0061`'s first addendum, `Option: casemode=<mode>`), which is
what the client integration asked for. From that day the arm was reachable
from a waveform a user loads:

> Written when that line was unconditional. Later the same day it went behind
> `casemodewrite`, default off (0061's third addendum), so the files that carry
> it are the ones whose writer asked for it rather than all of them. That
> narrows how often the arm is reached and changes nothing about the decision
> below, which is about what a key in a loaded plot's environment may do once
> it is there.

```
$ load new.raw ; set casemode=preserve
Error: casemode is a read-only variable.
```

and the next netlist read then parsed in the mode the *file* named, not the
one the session had just asked for. The only escape was to `setplot` off the
loaded plot.

---

## Decision 1 — a file the user loaded may not decide what the session may set

**Decided: the scan is deleted. A name that a loaded plot's environment
carries is an ordinary name for `set` and for `unset`.**

`doc/codex/issues/0061` had already settled the reading half of this: a plot's
environment describes a plot, and may not answer a question asked on behalf of
a file being turned into cards. The write half is the same principle from the
other side. The environment's contents arrive in a data file the user opened;
letting them narrow what the session may do gives a file authority over the
session that nothing else in the variable system gives it.

The rule names no variable, exactly as the read half names none. `casemode` is
the name that made this urgent, and `ngbehavior`, `sourcepath` and any key a
header ever carries are the same defect; a predicate that knew the name
`casemode` would have had to learn the next one.

**What is not affected.** The pair is still parsed, still filed, still listed
by `set`, still answered through `$name`, and still re-emitted by
`raw_write()` — the header stays readable, which is the whole of what the
client asked a header for. `US_READONLY` still means what it meant for the two
names that really are computed and stored nowhere, `plots` and `curcasemode`.
And the read narrowing is untouched, which is what keeps
`doc/codex/issues/0061`'s criterion 1 true now that a `set` can reach the same
name: a loaded plot answers no read taken inside `inp_readall()` — and, since
that issue's fourth addendum, none taken through `cp_getvar_policy()` either,
which is the same rule for a policy read that happens somewhere else. Both are
read narrowings and neither touches what `set` may write, which is this
decision.

## Decision 2 — `unset` is narrowed with `set`, and removes the key

**Decided: the same treatment, and the key is removed from the plot's
environment rather than refused.**

`cp_usrset()` serves both `cp_vset()` and `cp_remvar()`, so one deletion
covers both — but they could have been split, and refusing only the `unset`
was the alternative. It was rejected. `Error: casemode is read-only.` on an
`unset` is the same sentence, from the same file, about the same session; a
user who may set a name and may not unset it has been given a stranger rule,
not a safer one. `set` lists the key, so `unset` removes it.

**The two values shadow, they do not merge.** A session value goes on the
global list, which `cp_getvar()` and `cp_enqvar()` both consult ahead of the
plot environment, so setting a name a loaded plot carries leaves both in
place and the session's wins. An `unset` therefore takes one layer at a time:

```
load f.raw             # Option: probe=fromfile
set probe=fromsession  ->  $probe = fromsession
unset probe            ->  $probe = fromfile
unset probe            ->  $?probe = 0
```

That is the behaviour asserted, not merely permitted, in
`tests/regression/pipe/rawfile-option-set-policy.cmd`.

## Consequences

- A user can override a header. That is the point, and it is what
  `doc/codex/issues/0061` had left as a nuisance and the case-mode header line
  turned into a defect.
- A user can also remove a key from a plot he loaded, and a `write` of that
  plot afterwards will not carry it. ~~`raw_write()`'s own `casemode` line is
  written afresh from the session and is not affected.~~ — **amended
  2026-08-14** by `doc/codex/issues/0061`'s fourth addendum: `raw_write()`
  writes its own line only onto a plot that carries no environment, because the
  mode in force describes a plot this session produced and says nothing about
  one that came out of a file. So the sentence holds exactly when the `unset`
  removed the plot's last key — the shape
  `tests/regression/pipe/unset-rawfile-option-key.cmd` measures, where the
  re-written file does record the mode again — and not when other keys remain,
  in which case the copy is described by the file's own pair and by nothing
  this session adds.
- One upstream diagnostic pair stops appearing:
  `Error: sourcepath is a read-only variable.` followed by
  `cp_vset: Internal Error: it was already there too!!`, which the reader
  provoked on itself whenever it extended `sourcepath` while a plot carrying
  that key was current. Measured on `/usr/local/bin/ngspice` (`ngspice-46`)
  and on this build in `doc/codex/issues/0061`'s second addendum: two lines
  there, none here, and the extension now takes.
- Nothing announces the shadowing when it begins. `set` shows both values,
  under the two headings it has always shown them under.
