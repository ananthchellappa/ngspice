/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1988 Wayne A. Christopher, U. C. Berkeley CAD Group
Modified: 2000 AlansFixes, 2013/2015 patch by Krzysztof Blaszkowski
**********/

/*
 * This module replaces the old "writedata" routines in nutmeg.
 * Unlike the writedata routines, the OUT routines are only called by
 * the simulator routines, and only call routines in nutmeg.  The rest
 * of nutmeg doesn't deal with OUT at all.
 */

#include "ngspice/ngspice.h"
#include "ngspice/cpdefs.h"
#include "ngspice/ftedefs.h"
#include "ngspice/dvec.h"
#include "ngspice/plot.h"
#include "ngspice/sim.h"
#include "ngspice/inpdefs.h"        /* for INPtables */
#include "ngspice/ifsim.h"
#include "ngspice/jobdefs.h"
#include "ngspice/iferrmsg.h"
#include "circuits.h"
#include "outitf.h"
#include "variable.h"
#include <fcntl.h>
#include "ngspice/cktdefs.h"
#include "breakp2.h"
#include "runcoms.h"
#include "plotting/graf.h"
#include "../misc/misc_time.h"

#ifdef XSPICE
/* Evt_Ckt_Has_Node(): the event half of ckt_knows_name()'s question. */
#include "ngspice/evt.h"
#include "ngspice/evtproto.h"
#endif

extern char *spice_analysis_get_name(int index);
extern char *spice_analysis_get_description(int index);
extern int EVTsetup_plot(CKTcircuit* ckt, char* plotname);

extern IFsimulator SIMinfo;
extern char Spice_Build_Date[];

extern char* eng(double value, int digits, bool numeric, bool bytes);

extern unsigned long long getMemorySize(void);
extern unsigned long long getPeakRSS(void);
extern unsigned long long getCurrentRSS(void);
extern unsigned long long getAvailableMemorySize(void);

static int beginPlot(JOB *analysisPtr, CKTcircuit *circuitPtr, char *cktName, char *analName,
                     char *refName, int refType, int numNames, char **dataNames, int dataType,
                     bool windowed, runDesc **runp);
static int addDataDesc(runDesc *run, char *name, int type, int ind, int meminit);
static int addSpecialDesc(runDesc *run, char *name, char *devname, char *param, int depind, int meminit);
static void fileInit(runDesc *run);
static void fileInit_pass2(runDesc *run);
static void fileStartPoint(FILE *fp, bool bin, int num);
static void fileAddRealValue(FILE *fp, bool bin, double value);
static void fileAddComplexValue(FILE *fp, bool bin, IFcomplex value);
static void fileEndPoint(FILE *fp, bool bin);
static void fileEnd(runDesc *run);
static void plotInit(runDesc *run);
static void plotAddRealValue(dataDesc *desc, double value);
static void plotAddComplexValue(dataDesc *desc, IFcomplex value);
static void plotEnd(runDesc *run);
static bool parseSpecial(char *name, char *dev, char *param, char *ind);
static bool name_eq(char *n1, char *n2);
static bool name_eq_query(char *typed, char *stored);
static bool ckt_knows_name(CKTcircuit *ckt, char *name);
static bool case_twin_p(const char *token, const char *stored);
static char *ckt_case_twin(CKTcircuit *ckt, char *name);
static bool save_miss_first_time(char *name);
static bool report_save_case_miss(runDesc *run, char *name, int numNames, char **dataNames);
static bool getSpecial(dataDesc *desc, runDesc *run, IFvalue *val);
static void freeRun(runDesc *run);
static int InterpFileAdd(runDesc *plotPtr, IFvalue *refValue, IFvalue *valuePtr);
static int InterpPlotAdd(runDesc *plotPtr, IFvalue *refValue, IFvalue *valuePtr);
static inline int vlength2delta(int len);

/*Output data to spice module*/
#ifdef TCL_MODULE
#include "ngspice/tclspice.h"
#elif defined SHARED_MODULE
extern int sh_ExecutePerLoop(void);
extern int sh_vecinit(runDesc *run);
#endif

/*Suppressing progress info in -o option */
#ifndef HAS_WINGUI
extern bool orflag;
#endif

// fixme
//   ugly hack to work around missing api to specify the "type" of signals
int fixme_onoise_type = SV_NOTYPE;
int fixme_inoise_type = SV_NOTYPE;


#define DOUBLE_PRECISION    15


static clock_t lastclock, currclock, startclock;
static double *rowbuf;
static size_t column, rowbuflen;

static bool shouldstop = FALSE; /* Tell simulator to stop next time it asks. */

static bool interpolated = FALSE;
static double *valueold, *valuenew;

#ifdef SHARED_MODULE
static bool savenone = FALSE;
#endif

/* The two "begin plot" routines share all their internals... */

int
OUTpBeginPlot(CKTcircuit *circuitPtr, JOB *analysisPtr,
              IFuid analName,
              IFuid refName, int refType,
              int numNames, IFuid *dataNames, int dataType, runDesc **plotPtr)
{
    char *name;
    int ret, n;
    runDesc* plot;

    if (ft_curckt->ci_ckt == circuitPtr)
        name = ft_curckt->ci_name;
    else
        name = "circuit name";

    ret = beginPlot(analysisPtr, circuitPtr, name,
                      analName, refName, refType, numNames,
                      dataNames, dataType, FALSE,
                      plotPtr);
    plot = *plotPtr;
    n = plot->numData;

    if (!cp_getvar("no_mem_check", CP_BOOL, NULL, 0)) {
        /* Estimate the required memory */
        double timesteps = ft_curckt->ci_ckt->CKTfinalTime / ft_curckt->ci_ckt->CKTstep;
        double dmemrequ = timesteps * (double)n * sizeof(double);
        double dmemavail = (double)getAvailableMemorySize();
        char *cmemrequ = eng(dmemrequ, 6, FALSE, TRUE);
        char *cmemavail = eng(dmemavail, 6, FALSE, TRUE);
        char *ctimesteps = eng(timesteps, 6, TRUE, FALSE);
        if (dmemrequ > dmemavail) {
#ifdef _WIN32
            fprintf(stderr, "\nError: memory required (%sB), made of\n"
                "       %d nodes and approximately %s time steps,\n"
                "       is more than the memory available (%sB)!\n",
                cmemrequ, n,ctimesteps, cmemavail);
            fprintf(stderr, "Setting the output memory is not possible.\n");
            tfree(cmemrequ);
            tfree(cmemavail);
            tfree(ctimesteps);
            controlled_exit(1);
#else
            fprintf(stderr, "\nWarning: memory required (%sB), made of\n"
                "       %d nodes and approximately %s time steps,\n"
                "       is more than the DRAM memory available (%sB)!\n",
                cmemrequ, n, ctimesteps, cmemavail);
            fprintf(stderr, "       Swapping data to SSD may slow down the simulation.\n");
#endif
        }
        tfree(cmemrequ);
        tfree(cmemavail);
        tfree(ctimesteps);
    }

    return ret;
}


