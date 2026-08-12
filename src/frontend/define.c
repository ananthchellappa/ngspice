/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Wayne A. Christopher, U. C. Berkeley CAD Group
**********/

/*
 * User-defined functions. The user defines the function with
 *  define func(arg1, arg2, arg3) <expression involving args...>
 * Then when he types "func(1, 2, 3)", the commas are interpreted as
 * binary operations of the lowest priority by the parser, and ft_substdef()
 * below is given a chance to fill things in and return what the parse tree
 * would have been had the entire thing been typed.
 * Note that we have to take some care to distinguish between functions
 * with the same name and different arities.
 */

#include "ngspice/ngspice.h"
#include "ngspice/cpdefs.h"
#include "ngspice/ftedefs.h"
#include "ngspice/dvec.h"
#include "ngspice/fteparse.h"
#include "define.h"

#include "completion.h"


static void savetree(struct pnode *pn);
static void prdefs(char *name);
static void prtree(struct udfunc *ud, FILE *fp);
static void prtree1(struct pnode *pn, FILE *fp);
static struct pnode *trcopy(struct pnode *tree, char *arg_names, struct pnode *args);
static struct pnode *ntharg(int num, struct pnode *args);
static int numargs(struct pnode *args);
static struct pnode *copy_value_node(struct pnode *tree);
static bool node_of_arglist(const struct pnode *tree, const struct pnode *args);
static struct pnode *copy_arg_root(struct pnode *tree);

static struct udfunc *udfuncs = NULL;


/* Are these two user-defined function names the same name?
 *
 * Class C of doc/claude/decisions/0001-distinguish.md decision 3, not Class
 * A.  The second operand is a word typed at the control language, and the
 * reader folds such a word only when it arrived through inp_readall().
 * Measured under the *default* fold mode through `ngspice -p`, each
 * diagnostic joined here from the two lines it really occupies:
 *
 *     define f(x) x*3
 *     print F(2)    Error: no such function as F, / or F(2) is not available.
 *     print VM(1)   Error: no such function as VM, / or VM(1) is not available.
 *
 * so an identity predicate here would leave the shipped default mode unable
 * to call its own shipped functions from the prompt, from ngSpice_Command()
 * and from the shared library.  That is the regression
 * doc/claude/decisions/0004-unlet-vector-identity.md decision 1 refused at
 * vec_remove() and doc/claude/decisions/0001-distinguish.md decision 3
 * refused at findvec(), on the same measurement.  Only distinguish makes it
 * exact, and there two spellings really are two functions.
 *
 * This list holds ngspice's own shipped defines as well as the user's:
 * vm, vp, vdb, vr, vi, vg, gd, max and min are installed through
 * com_define() from ft_cpinit()'s udfs[] table (src/frontend/cpitf.c).  So
 * under distinguish they answer only to the lower-case spelling that table
 * writes, which is what decision 3 says about every name ngspice constructs
 * for itself.  doc/claude/decisions/0013-user-defined-function-identity.md. */

static bool
udf_name_eq(const char *ud_name, const char *typed)
{
    if (inp_case_mode() == NG_CASE_DISTINGUISH)
        /* case-lint: helper - this function IS the Class C classification */
        return eq(ud_name, typed) != 0;

    /* case-lint: helper - its fold and preserve arm, same classification */
    return cieq(ud_name, typed) != 0;
}


/* Set up a function definition. */

