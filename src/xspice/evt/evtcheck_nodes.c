/* Automatic insertion of XSPICE devices bridging event and analogue nodes.
 * Giles Atkinson 2022.
 * The contents of this file may belong in src/frontend, except that it is
 * part of XSPICE.
 */

#include <ctype.h>
#include <errno.h>

#include "ngspice/ngspice.h"
#include "ngspice/cktdefs.h"
#include "ngspice/devdefs.h"
#include "ngspice/inpdefs.h"
#include "ngspice/sperror.h"
#include "ngspice/wordlist.h"
#include "ngspice/fteext.h"

#include "ngspice/mif.h"
#include "ngspice/evt.h"
#include "ngspice/evtudn.h"
#include "ngspice/evtproto.h"
#include "ngspice/mif.h"
#include "../../frontend/inpcom.h"
#include "../../frontend/inp.h"
#include "../../frontend/subckt.h"
#include "../../frontend/variable.h"
#include "../../frontend/numparam/numpaif.h"
#include "../../misc/util.h"

/* Automatic insertion of bridge devices.

When using XSPICE to simulate digital devices in a mixed-mode simulation,
bridging devices must be included in the circuit to pass signals between the
analogue simulator and XSPICE.  Such devices may be included in the netlist,
or they can be inserted automatically.  Different types of automatic
bridge may exist in the same circuit, depending on signal direction and
characteristics of the connected device, for example digital devices
powered by differing voltages.  Non-digital XSPICE nodes are supported.

Bridging may be disabled by setting command variable "auto_bridge" to zero.
Values greater than one enable diagnostic outputs.

Automatic insertion works by checking node names; if the same name occurs in
both sides of the simulator, it indicates that a connection between
an analogue and a digital device has been specified.  At this point no such
connection exists: it is created by adding the bridging device.  Devices are
added by generating netlist lines describing them and then performing some
stages of ngspice's normal input processing.

To add a bridging device, one or two lines of netlist text are created.
Typically, one specifies the device itself (an XSPICE 'A' "device card")
and the other ("setup card") is the required model definition.  Alternatively,
the setup card may use .include to read in a subcircuit definition and the
device card is a subcircuit ('X' card).   Setup cards are processed once,
device cards, once per bridged node.

Inserted or included netlist lines use a restricted set of features.  There
may be further includes and subcircuits, but nothing else apart from device
and model cards has been tested.  Parameter and expression processing are
available, but parameters defined in the main circuit are inaccessible.
Models defined in the circuit may be used, but subcircuits are inaccessible.
These restictions arise from the fact that missing bridges are detected
after the circuit has been built.  Previous parameter and subcircuit
definitions are no longer fully available.

The device card must be a format string for the C library function sprintf()
with one %d and two %s insertion indicators.  The %d insertion makes the
device name unique and the string insertions supply a list of node names
that will be bridged.  Optionally a final substitution for a double may be
included that will receive the value of internal variable, vcc (see below).
The results of formatting should be an XSPICE device card or a subcircuit call.

The setup and device cards to be used are determined by examining the
event node that is to be connected.  The rules are a little complex,
but are intended to be very flexible, while easy to use in common
cases.   If all XSPICE nodes are digital, using 3.3V supply and good
simulation of the device's electrical characteristics is not needed,
then nothing need be done.  Other supply voltages can be handled by:

.param vcc=5

or similar, and the parameter may have different values in subcircuits.
Otherwise, the full rules follow.  The third rule is intended to be useful
for device libraries whose devices are defined by subcircuits.

1: The program looks for a command interpreter variable
   "auto_bridge_parm_TTTT" where TTTT is the node type string.  So
   "auto_bridge_parm_d" for digital nodes.  The value should be the name of
   a circuit parameter (.param card).  If not found, ignore the next step,
   except in the case of a digital node where the parameter name is assumed
   to be "vcc". Initialise an internal variable, vcc, to zero.

2: Given that parameter name, find the most deeply-nested (in subcircuits)
   XSPICE device that is connected to the node.  Search upwards through
   the enclosing subcircuits for a definition of the parameter.
   If found, set vcc from the parameter.

3: If a command interpreter variable "no_auto_bridge_family": exists,
   go to step 5.  Search the connected XSPICE device instances for a
   parameter, "family" with a string value.  The first one found will
   be used.  If no such instance exists, search for a string-valued
   parameter, "family", in enclosing subcircuits, as in step 2.  If
   the first character of the value is not '*', the setup card may be
   ".include bridge_FFFFF_TTTT_DDD.subcir" where FFFFF is the family,
   TTTT is as before and DDD is the signal direction for the XSPICE
   device: "in", "out" or "inout".  This form will be used only when
   the required file can be found.  In that case, the device card will
   be "Xauto_bridge%d %s %s bridge_FFFFF_TTTT_DDD vcc=%g", so a
   suitably parameterised subcircuit must be defined in the included
   file.

4: If the first character of "family" was '*', or no file was found,
   look for a variable "auto_bridge_FFFFF_TTTT_DDD" where FFFFF is the
   family without '*', TTTT is the node type string and DDD is the
   direction.  So this might be "auto_bridge_74HCT_d_inout" for a
   digital node.  Use the variable's value as in step 6, proceeding to
   step 5 if checks fail.

5: Look for a variable "auto_bridge_TTTT_DDD" where TTTT and DDD are as before.

6: Check and use the variable's value from step 4 or 5.  It must be a list
   of two or three elements: the setup card, device card and an optional
   third value.  If the setup card is not ".include", it is formatted
   with multiple copies of vcc (example below).  If present, the third
   value in the list must be an integer, specifying the maximum number of
   nodes handled by a single device.  If not present, or zero, there is
   no limit.  If no variable was found, the circuit fails, except for
   digital nodes where defaults are supplied.  For digital the defaults
   are:

   ( ".model auto_adc adc_bridge(in_low = '%g/2' in_high = '%g/2')"
     "auto_adc%d [ %s ] [ %s ] auto_adc" )

   for a digital input with

   ( ".model auto_dac dac_bridge(out_low = 0 out_high = %g)"
     "auto_dac%d [ %s ] [ %s ] auto_dac" )

  for digital output and

   ( ".model auto_bidi bidi_bridge(out_high=%g in_low='%g/2' in_high='%g/2')"
     "auto_bidi%d [ %s ] [ %s ] null auto_bidi" )

  for a node with a digital inout port or with both inputs and outputs.
  Note that single quotes surround expressions to be evaluated during
  netlist parsing.  They are preferred to braces because braces are stripped
  by the "set" command.

  A non-digital example (real to analogue) is:

   set auto_bridge_real_out = ( ".model real_to_v_bridge r_to_v"
   + "areal_bridge%d %s null %s real_to_v_bridge" 1 )

   (The spaces are important.)

  An example of an included subcircuit might be:

  * Subcircuit for digital output buffer with impedance

  .subckt auto_buf dig ana vcc=5
  .model auto_dac dac_bridge(out_low = 0 out_high = {vcc})
  auto_dac [ dig ] [ internal ] auto_dac
  rint internal ana 100
  .ends

   and invoked by:

   set auto_bridge_d_out = ( ".include test_sub.subcir"
   +                         "xauto_buf%d %s %s auto_buf vcc=%g"
   +                         1 )
*/

