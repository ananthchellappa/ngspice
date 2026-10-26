# Batch ledger — Monte Carlo and sensitivity support for ver_51

**Branch** `ver_51`, created 2026-10-25 off `ver_50` at `ccebdf2a2`, local only, not pushed. **HEAD at start** `ccebdf2a2`. **Baseline**
335 PASS / 0 FAIL (`ae2223e41`, binary `2976d0243ee800c7f461`).
Plan: `PLAN.md`. Receipts: `receipts/` (`00-scoping.md` written at planning).

## Items

| # | item | crew | outcome | commit |
| --- | --- | --- | --- | --- |
| 1 | P1: carry the R model through the R→B rewrite | 01 | landed, review pass, 338 PASS / 0 FAIL | `4e614b4da`, `16e41e0d8` |
| 2 | P2: warn when `.option seed=` overrides `setseed` | 02 | landed, review pass, 342 PASS / 0 FAIL | `1c58365bd`, `a51568215` |
| 3 | P3: stop seeding from the process id at load | 03 | landed, review pass, 344 PASS / 0 FAIL | `5243f8e3b`, `e955bc093` |
| 4 | P4: the draw log, `set rnddraws=<file>` | 04 | landed; review 1 **blocking** (reset order in `totalreset`), fixed; review 2 pass; 347 PASS / 0 FAIL | `29940ed39`, `8ce1ea5e9`, `a269a8a54`, `773e60c4d` |
| 5 | P5: native `mcrun` | — | deferred | — |
| 6 | close the batch | 06 | closed, 347 PASS / 0 FAIL re-verified | `b115681c2` |

The range is `a664faa2d..b115681c2`, 12 commits. Five of them change behaviour
(`4e614b4da`, `1c58365bd`, `5243f8e3b`, `29940ed39`, `a269a8a54`), and each went
RED first; the receipts carry the failing output. `a269a8a54` changes only
`sharedspice.c`, so the `ngspice` binary hash didn't change across it.

**Final `make check`** at close: **347 PASS / 0 FAIL**, `make` exit 0. XFAIL,
SKIP and ERROR are all zero. Both lint specs match the baseline (264 and 11
comparisons), and `identity.baseline` is unchanged across the batch. The final
binary is `e53fb551360f6cea4f69`. The owner's two uncommitted edits are still
uncommitted, and the range doesn't touch them.

## Notes

Owner decisions 1–8 in `PLAN.md` are open. Decision 7, the log contract, also
needs xschem's agreement.

*2026-10-24, xschem side (session `xschem-private-da`).* The xschem session
accepted every log-contract change in decision 7: the flattened
`instance_path`, `param` with the `#n` suffix, `value` as the raw draw, the
header/footer framing with the `offset != 0` drop rule, `unlogged_calls`, and
`set`/`unset rnddraws` re-set after `ngSpice_Reset`. It also accepted the
pushback in decisions 3 and 4 and the P1 acceptance changes. This is not the
owner's approval: decisions 1–8 still need the owner.

*Added to item 4 at xschem's request.* When P4's in-tree test lands, also
commit a sample log as a parser fixture: a few record sets from a two-instance
sky130 mismatch deck, kept in this batch dir as `fixtures/rnddraws-sample.tsv`.

*2026-10-25, owner, in this session.* The owner answered: decision 1 yes,
create `ver_51` off `ver_50`; decisions 2–8 go with the recommendations in
`PLAN.md`.

*Driver note on `build-ver_51-shared/`.* Receipt 06 A1 asks whether this tree
was sanctioned. It was. The driver's item-4 brief granted one extra tree,
configured `--with-ngshared`, for the `ngSpice_Reset` probe, overriding PLAN's
"no second configured tree" for that item only. It is untracked and covered by
`build*/`. It is the only place the shared-library paths of items 2–4 have run.

*What the crews falsified* is in `receipts/06-close.md` §"What each crew
falsified". The headline ones:
- P1's `{eq}`-first model card was commented out before the rewrite ran
  (`get_model_name()`, fixed in the same commit).
- A "duplicates that disagree" check can't be written without changing the lint
  baseline, so duplicate models count as not resolved.
- P1's size: +366/−10 lines.
- P4: `totalreset` zeroed the RNG counter before the log footer was written.

## Release note (ver_51)

- **P1 changes simulation results.** Every R that is rewritten as a B source
  and names a `.model` now gets that model's tc1/tc2/tnom, plus the instance's
  dtemp/temp. That includes sky130 `xhigh_po`. A rewritten R whose expression
  uses `temper` and whose model has a TC gets the TC plus a warning; a PDK patch
  that already folds the TC into the expression must be reverted. Thermal noise
  of a rewritten R stays behind `enable_noisy_r` / `noisy=1`, and now uses the
  model. New warnings: unresolved or ambiguous model, `tce`.
- **P2** adds a once-per-session warning when `.option seed=` overrides an
  explicit `setseed`. The draws don't change.
- **P3 makes decks without `setseed` reproducible across processes.** The
  startup stream equals `setseed 1`, TRNOISE included, and `setseed N` in
  `.spiceinit` now takes effect. For runs that differ, use
  `.option seed=random` or `unset rndseed`.
- **P4** adds `set rnddraws=<file>`: a log of every parse-time random draw,
  in the format agreed with xschem.

## Waiting on the owner

1. ~~Keep or delete `build-ver_51-shared/`.~~ *2026-10-26:* the owner said
   delete, and the driver deleted it. Its probe source is kept in
   `probes/rnddraws_reset_probe.c`. From now on nothing exercises the
   `libngspice` reset paths of items 2–4. Their only checks were the probe run
   recorded in receipt 04 and the syntax checks in receipts 02 and 03.
2. The minor review issues and the deliberate gaps (`receipts/06-close.md`
   §B and §C): fix them in a follow-up batch, or accept them. The two with most
   effect are B9, glibc-only decks with no skip on other libcs, and B10, the
   `visualc/*.vcxproj` files not updated for item 4's new sources.
3. Upstream for P2/P3 (decision 8): a separate batch, if wanted.
4. Push `ver_51`. That needs your SSH key.

## Possible later asks (not in this batch)

- Reproducible TRNOISE across loop iterations (`setseed` refilling the Wallace
  pool). xschem doesn't need transient-noise MC yet.
