/*
 * fcs_hash.h - header file of Freecell Solver's internal hash implementation.
 *
 * Written by Shlomi Fish (shlomif@vipe.technion.ac.il), 2000
 *
 * This file is in the public domain (it's uncopyrighted).
 */

#ifndef __FCS_HASH_H
#define __FCS_HASH_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int SFO_hash_value_t;

struct SFO_hash_symlink_item_struct
{
    void * key;
    SFO_hash_value_t hash_value;
    struct SFO_hash_symlink_item_struct * next;
};

typedef struct SFO_hash_symlink_item_struct SFO_hash_symlink_item_t;

struct SFO_hash_symlink_struct
{
    SFO_hash_symlink_item_t * first_item;    
};

typedef struct SFO_hash_symlink_struct SFO_hash_symlink_t;

struct SFO_hash_struct
{
    SFO_hash_symlink_t * entries;
    int (*compare_function)(const void * key1, const void * key2, void * context);
    int size;
    int num_elems;
    void * context;
};

typedef struct SFO_hash_struct SFO_hash_t;


SFO_hash_t * SFO_hash_init(
    SFO_hash_value_t wanted_size,
    int (*compare_function)(const void * key1, const void * key2, void * context),
    void * context       
    );

void * SFO_hash_insert(
    SFO_hash_t * hash,
    void * key,
    SFO_hash_value_t hash_value
    );


void SFO_hash_free(
    SFO_hash_t * hash
    );

void SFO_hash_free_with_callback(
    SFO_hash_t * hash,
    void (*function_ptr)(void * key, void * context)
    );


#ifdef __cplusplus
}
#endif

#endif /* __FCS_HASH_H */