static int
beginPlot(JOB *analysisPtr, CKTcircuit *circuitPtr, char *cktName, char *analName, char *refName, int refType, int numNames, char **dataNames, int dataType, bool windowed, runDesc **runp)
{
    runDesc *run;
    struct save_info *saves;
    bool *savesused = NULL;
    int numsaves;
    int i, j, depind = 0;
    char namebuf[BSIZE_SP], parambuf[BSIZE_SP], depbuf[BSIZE_SP];
    char *ch, tmpname[BSIZE_SP];
    bool saveall  = TRUE;
    bool savealli = FALSE;
    bool savenosub = FALSE;
    bool savenointernals = FALSE;
    char *an_name;
    int initmem;

    /*to resume a run, Reassign the file pointer and return
      (requires *runp to be NULL if this is not needed)*/
    if (dataType == 666 && numNames == 666) {
        run = *runp;
        run->writeOut = ft_getOutReq(&run->fp, &run->runPlot, &run->binary,
                                     run->type, run->name);

    } else {
        /*end saj*/

        /* Check to see if we want to print informational data. */
        if (cp_getvar("printinfo", CP_BOOL, NULL, 0))
            fprintf(cp_err, "(debug printing enabled)\n");

        /* Check to see if we want to save only interpolated data. */
        if (cp_getvar("interp", CP_BOOL, NULL, 0)) {
            interpolated = TRUE;
            fprintf(cp_out, "Warning: Interpolated raw file data!\n\n");
        }

        *runp = run = TMALLOC(struct runDesc, 1);

        /* First fill in some general information. */
        run->analysis = analysisPtr;
        run->circuit = circuitPtr;
        run->name = copy(cktName);
        run->type = copy(analName);
        run->windowed = windowed;
        run->numData = 0;

        an_name = spice_analysis_get_name(analysisPtr->JOBtype);
        ft_curckt->ci_last_an = an_name;

        /* Now let's see which of these things we need.  First toss in the
         * reference vector.  Then toss in anything that getSaves() tells
         * us to save that we can find in the name list.  Finally unpack
         * the remaining saves into parameters.
         */
        numsaves = ft_getSaves(&saves);
        if (numsaves) {
            savesused = TMALLOC(bool, numsaves);
            saveall = FALSE;
            for (i = 0; i < numsaves; i++) {
                if (saves[i].analysis && !cieq(saves[i].analysis, an_name)) {
                    /* ignore this one this time around */
                    savesused[i] = TRUE;
                    continue;
                }

                /*  Check for ".save all" and new synonym ".save allv"  */

                if (cieq(saves[i].name, "all") || cieq(saves[i].name, "allv")) {
                    saveall = TRUE;
                    savesused[i] = TRUE;
                    saves[i].used = 1;
                    continue;
                }

                /*  And now for the new ".save alli" option  */

                if (cieq(saves[i].name, "alli")) {
                    savealli = TRUE;
                    savesused[i] = TRUE;
                    saves[i].used = 1;
                    continue;
                }

                if (cieq(saves[i].name, "nosub")) {
                    savenosub = TRUE;
                    savesused[i] = TRUE;
                    saves[i].used = 1;
                    continue;
                }

                if (cieq(saves[i].name, "nointernals")) {
                    savenointernals = TRUE;
                    savesused[i] = TRUE;
                    saves[i].used = 1;
                    continue;
                }
#ifdef SHARED_MODULE
                /* this may happen if shared ngspice*/
                if (cieq(saves[i].name, "none")) {
                    savenone = TRUE;
                    saveall = TRUE;
                    savesused[i] = TRUE;
                    saves[i].used = 1;
                    continue;
                }
#endif
            }
        }

        if (numsaves && !saveall && !savenosub)
            initmem = numsaves;
        else
            initmem = numNames;

        /* Pass 0. */
        if (refName) {
            addDataDesc(run, refName, refType, -1, initmem);
            for (i = 0; i < numsaves; i++)
                if (!savesused[i] && name_eq_query(saves[i].name, refName)) {
                    savesused[i] = TRUE;
                    saves[i].used = 1;
                }
        } else {
            run->refIndex = -1;
        }


        /* Pass 1. */
        if (numsaves && !saveall && !savenosub && !savenointernals) {
            for (i = 0; i < numsaves; i++) {
                if (!savesused[i]) {
                    for (j = 0; j < numNames; j++) {
                        if (name_eq_query(saves[i].name, dataNames[j])) {
                            addDataDesc(run, dataNames[j], dataType, j, initmem);
                            savesused[i] = TRUE;
                            saves[i].used = 1;
                            break;
                        }
                        /* generate a vector of real time information */
                        else if (ft_ngdebug && refName && eq(refName, "time") && eqc(saves[i].name, "speedcheck")) {
                            addDataDesc(run, "speedcheck", IF_REAL, j, initmem);
                            savesused[i] = TRUE;
                            saves[i].used = 1;
                            break;
                        }
                        else if (ft_ngdebug && refName && eq(refName, "time") && eqc(saves[i].name, "deltacheck")) {
                            addDataDesc(run, "deltacheck", IF_REAL, j, initmem);
                            savesused[i] = TRUE;
                            saves[i].used = 1;
                            break;
                        }
                    }
                }
            }
        } else {
            for (i = 0; i < numNames; i++)
                if (!refName || !name_eq(dataNames[i], refName))
                    /*  Save the node (with restrictions) */
                        /* don't save subckt nodes */
                    if (!(savenosub && strchr(dataNames[i], '.')) &&
                        /* no internals at all, but still #branch */
                        (!(savenointernals && strstr(dataNames[i], "#")) || strstr(dataNames[i], "#branch")) &&
                        /* created by .probe */
                        !strstr(dataNames[i], "probe_int_") &&
                        /* don't save internal device nodes */
                        !strstr(dataNames[i], "#internal") &&
                        !strstr(dataNames[i], "#source") &&
                        !strstr(dataNames[i], "#drain") &&
                        !strstr(dataNames[i], "#collector") &&
                        !strstr(dataNames[i], "#collCX") &&
                        !strstr(dataNames[i], "#emitter") &&
                        !strstr(dataNames[i], "#base"))
                    {
                        addDataDesc(run, dataNames[i], dataType, i, initmem);
                    }
            /* generate a vector of real time information */
            if (ft_ngdebug && refName && eq(refName, "time")) {
                 addDataDesc(run, "speedcheck", IF_REAL, numNames, initmem);
                 addDataDesc(run, "deltacheck", IF_REAL, numNames, initmem);
            }
        }

        /* Pass 1 and a bit.
           This is a new pass which searches for all the internal device
           nodes, and saves the terminal currents instead  */

        if (savealli) {
            depind = 0;
            for (i = 0; i < numNames; i++) {
                if (strstr(dataNames[i], "#internal") ||
                    strstr(dataNames[i], "#source") ||
                    strstr(dataNames[i], "#drain") ||
                    strstr(dataNames[i], "#collector") ||
                    strstr(dataNames[i], "#collCX") ||
                    strstr(dataNames[i], "#emitter") ||
                    strstr(dataNames[i], "#base"))
                {
                    tmpname[0] = '@';
                    tmpname[1] = '\0';
                    strncat(tmpname, dataNames[i], BSIZE_SP-1);
                    ch = strchr(tmpname, '#');

                    if (strstr(ch, "#collector")) {
                        strcpy(ch, "[ic]");
                    } else if (strstr(ch, "#collCX")) {
                        strcpy(ch, "[ic]");
                    } else if (strstr(ch, "#base")) {
                        strcpy(ch, "[ib]");
                    } else if (strstr(ch, "#emitter")) {
                        strcpy(ch, "[ie]");
                        if (parseSpecial(tmpname, namebuf, parambuf, depbuf))
                            addSpecialDesc(run, tmpname, namebuf, parambuf, depind, initmem);
                        strcpy(ch, "[is]");
                    } else if (strstr(ch, "#drain")) {
                        strcpy(ch, "[id]");
                        if (parseSpecial(tmpname, namebuf, parambuf, depbuf))
                            addSpecialDesc(run, tmpname, namebuf, parambuf, depind, initmem);
                        strcpy(ch, "[ig]");
                    } else if (strstr(ch, "#source")) {
                        strcpy(ch, "[is]");
                        if (parseSpecial(tmpname, namebuf, parambuf, depbuf))
                            addSpecialDesc(run, tmpname, namebuf, parambuf, depind, initmem);
                        strcpy(ch, "[ib]");
                    } else if (strstr(ch, "#internal") && (tolower_c(tmpname[1]) == 'd')) {
                        strcpy(ch, "[id]");
                    } else {
                        fprintf(cp_err,
                                "Debug: could output current for %s\n", tmpname);
                        continue;
                    }
                    if (parseSpecial(tmpname, namebuf, parambuf, depbuf)) {
                        if (*depbuf) {
                            fprintf(stderr,
                                    "Warning : unexpected dependent variable on %s\n", tmpname);
                        } else {
                            addSpecialDesc(run, tmpname, namebuf, parambuf, depind, initmem);
                        }
                    }
                }
            }
        }


        /* Pass 2. */
        for (i = 0; i < numsaves; i++) {

            if (savesused[i])
                continue;

            if (!parseSpecial(saves[i].name, namebuf, parambuf, depbuf)) {
                /* The near miss, and nothing wider than it: a token that
                   simply does not resolve is left to the silence it has
                   always had.  doc/codex/issues/0057 Resolution 1. */
                if (report_save_case_miss(run, saves[i].name, numNames,
                                          dataNames))
                    continue;
                if (saves[i].analysis)
                    fprintf(cp_err, "Warning: can't parse '%s': ignored\n",
                            saves[i].name);
                continue;
            }

            /* Now, if there's a dep variable, do we already have it? */
            if (*depbuf) {
                for (j = 0; j < run->numData; j++)
                    if (name_eq_query(depbuf, run->data[j].name))
                        break;
                if (j == run->numData) {
                    /* Better add it. */
                    for (j = 0; j < numNames; j++)
                        if (name_eq_query(depbuf, dataNames[j]))
                            break;
                    if (j == numNames) {
                        fprintf(cp_err,
                                "Warning: can't find '%s': value '%s' ignored\n",
                                depbuf, saves[i].name);
                        continue;
                    }
                    addDataDesc(run, dataNames[j], dataType, j, initmem);
                    savesused[i] = TRUE;
                    saves[i].used = 1;
                    depind = j;
                } else {
                    depind = run->data[j].outIndex;
                }
            }

            addSpecialDesc(run, saves[i].name, namebuf, parambuf, depind, initmem);
        }

        if (numsaves) {
            for (i = 0; i < numsaves; i++) {
                tfree(saves[i].analysis);
                tfree(saves[i].name);
            }
            tfree(saves);
            tfree(savesused);
        }

        if (numNames &&
            ((run->numData == 1 && run->refIndex != -1) ||
             (run->numData == 0 && run->refIndex == -1)))
        {
            fprintf(cp_err, "Error: no data saved for %s; analysis not run\n",
                    spice_analysis_get_description(analysisPtr->JOBtype));
            return E_NOTFOUND;
        }

        /* Now that we have our own data structures built up, let's see what
         * nutmeg wants us to do.
         */
        run->writeOut = ft_getOutReq(&run->fp, &run->runPlot, &run->binary,
                                     run->type, run->name);

        if (run->writeOut) {
            fileInit(run);
        } else {
            plotInit(run);
            if (refName)
                run->runPlot->pl_ndims = 1;
#ifdef XSPICE
            /* set the current plot name into the event job */
            if (run->runPlot->pl_typename)
                EVTsetup_plot(run->circuit, run->runPlot->pl_typename);
#endif
        }
    }

    /* define storage for old and new data, to allow interpolation */
    if (interpolated && run->circuit->CKTcurJob->JOBtype == 4) {
        valueold = TMALLOC(double, run->numData);
        for (i = 0; i < run->numData; i++)
            valueold[i] = 0.0;
        valuenew = TMALLOC(double, run->numData);
    }

    /*Start BLT, initilises the blt vectors saj*/
#ifdef TCL_MODULE
    blt_init(run);
#elif defined SHARED_MODULE
    sh_vecinit(run);
#endif

    startclock = clock();
    return (OK);
}

