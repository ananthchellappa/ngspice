/* ngspice file inpc_stb.c

   Terminal probes of the loop-gain (stability) analysis "stb".

   The probe of the analysis is a 0 V voltage source in the loop. A probe
   written <instance>:<terminal> names a terminal of an element, or a port
   of a subcircuit instance, instead: the source is then made here, while
   the netlist is read. The terminal is taken off its net and put on a new
   node x, and the source is placed between x (its positive node) and the
   net y.

   The probes are collected from the .stb cards and from the stb lines of
   the .control section in which the probe is written out. A port is
   handled when its instance line is expanded (subckt.c), an element in
   the flat deck. The collected probes are kept with the circuit, where
   the analysis looks them up, and those of the .control section also
   with the stored deck, so that 'reset' inserts them again.

   Adding sources to a deck that is being read is what .probe does as
   well, see inpc_probe.c.

   The names follow the case mode of the netlist (casemode=fold, preserve
   or distinguish). An instance, a port of a subcircuit, a node and a
   model are names the deck wrote: they are compared with ng_ideq() and
   the helpers below, so that X1 and x1 are one instance under fold and
   preserve and two under distinguish, and the names made here are made
   from the spelling of the deck. A word of a command is folded to lower
   case only under fold, where the reader has folded the deck. What the
   language defines is taken in any case in every mode: .stb, stb, the
   letter of a device, the letter of a terminal, v( on an .ic card.
 */

#include "ngspice/ngspice.h"
#include "ngspice/cpextern.h"
#include "ngspice/ftedefs.h"
#include "ngspice/fteext.h"
#include "ngspice/inpdefs.h"
#include "ngspice/stringskip.h"
#include "ngspice/wordlist.h"

#include "inp.h"
#include "inpcom.h"
#include "inpc_stb.h"

extern int get_number_terminals(char *c);


/* one inserted 0 V source */
struct stb_ins {
    char *key;              /* <instance><infix><position> */
    char *inst;             /* the instance, as on its card */
    int pos;                /* the terminal, counted from 1 */
    char *tname;            /* the name of the port, or NULL */
    char *xnode;            /* the new node, on the side of the instance */
    char *ynode;            /* the net the terminal was on */
    char *vname;            /* the source: "v." and the new node */
    bool port;              /* made for a port of a subcircuit instance */
    bool live;              /* the new node is in the flat deck */
    bool used;              /* a probe stands for it */
    bool said;              /* it has been announced */
    struct card *card;      /* the card the source is put behind */
    struct stb_ins *next;
};

/* one collected spelling of a probe */
struct stb_probe {
    char *text;             /* as written; in lower case under fold */
    char *inst;             /* text before the last ':' */
    char *term;             /* text after it */
    char *key;              /* set when the terminal has been found */
    struct stb_ins *ins;    /* NULL when the probe is refused */
    char *reason;           /* why it is refused */
    struct stb_probe *next;
};


/* The line source of the cards that keep the probes of the .control
 * section with the stored deck. The cards are known by this address, not
 * by their text: a comment of the user can have the same text. */
static char inp_stb_marker[] = "stb";

#define STB_MARKER_TEXT "*stb-probe "

/* the probes of the deck that is being read */
static struct stb_probe *pending = NULL;
static struct stb_ins *hits = NULL;
/* the texts of the marker cards of a deck that is read again */
static wordlist *restored = NULL;
/* what the names made here have between the instance and the position */
static char *infix = NULL;
/* the subcircuits of that deck are being expanded */
static bool expanding = FALSE;


/* A word of a command that names something of the deck, in the spelling
 * the deck has it in. Under fold the reader has made the deck lower case,
 * and a word typed at the prompt or handed over by a program has not
 * been: it is folded here. Under preserve and distinguish the deck keeps
 * its spelling, and folding the word would make every name unreachable
 * (com_alter() in device.c does the same). */
static void
stb_fold(char *word)
{
    if (inp_case_folding())
        strtolower(word);
}


/* ng_ideq() for the first n characters of two names of the deck. */
static bool
stb_ideqn(const char *a, const char *b, size_t n)
{
    if (inp_case_exact_ids())
        /* case-lint: helper - the exact arm of ng_ideq(), for a prefix */
        return strncmp(a, b, n) == 0;

    /* case-lint: helper - the preserve arm of ng_ideq(), for a prefix */
    return cieqn(a, b, n) != 0;
}


