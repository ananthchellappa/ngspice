# Issue: `write` Emits The Constants Plot When No Analysis Ran

## Status

**Open.** The defect is real and reproduces; the fix was attempted three times
in one batch, refused correct work each time, and was **withdrawn on
2026-08-13**. No row is fixed. Row 2 remains **decided** — it keeps its
behaviour, by `doc/claude/decisions/0017-write-refuses-an-unchosen-plot.md`
decision 1 — and that decision record is still worth reading, but it is marked
withdrawn along with the code it justified.

The withdrawal is not a reversal of judgement about rows 1, 7, 8 and 9. It is
the finding of row 10, which the three rounds of guard all refused: the
constants plot is **also where a nutmeg session's own `let` vectors live**, so
"the current plot is a plot nothing chose" does not mean "the current plot is
garbage". Every shipped ngspice writes such a session's data; the guard wrote
nothing at all, in `--batch` and through `ngspice -p` alike. A guard that
produces no file for correct work is worse than a defect that produces a
self-labelled file for incorrect work, so it came out. `tests/regression/misc/
write-let-session.cir` now pins row 10 so that the next attempt cannot
reintroduce it, and the Resolution section records what each of the three
discriminators was, how each was falsified, and what a correct one has to do.
The user-visible sentence criterion 2 asks for is **not** in `NEWS` — it was
removed with the guard, because nothing user-visible changed.

**The withdrawal stands, and three routes measured after it make it more
clearly right, not less.** A verification pass on 2026-08-13 found three further
ways into the same fallback, none of which any of the three guards could have
covered: `set plainwrite`, which takes a **second branch of `com_write()`** that
never calls `ft_getpnames_quotes()`; `set appendwrite`, which hides the
constants plot *inside* a real run's rawfile; and `wrdata`, which is a different
command reaching the same `vec_get()` fallback and cannot be guarded from
`com_write()` at all. They are rows 11, 12 and 13 of the matrix below, each
re-measured 2026-08-13. Taken with rows 1, 7, 8 and 9 they mean a guard of the
attempted shape would have had to cover **at least four routes across two
commands** — and three rounds of widening had already failed to cover one. Row
12 also falsifies the sentence the trade in the Resolution rested on; that
sentence is corrected there.

**Corroborated independently on 2026-08-14, by the client, on stock.** The
xschem session re-measured the shape from the outside and got the four-row
table under Impact's *"Independent corroboration"* below: `.save v(nosuchnode)`
produces `Plotname: constants` in `fold`, `preserve` and `distinguish` **and**
on `/usr/local/bin/ngspice` with no flag, with zero mentions of the offending
token on either stream in all four. That does not change the withdrawal and it
is not a new ask — they say so themselves — but it moves the issue's priority,
because it settles two things this file had only argued. The failing shape
needs no `casemode` flag and no unusual command; and the two tells the
Resolution rests on are, from a consumer's side, the only two signals in
existence for it. They are implementing both as content checks plus a
vector-count floor.

Found by a client integration — xschem generating decks, reading the raw
file and showing the names back to the user — and reported as finding 3 of
`doc/claude/feedback/ngspice_upstream/FINDINGS.md`. Re-measured here at
`720c8743a` against `build-ver_50/src/ngspice` (`ngspice-46+`, build stamp
`Wed Aug 12 19:28:37 UTC 2026`), with `/usr/local/bin/ngspice` (`ngspice-46`,
no `casemode` support) as the baseline.

Every deck quoted below lives under
`doc/claude/feedback/ngspice_upstream/repro/` — `save_lower.cir`,
`nocase.cir`, `save_partial_miss.cir`, `seq.cir`, `rc0.cir`,
`destroy_curplot.cir`, `status_probe.cir`, `write_named.cir` and
`write_named_capture.cir` — and finding 3's section of that directory's
`run_all.sh` runs all nine, `save_lower.cir` at `:54` in the section above it
and the other eight at `:88-120`. The clean-run control the `Date:` comparison
needs, `divider.cir`, is in the same directory.

**The flag each deck needs, re-pinned 2026-08-13.** Six of the nine spell a
net `midnode` where the netlist says `MidNode`, so their `.save`/`save` only
misses under `-D casemode=distinguish`: `save_lower.cir`, `seq.cir`,
`rc0.cir`, `status_probe.cir`, `write_named.cir` and
`write_named_capture.cir`. Run with no `-D`, **or under `preserve`**, those six
take the healthy path instead — `save_lower.cir` exits 0 with a 296-byte
`Plotname: Operating Point` raw, and `write_named.cir` exits 0 and writes 324
bytes rather than refusing, and `seq.cir`'s second `op` succeeds so its raw is
317 bytes of its own data. The other three need no `-D` at all, because they
miss by a name the circuit has not got or by `destroy`: `nocase.cir`,
`save_partial_miss.cir`, `destroy_curplot.cir`. Every transcript below names
its flag, and `run_all.sh` passes the same ones (`:54`, `:103`, `:106`,
`:113`, `:117`, `:120`).

These six were first measured under `-D casemode=preserve`, which is the flag
the earlier revisions of this file named. `doc/codex/issues/0056` landed in the
same tree between the measurement and the fix, and a `.save` now resolves
case-insensitively under `preserve`, so `preserve` no longer reaches any of
these rows. Every number below was re-measured under `distinguish` and
reproduces: the byte counts, the `rc`s, the `Plotname:` lines and the
`status_probe` readings are unchanged, and the only difference is that each
`stderr` gains the two lines `distinguish` adds of its own — the experimental
banner, and `0057`'s near-miss report naming `midnode`. `save_lower.cir`'s own
first line still reads "kills the run under preserve"; it is quoted verbatim
below and has not been edited, because three findings share that deck.

Mode independent and pre-existing. No source line cited below is `casemode`
code, and the whole defect reproduces on the featureless
`/usr/local/bin/ngspice` (`ngspice-46`) with no `-D` at all — `nocase.cir` is
that measurement, and a case mode is only the most convenient way for a deck
that used to work to arrive here. It is therefore outside the Class A / B / C
taxonomy of `doc/claude/decisions/0001-distinguish.md` decision 3, which
classifies identifier *comparisons*; this issue contains none. The comparison
that used to make `preserve` an easy way in was `name_eq()`,
`src/frontend/outitf.c:1338`; `doc/codex/issues/0056` has since replaced it
with `vec_name_eq()`, which is why the easy way in is now `distinguish` and
`preserve` is not a way in at all.

## Summary

`write` with no vector list writes whatever plot is current. When the analysis
the deck just asked for did not run, nothing made a plot current, so `write`
emits the plot that was current before anything happened — on a fresh session,
the built-in constants plot — and says nothing.

Take `doc/claude/feedback/ngspice_upstream/repro/save_lower.cir`, whose `.save`
spells the net `MidNode` as `midnode`. The two streams are captured to separate
files and quoted in full below — blank lines and all, nothing cut:

```
$ ngspice -b -n -D casemode=distinguish save_lower.cir >out.txt 2>err.txt; echo rc=$?
rc=1

$ cat out.txt

Note: No compatibility mode selected!


Circuit: * folded .save spelling -- kills the run under preserve

Doing analysis at TEMP = 27.000000 and TNOM = 27.000000

Using SPARSE 1.3 as Direct Linear Solver
binary raw file "save_lower.raw"

$ cat err.txt
Warning: casemode 'distinguish' is experimental. Identifier identity is case sensitive, and a vector, a B source V() reference or an XSPICE node whose resolution misses by case is reported, but a deck that spells one net two ways becomes a deck with two nets and nothing says so, because both spellings are definitions.
Warning: no vector named 'midnode'; 'MidNode' differs only in case (casemode=distinguish)
Error: no data saved for D.C. Operating point analysis; analysis not run
doAnalyses: not found

op simulation(s) aborted
Error: incomplete or empty netlist
       or no ".plot", ".print", or ".fourier" lines in batch mode;
no simulations run!
```

