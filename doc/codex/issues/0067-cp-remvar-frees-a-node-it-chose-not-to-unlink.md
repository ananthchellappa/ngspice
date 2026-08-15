# Issue: `cp_remvar()` Frees a Node It Chose Not to Unlink

## Status

Closed 2026-08-14 by the ownership rule under **Resolution**: `cp_remvar()`
frees the node only when no list still points at it. All three arms, measured
before and after, with a deck each. Pre-existing and upstream; nothing in the
case-mode batch caused it, touched it, or is needed to reproduce it — what the
batch did was make the third arm reachable from any file this build writes,
which is why it was fixed here and not left upstream. Filed because
`doc/codex/issues/0065` closed with two loose ends in its **Left** section that
are the same defect seen from two lists, and because that issue's criterion 6
called `cp_remvar()` "correct with no change of its own" while its own Left
section recorded it aborting — a sentence since corrected, pointing here.

**One issue and not two.** Nor three: a third symptom was measured on
2026-08-13, through the `US_SIMVAR` arm. They differ only in which list still
points at the node, and therefore in whether the freed memory is noticed inside
the `unset` or on the command after it. The defect — the unconditional free at
`src/frontend/variable.c:671`-`:672` of a node that three of the five preceding
`switch` arms have already disposed of, two by deliberately leaving it linked
and one by freeing it themselves — is one line, one cause, and one fix. Filing
them apart would put the same three lines in three Resolutions and invite three
different repairs.

Measured 2026-08-13 on branch `ver_50`, against three binaries, so that "not
this batch" is a measurement and not a claim:

- **NEW** — `build-ver_50/src/ngspice`, `720c8743a` plus the batch's
  uncommitted work.
- **BASE** — the same tree with all twelve modified files under `src/` reverted
  to `720c8743a`, configured and built in a scratch copy outside the repo.
  This is the batch's own starting point.
- **STOCK** — `/usr/local/bin/ngspice`, `ngspice-46`, no `casemode` support.

**Confirmed independently 2026-08-14, by the client, on both binaries.** The
xschem session ran the `US_SIMVAR` sequence as **R6** of
`doc/claude/feedback/reply_from_xschem_session/REPLY.md` and got `rc=134`
(`SIGABRT`, core dumped) from `build-ver_50/src/ngspice` **and** from stock
`/usr/local/bin/ngspice`:

```
$ printf 'source deck.cir\nset temp=27\nunset temp\nquit 0\n' | ngspice -p -n
  ver_50            rc=134 (SIGABRT, core dumped)
  stock ngspice-46  rc=134 (SIGABRT, core dumped)
```

Re-run here from their `repro2/run_round2.sh` against the same two binaries:
`rc=134` from both, on a two-resistor divider. That matters more than an extra
transcript. `doc/claude/feedback/ngspice_upstream/RESPONSE.md` had flagged this
as the newest item in the batch and therefore the least scrutinised, so an
outside party reproducing it — on a released binary, from a deck they wrote,
without being handed this file's decks — is the scrutiny that flag asked for.
It confirms both halves of the Status paragraph below: pre-existing and
upstream, and not caused by anything in the case-mode batch.

They have adopted a client-side rule — never emit a `set` and an `unset` of the
same simulator variable into a generated `.control` block — which is a
mitigation and not a fix, and their generator did not do it before. That is
worth recording because it is the shape of a deck a *tool* writes, and this
issue's other two arms (`unset curplot`, `load` + `unset <key>`) are shapes a
tool writes too.

**Corrected 2026-08-13**, after this issue was first written, and the
correction is not cosmetic. The `US_READONLY` symptom was described here as
latent, allocator dependent and usually silent, on the strength of a
two-command sequence that returns `rc=0`. Re-measured: the ordinary
three-command sequence — `load`, `unset`, then anything that reads the plot
environment — is a `SIGSEGV`, `rc=139`, three runs of three, on NEW and on
STOCK. `doc/codex/issues/0065` had recorded exactly that and was overruled in
error. The Summary says how the error was made; the Impact and the acceptance
criteria below are the corrected text. A third arm, `US_SIMVAR`, was found
while re-measuring and is included.

## Summary

Two commands, no rawfile, no netlist, on a stock release:

```
$ printf 'unset curplot\nquit 0\n' | ngspice -p -n
cp_remvar: Internal Error: var 99
free(): double free detected in tcache 2
Aborted (core dumped)      rc=134
```

