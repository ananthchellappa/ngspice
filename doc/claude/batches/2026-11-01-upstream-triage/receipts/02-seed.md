# Receipt 02: upstream 95d7f2c2b seed handling, measured

Date 2026-11-01. Fork source and working tree untouched; nothing committed. No
fetch or push. All builds went through `ngheavy` at `-j4`; no exit 137.

## Verdict

The agauss Monte Carlo loop still works: samples differ per seed, repeat across
runs, and match a1148c969 bit for bit. The doubt about `rndseed` is **confirmed**
as a real bug, but in a narrower place than feared: only things that *read*
$rndseed afterwards are hit. A second regression turned up:
`.option seed=random` now repeats the same sample on every reset.

## Binaries

| label | what | how built |
|---|---|---|
| 45.2 | /usr/bin/ngspice | system |
| a1148 | a1148c969 | `git archive`, the two tests/xspice/analog build lines reverted (the directory is missing at that commit) |
| tip | 68b0d57b3 (contains 95d7f2c2b) | `git archive`, no revert needed, the directory exists there |
| fork | build-ver_50/stage/bin/ngspice | existing tree. **No build-ver_51 exists**; this tree holds drawlog.o, no file in src/ is newer than its binary, last src commit on ver_51 is 3beb0f2a1, so it is ver_51 despite the name |
| forkhunk | ver_51 export plus only the `if (!has_seed)` block of 95d7f2c2b | scratch only, for question 4 |

Builds: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/upstream-tip`, `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/upstream-a1148`, `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/fork-hunk`.
Each: `ngheavy ./autogen.sh`, `ngheavy ../configure --prefix=<dir>/stage --with-readline=no`,
`ngheavy make -j4`, `ngheavy make -j4 install`. All exit 0.

Only 95d7f2c2b touches seed code in a1148c969..68b0d57b3 (`git log` over inp.c,
com_option.c, options.c, randnumb.c, wallace.c, main.c lists it alone).

Run: `HOME=<empty> ngspice -b deck.cir` in a fresh directory with no .spiceinit
unless stated. Decks: `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/t/decks`, runner `/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/t/run.sh`. Output below filters
boilerplate only. Values printed with `set numdgt=15`, not raw bits; the draw log
uses %.17g.

## Who reads $rndseed (read from code, 68b0d57b3)

- `checkseed()`, randnumb.c:82, called from rnd/sgauss/sexp/sunif/spoisson in
  cmath2.c:137-292. Re-seeds when $rndseed differs from its static `oldseed`.
- `set_sseed(NULL)`, randnumb.c:309: a bare `setseed`.
- `com_option` report, com_option.c:55; and com_option.c:153, copying to rndseed_cf.
- main.c:947 and sharedspice.c:944, FastRand startup only.
- Not read by agauss/gauss/unif/aunif/limit: those draw from generator state. That
  is why the agauss loop survives.

`rndseed_cf` is set only at startup (=1) and by `option seed=` with a circuit
loaded. `setseed k`, `set rndseed=` and .spiceinit do not touch it.

## Scenarios 1 to 3 (each c1 and c3 run twice)

```
--- c1-loop [45.2] exit=0
  Reset re-loads circuit seed test
  @r1[r] = 8.860341506323210e+02
  rndseed=1
  Reset re-loads circuit seed test
  @r1[r] = 9.251333325967927e+02
  rndseed=2
  Reset re-loads circuit seed test
  @r1[r] = 8.392277021496984e+02
  rndseed=3
--- c1-loop [a1148] exit=0
  Note: Seed value for random number generator is set to 1
  Reset re-loads circuit seed test
  @r1[r] = 9.332898116479848e+02
  rndseed=1
  Note: Seed value for random number generator is set to 2
  Reset re-loads circuit seed test
  @r1[r] = 9.691315696061874e+02
  rndseed=2
  Note: Seed value for random number generator is set to 3
  Reset re-loads circuit seed test
  @r1[r] = 9.777444581821870e+02
  rndseed=3
--- c1-loop [tip] exit=0
  Note: Seed value for random number generator is set to 1
  Reset re-loads circuit seed test
  @r1[r] = 9.332898116479848e+02
  rndseed=1
  Note: Seed value for random number generator is set to 2
  Reset re-loads circuit seed test
  @r1[r] = 9.691315696061874e+02
  rndseed=1
  Note: Seed value for random number generator is set to 3
  Reset re-loads circuit seed test
  @r1[r] = 9.777444581821870e+02
  rndseed=1
--- c1-loop [fork] exit=0
  Reset re-loads circuit seed test
  @r1[r] = 8.860341506323210e+02
  rndseed=1
  Reset re-loads circuit seed test
  @r1[r] = 9.251333325967927e+02
  rndseed=2
  Reset re-loads circuit seed test
  @r1[r] = 8.392277021496984e+02
  rndseed=3
--- c1-loop [45.2] exit=0
  Reset re-loads circuit seed test
  @r1[r] = 8.860341506323210e+02
  rndseed=1
  Reset re-loads circuit seed test
  @r1[r] = 9.251333325967927e+02
  rndseed=2
  Reset re-loads circuit seed test
  @r1[r] = 8.392277021496984e+02
  rndseed=3
--- c1-loop [a1148] exit=0
  Note: Seed value for random number generator is set to 1
  Reset re-loads circuit seed test
  @r1[r] = 9.332898116479848e+02
  rndseed=1
  Note: Seed value for random number generator is set to 2
  Reset re-loads circuit seed test
  @r1[r] = 9.691315696061874e+02
  rndseed=2
  Note: Seed value for random number generator is set to 3
  Reset re-loads circuit seed test
  @r1[r] = 9.777444581821870e+02
  rndseed=3
--- c1-loop [tip] exit=0
  Note: Seed value for random number generator is set to 1
  Reset re-loads circuit seed test
  @r1[r] = 9.332898116479848e+02
  rndseed=1
  Note: Seed value for random number generator is set to 2
  Reset re-loads circuit seed test
  @r1[r] = 9.691315696061874e+02
  rndseed=1
  Note: Seed value for random number generator is set to 3
  Reset re-loads circuit seed test
  @r1[r] = 9.777444581821870e+02
  rndseed=1
