# Decision 0017 — what a bare `write` refuses, `doc/codex/issues/0059`

## Status

**WITHDRAWN, 2026-08-13, branch `ver_50`, together with the code it justifies.**
Decisions 2, 3, 4 and 5 describe a guard that is no longer in the tree, and
`plot_cur_chosen` — the pointer every one of them turns on — has been removed
from `plotting.c`, `fteext.h`, `vectors.c`, `dotcards.c`, `spec.c` and
`com_fft.c`. Read them as the record of an attempt, not as a description of
`write`.

**Decision 1 survives the withdrawal**, because it is a decision *not* to
change `write`: row 2 keeps its behaviour, and it keeps it now for the simpler
reason that nothing about `write` changed at all. The `dosim()` experiment
behind it was built and measured and its numbers stand.

Why the rest went: a fourth round of verification found that all three
generations of the guard refuse a plain nutmeg session that builds vectors with
`let` and writes them, because those vectors live in the constants plot — the
plot the guard treats as evidence that nothing was chosen. That is row 10 of
`doc/codex/issues/0059`'s matrix, and `0059`'s Resolution is the full account:
what each of the three discriminators was, how each was falsified, and the four
constraints a correct one has to satisfy at once. `0059` is Open.
`tests/regression/misc/write-let-session.cir` pins row 10.

The original status line, for the record:

Accepted, 2026-08-13, branch `ver_50`. Settles `doc/codex/issues/0059`
acceptance criterion 2, which asked for row 2 of the issue's matrix to be
"decided rather than inherited … a decision and a sentence in the migration
note, not a silent tightening", and which the implementation left open. It also
records the two invariants the implemented discriminator depends on, both of
which were found by adversarial verification after the fix landed and neither
of which is derivable from the issue text.

Decision 4 was added 2026-08-13 by the crew after the next verification, which
found a state no row of `0059` names and this decision did not mention:
`op`, `setplot new`, bare `write` — a plot the deck chose, with nothing in it,
and row 1's file with `rc=0`. Decisions 2 and 3 carry an amendment each because
that change moved what they measure; neither decision itself changed.

Decision 5 was added 2026-08-13 by the crew after the verification of decision
4, which found that decisions 1 and 4 were both keyed on the wrong thing — the
*syntax* of the command rather than its *subject* — so that spelling the
default argument, `write f.raw all`, walked past every refusal in this file.

**No computed value moves.** The subject is whether `write` produces a file.

## Context

`write` with no vector list writes the current plot. `0059` measured three
shapes in which "the current plot" is not something anybody asked for, and one
in which it is:

| row | how `plot_cur` got there | file today |
| --- | --- | --- |
| 1 | the static initialiser, `plotting.c:13` — nothing has run | 570 B of the twelve physical constants |
| 2 | a good analysis chose it; a *later* analysis then failed | the good analysis's real data, correctly labelled |
| 7 | `killplot()`'s fallback after `destroy` of the chosen plot | 570 B of constants again |

The fix records the plot something last *asked* to be current
(`plot_cur_chosen`, `src/frontend/plotting/plotting.c`) and refuses the
no-vector-list branch of `com_write()` when `plot_cur != plot_cur_chosen`. That
closes rows 1 and 7 and cannot see row 2, because in row 2 the good analysis
really did choose the plot and nothing unchose it.

---

## Decision 1 — row 2 keeps its behaviour; the discriminator is provenance, not freshness

**Decided: a bare `write` after a failed rerun still writes the previously
chosen plot. `write` does not consult the analysis outcome.**

The rule the fix implements, stated so that row 2's exclusion is a consequence
of it rather than an omission:

> `plot_cur_chosen` records **where `plot_cur` came from**, not **how fresh
> what it points at is**. A bare `write` refuses when `plot_cur` points
> somewhere nothing put it deliberately. It does not adjudicate whether the
> thing that put it there is recent enough.

Under that rule rows 1 and 7 are in scope and row 2 is out, and the refusal
always coincides with a state the user can be told about in one sentence: when
`write` refuses, `$curplot` names either the constants plot or a plot that has
just been destroyed — never a plot full of the deck's own data.