The first two lines are `distinguish`'s own: the mode banner, and the near-miss
report `doc/codex/issues/0057` added, which is the one thing in this transcript
that *does* name the offending token. Neither is a defence against what
follows — the `write` still reports success on `stdout` and the file is still
written.

The last line of `out.txt` is `write` reporting success. On disk:

```
$ ls -la save_lower.raw
-rw-r--r-- 1 qflow qflow 570 Aug 13 00:21 save_lower.raw

Title: Constant values
Date: Wed Aug 12 19:28:37 UTC 2026
Command: ngspice-46+, Build Wed Aug 12 19:28:37 UTC 2026
Plotname: constants
Flags: complex
No. Variables: 12
No. Points: 1
Variables:
	0	yes	notype
	1	FALSE	notype
	2	TRUE	notype
	3	boltz	notype
	4	c	notype
	5	e	notype
	6	echarge	notype
	7	i	notype
	8	kelvin	notype
	9	no	notype
	10	pi	notype
	11	planck	notype
Binary:
```

570 bytes, well formed, and it loads without a diagnostic in *both* binaries —
`load save_lower.raw` then `print pi boltz planck` answers
`pi = 3.141593e+00,0.000000e+00`, `boltz = 1.380649e-23,0.000000e+00`,
`planck = 6.626070e-34,0.000000e+00`. A consumer that tests only for a
well-formed raw attaches it and shows the user twelve complex signals named
`yes`, `FALSE`, `TRUE`, `boltz`, `pi`, `planck` and the rest.

**Reachable with no `casemode` at all.** The trigger is a `.save` whose names
*all* miss — the guard at `outitf.c:461` tests the count of saved data, not the
individual names, so one name that resolves is enough to keep the run alive.
Measured on the baseline build with `repro/nocase.cir` (`.save v(nosuchnode)`,
no `-D`). This transcript is cut, and each cut is a command: the stderr file is
reduced by `grep` to the one line that matters, and the raw to its first four
lines by `head`:

```
$ /usr/local/bin/ngspice -b -n nocase.cir >n.out 2>n.err; echo rc=$?     # ngspice-46
rc=1
$ grep 'no data saved' n.err
Error: no data saved for D.C. Operating point analysis; analysis not run
$ ls -la nocase.raw
-rw-r--r-- 1 qflow qflow 569 Aug 13 00:21 nocase.raw
$ head -4 nocase.raw
Title: Constant values
Date: Sun Aug  2 23:29:26 UTC 2026
Command: ngspice-46, Build Sun Aug  2 23:29:26 UTC 2026
Plotname: constants
```

569 rather than 570 because `ngspice-46` is one byte shorter than `ngspice-46+`.

*One good name and the deck is safe.* `repro/save_partial_miss.cir` is the same
circuit with `.save v(nosuchnode) v(In)`, no `-D`, case-capable binary:

```
rc=0
-rw-r--r-- 1 qflow qflow 303 Aug 13 00:21 save_partial_miss.raw
Title: * .save with one bad name and one good one -- the run is not killed
Date: Thu Aug 13 00:21:23  2026
Command: ngspice-46+, Build Wed Aug 12 19:28:37 UTC 2026
Plotname: Operating Point
Flags: real
No. Variables: 2
No. Points: 1
Variables:
	0	v(in)	voltage
	1	v(all)	voltage
Binary:
```

A real plot, a real date, the run's own title. (`v(all)` is the implicit `all`
of a bare `write` on a one-vector plot and is present on a clean run too — it is
not part of this defect. It is a defect of its own, filed 2026-08-13 as
`doc/codex/issues/0064`: a wildcard matching exactly one vector is renamed to
the wildcard's own text, and the write then adds the real scale back under its
own name, so one vector is written twice.) Two bad names and no good one, `.save v(nosuchnode)
v(alsonot)`, goes back to `rc=1` and 570 bytes of constants.

No `.save` is needed at all for the constants raw, and then no circuit is
either: `printf 'write bare.raw\nquit\n' | ngspice -p -n` on a fresh session
produces the same 570 bytes.

### The two tells, and what defeats them

FINDINGS names two, and both re-measure. `Plotname: constants`, and a `Date:`
that is the build stamp rather than the run time. The second is stronger than
it was stated, because the file carries its own reference: `rawfile.c:115` and
`:117` print `pl_date` and `Spice_Build_Date` two lines apart, so on the bogus
raw the two are byte equal, and on a good one they are not — measured on the
same pair of files: `save_lower.raw` as the `-D casemode=distinguish` run above
left it, against `divider.raw` from `repro/divider.cir` (the same circuit
spelled correctly, `op` then `write`). That deck spells every name the way the
netlist does, so the flag changes nothing that matters here — 325 bytes with no
`-D`, 325 under `preserve` and 325 under `distinguish`, differing only in the
case of the three vector names, and a run-time `Date:` either way:

```
save_lower.raw   Date  = [Wed Aug 12 19:28:37 UTC 2026]
                 Build = [Wed Aug 12 19:28:37 UTC 2026]   BYTE EQUAL
divider.raw      Date  = [Thu Aug 13 00:48:05  2026]
                 Build = [Wed Aug 12 19:28:37 UTC 2026]   differ
```

Neither tell is dependable, and two measurements say so.

*The name is writable.* `set curplotname` renames the constants plot in place —
`src/frontend/options.c:384-392`, whose `eq(plot_cur->pl_name, "constants")`
guard exists only to avoid freeing a string literal. Measured:

```
$ printf 'set curplotname = "Operating Point"\nwrite rn.raw\nquit\n' | ngspice -p -n >/dev/null 2>&1
$ head -6 rn.raw
Title: Constant values
Date: Wed Aug 12 19:28:37 UTC 2026
Command: ngspice-46+, Build Wed Aug 12 19:28:37 UTC 2026
Plotname: Operating Point          <- 12 constants under an analysis name
Flags: complex
No. Variables: 12
```

*Neither tell fires when a good analysis ran earlier.*
`repro/seq.cir` — a good `op`, then `save v(midnode)`, then a second `op` that
dies the same way, then `write seq.raw` — run
`ngspice -b -n -D casemode=distinguish seq.cir`, the same flag `save_lower.cir`
needs and for the same folded spelling. (With no `-D`, and under `preserve`,
the second `op` succeeds too: `rc=0`, empty stderr, a 317-byte raw of its own
data.) Its stderr is byte-identical to the `err.txt` above (`cmp` says so, nine
lines); the raw is 341 bytes and its header reads, in full down to the `Binary:` line:

```
Title: * good op, then save with the folded spelling, then a failing op, then write
Date: Thu Aug 13 00:42:46  2026          <- the run time
Command: ngspice-46+, Build Wed Aug 12 19:28:37 UTC 2026
Plotname: Operating Point                <- plausible
Flags: real
No. Variables: 3
No. Points: 1
Variables:
	0	v(In)	voltage
	1	v(MidNode)	voltage
	2	i(Vs)	current
Binary:
```

That is the *first* op's data, written as though it were the second's, with
nothing in the file to say so. The constants plot is only the fresh-session
face of the defect; the general one is that `write` emits a stale plot.

### `rc` is not the defence FINDINGS credits it with

FINDINGS calls `rc` "the obvious defence", unavailable to a streaming consumer.
Measured, it is weaker than that: `rc` is not `write`'s report at all. Batch
mode reads the control variable `sim_status` at `src/main.c:1559` and calls
`sp_shutdown(EXIT_BAD)` at `:1593` for the whole run; `sim_status` is set per
analysis in `src/frontend/runcoms.c:320`, `:343` and `:349`. So a run whose
*last* analysis succeeds exits 0 with the bogus raw already on disk.
`repro/rc0.cir` — `save v(midnode)`, `op`, `write rc0.raw`, `save all`, `op` —
with its whole stderr and the first five lines of the raw:

