# Next-session prompt — subcircuit formal pins, the last acceptance bullet

Paste everything below the line into a fresh session at
`/home/qflow/dev/ngspice_test`, branch `ver_50`, at `b48b765d8`.

Why this one next: it is the **only** item left on
`doc/claude/specs/case-sensitive-identifiers.md`'s own acceptance list —
"subcircuit formal pins and the rawfile are untested" — and measuring it
found that it is not a testing gap but a **silent wrong topology in
`preserve`, a mode that has shipped**. Every other open issue is bycatch found
along the way; this one is the definition of done.

---

Fix `doc/codex/issues/0049` and close the spec's acceptance list.

Read, in this order and in full:

1. `doc/codex/issues/0049-subcircuit-formal-pins-are-matched-byte-exactly.md`
   — the issue, with the measured three-mode table and the two sibling call
   sites forty-five lines apart.
2. `doc/claude/decisions/0001-distinguish.md` **decision 3**, Class A, and the
   two rows for `src/frontend/subckt.c`. `eq_substr_id()` is on that list;
   `gettrans()`'s use of bare `eq_substr()` is not, and why it was missed is
   the first thing to say in the record.
3. `doc/claude/decisions/0003-event-node-near-miss.md` and `0001` decision 3's
   "Two further Class A sites were added with Phase 3 gates 2 and 4" — the
   same discovery shape: a bare comparator, filed under `distinguish`, that
   turned out to move `preserve`. Read what those records concluded about
   which mode a bare comparator actually breaks.
4. `doc/claude/decisions/0002-deferred-node-resolution-check.md` — decision 2's
   resolution-versus-definition rule, because the `distinguish` half of `0049`
   is a resolution that misses in silence.
5. `doc/claude/decisions/0011-identity-lint.md` decisions 4 and 5, before you
   touch `src/`. `make check` now runs a lint that freezes every string
   comparison of two runtime operands; changing `eq_substr(...)` to
   `eq_substr_id(...)` **will** fail it in both directions, and clearing that
   correctly is part of the change, not an obstacle to it.

## The state, measured at `b48b765d8`

```c
    for (i = 0; table[i].t_old; i++)
        if (eq_substr(name, name_end, table[i].t_old)) {   /* subckt.c:1705 */
```
```c
                if (eq_substr_id(xname, xname_e, subs->su_name))  /* :1734 */
```

Reproduction, three decks differing only in which side is upper case:

```
.subckt div IN OUT
r1 in out 1k
r2 out 0 1k
.ends
v1 a 0 dc 3
x1 a b div
```

| deck | `fold` | `preserve` | `distinguish` |
| --- | --- | --- | --- |
| `.subckt` UPPER, body lower | 1.5 V | **no `v(b)`** | no `v(b)` |
| `.subckt` lower, body UPPER | 1.5 V | **no `v(b)`** | no `v(b)` |
| both lower | 1.5 V | 1.5 V | 1.5 V |

A miss is not an error: `translate_node_name()` prefixes the name with the
instance scope instead, so the body's `in` silently becomes `x1:in` and the
subcircuit is disconnected. The only sign is `Warning from checkvalid: vector
b is not available or has zero length`, which names neither pins nor case.

## The decisions this session has to make and record

1. **Which mode is this fix for?** State it before writing it. `preserve` is
   the mode that moves and the mode that has shipped decks; `distinguish`'s
   *result* is already right — two spellings are two names — and only its
   *silence* is wrong. Do not let the `distinguish` heading of the parent
   series make you claim a `distinguish` fix. `0001` decision 3's last two
   Class A rows are the precedent and say so explicitly.

2. **Does the `distinguish` half get a diagnostic?** `0001` decision 2 says a
   resolution that misses beside a case variant reports. This is a resolution
   and it misses in silence. But the near-miss is inside `subckt.c`'s
   expansion pass, which runs before any node exists, so the helper
   `vec_warn_case_near_miss()` and `INPtermCaseCheck()` are both out of reach.
   Decide: implement it here, defer it to the parser's existing end-of-parse
   scan (`doc/claude/decisions/0002`), or record why it stays silent. A
   deferral is defensible; an unstated silence is not.