void
com_define(wordlist *wlist)
{
    int arity = 0, i;
    char buf[BSIZE_SP], tbuf[BSIZE_SP], *s, *t, *b;
    wordlist *wl;
    struct pnode *names;
    struct udfunc *udf;
    const char **probes;

    /* If there's nothing then print all the definitions. */
    if (wlist == NULL) {
        prdefs(NULL);
        return;
    }

    /* Accumulate the function head in the buffer, w/out spaces. A
     * useful thing here would be to check to make sure that there
     * are no formal parameters here called "list". But you have
     * to try really hard to break this here.
     */
    buf[0] = '\0';

    for (wl = wlist; wl && (strchr(wl->wl_word, ')') == NULL);
         wl = wl->wl_next)
        (void) strcat(buf, wl->wl_word);

    if (wl) {
        t = strchr(buf, '\0');
        for (s = wl->wl_word; *s && (*s != ')');)
            *t++ = *s++;
        *t++ = ')';
        *t = '\0';
        if (*++s)
            wl->wl_word = copy(s);
        else
            wl = wl->wl_next;
    }

    /* If that's all, then print the definition. */
    if (wl == NULL) {
        s = strchr(buf, '(');
        if (s)
            *s = '\0';
        prdefs(buf);
        return;
    }

    /* Now check to see if this is a valid name for a function (i.e,
     * there isn't a predefined function of the same name).
     */
    (void) strcpy(tbuf, buf);

    for (b = tbuf; *b; b++)
        if (isspace_c(*b) || (*b == '(')) {
            *b = '\0';
            break;
        }

    /* ft_funcs[] is the language's own function table, so this is a keyword
     * test and not an identity one: a built-in function name is
     * case-insensitive in all three modes, per the spec's compatibility
     * contract point 2.  It is already eqc() and stays that way. */
    for (i = 0; ft_funcs[i].fu_name; i++)
        /* case-lint: keyword - ft_funcs[] is the language's own table */
        if (eqc(ft_funcs[i].fu_name, tbuf)) {
            fprintf(cp_err, "Error: %s is a predefined function.\n",
                    tbuf);
            return;
        }

    /* Format the name properly and add to the list. This has to happen before
     * the parse below, which needs the formal parameter names.
     */
    b = copy(buf);
    for (s = b; *s; s++) {
        if (*s == '(') {
            *s = '\0';
            if (s[1] != ')')
                arity++;    /* It will have been 0. */
        } else if (*s == ')') {
            *s = '\0';
        } else if (*s == ',') {
            *s = '\0';
            arity++;
        }
    }

    /* The formal parameters are not vectors. trcopy() below finds them by
     * matching their names against the zero-length placeholder a failed lookup
     * leaves behind, so a miss is the mechanism rather than a failure, and
     * under casemode=distinguish 'define f(x) x*2' on a deck with a net X used
     * to warn about a definition that is correct. They are named as probes, so
     * the parse does not report a case near miss for them -- and for nothing
     * else: every other identifier in the body IS resolved here, once and for
     * good, because the tree is frozen at definition time and a name that
     * misses now can never be resolved later. doc/codex/issues/0045.
     */
    {
        const char **p = probes = TMALLOC(const char *, arity + 1);
        char *arg = strchr(b, '\0') + 1;
        while (*arg) {
            *p++ = arg;
            arg = strchr(arg, '\0') + 1;
        }
        *p = NULL;
    }

    /* Parse the rest of it. We can't know if there are the right
     * number of undefined variables in the expression.
     */
    names = ft_getpnames_probe(wl, FALSE, probes);
    tfree(probes);
    if (names == NULL) {
        tfree(b);
        return;
    }

    /* This is a pain -- when things are garbage-collected, any
     * vectors that may have been mentioned here will be thrown
     * away. So go down the tree and save any vectors that aren't
     * formal parameters.
     */
    savetree(names);

    /* The definition side is deliberately NOT given udf_name_eq()'s
     * predicate, and this is the one place where the resolution and the
     * definition of a name in this file answer differently.  The test is a
     * *prefix* test, not an equality one -- doc/codex/issues/0051, where
     * `define f(x)` already destroys a stored `foo(y)` of the same arity in
     * every mode -- and case-blinding a prefix test *widens* the set of
     * names it destroys.  Measured: with ciprefix() here, `define VD(x) x*7`
     * at the prompt under the default fold silently destroys the shipped
     * `vdb(x)`, which `prefix()` never matched.  So the case fix at this
     * site is blocked on `0051`'s repair and waits for it; the residue is
     * that under preserve a `define VM(x)` beside the shipped `vm` prepends
     * a second entry rather than replacing it, which changes no number --
     * com_define() prepends and ft_substdef() takes the first match, so the
     * newer definition answers both spellings -- but does leave `define vm`
     * listing two entries under one identifier.
     * doc/claude/decisions/0013-user-defined-function-identity.md decision 2. */
    for (udf = udfuncs; udf; udf = udf->ud_next)
        if (prefix(b, udf->ud_name) && (arity == udf->ud_arity))
            break;

    if (udf == NULL) {
        udf = TMALLOC(struct udfunc, 1);
        udf->ud_next = udfuncs;
        udfuncs = udf;
    }

    udf->ud_text = names;
    udf->ud_name = b;
    udf->ud_arity = arity;

    cp_addkword(CT_UDFUNCS, b);
}


