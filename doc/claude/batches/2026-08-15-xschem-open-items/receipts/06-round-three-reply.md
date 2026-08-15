# Receipt 06 — the round-3 client reply

Item 6 of `doc/claude/batches/2026-08-15-xschem-open-items/PLAN.md`. Branch
`ver_50`, HEAD at start `c25c324e2` (item 5). **Documentation only.** Nothing
under `src/` or `tests/` was opened for editing, and nothing under
`doc/claude/upstream/` was touched.

Files changed:

- `doc/claude/feedback/ngspice_upstream/RESPONSE.md` — rewritten as round 3
- `doc/claude/feedback/ngspice_upstream/README.md` — the one bullet that
  describes which round `RESPONSE.md` is
- this receipt

## 1. The structure decision: in place, and why

**Round 3 rewrites `RESPONSE.md` in place. It is not a new document beside it.**

The house pattern is not inferred from round 2's preamble alone — it is written
down. `doc/claude/feedback/ngspice_upstream/README.md` says of `RESPONSE.md`:

> Rewritten 2026-08-14 as the **round-2** reply … Round 1's own subject … is
> restated in round-2 form rather than preserved verbatim; the round-1 text is
> recoverable from this batch's history.

And round 2's own preamble states the rule it followed: *"Round 1's text is not
preserved below. Where a round-1 claim still holds it is restated in its
round-2 form; where it does not, §1 says so by name."* Round 3 follows both
sentences literally, and its preamble says so in the same words.

Three further reasons, in the order they weighed:

1. **A second document would split the corrections from what they correct.**
   Five of round 3's items are corrections or replacements of round-2
   statements. A `RESPONSE-2.md` beside `RESPONSE.md` would leave a reader
   holding a document that is wrong in five named places and a second document
   saying so, with no way to know which they were sent. In place, the file a
   reader opens is the current one.
2. **The file has already been corrected in place twice this batch and its
   predecessor** — `4a042f0f4` (0070) and `731c01455` (the `-r` caveat). Adding
   a new file now would make the tree hold one document corrected in place and
   one appended beside it, which is the worst of both.
