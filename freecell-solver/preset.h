/*
 * fcs.h - header file of the preset management functions for Freecell Solver.
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000
 *
 * This file is in the public domain (it's uncopyrighted).
 */

#ifndef __PRESET_H
#define __PRESET_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fcs.h"

int fcs_apply_preset_by_name(
    freecell_solver_instance_t * instance,
    const char * name
    );

#ifdef __cplusplus
}
#endif

#endif
