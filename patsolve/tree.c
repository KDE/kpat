/* Position storage.  A forest of binary trees labeled by cluster. */

#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "pat.h"
#include "tree.h"

/* Given a cluster number, return a tree.  There are 14^4 possible
clusters, but we'll only use a few hundred of them at most.  Hash on
the cluster number, then locate its tree, creating it if necessary. */

#define NBUCKETS 499    /* a prime */

TREELIST *Treelist[NBUCKETS];

static int insert_node(TREE *n, int d, TREE **tree, TREE **node);
static TREELIST *cluster_tree(int cluster);
static void give_back_block(u_char *p);
static BLOCK *new_block(void);

int Pilebytes;

static INLINE int CMP(u_char *a, u_char *b)
{
	return memcmp(a, b, Pilebytes);
}

/* Insert key into the tree unless it's already there.  Return true if
it was new. */

int insert(int *cluster, int d, TREE **node)
{
	int i, k;
	TREE *new;
	TREELIST *tl;

	/* Get the cluster number from the Out cell contents. */

	i = O[0] + (O[1] << 4);
	k = i;
	i = O[2] + (O[3] << 4);
	k |= i << 8;
	*cluster = k;

	/* Get the tree for this cluster. */

	tl = cluster_tree(k);
	if (tl == NULL) {
		return ERR;
	}

	/* Create a compact position representation. */

	new = pack_position();
	if (new == NULL) {
		return ERR;
	}
	Total_generated++;

	i = insert_node(new, d, &tl->tree, node);

	if (i != NEW) {
		give_back_block((u_char *)new);
	}

	return i;
}

/* Add it to the binary tree for this cluster.  The piles are stored
following the TREE structure. */

static int insert_node(TREE *n, int d, TREE **tree, TREE **node)
{
	int c;
	u_char *key, *tkey;
	TREE *t;

	key = (u_char *)n + sizeof(TREE);
	n->depth = d;
	n->left = n->right = NULL;
	*node = n;
	t = *tree;
	if (t == NULL) {
		*tree = n;
		return NEW;
	}
	while (1) {
		tkey = (u_char *)t + sizeof(TREE);
		c = CMP(key, tkey);
		if (c == 0) {
			break;
		}
		if (c < 0) {
			if (t->left == NULL) {
				t->left = n;
				return NEW;
			}
			t = t->left;
		} else {
			if (t->right == NULL) {
				t->right = n;
				return NEW;
			}
			t = t->right;
		}
	}

	/* We get here if it's already in the tree.  Don't add it again.
	If the new path to this position was shorter, record the new depth
	so we can prune the original path. */

	c = FOUND;
	if (d < t->depth && !Stack) {
		t->depth = d;
		c = FOUND_BETTER;
		*node = t;
	}
	return c;
}

/* @@@ This goes somewhere else. */

BLOCK *Block;

/* Clusters are also stored in a hashed array. */

void init_clusters(void)
{
	memset(Treelist, 0, sizeof(Treelist));
	Block = new_block();                    /* @@@ */
}

static TREELIST *cluster_tree(int cluster)
{
	int bucket;
	TREELIST *tl, *last;

	/* Pick a bucket, any bucket. */

	bucket = cluster % NBUCKETS;

	/* Find the tree in this bucket with that cluster number. */

	last = NULL;
	for (tl = Treelist[bucket]; tl; tl = tl->next) {
		if (tl->cluster == cluster) {
			break;
		}
		last = tl;
	}

	/* If we didn't find it, make a new one and add it to the list. */

	if (tl == NULL) {
		tl = new(TREELIST);
		if (tl == NULL) {
			return NULL;
		}
		tl->tree = NULL;
		tl->cluster = cluster;
		tl->next = NULL;
		if (last == NULL) {
			Treelist[bucket] = tl;
		} else {
			last->next = tl;
		}
	}

	return tl;
}

/* Block storage.  Reduces overhead, and can be freed quickly. */

static BLOCK *new_block(void)
{
	BLOCK *b;

	b = new(BLOCK);
	if (b == NULL) {
		return NULL;
	}
	b->block = new_array(u_char, BLOCKSIZE);
	if (b->block == NULL) {
		free_ptr(b, BLOCK);
		return NULL;
	}
	b->ptr = b->block;
	b->remain = BLOCKSIZE;
	b->next = NULL;

	return b;
}

/* Like new(), only from the current block.  Make a new block if necessary. */

u_char *new_from_block(size_t s)
{
	u_char *p;
	BLOCK *b;

	b = Block;
	if (s > b->remain) {
		b = new_block();
		if (b == NULL) {
			return NULL;
		}
		b->next = Block;
		Block = b;
	}

	p = b->ptr;
	b->remain -= s;
	b->ptr += s;

	return p;
}

/* Return the previous result of new_from_block() to the block.  This
can ONLY be called once, immediately after the call to new_from_block().
That is, no other calls to new_from_block() are allowed. */

static void give_back_block(u_char *p)
{
	size_t s;
	BLOCK *b;

	b = Block;
	s = b->ptr - p;
	b->ptr -= s;
	b->remain += s;
}

void free_blocks(void)
{
	BLOCK *b, *next;

	b = Block;
	while (b) {
		next = b->next;
		free_array(b->block, u_char, BLOCKSIZE);
		free_ptr(b, BLOCK);
		b = next;
	}
}

void free_clusters(void)
{
	int i;
	TREELIST *l, *n;

	for (i = 0; i < NBUCKETS; i++) {
		l = Treelist[i];
		while (l) {
			n = l->next;
			free_ptr(l, TREELIST);
			l = n;
		}
	}
}