/* Where a name of the deck is found in a line of the deck, or NULL:
 * ng_ideq() for a search. */
static char *
stb_find_name(char *line, const char *name)
{
    if (inp_case_exact_ids())
        /* case-lint: helper - the exact arm of ng_ideq(), for a search */
        return strstr(line, name);

    /* case-lint: helper - the preserve arm of ng_ideq(), for a search */
    return cistrstr(line, name);
}


static bool
stb_all_digits(const char *s)
{
    if (!*s)
        return FALSE;
    for (; *s; s++)
        if (!isdigit_c(*s))
            return FALSE;
    return TRUE;
}


/* An element card, not a comment and not a dot card. */
static bool
stb_live(const struct card *c)
{
    return c->line && *c->line && *c->line != '*' && *c->line != '.';
}


/* The probe, the first word after ".stb" or "stb". It is cut with the
 * function that dot_stb() takes the probe with, so that what is collected
 * here is the word that the analysis asks for: a comma ends it as a blank
 * does. */
static char *
stb_first_token(const char *line)
{
    char *s = (char *) line;
    char *word = NULL;

    INPgetNetTok(&s, &word, 1);

    return word ? word : copy("");
}


/* Token number k of an element card, where the name is number 0. Parens
 * and commas separate nodes as white space does. */
static bool
stb_token(char *line, int k, char **start, char **end)
{
    char *s = line;
    int i;

    for (i = 0; ; i++) {
        while (isspace_c(*s) || *s == '(' || *s == ')' || *s == ',')
            s++;
        if (!*s)
            return FALSE;
        *start = s;
        while (*s && !isspace_c(*s) && *s != '(' && *s != ')' && *s != ',')
            s++;
        *end = s;
        if (i == k)
            return TRUE;
    }
}


/* Where the node called name is on the line, or NULL. */
static char *
stb_occurs(char *line, const char *name)
{
    size_t len = strlen(name);
    char *s;

    for (s = stb_find_name(line, name); s; s = stb_find_name(s + 1, name)) {
        char after = s[len];

        if (s > line && !isspace_c(s[-1]) && !strchr("(,[~=", s[-1]))
            continue;
        if (after && !isspace_c(after) && !strchr("),]=", after))
            continue;
        return s;
    }

    return NULL;
}


/* Put the node 'to' where the node 'from' is, on every card. */
static void
stb_replace_all(struct card *deck, const char *from, const char *to)
{
    size_t len = strlen(from);
    struct card *c;

    for (c = deck; c; c = c->nextcard) {
        char *s;

        if (!c->line)
            continue;
        while ((s = stb_occurs(c->line, from)) != NULL) {
            char *nl = tprintf("%.*s%s%s", (int) (s - c->line), c->line,
                               to, s + len);
            tfree(c->line);
            c->line = nl;
        }
    }
}


static void
stb_free_ins(struct stb_ins *h)
{
    tfree(h->key);
    tfree(h->inst);
    tfree(h->tname);
    tfree(h->xnode);
    tfree(h->ynode);
    tfree(h->vname);
    tfree(h);
}


static void
stb_free_probe(struct stb_probe *p)
{
    tfree(p->text);
    tfree(p->inst);
    tfree(p->term);
    tfree(p->key);
    tfree(p->reason);
    tfree(p);
}


static struct stb_ins *
stb_new_ins(const char *key, const char *inst, int pos, const char *tname,
            const char *ynode, bool port)
{
    struct stb_ins *h = TMALLOC(struct stb_ins, 1);
    struct stb_ins *t;
    int n = 0;

    /* the same instance name on a second card (another branch of an .if)
     * gets a node of its own */
    for (t = hits; t; t = t->next)
        if (ng_ideq(t->key, key))
            n++;

    h->key = copy(key);
    h->inst = copy(inst);
    h->pos = pos;
    h->tname = tname ? copy(tname) : NULL;
    h->xnode = n ? tprintf("%s_%d", key, n + 1) : copy(key);
    h->ynode = copy(ynode);
    h->vname = tprintf("v.%s", h->xnode);
    h->port = port;
    h->live = FALSE;
    h->used = FALSE;
    h->said = FALSE;
    h->card = NULL;
    h->next = NULL;

    if (!hits) {
        hits = h;
    } else {
        for (t = hits; t->next; t = t->next)
            ;
        t->next = h;
    }

    return h;
}


