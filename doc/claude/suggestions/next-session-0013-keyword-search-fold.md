# Next-session prompt — fix issue 0013

Paste everything below the line into a fresh session at
`/home/qflow/dev/ngspice_test`, branch `ver_50`.

---

Fix doc/codex/issues/0013 on branch ver_50: the three whole-token search
helpers in src/frontend/inpcom.c hit with strstr, so every lower-case keyword
literal they are called with stops matching under casemode=preserve. Read that
issue, doc/claude/checklists/phase2-differential-sweep.md,
doc/claude/suggestions/case-sensitive-identifiers-plan.md and
doc/claude/code_analysis/case-insensitivity-origins.md first — they are ground
truth. Also read doc/codex/issues/0009 and 0010, the two immediately preceding
pieces of work; 0013 is the keyword half of the defect whose device-letter half
was 0009, and 0010's Resolution explains why the two remaining PARSE-FAILs in
the sweep are yours and not its.

Do not start Phase 3. Do not touch issues 0011, 0014, 0015 or 0016.

Phases 0, 1, 2 and issues 0009 and 0010 are committed. Do not redo any of them.
HEAD is 33da7d1fe.

Baseline: full `make check` = 101 tests, 0 FAIL. That is the acceptance bar.
Second acceptance bar, and the real one:
  python3 doc/claude/scripts/case_differential_sweep.py --jobs 12 --timeout 90
currently reports 142 decks: OK=96, DIFF=41, PARSE-FAIL=2, SKIP=3, and zero
numeric differences. Both PARSE-FAILs (tests/polezero/pz2.cir,
tests/polezero/pzt.cir) are this issue and must go to 0. NUM-DIFF must stay 0.
The 3 SKIPs are stock-run timeouts (tests/mesa/mesa-12.cir,
tests/mesa/mesa12.cir, tests/vbic/FG.cir) and stay. The sweep takes about 8
minutes. Compare it per deck against a baseline you measure yourself, not
against the totals quoted here — one timing-sensitive deck moves between runs.

## The defect

Three helpers in src/frontend/inpcom.c find a whole-token occurrence of a
literal in a card. All three are delimiter-aware and all three hit with strstr:

  src/frontend/inpcom.c:5986   search_identifier()
                       :5989     while ((str = strstr(str, identifier)) != NULL)
  src/frontend/inpcom.c:6012   ya_search_identifier()
                       :6015     while ((str = strstr(str, identifier)) != NULL)
  src/frontend/inpcom.c:6039   search_plain_identifier()
                       :6043     while ((str = strstr(str, identifier)) != NULL)

Almost every call passes a lower-case *language keyword*, not an identifier:
ac, off, thermal, params:, tnodeout, save, print, value, table, pchan, alli,
gnd, mfg, icrating, vceo, type, temper. Those match only because inp_read()
lowercased the card first. Under -D casemode=preserve the fold is gated off and
every one of them stops matching on a deck written in upper case.

Two classes of consequence, both live today.

