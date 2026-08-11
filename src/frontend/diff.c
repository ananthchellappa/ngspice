/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Wayne A. Christopher, U. C. Berkeley CAD Group
Modified: 2001 Paolo Nenzi (printnum)
Patched: 2010/2012 by Bill Swartz (hash table for vectors)
**********/

/*
 * Do a 'diff' of two plots.
 */

#include "ngspice/dstring.h"
#include "ngspice/dvec.h"
#include "ngspice/ftedefs.h"
#include "ngspice/hash.h"
#include "ngspice/ngspice.h"
#include "ngspice/sim.h"

#include "diff.h"
#include "variable.h"

static bool nameeq(const char *n1, const char *n2);
static char *canonical_name(const char *name, DSTRINGPTR dbuf_p,
        bool make_i_name_lower);
static char *canonical_key(const char *canonical, DSTRINGPTR dbuf_p);




static char *
canonical_name(const char *name, DSTRINGPTR dbuf_p,
        bool make_i_name_lower)
{
    ds_clear(dbuf_p); /* Reset dynamic buffer */

    /* "i(some_name)" -> "some_name#branch" */
    /* "I(some_name)" -> "some_name#branch" */
    if (ciprefix("i(", name)) {
        static const char sz_branch[] = "#branch";
        const char *p_start = name + 2;
        size_t n = strlen(p_start) - 1; /* copy all but final ')' */
        ds_case_t case_type = make_i_name_lower ?
                ds_case_lower : ds_case_as_is;
        bool f_ok = ds_cat_mem_case(dbuf_p, p_start, n, case_type) == DS_E_OK;
        f_ok &= ds_cat_mem(dbuf_p, sz_branch,
                sizeof sz_branch / sizeof *sz_branch - 1) == DS_E_OK;
        if (!f_ok) {
            fprintf(stderr, "Error: DS could not convert %s\n", name);
            controlled_exit(-1);
        }
        return ds_get_buf(dbuf_p);
    }

    /* Convert a name starting with a digit, such as a numbered node to
     * something like v(33) */
    if (isdigit_c(*name)) {
        bool f_ok = ds_cat_mem(dbuf_p, "v(", 2) == DS_E_OK;
        f_ok &= ds_cat_str(dbuf_p, name) == DS_E_OK;
        f_ok &= ds_cat_char(dbuf_p, ')') == DS_E_OK;
        if (!f_ok) {
            fprintf(stderr, "Error: DS could not convert %s\n", name);
            controlled_exit(-1);
        }
        return ds_get_buf(dbuf_p);
    }

    /* Finally if neither of the special cases above occur, there is
     * no need to do anything with the name. A slight improvement in
     * performance could be achieved by simply returning the name
     * argument. Making a copy ensures that it can be modified without
     * changing the original, but in the current use cases that is
     * not an issue. */
    if (ds_cat_str(dbuf_p, name) != DS_E_OK) {
        fprintf(stderr, "Error: DS could not convert %s\n", name);
        controlled_exit(-1);
    }
    return ds_get_buf(dbuf_p);
} /* end of function canonical_name */



/* The cross-reference hash key of a canonical name: the same name folded.
 *
 * com_diff() pairs a vector of one plot with its twin in the other through a
 * string-keyed nghash, and every lookup path of that table compares with
 * strcmp (src/misc/hash.c:260, :310), so the pairing was byte exact in all
 * three modes -- too strict under fold and preserve, where two spellings are
 * one name.  doc/codex/issues/0037.
 *
 * The key is folded and the comparator is left alone, which is section 2.3 of
 * doc/claude/suggestions/case-sensitive-identifiers-plan.md: hash.c:549 copies
 * the key on insert only when hash_func is NGHASH_DEF_HASH(NGHASH_FUNC_STR),
 * and every free of a key is gated on the same test (:110, :182, :393, :471,
 * :787, :866), so installing a case-insensitive hash function would silently
 * flip this table from owning its keys to borrowing them.  The fold is unconditional and the duplicate
 * chain nghash_unique(..., FALSE) already permits is filtered with
 * vec_name_eq() instead, exactly as the frontend vector lookup table does
 * (src/frontend/vectors.c:74 and :423). */

