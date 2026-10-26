# Receipt: item 4 — P4, the draw log `set rnddraws=<file>`

PLAN item **4 — P4: the draw log, `set rnddraws=<file>`**, from
`doc/claude/batches/2026-10-24-mc-v51/PLAN.md`. The owner approved every
recommendation under *Decisions for the owner*. For this item that means
decision 7: the field order `seed instance_path param func args z value` is
kept, `instance_path` is ngspice's flattened form, `param` is the receiving
parameter with a `#n` suffix on a repeat, and each record set is framed by a
`#` header and footer. xschem accepted all of this (LEDGER, 2026-10-24) and
treats it as a contract. That same LEDGER note, not decision 7, is the source
for `value` being the raw draw: decision 7 does not mention `value`, and the
item spec asked for the `%g`-rounded value from `eval_agauss` (falsified
item 3 below). *Corrected by follow-up 4 (P4 fixes), review issue 4b.*

Crew P4, 2026-10-26, branch `ver_51`. HEAD at start **`e955bc093`**
(`docs: receipt for ver_51 item 3, no pid seeding at load`). The code commit
is **`29940ed39`** (`Log parse-time random draws with set rnddraws=<file>`).
This receipt, the xschem fixture and the shared-library probe are the docs
commit on top of it.

**Code was touched.** 13 source files (two new), 3 Visual Studio project
files, and the rng test directory: +779/−20 in the code commit. The owner's
uncommitted `CLAUDE.md` and `tests/regression/pipe/resource-keyword-case.cmd`
were not staged or modified. `PLAN.md`, `LEDGER.md` and
`tests/lint/identity.baseline` were not edited. One extra build tree was
configured, as the task allowed: `build-ver_51-shared/` (`../configure
--with-ngshared`), untracked.

Scratch: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/p4i/`.
It holds the RED capture (`red.log`, `red/`), the build, install and check
logs, the negative controls (`neg/`), the measurements (`meas/`: `zcheck.py`,
`loop1000.cir`, the readback captures), the pre-change binary
(`bin-old/ngspice`, `eb1ea195004d2b9f5bc0`), the tested binary
(`bin-new/ngspice`), the fixture run (`fixture/`) and the probe run
(`probe/`).

## The log as shipped

```
#	rnddraws	v1	seed=1	src=setseed	offset=0
1	r.xm1.r1	r	agauss	0,1,1	-1.13965849367679	-1.13965849367679
1	r.xm2.r1	r	agauss	0,1,1	0.90407562066411662	0.90407562066411662
#	end	logged=2	unlogged_calls=0
```

(the gaps are tabs). One record set per parse: the initial load and every
`reset`, whenever the log is open at that moment.

- **Header** `#\trnddraws\tv1\tseed=N\tsrc=S\toffset=K`. `seed` and `src`
  describe the last seeding of the generator: `startup` (`com_sseed(NULL)` at
  init, or a bare `setseed` before any deck is parsed), `setseed`, `option`
  (`.option seed=`) or `checkseed` (the first control-language draw after
  `$rndseed` changed). `offset` is the number of `CombLCGTaus()` calls since
  that seeding. `offset != 0` means the rows do not start at the seed, so the
  seed tag alone does not reproduce them. `$rndseed` is not read when the
  header is written.
- **Rows** `seed instance_path param func args z value`, numbers as `%.17g`,
  `?` for anything unknown. `instance_path` is the first token of the card
  (`r.xm1.r1`, `m.xm1.msky130_fd_pr__nfet_01v8`). On an X line it is the
  instance (`xt.xin`). On a `.model` card it is the model name
  (`xm1:nshort_model.12`). On another dot card inside a subckt it is the
  subckt instance, and on a top-level `.param` it is `?`. `param` is the key
  in front of the `=` the expression is assigned to (`r`, `delvto`, `v` on a
  B line, `w` for a subckt-call argument, the `.param` name), `?` for a
  positional value such as `r1 1 0 {rv}`. A key that draws again in the same
  set gets `#2`, `#3`, …. `args` are the function's arguments, comma
  separated. `z` is `gauss1()` for agauss/gauss and `drand()` in [-1,1) for
  unif/aunif/limit, not rescaled. `value` is the function's return before the
  caller rounds anything (`eval_agauss` splices it with `%g`).