`unset plots` is the same. So is `unset` of a simulator option a user has set,
which needs a circuit and no rawfile at all — any deck that yields one; a
two-element divider was used:

```
$ printf 'source divider.cir\nset temp=27\nunset temp\nquit 0\n' | ngspice -p -n
it's a US_SIMVAR!
double free or corruption (fasttop)
Aborted (core dumped)      rc=134
```

The rawfile shape does not kill the `unset`. It kills the command after it:

```
$ printf 'load hdr_zzz.raw\nunset zzz\nset\nquit 0\n' | ngspice -p -n
Error: zzz is read-only.
cp_remvar: Internal Error: var 122
Segmentation fault (core dumped)      rc=139
```

`display` or `write w.raw` in place of `set` faults identically; `set` is
simply the shortest command that walks the plot's environment.

| command | BASE | NEW | STOCK |
|---|---|---|---|
| `unset curplot` | rc=134 | rc=134 | rc=134 |
| `unset plots` | rc=134 | rc=134 | rc=134 |
| `source` + `set temp=27` + `unset temp` | n/m | rc=134 | rc=134 |
| `load` + `unset zzz` + `set` | n/m | **rc=139** | **rc=139** |
| `load` + `unset zzz`, nothing after | rc=0 | rc=0 | rc=0 |

Every row is three identical runs. `n/m` is *not measured*, not *not
reproduced*: BASE was a scratch build outside the repo, made once to settle the
"not this batch" question and since deleted, and the two middle rows were added
on 2026-08-13 after it was gone. Nothing rests on it — STOCK is `ngspice-46` as
released, which settles "pre-existing and upstream" by itself, and BASE differs
from STOCK only by `doc/codex/issues/0065`'s fix, which makes this defect
quieter rather than louder.

The last two rows are one command apart, and they are the trap. **The `unset`
survives; the next thing the user types does not.** An earlier draft of this
issue measured only the last row, read its `rc=0` as the symptom's verdict, and
wrote the `US_READONLY` arm up as latent and usually silent. It is neither: it
is an immediate, deterministic `SIGSEGV` on the next command, and
`doc/codex/issues/0065`'s Impact was right to record `rc=139`.

**Why that was missed, which is worth writing down.** The three-command form
was run under `valgrind`, whose allocator does not return a freed block to
anyone, so the walk that faults under `glibc` merely reads what is still
sitting there:

```
$ printf 'load hdr_zzz.raw\nunset zzz\nset\nquit 0\n' | valgrind -q ngspice -p -n
Error: zzz is read-only.
cp_remvar: Internal Error: var 122
   ... 34 invalid reads (NEW), 65 (STOCK), and rc=0 in both
```

That `rc=0` is the tool's, not the program's. The identical command line
without `valgrind` is the `rc=139` above. It is also the command line the
earlier draft quoted — run one way and reported the other, which is the whole
of the error.

**`valgrind` masks all three arms, not just this one**, so this is a trap and
not a slip: `printf 'unset curplot\nquit 0\n' | valgrind -q ngspice -p -n` is
also `rc=0`, with 3 invalid frees and 4 invalid reads, where bare `glibc`
aborts it at `rc=134`. On this defect the exit status has to be taken with no
tool underneath, and `valgrind` used only to say what the memory did.

`hdr_zzz.raw` is any ASCII rawfile with `Option: zzz=HAHA` spliced in after
`Plotname:` — the one-line `awk` splice
`doc/claude/feedback/ngspice_upstream/repro/hdr_variants.sh` performs, with
`Option: zzz=HAHA` as the inserted line. The invalid-read count differs between
builds because `doc/codex/issues/0065`'s `usrvar_push()` stopped putting
borrowed nodes on the `cp_usrvars()` chain, which removes some of the reads —
it makes the same defect quieter and does not touch it.

## Impact

Every shape below kills the process, deterministically, on this build and on
`ngspice-46` as released. None of them is latent and none of them is
allocator-luck: three runs of each, same exit status every time. Two of the
three need no rawfile and no unusual input at all — they are things a user
types to tidy up a session. In a `--with-ngshared` build the crash is the host
application's rather than ngspice's, which is an inference from the link and
not a measurement: `src/frontend/variable.c` is compiled into both targets and
nothing in `cp_remvar()` is conditional, but no shared build was run against
this.