```
$ ngspice -b -n -D casemode=distinguish rc0.cir >/dev/null 2>r.err; echo rc=$?
rc=0
$ cat r.err
Warning: casemode 'distinguish' is experimental. Identifier identity is case sensitive, and a vector, a B source V() reference or an XSPICE node whose resolution misses by case is reported, but a deck that spells one net two ways becomes a deck with two nets and nothing says so, because both spellings are definitions.
Warning: no vector named 'midnode'; 'MidNode' differs only in case (casemode=distinguish)
Error: no data saved for D.C. Operating point analysis; analysis not run
doAnalyses: not found

op simulation(s) aborted
Warning: no vector named 'midnode'; 'MidNode' differs only in case (casemode=distinguish)
$ ls -la rc0.raw
-rw-r--r-- 1 qflow qflow 570 Aug 13 00:21 rc0.raw
$ head -5 rc0.raw
Title: Constant values
Date: Wed Aug 12 19:28:37 UTC 2026
Command: ngspice-46+, Build Wed Aug 12 19:28:37 UTC 2026
Plotname: constants
Flags: complex
```

Note what is *not* in that stderr: the batch epilogue's
`Error: incomplete or empty netlist` never appears, because the run ended with
a successful analysis. Sourced from an interactive session the exit status is 0
too — `printf 'source rc0.cir\nquit\n' | ngspice -p -n -D casemode=distinguish`,
the flag carried onto the pipe, gives `rc=0` with zero matches for
`incomplete or empty netlist` and the same 570-byte constants `rc0.raw`; though
`quit` sets that status itself, so this half is not independent evidence.
(A `.cir` file is a netlist, not a command stream: it has to be `source`d, not
piped as commands. Drop the `-D`, or use `preserve`, and the deck simply runs:
`rc=0` with a 303-byte `Plotname: Operating Point` raw.)

What *is* a reliable in-band signal is the same `sim_status`, and it is
readable from the control language before the `write` — as is `curplot`.
`repro/status_probe.cir` echoes both after each op, under the same flag as
`rc0.cir`:

```
$ ngspice -b -n -D casemode=distinguish status_probe.cir 2>/dev/null | grep status-is
after-failed-op status-is 1 curplot-is const
after-good-op status-is 0 curplot-is op1
```

Without the flag neither op fails and the probe reads
`after-failed-op status-is 0 curplot-is op1` /
`after-good-op status-is 0 curplot-is op2`, and `preserve` now reads the same —
the deck needs `distinguish` to reach the state it is probing.

So a deck author can defend today. A raw-file consumer cannot: neither value
reaches the file.

## Impact

A consumer is handed a file that is valid in every way it knows how to check.
The failure is not that data is missing — it is that plausible data of the
wrong provenance is present, which is worse, because the consumer's error paths
never run. For xschem the user sees twelve signals with physical-constant names
where their nets should be; for the `repro/seq.cir` shape the user sees the
previous run's numbers and has no way to know they are stale.

The stderr lines are a signal, and calling `rc` the only one would be wrong.
But none of them names the file, none says the `write` that follows is of
something else, and none names the token that missed — that last half is
`doc/codex/issues/0057`, which owns the diagnostic. They also arrive on a
different stream from the `binary raw file "..."` success line that follows
them, and their order relative to it is not recoverable: stderr is unbuffered
and scrambles against the stdout line that caused it, which is why the
transcripts here split the streams instead of using `2>&1`
(`FINDINGS.md:371-372`). Splitting keeps each stream's own order intact and
discards the interleaving; there is no capture that preserves both.

Mode independent: `nocase.cir` under `fold`, `preserve` and `distinguish` gives
`rc=1` and a 570-byte `Plotname: constants` raw in all three, and `ngspice-46`
does the same at its own 569 bytes. A case mode matters only as a way in: a
`.save` card that worked under `fold` is fatal under `distinguish`, so an
existing deck starts arriving here the day the user selects that mode. It used
to be `preserve` that did this, and `doc/codex/issues/0056` was the reason;
`0056` is fixed, `preserve` folds a `.save` name again, and `distinguish` — in
which two spellings really are two names — is the only mode left that reaches
these rows by case.

No test deck reaches it: nothing under `tests/` writes a raw after an analysis
that failed. The three routes of rows 11 to 13 are less covered still — the
strings `plainwrite` and `appendwrite` do not occur anywhere under `tests/` at
all, and the two decks that do use `wrdata` name their vectors explicitly, so
no deck in the tree exercises `com_write()`'s second branch or either variant.

### Independent corroboration, 2026-08-14 — the client, from the outside

Measured by the xschem session as **R2** of
`doc/claude/feedback/reply_from_xschem_session/REPLY.md` and re-run here from
their `repro2/run_round2.sh` against `build-ver_50/src/ngspice` (build stamp
`Fri Aug 14 20:52:09 UTC 2026`) and `/usr/local/bin/ngspice`. Their deck is
`repro2/absent.cir`: the divider above, `.save v(nosuchnode)` — a token that is
in no netlist in any casing — an `.op` card, and a `.control` block that `run`s
and `write`s.

| binary / mode | rc | rawfile | mentions of `nosuchnode`, both streams |
| --- | --- | --- | --- |
| ver_50 `-D casemode=fold` | 0 | `Plotname: constants` | 0 |
| ver_50 `-D casemode=preserve` | 0 | `Plotname: constants` | 0 |
| ver_50 `-D casemode=distinguish` | 0 | `Plotname: constants` | 0 |
| **stock `ngspice-46`, no flag** | 0 | `Plotname: constants` | 0 |

Three things this settles that the file above had argued rather than shown from
a consumer's position.

- **The shape is not a `preserve` consequence and never was.** `preserve`'s
  strict `.save` was one route in; `doc/codex/issues/0056` closed that route
  and did not touch the destination. A plain typo in a `.save` card on released
  ngspice produces exit 0, no diagnostic naming anything, and a well-formed
  rawfile holding `yes`, `FALSE`, `boltz`.
- **The rc=0 is this deck's shape, not a contradiction of the rc=1 rows above.**
  The transcripts in the Summary are the `-r`-less, dot-card-less shape and are
  rc=1; `absent.cir` carries an `.op` card, so `main()`'s second batch epilogue
  arm re-runs the analysis with its own save list and exits on *that*.
  `doc/codex/issues/0069` is that mechanism, filed 2026-08-14 from the same
  report. The two rc columns are the two arms and both are correct.
- **The two tells are load bearing for a real consumer.** They are adopting
  both — reject `Plotname: constants`, reject a `Date:` equal to the build
  stamp — plus a vector-count floor, because rows 12 and 13 mean neither tell
  is sufficient alone. Nothing here asks for the withdrawn guard back: their
  own words are that the withdrawal reasoning is sound and they are not asking
  for a guard that refuses `let`-built vectors, which is row 10.

What it changes is the priority. The failing shape is not exotic, it needs no
`casemode` flag, it is reachable from a one-character typo in a deck a tool
generated, and the whole of a consumer's defence against it is two header
strings and a count.

**One byte count moved, and it is not this issue moving.** The constants raw is
**592** bytes on this build where every transcript above says 570, because
`raw_write()` gained an `Option: casemode=<mode>` line on 2026-08-14
(`doc/codex/issues/0061`, finding 1 of the client report). Stock is unchanged
at 569. The bogus file carries that line exactly as a good one does — it
records the mode, not the health of the run — so it adds nothing to the two
tells and subtracts nothing from them. Every other number above still
reproduces.

