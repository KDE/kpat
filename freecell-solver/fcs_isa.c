/* fcs_isa.c - Freecell Solver Indirect State Allocation Routines
   
   Written by Shlomi Fish, 2000
   This file is distributed under the public domain.
*/

#include "config.h"


#include "state.h"
#include "fcs.h"
#include <malloc.h>

#ifdef __osf__
extern void *malloc(size_t);
#endif

void fcs_state_ia_init(freecell_solver_instance_t * instance)
{
    instance->max_num_state_packs = IA_STATE_PACKS_GROW_BY;
    instance->state_packs = (fcs_state_with_locations_t * *)malloc(sizeof(fcs_state_with_locations_t *) * instance->max_num_state_packs);
    instance->num_state_packs = 1;
    instance->state_pack_len = 0x010000 / sizeof(fcs_state_with_locations_t);
    instance->state_packs[0] = (fcs_state_with_locations_t*) malloc(instance->state_pack_len*sizeof(fcs_state_with_locations_t));

    instance->num_states_in_last_pack = 0;
}

fcs_state_with_locations_t * fcs_state_ia_alloc(freecell_solver_instance_t * instance)
{
    if (instance->num_states_in_last_pack == instance->state_pack_len)
    {
        if (instance->num_state_packs == instance->max_num_state_packs)
        {
            instance->max_num_state_packs += IA_STATE_PACKS_GROW_BY;
            instance->state_packs = (fcs_state_with_locations_t * *)realloc(instance->state_packs, sizeof(fcs_state_with_locations_t *) * instance->max_num_state_packs);
        }
        instance->state_packs[instance->num_state_packs] = (fcs_state_with_locations_t*)malloc(instance->state_pack_len * sizeof(fcs_state_with_locations_t));
        instance->num_state_packs++;
        instance->num_states_in_last_pack = 0;
    }
    return &(instance->state_packs[instance->num_state_packs-1][instance->num_states_in_last_pack++]);
}

void fcs_state_ia_release(freecell_solver_instance_t * instance)
{
    instance->num_states_in_last_pack--;
}

void fcs_state_ia_finish(freecell_solver_instance_t * instance)
{
    int a;
    for(a=0;a<instance->num_state_packs;a++)
    {
        free(instance->state_packs[a]);
    }
    free(instance->state_packs);
}

void fcs_state_ia_foreach(freecell_solver_instance_t * instance, void (*ptr_function)(fcs_state_with_locations_t *, void *), void * context)
{
    int p,s;
    for(p=0;p<instance->num_state_packs-1;p++)
    {
        for(s=0 ; s < instance->state_pack_len ; s++)
        {
            ptr_function(&(instance->state_packs[p][s]), context);
        }
    }
    for(s=0; s < instance->num_states_in_last_pack ; s++)
    {
        ptr_function(&(instance->state_packs[p][s]), context);
    }
}
