# Receipt: item 4 — file the `.op`-with-no-netlist abort

Crew D, 2026-08-15, branch `ver_50`. HEAD at start `61f519208` (item 3).

**Documentation only. No file under `src/` or `tests/` was touched, nothing was
fixed, and nothing says the upstream submission has been sent.** One file added.

## What was written

`doc/codex/issues/0072-an-op-dot-card-with-no-netlist-aborts-on-an-assertion.md`

0072 was the next free number: `0071` was the highest in `doc/codex/issues/`
when this item started, and item 5 of this batch had not run.

House sections all present: Status, Summary, Impact, Root Cause, Acceptance
Criteria, Resolution, plus Related. Status is **Open, filed 2026-08-15, filing
only**. Resolution is **not started**, with the shape a fix would take and the
upstream question stated as the owner's decision.

## The minimal reproducer

Six bytes. The `.end` card is not needed; a title *line* is.

```
$ printf '*\n.op\n' > tiny.cir
$ ngspice --batch -n tiny.cir ; echo rc=$?
ngspice: ../../../src/frontend/dotcards.c:225: ft_cktcoms: Assertion `plot_cur->pl_dvecs != NULL' failed.
rc=134
```

`.op` with `\n` as the only other byte fails differently: with `.op` as the
first line it becomes the title, the deck is empty, and ngspice diagnoses it
properly (`Warning: Empty netlist!`, then `incomplete or empty netlist`,
`rc=1`).

## What was measured

Both binaries, every row. `build-ver_50/src/ngspice` = `ngspice-46+`, build
stamp `Sat Aug 15 18:18:34 UTC 2026`. `/usr/local/bin/ngspice` = `ngspice-46`
as released, build stamp `Sun Aug 2 23:29:26 UTC 2026`. Scratch under
`/tmp/claude-1000/-home-qflow-dev-ngspice-test/…/scratchpad/dotcards`; nothing
left in the repo.

**1. Exit status and streams, three runs of three each.**

```
NEW    rc=134  rc=134  rc=134
STOCK  rc=134  rc=134  rc=134
```

`stderr` is 106 bytes and byte-identical from both binaries — the assertion
line above, naming `dotcards.c:225` on both. `stdout` under a redirect is
**0 bytes**: the run gets as far as `No. of Data Rows : 1` and `abort()` does
not flush `stdio`, so everything buffered is discarded.

```
$ ngspice --batch tiny.cir > so.txt 2> se.txt ; echo rc=$?
rc=134
$ wc -c so.txt se.txt
  0 so.txt
106 se.txt
```

Under a pty the same output is line-buffered and does appear, which is why the
abort looks different by hand than under automation. With `-o log` the
assertion text lands in the log and `stderr` is empty, so the caller sees
nothing on either stream.

**2. Mode independence** (NEW only; stock has no `casemode`): `fold`,
`preserve`, `distinguish` and no `-D` are all `rc=134`.

**3. Which analyses.** Every row both binaries, identical.

| deck (title line plus these cards) | rc | reported |
| --- | --- | --- |
| `.op` | **134** | the assertion |
| `.op` + `.tran 1n 10n` | **134** | the assertion |
| `.op` + a `.control run` block | **134** | the assertion |
| `.tran 1n 10n` | 1 | `Error: incomplete or empty netlist … no simulations run!` |
| `.dc v1 0 1 .1` | 1 | the same |
| `.ac dec 10 1 1k` | 1 | the same |
| `.print tran v(1)` + `.tran` | 0 | `Error: no such vector 1` |
| `.print ac v(1)` + `.ac` | 0 | the same |
| `.print dc v(1)` + `.dc` | 1 | `Fatal error: … "v1" is not in the circuit` |
| `.four 1k v(1)` + `.tran` | 0 | `Error: no such vector 1` |
| `.tf v(1) v1` | 1 | `Warning: Transfer function source v1 not in circuit` |
| `.options noacct` only | 1 | `Error: incomplete or empty netlist …` |

`.op` alone. The same deck with `.tran` already gets the right answer, which is
the strongest single argument in the issue.

**4. It is "no non-ground node", not literally "no netlist".** With `.op`:
empty body → 134; `r1 0 0 1k` → 134; `.model m1 nmos` → 134;
`v1 1 0 1` / `r1 1 0 1k` → 0.

**5. Which routes reach it.** `grep -rn ft_cktcoms src/` finds exactly two call
sites, `main.c:1573` and `:1581`, both inside `if (ft_batchmode)`.

| route | rc |
| --- | --- |
| `ngspice --batch -n tiny.cir` | **134** |
| `ngspice -b -n < tiny.cir` | **134** |
| `ngspice -n tiny.cir < /dev/null` | **134** |
| `ngspice -b -n -r out.raw tiny.cir` | 0 |
| `ngspice -n tiny.cir` from a tty | 0 |
| `printf 'source tiny.cir\nrun\n' \| ngspice -p -n` | 0 |
| `printf 'op\n' \| ngspice -p -n` | 0, `Error: there aren't any circuits loaded.` |
| a `.control` block whose only card is `op`, no `.op` card | 0 |

