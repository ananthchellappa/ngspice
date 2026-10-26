# Submission texts for official ngspice — ready to paste

Checked by an independent crew on pre-master-48 `a1148c969`: series applies,
`make check` 63 PASS / 0 FAIL, every number re-measured. Nothing has been posted.

Steps: (1) log in to SourceForge; (2) post the forum text below at
https://sourceforge.net/p/ngspice/discussion/127605/; (3) file the three
tickets at https://sourceforge.net/p/ngspice/patches/ in order 1, 2, 3,
attaching the named `.patch` file to each, or attach them to the forum thread if
ticket creation is blocked. Patch P3 is not sent: upstream fixed it
(bugs #872/#873).

Before posting: decide whether to keep the AI-disclosure sentence (it matches
the `Co-Authored-By: Claude` trailer in each patch; drop both or keep both), and
whether `ananth.chellappa@outlook.com` is the address you want public.

## (A) Forum post

Subject: Three small patches against pre-master-48: R->B rewrite drops the .model, crash on a typeless .model with .if, .option seed vs setseed

Hello,

I'm Ananth, an IC designer. I use ngspice with xschem on sky130, mostly for Monte Carlo and sensitivity work. While doing that I ran into three problems and wrote a fix for each:

1. An R card rewritten into a B source (value uses v(), i(), temper, hertz or time) loses its .model, and with it the model's tc1/tc2/tnom, and its instance dtemp/temp. sky130's xhigh_po is such a card; in a minimal deck with similar coefficients the converted resistor is 28% off at 125 C.
2. A `.model` card without a type (e.g. `.model bad`) crashes ngspice with SIGSEGV when the deck has an `.if`, even in a branch that is not taken.
3. `.option seed=N` silently overrides `setseed` on every `reset`, so a `setseed k / reset / op` Monte Carlo loop draws the same values each time. The patch keeps the current behaviour but warns once. It also prints the existing "Multiple seed" and "Cannot convert" warnings once per session instead of on every reset.

The patches are git format-patch files against pre-master-48 a1148c969. Patches 1 and 2 add regression tests. Patch 3 has none, because its output goes to stderr, which check.sh does not compare. With all three applied, `make check` passes (63 PASS, 0 FAIL). I wrote them with help from an AI coding assistant (Claude), and the commits say so in a Co-Authored-By line; I have reviewed and tested them.

How would you prefer to receive them? I would have opened tickets on the patch tracker, but creating a ticket there needs admin authorization. I'm happy to attach them here, mail them, or use whatever you prefer.

A small side note: a1148c969 does not build from git as it is. configure.ac and tests/xspice/Makefile.am reference tests/xspice/analog/, but that directory was not committed. I reverted those two lines locally to build.

Thank you for ngspice and for your time.

Ananth

(B) Tickets

Apply order for all three: 1 (p1), then 2 (p4-crash, the on-p1 version), then 3 (p2). Patch 2 is made on top of patch 1, because both change the "no model type given" check in inpcom.c. There is also a version of patch 2 for a tree without patch 1, if patch 1 is not wanted.

---

Ticket 1

Title: inpcom.c: R rewritten into a B source loses its .model (tc1/tc2/tnom) and dtemp/temp

Description:

Problem
inp_compat() rewrites an R card whose value needs a behavioural source (it names v(), i(), temper, hertz or time) into a B source. The B line is built from the equation, the instance tc1/tc2 and m only. So a .model named on the card is dropped, with its tc1/tc2/tnom, and so are the instance dtemp and temp. The converted resistor stays flat over temperature. sky130's xhigh_po is such a card.

Minimal deck
```
R->B rewrite drops the model
.model rm r tc1=-2.47e-3 tc2=2.7e-6 tnom=25
i1 0 a 1u
r1 a 0 rm r={73k*(1+1e-9*abs(v(a)))}
i2 0 b 1u
r2 b 0 rm r=73k
.control
set temp=125
op
print v(a) v(b)
.endc
.end
```

Results at 125 C
- ngspice-45.2 and pre-master-48 a1148c969: v(a) = 7.300000e-02, v(b) = 5.694000e-02 (28% off).
- With the patch: v(a) = 5.694000e-02, v(b) = 5.694000e-02.

Fix
The rewrite now looks up the card's model in its own scope and the scopes above it, by exact name. It writes the model's tc1/tc2 onto the B line, and turns tnom into a dtemp (or temp) offset. Instance tc1/tc2 override the model's. Instance dtemp/temp are kept and combined with that offset. With the noisy-R switch on, the noise branch's resistor carries the model. If the model cannot be resolved, or uses tce, or uses temper, ngspice warns and the line is left as before. A card without a model is rewritten exactly as before.

Behaviour change: an R that is rewritten into a B source and names a .model now gets that model's temperature coefficients. A PDK workaround that folded the TC into the expression by hand would apply it twice.

Tests added (tests/regression/temper, check.sh)
- temper-res-behav-1.cir: eight cases at 0, 25 and 125 C, within 1e-9.
- temper-res-behav-noise.cir: the noise branch.
- temper-res-behav-noise-subckt.cir: the noise branch in a subckt whose model shadows a global one.
- temper-res-behav-dup.cir: the same .model in both branches of an .if.

On pre-master-48 the four decks fail without the patch (maxerr 0.314, 0.145, 0.156, 0.282) and pass with it. `make check` passes.

Apply order: first of the three.
Dependency: the decks use `let const.x` across `reset`, which needs 785be3194. That commit is already in ngspice-47 and pre-master-48, so nothing else is needed.

Attach: /home/analog/dev/ngspice/doc/claude/batches/2026-10-26-mc-v51-followup/upstream/p1/0001-inpcom.c-carry-the-R-model-through-the-R-B-rewrite.patch

---

Ticket 2

Title: inpcom.c: crash (SIGSEGV) on a .model card without a type in a deck with .if

Description:

Problem
A .model card that stops before its type (`.model bad`, or `.model` alone) crashes ngspice when the deck contains an .if, even when the card is in a branch that is not taken. Without .if, the same card gets "Error: no model type given in line .model bad!" and exit 1.

Cause: with .if, inp_readall() skips inp_rem_unused_models(), which is where the type check lives, and the card reaches the macro expander. There, inp_expand_macro_in_str() calls strchr() on the NULL that findtok_noparen() returns. For `.model` alone in a used subcircuit, modtranslate() puts a NULL name into its wordlist. has_if is never cleared, so later decks in the same session take the same path.

Minimal deck
```
typeless .model in a dead .if branch
.if (0)
.model bad
.endif
i1 0 1 1u
r1 1 0 1k
.control
op
print v(1)
.endc
.end
```

Results
- ngspice-45.2 and pre-master-48 a1148c969: Segmentation fault, exit 139. The same happens with `.if (1)`.
- With the patch: the dead-branch deck runs (v(1) = 1.000000e-03, exit 0). With `.if (1)`, ngspice prints "Error: no model type given in line .model bad!" and exits 1, the same as a deck without .if.
- The dead-branch deck also runs when it has an R that ticket 1 rewrites into a B source and that names a .model, e.g. `.model rm r tc1=-2.47e-3 tnom=25` and `r1 1 0 rm r={1k+0*v(1)}`: v(1) = 9.950600e-04, exit 0 (exit 139 on pre-master-48).

Fix
inp_expand_macro_in_str() skips the tokens with skip_token(), which never returns NULL. modtranslate() leaves a nameless .model card alone. The type check is factored out of inp_collect_models() (added by ticket 1) into inp_chk_model_type(), and inp_spsource() runs it on the flat deck right after dotifeval(). In a deck with .if, inp_collect_models() skips a typeless card instead: ticket 1 calls it for the R->B rewrite before the .if is decided, when the card may still be in a branch that is not taken. The error text is unchanged.

Tests added (tests/regression/model, check.sh)
- model-typeless-if-dead.cir: '.model bad' in an '.if (0)' branch.
- model-typeless-if-dead-behav.cir: the same, plus an R that the R->B rewrite converts and that names a .model. It also checks the carried value.
On pre-master-48 both crash without the fix and pass with it (3 of 3 runs). The error cases exit 1 with the message on stderr, which check.sh does not see.

Apply order: second, after ticket 1. A version for a tree without ticket 1 is available too.

Attach: /home/analog/dev/ngspice/doc/claude/batches/2026-10-26-mc-v51-followup/upstream/p4-crash/on-p1/0001-inpcom.c-do-not-crash-on-a-.model-card-without-a-typ.patch

---

Ticket 3

Title: .option seed= silently overrides setseed on every reset; warn once

Description:

Problem
eval_opt() re-seeds the generator from `.option seed=N` (or `seed=random`) on every parse, and `reset` is a parse. In the usual Monte Carlo loop `setseed k / reset / op`, the option beats the setseed just before it without any message, and every sample draws the same values. The existing "Multiple 'option seed=val|random' found!" and "Cannot convert 'option seed=...'" warnings are printed again on every reset.

Minimal deck
```
.option seed overrides setseed
.param rv=agauss(1k,100,1)
.option seed=5
r1 1 0 {rv}
i1 0 1 1m
.control
setseed 1
reset
op
print @r1[r]
setseed 2
reset
op
print @r1[r]
.endc
.end
```

Results
- ngspice-45.2: @r1[r] = 9.312134e+02 on both samples, and nothing is printed.
- pre-master-48 a1148c969: @r1[r] = 7.974070e+02 on both samples, and nothing is printed.
- With the patch: the values are unchanged (the option still wins). This warning is printed once per session:
```
Warning: '.option seed=5' re-seeds the random number generator each time the
    circuit is parsed, by 'source' and by every 'reset', so it overrides 'setseed'.
    Remove the option to let 'setseed' choose the seed.
    (This warning is printed once per session.)
```
- With `.option seed=abc` and `setseed 3 / reset / op` three times, "Cannot convert" is printed 4 times before the patch and once after. @r1[r] is 8.432359e+02 on every sample on pre-master-48 (8.392277e+02 on 45.2), before and after.
- With two `.option seed=5` lines and the same loop, "Multiple" is printed 4 times before the patch and once after.

Fix
eval_opt() calls the new seed_from_option(), which does the same cp_vset("rndseed") + set_sseed(NULL) as before. It warns first, but only after an explicit `setseed N` or `setseed random`; a bare `setseed` does not count, and a deck without setseed stays silent. seed_option_multiple() and seed_option_unusable() print the two existing warnings once per session. rnd_reset_state(), called from totalreset() in sharedspice.c, clears the flags, so a new shared-library session warns again. The sequence of draws and stdout are unchanged.

Tests: none in make check, because the output goes to stderr and check.sh does not compare stderr. On pre-master-48 I checked seven small decks: override, seed=random, two seed= options, seed=abc, a bare setseed only, setseed random, and no option. The warnings appear as described. Stdout is byte-identical before and after, except for the two decks whose seed changes from run to run: seed=random (clock) and setseed random (process id in its Note line). I also built libngspice (--with-ngshared) and ran a reset probe over three ngSpice_Init / ngSpice_Reset sessions, and it passed. `make check` passes.

Apply order: third, after tickets 1 and 2. It does not depend on them and applies on its own too.

Attach: /home/analog/dev/ngspice/doc/claude/batches/2026-10-26-mc-v51-followup/upstream/p2/0001-randnumb.c-warn-once-when-.option-seed-overrides-set.patch
