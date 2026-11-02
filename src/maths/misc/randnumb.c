/**********
Copyright 2008 Holger Vogt
**********/
/* Care about random numbers
   The seed value is set as random number in main.c, line 746.
   A fixed seed value may be set by 'set rndseed=value'.
*/


/*
	CombLCGTaus()
	Combined Tausworthe & LCG random number generator
	Algorithm has been suggested in: 
	GPUGems 3, Addison Wesley, 2008, Chapter 37.
	It combines a three component Tausworthe generator taus88 
	(see P. L’Ecuyer: "Maximally equidistributed combined Tausworthe
	generators", Mathematics of Computation, 1996, 
	http://www.iro.umontreal.ca/~lecuyer/myftp/papers/tausme.ps )
	and a quick linear congruent generator (LCG), decribed in:
	Press: "Numerical recipes in C", Cambridge, 1992, p. 284.
	Generator has passed the bbattery_SmallCrush(gen) test of the
	TestU01 library from Pierre L’Ecuyer and Richard Simard,
	http://www.iro.umontreal.ca/~simardr/testu01/tu01.html
*/


/* TausSeed creates three start values for Tausworthe state variables.
   Uses rand() from <stdlib.h>, therefore values depend on the value of 
   seed in srand(seed). A constant seed will result in a reproducible
   series of random variates.
   
   Calling sequence:
   srand(seed);
   TausSeed();
   double randvar = CombLCGTaus(void);
*/
//#define HVDEBUG

#include "ngspice/ngspice.h"
#include "ngspice/cpdefs.h"
#include "ngspice/ftedefs.h"
#include "ngspice/randnumb.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include <process.h>
#else
#include <unistd.h>
#endif
#include <stdarg.h>			// var. argumente

/* Tausworthe state variables for double variates*/
static unsigned CombState1 = 129, CombState2 = 130, CombState3 = 131;  
static unsigned CombState4 = 132; /* LCG state variable */ 

/* Tausworthe state variables for integer variates*/
static unsigned CombState5 = 133, CombState6 = 135, CombState7 = 137;  
static unsigned CombState8 = 138; /* LCG state variable */ 

static unsigned TauS(unsigned *state, int C1, int C2, int C3, unsigned m);
static unsigned LGCS(unsigned *state, unsigned A1, unsigned A2);

double CombLCGTaus(void);
float  CombLCGTaus2(void);

void rgauss(double* py1, double* py2);
static bool seedinfo = FALSE;

/* '.option seed=' versus 'setseed'.  eval_opt() re-seeds from the option on
   every parse, and 'reset' is a parse, so the option overrides a 'setseed N'
   or an 'option seed=' made before the reset.  The option keeps winning;
   seed_from_option() only says so, once per session, and only after one of
   those.  A bare 'setseed', which re-seeds from $rndseed, sets nothing, and
   seed_explicit names the last one made.
   eval_opt()'s warnings about more than one '.option seed=', and about a
   'seed=' value that is not a seed, are printed once per session too, by
   seed_option_multiple() and seed_option_unusable(), not on every parse.  The
   flags are cleared by rnd_reset_state(), called from totalreset() in
   sharedspice.c, so a new shared-library session warns again. */
static const char *seed_explicit = NULL;
static bool seed_override_warned = FALSE;
static bool seed_multiple_warned = FALSE;
static bool seed_unusable_warned = FALSE;

/* The seed record, for the draw log (drawlog.c).  rng_calls counts the
   CombLCGTaus() calls, rng_calls_at_seed is its value at the last seeding,
   and rng_draw_mark its value when the last gauss1() or drand() began.  The
   srand()/TausSeed() pairs below are unchanged; rng_note_seed() only records
   what they did.  rng_src_next lets seed_from_option() name itself to
   com_sseed(NULL), and rng_deck_seen tells a bare 'setseed' from the
   startup seeding.  rnd_reset_state() clears all of it. */
static unsigned long long rng_calls = 0;
static unsigned long long rng_calls_at_seed = 0;
static unsigned long long rng_draw_mark = 0;
static int rng_seed = 0;
static const char *rng_src = "startup";
static const char *rng_src_next = NULL;
static bool rng_deck_seen = FALSE;

static void
rng_note_seed(int seed, const char *src)
{
    rng_seed = seed;
    rng_src = src;
    rng_calls_at_seed = rng_calls;
}


