# Stage 13 receipt — crew Q

Crew N corrected a false claim in `doc/claude/decisions/0018` decision 3 and
put a new one into the very paragraph it wrote to remove the old. Its verifier
caught it. This stage re-measures all three modes, corrects the paragraph,
checks the claim did not travel, re-reads the whole of decision 3 for further
staleness, and fills the hole crew L left in `receipts/`.

`doc/` only. No file under `src/` or `tests/` is modified by this stage —
`src/spicelib/parser/inpsymt.c` was mutated three times for measurement and
restored byte-for-byte each time (md5 checked against a pre-work copy).

## RESULT per item

### 1. The false measurement — REPRODUCED FALSE, CORRECTED

The sentence, `0018` decision 3, "A consequence gap 3 does NOT have":

> Shipped, it prints one undefined-node line; with the outer guard deleted it
> prints that line and the pair line both, **in `fold` as in the other two
> modes**. The guard is therefore load-bearing in every mode …

Measured on the verifier's own deck, copied to scratch and run there, never in
the source tree:

```
.options noacct rshunt=1e9
V1 in 0 dc 0 ac 1
R1 in 0 1k
.noise v(MISS) V1 dec 10 1 100
.tf v(Miss) V1
```

`build-ver_50/src/ngspice -b -D casemode=<mode>`, one rebuild per column:

| mode | shipped | outer `if (t->t_unclaimed) continue;` deleted | inner `!u->t_unclaimed` deleted | both deleted |
| --- | --- | --- | --- | --- |
| `fold` | `no node named 'miss'` ×1 | ×1 **+ `… 'MISS' and 'Miss' … name one node (casemode=fold)`** | ×1, unchanged | ×1 + the pair line |
| `preserve` | `no node named 'MISS'` ×1 | ×1 **+ the pair line (`casemode=preserve`)** | ×1, unchanged | ×1 + the pair line |
| `distinguish` | `no node named 'Miss'` and `no node named 'MISS'` — **×2** | ×2, **unchanged** | ×2, unchanged | ×2 + `… name two nodes (casemode=distinguish)` |

So the sentence is false for `distinguish`, in three separate ways: the deck
does not print *one* undefined-node line there but two, the entry it describes
(`t_spelling = MISS` with `t_casetwin = Miss`) does not exist there at all, and
the outer-guard mutation changes nothing.

**Why.** Under `fold` and `preserve` `ent_eq()` merges, so both spellings land
on one entry and `t_casetwin` carries the second; the outer guard is the only
thing between that entry and the pair line. Under `distinguish` `ent_eq()` is
`strcmp`, so they are two entries, neither has a `t_casetwin`, and the pair is
reachable only through the `distinguish`-only chain scan — whose own
`!u->t_unclaimed` rejects the other entry for exactly the reason the outer one
rejects this one. Both guards must go. The same asymmetry explains the "×2":
the first scan's twin lookup is `!u->t_unclaimed` too, so with neither side
claimed `distinguish` prints the plain undefined-node line twice rather than
the near-miss line once.

**The guards are still load-bearing in every mode** — that half of the struck
sentence survives, but this deck is not what shows it under `distinguish`.
Re-ran stage 12's mutation matrix against the three guard decks to place it
properly:

| mutation | `misc` | `case` | `casedist` |
| --- | --- | --- | --- |
| shipped | PASS | PASS | PASS |
| outer deleted | **FAIL** | **FAIL** | **FAIL** (NMDEF, `pair5 0 → 1`) |
| inner deleted | PASS | PASS | **FAIL** (NMREF, `pair6 0 → 1`) |

which is `0068` criterion 5's table reproduced row for row.

Corrected in `doc/claude/decisions/0018` decision 3: the struck sentence is
quoted, the three-mode table replaces it, the mechanism is stated, and the
"load-bearing in every mode" claim is re-attributed to NMDEF/NMREF.

### 2. Did the claim travel? — NO to `0068`, ANNOTATED in the ledger

| document | verdict |
| --- | --- |
| `doc/codex/issues/0068` criterion 5 | **clean.** Its outer-guard sentence is scoped to `fold` and quotes the `casemode=fold` line. Its four-column mutation table is correct. Added a paragraph making the `distinguish` column explicit and deleted the loose clause "It was measured in all three modes", which is the ambiguity `0018` read as a licence to generalise. |
| `LEDGER.md` stage 12 P1 | **clean but loose** — "The verifier's deck reproduces on this tree, in all three modes". Struck in place with the correction, in the dated style stage 12 itself used on stage 11. Its outer-guard sentence is correct and now says so explicitly. |
| `receipts/stage12-crewN.md` | **correct.** Crew N measured **both** guards deleted and reported it that way, including the `two nodes (casemode=distinguish)` line. The error was introduced only when the finding was written into `0018` as an outer-guard result. Row annotated with a pointer to this stage. |
| `src/spicelib/parser/inpsymt.c` `term_insert()` comment | **clean.** It claims only that an unclaimed entry *can* hold a spelling and a case twin under `fold`, which is true. No edit. |
| the three guard decks' headers | **clean.** `misc`'s NMPAIR header says "Only the outer guard is reachable in this mode"; `casedist`'s says the inner guard is what holds NMREF. Both correct. No edit. |