## Root Cause

`com_write()` (`src/frontend/postcoms.c:578`) given no vector list substitutes
a static `all` (`:583`) and evaluates it (`:611-614`):

```c
    static wordlist all = { "all", NULL, NULL };
    ...
        if (wl)
            names = ft_getpnames_quotes(wl, TRUE);
        else
            names = ft_getpnames_quotes(&all, TRUE);
```

**and it does so twice.** `:610` branches on the `plainwrite` variable read at
`:606`, and the arm this issue quotes is the `!plainwrite` one. The other arm,
`:633-650`, substitutes the *same* static `all` at `:635-636` and passes each
word to `vec_get()` at `:638` — no `ft_getpnames_quotes()`, no `ft_evaluate()`,
same plot, same result (row 11). Everything below about `plot_cur` applies
identically to both, which is the point: the fallback is a property of the
resolver, not of either branch.

`all` resolves through `vec_get_maybe_report()` (`src/frontend/vectors.c:766`),
which for a word with no `.` in it takes `pl = plot_cur` (`:797`), and
`findvec()` (`:166`) dispatches it at `:174-176` to `findvec_all(pl)`,
generated at `:288` as every `VF_PERMANENT` vector of `pl`. There is no
selection of a plot anywhere on that path: `write` gets `plot_cur`, whatever it
is.

`plot_cur` is the constants plot because that is its **static initial value**,
not because of a fallback:

```c
/* src/frontend/plotting/plotting.c:7 */
struct plot constantplot = {
    "Constant values", Spice_Build_Date, "constants",
    "const", NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    TRUE, FALSE, 0, 0, 0
};

struct plot *plot_cur = &constantplot;
struct plot *plot_list = &constantplot;         /* :14 -- it is in the list, too */
```

The struct's first three members are `pl_title`, `pl_date`, `pl_name`
(`src/include/ngspice/plot.h:14-16`), which is where `Title: Constant values`,
the build-stamp `Date:` and `Plotname: constants` come from. Its twelve vectors
are `predefs[]` (`src/frontend/cpitf.c:41-53`), turned into vectors by the
`com_let()` loop at `:208-212` while `plot_cur` still points here.

An analysis moves `plot_cur` through `plotInit()`
(`src/frontend/outitf.c:1138`), which stamps the real date at `:1146` and calls
`plot_setcur()` (`src/frontend/vectors.c:1352`, assigning at `:1431`) at
`:1149`. That is this path's only writer, not the program's:
`grep -rn 'plot_cur = ' src/` — with the trailing space, which is what excludes
the one comparison, `if (plot_cur == NULL)` at `vectors.c:1117` — returns 22
lines, of which four are inside `plot_setcur()`
(`vectors.c:1362`, `:1368`, `:1393`, `:1431`) and 16 are direct assignments
elsewhere — `com_fft.c:140` and `:373`, `spec.c:195`, `dotcards.c:208`, `:223`,
`:268`, `:321`, `:349`, `:385`, `:402`, `postcoms.c:1092` and `:1109`,
`com_let.c:248` and `:250`, `numparam/xpressn.c:1123` and `:1125`. The other
two are definitions: the initialiser at `plotting/plotting.c:13`, and
`ngsconvert.c:38`, which is a different program's own copy of the symbol.
None of those 20 executes in this deck: the analysis aborted and no command
touched the current plot. The list matters again in the Acceptance Criteria, because a
fix that watches only `plot_setcur()` watches four sites out of 20.

`OUTpBeginPlot()` returns before `plotInit()` is ever reached:

```c
        /* src/frontend/outitf.c:461 */
        if (numNames &&
            ((run->numData == 1 && run->refIndex != -1) ||
             (run->numData == 0 && run->refIndex == -1)))
        {
            fprintf(cp_err, "Error: no data saved for %s; analysis not run\n",
                    spice_analysis_get_description(analysisPtr->JOBtype));
            return E_NOTFOUND;
        }
        ...
            plotInit(run);                      /* :479, never reached */
```

So no plot is created, `plot_cur` is never assigned, and the value the linker
put there survives to the `write`.

**The other constants fallback is not this one, and fixing it would fix
nothing.** `vec_get_maybe_report()` retries the constants plot explicitly when a
*named* vector misses in `plot_cur`:

```c
    /* src/frontend/vectors.c:800 */
    if (pl) {
        d = vec_fromplot_maybe_report(word, pl, report_case_miss);
        if (!d)
            d = vec_fromplot_maybe_report(word, &constantplot,
                    report_case_miss);
```

That is what makes `write f.raw pi` work from any plot, and it is off this
path: `findvec_all(plot_cur)` already returns twelve vectors, so the `if (!d)`
never runs. A reader searching for "where does the constants plot come from"
finds `:803` first; the answer is `plotting.c:13`.

**A second way to the same file, with no failed analysis in it.** Because the
constants plot is also the tail of `plot_list` (`plotting.c:14`), `killplot()`
falls back to it: destroying the head of the list sets
`plot_list = pl->pl_next` and then `plot_cur = plot_list`
(`postcoms.c:1090-1092`; the inside-the-list case is `:1107-1109`).
`repro/destroy_curplot.cir` — `op`, `destroy op1`, bare `write` — measured on
the case-capable binary with no `-D`:

```
curplot-before op1
curplot-after const
binary raw file "destroy_curplot.raw"
rc=0
-rw-r--r-- 1 qflow qflow 570 Aug 13 00:21 destroy_curplot.raw
Title: Constant values
Date: Wed Aug 12 19:28:37 UTC 2026
Command: ngspice-46+, Build Wed Aug 12 19:28:37 UTC 2026
Plotname: constants
Flags: complex
```

Byte for byte the row-1 file, reached with a successful analysis, `rc=0`, and
no `.save` at all. It is the reason the fix cannot be a flag that only
`plot_setcur()` and `plotInit()` maintain — see criterion 1.

`raw_write()` already knows how to refuse — `Error: plot is empty, nothing
written.` at `src/frontend/rawfile.c:61-64`. The constants plot is not empty,
so the existing guard passes it.

## Acceptance Criteria

The measured matrix the fix has to satisfy. Every row was run at `720c8743a`;
rows 3 to 6 and row 10 are legitimate and must not move. Rows 8, 9 and 10
were added by successive rounds of adversarial verification of the withdrawn
fix, and rows 11, 12 and 13 by a further pass taken after the withdrawal; all
six are dated below. The deck each row was measured from is named in the
last column, with the flag it was run under; the rows whose decks were deleted
with the fix name the pipe that reproduces them instead.

Rows 1, 7, 8, 9 and 11 to 13 are one defect reached eight ways. Rows 1, 7 and 8
differ in how `plot_cur` came to be the constants plot; row 9 in how the
wildcard was spelled; rows 11 and 13 in **which code path resolved it** — a
second branch of `com_write()` and a second command — and row 12 in what the
resulting bytes are attached to. A fix written against one branch of one
function satisfies at most rows 1, 7, 8 and 9.