/* Check if a seed has been set by the command 'set rndseed=value'
   in spinit, .spiceinit or in a .control section
   with integer value > 0. If available, call srand(value).
   rndseed set in main.c to 1, if no 'set rndseed=val' is given.
   Called from functions in cmath2.c.
*/
void checkseed(void)
{
   int newseed;
   static int oldseed;
/*   printf("Enter checkseed()\n"); */
   if (cp_getvar("rndseed", CP_NUM, &newseed, 0)) {
      if ((newseed > 0) && (oldseed != newseed)) {
         srand((unsigned int)newseed);
         TausSeed();
         rng_note_seed(newseed, "checkseed");
         if (oldseed > 0) /* no printout upon start-up */
             printf("Seed value for random number generator is set to %d\n", newseed);
         oldseed = newseed;
      }
   }
   
}

/* uniform random number generator, interval [-1 .. +1[ */
double drand(void)
{
   rng_draw_mark = rng_calls;
   return 2.0 * CombLCGTaus() - 1.0;
}


void TausSeed(void)
{    
   /* The Tausworthe initial states should be greater than 128.
      We restrict the values up to 32767. 
      Here we use the standard random functions srand, called in main.c
      upon ngspice startup or later in fcn checkseed(),
      rand() and the maximum return value RAND_MAX*/
   CombState1 = (unsigned int)((double)rand()/(double)RAND_MAX * 32638.) + 129;
   CombState2 = (unsigned int)((double)rand()/(double)RAND_MAX * 32638.) + 129;
   CombState3 = (unsigned int)((double)rand()/(double)RAND_MAX * 32638.) + 129;
   CombState4 = (unsigned int)((double)rand()/(double)RAND_MAX * 32638.) + 129;
   CombState5 = (unsigned int)((double)rand()/(double)RAND_MAX * 32638.) + 129;
   CombState6 = (unsigned int)((double)rand()/(double)RAND_MAX * 32638.) + 129;
   CombState7 = (unsigned int)((double)rand()/(double)RAND_MAX * 32638.) + 129;
   CombState8 = (unsigned int)((double)rand()/(double)RAND_MAX * 32638.) + 129;

#ifdef HVDEBUG
   printf("\nTausworthe Double generator init states: %d, %d, %d, %d\n", 
      CombState1, CombState2, CombState3, CombState4);
   printf("Tausworthe Integer generator init states: %d, %d, %d, %d\n", 
      CombState5, CombState6, CombState7, CombState8);
#endif
}   

static unsigned TauS(unsigned *state, int C1, int C2, int C3, unsigned m)
{
   unsigned b = (((*state << C1) ^ *state) >> C2);
   return *state = (((*state & m) << C3) ^ b);
}

static unsigned LGCS(unsigned *state, unsigned A1, unsigned A2)
{
   return *state = (A1 * *state + A2);
}

/* generate random variates randvar uniformly distributed in 
   [0.0 .. 1.0[ by calls to CombLCGTaus() like:
   double randvar = CombLCGTaus(); 
*/
double CombLCGTaus(void)
{
   rng_calls++;
   return 2.3283064365387e-10 * (
   TauS(&CombState1, 13, 19, 12, 4294967294UL) ^
   TauS(&CombState2, 2, 25, 4, 4294967288UL) ^
   TauS(&CombState3, 3, 11, 17, 4294967280UL) ^
   LGCS(&CombState4, 1664525, 1013904223UL)
   );
}

/* generate random variates randvarint uniformly distributed in 
   [0 .. 4294967296[ (32 bit unsigned int) by calls to CombLCGTausInt() like:
   unsigned int randvarint = CombLCGTausInt(); 
*/   
unsigned int CombLCGTausInt(void)
{
   return (
   TauS(&CombState5, 13, 19, 12, 4294967294UL) ^
   TauS(&CombState6, 2, 25, 4, 4294967288UL) ^
   TauS(&CombState7, 3, 11, 17, 4294967280UL) ^
   LGCS(&CombState8, 1664525, 1013904223UL)
   );
}
  
/* test versions of the generators listed above */
float CombLCGTaus2(void)
{
   unsigned long b;
   b = (((CombState1 << 13) ^ CombState1) >> 19);
   CombState1 = (unsigned int)(((CombState1 & 4294967294UL) << 12) ^ b);
   b = (((CombState2 << 2) ^ CombState2) >> 25);
   CombState2 = (unsigned int)(((CombState2 & 4294967288UL) << 4) ^ b);   
   b = (((CombState3 << 3) ^ CombState3) >> 11);
   CombState3 = (unsigned int)(((CombState3 & 4294967280UL) << 17) ^ b);
   CombState4 = (unsigned int)(1664525 * CombState4 + 1013904223UL);   
   return ((float)(CombState1 ^ CombState2 ^ CombState3 ^ CombState4) *  2.3283064365387e-10f);
}