/* Initialze memory for the list of all vectors in the current plot.
   Add a standard vector to this plot */
static int
addDataDesc(runDesc *run, char *name, int type, int ind, int meminit)
{
    dataDesc *data;

    /* initialize memory (for all vectors or given by 'save') */
    if (!run->numData) {
        /* even if input 0, do a malloc */
        run->data = TMALLOC(dataDesc, ++meminit);
        run->maxData = meminit;
    }
    /* If there is need for more memory */
    else if (run->numData == run->maxData) {
        run->maxData = (int)(run->maxData * 1.1) + 1;
        run->data = TREALLOC(dataDesc, run->data, run->maxData);
    }

    data = &run->data[run->numData];
    /* so freeRun will get nice NULL pointers for the fields we don't set */
    memset(data, 0, sizeof(dataDesc));

    data->name = copy(name);
    data->type = type;
    data->gtype = GRID_LIN;
    data->regular = TRUE;
    data->outIndex = ind;

    /* It's the reference vector. */
    if (ind == -1)
        run->refIndex = run->numData;

    run->numData++;

    return (OK);
}

/* Initialze memory for the list of all vectors in the current plot.
   Add a special vector (e.g. @q1[ib]) to this plot */
static int
addSpecialDesc(runDesc *run, char *name, char *devname, char *param, int depind, int meminit)
{
    dataDesc *data;
    char *unique, *freeunique;       /* unique char * from back-end */
    int ret;

    if (!run->numData) {
        /* even if input 0, do a malloc */
        run->data = TMALLOC(dataDesc, ++meminit);
        run->maxData = meminit;
    }
    else if (run->numData == run->maxData) {
        run->maxData = (int)(run->maxData * 1.1) + 1;
        run->data = TREALLOC(dataDesc, run->data, run->maxData);
    }

    data = &run->data[run->numData];
    /* so freeRun will get nice NULL pointers for the fields we don't set */
    memset(data, 0, sizeof(dataDesc));

    data->name = copy(name);

    freeunique = unique = copy(devname);

    /* unique will be overridden, if it already exists */
    ret = INPinsertNofree(&unique, ft_curckt->ci_symtab);
    data->specName = unique;

    if (ret == E_EXISTS)
        tfree(freeunique);

    data->specParamName = copy(param);

    data->specIndex = depind;
    data->specType = -1;
    data->specFast = NULL;
    data->regular = FALSE;

    run->numData++;

    return (OK);
}


static void
OUTpD_memory(runDesc *run, IFvalue *refValue, IFvalue *valuePtr)
{
    int i, n = run->numData;

    for (i = 0; i < n; i++) {

        dataDesc *d;

#ifdef TCL_MODULE
        /*Locks the blt vector to stop access*/
        blt_lockvec(i);
#endif

        d = &run->data[i];

        if (d->outIndex == -1) {
            if (d->type == IF_REAL)
                plotAddRealValue(d, refValue->rValue);
            else if (d->type == IF_COMPLEX)
                plotAddComplexValue(d, refValue->cValue);
        } else if (d->regular) {
            if (ft_ngdebug && d->type == IF_REAL && eq(d->name, "speedcheck")) {
                /* current time */
                clock_t cl = clock();
                double tt = ((double)cl - (double)startclock) / CLOCKS_PER_SEC;
                plotAddRealValue(d, tt);
            }
            else if (ft_ngdebug && d->type == IF_REAL && eq(d->name, "deltacheck")) {
                plotAddRealValue(d, ft_curckt->ci_ckt->CKTdeltaOld[0]);
            }
            else if (d->type == IF_REAL)
                plotAddRealValue(d, valuePtr->v.vec.rVec[d->outIndex]);
            else if (d->type == IF_COMPLEX)
                plotAddComplexValue(d, valuePtr->v.vec.cVec[d->outIndex]);
        } else {
            IFvalue val;

            /* should pre-check instance */
            if (!getSpecial(d, run, &val))
                continue;

            if (d->type == IF_REAL)
                plotAddRealValue(d, val.rValue);
            else if (d->type == IF_COMPLEX)
                plotAddComplexValue(d, val.cValue);
            else
                fprintf(stderr, "OUTpData: unsupported data type\n");
        }

#ifdef TCL_MODULE
        /*relinks and unlocks vector*/
        blt_relink(i, d->vec);
#endif

    }
}


