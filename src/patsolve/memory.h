/*
 * Copyright (C) 1998-2002 Tom Holroyd <tomh@kurage.nimh.nih.gov>
 * Copyright (C) 2006-2009 Stephan Kulow <coolo@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MEMORY_H
#define MEMORY_H

// Std
#include <cstdlib>
#include <sys/types.h>

struct TREE;

/* Memory. */

struct BLOCK;
struct BLOCK {
    unsigned char *block;
    unsigned char *ptr;
    size_t remain;
    BLOCK *next;
};

struct TREELIST;
struct TREELIST {
    TREE *tree;
    unsigned int cluster;
    TREELIST *next;
};

/* Position information.  We store a compact representation of the position;
Temp cells are stored separately since they don't have to be compared.
We also store the move that led to this position from the parent, as well
as a pointers back to the parent, and the btree of all positions examined so
far. */
struct TREE {
    TREE *left;
    TREE *right;
    short depth;
};

#ifdef ERR
#undef ERR
#endif

class MemoryManager
{
public:
    enum inscode { NEW, FOUND, ERR };

    unsigned char *new_from_block(size_t s);
    void init_clusters(void);
    void free_blocks(void);
    void free_clusters(void);
    TREELIST *cluster_tree(unsigned int cluster);
    inscode insert_node(TREE *n, int d, TREE **tree, TREE **node);
    void give_back_block(unsigned char *p);

    BLOCK *new_block(void);

    template<class T>
    static void free_ptr(T *ptr)
    {
        free(ptr);
        Mem_remain += sizeof(T);
    }

    template<class T>
    static void free_array(T *ptr, size_t size)
    {
        free(ptr);
        Mem_remain += size * sizeof(T);
    }

    static void *allocate_memory(size_t s);

    // ugly hack
    int Pilebytes;
    static size_t Mem_remain;

private:
    BLOCK *Block;
};

template<typename T>
T *mm_new_array(const size_t size)
{
    return static_cast<T *>(MemoryManager::allocate_memory(size * sizeof(T)));
}

template<typename T>
T *mm_allocate()
{
    return static_cast<T *>(MemoryManager::allocate_memory(sizeof(T)));
}

#endif // MEMORY_H
