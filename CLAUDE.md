# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Ngspice: mixed-level/mixed-signal circuit simulator, C (plus some C++ for the Verilator shim), GNU Autotools. Derived from Spice3f5 + Cider1b1 + XSPICE. `AGENTS.md` in the repo root carries the same contributor guidance in condensed form.

## Build

Out-of-tree build is the normal workflow. A configured tree already exists at `build-ver_50/` (configured with bare `../configure`).

```sh
mkdir -p build && cd build && ../configure   # add feature flags as needed
make -j8
```

- `./autogen.sh` — regenerate `configure`/`Makefile.in` after touching `configure.ac` or any `Makefile.am`. Required after adding source files.
- XSPICE and OSDI are **on** by default (`--disable-xspice`, `--disable-osdi` to turn off). CIDER is **off** (`--enable-cider`).
- `--enable-debug` adds `-g -Wall`; `--enable-shortcheck` cuts model QA to BSIM3/4 only.
- `./compile_linux.sh [d]` does a full release/debug build in `release/`|`debug/` and installs to `/usr/local`. It runs `autogen.sh` itself. Use the out-of-tree flow for normal development instead.

## Tests

`make check` from the build directory runs regression → XSPICE → model QA suites (`tests/Makefile.am` gates the last two on config flags). Automake is configured with `serial-tests`, so a single test is:

```sh
make -C build-ver_50/tests/general check TESTS=rc.cir
make -C build-ver_50/tests/regression/parser check      # whole directory
```

Test harness differs per directory — read the `TESTS_ENVIRONMENT` in the local `Makefile.am` before assuming:

- Most dirs: `tests/bin/check.sh $NGSPICE test.cir` → runs `ngspice --batch`, pipes both actual and expected through a large `egrep -v` **filter** (timings, memory, version, warnings, index columns…), then `diff -B -w`. Committed `.out` files are only compared after filtering, so many differences are invisible by design.
- `tests/regression/pipe/`: feeds a `.cmd` file to `ngspice -p` on stdin, no `.out` comparison.
- `tests/xspice/digital/`: `check.sh` with `ngspice -r foobaz`.
- Model QA (`tests/bsim3`, `bsim4`, `hisim*`, `hicum2`, `bsimsoi`): `tests/bin/check_cmc.sh` → `runQaTests.pl` against `nmos/qaSpec`, `pmos/qaSpec`.
- `tests/lint/`: runs no binary and no deck. `tests/bin/identity_lint.sh` reads a `.lint` spec, scans `src/` with `$(AWK)` and compares the result against a frozen baseline. Last in `tests/Makefile.am`'s `SUBDIRS`, because a failing directory aborts `make check`'s recursion.

Adding a test: put the `.cir` in the most specific `tests/<area>/` dir, add it to that dir's `TESTS` list, commit the matching `.out` reference, re-run `autogen.sh`.

### Testing discipline (from AGENTS.md)

RED-first TDD for behavioral changes: write/extend the test, confirm it fails for the expected reason, make the smallest production change, re-run targeted test then the regression suite, refactor only while green. Never weaken an assertion or regenerate an `.out` file to hide a failure. If a test genuinely can't be written first, say why before implementing. No numeric coverage target — regression behavior is the acceptance criterion. Run the targeted dir's `make check`, then full `make check`, before submitting.

## Architecture

Two link targets share one core: `src/main.c` → the `ngspice` binary; `src/sharedspice.c` → `libngspice` (`--with-ngshared`), whose public C API is `src/include/ngspice/sharedspice.h` (`ngSpice_Init`, `ngSpice_Command`, `ngSpice_Circ`, `ngSpice_Reset`, event/sync callbacks). Anything touching global/static simulator state must survive repeated `ngSpice_Reset` in the shared build — recent commits (`c5cd68015`, `5ad395d5e`) are exactly this class of bug.

Layers:

- `src/frontend/` — the interactive "nutmeg" shell: input reading and netlist preprocessing (`inp.c`, `inpcom.c`, `subckt.c`, `numparam/` for `.param`/`.func`), the control-language parser (`parse.c`), output (`outitf.c`, `rawfile.c`), plotting (`plotting/`, `wdisp/`). Shell commands live in `com_*.c` and are registered in the `spcp_coms[]` table in `src/frontend/commands.c` — a new command needs an entry there plus its `com_*.c`/`.h` and a `Makefile.am` update.
- `src/spicelib/parser/` — turns the expanded netlist into the CKT structure and matrix.
- `src/spicelib/analysis/` — DC/AC/TRAN/noise/pz/sens/PSS analyses.
- `src/spicelib/devices/<dev>/` — one directory per device model, each built as a libtool convenience library. Registration is the `static_devices[]` array in `src/spicelib/devices/dev.c`, which builds the runtime `DEVices[]` table; a new device needs an entry there, a subdir `Makefile.am`, and a line in `DYNAMIC_DEVICELIBS` in `src/Makefile.am`.
- `src/maths/` — `ni/` (numerical core), `sparse/` and `KLU/` (matrix solvers), `cmaths/` (control-language math), `fft/`, `deriv/`, `dense/`.
- `src/include/ngspice/` — all public headers; new cross-module declarations go here.
- `src/osdi/` — loads OpenVAF-compiled Verilog-A models (`.osdi`) via the `osdi` command.
- `src/ciderlib/` — CIDER 2D device simulator (opt-in).