3. **Git holds the previous rounds.** Round 2 is recoverable at `f829c9191`
   with its two in-place corrections; the preamble names all three hashes so a
   reader can diff. Note the limit, which is real: `RESPONSE.md` was *added* to
   git by `f829c9191`, so **round 1's text is not in this repository's history
   at all** — the README already said as much ("recoverable from this batch's
   history"), and round 3's preamble does not claim otherwise.

**What was checked before deciding, not assumed:** `git log --follow` on
`RESPONSE.md` returns exactly three commits (`f829c9191`, `4a042f0f4`,
`731c01455`), and `git log --diff-filter=A` confirms `f829c9191` added it.

### Why the README was touched too

The item says "the reply document plus your receipt". The README's
`RESPONSE.md` bullet is the index entry that tells the reader which round the
file is and what its §1 corrects; leaving it saying "round-2" while the file
says round 3 would have shipped a contradiction in the same directory. One
bullet was replaced. Nothing else in that file changed, and the round-1/round-2
history sentence was kept.

## 2. What was re-measured, with the output

All against `/home/qflow/dev/ngspice_test/build-ver_50/src/ngspice` —
`ngspice-46+`, build stamp **`Sat Aug 15 18:18:34 UTC 2026`** — with
`/usr/local/bin/ngspice` (`ngspice-46`) as the baseline. `make -C
build-ver_50/src -j8` reported *"Nothing to be done for 'all-am'"* at HEAD
`c25c324e2`, so the binary carries both of this batch's code commits. Scratch
under `/tmp/claude-1000/-home-qflow-dev-ngspice-test/…/scratchpad/r3`; nothing
was left in the repo, and `git status doc/` was re-checked after the one run
that could have written into the tree (§2.10).

**Every number, transcript and table in the document below was produced by a
run in this session.** Nothing was copied from a receipt or from round 2.

### 2.1 The copy carries the file's own mode, byte-identically (0070)

Original written under `-D casemode=preserve` with the gate set; loaded by a
**folding** session with the gate set and written straight back:

```
orig.raw:5:Option: casemode=preserve
copy.raw:8:Option: casemode=preserve
$ diff <(grep casemode orig.raw | cat -A) <(grep casemode copy.raw | cat -A)
                                     <- empty
$ grep -c casemode copy.raw
1
```

Copy's variables under the folding session: `v(In)`, `v(MidNode)`. And the
nothing-recorded case:

```
file written with the gate unset   -> casemode lines: 0
that file copied with the gate set -> casemode lines: 0
```

So both of round 2's statements this corrects — the spaced re-emission and the
copier's stamp — are measured false on this build.

### 2.2 A derived plot records no mode

One folding session, gate set, on a `preserve` `.tran` file carrying the line:

```
d_copy (the loaded plot, re-written)   casemode lines=1
d_lin  (linearize)                     casemode lines=0
d_cut  (cutout)                        casemode lines=0
d_fft  (fft v(MidNode))                casemode lines=0
d_psd  (psd 1 v(MidNode))              casemode lines=0
d_spec (spec 500 20000 500 v(MidNode)) casemode lines=0
```

Contrast, same transforms on a plot the session **simulated** (`-b`,
`-D casemode=preserve`, gate set): `linearize` → `Option: casemode=preserve`,
`fft` → `Option: casemode=preserve`. Both halves are in the document.

### 2.3 The wildcard rename (0064, `25e891ec3`)

Stock `ngspice-46` is the "before" column, so every row is two live runs:

```
.op  .save v(In)  bare write   new: 1 var v(in)        | stock: 2 vars v(in) v(all)
.op  .save v(In)  write f all  new: 1 var v(in)        | stock: 2 vars v(in) v(all)
.op  .save v(In)  write f allv new: 1 var v(in)        | stock: 2 vars v(in) v(allv)
.tran .save v(In) write f allv new: 2 vars time v(in)  | stock: 2 vars time v(allv)
.tran .save v(In) write f ally new: 2 vars time v(in)  | stock: 2 vars time v(ally)
```

Case-insensitivity of the exempt set, measured rather than read: `write f ALL`
and `write f AllV` both give `No. Variables: 1`, `v(in)`.

### 2.4 The `-r` writer (0071, `731c01455`)

```
gate set:
fold         line4=<Plotname: Transient Analysis> line5=<Option: casemode=fold>        line6=<Flags: real>
preserve     line4=<Plotname: Transient Analysis> line5=<Option: casemode=preserve>    line6=<Flags: real>
distinguish  line4=<Plotname: Transient Analysis> line5=<Option: casemode=distinguish> line6=<Flags: real>
gate unset, all three: line5=<Flags: real>, Option lines=0
```

Byte-identity against the **released** binary, whole file, `Date:`/`Command:`
removed:

```
gate unset, -r, binary: build-ver_50 vs /usr/local/bin/ngspice  IDENTICAL, 1602 bytes
gate set, same build/deck, only -D casemodewrite added:         +22 bytes
len("Option: casemode=fold\n") = 22
```

The flag-spelling trap: `-D casemodewrite=TRUE` → 0 `Option:` lines,
`-D casemodewrite` → 1. And the file loads on both binaries —
`v(MidNode) : voltage, real, 59 long`, `READBACK=preserve` on each.

### 2.5 0073's duplicate, and that filter-by-name does not reach it

```
.op  .save v(In)  write f.raw v(In)
  fold        0 v(in) | 1 v(In)
  preserve    0 v(In) | 1 v(In)
  distinguish 0 v(In) | 1 v(In)
  stock-46    0 v(in) | 1 v(in)
.op  .save v(In)  write f.raw v(in)   under fold:  0 v(in) | 1 v(in)
```

The trigger, isolated:

```
write f.raw in    -> 1 variable   v(in)
write f.raw v(in) -> 2 variables  v(in) v(in)
write f.raw v(In) -> 2 variables  v(in) v(In)
```

Wider than `.op`, and the `.dc` row is the one a round-tripping consumer hits:

```
.tran  write f time v(In)          -> 2 vars  time v(In)
.dc    write f v(v-sweep) v(In)    -> 3 vars  v(v-sweep) v(v-sweep) v(In)   (all modes + stock)
.op 2-save  write f v(In) v(MidNode) -> 3 vars  v(in) v(In) v(MidNode)
```

Read-back: `load` reports variable 0 as `[default scale]`, so the phantom
becomes the loaded plot's abscissa. That is the basis for the "deduplicate on
data, not name" advice.

### 2.6 0072's abort

```
$ printf '*\n.op\n' > tiny.cir      # 6 bytes
ver_50 rc=134   stock rc=134
stderr 106 bytes, byte-identical from both:
  ngspice: ../../../src/frontend/dotcards.c:225: ft_cktcoms: Assertion `plot_cur->pl_dvecs != NULL' failed.
stdout under redirect: 0 bytes
```

Routes and controls:

```
ngspice --batch -n tiny.cir        rc=134
ngspice -n tiny.cir < /dev/null    rc=134     <- no -b needed
ngspice -b -n -r out.raw tiny.cir  rc=0       <- "No. Variables: 0"
printf 'op\n' | ngspice -p -n      rc=0
'*\n.tran 1n 10n\n'                rc=1       <- "Error: incomplete or empty netlist"
'*\nr1 0 0 1k\n.op\n'              rc=134     <- "no non-ground node", not "no netlist"
'*\nv1 1 0 1\nr1 1 0 1k\n.op\n'    rc=0
```

### 2.7 The `$sim_status` guard (0069's decision, unchanged)

Guard deck as printed in the reply:

```
fold        rc=0 fired=0 raw=[Plotname: Operating Point]
preserve    rc=0 fired=0 raw=[Plotname: Operating Point]
distinguish rc=1 fired=1 raw=[ABSENT]
unguarded distinguish: rc=0, Plotname: constants, 570B
stock, .save v(nosuchnode), no flag: guarded rc=1 fired=1 ABSENT; unguarded rc=0 constants 569B
```

Properties: `HAVE-BEFORE=0`, `VALUE-BEFORE=`, `HAVE-AFTER=1`, `AFTER-BAD=1`,
`AFTER-GOOD=0`, stderr `Error: sim_status: no such variable.`

### 2.8 The byte-count calibration round 2 gave, re-derived

```
.save v(nosuchnode), no flag:  gate OFF 570B | gate ON 592B (Option: casemode=fold)
stock ngspice-46:              569B
gate ON under distinguish:     599B
```

570/592/569 still hold. The **599** is where round 2's control-only deck number
came from — it was measured on the build that wrote the line unconditionally,
so today's default for that file is 570. The reply states the correction rather
than swapping the number silently.

### 2.9 Other rows restated in §8 of the reply

Collision warning, three modes plus no-flag plus stock (silent); deck-shape /
near-miss counts (1/1, 2/2, 1/1 with rc 0/0/1); `.save v(midnode)` vs net
`MidNode` giving `v(midnode)` / `v(MidNode)` / rc=1; the probe answering
`CCM=preserve` here and `Error: curcasemode: no such variable.` on stock; 0067
still `rc=139` on stock and `rc=0` here — and now reachable through a file the
**`-r`** writer produced, which is new.

### 2.10 Their own harness

`repro2/run_round2.sh` was **copied to scratch first** and run there, because
the script begins `rm -f ./*.raw` and the repo copy has committed `.raw` files
beside it. `git status doc/` after the run shows the directory untouched.

| their finding | this tree |
| --- | --- |
| R1 | reproduces unchanged (rc=1 with `-r`, rc=0 through `.control`) |
| R2 | reproduces unchanged, all four rows, 0 mentions of the token |
| R3 | **no longer reproduces** — `-> 0 v(In) voltage\|` alone; the stock row in the same output still shows `v(in)` `v(all)` |
| R4 | reproduces unchanged, stderr=2 |
| R5 | reproduces unchanged, including the wrong-cwd row |
| R6 | stock rc=134, this build rc=0 |

## 3. The one thing this item found that no predecessor receipt has

**`-r` on a deck with a dot-card analysis and no `.control` block gives rc=1 and
no rawfile on a failed `.save`, on stock too.** Measured:

```
                                            ver_50   stock-46
ngspice -b -n rns.cir -r out.raw            rc=1     rc=1      raw ABSENT (both)
ngspice -b -n rns.cir                       rc=0     rc=0
```

(`rns.cir` is `.save v(nosuchnode)` plus `.op` — not a case failure, no
`casemode` flag.)

Composed with three properties that were separately known, this makes one deck
shape that avoids everything still open in the correspondence at once: no 0073
duplicate column (`-r` bypasses `com_write()`), immunity to 0072 (`-r` is rc=0
on the empty deck), the case-mode header line (0071), and a correct `rc`. It is
written up as **§5a** of the reply, with its costs stated — the deck cannot name
its own rawfile, `-r` takes no vector list, and there is no block to put a
`$sim_status` guard in.

It is an exception to §7's rule and the reply says so explicitly rather than
letting the two sections disagree. Also measured, and stated in §5a: keeping a
`.control run` alongside the dot card under `-r` puts you back to **two**
analyses.

## 4. What I could **not** re-measure, and said so in the document

The reply's §10 carries this list; it is repeated here so the owner does not
have to hunt for it.

- **Their client side.** `read_dataset`, the signal browser and their probe
  wrapper are theirs; nothing here was run against any of it. Their `repro2`
  *harness* was run (§2.10) — that is the part that is ours to run.
- **Round 1's `repro/run_all.sh`** was not re-run this round. It is stated as
  "still runs", which is inherited from round 2, not re-verified.
- **The upstream patch validation** quoted in §9 (six reproducers
  134/134/134/134/134/139 → 0/0/0/0/0/0, upstream `make check` 58 PASS / 0
  FAIL) is receipt 03 of the predecessor batch's measurement and was **not**
  re-run today. §10 says so and says that the apply check is the part that
  would need repeating if upstream master has moved.
- **The `.subckt` / `.include` collision silence tables** in §8 are quoted from
  `doc/claude/decisions/0018-node-name-collision-report.md` and the guide, where
  they were measured. §10 says so.
- **`make check`** was not run: this item changed no code and no test. The
  batch's baseline is item 2's **322 PASS, 0 FAIL** at `731c01455`, and items 3,
  4 and 5 changed nothing under `src/` or `tests/` either.

## 5. Cross-references verified, not assumed

Every commit hash the reply names resolves (`git cat-file -e`): `25e891ec3`,
`4a042f0f4`, `4e738fc3e`, `611076989`, `61f519208`, `731c01455`, `7b5884249`,
`9e341a8b7`, `c25c324e2`, `eb0b96c8c`, `f829c9191`. Every issue it names exists
in `doc/codex/issues/`: 0059, 0061, 0064, 0067, 0069, 0070, 0071, 0072, 0073.
So do `doc/claude/decisions/0001-distinguish.md` (whose decision-5 withdrawal
list does contain `save`, at `:505`),
`doc/claude/decisions/0018-node-name-collision-report.md`, and
`doc/claude/casemode-distinguish-guide.md` (11 `sim_status` mentions, §9
subsections for all three of questions 2, 3 and 4).

## 6. What the owner must check before this goes out

1. **The unsent-submission language, in four places.** Lines carrying it are
   the preamble, §2's "why it is still opt-in", §9, and the summary table's
   "keep the absent-means-unknown branch" row. All four say *not sent*, none
   gives a date, and none says "submitted". `grep -n -i 'submitted\|has been
   sent\|not been sent'` over the file is the check. If the owner sends the mail
   before this reply goes out, **all four need changing together**, and §2's
   "stop expecting it to become unnecessary" sentence needs softening.
2. **Whether §5a should be recommended this strongly.** It is the document's
   most actionable advice and it is a real trade: it takes away the deck's
   ability to name its own rawfile, which is *the* reason their generator uses
   `.control write`. I stated the cost plainly and did not hide it, but if the
   owner thinks a client should not be steered off `.control write` on our
   advice, §5a is one section to soften and one summary-table row to change.
3. **Two of six items are filings and the document says so three times** — the
   preamble table, §5's and §6's headings, and §9. That was the item's
   instruction and it is worth a glance that it reads as honest rather than as
   hedging.
4. **`RESPONSE.md` is now 874 lines** against round 2's 614 as it stood at
   `c25c324e2` (561 as first committed at `f829c9191`). The growth is §1's
   five named corrections, §5a, and §6, all of which are new material; §8
   compresses round 2's §§3, 4, 6 and 7 into one section to pay for some of it.
   If the owner wants it shorter, §8 is the section that can go, because all of
   it is now in the guide.
5. **Round 1's text is in no git history here.** Stated in §1 above. If the
   round-1 file matters, it is in the batch history that predates
   `RESPONSE.md`'s first commit and someone should say where before the trail
   goes cold.

## 7. What was left

- No fix, no `src/`, no `tests/`, no `.cir` committed anywhere.
- **Nothing was sent.** No mail, no push, no PR, no network call.
  `doc/claude/upstream/` was not opened.
- `doc/codex/issues/0064` was not edited to record that the client has now been
  told about the corrected count rule and the `-r` workaround. Item 5's receipt
  asked for those facts to reach the client and they have; whether the issue
  should note that is the owner's call and it is a one-line addition.
- `LEDGER.md`'s row 6 is the driver's to fill.
