/* silent.c -- fixture for the identifier-comparison lint.
 *
 * No comparison in this file may be reported, and none of them is in
 * tests/lint/selftest.baseline.  A scanner that starts reporting one of these
 * fails the selftest with an "unclassified comparison" line, which is the RED
 * for the two things that keep the lint quiet enough to be worth running: the
 * string-literal exemption, and the `case-lint:` escape hatch.
 *
 * This file is never compiled.
 */

/* --- the literal exemption ------------------------------------------------
 * A comparison against a string literal is asking whether a word is the one
 * the language defines, and keywords are case-insensitive in all three case
 * modes by the spec's compatibility contract point 2.  These are the 1267 of
 * the tree's 1541 comparison call sites that the lint never looks at.
 */

int keyword(const char *word)
{
    return eq(word, "tran") || cieq(word, "ac") || strcmp(word, "op") == 0;
}

int keyword_first(const char *word)
{
    return strcasecmp("dc", word) == 0;
}

int keyword_wrapped(const char *word)
{
    return cieq(word,
                "distinguish");
}

/* a parenthesis inside the literal must not end the argument list early */
int keyword_with_paren(const char *word)
{
    return eq(word, "v(") || eq(word, ")");
}

/* an escaped quote inside the literal must not end the literal early */
int keyword_with_quote(const char *word)
{
    return eq(word, "say \"tran\"");
}

/* An accepted false negative, pinned here so it is a decision and not a
 * surprise: the exemption tests the whole argument list for a quote, so a
 * literal buried in a nested call exempts the outer comparison even though
 * neither of its operands is a literal.  There is no call of this shape in
 * src/ today -- every exempted call there has a literal as a top-level
 * operand -- and tightening it would mean parsing the argument list rather
 * than scanning it. */
int nested_literal(const char *a, const char *b)
{
    return strcmp(a, getenv("NGSPICE_B") ? a : b) == 0;
}

/* --- the escape hatch -----------------------------------------------------
 * A comparison of two runtime strings that is not an identity test is
 * classified on the line, in the diff, where a reviewer sees it.  This is the
 * documented way to add one; see AGENTS.md.
 */

int annotated_same_line(const char *a, const char *b)
{
    return strcmp(a, b) == 0;   /* case-lint: keyword - both are command names */
}

int annotated_line_above(const char *name, const char **table, int i)
{
    /* case-lint: keyword - the table holds analysis names, not deck names */
    return cieq(name, table[i]);
}

int annotated_wrapped(const char *a, const char *b)
{
    /* case-lint: neither - an ordering for output, not an identity test */
    return strcmp(a,
                  b);
}

/* --- text that only looks like a call -------------------------------------
 * A comparator named inside a comment or a string is not a call.
 */

/* this comment mentions strcmp(a, b) and cieq(x, y) and must be ignored */

const char *doc = "use strcmp(a, b) here";

/* a comparator name that is part of a longer identifier is not a comparator */
int longer_identifier(const char *a, const char *b)
{
    return my_strcmp(a, b) + strcmp_wrapper(a, b) + xeq(a, b);
}