/* Working information about a type of bridge. */

struct bridge {
    int             udn_index;          // Node type.
    Mif_Dir_t       direction;          // IN, OUT or INOUT
    double          vcc;                // A .param value used for matching.
    const char     *family;             // Alternative match parameter.
    struct bridge  *next;               // Linked list
    int             max, count;         // How many nodes/device?
    char           *setup;              // For model or inclusion card.
    const char     *format;             // For device card.
    int             end_index;          // Free buffer space
    char            held[256];          // Buffer with node names.
};

/* Enable/verbosity level */

#define AB_OFF    0
#define AB_SILENT 1
#define AB_DECK   2 // Show generated deck

#define BIG 999990000 // For fake linenumbers

/* Expand .include cards and subcircuits in a deck. */

static struct card *expand_deck(struct card *head)
{
    struct card  *card, *next;
    char        **pointers;
    int           i, dico;
    bool          save_debug;

    /* Save the current parameter symbol table and debug global.
     * Prevent overwriting of debug output in inp_readall().
     */

    dico = nupa_add_dicoslist();
    save_debug = ft_ngdebug;
    ft_ngdebug = FALSE;

    /* Count the cards, allocate and fill a pointer array. */

    for (i = 0, card = head; card; ++i, card = card->nextcard)
        ;
    pointers = TMALLOC(char *, i + 1);
    for (i = 0, card = head; card; ++i, card = card->nextcard)
        pointers[i] = card->line;
    pointers[i] = NULL;

    /* Free the card headers. */

    for (card = head; card; card = next) {
        next = card->nextcard;
        tfree(card);
    }

    /* The cards are passed to _inp_readall() via a global. */

    circarray = pointers;
    card = inp_readall(NULL, Infile_Path, "autobridge", FALSE, TRUE, NULL);
    card = inp_subcktexpand(card);
    ft_ngdebug = save_debug;

    /* Destroy the parameter table that was created in subcircuit/parameter
     * expansion and restore the previous version.
     */

    nupa_del_dicoS();
    nupa_set_dicoslist(dico);
    nupa_rem_dicoslist(dico);
    return card;
}