- **Footer** `#\tend\tlogged=N\tunlogged_calls=M`, then `fflush`. `M` counts
  the `CombLCGTaus()` calls between header and footer that no row accounts
  for. It is 0 in every deck measured here, sky130 included.
- `set rnddraws=<file>` closes any open log and opens `<file>` for writing,
  which truncates it. `unset rnddraws` closes it, and so do `unset_all` and
  `ngSpice_Reset()`. `pre_set rnddraws=<file>` in `.control` opens it before
  the initial load, so the load is logged with `src=startup`, and it is not
  re-run on `reset` (measured, `meas/pre.cir`).

## What changed, file by file

| file | change |
| --- | --- |
| `src/maths/misc/drawlog.c` (new, 270 lines, mostly the format comment) | The log: open/close, the per-parse begin/end, the context setters, the row writer. The repeat count is an `nghash` keyed `"instance\tparam"`, freed at every footer. Nothing in it calls the generator. |
| `src/include/ngspice/drawlog.h` (new) | Its interface. |
| `src/maths/misc/randnumb.c` | `rng_calls++` in `CombLCGTaus()`, a draw mark at the start of `gauss1()` and `drand()`, and a seed record written by `rng_note_seed()` after each existing `srand(); TausSeed();` pair (`checkseed`, both `com_sseed` branches). The pairs themselves are unchanged. `seed_from_option()` tags its `com_sseed(NULL)` as `option`. `rnd_reset_state()` also clears the record and the counters. Accessors `rng_seed_info`, `rng_ncalls`, `rng_draw_start`, `rng_deck_parsed`. |
| `src/include/ngspice/randnumb.h` | Those four declarations. |
| `src/frontend/numparam/xpressn.c`, `src/frontend/inp.c` | In each of the two copies of agauss/gauss/unif/aunif/limit, `z = <draw>; r = <the same expression with z>;` and a row if the log is on. `nupa_substitute` sets the key from the text before the `{`, and `nupa_assignment` from its LHS. |
| `src/spicelib/parser/inpptree.c` | The same split in its `gauss()`. |
| `src/frontend/numparam/spicenum.c` | `nupa_eval` copies the card's instance name into the context, never the pointer. |
| `src/frontend/inp.c` (`inp_spsource`) | `ng_drawlog_begin()` after the `pre_` commands (so after `eval_opt` and any `pre_setseed`), `ng_drawlog_end()` after `inp_evaluate_temper()` and before the `.control` commands run, and on both early `return 1` paths in between. `eval_agauss` and `inp_parse_temper_trees` set their context. |
| `src/spicelib/parser/inppas2.c` | The card context for the `gauss()` the B-source parser folds. |
| `src/frontend/options.c` | `cp_usrset`: `rnddraws` as a string opens the log, and anything else, `unset` included, closes it (with a warning for a non-string `set`). |
| `src/sharedspice.c` | `totalreset()` calls `ng_drawlog_reset()` next to `rnd_reset_state()`. |
| `src/maths/misc/Makefile.am`, `src/include/ngspice/Makefile.am`, `visualc/{vngspice,vngspice-fftw,sharedspice}.vcxproj` | Register the new files. The vcxproj edits keep the CRLF line endings. |
| `tests/regression/rng/rnddraws-{mm,order,funcs}.{cir,status,err,files}`, `Makefile.am` | Three decks, run by `check_status.sh`. Each has `.status` 0 and an empty `.err`. Their `.files` `grep` lines hold literal tabs, which the driver's `read -r verb path rest` keeps inside `rest`. The `.tsv` logs are in `CLEANFILES`. |

`./autogen.sh` was re-run after the `Makefile.am` edits, then the tree was
rebuilt and `make install` was run.