--- c1-loop [fork] exit=0
  Reset re-loads circuit seed test
  @r1[r] = 8.860341506323210e+02
  rndseed=1
  Reset re-loads circuit seed test
  @r1[r] = 9.251333325967927e+02
  rndseed=2
  Reset re-loads circuit seed test
  @r1[r] = 8.392277021496984e+02
  rndseed=3
--- c1m-mcsource [45.2] exit=0
  @r1[r] = 8.860341506323210e+02
  rndseed=1
  @r1[r] = 9.251333325967927e+02
  rndseed=2
  @r1[r] = 8.392277021496984e+02
  rndseed=3
--- c1m-mcsource [a1148] exit=0
  Note: Seed value for random number generator is set to 1
  @r1[r] = 9.332898116479848e+02
  rndseed=1
  Note: Seed value for random number generator is set to 2
  @r1[r] = 9.691315696061874e+02
  rndseed=2
  Note: Seed value for random number generator is set to 3
  @r1[r] = 9.777444581821870e+02
  rndseed=3
--- c1m-mcsource [tip] exit=0
  Note: Seed value for random number generator is set to 1
  @r1[r] = 9.332898116479848e+02
  rndseed=1
  Note: Seed value for random number generator is set to 2
  @r1[r] = 9.691315696061874e+02
  rndseed=1
  Note: Seed value for random number generator is set to 3
  @r1[r] = 9.777444581821870e+02
  rndseed=1
--- c1m-mcsource [fork] exit=0
  @r1[r] = 8.860341506323210e+02
  rndseed=1
  @r1[r] = 9.251333325967927e+02
  rndseed=2
  @r1[r] = 8.392277021496984e+02
  rndseed=3
--- c2-optseed5 [45.2] exit=0
  Reset re-loads circuit seed test
  @r1[r] = 9.312133559184508e+02
  rndseed=5
  Reset re-loads circuit seed test
  @r1[r] = 9.312133559184508e+02
  rndseed=5
  Reset re-loads circuit seed test
  @r1[r] = 9.312133559184508e+02
  rndseed=5
--- c2-optseed5 [a1148] exit=0
  Note: Seed value for random number generator is set to 1
  Reset re-loads circuit seed test
  @r1[r] = 7.974070496223965e+02
  rndseed=5
  Note: Seed value for random number generator is set to 2
  Reset re-loads circuit seed test
  @r1[r] = 7.974070496223965e+02
  rndseed=5
  Note: Seed value for random number generator is set to 3
  Reset re-loads circuit seed test
  @r1[r] = 7.974070496223965e+02
  rndseed=5
--- c2-optseed5 [tip] exit=0
  Warning: Dot command .option seed = val/random is in the netlist:
      Command 'reset' will override all 'option seed=val' or 'setseed = val' settings!
  Note: Seed value for random number generator is set to 5
  Note: Seed value for random number generator is set to 1
  Reset re-loads circuit seed test
  Note: Seed value for random number generator is set to 5
  @r1[r] = 7.974070496223965e+02
  rndseed=5
  Note: Seed value for random number generator is set to 2
  Reset re-loads circuit seed test
  Note: Seed value for random number generator is set to 5
  @r1[r] = 7.974070496223965e+02
  rndseed=5
  Note: Seed value for random number generator is set to 3
  Reset re-loads circuit seed test
  Note: Seed value for random number generator is set to 5
  @r1[r] = 7.974070496223965e+02
  rndseed=5
--- c2-optseed5 [fork] exit=0
  Warning: '.option seed=5' re-seeds the random number generator each time the
      circuit is parsed, by 'source' and by every 'reset', so it overrides 'setseed'.
      Remove the option to let 'setseed' choose the seed.
      (This warning is printed once per session.)
  Reset re-loads circuit seed test
  @r1[r] = 9.312133559184508e+02
  rndseed=5
  Reset re-loads circuit seed test
  @r1[r] = 9.312133559184508e+02
  rndseed=5
  Reset re-loads circuit seed test
  @r1[r] = 9.312133559184508e+02
  rndseed=5
--- c3-nosetseed [45.2] exit=0
  @r1[r] = 1.078669821215895e+03
  rndseed=1
  Reset re-loads circuit seed test
  @r1[r] = 1.099507695351136e+03
  Reset re-loads circuit seed test
  @r1[r] = 9.104817240645616e+02
--- c3-nosetseed [a1148] exit=0
  @r1[r] = 9.332898116479848e+02
  rndseed=1
  Reset re-loads circuit seed test
  @r1[r] = 8.001941526624464e+02
  Reset re-loads circuit seed test
  @r1[r] = 1.148504877381393e+03
--- c3-nosetseed [tip] exit=0
  @r1[r] = 9.332898116479848e+02
  rndseed=1
  Reset re-loads circuit seed test
  @r1[r] = 8.001941526624464e+02
  Reset re-loads circuit seed test
  @r1[r] = 1.148504877381393e+03
--- c3-nosetseed [fork] exit=0
  @r1[r] = 8.860341506323210e+02
  rndseed=1
  Reset re-loads circuit seed test
  @r1[r] = 1.090407562066412e+03
  Reset re-loads circuit seed test
  @r1[r] = 1.110402241983024e+03
--- c3-nosetseed [45.2] exit=0
  @r1[r] = 1.027640610452845e+03
  rndseed=1
  Reset re-loads circuit seed test
  @r1[r] = 1.038843353230453e+03
  Reset re-loads circuit seed test
  @r1[r] = 8.926669707264430e+02
--- c3-nosetseed [a1148] exit=0
  @r1[r] = 9.332898116479848e+02
  rndseed=1
  Reset re-loads circuit seed test
  @r1[r] = 8.001941526624464e+02
  Reset re-loads circuit seed test
  @r1[r] = 1.148504877381393e+03
--- c3-nosetseed [tip] exit=0
  @r1[r] = 9.332898116479848e+02
  rndseed=1
  Reset re-loads circuit seed test
  @r1[r] = 8.001941526624464e+02
  Reset re-loads circuit seed test
  @r1[r] = 1.148504877381393e+03
--- c3-nosetseed [fork] exit=0
  @r1[r] = 8.860341506323210e+02
  rndseed=1
  Reset re-loads circuit seed test
  @r1[r] = 1.090407562066412e+03
  Reset re-loads circuit seed test
  @r1[r] = 1.110402241983024e+03
