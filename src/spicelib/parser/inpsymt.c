/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Wayne A. Christopher, U. C. Berkeley CAD Group
**********/

/*
 * Stuff for the terminal and node symbol tables.
 * Defined: INPtabInit, INPinsert, INPtermInsert, INPtabEnd
 */
/* MW. Special INPinsertNofree for routines from spiceif.c and outif.c */

#include "ngspice/ngspice.h"
#include <stdio.h>		/* Take this out soon. */
#include "ngspice/ifsim.h"
#include "ngspice/iferrmsg.h"
#include "ngspice/inpdefs.h"
#include "ngspice/cpstd.h"
#include "ngspice/fteext.h"
#include "inpxx.h"


static int hash(char *name, int tsize);

/* Identity of an interned token and a stored entry.  Under preserve two
   spellings of one identifier are still one identifier, so the comparison
   ignores case while t_ent keeps the spelling first seen.  In fold mode both
   sides are already lower case, so strcmp is retained to keep the default
   byte identical for the generated node names that carry upper case whatever
   the deck says, such as q1#collCX.  Under distinguish the same strcmp is
   what makes two case-variant nets two nets: this function, reached from
   INPtermInsert(), is where that is decided. */

static bool ent_eq(const char *token, const char *ent)
{
    return inp_case_exact_ids() ? (strcmp(token, ent) == 0) : cieq(token, ent);
}


/* Is this byte a boundary between one word of a card and the next?

   Deliberately narrow: only characters that cannot occur inside a name a deck
   writes.  '+', '-', '*' and '/' stay inside words, because a node name may
   contain them.  That costs nothing measurable: a node inside an expression
   is written V(name) or I(name) and '(' and ')' are boundaries, so the name
   is still a whole word.  What it does cost is a name written with no
   delimiter at all beside it -- '[' and ']' are not boundaries either, so
   'a1 [in1 in2] [out] m' yields no spelling where 'a1 [ in1 in2 ] [ out ] m'
   yields one.  Widening the set is the direction that could recover a WRONG
   spelling by splitting a real node name, so it is not widened without a
   measured line asking for it.
   doc/claude/decisions/0018-node-name-collision-report.md decision 3 gaps
   3 and 4. */

static bool card_word_boundary(char c)
{
    return c == '\0' || isspace_c(c) || c == '(' || c == ')' || c == ',' ||
            c == '=' || c == '\'' || c == '"' || c == '{' || c == '}' ||
            c == ';';
}


/* The spelling the deck used for `token` on the card being parsed, or NULL
   when that card cannot answer.  doc/codex/issues/0068.

   Under fold the token handed to term_insert() has already been lower cased
   -- inp_readall() rewrites the card as it reads it, before comment
   stripping, before continuation stitching, before .include, before numparam
   and before subcircuit expansion -- so the symbol table can never see a
   second spelling and the report has to be reconstructed from the one copy of
   the card that predates the fold.  That is struct card's line_case.

   It is only ever the card's own text.  A card the preprocessor built has
   none, so a subcircuit body expanded into an instance and a line numparam
   rewrote both answer NULL rather than answering from the text they were made
   from: a body card's `R1 a b 1k` describes the formal pins, and the node
   actually being interned is the caller's actual or a scoped x1.a, so an
   answer from there would be a spelling for a different node.  A missed
   report is this issue's stated gap; a wrong one is the thing it must not do.

   The largest instance of that gap is the B source, and it is worth naming
   here because nothing about a B card looks preprocessor-built from this end:
   inp_bsource_compat() (src/frontend/inpcom.c) comments every B card out and
   inserts a replacement, so no node named on one -- neither terminal, and no
   V() or I() in the expression -- can supply a spelling under fold.
   inp_compat() does the same to an E, G, R, C or L card whose value is an
   expression.  236 B cards in 53 of the tree's 875 decks.
   doc/claude/decisions/0018-node-name-collision-report.md decision 3 gap 3.

   The search starts after the first whitespace-delimited field, and that is
   what makes the first match the right one.  A device card's nodes are fields
   2..n+1, immediately after the instance name, so nothing that could match a
   node token stands before the node itself except that instance name -- which
   is a different namespace, and is acceptance criterion 4 of the issue.  A
   later field can hold a model name or a keyword spelled the other way
   (`ic=OUT` beside a node `Out`), and taking the first match rather than
   complaining about a disagreement is what keeps that silent. */

