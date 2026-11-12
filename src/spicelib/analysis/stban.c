/**********
Loop-gain (stability) analysis "stb".

The probe is a voltage source in the loop, or a terminal of an instance,
<instance>:<terminal>: then it is the 0 V source that the front end has
inserted at that terminal when it read the netlist (inpc_stb.c). At each
frequency the AC matrix is factored once and solved for two right-hand
sides, a unit voltage in series with the probe and a unit current into
its node. The two responses are combined into the return ratio T of the
loop after

M. Tian, V. Visvanathan, J. Hantgan, K. Kundert, "Striving for
small-signal stability", IEEE Circuits and Devices Magazine,
vol. 17, no. 1, 2001.

The plot holds loopgain = -T. A second plot holds the phase margin, the
gain margin and their frequencies, see stbmarg.c.

With localgnd=<node> the current returns to that node and the voltage of
the probe's node is taken against it, not against ground. With diag=1
the plot also holds the two parts of the loop gain, the normal and the
reverse one, and the four responses they are made of.
**********/

#include "ngspice/ngspice.h"
#include "ngspice/cktdefs.h"
#include "ngspice/acdefs.h"
#include "ngspice/stbdefs.h"
#include "ngspice/devdefs.h"
#include "ngspice/smpdefs.h"
#include "ngspice/sperror.h"
#include "ngspice/cpextern.h"
#include "ngspice/fteext.h"

#ifdef XSPICE
#include "ngspice/evt.h"
#include "ngspice/enh.h"
#include "ngspice/mif.h"
#include "ngspice/evtproto.h"
#include "ngspice/ipctiein.h"
#endif


#define INIT_STATS() \
do { \
    startTime  = SPfrontEnd->IFseconds();       \
    startdTime = ckt->CKTstat->STATdecompTime;  \
    startsTime = ckt->CKTstat->STATsolveTime;   \
    startlTime = ckt->CKTstat->STATloadTime;    \
    startkTime = ckt->CKTstat->STATsyncTime;    \
} while(0)

#define UPDATE_STATS(analysis) \
do { \
    ckt->CKTcurrentAnalysis = analysis; \
    ckt->CKTstat->STATacTime += SPfrontEnd->IFseconds() - startTime; \
    ckt->CKTstat->STATacDecompTime += ckt->CKTstat->STATdecompTime - startdTime; \
    ckt->CKTstat->STATacSolveTime += ckt->CKTstat->STATsolveTime - startsTime; \
    ckt->CKTstat->STATacLoadTime += ckt->CKTstat->STATloadTime - startlTime; \
    ckt->CKTstat->STATacSyncTime += ckt->CKTstat->STATsyncTime - startkTime; \
} while(0)


/* A word of a command that names something of the deck, in the spelling
 * the circuit has it in. Under the case mode fold the reader has made the
 * deck lower case, and a word typed at the prompt has not been: it is
 * folded here. Under preserve and distinguish the names of the circuit
 * keep the spelling of the deck, and folding the word would make every
 * one of them unreachable. The caller owns the copy. */
static char *
stb_key(const char *word)
{
    char *key = copy(word);

    if (inp_case_folding())
        strtolower(key);

    return key;
}


/* Are a and b the same letter of a name of the deck? ng_ideq() for one
 * character: any spelling is the name under preserve; under fold both
 * sides are lower case, under distinguish two spellings are two names. */
static int
stb_ch_eq(char a, char b)
{
    return inp_case_exact_ids() ? (a == b) : (tolower_c(a) == tolower_c(b));
}


/* The same with the arm given: exact is that of the case mode, or 0 in
 * the search for a name that differs only in case (stb_twin()). */
static int
stb_ch_eqx(char a, char b, int exact)
{
    return exact ? (a == b) : (tolower_c(a) == tolower_c(b));
}


/* Is path the path of the device whose flattened name goes on at flat?
 * Both are at the start of a part of the name. The flattened name has
 * the letter of the device and a dot before the path, v.x1.x2.vprb for
 * x1.x2.vprb, and once more before each instance of a subcircuit that is
 * defined inside another one: x3.xi.vprb is v.x3.v.xi.vprb. The letter is
 * that of the flattened name itself; the parts of the path are names of
 * the deck. */
static int
stb_is_path(char letter, const char *flat, const char *path, int exact)
{
    if (flat[0] == letter && flat[1] == '.' &&
        stb_is_path(letter, flat + 2, path, exact))
        return 1;

    while (*flat && *flat != '.' && stb_ch_eqx(*flat, *path, exact)) {
        flat++;
        path++;
    }
    if (!stb_ch_eqx(*flat, *path, exact))
        return 0;
    if (!*flat)
        return 1;

    return stb_is_path(letter, flat + 1, path + 1, exact);
}


static IFcomplex
stb_mul(IFcomplex a, IFcomplex b)
{
    IFcomplex c;

    c.real = a.real * b.real - a.imag * b.imag;
    c.imag = a.real * b.imag + a.imag * b.real;
    return c;
}