/* Free what a read has left behind. */
void
inp_stb_discard(void)
{
    while (pending) {
        struct stb_probe *next = pending->next;
        stb_free_probe(pending);
        pending = next;
    }
    while (hits) {
        struct stb_ins *next = hits->next;
        stb_free_ins(hits);
        hits = next;
    }
    wl_free(restored);
    restored = NULL;
    tfree(infix);
    expanding = FALSE;
}


/* Free the probes of a circuit. A source may stand for several of them. */
void
inp_stb_free(struct stb_probe *list)
{
    while (list) {
        struct stb_probe *next = list->next;
        struct stb_probe *p;

        if (list->ins) {
            for (p = next; p; p = p->next)
                if (p->ins == list->ins)
                    p->ins = NULL;
            stb_free_ins(list->ins);
        }
        stb_free_probe(list);
        list = next;
    }
}


/* The caller says when it expands the subcircuits of the deck whose
 * probes were collected. Other decks are expanded by the same code (the
 * bridges that XSPICE adds while the circuit is made, for one): their
 * instance lines are not those of the probes. */
void
inp_stb_expanding(bool on)
{
    expanding = on;
}


/* Does the expansion that is going on have probes to look for? */
bool
inp_stb_pending(void)
{
    return expanding && pending != NULL;
}


/* A deck is read again from its stored copy ('reset', 'mc_source'): take
 * the marker cards out of it and keep their probes for inp_stb_collect(). */
void
inp_stb_restore(struct card *deck)
{
    struct card *prev, *c;
    wordlist *end = NULL;

    wl_free(restored);
    restored = NULL;

    if (!deck)
        return;

    for (prev = deck; (c = prev->nextcard) != NULL; ) {
        if (c->linesource == inp_stb_marker) {
            wl_append_word(&restored, &end,
                           copy(c->line + strlen(STB_MARKER_TEXT)));
            prev->nextcard = c->nextcard;
            line_free_x(c, FALSE);
        } else {
            prev = c;
        }
    }
}


/* Keep the word if it is a terminal probe that is written out. Returns
 * TRUE if it is a new one. */
static bool
stb_add(const char *word)
{
    struct stb_probe *p, *t;
    char *text, *colon;

    /* a word that is made when the command runs is not known here */
    if (!strchr(word, ':') || strpbrk(word, "$\"'`{}"))
        return FALSE;

    text = copy(word);
    stb_fold(text);

    for (t = pending; t; t = t->next)
        if (ng_ideq(t->text, text)) {
            tfree(text);
            return FALSE;
        }

    p = TMALLOC(struct stb_probe, 1);
    colon = strrchr(text, ':');
    p->text = text;
    p->inst = copy_substring(text, colon);
    p->term = copy(colon + 1);
    p->key = NULL;
    p->ins = NULL;
    p->reason = NULL;
    p->next = NULL;
    if (!*p->inst || !*p->term)
        p->reason = copy("malformed terminal probe");

    if (!pending) {
        pending = p;
    } else {
        for (t = pending; t->next; t = t->next)
            ;
        t->next = p;
    }

    return TRUE;
}


/* Collect the terminal probes of a deck before its subcircuits are
 * expanded: from the .stb cards of the top level, and from the stb lines
 * of the .control section, which the caller has taken out of the deck
 * into the list 'controls', last line first. The lines inside if, while,
 * repeat, dowhile and foreach are in that list like the others.
 *
 * mcdeck is the stored copy of a deck that is read for the first time:
 * the probes of the .control section are added to it as marker cards,
 * since that section is not stored. It is NULL for a deck that is read
 * again; then inp_stb_restore() has found the marker cards. */
