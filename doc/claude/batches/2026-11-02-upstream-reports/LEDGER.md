# Ledger: first upstream reports

Plan: the driver's brief, from the owner's "confirm the sky130 cause and draft the first 4 reports". Receipt: `receipts/01-confirm.md`. Texts: `SUBMIT.md`. Nothing posted.

| # | item | outcome | commit |
| --- | --- | --- | --- |
| 1 | sky130 cause | `940bb50fa` confirmed first bad; numparam ends at `}`, 128 lines, 49 of 49 corners; already patch ticket #138, so a comment | `a2b616508` |
| 2 | binning | leads with the hsa float pruner; the outermost max is a question | `0b455a153` |
| 3 | `(` in a node name | withdrawn as a bug, manual 2.1.3.1 and 2.1.3.4; optional forum note | `0b455a153` |
| 4 | `.dc @m1[w]` and scale | holds; `af240ba02` first bad, not reverted upstream | `a2b616508` |

## Notes

*Review.* An independent reviewer reran every deck on the first draft: 3 blocking, 9 minor. One correction round. The corrected text was not re-reviewed.

*Driver's edits after that.* Title lines added to the deck fragments in the bodies, because ngspice reads the first line as a title and a pasted `.option scale` would be lost. The fork sentence removed from report 4.

*Corrections to earlier claims.* Receipt 12's list item 3 and receipt 07 say `r3 gnd(x) 0 1k` gets value 0. It stays 1k and its nodes change. The receipts stay as written; this line records the correction.

*Driver's calls on the fork.* `58e4cf26b` stays declined on ver_52 until upstream or the owner has a reason to move it: taking it silently rewires invalid names, declining it silently loses a first OSDI parameter on a card with no space, and both are narrow. `0f763b135` keeps loading a size on the top edge. Both can be revisited.
