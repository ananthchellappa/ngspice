# Issue: A `.model` Card With No Type, in a Deck With `.if`, Segfaults

## Status

**Fixed 2026-10-26** on branch `ver_51`, by crew "Crash" of the ver_51
follow-up batch 2. The receipt is
`doc/claude/batches/2026-10-26-mc-v51-followup2/receipts/01-typeless-model-segfault.md`.
It also holds the commit hashes.

Found by follow-up batch 1 as a side finding
(`doc/claude/batches/2026-10-26-mc-v51-followup/receipts/01-p1-fixes.md`, "1d",
and "For the owner" item 3). It is **pre-existing and upstream**. The crashing
line came in with `9e8e50cb5` (2020-03-20, first released in ngspice-32), and a
pre-P1 `ngspice-46+` binary crashes the same way.

## Summary

A `.model` card that stops before its type is a deck error. In a deck without
`.if`, ngspice reports it and exits 1:

```
$ cat typeless3.cir
typeless model
.model rok r tc1=1e-3
.model bad
i1 0 1 1u
r1 1 0 1k
.control
op
quit 0
.endc
.end
$ ngspice -b typeless3.cir ; echo rc=$?
Error: no model type given in line .model bad!
    line no. 3 from file typeless3.cir!

ERROR: fatal error in ngspice, exit(1)
rc=1
```

Add an `.if` block anywhere in the deck, even an empty `.if (1)` / `.endif`,
and the same card kills the process:

```
$ ngspice -b typeless2.cir ; echo rc=$?
Segmentation fault
rc=139
```

These shapes all segfaulted before the fix:

| deck shape | before | after |
| --- | --- | --- |
| `.if` block + top-level `.model bad` | 139 | 1, "no model type given" |
| `.if` block + top-level `.model` (no name either) | 139 | 1, same |
| `.if` block + `.model bad` inside a used `.subckt` | 139 | 1, same (names `.model x1:bad `) |
| `.if` block + `.model` alone inside a used `.subckt` | 139 | 1, same |
| `.model bad` inside the `.if` branch that is taken | 139 | 1, same |
| `.model bad` inside the `.if (0)` branch that is not taken | 139 | 0, the card is not part of the circuit |
| no `.if`, read in a session that already read a deck with `.if` | 139 | 1, same |

## Impact

- A user's typo kills the simulator with no message at all. In batch mode the
  exit status is 139, not 1. In `libngspice` it takes the host application
  down with it.
- A deck can crash without having `.if` itself. `has_if` (`inpcom.c:124`) is a
  file static. It is set by the first deck with `.if` that a session reads
  and is never cleared. So in an interactive session, or in a host that uses
  `ngSpice_Circ` more than once, every later deck is treated as having `.if`.
- A card in an `.if (0)` branch, which should not matter, crashes too.
- Once the crash is mended alone, the same decks do not fail. The parser warns
  `Unknown model type - ignored` and the run exits 0. So the two decks would
  still give different verdicts on the same mistake.

## Root Cause

There are three defects. The first is the crash. The second is a second crash
behind the first. The third is why a deck with `.if` reaches either.

