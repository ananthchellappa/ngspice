/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Wayne A. Christopher, U. C. Berkeley CAD Group
**********/

/*
 * Routines for dealing with the vector database.
 */

#include "ngspice/ngspice.h"
#include "ngspice/cpdefs.h"
#include "ngspice/ftedefs.h"
#include "ngspice/dvec.h"
#include "ngspice/sim.h"
#include "ngspice/stringskip.h"

#include "circuits.h"
#include "completion.h"
#include "variable.h"
#include "dimens.h"
#include "../misc/misc_time.h"
#include "vectors.h"
#include "ngspice/dstring.h"
#include "plotting/plotting.h"
#ifdef XSPICE
#include "ngspice/evt.h"
#include "ngspice/mif.h"
#endif

static struct dvec *findvec_all(struct plot *pl);
static struct dvec *findvec_allv(struct plot *pl);
static struct dvec *findvec_alli(struct plot *pl);
static struct dvec *findvec_ally(struct plot *pl);
#ifdef XSPICE
static struct dvec *findvec_alle(void);
#endif
static struct dvec *find_permanent_vector_by_name(
        NGHASHPTR pl_lookup_table, char *name, const char *typed);
static bool vec_wrapped_name_eq(const char *v_name, const char *typed);
static void vec_warn_case_near_miss(NGHASHPTR pl_lookup_table,
        const char *word);
static enum ALL_TYPE_ENUM get_all_type(const char *word);
static bool plot_prefix(const char *pre, const char *str);
static struct dvec *vec_fromplot_maybe_report(char *word, struct plot *plot,
        bool report_case_miss);
static struct dvec *vec_get_maybe_report(const char *vec_name,
        bool report_case_miss);

#ifdef XSPICE
extern int EVTswitch_plot(CKTcircuit* ckt, const char* plottypename);
struct dvec *EVTfindvec(char *node);
#endif


static void vec_rebuild_lookup_table(struct plot *pl)
{
    if (pl->pl_lookup_table) { /* existing table */
        nghash_empty(pl->pl_lookup_table, NULL, NULL);
    }
    else { /* new table */
        int cnt = 0; /* count entries */
        struct dvec *d; /* dynamic vector */
        for (d = pl->pl_dvecs; d; d = d->v_next) { /* get # vec */
            cnt++;
        }
        pl->pl_lookup_table = nghash_init(cnt);
        /* allow multiple entries */
        nghash_unique(pl->pl_lookup_table, FALSE);
    }

    {
        /* Access lookup table directly for speed */
        NGHASHPTR lookup_p = pl->pl_lookup_table;
        DS_CREATE(dbuf, 200); /* make dynamic buffer */
        struct dvec *d; /* dynamic vector */
        for (d = pl->pl_dvecs; d; d = d->v_next) {
            ds_clear(&dbuf);
            if (ds_cat_str_case(&dbuf, d->v_name, ds_case_lower) != DS_E_OK) {
                fprintf(stderr, "Error: DS could not add string %s\n", d->v_name);
                controlled_exit(-1);
            }
            char * const lower_name = ds_get_buf(&dbuf);
            nghash_insert(lookup_p, lower_name, d); /* add lower-cased name */
        } /* end of loop over vectors */
        ds_free(&dbuf);
    }

    pl->pl_lookup_valid = TRUE; /* now lookup table valid */
} /* end of function vec_rebuild_lookup_table */



enum ALL_TYPE_ENUM {
    ALL_TYPE_NONE,
    ALL_TYPE_ALL,
    ALL_TYPE_ALLV,
    ALL_TYPE_ALLI,
    ALL_TYPE_ALLY,
    ALL_TYPE_ALLE
};


/* Efficient identification of "all", "allv", "alli", "ally", "alle", and anything
 * else */
static enum ALL_TYPE_ENUM get_all_type(const char *word)
{
    /* Check for start of "all" */
    if (tolower_c(word[0]) != 'a') {
        return ALL_TYPE_NONE;
    }
    if (tolower_c(word[1]) != 'l') {
        return ALL_TYPE_NONE;
    }
    if (tolower_c(word[2]) != 'l') {
        return ALL_TYPE_NONE;
    }

    /* It may be some type of all */
    switch (tolower_c(word[3])) {
    case '\0':
        return ALL_TYPE_ALL;
    case 'v':
        if (word[4] == '\0') {
            return ALL_TYPE_ALLV;
        }
        else {
            return ALL_TYPE_NONE;
        }
    case 'i':
        if (word[4] == '\0') {
            return ALL_TYPE_ALLI;
        }
        else {
            return ALL_TYPE_NONE;
        }
    case 'y':
        if (word[4] == '\0') {
            return ALL_TYPE_ALLY;
        }
        else {
            return ALL_TYPE_NONE;
        }
    case 'e':
        if (word[4] == '\0') {
            return ALL_TYPE_ALLE;
        }
        else {
            return ALL_TYPE_NONE;
        }
    default:
        return ALL_TYPE_NONE;
    } /* end of swith over char after "all" */
} /* end of function get_all_type */



/* Find a named vector in a plot. We are careful to copy the vector if
 * v_link2 is set, because otherwise we will get screwed up.
 *
 * report_case_miss carries decision 2 of doc/claude/decisions/0001-distinguish.md
 * down from the caller: a lookup whose miss is a resolution failure says so
 * when a case variant is present, and a lookup whose miss is the ordinary
 * outcome -- a name being defined, or a question about whether a name exists
 * at all -- does not.  findvec() cannot tell the two apart on its own, which
 * is doc/codex/issues/0034.  */
static struct dvec *findvec(char *word, struct plot *pl, bool report_case_miss)
{
    /* If no plot, cannot find */
    if (pl == NULL) {
        return NULL;
    }

    /* Identify and handle special cases all, allv, alli, ally, alle */
    switch (get_all_type(word)) {
    case ALL_TYPE_ALL:
        return findvec_all(pl);
    case ALL_TYPE_ALLV:
        return findvec_allv(pl);
    case ALL_TYPE_ALLI:
        return findvec_alli(pl);
    case ALL_TYPE_ALLY:
        return findvec_ally(pl);
#ifdef XSPICE
    case ALL_TYPE_ALLE:
        return findvec_alle();
#endif
    default: /* case ALL_TYPE_NOT_ALL -- not some type of ALL */
        break;
    }

    /* The find is not for one of the "all" cases */
    if (!pl->pl_lookup_valid) {
        /* Table lookup not valid, so rebuild to make valid */
        vec_rebuild_lookup_table(pl);
    }

    DS_CREATE(dbuf, 200); /* make dynamic buffer */
    if (ds_cat_str_case(&dbuf, word, ds_case_lower) != DS_E_OK) {
        fprintf(stderr, "Error: DS could not add string %s\n", word);
        controlled_exit(-1);
    }
    char * const lower_name = ds_get_buf(&dbuf);
    NGHASHPTR pl_lookup_table = pl->pl_lookup_table;
    struct dvec *d = find_permanent_vector_by_name(pl_lookup_table,
            lower_name, word);