void
inp_stb_collect(struct card *deck, wordlist *controls, struct card *mcdeck)
{
    wordlist *again = restored;
    struct card *c;
    wordlist *wl;
    int depth = 0;

    restored = NULL;
    inp_stb_discard();
    infix = copy("#stb");

    for (c = deck; c; c = c->nextcard) {
        char *s = c->line;

        if (ciprefix(".subckt", s)) {
            depth++;
        } else if (ciprefix(".ends", s)) {
            depth--;
        } else if (depth == 0 && ciprefix(".stb", s) && isspace_c(s[4])) {
            char *word = stb_first_token(s + 4);
            stb_add(word);
            tfree(word);
        } else if (depth > 0 && ciprefix(".stb", s) &&
                   (isspace_c(s[4]) || !s[4])) {
            /* The expansion would copy the card for each instance with
             * the names of the definition, which the circuit does not
             * have: every copy would fail when it is run. */
            fprintf(cp_out, "stb: the card \"%s\" is inside a .subckt definition and is ignored: write it at the top level of the deck\n",
                    s);
            *s = '*';
        }
    }

    if (mcdeck) {
        struct card *at = mcdeck;

        for (wl = controls; wl && wl->wl_next; wl = wl->wl_next)
            ;
        for (; wl; wl = wl->wl_prev) {
            char *s = skip_ws(wl->wl_word);

            if (ciprefix("stb", s) && isspace_c(s[3])) {
                char *word = stb_first_token(s + 3);
                if (stb_add(word)) {
                    stb_fold(word);
                    at = insert_new_line(at,
                                         tprintf(STB_MARKER_TEXT "%s", word),
                                         0, 0, inp_stb_marker);
                }
                tfree(word);
            }
        }
    } else {
        for (wl = again; wl; wl = wl->wl_next)
            stb_add(wl->wl_word);
    }
    wl_free(again);

    /* The names made here have the infix between the instance and the
     * position. An element line of the deck that has it already makes it
     * longer, so that no made name can be a name of the deck. */
    if (pending) {
        c = deck;
        while (c) {
            if (stb_live(c) && stb_find_name(c->line, infix)) {
                char *longer = tprintf("%s#", infix);
                tfree(infix);
                infix = longer;
                c = deck;
            } else {
                c = c->nextcard;
            }
        }
    }
}


/* An instance line of a subcircuit is about to be expanded: xline is the
 * line, formals are the nformals port names of the subcircuit subname.
 * If a collected probe names a port of this instance, the node of the
 * instance line at that port is replaced by a new node, so that all
 * elements of this one instance that are on the port come to lie on the
 * new node. Returns the line to expand, which the caller owns, or NULL if
 * the line stays as it is. */
char *
inp_stb_xcard(const char *xline, const char *subname, const char *formals,
              int nformals, NGHASHPTR globals)
{
    struct stb_probe *p;
    struct stb_ins *first = NULL;   /* the first source made for this line */
    char *name = stb_first_token(xline);
    char **actual = NULL;
    char **formal = NULL;
    char *s, *tofree, *tok;
    char *newline = NULL;
    bool changed = FALSE;
    int nactual = 0;
    int i;

    NG_IGNORE(globals);

    for (p = pending; p; p = p->next)
        if (ng_ideq(p->inst, name))
            break;
    if (!p || nformals < 1) {
        tfree(name);
        return NULL;
    }

    /* the line, as settrans() in subckt.c takes it apart:
     * the name, a node for each port, the name of the subcircuit */
    actual = TMALLOC(char *, nformals + 2);
    s = tofree = copy(xline);
    while ((tok = gettok(&s)) != NULL) {
        if (nactual < nformals + 2)
            actual[nactual] = tok;
        else
            tfree(tok);
        nactual++;
    }
    tfree(tofree);

    formal = TMALLOC(char *, nformals);
    s = tofree = copy(formals);
    for (i = 0; i < nformals; i++)
        formal[i] = gettok(&s);
    tfree(tofree);

    if (nactual != nformals + 2 || !formal[nformals - 1])
        goto done;

    for (; p; p = p->next) {
        struct stb_ins *h;
        char *key;
        int k = 0;

        if (!ng_ideq(p->inst, name))
            continue;

        if (stb_all_digits(p->term)) {
            if (strlen(p->term) < 9)
                k = atoi(p->term);
            if (k < 1 || k > nformals) {
                if (!p->key && !p->reason)
                    p->reason = tprintf("%s has %d terminals", name, nformals);
                continue;
            }
        } else {
            for (i = 0; i < nformals && !k; i++)
                if (ng_ideq(formal[i], p->term))
                    k = i + 1;
            if (!k) {
                if (!p->key && !p->reason) {
                    char *ports = tprintf("%s", formal[0]);
                    for (i = 1; i < nformals; i++) {
                        char *more = tprintf("%s %s", ports, formal[i]);
                        tfree(ports);
                        ports = more;
                    }
                    p->reason = tprintf("%s has no terminal named %s (1 to %d, the ports of %s: %s)",
                                        name, p->term, nformals, subname, ports);
                    tfree(ports);
                }
                continue;
            }
        }

        /* two spellings of one port share the source */
        key = tprintf("%s%s%d", name, infix, k);
        for (h = first; h; h = h->next)
            if (ng_ideq(h->key, key))
                break;
        if (!h) {
            h = stb_new_ins(key, name, k, formal[k - 1], actual[k], TRUE);
            if (!first)
                first = h;
            tfree(actual[k]);
            actual[k] = copy(h->xnode);
            changed = TRUE;
        }
        if (!p->key)
            p->key = key;
        else
            tfree(key);
    }

    if (changed) {
        newline = copy(actual[0]);
        for (i = 1; i < nactual; i++) {
            char *more = tprintf("%s %s", newline, actual[i]);
            tfree(newline);
            newline = more;
        }
    }

done:
    for (i = 0; i < nformals + 2 && i < nactual; i++)
        tfree(actual[i]);
    for (i = 0; i < nformals; i++)
        tfree(formal[i]);
    tfree(actual);
    tfree(formal);
    tfree(name);

    return newline;
}


