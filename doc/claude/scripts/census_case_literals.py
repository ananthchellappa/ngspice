#!/usr/bin/env python3
"""Mechanical census of case-sensitive comparisons against lowercase literals.

Phase 1 of doc/claude/suggestions/case-sensitive-identifiers-plan.md requires a
scripted enumeration before any conversion, because three prior estimates of the
surface disagreed by an order of magnitude (642, ~304, ~120).

The script scans C/C++ translation units under a set of roots for calls to the
case-sensitive comparison primitives

    strcmp  strncmp  strstr  strcasestr  eq  eqn  prefix

where at least one argument is a string literal that contains no uppercase
ASCII letter.  Such a call can only ever match text that has already been
case-folded, so it is a candidate for conversion to a case-insensitive
comparison (cieq / cieqn / ciprefix / a delimiter-guarded scan).

A second pass reports every call to the same primitives that has *no* string
literal operand at all.  Those cannot be found by a literal scan, yet several
of the plan's known members live there: the comparison is against a table field
that the build generates in lowercase (`IFparm.keyword`,
`Mif_Conn_Info_t.allowed_type_str[]`, `Evt_Udn_Info_t.name`).  Pass B is
reported in full and classified by hand rather than guessed at, so that the
census stays mechanical.

Output is a TSV on stdout:

    pass <TAB> file <TAB> line <TAB> func <TAB> callee <TAB> literal <TAB> snippet

Comments and string/char literals are handled by a small lexer so that a
`strcmp` inside a comment or the text "strcmp(" inside a literal is not
counted.  Calls spanning several physical lines are reassembled; the reported
line is the line the callee name appears on.

Usage:
    python3 doc/claude/scripts/census_case_literals.py \
        src/frontend src/spicelib/parser src/xspice
"""

import os
import re
import sys

# Case-sensitive comparison primitives.  `eq`/`eqn` are the macros in
# src/include/ngspice/macros.h; `prefix` is numparam's own (spicenum.c).
CALLEES = ("strcmp", "strncmp", "strstr", "strcasestr", "eq", "eqn", "prefix")

SOURCE_EXT = (".c", ".h", ".cpp", ".hpp", ".cc")

IDENT_CHAR = re.compile(r"[A-Za-z0-9_]")


