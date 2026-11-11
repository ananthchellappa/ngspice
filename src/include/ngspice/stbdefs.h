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

#endif
