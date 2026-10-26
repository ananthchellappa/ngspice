# Batch ledger — Monte Carlo and sensitivity support for ver_51

**Branch** `ver_51`, created 2026-10-25 off `ver_50` at `ccebdf2a2`, local only, not pushed. **HEAD at start** `ccebdf2a2`. **Baseline** not yet
measured.
Plan: `PLAN.md`. Receipts: `receipts/` (`00-scoping.md` written at planning).

## Items

| # | item | crew | outcome | commit |
| --- | --- | --- | --- | --- |
| 1 | P1: carry the R model through the R→B rewrite | — | planned | — |
| 2 | P2: warn when `.option seed=` overrides `setseed` | — | planned | — |
| 3 | P3: stop seeding from the process id at load | — | planned | — |
| 4 | P4: the draw log, `set rnddraws=<file>` | — | planned | — |
| 5 | P5: native `mcrun` | — | planned (deferred) | — |
| 6 | close the batch | — | planned | — |

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

## Waiting on the owner

Nothing at present.

## Possible later asks (not in this batch)

- Reproducible TRNOISE across loop iterations (`setseed` refilling the Wallace
  pool). xschem doesn't need transient-noise MC yet.
