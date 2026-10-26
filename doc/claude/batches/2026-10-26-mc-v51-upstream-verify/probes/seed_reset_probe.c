/* Scratch adaptation of doc/claude/batches/2026-10-24-mc-v51/probes/
 * rnddraws_reset_probe.c for upstream (pre-master-48 + p1, p4-crash/on-p1,
 * p2).  The draw-log (fork-only P4) checks are gone.  What is left are the
 * init / reset / re-seed paths and the P2 warning flags, run through
 * libngspice across ngSpice_Init / ngSpice_Reset / ngSpice_Init. */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ngspice/sharedspice.h"

static int failures = 0;
static char outbuf[65536];
static size_t outlen = 0;

static int cb_char(char *s, int id, void *p)
{
    (void) id; (void) p;
    size_t n = strlen(s);
    if (outlen + n + 2 < sizeof outbuf) {
        memcpy(outbuf + outlen, s, n);
        outlen += n;
        outbuf[outlen++] = '\n';
        outbuf[outlen] = '\0';
    }
    return 0;
}
static int cb_exit(int st, NG_BOOL a, NG_BOOL b, int id, void *p)
{ (void) a; (void) b; (void) id; (void) p; printf("EXIT %d\n", st); return 0; }

static void clear(void) { outlen = 0; outbuf[0] = '\0'; }
static int count(const char *needle)
{
    int n = 0;
    for (const char *q = outbuf; (q = strstr(q, needle)) != NULL; q += strlen(needle))
        n++;
    return n;
}
static void cmd(const char *c)
{ char buf[256]; snprintf(buf, sizeof buf, "%s", c); ngSpice_Command(buf); }
static void check(bool ok, const char *what)
{ printf("%s  %s\n", ok ? "ok  " : "FAIL", what); if (!ok) failures++; }

/* print @r1[r] and return the value */
static double r1(void)
{
    clear();
    cmd("op");
    cmd("print @r1[r]");
    const char *q = strstr(outbuf, "@r1[r] = ");
    return q ? strtod(q + 9, NULL) : -1.0;
}
static int rndseed(void)
{
    clear();
    cmd("echo rndseed=$rndseed");
    const char *q = strstr(outbuf, "rndseed=");
    return q ? atoi(q + 8) : -1;
}

static char d0[] = "* seed reset probe";
static char d1[] = ".param rv=agauss(1k,100,1)";
static char d2[] = "v1 1 0 1";
static char d3[] = "r1 1 0 {rv}";
static char d4[] = ".end";
static char *plain[] = { d0, d1, d2, d3, d4, NULL };

static char o0[] = "* seed option probe";
static char o1[] = ".param rv=agauss(1k,100,1)";
static char o2[] = ".option seed=5";
static char o3[] = "v1 1 0 1";
static char o4[] = "r1 1 0 {rv}";
static char o5[] = ".end";
static char *withopt[] = { o0, o1, o2, o3, o4, o5, NULL };

static char m0[] = "* seed multiple/unusable probe";
static char m1[] = ".param rv=agauss(1k,100,1)";
static char m2[] = ".option seed=abc";
static char m3[] = ".option seed=7";
static char m4[] = "v1 1 0 1";
static char m5[] = "r1 1 0 {rv}";
static char m6[] = ".end";
static char *multi[] = { m0, m1, m2, m3, m4, m5, m6, NULL };

#define OVR "re-seeds the random number generator"
#define MUL "Multiple 'option seed=val|random' found!"
#define UNU "Cannot convert 'option seed=abc'"

int main(void)
{
    double s1_start, s1_seed1, s2_start, s2_seed1;
    int rs;

    /* session 1: startup stream and $rndseed */
    ngSpice_Init(cb_char, NULL, cb_exit, NULL, NULL, NULL, NULL);
    rs = rndseed();
    check(rs == 1, "session 1: $rndseed reads 1 after ngSpice_Init");
    ngSpice_Circ(plain);
    s1_start = r1();
    cmd("setseed 1");
    cmd("reset");
    s1_seed1 = r1();
    printf("      session 1: startup %.6e, after setseed 1 %.6e\n", s1_start, s1_seed1);
    check(s1_start > 0 && s1_start == s1_seed1, "session 1: the first parse draws what 'setseed 1' draws");

    /* P2 override warning: 'setseed 1' above was explicit, so it is printed,
       once, over four parses */
    clear();
    ngSpice_Circ(withopt);
    cmd("reset");
    cmd("setseed 2"); cmd("reset");
    cmd("setseed 3"); cmd("reset");
    check(count(OVR) == 1, "session 1: the override warning is printed once over four parses");

    /* multiple / unusable, once per session */
    clear();
    ngSpice_Circ(multi);
    cmd("reset"); cmd("reset");
    printf("      session 1: multiple=%d unusable=%d (3 parses)\n", count(MUL), count(UNU));
    check(count(MUL) == 1 && count(UNU) == 1, "session 1: multiple and unusable warnings once each");

    ngSpice_Reset();

    /* session 2: everything starts over */
    ngSpice_Init(cb_char, NULL, cb_exit, NULL, NULL, NULL, NULL);
    rs = rndseed();
    check(rs == 1, "session 2: $rndseed reads 1 again after ngSpice_Reset + ngSpice_Init");
    ngSpice_Circ(plain);
    s2_start = r1();
    cmd("setseed 1");
    cmd("reset");
    s2_seed1 = r1();
    printf("      session 2: startup %.6e, after setseed 1 %.6e\n", s2_start, s2_seed1);
    check(s2_start == s1_start, "session 2: the startup draw equals session 1's (generator re-seeded)");
    check(s2_seed1 == s1_seed1, "session 2: 'setseed 1' draws what it drew in session 1");

    clear();
    ngSpice_Circ(withopt);
    cmd("setseed 2"); cmd("reset");
    check(count(OVR) == 1, "session 2: the override warning is printed again (flags cleared by totalreset)");
    clear();
    ngSpice_Circ(multi);
    cmd("reset");
    check(count(MUL) == 1 && count(UNU) == 1, "session 2: multiple and unusable warnings printed again");
    clear();
    cmd("reset");
    check(count(MUL) == 0 && count(UNU) == 0 && count(OVR) == 0, "session 2: and not repeated on the next reset");

    ngSpice_Reset();

    /* session 3: no 'setseed N' yet in this session, so the option is
       silent; the first 'setseed N' makes it warn */
    ngSpice_Init(cb_char, NULL, cb_exit, NULL, NULL, NULL, NULL);
    clear();
    ngSpice_Circ(withopt);
    cmd("reset"); cmd("setseed"); cmd("reset");
    check(count(OVR) == 0, "session 3: .option seed= with no 'setseed N' (bare 'setseed' only) is silent");
    clear();
    cmd("setseed 4"); cmd("reset");
    check(count(OVR) == 1, "session 3: the first 'setseed N' makes it warn");

    ngSpice_Reset();
    printf("%s: %d failure(s)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