/* Kludge. */

static void
savetree(struct pnode *pn)
{
    struct dvec *d;

    if (pn->pn_value) {
        /* We specifically don't add this to the plot list
         * so it won't get gc'ed.
         */
        d = pn->pn_value;
        if ((d->v_length != 0) || eq(d->v_name, "list")) {
            pn->pn_value = dvec_alloc(copy(d->v_name),
                                      d->v_type,
                                      d->v_flags,
                                      d->v_length, NULL);

            /* this dvec isn't member of any plot */

            if (isreal(d)) {
                memcpy(pn->pn_value->v_realdata,
                      d->v_realdata,
                      sizeof(double) * (size_t) d->v_length);
            } else {
                memcpy(pn->pn_value->v_compdata,
                      d->v_compdata,
                      sizeof(ngcomplex_t) * (size_t) d->v_length);
            }
        }
    } else if (pn->pn_op) {
        savetree(pn->pn_left);
        if (pn->pn_op->op_arity == 2)
            savetree(pn->pn_right);
    } else if (pn->pn_func) {
        savetree(pn->pn_left);
    }
}


/* A bunch of junk to print out nodes. */

static void
prdefs(char *name)
{
    struct udfunc *udf;

    if (name && *name) {    /* You never know what people will do */
        for (udf = udfuncs; udf; udf = udf->ud_next)
            /* a resolution of the word `define <name>` was given */
            if (udf_name_eq(udf->ud_name, name))
                prtree(udf, cp_out);
    } else {
        for (udf = udfuncs; udf; udf = udf->ud_next)
            prtree(udf, cp_out);
    }
}


/* Print out one definition. */

static void
prtree(struct udfunc *ud, FILE *fp)
{
    const char *s = ud->ud_name;

    /* print the function name */
    fprintf(fp, "%s (", s);
    s = strchr(s, '\0') + 1;

    /* print the formal args */
    while (*s) {
        fputs(s, fp);
        s = strchr(s, '\0') + 1;
        if (*s)
            fputs(", ", fp);
    }
    fputs(") = ", fp);

    /* print the function body */
    prtree1(ud->ud_text, fp);
    putc('\n', fp);
}


static void
prtree1(struct pnode *pn, FILE *fp)
{
    if (pn->pn_value) {
        fputs(pn->pn_value->v_name, fp);
    } else if (pn->pn_func) {
        fprintf(fp, "%s (", pn->pn_func->fu_name);
        prtree1(pn->pn_left, fp);
        fputs(")", fp);
    } else if (pn->pn_op && (pn->pn_op->op_arity == 2)) {
        fputs("(", fp);
        prtree1(pn->pn_left, fp);
        fprintf(fp, ")%s(", pn->pn_op->op_name);
        prtree1(pn->pn_right, fp);
        fputs(")", fp);
    } else if (pn->pn_op && (pn->pn_op->op_arity == 1)) {
        fprintf(fp, "%s(", pn->pn_op->op_name);
        prtree1(pn->pn_left, fp);
        fputs(")", fp);
    } else {
        fputs("<something strange>", fp);
    }
}


struct pnode *
ft_substdef(const char *name, struct pnode *args)
{
    struct udfunc *udf, *wrong_udf = NULL;
    char *arg_names;

    int arity = numargs(args);

    for (udf = udfuncs; udf; udf = udf->ud_next)
        /* the resolution: `name` is the spelling the caller typed */
        if (udf_name_eq(udf->ud_name, name)) {
            if (arity == udf->ud_arity)
                break;
            wrong_udf = udf;
        }

    if (udf == NULL) {
        if (wrong_udf)
            fprintf(cp_err,
                    "Warning: the user-defined function %s has %d args\n",
                    name, wrong_udf->ud_arity);
        return NULL;
    }

    arg_names = strchr(udf->ud_name, '\0') + 1;

    /* Now we have to traverse the tree and copy it over,
     * substituting args.
     */
    {
        struct pnode *tree = trcopy(udf->ud_text, arg_names, args);

        /* trcopy() hands a value node back rather than copying it, and
         * leaves the reference to the parent node it is about to build.
         * When the body is a single value node there is no parent, and
         * this frame is the one that knows the node is on its way out to
         * an evaluator that will free it.  doc/codex/issues/0053.
         */
        if (tree == udf->ud_text)
            tree = copy_value_node(tree);
        else if (node_of_arglist(tree, args))
            tree = copy_arg_root(tree);

        return tree;
    }
}


