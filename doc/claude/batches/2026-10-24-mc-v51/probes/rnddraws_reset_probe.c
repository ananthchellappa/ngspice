/* ver_51 batch item 4 (P4): the draw log across ngSpice_Reset().
 *
 * Shows, through the shared library, that
 *   1. 'set rnddraws=<file>' logs ngSpice_Circ() and each 'reset';
 *   2. ngSpice_Reset() closes the log: no descriptor is left open on the
 *      file, the last record set has its footer, and nothing is written to
 *      it afterwards;
 *   3. after ngSpice_Init() again, the seed record starts over (seed=1,
 *      src=startup, offset=0, although session 1 used the generator), a
 *      new 'set rnddraws' logs again, and its z column equals session 1's
 *      setseed 1 set (startup draws what setseed 1 draws, item 3);
 *   4. 'unset rnddraws' closes the log;
 *   5. a parse that aborts through controlled_exit() (an undefined
 *      parameter) leaves its record set open, and the ngSpice_Reset() the
 *      host then calls writes that set's footer with a sane
 *      unlogged_calls: the footer must be written before the call counter
 *      is zeroed, or the unsigned subtraction wraps (review round 1).
 * It prints one line per check and exits 0 only if every check passed.
 *
 * Build and run (from the repo root), against a --with-ngshared tree:
 *
 *     gcc -Wall -O0 -o /tmp/rnddraws_reset_probe \
 *         doc/claude/batches/2026-10-24-mc-v51/probes/rnddraws_reset_probe.c \
 *         -I src/include -L build-ver_51-shared/src/.libs -lngspice
 *     cd <scratch dir> && LD_LIBRARY_PATH=<repo>/build-ver_51-shared/src/.libs \
 *         SPICE_SCRIPTS=<repo>/build-ver_51-shared/src /tmp/rnddraws_reset_probe
 *
 * The logs are written to the current directory.
 */

#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ngspice/sharedspice.h"

static int failures = 0;

static int cb_char(char *s, int id, void *p)
{ (void) s; (void) id; (void) p; return 0; }

static int cb_exit(int st, NG_BOOL a, NG_BOOL b, int id, void *p)
{ (void) a; (void) b; (void) id; (void) p; printf("EXIT %d\n", st); return 0; }

static char l0[] = "* rnddraws reset probe";
static char l1[] = ".subckt cell a b";
static char l2[] = ".param sx={agauss(0,1,1)}";
static char l3[] = "r1 a b r={1k*(1+0.01*sx)}";
static char l4[] = ".ends";
static char l5[] = "v1 1 0 1";
static char l6[] = "xm1 1 0 cell";
static char l7[] = "xm2 1 0 cell";
static char l8[] = ".end";
static char *deck[] = { l0, l1, l2, l3, l4, l5, l6, l7, l8, NULL };

/* aborts in numparam: r2's expression names no parameter */
static char b0[] = "* rnddraws abort probe";
static char b1[] = "v1 1 0 1";
static char b2[] = "r1 1 0 r={agauss(1k,100,1)}";
static char b3[] = "r2 1 0 r={nosuchparam*2}";
static char b4[] = ".end";
static char *bad[] = { b0, b1, b2, b3, b4, NULL };

static void check(bool ok, const char *what)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok)
        failures++;
}

static void cmd(const char *c)
{
    char buf[256];
    snprintf(buf, sizeof buf, "%s", c);
    ngSpice_Command(buf);
}

/* descriptors this process holds open on path */
static int open_fds(const char *path)
{
    char want[PATH_MAX], link[PATH_MAX], name[PATH_MAX];
    struct dirent *e;
    int n = 0;
    DIR *d;

    if (!realpath(path, want))
        return 0;
    if ((d = opendir("/proc/self/fd")) == NULL)
        return -1;
    while ((e = readdir(d)) != NULL) {
        ssize_t len;
        snprintf(name, sizeof name, "/proc/self/fd/%s", e->d_name);
        len = readlink(name, link, sizeof link - 1);
        if (len <= 0)
            continue;
        link[len] = '\0';
        if (strcmp(link, want) == 0)
            n++;
    }
    closedir(d);
    return n;
}

static long file_size(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 ? (long) st.st_size : -1;
}

/* The record sets of a log: header text after "#\trnddraws\tv1\t", the z
   column of each row, and whether every header has a footer. */
#define MAXSETS 8
#define MAXROWS 8
struct rset { char hdr[128]; double z[MAXROWS]; int nrows; bool closed; };

static int read_sets(const char *path, struct rset *sets)
{
    char line[1024];
    int n = 0;
    FILE *fp = fopen(path, "r");
    if (!fp)
        return -1;
    while (fgets(line, sizeof line, fp)) {
        line[strcspn(line, "\n")] = '\0';
        if (strncmp(line, "#\trnddraws\tv1\t", 14) == 0 && n < MAXSETS) {
            memset(&sets[n], 0, sizeof sets[n]);
            snprintf(sets[n].hdr, sizeof sets[n].hdr, "%s", line + 14);
            n++;
        } else if (strncmp(line, "#\tend\t", 6) == 0 && n > 0) {
            sets[n - 1].closed = true;
        } else if (n > 0 && sets[n - 1].nrows < MAXROWS) {
            /* seed instance param func args z value */
            char *f = line;
            int i;
            for (i = 0; i < 5 && f; i++) {
                f = strchr(f, '\t');
                if (f)
                    f++;
            }
            if (f)
                sets[n - 1].z[sets[n - 1].nrows++] = strtod(f, NULL);
        }
    }
    fclose(fp);
    return n;
}