static IFcomplex
stb_div(IFcomplex a, IFcomplex b)
{
    IFcomplex c;
    double d = b.real * b.real + b.imag * b.imag;

    c.real = (a.real * b.real + a.imag * b.imag) / d;
    c.imag = (a.imag * b.real - a.real * b.imag) / d;
    return c;
}


/* What goes into the plot: a NaN is a plain NaN there. */
static IFcomplex
stb_plain(IFcomplex a)
{
    if (isnan(a.real))
        a.real = NAN;
    if (isnan(a.imag))
        a.imag = NAN;
    return a;
}


/* Minus a, for the plot: a zero stays a plain zero, and a NaN a plain NaN. */
static IFcomplex
stb_minus(IFcomplex a)
{
    IFcomplex c;

    c.real = isnan(a.real) ? NAN : 0.0 - a.real;
    c.imag = isnan(a.imag) ? NAN : 0.0 - a.imag;
    return c;
}


/* Find the instance called name. A device in a subcircuit may be given by
 * its path, x1.x2.vprb, for the flattened name v.x1.x2.vprb. The name of
 * the device is the end of the path, so one of the parts of a path after
 * the first begins with the letter of the device: a word without a dot is
 * no path, and x1.vprb is not that of a resistor.
 *
 * The table of the instances is looked up byte for byte. Under the case
 * mode preserve another spelling of a name is the same name, so the
 * instances are compared one by one before a path is tried.
 *
 * exact is the arm of ng_ideq() to compare with: that of the case mode
 * (stb_find()), or 0 for any spelling whatever the mode (stb_twin()).
 * *bypath is set if the instance was found by its path. */
static GENinstance *
stb_find_arm(CKTcircuit *ckt, char *name, int exact, int *bypath)
{
    GENinstance *inst = CKTfndDev(ckt, name);
    GENmodel *model;
    int i;

    *bypath = 0;

    if (!exact)
        for (i = 0; !inst && i < DEVmaxnum; i++)
            for (model = ckt->CKThead[i]; !inst && model;
                 model = model->GENnextModel)
                for (inst = model->GENinstances; inst;
                     inst = inst->GENnextInstance)
                    /* case-lint: helper - the preserve arm of ng_ideq() */
                    if (cieq(inst->GENname, name))
                        break;

    if (inst)
        return inst;

    for (i = 0; !inst && i < DEVmaxnum; i++)
        for (model = ckt->CKThead[i]; !inst && model;
             model = model->GENnextModel)
            for (inst = model->GENinstances; inst;
                 inst = inst->GENnextInstance) {
                const char *s = inst->GENname;
                const char *p;
                if (!s[0] || s[1] != '.')
                    continue;
                for (p = strchr(name, '.');
                     p && !stb_ch_eqx(p[1], s[0], exact); )
                    p = strchr(p + 1, '.');
                if (p && stb_is_path(s[0], s + 2, name, exact))
                    break;
            }

    *bypath = inst != NULL;
    return inst;
}


static GENinstance *
stb_find(CKTcircuit *ckt, char *name)
{
    int bypath;

    return stb_find_arm(ckt, name, inp_case_exact_ids(), &bypath);
}


/* A lookup has failed. Under the case mode distinguish a name that
 * differs from the one looked up only in case is another name, and the
 * likely mistake: the text that points at it, to end the message with
 * (doc/claude/decisions/0001-distinguish.md, decision 2). An empty text
 * in the other modes, where such a name would have been found, and if
 * there is none. The caller owns the text.
 *
 * The instance called name, or the one whose path name is. */
static char *
stb_twin(CKTcircuit *ckt, char *name)
{
    GENinstance *inst;
    int bypath;

    if (inp_case_mode() != NG_CASE_DISTINGUISH)
        return copy("");

    inst = stb_find_arm(ckt, name, 0, &bypath);
    if (!inst || inst->GENmodPtr->GENmodType < 0)
        return copy("");

    return tprintf("; %s'%s' differs only in case (casemode=distinguish)",
                   bypath ? "the path of " : "", inst->GENname);
}


/* The same for the node called name. */
static char *
stb_twin_node(CKTcircuit *ckt, const char *name)
{
    CKTnode *node;

    if (inp_case_mode() != NG_CASE_DISTINGUISH)
        return copy("");

    for (node = ckt->CKTnodes; node; node = node->next)
        /* a name that differs only in case from the one not found */
        /* case-lint: neither - a deliberate scan without regard to case */
        if (node->type == SP_VOLTAGE && cieq(node->name, name))
            return tprintf("; '%s' differs only in case (casemode=distinguish)",
                           node->name);

    return copy("");
}


/* Does the name s begin with the name of the deck called name? ng_ideq()
 * for a prefix, one character at a time. */
static int
stb_begins(const char *s, const char *name)
{
    for (; *name; s++, name++)
        if (!stb_ch_eq(*s, *name))
            return 0;

    return 1;
}