/* Return a node of our own over a copy of a stored body's vector -- the
 * tree the parser would have built had the body been typed at the call
 * site, which is what this file promises its caller.
 *
 * The obvious repair, taking the reference count trcopy()'s parents take
 * and letting the two share the one node, is not enough, because a parse
 * tree's ROOT is not read-only the way its interior is.  parse-bison.y
 * writes pn_name on to whatever node an expression reduces to, and
 * ft_evaluate() renames the dvec that node carries after the enclosing
 * command.  Measured, with the reference count and nothing else:
 * `define c(x) 5` then `print c(2)` leaves `define c` reporting
 * `c (x) = c(2)`, and every further call leaks the pn_name it displaced.
 * A reference count keeps the node alive; it cannot keep it unwritten.
 * Sharing is safe for an interior node because nothing writes to one.
 *
 * doc/claude/decisions/0014-single-node-define-body.md.
 */

static struct pnode *
copy_value_node(struct pnode *tree)
{
    struct pnode *pn = alloc_pnode();
    struct dvec *d = tree->pn_value;

    if (d->v_length == 0) {
        /* A name the body never resolved.  PP_mksnode() leaves exactly
         * this placeholder behind on a miss and does not give it to a
         * plot, so neither do we.
         */
        pn->pn_value = dvec_alloc(copy(d->v_name), d->v_type, d->v_flags,
                                  0, NULL);
    } else {
        struct dvec *nv = vec_copy(d);
        vec_new(nv);
        pn->pn_value = nv;
    }

    return pn;
}


/* Is this node one the caller's argument list owns?
 *
 * The walk is ntharg()'s own, so the two agree by construction: ntharg()
 * hands back the comma node's left branch at every level of the list and
 * the list itself at the last, and those are exactly the nodes tested here.
 * trcopy()'s other branches allocate, so a node that answers TRUE here can
 * only have arrived through the formal-substitution branch.
 */

static bool
node_of_arglist(const struct pnode *tree, const struct pnode *args)
{
    for (; args; args = args->pn_right) {
        if (!(args->pn_op && (args->pn_op->op_num == PT_OP_COMMA)))
            return tree == args;
        if (tree == args->pn_left)
            return TRUE;
    }

    return FALSE;
}


/* Return a root of our own over an argument trcopy() substituted into the
 * root position -- the body was a bare formal, so the tree handed back is
 * a node of the caller's argument list and not one this file built.
 *
 * A root is owned implicitly, at pn_use == 0, so two owners cannot be
 * expressed for one: PP_mkfnode() releases the argument list on the way
 * out, and the caller frees the tree it is given.  The interior needs no
 * such treatment -- trcopy()'s parents take a reference on what they link,
 * so PP_mkfnode()'s release decrements rather than frees -- and it is only
 * the root that is written to afterwards, by parse-bison.y's pn_name and
 * by ft_evaluate()'s rename of the vector it carries.  So the node is
 * duplicated and the subtree beneath it is shared under a count, which is
 * the same rule doc/claude/decisions/0014-single-node-define-body.md drew
 * for a body shared out of udfuncs.  doc/codex/issues/0054.
 */

static struct pnode *
copy_arg_root(struct pnode *tree)
{
    struct pnode *pn;

    if (tree->pn_value)
        return copy_value_node(tree);

    pn = alloc_pnode();

    /* pn_func and pn_op are pointers to a global constant struct */
    pn->pn_func = tree->pn_func;
    pn->pn_op = tree->pn_op;

    pn->pn_left = tree->pn_left;
    if (pn->pn_left)
        pn->pn_left->pn_use++;

    pn->pn_right = tree->pn_right;
    if (pn->pn_right)
        pn->pn_right->pn_use++;

    return pn;
}


/* Copy the tree and replace formal args with the right stuff. The way
 * we know that something might be a formal arg is when it is a dvec
 * with length 0 and a name that isn't "list". I hope nobody calls their
 * formal parameters "list".
 */

