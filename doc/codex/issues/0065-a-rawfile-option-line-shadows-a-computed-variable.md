# Issue: A Rawfile `Option:` Line Shadows a Computed Variable — `SIGSEGV` on a Bare `load`

## Status

Closed 2026-08-13 by the two changes described under Resolution: what
`cp_enqvar()` computes it now answers itself, ahead of the plot environment,
and `cp_usrvars()` no longer links a node it does not own. Filed the same day
on branch `ver_50`; everything above the Resolution is the measurement as it
stood before the fix and is left as written.

Measured at `720c8743a` against
`/home/qflow/dev/ngspice_test/build-ver_50/src/ngspice` (`ngspice-46+`, build
stamp `Thu Aug 13 17:41:58 UTC 2026`) and against `/usr/local/bin/ngspice`
(`ngspice-46`, no `casemode` support). Every line number in `options.c` below is
at `720c8743a`, because the working tree has an uncommitted `curcasemode` arm in
that file from `doc/codex/issues/0060`; the file that was compiled into the
binary measured here does not.

**Found while explaining `doc/codex/issues/0061` to the repo owner.** 0061 owns
the mechanism by which a raw header's `Option:` line files an arbitrary
`name=value` pair into `plot_cur->pl_env`. This issue is what happens when the
name it files collides with a name `cp_enqvar()` computes: not a wrong value, a
memory-safety fault. `Option: ngbehavior=hs` on the same path does **not** fault
— it silently takes effect, which is 0061 and is not restated here.

**Scope: this is the crash only.** The policy question — whether a loaded file
may set `casemode` or `ngbehavior` at all — stays in 0061.

**The owner has decided 0061 takes shape A of its criterion 3**: the `Option:`
pair is still filed into the plot environment, and the reads that steer parsing
stop consulting it. Shape A therefore leaves `rawfile.c:472`-`:483` filing
arbitrary names into `pl_env`, which is exactly the precondition below. **A fix
for 0061 does not close this issue, and a fix for this issue must survive shape
A.** That is the reason this is a separate file and not a criterion over there.

Pre-existing, mode independent, and upstream: it reproduces identically on
`ngspice-46`, and no name below is looked up across a case boundary.

## Summary

`cp_enqvar()` (`src/frontend/options.c:52`) searches the current plot's
environment (`:60`) **before** the arm that computes the `curplot` family
(`:70`) and the arm that computes `plots` (`:90`). `cp_usrvars()` (`:185`) then
asks `cp_enqvar()` for exactly those five names and writes `tv->va_next` into
whatever comes back. When the answer is a node borrowed from `pl_env`, that
write is into a node the plot owns, and the chain it builds is handed to a
caller that frees it.

Measured. Build the file by splicing one line in after the `Plotname:` line of
an ASCII raw — `doc/claude/feedback/ngspice_upstream/repro/ascii_raw.cir`
writes the raw, and `repro/hdr_variants.sh` is the splicer:

```sh
awk '/^Plotname:/{print; print "Option: curplot=HAHA"; next} {print}' \
    ascii_raw.raw > curplot.raw
```

```
$ ngspice -p
ngspice 1 -> load curplot.raw
Loading raw data file ("curplot.raw") ...
done.
Title:  * writes an ascii raw, so finding 1's header experiments can splice a line in
Name: Operating Point
Date: Thu Aug 13 11:03:45  2026

Here are the vectors currently active:

Segmentation fault (core dumped)
```

`echo $?` is **139**. The vector list the banner promises never prints, and the
`quit` on the next line of the pipe never runs. Same result in batch from a
`.control` block (`load` then `echo done`; the `echo` never runs), and the same
on `/usr/local/bin/ngspice` in both modes — four runs, `rc=139` in all four.

**The whole computed family is exposed, `plots` included.** One raw per name,
each `load`ed and nothing else:

| spliced line | `rc` |
| --- | --- |
| `Option: curplot=HAHA` | 139 |
| `Option: curplotname=HAHA` | 139 |
| `Option: curplottitle=HAHA` | 139 |
| `Option: curplotdate=HAHA` | 139 |
| `Option: plots=HAHA` | 139 |
| `Option: ngbehavior=HAHA` | 0 |
| `Option: casemode=HAHA` | 0 |
| `Option: sourcepath=HAHA` | 0 |
| `Option: zzz=HAHA` | 0 |
| `Option: CurPlot=HAHA` | 0 |

