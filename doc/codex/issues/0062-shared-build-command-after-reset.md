# Issue: After `ngSpice_Reset()` the Shared Library Is De-Initialised, `ngSpice_Command()` Goes Silent and `ngSpice_Circ()` Segfaults

## Status

Open. Surfaced by the verifier of the case-mode batch, which needed a
`--with-ngshared` build to reach one line and tripped over this on the way;
that is the whole of the connection to case mode, and nothing below touches it
or depends on it.

Pre-existing and upstream. Both lines that produce it were added by
`d0ae65acc` (2024-06-23, "Add function ngSpice_Reset(void) to completely reset
shared ngspice, so that it may be restarted again by ngSpice_Init"), and
`git blame` puts no local change on either:

```
$ git blame -L 2531,2534 --date=short -- src/sharedspice.c
d0ae65accf (Holger Vogt 2024-06-23 2531) {
d0ae65accf (Holger Vogt 2024-06-23 2532) 
d0ae65accf (Holger Vogt 2024-06-23 2533)     is_initialized = FALSE;
d0ae65accf (Holger Vogt 2024-06-23 2534) 
$ git blame -L 2592,2595 --date=short -- src/sharedspice.c
d0ae65accf (Holger Vogt 2024-06-23 2592)     destroy_const_plot();
d0ae65accf (Holger Vogt 2024-06-23 2593)     spice_destroy_devices();
d0ae65accf (Holger Vogt 2024-06-23 2594)     unset_all();
d0ae65accf (Holger Vogt 2024-06-23 2595)     cp_resetcontrol(FALSE);
```

The working tree does carry one uncommitted line in `totalreset()`, an
`inp_case_announce_reset()` call at `:2596`; it is after both of the lines
above, it is not what this issue is about, and the Impact section measures it
out.

Measured 2026-08-13 at `720c8743a` on branch `ver_50`, against
`build-shared/`, configured with a bare `../configure --with-ngshared`
(`build-shared/config.log`, `$ ../configure --with-ngshared`) and reporting
itself as `ngspice-46+ shared library, Creation Date: Thu Aug 13 11:08:24 UTC
2026`. `build-shared/` is 98 MB and is not committed. The default
`build-ver_50/` tree is not a shared build, so nothing in this issue is
reachable from `make check` — see acceptance criterion 5.

The verifier attributed the crash to `CKTmodCrt()`. That is where it lands in
its own harness and it is not the cause; a fresh process lands one frame
earlier, in `INPtypelook()`. Both are named below and the reason the two
differ is a function-static cache, not a second defect.

Three probes accompany this issue, and every measurement of the running library
below is one of them run verbatim. They are not committed — `doc/claude/feedback/`
is untracked in its entirety, this batch included — so they are reproducible
but not preserved; `doc/codex/issues/0066` records that as an open decision.

- `doc/claude/feedback/ngspice_upstream/repro/shared/reset_reinit_probe.c` —
  the minimal case, four API calls.
- `doc/claude/feedback/ngspice_upstream/repro/shared/reset_sequence_probe.c` —
  takes the call order as `argv[1]`, for the table below and the backtraces.
- `doc/claude/feedback/ngspice_upstream/repro/shared/reset_devtable_probe.c` —
  prints the three values the crashing code reads, before and after the reset.

The verifier's original harness,
`doc/claude/feedback/ngspice_upstream/repro/shared/casemode_reset_probe.c
--reset`, still reproduces it and is what this issue was reduced from.

## Summary

`ngSpice_Reset()` de-initialises the library. Nothing in the header says so,
and the two most-used entry points react to that state differently: one fails
silently, the other does not check and crashes.

The whole of the repro is four calls, no deck features, no options, no case
mode:

```c
    ngSpice_Init(cb_char, NULL, cb_exit, NULL, NULL, NULL, NULL);
    ngSpice_Reset();

    printf("Command rc=%d\n", ngSpice_Command("echo COMMAND-RAN"));
    printf("Circ rc=%d\n", ngSpice_Circ(deck));      /* never returns */
```

with `deck` a two-device netlist (`v1 in 0 dc 1`, `r1 in 0 1k`).

```
$ gcc -g -O0 -o /tmp/reset_reinit_probe \
      doc/claude/feedback/ngspice_upstream/repro/shared/reset_reinit_probe.c \
      -I src/include -L build-shared/src/.libs -lngspice
$ LD_LIBRARY_PATH=build-shared/src/.libs SPICE_SCRIPTS=build-shared/src \
      /tmp/reset_reinit_probe ; echo "rc=$?"
NG| stdout ******
NG| stdout ** ngspice-46+ shared library
NG| stdout ** Creation Date: Thu Aug 13 11:08:24 UTC 2026
NG| stdout ******
NG| stdout Note: Resetting ngspice
Command rc=1
NG| stdout Note: No compatibility mode selected!
NG| stdout Circuit: * t
Segmentation fault (core dumped)
rc=139
```

Read it against the callback log, which is the whole of what a host sees.
`echo COMMAND-RAN` produced no `NG|` line at all — the command did not run,
and `ngSpice_Command()` returned `1` without a word about why. The following
`ngSpice_Circ()` got as far as echoing the title and died.

The same program with one extra `ngSpice_Init()` call inserted after the
reset, and no other difference:

```
$ LD_LIBRARY_PATH=build-shared/src/.libs SPICE_SCRIPTS=build-shared/src \
      /tmp/reset_reinit_probe --reinit ; echo "rc=$?"
NG| stdout ******
NG| stdout ** ngspice-46+ shared library
NG| stdout ** Creation Date: Thu Aug 13 11:08:24 UTC 2026
NG| stdout ******
NG| stdout Note: Resetting ngspice
NG| stdout ******
NG| stdout ** ngspice-46+ shared library
NG| stdout ** Creation Date: Thu Aug 13 11:08:24 UTC 2026
NG| stdout ******
NG| stdout COMMAND-RAN
Command rc=0
NG| stdout Note: No compatibility mode selected!
NG| stdout Circuit: * t
Circ rc=0
survived
rc=0
```

Both halves go away together. That is what identifies the missing half, and it
is why this is one issue and not two.

No prior circuit is needed. The reset can be the second call the host ever
makes. `reset_sequence_probe.c` takes the call order as `argv[1]` — `I` is
`ngSpice_Init()`, `R` is `ngSpice_Reset()`, `C` is `ngSpice_Circ(deck)` — and
announces each step before making it, so the last step printed is the one that
died. An initial `ngSpice_Init()` is always done first, so `C` is one `Init`
and one `Circ`:

```
$ gcc -g -O0 -o /tmp/reset_sequence_probe \
      doc/claude/feedback/ngspice_upstream/repro/shared/reset_sequence_probe.c \
      -I src/include -L build-shared/src/.libs -lngspice
$ for s in RC C CR CRC CRIC ; do
>   out=$(LD_LIBRARY_PATH=build-shared/src/.libs SPICE_SCRIPTS=build-shared/src \
>           /tmp/reset_sequence_probe $s 2>/dev/null ; echo "rc=$?")
>   printf '%-5s  %-18s  %s\n' "$s" \
>          "$(echo "$out" | grep -E '^== ' | tail -1)" "$(echo "$out" | tail -1)"
> done
RC     == step C ==        rc=139
C      == survived ==      rc=0
CR     == survived ==      rc=0
CRC    == step C ==        rc=139
CRIC   == survived ==      rc=0
```

`CR` is the row that shows the reset itself is not what crashes, and `CRIC` is
the `--reinit` result again by another route.

## Impact

**A host that follows the header crashes.** `sharedspice.h:552` documents the
call in full as

```c
/* Reset ngspice as far as possible. */
IMPEXP
int ngSpice_Reset(void);
```

and the prose block at `:151`-`:153` repeats the same six words. Neither says
the library is left unusable, and neither names `ngSpice_Init` as the
continuation. A caller that reads that and reuses the handle gets a SIGSEGV
*inside the library*, which for an embedding process — a schematic editor, a
language binding, a batch sweeper — takes down the whole process rather than
returning an error it could handle.

**Return codes are not a defence.** `ngSpice_Command()` at least returns `1`,
though silently. `ngSpice_Circ()` never returns at all, so the defensive host
and the careless host meet the same fate.

**The failure before the crash is silent, and it is the more dangerous half.**
A host that resets, re-applies its settings with `ngSpice_Command("set ...")`
and only later loads a deck will have had every one of those commands dropped
on the floor with no diagnostic. If the crash is fixed and the silence is not,
that host runs to completion with none of its configuration applied.

**The shipped example already works around it, which is how the intended
contract is known.** `examples/shared/shx.c` calls `ngSpice_Reset()` in three
places (`:183`, `:302`, `:524`) and in the two that continue afterwards it
immediately calls `register_cbs()` (`:174`, `:308`), whose first act is
`ngSpice_Init_handle(...)` (`:136`). So the example never exercises the
defect. The rule it encodes is written down nowhere except in `d0ae65acc`'s
commit message.

**A second reset is a silent no-op too.** `ngSpice_Reset()` is itself guarded
(`sharedspice.c:1473`), so it returns `1` and the "Note: Resetting ngspice"
banner appears once however many times it is called. `RRX` — reset, reset,
`echo` — is the whole of it, and the missing `CMD-RAN` is the silence again:

```
$ LD_LIBRARY_PATH=... /tmp/reset_sequence_probe RRX ; echo "rc=$?"
NG| stdout ******
NG| stdout ** ngspice-46+ shared library
NG| stdout ** Creation Date: Thu Aug 13 11:08:24 UTC 2026
NG| stdout ******
== step R ==
NG| stdout Note: Resetting ngspice
== step R ==
== step X ==
== survived ==
rc=0
```

**Invisible to the test suite.** No file under `tests/` links `libngspice` —
`grep -rn "ngshared\|libngspice" tests/ --include=Makefile.am` is empty — and
`build-ver_50/` is not a shared build, so `make check` cannot reach any of
this in any configuration. That is criterion 5.

**Not a case-mode defect and not mode-dependent.** `reset_reinit_probe.c` never
mentions `casemode`. The verifier separately measured its `--reset` path
identically with and without the working tree's `inp_case_announce_reset()`
line compiled into `totalreset()`, and that function only clears a static bool
and a static string (`src/frontend/inpcom.c:1117`-`:1121`).

## Root Cause

`totalreset()` (`src/sharedspice.c:2530`) is a de-initialiser, not a "reset to
freshly-initialised". It is the exact inverse of `ngSpice_Init()`, and
`d0ae65acc`'s commit message says so — *"so that it may be restarted again by
ngSpice_Init"*. Nothing is wrong with that design. The defect is that the API
neither states the requirement nor defends against its being missed, and the
two entry points a host will reach for next fail in two different ways.

**The de-initialised state is marked, and almost nobody reads the mark.**
`totalreset()` clears the flag first thing:

```c
static int totalreset(void)                          /* sharedspice.c:2530 */
{

    is_initialized = FALSE;                          /* :2533 */
```

`is_initialized` is written `TRUE` in exactly one place, `sharedspice.c:1091`,
near the end of `ngSpice_Init()`. Of the twenty-one exported entry points, four
sites read it: `ngSpice_Command()` at `:1140`, a second unreachable check
inside the same function at `:1163`, `ngGet_Vec_Info()` at `:1201`, and
`ngSpice_Reset()` itself at `:1473`. `ngSpice_Circ()` (`:1247`) reads nothing.

**Half one — the silence.** `ngSpice_Command()`'s guard returns without a
message:

```c
int  ngSpice_Command(char* comexec)                  /* sharedspice.c:1138 */
{
    if (!is_initialized) {                           /* :1140 */
        return 1;                                    /* :1141 */
    }
```

The message exists, twenty-three lines further down, and cannot be reached
because `:1141` has already returned:

```c
    if ( ! setjmp(errbufc) ) {                       /* :1158 */
        ...
        if (!is_initialized) {                       /* :1163 */
           fprintf(stderr, no_init);                 /* :1164 */
           return 1;
       }
```

`no_init` is `"Error: ngspice is not initialized!\n   Run ngSpice_Init first"`
(`sharedspice.c:417`) — the sentence the host needed, sitting in dead code.
`ngGet_Vec_Info()`'s guard at `:1201`-`:1203` prints it and is live, so the
string is not dead everywhere; it is dead on the path that matters here.

**Half two — the crash.** `totalreset()` frees the device table:

```c
    destroy_const_plot();
    spice_destroy_devices();                         /* sharedspice.c:2593 */
```

```c
void
spice_destroy_devices(void)                          /* dev.c:233 */
{
#ifdef XSPICE
    tfree(g_evt_udn_info);
    tfree(DEVicesfl);
#endif
    tfree(DEVices);                                  /* :239 */
    DEVNUM = 0;                                      /* :240 */
}
```

`tfree` is `#define tfree(x) (txfree(x), (x) = 0)` (`memory.h:16`), so
`DEVices` becomes `NULL`. The matching `spice_init_devices()` (`dev.c:245`) is
called from `SIMinit()` (`sharedspice.c:437`, `:440`) and from nowhere else in
this build, and `SIMinit()` is called only from `ngSpice_Init()`
(`sharedspice.c:927`). `totalreset()` does not call it on the way back up, and
`ngSpice_Circ()` does not check that anyone has.

What the freeing leaves behind is measurable and is worse than a null pointer,
because `SIMinit()` took *copies*:

```c
    spice_init_devices();                            /* sharedspice.c:440 */
    SIMinfo.numDevices = DEVmaxnum = num_devices();  /* :441 */
    SIMinfo.devices = devices_ptr();                 /* :442 */
```

`num_devices()` returns `DEVNUM` and `devices_ptr()` returns `DEVices`
(`dev.c:267`, `:272`), by value. `spice_destroy_devices()` updates the
originals and cannot reach the copies:

```
$ gcc -g -O0 -o /tmp/reset_devtable_probe \
      doc/claude/feedback/ngspice_upstream/repro/shared/reset_devtable_probe.c \
      -I build-shared/src/include -I src/include -L build-shared/src/.libs -lngspice
$ LD_LIBRARY_PATH=build-shared/src/.libs SPICE_SCRIPTS=build-shared/src \
      /tmp/reset_devtable_probe
after Init   DEVices=0x5947708a9460  ft_sim->devices=0x5947708a9460  ft_sim->numDevices=132
after Reset  DEVices=(nil)           ft_sim->devices=0x5947708a9460  ft_sim->numDevices=132
```

So after the reset there are two broken views of the same table: `DEVices` is
`NULL`, and `ft_sim->devices` still points at the freed block while
`ft_sim->numDevices` still claims 132 entries. Which one a re-read trips over
depends on a function-static cache in the parser, and that is the whole of the
difference between the two backtraces:

```c
void INP2V(CKTcircuit *ckt, INPtables * tab, struct card *current)
{
    static int type = -1;	/* the type the model says it is */   /* inp2v.c:20 */
    ...
    if (type < 0) {                                                  /* :34 */
        if ((type = INPtypelook("Vsource")) < 0) {                   /* :35 */
```

- **Cold cache** — the first `v` line the process ever parses. `INPtypelook()`
  walks `ft_sim->devices[i]` for `i < ft_sim->numDevices` (`inptyplk.c:27`,
  `:37`), i.e. 132 dereferences into freed memory:

  ```
  $ LD_LIBRARY_PATH=... gdb -q -batch -ex run -ex "bt 8" \
        --args /tmp/reset_sequence_probe RC
  #0  0x00007ffff7c81449 in INPtypelook () from build-shared/src/.libs/libngspice.so.0
  #1  0x00007ffff7c759ac in INP2V () from build-shared/src/.libs/libngspice.so.0
  #2  0x00007ffff7c7c328 in INPpas2 () from build-shared/src/.libs/libngspice.so.0
  #3  0x00007ffff7747680 in if_inpdeck () from build-shared/src/.libs/libngspice.so.0
  #4  0x00007ffff770c72c in inp_dodeck () from build-shared/src/.libs/libngspice.so.0
  #5  0x00007ffff770eb94 in inp_spsource () from build-shared/src/.libs/libngspice.so.0
  #6  0x00007ffff771038c in create_circbyline () from build-shared/src/.libs/libngspice.so.0
  #7  0x00007ffff76e6c65 in ngSpice_Circ () from build-shared/src/.libs/libngspice.so.0
  ```

- **Warm cache** — a circuit was read before the reset, so `type` still holds
  the index it resolved then, `INPtypelook()` is skipped entirely, and
  `IFC(newModel, ...)` (`inp2v.c:50`) reaches `CKTmodCrt()`, whose first use of
  the table is a plain `NULL` dereference:

  ```c
      model = (GENmodel *) tmalloc((size_t) *(DEVices[type]->DEVmodSize));  /* cktmcrt.c:31 */
  ```

  ```
  $ LD_LIBRARY_PATH=... gdb -q -batch -ex run -ex "bt 8" \
        --args /tmp/reset_sequence_probe CRC
  #0  0x00007ffff7c4048d in CKTmodCrt () from build-shared/src/.libs/libngspice.so.0
  #1  0x00007ffff7c75ad5 in INP2V () from build-shared/src/.libs/libngspice.so.0
  #2  0x00007ffff7c7c328 in INPpas2 () from build-shared/src/.libs/libngspice.so.0
  ...                                       # frames 3-7 identical to the above
  ```

  Only frames 0 and 1 differ, and the `INP2V` return address moves by 0x129
  because it is the other of that function's two calls into the table. This is
  the frame the verifier reported, and it is the `CRC` row of the table above.
  It is the second of two symptoms of one cause: a fix aimed at `CKTmodCrt()`
  would move the crash to `INPtypelook()` rather than remove it.

Confirming the pointer at the crash, from the cold-cache run:

```
$ LD_LIBRARY_PATH=build-shared/src/.libs SPICE_SCRIPTS=build-shared/src \
      gdb -q -batch -ex "break ngSpice_Reset" -ex run -ex "p (void*)DEVices" \
      -ex finish -ex "p (void*)DEVices" -ex continue -ex "bt 2" \
      --args /tmp/reset_sequence_probe RC
== step R ==

Breakpoint 1, 0x00007ffff76e6eb0 in ngSpice_Reset () from build-shared/src/.libs/libngspice.so.0
$1 = (void *) 0x55555557a460
NG| stdout Note: Resetting ngspice
main (argc=2, argv=0x7fffffffdc98) at .../repro/shared/reset_sequence_probe.c:48
48	        case 'R': ngSpice_Reset(); break;
$2 = (void *) 0x0
== step C ==
NG| stdout Note: No compatibility mode selected!
NG| stdout Circuit: * t

Program received signal SIGSEGV, Segmentation fault.
0x00007ffff7c81449 in INPtypelook () from build-shared/src/.libs/libngspice.so.0
#0  0x00007ffff7c81449 in INPtypelook () from build-shared/src/.libs/libngspice.so.0
#1  0x00007ffff7c759ac in INP2V () from build-shared/src/.libs/libngspice.so.0
```

`$1` and `$2` bracket the one call. The banner lines gdb interleaves are the
probe's own `SendChar` echo and the `main ()` frame is `finish` returning; the
two `$` values are the claim.

**Why only the shared build.** `spice_destroy_devices()` has one other caller,
`com_quit()` at `src/frontend/misccoms.c:116`, inside the `#ifdef SHARED_MODULE`
arm opened at `:114`; the `#else` at `:122` gives the binary `exit(exitcode)`
at `:123` instead. So even in the shared build that free is the last thing
before the process ends, and in the `ngspice` binary the function is a no-op
stub (`src/main.c:388`-`:390`) that never frees anything at all. Nothing in
this issue can be reached from a deck, from `ngspice --batch`, or from
`ngspice -p`. `ngSpice_Reset()` is the one call site that frees the table and
then expects the process to keep going.

**The other exported calls survive, and that is not reassurance.** After a
reset the read-only queries answer from wreckage rather than crashing —
`ngSpice_CurPlot()` returns `"const"` for the plot `destroy_const_plot()`
(`sharedspice.c:2592`) has just destroyed, `ngSpice_AllPlots()` returns a
non-empty list, `ngSpice_AllVecs("op1")` and `ngGet_Vec_Info("v(in)")` return
empty and `NULL`. So a host has no way to detect the de-initialised state by
asking: nothing reports it, one call lies, and one crashes.

## Acceptance Criteria

1. A host that calls `ngSpice_Reset()` and then reuses the library does not
   segfault. `reset_reinit_probe.c` with no arguments reaches its `survived`
   line and exits 0, and `reset_sequence_probe.c` gives `rc=0` on all five
   rows of the table above. Which of the two shapes is chosen —
   `totalreset()` re-initialises, or the entry points refuse — is criterion 3;
   under (b) the `Circ rc=` line becomes `1` rather than `0`, which is a pass.
2. A dropped command says so. `ngSpice_Command()` on a de-initialised library
   emits `no_init` (`sharedspice.c:417`) rather than returning `1` in silence,
   and the unreachable copy at `:1163`-`:1166` is either made reachable or
   removed. This holds whichever shape criterion 3 takes: even if reuse becomes
   legal, a call that does nothing must not do it quietly.
3. The contract is written down in `sharedspice.h`, next to the declaration at
   `:552` and in the prose block at `:151`. The two shapes are: **(a)**
   `ngSpice_Reset()` re-initialises, so the handle stays live and
   `examples/shared/shx.c`'s `ngSpice_Init()` calls after `:183` and `:302`
   become redundant rather than required; or **(b)** it stays a de-initialiser,
   the header says "the library is unusable until `ngSpice_Init()` is called
   again", and every unguarded entry point — starting with `ngSpice_Circ()` at
   `:1247` — checks `is_initialized` and returns an error. Shape (a) must
   decide what happens to the freed `ft_sim` copies at `:441`-`:442`, since
   re-running `spice_init_devices()` alone leaves them stale; shape (b) must
   cover `ngSpice_CurPlot()`, which today returns a pointer into a destroyed
   plot.
4. Whichever shape is chosen, `examples/shared/shx.c` still runs. It calls
   `ngSpice_Init()` after each reset today (`:174`, `:308`), which must remain
   legal under (a) and remain required under (b).
5. A test asserts it, and that needs somewhere to put it. `tests/` has no
   directory that links `libngspice` and `tests/Makefile.am`'s `SUBDIRS` has no
   arm for one, but the automake conditional already exists —
   `AM_CONDITIONAL([SHARED_MODULE], ...)` at `configure.ac:637` — so a
   `tests/shared/` gated `if SHARED_MODULE`, alongside the existing
   `if XSPICE_WANTED` arm at `tests/Makefile.am:6`, is the shape. The evidence
   in this issue is not that test: the three probes under
   `doc/claude/feedback/ngspice_upstream/repro/shared/` are enough to re-run
   every paste above by hand and are not enough to fail in `make check`.
6. `make check` unchanged in the default configuration. Nothing here is
   reachable without `--with-ngshared`, so no committed deck should move.

## Resolution

Unresolved.

A fix is a lifecycle decision rather than a code repair: either `totalreset()`
gains the re-initialising half it is missing — the `SIMinit()` call at
`sharedspice.c:927` that `ngSpice_Init()` makes, refreshing `ft_sim`'s copies
at `:441`-`:442` and not only `DEVices` — or `ngSpice_Reset()` is documented as
a de-initialiser and every exported entry point is made to check
`is_initialized` and say `no_init`, instead of the one that checks silently and
the one that does not check at all.
