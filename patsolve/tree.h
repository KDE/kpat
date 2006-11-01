#ifndef SPLAY_H
#define SPLAY_H

#include <string.h>
#include <sys/types.h>
#include "config.h"

/* A splay tree. */

typedef struct tree_node TREE;

struct tree_node {
	TREE *left;
	TREE *right;
	short depth;
};

extern int insert(int *cluster, int d, TREE **node);

#endif