static char *card_spelling(const char *raw, const char *token)
{
    size_t n = strlen(token);
    const char *p;

    if (!raw || n == 0)
        return NULL;

    for (p = raw; *p && isspace_c(*p); p++)
        ;
    for (; *p && !isspace_c(*p); p++)
        ;

    for (; *p; p++) {
        if (card_word_boundary(*p) || !card_word_boundary(p[-1]))
            continue;
        /* neither an identity test nor a keyword test: it locates a word in
           the card's own pre-fold text.  The token has been folded and the
           text has not, so this is the one comparison in the tree that must
           ignore case *in fold mode*; what it returns is a spelling, and the
           identity question was answered by ent_eq() before it was asked. */
        /* case-lint: neither - locates the folded token in the card's own pre-fold text */
        if (cieqn(p, token, n) && card_word_boundary(p[n]))
            return copy_substring(p, p + n);
    }

    return NULL;
}


/* The spelling for a node about to be interned, under the only mode that
   needs one recovered.  Kept separate from card_spelling() so that the two
   questions -- "does this mode hide the deck's spelling?" and "what did the
   card say?" -- are asked in that order and only once each. */

static char *node_spelling(const char *token)
{
    if (!inp_case_folding())
        return NULL;
    return card_spelling(INPcurrent_card ? INPcurrent_card->line_case : NULL,
            token);
}


/* Initialize the symbol tables. */

INPtables *INPtabInit(int numlines)
{
    INPtables *tab;

    tab = TMALLOC(INPtables, 1);
    tab->INPsymtab = TMALLOC(struct INPtab *, numlines / 4 + 1);
    ZERO(tab->INPsymtab, (numlines / 4 + 1) * sizeof(struct INPtab *));
    tab->INPtermsymtab = TMALLOC(struct INPnTab *, numlines);
    ZERO(tab->INPtermsymtab, numlines * sizeof(struct INPnTab *));
    tab->INPsize = numlines / 4 + 1;
    tab->INPtermsize = numlines;
    return (tab);
}

/* insert 'token' into the terminal symbol table */
/* create a NEW NODE and return a pointer to it in *node */

/* 'unclaimed' says the caller is a reference and not a definition: it names a
   node it expects to already exist, rather than putting one on the netlist.
   It has to keep creating on a miss, because a reference can be parsed before
   the card that defines what it names -- V(mid) on the first card of a deck is
   a legal forward reference to a node no card has defined yet.  The bit is
   therefore not a refusal; it records which nodes were born from a reference,
   so that INPtermCaseCheck() can ask at the end of the parse which of them no
   card ever defined.  A definition clears it, whichever order the two arrive
   in.

   Every caller of INPtermInsertRef(), not just the B source: mkvnode()
   (src/spicelib/parser/inpptree.c) for a V()/I() inside an expression,
   INPgetValue() (inpgval.c) for an IF_NODE value, and dot_noise(),
   dot_tf(), dot_sens(), dot_pss() and dot_hb() (inp2dot.c) for the output
   node of those analyses.  That distinction matters to the fold-mode report:
   a B card is rebuilt by inp_bsource_compat() and carries no line_case, but
   a '.tf v(MISS)' card is the deck's own text and carries one, so an
   unclaimed entry CAN hold a recovered spelling and a case twin under fold.
   tests/regression/misc/node-case-collision-report.cir NMPAIR is that shape
   and it is what holds the outer t_unclaimed guard below. */

static int term_insert(CKTcircuit *ckt, char **token, INPtables * tab,
                       CKTnode **node, bool unclaimed)
{
    int key;
    int error;
    struct INPnTab *t;
    char *spelling = node_spelling(*token);

    key = hash(*token, tab->INPtermsize);
    for (t = tab->INPtermsymtab[key]; t; t = t->t_next) {
        if (ent_eq(*token, t->t_ent)) {
            /* Under preserve this is the moment a deck that wrote one node
               two ways becomes one node, and it is the only place both
               spellings are in scope at once: the entry keeps the first and
               the caller hands in the second.  Under fold the two spellings
               come back from the two cards' pre-fold text instead, and the
               same two fields hold them.  Keep the second so that
               INPtermCaseCheck() reports the pair once at the end of the
               parse rather than once per card.  Neither strcmp is an identity
               test: ent_eq() has already decided that, and what is asked here
               is whether the two names the deck wrote are the same *text*.
               doc/codex/issues/0068 */
            if (spelling) {
                if (!t->t_spelling) {
                    t->t_spelling = spelling;
                    spelling = NULL;
                }
                else if (!t->t_casetwin &&
                        strcmp(t->t_spelling, spelling) != 0) { /* case-lint: neither - two spellings as text, ent_eq() decided identity */
                    t->t_casetwin = spelling;
                    spelling = NULL;
                }
            }
            else if (!t->t_casetwin &&
                    strcmp(*token, t->t_ent) != 0) /* case-lint: neither - two spellings as text, ent_eq() decided identity */
                t->t_casetwin = copy(*token);
            FREE(spelling);
            FREE(*token);
            *token = t->t_ent;
            if (!unclaimed)
                t->t_unclaimed = FALSE;
            if (node)
                *node = t->t_node;
            return (E_EXISTS);
        }
    }
    t = TMALLOC(struct INPnTab, 1);
    if (t == NULL) {
        FREE(spelling);
        return (E_NOMEM);
    }
    ZERO(t, struct INPnTab);
    error = ft_sim->newNode (ckt, &(t->t_node), *token);
    if (error) {
        FREE(spelling);
        return (error);
    }
    if (node)
        *node = t->t_node;
    t->t_ent = *token;
    t->t_spelling = spelling;
    t->t_unclaimed = unclaimed;
    t->t_next = tab->INPtermsymtab[key];
    tab->INPtermsymtab[key] = t;
    return (OK);
}