    /* If the vector was not using the lowercased name, try finding it as
     * v(lowercased name) */
    if (!d) {
        DS_CREATE(tbuf, 200); /* the same name with the deck's own spelling */
        ds_clear(&dbuf);
        bool f_ok = ds_cat_str(&dbuf, "v(") == DS_E_OK;
        f_ok &= ds_cat_str_case(&dbuf, word,
                ds_case_lower) == DS_E_OK;
        f_ok &= ds_cat_char(&dbuf, ')') == DS_E_OK;
        /* The 'v' and the parentheses are this function's, not the caller's,
         * so under distinguish the stored name has to spell them the way they
         * are spelled here. A vector actually named V(Out) is reached by the
         * first lookup above, where the caller typed the V. */
        f_ok &= ds_cat_str(&tbuf, "v(") == DS_E_OK;
        f_ok &= ds_cat_str(&tbuf, word) == DS_E_OK;
        f_ok &= ds_cat_char(&tbuf, ')') == DS_E_OK;
        if (!f_ok) {
            fprintf(stderr, "Error: DS could not add string V() around %s\n", word);
            controlled_exit(-1);
        }
        char * const node_name = ds_get_buf(&dbuf);
        d = find_permanent_vector_by_name(pl_lookup_table, node_name,
                ds_get_buf(&tbuf));
        ds_free(&tbuf);
    }

    ds_free(&dbuf);

#ifdef XSPICE
    /* gtri - begin - Add processing for getting event-driven vector */

    if (!d) {
        d = EVTfindvec(word);
    }

    /* gtri - end   - Add processing for getting event-driven vector */
#endif

    if (!d && report_case_miss && inp_case_mode() == NG_CASE_DISTINGUISH) {
        vec_warn_case_near_miss(pl_lookup_table, word);
    }

    if (d && d->v_link2) {
        d = vec_copy(d);
        vec_new(d);
    }

    return d;
} /* end of function findvec */



/* Macro taking a function name and vector filter as arguments that
 * generates the function that applies the filter */
#define FINDVEC_ALL_GEN(fun_name, filter)\
static struct dvec *fun_name(struct plot *pl)\
{\
    struct dvec *d, *newv = NULL, *end = NULL, *v;\
    for (d = pl->pl_dvecs; d; d = d->v_next) {\
        if (filter) {\
            if (d->v_link2) {\
                v = vec_copy(d);\
                vec_new(v);\
            }\
            else {\
                v = d;\
            }\
            if (end) {\
                end->v_link2 = v;\
            }\
            else {\
                newv = v;\
            }\
            end = v;\
        }\
    } /* end of loop over vectors in plot */\
\
    return newv;\
} /* end of function */

/* Generate the functions for each filter */
FINDVEC_ALL_GEN(findvec_all, d->v_flags & VF_PERMANENT)
FINDVEC_ALL_GEN(findvec_allv,
        (d->v_flags & VF_PERMANENT) && (d->v_type == SV_VOLTAGE))
FINDVEC_ALL_GEN(findvec_alli,
        (d->v_flags & VF_PERMANENT) && (d->v_type == SV_CURRENT))
FINDVEC_ALL_GEN(findvec_ally,
        (d->v_flags & VF_PERMANENT) &&
                (!vec_name_eq(d->v_name, pl->pl_scale->v_name)))

#if defined (XSPICE) && defined (SIMULATOR) /* SIMULATOR: disable old app nutmeg */
/* special case for finding all event nodes and return them as linked vectors */
static struct dvec* findvec_alle(void) {

    struct dvec* d, * newv = NULL, * end = NULL, * v;
    int i, num_nodes;
    Evt_Node_Info_t** node_table;

    /* Look for node name in the event-driven node list */
    num_nodes = g_mif_info.ckt->evt->counts.num_nodes;
    node_table = g_mif_info.ckt->evt->info.node_table;
    if (num_nodes == 0 || !node_table)
        return NULL;

    /* We need to create a new plot because of veccmp() is used */
    struct plot* pl = plot_alloc("digi");
    pl->pl_title = copy("DigitalData");
    pl->pl_name = copy("digital");
    pl->pl_date = copy(datestring());
    pl->pl_typename = copy("dig1");
    plot_new(pl);

    /* find all event data, create vectors, link them to v_link2 */
    for (i = 0; i < num_nodes; i++) {
        char* name = node_table[i]->name;
        d = EVTfindvec(name);
        if (!d)
            continue;
        /* nothing to plot */
        if (d->v_length == 1)
            continue;
        d->v_plot = pl;
        if (d->v_link2) {
            v = vec_copy(d);
            vec_new(v);
        }
        else {
            v = d;
        }
        if (end) {
            end->v_link2 = v;
         }
         else {
            newv = v;
         }
         end = v;
    }
    return newv;
}
#endif

#ifndef SIMULATOR
static struct dvec* findvec_alle(void) {
    return NULL;
}
#endif

/* Accept a difference confined to a leading v() or i() wrapper's letter.
 *
 * src/frontend/outitf.c:1164 stores a node name that begins with a digit as
 * V(<name>), with that upper case V in every mode, so the vector for node 1
 * is named V(1) whatever the deck says.  The wrapper is language syntax and
 * not part of the identifier -- the identifier is the name inside it, and v
 * and i are the voltage and current accessors, which the spec's
 * compatibility contract keeps case insensitive in all three modes.  So the
 * wrapper letter is folded while the name inside it is still compared
 * exactly.  Without this, 'print v(1)' would find nothing under distinguish
 * while working in both other modes, for a name no deck chose. */

static bool vec_wrapped_name_eq(const char *v_name, const char *typed)
{
    const size_t n = strlen(v_name);

    if (n < 4 || strlen(typed) != n)
        return FALSE;
    if (v_name[1] != '(' || typed[1] != '(' || v_name[n - 1] != ')')
        return FALSE;
    if (tolower_c(v_name[0]) != tolower_c(typed[0]))
        return FALSE;
    if (tolower_c(v_name[0]) != 'v' && tolower_c(v_name[0]) != 'i')
        return FALSE;

    return strncmp(v_name + 2, typed + 2, n - 2) == 0;
}