/* Write a completed bridge instance card. */

static struct card *flush_card(struct bridge *bridge, int ln,
                               struct card *last)
{
    char buff[2 * sizeof bridge->held + 128];

    bridge->held[bridge->end_index] = '\0';
    snprintf(buff, sizeof buff, bridge->format,
             ln, bridge->held, bridge->held, bridge->vcc);
    bridge->count = 0;
    bridge->end_index = 0;
    return insert_new_line(last, copy(buff), BIG + ln, last->linenum_orig, last->linesource);
}


/* Release memory used. */

static void free_bridges(struct bridge *bridge_list)
{
    struct bridge *bridge;

    while (bridge_list) {
        /* Free the structure.  Setup has gone. */

        bridge = bridge_list;
        bridge_list = bridge->next;
        if (bridge->format)
            tfree(bridge->format);
        if (bridge->family)
            tfree(bridge->family); // find_bridge() gave us a folded copy.
        tfree(bridge);
    }
}

/* Find an XSPICE device instance given its index number.  Uses link chasing
 * as at this point the indexable inst_table does not yet exist.
 */

static MIFinstance *find_inst(CKTcircuit *ckt, int index)
{
    struct Evt_Inst_Info *chase;
    int                   i;

    for (i = 0, chase = ckt->evt->info.inst_list;
         i < index && chase;
         ++i, chase = chase->next)
        ;
    return chase ? chase->inst_ptr : NULL;
}

/* All connected ports are declared as outputs, but some may be INOUT. */

static Mif_Dir_t scan_ports(Evt_Node_Info_t *event_node, CKTcircuit *ckt)
{
    Evt_Node_Info_t   *chase;
    Evt_Inst_Index_t  *inst_list;
    int                index;

    /* Find the node index. */

    for (index = 0, chase = ckt->evt->info.node_list;
         chase && chase != event_node;
         ++index, chase= chase->next);

    /* Look for inout ports connected to this node. */

    for (inst_list = event_node->inst_list;
         inst_list;
         inst_list = inst_list->next) {
        MIFinstance     *inst;
        int              i;

        inst = find_inst(ckt, inst_list->index);
        for (i = 0; i < inst->num_conn; ++i) {
            Mif_Conn_Data_t *conn;
            int              j;

            conn = inst->conn[i];
            if (conn->is_null || !conn->is_input || !conn->is_output)
                continue;
            for (j = 0; j < conn->size; ++j) {
                Mif_Port_Data_t  *port;

                port = conn->port[j];
                if (port->type != MIF_DIGITAL &&
                    port->type != MIF_USER_DEFINED) {
                    /* Analogue, ignore. */

                    continue;
                }
                if (port->evt_data.node_index == index) {
                    /* An inout connection to this node. */

                    return MIF_INOUT;
                }
            }
        }
    }
    return MIF_OUT;
}

/* Examine a device and return its subcircuit nesting depth
 * and possibly the value of its "family" parameter, if that exists.
 */

static int examine_device(MIFinstance *inst, const char **family)
{
    struct MIFmodel *mp;
    char            *dot;
    int              i;

     if (!*family) {
         Mif_Param_Data_t *pdp;
         IFparm           *pp;
         int               type;

         mp = MIFmodPtr(inst);
         type = mp->MIFmodType;
         pp = ft_sim->devices[type]->modelParms;
         for (i = 0; i < inst->num_param; ++i) {
             pdp = mp->param[i];
             if (!pdp->is_null && pdp->eltype == IF_STRING &&
                 !strcmp(pp[i].keyword, "family")) {
                 /* Return value for "family" parameter. */

                 *family = pdp->element->svalue; // May be NULL
                 if (*family && !(*family)[0])
                     *family = NULL;    // Ignore empty string.
                 break;
             }
         }
     }
 
     for (i = 0, dot = strchr(inst->MIFname, '.'); dot; dot += 1, ++i) {
         dot = strchr(dot, '.');
         if (dot == NULL)
             break;
     }
     return i;
}

/* Scan devices attached to node.  Return the name of the deepest-nested
 * one and the first "Family" parameter found.
 */

