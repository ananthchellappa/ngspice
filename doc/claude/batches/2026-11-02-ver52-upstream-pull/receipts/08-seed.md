# Receipt 08: group 08, seed

Fork-written, from ideas in upstream `95d7f2c2b`; that commit is not picked. All heavy commands went through `ngheavy` at `-j4` or less, no exit 137.

| on `ver_52` | change |
|---|---|
| `968e7042a` | Empty `.option seed=` is the unusable-value warning, not exit 139 |
| `69c754cdb` | `option seed=N` and `option seed=random` in `.control` seed the generator |

**Not taken**, as instructed: the `!has_seed` block that resets `rndseed`, `initw` in `com_sseed`, the pid for `seed=random`, `seedinfo` handling and the extra `option` report lines of that commit. `seed_from_option()`'s warning text and once-per-session rule are unchanged.

## RED

The five new decks, final versions, run through `check_status.sh` on the pre-change binary (`build-ver_50/stage/bin/ngspice`, run only):

```
option-seed-control:          FAILED: status files
    option-seed-control: seed=5 alone did not reach rndseed
option-seed-control-random:   FAILED: files
    #  rnddraws  v1  seed=9  src=setseed  offset=0
    #  rnddraws  v1  seed=9  src=setseed  offset=2
option-seed-control-unusable: FAILED: err     (only "Error:  seed equals what?." printed)
option-seed-control-netlist:  FAILED: status
    option-seed-control-netlist: the command did not reach rndseed
option-seed-empty:            FAILED: signal status err
    rc=139 is 128+11, SIGSEGV
```

An earlier RED on this tree's own binary, with the first drafts of the decks, failed the same way. The drafts changed afterwards: `pre_set` became `set`, the second reset's `offset=1` became any non-zero offset (measured: 2), `$reltol` is read before the reset, which clears it, and `option-seed-empty` gained the second card below.

## (b) Empty `.option seed=`

`eval_opt()` gave `gettok()`'s NULL to `eqc()`. Guarding only that was not enough, and this was measured: exit 0, but `Error:  seed equals what?.` on every parse, from `inp_dodeck()`'s `cp_setparse()`, and `.option reltol=2e-3 seed=` lost `reltol`, because that call returns NULL for the whole card. Since `.option seed=abc` loses nothing, the fix also cuts `seed=` off the card.

| `.option reltol=2e-3 seed=` | before | guard only | now |
|---|---|---|---|
| exit | 139 | 0 | 0 |
| stderr | | warning, one `equals what?` per parse | warning once |
| `$reltol` | | gone | 0.002 |

Valgrind `-q` on `option-seed-empty.cir`: exit 0 with `--error-exitcode=9`, no invalid read or uninitialised report. With `set strict_errorhandling`: exit 0.

**Left as it is:** `.option seed= reltol=2e-3`, value-less in the middle of a line. The parser's `x= 3` form makes `reltol=2e-3` the value, so the warning quotes `seed=reltol=2e-3` and `reltol` is not set. It did not crash before. The warning does name what happened.

## (a) `option seed=` in `.control`

`com_option()` reads this word's own `struct variable`, so position does not matter. Measured, `numdgt=15`, `-n`:

| command, then `reset`, `op` | `@r1[r]` | `$rndseed` | log header |
|---|---|---|---|
| `setseed 5` (45.2 value, receipt 02) | 9.312133559184508e+02 | 5 | `seed=5 src=setseed offset=0` |
| `option reltol=1e-3 seed=5` | 9.312133559184508e+02 | 5 | `seed=5 src=option offset=0` |
| `option seed=random` | 8.963272820051934e+02 | 155673 | `seed=155673 src=option offset=0` |

The deck asserts the same draw for `seed=5` alone, second word, first of two, and `seed = 5`, plus that `reltol` still applies. Upstream's failing form is the second row.

`option seed=7; let x=sunif(1)` gives 2.149714827537557e-01, the `setseed 7` value in receipt 02.

Choices made:

- **Seeds once.** `seed=random` does not draw again on `reset`: the second log record is `src=option`, non-zero offset. This differs from `.option seed=random`, which re-seeds per parse.
- **`.option seed=7` wins at the next parse.** `option seed=5` gives `$rndseed` 5, then `reset` gives 7, on empty stderr. The existing warning speaks of `setseed`, so the command neither prints nor arms it. That override is therefore silent. Worth the owner's eye.
- **A bad value warns every time**, not once per session, because the command is typed and a silent command is the defect here. `abc`, `-3`, `0`, `5.5` and bare `option seed`: `Warning: Cannot convert 'option seed=...' to seed value, skipped!`, seed unchanged. `option seed=` keeps `cp_setparse()`'s own `Error:  seed equals what?.`.
- **`seed` is still stored as a variable**, as before, so `$seed` still reads back.
- `option seed=1e3` seeds 1000; `.option seed=1e3` seeds 1, by `atoi`. Not changed.
- `option seed=` in a `*ng_script` or `.spiceinit` still does nothing: no circuit there, and it takes the `com_options` path.
- `seed_from_clock()` replaces the three lines in `eval_opt()`, same microseconds value, so both forms share it. No new static state.

## Gates

- `make` exit 0, `make install` exit 0.
- Full `make -k check`: **375 PASS, 1 FAIL, 0 SKIP**. The FAIL is `resource-keyword-case.cmd`, the baseline's named exception. The 5 over group 07 are the new decks. `identity.lint`, `selftest.lint` PASS, `identity.baseline` untouched. No `.out` regenerated, no existing reference changed.
- `measure/run.sh` on `stage/bin/ngspice`, saved in `measure/after-08/`, 92 KB. `compare.sh after-07 after-08 0 0`: **TOTAL changed: 0**, so no quantity needs attributing to these commits. Against `baseline/`: 73 changed, all carried from earlier groups, since `after-07` to `after-08` adds none.
- `mc_r` and `mc_sky130` identical to 45.2: yes. `setseed 1, 2, 3, 7`, by hand against `/usr/bin/ngspice`: 886.0341506323210, 925.1333325967927, 839.2277021496984, 906.8279585256480 in both.

## Unverified

- libngspice not run. The change adds no static state, read only.
- MSVC and MinGW not built. `gettimeofday` stays in `inp.c` beside its guards.
- `option seed=` was not exercised through `xschem`.
