#ifndef ngspice_DRAWLOG_H
#define ngspice_DRAWLOG_H

/* The draw log, 'set rnddraws=<file>': one tab-separated row per random
   draw made while a deck is parsed.  src/maths/misc/drawlog.c has the
   format. */

#include "ngspice/bool.h"

/* 'set rnddraws=<file>' and 'unset rnddraws', from cp_usrset() */
extern void ng_drawlog_open(const char *path);
extern void ng_drawlog_close(void);

/* one record set per parse, from inp_spsource() */
extern void ng_drawlog_begin(void);
extern void ng_drawlog_end(void);

/* TRUE while a record set is open: the hooks do nothing otherwise */
extern bool ng_drawlog_on(void);

/* context: the instance and the parameter the next draws belong to */
extern void ng_drawlog_card(const char *line, char first, const char *scope);
extern void ng_drawlog_context(const char *instance, const char *param);
extern void ng_drawlog_param(const char *param);
extern void ng_drawlog_param_at(const char *line, const char *pos);

/* one row: the random function, its arguments, z and the value returned */
extern void ng_drawlog_row(const char *func, int nargs,
                           double a1, double a2, double a3,
                           double z, double value);

/* ngSpice_Reset: close the log and forget the context */
extern void ng_drawlog_reset(void);

#endif
