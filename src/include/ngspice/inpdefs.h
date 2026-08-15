/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
Modified: 2000 AlansFixes
**********/

#ifndef ngspice_INPDEFS_H
#define ngspice_INPDEFS_H

/* structure declarations used by either/both input package */

#include "ngspice/bool.h"
#include "ngspice/gendefs.h"
#include "ngspice/ifsim.h"
#include "ngspice/inpptree.h"

typedef struct INPtables INPtables;
typedef struct INPmodel INPmodel;

struct INPtab {
    char *t_ent;
    struct INPtab *t_next;
};

struct INPnTab {
    char *t_ent;
    CKTnode *t_node;
    struct INPnTab *t_next;
    /* set when only a reference ever named this node, so that
       INPtermCaseCheck() can tell a node no card defines from an ordinary
       one; cleared by the first INPtermInsert() that claims it.
       doc/claude/decisions/0002-deferred-node-resolution-check.md,
       doc/claude/decisions/0008-undefined-node-diagnostic.md */
    bool t_unclaimed;
    /* The spelling the deck used for this node, when the mode is fold and the
       card that interned it still carried its pre-fold text.  Under preserve
       and distinguish t_ent is already the deck's spelling and this stays
       NULL.  doc/codex/issues/0068 */
    char *t_spelling;
    /* the second spelling of this node's name, when the deck wrote one node
       two ways and this mode made them one node.  Under preserve the two
       spellings meet in term_insert(); under fold they meet as two answers
       from two cards' pre-fold text; under distinguish two spellings are two
       entries and the pair is found by scanning the bucket instead.
       Reported once, at the end of the parse, by INPtermCaseCheck().
       doc/codex/issues/0068 */
    char *t_casetwin;
};

struct INPtables {
    struct INPtab **INPsymtab;
    struct INPnTab **INPtermsymtab;
    int INPsize;
    int INPtermsize;
    GENmodel *defAmod;
    GENmodel *defBmod;
    GENmodel *defCmod;
    GENmodel *defDmod;
    GENmodel *defEmod;
    GENmodel *defFmod;
    GENmodel *defGmod;
    GENmodel *defHmod;
    GENmodel *defImod;
    GENmodel *defJmod;
    GENmodel *defKmod;
    GENmodel *defLmod;
    GENmodel *defMmod;
    GENmodel *defNmod;
    GENmodel *defOmod;
    GENmodel *defPmod;
    GENmodel *defQmod;
    GENmodel *defRmod;
    GENmodel *defSmod;
    GENmodel *defTmod;
    GENmodel *defUmod;
    GENmodel *defVmod;
    GENmodel *defWmod;
    GENmodel *defYmod;
    GENmodel *defZmod;
};

/* Linked list of scoping information for each netlist line entry */
struct nscope {
    struct nscope *next;
    struct card_assoc *subckts;
    struct modellist *models;
};

/* A linked list of netlist line entries, associated for a specific reason */
struct card_assoc {
    const char *name;
    struct card *line;
    struct card_assoc *next;
};

/* The linked list of netlist line entries */
struct card {
    int linenum;
    int linenum_orig;
    char* linesource;
    char *line;
    char *error;
    struct card *nextcard;
    struct card *actualLine;
    struct nscope *level;
    float w;
    float l;
    float nf;
    int compmod;
    /* The card as the deck wrote it, before inp_readall() lower cased it, or
       NULL when this card has no such text: a card the preprocessor built,
       a card copied for a subcircuit instantiation, or any card at all in a
       mode that does not fold.  term_insert() reads it to report a node name
       the deck spelled two ways, which is the one thing a fold run cannot
       otherwise know.  Appended rather than inserted: src/xspice/icm/dlmain.c
       is compiled into every .cm, so a field ahead of w/l/nf/compmod moves
       them for a code model built against an older header.
       doc/codex/issues/0068 */
    char *line_case;
};

/* structure used to save models in after they are read during pass 1 */
struct INPmodel {
    IFuid INPmodName; /* uid of model */
    int INPmodType; /* type index of device type */
    INPmodel *INPnextModel; /* link to next model */
    struct card *INPmodLine; /* pointer to line describing model */
    GENmodel *INPmodfast; /* high speed pointer to model for access */
};