int INPtermInsert(CKTcircuit *ckt, char **token, INPtables * tab, CKTnode **node)
{
    return term_insert(ckt, token, tab, node, FALSE);
}


/* as INPtermInsert(), but the caller is resolving a name rather than defining
   one; see term_insert() above */

int INPtermInsertRef(CKTcircuit *ckt, char **token, INPtables * tab, CKTnode **node)
{
    return term_insert(ckt, token, tab, node, TRUE);
}


/* Report every node that a reference created and that no card ever defined.
   That is decision 2 of doc/claude/decisions/0001-distinguish.md applied to
   the parser: silence when a name is being defined, a warning when a name is
   being resolved and the resolution fails.

   It has to run after the whole netlist is parsed rather than at the
   reference, because at the reference a miss is indistinguishable from a
   forward reference to a card further down the deck.

   One scan, two reports, and which one a node gets is decided by whether a
   name differing from it only in case is defined:

   - with such a twin, the deck almost certainly meant the twin, so the
     message names both spellings.  That is the near-miss of
     doc/claude/decisions/0002-deferred-node-resolution-check.md and it is
     guarded on distinguish.  The guard is belt and braces rather than
     policy: under preserve ent_eq() is case insensitive, so two entries
     differing only in case cannot both be in the table, and under fold the
     reader has lowercased every card.
   - with no twin the reference is a plain undefined node, and that report is
     mode independent, because nothing about it is about case.  See
     doc/claude/decisions/0008-undefined-node-diagnostic.md; the name of this
     function is kept because four records cite it. */

/* One node name written two ways, reported once per colliding pair per parse.
   Both spellings are definitions, which is the case
   doc/claude/decisions/0001-distinguish.md decision 2 declined to report;
   doc/codex/issues/0068 is why that was re-opened and what the argument is.

   The sentence names the outcome rather than a mistake, because in every mode
   the deck is the ambiguous thing and the mode is only the reading taken:
   fold and preserve merge the two spellings into one node, distinguish keeps
   them apart as two.  That is the whole difference between the three
   sentences, and it is why one detection serves all three modes.  A user who
   meant two nodes under distinguish is told they got two, which is a
   confirmation; under the other two modes the same user learns that the merge
   happened at all.

   'node names' leads the line so that it cannot be confused, by a reader or
   by a deck's strstr(), with the two reports above, which begin 'no node
   named' and are about a name that failed to resolve rather than about a pair
   that both defined. */

static void report_case_collision(const char *first, const char *second)
{
    fprintf(cp_err,
            "Warning: node names '%s' and '%s' differ only in case and name %s (casemode=%s)\n",
            first, second,
            inp_case_mode() == NG_CASE_DISTINGUISH ? "two nodes" : "one node",
            inp_case_mode_name());
}