The five that fault are `cp_usrvars()`'s five and nothing else. The last row is
the containment: the `pl_env` scan at `options.c:61` is `eq()`, which is
`strcmp` (`src/include/ngspice/macros.h:25`), so only the exact lowercase
spelling shadows.

The consumer that dies is not a reader of `$curplot`. Nothing in `src/` calls
`cp_getvar("curplot", ...)` or `cp_getvar("plots", ...)`. The consumer is
`cp_usrvars()` itself, which every `cp_getvar()` call runs regardless of the
name being looked up — so the fault lands in whatever command next reads any
variable at all. In the transcript above that is `com_display()`
(`src/frontend/com_display.c:28`), which `com_load()` calls unconditionally
after a load (`src/frontend/postcoms.c:118`), and which reads two variables in
a row: `out_init()` at `:37` reads `moremode` (`src/frontend/terminal.c:77`) and
`:70` reads `nosort`. The first read frees; the second faults.

The frame, from `gdb` on an unstripped relink of the same objects (the shipped
`CFLAGS` carry `-s`):

```
Program received signal SIGSEGV, Segmentation fault.
#0  __strcmp_avx2 ()
#1  cp_enqvar ()          <- options.c:61, eq(vv->va_name, word)
#2  cp_usrvars ()
#3  cp_getvar ()
#4  com_display ()        <- the cp_getvar("nosort") at com_display.c:70
#5  doblock ()
#6  cp_evloop ()
#7  app_rl_readlines ()
#8  main ()
```

`valgrind` gives all three ends of it — where the node came from, who freed it,
who read it after:

```
Invalid read of size 1
   at strcmp
   by cp_enqvar / cp_usrvars / cp_getvar / com_display      <- the "nosort" read
 Address 0x6362840 is 0 bytes inside a block of size 13 free'd
   by free_struct_variable / cp_getvar / out_init / com_display   <- the "moremode" read
 Block was alloc'd at
   by tmalloc / cp_unquote / cp_setparse / raw_read / ft_loadfile / com_load
```

`cp_setparse()` called from `raw_read()` is the `Option:` arm
(`src/frontend/rawfile.c:481`/`:483`). So the string a file wrote is freed by
one `cp_getvar()` and dereferenced by the next, with `plot_cur->pl_env` still
pointing at it.

## Impact

A memory-safety fault reachable from a data file, with no prior warning and no
recovery. `load` is how a user reads a raw file that someone else produced; this
turns that file into control over a freed pointer.

- **The process dies, exit 139.** In `--batch` the deck stops mid-`.control`,
  so a marker printed after the `load` is absent — which is how a regression
  deck has to detect it, since there is no message on `stdout` or `stderr`.
- **Not confined to `load`.** The poison is per-plot and fires whenever the
  poisoned plot is `plot_cur` and one command reads two variables. Measured with
  a two-plot raw whose *first* plot carries the line, so that the load leaves
  the clean plot current: the `load` itself returns 0, and so does a bare
  `setplot op1`; `setplot op1` followed by `display`, by `print v(in)`, or by
  `write x.raw` is `rc=139` in each case. `echo hi` is 0, because it reads no
  variable.
- **The wrong value is never observed, only the fault.** The same `cp_getvar()`
  call that would return the file's string also frees the node it came from, so
  there is no window in which `$curplot` answers `HAHA`. This is a
  use-after-free, not a wrong answer; 0061's "a file-supplied string is returned
  where a computed value is expected" is the *shape*, and on these five names it
  never gets far enough to be read.
- **The shell route is guarded and the file route is not.** `set curplot=HAHA`
  at the prompt goes through `cp_vset()`/`cp_usrset()`, which dispatches it to
  plot selection — measured, `Error: no such plot named HAHA`, `rc=0`, and
  `pl_env` is untouched. The `Option:` arm bypasses that dispatch entirely,
  which is 0061's Root Cause observation and the reason only the file can do
  this.
- **Nothing in the tree trips it.** No committed deck or example contains an
  `Option:` line (measured in 0061), and no ngspice writer emits one unless it
  is round-tripping a header it read (`rawfile.c:132`).
- **Not measured in the shared build.** `src/frontend/options.c` and
  `src/frontend/variable.c` are in both link targets and `sharedspice.c` reaches
  `cp_getvar()` on the same paths, so a `ngSpice_Command("load ...")` should
  behave the same, but that was not run. Whoever fixes this should run it, since
  there a `SIGSEGV` takes the host application down.
