/* flagged.c -- fixture for the identifier-comparison lint.
 *
 * Every comparison in this file must be reported, and every one of them is
 * in tests/lint/selftest.baseline.  A scanner that stops seeing one of these
 * fails the selftest with a "no longer in the tree" line, which is the RED
 * for the detector itself: doc/codex/issues/0027, 0032 and 0037 are all this
 * shape, a raw comparator between two runtime strings.
 *
 * This file is never compiled.  It is C only so that the scanner's comment
 * and string-literal handling is exercised on the real thing.
 */

struct dvec {
    char *v_name;
};

/* the plain shape: the defect the lint exists to stop */
int plain(const char *name, struct dvec *v)
{
    return strcmp(v->v_name, name) == 0;
}

/* the eq() and eqc() macros over strcmp() and cieq() */
int through_macros(const char *a, const char *b)
{
    return eq(a, b) || eqc(a, b);
}

/* wrapped across lines: the argument list is joined before it is judged */
int wrapped(const char *query, struct dvec *v)
{
    return cieq(v->v_name,
                query);
}

/* a comment inside the argument list does not change the key */
int commented(const char *a, const char *b)
{
    return strcasecmp(a /* the stored spelling */, b);
}

/* a literal elsewhere on the same line does not exempt the comparison: the
 * exemption is per call, not per line */
int literal_nearby(const char *a, const char *b)
{
    return printf("comparing %s\n", a) && strcmp(a, b) == 0;
}

/* the length-taking forms, which decision 3 of 0001 has Class A sites for */
int lengths(const char *a, const char *b, size_t n)
{
    return strncmp(a, b, n) == 0 && cieqn(a, b, n);
}

/* the substring forms */
char *substrings(const char *hay, const char *needle)
{
    return cistrstr(hay, needle) ? strstr(hay, needle) : 0;
}

/* a marker that names a different tool is not this tool's marker */
int wrong_marker(const char *a, const char *b)
{
    return strcmp(a, b) == 0;   /* cppcheck-suppress something */
}