unsigned int CombLCGTausInt2(void)
{
   unsigned long b;
   b = (((CombState5 << 13) ^ CombState5) >> 19);
   CombState5 = (unsigned int)(((CombState5 & 4294967294UL) << 12) ^ b);
   b = (((CombState6 << 2) ^ CombState6) >> 25);
   CombState6 = (unsigned int)(((CombState6 & 4294967288UL) << 4) ^ b);   
   b = (((CombState7 << 3) ^ CombState7) >> 11);
   CombState7 = (unsigned int)(((CombState7 & 4294967280UL) << 17) ^ b);
   CombState8 = (unsigned int)(1664525 * CombState8 + 1013904223UL);   
   return (CombState5 ^ CombState6 ^ CombState7 ^ CombState8);
}


/***  gauss  ***
 for speed reasons get two values per pass */
double gauss0(void)
{
  static bool gliset = TRUE;
  static double glgset = 0.0;
  double fac,r,v1,v2;
  if (gliset) {
    do {
      v1 = 2.0 * CombLCGTaus() - 1.0;
      v2 = 2.0 * CombLCGTaus() - 1.0;
      r = v1*v1 + v2*v2;
    } while (r >= 1.0);
/*    printf("v1 %f, v2 %f\n", v1, v2); */
    fac = sqrt(-2.0 * log(r) / r);
    glgset = v1 * fac;
    gliset = FALSE;
    return v2 * fac;
  } else {
    gliset = TRUE;
    return glgset;
  }
}


/***  gauss  ***
to be reproducible, we just use one value per pass */
double gauss1(void)
{
    double fac, r, v1, v2;
    rng_draw_mark = rng_calls;
    do {
        v1 = 2.0 * CombLCGTaus() - 1.0;
        v2 = 2.0 * CombLCGTaus() - 1.0;
        r = v1 * v1 + v2 * v2;
    } while (r >= 1.0);
    /*    printf("v1 %f, v2 %f\n", v1, v2); */
    fac = sqrt(-2.0 * log(r) / r);
    return v2 * fac;
}


/* Polar form of the Box-Muller generator for Gaussian distributed
   random variates.
   Generator will be fed with two uniformly distributed random variates.
   Delivers two values per call
*/

void rgauss(double* py1, double* py2)
{
    double x1, x2, w;

    do {
        x1 = 2.0 * CombLCGTaus() - 1.0;
        x2 = 2.0 * CombLCGTaus() - 1.0;
        w = x1 * x1 + x2 * x2;
    } while ( w >= 1.0 );

     w = sqrt( (-2.0 * log( w ) ) / w );

    *py1 = x1 * w;
    *py2 = x2 * w;
}



/** Code by: Inexpensive
    http://everything2.com/title/Generating+random+numbers+with+a+Poisson+distribution **/
int poisson(double lambda)
{
  int k=0;                          //Counter
  const int max_k = 1000;           //k upper limit
  double p = CombLCGTaus();         //uniform random number
  double P = exp(-lambda);        //probability
  double sum=P;                     //cumulant
  if (sum>=p) return 0;             //done allready
  for (k=1; k<max_k; ++k) {         //Loop over all k:s
    P*=lambda/(double)k;           //Calc next prob
    sum+=P;                         //Increase cumulant
    if (sum>=p) break;              //Leave loop
  }
  return k;                         //return random number
}


/* return an exponentially distributed random number */
double exprand(double mean)
{
    double expval;
    expval = -log(CombLCGTaus()) * mean;
    return expval;
}


/* seed random number generators immediately
* command "setseed"
*   take value of variable rndseed as seed
* command "setseed <n>"
*   seed with number <n>
* Only the second form counts as an explicit 'setseed' for the
* '.option seed=' warning in seed_from_option().
*/
void
com_sseed(wordlist *wl)
{
    int newseed;

    if (wl == NULL) {
        if (!cp_getvar("rndseed", CP_NUM, &newseed, 0)) {
            newseed = getpid();
            cp_vset("rndseed", CP_NUM, &newseed);
        }
        srand((unsigned int)newseed);
        TausSeed();
        rng_note_seed(newseed, rng_src_next ? rng_src_next :
                      rng_deck_seen ? "setseed" : "startup");
    }
    else if ((sscanf(wl->wl_word, " %d ", &newseed) != 1) ||
        (newseed <= 0) || (newseed > INT_MAX))
    {
        fprintf(cp_err,
            "\nWarning: Cannot use %s as seed!\n"
            "    Command 'setseed %s' ignored.\n\n",
            wl->wl_word, wl->wl_word);
        return;
    }
    else {
        srand((unsigned int)newseed);
        TausSeed();
        cp_vset("rndseed", CP_NUM, &newseed);
        seed_explicit = "'setseed'";
        rng_note_seed(newseed, "setseed");
    }

    if (seedinfo)
        printf("\nSeed value for random number generator is set to %d\n", newseed);
}


