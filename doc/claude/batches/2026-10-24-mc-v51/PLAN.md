# Batch plan — Monte Carlo and sensitivity support for ver_51

**Date** 2026-10-24. **Branch** `ver_50` (see decision 1). **HEAD at start**
`ccebdf2a2`. **Baseline** not yet measured; the first crew to touch code takes
it from `build-ver_50/` before its RED step.

Source: the xschem session's handoff,
`/home/analog/private/xschem-private/doc/claude/mc_spike/NGSPICE_V51_HANDOFF.md`,
§3 (the changes), §4 (the do-nots) and §5 (the questions). Three read-only
crews checked every claim in §3 against the source. Their verdicts, with
spot checks of the ones that carry the most weight, are in
`receipts/00-scoping.md`. This plan follows from that receipt, and it differs
from the handoff in the ways listed under *Decisions for the owner*.

## The gap this batch closes

xschem runs Monte Carlo on sky130 with one long-lived ngspice per core, each
running `setseed k; reset; op|tran; write; destroy all` in a loop. The loop is
correct and fast. Around it there are one wrong result, two seeding traps and one
missing capability. All four were re-measured on
`build-ver_50/stage/bin/ngspice` during scoping:

- **A resistor converted to a B-source loses its `.model`.** At 125 °C,
  `R1 a 0 rm r={73k*(1+1e-9*abs(v(a)))}` with `.model rm r tc1=-2.47e-3
  tc2=2.7e-6 tnom=25` reads `v(a) = 7.300000e-02` under 1 µA. The plain
  `R2 b 0 rm r=73k` reads `5.694000e-02`, so the converted one is 28% off.
  `inpcom.c:7425` builds the B line from the equation, the instance `tc1=/tc2=`
  and `m` only.
- **`.option seed=` beats `setseed`.** With `.option seed=5`, `setseed 1; reset;
  op` and `setseed 2; reset; op` both read `@r3[r] = 9.312134e+02`, and nothing
  warns. `eval_opt` (`inp.c:442-466`) re-seeds on every parse, and `reset` is
  a parse.
- **The first parse is seeded from the process id.** The same deck run twice,
  with no `setseed`, read `918.377` and then `1147.4` before `setseed 1`, and
  `886.034` both times after it. `initw()` (`wallace.c:83-85`) runs
  `srand(getpid()); TausSeed();` after `main.c:936-938` has seeded from
  `rndseed=1`, and leaves `$rndseed` reading 1.
- **No record of the draws.** Sensitivity analysis has to rebuild each random
  input by reading the device parameters back and inverting each PDK formula,
  one device class at a time.

## Decisions for the owner

Nothing below is decided. Each item names the recommendation the crews and the
synthesis converged on, so a crew can start once the owner says yes or no.

1. **Is `ver_51` a new branch off `ver_50`?** This is the handoff's Q1, and
   only the owner can answer it. Nothing has been created. Every item is written
   so it can land on whichever branch the owner names. Until then crews work on
   `ver_50` and do not commit.
2. **P1 noise (handoff Q4).** Recommendation: keep the converted resistor's
   thermal noise behind the existing switch (`set enable_noisy_r` before
   `source`, or `noisy=1` on the instance), and when the switch is on, give the
   noise branch the model. The alternative, noise on whenever a model resolves,
   would silently change `.noise` for every sky130 `xhigh_po` user. It would make
   noise depend on whether a model token is present, and it would add three
   elements per converted R. Owner: yes or no.
3. **P1, the optional `300.15` → `CKTnomTemp` change in ASRC.**
   Recommendation: **decline**. It contradicts the parse-time `dtemp={27-tnom}`
   offset P1 emits, because `option tnom` can be set after the parse. It also
   changes existing B-source `tc` results for decks with `tnom≠27`. If a tnom
   reference is ever wanted, the right change is a new ASRC `tnom` instance
   parameter, in a later batch.