By arm:

- **`US_DONTRECORD`** (`:634`) — `curplot`, `curplotname`, `curplottitle`,
  `curplotdate`, and `plots` through its own `US_READONLY` arm. The node is
  found on the chain `cp_usrvars()` just built, so the free at `:672` and the
  free of the chain at `:674` hit the same block: an immediate, deterministic
  double free inside the `unset` itself, `rc=134`. `curplot` and `plots` are
  names ngspice documents and lists in `set`; a user who types `unset curplot`
  to get back to a clean state loses the session.
- **`US_READONLY`** (`:641`) — every key a rawfile's `Option:` line put in
  `plot_cur->pl_env`. The node is found in `pl_env`, which nothing frees at
  `:674`, so the free at `:672` leaves the plot holding freed memory. The
  `unset` itself returns, having printed only `Error: zzz is read-only.` and
  the bogus internal error, and then the **next** command that walks that
  environment dies: `set`, `display` and `write` all measured, `rc=139`, three
  runs each, on `build-ver_50/src/ngspice` and on `/usr/local/bin/ngspice`
  alike. Under `gdb` the fault is `#0 __strcmp_avx2`, reading the freed node's
  name while the list is being sorted for printing. The one-command delay is
  the only thing separating this arm from the one above; it is not a
  difference in severity, and it is what made the arm look latent for two
  drafts.
- **`US_SIMVAR`** (`:649`) — a simulator option the user has `set` in a session
  that has a circuit; `temp`, `gmin`, `trtol` and `abstol` each measured. This
  arm unlinks the node from `ft_curckt->ci_vars` and frees it *itself* at
  `:659`, and the tail then writes `v->va_next = NULL` into the freed block and
  frees it a second time. `rc=134`. It is the only one of the three for which
  `valgrind` reports an invalid **write**: `US_DONTRECORD` gives 3 invalid
  frees and 4 invalid reads, `US_READONLY` gives reads only, and this arm gives
  1 invalid write, 1 invalid free and 3 invalid reads.

Three arms, one tail, one defect. That is the argument for filing it once, and
the third arm is the argument for criterion 3's shape of fix: any repair
written as a list of arms would have been written with two of them in it.

## Root Cause

`cp_remvar()` (`src/frontend/variable.c:577`) finds the variable by walking
four lists in turn with a pointer-to-pointer, so that whichever list holds it
can be unlinked through `*p`:

```c
    uv1 = cp_usrvars();                             /* :583  a fresh chain */

    for (p = &variables; *p; p = &(*p)->va_next)    /* :585 */
        ...
    if (*p == NULL)
        for (p = &uv1; *p; p = &(*p)->va_next)      /* :592  chain, owned */
        ...
    if (*p == NULL && plot_cur)
        for (p = &plot_cur->pl_env; ...)            /* :600  plot, not ours */
        ...
    if (*p == NULL && ft_curckt)
        for (p = &ft_curckt->ci_vars; ...)          /* :608 */
        ...

    v = *p;                                         /* :615 */
```

Then it asks `cp_usrset()` what may be done with it and switches. `US_OK` is
the only arm that unlinks the node and leaves it for the tail to free, which is
the contract the tail is written to:

```c
    case US_OK:
        if (*p) {
            *p = v->va_next;                        /* :629  unlinked */
        }
        break;

    case US_DONTRECORD:
        if (*p)
            fprintf(cp_err, "cp_remvar: Internal Error: var %d\n", *varname);
        break;                                      /* :638  still linked */

    case US_READONLY:
        fprintf(cp_err, "Error: %s is read-only.\n", v->va_name);
        if (*p)
            fprintf(cp_err, "cp_remvar: Internal Error: var %d\n", *varname);
        break;                                      /* :646  still linked */
```

Both of those arms print a diagnostic *because* `*p` is non-NULL — that is,
they detect the situation exactly and then fall through to a tail that does not
know about it. A fourth arm gets there from the opposite direction, having
already freed the node:

```c
    case US_SIMVAR:
        ...
            if (*p) {
                struct variable *u = *p;
                *p = u->va_next;                    /* :658  unlinked */
                tfree(u);                           /* :659  and freed */
            }
        break;
```

and then the tail, which is reached by every arm:

```c
    v->va_next = NULL;                              /* :671 */
    free_struct_variable(v);                        /* :672 */

    free_struct_variable(uv1);                      /* :674 */
```

