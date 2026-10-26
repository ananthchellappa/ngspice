# Receipt: follow-up 2, item 1 — typeless `.model` segfault

Crew **"Crash"**, ver_51 follow-up batch 2, 2026-10-26, branch `ver_51`.
HEAD at start **`f0c8c25cd`** (`docs: restore the upstream README sections
dropped in 7a0c363cc`). The code commit is **`b70e5123e`** (`Report a .model
card with no type in a deck with .if`). This receipt is the docs commit on top
of it.

The bug came from follow-up batch 1's receipt
`doc/claude/batches/2026-10-26-mc-v51-followup/receipts/01-p1-fixes.md`
("For the owner", item 3). It is written up as
**`doc/codex/issues/0074-a-typeless-model-in-a-deck-with-if-segfaults.md`**,
numbered after 0073.

**Code was touched**, in three production files: `src/frontend/inpcom.c`,
`src/frontend/inp.c` and `src/frontend/subckt.c`. Seven new decks are in
`tests/regression/exitstatus/`, and `./autogen.sh` was re-run. The owner's
uncommitted `CLAUDE.md` and `tests/regression/pipe/resource-keyword-case.cmd`
were not staged or modified. No `LEDGER.md`, `PLAN.md` or
`tests/lint/identity.baseline` was edited. No shared-library tree was created
and no worktree was used. Nothing was pushed. Every heavy command went through
`ngheavy`, one at a time, with `-j4`.

Scratch: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/c1/`.
It holds the RED logs (`red-*.log`), the GREEN logs (`green-*.log`),
`build*.log`, `install.log`, `check-exitstatus.log`, `check.log`, and the
probe decks `v[a-s].cir`, `typeless*.cir`. `ngspice-sym` there is an
unstripped relink of the same objects, used for backtraces only.

## What was wrong

`gdb` on the unstripped relink:

```
#0  __strchr_avx2 ()
#1  inp_expand_macro_in_str ()
#2  inp_expand_macros_in_deck ()
#3  inp_readall ()
#4  inp_spsource ()
#5  main ()
```

There were three defects. Issue 0074 "Root Cause" has the detail.

1. In `inp_expand_macro_in_str()`, `findtok_noparen(&search_ptr, &search_ptr,
   &end)` sets `search_ptr` to NULL when the card has no type. For `.model`
   with no name either, the `nexttok()` before it already returns NULL. The
   code came in with `9e8e50cb5` (2020, ngspice-32).
2. For `.model` alone inside a used `.subckt`, `modtranslate()` puts a NULL
   name into its wordlist, and `wl_find()` calls `strcmp()` on it. This
   crash was found only after the first was fixed. Backtrace:
   `wl_find <- doit <- inp_subcktexpand <- inp_spsource`.
3. The "no model type given" check is reached only through
   `inp_rem_unused_models()`, which `inp_readall()` skips when `has_if` is
   set. With only the crash fixed, the deck runs and exits 0 after a
   parser warning.

**Without `.if`.** A deck without `.if` crashes too if the session has
already read a deck with `.if`, because `has_if` is never cleared
(`model-typeless-after-if.cir`). Read alone, a deck without `.if` always
exited 1 cleanly.

## What changed

- `inp_expand_macro_in_str()`: skips `.model` and the name with
  `skip_token()`, and gives the type its own pointer.
- `modtranslate()`: a `.model` card with no name is left alone.
- The diagnostic is now `inp_chk_model_type()`. `inp_collect_models()` calls
  it, and its text is the same as before. The new
  `inp_chk_model_types(deck)` walks the flat deck. `inp_spsource()` calls it
  right after `dotifeval()`, so a card in an `.if` branch that is not taken
  is already a comment. It runs on every deck, so it does not depend on
  `has_if`.
- `findtok_noparen()` is unchanged. I tried a write-order fix there first
  and reverted it before RED: the call site with its own variable is
  clearer, and it was the only caller that passed the same variable twice.

There are no new string comparisons, so the identity lint does not apply.

## Decks

All seven run under `check_status.sh` and assert both the status and stderr.

| deck | shape | before | after |
| --- | --- | --- | --- |
| `model-typeless-noif.cir` | control: no `.if` | 1 | 1 |
| `model-typeless-if.cir` | `.if` + `.model bad` | 139 | 1 |
| `model-bare-if.cir` | `.if` + `.model` | 139 | 1 |
| `model-typeless-if-subckt.cir` | `.if` + `.model bad` in a used subckt | 139 | 1 |
| `model-bare-if-subckt.cir` | `.if` + `.model` in a used subckt | 139 | 1 |
| `model-typeless-if-dead.cir` | `.model bad` in `.if (0)` | 139 | 0, empty stderr |
| `model-typeless-after-if.cir` | no `.if`; `.control` sources an `.if` deck, then a typeless one | 139 | 1 |

**One expected `.err` was corrected after GREEN, and this is the correction.**
I wrote `model-typeless-if-subckt.err` before the fix, guessing that it would
name `.model bad`. The check runs on the flat deck, so it prints the card as
expansion left it: `.model x1:bad ` (instance prefix, trailing space). The
line number and file are unchanged. The status assertion was not touched, and
the RED run had failed on `signal status err`, not on the text. The `.status`
file says this, and so does the issue.

## RED evidence

The binary is `build-ver_50/src/ngspice` rebuilt from HEAD `f0c8c25cd`, sha
`c1341e751ffd39377859`. That is the same sha batch 1's receipt 04 recorded.
Trimmed:

```
check_status: model-typeless-if: FAILED (signal)
  the simulator did not terminate normally: rc=139 is 128+11, SIGSEGV.
