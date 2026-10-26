# Receipt: item 0 — scoping the xschem ver_51 handoff

This receipt covers the scoping step in `doc/claude/batches/2026-10-24-mc-v51/PLAN.md`. It comes
before item 1.

Synthesis and critic pass, 2026-10-24, branch `ver_50`, HEAD at start
`ccebdf2a2`. **No code was touched.** No source files were edited and nothing was built.
Three read-only crews ran first, one each for P1, P2+P3 and P4. This pass then
challenged their findings and spot-checked the ones the plan relies on most,
reading source and running five tiny decks with
`ngheavy build-ver_50/stage/bin/ngspice -b`. The only files written in the
repository are the three batch files. The scratch decks are in
`/tmp/claude-1000/-home-analog-dev-ngspice/1fbc2faa-b3bb-423f-b156-a41158051465/scratchpad/`.
That directory also holds the crews' decks: `rb.cir`, `bfix.cir`, `tn.cir`,
`p2_*.cir`, `p3_*.cir`, `s7.cir`, `sub.cir`, `glob.cir`, `ck.cir`, `pid.cir`.

The handoff is
`/home/analog/private/xschem-private/doc/claude/mc_spike/NGSPICE_V51_HANDOFF.md`.

## What was verified

| claim (handoff §3) | verdict | evidence |
| --- | --- | --- |
| P1: R→B rewrite drops the `.model` | **confirmed, re-measured** | `inpcom.c:7425` builds `b<name> … i = v(n1,n2)/(eq) <tc1 tc2> <m> reciproctc=1 reciprocm=0`. Only `eval_tc` and `eval_m` are copied, and the model token is lost. `spot1.cir` at 125 °C read `v(a) = 7.300000e-02` (converted, with model) and `v(b) = 5.694000e-02` (plain R, same model). |
| P1: impact −5.95 % at 0 °C / +28.1 % at 125 °C | confirmed for the TC term | The crew's probe gave −5.96 % / +28.2 %. The 70 °C point and the bandgap slope flip were not re-measured. |
| P1: the converted R is noiseless unless `enable_noisy_r` / `noisy=1` | confirmed | `inpcom.c:7417-7422`. ASRC has no noise routine. |
| P1: ASRC supports `tc1`/`tc2`/`dtemp`/`temp` | confirmed | `asrc.c:17-23`. It has no `tnom` and no `tce`. A hand-written B line with `dtemp={27-25}` matched the model R to all printed digits (crew, `bfix.cir`). |
| P2: `.option seed=` re-seeds on every reset, silently | **confirmed, re-measured** | `inp.c:442-466`. `spot1.cir` with `.option seed=5` and `setseed 1|2; reset; op` read `@r3[r] = 9.312134e+02` both times, and nothing was written to stderr. |
| P3: `initw` seeds from the pid | **confirmed, re-measured** | `wallace.c:83-85` runs after `main.c:936-938` (`rndseed=1`, `com_sseed(NULL)`), with the call at `main.c:1371`. The same sequence is at `sharedspice.c:931-935, 1080`. In two runs of `spot3.cir`, startup read `918.377` and `1147.4`, and after `setseed 1; reset` it read `886.034` both times. |
| P3: `setseed k` stays bit-identical if `com_sseed` is kept | confirmed | `com_sseed` (`randnumb.c:293-322`) re-seeds all eight Taus states through `TausSeed`. The crew measured `setseed 7`, giving `@r1[r] = 9.068279585256480e+02` on both the fork and 45.2. |
| P4: draws in `formula()`, `eval_agauss()` | confirmed | `xpressn.c:1152-1163` dispatches to the helpers at `:47-86`, `limit` included. `eval_agauss` is at `inp.c:2569`, with its helpers at `:2513-2552`. |
| P4 Q3: numparam can see the expanded instance path | confirmed | `nupa_eval` sets `dicoS->cardline = s` (`spicenum.c:701`), and the first token of that line is already the flattened path (`subckt.c:401-409`). |

## What was refuted in the handoff

- **P1's acceptance criterion.** `expect.py` cannot judge the fix. Its line
  `bexp=rb1*f(-2.47e-3,2.7e-6,25,T)` takes the *measured* body resistance at
  each T as the nominal value. That is right only while the body is flat. Once
  the body has its TC, the oracle applies the TC a second time. The script also
  asserts nothing, unpacks only `r[:7]`, and hard-codes the `tt` value of
  `tc1`. The plan replaces it with a self-contained `check.sh` deck held to 1e-9.
  The "PDK-side fix ≤4.1e-7" figure cannot be reproduced from `fixp.cir` with
  `expect.py`, because the same double application happens there, so it is
  **unverified**.