/* Is name a subcircuit instance, x1 or x1.x2? It is one if an element of
 * the circuit lies inside it: r.x1.x2.r5 is the resistor r5 of x1.x2. The
 * letter x is a word of the language, in any case in every case mode; the
 * name is one of the deck. */
static int
stb_is_subckt(CKTcircuit *ckt, const char *name)
{
    const char *last = strrchr(name, '.');
    size_t len = strlen(name);
    GENmodel *model;
    GENinstance *inst;
    int i;

    if (tolower_c(last ? last[1] : name[0]) != 'x')
        return 0;

    for (i = 0; i < DEVmaxnum; i++)
        for (model = ckt->CKThead[i]; model; model = model->GENnextModel)
            for (inst = model->GENinstances; inst; inst = inst->GENnextInstance) {
                const char *s = inst->GENname;
                if (s[0] && s[1] == '.' && stb_begins(s + 2, name) &&
                    s[len + 2] == '.')
                    return 1;
            }

    return 0;
}


/* The number of the node called name, -1 if the circuit has none. No node
 * is made: the name of a branch equation is no node. */
static int
stb_find_node(CKTcircuit *ckt, const char *name)
{
    CKTnode *node;

    /* a name of the deck: any spelling under preserve, the exact one
     * under distinguish */
    for (node = ckt->CKTnodes; node; node = node->next)
        if (node->type == SP_VOLTAGE && ng_ideq(node->name, name))
            return node->number;

    /* ground by its other name, a word of the language: any case */
    if (cieq(name, "gnd"))
        return 0;

    return -1;
}


/* The operating point, as ACan() finds it. */
static int
stb_op(CKTcircuit *ckt, int noop)
{
    int error;

#ifdef XSPICE
    /* Call EVTop if event-driven instances exist */
    if (ckt->evt->counts.num_insts != 0) {
        error = EVTop(ckt,
            (ckt->CKTmode & MODEUIC) | MODEDCOP | MODEINITJCT,
            (ckt->CKTmode & MODEUIC) | MODEDCOP | MODEINITFLOAT,
            ckt->CKTdcMaxIter,
            MIF_TRUE);
        EVTdump(ckt, IPC_ANAL_DCOP, 0.0);
        EVTop_save(ckt, MIF_TRUE, 0.0);
    }
    else
#endif
        if (noop) {
            /* skip OP if option NOOPAC is set and circuit is linear */
            fprintf(stdout, "\n Linear circuit, option noopac given: no OP analysis\n");
            error = OK;
        }
        else
            error = CKTop(ckt,
                (ckt->CKTmode & MODEUIC) | MODEDCOP | MODEINITJCT,
                (ckt->CKTmode & MODEUIC) | MODEDCOP | MODEINITFLOAT,
                ckt->CKTdcMaxIter);

    if (error)
        return error;

    ckt->CKTmode = (ckt->CKTmode & MODEUIC) | MODEDCOP | MODEINITSMSIG;
    return CKTload(ckt);
}


/* Solve the factored AC matrix for the right-hand side in CKTrhs and
 * CKTirhs; the solution replaces it. CKTrhsOld is not touched, it keeps
 * the operating point. */
static void
stb_solve(CKTcircuit *ckt)
{
    double startTime = SPfrontEnd->IFseconds();

    SMPcSolve(ckt->CKTmatrix, ckt->CKTrhs, ckt->CKTirhs,
              ckt->CKTrhsSpare, ckt->CKTirhsSpare);
    ckt->CKTstat->STATsolveTime += SPfrontEnd->IFseconds() - startTime;

    ckt->CKTrhs[0] = 0;
    ckt->CKTirhs[0] = 0;
}


/* Step to the next frequency of the sweep, as ACan() does. Returns 0 if
 * there is none because the step is degenerate (a single point). */
static int
stb_step(const STBAN *job, double *freq)
{
    if (job->STBstepType == LINEAR) {
        *freq += job->STBfreqDelta;
        return job->STBfreqDelta != 0;
    }

    *freq *= job->STBfreqDelta;
    return job->STBfreqDelta != 1;
}


/* The sweep is checked strictly: a value that cannot be used is an error
 * here, where ACan() goes on with a default.
 * Says what is wrong with it and returns 1, or returns 0 if it is good.
 * One point (stop equal to start) and a linear sweep from 0 Hz are good. */
