/*
 * preset.c - game presets management for Freecell Solver
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000
 *
 * This file is in the public domain (it's uncopyrighted).
 * 
 */

#include "fcs.h"

#include <string.h>
#include "preset.h"

struct fcs_preset_struct
{
    int preset_id;
    int freecells_num;
    int stacks_num;
    int decks_num;

    int sequences_are_built_by;
    int unlimited_sequence_move;
    int empty_stacks_fill;
};

typedef struct fcs_preset_struct fcs_preset_t;

enum fcs_presets_ids 
{
    FCS_PRESET_BAKERS_DOZEN,
    FCS_PRESET_BAKERS_GAME,
    FCS_PRESET_CRUEL,
    FCS_PRESET_DER_KATZENSCHWANTZ,
    FCS_PRESET_DIE_SCHALANGE, 
    FCS_PRESET_EIGHT_OFF,
    FCS_PRESET_FORECELL, 
    FCS_PRESET_FREECELL,
    FCS_PRESET_GOOD_MEASURE,
    FCS_PRESET_KINGS_ONLY_BAKERS_GAME,
    FCS_PRESET_RELAXED_FREECELL,
    FCS_PRESET_RELAXED_SEAHAVEN_TOWERS,
    FCS_PRESET_SEAHAVEN_TOWERS
};

static fcs_preset_t fcs_presets[13] =
{
    {
        FCS_PRESET_BAKERS_DOZEN,
        0,
        13,
        1,

        FCS_SEQ_BUILT_BY_RANK,
        0,
        FCS_ES_FILLED_BY_NONE,
    },
    {
        FCS_PRESET_BAKERS_GAME,
        4,
        8,
        1,

        FCS_SEQ_BUILT_BY_SUIT,
        0,
        FCS_ES_FILLED_BY_ANY_CARD,
    },
    {
        FCS_PRESET_CRUEL,
        0,
        12,
        1,

        FCS_SEQ_BUILT_BY_SUIT,
        0,
        FCS_ES_FILLED_BY_NONE,
    },
    {
        FCS_PRESET_DER_KATZENSCHWANTZ,
        8,
        9,
        2,

        FCS_SEQ_BUILT_BY_ALTERNATE_COLOR,
        1,
        FCS_ES_FILLED_BY_NONE,
    },
    {
        FCS_PRESET_DIE_SCHALANGE,
        8,
        9,
        2,

        FCS_SEQ_BUILT_BY_ALTERNATE_COLOR,
        0,
        FCS_ES_FILLED_BY_NONE,
    },    
    {
        FCS_PRESET_EIGHT_OFF,
        8,
        8,
        1,

        FCS_SEQ_BUILT_BY_SUIT,
        0,
        FCS_ES_FILLED_BY_KINGS_ONLY
    },    
    {
        FCS_PRESET_FORECELL,
        4,
        8,
        1,

        FCS_SEQ_BUILT_BY_ALTERNATE_COLOR,
        0,
        FCS_ES_FILLED_BY_KINGS_ONLY        
    },
    {
        FCS_PRESET_FREECELL,
        4,
        8,
        1,

        FCS_SEQ_BUILT_BY_ALTERNATE_COLOR,
        0,
        FCS_ES_FILLED_BY_ANY_CARD
    },
    {
        FCS_PRESET_GOOD_MEASURE,
        0,
        10,
        1,

        FCS_SEQ_BUILT_BY_RANK,
        0,
        FCS_ES_FILLED_BY_NONE,
    },    
    {
        FCS_PRESET_KINGS_ONLY_BAKERS_GAME,
        4,
        8,
        1,

        FCS_SEQ_BUILT_BY_SUIT,
        0,
        FCS_ES_FILLED_BY_KINGS_ONLY,
    },
    {
        FCS_PRESET_RELAXED_FREECELL,
        4,
        8,
        1,

        FCS_SEQ_BUILT_BY_ALTERNATE_COLOR,
        1,
        FCS_ES_FILLED_BY_ANY_CARD,
    },
    {
        FCS_PRESET_RELAXED_SEAHAVEN_TOWERS,
        4,
        10,
        1,

        FCS_SEQ_BUILT_BY_SUIT,
        1,
        FCS_ES_FILLED_BY_KINGS_ONLY,
    },
    {
        FCS_PRESET_SEAHAVEN_TOWERS,
        4,
        10,
        1,

        FCS_SEQ_BUILT_BY_SUIT,
        0,
        FCS_ES_FILLED_BY_KINGS_ONLY,
    },
};