int
OUTpData(runDesc *plotPtr, IFvalue *refValue, IFvalue *valuePtr)
{
    runDesc *run = plotPtr;  // FIXME
    int i;

    run->pointCount++;

#ifdef TCL_MODULE
    steps_completed = run->pointCount;
#endif
    /* interpolated batch mode output to file/plot in transient analysis */
    if (interpolated && run->circuit->CKTcurJob->JOBtype == 4) {
        /* JOBtype == 4 means Transient Analysis.  FIX ME */

        if (run->writeOut) { /* To file */
            InterpFileAdd(run, refValue, valuePtr);
        }
        else { /* To plot */
            InterpPlotAdd(run, refValue, valuePtr);
        }
        return OK;
    } else if (run->writeOut) {
        /* standard batch mode output to file */

        if (run->pointCount == 1) {
            fileInit_pass2(run);
        }

        fileStartPoint(run->fp, run->binary, run->pointCount);

        if (run->refIndex != -1) {
            if (run->isComplex) {
                fileAddComplexValue(run->fp, run->binary, refValue->cValue);

                /*  While we're looking at the reference value, print it to the screen
                    every quarter of a second, to give some feedback without using
                    too much CPU time  */
#ifndef HAS_WINGUI
                if (!orflag && !ft_norefprint && !cp_background) {
                    currclock = clock();
                    if ((currclock-lastclock) > (0.25*CLOCKS_PER_SEC)) {
                        fprintf(stdout, " Reference value : % 12.5e\r",
                                refValue->cValue.real);
                        fflush(stdout);
                        lastclock = currclock;
                    }
                }
#endif
            }
            else { /* And the same for a non-complex (real) value  */
                fileAddRealValue(run->fp, run->binary, refValue->rValue);
#ifndef HAS_WINGUI
                if (!orflag && !ft_norefprint && !cp_background) {
                    currclock = clock();
                    if ((currclock-lastclock) > (0.25*CLOCKS_PER_SEC)) {
                        fprintf(stdout, " Reference value : % 12.5e\r",
                                refValue->rValue);
                        fflush(stdout);
                        lastclock = currclock;
                    }
                }
#endif
            }
        }

        for (i = 0; i < run->numData; i++) {
            /* we've already printed reference vec first */
            if (run->data[i].outIndex == -1) {
                continue;
            }

#ifdef TCL_MODULE
            blt_add(i, refValue ? refValue->rValue : NAN);
#endif

            if (run->data[i].regular) {
                if (ft_ngdebug && run->data[i].type == IF_REAL && eq(run->data[i].name, "speedcheck")) {
                    /* current time */
                    clock_t cl = clock();
                    double tt = ((double)cl - (double)startclock) / CLOCKS_PER_SEC;
                    fileAddRealValue(run->fp, run->binary, tt);
                }
                else if (ft_ngdebug && run->data[i].type == IF_REAL && eq(run->data[i].name, "deltacheck")) {
                    fileAddRealValue(run->fp, run->binary, ft_curckt->ci_ckt->CKTdeltaOld[0]);
                }
                else if (run->data[i].type == IF_REAL)
                    fileAddRealValue(run->fp, run->binary,
                            valuePtr->v.vec.rVec [run->data[i].outIndex]);
                else if (run->data[i].type == IF_COMPLEX)
                    fileAddComplexValue(run->fp, run->binary,
                            valuePtr->v.vec.cVec [run->data[i].outIndex]);
                else
                    fprintf(stderr, "OUTpData: unsupported data type\n");
            }
            else {
                IFvalue val;
                /* should pre-check instance */
                if (!getSpecial(&run->data[i], run, &val)) {

                    /*  If this is the first data point, print a warning for any unrecognized
                        variables, since this has not already been checked  */

                    if (run->pointCount == 1)
                        fprintf(stderr, "Warning: unrecognized variable - %s\n",
                                run->data[i].name);

                    if (run->isComplex) {
                        val.cValue.real = 0;
                        val.cValue.imag = 0;
                        fileAddComplexValue(run->fp, run->binary, val.cValue);
                    }
                    else {
                        val.rValue = 0;
                        fileAddRealValue(run->fp, run->binary, val.rValue);
                    }

                    continue;
                }

                if (run->data[i].type == IF_REAL)
                    fileAddRealValue(run->fp, run->binary, val.rValue);
                else if (run->data[i].type == IF_COMPLEX)
                    fileAddComplexValue(run->fp, run->binary, val.cValue);
                else
                    fprintf(stderr, "OUTpData: unsupported data type\n");
            }

#ifdef TCL_MODULE
            blt_add(i, valuePtr->v.vec.rVec [run->data[i].outIndex]);
#endif
        }

        fileEndPoint(run->fp, run->binary);

        /*  Check that the write to disk completed successfully, otherwise abort  */

        if (ferror(run->fp)) {
            fprintf(stderr, "Warning: rawfile write error !!\n");
            shouldstop = TRUE;
        }

    }
    else {
        OUTpD_memory(run, refValue, valuePtr);

        /*  This is interactive mode. Update the screen with the reference
            variable just the same  */

#ifndef HAS_WINGUI
        if (!orflag && !ft_norefprint && !cp_background) {
            currclock = clock();
            if ((currclock-lastclock) > (0.25*CLOCKS_PER_SEC)) {
                if (run->isComplex) {
                    fprintf(stdout, " Reference value : % 12.5e\r",
                            refValue ? refValue->cValue.real : NAN);
                } else {
                    fprintf(stdout, " Reference value : % 12.5e\r",
                            refValue ? refValue->rValue : NAN);
                }
                fflush(stdout);
                lastclock = currclock;
            }
        }
#endif

        gr_iplot(run->runPlot);
    }

    if (ft_bpcheck(run->runPlot, run->pointCount) == FALSE)
        shouldstop = TRUE;

#ifdef TCL_MODULE
    Tcl_ExecutePerLoop();
#elif defined SHARED_MODULE
    sh_ExecutePerLoop();
#endif

    return OK;
} /* end of function OUTpData */


int
OUTendPlot(runDesc *plotPtr)
{
    if (plotPtr->writeOut) {
        fileEnd(plotPtr);
    } else {
        gr_end_iplot();
        plotEnd(plotPtr);
    }

    tfree(valueold);
    tfree(valuenew);

    freeRun(plotPtr);

    return (OK);
}


int
OUTattributes(runDesc *plotPtr, IFuid varName, int param, IFvalue *value)
{
    runDesc *run = plotPtr;  // FIXME
    GRIDTYPE type;

    struct dvec *d;

    NG_IGNORE(value);

    if (param == OUT_SCALE_LIN)
        type = GRID_LIN;
    else if (param == OUT_SCALE_LOG)
        type = GRID_XLOG;
    else
        return E_UNSUPP;

    /* Both comparisons below ask name_eq()'s question rather than the save
       list's -- an analysis's own IFuid for a column against the name this
       run stored for it -- and stay byte exact for name_eq()'s reason: the
       search includes the scale, and a deck may call a node Time.  Every
       caller in the tree passes varName as NULL, so neither arm is reachable
       today; the interface is public, which is why they are classified rather
       than left frozen.  doc/codex/issues/0056 criterion 7. */

    if (run->writeOut) {
        if (varName) {
            int i;
            for (i = 0; i < run->numData; i++)
                /* case-lint: stored - both operands are names this run holds, see above */
                if (!strcmp(varName, run->data[i].name))
                    run->data[i].gtype = type;
        } else {
            run->data[run->refIndex].gtype = type;
        }
    } else {
        if (varName) {
            for (d = run->runPlot->pl_dvecs; d; d = d->v_next)
                /* case-lint: stored - both operands are names this run holds, see above */
                if (!strcmp(varName, d->v_name))
                    d->v_gridtype = type;
        } else if (param == PLOT_COMB) {
            for (d = run->runPlot->pl_dvecs; d; d = d->v_next)
                d->v_plottype = PLOT_COMB;
        } else {
            run->runPlot->pl_scale->v_gridtype = type;
        }
    }

    return (OK);
}


/* The file writing routines.
   Write a raw file in batch mode (-b and -r flags).
   Writing a raw file in interactive or control  mode is handled
   by raw_write() in rawfile.c */
static void
fileInit(runDesc *run)
{
    char buf[513];
    int i;
    size_t n;

    lastclock = clock();

    /* This is a hack. */
    run->isComplex = FALSE;
    for (i = 0; i < run->numData; i++)
        if (run->data[i].type == IF_COMPLEX)
            run->isComplex = TRUE;

    n = 0;
    sprintf(buf, "Title: %s\n", run->name);
    n += strlen(buf);
    fputs(buf, run->fp);
    sprintf(buf, "Date: %s\n", datestring());
    n += strlen(buf);
    fputs(buf, run->fp);
    sprintf(buf, "Command: %s-%s, Build %s\n", ft_sim->simulator, ft_sim->version, Spice_Build_Date);
    n += strlen(buf);
    fputs(buf, run->fp);
    sprintf(buf, "Plotname: %s\n", run->type);
    n += strlen(buf);
    fputs(buf, run->fp);
    /* The identifier case mode this run is being produced under, if the
       session asked for it to be recorded.  raw_write()
       (src/frontend/rawfile.c) writes the same line for the plots it
       serialises, and the two writers share this format and no code, so
       without this arm a file written through -r or 'run <file>' looked
       exactly like a file from a release that has no such feature, and its
       consumer had to spawn a probe simulation to learn what the writing
       session already knew: doc/codex/issues/0071.

       Nothing about the line is decided here.  The key is 'Option:', the one
       key every existing reader already parses -- a new key aborts the load as
       a strange line.  The place is the line after 'Plotname:', because the
       reader's Option: arm needs a current plot to file the pair into and
       calls the line misplaced when there is none.  The value is the mode in
       force, inp_case_mode_name() (src/frontend/inpcom.c), and not the
       'casemode' variable, which holds what was requested and stops
       describing this run the moment a .control block moves it
       (doc/codex/issues/0060).  raw_write()'s remaining condition, that the
       plot did not come out of a file (doc/codex/issues/0070), has no
       counterpart here: this writer only ever writes data the running
       analysis is producing.

       The gate is read through cp_getvar_policy() (src/frontend/variable.c)
       rather than cp_getvar(), whose chain ends in the current plot's
       environment: a raw header carrying 'Option: casemodewrite' is parsed
       straight into that, so a plain read would let a file the user opened
       switch this writer on.  It is off by default because a file carrying
       the line can crash a released ngspice-46 that unsets the key
       (doc/codex/issues/0067, fixed here and in nothing released); unset, the
       header this function writes stays the one it has written since spice3.

       It is written in this function's own idiom -- into buf, counted into n,
       fputs -- and not as a bare fprintf, because n is the seek position
       fileEnd() backfills the point count into when ftell() is unusable,
       which is the run->fp == stdout arm a few lines below ('ngspice -s').
       tests/regression/pipe/rawfile-casemode-batch.cmd. */
    if (cp_getvar_policy("casemodewrite", CP_BOOL, NULL, 0)) {
        sprintf(buf, "Option: casemode=%s\n", inp_case_mode_name());
        n += strlen(buf);
        fputs(buf, run->fp);
    }
    sprintf(buf, "Flags: %s\n", run->isComplex ? "complex" : "real");
    n += strlen(buf);
    fputs(buf, run->fp);
    sprintf(buf, "No. Variables: %d\n", run->numData);
    n += strlen(buf);
    fputs(buf, run->fp);
    sprintf(buf, "No. Points: ");
    n += strlen(buf);
    fputs(buf, run->fp);

    fflush(run->fp);        /* Gotta do this for LATTICE. */
    if (run->fp == stdout || (run->pointPos = ftell(run->fp)) <= 0)
        run->pointPos = (long) n;
    fprintf(run->fp, "0       \n"); /* Save 8 spaces here. */

    fprintf(run->fp, "Variables:\n");

    printf("No. of Data Columns : %d  \n", run->numData);
}

