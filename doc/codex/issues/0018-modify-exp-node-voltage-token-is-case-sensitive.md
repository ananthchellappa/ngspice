# Issue: `inp_modify_exp()` Recognises a Node Voltage Only in Lower Case

## Status

Fixed in `48b72c7b2`, in the commit that folds the other two lower-case-only
character scan sets. Written up here because it is **not** a row of
`doc/codex/issues/0009`'s follow-up table and the next sweep over
`src/frontend/inpcom.c` should not have to rediscover it.

## Summary

`inp_modify_exp()` is the tokenizer every behavioural expression goes
through. `inp_bsource_compat()` hands it the right-hand side of every B
card's `=` (`inpcom.c:7623`) and `inp_temper_compat()` hands it every
expression that mentions `temper` (`inpcom.c:7698`). It walks the expression
one token at a time and wraps anything it does not recognise in numparam
braces, so that numparam resolves `.param` names and leaves everything else
alone.

A node voltage is the one token that must not be braced, and it was
recognised on a lower-case letter:

```c
/* src/frontend/inpcom.c, inp_modify_exp() */
            if (((c == 'v') || (c == 'i')) && (s[1] == '(')) {
                while (*s != ')') {
                    buf[i++] = *s++;
                }
```

`c` is `*s`. Every other test in the same `else` branch is already
case-insensitive — `cieq(buf, "hertz")`, `cieq(buf, "temper")`,
`cieq(buf, "time")`, `cieq(buf, "pi")`, `cieq(buf, "pwl")`,
`cieq(buf, "tc1")` and the rest — so this is the one byte in the function
that was correct only because `inp_read()` had folded the card.

## Impact

Under the default `fold` mode the card is lower case by the time it gets
here, so nothing is reachable.

Under `casemode=preserve` an upper-case `V(` or `I(` — the ordinary SPICE
house style — falls into the generic identifier branch. That branch copies
the alphanumeric run, sees the `(` that follows and emits the bare token, so
`v = V(1)*2` is handed to numparam as a `V` with a parenthesised group after
it rather than as a node voltage. The operating point then collapses:

```spice
.OPTIONS noacct
v1 1 0 dc 2
b1 2 0 v = V(1)*2
r1 2 0 1k
.control
op
print v(2)
.endc
.end
```

```
$ ngspice --batch b.cir                        -> v(2) = 4.000000e+00
$ ngspice -D casemode=preserve --batch b.cir   -> Warning: singular matrix:
                                                    check node 1.0000000000e+00
                                                  ... nothing printed
```

The device letter is lower case in that deck, so this is not
`doc/codex/issues/0009`'s first-character class: it reproduces on a card
already spelled the way the preprocessor expects.

Its reach is wider than one card shape. Because `inp_compat()`'s R, C and L
arms rewrite a behavioural passive into a generated B source that carries
the user's equation verbatim, an upper-case `V(` inside `R={1k*V(3)}` fails
here even after `b_transformation_wanted()` (`inpcom.c:5975`) has been taught
to see it — which is what `tests/regression/case/behav-r-vnode-case.cir`
measures.

## Root Cause

The same one as `doc/codex/issues/0009` and `doc/codex/issues/0013`: a
character literal that the reader's fold made correct by accident. It is
invisible to both of those issues' enumerations for the same reason
`search_plain_identifier()` was invisible to the Phase 1 census — the Phase 1
census does not count character-literal comparisons, `doc/codex/issues/0009`
scoped itself to a card's *first* character, and `doc/codex/issues/0013`
scoped itself to keyword searches behind a helper. This is a character
comparison in the middle of a token.

## Acceptance Criteria

1. `inp_modify_exp()` recognises `V(` and `I(` in either case, folding the
   byte it tests and copying the token unchanged.
2. A deck in `tests/regression/case/` with an upper-case `V(` inside a
   lower-case B source's expression produces the same voltage as its
   lower-case twin.
3. `make check` unchanged with `casemode` unset.
4. `doc/codex/issues/0009`'s follow-up table gains a row for it, so the site
   is recorded where the rest of its class is.

## Resolution

Fixed in `48b72c7b2`:

```c
            if (((tolower_c(c) == 'v') || (tolower_c(c) == 'i')) &&
                    (s[1] == '(')) {
```

`tolower_c()` rather than `elem_letter()`: the byte is a character out of the
middle of an expression, not a card's leading device letter, and
`elem_letter()` would be a lie about what is being read.

### Acceptance criteria

1. **Met.**
2. **Met.** `tests/regression/case/bsource-vnode-case.cir` and its twin, both
   printing `v(2) = 4.000000e+00`. The upper deck was confirmed to FAIL
   against an empty reference, so its evidence is the number.
   `tests/regression/case/behav-r-vnode-case.cir` covers the same site
   reached through `inp_compat()`'s R arm, and needed `inpcom.c:5975` folded
   as well — which was measured, not assumed: with 5975 folded alone that
   deck still printed nothing.
3. **Met.** 152 tests, 0 FAIL with `casemode` unset at that commit.
4. **Met.** Recorded in `doc/codex/issues/0009`'s "One site fixed that this
   table did not list".

### Not fold-mode visible

`inp_bsource_compat()` and `inp_temper_compat()` both run inside
`inp_readall()`'s `if (!comfile && cc)` block and both skip `.control`
bodies. The reader's exemptions that keep unfolded text on a live card — the
`plot`/`gnuplot` arm's token after `title`/`xlabel`/`ylabel` and the print
arm's redirection tail — spare text *after* a keyword or after the first
`>`, and neither pass is reached with such a card carrying a `v(`/`i(` token
in an expression position: what reaches `inp_modify_exp()` is the text after
a B card's `=`, or an expression containing `temper` on a card whose device
letter is not in `strchr("*vbiegfhVBIEGFH", curr_line[0])`.

### What this does not reach

`src/spicelib/parser/inpptree.c` folds its own function names with
`strtolower()` (`:1161`, `:1335`), so the expression *parser* was never the
problem. The remaining case-sensitivity in that name space is
`doc/codex/issues/0015` (numparam symbols) and the `distinguish`-gated
`mkvnode()` create-on-miss at `inpptree.c:1256`, neither of which this
touches.