`:671` truncates whichever list `v` sits in, at `v`. `:672` frees `v` while
that list still points at it. `:674` then walks `uv1` from its head; if `v` was
on `uv1`, it arrives at the block freed one line earlier and frees it again.

For `US_SIMVAR` the sequence is the same two lines read the other way round:
`u` and `v` are the same node when the variable was found in
`ft_curckt->ci_vars`, so `:671` writes eight bytes into a block `:659` has
already handed back — `valgrind` calls it an invalid write of size 8, 24 bytes
into a block of 32 — and `:672` frees it twice. `US_OK` is the only arm whose
contract with the tail is coherent: it unlinks and does not free, and the tail
frees and does not unlink.

The `Internal Error: var %d` text is itself a fossil: `*varname` is the first
*character* of the name, so it prints `99` for `curplot` and `122` for `zzz`.
Whoever wrote it meant to print the name. It has been printing a letter's
ordinal for as long as anyone has been ignoring it.

**Why the diagnostic is wrong about itself.** `US_DONTRECORD` reaching
`cp_remvar()` with `*p` non-NULL is not an internal error at all: `cp_usrvars()`
manufactures those five names on every call, so `*p` is *always* non-NULL for
them by the time the switch runs. The message fires on the ordinary path. The
same goes for `US_READONLY` and `pl_env` — `cp_usrset()` returns it *because*
the name is in `pl_env`, which is where `p` found it.

## Acceptance Criteria

1. `unset curplot`, `unset curplotname`, `unset curplottitle`,
   `unset curplotdate` and `unset plots` each leave the process alive and the
   variable lists intact, on a session with no rawfile loaded. So does
   `unset temp` — or `gmin`, `trtol`, `abstol` — in a session that has sourced
   a deck and `set` the option first, which is the `US_SIMVAR` arm and is
   `rc=134` today.
2. `load` of a rawfile carrying `Option: <key>=<value>` followed by
   `unset <key>` leaves `plot_cur->pl_env` walkable. **The first half of this
   is an exit status, not a `valgrind` report**: `set`, `display` and `write`
   after the `unset` each return `rc=0` when run with no tool underneath, where
   today each is `rc=139`. The second half is that the same three are then
   clean under `valgrind` — no invalid read, no invalid write, no invalid free.
   Taking only the second half is how the crash was missed: `valgrind`'s
   allocator keeps the freed block readable, so it answers `rc=0` on the
   sequence that segfaults under `glibc`.
3. The fix is stated as a rule about ownership, not as a list of arms. The tail
   at `:671`-`:672` may free `v` only when `v` is both unlinked from every list
   and not already freed — the arms that decline to unlink must not reach it,
   and neither must `US_SIMVAR`, which frees the node itself at `:659`.
   `cp_vset()`'s switch (`:163`) has the same five arms and already guards its
   frees with `if (v_free)`; whatever shape is chosen here should be
   recognisable as the same idea. A repair that enumerates `US_DONTRECORD` and
   `US_READONLY` and stops satisfies criterion 2 and still aborts on
   criterion 1's second sentence.
4. What each arm *should* do about a user's `unset` of a read-only, dont-record
   or simulator-option name is decided explicitly, because it is a behaviour
   question and not a memory one. Today `unset curplot` prints a bogus internal
   error and then crashes; not crashing is necessary and is not obviously
   sufficient. `unset plots` refusing with `Error: plots is read-only.` and no
   internal error is the shape the `US_READONLY` arm already reaches for.
   `US_SIMVAR` additionally prints `it's a US_SIMVAR!` to `stderr`, which is
   debug output that reaches users on the ordinary path and should go with it.
5. The `Internal Error: var %d` text prints the name or goes away. It cannot
   stay as it is: it prints a character's ordinal, and it fires on the ordinary
   path rather than on an internal error.
6. A deck asserts it, and asserts the *exit status of the command after the
   `unset`* — not of the `unset`, which returns cleanly today and would pass a
   test written around it. `tests/regression/pipe/` is the shape: the sequence
   is `unset` and then something that touches the list, which is control
   language and not a netlist, and that directory is the one that can see an
   exit status. Three sequences, one per arm: `unset curplot` alone;
   `source` + `set temp` + `unset temp`; and `load` + `unset <key>` + `set`.
   `doc/codex/issues/0065`'s
   `tests/regression/pipe/rawfile-option-computed-name.cmd` is the nearest
   precedent and already builds the rawfiles this needs with `echo` and a
   redirect.