/* Trying to guess the type of a vector, using either their special names
   or special parameter names for @ vectors. FIXME This guessing may fail
   due to the many options, especially for the @ vectors. pltypename
   may be run->type in batch mode or the plot name in control mode. */
static int
guess_type(const char *name, char* pltypename)
{
    int type;

    if (substring("#branch", name))
        type = SV_CURRENT;
    else if (cieq(name, "time"))
        type = SV_TIME;
    else if ( cieq(name, "speedcheck"))
        type = SV_TIME;
    else if ( cieq(name, "deltacheck"))
        type = SV_TIME;
    else if (cieq(name, "frequency"))
        type = SV_FREQUENCY;
    else if (ciprefix("inoise", name))
        type = fixme_inoise_type;
    else if (ciprefix("onoise", name))
        type = fixme_onoise_type;
    else if (cieq(name, "temp-sweep"))
        type = SV_TEMP;
    else if (cieq(name, "res-sweep"))
        type = SV_RES;
    else if (cieq(name, "i-sweep"))
        type = SV_CURRENT;
    else if (strstr(name, ":power\0"))
        type = SV_POWER;
    /* Special treatment if plot has been generated by S-parameter simulation */
    else if (pltypename && ciprefix("sp", pltypename) && ciprefix("S_", name))
        type = SV_SPARAM;
    else if (pltypename && ciprefix("sp", pltypename) && ciprefix("Y_", name))
        type = SV_ADMITTANCE;
    else if (pltypename && ciprefix("sp", pltypename) && ciprefix("Z_", name))
        type = SV_IMPEDANCE;
    else if (pltypename && ciprefix("sp", pltypename) && cieq(name, "NF"))
        type = SV_DB;
    else if (pltypename && ciprefix("sp", pltypename) && cieq(name, "NFmin"))
        type = SV_DB;
    else if (pltypename && ciprefix("sp", pltypename) && cieq(name, "Rn"))
        type = SV_IMPEDANCE;
    else if (pltypename && ciprefix("sp", pltypename) && cieq(name, "SOpt"))
        type = SV_NOTYPE;
    else if (pltypename && ciprefix("sp", pltypename) && ciprefix("Cy_", name))
        type = SV_CURRENT;
    /* current source ISRC parameters for current */
    else if (substring("@i", name) && (substring("[c]", name) || substring("[dc]", name) || substring("[current]", name)))
            type = SV_CURRENT;
    else if ((*name == '@') && substring("[g", name)) /* token starting with [g */
        type = SV_ADMITTANCE;
    else if ((*name == '@') && substring("[c", name))
        type = SV_CAPACITANCE;
    else if ((*name == '@') && substring("[i", name))
        type = SV_CURRENT;
    else if ((*name == '@') && substring("[q", name))
        type = SV_CHARGE;
    else if ((*name == '@') && substring("[p]", name)) /* token is exactly [p] */
        type = SV_POWER;
    else
        type = SV_VOLTAGE;

    return type;
}


static void
fileInit_pass2(runDesc *run)
{
    int i, type;

    bool keepbranch = cp_getvar("keep#branch", CP_BOOL, NULL, 0);

    for (i = 0; i < run->numData; i++) {

        char *name = run->data[i].name;

        /* Use run->type to detect SP analysis */
        type = guess_type(name, run->type);

        if (type == SV_CURRENT && !keepbranch) {
            char *branch = strstr(name, "#branch");
            if (branch)
                *branch = '\0';
            fprintf(run->fp, "\t%d\ti(%s)\t%s", i, name, ft_typenames(type));
            if (branch)
                *branch = '#';
        } else if (type == SV_VOLTAGE) {
            fprintf(run->fp, "\t%d\tv(%s)\t%s", i, name, ft_typenames(type));
        } else {
            fprintf(run->fp, "\t%d\t%s\t%s", i, name, ft_typenames(type));
        }

        if (run->data[i].gtype == GRID_XLOG)
            fprintf(run->fp, "\tgrid=3");

        fprintf(run->fp, "\n");
    }

    fprintf(run->fp, "%s:\n", run->binary ? "Binary" : "Values");
    fflush(run->fp);

    /*  Allocate Row buffer  */

    if (run->binary) {
        rowbuflen = (size_t) (run->numData);
        if (run->isComplex)
            rowbuflen *= 2;
        rowbuf = TMALLOC(double, rowbuflen);
    } else {
        rowbuflen = 0;
        rowbuf = NULL;
    }
}


static void
fileStartPoint(FILE *fp, bool bin, int num)
{
    if (!bin)
        fprintf(fp, "%d\t", num - 1);

    /*  reset buffer pointer to zero  */

    column = 0;
}


static void
fileAddRealValue(FILE *fp, bool bin, double value)
{
    if (bin)
        rowbuf[column++] = value;
    else
        fprintf(fp, "\t%.*e\n", DOUBLE_PRECISION, value);
}


static void
fileAddComplexValue(FILE *fp, bool bin, IFcomplex value)
{
    if (bin) {
        rowbuf[column++] = value.real;
        rowbuf[column++] = value.imag;
    } else {
        fprintf(fp, "\t%.*e,%.*e\n", DOUBLE_PRECISION, value.real,
                DOUBLE_PRECISION, value.imag);
    }
}


static void
fileEndPoint(FILE *fp, bool bin)
{
    /*  write row buffer to file  */
    /* otherwise the data has already been written */

    if (bin)
        fwrite(rowbuf, sizeof(double), rowbuflen, fp);
}


/* Here's the hack...  Run back and fill in the number of points. */

static void
fileEnd(runDesc *run)
{
    if (run->fp != stdout) {
        long place = ftell(run->fp);
        fseek(run->fp, run->pointPos, SEEK_SET);
        fprintf(run->fp, "%d", run->pointCount);
        fprintf(stdout, "\nNo. of Data Rows : %d\n", run->pointCount);
        fseek(run->fp, place, SEEK_SET);
    } else {
        /* Yet another hack-around */
        fprintf(stderr, "@@@ %ld %d\n", run->pointPos, run->pointCount);
    }

    fflush(run->fp);

    tfree(rowbuf);
}


/* The plot maintenance routines. */

static void
plotInit(runDesc *run)
{
    struct plot *pl = plot_alloc(run->type);
    struct dvec *v;
    int i;

    pl->pl_title = copy(run->name);
    pl->pl_name = copy(run->type);
    pl->pl_date = copy(datestring());
    pl->pl_ndims = 0;
    plot_new(pl);
    plot_setcur(pl->pl_typename);
    run->runPlot = pl;

    /* This is a hack. */
    /* if any of them complex, make them all complex */
    run->isComplex = FALSE;
    for (i = 0; i < run->numData; i++)
        if (run->data[i].type == IF_COMPLEX)
            run->isComplex = TRUE;

    for (i = 0; i < run->numData; i++) {
        dataDesc *dd = &run->data[i];
        char *name;

        if (isdigit_c(dd->name[0]))
            name = tprintf("V(%s)", dd->name);
        else
            name = copy(dd->name);

        /* Use pl->pl_typename to detect SP analysis */
        v = dvec_alloc(name,
                       guess_type(name, pl->pl_typename),
                       run->isComplex
                       ? (VF_COMPLEX | VF_PERMANENT)
                       : (VF_REAL | VF_PERMANENT),
                       0, NULL);

        vec_new(v);
        dd->vec = v;
    }
}

/* prepare the vector length data for memory allocation
   If new, and tran or pss, length is TSTOP / TSTEP plus some margin.
   If allocated length is exceeded, check progress. When > 20% then extrapolate memory needed,
   if less than 20% then just double the size.
   If not tran or pss, return fixed value (1024) of memory to be added.
   */
