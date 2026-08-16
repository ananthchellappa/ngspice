# Upstream material for `doc/codex/issues/0072`

Two files, prepared for an upstream maintainer.

- **`0001-frontend-report-an-op-card-with-no-netlist.patch`** — the production
  change alone, `src/frontend/dotcards.c`, one hunk. It is the same change that
  landed here in commit `ce48f9630`, generated against
  `pre-master-47:src/frontend/dotcards.c` so it applies to an unmodified
  upstream tree with `git am` or `patch -p1`. It deliberately excludes
  `tests/regression/exitstatus/` and `tests/bin/check_status.sh`, which are
  downstream additions upstream does not have.
- **`REPORT.md`** — what a maintainer reads before the patch: the six-byte
  reproducer, the measurement that it reproduces on released `ngspice-46`, the
  failure, why `.op` is the only analysis affected, the before/after numbers,
  and why the wider fix at `beginPlot()` was measured and rejected.

**This material is prepared and has not been sent.** Nothing here has been
submitted to the ngspice project, and no bug report, mailing-list message or
merge request has been opened for it. Whether and when to send it is the
owner's call.

The fix itself is already applied on `ver_50`; see
`doc/codex/issues/0072-an-op-dot-card-with-no-netlist-aborts-on-an-assertion.md`
and `../receipts/04-op-no-netlist-fix.md`. The guard-site measurement the
report's section 6 draws on is `../receipts/03-op-guard-site-measurement.md`.