7. `make check` unchanged in all three modes.

## Resolution

Closed 2026-08-14 on branch `ver_50`.

**The rule, and it is a rule about ownership rather than a list of arms.**
`cp_remvar()` may free the node only when nothing points at it any more. It
owns the node in exactly three situations: it manufactured the node itself
because no list held the name; the arm that ran unlinked it; or the arm that
ran meant to dispose of it and left the freeing to the tail. Everything else
belongs to the list it was found in, and the tail must not touch it. That is
`cp_vset()`'s `if (v_free)` idea (`variable.c:171`, `:182`) read the other way
round, as criterion 3 asked.

Three production edits.

- **`src/frontend/variable.c`, `cp_remvar()`** (`:591`). A `bool v_free`,
  false until something makes the node ours, and a `struct variable **vp`
  holding the slot the search stopped in — saved because the `US_SIMVAR` arm
  walks a second list with `p` and the old code then unlinked through the
  wrong one. The tail is `if (v_free) { v->va_next = NULL;
  free_struct_variable(v); }` (`:702`). `US_OK` unlinks through `vp` and sets
  `v_free`. `US_DONTRECORD` and `US_READONLY` set neither: they decline to
  remove, so the node stays where it was and the chain `cp_usrvars()` built
  is freed once, at `:707`, as it always was. `US_SIMVAR` drops the circuit's
  copy if that is a *different* node — with `free_struct_variable()` rather
  than the bare `tfree()` it used, which leaked the name and the string —
  and then unlinks `v` through `vp` and lets the tail free it once. That last
  clause is why the arm needed rewriting rather than an early `break`: the
  node the search found is not always the node in `ft_curckt->ci_vars`. A
  variable set before a circuit was loaded lives in the global list and gets
  `US_SIMVAR` once one is, and the old arm unlinked the circuit's copy while
  the tail freed the global one out from under `variables`.
- **`src/frontend/options.c`, `cp_usrset()`'s `curplotdate` arm** (`:447`).
  Criterion 1's first sentence is about a session with nothing loaded, and
  there the current plot is the built-in `constants` one, whose three strings
  are the static initialisers of `constantplot`
  (`src/frontend/plotting/plotting.c:7`). The `curplotname` and `curplottitle`
  arms have always guarded their `FREE()` with a `/* not malloced! */` test
  and this one never did, so `unset curplotdate` — or a `set` of it — before
  any analysis handed `Spice_Build_Date` to `free()`: `free(): invalid size`,
  `rc=134`, a second and independent abort standing behind the one this issue
  is about. Guarded now by the pointer and not by the text, because the date
  is the one of the three whose value a session could reproduce by accident.
  This is not `cp_remvar()`'s defect and it is inside criterion 1's sentence,
  so it is fixed and named here rather than filed as a fourth thing.
- **`src/frontend/options.c`, the `plot_cur->pl_env` scan that answered
  `US_READONLY`** — deleted. That belongs to `doc/codex/issues/0061`, whose
  addendum records the decision and the measurement; it appears here because
  it removes the third and largest source of this arm. Every key a rawfile's
  `Option:` line filed used to arrive at `US_READONLY`; now such a key is an
  ordinary name and `unset` of it takes the `US_OK` path, which unlinks it
  from `pl_env` and frees it once — the one arm whose contract with the tail
  was always coherent. Either edit closes the reported `SIGSEGV` on its own;
  the four-way matrix under criterion 6 says which deck each one is needed
  for, and neither is redundant.

Five decks, all in `tests/regression/pipe/` — the one directory that can see
an exit status, which criterion 6 asks for — plus their `TESTS` and
`CLEANFILES` entries in that directory's `Makefile.am`:
`unset-computed-name.cmd`, `unset-readonly-name.cmd`, `unset-simvar-name.cmd`,
`unset-rawfile-option-key.cmd`, and `rawfile-option-set-policy.cmd`, the last
being `doc/codex/issues/0061`'s. No `.out` file was added or edited — this
directory has none. `tests/lint/identity.baseline` loses one line, the deleted
`pl_env` scan's `eq(tv->va_name, var->va_name)`; the lint reports 264 sites
where it reported 265.

**Criterion by criterion.**