```

Reading these:
- 1: tip samples differ per seed and repeat across runs. tip == a1148 bit for bit
  at 15 digits (933.29, 969.13, 977.74). Neither matches 45.2 or the fork (886.03,
  925.13, 839.23); fork == 45.2. So upstream's break with 45.2 predates 95d7f2c2b.
  Read from code: upstream's com_sseed calls initw() after seeding (cbb09dc25,
  ancestor of a1148c969), which consumes the stream. Supporting measurement: r1b
  below, where checkseed re-seeds without initw, gives tip == 45.2.
  $rndseed reads 1 on tip after every reset. Confirmed.
- 2: tip prints the two-line warning once, a Note per seeding, and the value
  797.407 == a1148, != 45.2/fork 931.213.
- 3: tip and a1148 repeat across processes, fork repeats, 45.2 does not (1078.67
  then 1027.64). tip == a1148.

## Prove or disprove: effects of rndseed going back to 1

```
--- r1-sunif-reset-7 [45.2] exit=1
  x = 2.149714827537557e-01
  rndseed=7
--- r1-sunif-reset-7 [a1148] exit=1
  Note: Seed value for random number generator is set to 7
  x = 2.149714827537557e-01
  rndseed=7
--- r1-sunif-reset-7 [tip] exit=1
  Note: Seed value for random number generator is set to 7
  x = 3.493843660689910e-01
  rndseed=1
--- r1-sunif-reset-7 [fork] exit=1
  x = 2.149714827537557e-01
  rndseed=7
--- r1-sunif-reset-8 [45.2] exit=1
  x = -6.05475077405571e-01
  rndseed=8
--- r1-sunif-reset-8 [a1148] exit=1
  Note: Seed value for random number generator is set to 8
  x = -6.05475077405571e-01
  rndseed=8
--- r1-sunif-reset-8 [tip] exit=1
  Note: Seed value for random number generator is set to 8
  x = 3.493843660689910e-01
  rndseed=1
--- r1-sunif-reset-8 [fork] exit=1
  x = -6.05475077405571e-01
  rndseed=8
--- r1b-sunif-noreset-7 [45.2] exit=1
  x = 2.149714827537557e-01
  rndseed=7
--- r1b-sunif-noreset-7 [a1148] exit=1
  Note: Seed value for random number generator is set to 7
  x = 2.149714827537557e-01
  rndseed=7
--- r1b-sunif-noreset-7 [tip] exit=1
  Note: Seed value for random number generator is set to 7
  x = 2.149714827537557e-01
  rndseed=7
--- r1b-sunif-noreset-7 [fork] exit=1
  x = 2.149714827537557e-01
  rndseed=7
--- r1b-sunif-noreset-8 [45.2] exit=1
  x = -6.05475077405571e-01
  rndseed=8
--- r1b-sunif-noreset-8 [a1148] exit=1
  Note: Seed value for random number generator is set to 8
  x = -6.05475077405571e-01
  rndseed=8
--- r1b-sunif-noreset-8 [tip] exit=1
  Note: Seed value for random number generator is set to 8
  x = -6.05475077405571e-01
  rndseed=8
--- r1b-sunif-noreset-8 [fork] exit=1
  x = -6.05475077405571e-01
  rndseed=8
--- r2-bare-setseed [45.2] exit=0
  rndseed=3
  @r1[r] = 8.392277021496984e+02
  rndseed=3
--- r2-bare-setseed [a1148] exit=0
  Note: Seed value for random number generator is set to 3
  rndseed=3
  Note: Seed value for random number generator is set to 3
  @r1[r] = 9.777444581821870e+02
  rndseed=3
--- r2-bare-setseed [tip] exit=0
  Note: Seed value for random number generator is set to 3
  rndseed=1
  Note: Seed value for random number generator is set to 1
  @r1[r] = 9.332898116479848e+02
  rndseed=1
--- r2-bare-setseed [fork] exit=0
  rndseed=3
  @r1[r] = 8.392277021496984e+02
  rndseed=3
--- r3-sunif-loop [45.2] exit=0
  @r1[r] = 8.860341506323210e+02
  x = 3.493843660689910e-01
  rndseed=1
  @r1[r] = 9.251333325967927e+02
  Seed value for random number generator is set to 2
  x = 5.056166448630417e-01
  rndseed=2
  @r1[r] = 8.392277021496984e+02
  Seed value for random number generator is set to 3
  x = 5.556579167023457e-02
  rndseed=3
--- r3-sunif-loop [a1148] exit=0
  Note: Seed value for random number generator is set to 1
  @r1[r] = 9.332898116479848e+02
  x = 3.493843660689910e-01
  rndseed=1
  Note: Seed value for random number generator is set to 2
  @r1[r] = 9.691315696061874e+02
  Seed value for random number generator is set to 2
  x = 5.056166448630417e-01
  rndseed=2
  Note: Seed value for random number generator is set to 3
  @r1[r] = 9.777444581821870e+02
  Seed value for random number generator is set to 3
  x = 5.556579167023457e-02
  rndseed=3
--- r3-sunif-loop [tip] exit=0
  Note: Seed value for random number generator is set to 1
  @r1[r] = 9.332898116479848e+02
  x = 3.493843660689910e-01
  rndseed=1
  Note: Seed value for random number generator is set to 2
  @r1[r] = 9.691315696061874e+02
  x = 9.666303829290004e-01
  rndseed=1
  Note: Seed value for random number generator is set to 3
  @r1[r] = 9.777444581821870e+02
  x = 3.003233531490124e-02
  rndseed=1
--- r3-sunif-loop [fork] exit=0
  @r1[r] = 8.860341506323210e+02
  x = 3.493843660689910e-01
  rndseed=1
  @r1[r] = 9.251333325967927e+02
  Seed value for random number generator is set to 2
  x = 5.056166448630417e-01
  rndseed=2
  @r1[r] = 8.392277021496984e+02
  Seed value for random number generator is set to 3
  x = 5.556579167023457e-02
  rndseed=3
--- r5-set-rndseed [45.2] exit=0
  rndseed=12
  @r1[r] = 8.818595908459030e+02
  rndseed=12
--- r5-set-rndseed [a1148] exit=0
  rndseed=12
  Note: Seed value for random number generator is set to 12
  @r1[r] = 1.069401363081993e+03
  rndseed=12
--- r5-set-rndseed [tip] exit=0
  rndseed=1
  Note: Seed value for random number generator is set to 1
  @r1[r] = 9.332898116479848e+02
  rndseed=1
