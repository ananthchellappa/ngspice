/**********
Loop-gain (stability) analysis: set a parameter of the job.
**********/

#include "ngspice/ngspice.h"
#include "ngspice/ifsim.h"
#include "ngspice/iferrmsg.h"
#include "ngspice/acdefs.h"
#include "ngspice/stbdefs.h"
#include "ngspice/cktdefs.h"

#include "analysis.h"

/* The values are stored as they come: STBan() checks them when the job is
 * run, so that a bad line is reported once, there, with the probe named. */
int
STBsetParm(CKTcircuit *ckt, JOB *anal, int which, IFvalue *value)
{
    STBAN *job = (STBAN *) anal;

    NG_IGNORE(ckt);

    switch(which) {

    case STB_START:
        job->STBstartFreq = value->rValue;
        break;

    case STB_STOP:
        job->STBstopFreq = value->rValue;
        break;

    case STB_STEPS:
        job->STBnumberSteps = value->iValue;
        break;

    case STB_DEC:
        if(value->iValue) {
            job->STBstepType = DECADE;
        } else {
            if (job->STBstepType == DECADE) {
                job->STBstepType = 0;
            }
        }
        break;

    case STB_OCT:
        if(value->iValue) {
            job->STBstepType = OCTAVE;
        } else {
            if (job->STBstepType == OCTAVE) {
                job->STBstepType = 0;
            }
        }
        break;

    case STB_LIN:
        if(value->iValue) {
            job->STBstepType = LINEAR;
        } else {
            if (job->STBstepType == LINEAR) {
                job->STBstepType = 0;
            }
        }
        break;

    case STB_PROBE:
        job->STBprobe = value->sValue;
        break;

    case STB_ERROR:
        job->STBerror = value->sValue;
        break;

    case STB_LOCALGND:
        job->STBlocalGnd = value->sValue;
        break;

    case STB_DIAG:
        job->STBdiag = value->iValue;
        break;

    default:
        return(E_BADPARM);
    }
    return(OK);
}


static IFparm STBparms[] = {
    { "start",      STB_START,  IF_SET|IF_ASK|IF_REAL, "starting frequency" },
    { "stop",       STB_STOP,   IF_SET|IF_ASK|IF_REAL, "ending frequency" },
    { "numsteps",   STB_STEPS,IF_SET|IF_ASK|IF_INTEGER, "number of frequencies"},
    { "dec",        STB_DEC,    IF_SET|IF_FLAG, "step by decades" },
    { "oct",        STB_OCT,    IF_SET|IF_FLAG, "step by octaves" },
    { "lin",        STB_LIN,    IF_SET|IF_FLAG, "step linearly" },
    { "probe",      STB_PROBE,  IF_SET|IF_STRING, "voltage source in the loop" },
    { "error",      STB_ERROR,  IF_SET|IF_STRING, "error found in the analysis line" },
    { "localgnd",   STB_LOCALGND, IF_SET|IF_STRING, "reference node of the probe" },
    { "diag",       STB_DIAG,   IF_SET|IF_ASK|IF_INTEGER, "write the diagnostic vectors" }
};

SPICEanalysis STBinfo  = {
    {
        "STB",
        "Loop-gain (stability) analysis",

        NUMELEMS(STBparms),
        STBparms
    },
    sizeof(STBAN),
    FREQUENCYDOMAIN,
    1,
    STBsetParm,
    STBaskQuest,
    NULL,
    STBan
};