static const char *scan_devices(Evt_Node_Info_t   *event_node,
                                CKTcircuit        *ckt,
                                const char       **family)
{
    Evt_Inst_Index_t   *inst_list;
    MIFinstance        *best_inst = NULL, *inst;
    int                 best_depth = -1, depth, left;

    /* Scan input connections. */

    for (inst_list = event_node->inst_list;
         inst_list;
         inst_list = inst_list->next) {
        inst = find_inst(ckt, inst_list->index);
        depth = examine_device(inst, family);
        if (depth > best_depth) {
            best_depth = depth;
            best_inst = inst;
        }
    }

    left = event_node->num_outputs;
    if (left) {
        Evt_Node_Info_t    *node;
        Evt_Output_Info_t  *oip;
        int                 i, my_index;

        /* Find the index of this node (not stored). */

        for (my_index = 0, node = ckt->evt->info.node_list;
             node != event_node;
             ++my_index, node = node->next)
            ;

        /* Scan output connections. */

        for (i = 0, oip = ckt->evt->info.output_list;
             oip;
             ++i, oip = oip->next) {
            if (oip->node_index == my_index) {
                inst = find_inst(ckt, oip->inst_index);
                depth = examine_device(inst, family);
                if (depth > best_depth) {
                    best_depth = depth;
                    best_inst = inst;
                }
                if (--left == 0)
                    break;
            }
        }
    }

    /* The device name is a.<subcircuit name>.<original device name> */

    return best_inst ? best_inst->MIFname + 2 : NULL;
}

/* Fold the leading device letter of a subcircuit instance path.
 *
 * The path a scoped parameter is stored under has one character forced to
 * lower case when the X card is evaluated: src/frontend/numparam/spicenum.c
 * :721 is `*inst_name = 'x'`, applied to the *first* character of the name it
 * builds, because a device letter is a keyword and is case insensitive in
 * every mode.  The path find_bridge() probes with is built from the MIF
 * instance name, which keeps whatever the deck wrote, so a deck writing "X1"
 * asked numparam for "X1.vcc" while the table held "x1.VCC".
 *
 * Fold exactly the same one character and no more.  Folding the letter of
 * every dot-separated component instead would be a different rule from the
 * one numparam applies, and for a nested instance it breaks a lookup that
 * worked: numparam stores "x1.XIn.vcc" - only index 0 folded - so a probe
 * folded per component asks for "x1.xIn.vcc" and misses.
 *
 * Under fold the reader had lower cased the card and under preserve
 * symbol_key() folds both sides, so the two spellings met anyway; this only
 * moves distinguish.
 */

static void fold_path_device_letter(char *path)
{
    if (*path)
        *path = tolower_c(*path);
}

/* Does this spelling of a family name select a bridge?
 *
 * The two things a family name can name are a subcircuit file resolved by
 * inp_pathresolve() and a command interpreter variable matched by cp_getvar()
 * with strcmp.  Both are byte exact in every casemode, which is why
 * find_bridge() has to choose a spelling before it uses one; this answers the
 * question it chooses on.  A leading '*' means "variable look-up only" and is
 * not part of the name.
 */

static bool family_is_known(const char *family, const char *type_name,
                            const char *dir)
{
    struct variable *cvar;
    char             buff[256];

    if (*family == '*') {
        family++;
    } else {
        char *path;

        snprintf(buff, sizeof buff, "bridge_%s_%s_%s.subcir",
                 family, type_name, dir);
        path = inp_pathresolve(buff);
        if (path) {
            tfree(path);
            return TRUE;
        }
    }
    snprintf(buff, sizeof buff, "auto_bridge_%s_%s_%s",
             family, type_name, dir);
    return cp_getvar(buff, CP_LIST, &cvar, sizeof cvar) ? TRUE : FALSE;
}

/* Can a bridge element be inserted? */

static struct bridge *find_bridge(Evt_Node_Info_t  *event_node,
                                  CKTcircuit       *ckt,
                                  struct bridge   **bridge_list_p)
{
    static const char * const   dirs[] = {"in", "out", "inout"};
    struct bridge              *bridge;
    Mif_Dir_t                   direction;
    char                       *setup = NULL;
    const char                 *format = NULL;
    const char                 *type_name, *family, *s_family, *deep;
    char                       *vcc_parm, *dot;
    double                      vcc = 0.0;
    int                         max = 0, ok = 0;
    bool                        our_vcc_parm;
    struct variable            *cvar = NULL;
    char                        buff[256];

    /* Find the direction for this node. */

    if (event_node->num_outputs == 0) {
        direction = MIF_IN;
    } else if (event_node->num_outputs  < event_node->num_ports) {
        direction = MIF_INOUT;
    } else {
        direction = scan_ports(event_node, ckt); // Ugly
    }

    /* Find the vcc parameter for this node. */

    /* our_vcc_parm records which of the two the name is.  "vcc" is ngspice's
     * own spelling and is looked up tolerantly below; a name that came out of
     * the auto_bridge_parm_<type> variable was typed by the user, so under
     * distinguish it must match the .param exactly, the way every other name
     * typed at the control language now does (decision 5 of
     * doc/claude/decisions/0001-distinguish.md).
     */

