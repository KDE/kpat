#ifndef __FCS_ISA_H
#define __FCS_ISA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "state.h"
#include "fcs.h"

extern void fcs_state_ia_init(freecell_solver_instance_t * instance);
extern fcs_state_with_locations_t * fcs_state_ia_alloc(freecell_solver_instance_t * instance);
extern void fcs_state_ia_release(freecell_solver_instance_t * instance);
extern void fcs_state_ia_finish(freecell_solver_instance_t * instance);

#ifdef __cplusplus
}
#endif

#endif