3. **Does `settrans()` need the same edit?** `:1621` builds `table[]` from the
   `.subckt` line and the invocation. If two spellings of one pin can land in
   two rows, fixing only the lookup leaves the table wrong. Read it; say what
   you found either way.

4. **Is the `.subckt` *name* row symmetric?** `:1734` uses `eq_substr_id()`.
   Check the other direction — a `.subckt` defined upper case and invoked
   lower case, and the reverse — and say whether it is already right. If it
   is, that is a one-line finding, not a new issue.

5. **What does the lint do to this change, and what do you do to the lint?**
   `tests/lint/identity.baseline` has three `src/frontend/subckt.c` entries, at
   its lines 180-182: the declaration and the definition of `eq_substr()`
   itself, and `eq_substr(name, name_end, table[i].t_old)`, which is the call
   you are changing. That third one leaves the tree with your fix and its
   baseline line must go with it; `make check` prints the exact line, and the
   run fails until you delete it. Do **not** regenerate the baseline. If
   the fix introduces a new comparison, classify it in the source with a
   `/* case-lint: ... */` comment naming the reason, per `AGENTS.md`.

## Deliverables

- The fix, one line, plus whatever decisions 2 and 3 require.
- A **RED** in `tests/regression/case/` (mode `preserve`): a `.subckt` whose
  formal pins are upper case and whose body spells them lower case gives the
  same node voltage as the all-lower-case deck. It must fail at the parent
  commit with the voltage **absent**, not merely different — quote the failure.
- A second deck in `tests/regression/casedist/`: the two spellings stay two
  nets under `distinguish`, and decision 2's answer is asserted whichever way
  it went. `tests/regression/casedist/` has twenty-one decks and none touches
  a subcircuit pin; this is the first.
- The record, `doc/claude/decisions/0012-<slug>.md`, and a Resolution on
  `doc/codex/issues/0049`.
- **The spec's acceptance list updated.** Its `distinguish` section still says
  "Subcircuit formal pins and the rawfile are untested", and its last bullet
  still reads as though `doc/codex/issues/0028` were open — `0028` closed on
  2026-08-11. Correct both. If the rawfile half is still untested after this
  session, say so in one sentence rather than leaving the bullet ambiguous.

## Baselines, measured at `b48b765d8`

- full `make check` = **276 PASS, 0 FAIL**. That is the bar. It includes
  `tests/lint`'s two tests, which are new with `0011`.
- `make -C build-ver_50/tests/lint check` runs the lint alone in ~1.6 s. Use it
  after every `src/` edit; do not wait for the full suite to learn you moved a
  comparison.
- `python3 doc/claude/scripts/case_differential_sweep.py --jobs 8 --timeout 90`
  = **315 decks, DIFF=69, OK=243, SKIP=3**, `PARSE-FAIL=0`, `NUM-DIFF=0`. The
  last two must stay 0. Compare per deck, not against the totals: the four
  `alter-rebin` decks give DIFF=1, 4 and 3 on three consecutive runs against a
  **single** binary, so the total alone is worth ±2.
- `python3 doc/claude/scripts/deck_output_differ.py --before X --after Y
  --jobs 8 --timeout 200 --mode {,preserve,distinguish}` = **fold 2, preserve
  2, distinguish 1** at no change, every one of them an `alter-rebin` deck.
  This fix **should** move decks in `preserve` — that is the point — so the
  interesting question is which ones and whether every one of them contains a
  subcircuit whose pins differ in case from its body. Expect the sweep to move
  too, in the direction of *fewer* `DIFF`s.
- **Use `--timeout 200`, not the default.** `tests/mesa/mesa12.cir` takes
  ~143 s on any binary and reports as `rc TIMEOUT -> 1` at 120 — a MOVED line
  that is not a difference and that does not look like a flap, because
  re-running the deck serially makes it vanish instead of reproducing.
- Decks that flap run to run: the four `alter-rebin*`, `bsim3soidd/ring51.cir`,
  `bsim3soifd/ring51.cir`, `tests/general/mosamp.cir` and
  `tests/regression/model/binning-1.cir` (both on a ` Reference value` line
  that appears only under load), and `tests/mesa/mesa12.cir` (wall-clock
  banner). Re-run any MOVED deck against the **same** binary before calling it
  a regression.