    type_name = g_evt_udn_info[event_node->udn_index]->name;
    snprintf(buff, sizeof buff, "auto_bridge_parm_%s", type_name);
    our_vcc_parm = FALSE;
    if (cp_getvar(buff, CP_STRING, buff, sizeof buff)) {
        vcc_parm = buff;
    } else if (event_node->udn_index == 0) {
        vcc_parm = "vcc";
        our_vcc_parm = TRUE;
    } else {
        vcc_parm = NULL;
    }

    /* Scan attached XSPICE devices for the deepest nested one and
     * a device with a "family" parameter.
     */

    family = NULL;
    deep = scan_devices(event_node, ckt, &family);

    /* Look for a real parameter (.param type) and perhaps a string-valued
     * "family" parameter in the device's subcircuit and those enclosing it.
     */

    snprintf(buff, sizeof buff, "%s", deep);
    fold_path_device_letter(buff);
    dot = strrchr(buff, '.');
    while (dot) {
        if (!ok) {
            snprintf(dot + 1, sizeof buff - (size_t)(dot - buff) - 1, "%s",
                     vcc_parm);
            vcc = our_vcc_parm ? nupa_get_constructed_param(buff, &ok)
                               : nupa_get_param(buff, &ok);
        }
        if (!family) {
            snprintf(dot + 1, sizeof buff - (size_t)(dot - buff) - 1,
                     "family");
            family = nupa_get_constructed_string_param(buff);
        }
        if (ok && family)
            break;
        *dot = '\0';
        dot = strrchr(buff, '.');
    }

    if (!ok) {
        if (vcc_parm)
            vcc = our_vcc_parm ? nupa_get_constructed_param(vcc_parm, &ok)
                               : nupa_get_param(vcc_parm, &ok);
        if (!ok) {
            if (event_node->udn_index == 0)
                vcc = 3.3; // Fallback default for digital.
            else
                vcc = 0;
        }
    }

    if (!family)
        family = nupa_get_constructed_string_param("family");
    if (family && cp_getvar("no_auto_bridge_family", CP_BOOL, NULL, 0))
        family = NULL;

    /* "family" is a parameter *value*, not an identifier, and everything
     * below turns it into a name that lives outside the deck: a file name
     * resolved by inp_pathresolve(), a command interpreter variable name that
     * cp_getvar() matches with strcmp in every mode, a .include card and a
     * subcircuit name.  Both consumers are byte exact whatever the casemode,
     * so a deck writing family="74HCT" found no bridge_74HCT_... file under
     * 'preserve' or 'distinguish' and silently took the built-in default
     * bridge, while under 'fold' the reader had lower cased the card and the
     * same deck worked.
     *
     * Choose the spelling once, here, where the three places family can come
     * from have converged - a model card's "family" parameter, a scoped
     * .param and a global .param - rather than gating the five snprintf()s
     * below.  That is the "Fold sites outside the reader" rule of
     * doc/claude/specs/case-sensitive-identifiers.md, at the convergence
     * point rather than at the examine_device() capture the spec names,
     * because the two .param sources do not pass through that function.
     *
     * The deck's own spelling wins if it names something that exists, and the
     * folded spelling is used otherwise.  Folding unconditionally would be
     * simpler and is wrong in both shipped modes: a deck whose family bridge
     * file or variable is named with the case the deck wrote - which is the
     * only naming that worked under 'preserve' before this - would stop
     * selecting it, silently.  It is not even a no-op under 'fold', because a
     * quoted value on a .model card survives the reader's fold whenever
     * is_xspice_model() matches the card, which is a substring test over the
     * whole line and fires on a trailing comment (doc/codex/issues/0005).
     * Trying the deck's spelling first is a superset of both: nothing that
     * resolved before stops resolving, and the family bridge that only 'fold'
     * could reach is now reachable in every mode.
     *
     * From here on the string is ours.  It is handed to the new bridge below
     * and released by free_bridges(); the two early returns free it.
     */

    if (family) {
        char *folded = copy(family);

        strtolower(folded);
        if (strcmp(folded, family) != 0 &&
            family_is_known(family, type_name, dirs[direction])) {
            tfree(folded);
            family = copy(family);      // The deck's spelling names a bridge.
        } else {
            family = folded;
        }
    }

    if (family) {
        if (*family == '*') {
            s_family = family + 1; // Use variable look-up.
        } else {
            char *fam_inc_path;

            /* Check if an include file exists for the family. */

            snprintf(buff, sizeof buff, "bridge_%s_%s_%s.subcir",
                     family, type_name, dirs[direction]);
            fam_inc_path = inp_pathresolve(buff);
            if (fam_inc_path) {
                tfree(fam_inc_path);
                s_family = NULL;
            } else {
                s_family = family; // Use variable look-up.
            }
        }
    } else {
        s_family = NULL;
    }

    /* Look for an existing entry. */