/* Are these two vector names the same name?
 *
 * Written for findvec(), where the first operand is a stored v_name and the
 * second is the spelling the caller typed, and now also the frontend's one
 * answer to that question wherever it is asked outside the lookup table:
 * findvec_ally() and vec_eq() below, is_scale_vec_of_current_plot()
 * (src/frontend/postcoms.c) and nameeq() (src/frontend/diff.c).  The argument
 * order is a convention rather than a constraint -- cieq() is symmetric, and
 * so is vec_wrapped_name_eq(), which permits a difference only at index 0 of
 * a v() or i() wrapper and compares every other byte including the closing
 * parenthesis.  doc/codex/issues/0032.
 *
 * The lookup key is folded at vec_rebuild_lookup_table():71 and the query at
 * findvec():184, and nghash_unique(..., FALSE) at :61 lets two vectors share
 * one key, so a folded-key chain can hold Out and OUT at once.  The fold is
 * load bearing and stays: it is what lets a caller type any case at all, and
 * swapping the hash comparator instead would flip the table from owning its
 * keys to borrowing them, because src/misc/hash.c:549 copies the key only for
 * NGHASH_DEF_HASH(NGHASH_FUNC_STR).  So the chain is filtered here instead.
 *
 * This is a lookup, not an identifier identity, and it deliberately does not
 * use inp_case_exact_ids().  In fold mode the query may still arrive with
 * upper case in it -- from the shared library, from the interactive prompt,
 * or from a generated name such as q1#collCX -- and has always been matched
 * without regard to case; making it exact there would be a regression in the
 * default mode.  Only distinguish makes it exact.  Under fold and preserve
 * every candidate on a folded-key chain satisfies cieq() by construction, so
 * this is a tautology and the lookup is byte identical to the historical one.
 * doc/claude/decisions/0001-distinguish.md decision 3. */

bool vec_name_eq(const char *v_name, const char *typed)
{
    if (inp_case_mode() != NG_CASE_DISTINGUISH)
        return cieq(v_name, typed) != 0;

    return (strcmp(v_name, typed) == 0) ||
            vec_wrapped_name_eq(v_name, typed);
}


/* Find a permanent vector with the given name.  'name' is the folded lookup
 * key; 'typed' is the same name with the spelling the caller used. */
static struct dvec *find_permanent_vector_by_name(
        NGHASHPTR pl_lookup_table, char *name, const char *typed)
{
    struct dvec *d;
    /* Find the first vector with the given name and then find others
     * until one having the VF_PERMANENT flag set is found. */
    for (d = nghash_find(pl_lookup_table, name);
            d;
            d = nghash_find_again(pl_lookup_table, name)) {
        if ((d->v_flags & VF_PERMANENT) && vec_name_eq(d->v_name, typed)) {
            /* A "permanent" vector was found with the name, so done */
            return d;
        }
    } /* end of loop over vectors in the plot having this name */
    /* try again, this time without quotes around the name */
    char *nname = cp_unquote(name);
    char *ntyped = cp_unquote(typed);
    for (d = nghash_find(pl_lookup_table, nname);
            d;
            d = nghash_find_again(pl_lookup_table, nname)) {
        if ((d->v_flags & VF_PERMANENT) && vec_name_eq(d->v_name, ntyped)) {
            /* A "permanent" vector was found with the name, so done */
            tfree(nname);
            tfree(ntyped);
            return d;
        }
    } /* end of loop over vectors in the plot having this name */
    tfree(nname);
    tfree(ntyped);
    return (struct dvec *) NULL; /* not found */
} /* end of function find_permanent_vector_by_name */


/* A lookup missed under distinguish.  Say so if the plot holds a vector whose
 * name differs from the one asked for only in case, because that is the one
 * mistake the page cannot show the user.  Silence when a name is being
 * defined, a warning when a name is being resolved:
 * doc/claude/decisions/0001-distinguish.md decision 2. */

static void vec_warn_case_near_miss(NGHASHPTR pl_lookup_table,
        const char *word)
{
    DS_CREATE(dbuf, 200);
    if (ds_cat_str_case(&dbuf, word, ds_case_lower) != DS_E_OK) {
        ds_free(&dbuf);
        return;
    }
    char * const lower_name = ds_get_buf(&dbuf);
    struct dvec *d;
    for (d = nghash_find(pl_lookup_table, lower_name);
            d;
            d = nghash_find_again(pl_lookup_table, lower_name)) {
        if (d->v_flags & VF_PERMANENT) {
            fprintf(cp_err,
                    "Warning: no vector named '%s'; '%s' differs only in "
                    "case (casemode=distinguish)\n",
                    word, d->v_name);
            break;
        }
    }
    ds_free(&dbuf);
} /* end of function vec_warn_case_near_miss */



/* If there are imbedded numeric strings, compare them numerically, not
 * alphabetically.
 */
static int namecmp(const void *a, const void *b)
{
    int i, j;

    const char *s = (const char *) a;
    const char *t = (const char *) b;

    for (;;) {
        while ((*s == *t) && !isdigit_c(*s) && *s)
            s++, t++;
        if (!*s)
            return (0);
        if ((*s != *t) && (!isdigit_c(*s) || !isdigit_c(*t)))
            return (*s - *t);

        /* The beginning of a number... Grab the two numbers and then
         * compare them...  */
        for (i = 0; isdigit_c(*s); s++)
            i = i * 10 + *s - '0';
        for (j = 0; isdigit_c(*t); t++)
            j = j * 10 + *t - '0';

        if (i != j)
            return (i - j);
    }
}


static int
veccmp(const void *a, const void *b)
{
    int i;
    struct dvec **d1 = (struct dvec **) a;
    struct dvec **d2 = (struct dvec **) b;

    if ((i = namecmp((*d1)->v_plot->pl_typename,
                     (*d2)->v_plot->pl_typename)) != 0)
        return (i);
    return (namecmp((*d1)->v_name, (*d2)->v_name));
}


/* Sort all the vectors in d, first by plot name and then by vector
 * name.  Do the right thing with numbers.  */
static struct dvec *
sortvecs(struct dvec *d) {
    struct dvec **array, *t;
    int i, j;

    for (t = d, i = 0; t; t = t->v_link2)
        i++;
    if (i < 2)
        return (d);
    array = TMALLOC(struct dvec *, i);
    for (t = d, i = 0; t; t = t->v_link2)
        array[i++] = t;

    qsort(array, (size_t) i, sizeof(struct dvec *), veccmp);

    /* Now string everything back together... */
    for (j = 0; j < i - 1; j++)
        array[j]->v_link2 = array[j + 1];
    array[j]->v_link2 = NULL;
    d = array[0];
    tfree(array);
    return (d);
}


/* Load in a rawfile. */
void
ft_loadfile(char *file)
{
    struct plot *pl, *np, *pp;

    fprintf(cp_out, "Loading raw data file (\"%s\") ...\n", file);
    pl = raw_read(file);
    if (pl)
        fprintf(cp_out, "done.\n");
    else
        fprintf(cp_out, "no data read.\n");

    /* This is a minor annoyance -- we should reverse the plot list so
     * they get numbered in the correct order.
     */
    for (pp = pl, pl = NULL; pp; pp = np) {
        np = pp->pl_next;
        pp->pl_next = pl;
        pl = pp;
    }
    for (; pl; pl = np) {
        np = pl->pl_next;
        plot_add(pl);
        /* Don't want to get too many "plot not written" messages. */
        pl->pl_written = TRUE;
    }
    plot_num++;
    plotl_changed = TRUE;
}