static inline int
vlength2delta(int len)
{
#ifdef SHARED_MODULE
    if (savenone)
        /* We need just a vector length of 1 */
        return 1;
#endif
    /* TSTOP / TSTEP */
    int points = ft_curckt->ci_ckt->CKTtimeListSize;
    /* transient and pss analysis (points > 0) upon start */
    if ((ft_curckt->ci_ckt->CKTmode & MODETRAN) && len == 0 && points > 0) {
        /* number of timesteps plus some overhead */
        return points + 100;
    }
    /* transient and pss if original estimate is exceeded */
    else if ((ft_curckt->ci_ckt->CKTmode & MODETRAN) && points > 0) {
        /* check where we are */
        double timerel = ft_curckt->ci_ckt->CKTtime / ft_curckt->ci_ckt->CKTfinalTime;
        /* return an estimate of the appropriate number of time points, if more than 20% of
           the anticipated total time has passed */
        if (timerel > 0.2) {
            int proposed = (int)(len / timerel) - len + 1;

            if (proposed > 0)
                return proposed;
            return 16; // Probably enough as past end of simulation.
        } else {
            /* If not, just double the available memory */

            return len;
        }
    }
    /* op */
    else if (ft_curckt->ci_ckt->CKTmode & MODEDCOP) {
        /* op with length 1 */
        return 1;
    }
    /* other analysis types that do not set CKTtimeListSize */
    else
        return 1024;
}

void
AddRealValueToVector(struct dvec *v, double value)
{
#ifdef SHARED_MODULE
    if (savenone)
        /* always save new data to same location */
        v->v_length = 0;
#endif

    if (v->v_length >= v->v_alloc_length)
        dvec_extend(v, v->v_length + vlength2delta(v->v_length));

    if (isreal(v)) {
        v->v_realdata[v->v_length] = value;
    } else {
        /* a real parading as a VF_COMPLEX */
        v->v_compdata[v->v_length].cx_real = value;
        v->v_compdata[v->v_length].cx_imag = 0.0;
    }

    v->v_length++;
    v->v_dims[0] = v->v_length; /* va, must be updated */
}

static void
plotAddRealValue(dataDesc *desc, double value)
{
    AddRealValueToVector(desc->vec, value);
}

static void
plotAddComplexValue(dataDesc *desc, IFcomplex value)
{
    struct dvec *v = desc->vec;

#ifdef SHARED_MODULE
    if (savenone)
        v->v_length = 0;
#endif

    if (v->v_length >= v->v_alloc_length)
        dvec_extend(v, v->v_length + vlength2delta(v->v_length));

    v->v_compdata[v->v_length].cx_real = value.real;
    v->v_compdata[v->v_length].cx_imag = value.imag;

    v->v_length++;
    v->v_dims[0] = v->v_length; /* va, must be updated */
}


static void
plotEnd(runDesc *run)
{
    fprintf(stdout, "\nNo. of Data Rows : %d\n", run->pointCount);
}


/* ParseSpecial takes something of the form "@name[param,index]" and rips
 * out name, param, andstrchr.
 */

static bool
parseSpecial(char *name, char *dev, char *param, char *ind)
{
    char *s;

    *dev = *param = *ind = '\0';

    if (*name != '@')
        return FALSE;
    name++;

    s = dev;
    while (*name && (*name != '['))
        *s++ = *name++;
    *s = '\0';

    if (!*name)
        return TRUE;
    name++;

    s = param;
    while (*name && (*name != ',') && (*name != ']'))
        *s++ = *name++;
    *s = '\0';

    if (*name == ']')
        return (!name[1] ? TRUE : FALSE);
    else if (!*name)
        return FALSE;
    name++;

    s = ind;
    while (*name && (*name != ']'))
        *s++ = *name++;
    *s = '\0';

    if (*name && !name[1])
        return TRUE;
    else
        return FALSE;
}


/* Reduce a name with a V() around it to the name inside, in buf.  Returns
   NULL on an opening parenthesis that is never closed, which both callers
   below report as "not the same name". */

static char *
name_unwrap(char *n, char *buf)
{
    char *s;

    if ((s = strchr(n, '(')) != NULL) {
        strcpy(buf, s);
        if ((s = strchr(buf, ')')) == NULL)
            return NULL;
        *s = '\0';
        return buf;
    }

    return n;
}


/* This routine must match two names with or without a V() around them.
   Both operands are names this run produced -- a variable name out of
   dataNames[] and the reference vector's name -- so the question is which
   column of the run this is, not whether two spellings are one name.  It
   stays byte exact in every mode, and folding it would delete a vector: a
   deck whose mid node is called Time keeps that node beside the constructed
   scale vector 'time' under preserve only because this compare is exact.
   doc/codex/issues/0056 criterion 2. */

static bool
name_eq(char *n1, char *n2)
{
    char buf1[BSIZE_SP], buf2[BSIZE_SP];

    if ((n1 = name_unwrap(n1, buf1)) == NULL)
        return FALSE;

    if ((n2 = name_unwrap(n2, buf2)) == NULL)
        return FALSE;

    /* case-lint: stored - both operands are names this run holds, see above */
    return (strcmp(n1, n2) ? FALSE : TRUE);
}


/* The same match for a name the caller asked for -- a .save card's token or a
   'save' command's -- against a name this run stores.  That is the frontend's
   Class C question and vec_name_eq() is its one answer: case insensitive
   under fold and preserve, exact only under distinguish.
   doc/claude/decisions/0001-distinguish.md decision 3, doc/codex/issues/0056
   criterion 1. */

static bool
name_eq_query(char *typed, char *stored)
{
    char buf1[BSIZE_SP], buf2[BSIZE_SP];

    if ((typed = name_unwrap(typed, buf1)) == NULL)
        return FALSE;

    if ((stored = name_unwrap(stored, buf2)) == NULL)
        return FALSE;

    return vec_name_eq(stored, typed);
}


/* Does the circuit itself know this name, exactly as the token spells it?
   Only the near miss below asks, and it asks because a circuit that has the
   token has no case mistake in it to report: a run whose columns are only
   part of the circuit -- noise, disto, sens and tf build their own column
   list holding none of the node voltages -- would otherwise offer the twin
   of a name the deck spelled correctly.  Branch currents are on the analog
   list too, under the '<name>#branch' that copynode()
   (src/frontend/breakp2.c) has already produced.

   A mixed-signal circuit has two node lists, and an XSPICE event node is on
   only the second: EVTinit() builds ckt->evt->info.node_table and puts
   nothing on ckt->CKTnodes.  The second is asked through Evt_Ckt_Has_Node()
   (src/xspice/evt/evtplot.c), which is Evt_Node_Name_Eq()'s case rule -- the
   event side of the same question vec_name_eq() answers here.
   doc/codex/issues/0057 criterion 3. */

static bool
ckt_knows_name(CKTcircuit *ckt, char *name)
{
    CKTnode *n;

    if (!ckt)
        return FALSE;

    for (n = ckt->CKTnodes; n; n = n->next)
        if (n->name && vec_name_eq(n->name, name))
            return TRUE;

#ifdef XSPICE
    if (Evt_Ckt_Has_Node(ckt, name))
        return TRUE;
#endif

    return FALSE;
}


/* A name this circuit has that differs from the token only in case, or NULL.
   The '!eq' is the whole of the test's meaning and not a micro-optimisation:
   a name equal ignoring case AND equal byte for byte is the token itself,
   which is not a near miss.  Reachable with ".save all" in force, where a
   named save entry is never marked used and its column is in the run all the
   same -- without the exclusion that deck is told 'onoise_spectrum' differs
   only in case from 'onoise_spectrum'.

   The event node table is deliberately not searched: an event node with a
   case variant has its own diagnostic in EVTnode_case_check()
   (src/xspice/evt/evttermi.c, doc/claude/decisions/0003), and reaching the
   event table here would need a second lookup shape -- this scan folds,
   Evt_Node_Name_Eq() does not.  So a save spelled DOUT against the event
   node dout is silent from this site; that is a stated gap, recorded in
   doc/codex/issues/0057 criterion 3. */

static bool
case_twin_p(const char *token, const char *stored)
{
    /* case-lint: neither - the deliberate case-insensitive twin scan of decision 2, not an identity test */
    if (!cieq(token, stored))
        return FALSE;

    /* case-lint: neither - excludes the token itself from its own twin scan */
    return eq(token, stored) ? FALSE : TRUE;
}


static char *
ckt_case_twin(CKTcircuit *ckt, char *name)
{
    CKTnode *n;

    if (!ckt)
        return NULL;

    for (n = ckt->CKTnodes; n; n = n->next)
        if (n->name && case_twin_p(name, n->name))
            return n->name;

    return NULL;
}