static struct pnode *
trcopy(struct pnode *tree, char *arg_names, struct pnode *args)
{
    if (tree->pn_value) {

        struct dvec *d = tree->pn_value;

        if ((d->v_length == 0) && strcmp(d->v_name, "list")) {

            /* Yep, it's a formal parameter. Substitute for it.
             * IMPORTANT: we never free parse trees, so we
             * needn't worry that they aren't trees here.
             */

            char *s = arg_names;
            int i;

            for (i = 1; *s; i++) {
                /* The formal's spelling on the `define` card against the
                 * body's spelling on the same card -- two tokens the deck
                 * wrote, so this is Class A of
                 * doc/claude/decisions/0001-distinguish.md decision 3 and
                 * takes ng_ideq(), like a .func formal.  Under preserve
                 * `define g(X) x*2` used to leave the body's x
                 * unsubstituted, so g(5) was unavailable; under distinguish
                 * X and x are two names and the miss is the right answer.
                 * parse.c's is_probe_name() matches these same formals byte
                 * exactly, which agrees with this predicate in the only mode
                 * where a probe's silence is observable. */
                if (ng_ideq(s, d->v_name))
                    return ntharg(i, args);
                s = strchr(s, '\0') + 1;
            }

            return tree;
        }

        return tree;
    }

    if (tree->pn_func) {

        struct pnode *pn = alloc_pnode();

        /* pn_func are pointers to a global constant struct */
        pn->pn_func = tree->pn_func;

        pn->pn_left = trcopy(tree->pn_left, arg_names, args);
        pn->pn_left->pn_use++;

        return pn;
    }

    if (tree->pn_op) {

        struct pnode *pn = alloc_pnode();

        /* pn_op are pointers to a global constant struct */
        pn->pn_op = tree->pn_op;

        pn->pn_left = trcopy(tree->pn_left, arg_names, args);
        pn->pn_left->pn_use++;

        if (pn->pn_op->op_arity == 2) {
            pn->pn_right = trcopy(tree->pn_right, arg_names, args);
            pn->pn_right->pn_use++;
        }

        return pn;
    }

    fprintf(cp_err, "trcopy: Internal Error: bad parse node\n");
    return NULL;
}


/* Find the n'th arg in the arglist, returning NULL if there isn't one.
 * Since comma has such a low priority and associates to the right,
 * we can just follow the right branch of the tree num times.
 * Note that we start at 1 when numbering the args.
 */

static struct pnode *
ntharg(int num, struct pnode *args)
{
    for (; args; args = args->pn_right, --num) {
        if (num <= 1) {
            if (args->pn_op && (args->pn_op->op_num == PT_OP_COMMA))
                return args->pn_left;
            return args;
        }
        if (!(args->pn_op && (args->pn_op->op_num == PT_OP_COMMA)))
            return NULL;
    }

    return NULL;
}


static int
numargs(struct pnode *args)
{
    int arity;

    if (!args)
        return 0;

    for (arity = 1; args; args = args->pn_right, arity++)
        if (!(args->pn_op && (args->pn_op->op_num == PT_OP_COMMA)))
            return arity;

    // note: a trailing NULL pn_right will be counted too
    return arity;
}


void
com_undefine(wordlist *wlist)
{
    struct udfunc *udf;

    if (!wlist)
        return;

    if (*wlist->wl_word == '*') {
        for (udf = udfuncs; udf;) {
            struct udfunc *next = udf->ud_next;
            cp_remkword(CT_UDFUNCS, udf->ud_name);
            free_pnode(udf->ud_text);
            tfree(udf->ud_name);
            tfree(udf);
            udf = next;
        }
        udfuncs = NULL;
        return;
    }

    for (; wlist; wlist = wlist->wl_next) {
        struct udfunc *prev_udf = NULL;
        for (udf = udfuncs; udf;) {
            struct udfunc *next = udf->ud_next;
            /* a resolution: the word `undefine` was given */
            if (udf_name_eq(udf->ud_name, wlist->wl_word)) {
                if (prev_udf)
                    prev_udf->ud_next = udf->ud_next;
                else
                    udfuncs = udf->ud_next;
                cp_remkword(CT_UDFUNCS, wlist->wl_word);
                free_pnode(udf->ud_text);
                tfree(udf->ud_name);
                tfree(udf);
            } else {
                prev_udf = udf;
            }
            udf = next;
        }
    }
}


/*
 * This is only here so I can "call" it from gdb/dbx
 */

void
ft_pnode(struct pnode *pn)
{
    prtree1(pn, cp_err);
}