struct fcs_preset_name_struct
{
    char name[32];
    int preset_id;
};

typedef struct fcs_preset_name_struct fcs_preset_name_t;

static fcs_preset_name_t fcs_preset_names[17] =
{
    {
        "bakers_dozen",
        FCS_PRESET_BAKERS_DOZEN,
    },    
    {
        "bakers_game",
        FCS_PRESET_BAKERS_GAME,
    },
    {
        "cruel",
        FCS_PRESET_CRUEL,
    },
    {
        "der_katzenschwantz",
        FCS_PRESET_DER_KATZENSCHWANTZ,
    },
    {
        "der_katz",
        FCS_PRESET_DER_KATZENSCHWANTZ,
    },
    {
        "die_schlange",
        FCS_PRESET_DIE_SCHALANGE,
    },
    {
        "eight_off",
        FCS_PRESET_EIGHT_OFF,
    },
    {
        "forecell",
        FCS_PRESET_FORECELL,
    },    
    {
        "freecell",
        FCS_PRESET_FREECELL,
    },
    {
        "good_measure",
        FCS_PRESET_GOOD_MEASURE,
    },
    {
        "ko_bakers_game",
        FCS_PRESET_KINGS_ONLY_BAKERS_GAME,
    },
    {
        "kings_only_bakers_game",
        FCS_PRESET_KINGS_ONLY_BAKERS_GAME,
    },
    {
        "relaxed_freecell",
        FCS_PRESET_RELAXED_FREECELL,
    },
    {
        "relaxed_seahaven_towers",
        FCS_PRESET_RELAXED_SEAHAVEN_TOWERS,
    },
    {
        "relaxed_seahaven",
        FCS_PRESET_RELAXED_SEAHAVEN_TOWERS,
    },
    {
        "seahaven_towers",
        FCS_PRESET_SEAHAVEN_TOWERS,
    },
    {
        "seahaven",
        FCS_PRESET_SEAHAVEN_TOWERS,
    },
};

static int fcs_get_preset_id_by_name(
    char * name
)
{
    unsigned int a;
    int ret = -1;
    for(a=0;a<(sizeof(fcs_preset_names)/sizeof(fcs_preset_names[0]));a++)
    {
        if (!strcmp(name, fcs_preset_names[a].name))
        {
            ret = fcs_preset_names[a].preset_id;
            break;
        }
    }
    
    return ret;
}

enum fcs_presets_return_codes
{
    FCS_PRESET_CODE_OK,
    FCS_PRESET_CODE_NOT_FOUND,
    FCS_PRESET_CODE_FREECELLS_EXCEED_MAX,
    FCS_PRESET_CODE_STACKS_EXCEED_MAX,
    FCS_PRESET_CODE_DECKS_EXCEED_MAX
};

static int fcs_apply_preset(
    freecell_solver_instance_t * instance,
    int preset_id
    )
{
    unsigned int preset_index;
    for(preset_index=0 ; preset_index < (sizeof(fcs_presets)/sizeof(fcs_presets[0])) ; preset_index++)
    {
        if (fcs_presets[preset_index].preset_id == preset_id)
        {
#define preset (fcs_presets[preset_index])
            if (preset.freecells_num > MAX_NUM_FREECELLS)
            {
                return FCS_PRESET_CODE_FREECELLS_EXCEED_MAX;
            }
            if (preset.stacks_num > MAX_NUM_STACKS)
            {
                return FCS_PRESET_CODE_STACKS_EXCEED_MAX;
            }
            if (preset.decks_num > MAX_NUM_DECKS)
            {
                return FCS_PRESET_CODE_DECKS_EXCEED_MAX;
            }
            instance->freecells_num = preset.freecells_num;
            instance->stacks_num = preset.stacks_num;
            instance->decks_num = preset.decks_num;

            instance->sequences_are_built_by = preset.sequences_are_built_by;
            instance->unlimited_sequence_move = preset.unlimited_sequence_move;
            instance->empty_stacks_fill = preset.empty_stacks_fill;

            return FCS_PRESET_CODE_OK;
        }
    }
    
    return FCS_PRESET_CODE_NOT_FOUND;
}

int fcs_apply_preset_by_name(
    freecell_solver_instance_t * instance,
    char * name
    )
{
    int preset_id = fcs_get_preset_id_by_name(name);
    if (preset_id >= 0)
    {
        return ((fcs_apply_preset(instance, preset_id) == FCS_PRESET_CODE_OK)?0:2);
    }
    return 1;
}