1. *The five names, and a simulator option, leave the process alive.* Every
   row is three runs, taken with no tool underneath, on
   `build-ver_50/src/ngspice`. The RED column is the same tree with only
   `src/frontend/variable.c` and `src/frontend/options.c` reverted to
   `58496a8dc` and rebuilt, so it is this batch's binary in every other
   respect:

   | sequence | before | after |
   |---|---|---|
   | `unset curplot` | rc=134 | rc=0 |
   | `unset plots` | rc=134 | rc=0 |
   | `unset curplotdate` (fresh session) | rc=134 | rc=0 |
   | `source` + `set temp=27` + `unset temp` | rc=134 | rc=0 |
   | `load <raw this build wrote>` + `unset casemode` + `display` | rc=139 | rc=0 |

   The four computed names are refused and unchanged afterwards, on the
   constants plot and on a plot an analysis made; `temp`, `gmin`, `trtol` and
   `abstol` are removed, `$?name` answering 0 where it answered 1, and the
   circuit still runs.
2. *The rawfile shape, as an exit status first.* `display`, `set` and `write`
   after the `unset` are each `rc=0` where each was `rc=139`, and the loaded
   plot keeps its title, its current-plot identity and its vectors —
   `unset-rawfile-option-key.cmd` asserts all four. The second half, under
   `valgrind`, is now clean on all three arms: no invalid read, no invalid
   write, no invalid free, for `unset curplot`, for `unset plots` + `set`,
   for `source` + `set temp` + `unset temp`, and for `load` + `unset` +
   `display` + `set` + `write`. Both halves were taken, in that order, for
   the reason the Summary gives.
3. *Stated as a rule about ownership.* Above. The word `US_DONTRECORD` does
   not appear in the tail; what appears is `if (v_free)`.
4. *What each arm does about the user's `unset`, decided.* `US_DONTRECORD`
   prints `Error: <name> cannot be unset.` — the four `curplot*` names are
   computed on every read and nothing stores them, so there is nothing to
   remove and saying so is the whole of the arm. `US_READONLY` keeps its
   `Error: <name> is read-only.`, which now covers only `plots` and
   `curcasemode`, both computed. `US_SIMVAR` removes the option, and its
   `fprintf(stderr, "it's a US_SIMVAR!\n")` — debug output on the ordinary
   path — is gone. A key a loaded rawfile carried is removed rather than
   refused, which is `doc/codex/issues/0061`'s decision and not this one's.
5. *`Internal Error: var %d` prints the name or goes away.* Gone, from both
   arms. It was not an internal error: `cp_usrvars()` manufactures those five
   names on every call, so `*p` is always non-NULL for them by the time the
   switch runs, and `cp_usrset()` returned `US_READONLY` for a `pl_env` name
   *because* it was in `pl_env`, which is where `p` found it. It fired on the
   ordinary path, and it printed a character's ordinal. The two remaining
   `cp_remvar: Internal Error: US val %d` prints are untouched; the
   `US_NOSIMVAR` one is also reachable from an ordinary `unset gmin` in a
   session with no circuit, which is a separate wart and is left alone.
6. *A deck asserts it, and asserts the command after the `unset`.* Five, one
   per arm plus the rawfile shape plus 0061's. RED, each deck run against the
   reverted build described above:

   ```
   unset-computed-name.cmd        rc=134
   unset-readonly-name.cmd        rc=134
   unset-simvar-name.cmd          rc=134
   unset-rawfile-option-key.cmd   rc=139
   rawfile-option-set-policy.cmd  rc=1
     ERROR: a loaded rawfile made casemode read-only for the session
   ```

   GREEN, all five `rc=0`. Each deck also passes under `-D casemode=fold`,
   `-D casemode=preserve` and `-D casemode=distinguish`: they compare a value
   against one the same session computed, or against a mode they set
   themselves, so none of them keys on a spelling.

   **And each deck was measured against each half of the change on its own**,
   because two files moved and a deck that passes either way is guarding
   nothing. Four builds, the same five decks:

   | deck | neither | `variable.c` only | `options.c` only | both |
   |---|---|---|---|---|
   | `unset-computed-name` | 134 | **134** | 134 | 0 |
   | `unset-readonly-name` | 134 | 0 | 134 | 0 |
   | `unset-simvar-name` | 134 | 0 | 134 | 0 |
   | `unset-rawfile-option-key` | 139 | **1** | 0 | 0 |
   | `rawfile-option-set-policy` | 1 | 1 | 0 | 0 |

   The two bold cells are the ones worth reading. `unset-computed-name`
   survives the ownership rule and still aborts, because its first round runs
   on the constants plot and that is the `curplotdate` free — the deck holds
   the second edit as well as the first. `unset-rawfile-option-key` stops
   crashing under the ownership rule and then fails on its assertion,
   `ERROR: unset left the key in the loaded plot's environment`, which is the
   `US_READONLY` refusal still in place; it is a guard on the reported
   sequence and not the ownership rule's witness. The rule's witnesses are
   the three arm decks, and the `options.c`-only column is what says so: with
   `cp_remvar()` unrepaired they abort, whatever the plot environment does.