What each deck covers:

- `rnddraws-mm.cir`: two instances of a subckt whose `.param sx={agauss…}`
  becomes a `.func`. It runs `setseed 1/2/3; reset; op`, then a `reset`
  without `setseed`, `unset`, an unlogged `setseed 4` parse, a second
  `set rnddraws=` file and `setseed 5`. The `.files` assert a header per seed
  with `src=setseed offset=0`, the stale set's `offset=[1-9]…`, rows
  `1\tr.xm1.r1\tr\tagauss\t0,1,1\t…` for both instances, the footer, and the
  second file. The deck counts both files' lines with `fopen`/`fread`,
  `quit 1` unless they are 16 and 4. That is how `unset` (nothing from
  `setseed 4`) and a new record set per reset are pinned, since the driver
  has no negative `grep`.
- `rnddraws-order.cir`: a third instance in a nested subckt (`xt.x0`) ahead
  of `xm1`. Asserts the rows `r.xt.x0.r1`, `r.xm1.r1`, `r.xm2.r1`.
- `rnddraws-funcs.cir`: one draw from every path (numparam's five functions,
  `eval_agauss` on a B line keyed `v`, the parser's `gauss` in a `.model`
  with `temper` keyed `rm`/`rsh`), a repeated key (`r8`, `r` and `r#2`) and a
  global `.param` through a `.func` (`?`/`pq`). It also carries the log-off
  guard. Pass 1 (log off) must equal the values the pre-change binary drew
  within 1e-9, and pass 2 (log on) must equal pass 1 exactly. glibc-only.

## The RED failure, verbatim

The decks were written first and run through `tests/bin/check_status.sh` by
hand against the current binary `a6c67b795d990487275b` (the item-3 binary),
before any `src/` edit. Trimmed:

```
check_status: rnddraws-mm: FAILED (status)
  expected exit status 0, got 1
  invocation: /home/analog/dev/ngspice/build-ver_50/src/ngspice --batch -n /home/analog/dev/ngspice/tests/regression/rng/rnddraws-mm.cir
  stderr was:
    com_fopen() cannot open rnddraws-mm.tsv: No such file or directory
check_status: rnddraws-mm: FAILED (err)
  present rnddraws-mm.tsv: the run did not create it
  present rnddraws-mm-2.tsv: the run did not create it
  grep rnddraws-mm.tsv: no such file
  [... 12 grep lines ...]
check_status: rnddraws-mm: FAILED: status err files
rc=1
  present rnddraws-order.tsv: the run did not create it
  [... 5 grep lines ...]
check_status: rnddraws-order: FAILED: files
rc=1
  present rnddraws-funcs.tsv: the run did not create it
  [... 12 grep lines ...]
check_status: rnddraws-funcs: FAILED: files
rnddraws-funcs: max change from the old draws 1.13687E-13 log on vs off 0
rnddraws-funcs: the draws are unchanged with the log off and on
```

That is the expected reason. `rnddraws` was an inert variable, so no log was
written, and the deck's own line count found no file. The guard half of
`rnddraws-funcs` was green, as the plan says a guard must be.

One test edit after RED, which widens a pattern and drops no assertion: two
`rnddraws-funcs.files` rows expected `0\.1[0-9]*` in `args` for `b1` and
`rm`. `eval_agauss()` and the B-source parser read their literals with
`INPevaluate()`, which makes `0.1` into `0.099999999999999992`, so those two
patterns now accept any decimal (`0\.[0-9]*`). The `.files` comment says why.

Negative controls on the new binary (`neg/`): with one golden in the guard
moved by 1e-8, `rnddraws-funcs` reports `the log-off draws are not the draws
from before the log` and returns 1. With the expected line count at 17,
`rnddraws-mm` reports `should hold 4 record sets of 4 lines` and returns 1.
(A first try moved the golden by exactly 1e-9 and passed, because that is the
tolerance.)

## The production change