- **An adjacent fault with a harmless key, which is a *different* defect.**
  `load` a raw carrying `Option: zzz=HAHA` (`rc=0`, and `set` lists it as
  `* zzz HAHA`, read-only), then `unset zzz`, then any command that walks the
  plot's environment: `rc=139`. The `unset` itself returns — the fault lands on
  the command after it, measured with `set`, with `display` and with
  `write w.raw`, three runs each on this build and on stock. That one needs no
  collision with a computed name — `cp_remvar()` reaches `free_struct_variable(v)`
  at `src/frontend/variable.c:672` on a node still linked into `pl_env`, through
  the `US_READONLY` arm at `:641` that explicitly says "any var in
  `plot_cur->pl_env`". It shares this issue's precondition and not its
  mechanism, so it wants its own file; it is recorded here only so that the
  next person does not mistake it for this one.

## Root Cause

`cp_enqvar()`'s contract is written down at `src/frontend/options.c:38`-`:50`:
`tbfreed` is "set to 1, if the variable is malloced here and may safely be
freed, and is set to 0 if plot and circuit environment variables are returned",
and a `tbfreed == 0` result is a **borrowed** node — the caller may neither free
it nor modify it, since "any changes will have no effect on the original
variable" is stated only of the copied case.

`cp_usrvars()` (`options.c:185`-`:214`) declares `int tbfreed;` at `:188`,
passes its address to `cp_enqvar()` five times, and never reads it. Each of the
five arms does the same two lines unconditionally:

```c
    if ((tv = cp_enqvar("plots", &tbfreed)) != NULL) {   /* options.c:192 */
        tv->va_next = v;                                /* :193 */
        v = tv;
    }
```

with the same pair repeated at `:197`, `:201`, `:205` and `:209`. On a borrowed
node that single assignment does two separate kinds of damage:

1. **It rewrites the plot's own list.** `tv` is a link of `plot_cur->pl_env`.
   Setting its `va_next` re-points the plot's environment at a temporary chain
   and orphans whatever followed it. With `pl_env` = [`curplot`, `zzz`], the
   `zzz` node becomes unreachable; with [`zzz`, `curplot`], `zzz` now points
   into the temporary chain.
2. **It enlists a node the plot owns into a chain the caller frees.** All three
   callers of `cp_usrvars()` free the returned list with
   `free_struct_variable()` (`variable.c:560`), which walks `va_next` to the
   end: `cp_getvar()` at `variable.c:687` frees on all four exits (`:716`,
   `:753`, `:767`, `:771`), `cp_remvar()` at `:583` frees at `:674`, and
   `cp_vprint()` at `:1116` frees at `:1182`. So the file's node, and its
   `va_name` and `va_string`, are freed while `plot_cur->pl_env` still points at
   them, and the next walk of `pl_env` — which is the very next `cp_getvar()` —
   reads freed memory.

The ordering inside `cp_enqvar()` is what makes the collision possible at all.
The `pl_env` scan at `:60`-`:65` runs first and returns on any exact match; the
`curplot` family at `:70`-`:88` and `plots` at `:90`-`:97` are only reached when
that scan misses. So `pl_env` — a namespace 0061 shows a file can write into —
sits in front of the computed one, and the five names `cp_usrvars()` asks for
are precisely the five that live behind it.

There is a precedent in the tree for the ordering that avoids this: 0060's
`curcasemode` arm (in the working tree at the time of filing, not yet committed)
is placed *ahead* of the `pl_env` scan, with a comment naming `rawfile.c`'s
`Option:` line as the reason. That arm is consequently not shadowable and is not
one of `cp_usrvars()`'s five. It is the pattern, and it does not fix the five.

## Acceptance Criteria

1. Each of the ten rows of the Summary's table gives `rc=0`, and the `load`
   prints the vector list the banner promises.
2. The two-plot sequence gives `rc=0` as well: `load` the two-plot raw, then
   `setplot op1`, then each of `display`, `print v(in)`, `write x.raw`.
3. Under `valgrind`, the load-then-`display` sequence reports no invalid read,
   no invalid free and no new leak.