void
plot_add(struct plot *pl)
{
    struct dvec *v;
    struct plot *tp;
    char *s, buf[BSIZE_SP];

    fprintf(cp_out, "Title:  %s\nName: %s\nDate: %s\n\n", pl->pl_title,
            pl->pl_name, pl->pl_date);

    if (plot_cur)
        plot_cur->pl_ccom = cp_kwswitch(CT_VECTOR, pl->pl_ccom);

    for (v = pl->pl_dvecs; v; v = v->v_next)
        cp_addkword(CT_VECTOR, v->v_name);
    cp_addkword(CT_VECTOR, "all");

    if ((s = ft_plotabbrev(pl->pl_name)) == NULL)
        s = "unknown";
    do {
        (void) sprintf(buf, "%s%d", s, plot_num);
        for (tp = plot_list; tp; tp = tp->pl_next)
            if (cieq(tp->pl_typename, buf)) {
                plot_num++;
                break;
            }
    } while (tp);

    pl->pl_typename = copy(buf);
    plot_new(pl);
    cp_addkword(CT_PLOT, buf);
    pl->pl_ccom = cp_kwswitch(CT_VECTOR, NULL);
    plot_setcur(pl->pl_typename);
}


/* Remove a vector from the database, if it is there.
 *
 * The vector is picked with findvec()'s rule and not with a bare cieq(),
 * because under distinguish two spellings are two vectors and this walks
 * pl_dvecs in list order: vec_new() prepends, so the first cieq() hit was the
 * most recently created case variant and not the name the caller asked for.
 * 'unlet Out' beside an OUT therefore removed OUT.  vec_name_eq() is cieq()
 * in both other modes, where two spellings are one vector and the first hit
 * is the only hit, so nothing moves there.  doc/codex/issues/0027.
 *
 * report_case_miss splits the callers by decision 2 of
 * doc/claude/decisions/0001-distinguish.md.  'unlet' *resolves* a name, so a
 * miss that a case variant would have matched is reported: without that, the
 * exact compare above turns a silent wrong removal into a silent no-op, which
 * is a smaller mistake but just as invisible.  'compose' and 'cross' *define*
 * the name they pass, so they are silent -- warning there would fire on the
 * legitimate deck, which is the option decision 2 rejected. */

void
vec_remove(const char *name, bool report_case_miss)
{
    struct dvec *ov;

    for (ov = plot_cur->pl_dvecs; ov; ov = ov->v_next)
        if (vec_name_eq(ov->v_name, name) && (ov->v_flags & VF_PERMANENT))
            break;

    if (!ov) {
        if (report_case_miss && inp_case_mode() == NG_CASE_DISTINGUISH) {
            if (!plot_cur->pl_lookup_valid)
                vec_rebuild_lookup_table(plot_cur);
            vec_warn_case_near_miss(plot_cur->pl_lookup_table, name);
        }
        return;
    }

    ov->v_flags &= ~VF_PERMANENT;

    /* Remove from the keyword list. */
    cp_remkword(CT_VECTOR, name);
}


/* Get a vector by name. This deals with v(1), etc. almost properly. Also,
 * it checks for pre-defined vectors.
 */

/* Every caller of vec_fromplot() outside this file is resolving a name, so the
 * public entry point reports; only vec_get() needs the choice.  */

struct dvec *vec_fromplot(char *word, struct plot *plot) {
    return vec_fromplot_maybe_report(word, plot, TRUE);
}


static struct dvec *vec_fromplot_maybe_report(char *word, struct plot *plot,
        bool report_case_miss) {
    struct dvec *d = findvec(word, plot, report_case_miss);
    if (d != (struct dvec *) NULL) {
        return d;
    }

    /* Forms I(node) and i(node) are converted to node#branch;
     * forms x(node), x != i, x != I, and x != '(' are converted to node */
    if (word[0] != '\0' && word[0] != '(') { /* 1 or more char, not '(' */
        if (word[1] == '(') { /* x(, x != '(' */
            const char * const p_last_close_paren = strrchr(word + 2, ')');
            if (p_last_close_paren != (char *) NULL &&
                    p_last_close_paren - word > (ptrdiff_t) 2 &&
                    p_last_close_paren[1] == '\0') {
                /* Of form x(node). Create node string. */
                DS_CREATE(ds, 100);
                const char * const node_start = word + 2;
                bool ds_ok = ds_cat_mem(&ds, node_start,
                        (size_t) (p_last_close_paren - node_start)) ==
                        DS_E_OK;
                /* If i(node) or I(node), append #branch */
                if (tolower(word[0]) == (int) 'i') {
                    /* i(node) or I(node) */
                    ds_ok &= ds_cat_mem(&ds, "#branch", 7) == DS_E_OK;
                }
                if (!ds_ok) { /* Dstring error (allocation failure) */
                    (void) fprintf(cp_err, "Unable to build vector name.\n");
                }
                else { /* name built OK */
                    d = findvec(ds_get_buf(&ds), plot, report_case_miss);
                } /* end of case of vector name built OK */
                ds_free(&ds);
            } /* end of case of x(node) */
        } /* end of case of x( */
    } /* end of case of non-empty string and not leading '(' */

    return d;
} /* end of function vec_fromplot_maybe_report */



/* This is the main lookup routine for names. The possible types of names are:
 *  name        An ordinary vector.
 *  plot.name   A vector from a particular plot.
 *  @device[parm]   A device parameter.
 *  @model[parm]    A model parameter.
 *  @param      A circuit parameter.
 * For the @ cases, we construct a dvec with length 1 to hold the value.
 * In the other two cases, either the plot or the name can be "all", a
 * wildcard.
 * The vector name may have imbedded dots -- if the first component is a plot
 * name, it is considered the plot, otherwise the current plot is used.
 */

#define SPECCHAR '@'

/* vec_get() resolves a name, so a lookup that misses with a case variant
 * present reports it.  vec_get_quiet() is the same lookup for a caller whose
 * miss is not a resolution failure, and there are two kinds of those:
 * com_let()'s left-hand side is DEFINING the name it looks up, and a PROBE is
 * asking whether the name exists in order to decide what kind of token it is --
 * a formal parameter of a 'define', plotit()'s 'vs' separator.  Nothing else is
 * suppressed: the wildcard and '@' diagnostics below are the same in both.
 * doc/codex/issues/0034, doc/codex/issues/0045, decision 2 of
 * doc/claude/decisions/0001-distinguish.md and
 * doc/claude/decisions/0010-probe-category.md.  */