## Hazards

- `tests/.gitignore` ignores `*.out`; every reference needs **`git add -f`**.
- `./autogen.sh` after any `Makefile.am` edit, then `../configure` in
  `build-ver_50`.
- A `.cir` absent from its directory's `TESTS` is silently never run.
- The `egrep -v` filter (`tests/bin/check.sh:20`) eats any line containing
  `Warning`, `Error`, `Index`, `time`, `trans`, `est`, `urrent`, `nalysis`,
  `Data`, `Operating` and about thirty more. It is **case sensitive**. This
  bites here: `Warning from checkvalid` is exactly what a broken pin produces
  and the filter drops it, so the deck must assert on a **value**, not on the
  warning — or capture `cp_err` with `>&` and read it back the way
  `tests/regression/casedist/vector-unlet-report.cir` does.
- A shell `cd` persists between commands, **including into backgrounded ones**.
  Use absolute paths, or `cd` inside the backgrounded command.
- Do not write a background wait loop that greps `ps` or `pgrep -f "<string>"`
  for a command string — it matches its own command line and never exits.
- Never pipe `make` through `head`; SIGPIPE kills it and leaves a stale binary.
- **Re-grep every line number after your edits.**
- **Snapshot the binary before you measure** — `cp build-ver_50/src/ngspice
  <scratch>/ngspice-baseline`.

## Deliberately out of scope — enumerate, do not fix

- `doc/codex/issues/0047`, `plotit()` finding its `vs` separator by a failed
  lookup — the largest of the open ones.
- `doc/codex/issues/0048`, a control variable dispatched with `eqc()` and read
  back with `eq()`, so `set WIDTH=200` is silently ignored while
  `set NUMDGT=12` is honoured. Filed with `0011`; mode independent.
- `doc/codex/issues/0039`, the `.param` ambiguity diagnostic still on `stderr`
  and so unguardable — the cheapest remaining item, one token plus a deck.
- `doc/codex/issues/0046`, the duplicate near-miss report before the first
  analysis; `0043`, the command forms of `.sens`/`.tf`/`.noise`/`.pz`; `0038`,
  the per-command fold exemption; `0042`, `remcirc` under `-r`; `0041`, the
  `load` path leak; `0021`, the stale codemodel crash; `0031`, the family-less
  node; `0033`, `cp_remkword()`'s spelling; `0035`, all three items.
- `com_let()`'s `plainlet` leading space, `0009` deferral 4.
- The **rawfile** half of the acceptance bullet, unless it falls out for free.
  It wants its own deck asserting that two case-variant vectors are separately
  present in a `-r` rawfile under `distinguish`; say whether you did it.

Anything new you find that you do not fix goes in
`doc/codex/issues/NNNN-slug.md` with the Status / Summary / Impact / Root
Cause / Acceptance Criteria / Resolution structure. Next free number is
**0050**.

## Conventions

- Match the style of the file being edited; `src/frontend/` is four-space,
  same-line brace. Do not reflow.
- One commit per mechanism, one for the documentation.
- Commit messages: short imperative summary, then problem, mechanism, RED
  evidence. **State what moves in each of the three modes** — and this time
  something does move, so name the decks.
- **Review your own commits adversarially before declaring it done.** Every
  session since `0027` has found something in its own work this way, most often
  in its own test artefacts rather than in the fix. `0011` found that a commit
  message credited `AGENTS.md` to the wrong commit, a stale line count, a
  cold-cache timing quoted as steady state, and a table left holding
  pre-audit numbers. Ask: is every token I assert reachable, can any assertion
  pass vacuously, does each artefact fail for the reason I say, is every
  factual claim in my commit message something I ran against *this* commit,
  and does `git show --stat` list only files I meant to touch?

## Stop and report

What changed per file, the RED before and the PASS after, your answer to
decision 1 and what it rests on, decision 2's answer for the `distinguish`
silence, what `settrans()` turned out to be, how you cleared the lint's
baseline entry, which modes move and **which decks**, the before/after sweep
and differ verdicts compared per deck, your adversarial review of your own
commits, whether the spec's acceptance list is now closed or what is left of
it, and the full `make check` result.
