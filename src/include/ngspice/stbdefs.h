/**********
Loop-gain (stability) analysis "stb".

M. Tian, V. Visvanathan, J. Hantgan, K. Kundert, "Striving for
small-signal stability", IEEE Circuits and Devices Magazine,
vol. 17, no. 1, 2001.
**********/

#ifndef ngspice_STBDEFS_H
#define ngspice_STBDEFS_H

#include "ngspice/jobdefs.h"

/* the titles of the plots, by which the front end knows them */
#define STB_PLOT_TITLE    "STB Analysis"
#define STB_MARGIN_TITLE  "STB Margins"
/* the operating point, a third plot made for the option keepopinfo */
#define STB_OP_TITLE      "STB Operating Point"

    /* structure used to describe a loop-gain analysis to be performed */

typedef struct {
    int JOBtype;
    JOB *JOBnextJob;    /* pointer to next thing to do */
    char *JOBname;      /* name of this job */
    double STBstartFreq;
    double STBstopFreq;
    double STBfreqDelta; /* multiplier for decade/octave stepping, */
                         /* step for linear steps. */
    int STBstepType;    /* DECADE, OCTAVE or LINEAR of acdefs.h, 0: not given */
    int STBnumberSteps;
    char *STBprobe;     /* the probe, as the user wrote it */
    char *STBerror;     /* what was wrong with the line that made this job, */
                        /* reported when the job is run; NULL: nothing */
} STBAN;

enum {
    STB_DEC = 1,
    STB_OCT,
    STB_LIN,
    STB_START,
    STB_STOP,
    STB_STEPS,
    STB_PROBE,
    STB_ERROR,
};

/* The margins of a loop gain, as STBmargin() finds them. */

typedef struct {
    double pmFreq;      /* where |T| falls through 0 dB, NAN: nowhere */
    double pm;          /* the phase margin, in degrees */
    double gmFreq;      /* where the phase of T falls through -180 degrees */
    double gm;          /* the gain margin, in dB */
    int status;         /* STB_MG_OK, or why there are no margins at all */
    int pmWhy;          /* status STB_MG_OK: STB_PM_FOUND, or why not */
    int gmFound;        /* status STB_MG_OK: there is a gain margin */
    int crossings;      /* the 0 dB crossings, rising and falling */
    double riseFreq;    /* a rising 0 dB crossing before the falling one */
                        /* that the margins are taken from, NAN: none */
    double atFreq;      /* the frequency that status is about */
    double atVal;       /* STB_MG_POSFB: the phase of -T there, in degrees */
    int nBad;           /* STB_MG_NONFINITE: the number of such points */
    double lastFreq;    /* status STB_MG_OK: the last point, */
    double lastDb;      /* |T| there, in dB, */
    double lastPhase;   /* and the phase of -T there, in degrees */
    double maxDb;       /* status STB_MG_OK: the largest |T|, in dB, */
    double maxDbFreq;   /* and its frequency */
} STBmargins;

enum {
    STB_MG_OK = 0,
    STB_MG_NONFINITE,   /* T is not finite at some frequency */
    STB_MG_ZEROALL,     /* |T| is below 1e-12 at every frequency */
    STB_MG_SHORT,       /* fewer than two points */
    STB_MG_BADFREQ,     /* a frequency is not positive, or not rising */
    STB_MG_ZERO,        /* T is exactly zero at some frequency */
    STB_MG_POSFB,       /* the real part of T is negative at the first point */
};

enum {
    STB_PM_FOUND = 0,
    STB_PM_STILL_ABOVE, /* |T| is above 0 dB at the last point */
    STB_PM_ALWAYS_BELOW, /* |T| does not come from above 0 dB */
};

extern void STBmargin(int n, const double *freq, const double *tre,
                      const double *tim, STBmargins *m);

#endif
