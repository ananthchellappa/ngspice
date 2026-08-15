# Ledger: the open items after the xschem follow-up

Driver session, 2026-08-15, branch `ver_50`. Plan is `PLAN.md`; receipts are
under `receipts/`. One crew per item, handed off one at a time. The driver did
no item work.

Starting state: HEAD `16a0b3156`. Last full `make check` was at `eb0b96c8c` —
320 PASS, 0 FAIL, identity lint baseline matched — and nothing under `src/` or
`tests/` has changed since, so items 1 and 2 are the first code in this batch.

Scope comes from the owner's answers to the four decisions the previous batch
left open: fix 0064's rename only, give the `-r` writer the line, correct
0069, file the abort, file 0064's second mechanism, write round 3. **The
upstream mail is unsent and the owner is sending it** — no crew touches it.

| item | crew | outcome | commit |
| --- | --- | --- | --- |
| 1 — fix the wildcard rename (0064, narrow) | A | **fixed, RED first** — one conjunct in `ft_evaluate()`, the wildcard set published from `get_all_type()`. misc 36/36, full check **321 PASS, 0 FAIL** | `25e891ec3` |
| 2 — the `-r` writer records the case mode (0071) | B | **fixed, RED first** — gated arm in `fileInit()`, gate-unset output byte-identical to stock. pipe 33/33, full check **322 PASS, 0 FAIL** | `731c01455` |
| 3 — correct 0069's Resolution | C | **corrected** — four runnable decks inline, three phantom filenames gone, whole table re-measured; item 1's fix had already moved a row | `61f519208` |
| 4 — file the `.op`-with-no-netlist abort | D | **filed** — `doc/codex/issues/0072`, six-byte reproducer, identical on stock; corrected the item's framing twice | `611076989` |
| 5 — file 0064's second mechanism | E | **filed** — `doc/codex/issues/0073`; the client's claim confirmed and found wider than 0064 said, receipt 05's `.tran`/`.dc` row corrected | `c25c324e2` |
| 6 — the round-3 client reply | F | **written** — `RESPONSE.md` rewritten in place, five round-2 statements corrected by name, six items reported as four fixes and two filings | `0e28d789c` |

## Notes

Six crews, six items, one at a time. Two items changed behaviour and both went
RED first. Full `make check` after item 1: **321 PASS, 0 FAIL**; after item 2:
**322 PASS, 0 FAIL**, identity lint baseline unchanged in both. Items 3 to 6
touched no code.

Every crew re-measured rather than carrying a number over, and four of them
falsified something they had been handed:

- **Item 1** exempted `alle` as well as the four documented wildcard tokens,
  deciding it from `findvec()`'s dispatch rather than from the issue's list,
  and noted that `print alle` produces no output in this tree either way.
- **Item 3** found that item 1's fix had already moved a row in 0069 — the
  fold/preserve raw now holds one variable where it held two yesterday — which
  is the drift the item existed to catch. It also found the stale byte counts
  in that issue came from the pre-gate build and return exactly when
  `casemodewrite` is set.
- **Item 4** corrected the framing it was given, twice: it is the `.op` **dot
  card** that aborts, not `op` inside a `.control` block, which exits 0; and
  batch mode needs no `-b`, only the absence of a tty. Six-byte reproducer.
- **Item 5** confirmed the client's claim and found it wider than 0064 said:
  `fold` is not exempt, and `.tran`/`.dc`/`.ac` do inflate the count when the
  deck names the scale itself — correcting the previous batch's own receipt.
  It also found the guard it was asked about exists because `rawfile.c` has no
  NULL test and would walk off the tail without it.
- **Item 6** found that a `-r` deck with no `.control` block escapes 0073 and
  0072 both, and gives rc=1 with no artefact on stock. Written up for the
  client as §5a.

## Waiting on the owner

1. **Send the upstream mail.** Still unsent, deliberately untouched by this
   batch. Item 6 flags that four places in the round-3 reply — the preamble,
   §2, §9 and one summary row — have to change together if it goes out before
   the reply does.
2. **Read round 3 before it goes out.** Item 6's receipt names the one call it
   made that the owner may want to reverse: whether §5a steers the client off
   `.control write` too hard, given that it costs them the deck-named rawfile.
3. **0072** — guard at `outitf.c:479` or at `dotcards.c:225`, here or upstream,
   and how any deck could assert a fix: no existing harness checks exit status.
4. **0073** — whether an `.op` plot's pseudo-scale stops being prepended to a
   partial write, or the issue closes as working-as-intended there.
5. **0069 criterion 3** — there is still no `sim_status` deck anywhere under
   `tests/`, now stated plainly in the issue rather than left implied.
