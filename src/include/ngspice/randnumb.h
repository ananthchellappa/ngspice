#ifndef ngspice_RANDNUMB_H
#define ngspice_RANDNUMB_H

#include "ngspice/bool.h"
#include "ngspice/wordlist.h"
extern void com_sseed(wordlist *wl);
extern void setseedinfo(void);
/* '.option seed=N|random': seed, warning once if 'setseed N' was used */
extern void seed_from_option(int seed, bool is_random);
extern void rnd_reset_state(void);

/* initialize random number generators */
extern void initw(void);

extern void checkseed(void);    /* seed random or set by 'set rndseed=value'*/
extern double drand(void);
extern double gauss0(void);
extern double gauss1(void);
extern int poisson(double);
extern double exprand(double);

extern void TausSeed(void);
extern unsigned int CombLCGTausInt(void);
extern unsigned int CombLCGTausInt2(void);

#endif