| # | request | `plot_cur` | today | wanted | deck (flag) |
| --- | --- | --- | --- | --- | --- |
| 1 | `write f.raw`, first analysis failed | `&constantplot`, never selected | 570-byte constants raw | **refuse** | `repro/save_lower.cir` (`-D casemode=distinguish`), `repro/nocase.cir` (no `-D`) |
| 2 | `write f.raw`, later analysis failed after a good one | previous run's plot | that plot, `Plotname: Operating Point`, real `Date:` | **decide** | `repro/seq.cir` (`-D casemode=distinguish`) |
| 3 | `setplot const` then `write f.raw` | `&constantplot`, selected | byte-identical to row 1 | keep | `-p` pipe, quoted below (no `-D`) |
| 4 | `write f.raw const.all` | any | 570-byte constants raw | keep | same pipe |
| 5 | `write f.raw pi boltz` | any | 264-byte, `yes` + the two named | keep | same pipe |
| 6 | `write f.raw v(MidNode)`, analysis failed | `&constantplot` | already refuses, **no file** | keep | `repro/write_named.cir` (`-D casemode=distinguish`) |
| 7 | `write f.raw` after `destroy` of the current plot | `&constantplot`, *fallen back to* | byte-identical to row 1, `rc=0` | **refuse** | `repro/destroy_curplot.cir` (no `-D`) |
| 8 | `op`, `setplot new`, `write f.raw` | the new plot, *chosen*, empty | byte-identical to row 1, `rc=0` | **refuse** | `-p` pipe: `op` / `setplot new` / `write f.raw` (no `-D`) |
| 9 | rows 1, 7 and 8 with the default argument spelled — `write f.raw all`, and equally `ALL`, `ally`, `"all"`, `all all` | as rows 1, 7, 8 | byte-identical to row 1 in all three, by every spelling | **refuse, as rows 1/7/8** | `-p` pipe, one `write` per spelling (no `-D`) |
| 10 | `let x = vector(5)` / `let y = x*2` / `write f.raw`, and the same with `all` | `&constantplot`, never selected, **holding x and y** | 3275-byte raw, 14 variables, the session's `x` and `y` at 12 and 13 | keep | `tests/regression/misc/write-let-session.cir` (no `-D`) |
| 11 | `set plainwrite` then `write f.raw` — `com_write()`'s **other branch** | as rows 1, 7, 8 | 570-byte constants raw, `cmp`-identical to row 1's | **refuse, as rows 1/7/8** | row-1 deck + `set plainwrite` (no `-D`) |
| 12 | `set appendwrite` then `write f.raw` onto an existing rawfile | as rows 1, 7, 8 | the twelve constants **appended as a second plot** behind a real run's `Title:`, `Date:` and first `Plotname:` | **refuse, as rows 1/7/8** | good-run deck, then row-1 deck + `set appendwrite` (no `-D`) |
| 13 | `wrdata f.dat all` — a **different command** on the same fallback | as rows 1, 7, 8 | 401 bytes of the twelve constants with **no header at all** | **refuse, as rows 1/7/8** | row-1 deck with `wrdata` in place of `write` (no `-D`) |

Rows 11, 12 and 13 were added 2026-08-13, by a verification pass taken *after*
the withdrawal, and they are the reason the Status section says the withdrawal
is more clearly right rather than less. All three were measured from the row-1
state — `.save v(nosuchnode)`, the `op` aborted at `outitf.c:461`, `$curplot`
echoing `const` — against `build-ver_50/src/ngspice` (build stamp
`Thu Aug 13 17:41:58 UTC 2026`), whose `com_write()`, `vec_get()` and `plotit()`
are byte-identical to `720c8743a`. None of them needs a `casemode`. The strings
`plainwrite` and `appendwrite` occur nowhere under `tests/`, and neither
occurred anywhere in this issue before these rows.

- **Row 11.** `com_write()` has two branches, and the whole of this issue up to
  now was written against one of them. `plainwrite = cp_getvar("plainwrite",
  ...)` at `postcoms.c:606` selects between them at `:610`; the `else` arm,
  `postcoms.c:633-650`, substitutes the *same* static `{ "all" }` at `:635-636`
  and passes each word straight to `vec_get()` at `:638`, never touching
  `ft_getpnames_quotes()`. Measured: `set plainwrite` in the row-1 deck gives
  `rc=1`, `curplot-is const`, and a 570-byte raw that `cmp` reports **identical**
  to the same deck's file without `plainwrite`. That the second branch is the
  one executing is proved separately, because the two branches differ on
  expressions: `set plainwrite` + `write e1.raw pi+1` prints
  `Error during 'write': vector pi+1 not found` and writes nothing, while
  without it the same request evaluates and writes a 242-byte raw. A guard
  placed where all three attempts sat — inside the `!plainwrite` arm — is
  therefore bypassed by one `set plainwrite`.
- **Row 12.** `appendwrite` (`postcoms.c:604`, passed to `raw_write()` at
  `:723`) opens the file for append instead of truncate, so the constants plot
  becomes the *second* plot of a file whose first is real. Measured as a pair:
  a good `op` writes `acc.raw` at 305 bytes,
  `Title: * a real run whose plot is written first`, `Plotname: Operating
  Point`, `Flags: real`, `No. Variables: 3`, with a run-time `Date:`; the row-1
  deck with `set appendwrite` then takes it to 875 bytes, and the appended
  header reads `Title: Constant values` / `Plotname: constants` /
  `Flags: complex` / `No. Variables: 12` with the build-stamp `Date:`. **Both
  tells of the Summary are defeated here, not weakened**: a consumer that reads
  the file's `Title:`, its `Date:` and its first `Plotname:` — which is what
  reading a rawfile's header means — sees a real run, and the twelve constants
  arrive silently behind it. This is the row that falsifies the trade sentence
  in the Resolution.