    for (bridge = *bridge_list_p; bridge; bridge = bridge->next) {
        if (bridge->udn_index == event_node->udn_index &&
            bridge->direction == direction) {
            if (family) {
                /* bridge->family is NULL for a bridge built for a node that
                 * named no family, and such a bridge can precede this one on
                 * the list with the same udn_index and direction.  It is not
                 * a match for a node that does name a family.
                 */

                if (bridge->family && !strcmp(family, bridge->family)) {
                    if (!s_family && bridge->max == 1) {
                        /* Set bridge vcc for formatting. */

                        bridge->vcc = vcc;
                        break;
                    } else {
                        /* Using cards from variable, or shared sub-circuit:
                         * vcc must also match.
                         */

                        if (bridge->vcc == vcc)
                            break;
                    }
                }
            } else if (bridge->vcc == vcc) {            // Match vcc.
                break;
            }
        }
    }
    if (bridge) {
        if (family)
            tfree(family); // The matched bridge has its own copy.
        return bridge;
    }

    /* Determine if a bridging element exists, starting with the node type. */

    if (s_family) {
        /* Family variable lookup. */

        snprintf(buff, sizeof buff, "auto_bridge_%s_%s_%s",
                 s_family, type_name, dirs[direction]);
        cp_getvar(buff, CP_LIST, &cvar, sizeof cvar);
    } else if (family) {
        /* Use standard pattern for known parts family. */

        snprintf(buff, sizeof buff, ".include bridge_%s_%s_%s.subcir",
                 family, type_name, dirs[direction]);
        setup = copy(buff);
        snprintf(buff, sizeof buff,
                 "Xauto_bridge%%d %%s %%s bridge_%s_%s_%s vcc=%%g",
                 family, type_name, dirs[direction]);
        format = copy(buff);
        max = 1;
    }

    if (!format && !cvar) {
        /* General variable lookup. */

        snprintf(buff, sizeof buff, "auto_bridge_%s_%s",
                 type_name, dirs[direction]);
        cp_getvar(buff, CP_LIST, &cvar, sizeof cvar);
    }
    if (!format && cvar) {
        struct variable *v2, *v3;

        /* The first element of the list is returned. */

        if (cvar && cvar->va_type == CP_STRING &&
            cvar->va_string && cvar->va_string[0]) {
            v2 = cvar->va_next;
            if (v2->va_type == CP_STRING &&
                v2->va_string && v2->va_string[0]) {
                format = copy(v2->va_string);
                setup = copy(cvar->va_string);

                v3 = v2->va_next;
                if (v3 && v3->va_type == CP_NUM)
                    max = v3->va_num;
            } else {
                cvar = NULL;
            }
        } else {
                cvar = NULL;
        }
    }

    /* Last and probably most common case, default digital bridges. */

    if (!format && event_node->udn_index == 0) {
        if (direction == MIF_IN) {
            setup = ".model auto_adc adc_bridge("
                    "in_low = '%g/2' in_high = '%g/2')";
            format = "auto_adc%d [ %s ] [ %s ] auto_adc";
        } else if (direction == MIF_OUT) {    // MIF_OUT
            setup = ".model auto_dac dac_bridge("
                    "out_low = 0 out_high = %g)";
            format = "auto_dac%d [ %s ] [ %s ] auto_dac";
        } else {
            setup = ".model auto_bidi bidi_bridge("
                    "out_high = %g in_low = '%g/2' in_high = '%g/2')";
            format = "auto_bidi%d [ %s ] [ %s ] null auto_bidi";
        }
        setup = copy(setup);
        format = copy(format);
    }
    if (!format) {
        if (family)
            tfree(family);
        return NULL;
    }

    /* If the setup is not a .include card, format it with vcc. */

    if (!cieqn(setup, ".inc", 4)) {
        snprintf(buff, sizeof buff, setup, vcc, vcc, vcc, vcc, vcc);
        tfree(setup);
        setup = copy(buff);
    }

    /* Make a new bridge structure. */

    bridge = TMALLOC(struct bridge, 1);
    bridge->udn_index = event_node->udn_index;
    bridge->direction = direction;
    bridge->vcc = vcc;
    bridge->family = family;
    bridge->next = *bridge_list_p;
    *bridge_list_p = bridge;
    bridge->max = max;
    bridge->count = 0;
    bridge->setup = setup;
    bridge->format = format;
    bridge->end_index = 0;
    return bridge;
}