### 3. The rest of decision 3, re-read and re-measured — NOTHING ELSE STALE

Corrected twice in two days, so every falsifiable claim in it was checked
rather than read:

| claim | check | result |
| --- | --- | --- |
| gap 3's six-row `B`/`E`/`G`/`R` table | six decks in scratch, `fold` and `preserve` | reproduces exactly: `B` node field, `B` expression, `E` `vol=`, `G` `cur=`, `R` `R=` all report under `preserve` and are silent under `fold`; the plain linear `E1 o 0 mid 0 2` control reports in both |
| gap 4, XSPICE `a` card brackets | `a1 [ in ] […]` vs `a1 [in] […]`, the only source of the second spelling | reproduces: spaced reports under `fold`, tight does not; `preserve` reports both ways |
| `(`/`)` are boundaries | `card_word_boundary()` source | confirmed; the `card_spelling("BVAC IN 0 V = sin( …*V(VFreq)*V(VTime))", "vfreq") = VFreq` claim is consistent with it, and the cited line exists at `examples/various/FFT_Leakage.cir:5` |
| `line_case` appended, not inserted | `src/include/ngspice/inpdefs.h` | confirmed last field of `struct card`, after `nf`/`compmod` |
| auto-bridge clears `line_case` | `src/xspice/evt/evtcheck_nodes.c:249` | confirmed, `tfree` + `NULL` |
| gap 1 deck citations | `Fig27_12_see.sp` (`Vr` on `M2`/`Rbias`, `x1.vr` in the `a` card), `global-node-case.cir` and `name-lookup-case.cir` (`.global vss` + `rb b VSS`) | all present |
| gap 2 deck citation | `FB14.cir` `Vcc` (`:399`, `:416`) vs `vcc` (`:423`) | present |
| the rejected deck-global witness's scope example | `examples/p-to-n-examples/switch-oscillators.cir:41` `.subckt invertern In Out VDD DGND` with body `Rl out vdd 10k` | present |
| "236 B cards in 53 of the 875 files" | a looser regex over `tests/` + `examples/` gives 74 files of 892 | right order, and the text already says syntactic-not-exact — left alone |
| decision 2's "deleting one guard at a time fails exactly one of them" (NMDEF / NMREF) | the mutation matrix above | confirmed |
| decision 3 item 1's "all three decks guard this" | BODY case present in all three `node-case-collision-report.cir` | confirmed |

### 4. `receipts/stage11-crewL.md` — WRITTEN, marked reconstructed

`receipts/` ran `stage1-*` … `stage10-final` plus `stage9-crewF`, then jumped
to `stage12-crewN`. Crew L wrote nothing. The new file opens with a blockquote
saying it was assembled after the fact by crew Q from the ledger's stage 11
section and the shipped text of `0018`/`0068`, that every other receipt in the
directory is first-hand and this one is not, and that an unmarked number in it
is a quotation rather than a measurement. Each section carries its provenance,
"Files changed" marks the one row that is inference, and a "What could not be
reconstructed" section names the four things nobody can recover: crew L's
commands, its RED/GREEN hunks, whether each `.out` was hand-written before
first green, and its own residuals.

Crew N's residual — "the corrected sentence must not be copied into it" — is
honoured: P2's closing sentence appears there only as the thing that was
struck, with a line saying it must not be quoted from that receipt as a
finding.

### 5. "Seven call sites" — CORRECTED where it was written

`INPtermInsertRef()` has **seven calling functions** and **ten call sites**:
`mkvnode()` (`inpptree.c:1259`), `INPgetValue()` (`inpgval.c:94`), and
`dot_noise()` (`inp2dot.c:58`, `:64`), `dot_tf()` (`:377`, `:382`),
`dot_sens()` (`:502`, `:508`), `dot_pss()` (`:685`), `dot_hb()` (`:784`) —
the first three of the `inp2dot.c` five call it twice each.

Every shipped document says "callers", which is correct: `0018` decision 2 and
decision 3, `0068` criterion 5's correction, and `LEDGER.md` stage 12 P1. The
wrong noun is `receipts/stage12-crewN.md` only, whose own line list already
showed ten. Annotated there with the date and the reason; `0018` decision 3
now states both numbers so it does not have to be re-derived again.

## Files changed

