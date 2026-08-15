/* Front-end symbols the rawfile layer calls and ngsconvert does not have.

   ngsconvert links a hand-picked list of front-end objects rather than
   libfte (see ngsconvert_LDADD in src/Makefile.am) and supplies the rest of
   what they reference itself -- most of it at the bottom of ngsconvert.c,
   which has stubbed cp_usrset(), cp_usrvars() and their neighbours for as
   long as the program has existed.  The three below were added when
   frontend/rawfile.c and frontend/variable.c acquired the identifier case
   mode: raw_write() records the mode in the header, raw_read() binds a
   scale reference by the front-end's vector-name rule, and cp_getvar() asks
   whether a netlist is being read.  All three live in frontend/inpcom.c and
   frontend/vectors.c, and adding either object to that link list pulls in
   fifty more symbols behind it -- the netlist reader, the plot list, the
   hash tables -- for a program that reads no netlist and keeps no plots.

   These are not placeholders that lie about the answer.  ngsconvert never
   reads a netlist, so ng_case_mode never leaves its initial NG_CASE_FOLD and
   the real functions, linked in, would answer exactly what these do: 'fold',
   FALSE, and the case-insensitive comparison that fold and preserve both
   use.  If ngsconvert ever grows a netlist reader, they stop being right and
   the objects have to come in for real.  doc/codex/issues/0061.

   inp_case_mode_name() is nonetheless never reached here, and that is the
   point of the gate rather than an accident of it.  raw_write() writes the
   'Option: casemode=' line only when cp_getvar() finds 'casemodewrite', and
   in this program the global variable list is empty, cp_usrvars() is the
   stub below in ngsconvert.c, plot_cur is NULL and ft_curckt is NULL -- so
   that read is FALSE on every link of the chain and there is no way to make
   it TRUE, this program having no way to set a variable.  A converter that
   stamped the mode would be stamping its own default on a file it knows
   nothing about: what it converts is a file, not a run.  What the file said
   survives instead, because raw_read() files the pair into the plot's
   environment and raw_write()'s pl_env loop re-emits it -- so a 'preserve'
   file converts to a 'preserve' file, and one that carried no such line
   still carries none.  The reference has to keep resolving all the same,
   which is why this definition stays. */

#include "ngspice/ngspice.h"
#include "ngspice/cpdefs.h"
#include "ngspice/ftedefs.h"
#include "ngspice/fteext.h"


const char *inp_case_mode_name(void)
{
    return "fold";
}


bool inp_reading_netlist(void)
{
    return FALSE;
}


bool vec_name_eq(const char *v_name, const char *typed)
{
    /* the fold arm of the real one in src/frontend/vectors.c, which is the
       only arm a program that reads no netlist can be in */
    /* case-lint: helper - this call IS vec_name_eq() */
    return cieq(v_name, typed) != 0;
}
