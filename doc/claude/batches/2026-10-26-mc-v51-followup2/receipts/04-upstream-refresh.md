# Receipt: follow-up 2, item 4 — upstream refresh

Crew **"Upstream refresh"**, ver_51 follow-up batch 2, 2026-10-26, branch
`ver_51`. HEAD at start **`eecdc5a97`** (`docs: receipt for ver_51 follow-up
2, item 3, the seed warning`). `origin/pre-master-47` is **`c5cd68015`**, as
in batch 1. There is one commit, the docs commit that holds this receipt and
the changes to `doc/claude/batches/2026-10-26-mc-v51-followup/upstream/`.

The task was to regenerate the unsent upstream patches that this batch's
items 1–3 affect, with the method and the fork-only exclusions of
`doc/claude/batches/2026-10-26-mc-v51-followup/receipts/05-upstream-prep.md`,
and to add the crash fix as `p4-crash/` if `pre-master-47` has the same bug.

**No production code or test on `ver_51` was touched.** The upstream patches
are dangling commits, made with `git commit-tree` from a temporary index
(`GIT_INDEX_FILE`) whose parent is `c5cd68015`, then written out with `git
format-patch -1`. No ref was created or moved, and no branch was switched.
One `git worktree add --detach` under the scratchpad was used for the `git
apply` runs, as the task allowed. It was removed with `git worktree remove`
and pruned, and `git worktree list` shows only the main tree. Nothing was
built in it. The owner's `CLAUDE.md` and
`tests/regression/pipe/resource-keyword-case.cmd` were not staged or
modified. No `LEDGER.md`, `PLAN.md` or `tests/lint/identity.baseline` was
edited. No shared-library tree was created. Nothing was pushed or sent
anywhere. The one heavy command, `make check`, ran under `ngheavy` with
`-j4`.

