/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
**********/

#include "ngspice/ngspice.h"
#include <stdio.h>
#include "ngspice/inpdefs.h"
#include "ngspice/iferrmsg.h"
#include "ngspice/hash.h"
#include "ngspice/fteext.h"
#include "inpxx.h"

/*  global input model table.  */
INPmodel *modtab = NULL;
/* Global input model hash table.
   The modelname is the key, the return value is the pointer to the model. */
NGHASHPTR modtabhash = NULL;

/*--------------------------------------------------------------
 * The key a model name is stored under in modtabhash, as a fresh string the
 * caller must tfree().  Under a non-folding case mode the key is folded so
 * that two spellings of one model name reach one entry, while INPmodName
 * keeps the spelling the deck used: that pointer is the IFuid handed to
 * newModel and is what every diagnostic prints.  The key is folded here
 * rather than by installing a case insensitive hash function, because
 * src/misc/hash.c copies the key on insert and frees it again only when
 * hash_func is NGHASH_DEF_HASH(NGHASH_FUNC_STR).  Under distinguish the key
 * is not folded at all, so two model names differing only in case are two
 * entries rather than one.
 *--------------------------------------------------------------*/

char *INPmodKey(const char *name)
{
   char *key = copy(name);

   if (!inp_case_exact_ids())
       strtolower(key);

   return key;
}

/*--------------------------------------------------------------
 * This fcn takes the model name and looks to see if it is already
 * in the model table.  If it is, then just return.  Otherwise,
 * stick the model into the model table.
 * Note that the model table INPmodel *modtab is a linked list,
 * in parallel a hash table modtabhash is filled in for faster
 * access to modtab elements by giving the model name.
 *--------------------------------------------------------------*/

int INPmakeMod(char *token, int type, struct card *line)
{
   register INPmodel *newm;
   /* The probe below and the insert further down must be keyed the same way,
      so the key is built once here rather than inside either branch. */
   char *key = INPmodKey(token);

   /* Initialze the hash table. The default key type is string.
      The default comparison function is strcmp.*/
   if (!modtabhash) {
       modtabhash = nghash_init(NGHASH_MIN_SIZE);
       nghash_unique(modtabhash, TRUE);
   }
   /* If the model is already there, just return. */
   else if (nghash_find(modtabhash, key)) {
       tfree(key);
       return (OK);
   }

   /* Model name was not already in model table. Therefore stick
      it in the front of the model table, also into the model hash table.
      Then return.  */

#ifdef TRACE
   /* debug statement */
   printf("In INPmakeMod, about to insert new model name = %s . . .\n", token);
#endif

   newm = TMALLOC(INPmodel, 1);
   if (newm == NULL) {
      tfree(key);
      return (E_NOMEM);
   }

   newm->INPmodName = token;                 /* model name */
   newm->INPmodType = type;                  /* model type */
   newm->INPnextModel = modtab;              /* pointer to second model */
   newm->INPmodLine = line;                  /* model line */
   newm->INPmodfast = NULL;

   nghash_insert(modtabhash, key, newm);
   tfree(key);

   modtab = newm;

   return (OK);
}