/* Is name the model of a .model card of the deck? A model that is binned
 * has cards name.1, name.2, ... */
static bool
stb_is_model(struct card *deck, const char *name)
{
    size_t len = strlen(name);
    struct card *c;

    for (c = deck; c; c = c->nextcard)
        if (c->line && ciprefix(".model", c->line) && isspace_c(c->line[6])) {
            char *s = skip_ws(c->line + 6);

            if (stb_ideqn(s, name, len)) {
                s += len;
                if (*s == '.' && isdigit_c(s[1]))
                    for (s++; isdigit_c(*s); s++)
                        ;
                if (!*s || isspace_c(*s) || *s == '(')
                    return TRUE;
            }
        }

    return FALSE;
}


/* The probe names a terminal of the element on card c: put the terminal
 * on a new node. On failure the probe gets the reason. */
static void
stb_element(struct card *deck, struct card *c, struct stb_probe *p)
{
    char *line = c->line;
    char *name = stb_first_token(line);
    struct stb_ins *h;
    char *start, *end;
    char *key, *ynode;
    int n = 0, k = 0;

    switch (tolower_c(*name)) {
    case 'a':
        p->reason = tprintf("%s is a code model instance; place a voltage source", name);
        break;
    case 'k':
        p->reason = tprintf("%s has no terminals", name);
        break;
    case 'w': case 'f': case 'h':
        n = 2;
        break;
    case 'e': case 'g':
        n = 4;
        break;
    case 'x':
        n = 0;
        break;
    default:
        n = get_number_terminals(line);
        break;
    }

    if (!p->reason && n < 1)
        p->reason = tprintf("the terminals of %s cannot be counted", name);

    /* a count that is wrong would put the new node in the place of the
     * model: the word after the nodes of these devices is a model */
    if (!p->reason && strchr("dmqpn", tolower_c(*name))) {
        bool ok = stb_token(line, n + 1, &start, &end);

        if (ok) {
            char *model = copy_substring(start, end);
            ok = stb_is_model(deck, model);
            tfree(model);
        }
        if (!ok)
            p->reason = tprintf("the terminals of %s cannot be counted", name);
    }

    if (!p->reason) {
        if (!stb_all_digits(p->term))
            p->reason = tprintf("%s has no terminal named %s (1 to %d)",
                                name, p->term, n);
        else if (strlen(p->term) < 9)
            k = atoi(p->term);
    }

    if (!p->reason && (k < 1 || k > n || !stb_token(line, k, &start, &end)))
        p->reason = tprintf("%s has %d terminals", name, n);

    if (p->reason) {
        tfree(name);
        return;
    }

    key = tprintf("%s%s%d", name, infix, k);
    for (h = hits; h; h = h->next)
        if (!h->port && ng_ideq(h->key, key))
            break;
    if (!h) {
        ynode = copy_substring(start, end);
        h = stb_new_ins(key, name, k, NULL, ynode, FALSE);
        c->line = tprintf("%.*s%s%s", (int) (start - line), line,
                          h->xnode, end);
        tfree(line);
        tfree(ynode);
        h->live = TRUE;
        h->card = c;
    }
    h->used = TRUE;
    p->key = key;
    p->ins = h;
    tfree(name);
}


