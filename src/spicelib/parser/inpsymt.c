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
   Only mkvnode() does that, and it has to keep creating on a miss because a
   B source is parsed before the cards below it, so V(mid) on the first card
   of a deck is a legal forward reference to a node no card has defined yet.
   The bit is therefore not a refusal; it records which nodes were born from a
   reference, so that INPtermCaseCheck() can ask at the end of the parse which
   of them no card ever defined.  A definition clears it, whichever order the
   two arrive in. */

static int term_insert(CKTcircuit *ckt, char **token, INPtables * tab,
                       CKTnode **node, bool unclaimed)
{
    int key;
    int error;
    struct INPnTab *t;

    key = hash(*token, tab->INPtermsize);
    for (t = tab->INPtermsymtab[key]; t; t = t->t_next) {
        if (ent_eq(*token, t->t_ent)) {
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
    if (t == NULL)
        return (E_NOMEM);
    ZERO(t, struct INPnTab);
    error = ft_sim->newNode (ckt, &(t->t_node), *token);
    if (error)
        return (error);
    if (node)
        *node = t->t_node;
    t->t_ent = *token;
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