See the table. The core of it, shown for agauss (the other four functions
and the other two copies follow the same pattern):

```
-    return (nominal_val + stdvar * gauss1());
+    z = gauss1();
+    r = nominal_val + stdvar * z;
+    if (ng_drawlog_on())
+        ng_drawlog_row("agauss", 3, nominal_val, abs_variation, sigma, z, r);
+    return r;
```

## The two `make check` counts

| run | binary | PASS | FAIL | XFAIL | XPASS | SKIP | ERROR | `make` exit |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| targeted: `tests/regression/rng` | `e53fb551360f6cea4f69` | 9 | 0 | 0 | 0 | 0 | 0 | 0 |
| full `make check` from `build-ver_50/` | `e53fb551360f6cea4f69` | **347** | 0 | 0 | 0 | 0 | 0 automake results | **0** |

347 = item 3's 344 + this item's three decks. No existing test moved. One log
line matches `^ERROR:`: `ERROR: (internal)  tried to destroy non-existent
graph`. It is the pre-existing simulator output from
`vector-probe-report.cir`, which passes.

**Lint:** `identity lint: 264 comparisons, baseline matches` and `identity
lint: 11 comparisons, baseline matches`. The only new comparisons have a
literal operand (`eqc(var->va_name, "rnddraws")`, `ciprefix(".model", …)`).
The repeat count goes through `nghash`, so the new code adds no
runtime-against-runtime comparison.

**Binary:** `sha256sum build-ver_50/src/ngspice | cut -c1-20` →
`e53fb551360f6cea4f69`, and `stage/bin/ngspice` has the same hash after
`make install`. To build the pre-change binary for the timing, my nine
modified `.c` files were temporarily replaced by their HEAD versions (the
`randnumb.h` declarations were kept, because `drawlog.c` still compiles) and
the tree was built. The result was `eb1ea195004d2b9f5bc0`, with no
`rnddraws` string in it. Then my files were restored and touched, and the
tree was rebuilt and installed. The rebuilt binary is `cmp`-identical to the
one `make check` ran on.

## What was measured

- **z reproduces the readback** (`meas/zcheck.py`, on `rnddraws-mm.cir`'s
  logs and its `numdgt=15` prints): `1000*(1+0.01*z/sqrt(2))` against
  `@r.xmN.r1[r]` for all 10 rows, including the `offset=6` set. The worst
  difference is **4.547e-13 ohm** (bar 1e-9).
- **The same check on sky130** (the fixture deck, xschem's `zstat.py` sigma
  `g*3.356e-3/sqrt(l*w)`): `z*sigma` against `@m.xmN.msky130_fd_pr__nfet_01v8[delvto]`
  over 4 sets. The worst difference is **3.469e-18 V**.
- **Keyed diff of the two order decks** (seed 1, key = instance/param, z):

  ```
  r.xm1.r1/r    -1.13965849367679    0.90407562066411662
  r.xm2.r1/r    0.90407562066411662  1.1040224198302389
  r.xt.x0.r1/r  MISSING              -1.13965849367679
  ```

  Columns: `rnddraws-mm`, `rnddraws-order`. Inserting `xt.x0` ahead hands it
  xm1's old z and shifts xm1 and xm2 along, which is the mmB effect. The log
  makes it visible and does not change the draw order.
- **1000-sample loop, log off** (`meas/loop1000.cir`, `setseed k; reset; op`
  for k = 1..1000, two instances), run alternately three times each:

  | binary | run 1 | run 2 | run 3 |
  | --- | --- | --- | --- |
  | old `eb1ea195…` | 0.617 s | 0.601 s | 0.599 s |
  | new `e53fb551…` | 0.595 s | 0.584 s | 0.596 s |

  All six captures of the 2000 readbacks have the same md5, so old and new
  are byte-identical. Wall time is within noise. With the log **on** the loop
  took 0.605 s and 0.582 s, the readbacks are still identical to the old
  binary's, and the log has 1000 headers, all `offset=0`.