### The alternative was built and measured, not guessed

Refusing row 2 is one line, and the site already exists. Crew A's `0057` work
established `dosim()` (`src/frontend/runcoms.c`, beside `OUTsaveMissClear()`)
as the "a simulation begins here" funnel, with the comment that one analysis
can open several plots and so a per-plot hook would count plots instead of
attempts. Adding

```c
    plot_cur_chosen = NULL;   /* an attempt retracts the previous choice */
```

there, paired with the `plot_setcur()` call `plotInit()` already makes, gives
exactly row 2's refusal. Measured with that line in, `build-ver_50` rebuilt:

- `repro/seq.cir` under `-D casemode=distinguish` — the row 2 deck — goes from
  `rc=1` with a 341-byte `Plotname: Operating Point` raw of the *first* op's
  data to `rc=1` with **no file**. (That run discarded `stderr`, so the refusal
  text itself was not read back; the file's absence is what was measured.)
- `repro/seq.cir` with no `-D`, where both ops succeed: unchanged, `rc=0`, 317
  bytes. `repro/divider.cir`: unchanged, 325 bytes.
- `make -C build-ver_50/tests/regression check`: **all green**, every
  directory, including the three decks that contain a bare `write`
  (`tests/regression/pipe/options-keyword-case.cmd`,
  `tests/regression/case/write-roundtrip.cir`,
  `tests/regression/misc/write-unchosen-plot.cir`).

So the cost of taking it is known to be zero inside this tree, and the reasons
below are not "it is hard" or "it might break something unnamed".

### Why it is not taken

1. **Row 2's file is not wrong; the deck's belief is.** Rows 1 and 7 produce a
   file that misrepresents *itself* — its `Date:` is the build stamp rather
   than a run time (`0059` measures the two as byte-equal), its `Plotname:` is
   `constants`, and no deck ever asked for it. Row 2 produces a faithful,
   correctly labelled rawfile of a plot the deck really computed, with a real
   run time in it. What is wrong is the deck's assumption that a second
   analysis ran, and that assumption is falsifiable in-band before the `write`:
   `0059`'s `repro/status_probe.cir` reads `sim_status` and `$curplot` and
   measures the difference. There is no such reading for rows 1 and 7 — there
   the answer is that no analysis has ever run, which is not a thing the file
   can be about.

2. **Refusing it makes `write` disagree with `print` about the same plot.**
   Under the un-choose rule, `$curplot` reads `op1`, `print v(out)` prints
   `op1`'s data, `display` lists `op1`'s vectors, and `write f.raw` refuses to
   write them — with a message that has to say something other than what it
   says now, because a plot *has* been selected. Under the shipped rule every
   refusal coincides with `$curplot` naming something the deck did not choose,
   so one sentence explains all of them.

3. **It is a mode-independent, pre-existing change to a 30-year-old command,
   arriving on a batch about case modes.** `0059` is explicit that the whole
   defect reproduces on `/usr/local/bin/ngspice` with no `-D` at all, and the
   client report (finding 3 of
   `doc/claude/feedback/ngspice_upstream/FINDINGS.md`) is about the constants
   plot — row 2 was constructed by the issue's author while exploring, not
   reported. Rows 1 and 7 replace a file that is *never* what anyone wanted
   with a refusal, which needs no permission. Row 2 replaces a file that is
   *sometimes* exactly right.

### What would change it

Named so the next reader does not have to re-derive the case for the other
side, which is real:

- **A measured sweep-loop miss.** `foreach`/`alter`/`op`/`write f$i.raw`, where
  one iteration's analysis fails and that iteration's file silently receives
  the previous iteration's numbers under its own name. That is data corruption
  in a shape a user cannot see, it is row 2 exactly, and one report of it from
  a real deck outweighs reasons 1 and 2 above. Nothing in `tests/` or
  `examples/` has this shape today: `grep -rn '^[[:space:]]*write' tests/
  examples/` finds 56 command lines, and the ones with no vector list live in
  seven files — five besides this issue's own two decks. Of those five only
  `examples/various/3d_loop.sp` contains a loop, and its three bare writes sit
  at loop depth 0, after the `foreach` bodies that fill the vectors they write.
- **Provenance in the rawfile header.** If a `write` ever stamps which analysis
  produced the plot and whether it completed, row 2 stops being a `write`
  question at all and becomes a reader question, and the case for refusing
  weakens further rather than strengthening.

### The migration note

`doc/claude/decisions/0001-distinguish.md` decision 5 gains **no sentence**,
and that is the answer to criterion 2's second half rather than an oversight.
That note lists guarantees `preserve` gives which `distinguish` withdraws;
`0059` is mode-independent — the same rows reproduce under `fold`, `preserve`
and `distinguish` and on a binary with no `casemode` support — so nothing about
it belongs there. Decision 1 withdraws nothing at all.

What rows 1 and 7 withdrew is a `write` behaviour, not a mode behaviour, and
belongs in `NEWS`.

It **was** in `NEWS` from 2026-08-13 until the withdrawal later the same day,
as a `Bug fixes:` entry under `Ngspice-47`; it came out with the code, because
a released `write` now behaves exactly as it did before and there is nothing to
tell a user about. The entry is quoted here so the record of what was claimed
survives the removal:

> `write` with no vector list now refuses, rather than writing the built-in
> constants plot, when no analysis has run, when the current plot was reached
> by `destroy`ing the plot that was chosen, or when the current plot has no
> vectors of its own. It says which it is and writes no file. Name the plot
> (`write f.raw const.all`), select it (`setplot const`), or name the vectors
> to get the old file. A bare `write` after an analysis that failed still
> writes the plot of the last analysis that succeeded, which is unchanged.

Until it landed there, this sentence existed only in this file, which is what
the verification of 2026-08-13 called "answered by refusal rather than met":
the argument that `0001-distinguish.md` decision 5 gains nothing is sound, but
a reader auditing criterion 2 needs the user-visible half to be somewhere
shippable. That half is void now — criterion 2's *migration-note* half is
still answered by decision 1, which withdraws nothing and therefore owes
`0001-distinguish.md` decision 5 no sentence, and that part is unaffected by
the withdrawal.

If decision 1 is ever reversed, the sentence that goes with it is:

> `write` with no vector list now also refuses after an analysis that failed,
> where it used to write the plot of the last analysis that succeeded. Use
> `setplot <plot>` or `write f.raw <plot>.all` to write that plot deliberately.

---

## Decision 2 — `killplot()` forgets the choice *before* it unlinks, and that order is load-bearing

**Decided: the `plot_cur_chosen` clear stays above the unlink in `killplot()`
(`src/frontend/postcoms.c`), and the reason is written at the site.**

Adversarial verification read the clear's position as a defect — "the clear
belongs after the unlink succeeds" — on the grounds that the inside-list arm
returns early on `Internal Error: kill plot -- not in list` without freeing the
plot, so a plot that could not be killed has already lost its chosen status.
The premise is right and the conclusion is backwards, because by the time that
early return is reached the plot has *already* lost its vectors: `killplot()`
frees them at the top, and `vec_free()` unlinks each one, so the plot survives
the early return with `pl_dvecs == NULL`.

**The path is reachable from a deck.** `com_removecirc()`
(`src/frontend/mw_coms.c:85-109`) drops a circuit's plots out of `plot_list`
without freeing them and without moving `plot_cur`, so `plot_cur` is left
naming a plot that is no longer in the list. A bare `destroy` then takes
`killplot()`'s failing arm. Measured, `op` then `removecirc` then `destroy`:

```
Internal Error: kill plot -- not in list
C curplot=op1
```

— the plot is still current, still named by `$curplot`, and has no vectors.
A bare `write` of it falls back to the constants plot, so with the clear moved
below the unlink the deck writes `drc_bare.raw` with `Plotname: constants` in
it: `0059`'s row 1 file, reached with a successful analysis behind it and
`$curplot` naming the analysis plot. That is a *worse* face of the defect than
any row in the issue, and it is what the proposed reordering restores.

`tests/regression/misc/destroy-removed-circuit.cir` pins it, and
`write-unchosen-plot.cir` passes under both placements — the case is outside
what that deck can reach, which is why it needed its own.

**Amended 2026-08-13, after decision 4.** Two things above are no longer what
the tree measures, and the decision itself is unchanged.

- The deck as first written did not pin what this decision says it pins: it
  passed identically with its `removecirc` commented out, because an ordinary
  `destroy` of the current plot refuses the following `write` too, by the other
  arm. It now reads its `destroy` capture back and asserts
  `DESTROY-NOT-IN-LIST-CAPTURED`, so the arm this decision is about has to be
  the arm that ran. Measured: with `removecirc` commented out the deck prints
  `DESTROY-NOT-IN-LIST-MISSING` and `make check` fails it.
- The reordering no longer reaches a file, because decision 4's test refuses an
  empty current plot and a gutted plot is empty. Re-measured with the clear
  moved below the unlink: **no `drc_bare.raw`**, and the capture holds
  `the current plot has no vectors` where it should hold `no plot has been
  selected`. The deck still discriminates the placement — it reads the message,
  not just the file — and the clear stays above the unlink because that is the
  message which is true of this state. What the reordering costs is now the
  accuracy of the diagnostic rather than the file, which is a smaller loss than
  the one measured above, and the two tests are independent defences rather
  than one.

Not fixed here, and filed rather than folded in: `com_removecirc()` leaving
`plot_cur` outside `plot_list` is a dangling-current-plot bug of its own,
pre-existing and independent of `0059` (it produces the same internal error on
a tree without `plot_cur_chosen` in it). Moving `plot_cur` to `plot_list` in
`killplot()`'s failing arm would be the local fix and does not change any
`write` outcome, because `plot_cur_chosen` is `NULL` either way.

**It is filed as `doc/codex/issues/0063`, 2026-08-13**, with its own acceptance
criteria, the measurement that `display` and `setplot` disagree about the
current plot straight after `removecirc` — before any `destroy` — and the
reason the one-line fix is not the right one. Until that landed, this paragraph
was the only record of it outside a receipt, which is not somewhere a reader
looking for open defects would find it. Criterion 4 there is the one that
matters to this file: whoever fixes `0063` has to give
`destroy-removed-circuit.cir` another discriminator, because that deck asserts
the internal error *is* printed.

---

## Decision 3 — the `NULL`/`NULL` hole in the discriminator is closed by argument, not by code

**Decided: `com_write()`'s test stays `plot_cur != plot_cur_chosen`, and the
argument that a `NULL` `plot_cur` cannot reach it is written at the site that
produces the `NULL` (`src/frontend/dotcards.c`).**

The test's invariant is "equal implies chosen". A `NULL`/`NULL` pair satisfies
it while representing no choice at all, so a bare `write` with
`plot_cur == NULL` would pass the guard and then evaluate `all` against a null
plot. `setcplot()` returns `NULL` when the deck ran no analysis of the type
asked for, and `plot_cur = plot_cur_chosen = setcplot(…)` is written three
times in `ft_cktcoms()`.

Unreachable, in three steps, each checked rather than assumed:

1. Those three sites — the two `setcplot("op")` calls and the
   `setcplot("tran")` in the `.four` arm — are the **only** assignments in the
   tree that can store `NULL` in `plot_cur`. `grep -rn 'plot_cur *=' src/`
   returns 22 lines: two definitions (`plotting/plotting.c:13` and
   `src/ngsconvert.c:38`, a different program's own symbol) and the 20
   in-process assignment sites `0059`'s Root Cause enumerates. Every one of the
   other 17 stores a plot it has just found or allocated, and
   `com_let.c:248`/`:250` and `numparam/xpressn.c:1123`/`:1125` only restore a
   value they saved.
2. `ft_cktcoms()`'s only caller is `src/main.c:1573` and `:1581`, both inside
   the `ft_batchmode` tail, and both are followed immediately by
   `sp_shutdown()`. No command interpreter runs after it.
3. A deck's `.control` block has already run by then.

Measured, with a temporary probe at `ft_cktcoms()`'s exit: `-b -r out.raw` on a
tran-only deck carrying a `.print tran` line exits `ft_cktcoms()` with
`plot_cur=(nil)` — the terse arm prints ".print line ignored" without assigning
— while the same deck without `-r` exits with a real plot; and with a
`.control` block added, the block's `echo` appears 15 lines *before* the probe.

So the hole is real and has no door. The comment at `dotcards.c:211` states the
three steps and names the one change that would open it: giving
`ft_cktcoms()` a caller that returns to the command loop.

**Amended 2026-08-13.** Decision 4's test is written
`!plot_cur || !plot_cur->pl_dvecs`, so a `NULL` `plot_cur` is now refused by
code as well, and the argument above is what says the arm cannot be reached
rather than what keeps the command safe. The comment at `dotcards.c` is left as
written: it is still the reason the first test's `NULL`/`NULL` pair is not a
defect, and it names the change that would matter.

---

## Decision 4 — a bare `write` of an *empty* current plot refuses as well

**Decided: `com_write()` gains a second test on the no-vector-list branch —
`!plot_cur || !plot_cur->pl_dvecs` — and refuses with a message of its own.
Added 2026-08-13, after adversarial verification found the state decision 1's
rule cannot see.**

The state, reachable in one command and named nowhere in `0059`:

```
op                  <- a real analysis, plot_cur and plot_cur_chosen are op1
setplot new         <- makes a plot AND chooses it: the two stay equal
write f.raw         <- rc=0, 570 bytes, "Plotname: constants", Date: == Build:
```

Decision 1's rule passes it correctly — the deck *did* ask for this plot — and
the file is still `0059` row 1's file, now with a successful analysis behind it
and `rc=0`. The mechanism is not provenance at all: `all` resolves nothing in an
empty plot, and `vec_get()` (`src/frontend/vectors.c`) retries every name it
cannot find against the constants plot, so the write silently changes its
subject. Stated as a defect of `com_write()` alone, with no reference to
`0059`: **with no vector list this command writes the current plot
(`com_write()`'s own header comment says so), and it was writing a different
one.**

That is why this is a second test rather than a change to the first. The first
answers "did anything choose this plot"; this one answers "is the plot it
chose what the file will contain". Both are needed and each is pinned by a deck
that fails when it alone is removed — `write-unchosen-plot.cir`'s FRESH,
FAILED and DESTROYED cases for the first (`plot_cur` is the constants plot
there, which is not empty), `write-empty-plot.cir` for the second.

Measured before the change: `WRITE-EMPTY-PRESENT`, 570 bytes, `Plotname:
constants`, `Date:` byte-equal to the build stamp. After: no file, and
`Error: the current plot has no vectors, "wep_empty.raw" not written.` on
`cp_err`, read back out of a `>&` capture. The controls in the same deck do not
move: `write f.raw pi boltz` from that same empty plot still writes 264 bytes,
and one `let` of the deck's own makes the bare write land on 234 bytes of the
new plot. `tests/regression/misc` is 36/36 and the deck matches its reference
under no `-D`, `fold`, `preserve` and `distinguish`.

Taken rather than documented-as-known, on decision 1's own reason 1: this file
misrepresents *itself* exactly as rows 1 and 7 do — `Plotname: constants`, a
build stamp for a `Date:` — and replacing a file that is never what anyone
wanted with a refusal needs no permission. It is not row 2: nothing here is the
deck's own data under a wrong label.

What it costs, stated so a reader can weigh it: a deck that pointed `write` at
an empty plot used to get the constants and now gets nothing. Nothing in
`tests/` or `examples/` has that shape, and the two ways to ask for the
constants deliberately — `setplot const`, or naming the vectors or the plot —
are unaffected and are asserted in both decks.

---

## Decision 5 — the refusals key on what is written, not on how much of it was typed

**Decided: both tests move off `!wl` and onto a predicate over the request —
`!wl || (!wl->wl_next && eqc(wl->wl_word, "all"))` — so that `write f.raw all`
is refused wherever `write f.raw` is. Added 2026-08-13, after adversarial
verification found decisions 1 and 4 bypassable by one word.**

`com_write()` holds a static one-word list `{ "all" }` and substitutes it when
the command carried no vector list (`src/frontend/postcoms.c`, the `all`
declaration and its two uses). The two spellings are therefore the same
operation on the same plot — the command's own header comment says so — and
testing `wl` for `NULL` asked which of them the deck typed.

Measured on the delivered tree, before this change, all three with `rc` as
shown and a file of the twelve physical constants, `Plotname: constants`,
`Date:` byte-equal to the build stamp:

| state | command | before | after |
| --- | --- | --- | --- |
| nothing has run (row 1) | `write y.raw all` | `rc=1`, 570 B | refused, no file |
| `op`, `destroy` (row 7) | `write y.raw all` | `rc=0`, 570 B | refused, no file |
| `op`, `setplot new` (decision 4) | `write y.raw all` | `rc=0`, 570 B | refused, no file |

The token is matched with `eqc`, not `eq`, because that is how the resolver
matches it: `get_all_type()` (`src/frontend/vectors.c`) lowercases each
character in every mode, so `write f.raw ALL` reaches the same wildcard. A
byte-exact test here would refuse a strictly smaller set than the one that
reaches the constants plot, which is the same defect one shift key along —
measured: with `eq` in place of `eqc`, `tests/regression/misc/write-spelled-all.cir`
fails on its `UPPER-ALL` case alone.

**The scope is exactly one token wide, and that is deliberate.** `!wl->wl_next`
keeps a longer list — `write f.raw all pi` — outside: it names something the
current plot does not hold, which is a request in its own right and is item 4
of "what this decision does not decide" below, unchanged. Widening the
predicate to *any* list whose first word is the wildcard would refuse a command
that named a vector, which is over-refusal in the direction decision 4's
controls exist to prevent.

`tests/regression/misc/write-spelled-all.cir` pins all of it: the three
refusals above, the `ALL` spelling, and three controls that must keep writing
(`pi boltz` from the empty plot, a bare-equivalent after a real analysis, and
`setplot const` then `write f.raw all`, which asks for exactly the file the
three refusals kept back). Measured RED under the real harness with the
predicate reverted to `!wl`: six assertions flip, `FAIL: write-spelled-all.cir`,
1 of 37. `make -C build-ver_50/tests/regression/misc check` is 37/37 with it in.

The `NEWS` entry decision 1 records gains two clauses rather than an entry of
its own, because nothing new is withdrawn — the entry already says "`write`
with no vector list now refuses…", and this change only makes that sentence
true of the command rather than of one spelling of it. What the clauses add is
the boundary, which a reader cannot derive from the sentence: `write f.raw all`
is refused in the same three cases, and `write f.raw all pi` is not.

---

## What this decision does not decide

1. **`Error during 'write': no writable vector found.` is still on raw
   `stderr`** (`postcoms.c`), so a deck cannot capture it — `0059` criterion 3
   names it as the counter-example and does not ask for it to move. Moving an
   existing message's stream is user-visible and is its own change.
2. **`com_removecirc()`'s dangling `plot_cur`**, decision 2's last paragraph.
   Filed as `doc/codex/issues/0063`.
3. **Whether the rawfile should carry analysis provenance**, which is the only
   thing that closes the consumer's half of `0059` — every row of that issue is
   a file a reader cannot distinguish from a good one, and rows 1 and 7 are now
   *absent* files rather than distinguishable ones.
4. **`vec_get()`'s fallback to the constants plot itself.** Decisions 4 and 5
   stop a bare `write` riding it into a file; they do not touch the rule, which is
   what makes `write f.raw pi` work from any plot and is a request there rather
   than a substitution. Any *named* lookup that misses in the current plot and
   hits in the constants plot still resolves silently, and a deck can still
   reach the constants file deliberately in the three ways both decks assert.