- **Row 13.** `wrdata` is `com_write_simple()` (`src/frontend/com_gnuplot.c:48`,
  registered at `src/frontend/commands.c:220`), which calls `plotit()`
  (`src/frontend/plotting/plotit.c`, resolving at `:821` and, under `set plain`,
  at `:745`) — the same `vec_get()` fallback against `plot_cur`, reached from a
  command `com_write()` cannot see. Measured from the row-1 state:
  `wrdata wd.dat all` writes 401 bytes, `rc=1`, and the file is a single line
  of twenty-five bare numeric columns carrying the twelve constants
  (`1.38064852e-23`, `2.99792458e+08`, `2.71828183e+00`, `1.60217662e-19`,
  `-2.73150000e+02`, `3.14159265e+00`, `6.62607004e-34` among them, each
  alongside the plot's scale) — with **no `Title:`, no `Plotname:`, no `Date:`,
  no header of any kind**. Nothing in it
  labels itself, so the trade argument does not merely narrow here, it fails
  outright. Unguardable from `com_write()` on two independent grounds. By
  construction: `wrdata` is registered to a different function in a different
  file and no call path from it enters `com_write()` — confirmed by reading
  `commands.c`, `com_gnuplot.c` and `plotit.c`. And by experiment, in the
  verification pass that found the row: with a guard of exactly the withdrawn
  shape compiled in, `write g_write.raw` and `write g_brace.raw {all}` were
  refused while `wrdata g_wrdata.dat all` wrote the constants anyway. (That
  build is not in the tree; the byte counts above were re-measured on the
  withdrawn tree, the compiled-guard result is carried from that pass.)
  The two decks under `tests/` that use `wrdata`
  (`tests/regression/misc/gnd-command-text.cir`,
  `tests/regression/pipe/options-keyword-case.cmd`) name their vectors
  explicitly and never reach the fallback.

Row 10 was added 2026-08-13, by the verification of the third guard, and it is
the row that ended the attempt. A plain nutmeg session that builds vectors with
`let` and writes them puts those vectors in the constants plot, because
`plot_cur` is `&constantplot` from the static initialiser until something moves
it. So rows 1 and 10 are the *same plot in the same state of chosenness*, and
differ only in what the session put there. Measured: stock
`/usr/local/bin/ngspice` and a HEAD baseline both write 3275 bytes with `x` and
`y` at indices 12 and 13; all three guards wrote nothing and printed row 1's
refusal, in `--batch` and through `ngspice -p`. The third guard extended that
to `write f.raw all` as well. Row 10 is legitimate and must not move, like rows
3 to 6.

Row 9 was added 2026-08-13, after the fix for row 8 had landed and adversarial
verification found that rows 1, 7 and 8 were all still reachable by typing the
argument the command supplies for itself. `com_write()` substitutes a static
one-word list `{ "all" }` for an omitted vector list, so `write f.raw all` is
the same operation on the same plot; the guards tested the wordlist pointer,
which is the spelling. Measured before the change: `write y.raw all` gives
`rc=1` and 570 bytes with nothing run, `rc=0` and 570 bytes after `destroy`,
and `rc=0` and 570 bytes after `setplot new` — three files byte-identical to
row 1's, with `$curplot` reading `unknown1` in the third. It is not row 4:
row 4 names the *plot* (`const.all`) and must keep writing, and does.

The row is written as "every spelling" and not as a list of tokens on purpose.
The third guard matched the single token `all`, case-folded, and verification
walked past it four ways in the same afternoon: `ally` (570 bytes),
`ALLY` (570), `"all"` with double quotes (570; single quotes *are* stripped
earlier and were refused, so the guard was inconsistent between the two
quoting forms) and `all all` (1254 bytes, the constants twice).
`get_all_type()` (`src/frontend/vectors.c:105-153`) recognises `all`, `allv`,
`alli`, `ally` and `alle`, each case-folded, and `findvec_ally()` (`:293-295`)
is "every permanent vector except the scale", which on the constants plot is
all twelve — so `ally` is a whole-plot wildcard too. Any fix has to satisfy
this row as a class, not as an enumeration; see the Resolution.

Row 8 was added 2026-08-13, after the fix for rows 1 and 7 had landed and
adversarial verification measured a state the matrix had not: a plot the deck
really did choose, with nothing in it, which is invisible to a test of how
`plot_cur` got where it is. `all` resolves nothing in an empty plot and
`vec_get()` retries each name against the constants plot, so the write changes
its subject and produces row 1's file — with a successful analysis behind it
and `rc=0`. `doc/claude/decisions/0017` decision 4 is the record. The row is
mode independent and needs no `-D`.

Row 6's deck needs the flag to reach the row at all: with no `-D`, and under
`preserve`, its `op` succeeds, `write` finds `v(MidNode)` and the deck exits 0
having written a 324-byte raw, which is a clean run and not this row. Rows 1, 2 and 6 are the
folded-spelling decks; rows 1 (`nocase.cir`), 3, 4, 5 and 7 need no `casemode`
at all, which is the mode-independence claim of the Status section restated as
coverage.

Rows 3, 4 and 5 are one pipe with no `-D` on it, all three files measured
together; `save_lower.raw` in the `cmp` is the `-D casemode=distinguish` run's
file from the Summary:

```
$ printf 'setplot const\nwrite sc.raw\nwrite pb.raw pi boltz\nwrite ca.raw const.all\nquit\n' | ngspice -p -n
-rw-r--r-- 1 qflow qflow 570 Aug 13 00:19 sc.raw
-rw-r--r-- 1 qflow qflow 264 Aug 13 00:19 pb.raw
-rw-r--r-- 1 qflow qflow 570 Aug 13 00:19 ca.raw
$ cmp sc.raw save_lower.raw && echo IDENTICAL
IDENTICAL
```

1. **Rows 1, 3 and 7 are separated by the request, not by the output.** All
   three are byte identical on disk, so no test of the plot can tell them apart.
   Two bits do:
   - `com_write()` was given no vector list — `wl == NULL` at the
     `if (wl)` of `postcoms.c:611-614`, where the static `all` is substituted;
     the earlier `if (wl)` at `:589-594` only takes the filename off the
     wordlist. Rows 4, 5 and 6 fail this bit and are already safe; row 6 shows
     the named form already has a defence (criterion 3) and writes no file.
     **This bit is not one site.** The same static `{ "all" }` is substituted a
     second time at `postcoms.c:635-636`, in the `plainwrite` arm the `:610`
     test selects, and resolved there by `vec_get()` at `:638` with no
     `ft_getpnames_quotes()` in between (row 11); and `wrdata` reaches the
     fallback through `plotit()` without passing through `com_write()` at all
     (row 13). Reading the bit at `:611-614` alone is what rows 11 and 13
     falsify, and it is why criterion 6 exists.
   - `plot_cur` was never *chosen*. Row 3 reached it through `com_splot()`
     (`postcoms.c:1216`) → `plot_setcur()` (`vectors.c:1431`); row 1 has only
     the static initialiser at `plotting.c:13`; row 7 was pushed there by
     `killplot()`'s fallback at `postcoms.c:1092`, which is neither.
     `plot_cur == &constantplot` on its own is **not** the discriminator, and
     neither is `eq(plot_cur->pl_name, "constants")` in the shape
     `options.c:386` already uses — it passes row 3 and, per the
     `set curplotname` measurement above, is defeatable in one command.

   A flag *set* by `plot_setcur()` and `plotInit()` is likewise not enough on
   its own: row 7 runs `plotInit()` first and would leave such a flag reading
   true while `plot_cur` moved underneath it. The bit has to be maintained
   wherever `plot_cur` is written — the 20 in-process sites listed in the
   Root Cause —
   set where the assignment answers a request (`plot_setcur()`, `plotInit()`,
   and the plot-creating commands in `spec.c`, `com_fft.c`, `dotcards.c`) and
   cleared where it is a fallback (`postcoms.c:1092` and `:1109`). The
   save-and-restore pairs, `com_let.c:248`/`:250` and
   `numparam/xpressn.c:1123`/`:1125`, must restore the bit with the pointer.
   Row 7 is the acceptance test for that, and it is why the flag cannot live
   only inside `plot_setcur()`.
2. **Row 2 is decided rather than inherited.** A name test misses it entirely,
   which is the argument that the condition is about the analysis outcome and
   not about the plot's identity. `sim_status` is that outcome, already exists
   (`runcoms.c:320`, `:343`, `:349`) and already reads `1` there. Refusing row 2
   changes `write` for any deck that deliberately keeps the previous plot after
   a failed run, so it needs the same treatment `doc/codex/issues/0032`
   criterion 2 gave `rawfile.c:618`: a decision and a sentence in the migration
   note, not a silent tightening.
3. **Whatever it refuses, it says so on `cp_err`**, not `stderr`, per
   `doc/claude/decisions/0001-distinguish.md` decision 2 as corrected
   2026-08-11 — otherwise the deck in criterion 4 cannot capture it. Row 6's
   existing refusal is the counter-example, and it is two messages on two
   different streams. `repro/write_named.cir` run under the flag row 6 names —
   without it the `op` succeeds and the `write` never refuses — `rc=1`, no file
   written, and the two consecutive stderr lines picked out of its eleven by the
   `grep` shown:

   ```
   $ ngspice -b -n -D casemode=distinguish write_named.cir >/dev/null 2>wn.err
   $ grep -E "checkvalid|during 'write'" wn.err
   Warning from checkvalid: vector MidNode is not available or has zero length.
   Error during 'write': no writable vector found.
   ```

   The first is `cp_err` (`parse.c:290`), the second is literal `stderr`
   (`postcoms.c:617`). Redirecting the command with `>&` — the mechanism
   criterion 4 relies on, which sets `cp_err` as well as `cp_out` — captures
   only the first. `repro/write_named_capture.cir` runs the same `write` into
   a capture file and reads the first line back, under the same flag
   (`ngspice -b -n -D casemode=distinguish write_named_capture.cir`; with no
   `-D`, and under `preserve`, the `write` succeeds and the capture holds
   `binary raw file "write_named.raw"` instead):

   ```
   captured: Warning from checkvalid: vector MidNode is not available or has zero length.
   ```

   while `Error during 'write': no writable vector found.` is still on the
   process's stderr, absent from the capture. A new refusal written the way
   `:617` is written would be invisible to a deck; it has to go to `cp_err`
   like `Error: plot is empty, nothing written.` at `rawfile.c:62`, and it
   names the file it did not write and the reason, so the two read as one
   family.
4. **A deck asserts it,** and the assertion is that the file is *absent*. The
   control language can test that directly, which is the mechanism `vlnggen`
   uses to decide whether an optional Verilator object exists before linking it
   (`src/xspice/verilog/vlnggen:327-333`, the `verilated_vcd_c.o` probe; the
   `fopen`/`if $fh < 0` at `:249-261` is the same idiom used to locate the shim
   source) and which measures clean here:

   ```
   $ printf 'set silent_fileio\nfopen fh nosuchfile.raw\nif $fh < 0\n  echo ABSENT\nend\nquit\n' | ngspice -p -n
   >   echo ABSENT
   ABSENT
   ```

   (`>   echo ABSENT` is the interpreter echoing the block's continuation line;
   `ABSENT` is the assertion.) Rows 3, 4 and 5 assert the opposite on the same
   mechanism, and row 7 asserts absence exactly as row 1 does. The refusal text
   of criterion 3 is captured with `>&` and read back, as
   `tests/regression/casedist/vector-unlet-report.cir` does — which only works
   once criterion 3's `cp_err` requirement is met. The trigger needs no
   `casemode`: `.save v(nosuchnode)` reaches row 1 in the default mode and
   `destroy` reaches row 7 with no `.save` at all, so the deck belongs in a
   mode-independent directory rather than under `tests/regression/casedist/`.
5. `make check` unchanged in all three modes. The fix touches no identifier
   comparison and must move none of them.
6. **It covers every route, not one branch of one function.** Added 2026-08-13
   with rows 11 to 13. The fallback is reachable from at least four places, and
   a fix has to be measured at each:
   - `com_write()`'s `!plainwrite` arm (`postcoms.c:610-632`) — rows 1, 7, 8, 9;
   - `com_write()`'s `plainwrite` arm (`:633-650`) — row 11, which no test deck
     in the tree exercises;
   - `com_write()` with `appendwrite` (`:604`, `:723`) — row 12, where the
     output is not a file of its own but a plot appended to somebody else's;
   - `wrdata` → `com_write_simple()` → `plotit()` — row 13, a **different
     command**, which a guard inside `com_write()` cannot reach. Established
     twice over: by reading the call path, and by experiment — with a guard of
     the withdrawn shape compiled in, `write` was refused and `wrdata … all`
     wrote the constants regardless.

   The four are two commands, and the second command is the reason the fix
   belongs in the resolver rather than in `com_write()` — see the Resolution's
   constraint 5. Any fourth attempt that cannot state where it sits relative to
   all four is not yet a fix.

## Resolution

**Withdrawn, 2026-08-13. The issue stays Open.** Three discriminators were
built, measured and shipped in one batch; each was falsified by the next round
of verification, and the third was falsified in the direction that matters —
it refused correct work. The code is out of the tree: `com_write()` is back to
its `720c8743a` text, `killplot()` with it, and the `plot_cur_chosen` pointer
that fed them is gone from `plotting.c`, `fteext.h`, `vectors.c`, `dotcards.c`,
`spec.c` and `com_fft.c`, because with no reader it was a global written at
fourteen sites and read at none. `NEWS` loses its `Bug fixes:` entry for the
same reason: nothing user-visible changed.

What follows is the record for whoever takes this next. It is the useful part
of three rounds of work and it is deliberately concrete, because the trap here
is not hard to fall into twice.

### The defect is real, is unchanged, and is wider than three attempts assumed

Rows 1, 7, 8 and 9 all still reproduce, exactly as the Summary and the matrix
measure them. `write` with no vector list still emits a 570-byte rawfile of the
twelve physical constants after an analysis that did not run, after a `destroy`
of the plot that did run, and out of a chosen-but-empty plot; and it does so by
every spelling of the wildcard. Nothing below argues that this is acceptable.

Rows 11, 12 and 13, measured after the withdrawal, add three routes the three
attempts never saw: `com_write()`'s second (`plainwrite`) branch, which
substitutes the same static `{ "all" }` and calls `vec_get()` without ever
entering `ft_getpnames_quotes()`; `appendwrite`, which puts the constants plot
inside somebody else's rawfile; and `wrdata`, a different command on the same
fallback, which a guard in `com_write()` cannot reach at all. They do not
change the verdict below — they harden it.

### Attempt 1 — "the plot was never chosen" (rows 1 and 7)

`plot_cur_chosen` recorded the plot something last *asked* to be current;
`com_write()` refused the no-vector-list branch when `plot_cur !=
plot_cur_chosen`. Correct for rows 1 and 7, and it kept rows 3, 4 and 5 —
`setplot const` and `write f.raw const.all` still wrote, which a test of the
plot's identity or name could not have managed (`set curplotname` defeats the
name test in one command).