--- r5-set-rndseed [fork] exit=0
  rndseed=12
  @r1[r] = 8.818595908459030e+02
  rndseed=12
--- r6-option-then-setseed [45.2] exit=0
  rndseed=9
  @r1[r] = 1.005625111229858e+03
--- r6-option-then-setseed [a1148] exit=0
  Note: Seed value for random number generator is set to 9
  rndseed=9
  @r1[r] = 9.185773967236976e+02
--- r6-option-then-setseed [tip] exit=0
  Note: Seed value for random number generator is set to 7
  Note: Seed value for random number generator is set to 9
  rndseed=7
  @r1[r] = 9.185773967236976e+02
--- r6-option-then-setseed [fork] exit=0
  rndseed=9
  @r1[r] = 1.005625111229858e+03
--- r7-spiceinit [45.2] exit=0
  rndseed=12
  x = 3.431054120883366e-01
  rndseed=12
  @r1[r] = 9.464521938083052e+02
--- r7-spiceinit [a1148] exit=0
  rndseed=12
  x = 3.431054120883366e-01
  rndseed=12
  @r1[r] = 9.332898116479848e+02
--- r7-spiceinit [tip] exit=0
  rndseed=1
  x = 3.493843660689910e-01
  rndseed=1
  @r1[r] = 9.332898116479848e+02
--- r7-spiceinit [fork] exit=0
  rndseed=12
  x = 3.431054120883366e-01
  rndseed=12
  @r1[r] = 8.818595908459030e+02
--- r4-option-print [a1148] exit=0
  rndseed=3
--- r4-option-print [tip] exit=0
  rndseed=1
  seed = 1
--- r8-replay-2 [45.2] exit=0
  @r1[r] = 9.251333325967927e+02
  x = 5.056166448630417e-01
  rndseed=2
--- r8-replay-2 [a1148] exit=0
  Note: Seed value for random number generator is set to 2
  @r1[r] = 9.691315696061874e+02
  x = 5.056166448630417e-01
  rndseed=2
--- r8-replay-2 [tip] exit=0
  Note: Seed value for random number generator is set to 2
  @r1[r] = 9.691315696061874e+02
  x = 3.493843660689910e-01
  rndseed=1
--- r8-replay-2 [fork] exit=0
  @r1[r] = 9.251333325967927e+02
  x = 5.056166448630417e-01
  rndseed=2
--- r8-replay-3 [45.2] exit=0
  @r1[r] = 8.392277021496984e+02
  x = 5.556579167023457e-02
  rndseed=3
--- r8-replay-3 [a1148] exit=0
  Note: Seed value for random number generator is set to 3
  @r1[r] = 9.777444581821870e+02
  x = 5.556579167023457e-02
  rndseed=3
--- r8-replay-3 [tip] exit=0
  Note: Seed value for random number generator is set to 3
  @r1[r] = 9.777444581821870e+02
  x = 3.493843660689910e-01
  rndseed=1
--- r8-replay-3 [fork] exit=0
  @r1[r] = 8.392277021496984e+02
  x = 5.556579167023457e-02
  rndseed=3
--- r9-spinit-setseed [45.2] exit=0
  rndseed=12
  x = 3.431054120883366e-01
  rndseed=12
  @r1[r] = 1.097839520417140e+03
--- r9-spinit-setseed [a1148] exit=0
  Note: Seed value for random number generator is set to 12
  rndseed=12
  x = 3.431054120883366e-01
  rndseed=12
  @r1[r] = 1.069401363081993e+03
--- r9-spinit-setseed [tip] exit=0
  Note: Seed value for random number generator is set to 12
  rndseed=1
  x = 3.493843660689910e-01
  rndseed=1
  @r1[r] = 1.069401363081993e+03
--- r9-spinit-setseed [fork] exit=0
  rndseed=12
  x = 3.431054120883366e-01
  rndseed=12
  @r1[r] = 8.818595908459030e+02
--- r10-spinit-option [45.2] exit=0
  rndseed=1
  x = 3.493843660689910e-01
  rndseed=1
  @r1[r] = 1.202691110677467e+03
--- r10-spinit-option [a1148] exit=0
  rndseed=1
  x = 3.493843660689910e-01
  rndseed=1
  @r1[r] = 9.332898116479848e+02
--- r10-spinit-option [tip] exit=0
  rndseed=1
  x = 3.493843660689910e-01
  rndseed=1
  @r1[r] = 9.332898116479848e+02
--- r10-spinit-option [fork] exit=0
  rndseed=1
  x = 3.493843660689910e-01
  rndseed=1
  @r1[r] = 8.860341506323210e+02
--- r11-rndseed-spiceinit [45.2] exit=1
  rndseed=12
  x = 3.431054120883366e-01
--- r11-rndseed-spiceinit [a1148] exit=1
  rndseed=12
  x = 3.431054120883366e-01
--- r11-rndseed-spiceinit [tip] exit=1
  rndseed=1
  x = 3.493843660689910e-01
--- r11-rndseed-spiceinit [fork] exit=1
  rndseed=12
  x = 3.431054120883366e-01
--- c6-random-loop [45.2] exit=0
  @r1[r] = 1.045750097879935e+03
  @r1[r] = 1.055309905554966e+03
  @r1[r] = 1.096542955320899e+03
--- c6-random-loop [a1148] exit=0
  @r1[r] = 8.894573974502116e+02
  @r1[r] = 1.142381009498010e+03
  @r1[r] = 1.055841263749492e+03
--- c6-random-loop [tip] exit=0
  Warning: Dot command .option seed = val/random is in the netlist:
      Command 'reset' will override all 'option seed=val' or 'setseed = val' settings!
  Note: Seed value for random number generator is set to 2960543
  Note: Seed value for random number generator is set to 2960543
  @r1[r] = 9.912895911689916e+02
  Note: Seed value for random number generator is set to 2960543
  @r1[r] = 9.912895911689916e+02
  Note: Seed value for random number generator is set to 2960543
  @r1[r] = 9.912895911689916e+02
--- c6-random-loop [fork] exit=0
  @r1[r] = 1.202421658305815e+03
  @r1[r] = 1.123729028480816e+03
  @r1[r] = 1.019168781179183e+03
