/* The draw log: 'set rnddraws=<file>' writes one tab-separated row for each
   random draw made while a deck is parsed, so that a Monte Carlo run can
   regress its outputs on the random inputs without reading device
   parameters back.  'unset rnddraws' closes the file.

   Each parse, the initial load and every 'reset', is one record set:

     #<TAB>rnddraws<TAB>v1<TAB>seed=N<TAB>src=S<TAB>offset=K
     seed<TAB>instance_path<TAB>param<TAB>func<TAB>args<TAB>z<TAB>value
     ...
     #<TAB>end<TAB>logged=N<TAB>unlogged_calls=M

   seed=N is the seed the generator was last seeded with, and src=S says how:
   startup, setseed, option ('.option seed=') or checkseed (the first
   control-language draw after $rndseed changed).  offset=K counts the
   uniform-generator calls made since that seeding; K != 0 means the parse
   did not start from the seed (a 'reset' without 'setseed'), so the tag
   alone does not reproduce the rows.

   A row:  seed is N again.  instance_path is the first token of the card
   the draw was made for, in ngspice's flattened form (r.xm1.r1), the model
   name on a .model card, the subckt instance on another dot card inside a
   subckt, and '?' when there is none (a global .param).  param is the
   parameter the expression is assigned to; a key (instance, param) that
   draws again in the same record set is written param#2, param#3, ...
   func is agauss, gauss, unif, aunif or limit, args its arguments separated
   by commas.  z is the variate before scaling: gauss1() for agauss and
   gauss, drand() in [-1,1) for unif, aunif and limit.  value is what the
   random function returned, before any rounding by the caller.  Numbers are
   written with %.17g, and '?' stands for anything unknown.

   The footer counts the rows, and the uniform-generator calls made between
   header and footer that no row accounts for.  Draws made after the footer
   (control-language functions, transient noise sources) are in no record
   set; they show up as the next set's offset, unless a re-seed comes first.

   Nothing here calls the generator.  All state is closed or freed by
   ng_drawlog_reset(), which totalreset() calls for the shared library. */

#include "ngspice/ngspice.h"
#include "ngspice/cpextern.h"
#include "ngspice/hash.h"
#include "ngspice/stringskip.h"
#include "ngspice/randnumb.h"
#include "ngspice/drawlog.h"

#include <errno.h>

static FILE *log_fp = NULL;
static bool set_open = FALSE;
static int set_seed = 0;
static unsigned long long set_start_calls = 0;
static unsigned long long set_logged_calls = 0;
static long set_logged = 0;
static NGHASHPTR set_keys = NULL;   /* "instance\tparam" -> count */
static char *ctx_instance = NULL;
static char *ctx_param = NULL;


static void
free_count(void *p)
{
    tfree(p);
}


static void
clear_context(void)
{
    tfree(ctx_instance);
    tfree(ctx_param);
}


void
ng_drawlog_open(const char *path)
{
    ng_drawlog_close();
    if (!path || !*path)
        return;
    log_fp = fopen(path, "w");
    if (!log_fp)
        fprintf(cp_err, "Error: rnddraws: cannot open %s: %s\n",
                path, strerror(errno));
}


void
ng_drawlog_close(void)
{
    ng_drawlog_end();
    if (log_fp) {
        fclose(log_fp);
        log_fp = NULL;
    }
}


void
ng_drawlog_reset(void)
{
    ng_drawlog_close();
    clear_context();
}


bool
ng_drawlog_on(void)
{
    return set_open;
}


void
ng_drawlog_begin(void)
{
    const char *src;
    unsigned long long offset;

    rng_deck_parsed();
    if (!log_fp)
        return;
    ng_drawlog_end();   /* a parse that returned early left its set open */
    rng_seed_info(&set_seed, &src, &offset);
    fprintf(log_fp, "#\trnddraws\tv1\tseed=%d\tsrc=%s\toffset=%llu\n",
            set_seed, src, offset);
    set_start_calls = rng_ncalls();
    set_logged_calls = 0;
    set_logged = 0;
    set_keys = nghash_init(NGHASH_MIN_SIZE);
    clear_context();
    set_open = TRUE;
}


void
ng_drawlog_end(void)
{
    if (!set_open)
        return;
    set_open = FALSE;
    fprintf(log_fp, "#\tend\tlogged=%ld\tunlogged_calls=%llu\n", set_logged,
            rng_ncalls() - set_start_calls - set_logged_calls);
    fflush(log_fp);
    nghash_free(set_keys, free_count, NULL);
    set_keys = NULL;
    clear_context();
}


/* The card the next draws are made for.  first, if not '\0', replaces the
   first character of the instance name (numparam has commented the X line
   out).  scope is the subckt instance, for a dot card inside a subckt. */
void
ng_drawlog_card(const char *line, char first, const char *scope)
{
    clear_context();
    if (!line)
        return;
    line = skip_ws(line);
    if (*line == '.') {
        if (ciprefix(".model", line)) {
            const char *name = skip_ws(skip_non_ws(line));
            if (*name)
                ctx_instance = copy_substring(name, skip_non_ws(name));
        }
        else if (scope) {
            ctx_instance = copy(scope);
        }
        return;
    }
    if (*line) {
        ctx_instance = copy_substring(line, skip_non_ws(line));
        if (first)
            ctx_instance[0] = first;
    }
}


void
ng_drawlog_context(const char *instance, const char *param)
{
    clear_context();
    if (instance)
        ctx_instance = copy(instance);
    if (param)
        ctx_param = copy(param);
}


void
ng_drawlog_param(const char *param)
{
    tfree(ctx_param);
    if (param && *param)
        ctx_param = copy(param);
}


/* The parameter an expression at pos is assigned to: the identifier in front
   of the nearest '=' before pos that is not part of ==, <=, >= or !=. */
void
ng_drawlog_param_at(const char *line, const char *pos)
{
    const char *p = pos;

    tfree(ctx_param);
    while (p > line) {
        p--;
        if (*p != '=')
            continue;
        if (p > line && strchr("=<>!", p[-1])) {
            p--;
            continue;
        }
        const char *end = p;
        while (end > line && isspace_c(end[-1]))
            end--;
        const char *beg = end;
        while (beg > line && (isalnum_c(beg[-1]) || beg[-1] == '_'))
            beg--;
        if (beg < end)
            ctx_param = copy_substring(beg, end);
        return;
    }
}


void
ng_drawlog_row(const char *func, int nargs, double a1, double a2, double a3,
               double z, double value)
{
    const char *instance, *param;
    char *key;
    int *count;

    if (!set_open)
        return;

    instance = ctx_instance ? ctx_instance : "?";
    param = ctx_param ? ctx_param : "?";

    key = tprintf("%s\t%s", instance, param);
    count = (int *) nghash_find(set_keys, key);
    if (!count) {
        count = TMALLOC(int, 1);
        *count = 0;
        nghash_insert(set_keys, key, count);
    }
    tfree(key);
    (*count)++;

    fprintf(log_fp, "%d\t%s\t%s", set_seed, instance, param);
    if (*count > 1)
        fprintf(log_fp, "#%d", *count);
    fprintf(log_fp, "\t%s\t%.17g", func, a1);
    if (nargs > 1)
        fprintf(log_fp, ",%.17g", a2);
    if (nargs > 2)
        fprintf(log_fp, ",%.17g", a3);
    fprintf(log_fp, "\t%.17g\t%.17g\n", z, value);

    set_logged++;
    set_logged_calls += rng_ncalls() - rng_draw_start();
}
