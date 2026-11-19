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

   An instance inside a subcircuit instance is named by the path of the
   instances, x1.x2.r3, or by the name it has in the circuit, r.x1.x2.r3.
   A terminal is its position on the instance line, counted from 1, the
   name of the port for a subcircuit instance, or one of the letters
   below for a transistor.

   Adding sources to a deck that is being read is what .probe does as
   well, see inpc_probe.c.

   Current probes. A .probe I(...) that the code of inpc_probe.c cannot
   serve itself, because it names a terminal inside a subcircuit
   instance, a port of an instance by its name or a terminal of an OSDI
   device, is handed over here as an .iprobe card and inserted by the
   same code as a terminal probe of stb. Such a probe is a current probe:
   its source has the opposite sign, the net y its positive node, so
   that its branch is the current into the terminal; its new node has
   the infix #probe_int_, which no plot saves; after the circuit is made
   the source is renamed <path>:<terminal>, so that its vector is
   <path>:<terminal>#branch, and it is saved. Without a terminal a probe
   stands for every terminal of the instance; for an instance of two
   terminals it is one probe, named <path>, of the current out of the
   second terminal, as .probe has it at the top level. A current probe
   and a probe of stb never share a source: on the same terminal their
   sources are in series. See doc/claude/specs/terminal-current-probes.md.

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
#include "ngspice/ifsim.h"
#include "ngspice/cktdefs.h"
#include "ngspice/hash.h"

#ifdef XSPICE
#include "ngspice/mifparse.h"
#endif

#include "inp.h"
#include "inpcom.h"
#include "inpc_stb.h"
#include "subckt.h"

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
    char *cname;            /* a current probe: the name the source gets
                             * when the circuit is made, else NULL */
    bool current;           /* made for a current probe (.probe) */
    bool whole;             /* a current probe of an instance of two
                             * terminals: at the second, the current out of
                             * it into the net */
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
    char *twin;             /* a subcircuit instance whose name differs
                             * from inst only in case (distinguish) */
    bool posnote;           /* its number is also the name of a port */
    bool current;           /* a current probe of .probe, not of stb */
    bool whole;             /* a current probe of an instance of two
                             * terminals without a terminal: at the
                             * second, named <path> */
    bool spawned;           /* a probe without a terminal, which has been
                             * replaced by one probe per terminal */
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
/* the same for the nodes of current probes; outitf.c leaves a node with
 * "probe_int_" in its name out of the plots, as it does for .probe */
static char *cinfix = NULL;
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