--- c7-draws-sunif [fork] exit=0
  #	rnddraws	v1	seed=7	src=setseed	offset=0
  7	r1	?	agauss	1000,100,1	-0.93172041474352052	906.827958525648
  #	end	logged=1	unlogged_calls=0
  #	rnddraws	v1	seed=7	src=checkseed	offset=1
  7	r1	?	agauss	1000,100,1	0.11306734566646565	1011.3067345666466
  #	end	logged=1	unlogged_calls=0
  #	rnddraws	v1	seed=7	src=setseed	offset=0
  7	r1	?	agauss	1000,100,1	-0.93172041474352052	906.827958525648
  #	end	logged=1	unlogged_calls=0
  rndseed=7
  @r1[r] = 9.068279585256480e+02
  x = 2.149714827537557e-01
```

Conclusions:
- **Disproved for agauss etc.**: r8, c1: parse-time draws are unaffected.
- **Proved for control-language draws**: r1 and m1: `setseed 7; reset; let
  x=sunif(1)` and the same with 8 give the identical x = 0.349384 on tip. Without
  the reset (r1b) they differ and match 45.2. Replaying one sample is also broken:
  in a loop tip gives x = 0.966630 at k=2 (r3), alone it gives 0.349384 (r8).
- **Proved for bare `setseed`**: r2: `setseed 3; reset; setseed` seeds with 1 on tip.
- **Proved for `set rndseed=`**: r5: `set rndseed=12; reset` gives rndseed=1.
- **Contradicts the commit message** ("Resetting seed to .spiceinit values or 1"):
  r9, r10, r11: `setseed 12`, `option seed=12`, `set rndseed=12` in .spiceinit all
  end with rndseed=1 on tip, though `setseed 12` still seeds the first agauss. r10:
  `option seed=` in .spiceinit does nothing in any build.
- **Wrong report**: r4: `option` prints `seed = 1` after `setseed 3; reset`. r6:
  `option seed=7; setseed 9; reset` shows rndseed=7 while the draw (918.577 ==
  a1148) comes from seed 9.
- **Not reached**: libngspice was not run; the same eval_opt is shared (read).

## Scenario 5: option in .control, abc, random, others

```
--- c5a-ctl-option [45.2] exit=0
  rndseed=1
  @r1[r] = 1.235076549826662e+03
  rndseed=1
  @r1[r] = 8.250254167992554e+02
  rndseed=1
--- c5a-ctl-option [a1148] exit=0
  rndseed=5
  @r1[r] = 8.174011127491641e+02
  rndseed=5
  @r1[r] = 9.428221245146516e+02
  rndseed=5
--- c5a-ctl-option [tip] exit=0
  Note: Seed value for random number generator is set to 5
  rndseed=5
  @r1[r] = 7.974070496223965e+02
  rndseed=5
  @r1[r] = 8.174011127491641e+02
  rndseed=5
--- c5a-ctl-option [fork] exit=0
  rndseed=1
  @r1[r] = 1.090407562066412e+03
  rndseed=1
  @r1[r] = 1.110402241983024e+03
  rndseed=1
--- c5a2-ctl-option-spaces [45.2] exit=0
  rndseed=1
  @r1[r] = 1.027096773985277e+03
--- c5a2-ctl-option-spaces [a1148] exit=0
  rndseed=5
  @r1[r] = 8.174011127491641e+02
--- c5a2-ctl-option-spaces [tip] exit=0
  Note: Seed value for random number generator is set to 5
  rndseed=5
  @r1[r] = 7.974070496223965e+02
--- c5a2-ctl-option-spaces [fork] exit=0
  rndseed=1
  @r1[r] = 1.090407562066412e+03
--- c5a3-ctl-option-second [45.2] exit=0
  rndseed=1
  @r1[r] = 1.141357753133843e+03
--- c5a3-ctl-option-second [a1148] exit=0
  rndseed=5
  @r1[r] = 8.174011127491641e+02
--- c5a3-ctl-option-second [tip] exit=0
  Warning: Cannot use l=1e-3 as seed!
      Command 'setseed l=1e-3' ignored.
  rndseed=1
  @r1[r] = 9.794005460299657e+02
--- c5a3-ctl-option-second [fork] exit=0
  rndseed=1
  @r1[r] = 1.090407562066412e+03
--- c5a4-ctl-option-first [45.2] exit=0
  rndseed=1
  @r1[r] = 1.036168395902017e+03
--- c5a4-ctl-option-first [a1148] exit=0
  rndseed=5
  @r1[r] = 8.174011127491641e+02
--- c5a4-ctl-option-first [tip] exit=0
  Note: Seed value for random number generator is set to 5
  rndseed=5
  @r1[r] = 7.974070496223965e+02
--- c5a4-ctl-option-first [fork] exit=0
  rndseed=1
  @r1[r] = 1.090407562066412e+03
--- c5a5-ctl-option-abc [45.2] exit=0
  rndseed=1
  @r1[r] = 9.068555457054330e+02
--- c5a5-ctl-option-abc [a1148] exit=0
  Warning: Cannot convert 'option seed=abc' to seed value, skipped!
  rndseed=1
  @r1[r] = 9.794005460299657e+02
--- c5a5-ctl-option-abc [tip] exit=0
  Warning: Cannot use abc as seed!
      Command 'setseed abc' ignored.
  rndseed=1
  @r1[r] = 9.794005460299657e+02
--- c5a5-ctl-option-abc [fork] exit=0
  rndseed=1
  @r1[r] = 1.090407562066412e+03
--- c5a6-ctl-option-random [45.2] exit=0
  rndseed=1
  @r1[r] = 7.834000425179381e+02
  rndseed=1
--- c5a6-ctl-option-random [a1148] exit=0
  rndseed=170985
  @r1[r] = 9.916260357642020e+02
  rndseed=170985
--- c5a6-ctl-option-random [tip] exit=0
  Note: Seed value for random number generator is set to 2959283
  rndseed=2959283
  @r1[r] = 1.113002270731935e+03
  rndseed=2959283
--- c5a6-ctl-option-random [fork] exit=0
  rndseed=1
  @r1[r] = 1.090407562066412e+03
  rndseed=1
--- c5b-abc [45.2] exit=0
  Warning: Cannot convert 'option seed=abc' to seed value, skipped!
  Warning: Cannot convert 'option seed=abc' to seed value, skipped!
  @r1[r] = 1.039049130713020e+03
  rndseed=1
  @r1[r] = 9.023818783213133e+02
  rndseed=1