**Falsified by row 8.** A plot the deck really did choose can be empty:
`op`, `setplot new`, `write f.raw`. `all` resolves nothing in an empty plot,
`vec_get()` retries each name against the constants plot, and the write changes
its subject and produces row 1's file — with `rc=0` and a successful analysis
behind it.

### Attempt 2 — "…or the chosen plot is empty" (row 8)

A second refusal on the same branch, `!plot_cur || !plot_cur->pl_dvecs`, with a
message of its own. Correct for row 8.

**Falsified by row 9.** Both refusals read `!wl` — "the deck typed no vector
list" — and `com_write()` substitutes a static one-word list `{ "all" }` for
exactly that case. So `write f.raw all` is the same operation on the same plot
and walked past both, in all three of rows 1, 7 and 8. The guards keyed on the
*syntax* of the request, not on its subject.

### Attempt 3 — "…and `all` counts as no vector list" (row 9)

`whole_current_plot = !wl || (!wl->wl_next && eqc(wl->wl_word, "all"))`,
computed once and read by both refusals. Case-folded, because `get_all_type()`
folds the token in every mode.

**Falsified twice.** First on completeness: `ally`, `ALLY`, `"all"` in double
quotes and `all all` each walked past all three refusals and wrote the same
570-byte constants raw (1254 bytes for the doubled form). Second, and fatally,
on correctness: it *widened* a false refusal that attempts 1 and 2 had already
introduced, onto a second spelling. That is row 10.

### Row 10 is why it came out

`let` before any analysis creates a permanent vector **in the constants plot**,
because `plot_cur` is `&constantplot` until something moves it. A nutmeg
data-processing session — the workflow `ngspice -p` exists for — therefore
computes into the constants plot and writes it, and has always been able to:

```
set filetype=ascii
let x = vector(5)
let y = x*2
write out.raw
```

Stock `/usr/local/bin/ngspice` and a HEAD baseline both write 3275 bytes,
14 variables, `x` and `y` at indices 12 and 13. All three guards printed
`Error: no plot has been selected, "out.raw" not written.` and wrote nothing,
in `--batch` and through `ngspice -p` alike; attempt 3 did the same for
`write out.raw all`.

So the premise the whole guard rested on is false. "The current plot is a plot
nothing chose" is not the same proposition as "the current plot is garbage",
because the constants plot is where a session's own work lands *precisely
when* nothing has chosen a plot. Rows 1 and 10 are the same plot in the same
state of chosenness. The discriminator has to be what is *in* the plot, and
none of the three attempts asked that.

The trade decides itself, and it decides the same way for a blunter reason than
the one first written here.

**Corrected 2026-08-13.** This paragraph used to read: "Row 1's defect produces
a file that at least labels itself — `Title: Constant values`,
`Plotname: constants`, `Date:` equal to the build stamp, which is how every
measurement in this issue identifies it." That is true of row 1 and **false of
two of the three routes measured since**, so it is not a property of the defect
and cannot carry the trade:

