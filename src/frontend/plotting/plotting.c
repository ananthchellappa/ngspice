#include "ngspice/ngspice.h"

#include "plotting.h"

/* Where 'constants' go when defined on initialization. */

/* Positional, so the trailing run is: pl_written, pl_fromfile,
   pl_lookup_valid, pl_ndims, pl_xdim2d, pl_ydim2d.  One value per member --
   a short list still compiles and zero-fills the tail, which is how a field
   added to struct plot silently takes the next member's value here. */
struct plot constantplot = {
    "Constant values", Spice_Build_Date, "constants",
    "const", NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    TRUE, FALSE, FALSE, 0, 0, 0
};

struct plot *plot_cur = &constantplot;
struct plot *plot_list = &constantplot;

int plotl_changed;      /* TRUE after a load */

int plot_num = 1;
