# Repository Guidelines

## Project Structure & Module Organization

Ngspice is an Autotools-based C project. Core code lives in `src/`: commands in `frontend/`, circuit logic in `spicelib/`, numerical routines in `maths/`, and public headers in `include/ngspice/`. XSPICE, CIDER, and OSDI have dedicated subdirectories. Tests are organized by feature in `tests/`, with drivers in `tests/bin/`. Demonstrations belong in `examples/`, manual pages in `man/`, Autoconf macros in `m4/`, and Visual Studio files in `visualc/`.

## Build, Test, and Development Commands

- `./autogen.sh` regenerates Autotools files after build metadata changes.
- `mkdir -p build && cd build && ../configure` creates an out-of-tree build. Add feature options such as `--enable-cider` when needed.
- `make -j8` builds ngspice from the configured directory.
- `make check` runs regression, XSPICE, and enabled model QA suites.
- `../configure --enable-shortcheck && make check` runs a reduced regression set.
- `./compile_linux.sh d` performs a full Linux debug build and installs to `/usr/local`; prefer the out-of-tree workflow for normal development.

## Coding Style & Naming Conventions

Follow the edited file's style. C generally uses four-space indentation, same-line opening braces, `snake_case` for newer identifiers, and uppercase macros. Keep public declarations in `src/include/ngspice/` and use existing helpers. No repository-wide formatter is configured; avoid whitespace churn. Update the nearest `Makefile.am` when adding sources or tests.

### Comparing two strings

`make check` runs a lint (`tests/lint/`) over every `strcmp`, `cieq`, `eq`, `eqc` and relative in `src/` whose operands are **both runtime expressions**. A comparison against a string literal is never reported. A reported comparison is not a bug — it is unclassified, and it is asking one of two questions that no regular expression can tell apart, so you have to say which:

- **A name a deck wrote** — a node, a vector, an instance, a `.model`, a `.subckt`, a `.param`, an XSPICE event node. Two spellings are two names under `casemode=distinguish`, so call `ng_ideq()`, `vec_name_eq()` or `Evt_Node_Name_Eq()`, or guard with `inp_case_exact_ids()`. `doc/claude/decisions/0001-distinguish.md` decision 3 classifies every existing site.
- **A word the language defines** — a command, an option, a device letter, an analysis or model type, a parameter keyword. These stay byte-exact in all three case modes. Say so on the line and say why:

  ```c
  if (eq(a, b))   /* case-lint: keyword - both operands are command names */
  ```

  The marker suppresses the call on its own line and the call on the line directly below it, and nothing else.

Adding the line to `tests/lint/identity.baseline` is the third option and is for a comparison that is a genuine identity test and cannot be fixed yet; such an entry must name the issue that will fix it. The lint also fails on a baseline entry it can no longer find, so a rewrite of an existing comparison means deleting its line — `make check` prints exactly which.

## Testing Guidelines

Use RED-first test-driven development for behavioral changes:

1. Add or update a test that expresses the required behavior before editing production code.
2. Run the targeted test and confirm that it fails for the expected reason (RED).
3. Make the smallest production change that makes the test pass (GREEN).
4. Re-run the targeted test, then the relevant regression suite.
5. Refactor only while all affected tests remain green.

Add `.cir` inputs to the most specific `tests/<area>/` directory and register them in that directory's `TESTS` list. Commit the matching `.out` or model-specific reference file when comparison is required. For example, run `make -C tests/general check`, then `make check` before submission. Do not weaken assertions or update expected output merely to hide a failure. If a test cannot practically be written first, explain why before implementation. The project has no numeric coverage target; regression behavior is the acceptance criterion.

## Commit & Pull Request Guidelines

Recent history uses short, imperative summaries; describe the change directly and keep unrelated work separate. Pull requests should explain the problem, solution, affected features, configuration flags, and tests run. Link relevant reports. Include logs or minimal circuit decks for simulation changes; screenshots matter only for plotting or GUI changes.