Two corrections to the item's framing, both measured:

- **A `.control` block with `op` and no netlist above it does not abort.** It
  runs the operating point and exits 0 with
  `Note: Simulation executed from .control section`. The card that aborts is
  the **dot card** `.op`. `0069`'s listing aborted because it carried both.
- **Batch mode does not need `-b`.** `main.c:1175` sets `ft_batchmode` whenever
  there is no tty and no `-i`, so `ngspice deck.cir` from a `Makefile`, a CI
  job or a `subprocess.run()` is enough. An interactive tty session with the
  same deck loads the circuit and prompts.

`-r` escapes because `OUTpBeginPlot()` calls `fileInit()` instead of
`plotInit()`, so no in-memory plot exists to find. That writer's own file says
why `.op` is special:

```
$ ngspice -b -n -r r1.raw tiny.cir ; cat r1.raw
Plotname: Operating Point
No. Variables: 0
```

The same `-r` run of the `.tran` deck writes `No. Variables: 1` — the `time`
column.

**6. What the assertion asserts, traced to the caller.** Three steps:

- `DCop()` passes `refName == NULL` (`src/spicelib/analysis/dcop.c:50`), so
  `beginPlot()` takes the `else` at `outitf.c:302` and sets `refIndex = -1`.
  Every other analysis passes a reference (`dctran.c:183` passes `timeUid`), so
  every other analysis's plot has a column before any node is considered.
- **`beginPlot()`'s own "no data saved" guard excludes this case.**
  `outitf.c:479` reads
  `if (numNames && ((numData == 1 && refIndex != -1) || (numData == 0 && refIndex == -1)))`
  — the second disjunct *is* this state, but `CKTnames()` returns
  `numNames == 0` for a nodeless circuit, so the leading conjunct is false and
  the guard declines to fire. **This is the caller that was supposed to have
  stopped it.** `plotInit()` (`:1207`) then allocates the plot, links it into
  `plot_list`, and loops `for (i = 0; i < run->numData; i++)` zero times;
  `plot_alloc()` (`vectors.c:1094`) `ZERO`s the struct, so `pl_dvecs` stays
  NULL.
- `ft_cktcoms()`'s `!ft_curckt` guard (`dotcards.c:204`) passes — there *is* a
  circuit, it just has no nodes — `setcplot("op")` finds the plot, and `:225`
  asserts. The next line already NULL-tests a member of the same struct before
  dereferencing it.

**7. Whether an abort is right: no, on three measured grounds.**

- The same mistake with `.tran`/`.dc`/`.ac` is `Error: incomplete or empty
  netlist … no simulations run!` and `rc=1`, from `main.c:1589`, one branch
  away from the call that aborts.