- **Bit identity, log on.** The new binary's `rnddraws-mm` readbacks with the
  log on equal the RED run's log-off readbacks from the old binary digit for
  digit (`9.919413975088428e+02` …). `rnddraws-funcs` asserts the same in
  tree for every draw path.
- **ngSpice_Reset survival**, through the shared library.
  `build-ver_51-shared/` was configured with `--with-ngshared` and built
  through `ngheavy make -j4` (`libngspice.so.0.0.15`,
  `32a0b82c1831a3f2cd42`). The probe is
  `doc/claude/batches/2026-10-24-mc-v51/probes/rnddraws_reset_probe.c`, and
  its build line is in its header. It runs Init, `set rnddraws`, `setseed 1`,
  Circ, `reset`, `ngSpice_Reset`, then Init, Circ, `set rnddraws`, `reset`,
  `setseed 1`, `reset`, `unset`. It counts descriptors on each log through
  `/proc/self/fd`. Output, verbatim:

  ```
  ok    session 1: the log is open
  ok    ngSpice_Reset closes the log
  ok    session 1 wrote two record sets (ngSpice_Circ, reset)
  ok    both sets have a footer
  ok    set 1: seed=1 src=setseed offset=0
  ok    set 2 (reset, no setseed): offset != 0
  ok    after re-init, the old log is neither open nor written
  ok    session 2: set rnddraws opens a new log
  ok    unset rnddraws closes it
  ok    session 2 wrote two closed sets
        session 1 set 2 header: seed=1	src=setseed	offset=8
        session 2 set 1 header: seed=1	src=startup	offset=8
        session 2 set 2 header: seed=1	src=setseed	offset=0
  ok    session 2 set 1: the seed record restarted (src=startup, session 1's one-parse offset)
  ok    session 2 set 2: seed=1 src=setseed offset=0
  ok    session 2's setseed 1 rows draw session 1's setseed 1 z values
  ok    the session 1 log is unchanged
  PASSED: 0 failure(s)
  ```

  Session 2's first logged set reads `src=startup` with the same offset that
  session 1 had after one parse. A seed record or counter carried over from
  session 1 would read `src=setseed` or a larger offset. The probe is not in
  `make check`, because the tree has no shared-library test harness.
- **Review of the new statics against `totalreset`.** In `drawlog.c`,
  `log_fp`, `set_open`, the set counters and `set_keys` are closed or freed by
  `ng_drawlog_reset()` → `ng_drawlog_close()` → `ng_drawlog_end()`, and the
  context strings by `clear_context()`. `unset_all` would close the log as
  well. In `randnumb.c`, `rng_calls`, `rng_calls_at_seed`, `rng_draw_mark`,
  `rng_seed`, `rng_src`, `rng_src_next` and `rng_deck_seen` are all reset in
  `rnd_reset_state()`.
- **The fixture for xschem**:
  `doc/claude/batches/2026-10-24-mc-v51/fixtures/rnddraws-sample.tsv`
  (134,691 bytes), from `fixtures/rnddraws-sample.cir`. That deck is a copy of
  xschem's `verify_readback/mmA.cir` with the same `.lib` path. The sky130 PDK
  is installed at `/home/analog/private/xschem-private/sky130A/`, so the real
  PDK was used. It has five record sets:

  | set | header | rows |
  | --- | --- | --- |
  | 1 | `seed=1 src=startup offset=0` (initial load, via `pre_set`) | 297 |
  | 2 | `seed=1 src=setseed offset=0` | 297 |
  | 3 | `seed=2 src=setseed offset=0` | 297 |
  | 4 | `seed=2 src=setseed offset=750` (reset without setseed) | 297 |
  | 5 | `seed=1 src=setseed offset=0` | 297 |

  The row bodies (all fields but the seed) of sets 1, 2 and 5 have the same
  md5, and sets 3 and 4 differ. Per set and instance there is 1 `delvto`,
  then `wint`, `lint` and `cjs` on each of the 48 binned `xmN:nshort_model.K`
  cards, which is 2 + 288 rows. There are also 7 global `?`/`sw_func_*`
  `gauss(0,…)` draws whose `value` is 0. Those are the dead terms the
  handoff's §4 describes, now visible as rows.

