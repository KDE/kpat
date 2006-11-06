#ifndef _MEMORY_H
#define _MEMORY_H

#include <sys/types.h>

struct TREE;

/* Memory. */

struct BLOCK;
struct BLOCK {
	u_char *block;
	u_char *ptr;
	size_t remain;
        BLOCK *next;
};

struct TREELIST;
struct TREELIST {
	TREE *tree;
	int cluster;
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

class MemoryManager
{
public:
    enum inscode { NEW, FOUND, FOUND_BETTER, ERR };

    u_char *new_from_block(size_t s);
    void init_clusters(void);
    void free_blocks(void);
    void free_clusters(void);
    TREELIST *cluster_tree(int cluster);
    inscode insert_node(TREE *n, int d, TREE **tree, TREE **node);
    void give_back_block(u_char *p);
    void init_buckets( int i );
    BLOCK *new_block(void);

    template<class T>
    static void free_ptr(T *ptr) {
        free(ptr); Mem_remain += sizeof(T);
    }

    template<class T>
    static void free_array(T *ptr, size_t size) {
        free(ptr);
        Mem_remain += size * sizeof(T);
    }

    static void *allocate_memory(size_t s);

    // ugly hack
    int Pilebytes;
private:
    BLOCK *Block;
    static size_t Mem_remain;
};

#define new_array( type, size ) ( type* )MemoryManager::allocate_memory( ( size )*sizeof( type ) );
#define allocate( type ) ( type* )MemoryManager::allocate_memory( sizeof( type ) );

#endif