- ngspice's established refusal for a command needing a circuit is a message
  and a return: four sites print `Error: there aren't any circuits loaded.`
  (`runcoms.c:72`, `runcoms.c:255`, `runcoms2.c:78`, `com_wr_ic.c:38`) and
  about fifteen more use `Error: no circuit loaded` variants. Measured on both
  binaries: `printf 'op\nquit 0\n' | ngspice -p -n` prints that line and exits
  0. `ft_cktcoms()` is written in that idiom at `:204` and forgets it at `:225`.
- `src/frontend/` has five live assertions. Four are in `logicexp.c` and guard
  internal invariants; this is the only one in the directory whose truth value
  is decided by a user's file. And it is **not** debug-only: `NDEBUG` appears
  nowhere in `configure.ac`, `compile_linux.sh`, `src/Makefile.am` or `m4/`,
  and `build-ver_50/src/Makefile` has `CFLAGS = -O2 -s -Wall …` with no
  `-DNDEBUG`. The released binary aborts.

**8. What in `tests/` reaches this path: nothing.** A scan for a bare `.op` dot
card (`^\s*\.op(?![a-z])`, excluding `.options`) finds eight decks —
`tests/jfet/jfet_vds-vgs.cir`, `tests/vbic/diffamp.cir`,
`tests/polezero/filt_bridge_t.cir`, `tests/filters/lowpass.cir`,
`tests/mesa/mesa11.cir`, `tests/resistance/res_partition.cir`,
`tests/resistance/res_array.cir`, and the reference `tests/filters/lowpass.out`
— all with netlists. No deck in the tree has an analysis card and an empty
netlist. `make check` was **not** re-run: this item changed no code and no
test, and the batch's baseline is item 2's 322 PASS / 0 FAIL at `731c01455`.

## What the owner must decide

1. **Where the guard goes**, and the two places are not equivalent. Relaxing
   the `numNames &&` conjunct at `outitf.c:479` stops the empty plot from being
   created at all and fixes consumers this issue has not enumerated, but
   `numNames == 0` reaches that function from analyses other than `.op`, so its
   effect elsewhere has to be measured rather than reasoned about. Replacing
   the assertion at `dotcards.c:225` with a test is narrow and obviously safe
   and leaves the empty plot in `plot_list` for everything else to meet.
   Criterion 3 of 0072 states both; a change that only deletes the assertion
   trades `SIGABRT` for `SIGSEGV` at `:226` and fails criterion 1.
2. **Here or upstream.** It reproduces byte-for-byte on `ngspice-46` as
   released, both files involved are unmodified from upstream, it has nothing
   to do with case modes, and `libngspice` cannot reach it. So the argument
   that closed `0067` here — branch work had widened a pre-existing arm's reach
   — does not apply. Resolution lists the three options: report only, report
   plus carry the patch here, or fix here only (not recommended).
3. **How a deck asserts it, because no existing harness can.** Measured:
   `tests/regression/pipe/` sees an exit status but drives `ngspice -p`, which
   never reaches `ft_cktcoms()`; the `check.sh` directories run `--batch` but
   `tests/bin/check.sh:29`-`:39` ignores ngspice's exit status entirely,
   compares `stdout` only, and this deck's `stdout` is empty before and after a
   fix because the banner lines are inside the script's `FILTER`. A pipe deck
   could `shell` out to a second ngspice, but `$program` is
   `ft_sim->simulator` (`main.c:933`), the compiled-in string `ngspice` and not
   a path — measured: invoked by absolute path, `echo $program` answers
   `ngspice` — so such a deck would test whatever is on `PATH`. Either the
   diagnostic is routed somewhere `check.sh` compares, or the fixing crew adds
   a driver. It should be costed with the fix, not discovered during it.

## What was left

- No fix, per the item.
- `doc/codex/issues/0069`'s Related entry in 0072 records that its listing was
  the accident that found this. 0069 itself was not edited — item 3 already
  corrected it and this item does not reopen it.
- Nothing was added to `doc/claude/upstream/`, and no document says the
  submission has been sent.