static char *
canonical_key(const char *canonical, DSTRINGPTR dbuf_p)
{
    ds_clear(dbuf_p); /* Reset dynamic buffer */
    if (ds_cat_str_case(dbuf_p, canonical, ds_case_lower) != DS_E_OK) {
        fprintf(stderr, "Error: DS could not convert %s\n", canonical);
        controlled_exit(-1);
    }
    return ds_get_buf(dbuf_p);
} /* end of function canonical_key */



/* Determine if two vectors have the 'same' name. Note that this compare can
 * be performed by using the "canonical" forms returned by
 * canonical_name().
 *
 * Both call sites hand this a stored v_name and a word from diff's argument
 * list, so it is the frontend's vector-name compare and takes vec_name_eq():
 * cieq() under fold and preserve, exact under distinguish, where two
 * spellings are two vectors and asking for one must not select the other.
 * doc/codex/issues/0032. */
static bool
nameeq(const char *n1, const char *n2)
{
    /* First compare them the way they came in.
     * If they match nothing more to do */
    if (vec_name_eq(n1, n2)) {
        return TRUE;
    }

    /* Init the dynamic string buffers to build canonical names */
    DS_CREATE(ds1, 100);
    DS_CREATE(ds2, 100);

    /* Compare canonical names */
    const bool rc = vec_name_eq(canonical_name(n1, &ds1, FALSE),
            canonical_name(n2, &ds2, FALSE));

    /* Free the dynamic string buffers */
    ds_free(&ds1);
    ds_free(&ds2);

    return rc;
} /* end of function nameeq */