- **P1's rule "a B-source whose expression uses `temper` must not also get the
  model TC".** It is refuted as a skip. `b_transformation_wanted`
  (`inpcom.c:6354`) converts any R whose value mentions `temper`, so a skip would
  drop real model TCs. The plan applies the TC and warns.
- **P1's sketch "resolve in the enclosing subckt, else at top level" and
  "50–80 lines".** Scopes can be nested to any depth, and sky130 defines the
  model *after* the instance. The per-scope model lists are freed before
  `inp_compat` and never built in `.if` decks (`inpcom.c:1504`). Instance
  `dtemp=`/`temp=` are dropped today even with no model. When `temp` is given,
  ASRC ignores `dtemp` (`asrctemp.c:28-36`). The parameters override one by
  one, with aliases (`tc`, `tc1r`, `tc2r`). This comes to about 130–180 lines.
- **P1's optional `300.15 → CKTnomTemp`.** It is incompatible with the
  parse-time `dtemp={27-tnom}`, because `option tnom` can be set after the
  parse (crew `tn.cir`). Declined, as decision 3.
- **P1's "candidate N" (`tc` on the instance line) in `tc.cir`.** It is not a
  fix. It uses a 27 °C reference: under `option tnom=25`, R4 reads `0.0733614`
  against the model R's `0.073`, and R6's `dtemp=2` is dropped.
- **P2's "with `seed=random` nothing is repeatable".** Each re-seed is
  written to `$rndseed` (`inp.c:455`), so a run can be recovered afterwards.
  The warning text must not claim otherwise.
- **P3's "any draw made before the first `setseed` differs".** That is too
  broad. The control-language `sunif`/`sgauss`/`rnd`/`poisson`/`exponential`
  re-seed through `checkseed` (`randnumb.c:77-92`, whose static `oldseed`
  starts at 0) and are deterministic. What differs is the parse-time draws and
  the Wallace pool (TRNOISE).
- **P3's path.** `src/maths/misc/wallace.c` does not exist. The file is
  `src/frontend/trannoise/wallace.c`, and the RNG core is
  `src/maths/misc/randnumb.c`.
- **P4's "agauss in `inpptree.c`".** That file has `gauss` only. It folds
  `gauss` to a constant (`inpptree.c:1187-1210, 1695-1702`) and has no
  agauss/aunif/unif/limit.
- **P4's `param` = `rbody`.** `.func` names are expanded away as text before
  any draw (`inpcom.c:1528`), and the body R has become a B line by then, so the
  key is `i`. The instance path (`b.xr1.brbody`) is still unique.
- **P4's "instance context from `nupa_substitute()`".** The `s` there is
  the unexpanded subckt-body line that every instance shares. The instance comes
  from `nupa_eval`'s card line. Only the *parameter* key comes from
  `nupa_substitute` or `nupa_assignment`.
- **P4's `.param` agauss redrawn at every reference.** True, but not always.
  When the agauss is reached only through a `.func` call, the `.param` is not
  converted, and it is drawn once and shared (crew `glob.cir`).
- **P4's "z is unit-uniform".** `drand()` is in [-1,1)
  (`randnumb.c:95-98`).
- **The handoff's 80–150 lines for P4** is low. It comes to about 180–250
  lines once seed-source tracking and context capture in five places are
  counted.

## Where the crews were challenged

- **Two RED tests would have failed for the wrong reason.** The P3 deck in
  the P2+P3 crew's findings (`let a=…; setseed 1; reset; op; let b=…; if a = b`)
  can never pass. A vector from plain `let` is gone after `reset`. `spot4.cir`
  printed `Warning from checkvalid: vector a is not available or has zero
  length.` and took the else branch. The fix is `let const.a = …`. With that,
  the positive control (`spot5.cir`, `setseed 1` on both sides) reads `SAME`,
  and the pid case (`spot4.cir` with `const.a`) reads `DIFF` today. The P1
  deck's `vecmax` accumulation across `foreach … reset` and P2's optional
  "option still wins" assertion have the same flaw. PLAN makes `const` a
  standing constraint.