--- c5b-abc [a1148] exit=0
  Warning: Cannot convert 'option seed=abc' to seed value, skipped!
  Warning: Cannot convert 'option seed=abc' to seed value, skipped!
  @r1[r] = 9.103375054904176e+02
  rndseed=1
  @r1[r] = 8.931193604808898e+02
  rndseed=1
--- c5b-abc [tip] exit=0
  Warning: Cannot use abc as seed!
      Command 'setseed abc' ignored.
  Warning: Cannot use abc as seed!
      Command 'setseed abc' ignored.
  Warning: Dot command .option seed = val/random is in the netlist:
      Command 'reset' will override all 'option seed=val' or 'setseed = val' settings!
  @r1[r] = 9.103375054904176e+02
  rndseed=1
  @r1[r] = 8.931193604808898e+02
  rndseed=1
--- c5b-abc [fork] exit=0
  Warning: Cannot convert 'option seed=abc' to seed value, skipped!
      (This warning is printed once per session.)
  @r1[r] = 8.860341506323210e+02
  rndseed=1
  @r1[r] = 1.090407562066412e+03
  rndseed=1
--- c5c-random [45.2] exit=0
  @r1[r] = 8.359528471377887e+02
  rndseed=932401
  @r1[r] = 1.157603230499153e+03
  rndseed=932557
--- c5c-random [a1148] exit=0
  @r1[r] = 1.089711719193701e+03
  rndseed=42637
  @r1[r] = 9.308135801385893e+02
  rndseed=42920
--- c5c-random [tip] exit=0
  Warning: Dot command .option seed = val/random is in the netlist:
      Command 'reset' will override all 'option seed=val' or 'setseed = val' settings!
  Note: Seed value for random number generator is set to 2959333
  @r1[r] = 1.014123505350682e+03
  rndseed=2959333
  Note: Seed value for random number generator is set to 2959333
  @r1[r] = 1.014123505350682e+03
  rndseed=2959333
--- c5c-random [fork] exit=0
  @r1[r] = 1.132841288729760e+03
  rndseed=258203
  @r1[r] = 9.357735107568120e+02
  rndseed=258371
--- c5c-random [45.2] exit=0
  @r1[r] = 9.951102441791917e+02
  rndseed=366268
  @r1[r] = 9.294994239985108e+02
  rndseed=366447
--- c5c-random [a1148] exit=0
  @r1[r] = 9.957063723577618e+02
  rndseed=474040
  @r1[r] = 9.455628872315413e+02
  rndseed=474284
--- c5c-random [tip] exit=0
  Warning: Dot command .option seed = val/random is in the netlist:
      Command 'reset' will override all 'option seed=val' or 'setseed = val' settings!
  Note: Seed value for random number generator is set to 2959365
  @r1[r] = 8.925771765059520e+02
  rndseed=2959365
  Note: Seed value for random number generator is set to 2959365
  @r1[r] = 8.925771765059520e+02
  rndseed=2959365
--- c5c-random [fork] exit=0
  @r1[r] = 8.505349453144836e+02
  rndseed=691281
  @r1[r] = 9.996812524977477e+02
  rndseed=691464
./run.sh: line 7: 2959377 Segmentation fault         HOME=$S/t/home timeout 60 ${B[$b]} -b $D/$c.cir > out 2>&1
--- c5d-empty [45.2] exit=139
./run.sh: line 7: 2959383 Segmentation fault         HOME=$S/t/home timeout 60 ${B[$b]} -b $D/$c.cir > out 2>&1
--- c5d-empty [a1148] exit=139
--- c5d-empty [tip] exit=0
  Error:  seed equals what?.
  Warning: Dot command .option seed = val/random is in the netlist:
      Command 'reset' will override all 'option seed=val' or 'setseed = val' settings!
  Error:  seed equals what?.
  Note: Seed value for random number generator is set to 1
  @r1[r] = 9.332898116479848e+02
  rndseed=1
  Note: Seed value for random number generator is set to 1
  @r1[r] = 9.332898116479848e+02
  rndseed=1
./run.sh: line 7: 2959395 Segmentation fault         HOME=$S/t/home timeout 60 ${B[$b]} -b $D/$c.cir > out 2>&1
--- c5d-empty [fork] exit=139
--- c5e-brace [45.2] exit=1
  Netlist line no. 5:
  Undefined parameter [random]
  Netlist line no. 5:
  Cannot compute substitute
  ERROR: fatal error in ngspice, exit(1)
--- c5e-brace [a1148] exit=1
  Error in netlist line no. 6, new internal line no. 5:
  Undefined parameter [random]
  Error in netlist line no. 6, new internal line no. 5:
  Cannot compute substitute
  ERROR: fatal error in ngspice, exit(1)
--- c5e-brace [tip] exit=1
  Error in netlist line no. 6, new internal line no. 5:
  Undefined parameter [random]
  Error in netlist line no. 6, new internal line no. 5:
  Cannot compute substitute
  ERROR: fatal error in ngspice, exit(1)
--- c5e-brace [fork] exit=1
  Error in netlist line no. 8, new internal line no. 5:
  Undefined parameter [random]
  Error in netlist line no. 8, new internal line no. 5:
  Cannot compute substitute
  ERROR: fatal error in ngspice, exit(1)
--- c5f-neg [45.2] exit=0
  Warning: Cannot convert 'option seed=-3' to seed value, skipped!
  Warning: Cannot convert 'option seed=-3' to seed value, skipped!
  @r1[r] = 9.797407056962991e+02
  rndseed=1
  @r1[r] = 1.123968327950260e+03
  rndseed=1
--- c5f-neg [a1148] exit=0
  Warning: Cannot convert 'option seed=-3' to seed value, skipped!
  Warning: Cannot convert 'option seed=-3' to seed value, skipped!
  @r1[r] = 9.103375054904176e+02
  rndseed=1
  @r1[r] = 8.931193604808898e+02
  rndseed=1
--- c5f-neg [tip] exit=0
  Warning: Cannot use -3 as seed!
      Command 'setseed -3' ignored.
  Warning: Cannot use -3 as seed!
      Command 'setseed -3' ignored.
  Warning: Dot command .option seed = val/random is in the netlist:
      Command 'reset' will override all 'option seed=val' or 'setseed = val' settings!
  @r1[r] = 9.103375054904176e+02
  rndseed=1
  @r1[r] = 8.931193604808898e+02
  rndseed=1