Outside `src/`: `tests/` (per-feature suites, drivers in `tests/bin/`), `examples/` (demo decks), `man/` (short UNIX man pages), `m4/` (Autoconf macros), `visualc/` (MS Visual Studio project files).

`src/spinit.in` is the runtime init script installed to `$pkgdatadir/scripts/spinit`; it is what actually loads the code models and OSDI models at startup. Behavior that depends on a `codemodel`/`osdi` line being present is configured there, not in C.

### XSPICE and code models

`src/xspice/` implements the event-driven mixed-signal layer. `cmpp/` is a preprocessor (lex/yacc) that turns a code model's `cfunc.mod` + `ifspec.ifs` into C; `icm/{analog,digital,xtradev,xtraevt,table,tlines,spice2poly}/` are the shipped code models, built into loadable `*.cm` shared libraries. `evt/` is the event scheduler, `mif/` the model interface glue.

### Verilog co-simulation (`src/xspice/verilog/`)

The `d_cosim` code model (`src/xspice/icm/digital/d_cosim/cfunc.mod`, interface in `ngspice/cosim.h`) loads a user-built shared library that wraps a compiled Verilog design. Two backends:

- **Verilator**: `vlnggen` + `verilator_main.cpp` + `verilator_shim.cpp`. **`vlnggen` is an ngspice interpreter script, not a shell script** — it starts with `*ng_script_with_params` and is written in ngspice control language (`set`, `if`, `fopen`, `$oscompiled`, `shell`). It drives Verilator, then links the generated `Vlng__ALL.a` plus the global objects (`verilated.o`, `verilated_threads.o`, conditionally `verilated_timing.o` and `verilated_vcd_c.o`) into a `.so`. Object selection is probed with `fopen` because which globals exist depends on the Verilator options the user passed.
- **Icarus Verilog**: `icarus_shim.c`, `vpi.c`, `coroutine*.h`, built into shared libraries during the ngspice build.

Examples: `examples/xspice/verilator/`, `examples/xspice/icarus_verilog/`.

## Conventions

- Match the style of the file being edited. C is generally four-space indent, same-line opening brace, `snake_case` for newer identifiers, uppercase macros. No repo-wide formatter — do not reflow or churn whitespace. Prefer existing helpers over new ones.
- **Comparing two strings.** `make check` lints every `strcmp`/`cieq`/`eq`/`eqc` in `src/` whose operands are both runtime expressions (a literal operand is never reported). A comparison of a name a deck wrote must go through `ng_ideq()`, `vec_name_eq()` or `Evt_Node_Name_Eq()`; a comparison of a word the language defines stays byte-exact and takes a `/* case-lint: keyword - <why> */` comment on its own line or the line above. Nothing checks the reason word — it is written for the reviewer — so beyond `keyword` the tree also uses `helper` (the call *is* an identity helper, or a mode arm of one), `stored` (both operands are names the simulator holds for one run) and `neither` (a comparator's own definition, a sort, a deliberate case-insensitive scan); `tests/bin/identity_lint.sh` prints the list when it fails. See `AGENTS.md` and `doc/claude/decisions/0011-identity-lint.md`.
- Update the nearest `Makefile.am` when adding sources or tests, then re-run `./autogen.sh`.
- Commits: short imperative summaries; keep unrelated work separate. PRs state problem, solution, affected features, config flags, and tests run, and link relevant reports; include logs or a minimal `.cir` deck for simulation changes. Screenshots only matter for plotting/GUI changes.
- `doc/codex/` holds analysis artifacts in a numbered convention: `issues/NNNN-slug.md` (Status / Summary / Impact / Root Cause / Acceptance Criteria / Resolution), plus `code_analysis/` and `suggestions/`. Follow that structure when writing up a defect.
- Branching: `pre-master-47` is the upstream-tracking main branch; feature work happens on branches like `ver_50`.