int main(void)
{
    struct rset a[MAXSETS], c[MAXSETS];
    int na, nc;
    char line[1024], footer[1024] = "";
    unsigned long long unlogged = 0;
    FILE *fp;
    long size_a;

    remove("probe-a.tsv");
    remove("probe-c.tsv");
    remove("probe-d.tsv");

    /* session 1 */
    ngSpice_Init(cb_char, NULL, cb_exit, NULL, NULL, NULL, NULL);
    cmd("set rnddraws=probe-a.tsv");
    cmd("setseed 1");
    ngSpice_Circ(deck);
    cmd("reset");
    check(open_fds("probe-a.tsv") == 1, "session 1: the log is open");

    ngSpice_Reset();
    check(open_fds("probe-a.tsv") == 0, "ngSpice_Reset closes the log");
    na = read_sets("probe-a.tsv", a);
    check(na == 2, "session 1 wrote two record sets (ngSpice_Circ, reset)");
    check(na == 2 && a[0].closed && a[1].closed, "both sets have a footer");
    check(na >= 1 && strcmp(a[0].hdr, "seed=1\tsrc=setseed\toffset=0") == 0,
          "set 1: seed=1 src=setseed offset=0");
    check(na >= 2 && strncmp(a[1].hdr, "seed=1\tsrc=setseed\toffset=", 26) == 0
          && strcmp(a[1].hdr, "seed=1\tsrc=setseed\toffset=0") != 0,
          "set 2 (reset, no setseed): offset != 0");
    size_a = file_size("probe-a.tsv");

    /* session 2 */
    ngSpice_Init(cb_char, NULL, cb_exit, NULL, NULL, NULL, NULL);
    ngSpice_Circ(deck);
    check(file_size("probe-a.tsv") == size_a && open_fds("probe-a.tsv") == 0,
          "after re-init, the old log is neither open nor written");
    cmd("set rnddraws=probe-c.tsv");
    cmd("reset");
    cmd("setseed 1");
    cmd("reset");
    check(open_fds("probe-c.tsv") == 1, "session 2: set rnddraws opens a new log");
    cmd("unset rnddraws");
    check(open_fds("probe-c.tsv") == 0, "unset rnddraws closes it");
    nc = read_sets("probe-c.tsv", c);
    check(nc == 2 && c[0].closed && c[1].closed, "session 2 wrote two closed sets");
    /* Session 2's first parse was not logged, so its first logged set is
       the seed-1 stream after one parse: the same offset as session 1's
       second set, under src=startup.  A seed record or a call counter
       carried over from session 1 would give src=setseed or a larger
       offset. */
    printf("      session 1 set 2 header: %s\n", na >= 2 ? a[1].hdr : "-");
    printf("      session 2 set 1 header: %s\n", nc >= 1 ? c[0].hdr : "-");
    printf("      session 2 set 2 header: %s\n", nc >= 2 ? c[1].hdr : "-");
    check(nc >= 1 && na >= 2 && strncmp(c[0].hdr, "seed=1\tsrc=startup\toffset=", 26) == 0
          && strcmp(c[0].hdr + 26, a[1].hdr + 26) == 0,
          "session 2 set 1: the seed record restarted (src=startup, session 1's one-parse offset)");
    check(nc >= 2 && strcmp(c[1].hdr, "seed=1\tsrc=setseed\toffset=0") == 0,
          "session 2 set 2: seed=1 src=setseed offset=0");
    check(na >= 1 && nc >= 2 && a[0].nrows == 2 && c[1].nrows == 2
          && a[0].z[0] == c[1].z[0] && a[0].z[1] == c[1].z[1],
          "session 2's setseed 1 rows draw session 1's setseed 1 z values");
    check(file_size("probe-a.tsv") == size_a, "the session 1 log is unchanged");

    ngSpice_Reset();

    /* session 3: a parse aborts with its record set open, then the host
       resets.  The generator has been used before the set opens, so its
       start count is not 0. */
    ngSpice_Init(cb_char, NULL, cb_exit, NULL, NULL, NULL, NULL);
    cmd("setseed 1");
    ngSpice_Circ(deck);
    cmd("set rnddraws=probe-d.tsv");
    ngSpice_Circ(bad);
    ngSpice_Reset();
    check(open_fds("probe-d.tsv") == 0, "session 3: ngSpice_Reset after an aborted parse closes the log");
    if ((fp = fopen("probe-d.tsv", "r")) != NULL) {
        while (fgets(line, sizeof line, fp))
            if (strncmp(line, "#\tend\t", 6) == 0)
                snprintf(footer, sizeof footer, "%s", line);
        fclose(fp);
    }
    footer[strcspn(footer, "\n")] = '\0';
    printf("      session 3 footer: %s\n", footer);
    {
        const char *u = strstr(footer, "unlogged_calls=");
        if (u)
            unlogged = strtoull(u + 15, NULL, 10);
        check(strncmp(footer, "#\tend\tlogged=1\t", 14) == 0 && u && unlogged == 0,
              "session 3: the aborted set's footer reads logged=1 unlogged_calls=0");
    }
    printf("%s: %d failure(s)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