--- c5f-neg [fork] exit=0
  Warning: Cannot convert 'option seed=-3' to seed value, skipped!
      (This warning is printed once per session.)
  @r1[r] = 8.860341506323210e+02
  rndseed=1
  @r1[r] = 1.090407562066412e+03
  rndseed=1
--- c5g-spaces [45.2] exit=0
  @r1[r] = 9.847472405312608e+02
  rndseed=55
  @r1[r] = 9.847472405312608e+02
  rndseed=55
--- c5g-spaces [a1148] exit=0
  @r1[r] = 8.748515810101544e+02
  rndseed=55
  @r1[r] = 8.748515810101544e+02
  rndseed=55
--- c5g-spaces [tip] exit=0
  Warning: Dot command .option seed = val/random is in the netlist:
      Command 'reset' will override all 'option seed=val' or 'setseed = val' settings!
  Note: Seed value for random number generator is set to 55
  @r1[r] = 8.748515810101544e+02
  rndseed=55
  Note: Seed value for random number generator is set to 55
  @r1[r] = 8.748515810101544e+02
  rndseed=55
--- c5g-spaces [fork] exit=0
  @r1[r] = 9.847472405312608e+02
  rndseed=55
  @r1[r] = 9.847472405312608e+02
  rndseed=55
--- c5h-seedinfo [45.2] exit=0
  @r1[r] = 9.479275736968440e+02
  rndseed=1
  @r1[r] = 1.071974186426176e+03
  rndseed=1
--- c5h-seedinfo [a1148] exit=0
  @r1[r] = 9.332898116479848e+02
  rndseed=1
  @r1[r] = 8.001941526624464e+02
  rndseed=1
--- c5h-seedinfo [tip] exit=0
  @r1[r] = 9.332898116479848e+02
  rndseed=1
  @r1[r] = 8.001941526624464e+02
  rndseed=1
--- c5h-seedinfo [fork] exit=0
  @r1[r] = 8.860341506323210e+02
  rndseed=1
  @r1[r] = 1.090407562066412e+03
  rndseed=1
--- c5i-card [45.2] exit=1
  Error on line 5 or its substitute:
    option seed=5
  Unable to find definition of model 
      Simulation interrupted due to error!
  Error: incomplete or empty netlist
         or no ".plot", ".print", or ".fourier" lines in batch mode;
  no simulations run!
--- c5i-card [a1148] exit=0
  @r1[r] = 7.974070496223965e+02
  rndseed=5
  @r1[r] = 7.974070496223965e+02
  rndseed=5
--- c5i-card [tip] exit=1
  Error on line 5 or its substitute:
    option seed=5
  Unable to find definition of model 
      Simulation interrupted due to error!
  Error: incomplete or empty netlist
         or no ".plot", ".print", or ".fourier" lines in batch mode;
  no simulations run!
--- c5i-card [fork] exit=1
  Error on line 5 or its substitute:
    option seed=5
  Unable to find definition of model 
      Simulation interrupted due to error!
  Error: incomplete or empty netlist
         or no ".plot", ".print", or ".fourier" lines in batch mode;
  no simulations run!
```

Valgrind (`-q`) on tip for c5a, c5a2, c5a3, c5b, c5d: no invalid read or
uninitialised-value report. A check filtered by grep, so treat as absence of a
finding, not a clean-bill proof.

- `option seed=5` in .control: tip works, sets $rndseed and seeds at the command.
  **fork and 45.2 ignore it silently** (rndseed=1). a1148 applied it at parse.
- `option reltol=1e-3 seed=5` on tip: `Warning: Cannot use l=1e-3 as seed!`, seed
  dropped. com_option.c uses `wl->wl_word + 5` where wl is the whole list's first
  word, not this variable's. `option seed=5 reltol=1e-3` works.
- `.option seed=abc`, `seed=-3`: no crash anywhere. tip warns per parse, then also
  prints the reset warning about a seed that was ignored.
- `.option seed=random`: **tip repeats** (c5c, c6, m2): all resets give the same
  sample, seed is the pid. 45.2, a1148, fork draw a new clock seed per parse.
- `.option seed=` (empty): **45.2, a1148, fork segfault, exit 139**. tip: `Error:
  seed equals what?`, no crash. tip is better; this is a fork bug.
- `.option seed={random}`: fatal "Undefined parameter" in all four, no change.
- `option seed=5` bare netlist card: tip, 45.2, fork error; only a1148 took it.

## Scenario 4: draw log and merging

Fork output, plus forkhunk:

```
--- c4-draws [fork] exit=0
  #	rnddraws	v1	seed=1	src=setseed	offset=0
  1	r1	?	agauss	1000,100,1	-1.13965849367679	886.03415063232103
  #	end	logged=1	unlogged_calls=0
  #	rnddraws	v1	seed=2	src=setseed	offset=0
  2	r1	?	agauss	1000,100,1	-0.74866667403207465	925.13333259679257
  #	end	logged=1	unlogged_calls=0
  #	rnddraws	v1	seed=3	src=setseed	offset=0
  3	r1	?	agauss	1000,100,1	-1.6077229785030163	839.22770214969842
  #	end	logged=1	unlogged_calls=0
  @r1[r] = 8.860341506323210e+02
  @r1[r] = 9.251333325967927e+02
  @r1[r] = 8.392277021496984e+02
--- c4-draws [forkhunk] exit=0
  #	rnddraws	v1	seed=1	src=setseed	offset=0
  1	r1	?	agauss	1000,100,1	-1.13965849367679	886.03415063232103
  #	end	logged=1	unlogged_calls=0
  #	rnddraws	v1	seed=2	src=setseed	offset=0
  2	r1	?	agauss	1000,100,1	-0.74866667403207465	925.13333259679257
  #	end	logged=1	unlogged_calls=0
  #	rnddraws	v1	seed=3	src=setseed	offset=0
  3	r1	?	agauss	1000,100,1	-1.6077229785030163	839.22770214969842
  #	end	logged=1	unlogged_calls=0
  @r1[r] = 8.860341506323210e+02
  @r1[r] = 9.251333325967927e+02
  @r1[r] = 8.392277021496984e+02
