/**********
Loop-gain (stability) analysis: ask for a parameter of the job.
**********/

#include "ngspice/ngspice.h"
#include "ngspice/ifsim.h"
#include "ngspice/iferrmsg.h"
#include "ngspice/acdefs.h"
#include "ngspice/stbdefs.h"
#include "ngspice/cktdefs.h"


int
STBaskQuest(CKTcircuit *ckt, JOB *anal, int which, IFvalue *value)
{
    STBAN *job = (STBAN *) anal;

    NG_IGNORE(ckt);

    switch(which) {

    case STB_START:
        value->rValue = job->STBstartFreq;
        break;

    case STB_STOP:
        value->rValue = job->STBstopFreq;
        break;

    case STB_STEPS:
        value->iValue = job->STBnumberSteps;
        break;

    case STB_DEC:
        value->iValue = (job->STBstepType == DECADE);
        break;

    case STB_OCT:
        value->iValue = (job->STBstepType == OCTAVE);
        break;

    case STB_LIN:
        value->iValue = (job->STBstepType == LINEAR);
        break;

    default:
        return(E_BADPARM);
    }
    return(OK);
}