/* True if the deck already joins this event node to the analog node with the
 * given equation number: some XSPICE instance has a port on both, which is
 * exactly the connection the auto-bridge would have made, made by hand.
 *
 * Under distinguish the two sides of a hand-written bridge are two names, so
 * writing them as one name in two cases - event 'DOUT' against analog 'dout'
 * - is a natural way to spell that device, and it is correct.  Without this
 * test the report below fires on it, and a warning on the legitimate deck is
 * what decision 2 of doc/claude/decisions/0001-distinguish.md rejects: it
 * trains users to ignore the warning that matters.
 *
 * The event side is matched by node index, the way scan_ports() does it, and
 * the analog side by CKTnode number rather than by name, so no comparison of
 * spellings enters the test.  A port that names no analog node has
 * smp_data.pos_node and .neg_node zero from tmalloc()'s calloc, and node
 * number zero is ground, which has no case variant and so is never the node
 * being asked about.
 */

static bool already_joined(
    CKTcircuit *ckt,              /* The circuit structure */
    int         node_index,       /* Index of the event node in node_list */
    int         analog_number)    /* Equation number of the analog node */
{
    Evt_Inst_Info_t *inst_info;

    for (inst_info = ckt->evt->info.inst_list;
         inst_info;
         inst_info = inst_info->next) {
        MIFinstance *inst;
        bool         on_event = FALSE, on_analog = FALSE;
        int          i, j;

        inst = inst_info->inst_ptr;
        for (i = 0; i < inst->num_conn; ++i) {
            Mif_Conn_Data_t *conn;

            conn = inst->conn[i];
            if (conn->is_null)
                continue;
            for (j = 0; j < conn->size; ++j) {
                Mif_Port_Data_t *port;

                port = conn->port[j];
                if (port->is_null)
                    continue;
                if (port->type == MIF_DIGITAL ||
                    port->type == MIF_USER_DEFINED) {
                    if (port->evt_data.node_index == node_index)
                        on_event = TRUE;
                } else if (port->smp_data.pos_node == analog_number ||
                           port->smp_data.neg_node == analog_number) {
                    on_analog = TRUE;
                }
            }
        }
        if (on_event && on_analog)
            return TRUE;
    }
    return FALSE;
}

/* Report an event node that matched no analog node exactly but does match one
 * when case is ignored, and that the deck has not joined to it by hand.  That
 * is decision 2 of
 * doc/claude/decisions/0001-distinguish.md at this site: silence when a name
 * is being defined, a warning when a name is being resolved, the resolution
 * fails, and a name differing only in case exists.  The bridging loop below
 * is the resolution - an event node and an analog node of the same name are
 * one mixed-type net - so a near miss there leaves the analog net driven by
 * nothing and the run prints a plausible number.
 *
 * Called once per event node, after that node's scan of CKTnodes has
 * finished, rather than at the comparison that failed.  A single failed
 * comparison is not a failed resolution: the loop compares one event node
 * against every analog node, so 'A' can miss 'a' and still match 'A' further
 * down the list, and reporting inside the loop would warn about a net that
 * was bridged and would warn once per analog node besides.  A separate pass
 * over the whole node list after the bridging loop, which
 * doc/codex/issues/0030 suggested, buys nothing: the loop mutates neither
 * CKTnodes nor the event node list - it only accumulates cards, which are
 * parsed after it - so the question is already decidable here.
 *
 * Only distinguish can reach the condition.  Under preserve ng_ideq() is
 * case insensitive, so a name that matches when case is ignored has already
 * matched and been bridged; under fold the reader has lowercased every card,
 * so the two spellings are one.  The guard is therefore on the mode and not
 * on the spelling.
 */

static void report_bridge_case_miss(
    CKTcircuit      *ckt,             /* The circuit structure */
    Evt_Node_Info_t *event_node)      /* The event node that matched nothing */
{
    Evt_Node_Info_t *chase;
    CKTnode         *analog_node;
    int              node_index;

    if (inp_case_mode() != NG_CASE_DISTINGUISH)
        return;

    /* Find the node index, as scan_ports() does. */

    for (node_index = 0, chase = ckt->evt->info.node_list;
         chase && chase != event_node;
         ++node_index, chase = chase->next)
        ;

    for (analog_node = ckt->CKTnodes;
         analog_node;
         analog_node = analog_node->next) {
        if (cieq(event_node->name, analog_node->name)) {
            if (already_joined(ckt, node_index, analog_node->number))
                return;
            fprintf(cp_err,
                    "Warning: no analog node named '%s'; '%s' differs only in case (casemode=distinguish)\n",
                    event_node->name, analog_node->name);
            return;
        }
    }
}

/* Early detection of node type clashes and attempted fix by
 * automatic insertion of a bridging device.
 */