Scratch: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/u2/`.
It holds:

- `p1w/`, `p2w/`, `p4w/` and `p4s/`, the patched upstream files;
- `up-inpcom.c`, `up-inp.c` and `up-subckt.c`, `pre-master-47`'s files;
- `msg/`, the four commit messages;
- `repro/`, the minimal decks (`p1_dup.cir`, `p2_unusable.cir`);
- `t/`, the `check.sh` runs;
- `commits.txt`, the four dangling commit ids;
- `check.log`, the full `make check`.

## What changed

| dir | dangling commit | what was added | `p3`/`pre-master-47` apply |
| --- | --- | --- | --- |
| `p1/` | `f9619786f` (was `44da6f8de`) | `3a904f87d` (a) and (c): identical duplicate `.model` cards are one model, `temper-res-behav-dup.cir`, the `rg` line in `temper-res-behav-noise-subckt.sp` | clean |
| `p2/` | `cfac78314` (was `1e95f8edc`) | `f8f0f60d5`: `seed_option_unusable()`, so the "Cannot convert" warning is printed once per session | clean |
| `p3/` | unchanged | nothing in this batch touches it | clean |
| `p4-crash/` | `ed5d12af1`, new | `b70e5123e`, the crash fix, on `pre-master-47` | clean |
| `p4-crash/on-p1/` | `0221985cc`, new, parent `f9619786f` | the same fix on top of `p1` | applies after `p1` |

**P1.** The duplicate-model change is the fork's, line for line, without
the `/* case-lint: stored ... */` comment (upstream has no identity lint).
Item 2 (b), the free order around `behav_r_models_root` and in
`inp_free_models()`, is left out. It only protects the fatal-exit hook that
is already a fork-only exclusion, and upstream has no reader of those lists
on any exit path. The r10/r11 cards in `behav-r-model-warn.cir` are left out
with that deck, as before. The temper decks in the patch are now
byte-identical to `ver_51`'s.

**P2.** The `seed_option_unusable()` change is the fork's, line for line.
Its deck, `tests/regression/rng/option-seed-unusable.cir`, is left out with
the rest of `rng/`: it asserts stderr. The probe fix (4f) in the same fork
commit is fork-only.

**p4-crash.** `pre-master-47` crashes the same way. That was found by
reading its code; it was not built:

- `inp_expand_macro_in_str()` has the same `findtok_noparen(&search_ptr,
  &search_ptr, &end)` and then `strchr(search_ptr, '(')`;
- `modtranslate()` has the same unchecked `gettok()`, and `wl_find()` the
  same `eq()`;
- `inp_readall()` skips `inp_rem_unused_models()` when `has_if` is set,
  before `inp_expand_macros_in_deck()`, and `has_if` is never cleared.

ngspice-45.2 behaves the same: exit 139 on the five `.if` decks of
`tests/regression/exitstatus/`, exit 1 on `model-typeless-noif.cir`.

Without P1, `pre-master-47` has the "no model type given" check inline in
`inp_rem_unused_models()`, so the patch factors `inp_chk_model_type()` out
of there. P1 moves that loop into `inp_collect_models()`, so the two patches
edit the same lines and neither applies after the other. `p4-crash/on-p1/` is
the same change made on the `p1` commit. Its `inpcom.c` change matches the
fork's `b70e5123e` line for line (the `diff` of the two diffs is empty).

The patch has one test, in `check.sh` form:
`tests/regression/model/model-typeless-if-dead.cir`, with `.model bad` in an
`.if (0)` branch. The deck must run, so a crash leaves `INFO: success` out of
its output. The other six decks assert exit 1 and stderr, which `check.sh`
cannot see: a crash and a clean exit 1 both give empty stdout. They are left
out. The issue file `doc/codex/issues/0074` is left out too, since the commit
message carries its substance.

**Commit messages.** These are still the reports:

- P1 gains the duplicate rule, with numbers, and two test bullets.
- P2 gains a paragraph on "Cannot convert", with numbers.
- p4-crash is new, and `on-p1` adds one paragraph saying which tree it is
  for.

The other P1 and P2 numbers did not change.

**README.md** was edited in place. These parts changed:

- the title and intro;
- two rows in the table, and two new `p4-crash` rows;
- a note that `p4-crash/on-p1/` has a different parent;
- a paragraph saying what this crew regenerated;
- the P1/p4-crash apply-order paragraph;
- the binary sha;
- the identity-lint comment;
- P1's and P2's left-out lists;
- a new p4-crash left-out section, with the code-reading answer;
- a second test table.

`git diff` was checked before the commit. Every removed line is a line that
was replaced, and no section was dropped.

## Evidence

**`git apply`, in full, from a clean checkout of `c5cd68015`** (`git clean
-fdx` between runs; `-x` is needed because `tests/.gitignore` ignores the
`.out` files a previous run left):

```
alone ok: p1, p2, p3, p4-crash
seq ok: p1 p2 p3
seq ok: p3 p2 p1
seq ok: p2 p3 / p3 p2
seq ok: p4-crash p2 p3
seq ok: p1 p4-crash/on-p1
seq ok: p1 p2 p3 p4-crash/on-p1
seq ok: p3 p2 p1 p4-crash/on-p1
FAIL:   p1 then p4-crash, p4-crash then p1   (inpcom.c:182, expected; see above)
FAIL:   p4-crash/on-p1 alone on c5cd68015    (expected; it needs p1)
```

`git diff --check` is clean for all four commits.

**`gcc -fsyntax-only -Wall -DHAVE_CONFIG_H`**, with `pre-master-47`'s
`src/include` first and `build-ver_50`'s `config.h`. For P2 the patched
`randnumb.h` was put first on the include path. Each warning count equals
the unpatched file's, and there are no errors:

| file | unpatched | patched |
| --- | --- | --- |
| `inpcom.c` (`p1`, `p4-crash`, `p4-crash/on-p1`) | 2 | 2 |
| `inp.c` (`p2`, `p4-crash`) | 0 | 0 |
| `subckt.c` | 1 | 1 |
| `randnumb.c` | 0 | 0 |

(Batch 1 reported 3 and 5 for `inpcom.c` and `inp.c`. Its flags were
different, and both counts were compared within one set of flags.)

**RED for the new upstream deck.** Under `pre-master-47`'s `check.sh`, which
is byte-identical to the fork's, on ngspice-45.2:

```
Segmentation fault
--- model-typeless-if-dead.out_tmp
+++ model-typeless-if-dead.test_tmp
@@ -1 +0,0 @@
-INFO: success
rc=1
```

It failed 3 of 3 runs on 45.2, and 3 of 3 on the pre-patch fork binary
(`7e7b5610a68aa28a079e`). It passed 3 of 3 on the item 1 build and on
`build-ver_50`.

**Numbers in the messages.** These are from minimal decks in `repro/`:

- **P1 duplicate**: the same `.model rm` in `.if`/`.else`, at 125 °C.
  - 45.2: `v(a) = 7.300000e-02`, with no model carried at all.
  - Item 1 build: `7.300000e-02` and `model rm not resolved`.
  - `build-ver_50`: `5.694000e-02`, which equals `v(b)`.
- **P2 `seed=abc`**, with three `setseed 3 / reset / op` passes.
  - 45.2: the warning 4 times.
  - `build-ver_50`: once, plus the "(printed once per session)" line.
  - `@r1[r] = 8.392277e+02` on every pass, on both.
- **`temper-res-behav-dup.cir`**: FAIL with maxerr 0.282051 and spread 0 on
  the pre-patch and item 1 binaries. FAIL with an empty maxerr on 45.2,
  which has no const-plot `let`. PASS on `build-ver_50`.

## `make check`

No source or test on `ver_51` changed, so nothing was rebuilt, and `make
install` was not needed. The count was taken for the receipt:

| run | binary | PASS | FAIL | XFAIL | XPASS | SKIP | `make` exit |
| --- | --- | --- | --- | --- | --- | --- | --- |
| full `make check` from `build-ver_50/` | `68e577b28b297b0054d9` | **362** | 0 | 0 | 0 | 0 | **0** |

362 is the same count as item 3. One log line starts with `ERROR:`: `ERROR:
(internal)  tried to destroy non-existent graph`. It is the known output of
`vector-probe-report.cir`, which passes.

**Lint**, from the log:

```
identity lint: 264 comparisons, baseline matches
identity lint: 11 comparisons, baseline matches
```

**Binary:** `sha256sum build-ver_50/src/ngspice | cut -c1-20` →
**`68e577b28b297b0054d9`**, unchanged.

## For the owner

1. **Four patches are ready, and none has been sent.** P1 now also handles a
   model written the same way in both branches of an `.if`. P2 now also stops
   repeating the "Cannot convert" seed warning. P3 is as before. The crash fix
   is new, in `p4-crash/`.
2. **Upstream has the crash.** A `.model` line with no type, in any deck
   that uses `.if`, crashes `pre-master-47` and the 45.2 release. That
   includes a line inside an `.if` branch that is turned off. This was found
   by reading the code and by running 45.2; `pre-master-47` was not built.
   Of the four, this is the fix a maintainer is most likely to take as it is.
3. **P1 and the crash fix touch the same lines.** If you send both, send P1
   first and then `p4-crash/on-p1/`. If you send only the crash fix, use the
   one in `p4-crash/`. The README says the same.
4. **The crash fix has only one upstream test.** It is the dead-branch case.
   The upstream test harness cannot tell a crash from a clean error exit,
   because it does not see stderr or the exit status, so the error-path
   decks stay in the fork.
5. **The library caveat still applies.** P2's new flag is cleared on
   `ngSpice_Reset` by the same code as the other two flags. That path has not
   been run, because there is no library build.

Nothing was pushed or sent. One commit on `ver_51`: the docs commit with this
receipt and the `upstream/` changes.