def strip_comments(text):
    """Blank out comments, keeping every byte offset and every newline.

    String and character literals are left intact but are marked in `mask` so
    that a later scan can tell code from literal.  Returns (code, mask) where
    mask[i] is 'S' inside a string literal, 'C' inside a comment, ' ' in code.
    """
    out = list(text)
    mask = [" "] * len(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            while i < n and text[i] != "\n":
                mask[i] = "C"
                out[i] = " "
                i += 1
        elif c == "/" and i + 1 < n and text[i + 1] == "*":
            mask[i] = mask[i + 1] = "C"
            out[i] = out[i + 1] = " "
            i += 2
            while i < n and not (text[i] == "*" and i + 1 < n and text[i + 1] == "/"):
                mask[i] = "C"
                if text[i] != "\n":
                    out[i] = " "
                i += 1
            if i < n:
                mask[i] = mask[i + 1] = "C"
                out[i] = out[i + 1] = " "
                i += 2
        elif c in ('"', "'"):
            quote = c
            mask[i] = "S"
            i += 1
            while i < n:
                mask[i] = "S"
                if text[i] == "\\":
                    if i + 1 < n:
                        mask[i + 1] = "S"
                    i += 2
                    continue
                if text[i] == quote:
                    i += 1
                    break
                if text[i] == "\n":  # unterminated, bail out of the literal
                    break
                i += 1
        else:
            i += 1
    return "".join(out), "".join(mask)


def split_args(text, start, end, mask):
    """Split the top-level comma-separated arguments of text[start:end]."""
    args, depth, cur = [], 0, start
    for i in range(start, end):
        if mask[i] == "S":
            continue
        c = text[i]
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        elif c == "," and depth == 0:
            args.append((cur, i))
            cur = i + 1
    args.append((cur, end))
    return args


def literals_in(text, mask, start, end):
    """Return the concatenated string literals inside text[start:end]."""
    lits, i = [], start
    while i < end:
        if text[i] == '"' and mask[i] == "S":
            j = i + 1
            buf = []
            while j < end and mask[j] == "S":
                if text[j] == "\\" and j + 1 < end:
                    buf.append(text[j : j + 2])
                    j += 2
                    continue
                if text[j] == '"':
                    j += 1
                    break
                buf.append(text[j])
                j += 1
            lits.append("".join(buf))
            i = j
            continue
        i += 1
    return lits


def is_lowercase_literal(lit):
    """A literal that can only match already-folded text.

    Requires at least one ASCII letter and no uppercase ASCII letter.  Escapes
    such as \\n carry no case, so they neither qualify nor disqualify.
    """
    body = re.sub(r"\\.", "", lit)
    if not any("a" <= ch <= "z" for ch in body):
        return False
    return not any("A" <= ch <= "Z" for ch in body)


def enclosing_function(text, pos):
    """Best-effort name of the function containing offset pos."""
    best = "?"
    for m in re.finditer(
        r"^[A-Za-z_][A-Za-z0-9_ \t\*]*?([A-Za-z_][A-Za-z0-9_]*)[ \t]*\(", text[:pos], re.M
    ):
        best = m.group(1)
    return best


def scan_file(path, want_literal=True):
    """Yield hits in `path`.

    want_literal=True  -> pass A: at least one lowercase string literal operand
    want_literal=False -> pass B: no string literal operand anywhere in the call
    """
    with open(path, "r", errors="replace") as fh:
        raw = fh.read()
    code, mask = strip_comments(raw)
    hits = []
    for m in re.finditer(r"\b(" + "|".join(CALLEES) + r")\b[ \t\n]*\(", code):
        name = m.group(1)
        if mask[m.start()] != " ":
            continue
        # not a definition/declaration of the primitive itself
        open_paren = m.end() - 1
        depth, i, n = 1, open_paren + 1, len(code)
        while i < n and depth:
            if mask[i] != "S":
                if code[i] == "(":
                    depth += 1
                elif code[i] == ")":
                    depth -= 1
            i += 1
        if depth:
            continue
        close = i - 1
        args = split_args(code, open_paren + 1, close, mask)
        if name in ("strcmp", "strstr", "strcasestr", "eq", "prefix") and len(args) != 2:
            continue
        if name in ("strncmp", "eqn") and len(args) != 3:
            continue
        all_lits, lits = [], []
        for a, b in args:
            for lit in literals_in(raw, mask, a, b):
                all_lits.append(lit)
                if is_lowercase_literal(lit):
                    lits.append(lit)
        if want_literal:
            if not lits:
                continue
        else:
            if all_lits:
                continue
        line = code.count("\n", 0, m.start()) + 1
        snippet = " ".join(raw[m.start() : close + 1].split())
        if len(snippet) > 150:
            snippet = snippet[:147] + "..."
        hits.append((line, name, "|".join(lits), snippet, enclosing_function(code, m.start())))
    return hits


def main(roots):
    rows = []
    for want, tag in ((True, "A"), (False, "B")):
        for root in roots:
            for dirpath, _dirnames, filenames in os.walk(root):
                for fn in sorted(filenames):
                    if not fn.endswith(SOURCE_EXT):
                        continue
                    path = os.path.join(dirpath, fn)
                    for line, name, lits, snippet, func in scan_file(path, want):
                        rows.append((tag, path, line, func, name, lits, snippet))
    rows.sort(key=lambda r: (r[0], r[1], r[2]))
    print("pass\tfile\tline\tfunc\tcallee\tliteral\tsnippet")
    for r in rows:
        print("%s\t%s\t%d\t%s\t%s\t%s\t%s" % r)

    def note(fmt, *a):
        print(("# " + fmt) % a, file=sys.stderr)

    for tag in ("A", "B"):
        sel = [r for r in rows if r[0] == tag]
        note("pass %s total\t%d", tag, len(sel))
        for root in roots:
            note("pass %s %s\t%d", tag, root, sum(1 for r in sel if r[1].startswith(root)))
        by_callee = {}
        for r in sel:
            by_callee[r[4]] = by_callee.get(r[4], 0) + 1
        for k in sorted(by_callee):
            note("pass %s callee %s\t%d", tag, k, by_callee[k])
    note("grand total\t%d", len(rows))


if __name__ == "__main__":
    main(sys.argv[1:] or ["src/frontend", "src/spicelib/parser", "src/xspice"])