static int
stb_sweep_check(const STBAN *job)
{
    const char *probe = job->STBprobe;
    double start = job->STBstartFreq;
    double stop = job->STBstopFreq;

    if (job->STBstepType != DECADE && job->STBstepType != OCTAVE &&
        job->STBstepType != LINEAR)
        fprintf(cp_err, "Error: stb: probe %s: the sweep is missing (dec|oct|lin <points> <fstart> <fstop>)\n",
                probe);
    else if (job->STBnumberSteps < 1)
        fprintf(cp_err, "Error: stb: probe %s: the number of points must be at least 1 (it is %d)\n",
                probe, job->STBnumberSteps);
    else if (!isfinite(start))
        fprintf(cp_err, "Error: stb: probe %s: the start frequency is not a finite number\n",
                probe);
    else if (!isfinite(stop))
        fprintf(cp_err, "Error: stb: probe %s: the stop frequency is not a finite number\n",
                probe);
    else if (job->STBstepType != LINEAR && start <= 0)
        fprintf(cp_err, "Error: stb: probe %s: the start frequency must be positive for a dec or oct sweep (it is %g)\n",
                probe, start);
    else if (start < 0)
        fprintf(cp_err, "Error: stb: probe %s: the start frequency must not be negative (it is %g)\n",
                probe, start);
    else if (stop <= 0)
        fprintf(cp_err, "Error: stb: probe %s: the stop frequency must be positive (it is %g)\n",
                probe, stop);
    else if (stop < start)
        fprintf(cp_err, "Error: stb: probe %s: the stop frequency %g is below the start frequency %g\n",
                probe, stop, start);
    else
        return 0;

    return 1;
}


#ifdef KLU
static void
stb_klu_complex(CKTcircuit *ckt)
{
    int i;

    /* Conversion from Real Matrix to Complex Matrix */
    if (ckt->CKTmatrix->CKTkluMODE &&
        !ckt->CKTmatrix->SMPkluMatrix->KLUmatrixIsComplex)
    {
        for (i = 0 ; i < DEVmaxnum ; i++)
            if (DEVices [i] && DEVices [i]->DEVbindCSCComplex && ckt->CKThead [i])
                DEVices [i]->DEVbindCSCComplex (ckt->CKThead [i], ckt) ;

        ckt->CKTmatrix->SMPkluMatrix->KLUmatrixIsComplex = KLUMatrixComplex ;
    }
}


static void
stb_klu_real(CKTcircuit *ckt)
{
    int i;

    /* Conversion from Complex Matrix to Real Matrix */
    if (ckt->CKTmatrix->CKTkluMODE &&
        ckt->CKTmatrix->SMPkluMatrix->KLUmatrixIsComplex)
    {
        for (i = 0 ; i < DEVmaxnum ; i++)
            if (DEVices [i] && DEVices [i]->DEVbindCSCComplexToReal && ckt->CKThead [i])
                DEVices [i]->DEVbindCSCComplexToReal (ckt->CKThead [i], ckt) ;

        ckt->CKTmatrix->SMPkluMatrix->KLUmatrixIsComplex = KLUmatrixReal ;
    }
}
#endif


/* Say what the margins are, or why there are none. */
static void
stb_log(const STBmargins *m)
{
    switch (m->status) {

    case STB_MG_NONFINITE:
        fprintf(cp_out, "stb: warning: the loop gain is not finite at %g Hz (%d point(s)); no margins\n",
                m->atFreq, m->nBad);
        return;
    case STB_MG_ZEROALL:
        fprintf(cp_out, "stb: the loop gain is zero at every frequency: is the probe in a feedback loop?\n");
        return;
    case STB_MG_SHORT:
        fprintf(cp_out, "stb: no margins: the sweep has fewer than two points\n");
        return;
    case STB_MG_BADFREQ:
        fprintf(cp_out, "stb: no margins: the sweep does not start above 0 Hz\n");
        return;
    case STB_MG_ZERO:
        fprintf(cp_out, "stb: no margins: the loop gain is exactly zero at %g Hz\n",
                m->atFreq);
        return;
    case STB_MG_POSFB:
        fprintf(cp_out, "stb: no margins: positive feedback at the start frequency (loop-gain phase %g deg at %g Hz): the phase lag of the return ratio at the first point exceeds 90 degrees; a loop with an ideal integrator triggers this\n",
                m->atVal, m->atFreq);
        return;
    default:
        break;
    }

    if (m->pmWhy == STB_PM_FOUND)
        fprintf(cp_out, "stb: Phase margin = %g deg at frequency = %g Hz\n",
                m->pm, m->pmFreq);
    else if (m->pmWhy == STB_PM_STILL_ABOVE)
        fprintf(cp_out, "stb: no phase margin: the loop gain is still %+g dB at the last frequency %g Hz\n",
                m->lastDb, m->lastFreq);
    else
        fprintf(cp_out, "stb: no phase margin: the loop gain is below 0 dB at every frequency of the sweep (at most %g dB, at %g Hz)\n",
                m->maxDb, m->maxDbFreq);

    if (m->gmFound)
        fprintf(cp_out, "stb: Gain margin = %g dB at frequency = %g Hz\n",
                m->gm, m->gmFreq);
    else
        fprintf(cp_out, "stb: no gain margin: the loop-gain phase never falls to 0 deg (it is %g deg at the last frequency %g Hz)\n",
                m->lastPhase, m->lastFreq);

    if (m->crossings > 1)
        fprintf(cp_out, "stb: the loop gain crosses 0 dB at %d frequencies; the margins are at the first falling one\n",
                m->crossings);
    if (!isnan(m->riseFreq))
        fprintf(cp_out, "stb: the loop gain rises through 0 dB at %g Hz, before the crossing used for the margins\n",
                m->riseFreq);
}