- **P4's numeric RED cannot self-check.** `spot6.cir` showed that `fread`
  returns the tab-separated row intact, but `set row = ( $line )` gives
  `$#row = 1`, so the control language cannot split the row into fields. The
  crew's `.out`-golden alternative would freeze z values that depend on glibc.
  The plan uses `check_status.sh`'s `.files` `grep` for structural checks and
  does the 1e-9 z-to-readback check in the receipt.
- **P4's log-off guard deck is not a RED test.** It is green today by design.
  The plan labels it a guard.
- **Claims marked "confirmed" without a run:**
  - P4's "mmB: XM1 takes M2's old draw". The crew says it was not re-measured
    on sky130, and only the deck mechanics were checked (`sub.cir`). It is
    better read as *partly*.
  - P1's "control R3 and high_po unchanged". This is argued from the read path
    (`@r.xr2.x0.rbody`, which is a real R), not measured. It is plausible, and
    item 1's acceptance covers it through the in-tree R→B decks.

  Every other "confirmed" verdict carries file:line evidence, and the most
  load-bearing ones were re-read here: `inpcom.c:7383-7451`, `inpcom.c:1485-1510`
  and `:1570-1582`, `inp.c:428-470`, `options.c:263-282`,
  `wallace.c:76-90`, `main.c:930-940` and `:1365-1373`,
  `sharedspice.c:928-936` and `:1076-1082`, `randnumb.c:290-325`,
  `spicenum.c:695-705`, `xpressn.c:1150-1165` and `:1420-1437`.
- **The ngSpice_Reset rule.** No crew design keeps state past `totalreset`
  without clearing it. The plan's P4 enable is changed from "read `rnddraws`
  at each parse" to a `cp_usrset` hook. Then `unset_all` closes the log, and no
  path-against-path comparison is needed, so the design neither breaks the
  identity lint nor truncates the file on every reset.
- **Identity lint.** P1 matches names through `model_name_match()`
  (`inpcom.c:9986`, `:10106`) and looks up parameter names against literals. P2
  and P3 add no comparisons. P4's only literal-operand checks are on
  `"rnddraws"`. None of them is reported.
- **Harness choice for P2.** `check.sh` deletes every `Warning` line
  (`check.sh:20` FILTER), and `check_status.sh` keeps it (`:156-159`). That
  confirms the crew's choice. One new directory, `tests/regression/rng/`, holds
  P2, P3 and P4.

## What is left open

- **Q1, whether `ver_51` is a new branch off `ver_50`.** That is the owner's
  decision, recorded as decision 1. Nothing has been created.
- **Q2, `reset` about 20 % slower than 45.2.** Not profiled. In rough order, the
  hypotheses are:
  - 368 of the 575 commits from `ngspice-45.2` to `ver_50` are upstream, so time
    an upstream `pre-master-47` build before blaming fork commits. `c896fed41`,
    which does more subckt-expansion work, is one candidate.
  - `reset` bypasses `inp_readall` (`inp.c:546-555`), so it gets none of the
    4.2× parse gain.
  - The fork's case-report passes run per parse (`spiceif.c:214, 223`,
    `inpsymt.c:135-139`). They are probably small.
  - numparam key folding runs only under `casemode=preserve`.
  - Ruled out by reading the code: duplicated `line_case` copies, because
    `inp_deckcopy` does not copy it (`subckt.c:856`).
- **Q4, P1 noise.** The recommendation is to keep it behind the switch and
  give the noise branch the model. The owner decides, as decision 2.
- **The P4 log contract.** The field order is kept. Changes to the field values
  and the added `#` lines need xschem's agreement, as decision 7.
- **Not re-measured here:** the 70 °C point and the bandgap Vbg sign flip, the
  sky130 mmA/mmB behaviour, and the origin of the ≤4.1e-7 PDK-side figure.
- **Recorded and not planned:**
  - TRNOISE is not reproducible even after `setseed`, because the Wallace pool
    is never refilled.
  - `checkseed`'s `oldseed`, `seedinfo` and `gauss0`'s cache survive
    `ngSpice_Reset`.
  - The `{random}` arm in `eval_opt` is dead code (`options.c:271`).
  - `insertnumber` can leave `dico->cardline` dangling, and
    `message()` reads it under `ngdebug` (`xpressn.c:235`).

## For the owner

Decisions 1–8 in PLAN must be answered before item 1 starts, and decision 7 needs
xschem's answer as well. The baseline `make check` count has not been taken. The
first code crew takes it.
