/**********
Loop-gain (stability) analysis "stb": the margins of a loop gain.

The return ratio T of the sweep is turned into |T| in dB and the phase of
T in degrees, both over the logarithm of the frequency. The phase is made
continuous. The phase margin is taken where |T| falls through 0 dB for
the first time, the gain margin where the phase falls through -180
degrees for the first time; between two points of the sweep dB and phase
are taken as straight lines over log(f).

This file does arithmetic only: it prints nothing and keeps nothing.
**********/

#include "ngspice/ngspice.h"
#include "ngspice/stbdefs.h"


/* Where the straight line through (x0, y0) and (x1, y1) has the value
 * level. */
static double
stb_cross(double x0, double x1, double y0, double y1, double level)
{
    return x0 + (level - y0) * (x1 - x0) / (y1 - y0);
}


/* The value at x of the curve ys over xs, a straight line between its
 * points; the first pair of points that has x between them is used.
 * Returns 0 if there is none. */
static int
stb_sample(int n, const double *xs, const double *ys, double x, double *y)
{
    int p;

    for (p = 1; p < n; p++) {
        double x0 = xs[p - 1], x1 = xs[p];

        if (x < MIN(x0, x1) || x > MAX(x0, x1))
            continue;
        if (x1 == x0)
            *y = ys[p - 1];
        else
            *y = ys[p - 1] + (x - x0) * (ys[p] - ys[p - 1]) / (x1 - x0);
        return 1;
    }

    return 0;
}


/* The margins of the return ratio T = (tre, tim) at the n frequencies
 * freq. (The loop gain that the analysis writes is -T.) A margin that
 * does not exist is NAN, and *m says why. */
void
STBmargin(int n, const double *freq, const double *tre, const double *tim,
          STBmargins *m)
{
    double *lx;     /* log10 of the frequency */
    double *db;     /* |T| in dB */
    double *ph;     /* the phase of T in degrees, continuous */
    double maxmag = 0;
    double x, y;
    int k;
    int fall = 0;   /* the second point of the falling 0 dB crossing */

    m->pmFreq = m->pm = m->gmFreq = m->gm = NAN;
    m->status = STB_MG_OK;
    m->pmWhy = STB_PM_ALWAYS_BELOW;
    m->gmFound = 0;
    m->crossings = 0;
    m->riseFreq = NAN;
    m->atFreq = m->atVal = NAN;
    m->nBad = 0;
    m->lastFreq = m->lastDb = m->lastPhase = NAN;
    m->maxDb = m->maxDbFreq = NAN;

    /* T is not finite somewhere */
    for (k = 0; k < n; k++) {
        double mag = hypot(tre[k], tim[k]);

        if (!isfinite(tre[k]) || !isfinite(tim[k]) || !isfinite(mag)) {
            if (m->nBad == 0)
                m->atFreq = freq[k];
            m->nBad++;
        }
        else if (mag > maxmag) {
            maxmag = mag;
        }
    }
    if (m->nBad) {
        m->status = STB_MG_NONFINITE;
        return;
    }

    /* no points: nothing can be said about the loop gain */
    if (n < 1) {
        m->status = STB_MG_SHORT;
        return;
    }

    /* no loop gain at all */
    if (maxmag < 1e-12) {
        m->status = STB_MG_ZEROALL;
        return;
    }

    if (n < 2) {
        m->status = STB_MG_SHORT;
        return;
    }

    /* the frequencies must be positive and rising */
    for (k = 0; k < n; k++)
        if (!isfinite(freq[k]) || freq[k] <= 0 ||
            (k > 0 && freq[k] <= freq[k - 1])) {
            m->status = STB_MG_BADFREQ;
            m->atFreq = freq[k];
            return;
        }

    /* T is zero somewhere: no dB, no phase */
    for (k = 0; k < n; k++)
        if (hypot(tre[k], tim[k]) == 0) {
            m->status = STB_MG_ZERO;
            m->atFreq = freq[k];
            return;
        }

    lx = TMALLOC(double, n);
    db = TMALLOC(double, n);
    ph = TMALLOC(double, n);

    for (k = 0; k < n; k++) {
        double a = atan2(tim[k], tre[k]) * 180.0 / M_PI;

        lx[k] = log10(freq[k]);
        db[k] = 20.0 * log10(hypot(tre[k], tim[k]));

        /* The phase of T, not of -T, is made continuous: that of a loop
         * with negative feedback starts near 0 degrees, far from the cut
         * at 180 degrees. */
        if (k == 0) {
            while (a > 180.0)
                a -= 360.0;
            while (a <= -180.0)
                a += 360.0;
        }
        else {
            a = a - 360.0 * floor((a - ph[k - 1]) / 360.0 + 0.5);
        }
        ph[k] = a;
    }

    /* positive feedback at the first point: no margins */
    if (tre[0] < 0) {
        m->status = STB_MG_POSFB;
        m->atFreq = freq[0];
        m->atVal = ph[0] + 180.0;
        goto done;
    }

    m->lastFreq = freq[n - 1];
    m->lastDb = db[n - 1];
    m->lastPhase = ph[n - 1] + 180.0;
    m->maxDb = db[0];
    m->maxDbFreq = freq[0];
    for (k = 1; k < n; k++)
        if (db[k] > m->maxDb) {
            m->maxDb = db[k];
            m->maxDbFreq = freq[k];
        }

    /* the phase margin: where |T| falls through 0 dB for the first time */
    if (db[n - 1] > 0)
        m->pmWhy = STB_PM_STILL_ABOVE;
    for (k = 1; k < n; k++)
        if (db[k - 1] > 0 && db[k] <= 0) {
            x = stb_cross(lx[k - 1], lx[k], db[k - 1], db[k], 0.0);
            if (isfinite(x) && stb_sample(n, lx, ph, x, &y) && isfinite(y)) {
                m->pmFreq = pow(10.0, x);
                m->pm = 180.0 + y;
                m->pmWhy = STB_PM_FOUND;
                fall = k;
            }
            break;
        }

    /* the gain margin: where the phase of T falls through -180 degrees
     * for the first time */
    for (k = 1; k < n; k++)
        if (ph[k - 1] > -180.0 && ph[k] <= -180.0) {
            x = stb_cross(lx[k - 1], lx[k], ph[k - 1], ph[k], -180.0);
            if (isfinite(x) && stb_sample(n, lx, db, x, &y) && isfinite(y)) {
                m->gmFreq = pow(10.0, x);
                m->gm = -1.0 * y;
                m->gmFound = 1;
            }
            break;
        }

    /* all 0 dB crossings, and a rising one before the one used above */
    for (k = 1; k < n; k++) {
        if (db[k - 1] < 0 && db[k] >= 0) {
            m->crossings++;
            if (k < fall && isnan(m->riseFreq))
                m->riseFreq = pow(10.0, stb_cross(lx[k - 1], lx[k], db[k - 1], db[k], 0.0));
        }
        else if (db[k - 1] > 0 && db[k] <= 0) {
            m->crossings++;
        }
    }

done:
    tfree(lx);
    tfree(db);
    tfree(ph);
}