## What was falsified in the plan

1. **"`rng_reseed(seed, src)` … Route every existing pair through it."**
   Not done that way, because the standing constraint says `srand`+`TausSeed`
   and `com_sseed`'s seeded branch stay byte for byte. The pairs are
   untouched, and a recording call `rng_note_seed()` follows each one. The
   effect is the same.
2. **"TRNOISE/TRRANDOM/RTS and control-language draws … count toward
   `unlogged_calls`."** That cannot hold with a footer written at the end of
   the parse, as the plan requires, because those draws happen after the
   footer. `unlogged_calls` counts the uniform calls inside the parse window
   that no row accounts for. A draw after the footer shows up in the next
   set's `offset`, unless a re-seed comes first. RTS noise
   (`vsrcacct.c`/`isrcacct.c`, through `exprand()`) does move `offset`.
   TRNOISE draws from the Wallace pool and the integer generator
   (`CombLCGTausInt`), so it does not.
3. **"For `eval_agauss` [value] is the `%g`-rounded value that is actually
   spliced."** Superseded by the contract xschem accepted (`value` = the raw
   draw). The row has the unrounded return, and the deck gets the `%g` text
   as before. A consumer that wants the spliced value must round it the same
   way.
4. **"the log-off guard is a fourth deck"** with goldens printed at
   `numdgt=15`. It is folded into `rnddraws-funcs.cir` (pass 1) and asserted
   numerically, because `check_status.sh` does not compare stdout. It covers
   every draw path rather than the mm loop alone. The mm loop's log-off
   identity is the 1000-sample measurement above.
5. **"No in-tree harness exists for the shared build. The receipt says so."**
   Still true, and the receipt says so. With the task's permission the probe
   was also run on a real `--with-ngshared` build, so the reset behaviour is
   measured rather than only reviewed.
6. **The scope's file list was complete.** One addition: the three
   `visualc/*.vcxproj` files, so that MSVC builds link `drawlog.c`. Those
   edits were not built.
7. **Size.** About 180-250 lines was planned. The executable lines come to
   roughly that. `drawlog.c` is 270 lines because a third of it is the format
   comment.

## What was left, deliberately

- No `mcdraws` command, as the plan says.
- Draws whose context means little get honest fallbacks rather than guesses:
  a positional value (`r1 1 0 {rv}`) has `param` `?`, and a global `.param`
  has `instance` `?`. In sky130, model-card draws are keyed by the binned
  model name, not the device that uses it.
- `eval_agauss`'s existing quirks are unchanged: it sweeps once per function
  name, it skips non-literal arguments, and it rounds to `%g`.
- A parse that fails between header and footer closes its set on the two
  early `return 1` paths in `inp_spsource`. A deeper `controlled_exit`
  leaves the set unterminated. The next `begin`, `close` or `unset` writes the
  footer.
- The `.measure` path calls `nupa_eval` after the run, outside any record
  set. Such draws are not logged and not counted.

## For the owner

1. **The contract as shipped matches decision 7**, plus these details xschem
   should hear in item 6's reply:
   - `src=` takes the values `startup|setseed|option|checkseed`.
   - `offset` counts uniform-generator calls.
   - `unlogged_calls` covers the parse window only.
   - `value` is unrounded.
   - `?` is used for a positional value and a global `.param`.
   - `.model` rows are keyed by model name.
   - `pre_set rnddraws=` logs the initial load without being re-run by
     `reset`.
   - `set rnddraws=` truncates the file.
2. **The fixture is committed** at `fixtures/rnddraws-sample.tsv`, with the
   deck that made it. xschem's Q7 can use it as is.
3. **Bit identity holds**, with the log off and on: in tree for every draw
   path (glibc), and over a 1000-sample loop in the receipt.