// Ugly way to pass line onfo (number and source file) to lower-level error handlers.
extern int Current_parse_line;
extern char* Sourcefile;

/* The card the parser is reading right now, or NULL outside a parse pass.
   Set the same way and for the same reason as the two above: term_insert()
   is five call layers below the loop that holds the card, and what it needs
   from it is the pre-fold text of the line the node was written on.
   doc/codex/issues/0068 */
extern struct card *INPcurrent_card;

/* listing types - used for debug listings */
#define LOGICAL 1
#define PHYSICAL 2

int IFnewUid(CKTcircuit *, IFuid *, IFuid, char *, int, CKTnode **);
int IFdelUid(CKTcircuit *, IFuid, int);
int INPaName(char *, IFvalue *, CKTcircuit *, int *, char *, GENinstance **,
        IFsimulator *, int *, IFvalue *);
int INPapName(CKTcircuit *, int, JOB *, char *, IFvalue *);
void INPcaseFix(char *);
char *INPdevParse(char **, CKTcircuit *, int, GENinstance *, double *, int *,
        INPtables *);
char *INPdomodel(CKTcircuit *, struct card *, INPtables *);
void INPdoOpts(CKTcircuit *, JOB *, struct card *, INPtables *);
char *INPerrCat(char *, char *);
char *INPstrCat(char *, char, char *);
char *INPerror(int);
double INPevaluate(char **, int *, int);
double INPevaluate2(char **, int *, int);
double INPevaluateRKM_R(char **, int *, int);
double INPevaluateRKM_C(char **, int *, int);
double INPevaluateRKM_L(char **, int *, int);
char *INPfindLev(char *, int *);
char *INPgetMod(CKTcircuit *, char *, INPmodel **, INPtables *);
char *INPgetModBin(CKTcircuit *, char *, INPmodel **, INPtables *, char *);
int INPgetTok(char **, char **, int);
int INPgetNetTok(char **, char **, int);
void INPgetTree(char **, INPparseTree **, CKTcircuit *, INPtables *);
void INPfreeTree(IFparseTree *);
IFvalue *INPgetValue(CKTcircuit *, char **, int, INPtables *);
int INPgndInsert(CKTcircuit *, char **, INPtables *, CKTnode **);
int INPinsertNofree(char **token, INPtables *tab);
int INPinsert(char **, INPtables *);
int INPretrieve(char **, INPtables *);
int INPremove(char *, INPtables *);
INPmodel *INPlookMod(const char *);
char *INPmodKey(const char *);
int INPmakeMod(char *, int, struct card *);
char *INPmkTemp(char *);
void INPpas1(CKTcircuit *, struct card *, INPtables *);
int INPpas2(CKTcircuit *, struct card *, INPtables *, TSKtask *);
void INPpas3(
        CKTcircuit *, struct card *, INPtables *, TSKtask *, IFparm *, int);
void INPpas4(CKTcircuit *, INPtables *);
int INPpName(char *, IFvalue *, CKTcircuit *, int, GENinstance *);
int INPtermInsert(CKTcircuit *, char **, INPtables *, CKTnode **);
int INPtermInsertRef(CKTcircuit *, char **, INPtables *, CKTnode **);
void INPtermCaseCheck(INPtables *);
int INPtermSearch(CKTcircuit*, char**, INPtables*, CKTnode**);
int INPmkTerm(CKTcircuit *, char **, INPtables *, CKTnode **);
int INPtypelook(char *);
int INP2dot(CKTcircuit *, INPtables *, struct card *, TSKtask *, CKTnode *);
INPtables *INPtabInit(int);
void INPkillMods(void);
void INPtabEnd(INPtables *);
char *INPfindVer(char *line, char *version);
int INPgetStr(char **line, char **token, int gobble);
int INPgetTitle(CKTcircuit **ckt, struct card **data);
int INPgetUTok(char **line, char **token, int gobble);
int INPremTerm(char *token, INPtables *tab);



#endif
