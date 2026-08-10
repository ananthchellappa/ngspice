/*************
 * Header file for inpcom.c
 * 1999 E. Rouat
 ************/

#ifndef ngspice_INPCOM_H
#define ngspice_INPCOM_H

struct card *insert_new_line(struct card *card, char *line,
                             int linenum, int linenum_orig, char *linesource);
char *inp_pathresolve(const char *name);

extern char* inp_remove_ws(char* s);
extern char* search_plain_identifier(char* str, const char* identifier);

/* Leading device letter of a card, folded for dispatch only.  Defined in
   inpcom.c; shared so that inp.c and inpcompat.c classify a card the same
   way rather than growing their own copies. */
extern char elem_letter(const char *line);

#endif
