/*************
 * Header file for inpc_stb.c
 ************/

#ifndef ngspice_INPC_STB_H
#define ngspice_INPC_STB_H

#include "ngspice/bool.h"
#include "ngspice/hash.h"
#include "ngspice/wordlist.h"
#include "ngspice/typedefs.h"
#include "ngspice/inpdefs.h"

struct card;
struct stb_probe;

void inp_stb_restore(struct card *deck);
void inp_stb_collect(struct card *deck, wordlist *controls, struct card *mcdeck);
void inp_stb_expanding(bool on);
bool inp_stb_pending(void);
char *inp_stb_xcard(const char *xline, const char *subname, const char *formals,
                    int nformals, NGHASHPTR globals);
void inp_stb_insert(struct card *deck);
wordlist *inp_stb_saves(wordlist *wl);
void inp_stb_rename(CKTcircuit *ckt, INPtables *tab);
struct stb_probe *inp_stb_take(void);
void inp_stb_discard(void);
void inp_stb_free(struct stb_probe *list);

#endif