1. **The macro expander assumes a `.model` card has a name and a type.**
   `inp_expand_macro_in_str()` (`src/frontend/inpcom.c`) skips
   `.model <name> <type>` before it looks for function calls:

   ```c
   search_ptr = nexttok(search_ptr);
   search_ptr = nexttok(search_ptr);
   char *end;
   findtok_noparen(&search_ptr, &search_ptr, &end);
   ```

   `findtok_noparen()` (`src/misc/string.c`) writes `*p_str` and then sets
   `*p_token = NULL` when there is no token. Here both are `search_ptr`, so
   for `.model bad` it ends up NULL, and the next line's `strchr(search_ptr,
   '(')` dereferences it. For `.model` alone, the second `nexttok()` already
   returns NULL, and `findtok_noparen()` dereferences that itself.
   `inp_expand_macros_in_deck()` runs on every deck, from `inp_readall()`.

2. **`modtranslate()` assumes a `.model` card has a name.**
   `src/frontend/subckt.c` renames a subcircuit's models to
   `<instance>:<name>`. For `.model` alone, `gettok()` returns NULL and
   `wl_cons(NULL, ...)` puts it in the list of original names.
   `wl_find()` then calls `strcmp()` on it.

3. **A deck with `.if` never meets the "no model type" check.** That check
   lives only in `inp_collect_models()`, which `inp_rem_unused_models()`
   calls. `inp_readall()` skips that pass when `has_if` is set, because which
   models are used is not known until the branches are decided. They are
   decided by `dotifeval()` in `inp_spsource()` (`src/frontend/inp.c`), after
   subcircuit expansion, and nothing there looks at model types. Without a
   crash, a typeless card goes on to the parser, which only warns.

Before the fix, an unused-model pass in `inp_readall()` was the only thing
that stood between a typeless card and the first two crashes. A deck without
`.if` has that pass, and it exits cleanly before reaching them.

Other walkers of `.model` cards were checked with the same shapes, and none
crashes. `get_model_type()` and `get_subckt_model_name()` use `skip_token()`,
which never returns NULL. The `inp_vdmos_model()` scan, `rem_mfg_from_models()`
and `comment_out_unused_subckt_models()` do the same, or only search for
identifiers. `inp_parse_temper()` reads a `.model` card only if it holds
`temper`, and it runs after the new check.

## Acceptance Criteria

1. A deck with `.if` and a top-level `.model` card with no type, or with no
   name either, exits 1 and prints the same diagnostic as the deck without
   `.if`. No signal.
2. A typeless card in an `.if` branch that is not taken is not an error: the
   run exits 0.
3. The same holds for the card inside a used `.subckt`, with or without a name.
4. A deck without `.if`, read after a deck with `.if` in the same session,
   gives the same diagnostic as when it is read alone.
5. The deck without `.if` is unchanged: exit 1, same three lines.
6. Each criterion is asserted by a committed deck in
   `tests/regression/exitstatus/`, on status and stderr.

## Resolution

Fixed at the three places above, so that no single call site carries the fix:

- **`inp_expand_macro_in_str()`** skips `.model` and the name with
  `skip_token()`, which never returns NULL. It gives `findtok_noparen()` its
  own `type` pointer, so a missing type no longer clears `search_ptr`. The
  expander no longer crashes on any `.model` card, whatever is in the deck.
  `findtok_noparen()` itself is unchanged; this was its only caller that passed
  the same variable twice.
- **`modtranslate()`** leaves a `.model` card with no name as it is and does
  not add it to the rename lists.
- **The diagnostic** moved into `inp_chk_model_type()`. `inp_collect_models()`
  calls it as before, with the same text. A new `inp_chk_model_types(deck)`
  walks the flat deck and calls it on every `.model` card outside `.control`
  that is not commented out. `inp_spsource()` calls it right after
  `dotifeval()`, so a card in a branch that is not taken is already a comment
  and is not reported. For a deck without `.if` it finds nothing, because
  `inp_readall()` already stopped on the card. It runs on every deck, not only
  when `has_if` is set, so criterion 4 holds without touching `has_if`.

The decks, all in `tests/regression/exitstatus/` and run by `check_status.sh`:

| criterion | deck | status |
| --- | --- | --- |
| 1 | `model-typeless-if.cir`, `model-bare-if.cir` | 1 |
| 2 | `model-typeless-if-dead.cir` (empty `.err`) | 0 |
| 3 | `model-typeless-if-subckt.cir`, `model-bare-if-subckt.cir` | 1 |
| 4 | `model-typeless-after-if.cir` | 1 |
| 5 | `model-typeless-noif.cir` | 1 |

All of them except `model-typeless-noif.cir` failed with `signal status` on the
binary from before the fix. `model-bare-if-subckt.cir` still failed that way
after the first and third changes alone, which is how the second crash was
found.

**The subcircuit diagnostic names the card as expansion left it.** The check
runs on the flat deck, so `model-typeless-if-subckt.cir` reports
`.model x1:bad ` (instance prefix, trailing space). Its line number and file
are those of the card the deck wrote. The deck without `.if` names the card
as written, `.model bad`, because its check runs before expansion.

**Not changed:**

- `has_if` is still never cleared. After the fix that no longer costs a crash
  or a missed diagnostic. It still means that every deck read after one with
  `.if` skips the unused-model pruning in `inp_readall()`. That is a separate
  defect, of the `ngSpice_Reset` kind.
- In a deck with `.if` and a resistor that the R->B rewrite converts,
  `inp_collect_models()` runs before `dotifeval()`. So a typeless card in an
  `.if (0)` branch of such a deck is still reported, and models in branches
  not taken are still collected for the rewrite. This came with P1's model
  carry and is recorded for the owner.
- A `.model` card whose type is a word ngspice does not know, such as
  `.model bad is=1`, still only warns. That is a different mistake, and some
  such types are code models that may load later.