void
setseedinfo(void)
{
    seedinfo = TRUE;
}


/* Seed from an option, either form: the draw log labels it src=option. */
static void
seed_now(int seed)
{
    cp_vset("rndseed", CP_NUM, &seed);
    rng_src_next = "option";
    com_sseed(NULL);
    rng_src_next = NULL;
}


/* Seed from '.option seed=N' or '.option seed=random', for eval_opt().  This
   is the cp_vset() + com_sseed(NULL) pair eval_opt() has always done, with
   the warning in front of it.  The warning quotes 'written', the value as
   the deck spells it, so '.option seed=05' is not shown as 'seed=5'. */
void
seed_from_option(int seed, bool is_random, const char *written)
{
    if (seed_explicit && !seed_override_warned) {
        if (is_random)
            fprintf(cp_err,
                "Warning: '.option seed=%s' re-seeds the random number generator from the clock\n"
                "    each time the circuit is parsed, by 'source' and by every 'reset', so it\n"
                "    overrides %s. Each seed it uses is stored in $rndseed. Remove the\n"
                "    option to let %s choose the seed.\n"
                "    (This warning is printed once per session.)\n",
                written, seed_explicit, seed_explicit);
        else
            fprintf(cp_err,
                "Warning: '.option seed=%s' re-seeds the random number generator each time the\n"
                "    circuit is parsed, by 'source' and by every 'reset', so it overrides %s.\n"
                "    Remove the option to let %s choose the seed.\n"
                "    (This warning is printed once per session.)\n",
                written, seed_explicit, seed_explicit);
        seed_override_warned = TRUE;
    }
    seed_now(seed);
}


/* Seed from the 'option seed=' command, for com_option().  The command seeds
   once, when it is given, so seed_from_option()'s warning about every parse is
   not about the command itself, but a '.option seed=' in the deck overrides it
   at the next parse, as it does 'setseed N', and that warning says so.  The
   draw log labels it src=option. */
void
seed_from_option_command(int seed)
{
    seed_explicit = "'option seed='";
    seed_now(seed);
}


/* For eval_opt(): the option deck has more than one 'seed='.  eval_opt()
   runs on every parse, so this is printed once per session, not once per
   'reset'. */
void
seed_option_multiple(void)
{
    if (!seed_multiple_warned) {
        fprintf(cp_err,
            "Warning: Multiple 'option seed=val|random' found!\n"
            "    (This warning is printed once per session.)\n");
        seed_multiple_warned = TRUE;
    }
}


/* For eval_opt(): a 'seed=' value that atoi() does not turn into a positive
   seed, quoted as the deck wrote it.  The option is skipped on every parse;
   this says so once per session. */
void
seed_option_unusable(const char *written)
{
    if (!seed_unusable_warned) {
        fprintf(cp_err,
            "Warning: Cannot convert 'option seed=%s' to seed value, skipped!\n"
            "    (This warning is printed once per session.)\n",
            written);
        seed_unusable_warned = TRUE;
    }
}


/* Forget the setseed and warning flags, for a new session. */
void
rnd_reset_state(void)
{
    seed_explicit = NULL;
    seed_override_warned = FALSE;
    seed_multiple_warned = FALSE;
    seed_unusable_warned = FALSE;
    rng_calls = rng_calls_at_seed = rng_draw_mark = 0;
    rng_seed = 0;
    rng_src = "startup";
    rng_src_next = NULL;
    rng_deck_seen = FALSE;
}


/* For the draw log: the last seed, how it was set, and the uniform-generator
   calls made since. */
void
rng_seed_info(int *seed, const char **src, unsigned long long *offset)
{
    *seed = rng_seed;
    *src = rng_src;
    *offset = rng_calls - rng_calls_at_seed;
}


unsigned long long
rng_ncalls(void)
{
    return rng_calls;
}


/* rng_ncalls() when the last gauss1() or drand() began */
unsigned long long
rng_draw_start(void)
{
    return rng_draw_mark;
}


/* A deck is being parsed: a bare 'setseed' from now on is a 'setseed'. */
void
rng_deck_parsed(void)
{
    rng_deck_seen = TRUE;
}