/* ng_ideq() for one character of a name of the deck. */
static bool
stb_ch_eq(char a, char b)
{
    return inp_case_exact_ids() ? (a == b) : (tolower_c(a) == tolower_c(b));
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


/* The letters of the terminals of a device, in the order of the nodes on
 * its line, or NULL. The letter of a device and the letters of its
 * terminals are words of the language: any case, in every case mode. */
static const char *
stb_letters(char dev)
{
    switch (tolower_c(dev)) {
    case 'm':
        return "dgsb";
    case 'q':
        return "cbes";
    case 'j': case 'z':
        return "dgs";
    default:
        return NULL;
    }
}


/* An instance inside a subcircuit instance has its path behind its
 * letter in the circuit: r0 of x1 is r.x1.r0. The letter is there once
 * more for each instance on the path whose subcircuit is defined inside
 * another one, since that instance is expanded with the definition
 * around it: r1 of xi of x3 is r.x3.r.xi.r1. Returns the path, x1.r0 or
 * x3.xi.r1, of such a name, and the name itself if it is not of that
 * kind. The caller owns it. */
static char *
stb_path(const char *name)
{
    const char *leaf = strrchr(name, '.');
    const char *s;
    char *path, *d;

    if (!leaf || leaf <= name + 1 || name[1] != '.' ||
        !stb_ch_eq(name[0], leaf[1]))
        return copy(name);

    path = d = TMALLOC(char, strlen(name));
    for (s = name + 2; *s; ) {
        if (stb_ch_eq(s[0], name[0]) && s[1] == '.') {
            s += 2;
            continue;
        }
        while (*s && *s != '.')
            *d++ = *s++;
        if (*s)
            *d++ = *s++;
    }
    *d = '\0';

    return path;
}


/* Is name the name that the instance with this path has in the circuit? */
static bool
stb_names(const char *name, const char *path)
{
    char *p;
    bool same;

    if (tolower_c(*path) != 'x' || !name[0] || name[1] != '.')
        return FALSE;
    p = stb_path(name);
    same = ng_ideq(p, path);
    tfree(p);

    return same;
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
    tfree(h->cname);
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
    tfree(p->twin);
    tfree(p);
}


static struct stb_ins *
stb_new_ins(const char *key, const char *inst, int pos, const char *tname,
            const char *ynode, bool port, bool current)
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
    h->cname = NULL;
    h->current = current;
    h->whole = FALSE;
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
    tfree(cinfix);
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
 * TRUE if it is a new one. A current probe (current) may have no
 * terminal: it stands for every terminal of the instance. A probe of
 * stb and a current probe are two probes even when they are spelt the
 * same, since their sources have opposite signs. */
static bool
stb_add_probe(const char *word, bool current)
{
    struct stb_probe *p, *t;
    char *text, *colon;

    /* a word that is made when the command runs is not known here */
    if ((!current && !strchr(word, ':')) || strpbrk(word, "$\"'`{}"))
        return FALSE;

    text = copy(word);
    stb_fold(text);

    for (t = pending; t; t = t->next)
        if (t->current == current && ng_ideq(t->text, text)) {
            tfree(text);
            return FALSE;
        }

    p = TMALLOC(struct stb_probe, 1);
    colon = strrchr(text, ':');
    p->text = text;
    p->inst = colon ? copy_substring(text, colon) : copy(text);
    p->term = copy(colon ? colon + 1 : "");
    p->key = NULL;
    p->ins = NULL;
    p->reason = NULL;
    p->twin = NULL;
    p->posnote = FALSE;
    p->current = current;
    p->whole = FALSE;
    p->spawned = FALSE;
    p->next = NULL;
    if (!*p->inst || (colon && !*p->term))
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


static bool
stb_add(const char *word)
{
    return stb_add_probe(word, FALSE);
}


/* A probe without a terminal has been found to be an instance of
 * terminals that can be counted: put a probe of the terminal term after
 * 'after', in the place of the probe p. Returns the new probe. */
static struct stb_probe *
stb_spawn(struct stb_probe *p, struct stb_probe *after, const char *term,
          bool whole)
{
    struct stb_probe *q = TMALLOC(struct stb_probe, 1);

    q->text = tprintf("%s:%s", p->inst, term);
    q->inst = copy(p->inst);
    q->term = copy(term);
    q->key = NULL;
    q->ins = NULL;
    q->reason = NULL;
    q->twin = p->twin ? copy(p->twin) : NULL;
    q->posnote = FALSE;
    q->current = p->current;
    q->whole = whole;
    q->spawned = FALSE;
    q->next = after->next;
    after->next = q;
    p->spawned = TRUE;

    return q;
}


/* The name that the source of a current probe gets: <path>:<terminal>,
 * or <path> for a probe of an instance of two terminals. Under fold the
 * name is in lower case, as the names of the deck are. */
static char *
stb_cname(const char *path, const char *term, bool whole)
{
    char *name = whole ? copy(path) : tprintf("%s:%s", path, term);

    if (inp_case_folding())
        strtolower(name);

    return name;
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

    /* The current probes that inp_probe() (inpc_probe.c) has handed over,
     * after the probes of stb: on a terminal that is both, the source of
     * stb is the one next to the net. The cards are in the stored copy
     * of the deck, so that a deck read again has them as well; here they
     * become comments. */
    for (c = deck; c; c = c->nextcard) {
        char *s = c->line;

        if (ciprefix(".iprobe", s) && isspace_c(s[7])) {
            char *word = stb_first_token(s + 7);
            stb_add_probe(word, TRUE);
            tfree(word);
            *s = '*';
        }
    }

    /* The names made here have the infix between the instance and the
     * position. An element line of the deck that has it already makes it
     * longer, so that no made name can be a name of the deck. */
    if (pending) {
        cinfix = copy("#probe_int_");
        c = deck;
        while (c) {
            if (stb_live(c) && stb_find_name(c->line, infix)) {
                char *longer = tprintf("%s#", infix);
                tfree(infix);
                infix = longer;
                c = deck;
            } else if (stb_live(c) && stb_find_name(c->line, cinfix)) {
                char *longer = tprintf("%s#", cinfix);
                tfree(cinfix);
                cinfix = longer;
                c = deck;
            } else {
                c = c->nextcard;
            }
        }
    }
}


/* Is the port a global node? The table of the global nodes is that of
 * the expansion (subckt.c), which makes its keys with glo_key(): folded
 * under preserve, where two spellings are one node. */
static bool
stb_is_global(NGHASHPTR globals, const char *name)
{
    char *key = glo_key(name);
    bool is = nghash_find(globals, key) != NULL;

    tfree(key);
    return is;
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

    /* Under the case mode distinguish an instance whose name differs from
     * that of a probe only in case is another instance. It is kept for the
     * message of a probe that finds none (stb_why_missing()): the instance
     * lines are gone when that is known. */
    if (inp_case_mode() == NG_CASE_DISTINGUISH)
        for (p = pending; p; p = p->next)
            /* case-lint: neither - a deliberate scan without regard to case */
            if (!p->twin && !ng_ideq(p->inst, name) && cieq(p->inst, name))
                p->twin = copy(name);

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

        if (!ng_ideq(p->inst, name) || p->spawned)
            continue;

        /* A current probe without a terminal: every port, as probes of
         * their own that the loop comes to next. An instance of two ports
         * is one probe, at the second, as .probe has it at the top level:
         * the current out of that port. What goes in at the first need
         * not be the same, since an element inside can be on ground. */
        if (p->current && !*p->term) {
            struct stb_probe *at = p;

            if (nformals == 2)
                stb_spawn(p, p, formal[1], TRUE);
            else
                for (i = 0; i < nformals; i++)
                    at = stb_spawn(p, at, formal[i], FALSE);
            continue;
        }

        if (stb_all_digits(p->term)) {
            if (strlen(p->term) < 9)
                k = atoi(p->term);
            if (k < 1 || k > nformals) {
                if (!p->key && !p->reason)
                    p->reason = tprintf("%s has %d terminals", name, nformals);
                continue;
            }
            /* a number is a position, also where a port has it as its
             * name */
            for (i = 0; i < nformals; i++)
                if (i != k - 1 && ng_ideq(formal[i], p->term))
                    p->posnote = TRUE;
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

        /* A port that is a global node: the elements of the subcircuit
         * are on the global net, whatever the instance line has. A port
         * that has the name of an earlier one: the node of the instance
         * line is not used. */
        if (stb_is_global(globals, formal[k - 1])) {
            if (!p->key && !p->reason)
                p->reason = tprintf("port %d of %s is a global node and is not connected through the instance line",
                                    k, subname);
            continue;
        }
        for (i = 0; i < k - 1; i++)
            if (ng_ideq(formal[i], formal[k - 1]))
                break;
        if (i < k - 1) {
            if (!p->key && !p->reason)
                p->reason = tprintf("port %d of %s has the same name as port %d",
                                    k, subname, i + 1);
            continue;
        }

        /* two spellings of one port share the source; a current probe has
         * a source of its own, in series with that of stb, and the probe
         * of an instance of two ports another one */
        if (!p->current)
            key = tprintf("%s%s%d", name, infix, k);
        else if (p->whole)
            key = tprintf("%s%s0", name, cinfix);
        else
            key = tprintf("%s%s%d", name, cinfix, k);
        for (h = first; h; h = h->next)
            if (ng_ideq(h->key, key))
                break;
        if (!h) {
            h = stb_new_ins(key, name, k, formal[k - 1], actual[k], TRUE,
                            p->current);
            h->whole = p->whole;
            if (p->current)
                h->cname = stb_cname(name, formal[k - 1], p->whole);
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


/* The letters of the terminals for a current probe: those of stb, and a
 * c for a diode, the anode and the cathode (k is the cathode as well). */
static const char *
stb_cur_letters(char dev)
{
    if (tolower_c(dev) == 'd')
        return "ac";

    return stb_letters(dev);
}


/* The device of an N line (an OSDI device) whose model is the word after
 * its n nodes: the type on the .model card of that model, or NULL. A
 * model that is binned has cards name.1, name.2, ... of one type. */
static IFdevice *
stb_osdi_device(struct card *deck, char *line, int n)
{
    char *start, *end, *model;
    char *s = NULL, *e = NULL;
    struct card *c;
    size_t len;
    int i;

    if (!stb_token(line, n + 1, &start, &end))
        return NULL;
    model = copy_substring(start, end);
    len = strlen(model);

    for (c = deck; c; c = c->nextcard) {
        if (!c->line || !ciprefix(".model", c->line) || !isspace_c(c->line[6]))
            continue;
        s = skip_ws(c->line + 6);
        if (!stb_ideqn(s, model, len))
            continue;
        s += len;
        if (*s == '.' && isdigit_c(s[1]))
            for (s++; isdigit_c(*s); s++)
                ;
        if (!isspace_c(*s))
            continue;
        s = skip_ws(s);
        for (e = s; *e && !isspace_c(*e) && *e != '('; e++)
            ;
        break;
    }
    tfree(model);
    if (!c)
        return NULL;

    /* the type of a model is a word the simulator defines: any case */
    for (i = 0; i < ft_sim->numDevices; i++) {
        IFdevice *dev = ft_sim->devices[i];

        if (dev && dev->termNames && dev->terms &&
            strlen(dev->name) == (size_t) (e - s) && ciprefix(dev->name, s))
            return dev;
    }

    return NULL;
}


/* The spelling of terminal k (from 1) of an element for a current probe
 * that stands for one terminal of a probe without a terminal: its letter,
 * the name the OSDI device gives it, or its position. */
static char *
stb_term_name(const char *letters, const IFdevice *odev, int k)
{
    if (letters && k <= (int) strlen(letters))
        return tprintf("%c", letters[k - 1]);
    if (odev && k <= *odev->terms && odev->termNames[k - 1])
        return copy(odev->termNames[k - 1]);

    return tprintf("%d", k);
}


/* The name of terminal k (from 1) of an element in the vector of a
 * current probe: the name .probe gives it at the top level
 * (get_terminal_name() of inpc_probe.c), so that a terminal has one name
 * at every depth; of an OSDI device the name its model gives it, which
 * the top level has from here as well. */
static char *
stb_vec_term(char dev, const IFdevice *odev, int k)
{
    static const char *const mos[] = { "d", "g", "s", "b_tj", "tc", "n6", "n7" };
    static const char *const bjt[] = { "c", "b", "e", "s", "t" };
    static const char *const jfet[] = { "d", "g", "s" };
    static const char *const diode[] = { "a", "c" };
    const char *const *names = NULL;
    int n = 0;

    if (odev && k <= *odev->terms && odev->termNames[k - 1])
        return copy(odev->termNames[k - 1]);

    switch (tolower_c(dev)) {
    case 'm':
        names = mos;
        n = 7;
        break;
    case 'q':
        names = bjt;
        n = 5;
        break;
    case 'j':
    case 'z':
        names = jfet;
        n = 3;
        break;
    case 'd':
        names = diode;
        n = 2;
        break;
    default:
        break;
    }
    if (k <= n)
        return copy(names[k - 1]);

    return tprintf("n%d", k);
}


/* The probe names a terminal of the element on card c: put the terminal
 * on a new node. On failure the probe gets the reason. */
static void
stb_element(struct card *deck, struct card *c, struct stb_probe *p)
{
    char *line = c->line;
    char *name = stb_first_token(line);
    const char *letters = p->current ? stb_cur_letters(*name) :
                                       stb_letters(*name);
    IFdevice *odev = NULL;
    struct stb_ins *h;
    char *start, *end;
    char *key, *ynode;
    int n = 0, k = 0;

    switch (tolower_c(*name)) {
    case 'a':
        p->reason = tprintf("%s is a code model instance; place a voltage source", p->inst);
        break;
    case 'k':
        p->reason = tprintf("%s has no terminals", p->inst);
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
        p->reason = tprintf("the terminals of %s cannot be counted", p->inst);

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
            p->reason = tprintf("the terminals of %s cannot be counted", p->inst);
    }

    /* An E or G source with an expression: the reader has put its
     * controlling input on a node of its own, which a B source drives.
     * The node has the name of the source, after its path if it is
     * inside an instance, and _int1 behind it. */
    if (!p->reason &&
        (tolower_c(*name) == 'e' || tolower_c(*name) == 'g') &&
        stb_token(line, 3, &start, &end)) {
        const char *leaf = strrchr(name, '.');
        char *made = tprintf("%s_int1", leaf ? leaf + 1 : name);
        size_t len = strlen(made);

        if ((size_t) (end - start) >= len &&
            /* case-lint: stored - the node the reader made from the name on this card */
            strncmp(end - len, made, len) == 0 &&
            (end - len == start || *(end - len - 1) == '.'))
            p->reason = tprintf("%s is a source with an expression and is replaced by other elements; place a voltage source", p->inst);
        tfree(made);
    }

    /* the terminals of an OSDI device have the names of its model */
    if (!p->reason && p->current && tolower_c(*name) == 'n')
        odev = stb_osdi_device(deck, line, n);

    /* A current probe without a terminal: every terminal, as probes of
     * their own that the caller comes to next; of a controlled source or
     * a switch the two nodes of its output. An element of two terminals
     * is one probe, at the second, as .probe has it at the top level: the
     * current out of it, which is the current through the element from
     * its first terminal to its second. */
    if (!p->reason && p->current && !*p->term) {
        struct stb_probe *at = p;
        int nn = strchr("efghsw", tolower_c(*name)) ? 2 : n;
        char *term;

        if (nn == 2) {
            term = stb_term_name(letters, odev, 2);
            stb_spawn(p, p, term, TRUE);
            tfree(term);
        } else {
            for (k = 1; k <= nn; k++) {
                term = stb_term_name(letters, odev, k);
                at = stb_spawn(p, at, term, FALSE);
                tfree(term);
            }
        }
        tfree(name);
        return;
    }

    if (!p->reason) {
        const char *l = NULL;

        if (letters && p->term[0] && !p->term[1]) {
            char t = (char) tolower_c(p->term[0]);
            /* k, the cathode of a diode, is c */
            if (p->current && tolower_c(*name) == 'd' && t == 'k')
                t = 'c';
            l = strchr(letters, t);
        }
        if (odev && !stb_all_digits(p->term)) {
            int i;
            /* a name of a terminal is a word of the model: any case */
            for (i = 0; i < *odev->terms && !k; i++)
                if (odev->termNames[i] &&
                    strlen(odev->termNames[i]) == strlen(p->term) &&
                    ciprefix(odev->termNames[i], p->term))
                    k = i + 1;
        }
        if (k) {
            ;
        } else if (stb_all_digits(p->term)) {
            if (strlen(p->term) < 9)
                k = atoi(p->term);
        } else if (l && l - letters < n) {
            k = (int) (l - letters) + 1;
        } else if (odev) {
            char *list = copy(odev->termNames[0] ? odev->termNames[0] : "");
            int i;

            for (i = 1; i < *odev->terms && i < n; i++) {
                char *more = tprintf("%s %s", list,
                                     odev->termNames[i] ? odev->termNames[i] : "");
                tfree(list);
                list = more;
            }
            p->reason = tprintf("%s has no terminal named %s (1 to %d, or %s)",
                                p->inst, p->term, n, list);
            tfree(list);
        } else if (letters) {
            /* the letters of the nodes that the line has */
            char *list = TMALLOC(char, 2 * strlen(letters) + 1);
            int i;

            for (i = 0; letters[i] && i < n; i++) {
                list[2 * i] = letters[i];
                list[2 * i + 1] = ' ';
            }
            list[i > 0 ? 2 * i - 1 : 0] = '\0';
            p->reason = tprintf("%s has no terminal named %s (1 to %d, or %s)",
                                p->inst, p->term, n, list);
            tfree(list);
        } else {
            p->reason = tprintf("%s has no terminal named %s (1 to %d)",
                                p->inst, p->term, n);
        }
    }

    if (!p->reason && (k < 1 || k > n || !stb_token(line, k, &start, &end)))
        p->reason = tprintf("%s has %d terminals", p->inst, n);

    if (p->reason) {
        tfree(name);
        return;
    }

    if (!p->current)
        key = tprintf("%s%s%d", name, infix, k);
    else if (p->whole)
        key = tprintf("%s%s0", name, cinfix);
    else
        key = tprintf("%s%s%d", name, cinfix, k);
    for (h = hits; h; h = h->next)
        if (!h->port && ng_ideq(h->key, key))
            break;
    if (!h) {
        ynode = copy_substring(start, end);
        h = stb_new_ins(key, name, k, NULL, ynode, FALSE, p->current);
        h->whole = p->whole;
        if (p->current) {
            char *path = stb_path(name);
            char *term = stb_vec_term(*name, odev, k);

            h->cname = stb_cname(path, term, p->whole);
            tfree(path);
            tfree(term);
        }
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


/* The card of the element called name, or NULL. With bypath, name is
 * the path of the instances of the element. *count receives the number
 * of the cards that have this name. */
static struct card *
stb_find_card(struct card *deck, const char *name, bool bypath, int *count)
{
    struct card *found = NULL;
    struct card *c;

    *count = 0;
    for (c = deck; c; c = c->nextcard)
        if (stb_live(c)) {
            char *first = stb_first_token(c->line);
            if (bypath ? stb_names(first, name) : ng_ideq(first, name)) {
                found = c;
                (*count)++;
            }
            tfree(first);
        }

    return found;
}


/* No card has the name of the instance of a probe. The reader has
 * replaced some elements by others, which have names of their own:
 *   R, C, L with a value that is an expression: a B source, b<name>
 *   (r1 of x1: b.x1.br1)
 *   a POLY source: a code model instance, a$poly$<name in the circuit>
 * An instance of a subcircuit that is defined inside another subcircuit
 * is expanded with that one, before the instance lines of the deck: its
 * elements are there, its instance line was never seen.
 * twin is the instance of a subcircuit whose name differs from inst only
 * in case, if one has been expanded, or NULL.
 * Returns the reason, which the caller owns. */
static char *
stb_why_missing(struct card *deck, const char *inst, const char *twin)
{
    char *path = stb_path(inst);
    const char *dot = strrchr(path, '.');
    const char *leaf = dot ? dot + 1 : path;
    char *bsrc = NULL;
    char *reason = NULL;
    size_t len = strlen(path);
    struct card *c;

    if (*leaf && strchr("rcl", tolower_c(*leaf)))
        bsrc = tprintf("%.*sb%s", (int) (leaf - path), path, leaf);

    for (c = deck; c && !reason; c = c->nextcard) {
        char *first, *made;

        if (!stb_live(c))
            continue;
        first = stb_first_token(c->line);
        if (ciprefix("a$poly$", first)) {
            made = stb_path(first + 7);
            if (ng_ideq(made, path))
                reason = tprintf("%s is a POLY source; place a voltage source", inst);
        } else {
            made = stb_path(first);
            if (bsrc && ng_ideq(made, bsrc))
                reason = tprintf("%s has a behavioural value and is replaced by other elements; place a voltage source", inst);
            else if (dot && tolower_c(*leaf) == 'x' &&
                     stb_ideqn(made, path, len) && made[len] == '.')
                reason = tprintf("%s is an instance of a subcircuit that is defined inside another one; its ports cannot be probes, name a terminal of one of its elements", inst);
        }
        tfree(made);
        tfree(first);
    }

    /* Under the case mode distinguish an element whose name differs from
     * this one only in case is another element, and the likely mistake:
     * the message points at it (doc/claude/decisions/0001-distinguish.md,
     * decision 2). In the other modes such a card would have been found. */
    if (!reason && twin)
        reason = tprintf("no instance named %s; '%s' differs only in case (casemode=distinguish)", inst, twin);
    if (!reason && inp_case_mode() == NG_CASE_DISTINGUISH)
        for (c = deck; c && !reason; c = c->nextcard) {
            char *first, *made;

            if (!stb_live(c))
                continue;
            first = stb_first_token(c->line);
            made = stb_path(first);
            /* case-lint: neither - a deliberate scan without regard to case */
            if (cieq(first, inst))
                reason = tprintf("no instance named %s; '%s' differs only in case (casemode=distinguish)", inst, first);
            /* case-lint: neither - the same scan, by the path */
            else if (cieq(made, path))
                reason = tprintf("no instance named %s; '%s' differs only in case (casemode=distinguish)", inst, made);
            tfree(made);
            tfree(first);
        }

    tfree(bsrc);
    tfree(path);

    return reason ? reason : tprintf("no instance named %s", inst);
}


#ifdef XSPICE

/* The word that ends before s on the line, with the brackets that open
 * a group of nodes skipped. Returns its start and sets *end, or returns
 * NULL if there is no word in front of s in its group. */
static char *
stb_word_before(char *line, char *s, char **end)
{
    while (s > line && (isspace_c(s[-1]) || s[-1] == '(' || s[-1] == '['))
        s--;
    *end = s;
    while (s > line && !isspace_c(s[-1]) && !strchr("()[]", s[-1]))
        s--;

    return s < *end ? s : NULL;
}


/* Is the port type between s and end one of an analog node? */
static bool
stb_analog_type(const char *s, const char *end)
{
    static const char *const types[] = {
        "v", "vd", "i", "id", "g", "gd", "h", "hd", "vnam"
    };
    size_t len = (size_t) (end - s);
    size_t i;

    for (i = 0; i < NUMELEMS(types); i++)
        /* case-lint: keyword - a port type is a word of the code model language */
        if (strlen(types[i]) == len && cieqn(types[i], s, len))
            return TRUE;

    return FALSE;
}


/* The node at 'at' on the line of a code model instance: is it on an
 * analog port? The type that is written in front of the port decides,
 * %v node or %vd(node node). Without one, the port has the type that
 * its model gives it, and which port of the model it is is not worked
 * out here: the node counts as analog only if no port of the model is
 * digital or of a type of the user. */
static bool
stb_analog_port(struct card *deck, char *line, char *at)
{
    char *w, *end, *model, *s;
    struct card *c;
    size_t len;
    int i, j;

    if (at > line && at[-1] == '~')
        return FALSE;

    w = stb_word_before(line, at, &end);
    if (w && *w != '%') {
        /* the second node of a differential port */
        w = stb_word_before(line, w, &end);
        if (w && (*w != '%' || end - w < 3 || tolower_c(end[-1]) != 'd'))
            w = NULL;
    }
    if (w && *w == '%')
        return stb_analog_type(w + 1, end);

    /* the model is the last word of the line */
    for (end = line + strlen(line); end > line && isspace_c(end[-1]); end--)
        ;
    for (s = end; s > line && !isspace_c(s[-1]); s--)
        ;
    model = copy_substring(s, end);
    len = strlen(model);

    for (c = deck; c; c = c->nextcard) {
        if (!c->line || !ciprefix(".model", c->line) || !isspace_c(c->line[6]))
            continue;
        s = skip_ws(c->line + 6);
        if (!stb_ideqn(s, model, len) || !isspace_c(s[len]))
            continue;
        s = skip_ws(s + len);
        for (end = s; *end && !isspace_c(*end) && *end != '('; end++)
            ;
        break;
    }
    tfree(model);
    if (!c)
        return FALSE;

    for (i = 0; i < ft_sim->numDevices; i++) {
        IFdevice *dev = ft_sim->devices[i];

        if (!dev || strlen(dev->name) != (size_t) (end - s) ||
            !ciprefix(dev->name, s))
            continue;
        for (j = 0; j < dev->num_conn; j++)
            if (dev->conn[j].default_port_type >= MIF_DIGITAL)
                return FALSE;
        return dev->num_conn > 0;
    }

    return FALSE;
}


/* Is the node on a port of a code model instance that is not analog? A
 * 0 V source there would make the reader add a bridge. */
static bool
stb_on_digital(struct card *deck, const char *node)
{
    struct card *c;

    for (c = deck; c; c = c->nextcard)
        if (stb_live(c) && tolower_c(*c->line) == 'a') {
            char *at = stb_occurs(c->line, node);

            if (at && !stb_analog_port(deck, c->line, at))
                return TRUE;
        }

    return FALSE;
}

#endif


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
     * probe: .probe makes such names. A current probe is one of .probe. */
    for (pp = &pending; (p = *pp) != NULL; ) {
        bool isname = FALSE;

        for (c = deck; c && !isname && !p->key && !p->current;
             c = c->nextcard)
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

    /* A port with a probe of stb and a current probe has two sources in
     * series: the elements are on the new node of the inner one, whose y
     * is the new node of the outer one. That node is on no card until
     * the inner source is placed, behind the same card. */
    for (;;) {
        bool more = FALSE;

        for (h = hits; h; h = h->next) {
            struct stb_ins *t;

            if (h->live || !h->port)
                continue;
            for (t = hits; t; t = t->next)
                if (t != h && t->live && t->port &&
                    ng_ideq(t->ynode, h->xnode)) {
                    h->live = TRUE;
                    h->card = t->card;
                    more = TRUE;
                    break;
                }
        }
        if (!more)
            break;
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

#ifdef XSPICE
    /* a port that leads to a digital port of a code model */
    for (h = hits; h; h = h->next) {
        if (!h->live || !h->used || !stb_on_digital(deck, h->xnode))
            continue;
        for (p = pending; p; p = p->next)
            if (p->ins == h) {
                p->ins = NULL;
                p->reason = copy("the port leads to a digital or mixed code model port; write the type of an analog port (%v) on the code model line, or place a voltage source");
            }
        h->used = FALSE;
    }
#endif

    /* a new node that no probe stands for goes back to its net, also as
     * the y of a source in series with it */
    for (h = hits; h; h = h->next)
        if (h->live && !h->used) {
            struct stb_ins *t;

            stb_replace_all(deck, h->xnode, h->ynode);
            for (t = hits; t; t = t->next)
                if (t != h && ng_ideq(t->ynode, h->xnode)) {
                    tfree(t->ynode);
                    t->ynode = copy(h->ynode);
                }
            h->live = FALSE;
        }

    /* elements */
    for (p = pending; p; p = p->next) {
        struct card *found;
        int n;

        if (p->key || p->reason || p->spawned)
            continue;
        found = stb_find_card(deck, p->inst, FALSE, &n);
        if (n == 0)
            /* by the path of its instances */
            found = stb_find_card(deck, p->inst, TRUE, &n);
        if (n == 0)
            p->reason = stb_why_missing(deck, p->inst, p->twin);
        else if (n > 1)
            p->reason = tprintf("%s is not a unique instance name", p->inst);
        else
            stb_element(deck, found, p);
    }

    /* the sources: for stb the new node is the positive one, for a
     * current probe the net, so that the branch is the current into the
     * terminal; for the probe of an instance of two terminals, at the
     * second, the new node again: the current out of it */
    for (h = hits; h; h = h->next)
        if (h->live && h->used) {
            bool into = h->current && !h->whole;

            insert_new_line(h->card,
                            tprintf("%s %s %s dc 0", h->vname,
                                    into ? h->ynode : h->xnode,
                                    into ? h->xnode : h->ynode),
                            0, h->card->linenum_orig,
                            h->card->linesource ? h->card->linesource : "internal");
        }

    stb_ic_twin(deck);

    /* A current probe is silent when it is inserted, as .probe is; when it
     * is not, the line says why. */
    for (p = pending; p; p = p->next) {
        if (p->spawned)
            continue;
        if (p->current) {
            if (p->posnote)
                fprintf(cp_out, "probe: I(%s): %s is taken as a position, not as the port named %s\n",
                        p->text, p->term, p->term);
            if (!p->ins)
                fprintf(cp_out, "probe: I(%s) not inserted: %s\n",
                        p->text, p->reason ? p->reason : "no such terminal");
            continue;
        }
        if (p->posnote)
            fprintf(cp_out, "stb: probe %s: %s is taken as a position, not as the port named %s\n",
                    p->text, p->term, p->term);
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
}


/* The deck is flat and its probes are inserted: the .save of the vectors
 * of the current probes, appended to the list of the dot cards of the
 * deck (inp.c). .probe has added '.save all' already if the deck has no
 * save of its own. */
wordlist *
inp_stb_saves(wordlist *wl)
{
    struct stb_ins *h;
    char *line = NULL;

    for (h = hits; h; h = h->next) {
        char *more;

        if (!h->current || !h->live || !h->used || !h->cname)
            continue;
        more = line ? tprintf("%s %s#branch", line, h->cname) :
                      tprintf(".save %s#branch", h->cname);
        tfree(line);
        line = more;
    }
    if (line)
        wl = wl_append(wl, wl_cons(line, NULL));

    return wl;
}


/* The circuit has been made: the source of each current probe gets the
 * name of its probe, <path>:<terminal>, so that the branch it makes when
 * the circuit is set up is <path>:<terminal>#branch. The name is entered
 * into the table of the names of the circuit in place of the old one,
 * unless an instance has it already (an element of two terminals at the
 * top level, which gives its name to its probe as .probe does). */
void
inp_stb_rename(CKTcircuit *ckt, INPtables *tab)
{
    struct stb_ins *h;

    if (!ckt || !tab)
        return;

    for (h = hits; h; h = h->next) {
        GENinstance *inst;
        char *name;

        if (!h->current || !h->live || !h->used || !h->cname)
            continue;
        inst = ft_sim->findInstance(ckt, h->vname);
        if (!inst)
            continue;
        name = copy(h->cname);
        INPinsert(&name, tab);
        nghash_delete(ckt->DEVnameHash, inst->GENname);
        inst->GENname = name;
        if (!nghash_find(ckt->DEVnameHash, name))
            nghash_insert(ckt->DEVnameHash, name, inst);
        tfree(h->vname);
        h->vname = copy(name);
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


/* Does the word name the terminal that the source was inserted at? */
static bool
stb_same_terminal(const struct stb_ins *h, const char *term)
{
    const char *letters, *l;

    if (stb_all_digits(term))
        return strlen(term) < 9 && atoi(term) == h->pos;
    if (h->port)
        return h->tname && ng_ideq(h->tname, term);
    letters = stb_letters(*h->inst);
    l = (letters && term[0] && !term[1]) ?
        strchr(letters, tolower_c(term[0])) : NULL;

    return l && l - letters == h->pos - 1;
}


/* For the analysis: the source that was inserted for the terminal probe.
 * Returns 0 and the name of the source, 1 and the reason if the probe was
 * collected but could not be inserted, -1 if it was not collected. A
 * probe that was not collected in this spelling is found if it names a
 * terminal that has a source. */
int
inp_stb_lookup(CKTcircuit *ckt, const char *probe, const char **vname,
               const char **reason)
{
    struct stb_probe *p;
    char *text, *colon;
    int ret = -1;

    if (!ft_curckt || ft_curckt->ci_ckt != ckt)
        return -1;

    /* the word, in the spelling the deck has its names in */
    text = copy(probe);
    stb_fold(text);

    /* a current probe of .probe is none of the analysis: its source has
     * the other sign */
    for (p = ft_curckt->ci_stbprobes; p && ret < 0; p = p->next)
        if (!p->current && ng_ideq(p->text, text)) {
            if (p->ins) {
                *vname = p->ins->vname;
                ret = 0;
            } else {
                *reason = p->reason;
                ret = 1;
            }
        }

    colon = strrchr(text, ':');
    if (ret < 0 && colon && colon > text && colon[1]) {
        char *inst = copy_substring(text, colon);

        for (p = ft_curckt->ci_stbprobes; p && ret < 0; p = p->next)
            if (!p->current && p->ins && stb_same_terminal(p->ins, colon + 1) &&
                (ng_ideq(p->ins->inst, inst) ||
                 stb_names(p->ins->inst, inst))) {
                *vname = p->ins->vname;
                ret = 0;
            }
        tfree(inst);
    }

    tfree(text);
    return ret;
}