bool Evtcheck_nodes(
    CKTcircuit  *ckt,             /* The circuit structure */
    INPtables   *stab)            /* Symbol table. */
{
    struct bridge       *bridge_list = NULL, *bridge;
    CKTnode             *analog_node;
    Evt_Node_Info_t     *event_node;
    struct card         *head = NULL, *lastcard = NULL;
    int                  ln = 0, show;

    /* Is auto-bridge enabled? */

    if (!cp_getvar("auto_bridge", CP_NUM, &show, sizeof show))
        show = AB_SILENT;

    /* Check for incompatible command '.probe alli' */

    if (ckt->evt->info.node_list && cp_getvar("probe_alli_given", CP_BOOL, NULL, 0)) {
        fprintf(stderr,
            "\nError: Dot command '.probe alli' and "
            "digital nodes are not compatible.\n");
        fprintf(stderr, "    Simulation will fail!\n\n");
        controlled_exit(EXIT_FAILURE);
    }

    /* Try to create joining device if any analog node name matches
     * an event node. Failure is fatal.
     *
     * Both names were written by the deck - the event side is an A card's
     * port token, the analog side the interned spelling of a device card's
     * node - so the match is an identity test and takes ng_ideq(): under
     * 'preserve' two spellings are one identifier, so digital A and analog a
     * are one mixed-type net and must be bridged, while 'fold' and
     * 'distinguish' keep the byte-exact comparison.
     * doc/claude/decisions/0001-distinguish.md decision 3, Class A.
     */

    for (event_node = ckt->evt->info.node_list;
         event_node;
         event_node = event_node->next) {
         bool matched = FALSE;

         for (analog_node = ckt->CKTnodes;
             analog_node;
             analog_node = analog_node->next) {
             int nl;
             if (ng_ideq(event_node->name, analog_node->name)) {
                 matched = TRUE;
                 if (show == AB_OFF) {
                     FREE(errMsg);
                     errMsg = tprintf("Auto bridging is switched off "
                                      "but node %s is mixed-type.\n",
                                      event_node->name);
                     return FALSE;      // Auto-bridge disabled
                 }
                 bridge = find_bridge(event_node, ckt, &bridge_list);
                 if (!bridge) {
                    /* Fatal, circuit cannot run. */

                    errMsg = tprintf("Can not insert bridge for mixed-type "
                                     "node %s\n",
                                     analog_node->name);
                    free_bridges(bridge_list);
                    if (head)
                        line_free(head, TRUE);
                    return FALSE;
                 }

                 if (!head) {
                     /* Add a title card, so it can be skipped over. */

                     head = insert_new_line(lastcard,
                                            copy("* Auto-bridge sub-deck."),
                                            BIG + ln++, (lastcard ? lastcard->linenum_orig : 0),
                                            (lastcard ? lastcard->linesource : NULL));
                     lastcard = head;
                 }

                 if (bridge->setup) {
                     /* Model/include card for bridge.
                      * bridge->setup must be dynamic (for tfree()).
                      */

                     lastcard = insert_new_line(lastcard, bridge->setup,
                                                BIG + ln++, (lastcard ? lastcard->linenum_orig : 0),
                                                (lastcard ? lastcard->linesource : NULL));
                     bridge->setup = NULL;       // Output just once.
                 }

                 /* Card buffer full? */

                 nl = (int)strlen(analog_node->name);
                 if ((bridge->max && bridge->count >= bridge->max) ||
                     bridge->end_index + nl + 2 > (int)(sizeof bridge->held)) {
                     /* Buffer full, flush card. */

                     lastcard = flush_card(bridge, ln++, lastcard);
                 }

                 /* Copy node name to card contents buffer. */

                 bridge->count++;
                 if (bridge->end_index)
                     bridge->held[bridge->end_index++] = ' ';
                 strcpy(bridge->held + bridge->end_index, analog_node->name);
                 bridge->end_index += nl;
             }
         }

         /* This node's resolution against the analog side is now decided. */

         if (!matched)
             report_bridge_case_miss(ckt, event_node);
    }

    /* Flush cards. */

    for (bridge = bridge_list; bridge; bridge = bridge->next) {
        if (bridge->end_index > 0) 
            lastcard = flush_card(bridge, ln++, lastcard);
    }

    if (!head)
        return TRUE;  // No success, but also no failures - nothing to bridge.

    if (show >= AB_DECK) {
        struct card *card;

        for (card = head; card; card = card->nextcard)
            printf("%d: %s\n", card->linenum, card->line);
    }

    /* Expand .include cards and expressions. */

    head = expand_deck(head);
    if (!head) {
        free_bridges(bridge_list);      // Including each bridge's family.
        return FALSE;
    }

    /* Push the cards into the circuit. */

    INPpas1(ckt, head, stab);
    INPpas2(ckt, head, stab, ft_curckt->ci_defTask);

    /* Store them so that they show in "listing e". */

    ft_curckt->ci_auto = head;

    free_bridges(bridge_list);
    return TRUE;
}
