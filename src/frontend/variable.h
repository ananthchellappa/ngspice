/*
  variable.h
*/

#ifndef ngspice_VARIABLE_H
#define ngspice_VARIABLE_H

#include "ngspice/cpextern.h"

/* Variables that are accessible to the parser via $varname
 * expansions.  If the type is CP_LIST the value is a pointer to a
 * list of the elements.  */
struct variable {
    enum cp_types va_type;
    char *va_name;
    union {
        bool vV_bool;
        int vV_num;
        double vV_real;
        char *vV_string;
        struct variable *vV_list;
    } va_V;
    struct variable *va_next;      /* Link. */
};

#define va_bool   va_V.vV_bool
#define va_num    va_V.vV_num
#define va_real   va_V.vV_real
#define va_string va_V.vV_string
#define va_vlist  va_V.vV_list


extern struct variable *variables;
extern bool cp_echo;

/* A policy read: the same chain cp_getvar() walks (declared in
 * ngspice/cpextern.h), minus the current plot's environment, which describes
 * a plot the user loaded and may not decide what this session does.  For the
 * reads a caller knows are policy; the reads taken while a netlist is being
 * read are narrowed without asking the caller.  src/frontend/variable.c says
 * why, doc/codex/issues/0061 is the issue. */
bool cp_getvar_policy(char *name, enum cp_types type, void *retval, size_t rsize);

/* extern struct variable *variables; */
wordlist *cp_varwl(struct variable *var);
wordlist *cp_variablesubst(wordlist *wlist);
void free_struct_variable(struct variable *v);

struct variable *var_alloc(char *name, struct variable *next);

struct variable *var_alloc_bool(char *name, bool, struct variable *next);
struct variable *var_alloc_num(char *name, int, struct variable *next);
struct variable *var_alloc_real(char *name, double, struct variable *next);
struct variable *var_alloc_string(char *name, char *, struct variable *next);
struct variable *var_alloc_vlist(char *name, struct variable *, struct variable *next);

void var_set_bool(struct variable *, bool);
void var_set_num(struct variable *, int);
void var_set_real(struct variable *, double);
void var_set_string(struct variable *, char *);
void var_set_vlist(struct variable *, struct variable *);

#endif