void INPtermCaseCheck(INPtables *tab)
{
    int i;
    struct INPnTab *t, *u, *twin;

    for (i = 0; i < tab->INPtermsize; i++)
        for (t = tab->INPtermsymtab[i]; t; t = t->t_next) {
            if (!t->t_unclaimed)
                continue;
            twin = NULL;
            if (inp_case_mode() == NG_CASE_DISTINGUISH)
                /* hash() folds the bucket key whenever the reader is not
                   folding, so a name differing from t only in case is
                   necessarily in this same bucket and the scan does not have
                   to walk the table */
                for (u = tab->INPtermsymtab[i]; u; u = u->t_next)
                    if (u != t && !u->t_unclaimed && cieq(t->t_ent, u->t_ent)) {
                        twin = u;
                        break;
                    }
            if (twin)
                fprintf(cp_err,
                        "Warning: no node named '%s'; '%s' differs only in case (casemode=distinguish)\n",
                        t->t_ent, twin->t_ent);
            else
                fprintf(cp_err,
                        "Warning: no node named '%s'; it is referenced but no card defines it\n",
                        t->t_ent);
        }

    /* The second scan is the pair report of doc/codex/issues/0068, and it is
       about the nodes the first scan skips: those that a card defines.  A
       node nothing defines is already reported above, as a near miss or as an
       undefined node, and reporting it again here as half of a pair would say
       the same thing twice in different words -- hence the t_unclaimed guards
       on both sides.

       Two shapes, because the two spellings end up in different places.
       Under preserve they are one entry and term_insert() kept the second
       spelling on it.  Under distinguish they are two entries, necessarily in
       the same bucket, because hash() folds the bucket key whenever the
       reader is not folding.  Under fold neither shape can occur: the reader
       lowercased every card before the first token was interned, so this
       table has no record that a second spelling was ever written, and the
       report is owed by whatever can still see it. */

    for (i = 0; i < tab->INPtermsize; i++)
        for (t = tab->INPtermsymtab[i]; t; t = t->t_next) {
            if (t->t_unclaimed)
                continue;
            if (t->t_casetwin) {
                report_case_collision(
                        t->t_spelling ? t->t_spelling : t->t_ent,
                        t->t_casetwin);
                continue;
            }
            if (inp_case_mode() != NG_CASE_DISTINGUISH)
                continue;
            /* The chain is prepended to, so an entry further along it was
               interned earlier: naming u first puts the two spellings in the
               order the deck wrote them. */
            for (u = t->t_next; u; u = u->t_next)
                /* case-lint: neither - the deliberate case-insensitive pair scan of doc/codex/issues/0068 */
                if (!u->t_unclaimed && cieq(t->t_ent, u->t_ent))
                    report_case_collision(u->t_ent, t->t_ent);
        }
}


/* insert 'token' into the terminal symbol table */
/* USE node as the node pointer */


int INPmkTerm(CKTcircuit *ckt, char **token, INPtables * tab, CKTnode **node)
{
    int key;
    struct INPnTab *t;

    NG_IGNORE(ckt);

    key = hash(*token, tab->INPtermsize);
    for (t = tab->INPtermsymtab[key]; t; t = t->t_next) {
        if (ent_eq(*token, t->t_ent)) {
            FREE(*token);
            *token = t->t_ent;
            if (node)
                *node = t->t_node;
            return (E_EXISTS);
        }
    }
    t = TMALLOC(struct INPnTab, 1);
    if (t == NULL)
        return (E_NOMEM);
    ZERO(t, struct INPnTab);
    t->t_node = *node;
    t->t_ent = *token;
    t->t_next = tab->INPtermsymtab[key];
    tab->INPtermsymtab[key] = t;
    return (OK);
}

/* insert 'token' into the terminal symbol table as a name for ground*/

int INPgndInsert(CKTcircuit *ckt, char **token, INPtables * tab, CKTnode **node)
{
    int key;
    int error;
    struct INPnTab *t;

    key = hash(*token, tab->INPtermsize);
    for (t = tab->INPtermsymtab[key]; t; t = t->t_next) {
        if (ent_eq(*token, t->t_ent)) {
            FREE(*token);
            *token = t->t_ent;
            if (node)
                *node = t->t_node;
            return (E_EXISTS);
        }
    }
    t = TMALLOC(struct INPnTab, 1);
    if (t == NULL)
        return (E_NOMEM);
    ZERO(t, struct INPnTab);
    error = ft_sim->groundNode (ckt, &(t->t_node), *token);
    if (error)
        return (error);
    if (node)
        *node = t->t_node;
    t->t_ent = *token;
    t->t_next = tab->INPtermsymtab[key];
    tab->INPtermsymtab[key] = t;
    return (OK);
}

/* retrieve 'token' from the symbol table */

int INPretrieve(char **token, INPtables * tab)
{
    struct INPtab *t;
    int key;

    key = hash(*token, tab->INPsize);
    for (t = tab->INPsymtab[key]; t; t = t->t_next)
        if (ent_eq(*token, t->t_ent)) {
            *token = t->t_ent;
            return (OK);
        }
    return (E_BADPARM);
}