- Row 12 (`set appendwrite`): the constants plot is appended to an existing
  rawfile, so the file's `Title:`, its `Date:` and its **first** `Plotname:` are
  a real run's. Measured, 305 bytes of `Plotname: Operating Point` with a
  run-time `Date:`, then 875 bytes once the twelve constants are appended
  behind them. Reading the header — which is what "labels itself" means — gets
  a consumer the real run and nothing else.
- Row 13 (`wrdata f.dat all`): 401 bytes of bare numeric columns. No `Title:`,
  no `Plotname:`, no `Date:`, no header of any kind. There is nothing there to
  label anything.

So the honest statement of the trade is narrower and stronger. It is not that
the defect's output is self-labelling; it is that **the defect's output is
recoverable and the guard's is not**. The defect writes bytes that a consumer
can inspect, diff against a known-good run, or discover to be wrong at any later
point. The guard produced *no file at all* for work that is correct — a
nutmeg `let` session's own vectors, row 10 — in `--batch` and through
`ngspice -p` alike, and a silent no-op on correct input leaves nothing behind to
inspect. That is the worse failure, so the guard loses, and it would have lost
on row 10 alone.

Rows 11 to 13 make the same call easier rather than harder. A guard of the
attempted shape would have had to cover **at least four routes across two
commands** — `com_write()`'s two branches, the append variant, and `wrdata` —
and three rounds of widening had already failed to cover one of them. Widening
it a fourth time would have widened the false refusal of row 10 along with it,
which is exactly what attempt 3 did to attempt 2. **The withdrawal stands.**

### What a correct discriminator has to do

Five constraints. The first four were each falsified by an attempt; the fifth
was measured after the withdrawal, by rows 11 to 13, and it is the one that
decides *where* a fix can live. Constraints 1 to 4 are stated below in terms of
"the request" and "the plot", not in terms of `com_write()`'s `!plainwrite`
arm — that narrowing is what the three attempts inherited from each other, and
constraint 5 is the correction.

1. **Key on the subject, not the plot's provenance alone.** Refuse only when
   the plot to be written holds nothing this session put there. Rows 1, 7 and 8
   are then refused because the constants plot holds only its twelve startup
   built-ins (or, for row 8, nothing at all); row 10 is written because `x` and
   `y` are the session's. Nothing in the tree currently marks the twelve
   built-ins — they are created by `ft_cpinit()` (`src/frontend/cpitf.c`,
   the `predefs[]` loop, via `com_let()`) with no flag distinguishing them from
   a user's `let`, so this needs one.
2. **Still keep rows 3 to 5.** `setplot const` then `write` legitimately asks
   for exactly the twelve built-ins and must get them, so constraint 1 alone
   over-refuses and the provenance test (`plot_cur_chosen`, or an equivalent)
   is still needed *beside* it, not instead of it.
3. **Decide "this request is for the whole current plot" without enumerating
   spellings.** This is the hard one, and it is why the withdrawal is a
   withdrawal rather than a fourth patch:
   - Before resolution the wordlist is unparsed. `get_all_type()`
     (`vectors.c:105-153`) folds five tokens; `ft_getpnames_quotes()` strips
     double quotes *later*; `$`-expansion has already happened; and a repeated
     wildcard is still a wildcard. Matching text here means re-implementing the
     resolver, and each round of this issue found one more spelling it had not.
     It also means doing it more than once: under `plainwrite` there is no
     `ft_getpnames_quotes()` in the path at all (row 11), and under `wrdata`
     there is no `com_write()` in the path at all (row 13), so a text test in
     `com_write()`'s first arm is a test that two of the four routes never
     reach.
   - After resolution the information is gone. `write f.raw all` reached
     through `vec_get()`'s constants fallback and `write f.raw const.all`
     (row 4, which **must** write) produce the identical dvec list from the
     identical plot; nothing in the result separates them. And when the current
     plot is empty — row 8, the row this test most needs to cover — "the
     request covers the whole current plot" is vacuously true of *every*
     request, so `write f.raw pi boltz` out of a `setplot new` plot would be
     refused, which is over-refusal again.

   A workable shape is probably to have the *caller* learn from the resolver
   rather than guess ahead of it: expose whether a wildcard was expanded and
   against which plot, so a caller can ask "did this request reach the
   constants plot only by `vec_get()`'s fallback?" rather than "did the deck
   type `all`?". That is a change to `ft_getpnames_quotes()`/`vec_get()`, not
   to `com_write()`, and it is bigger than this issue as written. Constraint 5
   is the reason it is also the *only* shape that can work: `vec_get()` is the
   one point all four routes pass through.
4. **Prove it against row 10 in both drivers.** `--batch` and `ngspice -p`,
   because the `-p` route is the one a data-processing session actually uses
   and it was refused for three rounds without a deck noticing.
5. **Cover every route into the fallback, not one branch of one function.**
   Added 2026-08-13 with rows 11 to 13; Acceptance criterion 6 lists the four
   routes and their line numbers. Restated as a constraint on the fix:
   - A guard inside `com_write()`'s `!plainwrite` arm is bypassed by
     `set plainwrite`, which substitutes the same static `{ "all" }` in the
     other arm and hands it to `vec_get()` (row 11, measured `cmp`-identical to
     row 1's file).
   - A guard that reasons about "the file this command writes" is bypassed by
     `set appendwrite`, where the bytes go into a file another run titled
     (row 12).
   - A guard anywhere in `com_write()` is bypassed by `wrdata`, which reaches
     the same fallback through `plotit()` (row 13). This one is not only an
     argument from reading: a guard of the withdrawn shape was compiled in and
     measured refusing `write` while `wrdata … all` wrote the constants
     regardless.

   Together these say the fix cannot be a predicate in a command at all. It has
   to be in the resolver, where all four routes meet — which is the same
   conclusion constraint 3 reaches from the other direction, and the two
   agreeing is the strongest thing this issue currently knows.

`doc/codex/issues/0064` is adjacent and should probably be settled first: it
records that `all`, `allv`, `alli` and `ally` are four spellings of one request
and that a wildcard matching a single vector is renamed to the wildcard's own
text. Whatever the resolver learns to report will want to be reported once.

### Tests

`tests/regression/misc/write-let-session.cir` is new and pins **row 10**: a
session's `let` vectors are written by a bare `write`, by the spelled `write
f.raw all`, and by naming them, and the bare file is read back inside the deck
to assert it holds `wls_x` and `wls_y` rather than merely existing. It is RED
against every one of the three guards (`WRITE-LET-BARE-ABSENT`,
`-MISSING-X`, `-MISSING-Y`, `WRITE-LET-ALL-ABSENT`) and GREEN against stock,
against HEAD and against the withdrawn tree.

Four decks were **deleted**, because each pinned behaviour that no longer
exists and a test that asserts a withdrawn refusal is a test that blocks the
withdrawal:

- `write-unchosen-plot.cir` (attempt 1: rows 1 and 7, plus rows 3-6 as
  controls),
- `write-empty-plot.cir` (attempt 2: row 8),
- `write-spelled-all.cir` (attempt 3: row 9 for the one token `all`),
- `destroy-removed-circuit.cir` (the `killplot()` clear ordering, which existed
  only to serve attempt 1; it also asserted that
  `Internal Error: kill plot -- not in list` **is** printed, which
  `doc/codex/issues/0063` wants to stop being true, so deleting it also
  unblocks that issue's criterion 4).

Their contents are recoverable from this batch's history; the states they
covered are rows 1, 7, 8 and 9 of the matrix above, which is the durable
record. `doc/claude/decisions/0017-write-refuses-an-unchosen-plot.md` is marked
withdrawn and keeps the measurements behind decisions 1 to 5, including the
`dosim()` experiment that settled row 2 — that decision survives the
withdrawal, because it is a decision *not* to change `write`.