4. **`build-ver_51-shared/` is left in place**, untracked, for the next crew
   or for deletion.
5. `rnddraws-funcs.cir` is glibc-dependent in its guard half, like
   `setseed7-bit-identity.cir`.

Nothing was pushed. Two commits on `ver_51`: `29940ed39` (code and tests)
and the docs commit with this receipt, the fixture and the probe.

## Review round 1

- **Crew:** item 4 fix crew. **Date:** 2026-10-26. **Branch:** `ver_51`.
  **HEAD at start:** `8ce1ea5e9`.
- **Finding (blocking), accepted.** `totalreset()` in `src/sharedspice.c`
  called `rnd_reset_state()`, which zeroes `rng_calls`, before
  `ng_drawlog_reset()`. When a parse aborts through `controlled_exit()` (an
  undefined parameter in numparam, `spicenum.c`), its record set is still
  open. The footer that `ng_drawlog_reset()` → `ng_drawlog_end()` then
  writes computes `rng_ncalls() - set_start_calls - set_logged_calls` from
  a zeroed counter, and the unsigned result wraps. The earlier static review
  ("closed or zeroed") checked that each static is reset, but not the order
  in which they are reset.

### What changed

- `src/sharedspice.c`: `ng_drawlog_reset()` now runs before
  `rnd_reset_state()`, with a comment saying why. No other change. The
  subtraction in `drawlog.c` is left as it is, because with this order it
  can no longer see a counter that went backwards.
- `doc/claude/batches/2026-10-24-mc-v51/probes/rnddraws_reset_probe.c`:
  a third session. Init, `setseed 1`, Circ (the good deck, so the set does
  not start at call 0), `set rnddraws=probe-d.tsv`, Circ (a deck with an
  `agauss` and `r={nosuchparam*2}`), `ngSpice_Reset`. It checks that the
  log is closed and that the footer reads exactly
  `logged=1	unlogged_calls=0`.

### RED evidence

The extended probe was run against the unchanged `build-ver_51-shared`
library (`libngspice.so.0.0.15` sha `32a0b82c1831a3f2cd42`, the item-4
build). Trimmed:

```
EXIT 1
ok    session 3: ngSpice_Reset after an aborted parse closes the log
      session 3 footer: #	end	logged=1	unlogged_calls=18446744073709544674
FAIL  session 3: the aborted set's footer has logged=1 and a small unlogged_calls
FAILED: 1 failure(s)
```

(`EXIT 1` is the library's exit callback from the aborted parse.) After the
fix, rebuilt with `ngheavy make -j4` (library sha `5c5a899d86a5d96bf23f`),
the check was tightened to the exact value. All 19 checks pass:

```
ok    session 3: ngSpice_Reset after an aborted parse closes the log
      session 3 footer: #	end	logged=1	unlogged_calls=0
ok    session 3: the aborted set's footer reads logged=1 unlogged_calls=0
PASSED: 0 failure(s)
```

Sessions 1 and 2 print the same lines as before.

### Checks

- `build-ver_50` rebuilt and reinstalled. `ngspice` sha256 is
  `e53fb551360f6cea4f69`, unchanged, because `sharedspice.c` is linked only
  into libngspice.
- `tests/regression/rng`: 9 PASS / 0 FAIL.
- Full `make check`: 347 PASS / 0 FAIL / 0 XFAIL / 0 XPASS / 0 SKIP /
  0 ERROR, make exit 0. The single `^ERROR:` line is the known
  `tried to destroy non-existent graph` output.
- Identity lint: 264 comparisons, baseline matches; 11 comparisons, baseline
  matches. The fix adds no comparisons.

### For the owner

- The probe is still outside `make check`, because the tree has no
  shared-library test harness. A session that leaves a record set open across
  `ngSpice_Reset` is now covered there.
- In the standalone `ngspice` binary, `controlled_exit` ends the process, so
  this ordering can't happen there.

Commits this round: `a269a8a54` (fix + probe) and this receipt update.