--- c7-draws-sunif [fork] exit=0
  #	rnddraws	v1	seed=7	src=setseed	offset=0
  7	r1	?	agauss	1000,100,1	-0.93172041474352052	906.827958525648
  #	end	logged=1	unlogged_calls=0
  #	rnddraws	v1	seed=7	src=checkseed	offset=1
  7	r1	?	agauss	1000,100,1	0.11306734566646565	1011.3067345666466
  #	end	logged=1	unlogged_calls=0
  #	rnddraws	v1	seed=7	src=setseed	offset=0
  7	r1	?	agauss	1000,100,1	-0.93172041474352052	906.827958525648
  #	end	logged=1	unlogged_calls=0
  rndseed=7
  @r1[r] = 9.068279585256480e+02
  x = 2.149714827537557e-01
--- c7-draws-sunif [forkhunk] exit=0
  #	rnddraws	v1	seed=7	src=setseed	offset=0
  7	r1	?	agauss	1000,100,1	-0.93172041474352052	906.827958525648
  #	end	logged=1	unlogged_calls=0
  #	rnddraws	v1	seed=1	src=checkseed	offset=1
  1	r1	?	agauss	1000,100,1	-0.71651925482124901	928.34807451787515
  #	end	logged=1	unlogged_calls=0
  #	rnddraws	v1	seed=1	src=setseed	offset=0
  1	r1	?	agauss	1000,100,1	-1.13965849367679	886.03415063232103
  #	end	logged=1	unlogged_calls=0
  rndseed=1
  @r1[r] = 9.068279585256480e+02
  x = 3.493843660689910e-01
--- r2-bare-setseed [fork] exit=0
  rndseed=3
  @r1[r] = 8.392277021496984e+02
  rndseed=3
--- r2-bare-setseed [forkhunk] exit=0
  rndseed=1
  @r1[r] = 8.860341506323210e+02
  rndseed=1
--- r8-replay-2 [fork] exit=0
  @r1[r] = 9.251333325967927e+02
  x = 5.056166448630417e-01
  rndseed=2
--- r8-replay-2 [forkhunk] exit=0
  @r1[r] = 9.251333325967927e+02
  x = 3.493843660689910e-01
  rndseed=1
```

How the log gets its seed (read from code): `ng_drawlog_begin()` calls
`rng_seed_info()`, which returns static `rng_seed`, set only by `rng_note_seed()` at
each real srand/TausSeed. It never reads $rndseed. The seed column prints the same
`set_seed`. `eval_opt` runs before `ng_drawlog_begin` (inp.c:612, 864).

So: **no, the header and column would stay right** after `setseed k; reset`
(c4, measured: forkhunk identical to fork). They follow the generator. What changes
is the generator itself, whenever a control-language draw or bare `setseed` follows:
c7 forkhunk logs seed=1 src=checkseed, then seed=1 src=setseed, where the fork logs
7. The log is truthful about a wrong seed.

Read from code, not built: taking the whole commit, `.option seed=` would go through
com_sseed(wl), which sets `setseed_used` and notes src=setseed, so the log would say
src=setseed rather than src=option, and seed_from_option's warning would never fire.

`git merge-tree --write-tree --merge-base=95d7f2c2b^ ver_51 95d7f2c2b`:

```
Auto-merging src/frontend/com_option.c
Auto-merging src/frontend/inp.c
CONFLICT (content): Merge conflict in src/frontend/inp.c
Auto-merging src/frontend/options.c
CONFLICT (content): Merge conflict in src/frontend/options.c
Auto-merging src/main.c
CONFLICT (content): Merge conflict in src/main.c
Auto-merging src/sharedspice.c
CONFLICT (content): Merge conflict in src/sharedspice.c
```

Conflicts are real. e1db1d3df (with later f8f0f60d5) rewrites the block in eval_opt
that 95d7f2c2b deletes, so it conflicts there. a51568215 and 3db79a2e3 are receipts
only and cannot conflict. Other conflicts come from upstream changes the fork lacks:
eval_opt's signature, rshunt, inp_getopts' comfile, set_sseed in main.c and
sharedspice.c.

Recommendation: **keep ours, adapt two pieces.** Do not take the `!has_seed` block,
com_sseed's initw (breaks xschem's 45.2 identity), or getpid for seed=random. Take:
1. `option seed=` handling in com_option, because the fork ignores it silently, but
   fix the `wl->wl_word + 5` indexing when porting.
2. A guard for `.option seed=` with no value, because the fork segfaults.

Our warning is also tighter: it fires only after an explicit setseed; upstream's
fires on every deck with the option that is reset.

## For upstream

Minimal deck A, run with k=7 and k=8:

```
setseed lost on reset
v1 a 0 1
r1 a 0 1k
.control
set numdgt=15
setseed 7
reset
op
echo rndseed=$rndseed
let x=sunif(1)
print x
.endc
.end
```

Minimal deck B:

```
seed=random repeats on reset
.option seed=random
.param rv=agauss(1k,100,1)
r1 a 0 {rv}
v1 a 0 1
.control
set numdgt=15
foreach k 1 2 3
reset
op
print @r1[r]
end
.endc
.end
```

Output, before (a1148c969) and after (68b0d57b3):

```
--- m1-7 [a1148] exit=0
  Note: Seed value for random number generator is set to 7
  rndseed=7
  x = 2.149714827537557e-01
--- m1-7 [tip] exit=0
  Note: Seed value for random number generator is set to 7
  rndseed=1
  x = 3.493843660689910e-01
--- m1-8 [a1148] exit=0
  Note: Seed value for random number generator is set to 8
  rndseed=8
  x = -6.05475077405571e-01
--- m1-8 [tip] exit=0
  Note: Seed value for random number generator is set to 8
  rndseed=1
  x = 3.493843660689910e-01
--- m2 [a1148] exit=0
  @r1[r] = 9.920046254362252e+02
  @r1[r] = 9.810497760354855e+02
  @r1[r] = 1.018499132513811e+03
--- m2 [tip] exit=0
  Warning: Dot command .option seed = val/random is in the netlist:
      Command 'reset' will override all 'option seed=val' or 'setseed = val' settings!
  Note: Seed value for random number generator is set to 3049957
  Note: Seed value for random number generator is set to 3049957
  @r1[r] = 1.070122048365761e+03
  Note: Seed value for random number generator is set to 3049957
  @r1[r] = 1.070122048365761e+03
  Note: Seed value for random number generator is set to 3049957
  @r1[r] = 1.070122048365761e+03
```

Untested suggestion: the block should restore $rndseed only when an earlier deck's
`.option seed=` set it, rather than on every parse without the option.