int
STBan(CKTcircuit *ckt, int restart)
{
    STBAN *job = (STBAN *) ckt->CKTcurJob;

    double freq;
    double freqTol; /* tolerance parameter for finding final frequency */
    double startdTime;
    double startsTime;
    double startlTime;
    double startkTime;
    double startTime;
    int error;
    int size;
    int i;
    int n;
    int npoints;
    char *key;              /* the probe, as the circuit spells it */
    const char *vname;      /* the source inserted for a terminal probe */
    const char *reason;     /* why none was inserted */
    int issubckt;
    GENinstance *inst;
    int vtype;
    int x;                  /* the node of the probe that is excited */
    int y;                  /* its other node */
    int lg;                 /* the reference node: ground, or localgnd */
    int br;                 /* the branch equation of the probe */
    double s;               /* -1 if the source is taken reversed */
    int ncol;               /* the vectors of the plot: 1, with diag 7 */
    double *freqs = NULL;   /* the results, written when the sweep is done */
    IFcomplex *rows = NULL; /* ncol values for each frequency */
    double *tre = NULL;     /* the return ratio T */
    double *tim = NULL;
    double *opsave = NULL;  /* the operating point, for the option keepopinfo */
    STBmargins mg;
    double mvals[4];
    IFuid freqUid;
    IFuid gainUids[7];
    char *gainNames[7] = {
        "loopgain", "loopgain_normal", "loopgain_reverse",
        "stb_i1", "stb_v1", "stb_i2", "stb_v2"
    };
    IFuid marginUids[4];
    IFuid *nameList = NULL;
    int numNames;
    IFvalue refValue;
    IFvalue value;
    runDesc *plot = NULL;

    NG_IGNORE(restart);

    /* Nothing is created and nothing is changed before the line that made
     * this job, its sweep and its probe are known to be good. */

    if (job->STBerror) {
        fprintf(cp_err, "Error: stb: %s\n", job->STBerror);
        return E_PARMVAL;
    }

    if (!job->STBprobe) {
        fprintf(cp_err, "Error: stb: no probe given\n");
        return E_PARMVAL;
    }

    if (stb_sweep_check(job))
        return E_PARMVAL;

    /* the probe, as the circuit spells its names */
    key = stb_key(job->STBprobe);
    inst = stb_find(ckt, key);

    /* A terminal of an instance, <instance>:<terminal>: the 0 V source
     * that the front end has inserted there when it read the netlist is
     * the probe. A word that is the name of an instance is no terminal. */
    if (!inst && strchr(key, ':')) {
        switch (inp_stb_lookup(ckt, key, &vname, &reason)) {
        case 0:
            inst = CKTfndDev(ckt, (char *) vname);
            break;
        case 1:
            fprintf(cp_err, "Error: stb: probe %s: %s\n",
                    job->STBprobe, reason);
            tfree(key);
            return E_PARMVAL;
        default:
            fprintf(cp_err, "Error: stb: probe %s: this terminal was not declared when the netlist was read: add a .stb line for it, or write the stb command with this probe literally in the .control section of the deck\n",
                    job->STBprobe);
            tfree(key);
            return E_NOTFOUND;
        }
    }

    issubckt = !inst && stb_is_subckt(ckt, key);

    if (!inst || inst->GENmodPtr->GENmodType < 0) {
        if (issubckt) {
            fprintf(cp_err, "Error: stb: probe %s is a subcircuit instance: name one of its ports, e.g. %s:1\n",
                    job->STBprobe, job->STBprobe);
        } else {
            char *twin = stb_twin(ckt, key);
            fprintf(cp_err, "Error: stb: probe %s: no such instance in the circuit%s\n",
                    job->STBprobe, twin);
            tfree(twin);
        }
        tfree(key);
        return E_NOTFOUND;
    }
    tfree(key);

    /* Only a voltage source is a probe by itself. The terminal that the
     * message names is written with the name the instance has in the
     * circuit. */
    vtype = CKTtypelook("Vsource");
    if (inst->GENmodPtr->GENmodType != vtype) {
        /* the letter of the device, a word of the language: any case */
        char dev = tolower_c(inst->GENname[0]);

        if (strchr("mqjz", dev))
            fprintf(cp_err, "Error: stb: probe %s: the loop gain of an active device by itself is not supported: name one of its terminals, e.g. %s:%c\n",
                    job->STBprobe, inst->GENname,
                    dev == 'q' ? 'b' : 'g');
        else if (strchr("efgh", dev))
            fprintf(cp_err, "Error: stb: probe %s: the loop gain of an active device by itself is not supported: name one of its terminals, e.g. %s:1\n",
                    job->STBprobe, inst->GENname);
        else if (strchr("ka", dev))
            /* a coupling has no terminals, and those of a code model
             * instance cannot be probes */
            fprintf(cp_err, "Error: stb: probe %s is a %s, not a voltage source\n",
                    job->STBprobe,
                    DEVices[inst->GENmodPtr->GENmodType]->DEVpublic.name);
        else
            fprintf(cp_err, "Error: stb: probe %s is a %s, not a voltage source: to break the loop at one of its terminals write %s:1\n",
                    job->STBprobe,
                    DEVices[inst->GENmodPtr->GENmodType]->DEVpublic.name,
                    inst->GENname);
        return E_UNSUPP;
    }

    /* The reference node is ground, or the node that localgnd names, in
     * the spelling the circuit has its names in (stb_key()). */
    lg = 0;
    if (job->STBlocalGnd) {
        key = stb_key(job->STBlocalGnd);
        lg = stb_find_node(ckt, key);
        if (lg < 0) {
            char *twin = stb_twin_node(ckt, key);
            fprintf(cp_err, "Error: stb: probe %s: localgnd node %s does not exist%s\n",
                    job->STBprobe, job->STBlocalGnd, twin);
            tfree(twin);
            tfree(key);
            return E_NOTFOUND;
        }
        tfree(key);
    }

    /* The excited node is the positive node of the source. If that is the
     * reference node, the source is taken the other way round. */
    x = GENnode(inst)[0];
    y = GENnode(inst)[1];
    s = 1;
    if (x == lg) {
        x = y;
        y = lg;
        s = -1;
    }
    if (x == lg) {
        fprintf(cp_err, "Error: stb: probe %s: both of its nodes are the reference node\n",
                job->STBprobe);
        return E_PARMVAL;
    }
    br = CKTfndBranch(ckt, inst->GENname);

    /* the sweep, as ACan() makes it */
    switch (job->STBstepType) {

    case DECADE:
        if (job->STBstopFreq / 10. < job->STBstartFreq) {
            /* start-stop frequencies less than a decade apart */
            if (job->STBstopFreq == job->STBstartFreq)
                job->STBfreqDelta = 1;
            else
                job->STBfreqDelta = exp(log(10.0) / job->STBnumberSteps);
        }
        else {
            double num_steps = floor(fabs(log10(job->STBstopFreq / job->STBstartFreq)) * job->STBnumberSteps);
            job->STBfreqDelta = exp((log(job->STBstopFreq / job->STBstartFreq)) / num_steps);
        }
        freqTol = job->STBfreqDelta * job->STBstopFreq * ckt->CKTreltol;
        break;
    case OCTAVE:
        job->STBfreqDelta = exp(log(2.0) / job->STBnumberSteps);
        freqTol = job->STBfreqDelta * job->STBstopFreq * ckt->CKTreltol;
        break;
    default:
        if (job->STBnumberSteps - 1 > 1)
            job->STBfreqDelta =
                (job->STBstopFreq - job->STBstartFreq) /
                (job->STBnumberSteps - 1);
        else
            job->STBfreqDelta = 0;
        freqTol = job->STBfreqDelta * ckt->CKTreltol;
        break;
    }

    /* The number of points. A step that is too small to change the
     * frequency, more points than can be counted, or a sweep so wide that
     * its step is not a number and no point is counted, is an error. */
    npoints = 0;
    for (freq = job->STBstartFreq; freq <= job->STBstopFreq + freqTol; ) {
        double prev = freq;

        npoints++;
        if (!stb_step(job, &freq))
            break;
        if (freq == prev) {
            fprintf(cp_err, "Error: stb: probe %s: the step of the sweep is too small to change the frequency\n",
                    job->STBprobe);
            return E_PARMVAL;
        }
        if (npoints == INT_MAX) {
            fprintf(cp_err, "Error: stb: probe %s: the sweep has too many points\n",
                    job->STBprobe);
            return E_PARMVAL;
        }
    }
    if (npoints < 1) {
        fprintf(cp_err, "Error: stb: probe %s: the sweep from %g Hz to %g Hz is too wide\n",
                job->STBprobe, job->STBstartFreq, job->STBstopFreq);
        return E_PARMVAL;
    }

    fprintf(cp_out, "stb: probe %s (voltage source %s, x = %s, y = %s%s%s%s), %d points, %g Hz to %g Hz\n",
            job->STBprobe, inst->GENname,
            CKTnodName(ckt, x), CKTnodName(ckt, y),
            job->STBlocalGnd ? ", localgnd = " : "",
            job->STBlocalGnd ? CKTnodName(ckt, lg) : "",
            (s < 0) ? ", source reversed" : "",
            npoints, job->STBstartFreq, job->STBstopFreq);

#ifdef XSPICE
    /* Tell the code models what mode we're in */
    g_mif_info.circuit.anal_type = MIF_DC;
    g_mif_info.circuit.anal_init = MIF_TRUE;
#endif

#ifdef KLU
    /* an analysis that failed may have left the matrix in its complex form */
    stb_klu_real(ckt);
#endif

    error = stb_op(ckt, ckt->CKTnoopac);
    if (error) {
        fprintf(stdout, "\nSTB operating point failed -\n");
        CKTncDump(ckt);
        fprintf(cp_err, "Error: stb: probe %s: the operating point did not converge\n",
                job->STBprobe);
        return error;
    }

    /* The operating point is written when the sweep is done, with the
     * other plots. It is kept here: the sweep solves it again at each
     * frequency if a source depends on "hertz". */
    if (ckt->CKTkeepOpInfo) {
        opsave = TMALLOC(double, ckt->CKTmaxEqNum);
        memcpy(opsave, ckt->CKTrhsOld, (size_t) ckt->CKTmaxEqNum * sizeof(double));
    }

    ncol = job->STBdiag ? 7 : 1;
    freqs = TMALLOC(double, npoints);
    rows = TMALLOC(IFcomplex, (size_t) npoints * (size_t) ncol);
    tre = TMALLOC(double, npoints);
    tim = TMALLOC(double, npoints);

#ifdef XSPICE
    g_mif_info.circuit.anal_init = MIF_TRUE;

    /* Tell the code models what mode we're in */
    g_mif_info.circuit.anal_type = MIF_AC;
#endif

    INIT_STATS();

    ckt->CKTcurrentAnalysis = DOING_AC;

#ifdef KLU
    stb_klu_complex(ckt);
#endif

    size = SMPmatSize(ckt->CKTmatrix);
    freq = job->STBstartFreq;

    /* main loop through all scheduled frequencies */
    for (n = 0; n < npoints; n++) {
        IFcomplex i1, v1, i2, v2;   /* the responses to the two excitations */
        IFcomplex p, k, d, t;
        IFcomplex *row = rows + (size_t) n * (size_t) ncol;

        if (SPfrontEnd->IFpauseTest()) {
            /* user asked us to stop via an interrupt */
            fprintf(cp_err, "Error: stb: probe %s: interrupted at %g Hz; no plot was made\n",
                    job->STBprobe, freq);
            errMsg = copy("stb interrupted");
            error = E_UNSUPP;
            break;
        }
        ckt->CKTomega = 2.0 * M_PI * freq;

        /* Update operating point, if variable 'hertz' is given */
        if (ckt->CKTvarHertz) {
#ifdef KLU
            stb_klu_real(ckt);
#endif
            error = stb_op(ckt, 0);
            if (error) {
                fprintf(stdout, "\nSTB operating point failed -\n");
                CKTncDump(ckt);
                fprintf(cp_err, "Error: stb: probe %s: the operating point did not converge\n",
                        job->STBprobe);
                break;
            }
#ifdef KLU
            stb_klu_complex(ckt);
            /* the operating point has left real factors behind: the
             * complex ones are made anew, not refreshed */
            if (ckt->CKTmatrix->CKTkluMODE)
                ckt->CKTniState |= NIACSHOULDREORDER;
#endif
        }

        ckt->CKTmode = (ckt->CKTmode & MODEUIC) | MODEAC;
        error = NIacFactor(ckt);
        if (error) {
            fprintf(cp_err, "Error: stb: probe %s: cannot solve the AC matrix at %g Hz\n",
                    job->STBprobe, freq);
            break;
        }

        /* The right-hand side that the load has made is dropped as a whole:
         * the AC values of all sources are ignored. Their matrix entries
         * stay, so a voltage source is a short and a current source is
         * open. */

        /* first excitation: what "V x y ac 1" puts on the right-hand side */
        for (i = 0; i <= size; i++) {
            ckt->CKTrhs[i] = 0;
            ckt->CKTirhs[i] = 0;
        }
        ckt->CKTrhs[br] += s;
        stb_solve(ckt);
        i1.real = s * ckt->CKTrhs[br];
        i1.imag = s * ckt->CKTirhs[br];
        v1.real = ckt->CKTrhs[x] - ckt->CKTrhs[lg];
        v1.imag = ckt->CKTirhs[x] - ckt->CKTirhs[lg];

        /* second excitation: what "I <reference> x ac 1" puts on the
         * right-hand side; ground has no equation */
        for (i = 0; i <= size; i++) {
            ckt->CKTrhs[i] = 0;
            ckt->CKTirhs[i] = 0;
        }
        ckt->CKTrhs[x] += 1;
        ckt->CKTrhs[lg] -= 1;
        ckt->CKTrhs[0] = 0;
        stb_solve(ckt);
        i2.real = s * ckt->CKTrhs[br];
        i2.imag = s * ckt->CKTirhs[br];
        v2.real = ckt->CKTrhs[x] - ckt->CKTrhs[lg];
        v2.imag = ckt->CKTirhs[x] - ckt->CKTirhs[lg];

        /* K = 2 (i1 v2 - v1 i2) + v1 + i2,  T = K / (1 - K) */
        p = stb_mul(i1, v2);
        t = stb_mul(v1, i2);
        p.real -= t.real;
        p.imag -= t.imag;
        k.real = 2 * p.real + v1.real + i2.real;
        k.imag = 2 * p.imag + v1.imag + i2.imag;
        d.real = 1 - k.real;
        d.imag = -k.imag;
        t = stb_div(k, d);

        freqs[n] = freq;
        tre[n] = t.real;
        tim[n] = t.imag;
        row[0] = stb_minus(t);
        if (ncol > 1) {
            /* the normal and the reverse part of T, which sum to it:
             * (P + i2) / (1 - K) and (P + v1) / (1 - K), where
             * P = i1 v2 - v1 i2 */
            t.real = p.real + i2.real;
            t.imag = p.imag + i2.imag;
            row[1] = stb_minus(stb_div(t, d));
            t.real = p.real + v1.real;
            t.imag = p.imag + v1.imag;
            row[2] = stb_minus(stb_div(t, d));
            row[3] = stb_plain(i1);
            row[4] = stb_plain(v1);
            row[5] = stb_plain(i2);
            row[6] = stb_plain(v2);
        }

#ifdef HAS_PROGREP
        SetAnalyse("stb", (int) (1000.0 * (n + 1) / npoints));
#endif

        stb_step(job, &freq);
    }

#ifdef KLU
    /* the matrix is handed back as it was taken, error or not */
    stb_klu_real(ckt);
#endif

    UPDATE_STATS(0);

    if (!error) {
        STBmargin(npoints, freqs, tre, tim, &mg);
        stb_log(&mg);
    }

    /* The operating point plot, as ACan() writes it for the option
     * keepopinfo, but only now: a sweep that failed makes no plot. */
    if (!error && opsave)
        error = CKTnames(ckt, &numNames, &nameList);

    if (!error && opsave)
        error = SPfrontEnd->OUTpBeginPlot(ckt, ckt->CKTcurJob,
            STB_OP_TITLE,
            NULL, IF_REAL,
            numNames, nameList, IF_REAL,
            &plot);

    if (!error && opsave) {
        /* CKTdump() writes CKTrhsOld, in the mode of an operating point */
        double *rhsOld = ckt->CKTrhsOld;
        long mode = ckt->CKTmode;

        ckt->CKTrhsOld = opsave;
        ckt->CKTmode = (mode & MODEUIC) | MODEDCOP | MODEINITSMSIG;
        CKTdump(ckt, 0.0, plot);
        ckt->CKTmode = mode;
        ckt->CKTrhsOld = rhsOld;
        SPfrontEnd->OUTendPlot(plot);
        plot = NULL;
    }

    if (!error) {
        SPfrontEnd->IFnewUid(ckt, &freqUid, NULL, "frequency", UID_OTHER, NULL);
        for (i = 0; i < ncol; i++)
            SPfrontEnd->IFnewUid(ckt, &gainUids[i], NULL, gainNames[i], UID_OTHER, NULL);
        error = SPfrontEnd->OUTpBeginPlot(ckt, ckt->CKTcurJob,
            STB_PLOT_TITLE,
            freqUid, IF_REAL,
            ncol, gainUids, IF_COMPLEX,
            &plot);
    }

    if (!error) {
        if (job->STBstepType != LINEAR)
            SPfrontEnd->OUTattributes(plot, NULL, OUT_SCALE_LOG, NULL);

        for (n = 0; n < npoints; n++) {
            refValue.cValue.real = freqs[n];
            refValue.cValue.imag = 0.0;
            value.v.numValue = ncol;
            value.v.vec.cVec = rows + (size_t) n * (size_t) ncol;
            SPfrontEnd->OUTpData(plot, &refValue, &value);
        }
        SPfrontEnd->OUTendPlot(plot);

        /* The margins, in a plot of their own with no reference vector.
         * The front end knows it by its title: the loop-gain plot is the
         * current plot again when this one is complete. */
        SPfrontEnd->IFnewUid(ckt, &marginUids[0], NULL, "phasemarginfreq", UID_OTHER, NULL);
        SPfrontEnd->IFnewUid(ckt, &marginUids[1], NULL, "phasemargin", UID_OTHER, NULL);
        SPfrontEnd->IFnewUid(ckt, &marginUids[2], NULL, "gainmarginfreq", UID_OTHER, NULL);
        SPfrontEnd->IFnewUid(ckt, &marginUids[3], NULL, "gainmargin", UID_OTHER, NULL);
        error = SPfrontEnd->OUTpBeginPlot(ckt, ckt->CKTcurJob,
            STB_MARGIN_TITLE,
            NULL, 0,
            4, marginUids, IF_REAL,
            &plot);
    }

    if (!error) {
        mvals[0] = mg.pmFreq;
        mvals[1] = mg.pm;
        mvals[2] = mg.gmFreq;
        mvals[3] = mg.gm;
        refValue.rValue = 0.0;
        value.v.numValue = 4;
        value.v.vec.rVec = mvals;
        SPfrontEnd->OUTpData(plot, &refValue, &value);
        SPfrontEnd->OUTendPlot(plot);

        /* a stop asked for while the finished sweep was written (a
         * breakpoint on a vector of the plots) has nothing left to stop */
        SPfrontEnd->IFpauseTest();
    }

    tfree(freqs);
    tfree(rows);
    tfree(tre);
    tfree(tim);
    tfree(opsave);
    tfree(nameList);

    return error;
}