4. **The rule is about ownership, not about five names.** A fix that
   special-cases `curplot` leaves `plots`, and leaves the next computed name
   somebody adds. The invariant to state and hold is that `cp_usrvars()` must
   not write into, nor hand to a freeing caller, a node `cp_enqvar()` returned
   with `tbfreed == 0`.
5. **It survives 0061 shape A.** Shape A keeps `rawfile.c:472`-`:483` filing the
   pair, so the precondition remains; the fix must not be written as if the
   `Option:` arm will stop accepting these names. Conversely, if 0061 is ever
   re-scoped to refuse names at that arm, this issue is still live, because
   `pl_env` is also writable by the plot's own `Option:` round-trip and the
   ownership bug in `cp_usrvars()` is independent of who wrote the node.
6. The other two `cp_usrvars()` callers are inspected in the same change and
   named in the resolution: `cp_remvar()` (`variable.c:583`, frees at `:674`,
   and additionally takes `p = &uv1` at `:592`, so it can unlink through the
   borrowed chain) and `cp_vprint()` (`:1116`, frees at `:1182`).
7. A deck asserts it. `tests/regression/pipe/` is the shape, for the reason
   0061 criterion 5 gives: `load` then `display` is control language, not a
   netlist. The deck has to build its own header, because no writer emits
   `Option:` unless round-tripping one —
   `tests/regression/casedist/vector-rawfile-scale-case.cir` is the precedent
   for a deck that writes the rawfile it then loads. The assertion must be a
   marker printed *after* the load, not the absence of a message: the process
   dies silently and `tests/bin/check.sh` filters heavily, so a missing marker
   is the only reliable tell.
8. `make check` unchanged in all three modes. No committed deck contains an
   `Option:` line, so the fix should move no `.out` file.

## Resolution

Closed 2026-08-13. Two edits, both in `src/frontend/options.c`, neither of
them a list of names.

**1. `cp_enqvar()` — what this function computes, it answers itself.** The
`curplot` family and `plots` arms now come *before* the scan of
`plot_cur->pl_env`, not after it, so the collision the fault needs cannot
occur: the environment answers only names this function does not compute.
That is the ordering `doc/codex/issues/0060` already used for `curcasemode`,
generalised to the rest of the family, and it is also the answer to "a
file-supplied string where a computed value is expected" — the wrong value
this issue could never observe, because it faulted first. Nothing else in the
function moved; the `$&vector` arm, the `curcasemode` arm and the
`ft_curckt->ci_vars` tail are where they were.

**2. `cp_usrvars()` — only an owned answer goes on the list.** The five
repeated `tv->va_next = v; v = tv;` pairs are replaced by five calls to one
new static helper:

```c
static struct variable *usrvar_push(const char *name, struct variable *list)
{
    int tbfreed = 0;
    struct variable * const tv = cp_enqvar(name, &tbfreed);

    if (!tv || !tbfreed)
        return list;

    tv->va_next = list;
    return tv;
}
```

That is criterion 4's invariant, stated as a rule about ownership: a
`tbfreed == 0` answer is borrowed from a plot's or a circuit's environment,
the caller may neither modify nor free it, and this list is both — modified
by the `va_next` assignment that links it and freed by all three callers. It
holds for any name `cp_enqvar()` ever learns to answer from an environment,
not for the five it is asked for today. Dropping rather than copying loses no
answer, because each of the three callers reaches both environments by its own
path anyway; that is spelled out in the comment above the helper and in
criterion 6 below.

One test file, `tests/regression/pipe/rawfile-option-computed-name.cmd`, plus
its `TESTS` and `CLEANFILES` entries in that directory's `Makefile.am`. No
`.out` file was added or edited, and `identity.baseline` is unchanged: the
`eq(vv->va_name, word)` of the environment scan moved but was not rewritten,
and the baseline records the call text without a line number, so it matches
where it stands (`make check` reports the same 265 comparisons).

**Criterion by criterion.**