void
com_diff(wordlist *wl)
{
    double vntol, abstol, reltol, tol, cmax, cm1, cm2;
    struct plot *p1, *p2 = NULL;
    struct dvec *v1, *v2;
    double d1, d2;
    ngcomplex_t c1, c2, c3;
    int i, j;
    char *v1_name;          /* canonical v1 name */
    char *v2_name;          /* canonical v2 name */
    char *key;              /* folded canonical name, the hash key */
    NGHASHPTR crossref_p;   /* cross reference hash table */
    wordlist *tw;
    char numbuf[BSIZE_SP], numbuf2[BSIZE_SP], numbuf3[BSIZE_SP], numbuf4[BSIZE_SP]; /* For printnum */

    if (!cp_getvar("diff_vntol", CP_REAL, &vntol, 0))
        vntol = 1.0e-6;
    if (!cp_getvar("diff_abstol", CP_REAL, &abstol, 0))
        abstol = 1.0e-12;
    if (!cp_getvar("diff_reltol", CP_REAL, &reltol, 0))
        reltol = 0.001;

    /* Let's try to be clever about defaults. This code is ugly. */
    if (!wl || !wl->wl_next) {
        if (plot_list && plot_list->pl_next && !plot_list->pl_next->pl_next) {
            p1 = plot_list;
            p2 = plot_list->pl_next;
            if (wl && !eq(wl->wl_word, p1->pl_typename) &&
                !eq(wl->wl_word, p2->pl_typename)) {
                fprintf(cp_err, "Error: no such plot \"%s\"\n",
                        wl->wl_word);
                return;
            }
            fprintf(cp_err, "Plots are \"%s\" and \"%s\"\n",
                    plot_list->pl_typename,
                    plot_list->pl_next->pl_typename);
            if (wl)
                wl = NULL;
        } else {
            fprintf(cp_err, "Error: plot names not given.\n");
            return;
        }
    } else {
        for (p1 = plot_list; p1; p1 = p1->pl_next)
            if (eq(wl->wl_word, p1->pl_typename))
                break;
        if (!p1) {
            fprintf(cp_err, "Error: no such plot %s\n", wl->wl_word);
            return;
        }
        wl = wl->wl_next;
    }

    if (!p2) {
        for (p2 = plot_list; p2; p2 = p2->pl_next)
            if (eq(wl->wl_word, p2->pl_typename))
                break;
        if (!p2) {
            fprintf(cp_err, "Error: no such plot %s\n", wl->wl_word);
            return;
        }
        wl = wl->wl_next;
    }

    /* Now do some tests to make sure these plots are really the
     * same type, etc.
     */
    if (!eq(p1->pl_name, p2->pl_name))
        fprintf(cp_err,
                "Warning: plots %s and %s seem to be of different types\n",
                p1->pl_typename, p2->pl_typename);
    if (!eq(p1->pl_title, p2->pl_title))
        fprintf(cp_err,
                "Warning: plots %s and %s seem to be from different circuits\n",
                p1->pl_typename, p2->pl_typename);

    /* This may not be the best way to do this.  It wasn't :).  The original
     * was O(n2) - not good.  Now use a hash table to reduce it to O(n). */
    for (v1 = p1->pl_dvecs; v1; v1 = v1->v_next)
        v1->v_link2 = NULL;

    DS_CREATE(ibuf, 100); /* used to build canonical name */
    DS_CREATE(kbuf, 100); /* used to fold a canonical name into a hash key */
    DS_CREATE(jbuf, 100); /* canonical name of a candidate on the chain */
    crossref_p = nghash_init(NGHASH_MIN_SIZE);
    nghash_unique(crossref_p, FALSE);

    /* The canonical name keeps the spelling inside an i() wrapper, which the
     * key's fold makes unnecessary here and which the mode decides below.
     * Folding it in the name as well paired i(Vsrc) with i(VSRC) under
     * distinguish, where they are two names.  doc/codex/issues/0040. */
    for (v2 = p2->pl_dvecs; v2; v2 = v2->v_next) {
        v2->v_link2 = NULL;
        v2_name = canonical_name(v2->v_name, &ibuf, FALSE);
        nghash_insert(crossref_p, canonical_key(v2_name, &kbuf), v2);
    }

    /* The key is folded, so the chain a lookup walks can hold Out and OUT at
     * once and the spellings are separated here instead.  vec_name_eq() is
     * cieq() under fold and preserve, where two spellings are one vector and
     * every candidate on a folded-key chain satisfies it by construction, and
     * exact under distinguish, where two spellings are two vectors and the
     * pairing must stay byte exact.  doc/codex/issues/0037. */
    for (v1 = p1->pl_dvecs; v1; v1 = v1->v_next) {
        v1_name = canonical_name(v1->v_name, &ibuf, FALSE);
        key = canonical_key(v1_name, &kbuf);
        for (v2 = nghash_find(crossref_p, key);
             v2;
             v2 = nghash_find_again(crossref_p, key))
        {
            if (!v2->v_link2 &&
                vec_name_eq(canonical_name(v2->v_name, &jbuf, FALSE),
                        v1_name) &&
                ((v1->v_flags & (VF_REAL | VF_COMPLEX)) ==
                 (v2->v_flags & (VF_REAL | VF_COMPLEX))) &&
                (v1->v_type == v2->v_type))
            {
                v1->v_link2 = v2;
                v2->v_link2 = v1;
                break;
            }
        }
    }

    ds_free(&jbuf);
    ds_free(&kbuf);
    ds_free(&ibuf);
    nghash_free(crossref_p, NULL, NULL);

    for (v1 = p1->pl_dvecs; v1; v1 = v1->v_next)
        if (!v1->v_link2)
            fprintf(cp_err,
                    ">>> %s vector %s in %s not in %s, or of wrong type\n",
                    isreal(v1) ? "real" : "complex",
                    v1->v_name, p1->pl_typename, p2->pl_typename);

    for (v2 = p2->pl_dvecs; v2; v2 = v2->v_next)
        if (!v2->v_link2)
            fprintf(cp_err,
                    ">>> %s vector %s in %s not in %s, or of wrong type\n",
                    isreal(v2) ? "real" : "complex",
                    v2->v_name, p2->pl_typename, p1->pl_typename);

    /* Throw out the ones that aren't in the arg list */
    if (wl && !eqc(wl->wl_word, "all")) {    /* Just in case */
        for (v1 = p1->pl_dvecs; v1; v1 = v1->v_next)
            if (v1->v_link2) {
                for (tw = wl; tw; tw = tw->wl_next)
                    if (nameeq(v1->v_name, tw->wl_word))
                        break;
                if (!tw)
                    v1->v_link2 = NULL;
            }
        for (v2 = p2->pl_dvecs; v2; v2 = v2->v_next)
            if (v2->v_link2) {
                for (tw = wl; tw; tw = tw->wl_next)
                    if (nameeq(v2->v_name, tw->wl_word))
                        break;
                if (!tw)
                    v2->v_link2 = NULL;
            }
    }

    /* Now we have all the vectors linked to their twins.  Travel
     * down each one and print values that differ enough.
     */
    for (v1 = p1->pl_dvecs; v1; v1 = v1->v_next) {
        if (!v1->v_link2)
            continue;
        v2 = v1->v_link2;
        if (v1->v_type == SV_VOLTAGE)
            tol = vntol;
        else
            tol = abstol;
        j = MAX(v1->v_length, v2->v_length);
        for (i = 0; i < j; i++) {
            if (v1->v_length <= i) {
                fprintf(cp_out,
                        ">>> %s is %d long in %s and %d long in %s\n",
                        v1->v_name, v1->v_length,
                        p1->pl_typename, v2->v_length, p2->pl_typename);
                break;
            } else if (v2->v_length <= i) {
                fprintf(cp_out,
                        ">>> %s is %d long in %s and %d long in %s\n",
                        v2->v_name, v2->v_length,
                        p2->pl_typename, v1->v_length, p1->pl_typename);
                break;
            } else {
                if (isreal(v1)) {
                    d1 = v1->v_realdata[i];
                    d2 = v2->v_realdata[i];
                    if (MAX(fabs(d1), fabs(d2)) * reltol +
                        tol < fabs(d1 - d2)) {
                        printnum(numbuf, d1);
                        fprintf(cp_out,
                                "%s.%s[%d] = %-15s ",
                                p1->pl_typename, v1->v_name, i, numbuf);
                        printnum(numbuf, d2);
                        fprintf(cp_out,
                                "%s.%s[%d] = %s\n",
                                p2->pl_typename, v2->v_name, i, numbuf);
                    }
                } else {
                    c1 = v1->v_compdata[i];
                    c2 = v2->v_compdata[i];
                    realpart(c3) = realpart(c1) - realpart(c2);
                    imagpart(c3) = imagpart(c1) - imagpart(c2);
                    /* Stupid evil PC compilers */
                    cm1 = cmag(c1);
                    cm2 = cmag(c2);
                    cmax = MAX(cm1, cm2);
                    if (cmax * reltol + tol < cmag(c3)) {

                        printnum(numbuf, realpart(c1));
                        printnum(numbuf2, imagpart(c1));
                        printnum(numbuf3, realpart(c2));
                        printnum(numbuf4, imagpart(c2));

                        fprintf(cp_out,
                                "%s.%s[%d] = %-10s, %-10s %s.%s[%d] = %-10s, %s\n",
                                p1->pl_typename, v1->v_name, i,
                                numbuf,
                                numbuf2,
                                p2->pl_typename, v2->v_name, i,
                                numbuf3,
                                numbuf4);
                    }
                }
            }
        }
    }
}