check_status: model-typeless-if: FAILED (status)
  expected exit status 1, got 139
  stderr was:
    Segmentation fault
-Error: no model type given in line .model bad!
-    line no. 10 from file model-typeless-if.cir!
-
-ERROR: fatal error in ngspice, exit(1)
check_status: model-typeless-if: FAILED: signal status err

model-typeless-if-dead     FAILED: signal status
model-typeless-if-subckt   FAILED: signal status err
model-bare-if              FAILED: signal status err
model-typeless-after-if    FAILED: signal status err
model-typeless-noif        exit status 1 as expected   (control)
```

The run with the expander and the flat-deck check in place, before the
`modtranslate()` change:

```
check_status: model-bare-if-subckt: FAILED (signal)
  the simulator did not terminate normally: rc=139 is 128+11, SIGSEGV.
check_status: model-bare-if-subckt: FAILED: signal status err
```

The pre-P1 `ngspice-46+` binary (`scratchpad/p1/old/ngspice`) also exits 139
on the `.if` shapes, and exits 1 on the deck without `.if`.

## `make check`

| run | binary | PASS | FAIL | XFAIL | XPASS | SKIP | `make` exit |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `make -C tests/regression/exitstatus check` | `4b677f485e4dcdeed58d` | **21** | 0 | 0 | 0 | 0 | 0 |
| full `make check` from `build-ver_50/` | `4b677f485e4dcdeed58d` | **360** | 0 | 0 | 0 | 0 | **0** |

The full run had 353 tests before (receipt 04 of batch 1). The 7 new decks
make 360. Before the check, `ngheavy make install` was run after the build.

Lint: `identity lint: 11 comparisons, baseline matches`, and `PASS:
identity.lint`, `PASS: selftest.lint`. The baseline was not edited.

Binary sha (`sha256sum build-ver_50/src/ngspice | cut -c1-20`):
**`4b677f485e4dcdeed58d`**.

## For the owner

1. **The crash is fixed and the decks agree.** A `.model` card with no type
   now stops every deck with `Error: no model type given ...` and exit 1,
   with `.if` or without. In a subcircuit, the message shows the expanded
   name (`.model x1:bad `), with the deck's own line number.
2. **`has_if` is never cleared** (`inpcom.c:124`). This is a separate bug of
   the `ngSpice_Reset` kind. After this fix it no longer causes a crash or a
   missing error. But in an interactive session or a `libngspice` host, every
   deck read after one with `.if` still skips unused-model pruning. I left it
   alone. Clearing it at the start of each top-level read would be a small
   follow-up.
3. **The R->B model collection runs before `.if` is decided.** In a deck with
   `.if` and a resistor that the rewrite converts, `inp_collect_models()` sees
   the `.if (0)` branches too. So a typeless card in a branch that is not
   taken is reported, and a model there can be collected for the rewrite.
   This comes from P1, not from this item. Probe:
   `scratchpad/c1/vs.cir`, exit 1.
4. **The new exit has the same shared-build caveat as the old one.** The
   flat-deck check exits through `controlled_exit()` from `inp_spsource()`,
   as the other fatal deck errors there already do. It was not run on a
   shared build, because there is none.

Nothing was pushed. Two commits on `ver_51`: `b70e5123e` (code, tests, issue)
and this receipt.
