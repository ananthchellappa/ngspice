#ifndef ngspice_RANDNUMB_H
#define ngspice_RANDNUMB_H

#include "ngspice/bool.h"
#include "ngspice/wordlist.h"
extern void com_sseed(wordlist *wl);
extern void setseedinfo(void);
/* '.option seed=N|random': seed, warning once if 'setseed N' was used;
   'written' is the option's value as the deck spells it */
extern void seed_from_option(int seed, bool is_random, const char *written);
/* the 'option seed=' command: seed once, no warning */
extern void seed_from_option_command(int seed);
/* more than one '.option seed=': warn, once per session */
extern void seed_option_multiple(void);
/* a '.option seed=' value that is not a seed: warn, once per session */
extern void seed_option_unusable(const char *written);
extern void rnd_reset_state(void);
/* the seed record and the call counter, for the draw log (drawlog.h) */
extern void rng_seed_info(int *seed, const char **src, unsigned long long *offset);
extern unsigned long long rng_ncalls(void);
extern unsigned long long rng_draw_start(void);
extern void rng_deck_parsed(void);

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