/* The value that an .ic card gives the node, or NULL. */
static char *
stb_ic_value(char *line, const char *node)
{
    size_t len = strlen(node);
    char *s;

    for (s = cistrstr(line, "v("); s; s = cistrstr(s + 1, "v(")) {
        char *t = skip_ws(s + 2);

        if (!isspace_c(s[-1]) || !stb_ideqn(t, node, len))
            continue;
        t = skip_ws(t + len);
        if (*t != ')')
            continue;
        t = skip_ws(t + 1);
        if (*t != '=')
            continue;
        t = skip_ws(t + 1);
        return copy_substring(t, skip_non_ws(t));
    }

    return NULL;
}


/* An initial condition on the net of a probed terminal is repeated for
 * the new node, and the other way round. The source between them has no
 * state: with "uic" the node that has no .ic would start at 0 V.
 *
 * A new node with an .ic also gets a capacitor of 0 F to ground. An .ic
 * needs the element of the matrix on the diagonal of its node, and KLU
 * cannot add one when the initial conditions are set (CKTic): if the
 * terminal makes none, as a controlling input of an E or G element does,
 * every analysis of the circuit would stop there. The capacitor makes
 * the element and adds nothing to it. */
static void
stb_ic_twin(struct card *deck)
{
    struct stb_ins *h;
    struct card *c;

    for (c = deck; c; c = c->nextcard) {
        bool again;

        if (!c->line || !ciprefix(".ic", c->line) || !isspace_c(c->line[3]))
            continue;

        do {
            again = FALSE;
            for (h = hits; h; h = h->next) {
                char *valx, *valy;
                char *nl = NULL;

                if (!h->live || !h->used)
                    continue;
                valx = stb_ic_value(c->line, h->xnode);
                valy = stb_ic_value(c->line, h->ynode);
                if (valy && !valx)
                    nl = tprintf("%s v(%s)=%s", c->line, h->xnode, valy);
                else if (valx && !valy && !eq(h->ynode, "0"))
                    nl = tprintf("%s v(%s)=%s", c->line, h->ynode, valx);
                if (nl) {
                    tfree(c->line);
                    c->line = nl;
                    again = TRUE;
                }
                tfree(valx);
                tfree(valy);
            }
        } while (again);
    }

    for (h = hits; h; h = h->next) {
        char *val = NULL;

        if (!h->live || !h->used)
            continue;
        for (c = deck; c && !val; c = c->nextcard)
            if (c->line && ciprefix(".ic", c->line) && isspace_c(c->line[3]))
                val = stb_ic_value(c->line, h->xnode);
        if (val)
            insert_new_line(h->card,
                            tprintf("c.%s %s 0 0", h->xnode, h->xnode),
                            0, h->card->linenum_orig,
                            h->card->linesource ? h->card->linesource : "internal");
        tfree(val);
    }
}


/* The deck is flat: the subcircuits are expanded, the parameters are
 * numbers, the inactive branches of .if are comments. Find the terminals
 * of the probes that name an element, add the 0 V sources, and say what
 * was done. */
