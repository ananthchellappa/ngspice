# Stage 4 receipt — issue 0062

**Path** — `/home/qflow/dev/ngspice_test/doc/codex/issues/0062-shared-build-command-after-reset.md` (478 lines, six headings in order, Resolution left "Unresolved."). Nothing under `src/` or `tests/` touched.

**Title** — "Issue: After `ngSpice_Reset()` the Shared Library Is De-Initialised, `ngSpice_Command()` Goes Silent and `ngSpice_Circ()` Segfaults"

**Reproduced** — Yes. The verifier's harness at `/home/qflow/dev/ngspice_test/doc/claude/feedback/ngspice_upstream/repro/shared/casemode_reset_probe.c --reset` still segfaults (rc=139) against the existing `build-shared/` tree (98 MB, `../configure --with-ngshared`, `ngspice-46+`). Both reported symptoms confirmed.

**Minimal repro** — four API calls, no deck features, no casemode. Committed as `/home/qflow/dev/ngspice_test/doc/claude/feedback/ngspice_upstream/repro/shared/reset_reinit_probe.c`:

```c
ngSpice_Init(cb_char, NULL, cb_exit, NULL, NULL, NULL, NULL);
ngSpice_Reset();
printf("Command rc=%d\n", ngSpice_Command("echo COMMAND-RAN"));  /* rc=1, no output */
printf("Circ rc=%d\n",    ngSpice_Circ(deck));                   /* SIGSEGV */
```

Adding one `ngSpice_Init()` after the reset (`--reinit`) makes both halves vanish — that pair is the load-bearing measurement. Two more probes committed alongside: `reset_sequence_probe.c` (call order as `argv[1]`; `RC` crashes, `CR` does not, `CRIC` survives) and `reset_devtable_probe.c`.

**Root cause I actually landed on** — not what the verifier guessed. `totalreset()` (`src/sharedspice.c:2530`) is a *de-initialiser* by design; `d0ae65acc`'s own commit message says "so that it may be restarted again by ngSpice_Init", and the shipped `examples/shared/shx.c` re-Inits after every reset (`:174`, `:308`). The defect is that nothing states or enforces that: `sharedspice.h:552` documents the call as six words, `ngSpice_Command()`'s guard at `:1140` returns `1` in silence while the `no_init` message twenty-three lines below at `:1163`-`:1166` is dead code, and `ngSpice_Circ()` (`:1247`) checks nothing at all.

The crash itself is `spice_destroy_devices()` (`sharedspice.c:2593` → `dev.c:239`) with no matching `spice_init_devices()` on the way back up. It leaves *two* broken views, measured:

```
after Init   DEVices=0x5947708a9460  ft_sim->devices=0x5947708a9460  ft_sim->numDevices=132
after Reset  DEVices=(nil)           ft_sim->devices=0x5947708a9460  ft_sim->numDevices=132
```

`SIMinit()` took copies at `sharedspice.c:441`-`:442` that the free cannot reach.

**Correction to the verifier's attribution** — `CKTmodCrt()` is a symptom, and only one of two. Which frame dies is selected by the function-static `static int type = -1` at `src/spicelib/parser/inp2v.c:20`: cold cache → `INPtypelook()` walking 132 entries of freed memory (`inptyplk.c:27`, `:37`); warm cache → `CKTmodCrt()`'s `DEVices[type]` NULL deref (`cktmcrt.c:31`), which is what the verifier's harness hits because it reads a circuit before resetting. A fix aimed at `CKTmodCrt()` would relocate the crash, not remove it.

**Scope** — casemode appears in two sentences: one in Status (how it was found) and one in Impact measuring it *out* (`inp_case_announce_reset()` only clears a static bool, `inpcom.c:1117`-`:1121`). Pre-existing and upstream: both lines are `d0ae65accf`, 2024-06-23, with no local change on either. Invisible to `make check` — no file under `tests/` links `libngspice`, and `build-ver_50/` is not a shared build.