| file | why |
| --- | --- |
| `doc/claude/decisions/0018-node-name-collision-report.md` | decision 3's gap-3 consequence paragraph: the false sentence quoted and replaced by the measured three-mode table, the mechanism, and the re-attribution of "load-bearing in every mode" to NMDEF/NMREF. "Seven callers" gains the ten call sites. |
| `doc/codex/issues/0068-a-node-name-spelled-two-ways-is-never-reported.md` | criterion 5: the loose "measured in all three modes" clause replaced by an explicit `distinguish` paragraph, so the outer-guard result cannot be read as a three-mode result again. |
| `doc/claude/batches/…/LEDGER.md` | stage 12 P1 annotated in place (twice); stage 13 section added; log row added. |
| `doc/claude/batches/…/receipts/stage11-crewL.md` | **new** — the reconstructed stage 11 receipt. |
| `doc/claude/batches/…/receipts/stage12-crewN.md` | "seven call sites" → seven calling functions / ten call sites, dated; the decision-3 row gains a pointer to this stage's correction. |
| `doc/claude/batches/…/receipts/stage13-crewQ.md` | this receipt. |

Nothing under `src/` or `tests/`.

## Tests

`make -C build-ver_50/tests/regression/<dir> check TESTS=node-case-collision-report.cir`
for `misc`, `case` and `casedist`, cached `*.log`/`*.trs` deleted before each
run; plus the scratch decks run by hand with `-D casemode=`.

## RED

This stage changes no behaviour, so the RED is the falsification itself: the
shipped sentence predicts a pair line under `distinguish` with the outer guard
deleted, and the measured run does not produce one. That is the table in item
1, column 3, row 3 — `2 lines, unchanged`.

The three mutations were applied with `Edit` on an exact string and reverted
with `Edit` on the exact inverse, never with `cp`. `src/spicelib/parser/inpsymt.c`
was copied to scratch before any of them and `diff -q` against that copy after
the last revert returns identical (md5 `e35eba2c…` before and after), and the
tree was rebuilt from the reverted text before the green runs.

Mutations used, all temporary:

1. outer `if (t->t_unclaimed) continue;` removed from the second scan of `INPtermCaseCheck()`
2. that plus inner `!u->t_unclaimed &&` removed from the chain scan
3. inner only

## GREEN

Shipped code, rebuilt from the restored source:

```
tests/regression/misc      PASS: node-case-collision-report.cir
tests/regression/case      PASS: node-case-collision-report.cir
tests/regression/casedist  PASS: node-case-collision-report.cir
```

## Suite status

`make -C build-ver_50 check` from cleared `*.log`/`*.trs` under
`build-ver_50/tests`: **315 PASS, 0 FAIL, 0 XFAIL, 0 SKIP**, `EXIT=0`.
Identity lint 264 comparisons plus the 11-comparison selftest, baseline matches
both. Same 315 as stage 12 — expected, since this stage adds no deck.

One `ERROR: (internal)  tried to destroy non-existent graph` line appears in
the captured output, immediately before
`PASS: vector-probe-report.cir` in `tests/regression/casedist`. It is a
plotting message on a passing deck's stream, not a test result — the automake
`FAIL:` and `ERROR:` result counts are both zero and `EXIT=0`. Not this
stage's: nothing here touches plotting, and the deck is unmodified. Named so
the next reader does not read it as a new failure.

## Residuals

- **The `0018` correction is now three deep on one paragraph.** Crew H wrote
  it, crew L falsified its cause, crew N falsified its `t_unclaimed`
  consequence and introduced a mode error, crew Q corrected that. The paragraph
  now carries its own history, which is right for a decision record and is also
  a sign that this is the hardest thing in the document to state correctly. A
  reader who needs only the current behaviour should read the three-mode table
  and nothing else in it.
- **No deck asserts the three-mode table.** It is a mutation measurement, and
  mutation results cannot be frozen in `TESTS` — what *is* frozen is the pair
  of guards, by NMDEF/NMREF/NMPAIR. If the guards are ever restructured the
  table becomes stale silently, exactly as the sentence it replaces did.
- **`stage11-crewL.md` is testimony about a session nobody has.** It adds no
  independent confirmation of crew L's work; the ledger section and the shipped
  text remain the only evidence. Flagged in the file itself.
- **Untracked byproducts under `examples/` and `tests/`, not this crew's.**
  Present when this stage opened and left alone: `examples/plot/py*.{py,data}`,
  `examples/measure/meas.out`, `examples/control_structs/s3046.s2p`, and the
  large set of `tests/regression/*` decks and captures belonging to the
  concurrent crew. No tracked file outside this stage's six is modified by it.
- **Changes wanted outside this crew's files: none.** The `term_insert()` and
  `card_word_boundary()` comments in `src/spicelib/parser/inpsymt.c` and the
  three guard decks' headers were all checked against the new measurement and
  all read correctly as they stand.