1. Hard parse failure. inp_check_syntax() rewrites a source card that names ac
   with no value into `ac ( 1 0 )`:

     src/frontend/inpcom.c:9271   if (check_control == 0 && strchr("VvIi", *cut_line)) {
     src/frontend/inpcom.c:9286       acline = search_plain_identifier(acline, "ac");

   With `AC` the fixup never fires, the card reaches INPdevParse(), which
   matches the ac keyword case-insensitively with cieq and then calls
   INPgetValue() on an empty tail:

     stock                -> Note: iin: has no value, DC 0 assumed
     -D casemode=preserve -> Error on line 2 or its substitute:
                               IIN 1 0 AC
                             parameter value out of range or the wrong type

   Note the shape: the device letter at :9271 was thought about — it lists both
   cases — and the keyword at :9286 was not. Same at inpcom.c:7604's
   strchr("*vbiegfhVBIEGFH", ...).

2. Silently wrong terminal counts, with no diagnostic naming the cause. Ten
   calls sit inside get_number_terminals(), the function issue 0009 just taught
   to recognise an upper-case device letter:

     src/frontend/inpcom.c:5401  "thermal"   D, PSpice self-heating
                          :5407  "off"       D
                          :5408  "thermal"   D
                          :5422  "params:"   X
                          :5457  "off"       M
                          :5459  "tnodeout"  M
                          :5460  "thermal"   M
                          :5499  "off"       Q
                          :5503  "save"      Q, #ifdef CIDER
                          :5505  "print"     Q, #ifdef CIDER

   Each terminates a token scan. `D1 1 2 dmod OFF`, `M1 d g s b nmod TNODEOUT`,
   `X1 a b sub PARAMS: w=1u` and `Q1 c b e qmod OFF` therefore count one
   terminal too many under preserve, and a wrong num_terminals feeds
   get_model_name(), which picks the wrong token as the model name. No deck in
   tests/ covers any of these, so the sweep cannot see them; that is an absence
   of coverage, not an absence of defect, and it is the same property that made
   issue 0009 hard to find.

## Scope — verified anchors, all confirmed present at HEAD

Call-site counts, measured at HEAD, definitions included:

  search_plain_identifier   src/frontend/inpcom.c    24
                            src/frontend/inpcompat.c 17
                            src/frontend/inpc_probe.c 2
  search_identifier         src/frontend/inpcom.c    10
                            src/frontend/inp.c        2
  ya_search_identifier      enumerate it yourself; it was not in issue 0013's
                            original scope and is the same defect

The 17 inpcompat.c call sites are not enumerated anywhere in doc/. Enumerate
them and classify each literal as keyword or identifier before you change
anything.

Decide explicitly, and say which:
  - whether the fix belongs inside the three helpers (one edit each, every call
    site inherits it) or at the call sites (explicit per literal, 40-odd
    edits). Weigh it against acceptance criterion 2 of issue 0013, which asks
    for every call site to be reviewed either way, and against what Phase 3
    will need: under `distinguish` a *keyword* is still case-insensitive but an
    *identifier* is not, so any site that passes an identifier must be
    separable then;
  - whether ya_search_identifier() is reachable with an unfolded card at all,
    and if not, whether to fold it anyway for the reason inpsymt.c:31 gives;
  - whether the delimiter tests themselves need anything. They test
    is_arith_char / isspace_c / identifier_char, none of which is case
    sensitive, but say so rather than assuming it.

## The one call site that must not become case-insensitive

  src/frontend/inpcom.c:5637
      search_plain_identifier(deps[j].param_str, param)
  where param is deps[i].param_name

That operand is a user parameter name, not a keyword. It is acceptance
criterion 2 of issue 0013 and it is the only exception found so far. Under
`preserve` folding it is harmless — identity is unchanged in that mode — but
under `distinguish` it would be wrong. Give it either a separate exact-match
entry point now, or route it through the mode dispatch, and state the choice in
the commit message. Check whether the 17 inpcompat.c sites and the 2
inpc_probe.c sites contain any further exceptions of the same kind;
inpc_probe.c handles .probe cards, which carry user instance names.

## Conventions

- Use cistrstr() rather than strcasestr(): configure.ac does not check for
  strcasestr and there is no compat implementation. cistrstr() is already in
  src/misc/string.c and is the convention named in AGENTS.md. For exact
  identity use ng_ideq() (src/frontend/inpcom.c, declared in fteext.h), which
  issue 0010 added and which is gated on inp_case_folding().
- Gate every fold on inp_case_folding(), the way src/spicelib/parser/inpsymt.c:31
  and the issue-0010 sites do, so the default mode is provably the same
  predicate on the same bytes. Do not spell it as a bare cistrstr.
- Match the style of the file being edited, four-space indent, no reflowing, no
  whitespace churn. inpcom.c churns constantly upstream, so keep the diff to
  single-token edits where possible.
- Anything touching global or static simulator state must survive repeated
  ngSpice_Reset in the shared build.
- Commit messages: short imperative summary, then problem, mechanism, and the
  RED evidence. State whether the change is fold-mode-visible.

## The fold-mode question, which is harder here than it was for 0009 or 0010

Issue 0010's Resolution concluded that no fold-mode guard was possible for the
name lookups, and gave a per-exemption argument plus the second fold pass at
src/frontend/inp.c:817. Do not carry that conclusion over. Acceptance criterion
5 of issue 0013 says explicitly that this change is *not* a guaranteed no-op on
the lines the reader exempts, and names why: search_plain_identifier is called
on .subckt cards and, through inpc_probe.c, on .probe cards. Work the exemption
list again for these specific call sites:

  the unconditional .lib/.inc exemption near src/frontend/inpcom.c:1898
  the 13-command .control whitelist at :1899-1910
  the print/eprint/eprvcd/asciiplot redirection tail at :1866-1880
  the plot/gnuplot/hardcopy arm
  keep_case_of_cider_param() at :223 and is_xspice_model() at :416

If any of them can deliver an unfolded card to one of these helpers, the change
IS fold-mode visible and needs a fold-mode regression deck under
tests/regression/misc/ and a paragraph in the commit message. If none can, say
so with the argument, not with a reference to issue 0010's argument.

## Method — RED-first per AGENTS.md, non-negotiable

1. write or extend the test, 2. run it and show the failure is for the expected
reason, 3. smallest production change, 4. re-run the targeted test then the
directory's make check, 5. full make check before the commit. Never weaken an
assertion or regenerate a .out to hide a failure. If a site genuinely cannot be
given a failing test first, say why before implementing it.

RED tests: new decks in tests/regression/case/, which already runs every deck
under -D casemode=preserve. Follow the twin-pair pattern of
tests/regression/case/device-letter-case.cir and its -lower twin: an upper/lower
pair with byte-identical .out files, so the assertion is "the spelling cannot
change the number" rather than "this run printed this". Issue 0013 asks for
five decks:

  criterion 3   IIN 1 0 AC with no value, against its `ac` twin
  criterion 4   D ... OFF, M ... TNODEOUT, X ... PARAMS:, Q ... OFF, each
                resolving its model and matching a lower-case twin

Split them per mechanism if a single deck aborts on the first error before
reaching the later evidence — that is what happened on issues 0009 and 0010,
and per-mechanism decks made the RED evidence usable. The criterion-4 decks are
the valuable ones: their failure is a wrong model-name token rather than a
diagnostic, and check.sh cannot see diagnostics.

Before writing the fix, confirm the RED for the stated reason: run an uppercased
copy of tests/polezero/pz2.cir under -D casemode=preserve and check that the
diagnostic traces to the missing `ac` fixup at inpcom.c:9286 and not to
something downstream in INP2I. The sweep attributes it to 0013; verify that
rather than inheriting it.

## Test harness hazards — most obvious tests are invalid without these

- tests/bin/check.sh:20 filters BOTH actual and expected through egrep -v
  "SPARSE|KLU|CPU|Dynamic|Note|Circuit|Trying|Reference|Date|Doing|---|v-sweep|
  time|est|Error|Warning|Data|Index|trans|acan|oise|nalysis|ole|Total|memory|
  urrent|Got|Added|BSIM|bsim|B4SOI|b4soi|codemodel|^binary raw file|
  ^ngspice.*done|Operating"
  A test whose only evidence is a diagnostic passes whether or not the bug
  exists. "est" eats the word "test", "urrent" eats any line containing
  "current", "Data" eats "No. of Data Rows", "Reference" eats the transient
  "Reference value" line. The filter is case-sensitive, so UPPERCASE sentinels
  survive. Note that "acan" and "Note" between them eat the very diagnostic
  this issue produces.
- Start every deck with .OPTIONS noacct — the rusage block is not filtered and
  is machine-dependent.
- Use numeric node names for anything you print. Under preserve a node name
  keeps its case in the vector name, so `print v(Out)` and `print v(out)`
  produce different text and the twin decks stop having identical .out files.
- A .cir absent from its directory's TESTS is silently never run, and a RED
  test that never ran looks exactly like a passing one.
- tests/.gitignore ignores *.out, so every expected-output file needs
  `git add -f`.
- Tests that write files need a CLEANFILES entry in the local Makefile.am.
- tests/regression/case/ and tests/regression/pipe/ use tests/bin/spinit, which
  loads no code models, so nothing XSPICE can be tested there; a `quit 0` skip
  is indistinguishable from a pass.
- The Q-device "save"/"print" calls at inpcom.c:5503 and :5505 are inside
  #ifdef CIDER, and CIDER is off in build-ver_50
  (build-ver_50/src/include/ngspice/config.h). Do not claim coverage of them
  from a build that cannot compile them.

## Build

  make -C build-ver_50 -j14
  make -C build-ver_50/tests/regression/case check TESTS=<name>.cir
  make -C build-ver_50 check                       # full suite, currently 101
After any Makefile.am or configure.ac edit: ./autogen.sh, then
  (cd build-ver_50 && ./config.status tests/regression/case/Makefile)
Existing out-of-tree tree is build-ver_50 (bare ../configure). XSPICE and OSDI
on, CIDER off, WITH_PSS off, RFSPICE on — check
build-ver_50/src/include/ngspice/config.h before claiming a code path is
reachable.

Hazard: never pipe make through `head` or any other command that can close the
pipe early. SIGPIPE kills make mid-build and leaves a stale binary, and the
next test run reports a failure that has nothing to do with your change. Send
build output to a file and grep the file.

Hazard: never interrupt ./autogen.sh or ./config.status. A killed run leaves
half-regenerated build files and the next build fails in src/xspice/verilog
with "relocation R_X86_64_PC32 against symbol stderr ... recompile with -fPIC".
That is not a real defect; re-run both to completion and it goes away.

Hazard: do not rebuild while the differential sweep is running. It swaps the
binary underneath the sweep and the result is meaningless.

## Deliberately out of scope — enumerate, do not fix

- Issue 0009's follow-up table: 30 first-character device-letter sites still
  unconverted in src/frontend/inpcom.c. Line numbers there are against
  e96b4cd5c and are +29 at HEAD. One of them, inpcom.c:3235
  ((*st == 'f') || (*st == 'h')) in is_a_modelname(), is what makes
  tests/bsim3soi{dd,fd,pd}/ring51.cir report DIFF rather than OK for their
  `cout buf ss 1pF` card. It is a warning, not a number. Leave it.
- Issue 0011 (gnd rewrite corrupts case-preserved command text). Its acceptance
  criterion 4 depends on this issue, so note the unblock but do not fix it.
- Issue 0014 (numnodes() dispatches on a lower-case device letter),
  issue 0015 (numparam .param/.func symbol names are case-sensitive) and
  issue 0016 (show/showmod and friends match instance names outside
  DEVnameHash).
- Everything gated behind `distinguish`: src/spicelib/parser/inpptree.c:1256
  mkvnode create-on-miss, src/frontend/vectors.c:61/71/184 vector-table
  aliasing, src/xspice/evt/evtcheck_nodes.c:720 and evttermi.c:304. If your
  change makes an XSPICE deck behave differently, stop and write it up rather
  than following it.

Anything new you find that you do not fix goes in doc/codex/issues/NNNN-slug.md
with the Status / Summary / Impact / Root Cause / Acceptance Criteria /
Resolution structure. Next free number is 0017.

## Doc-citation repair, in the same series

Several citations in the ground-truth documents no longer match the code,
because Phase 1 and Phase 2 converted the sites without updating them. Fix
these in a docs commit at the end, and check the rest while you are there:

  specs/case-sensitive-identifiers.md:153   vec_basename strtolower at
      vectors.c:1129 — removed by 47c52c7dd; :1129 is now the comment saying so
  specs/...:158    print consumer is postcoms.c:207, not :203
  specs/...:164-167 "print v(OutB) still prints v(outb)" — no longer true
  specs/...:216    subckt.c:636,652,669,685 strstr(" wmin=") — now cistrstr at
      :659, :675, :692, :708
  specs/...:218    inp2dot.c .SENS/.TF lowercase-only v/i — now cieq at
      :366, :387, :489, :512
  specs/...:215    measure.c:236 strtolower feeding strcmp — now gated at :236
      with cieq consumers at :351 and :446
  suggestions/case-sensitive-identifiers-plan.md §2.5 — deliverable closed by
      47c52c7dd, still written in the imperative
  suggestions/...:284 "assert on values only, unless 2.5 has landed" — it has

Do not rewrite the documents wholesale. Mark what is done, correct what is
wrong, leave the reasoning intact.

## Stop and report

What changed per file, RED evidence per behavioural change, the before/after
sweep verdict counts compared per deck, which of issue 0013's six acceptance
criteria are met and which are not and why, whether the change is fold-mode
visible and the argument either way, anything deliberately left undone, and the
full make check result.

---

## Why this step, and what follows it

Chosen over the alternatives because it is the only remaining item with a
measured, reproducible failure at HEAD — the sweep's last two PARSE-FAILs — and
because it is the smallest of the critical-path items: three helper lines
against issue 0009's 30 remaining sites or the frontend vector table's hot
lookup path. It also unblocks more than its size suggests: it closes issue
0011's acceptance criterion 4, retires the two `unclear` rows of the Phase 1
census (which are precisely these helpers), and makes the 17 unenumerated
inpcompat.c call sites correct as a side effect.

If the goal is upstream credibility rather than the feature, do
doc/codex/issues/0005 instead — a 0-or-4-quote `.model` card is lowercased in
full, quoted paths included, in the **default** mode, so it merges on its own
merits.

After this, in order: issue 0009's 30 follow-up sites, then 0015 (numparam),
then the four `distinguish` gates (inpptree.c mkvnode, vectors.c aliasing,
evtcheck_nodes.c auto-bridge, evttermi.c event nodes), then a build-enforced
lint so the sweep does not decay, then Phase 3 itself and its five RED decks —
none of which exist yet, and one of which needs its own spinit.