void
inp_stb_insert(struct card *deck)
{
    struct stb_probe *p, **pp;
    struct stb_ins *h;
    struct card *c;

    if (!pending)
        return;

    /* a word with a colon that is the name of an element is no terminal
     * probe: .probe makes such names */
    for (pp = &pending; (p = *pp) != NULL; ) {
        bool isname = FALSE;

        for (c = deck; c && !isname && !p->key; c = c->nextcard)
            if (stb_live(c)) {
                char *name = stb_first_token(c->line);
                isname = ng_ideq(name, p->text);
                tfree(name);
            }
        if (isname) {
            *pp = p->next;
            stb_free_probe(p);
        } else {
            pp = &p->next;
        }
    }

    /* ports: the instance line that is in the flat deck has its new node
     * on an element */
    for (h = hits; h; h = h->next)
        for (c = deck; c && !h->live; c = c->nextcard)
            if (stb_live(c) && stb_occurs(c->line, h->xnode)) {
                h->live = TRUE;
                h->card = c;
            }

    for (p = pending; p; p = p->next) {
        struct stb_ins *found = NULL;
        int n = 0;

        if (!p->key)
            continue;
        for (h = hits; h; h = h->next)
            if (h->live && ng_ideq(h->key, p->key)) {
                found = h;
                n++;
            }
        if (n == 1) {
            tfree(p->reason);
            p->ins = found;
            found->used = TRUE;
        } else if (!p->reason) {
            if (n == 0)
                p->reason = tprintf("%s has nothing connected to that port, or is in an inactive .if branch", p->inst);
            else
                p->reason = tprintf("%s is not a unique instance name", p->inst);
        }
    }

    /* a new node that no probe stands for goes back to its net */
    for (h = hits; h; h = h->next)
        if (h->live && !h->used) {
            stb_replace_all(deck, h->xnode, h->ynode);
            h->live = FALSE;
        }

    /* elements */
    for (p = pending; p; p = p->next) {
        struct card *found = NULL;
        int n = 0;

        if (p->key || p->reason)
            continue;
        for (c = deck; c; c = c->nextcard)
            if (stb_live(c)) {
                char *name = stb_first_token(c->line);
                if (ng_ideq(name, p->inst)) {
                    found = c;
                    n++;
                }
                tfree(name);
            }
        if (n == 0)
            p->reason = tprintf("no instance named %s", p->inst);
        else if (n > 1)
            p->reason = tprintf("%s is not a unique instance name", p->inst);
        else
            stb_element(deck, found, p);
    }

    /* the sources: the new node is the positive one */
    for (h = hits; h; h = h->next)
        if (h->live && h->used)
            insert_new_line(h->card,
                            tprintf("%s %s %s dc 0", h->vname, h->xnode, h->ynode),
                            0, h->card->linenum_orig,
                            h->card->linesource ? h->card->linesource : "internal");

    stb_ic_twin(deck);

    for (p = pending; p; p = p->next)
        if (p->ins) {
            if (!p->ins->said)
                fprintf(cp_out, "stb: probe %s inserted as %s (x = %s, y = %s)\n",
                        p->text, p->ins->vname, p->ins->xnode, p->ins->ynode);
            p->ins->said = TRUE;
        } else {
            fprintf(cp_out, "stb: probe %s not inserted: %s\n",
                    p->text, p->reason);
        }
}


/* The circuit has been made: hand its probes over. The caller owns the
 * list and frees it with inp_stb_free(). */
struct stb_probe *
inp_stb_take(void)
{
    struct stb_probe *list = pending;

    pending = NULL;
    while (hits) {
        struct stb_ins *next = hits->next;
        if (hits->live && hits->used) {
            hits->card = NULL;
            hits->next = NULL;
        } else {
            stb_free_ins(hits);
        }
        hits = next;
    }
    inp_stb_discard();

    return list;
}


/* For the analysis: the source that was inserted for the terminal probe.
 * Returns 0 and the name of the source, 1 and the reason if the probe was
 * collected but could not be inserted, -1 if it was not collected. */
int
inp_stb_lookup(CKTcircuit *ckt, const char *probe, const char **vname,
               const char **reason)
{
    struct stb_probe *p;
    char *text;
    int ret = -1;

    if (!ft_curckt || ft_curckt->ci_ckt != ckt)
        return -1;

    /* the word, in the spelling the deck has its names in */
    text = copy(probe);
    stb_fold(text);

    for (p = ft_curckt->ci_stbprobes; p && ret < 0; p = p->next)
        if (ng_ideq(p->text, text)) {
            if (p->ins) {
                *vname = p->ins->vname;
                ret = 0;
            } else {
                *reason = p->reason;
                ret = 1;
            }
        }

    tfree(text);
    return ret;
}
