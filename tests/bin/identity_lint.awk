# identity_lint.awk -- the build-enforced identifier-comparison lint.
#
# doc/claude/decisions/0001-distinguish.md decision 3 sorts every comparison
# in the tree into two questions: "are these two names the same?", which must
# go through one of the three identity helpers, and "is this word the one the
# language defines?", which must keep comparing bytes.  A new raw strcmp() or
# cieq() on a vector, node or model name is the defect this file exists to
# stop, and it is the shape of doc/codex/issues/0027, 0032 and 0037.
#
# No regular expression can tell the two questions apart, so this does not
# try.  It reports every call to a raw comparator, inside the guarded paths,
# whose operands are BOTH runtime expressions -- a comparison against a string
# literal is asking about a keyword and is never reported.  What remains is a
# ratchet: the sites present when the lint landed are in the baseline, and
# only a site that is not there fails the build.
#
# Invoked by tests/bin/identity_lint.sh; see tests/lint/identity.lint for the
# scope and AGENTS.md for how to clear a report.
#
# What it does not see, stated so nobody reads a pass as a proof:
#
#   - a comparison reached other than by naming a comparator, which is how
#     doc/codex/issues/0037 got in: nghash's own strcmp, behind a table.
#   - a call whose '(' is on the line after the comparator's name.  There is
#     no such call in the tree today; the regex requires them on one line.
#   - a call to a comparator through a function pointer or through a macro
#     that is not itself in the comparator list.
#   - doc/codex/issues/0033, which passes the wrong string to a function
#     rather than comparing two.  See doc/claude/decisions/0011 decision 1.
#   - a comparison whose operands are both runtime but whose argument list
#     contains a literal inside a nested call, because the literal exemption
#     scans the whole list rather than parsing it.  No call in src/ has that
#     shape; tests/lint/selftest/silent.c pins it as a decision.
#
# Variables set with -v:
#   comparators  space-separated comparator names
#   marker       the escape-hatch comment token
#
# Output: one record per unannotated two-runtime-operand call,
#   <path><TAB><line><TAB><normalized call text>
# The line number is for the reader; the driver keys the baseline on the path
# and the call text only, so an unrelated edit above a site does not
# invalidate its baseline entry.

function blanks(n,   s) {
    s = ""
    while (n-- > 0)
        s = s " "
    return s
}

# Index of the closing quote of a literal whose opening quote is at from - 1,
# or 0 if the line ends first.
function scan_literal(s, from, q,   n, i, c) {
    n = length(s)
    i = from
    while (i <= n) {
        c = substr(s, i, 1)
        if (c == "\\") {
            i += 2
            continue
        }
        if (c == q)
            return i
        i++
    }
    return 0
}

# Blank out comment bodies and string/char literal interiors, preserving
# length so the masked text and the raw text share offsets.  The quotes
# themselves are kept, so "does this argument hold a literal?" is a test for a
# quote character.
function mask_line(s,   out, n, i, rest, p, len, tok, j) {
    out = ""
    n = length(s)
    i = 1
    while (i <= n) {
        rest = substr(s, i)
        if (in_comment) {
            p = index(rest, "*/")
            if (p == 0) {
                out = out blanks(length(rest))
                i = n + 1
            } else {
                out = out blanks(p + 1)
                i += p + 1
                in_comment = 0
            }
            continue
        }
        if (in_string) {
            j = scan_literal(rest, 1, "\"")
            if (j == 0) {
                out = out blanks(length(rest))
                i = n + 1
            } else {
                out = out blanks(j - 1) "\""
                i += j
                in_string = 0
            }
            continue
        }
        if (!match(rest, /\/\*|\/\/|"|'/)) {
            out = out rest
            break
        }
        p = RSTART
        len = RLENGTH
        out = out substr(rest, 1, p - 1)
        i += p - 1
        tok = substr(rest, p, len)
        if (tok == "/*") {
            in_comment = 1
            out = out "  "
            i += 2
        } else if (tok == "//") {
            out = out blanks(length(rest) - p + 1)
            i = n + 1
        } else {
            j = scan_literal(rest, p + 1, tok)
            if (j == 0) {
                # a literal continued onto the next line with a backslash
                out = out tok blanks(length(rest) - p)
                in_string = (tok == "\"")
                i = n + 1
            } else {
                out = out tok blanks(j - p - 1) tok
                i += (j - p) + 1
            }
        }
    }
    return out
}

BEGIN {
    ncmp = split(comparators, cmp, " ")
    alt = ""
    for (i = 1; i <= ncmp; i++) {
        is_cmp[cmp[i]] = 1
        alt = alt (alt == "" ? "" : "|") cmp[i]
    }
    callre = "(^|[^A-Za-z0-9_])(" alt ")[ \t]*\\("
}

FNR == 1 {
    if (nlines > 0)
        scan_file()
    in_comment = 0
    in_string = 0
    nlines = 0
    file = FILENAME
}

{
    nlines++
    raw[nlines] = $0
    masked[nlines] = mask_line($0)
    # the escape hatch is a comment, so it survives masking only in the raw
    annotated[nlines] = (index($0, marker) > 0)
}

END {
    if (nlines > 0)
        scan_file()
}

function scan_file(   i, s, off, k, paren, name) {
    for (i = 1; i <= nlines; i++) {
        off = 0
        s = masked[i]
        while (match(s, callre)) {
            # the last character of the match is the '(' that opens the call
            paren = off + RSTART + RLENGTH - 1
            k = off + RSTART
            if (substr(masked[i], k, 1) !~ /[A-Za-z_]/)
                k++
            name = substr(masked[i], k, paren - k)
            sub(/[ \t]*$/, "", name)
            if (name in is_cmp)
                report(i, paren, name)
            off = paren
            s = substr(masked[i], off + 1)
        }
    }
}

# col is the 1-based index, within masked[ln], of the '(' that opens the call.
# The normalized text is taken from the masked lines, so a comment or a line
# break inside the argument list cannot change the baseline key.
function report(ln, col, name,   depth, l, c, line, endln, args, norm) {
    depth = 0
    l = ln
    c = col
    while (l <= nlines) {
        line = masked[l]
        while (c <= length(line)) {
            if (substr(line, c, 1) == "(")
                depth++
            else if (substr(line, c, 1) == ")") {
                depth--
                if (depth == 0)
                    break
            }
            c++
        }
        if (depth == 0 && c <= length(line))
            break
        l++
        c = 1
    }
    if (depth != 0 || l > nlines)
        return
    endln = l
    if (endln == ln) {
        args = substr(masked[ln], col + 1, c - col - 1)
    } else {
        args = substr(masked[ln], col + 1) "\n"
        for (l = ln + 1; l < endln; l++)
            args = args masked[l] "\n"
        args = args substr(masked[endln], 1, c - 1)
    }
    # a comparison against a string literal is a keyword test, never a name
    if (index(args, "\"") > 0)
        return
    if (is_annotated(ln, endln))
        return
    gsub(/[ \t\n]+/, " ", args)
    sub(/^ +/, "", args)
    sub(/ +$/, "", args)
    norm = name "(" args ")"
    printf "%s\t%d\t%s\n", file, ln, norm
}

function is_annotated(a, b,   i) {
    if (a > 1 && annotated[a - 1])
        return 1
    for (i = a; i <= b; i++)
        if (annotated[i])
            return 1
    return 0
}
