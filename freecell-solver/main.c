/*
 * main.c - main program of Freecell Solver (single-threaded)
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000
 *
 * This file is in the public domain (it's uncopyrighted).
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fcs.h"
#include "card.h"
#include "preset.h"

#ifdef DEBUG
#include <signal.h>
#endif


struct struct_freecell_solver_display_information_context
{
    int debug_iter_state_output;
    int freecells_num;
    int stacks_num;
    int decks_num;
    int parseable_output;
    int canonized_order_output;
    int display_10_as_t;
};

typedef struct struct_freecell_solver_display_information_context freecell_solver_display_information_context_t;

void freecell_solver_display_information(
    void * lp_context,
    int iter_num,
    int depth,
    void * lp_instance,
    fcs_state_with_locations_t * ptr_state_with_locations
    )
{
    freecell_solver_display_information_context_t * context;
    
    context = (freecell_solver_display_information_context_t*)lp_context;
    
    fprintf(stdout, "Iteration: %i\n", iter_num);
    fprintf(stdout, "Depth: %i\n\n", depth);


    if (context->debug_iter_state_output)
    {           
        char * state_string = fcs_state_as_string(
                ptr_state_with_locations, 
                context->freecells_num,
                context->stacks_num,
                context->decks_num,
                context->parseable_output, 
                context->canonized_order_output,
                context->display_10_as_t
                );
        printf("%s\n---------------\n\n\n", state_string);
        free((void*)state_string);
    }
}


#ifdef DEBUG

int command_num = 0;

void select_signal_handler(int signal_num)
{
    command_num = (command_num+1)%3;
}

freecell_solver_instance_t * current_instance;

void command_signal_handler(int signal_num)
{
    freecell_solver_display_information_context_t * dc;

    dc = (freecell_solver_display_information_context_t *)current_instance->debug_iter_output_context;
    
    if (command_num == 0)
    {
        fprintf(stderr, "The number of iterations is %i\n", current_instance->num_times);
    }
    else if (command_num == 1)
    {
        current_instance->debug_iter_output = ! current_instance->debug_iter_output;
    }
    else if (command_num == 2)
    {
        dc->debug_iter_state_output = ! dc->debug_iter_state_output;
    }

    command_num = 0;
}

#endif


char * help_string = 
"fc-solve [options] board_file\n"
"\n"
"If board_file is - or unspecified reads standard input\n"
"\n"
"Available Options:\n"
"-h   --help           \n"
"     display this help screen\n"
"-i   --iter-output    \n"
"     display the iteration number and depth in every state that is checked\n"
"     ( applicable only for fc-solve-debug )\n"
"-s   --state-output   \n"
"     also output the state in every state that is checked\n"
"     ( applicable only for fc-solve-debug )\n"
"-p   --parseable-output \n"
"     Output the states in a format that is friendly to perl, grep and\n"
"     friends.\n"
"-c   --canonized-order-output \n"
"     Output the stacks and freecells according to their canonic order.\n"
"     (That means that stacks and freecells won't retain their place.)\n"
"-t   --display-10-as-t \n"
"     Display the card 10 as a capital T instead of \"10\".\n"
"-m   --display-moves \n"
"     Display the moves instead of the intermediate states.\n"
"\n"
"--freecells-num [Freecells\' Number]\n"
"     The number of freecells present in the board.\n"
"--stacks-num [Stacks\' Number]\n"
"     The number of stacks present in the board.\n"
"--decks-num [Decks\' Number]\n"
"     The number of decks in the board.\n"
"\n"
"--sequences-are-built-by {suit|alternate_color|rank}\n"
"     Specifies the type of sequence\n"
"--sequence-move {limited|unlimited}\n"
"     Specifies whether the sequence move is limited by the number of\n"
"     freecells or vacant stacks or not.\n"
"--empty-stacks-filled-by {kings|none|all}\n"
"     Specifies which cards can fill empty stacks.\n"
"\n"
"--game [game]   --preset [game]  -g [game]\n"
"     Specifies the type of game. (Implies several of the game settings\n"
"     options above.). Available presets:\n"
"     bakers_dozen      - Baker\'s Dozen\n"
"     bakers_game       - Baker\'s Game\n"
"     cruel             - Cruel\n"
"     der_katz          - Der Katzenschwantz\n"
"     die_schlange      - Die Schlange\n"
"     eight_off         - Eight Off\n"
"     forecell          - Forecell\n"
"     freecell          - Freecell\n"
"     good_measure      - Good Measure\n"
"     ko_bakers_game    - Kings\' Only Baker\'s Game\n"
"     relaxed_freecell  - Relaxed Freecell\n"
"     relaxed_seahaven  - Relaxed Seahaven Towers\n"
"     seahaven          - Seahaven Towers\n"
"\n"
"-md [depth]       --max-depth [depth] \n"
"     Specify a maximal search depth for the solution process.\n"
"-mi [iter_num]    --max-iters [iter_num] \n"
"     Specify a maximal number of iterations number.\n"
"\n"
"-to [tests_order]   --tests-order  [tests_order] \n"
"     Specify a test order string. Each test is represented by one character.\n"
"     Valid tests:\n"
"        '0' - put top stack cards in the foundations.\n"
"        '1' - put freecell cards in the foundations.\n"
"        '2' - put freecell cards on top of stacks.\n"
"        '3' - put non-top stack cards in the foundations.\n"
"        '4' - move stack cards to different stacks.\n"
"        '5' - move stack cards to a parent card on the same stack.\n"
"        '6' - move sequences of cards onto free stacks.\n"
"        '7' - put freecell cards on empty stacks.\n"
"        '8' - move cards to a different parent.\n"
"        '9' - empty an entire stack into the freecells.\n"
"\n"
"-me [solving_method]   --method [solving_method]\n"
"     Specify a solving method. Available methods are:\n"
"        \"a-star\" - A*\n"
"        \"bfs\" - Breadth-First Search\n"
"        \"dfs\" - Depth-First Search (default)\n"
"        \"soft-dfs\" - \"Soft\" DFS\n"
"\n"
"-asw [A* Weights]   --a-star-weight [A* Weights]\n" 
"     Specify weights for the A* scan, assuming it is used. The parameter\n"
"     should be a comma-separated list of numbers, each one is proportional\n"
"     to the weight of its corresponding test.\n"
"\n"
"     The numbers are, in order:\n"
"     1. The number of cards out.\n"
"     2. The maximal sequence move.\n"
"     3. The number of cards under sequences.\n"
"     4. The length of the sequences which are found over renegade cards.\n"
"     5. The depth of the board in the solution.\n"
"\n"
"\n"
"Signals: (applicable only for fc-solve-debug)\n"
"SIGUSR1 - Prints the number of states that were checked so far to stderr.\n"
"SIGUSR2 SIGUSR1 - Turns iteration output on/off.\n"
"SIGUSR2 SIGUSR2 SIGUSR1 - Turns iteration's state output on/off.\n"
"\n"
"\n"
"Freecell Solver was written by Shlomi Fish.\n"
"Homepage: http://vipe.technion.ac.il/~shlomif/freecell-solver/\n"
"Send comments and suggestions to shlomif@vipe.technion.ac.il\n"
;

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif


int main(int argc, char * argv[])
{
    char user_state[1024];

    FILE * file;

    fcs_state_with_locations_t state, normalized_state;
    int ret;
    fcs_card_t card;

    int arg, a;

    int display_moves;

    freecell_solver_instance_t * instance;
    freecell_solver_display_information_context_t debug_context;

    instance = freecell_solver_alloc_instance();

    debug_context.parseable_output = 0;
    debug_context.display_10_as_t = 0;
    debug_context.canonized_order_output = 0;
    debug_context.debug_iter_state_output = 0;

    display_moves = 0;

    for(arg=1;arg<argc;arg++)
    {
        if ((!strcmp(argv[arg], "-h")) || (!strcmp(argv[arg], "--help")))
        {
            printf("%s", help_string);
            return 0;
        }
        else if ((!strcmp(argv[arg], "-i")) || (!strcmp(argv[arg], "--iter-output")))
        {
            instance->debug_iter_output = 1;
        }
        else if ((!strcmp(argv[arg], "-s")) || (!strcmp(argv[arg], "--state-output")))
        {
            debug_context.debug_iter_state_output = 1;
        }
        else if ((!strcmp(argv[arg], "-p")) || (!strcmp(argv[arg], "--parseable-output")))
        {
            debug_context.parseable_output = 1;
        }
        else if ((!strcmp(argv[arg], "-c")) || (!strcmp(argv[arg], "--canonized-order-output")))
        {
            debug_context.canonized_order_output = 1;
        }
        else if ((!strcmp(argv[arg], "-md")) || (!strcmp(argv[arg], "--max-depth")))
        {
            arg++;
            if (arg == argc)
            {
                fprintf(
                    stderr,
                    "The option \"%s\" should be followed by a parameter.\n",
                    argv[arg-1]
                    );

                freecell_solver_free_instance(instance);
                return -1;                        
            }
            
            instance->max_depth = atoi(argv[arg]);
        }
        else if ((!strcmp(argv[arg], "-mi")) || (!strcmp(argv[arg], "--max-iters")))
        {
            arg++;
            if (arg == argc)
            {
                fprintf(
                    stderr,
                    "The option \"%s\" should be followed by a parameter.\n",
                    argv[arg-1]
                    );

                freecell_solver_free_instance(instance);
                return -1;                        
            }
            
            instance->max_num_times = atoi(argv[arg]);
        }
        else if ((!strcmp(argv[arg], "-to")) || (!strcmp(argv[arg], "--tests-order")))
        {
            arg++;
            if (arg == argc)
            {
                fprintf(
                    stderr,
                    "The option \"%s\" should be followed by a parameter.\n",
                    argv[arg-1]
                    );

                freecell_solver_free_instance(instance);
                return -1;                        
            }
            instance->tests_order_num = min(strlen(argv[arg]), FCS_TESTS_NUM);
            for(a=0;a<instance->tests_order_num;a++)
            {
                instance->tests_order[a] = (argv[arg][a]-'0')%FCS_TESTS_NUM;
            }
        }
        else if ((!strcmp(argv[arg], "--freecells-num")))
        {
            arg++;
            if (arg == argc)
            {
                fprintf(
                    stderr,
                    "The option \"%s\" should be followed by a parameter.\n",
                    argv[arg-1]
                    );

                freecell_solver_free_instance(instance);
                return -1;                        
            }
            instance->freecells_num = atoi(argv[arg]);
            if (instance->freecells_num > MAX_NUM_FREECELLS)
            {
                fprintf(stderr, 
                    "Error! The freecells\' number " 
                    "exceeds the maximum of %i.\n"
                    "Recompile the program if you wish to have more.\n", 
                    MAX_NUM_FREECELLS
                    );

                freecell_solver_free_instance(instance);
                return (-1);
            }
        }
        else if ((!strcmp(argv[arg], "--stacks-num")))
        {
            arg++;
            if (arg == argc)
            {
                fprintf(
                    stderr,
                    "The option \"%s\" should be followed by a parameter.\n",
                    argv[arg-1]
                    );

                freecell_solver_free_instance(instance);
                return -1;                        
            }            
            instance->stacks_num = atoi(argv[arg]);
            if (instance->stacks_num > MAX_NUM_STACKS)
            {
                fprintf(stderr, 
                    "Error! The stacks\' number " 
                    "exceeds the maximum of %i.\n"
                    "Recompile the program if you wish to have more.\n", 
                    MAX_NUM_STACKS
                    );
                return (-1);
            }
        }
        else if ((!strcmp(argv[arg], "-t")) || (!strcmp(argv[arg], "--display-10-as-t")))
        {
            debug_context.display_10_as_t = 1;
        }
        else if ((!strcmp(argv[arg], "--decks-num")))
        {
            arg++;
            if (arg == argc)
            {
                fprintf(
                    stderr,
                    "The option \"%s\" should be followed by a parameter.\n",
                    argv[arg-1]
                    );

                freecell_solver_free_instance(instance);
                return -1;                        
            }            
            instance->decks_num = atoi(argv[arg]);
            if (instance->decks_num > MAX_NUM_DECKS)
            {
                fprintf(stderr,
                    "Error! The decks\' number "
                    "exceeds the maximum of %i.\n"
                    "Recopmile the program if you wish to have more.\n",
                    MAX_NUM_DECKS
                    );
                return (-1);
            }
        }            
        else if ((!strcmp(argv[arg], "--sequences-are-built-by")))
        {
            arg++;
            if (arg == argc)
            {
                fprintf(
                    stderr,
                    "The option \"%s\" should be followed by a parameter.\n",
                    argv[arg-1]
                    );

                freecell_solver_free_instance(instance);
                return -1;                        
            }
            
            if (!strcmp(argv[arg], "suit"))
            {
                instance->sequences_are_built_by = FCS_SEQ_BUILT_BY_SUIT;
            }
            else if (!strcmp(argv[arg], "rank"))
            {
                instance->sequences_are_built_by = FCS_SEQ_BUILT_BY_RANK;
            }
            else
            {
                instance->sequences_are_built_by = FCS_SEQ_BUILT_BY_ALTERNATE_COLOR;
            }
        }
        else if ((!strcmp(argv[arg], "--sequence-move")))
        {
            arg++;
            if (arg == argc)
            {
                fprintf(
                    stderr,
                    "The option \"%s\" should be followed by a parameter.\n",
                    argv[arg-1]
                    );

                freecell_solver_free_instance(instance);
                return -1;                        
            }
            
            if (!strcmp(argv[arg], "unlimited"))
            {
                instance->unlimited_sequence_move = 1;
            }
            else
            {
                instance->unlimited_sequence_move = 0;
            }
        }
        else if (!strcmp(argv[arg], "--empty-stacks-filled-by"))
        {
            arg++;
            if (arg == argc)
            {
                fprintf(
                    stderr,
                    "The option \"%s\" should be followed by a parameter.\n",
                    argv[arg-1]
                    );

                freecell_solver_free_instance(instance);
                return -1;                        
            }
            
            if (!strcmp(argv[arg], "kings"))
            {
                instance->empty_stacks_fill = FCS_ES_FILLED_BY_KINGS_ONLY;
            }
            else if (!strcmp(argv[arg], "none"))
            {
                instance->empty_stacks_fill = FCS_ES_FILLED_BY_NONE;
            }
            else
            {
                instance->empty_stacks_fill = FCS_ES_FILLED_BY_ANY_CARD;
            }
        }
        else if (
            (!strcmp(argv[arg], "--game")) ||
            (!strcmp(argv[arg], "--preset")) ||
            (!strcmp(argv[arg], "-g"))
            )
        {
            arg++;
            if (arg == argc)
            {
                fprintf(
                    stderr,
                    "The option \"%s\" should be followed by a parameter.\n",
                    argv[arg-1]
                    );

                freecell_solver_free_instance(instance);
                return -1;                        
            }
            
            ret = fcs_apply_preset_by_name(instance, argv[arg]);
            if (ret == 1)
            {
                fprintf(stderr, "Unknown game \"%s\"!\n\n", argv[arg]);
                return (-1);
            }
            else if (ret == 2)
            {
                fprintf(stderr, 
                    "The game \"%s\" exceeds the limits of the program.\n"
                    "Modify the file \"config.h\" and recompile, if you wish to solve one of its boards.\n", 
                    argv[arg]
                );
                return (-1);
            }
        }
        else if ((!strcmp(argv[arg], "-m")) || (!strcmp(argv[arg], "--display-moves")))
        {
            display_moves = 1;
        }
        else if ((!strcmp(argv[arg], "-me")) || (!strcmp(argv[arg], "--method")))
        {
            arg++;
            if (arg == argc)
            {
                fprintf(
                    stderr,
                    "The option \"%s\" should be followed by a parameter.\n",
                    argv[arg-1]
                    );

                freecell_solver_free_instance(instance);
                return -1;                        
            }
            if (!strcmp(argv[arg], "dfs"))
            {
                instance->method = FCS_METHOD_HARD_DFS;
            }
            else if (!strcmp(argv[arg], "soft-dfs"))
            {
                instance->method = FCS_METHOD_SOFT_DFS;
            }
            else if (!strcmp(argv[arg], "bfs"))
            {
                instance->method = FCS_METHOD_BFS;
            }
            else if (!strcmp(argv[arg], "a-star"))
            {
                instance->method = FCS_METHOD_A_STAR;
            }
            else
            {
                fprintf(
                    stderr,
                    "Unknown solving method \"%s\".\n", 
                    argv[arg]
                    );

                freecell_solver_free_instance(instance);
                return -1;                        
            }
        }
        else if ((!strcmp(argv[arg], "-asw")) || (!strcmp(argv[arg], "--a-star-weights")))
        {
            arg++;
            if (arg == argc)
            {
                fprintf(
                    stderr,
                    "The option \"%s\" should be followed by a parameter.\n",
                    argv[arg-1]
                    );

                freecell_solver_free_instance(instance);
                return -1;                        
            }
            {
                int a;
                char * start_num;
                char * end_num;
                char save;
                start_num = argv[arg];
                for(a=0;a<5;a++)
                {
                    while ((*start_num > '9') && (*start_num < '0') && (*start_num != '\0'))
                    {
                        start_num++;
                    }
                    if (*start_num == '\0')
                    {
                        break;
                    }
                    end_num = start_num+1;
                    while ((((*end_num >= '0') && (*end_num <= '9')) || (*end_num == '.')) && (*end_num != '\0'))
                    {
                        end_num++;
                    }
                    save = *end_num;
                    *end_num = '\0';
                    instance->a_star_weights[a] = atof(start_num);
                    *end_num = save;
                    start_num=end_num+1;
                }
            }
        }
        else
        {
            break;
        } 
    }

    
    if ((arg == argc) || (!strcmp(argv[arg], "-")))
    {
        file = stdin;
    }
    else if (argv[arg][0] == '-')
    {
        fprintf(stderr, 
                "Unknown option \"%s\". " 
                "Type \"%s --help\" for usage information.\n", 
                argv[arg],
                argv[0]
                );
        freecell_solver_free_instance(instance);

        return -1;
    }
    else
    {
        file = fopen(argv[arg], "r");
        if (file == NULL)
        {
            fprintf(stderr, 
                "Could not open file \"%s\" for input. Exiting.\n", 
                argv[arg]
                );
            freecell_solver_free_instance(instance);
            
            return -1;
        }
    }
    fread(user_state, sizeof(user_state[0]), sizeof(user_state)/sizeof(user_state[0]), file);
    fclose(file);
        
   



    state = fcs_initial_user_state_to_c(
        user_state, 
        instance->freecells_num, 
        instance->stacks_num,
        instance->decks_num);
    
    ret = fcs_check_state_validity(
        &state, 
        instance->freecells_num, 
        instance->stacks_num, 
        instance->decks_num,
        &card);
    
    if (ret != 0)
    {
        char card_str[10];
        fcs_card_perl2user(card, card_str, debug_context.display_10_as_t);
        if (ret == 3)
        {
            fprintf(stderr, "%s\n", 
                "There's an empty slot in one of the stacks."
                );
        }
        else
        {
            fprintf(stderr, "%s%s.\n",
                ((ret == 2)? "There's an extra card: " : "There's a missing card: "),
                card_str
            );            
        }

        freecell_solver_free_instance(instance);
        
        return -1;    
    }
    
    
    fcs_duplicate_state(normalized_state, state);

    fcs_canonize_state(
        &state, 
        instance->freecells_num, 
        instance->stacks_num
        );

#ifdef DEBUG
    current_instance = instance;
#endif

    instance->debug_iter_output_func = freecell_solver_display_information;
    instance->debug_iter_output_context = &debug_context;
    
    debug_context.freecells_num = instance->freecells_num;
    debug_context.stacks_num = instance->stacks_num;
    debug_context.decks_num = instance->decks_num;

#ifdef DEBUG
/* Win32 Does not have those signals */
#ifndef WIN32
    signal(SIGUSR1, command_signal_handler);
    signal(SIGUSR2, select_signal_handler);