/* The tokens this simulation has already reported, emptied by dosim()
   (src/frontend/runcoms.c) when the next one starts.  beginPlot() runs once
   per plot and one analysis may open several -- noise opens two, disto up to
   six -- so without a memory the report would count plots instead of
   mistakes and a consumer counting lines would read one mistake as two or
   six.  That is doc/codex/issues/0046's hazard and doc/codex/issues/0058's,
   and refusing it is doc/codex/issues/0057 criterion 6, whose unit is one
   line per token per simulation. */

static wordlist *save_miss_reported = NULL;


void
OUTsaveMissClear(void)
{
    wl_free(save_miss_reported);
    save_miss_reported = NULL;
}


static bool
save_miss_first_time(char *name)
{
    wordlist *wl;

    for (wl = save_miss_reported; wl; wl = wl->wl_next)
        /* case-lint: neither - a token against a remembered copy of itself, a dedup and not an identity test */
        if (eq(wl->wl_word, name))
            return FALSE;

    save_miss_reported = wl_cons(copy(name), save_miss_reported);

    return TRUE;
}


/* A save name that missed AND has a case variant among the names this run
   does have.  Both halves are required, which is the whole of this report's
   contract: decision 2 of doc/claude/decisions/0001-distinguish.md, ":76-78",
   conditions the near-miss warning on exactly that pair, and the wider report
   0057 first shipped -- every unresolved save name, in every mode -- was
   withdrawn because a name that is merely absent is not a case defect and is
   not this diagnostic's business.  An unresolved .save has always been silent
   here and is silent again; what is new is only the case near miss, at a
   resolution site decision 2's table did not list.  Returns TRUE when it
   spoke, so that a .print-derived token gets this message instead of, and not
   beside, "can't parse".

   Structurally this fires only under distinguish, and the mode test says so
   rather than relying on the argument: under fold and preserve the resolution
   in Pass 1 folds (name_eq_query(), doc/codex/issues/0056), so a token with a
   case variant among the run's columns cannot reach here at all.  The two
   shipped modes are therefore byte identical to what they printed before,
   which is the claim the withdrawal is worth making.

   The wording is decision 2's, duplicated rather than reused because
   vec_warn_case_near_miss() (src/frontend/vectors.c) needs a plot's lookup
   table and there is no plot here yet -- plotInit() is below.  That is
   doc/codex/issues/0032's conclusion for rawfile.c's scale= reference, for
   the same reason. */

static bool
report_save_case_miss(runDesc *run, char *name, int numNames, char **dataNames)
{
    char buf[BSIZE_SP];
    char *stored;
    int j;

    if (inp_case_mode() != NG_CASE_DISTINGUISH)
        return FALSE;

    if (ckt_knows_name(run->circuit, name))
        return FALSE;

    stored = NULL;

    for (j = 0; j < numNames; j++) {
        char *column = name_unwrap(dataNames[j], buf);
        if (column && case_twin_p(name, column)) {
            stored = dataNames[j];
            break;
        }
    }

    /* dataNames[] is the column list of the analysis opening this plot, and
       for op, dc, ac and tran that is the circuit's node list entire; noise,
       disto, sens and tf build their own, holding none of the node voltages,
       so a deck whose only analysis is one of those needs the node list
       asked as well.  dataNames[] is searched first, so the twin named is
       the one the run would have produced when it has one. */
    if (!stored)
        stored = ckt_case_twin(run->circuit, name);

    if (!stored)
        return FALSE;

    if (!save_miss_first_time(name))
        return TRUE;

    fprintf(cp_err,
            "Warning: no vector named '%s'; '%s' differs only in case (casemode=distinguish)\n",
            name, stored);

    return TRUE;
}


static bool
getSpecial(dataDesc *desc, runDesc *run, IFvalue *val)
{
    IFvalue selector;
    struct variable *vv;

    selector.iValue = desc->specIndex;
    if (INPaName(desc->specParamName, val, run->circuit, &desc->specType,
                 desc->specName, &desc->specFast, ft_sim, &desc->type,
                 &selector) == OK) {
        desc->type &= (IF_REAL | IF_COMPLEX);   /* mask out other bits */
        return TRUE;
    }

    if ((vv = if_getstat(run->circuit, &desc->name[1])) != NULL) {
        /* skip @ sign */
        desc->type = IF_REAL;
        if (vv->va_type == CP_REAL)
            val->rValue = vv->va_real;
        else if (vv->va_type == CP_NUM)
            val->rValue = vv->va_num;
        else if (vv->va_type == CP_BOOL)
            val->rValue = (vv->va_bool ? 1.0 : 0.0);
        else
            return FALSE; /* not a real */
        tfree(vv);
        return TRUE;
    }

    return FALSE;
}


static void
freeRun(runDesc *run)
{
    int i;

    for (i = 0; i < run->numData; i++) {
        tfree(run->data[i].name);
        tfree(run->data[i].specParamName);
    }

    tfree(run->data);
    tfree(run->type);
    tfree(run->name);

    tfree(run);
}


int
OUTstopnow(void)
{
    if (ft_intrpt || shouldstop) {
        ft_intrpt = shouldstop = FALSE;
        return (1);
    }

    return (0);
}


/* Print out error messages. */

static struct mesg {
    char *string;
    long flag;
} msgs[] = {
    { "Warning", ERR_WARNING } ,
    { "Fatal error", ERR_FATAL } ,
    { "Panic", ERR_PANIC } ,
    { "Note", ERR_INFO } ,
    { NULL, 0 }
};


void
OUTerror(int flags, char *format, IFuid *names)
{
    struct mesg *m;
    char buf[BSIZE_SP], *s, *bptr;
    int nindex = 0;

    if ((flags == ERR_INFO) && cp_getvar("printinfo", CP_BOOL, NULL, 0))
        return;

    for (m = msgs; m->flag; m++)
        if (flags & m->flag)
            fprintf(cp_err, "%s: ", m->string);

    for (s = format, bptr = buf; *s; s++) {
        if (*s == '%' && (s == format || s[-1] != '%') && s[1] == 's') {
            if (names[nindex])
                strcpy(bptr, names[nindex]);
            else
                strcpy(bptr, "(null)");
            bptr += strlen(bptr);
            s++;
            nindex++;
        } else {
            *bptr++ = *s;
        }
    }

    *bptr = '\0';
    fprintf(cp_err, "%s\n", buf);
    fflush(cp_err);
}


void
OUTerrorf(int flags, const char *format, ...)
{
    struct mesg *m;
    va_list args;

    if ((flags == ERR_INFO) && cp_getvar("printinfo", CP_BOOL, NULL, 0))
        return;

    for (m = msgs; m->flag; m++)
        if (flags & m->flag)
            fprintf(cp_err, "%s: ", m->string);

    va_start (args, format);

    vfprintf(cp_err, format, args);
    fputc('\n', cp_err);

    fflush(cp_err);

    va_end(args);
}