struct dvec *
vec_get(const char *vec_name) {
    return vec_get_maybe_report(vec_name, TRUE);
}


struct dvec *
vec_get_quiet(const char *vec_name) {
    return vec_get_maybe_report(vec_name, FALSE);
}


static struct dvec *
vec_get_maybe_report(const char *vec_name, bool report_case_miss) {
    struct dvec *d, *end = NULL, *newv = NULL;
    struct plot *pl;
    char buf[BSIZE_SP], *s, *wd, *word, *whole, *name = NULL, *param;
    int  i = 0;
    struct variable *vv, *v;

    wd = word = copy(vec_name);   /* Gets mangled below... */

    if (strchr(word, '.')) {
        /* Snag the plot... */
        for (i = 0, s = word; *s != '.'; i++, s++)
            buf[i] = *s;
        buf[i] = '\0';
        if (cieq(buf, "all")) {
            word = ++s;
            pl = NULL;  /* NULL pl signifies a wildcard. */
        } else {
            for (pl = plot_list;
                    pl && !plot_prefix(buf, pl->pl_typename);
                    pl = pl->pl_next)
                ;
            if (pl) {
                word = ++s;
            } else {
                /* This used to be an error... */
                pl = plot_cur;
            }
        }
    } else {
        pl = plot_cur;
    }

    if (pl) {
        d = vec_fromplot_maybe_report(word, pl, report_case_miss);
        if (!d)
            d = vec_fromplot_maybe_report(word, &constantplot,
                    report_case_miss);
    } else {
        for (pl = plot_list; pl; pl = pl->pl_next) {
            if (cieq(pl->pl_typename, "const"))
                continue;
            d = vec_fromplot_maybe_report(word, pl, report_case_miss);
            if (d) {
                if (end)
                    end->v_link2 = d;
                else
                    newv = d;
                for (end = d; end->v_link2; end = end->v_link2)
                    ;
            }
        }
        d = newv;
        if (!d) {
            fprintf(cp_err,
                    "Error: plot wildcard (name %s) matches nothing\n",
                    word);
            tfree(wd); /* MW. I don't want core leaks here */
            return (NULL);
        }
    }

    if (!d && (*word == SPECCHAR)) { /* "@" */
        int  multiple;

        /* This is a special quantity... */
        if (ft_nutmeg) {
            fprintf(cp_err,
                    "Error: circuit parameters only available with spice\n");
            tfree(wd);  /* MW. Memory leak fixed again */
            return (NULL); /* va: use NULL */
        }

        if (!ft_curckt) {
            fprintf(cp_err, "Error: No circuit loaded.\n");
            tfree(wd);
            return (NULL);
        }

        whole = copy(word);
        name = ++word;
        for (param = name; *param && (*param != '['); param++)
            ;

        if (*param) {
            *param++ = '\0';
            for (s = param; *s && *s != ']'; s++)
                ;
            *s = '\0';
        } else {
            param = NULL;
        }

        /*
         *  This is what is done in case of "alter r1 resistance = 1234"
         *                                r1    resistance, 0
         * if_setparam(ft_curckt->ci_ckt, &dev, param, dv, do_model);
         */

        vv = if_getparam(ft_curckt->ci_ckt, &name, param, 0, 0);
        if (!vv) {
            tfree(whole);
            tfree(wd);
            return (NULL);
        }

        /* If vec_name was "@dev", "@model", "@dev[all]" or @model[all]",
         * if_getparam() returns a list.
         */

        multiple = (vv->va_next != NULL);
        if (multiple && param)
            *--param = '\0';

        for (v = vv; v; v = v->va_next) {
            struct dvec *nd;
            char        *new_vec_name, new_name[256];

            if (multiple) {
                snprintf(new_name, sizeof new_name, "@%s[%s]",
                         name, v->va_name);
                new_vec_name = new_name;
            } else {
                new_vec_name = whole;
            }
            nd = dvec_alloc(copy(new_vec_name),
                            SV_NOTYPE,
                            VF_REAL,  /* No complex values yet... */
                            1, NULL);

            switch (v->va_type) {
            case CP_BOOL:
                *nd->v_realdata = (double)v->va_bool;
                break;
            case CP_NUM:
                *nd->v_realdata = (double)v->va_num;
                break;
            case CP_REAL:
                *nd->v_realdata = v->va_real;
                break;
            case CP_STRING:
                fprintf(stderr,
                        "ERROR: can not handle string value "
                        "of '%s' in vec_get(%s)\nIgnoring...\n",
                        v->va_name, new_vec_name);
                dvec_free(nd);
                continue;
                break;
            case CP_LIST:
                {
                    struct variable *nv;
                    enum cp_types    ft;

                    /* Array values are presented as a list.
                     * Compute the length of the vector, and check that
                     * it is homogenous:
                     * used with the parameters of isrc and vsrc
                     */

                    i = 0;
                    nv = v->va_vlist;
                    if (!nv) {
                        dvec_free(nd);
                        continue;
                    }
                    ft = nv->va_type;
                    for (; nv; nv = nv->va_next) {
                        /* Count the number of nodes in the list */

                        i++;
                        if (nv->va_type != ft)
                            break;
                    }
                    if (nv || ft == CP_STRING || ft == CP_LIST) {
                        fprintf(stderr,
                                "ERROR: can not handle mixed, string or list "
                                "value of '%s' in vec_get(%s)\nIgnoring...\n",
                                v->va_name, new_vec_name);
                        dvec_free(nd);
                        continue;
                    }
                    dvec_realloc(nd, i, NULL); /* Resize to # nodes */

                    /* Step through the list again, setting values this time */

                    i = 0;
                    for (nv = v->va_vlist; nv; nv = nv->va_next) {
                        switch (ft) {
                        case CP_BOOL:
                            nd->v_realdata[i++] = (double)nv->va_bool;
                            break;
                        case CP_NUM:
                            nd->v_realdata[i++] = (double)nv->va_num;
                            break;
                        default:
                        case CP_REAL:
                            nd->v_realdata[i++] = nv->va_real;
                            break;
                        }

                        /* To be able to identify the vector to represent
                         * belongs to a special "conunto" and should be printed
                         * in a special way.
                         */
                        nd->v_dims[1] = 1;
                    }
                }
                break;
            }
            /* Chain it on. */

            vec_new(nd);
            nd->v_link2 = d;
            d = nd;
        }

        free_struct_variable(vv);
        tfree(wd);
        tfree(whole);
        return d;
    }

    tfree(wd);
    return (sortvecs(d));
}


/* Execute the commands for a plot. This is done whenever a plot becomes
 * the current plot.
 */

void
plot_docoms(wordlist *wl)
{
    bool inter;

    inter = cp_interactive;
    cp_interactive = FALSE;
    while (wl) {
        (void) cp_evloop(wl->wl_word);
        wl = wl->wl_next;
    }
    cp_resetcontrol(TRUE);
    cp_interactive = inter;
}