1. *Each of the ten rows gives `rc=0`, and the `load` prints the vector list.*
   Re-run on the fixed binary, one raw per row, `load` then `echo MARKER`:

   ```
   curplot        rc=0    marker printed
   curplotname    rc=0    marker printed
   curplottitle   rc=0    marker printed
   curplotdate    rc=0    marker printed
   plots          rc=0    marker printed
   ngbehavior     rc=0    marker printed
   casemode       rc=0    marker printed
   sourcepath     rc=0    marker printed
   zzz            rc=0    marker printed
   CurPlot        rc=0    marker printed
   ```

   and the read that used to be impossible now answers the session's own
   value: after `load` of the `Option: curplot=HAHA` file, `echo $curplot`
   prints `op1`, and `set` lists `* curplot op1`. The last row's containment
   is moot now that no spelling can shadow, and in one direction it moved:
   the environment scan is still `eq()`, so a mixed-case key is still filed
   under its own spelling and still listed by `set`, but `$CurPlot` answers
   `op1` where it used to answer `HAHA`, because the arm it now sits behind
   matches with `cieqn()`. That is that arm's own case rule and not a new
   one — it is what `cp_usrset()` already dispatches a `set CurPlot=…` write
   by — so the read and the write of that name now agree.
   `doc/codex/issues/0048` is neither fixed nor widened here: no name a deck
   wrote is compared on this path.
2. *The two-plot sequence gives `rc=0`.* `load` the two-plot raw whose first
   plot carries the line, `setplot tran1`, then each of `display`,
   `print OUT` and `write x.raw`: `rc=0` and the marker after each. All three
   are in the deck.
3. *Under `valgrind`, no invalid read, no invalid free, no new leak.*
   `load` + `display` on the `Option: curplot=HAHA` file:

   ```
   $ valgrind --leak-check=full --show-leak-kinds=definite build-ver_50/src/ngspice -p < vg.cmd
   ... definitely lost: 26 bytes in 2 blocks
   ... ERROR SUMMARY: 2 errors from 2 contexts
   ```

   No `Invalid read`, no `Invalid write`, no `Invalid free` — against the same
   run on `/usr/local/bin/ngspice`, which reports `Invalid read of size 8`,
   `Invalid read of size 1` and `Address … free'd`. The two remaining blocks
   are both pre-existing and neither is new: 2 bytes appear on a `load` of a
   rawfile with **no** `Option:` line at all, and the other block —
   `24 direct + 9 indirect` for `Option: zzz=HAHA` — is the plot-environment
   node itself, never freed at exit, and is reported byte-identically by stock
   `ngspice-46` on the same input. `doc/codex/issues/0041` owns `load`'s
   leaks.
4. *The rule is about ownership, not about five names.* Both halves are
   stated that way and neither mentions a name: `usrvar_push()` keys on
   `tbfreed`, and the reordering says that a computed name is answered by its
   computation. A sixth computed name added tomorrow is covered by both.
5. *It survives 0061 shape A.* Shape A landed in the same tree and keeps
   `rawfile.c:472`-`:483` filing arbitrary pairs into `pl_env` — the deck
   depends on that, since every check begins by loading a header that files
   one and the last check reads one back. Neither change here touches the
   `Option:` arm.
6. *The other two `cp_usrvars()` callers, inspected and named.*
   `cp_remvar()` (`variable.c:577`) takes `p = &uv1` at `:592` and can unlink
   through the chain, then frees the unlinked node at `:672` and the rest of
   the chain at `:674`; `cp_vprint()` (`:1119`) prints the chain and frees it
   at `:1194`. (Those are the numbers after this change and after 0061's,
   which added eleven lines to `cp_getvar()`; the Root Cause above quotes them
   as they were.) Neither needs a change **for this issue**, and for the same
   reason: the chain they are handed now contains only nodes the chain owns,
   so unlinking from it, printing it and freeing it are all operations on
   `cp_usrvars()`' own memory. Their access to the plot and circuit
   environments is unaffected — `cp_vprint()` prints those as their own
   section (`variable.c:1137`, `:1159`) and `cp_remvar()` unlinks from them
   directly (`:600`, `:608`) — which is why dropping a borrowed node from the
   chain removes no answer from either.

   That is the whole of the claim, and an earlier draft of this criterion
   overstated it as "both correct with no change of their own".
   **`cp_vprint()` is correct. `cp_remvar()` is not**, and the two bullets
   under Left below are the evidence: `unset curplot` aborts inside it with a
   double free of `cp_usrvars()`' own memory, on this build and on stock, with
   no rawfile anywhere. Its defect is a pre-existing one this issue neither
   causes nor cures — the free at `:672` runs whether or not the arm above it
   unlinked — and it is now filed as `doc/codex/issues/0067`, which takes both
   Left bullets as one defect and says why they are one. What this criterion
   asserts about `cp_remvar()` is only that the *chain* it is handed is now
   unambiguously its own to free, which is what dropping borrowed nodes bought;
   what it does with that chain afterwards is 0067's.