/* insert 'token' into the symbol table */

int INPinsert(char **token, INPtables * tab)
{
    struct INPtab *t;
    int key;

    key = hash(*token, tab->INPsize);
    for (t = tab->INPsymtab[key]; t; t = t->t_next)
        if (ent_eq(*token, t->t_ent)) {
            FREE(*token);
            *token = t->t_ent;
            return (E_EXISTS);
        }
    t = TMALLOC(struct INPtab, 1);
    if (t == NULL)
        return (E_NOMEM);
    ZERO(t, struct INPtab);
    t->t_ent = *token;
    t->t_next = tab->INPsymtab[key];
    tab->INPsymtab[key] = t;
    return (OK);
}


/* MW. insert 'token' into the symbol table but no free() token pointer.
*	Calling routine should take care for this */

int INPinsertNofree(char **token, INPtables * tab)
{
    struct INPtab *t;
    int key;

    key = hash(*token, tab->INPsize);
    for (t = tab->INPsymtab[key]; t; t = t->t_next)
        if (ent_eq(*token, t->t_ent)) {

            /* MW. We can't touch memory pointed by token now */
            *token = t->t_ent;
            return (E_EXISTS);
        }
    t = TMALLOC(struct INPtab, 1);
    if (t == NULL)
        return (E_NOMEM);
    ZERO(t, struct INPtab);
    t->t_ent = *token;
    t->t_next = tab->INPsymtab[key];
    tab->INPsymtab[key] = t;
    return (OK);
}

/* remove 'token' from the symbol table */
int INPremove(char *token, INPtables * tab)
{
    struct INPtab *t, **prevp;
    int key;

    key = hash(token, tab->INPsize);
    prevp = &tab->INPsymtab[key];
    for (t = *prevp; t && token != t->t_ent; t = t->t_next)
        prevp = &t->t_next;
    if (!t)
        return OK;

    *prevp = t->t_next;
    tfree(t->t_ent);
    tfree(t);

    return OK;
}

/* remove 'token' from the symbol table */
int INPremTerm(char *token, INPtables * tab)
{
    struct INPnTab *t, **prevp;
    int key;

    key = hash(token, tab->INPtermsize);
    prevp = &tab->INPtermsymtab[key];
    for (t = *prevp; t && token != t->t_ent; t = t->t_next)
        prevp = &t->t_next;
    if (!t)
        return OK;

    *prevp = t->t_next;
    tfree(t->t_ent);
    tfree(t);

    return OK;
}

/* Free the space used by the symbol tables. */

void INPtabEnd(INPtables * tab)
{
    struct INPtab *t, *lt;
    struct INPnTab *n, *ln;
    int i;

    for (i = 0; i < tab->INPsize; i++)
        for (t = tab->INPsymtab[i]; t; t = lt) {
            lt = t->t_next;
            FREE(t->t_ent);
            FREE(t);
        }
    FREE(tab->INPsymtab);
    for (i = 0; i < tab->INPtermsize; i++)
        for (n = tab->INPtermsymtab[i]; n; n = ln) {
            ln = n->t_next;
            FREE(n->t_ent);
            FREE(n->t_spelling);
            FREE(n->t_casetwin);
            FREE(n);		/* But not t_node ! */
        }
    FREE(tab->INPtermsymtab);
    FREE(tab);
    return;
}

static int hash(char *name, int tsize)
{
    unsigned int hash = 5381;
    char c;

    if (inp_case_folding()) {
        while ((c = *name++) != '\0')
            hash = (hash * 33) ^ (unsigned) c;
    } else {
        /* fold the bucket key so that the case insensitive ent_eq() above
           can only ever be asked about entries in the same bucket */
        while ((c = *name++) != '\0')
            hash = (hash * 33) ^ (unsigned) tolower_c(c);
    }

    return (int) (hash % (unsigned) tsize);
}

/* Just tests for the existence of a node. If node is found, its
   token and node adresses are fed back.
   Return value 0 if no node is found, E_EXISTS if its already there */
int INPtermSearch(CKTcircuit* ckt, char** token, INPtables* tab, CKTnode** node)
{
    int key;
    struct INPnTab* t;
    NG_IGNORE(ckt);

    key = hash(*token, tab->INPtermsize);
    for (t = tab->INPtermsymtab[key]; t; t = t->t_next) {
        if (ent_eq(*token, t->t_ent)) {
            FREE(*token);
            *token = t->t_ent;
            if (node)
                *node = t->t_node;
            return (E_EXISTS);
        }
    }
    return (0);
}