/* Create a copy of a vector. The vector is not "permananent" */
struct dvec *vec_copy(struct dvec *v) {
    struct dvec *nv;

    if (!v) {
        return (struct dvec *) NULL;
    }

    /* Make a copy with the VF_PERMANENT bit cleared in v_flags */
    nv = dvec_alloc(copy(v->v_name),
                    v->v_type,
                    v->v_flags & ~VF_PERMANENT,
                    v->v_length, NULL);

    /* Copy the data to the new vector */
    if (isreal(v)) {
        (void) memcpy(nv->v_realdata, v->v_realdata,
               sizeof(double) * (size_t) v->v_length);
    }
    else {
        (void) memcpy(nv->v_compdata, v->v_compdata,
               sizeof(ngcomplex_t) * (size_t) v->v_length);
    }

    nv->v_minsignal = v->v_minsignal;
    nv->v_maxsignal = v->v_maxsignal;
    nv->v_gridtype = v->v_gridtype;
    nv->v_plottype = v->v_plottype;

    /* Modified to copy the rlength of origin to destination vecor
     * instead of always putting it to 0.
     * As when it comes to make a print does not leave M1 @ @ M1 = 0.0,
     * to do so in the event that rlength = 0 not print anything on screen
     * nv-> v_rlength = 0;
     * Default -> v_rlength = 0 and only if you come from a print or M1 @
     * @ M1 [all] rlength = 1, after control is one of
     * if (v-> v_rlength == 0) com_print (wordlist * wl)
     */
    nv->v_rlength = v->v_rlength;

    nv->v_outindex = 0; /*XXX???*/
    nv->v_linestyle = 0; /*XXX???*/
    nv->v_color = 0; /*XXX???*/
    nv->v_defcolor = v->v_defcolor;
    nv->v_numdims = v->v_numdims;

    /* Copy defined dimensions */
    (void) memcpy(nv->v_dims, v->v_dims,
            (size_t) v->v_numdims * sizeof *v->v_dims);

    nv->v_plot = v->v_plot;
    nv->v_next = NULL;
    nv->v_link2 = NULL;
    nv->v_scale = v->v_scale;

    return nv;
} /* end of function vec_copy */



/* Create a new plot structure. This just fills in the typename and sets up
 * the ccom struct.
 */

struct plot * plot_alloc(char *name)
{
    struct plot *pl = TMALLOC(struct plot, 1), *tp;
    char *s;
    struct ccom *ccom;
    char buf[BSIZE_SP];

    ZERO(pl, struct plot);
    if ((s = ft_plotabbrev(name)) == NULL)
        s = "unknown";
    do {
        (void) sprintf(buf, "%s%d", s, plot_num);
        for (tp = plot_list; tp; tp = tp->pl_next)
            if (cieq(tp->pl_typename, buf)) {
                plot_num++;
                break;
            }
    } while (tp);
    pl->pl_typename = copy(buf);
    cp_addkword(CT_PLOT, buf);
    /* va: create a new, empty keyword tree for class CT_VECTOR, s=old tree */
    ccom = cp_kwswitch(CT_VECTOR, NULL);
    cp_addkword(CT_VECTOR, "all");
    pl->pl_ccom = cp_kwswitch(CT_VECTOR, ccom);
    /* va: keyword tree is old tree again, new tree is linked to pl->pl_ccom */
    return (pl);
}


/* Stick a new vector in the proper place in the plot list. */

void
vec_new(struct dvec *d)
{
#ifdef FTEDEBUG
    if (ft_vecdb)
        fprintf(cp_err, "new vector %s\n", d->v_name);
#endif
    /* Note that this can't happen. */
    if (plot_cur == NULL) {
        fprintf(cp_err, "vec_new: Internal Error: no cur plot\n");
    }
    else {
        plot_cur->pl_lookup_valid = FALSE;
        if ((d->v_flags & VF_PERMANENT) && (plot_cur->pl_scale == NULL)) {
            plot_cur->pl_scale = d;
        }
        if (!d->v_plot) {
            d->v_plot = plot_cur;
        }
    }

    /* This code appears to be a patch for incorrectly specified vectors */
    if (d->v_numdims < 1) {
        d->v_numdims = 1;
        d->v_dims[0] = d->v_length;
    }

    {
        /* Make this vector the first plot vector and link the old first plot
         * vector via its next pointer */
        struct plot *v_plot = d->v_plot;
        d->v_next = v_plot->pl_dvecs;
        v_plot->pl_dvecs = d;
    }
}


/* Because of the way that all vectors, including temporary vectors,
 * are linked together under the current plot, they can often be
 * left lying around. This gets rid of all vectors that don't have
 * the permanent flag set. Also, for the remaining vectors, it
 * clears the v_link2 pointer.
 */

void
vec_gc(void)
{
    struct dvec *d, *nd;
    struct plot *pl;

    for (pl = plot_list; pl; pl = pl->pl_next)
        for (d = pl->pl_dvecs; d; d = nd) {
            nd = d->v_next;
            if (!(d->v_flags & VF_PERMANENT)) {
                if (ft_vecdb)
                    fprintf(cp_err,
                            "vec_gc: throwing away %s.%s\n",
                            pl->pl_typename, d->v_name);
                vec_free(d);
            }
        }

    for (pl = plot_list; pl; pl = pl->pl_next)
        for (d = pl->pl_dvecs; d; d = d->v_next)
            d->v_link2 = NULL;
}


/* Free a dvector. This is sort of a pain because we also have to make sure
 * that it has been unlinked from its plot structure. If the name of the
 * vector is NULL, then we have already freed it so don't try again. (This
 * situation can happen with user-defined functions.) Note that this depends
 * on our having tfree set its argument to NULL. Note that if all the vectors
 * in a plot are gone it stays around...
 */

void vec_free_x(struct dvec *v)
{
    /* Do not free if NULL or name is NULL. The second possibility is a
     * special case */
    if ((v == NULL) || (v->v_name == NULL)) {
        return;
    }
    struct plot * const pl = v->v_plot;

    /* Now we have to take this dvec out of the plot list. */
    if (pl != NULL) {
        pl->pl_lookup_valid = FALSE;

        /* If at head of list of vectors in the plot, make the next one
         * the new head of the list */
        if (pl->pl_dvecs == v) {
            pl->pl_dvecs = v->v_next;
        }
        else {
            /* Not at head of list so must locate and fix links */
            struct dvec *lv = pl->pl_dvecs;
            if (lv) { /* the plot has at least one vector */
                for ( ; lv->v_next; lv = lv->v_next) {
                    if (lv->v_next == v) { /* found prev vector */
                        break;
                    }
                }
            }

            /* If found in the list, link prev vector to next one */
            if (lv && lv->v_next) {
                lv->v_next = v->v_next;
            }
            else {
                (void) fprintf(cp_err,
                        "vec_free: Internal Error: %s not in plot\n",
                        v->v_name);
            }
        } /* end of case that vector being freed is not at head of list */

        if (pl->pl_scale == v) {
            if (pl->pl_dvecs) {
                pl->pl_scale = pl->pl_dvecs;    /* Random one... */
            }
            else {
                pl->pl_scale = NULL;
            }
        }
    } /* end of case that have a plot */

    dvec_free(v);
} /* end of function vec_free_x */