7. *A deck asserts it.* `tests/regression/pipe/rawfile-option-computed-name.cmd`,
   in the directory that can see an exit status, building every rawfile with
   `echo` and its redirect. Four checks: five loads in a `foreach`, one per
   computed name, whose assertion is the marker after the loop; one load of a
   rawfile naming all five at once, followed by a capture of all five reads;
   the two-plot `setplot`/`display`/`print`/`write` sequence of criterion 2;
   and a single scan of the capture for the file's string. RED at
   `720c8743a` + this deck, before the production change:

   ```
   $ ngspice -p < tests/regression/pipe/rawfile-option-computed-name.cmd ; echo rc=$?
   Segmentation fault (core dumped)
   rc=139
   ```

   with no marker printed — it dies inside the first `load` of the loop.
   GREEN after it, `rc=0` and all three markers.
8. *`make check` unchanged in all three modes.* 305 passed, 0 failed, with
   every cached `*.log`/`*.trs` deleted first; the 303 this batch had been
   running plus this deck and `doc/codex/issues/0061`'s. No `.out` file moved.

**What each half is worth, measured separately.** The two edits were
disabled one at a time against the finished deck, so that neither is taken on
trust:

- ordering reverted, `usrvar_push()` kept: **no fault at all** — the five
  loads survive and so do `setplot`/`display`/`print`/`write` — and the deck
  fails on the value instead, `ERROR: a rawfile Option: key answered a read of
  a computed variable`. So the ownership rule alone is what stops the
  memory-safety fault, and it is measured doing it.
- ordering kept, `usrvar_push()`'s `!tbfreed` test removed: the deck passes.
  With the ordering in place, `cp_usrvars()` cannot be handed a borrowed node
  by any input this deck can write, so the guard is not what the deck detects
  in the shipped pair. It is kept because it is the invariant the contract at
  `options.c:38`-`:50` already states, and because the ordering only protects
  the names `cp_enqvar()` computes *inside* `if (plot_cur)` — the
  `ft_curckt->ci_vars` tail still returns borrowed nodes.

**Left.**

- **`unset` of a computed name aborts, and it is not this issue.** After the
  fix, `load` of the `Option: curplot=HAHA` file followed by `unset curplot`
  gives `rc=134`, which is what the *same session without any rawfile* gives:
  `printf 'unset curplot\nquit 0\n' | ngspice -p` is `rc=134` on this build
  **and on stock `ngspice-46`**, as is `unset plots`. So the file no longer
  changes the outcome — that is this issue's claim — and what is left is a
  pre-existing `cp_remvar()` defect on a computed name with no file involved
  anywhere. It wants its own file; it is not in the ten-row table, which is
  about `load`. **Filed as `doc/codex/issues/0067`.**
- The adjacent `unset zzz` fault this issue's Impact recorded as `rc=139` is
  **correct, and stands**. An intermediate re-measurement reported it as not
  reproducing; that re-measurement ran `load` then `unset zzz` then `quit`, and
  the `unset` is not what dies. Add the ordinary next command and the fault is
  back: `load`, `unset zzz`, `set` is `rc=139`, three runs of three, on
  `build-ver_50/src/ngspice` and on stock `/usr/local/bin/ngspice`, with
  `display` and `write w.raw` giving the same. The same three commands under
  `valgrind` are `rc=0` with 34 invalid reads here and 65 on stock — that is
  the tool holding the freed block readable, not the program surviving, and
  running it only that way is how the crash came to be written off. It is the
  same `cp_remvar()` arm as the bullet above; whoever files that issue should
  treat both as one. **They were: `doc/codex/issues/0067` is one issue for
  both**, on the argument that they are one unconditional free at
  `variable.c:671`-`:672` of a node the `switch` arms above it have already
  disposed of, differing only in which list still points at it — the
  `cp_usrvars()` chain, freed two lines later, so a deterministic double free
  inside the `unset`; or `plot_cur->pl_env`, which nothing frees, so a freed
  node the plot keeps and the next command walks. 0067 records a third arm,
  `US_SIMVAR`, found later.
- The shared build is still not exercised. `src/frontend/options.c` is in both
  link targets and nothing here is conditional, but no `libngspice` is built
  in `build-ver_50`, so `ngSpice_Command("load …")` was not run.