#endif
#endif

    freecell_solver_init_instance(instance);

    ret = freecell_solver_solve_instance(instance, &state);
#if 0
    while (ret == FCS_STATE_SUSPEND_PROCESS) 
    {        
        instance->max_num_times += 1000;
        ret = freecell_solver_resume_instance(instance);
    }
#endif

    if (ret == FCS_STATE_WAS_SOLVED)
    {
        int a;

        printf("-=-=-=-=-=-=-=-=-=-=-=-\n\n");
        for(a=0;a<instance->num_solution_states;a++)
        {
#if 0
            char * as_string;

            as_string = fcs_state_as_string(
                instance->solution_states[a], 
                instance->freecells_num,
                instance->stacks_num,
                instance->decks_num,
                debug_context.parseable_output, 
                debug_context.canonized_order_output,
                debug_context.display_10_as_t);

            printf("%s\n\n====================\n\n", as_string);
            free(as_string);            
#endif
            free((void*)instance->solution_states[a]);

        }
        free((void*)instance->solution_states);

        if (!display_moves)
        {
            fcs_move_t move;
            fcs_state_with_locations_t dynamic_state;
            FILE * move_dump;
            char * as_string;
            int counter = 0;

            fcs_move_stack_normalize(
                instance->solution_moves,
                &state,
                instance->freecells_num,
                instance->stacks_num,
                instance->decks_num
                );

            fcs_duplicate_state(dynamic_state, normalized_state);

            
            move_dump = stdout;
            as_string = fcs_state_as_string(
                &dynamic_state, 
                instance->freecells_num,
                instance->stacks_num,
                instance->decks_num,
                debug_context.parseable_output, 
                debug_context.canonized_order_output,
                debug_context.display_10_as_t);
            
            fprintf(move_dump, "%s\n\n====================\n\n", as_string);
            fflush(move_dump);
            free(as_string);            
            while (
                fcs_move_stack_pop(
                    instance->solution_moves,
                    &move
                    ) == 0)
            {
                fcs_apply_move(
                    &dynamic_state,
                    move,
                    instance->freecells_num,
                    instance->stacks_num,
                    instance->decks_num
                    );
                as_string = fcs_state_as_string(
                    &dynamic_state, 
                    instance->freecells_num,
                    instance->stacks_num,
                    instance->decks_num,
                    debug_context.parseable_output, 
                    debug_context.canonized_order_output,
                    debug_context.display_10_as_t);
                    
                fprintf(move_dump, "%s\n\n====================\n\n", as_string);
                fflush(move_dump);
                free(as_string);
                
                ret = fcs_check_state_validity(
                    &dynamic_state, 
                    instance->freecells_num, 
                    instance->stacks_num, 
                    instance->decks_num,
                    &card);
                
                if (ret != 0)
                {
                    char card_str[10];
                    fcs_card_perl2user(card, card_str, debug_context.display_10_as_t);
                    if (ret == 3)
                    {
                        fprintf(move_dump, "%s\n",
                            "There's an empty slot in one of the stacks."
                            );
                    }
                    else
                    {
                        fprintf(move_dump, "%s%s counter=%i.\n",
                            ((ret == 2)? "There's an extra card: " : "There's a missing card: "),
                            card_str,
                            counter
                        );
                    }

                    return -1;
                }
                counter++;
            }
#if 0       
            fclose(move_dump);
#endif
        }
        else
        {
            fcs_move_t move;
            FILE * move_dump;
            char * as_string;

            fcs_move_stack_normalize(
                instance->solution_moves,
                &state,
                instance->freecells_num,
                instance->stacks_num,
                instance->decks_num
                );

            move_dump = stdout;
            while (
                fcs_move_stack_pop(
                    instance->solution_moves,
                    &move
                    ) == 0)
            {
                as_string = fcs_move_to_string(move);
            
                fprintf(move_dump, "%s\n\n====================\n\n", as_string);
                fflush(move_dump);
                free(as_string);
            }
#if 0       
            fclose(move_dump);
#endif
        }
 
        printf("This game is solveable.\n");

        fcs_move_stack_destroy(instance->solution_moves);
    }
    else
    {
        printf ("I could not solve this game.\n");

        if (ret == FCS_STATE_SUSPEND_PROCESS)
        {
            freecell_solver_unresume_instance(instance);
        }
    }
    
    printf ("Total number of states checked is %i.\n", instance->num_times);
#if 0
    printf ("Stored states: %i.\n", instance->hash->num_elems);
#endif

    freecell_solver_finish_instance(instance);

    freecell_solver_free_instance(instance);


    return 0;
}