/* This function returns TRUE if every element of v and every element of
 * every vector linked to v through v_link2 is zero and FALSE otherwise. */
bool vec_iszero(const struct dvec *v)
{
    for (; v; v = v->v_link2) { /* step through linked vectors */
        if (isreal(v)) { /* current vector is real */
            const int n = v->v_length;
            int i;
            for (i = 0; i < n; i++) {
                if (v->v_realdata[i] != 0.0) {
                    return FALSE;
                }
            }
        }
        else { /* current vector is complex */
            const int n = v->v_length;
            int i;
            for (i = 0; i < n; i++) {
                if (realpart(v->v_compdata[i]) != 0.0) {
                    return FALSE;
                }
                if (imagpart(v->v_compdata[i]) != 0.0) {
                    return FALSE;
                }
            }
        }
    }

    return TRUE; /* every value tested was 0.0 */
} /* end of function vec_iszero */



/* This is something we do in a few places...  Since vectors get copied a lot,
 * we can't just compare pointers to tell if two vectors are 'really' the same.
 */

bool
vec_eq(struct dvec *v1, struct dvec *v2)
{
    char *s1, *s2;
    bool rtn;

    if (v1->v_plot != v2->v_plot)
        return (FALSE);

    s1 = vec_basename(v1);
    s2 = vec_basename(v2);

    if (vec_name_eq(s1, s2))
        rtn = TRUE;
    else
        rtn = FALSE;

    tfree(s1);
    tfree(s2);
    return rtn;
}


/* Return the name of the vector with the plot prefix stripped off.  This
 * is no longer trivial since '.' doesn't always mean 'plot prefix'.
 */

char *
vec_basename(struct dvec *v)
{
    char buf[BSIZE_SP], *t, *s;

    if (strchr(v->v_name, '.')) {
        if (cieq(v->v_plot->pl_typename, v->v_name))
            (void) strcpy(buf, v->v_name + strlen(v->v_name) + 1);
        else
            (void) strcpy(buf, v->v_name);
    } else {
        (void) strcpy(buf, v->v_name);
    }

    /* The name is reported as the simulator stored it.  This used to be
       lowercased here, which made 'print', 'write', 'wrs2p', 'spec', 'fft'
       and 'psd' disagree with the batch rawfile and with the operating point
       node dump about internally generated names such as q1#collCX. */
    s = skip_ws(buf);
    for (t = s; *t; t++)
        ;
    while ((t > s) && isspace_c(t[-1]))
        *--t = '\0';
    return (copy(s));
}

/* get address of plot named 'name' */
struct plot *get_plot(const char *name)


{
    struct plot *pl;
    for (pl = plot_list; pl; pl = pl->pl_next) {
        if (plot_prefix(name, pl->pl_typename)) {
            return pl;
        }
    }
    fprintf(cp_err, "Error: no such plot named %s\n", name);
    return (struct plot *) NULL;
} /* end of function get_plot */



/* Make a plot the current one.  This gets called by cp_usrset() when one
 * does a 'set curplot = name'.
 * va: ATTENTION: has unlinked old keyword-class-tree from keywords[CT_VECTOR]
 *                (potentially memory leak)
 */
void plot_setcur(const char *name)
{
    struct plot *pl;

    if (cieq(name, "new")) {
        pl = plot_alloc("unknown");
        pl->pl_title = copy("Anonymous");
        pl->pl_name = copy("unknown");
        pl->pl_date = copy(datestring());
        plot_new(pl);
        plot_cur = pl;
        return;
    }
    /* plots are listed in pl in reverse order */
    else if (cieq(name, "previous")) {
        if (plot_cur->pl_next) {
            plot_cur = plot_cur->pl_next;
#ifdef XSPICE
            if (ft_curckt) {
                EVTswitch_plot(ft_curckt->ci_ckt, plot_cur->pl_typename);
            }
#endif
        }
        else {
            fprintf(cp_err,
                    "Warning: No previous plot is available. "
                    "Plot remains unchanged (%s).\n",
                    plot_cur->pl_typename);
        }
        return;
    }
    else if (cieq(name, "next")) {
        /* Step through the list, which has plots in reverse order */
        struct plot *prev_pl = NULL;
        for (pl = plot_list; pl; pl = pl->pl_next) {
            if (pl == plot_cur) {
                break;
            }
            prev_pl = pl;
        }
        if (prev_pl) { /* found */
            plot_cur = prev_pl;
#ifdef XSPICE
            if (ft_curckt) {
                EVTswitch_plot(ft_curckt->ci_ckt, plot_cur->pl_typename);
            }
#endif
        }
        else { /* no next plot */
            fprintf(cp_err,
                    "Warning: No next plot is available. "
                    "Plot remains unchanged (%s).\n",
                    plot_cur->pl_typename);
        }
        return;
    }

    pl = get_plot(name);
    if (!pl) {
        return;
    }

    /* va: we skip cp_kwswitch, because it confuses the keyword-tree management for
     *     repeated op-commands. When however cp_kwswitch is necessary for other
     *     reasons, we should hold the original keyword table pointer in an
     *     permanent variable, since it will lost here, and can never tfree'd.
     if (plot_cur)
     {
     plot_cur->pl_ccom = cp_kwswitch(CT_VECTOR, pl->pl_ccom);
     }
    */
#ifdef XSPICE
    /* XSPICE event data are linked to the current circuit. It must not be removed
       when manipulating the data by any command.
    */
    if (ft_curckt) {
        EVTswitch_plot(ft_curckt->ci_ckt, name);
    }
#endif
    plot_cur = pl;
} /* end of function plot_setcur */



/* Add a plot to the plot list. This is different from plot_add() in that
 * all this does is update the list and the variable $plots.
 */
void plot_new(struct plot *pl)
{
    pl->pl_next = plot_list;
    plot_list = pl;
}


/* This routine takes a multi-dimensional vector, treats it as a
 * group of 2-dimensional matrices and transposes each matrix.
 * The data array is replaced with a new one that has the elements
 * in the proper order.  Otherwise the transposition is done in place.
 */