4. **P1, `temper` already in the expression.** The handoff's INFERRED rule says
   not to apply the model TC when the expression uses `temper`.
   Recommendation: **apply the TC and warn**. `b_transformation_wanted`
   (`inpcom.c:6354`) converts any R whose value mentions `temper`, so a skip
   would drop a legitimate model TC, which is the bug being fixed. A PDK-side
   patch that puts a temper factor into the expression (xschem's `fixp.cir`) must
   be reverted when ver_51 is adopted.
5. **P2: warn, or let `setseed` win.** Recommendation: **warn only**, once per
   session, and only after an explicit `setseed N`. Letting `setseed` win
   changes long-standing upstream behaviour and belongs with `mcrun` (item 5).
6. **P3: how far.** Recommendation: remove the pid seeding from `initw` **and**
   re-seed from `$rndseed` straight after `initw` returns. Then the startup
   stream equals `setseed 1`, `$rndseed` tells the truth, and a `setseed` in
   `.spiceinit` is honoured. A side effect: TRNOISE becomes deterministic per
   process, where today it is pid-random on 45.2 too.
7. **P4: changes to the log contract.** The handoff calls the field order and
   the seed tag a contract. The field order is **kept**:
   `seed instance_path param func args z value`. What changes:
   - The values of two fields differ from the handoff's examples.
     `instance_path` is ngspice's flattened form, e.g.
     `m.xm1.msky130_fd_pr__nfet_01v8`, with the device letter first.
     `param` is the device or model parameter the brace is assigned to (`delvto`,
     `r`, or `i` on a converted B line), never a `.func` name such as `rbody`,
     because those are expanded away as text before any draw happens. A key that
     draws more than once gets a `#n` suffix (`delvto#2`).
   - `#` header and footer lines are added, one pair per record set.
   - There are no rows for transient-time or control-language draws. The footer
     counts them instead.

   xschem has to agree to these before item 4 lands. The reply to the xschem
   session states them.
8. **Upstream.** None of this goes upstream in this batch, and nothing is to be
   described as sent. P2 and P3 are upstream-shaped, so if the owner wants patches
   prepared, that is a follow-up batch.

## Standing constraints

- **Builds go through `ngheavy`, with `-j4`.** That means `ngheavy make -j4`
  and `ngheavy make check`, and `ngheavy` before any run of the binary on more
  than a trivial deck.
- **Crews are serialized on `build-ver_50/`.** One crew builds or tests at a
  time. No worktree build crews: no `EnterWorktree`, and no second
  configured tree.
- **The uncommitted edits to `CLAUDE.md` and
  `tests/regression/pipe/resource-keyword-case.cmd` stay untouched.** No crew
  stages, reverts, reformats or commits them. Every `git add` names its paths
  explicitly. `git add -A`, `git add .` and `git commit -a` are forbidden.
- **Nothing is sent upstream.** Do not open `doc/claude/upstream/`.
- **Respect §4 of the handoff.** `srand`+`TausSeed` and `com_sseed`'s seeded
  branch stay byte for byte as they are. `setseed 7` draws stay bit-identical
  with 45.2: `@r1[r] = 9.068279585256480e+02` in the scoping crew's
  `s7.cir`. No solver, OpenMP, parser-speed or raw-writer work. No sky130
  `.lib mc` fixes, and no redrawing without a re-source.
- **ngSpice_Reset rule.** Every new static (P2's flags, P4's `FILE *`, context
  strings, seed record and call counter) is cleared or closed in `totalreset`
  (`sharedspice.c:2552ff`). P1 state lives on the stack and is freed on every
  return path of `inp_readall`, including `1497-1499` and `1552-1554`.
- **Identity lint.** No new runtime-vs-runtime `strcmp`/`cieq`/`eq`/`eqc`.
  Model names are matched through `model_name_match()`, and parameter names are
  searched for against literals. `tests/lint/identity.baseline` stays unchanged,
  or the receipt justifies the change.
- **RED first.** Quote the failure verbatim in the receipt, then make the
  smallest production change. Never weaken an assertion or regenerate an `.out`
  to hide a failure.
- **Two `make check` runs per code item.** Run the targeted directory, then the
  full `make check` from `build-ver_50/`, then `make install`. Record both counts
  as a two-row table in the receipt.
- Re-run `./autogen.sh` after any `Makefile.am` or `configure.ac` change. A
  new test directory goes before `tests/lint` in `SUBDIRS`.
- **Tests that span a `reset` keep their vectors in the `const` plot**
  (`let const.a = …`). Measured during scoping: a vector made with plain `let`
  is gone after `reset`. `if a = b` then raises `Warning from checkvalid: vector
  a is not available` and takes the else branch, so the test fails for the wrong
  reason, now and after the fix.
- Receipts go in `receipts/NN-slug.md`, in the house shape: PLAN item, branch,
  HEAD at start, whether code was touched, *The RED failure, verbatim*, *The
  production change*, *The two `make check` counts*, *What was measured*, *What
  was left, deliberately*, *For the owner*.

## Items

### 1 — P1: carry the R model through the R→B rewrite

**Scope.** All changes are in `src/frontend/inpcom.c`. The ASRC device is not
changed.

- **Model table.** Before `inp_compat` (`:1574`), and only when some R card
  needs the rewrite and has a model token, collect the `.model <name> r` cards
  into the per-scope lists that `inp_add_levels` created. To do this, factor the
  collection loop out of `inp_rem_unused_models` (`:10111-10158`) into a helper.
  Free the lists with a new free-only walk, because `rem_unused_xxx` would
  comment models out. A pre-pass is needed because sky130 defines the model
  *after* the instance, and the existing lists are freed before `inp_compat` and
  never built when the deck has `.if` (`:1504`).
- **Token.** After `node2`, skip one optional value token (a number or a whole
  brace group), then take the next token if `is_a_modelname()` accepts it. A
  card without a model token gets **exactly today's line**, byte for byte.
- **Lookup.** Walk `card->level` outwards and accept only exact matches
  (`model_name_match()==1`). Binned `name.N` matches do not count, because R has
  no binning. If several exact matches in the winning scope disagree (`.if` and
  `.else` branches, since `inp_dot_if` runs later), carry nothing. If nothing is
  found, carry nothing. Both cases print a warning:
  `behavioral R <name>: model <m> not resolved, tc1/tc2/tnom not applied`.
- **Extraction** is textual, because numparam has not run yet. Take
  `tc1|tc1r`, `tc2|tc2r` and `tnom` from the model, either as a number or as a
  `{expr}`. Instance `tc1`/`tc`/`tc2` override the model per parameter
  (`restemp.c:87-95`). If the model has `tce`, warn that it cannot be expressed.
  If the model text uses `temper`, warn and skip it.
- **Temperature reference.** When the model has `tnom`, emit
  `dtemp={<inst dtemp|0>+27-(<tnom>)}`. If the instance gives `temp=`, emit
  `temp={<T>+27-(<tnom>)}` instead, because `asrctemp.c:28-36` ignores dtemp
  when temp is given. Without `tnom`, pass the instance `dtemp=`/`temp=`
  through. That fixes today's silent drop, which also happens with no model.
  Under `.option tnom≠27`, a model without `tnom` keeps the 27 °C reference.
  Document that.
- **Noise** (decision 2). The gate at `:7417-7422` is unchanged. When it is on
  and a model resolved, the 1-ohm R of the noise branch becomes `r<name>_2
  <name>_3 0 1.0 <model> <inst tc> <inst dtemp|temp>`. The real resistor device
  then applies the TC, the tnom and the noise temperature. Flicker noise cannot
  be reproduced. Document that.
- **Double TC** (decision 4). When the expression uses `temper` and a model TC
  resolves, apply the TC and print a one-line warning.

**RED-first test.** `tests/regression/temper/temper-res-behav-1.cir`, run by
`check.sh`, following the pattern of `temper-res-1.cir`. Add it to `TESTS`. If
it uses a companion `.lib`, add that to `EXTRA_DIST`. Each pair is 1 µA into a
converted R with a model, against a reference:

| pair | converted R with a model | reference |
| --- | --- | --- |
| A | top level | the same expression × `(1+tc1*(temper-25)+tc2*(temper-25)^2)`, no model |
| B | in a subckt, model defined after the instance, `tc1={…*rsq}` from a global `.param` | as A |
| C | model pulled in from a `.lib file sec` section | as A |
| D | instance `dtemp=10` | plain model R with `dtemp=10` |
| E | instance `temp=50` | plain model R with `temp=50` |
| F | instance `tc1` overriding, model `tc2` kept | plain model R with the same instance `tc1` |
| G | model without `tnom` | plain model R |

Loop `foreach tk 0 25 125` / `reset` / `set temp=$tk` / `op`. Keep the running
`vecmax(abs(ratio-1))` in the `const` plot, and print `INFO: success` only if
every ratio is within 1e-9. Today the converted R is flat with temperature, so
the success line is missing from the diff. The 0.073-vs-0.05694 measurement
above is the expected failure. The new warnings go to stderr, which `check.sh`
does not see. Asserting them is optional: a `check_status.sh` deck with a
`.err`.

**Acceptance.** The new deck passes. The committed R→B decks
(`tests/regression/misc/asrc-tc-2.cir` and
`tests/regression/case/{compat-r-behav-case,behav-r-vnode-case}{,-lower}.cir`)
are unchanged and still pass, because none of them has a model token. Both
`make check` counts are recorded. `listing e` of pair B is in the receipt,
showing the `tc1={…}`/`dtemp={…}` text on the B line. The handoff's
`expect.py` ≤1e-6 criterion is **replaced** by this deck (see `receipts/00-scoping.md`).

**Files.** `src/frontend/inpcom.c`, about 130-180 lines.
`tests/regression/temper/{temper-res-behav-1.cir,.out,Makefile.am}`, plus the
optional `.lib`.

**Risks.**
- Real results change for every sky130 `xhigh_po` deck. That is the purpose, and
  the release note must say so.
- A value-first card `R n1 n2 {eq} mname` could be mis-tokenized. Tokenize with
  brace awareness and gate on `is_a_modelname`.
- If an outer-scope model's `{expr}` names a param that the subckt shadows, it
  binds to the inner one. This is rare. Note it in the receipt.
- `eval_tc` (`:6509`) runs `cistrstr` over the whole line, equation included.
  Do not widen that.
- The noise-branch rework must keep the noise-off line identical to today's.

### 2 — P2: warn when `.option seed=` overrides `setseed`

**Scope.** In `src/maths/misc/randnumb.c`, add file-scope `setseed_used` and
`seed_override_warned` flags. Set `setseed_used` in `com_sseed`'s
explicit-value branch only, and leave the `srand`/`TausSeed`/`cp_vset` lines
untouched. Add `seed_from_option(int seed, bool is_random)`, declared in
`ngspice/randnumb.h`. It does exactly the `cp_vset("rndseed")` + `com_sseed(NULL)`
pair that `eval_opt` does today. Before that pair it prints, once, to `cp_err`,
if `setseed_used`:

> Warning: '.option seed=N' re-seeds the random number generator on every
> 'reset' and overrides 'setseed' …

The `seed=random` variant has its own wording, which must not claim the run is
unrepeatable: the seed is in `$rndseed`. `eval_opt` (`inp.c:452-456, 465-466`)
calls the helper. Add `rnd_reset_state()`, which clears both flags, and call it
from `totalreset` next to `destroy_wallace`. A bare `setseed` with no argument
does not set the flag. Document that.

**RED-first test.** A new directory `tests/regression/rng/` driven by
`tests/bin/check_status.sh`, because `check.sh`'s `FILTER` deletes every line
containing `Warning` (`check.sh:20`) and `check_status.sh` deliberately keeps them
(`:156-159`). Register it in `configure.ac` `AC_CONFIG_FILES`, add it to
`tests/regression/Makefile.am` `SUBDIRS`, then run `./autogen.sh`. Three decks,
each with `.status` 0:

- `option-seed-setseed.cir`: `.param rv=agauss(1k,100,1)`, `.option seed=5`,
  then `foreach k 1 2 3` / `setseed $k` / `reset` / `op`. Its `.err` holds the
  warning **exactly once**. That fails today because stderr is empty, and it
  also fails if the warning is printed on every reset. The deck should also
  assert that the option still wins (the k=1 and k=2 readbacks are equal, kept
  in `const`, otherwise `quit 1`), so the warn-only behaviour is pinned.
- `option-seed-nosetseed.cir`: the same, with `reset; op` only. Its `.err` is
  empty. This is the false-positive guard, and it is green today.
- `option-seed-absent.cir`: the `setseed` loop without the option. Its `.err`
  is empty, and it is green today.

**Acceptance.** All three pass. The sequence of draws is unchanged: compare the
`@r[r]` readback of the first deck before and after the change at
`numdgt=15`. Both `make check` counts are recorded.

**Files.** `src/maths/misc/randnumb.c`, `src/include/ngspice/randnumb.h`,
`src/frontend/inp.c`, `src/sharedspice.c`, about 25 lines in all.
`tests/regression/rng/*`, `tests/regression/Makefile.am` and `configure.ac`.

**Risks.**
- In the shared build, the flags must be cleared in `totalreset`, or the second
  session never warns.
- `eval_opt`'s existing `Multiple 'option seed'` warning repeats on every reset.
  Do not copy that pattern. Fixing the existing one is out of scope.
- Upstream spells `eval_opt` with `strstr`/`eq` where this branch has the
  case-lint `cistrstr`/`eqc`. Note that for any later upstream patch.

### 3 — P3: stop seeding from the process id at load

**Scope.** Delete `srand(getpid()); TausSeed();` from `initw`
(`wallace.c:83-85`) and leave a comment saying that `com_sseed(NULL)` has
already seeded the generator. Call `com_sseed(NULL)` again immediately after
`initw()` at `main.c:1371` and `sharedspice.c:1080` (decision 6).
`com_sseed` and `TausSeed` are untouched. `tclspice.c:2497` is out of scope.

**RED-first test.** In `tests/regression/rng/`, add
`startup-equals-setseed1.cir` with `.status` 0. The deck runs `op`, then
`let const.a = @r1[r]`, then `setseed 1`, `reset`, `op`, `let b = @r1[r]`, and
`quit 1` unless `const.a = b`. Today the startup value is pid-seeded, so the deck
fails at any pid. This was measured: `918.377` and `1147.4` against `886.034`. A
companion deck with the same comparison through a `const` vector set up
`setseed 1` twice reads `SAME` today. That proves the comparison itself can
pass. The test is libc-independent because it compares two values within one
process.

Two more decks, green today and kept green, which guard bit identity:
- `setseed 7` + `reset` + `op` at `numdgt=15` prints
  `@r1[r] = 9.068279585256480e+02`, which is the 45.2 value. That value depends
  on glibc. Mark the deck Linux-only in its header comment.
- The handoff's cross-process check, the same deck run twice, goes into the
  receipt as a measurement rather than a test, because no in-tree driver runs a
  deck twice.

**Acceptance.** The RED deck passes, the `setseed 7` guard is unchanged, and
both `make check` counts are recorded. The receipt also measures that a
TRNOISE deck (the scoping crew's `tn.cir`) is now identical across two
processes.

**Files.** `src/frontend/trannoise/wallace.c`, `src/main.c`,
`src/sharedspice.c`, 4-6 lines in all, plus `tests/regression/rng/*`.

**Risks.**
- Every deck that never calls `setseed` now draws deterministically. Anyone who
  relied on runs differing must use `.option seed=random` or unset `rndseed`.
  The commit message must say so.
- libc `rand()` now runs deterministically from startup, so `d_process` and
  `dense.c` see a fixed sequence. That is harmless.
- Not fixed, and recorded for xschem: `setseed` does not refill the Wallace pool,
  so TRNOISE sample k still depends on the samples before it in a loop.
  `checkseed`'s `oldseed` and `gauss0`'s cache also survive `setseed` and
  `ngSpice_Reset` (`randnumb.c:80, 197-198`). Both existed before this batch.

### 4 — P4: the draw log, `set rnddraws=<file>`

**Scope.** Add `src/maths/misc/drawlog.c` and `src/include/ngspice/drawlog.h`,
and register them in `src/maths/misc/Makefile.am`. Only the variable is
shipped, with no `mcdraws` command, because a command adds a `commands.c` entry
and `com_*` churn for no extra capability.

- **Enable.** Handle it in `cp_usrset` (`options.c:322`), which already runs on
  every `set` and on every `unset`, `unset_all` included. `set rnddraws=f`
  closes any open log and opens `f` for writing. `unset` closes it. No path is
  compared with another path, so the identity lint is not involved, and
  `totalreset` closes the log through `unset_all`. `totalreset` also calls
  `ng_drawlog_close()` explicitly and clears the context.
- **Seed tag.** Add `rng_reseed(seed, src)`, which runs exactly `srand(seed);
  TausSeed();` and then records the seed, its source and `rng_calls = 0`. Route
  every existing pair through it: `com_sseed` both branches, `checkseed`, and
  whatever item 3 leaves of `initw`. `rng_calls` is a plain counter in
  `CombLCGTaus`. `$rndseed` is never read at draw time.
- **Record set.** At the start of each `inp_spsource`, which covers the initial
  load and every `reset`, write the header
  `#\trnddraws\tv1\tseed=<n>\tsrc=<startup|setseed|option|checkseed>\toffset=<rng_calls>`.
  At the end of the parse, write `#\tend\tlogged=<n>\tunlogged_calls=<m>`,
  then `fflush`. A non-zero `offset` means a reset without `setseed`, so the tag
  is stale.
- **Rows.** `seed\tinstance_path\tparam\tfunc\targs\tz\tvalue`, with numbers
  written as `%.17g` and `?` for anything unknown. In each of the three copies of
  the helpers (`xpressn.c:47-86`, `inp.c:2513-2552`, `inpptree.c:1695-1702`,
  which has gauss only), split the expression into `z = gauss1(); r = nominal +
  stdvar*z;` (the same arithmetic) and write the row only if the log is open.
  `limit` is included. For uniform draws, `z` is `drand()` in [-1,1), and that is
  documented rather than rescaled. `value` is the random function's return. For
  `eval_agauss` it is the `%g`-rounded value that is actually spliced.
- **Context**, captured only when the log is open:
  - `nupa_eval` (`spicenum.c:701`) copies the first token of the card line. It
    copies the token, never the pointer, because `insertnumber` can free the
    line (`xpressn.c:1426-1435`). `X` cards use `inst_name[stack_depth]`.
  - `nupa_substitute` scans back from the `{` for `key =`.
  - `nupa_assignment` uses its LHS.
  - `eval_agauss` uses its own line.
  - `inppas2.c:99` and `inp_parse_temper_trees` use the card or word list.
  - When the same key draws again, add a `#n` suffix.
- **No rows** for TRNOISE/TRRANDOM/RTS draws during a transient, or for
  control-language draws. They count toward `unlogged_calls`.

**RED-first test.** Three decks in `tests/regression/rng/`, run by
`check_status.sh`. Its `.files` `grep <path> <text>` lines assert structure
without freezing values that depend on libc.

- `rnddraws-mm.cir`: `.subckt cell a b w=2 l=1` / `.param
  sx={agauss(0,1,1)/sqrt(w*l)}` / `r1 a b r={1k*(1+0.01*sx)}` / `.ends`,
  instances `xm1` and `xm2`, `set rnddraws=rnddraws-mm.tsv`, then `setseed k;
  reset; op` for k = 1, 2, 3. The `.files` lines assert that the log is present,
  that it has a header with `src=setseed` and `offset=0`, that it has rows
  starting `1\tr.xm1.r1\tr\tagauss` and `1\tr.xm2.r1\tr\tagauss`, and that it
  has a footer. If the driver's `grep` cannot express a tab, the crew
  asserts tab-free substrings (`r.xm1.r1`, `agauss`) and says so in the
  receipt. Today `rnddraws` is an inert variable, so the `present` check
  fails. That is the expected RED.
- `rnddraws-order.cir`: the same with a third instance before `xm1`. Its rows
  are keyed `r.x0.r1`, `r.xm1.r1` and `r.xm2.r1`, which shows the mmB effect.
- The numeric check, `1000*(1+0.01*z/sqrt(2))` equal to the readback within
  1e-9, cannot be done inside the control language. Measured during scoping:
  `fread` returns the row, but `set row = ( $line )` makes one word from a
  tab-separated line. So the check is done once in the receipt by a script in
  the scratchpad, and the log-off guard is a fourth deck. That deck runs the same
  loop with `rnddraws` unset, prints readbacks at `numdgt=15`, and its golden is
  taken from `build-ver_50` before the change. It is green today and must stay
  green. It is glibc-dependent, so mark it Linux-only.

**Acceptance.**
- All decks pass, and both `make check` counts are recorded.
- The receipt shows the z-to-readback check at ≤1e-9 on `rnddraws-mm` and the
  keyed diff between the two order decks.
- The receipt times a 1000-sample `op` loop with the log off, on
  `build-ver_50` before and after. The readbacks are identical and the wall time
  is within noise.
- After `ngSpice_Reset`, review shows that every new static is closed or
  zeroed. No in-tree harness exists for the shared build. The receipt says so.

**Files.** `src/maths/misc/{drawlog.c,Makefile.am}`,
`src/include/ngspice/drawlog.h`, `src/maths/misc/randnumb.c`,
`src/frontend/numparam/{xpressn.c,spicenum.c}`, `src/frontend/inp.c`,
`src/frontend/options.c`, `src/spicelib/parser/{inpptree.c,inppas2.c}`,
`src/frontend/trannoise/wallace.c`, `src/sharedspice.c`. About 180-250 lines.
`tests/regression/rng/*`.

**Risks.**
- **Bit identity.** Logging must call no RNG function and must not reorder the
  arithmetic. The draw comparison before and after is part of acceptance.
- **Use-after-free.** Never keep `dico->cardline`.
- **The ngSpice_Reset rule** applies to the `FILE *`, the context strings, the
  seed record and the counter.
- **Draws whose instance means nothing**: a global `.param` that draws through a
  `.func` (drawn once, instance `?`), braces on dot cards, `.model` cards, and
  subckt-call arguments. In xschem's mmA, with `mc_mm_switch=0`, a z is still
  drawn and then multiplied away, and the log cannot show that.
- **`eval_agauss` quirks**: it sweeps once per function name, it silently skips
  draws whose arguments are not literals, and it rounds to `%g`.
- **Upstream conflict**: `xpressn.c`, `inp.c` and `inpptree.c` are busy files.
  Keep each hook to a few lines, and do not consolidate the three copies of the
  helpers.
- **Item 4 depends on item 3.** Until item 3 lands, the initial load's record
  set is tagged with a seed that was not used.

### 5 — P5: native `mcrun` — deferred

Deferred. By the handoff's own measure it brings no speed. xschem generates the
loop itself, and items 2-4 cover its risk. The one semantic piece it would
carry, `setseed` winning over `.option seed`, is held back from item 2 for
exactly this item. Revisit only if the owner wants it for hand-written decks.

### 6 — Close the batch

Fill in `LEDGER.md`: the item table, what each crew falsified, the final
`make check` counts, and what is left for the owner. Write a release note for
P1's result change and P3's determinism change. Write a reply to the xschem
session that confirms or revises the log contract against what item 4 shipped.

## Out of scope

- The handoff's §4 list, in full.
- The pre-existing RNG statics that survive `ngSpice_Reset` (`oldseed`,
  `seedinfo`, `gauss0`'s cache), `setseed` refilling the Wallace pool,
  `tclspice.c`'s pid seeding, and the dead `{random}` arm in `eval_opt`
  (`.opt` lines containing `{` never reach it, `options.c:271`). These are
  recorded here and not fixed.
- Handoff Q2 (the reset slowdown). Answered with hypotheses only, in
  `receipts/00-scoping.md`, and not profiled.
- `doc/claude/upstream/`.
