/* fcs_isa.c - Freecell Solver Indirect State Allocation Routines
   
   Written by Shlomi Fish, 2000
   This file is distributed under the public domain.
*/

#include "config.h"
#if defined(INDIRECT_STATE_STORAGE) || defined(TREE_STATE_STORAGE) || defined(HASH_STATE_STORAGE) || defined(DB_FILE_STATE_STORAGE)


#include "state.h"
#include "fcs.h"
#include <malloc.h>


void fcs_state_ia_init(freecell_solver_instance_t * instance)
{
    instance->max_num_state_packs = IA_STATE_PACKS_GROW_BY;
    instance->state_packs = (fcs_state_with_locations_t * *)malloc(sizeof(fcs_state_with_locations_t *) * instance->max_num_state_packs);
    instance->num_state_packs = 1;
    instance->state_pack_len = 0x010000 / sizeof(fcs_state_with_locations_t);
    instance->state_packs[0] = malloc(instance->state_pack_len*sizeof(fcs_state_with_locations_t));

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
        instance->state_packs[instance->num_state_packs] = malloc(instance->state_pack_len*sizeof(fcs_state_with_locations_t));
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
#endif