void
vec_transpose(struct dvec *v)
{
    int dim0, dim1, nummatrices;
    int i, j, k, joffset, koffset, blocksize;
    double *newreal, *oldreal;
    ngcomplex_t *newcomp, *oldcomp;

    if (v->v_numdims < 2 || v->v_length <= 1)
        return;

    dim0 = v->v_dims[v->v_numdims-1];
    dim1 = v->v_dims[v->v_numdims-2];
    v->v_dims[v->v_numdims-1] = dim1;
    v->v_dims[v->v_numdims-2] = dim0;
    /* Assume length is a multiple of each dimension size.
     * This may not be safe, in which case a test should be
     * made that the length is the product of all the dimensions.
     */
    blocksize = dim0*dim1;
    nummatrices = v->v_length / blocksize;

    /* Note:
     *   olda[i,j] is at data[i*dim0+j]
     *   newa[j,i] is at data[j*dim1+i]
     *   where j is in [0, dim0-1]  and  i is in [0, dim1-1]
     * Since contiguous data in the old array is scattered in the new array
     * we can't use memcpy :(.  There is probably a BLAS2 function for this
     * though.  The formulation below gathers scattered old data into
     * consecutive new data.
     */

    if (isreal(v)) {
        newreal = TMALLOC(double, v->v_length);
        oldreal = v->v_realdata;
        koffset = 0;
        for (k = 0; k < nummatrices; k++) {
            joffset = 0;
            for (j = 0; j < dim0; j++) {
                for (i = 0; i < dim1; i++) {
                    newreal[ koffset + joffset + i ] =
                        oldreal[ koffset + i*dim0 + j ];
                }
                joffset += dim1;  /* joffset = j*dim0 */
            }
            koffset += blocksize; /* koffset = k*blocksize = k*dim0*dim1 */
        }
        dvec_realloc(v, v->v_length, newreal);
    } else {
        newcomp = TMALLOC(ngcomplex_t, v->v_length);
        oldcomp = v->v_compdata;
        koffset = 0;
        for (k = 0; k < nummatrices; k++) {
            joffset = 0;
            for (j = 0; j < dim0; j++) {
                for (i = 0; i < dim1; i++) {
                    newcomp[ koffset + joffset + i ] =
                        oldcomp[ koffset + i*dim0 + j ];
                }
                joffset += dim1;  /* joffset = j*dim0 */
            }
            koffset += blocksize; /* koffset = k*blocksize = k*dim0*dim1 */
        }
        dvec_realloc(v, v->v_length, newcomp);
    }
}


/* This routine takes a multi-dimensional vector and turns it into a family
 * of 1-d vectors, linked together with v_link2.  It is here so that plot
 * can do intelligent things.
 */

struct dvec *
vec_mkfamily(struct dvec *v) {
    int size, numvecs, i, count[MAXDIMS];
    struct dvec *vecs, *d, **t;
    char buf2[BSIZE_SP];

    if (v->v_numdims < 2)
        return (v);

    size = v->v_dims[v->v_numdims - 1];
    for (i = 0, numvecs = 1; i < v->v_numdims - 1; i++)
        numvecs *= v->v_dims[i];
    for (i = 0; i < MAXDIMS; i++)
        count[i] = 0;
    for (t = &vecs, i = 0; i < numvecs; i++) {

        indexstring(count, v->v_numdims - 1, buf2);

        d = dvec_alloc(tprintf("%s%s", v->v_name, buf2),
                       v->v_type,
                       v->v_flags,
                       size, NULL);

        d->v_minsignal = v->v_minsignal;
        d->v_maxsignal = v->v_maxsignal;
        d->v_gridtype = v->v_gridtype;
        d->v_plottype = v->v_plottype;
        d->v_scale = v->v_scale;
        /* Don't copy the default color, since there will be many
         * of these things...
         */
        d->v_numdims = 1;
        d->v_dims[0] = size;

        if (isreal(v)) {
            memcpy(d->v_realdata, v->v_realdata + (size_t) (size * i),
                    (size_t) size * sizeof(double));
        } else {
            memcpy(d->v_compdata, v->v_compdata + (size_t) (size * i),
                    (size_t) size * sizeof(ngcomplex_t));
        }
        /* Add one to the counter. */
        (void) incindex(count, v->v_numdims - 1, v->v_dims, v->v_numdims);

        *t = d;
        t = &(d->v_link2);
    }

    for (d = vecs; d; d = d->v_link2)
        vec_new(d);

    return (vecs);
}


/* This function will match "op" with "op1", but not "op1" with "op12". */
static bool plot_prefix(const char *pre, const char *str)
{
    if (!*pre) { /* prefix is empty string */
        return TRUE; /* Define "" to be prefix */
    }

    while (*pre && *str) {
        if (*pre != *str) { /* stop at first mismatch */
            break;
        }
        pre++;
        str++;
    }

    if (*pre || (*str && isdigit_c(pre[-1])))
        return (FALSE);
    else
        return (TRUE);
}

struct dvec*
copycut(struct dvec* v, struct dvec* newscalevec, int istart, int istop)
{
    struct dvec* nv;
    int i;
    int len = istop - istart;

    if (!v) {
        return (struct dvec*)NULL;
    }

    /* Make a copy with the VF_PERMANENT bit cleared in v_flags */
    nv = dvec_alloc(copy(v->v_name),
        v->v_type,
        v->v_flags,// & ~VF_PERMANENT,
        len, NULL);

    /* Copy the data to the new vector */
    if (isreal(v)) {
        for (i = 0; i < len; i++) {
            nv->v_realdata[i] = v->v_realdata[istart + i];
        }
    }
    else {
        for (i = 0; i < len; i++) {
            nv->v_compdata[i] = v->v_compdata[istart + i];
        }
    }

    nv->v_minsignal = v->v_minsignal;
    nv->v_maxsignal = v->v_maxsignal;
    nv->v_gridtype = v->v_gridtype;
    nv->v_plottype = v->v_plottype;

    /* Modified to copy the rlength of origin to destination vecor
        * instead of always putting it to 0.
        * As when it comes to make a print does not leave M1 @ @ M1 = 0.0,
        * to do so in the event that rlength = 0 not print anything on screen
        * nv-> v_rlength = 0;
        * Default -> v_rlength = 0 and only if you come from a print or M1 @
        * @ M1 [all] rlength = 1, after control is one of
        * if (v-> v_rlength == 0) com_print (wordlist * wl)
        */
    nv->v_rlength = v->v_rlength;

    nv->v_outindex = 0; /*XXX???*/
    nv->v_linestyle = 0; /*XXX???*/
    nv->v_color = 0; /*XXX???*/
    nv->v_defcolor = v->v_defcolor;
    nv->v_numdims = v->v_numdims;

    /* Copy defined dimensions */
    (void)memcpy(nv->v_dims, v->v_dims,
        (size_t)v->v_numdims * sizeof * v->v_dims);

    nv->v_plot = newscalevec->v_plot;
    nv->v_next = NULL;
    nv->v_link2 = NULL;

    return nv;
} /* end of function copycut */