7. *`make check` unchanged.* Every cached `*.log`/`*.trs` deleted first. The
   recursion aborts at `tests/regression/case`, so the directories after it
   were run one at a time: 206 pass in the main recursion, then `casedist`
   29, `case-lt` 8, `case-pspice` 8, `xspice` 47, `lint` 2 — 300 pass. The
   `pipe` directory is 27 of 27, the four decks above and
   `rawfile-option-set-policy.cmd` among them. Two failures, and they are one
   deck in its two copies: `node-case-collision-report.cir`, which another
   crew was writing in this tree while this ran —
   `src/spicelib/parser/inpsymt.c` was edited a minute before the run and the
   deck a minute before that. It is not this fix's: with
   `src/frontend/variable.c` and `src/frontend/options.c` reverted to
   `58496a8dc` and the tree rebuilt, that deck fails identically, `pair5` 1
   where the reference says 0. Identity lint 264 comparisons, baseline
   matches — one fewer than before, which is the deleted `pl_env` scan and is
   the only frozen file this change moves.

**Left.**

- **A released binary cannot be fixed from here, and this is the residue that
  matters.** `/usr/local/bin/ngspice`, `ngspice-46`, still segfaults on
  `load <a raw this build writes>` + `unset casemode` + `display`, 3 runs of
  3, because the defect is in its own `cp_remvar()`. The control is the
  byte-identical file with the `Option:` line stripped, which exits 0. So for
  as long as this build writes that line, a user of an older ngspice who
  unsets the key gets that crash; the fix above closes it for this build and
  for anyone who rebuilds, and closes nothing on a binary already installed.
  Whether that is a reason to reconsider the header line is `raw_write()`'s
  question and not this issue's — it is recorded in
  `doc/codex/issues/0061`'s addendum, where the line was decided.
- `cp_remvar: Internal Error: US val 5` on `unset gmin` in a session with no
  circuit, from the `US_NOSIMVAR` arm. `if_option(NULL, …)` answers 1 for any
  simulator option and prints its own two lines, so the arm is on an ordinary
  path and its message is as wrong about itself as the two that were removed.
  Not touched: nothing frees anything twice there, the aux node is the only
  node involved, and the message has been printing since the arm was written.
- `unset` of a `US_SIMVAR` name no longer leaves the circuit's copy behind,
  but nothing tells the simulator to go back to the default the option had
  before the `set` — `if_option()` was called on the way in and is not called
  on the way out. That is what `unset` of a simulator option has always done
  and this issue does not change it.

## Related

- `doc/codex/issues/0065` — a rawfile `Option:` line shadowing a computed
  variable. Its fix removed borrowed nodes from the `cp_usrvars()` chain, which
  is why `cp_remvar()` walking that chain now finds only nodes the chain owns.
  That is what makes the `US_DONTRECORD` symptom a clean double free rather
  than something worse, and it is not a fix for this. Its criterion 6 named
  both `cp_remvar()` and `cp_vprint()` as correct; `cp_vprint()` is, and the
  sentence about `cp_remvar()` has been corrected to point here. **Its Impact's
  `rc=139` for `load` + `unset zzz` is correct and stands.** The Left bullet
  that later reported it as not reproducing was measuring the two-command form,
  which returns cleanly and then exits before anything walks the plot; that
  bullet has been corrected in place.
- `doc/codex/issues/0041` owns `load`'s leaks. The `24 direct + 9 indirect`
  bytes an `Option:` node leaks at exit are that issue's, not this one's, and
  are reported byte-identically by stock `ngspice-46`.