static int
InterpFileAdd(runDesc *run, IFvalue *refValue, IFvalue *valuePtr)
{
    int i;
    static double timeold = 0.0, timenew = 0.0, timestep = 0.0;
    bool nodata = FALSE;
    bool interpolatenow = FALSE;

    if (run->pointCount == 1) {
        fileInit_pass2(run);
        timestep = run->circuit->CKTinitTime + run->circuit->CKTstep;
    }

    if (run->refIndex != -1) {
        /*  Save first time step  */
        if (refValue->rValue == run->circuit->CKTinitTime) {
            fileStartPoint(run->fp, run->binary, run->pointCount);
            fileAddRealValue(run->fp, run->binary, run->circuit->CKTinitTime);
            interpolatenow = nodata = FALSE;
        }
        /*  Save last time step  */
        else if (refValue->rValue == run->circuit->CKTfinalTime) {
            fileStartPoint(run->fp, run->binary, run->pointCount);
            fileAddRealValue(run->fp, run->binary, run->circuit->CKTfinalTime);
            interpolatenow = nodata = FALSE;
        }
        /*  Save exact point  */
        else if (refValue->rValue == timestep) {
            fileStartPoint(run->fp, run->binary, run->pointCount);
            fileAddRealValue(run->fp, run->binary, timestep);
            timestep += run->circuit->CKTstep;
            interpolatenow = nodata = FALSE;
        }
        else if (refValue->rValue > timestep) {
            /* add the next time step value to the vector */
            fileStartPoint(run->fp, run->binary, run->pointCount);
            timenew = refValue->rValue;
            fileAddRealValue(run->fp, run->binary, timestep);
            timestep += run->circuit->CKTstep;
            nodata = FALSE;
            interpolatenow = TRUE;
        }
        else {
            /* Do not save this step */
            run->pointCount--;
            nodata = TRUE;
            interpolatenow = FALSE;
        }
#ifndef HAS_WINGUI
        if (!orflag && !ft_norefprint && !cp_background) {
            currclock = clock();
            if ((currclock-lastclock) > (0.25*CLOCKS_PER_SEC)) {
                fprintf(stdout, " Reference value : % 12.5e\r",
                        refValue->rValue);
                fflush(stdout);
                lastclock = currclock;
            }
        }
#endif

    }

    for (i = 0; i < run->numData; i++) {
        /* we've already printed reference vec first */
        if (run->data[i].outIndex == -1)
            continue;

#ifdef TCL_MODULE
        blt_add(i, refValue ? refValue->rValue : NAN);
#endif

        if (run->data[i].regular) {
        /*  Store value or interpolate and store or do not store any value to file */
            if (!interpolatenow && !nodata) {
                /* store the first or last value */
                valueold[i] = valuePtr->v.vec.rVec [run->data[i].outIndex];
                fileAddRealValue(run->fp, run->binary, valueold[i]);
            }
            else if (interpolatenow) {
            /*  Interpolate time if actual time is greater than proposed next time step  */
                double newval;
                valuenew[i] = valuePtr->v.vec.rVec [run->data[i].outIndex];
                newval = (timestep -  run->circuit->CKTstep - timeold)/(timenew - timeold) * (valuenew[i] - valueold[i]) + valueold[i];
                fileAddRealValue(run->fp, run->binary, newval);
                valueold[i] = valuenew[i];
            }
            else if (nodata)
                /* Just keep the transient output value corresponding to timeold, 
                    but do not store to file */
                valueold[i] = valuePtr->v.vec.rVec [run->data[i].outIndex];
        } else {
            IFvalue val;
            /* should pre-check instance */
            if (!getSpecial(&run->data[i], run, &val)) {

                /*  If this is the first data point, print a warning for any unrecognized
                    variables, since this has not already been checked  */
                if (run->pointCount == 1)
                fprintf(stderr, "Warning: unrecognized variable - %s\n",
                        run->data[i].name);
                val.rValue = 0;
                fileAddRealValue(run->fp, run->binary, val.rValue);
                continue;
            }
            if (!interpolatenow && !nodata) {
                /* store the first or last value */
                valueold[i] = val.rValue;
                fileAddRealValue(run->fp, run->binary, valueold[i]);
            }
            else if (interpolatenow) {
            /*  Interpolate time if actual time is greater than proposed next time step  */
                double newval;
                valuenew[i] = val.rValue;
                newval = (timestep -  run->circuit->CKTstep - timeold)/(timenew - timeold) * (valuenew[i] - valueold[i]) + valueold[i];
                fileAddRealValue(run->fp, run->binary, newval);
                valueold[i] = valuenew[i];
            }
            else if (nodata)
                /* Just keep the transient output value corresponding to timeold, 
                    but do not store to file */
                valueold[i] = val.rValue;
        }

#ifdef TCL_MODULE
        blt_add(i, valuePtr->v.vec.rVec [run->data[i].outIndex]);
#endif

    }
    timeold = refValue->rValue;
    fileEndPoint(run->fp, run->binary);

    /*  Check that the write to disk completed successfully, otherwise abort  */
    if (ferror(run->fp)) {
        fprintf(stderr, "Warning: rawfile write error !!\n");
        shouldstop = TRUE;
    }

    if (ft_bpcheck(run->runPlot, run->pointCount) == FALSE)
        shouldstop = TRUE;

#ifdef TCL_MODULE
    Tcl_ExecutePerLoop();
#elif defined SHARED_MODULE
    sh_ExecutePerLoop();
#endif
    return(OK);
}

static int
InterpPlotAdd(runDesc *run, IFvalue *refValue, IFvalue *valuePtr)
{
    int i, iscale = -1;
    static double timeold = 0.0, timenew = 0.0, timestep = 0.0;
    bool nodata = FALSE;
    bool interpolatenow = FALSE;

    if (run->pointCount == 1)
        timestep = run->circuit->CKTinitTime + run->circuit->CKTstep;

    /* find the scale vector */
    for (i = 0; i < run->numData; i++)
        if (run->data[i].outIndex == -1) {
            iscale = i;
            break;
        }
    if (iscale == -1)
        fprintf(stderr, "Error: no scale vector found\n");

#ifdef TCL_MODULE
    /*Locks the blt vector to stop access*/
    blt_lockvec(iscale);
#endif

    /*  Save first time step  */
    if (refValue->rValue == run->circuit->CKTinitTime) {
        plotAddRealValue(&run->data[iscale], refValue->rValue);
        interpolatenow = nodata = FALSE;
    }
    /*  Save last time step  */
    else if (refValue->rValue == run->circuit->CKTfinalTime) {
        plotAddRealValue(&run->data[iscale], run->circuit->CKTfinalTime);
        interpolatenow = nodata = FALSE;
    }
    /*  Save exact point  */
    else if (refValue->rValue == timestep) {
        plotAddRealValue(&run->data[iscale], timestep);
        timestep += run->circuit->CKTstep;
        interpolatenow = nodata = FALSE;
    }
    else if (refValue->rValue > timestep) {
        /* add the next time step value to the vector */
        timenew = refValue->rValue;
        plotAddRealValue(&run->data[iscale], timestep);
        timestep += run->circuit->CKTstep;
        nodata = FALSE;
        interpolatenow = TRUE;
    }
    else {
        /* Do not save this step */
        run->pointCount--;
        nodata = TRUE;
        interpolatenow = FALSE;
    }

#ifdef TCL_MODULE
    /*relinks and unlocks vector*/
    blt_relink(iscale, (run->data[iscale]).vec);
#endif

#ifndef HAS_WINGUI
    if (!orflag && !ft_norefprint && !cp_background) {
        currclock = clock();
        if ((currclock-lastclock) > (0.25*CLOCKS_PER_SEC)) {
            fprintf(stdout, " Reference value : % 12.5e\r",
                    refValue->rValue);
            fflush(stdout);
            lastclock = currclock;
        }
    }
#endif

    for (i = 0; i < run->numData; i++) {
        if (i == iscale)
            continue;

#ifdef TCL_MODULE
        /*Locks the blt vector to stop access*/
        blt_lockvec(i);
#endif

        if (run->data[i].regular) {
        /*  Store value or interpolate and store or do not store any value to file */
            if (!interpolatenow && !nodata) {
                /* store the first or last value */
                valueold[i] = valuePtr->v.vec.rVec [run->data[i].outIndex];
                plotAddRealValue(&run->data[i], valueold[i]);
            }
            else if (interpolatenow) {
            /*  Interpolate time if actual time is greater than proposed next time step  */
                double newval;
                valuenew[i] = valuePtr->v.vec.rVec [run->data[i].outIndex];
                newval = (timestep -  run->circuit->CKTstep - timeold)/(timenew - timeold) * (valuenew[i] - valueold[i]) + valueold[i];
                plotAddRealValue(&run->data[i], newval);
                valueold[i] = valuenew[i];
            }
            else if (nodata)
                /* Just keep the transient output value corresponding to timeold, 
                    but do not store to file */
                valueold[i] = valuePtr->v.vec.rVec [run->data[i].outIndex];
        } else {
            IFvalue val;
            /* should pre-check instance */
            if (!getSpecial(&run->data[i], run, &val))
                continue;
            if (!interpolatenow && !nodata) {
                /* store the first or last value */
                valueold[i] = val.rValue;
                plotAddRealValue(&run->data[i], valueold[i]);
            }
            else if (interpolatenow) {
            /*  Interpolate time if actual time is greater than proposed next time step  */
                double newval;
                valuenew[i] = val.rValue;
                newval = (timestep -  run->circuit->CKTstep - timeold)/(timenew - timeold) * (valuenew[i] - valueold[i]) + valueold[i];
                plotAddRealValue(&run->data[i], newval);
                valueold[i] = valuenew[i];
            }
            else if (nodata)
                /* Just keep the transient output value corresponding to timeold, 
                    but do not store to file */
                valueold[i] = val.rValue;
        }

#ifdef TCL_MODULE
        /*relinks and unlocks vector*/
        blt_relink(i, (run->data[i]).vec);
#endif

    }
    timeold = refValue->rValue;
    gr_iplot(run->runPlot);

    if (ft_bpcheck(run->runPlot, run->pointCount) == FALSE)
        shouldstop = TRUE;

#ifdef TCL_MODULE
    Tcl_ExecutePerLoop();
#elif defined SHARED_MODULE
    sh_ExecutePerLoop();
#endif

    return(OK);
}
