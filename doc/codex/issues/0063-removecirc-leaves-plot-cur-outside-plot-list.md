# Issue: `removecirc` Leaves `plot_cur` Pointing Outside `plot_list`

## Status

Open, filed 2026-08-13 on branch `ver_50`. **Filed rather than fixed, and that
is a decision, not a backlog entry** — `doc/claude/decisions/0017` decision 2
takes it, and its "what this decision does not decide" item 2 points here.

**Amended 2026-08-13, later the same day: criterion 4 is discharged and this
issue is now unblocked.** `doc/codex/issues/0059`'s fix was withdrawn, and
`tests/regression/misc/destroy-removed-circuit.cir` — the deck that asserted
`Internal Error: kill plot -- not in list` **is** printed — was deleted with
it. Nothing in the tree now pins the internal error as correct behaviour, so
criterion 2 can simply be met. The `plot_cur_chosen` pointer the Impact and
Resolution sections below refer to is gone from the tree as well; those two
paragraphs are amended in place.

Pre-existing and mode independent: it reproduces on `/usr/local/bin/ngspice`
(`ngspice-46`, no `casemode` support) exactly as it does on `build-ver_50`, and
no name below is looked up across a case boundary. It reached this batch only
because `doc/codex/issues/0059`'s fix needed a deck that takes `killplot()`'s
failing arm, and this is the only way a deck can get there.

## Summary

`com_removecirc()` (`src/frontend/mw_coms.c:22`) drops a circuit's plots out of
`plot_list` without freeing them and without moving `plot_cur`. When the
current plot belongs to the circuit being removed, `plot_cur` is left naming a
plot that is no longer in the list, and the interpreter is then in a state no
command can talk about consistently: `$curplot` and `display` still answer from
`plot_cur`, and every command that resolves a name walks `plot_list` and cannot
find it.

Measured, `op` then `removecirc`, on `build-ver_50/src/ngspice`:

```
echo B curplot=$curplot   -> B curplot=op1
display                   -> Here are the vectors currently active:
                             Name: op1 (Operating Point)
                                 in        : voltage, real, 1 long [default scale]
                                 midnode   : voltage, real, 1 long
                                 vs#branch : current, real, 1 long
setplot                   -> List of plots available:
                                 const   Constant values (constants)
setplot op1               -> Error: no such plot named op1
print v(midnode)          -> Error: no such vector midnode
```

So `display` lists three vectors of a plot `setplot` says does not exist, and
`print` cannot reach the vector `display` has just printed. `$curplot` names it
throughout.

Adding `destroy` gives the second face, which is the one `0059` needed:

```
Internal Error: kill plot -- not in list
C curplot=op1
```

`killplot()` (`src/frontend/postcoms.c`) frees the plot's vectors at the top,
then fails to find the plot in `plot_list` and returns early without freeing the
plot and without moving `plot_cur`. The plot survives as the current plot with
`pl_dvecs == NULL`, and it is leaked.

## Impact

Bounded, which is half the reason this is filed rather than fixed.

- **Straight after `removecirc` the file is right.** The plot is still the
  deck's own data and a bare `write` writes it: measured, 283 bytes,
  `Plotname: Operating Point`, the deck's three vectors.
- **After the `destroy` the file is `0059`'s.** The gutted plot has no vectors,
  `all` resolves nothing in it, and `vec_get()`'s fallback delivers the
  constants plot — so a bare `write` here writes the 570-byte
  `Plotname: constants` raw that `doc/codex/issues/0059` is about. That was
  refused while `0059`'s fix was in the tree; the fix was withdrawn, so this
  route is open again and belongs to `0059`, not here. It is why the two issues
  keep touching each other.
- **One leaked plot per occurrence**, plus its `pl_lookup_table`. Not a loop.
- **The internal error is printed on `cp_err`** and a deck can capture it — the
  deck that used to is gone (see the Status amendment).
- **`removecirc` is rare.** `grep -rn 'removecirc' tests/ examples/` finds it in
  no deck but this issue's own.

## Root Cause

`com_removecirc()` unlinks by title, comparing `pl_title` against the circuit
name and re-linking `pl_next` around each match (`src/frontend/mw_coms.c:85-109`).
It never asks whether `plot_cur` is one of the plots it just unlinked, and there
is no other owner of `plot_cur` to notice: `plot_cur` is only moved by commands
that choose a plot and by `killplot()`'s fallback.

`killplot()`'s failing arm is a consequence, not a second defect. Its
`Internal Error: kill plot -- not in list` is an accurate report of a list that
someone else corrupted.

## Acceptance Criteria

1. After `removecirc` of the circuit owning the current plot, `$curplot`,
   `display`, `setplot` and `print` agree with each other about which plot is
   current, whatever they agree on.
2. `destroy` after `removecirc` does not print `Internal Error: kill plot --
   not in list`.
3. No plot is leaked by the pair.
4. ~~`tests/regression/misc/destroy-removed-circuit.cir` is updated in the same
   change, because that deck asserts the internal error *is* printed.~~
   **Discharged 2026-08-13**: that deck existed only to pin `0059`'s withdrawn
   guard and was deleted with it, so no test now asserts the internal error.
   The new deck this issue needs for criteria 1-3 still has to discriminate
   `killplot()`'s failing arm from an ordinary `destroy` somehow — with the
   internal error gone, `$curplot` and `display` disagreeing (the Summary's
   transcript) is the available tell.

## Resolution

**None yet.** The local fix is one line — `plot_cur = plot_list;` in
`killplot()`'s failing arm — and it is deliberately not taken:

- It treats the symptom at the wrong end. `killplot()` is not the command that
  broke the invariant, and the window between `removecirc` and any later
  `destroy` (the `display`/`print` disagreement above) stays open regardless.
- It changes the `write` outcome only by moving which plot the constants
  fallback is reached *from*; the file is `0059`'s either way, and `0059` is
  Open. Fixing this issue does not fix that one and must not be mistaken for
  it.

The fix that matches the root cause is in `com_removecirc()`: before unlinking,
note whether `plot_cur` is among the plots about to leave the list, and if so move both to a plot that stays — `plot_list` after
the unlink, as `killplot()` does — and free the unlinked plots rather than
dropping them. That is a change to a command outside `0059`'s subject, in a
batch about case modes, and it wants its own RED deck asserting criteria 1-3.
