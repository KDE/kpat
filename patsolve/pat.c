/* Common routines & arrays. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/* A function and some macros for allocating memory. */

static void *allocate_memory(size_t s);

#define allocate(type) (type *)allocate_memory(sizeof(type))
#define free_ptr(ptr, type) free(ptr); Mem_remain += sizeof(type)

#define new_array(type, size) (type *)allocate_memory((size) * sizeof(type))
#define free_array(ptr, type, size) free(ptr); \
				    Mem_remain += (size) * sizeof(type)

static void free_buckets(void);
static void free_clusters(void);
static void free_blocks(void);

static int Stack = TRUE;      /* -S means stack, not queue, the moves to be done */
static int Cutoff = 1;         /* switch between depth- and breadth-first */
static int Status;             /* win, lose, or fail */

static size_t Mem_remain = 30 * 1000 * 1000;

/* Statistics. */

static int Total_positions;
static int Total_generated;

static int Xparam[] = { 4, 1, 8, -1, 7, 11, 4, 2, 2, 1, 2 };
static double Yparam[] = { 0.0032, 0.32, -3.0 };

/* Misc. */

static int strecpy(unsigned char *dest, unsigned char *src);

/* A card is represented as (suit << 4) + rank. */

typedef u_char card_t;

#define PS_DIAMOND 0x00         /* red */
#define PS_CLUB    0x10         /* black */
#define PS_HEART   0x20         /* red */
#define PS_SPADE   0x30         /* black */
#define PS_COLOR   0x10         /* black if set */
#define PS_SUIT    0x30         /* mask both suit bits */

#define NONE    0
#define PS_ACE  1
#define PS_KING 13

#define rank(card) ((card) & 0xF)
#define suit(card) ((card) >> 4)
#define color(card) ((card) & PS_COLOR)

/* Some macros used in get_possible_moves(). */

/* The following macro implements
	(Same_suit ? (suit(a) == suit(b)) : (color(a) != color(b)))
*/
#define suitable(a, b) ((((a) ^ (b)) & Suit_mask) == Suit_val)
static card_t Suit_mask;
static card_t Suit_val;

#define king_only(card) (!King_only || rank(card) == PS_KING)

/* Represent a move. */

typedef struct {
	card_t card;            /* the card we're moving */
	u_char from;            /* from pile number */
	u_char to;              /* to pile number */
	u_char fromtype;        /* O, T, or W */
	u_char totype;
	card_t srccard;         /* card we're uncovering */
	card_t destcard;        /* card we're moving to */
	signed char pri;        /* move priority (low priority == low value) */
} MOVE;

/* A splay tree. */

typedef struct tree_node TREE;

struct tree_node {
	TREE *left;
	TREE *right;
	short depth;
};

#define O_TYPE 1                /* pile types */
#define T_TYPE 2
#define W_TYPE 3

#define MAXTPILES       8       /* max number of piles */
#define MAXWPILES      13

/* Position information.  We store a compact representation of the position;
Temp cells are stored separately since they don't have to be compared.
We also store the move that led to this position from the parent, as well
as a pointers back to the parent, and the btree of all positions examined so
far. */

typedef struct pos {
	struct pos *queue;      /* next position in the queue */
	struct pos *parent;     /* point back up the move stack */
	TREE *node;             /* compact position rep.'s tree node */
	MOVE move;              /* move that got us here from the parent */
	unsigned short cluster; /* the cluster this node is in */
	short depth;            /* number of moves so far */
	u_char ntemp;           /* number of cards in T */
	u_char nchild;          /* number of child nodes left */
} POSITION;

/* Work arrays. */

card_t T[MAXTPILES];     /* one card in each temp cell */
card_t W[MAXWPILES][52]; /* the workspace */
card_t *Wp[MAXWPILES];   /* point to the top card of each work pile */
int Wlen[MAXWPILES];     /* the number of cards in each pile */
int Widx[MAXWPILES];     /* used to keep the piles sorted */
int Widxi[MAXWPILES];    /* inverse of the above */

enum inscode { NEW, FOUND, FOUND_BETTER, ERR };

/* Command line flags. */

enum statuscode { FAIL = -1, WIN = 0, NOSOL = 1 };

/* Memory. */

typedef struct block {
	u_char *block;
	u_char *ptr;
	size_t remain;
	struct block *next;
} BLOCK;

#define BLOCKSIZE (32 * 4096)

typedef struct treelist {
	TREE *tree;
	int cluster;
	struct treelist *next;
} TREELIST;

static u_char *new_from_block(size_t);
static void init_clusters(void);

/* This is a 32 bit FNV hash.  For more information, see
http://www.isthe.com/chongo/tech/comp/fnv/index.html */

#include <sys/types.h>

#define FNV1_32_INIT 0x811C9DC5
#define FNV_32_PRIME 0x01000193

#define fnv_hash(x, hash) (((hash) * FNV_32_PRIME) ^ (x))

/* Hash a buffer. */

static __inline__ u_int32_t fnv_hash_buf(u_char *s, int len)
{
	int i;
	u_int32_t h;

	h = FNV1_32_INIT;
	for (i = 0; i < len; i++) {
		h = fnv_hash(*s++, h);
	}

	return h;
}

/* Hash a 0 terminated string. */

static __inline__ u_int32_t fnv_hash_str(u_char *s)
{
	u_int32_t h;

	h = FNV1_32_INIT;
	while (*s) {
		h = fnv_hash(*s++, h);
	}

	return h;
}


#include <string.h>
#include <sys/types.h>

static int insert(int *cluster, int d, TREE **node);

/* Default variation. */

#define SAME_SUIT 1
#define KING_ONLY 0
#define NWPILES 10      /* number of W piles */
#define NTPILES 4       /* number of T cells */

static int Same_suit = SAME_SUIT;
static int King_only = KING_ONLY;
static int Nwpiles = NWPILES; /* the numbers we're actually using */
static int Ntpiles = NTPILES;

/* Names of the cards.  The ordering is defined in pat.h. */

static char Rank[] = " A23456789TJQK";
static char Suit[] = "DCHS";

static card_t O[4]; /* output piles store only the rank or NONE */
static card_t Osuit[4] = { PS_DIAMOND, PS_CLUB, PS_HEART, PS_SPADE }; /* suits of the output piles */

/* Position freelist. */

static POSITION *Freepos = NULL;

/* Every different pile has a hash and a unique id. */

static u_int32_t Whash[MAXWPILES];
static int Wpilenum[MAXWPILES];

/* Temp storage for possible moves. */

#define MAXMOVES 64             /* > max # moves from any position */
static MOVE Possible[MAXMOVES];

static int Pilebytes;

static int get_possible_moves(int *, int *);
static void mark_irreversible(int n);
static void win(POSITION *pos);
static __inline__ int get_pilenum(int w);

/* Hash a pile. */

static __inline__ void hashpile(int w)
{
	W[w][Wlen[w]] = 0;
	Whash[w] = fnv_hash_str(W[w]);

	/* Invalidate this pile's id.  We'll calculate it later. */

	Wpilenum[w] = -1;
}

/* Hash the whole layout.  This is called once, at the start. */

static void hash_layout(void)
{
	int w;

	for (w = 0; w < Nwpiles; w++) {
		hashpile(w);
	}
}

/* These two routines make and unmake moves. */

static void make_move(MOVE *m)
{
	int from, to;
	card_t card;

	from = m->from;
	to = m->to;

	/* Remove from pile. */

	if (m->fromtype == T_TYPE) {
		card = T[from];
		T[from] = NONE;
	} else {
		card = *Wp[from]--;
		Wlen[from]--;
		hashpile(from);
	}

	/* Add to pile. */

	if (m->totype == T_TYPE) {
		T[to] = card;
	} else if (m->totype == W_TYPE) {
		*++Wp[to] = card;
		Wlen[to]++;
		hashpile(to);
	} else {
		O[to]++;
	}
}

static void undo_move(MOVE *m)
{
	int from, to;
	card_t card;

	from = m->from;
	to = m->to;

	/* Remove from 'to' pile. */

	if (m->totype == T_TYPE) {
		card = T[to];
		T[to] = NONE;
	} else if (m->totype == W_TYPE) {
		card = *Wp[to]--;
		Wlen[to]--;
		hashpile(to);
	} else {
		card = O[to] + Osuit[to];
		O[to]--;
	}

	/* Add to 'from' pile. */

	if (m->fromtype == T_TYPE) {
		T[from] = card;
	} else {
		*++Wp[from] = card;
		Wlen[from]++;
		hashpile(from);
	}
}

/* Move prioritization.  Given legal, pruned moves, there are still some
that are a waste of time, especially in the endgame where there are lots of
possible moves, but few productive ones.  Note that we also prioritize
positions when they are added to the queue. */

#define NNEED 8

static void prioritize(MOVE *mp0, int n)
{
	int i, j, s, w, pile[NNEED], npile;
	card_t card, need[4];
	MOVE *mp;

	/* There are 4 cards that we "need": the next cards to go out.  We
	give higher priority to the moves that remove cards from the piles
	containing these cards. */

	for (i = 0; i < NNEED; i++) {
		pile[i] = -1;
	}
	npile = 0;

	for (s = 0; s < 4; s++) {
		need[s] = NONE;
		if (O[s] == NONE) {
			need[s] = Osuit[s] + PS_ACE;
		} else if (O[s] != PS_KING) {
			need[s] = Osuit[s] + O[s] + 1;
		}
	}

	/* Locate the needed cards.  There's room for optimization here,
	like maybe an array that keeps track of every card; if maintaining
	such an array is not too expensive. */

	for (w = 0; w < Nwpiles; w++) {
		j = Wlen[w];
		for (i = 0; i < j; i++) {
			card = W[w][i];
			s = suit(card);

			/* Save the locations of the piles containing
			not only the card we need next, but the card
			after that as well. */

			if (need[s] != NONE &&
			    (card == need[s] || card == need[s] + 1)) {
				pile[npile++] = w;
				if (npile == NNEED) {
					break;
				}
			}
		}
		if (npile == NNEED) {
			break;
		}
	}

	/* Now if any of the moves remove a card from any of the piles
	listed in pile[], bump their priority.  Likewise, if a move
	covers a card we need, decrease its priority.  These priority
	increments and decrements were determined empirically. */

	for (i = 0, mp = mp0; i < n; i++, mp++) {
		if (mp->card != NONE) {
			if (mp->fromtype == W_TYPE) {
				w = mp->from;
				for (j = 0; j < npile; j++) {
					if (w == pile[j]) {
						mp->pri += Xparam[0];
					}
				}
				if (Wlen[w] > 1) {
					card = W[w][Wlen[w] - 2];
					for (s = 0; s < 4; s++) {
						if (card == need[s]) {
							mp->pri += Xparam[1];
							break;
						}
					}
				}
			}
			if (mp->totype == W_TYPE) {
				for (j = 0; j < npile; j++) {
					if (mp->to == pile[j]) {
						mp->pri -= Xparam[2];
					}
				}
			}
		}
	}
}

/* Generate an array of the moves we can make from this position. */

static MOVE *get_moves(POSITION *pos, int *nmoves)
{
	int i, n, alln, o, a, numout = 0;
	MOVE *mp, *mp0;

	/* Fill in the Possible array. */

	alln = n = get_possible_moves(&a, &numout);

	if (!a) {

		/* Mark any irreversible moves. */

		mark_irreversible(n);
	}

	/* No moves?  Maybe we won. */

	if (n == 0) {
		for (o = 0; o < 4; o++) {
			if (O[o] != PS_KING) {
				break;
			}
		}

		if (o == 4) {

			/* Report the win. */

			win(pos);
			Status = WIN;
			return NULL;
		}

		/* We lost. */

		return NULL;
	}

	/* Prioritize these moves.  Automoves don't get queued, so they
	don't need a priority. */

	if (!a) {
		prioritize(Possible, alln);
	}

	/* Now copy to safe storage and return.  Non-auto moves out get put
	at the end.  Queueing them isn't a good idea because they are still
	good moves, even if they didn't pass the automove test.  So we still
	do the recursive solve() on them, but only after queueing the other
	moves. */

	mp = mp0 = new_array(MOVE, n);
	if (mp == NULL) {
		return NULL;
	}
	*nmoves = n;
	i = 0;
	if (a || numout == 0) {
		for (i = 0; i < alln; i++) {
			if (Possible[i].card != NONE) {
				*mp = Possible[i];      /* struct copy */
				mp++;
			}
		}
	} else {
		for (i = numout; i < alln; i++) {
			if (Possible[i].card != NONE) {
				*mp = Possible[i];      /* struct copy */
				mp++;
			}
		}
		for (i = 0; i < numout; i++) {
			if (Possible[i].card != NONE) {
				*mp = Possible[i];      /* struct copy */
				mp++;
			}
		}
	}

	return mp0;
}

/* Automove logic.  Freecell games must avoid certain types of automoves. */

static __inline__ int good_automove(int o, int r)
{
	int i;

	if (Same_suit || r <= 2) {
		return TRUE;
	}

	/* Check the Out piles of opposite color. */

	for (i = 1 - (o & 1); i < 4; i += 2) {
		if (O[i] < r - 1) {

#if 1   /* Raymond's Rule */
			/* Not all the N-1's of opposite color are out
			yet.  We can still make an automove if either
			both N-2's are out or the other same color N-3
			is out (Raymond's rule).  Note the re-use of
			the loop variable i.  We return here and never
			make it back to the outer loop. */

			for (i = 1 - (o & 1); i < 4; i += 2) {
				if (O[i] < r - 2) {
					return FALSE;
				}
			}
			if (O[(o + 2) & 3] < r - 3) {
				return FALSE;
			}

			return TRUE;
#else   /* Horne's Rule */
			return FALSE;
#endif
		}
	}

	return TRUE;
}

/* Get the possible moves from a position, and store them in Possible[]. */

static int get_possible_moves(int *a, int *numout)
{
	int i, n, t, w, o, empty, emptyw;
	card_t card;
	MOVE *mp;

	/* Check for moves from W to O. */

	n = 0;
	mp = Possible;
	for (w = 0; w < Nwpiles; w++) {
		if (Wlen[w] > 0) {
			card = *Wp[w];
			o = suit(card);
			empty = (O[o] == NONE);
			if ((empty && (rank(card) == PS_ACE)) ||
			    (!empty && (rank(card) == O[o] + 1))) {
				mp->card = card;
				mp->from = w;
				mp->fromtype = W_TYPE;
				mp->to = o;
				mp->totype = O_TYPE;
				mp->srccard = NONE;
				if (Wlen[w] > 1) {
					mp->srccard = Wp[w][-1];
				}
				mp->destcard = NONE;
				mp->pri = 0;    /* unused */
				n++;
				mp++;

				/* If it's an automove, just do it. */

				if (good_automove(o, rank(card))) {
					*a = TRUE;
					if (n != 1) {
						Possible[0] = mp[-1];
						return 1;
					}
					return n;
				}
			}
		}
	}

	/* Check for moves from T to O. */

	for (t = 0; t < Ntpiles; t++) {
		if (T[t] != NONE) {
			card = T[t];
			o = suit(card);
			empty = (O[o] == NONE);
			if ((empty && (rank(card) == PS_ACE)) ||
			    (!empty && (rank(card) == O[o] + 1))) {
				mp->card = card;
				mp->from = t;
				mp->fromtype = T_TYPE;
				mp->to = o;
				mp->totype = O_TYPE;
				mp->srccard = NONE;
				mp->destcard = NONE;
				mp->pri = 0;    /* unused */
				n++;
				mp++;

				/* If it's an automove, just do it. */

				if (good_automove(o, rank(card))) {
					*a = TRUE;
					if (n != 1) {
						Possible[0] = mp[-1];
						return 1;
					}
					return n;
				}
			}
		}
	}

	/* No more automoves, but remember if there were any moves out. */

	*a = FALSE;
	*numout = n;

	/* Check for moves from non-singleton W cells to one of any
	empty W cells. */

	emptyw = -1;
	for (w = 0; w < Nwpiles; w++) {
		if (Wlen[w] == 0) {
			emptyw = w;
			break;
		}
	}
	if (emptyw >= 0) {
		for (i = 0; i < Nwpiles; i++) {
			if (i == emptyw) {
				continue;
			}
			if (Wlen[i] > 1 && king_only(*Wp[i])) {
				card = *Wp[i];
				mp->card = card;
				mp->from = i;
				mp->fromtype = W_TYPE;
				mp->to = emptyw;
				mp->totype = W_TYPE;
				mp->srccard = Wp[i][-1];
				mp->destcard = NONE;
				mp->pri = Xparam[3];
				n++;
				mp++;
			}
		}
	}

	/* Check for moves from W to non-empty W cells. */

	for (i = 0; i < Nwpiles; i++) {
		if (Wlen[i] > 0) {
			card = *Wp[i];
			for (w = 0; w < Nwpiles; w++) {
				if (i == w) {
					continue;
				}
				if (Wlen[w] > 0 &&
				    (rank(card) == rank(*Wp[w]) - 1 &&
				     suitable(card, *Wp[w]))) {
					mp->card = card;
					mp->from = i;
					mp->fromtype = W_TYPE;
					mp->to = w;
					mp->totype = W_TYPE;
					mp->srccard = NONE;
					if (Wlen[i] > 1) {
						mp->srccard = Wp[i][-1];
					}
					mp->destcard = *Wp[w];
					mp->pri = Xparam[4];
					n++;
					mp++;
				}
			}
		}
	}

	/* Check for moves from T to non-empty W cells. */

	for (t = 0; t < Ntpiles; t++) {
		card = T[t];
		if (card != NONE) {
			for (w = 0; w < Nwpiles; w++) {
				if (Wlen[w] > 0 &&
				    (rank(card) == rank(*Wp[w]) - 1 &&
				     suitable(card, *Wp[w]))) {
					mp->card = card;
					mp->from = t;
					mp->fromtype = T_TYPE;
					mp->to = w;
					mp->totype = W_TYPE;
					mp->srccard = NONE;
					mp->destcard = *Wp[w];
					mp->pri = Xparam[5];
					n++;
					mp++;
				}
			}
		}
	}

	/* Check for moves from T to one of any empty W cells. */

	if (emptyw >= 0) {
		for (t = 0; t < Ntpiles; t++) {
			card = T[t];
			if (card != NONE && king_only(card)) {
				mp->card = card;
				mp->from = t;
				mp->fromtype = T_TYPE;
				mp->to = emptyw;
				mp->totype = W_TYPE;
				mp->srccard = NONE;
				mp->destcard = NONE;
				mp->pri = Xparam[6];
				n++;
				mp++;
			}
		}
	}

	/* Check for moves from W to one of any empty T cells. */

	for (t = 0; t < Ntpiles; t++) {
		if (T[t] == NONE) {
			break;
		}
	}
	if (t < Ntpiles) {
		for (w = 0; w < Nwpiles; w++) {
			if (Wlen[w] > 0) {
				card = *Wp[w];
				mp->card = card;
				mp->from = w;
				mp->fromtype = W_TYPE;
				mp->to = t;
				mp->totype = T_TYPE;
				mp->srccard = NONE;
				if (Wlen[w] > 1) {
					mp->srccard = Wp[w][-1];
				}
				mp->destcard = NONE;
				mp->pri = Xparam[7];
				n++;
				mp++;
			}
		}
	}

	return n;
}

/* Moves that can't be undone get slightly higher priority, since it means
we are moving a card for the first time. */

static void mark_irreversible(int n)
{
	int i, irr;
	card_t card, srccard;
	MOVE *mp;

	for (i = 0, mp = Possible; i < n; i++, mp++) {
		irr = FALSE;
		if (mp->totype == O_TYPE) {
			irr = TRUE;
		} else if (mp->fromtype == W_TYPE) {
			srccard = mp->srccard;
			if (srccard != NONE) {
				card = mp->card;
				if (rank(card) != rank(srccard) - 1 ||
				    !suitable(card, srccard)) {
					irr = TRUE;
				}
			} else if (King_only && mp->card != PS_KING) {
				irr = TRUE;
			}
		}
		if (irr) {
			mp->pri += Xparam[8];
		}
	}
}

/* Test the current position to see if it's new (or better).  If it is, save
it, along with the pointer to its parent and the move we used to get here. */

int Posbytes;

static POSITION *new_position(POSITION *parent, MOVE *m)
{
	int i, t, depth, cluster;
	u_char *p;
	POSITION *pos;
	TREE *node;

	/* Search the list of stored positions.  If this position is found,
	then ignore it and return (unless this position is better). */

	if (parent == NULL) {
		depth = 0;
	} else {
		depth = parent->depth + 1;
	}
	i = insert(&cluster, depth, &node);
	if (i == NEW) {
		Total_positions++;
	} else if (i != FOUND_BETTER) {
		return NULL;
	}

	/* A new or better position.  insert() already stashed it in the
	tree, we just have to wrap a POSITION struct around it, and link it
	into the move stack.  Store the temp cells after the POSITION. */

	if (Freepos) {
		p = (u_char *)Freepos;
		Freepos = Freepos->queue;
	} else {
		p = new_from_block(Posbytes);
		if (p == NULL) {
			return NULL;
		}
	}

	pos = (POSITION *)p;
	pos->queue = NULL;
	pos->parent = parent;
	pos->node = node;
	pos->move = *m;                 /* struct copy */
	pos->cluster = cluster;
	pos->depth = depth;
	pos->nchild = 0;

	p += sizeof(POSITION);
	i = 0;
	for (t = 0; t < Ntpiles; t++) {
		*p++ = T[t];
		if (T[t] != NONE) {
			i++;
		}
	}
	pos->ntemp = i;

	return pos;
}

/* Comparison function for sorting the W piles. */

static __inline__ int wcmp(int a, int b)
{
	if (Xparam[9] < 0) {
		return Wpilenum[b] - Wpilenum[a];       /* newer piles first */
	} else {
		return Wpilenum[a] - Wpilenum[b];       /* older piles first */
	}
}

#if 0
static inline int wcmp(int a, int b)
{
	if (Xparam[9] < 0) {
		return Wlen[b] - Wlen[a];       /* longer piles first */
	} else {
		return Wlen[a] - Wlen[b];       /* shorter piles first */
	}
}
#endif

/* Sort the piles, to remove the physical differences between logically
equivalent layouts.  Assume it's already mostly sorted.  */

static void pilesort(void)
{
	int w, i, j;

	/* Make sure all the piles have id numbers. */

	for (w = 0; w < Nwpiles; w++) {
		if (Wpilenum[w] < 0) {
			Wpilenum[w] = get_pilenum(w);
			if (Wpilenum[w] < 0) {
				return;
			}
		}
	}

	/* Sort them. */

	Widx[0] = 0;
	w = 0;
	for (i = 1; i < Nwpiles; i++) {
		if (wcmp(Widx[w], i) < 0) {
			w++;
			Widx[w] = i;
		} else {
			for (j = w; j >= 0; --j) {
				Widx[j + 1] = Widx[j];
				if (j == 0 || wcmp(Widx[j - 1], i) < 0) {
					Widx[j] = i;
					break;
				}
			}
			w++;
		}
	}

	/* Compute the inverse. */

	for (i = 0; i < Nwpiles; i++) {
		Widxi[Widx[i]] = i;
	}
}

#define NBUCKETS 4093           /* the largest 12 bit prime */
#define NPILES  4096            /* a 12 bit code */

typedef struct bucketlist {
	u_char *pile;           /* 0 terminated copy of the pile */
	u_int32_t hash;         /* the pile's hash code */
	int pilenum;            /* the unique id for this pile */
	struct bucketlist *next;
} BUCKETLIST;

BUCKETLIST *Bucketlist[NBUCKETS];
int Pilenum;                    /* the next pile number to be assigned */

BUCKETLIST *Pilebucket[NPILES]; /* reverse lookup for unpack to get the bucket
				   from the pile */

/* Compact position representation.  The position is stored as an
array with the following format:
	pile0# pile1# ... pileN# (N = Nwpiles)
where each pile number is packed into 12 bits (so 2 piles take 3 bytes).
Positions in this format are unique can be compared with memcmp().  The O
cells are encoded as a cluster number: no two positions with different
cluster numbers can ever be the same, so we store different clusters in
different trees.  */

int Treebytes;

static TREE *pack_position(void)
{
	int j, k, w;
	u_char *p;
	TREE *node;

	/* Allocate space and store the pile numbers.  The tree node
	will get filled in later, by insert_node(). */

	p = new_from_block(Treebytes);
	if (p == NULL) {
		return NULL;
	}
	node = (TREE *)p;
	p += sizeof(TREE);

	/* Pack the pile numers j into bytes p.
		       j             j
		+--------+----:----+--------+
		|76543210|7654:3210|76543210|
		+--------+----:----+--------+
		    p         p         p
	*/

	k = 0;
	for (w = 0; w < Nwpiles; w++) {
		j = Wpilenum[Widx[w]];
		switch (k) {
		case 0:
			*p++ = j >> 4;
			*p = (j & 0xF) << 4;
			k = 1;
			break;
		case 1:
			*p++ |= j >> 8;         /* j is positive */
			*p++ = j & 0xFF;
			k = 0;
			break;
		}
	}

	return node;
}

/* Unpack a compact position rep.  T cells must be restored from the
array following the POSITION struct. */

static void unpack_position(POSITION *pos)
{
	int i, k, w;
	u_char c, *p;
	BUCKETLIST *l;

	/* Get the Out cells from the cluster number. */

	k = pos->cluster;
	O[0] = k & 0xF;
	k >>= 4;
	O[1] = k & 0xF;
	k >>= 4;
	O[2] = k & 0xF;
	k >>= 4;
	O[3] = k & 0xF;

	/* Unpack bytes p into pile numbers j.
		    p         p         p
		+--------+----:----+--------+
		|76543210|7654:3210|76543210|
		+--------+----:----+--------+
		       j             j
	*/

	k = w = i = c = 0;
	p = (u_char *)(pos->node) + sizeof(TREE);
	while (w < Nwpiles) {
		switch (k) {
		case 0:
			i = *p++ << 4;
			c = *p++;
			i |= (c >> 4) & 0xF;
			k = 1;
			break;
		case 1:
			i = (c & 0xF) << 8;
			i |= *p++;
			k = 0;
			break;
		}
		Wpilenum[w] = i;
		l = Pilebucket[i];
		i = strecpy(W[w], l->pile);
		Wp[w] = &W[w][i - 1];
		Wlen[w] = i;
		Whash[w] = l->hash;
		w++;
	}

	/* T cells. */

	p = (u_char *)pos;
	p += sizeof(POSITION);
	for (i = 0; i < Ntpiles; i++) {
		T[i] = *p++;
	}
}

static void printcard(card_t card, FILE *outfile)
{
       if (rank(card) == NONE) {
               fprintf(outfile, "   ");
       } else {
               fprintf(outfile, "%c%c ", Rank[rank(card)], Suit[suit(card)]);
       }
}

/* Win.  Print out the move stack. */

static void win(POSITION *pos)
{
	int i, nmoves;
	POSITION *p;
	MOVE *mp, **mpp, **mpp0;
	int count = 10;

	/* Go back up the chain of parents and store the moves
	in reverse order. */

	i = 0;
	for (p = pos; p->parent; p = p->parent) {
		i++;
	}
	nmoves = i;
	mpp0 = new_array(MOVE *, nmoves);
	if (mpp0 == NULL) {
		return; /* how sad, so close... */
	}
	mpp = mpp0 + nmoves - 1;
	for (p = pos; p->parent; p = p->parent) {
		*mpp-- = &p->move;
	}

	/* Now print them out in the correct order. */
	

	for (i = 0, mpp = mpp0; i < nmoves; i++, mpp++) {
	    --count;
	    if (count == 0)
		break;

		mp = *mpp;
		printcard(mp->card, stderr);
		if (mp->totype == T_TYPE) {
			fprintf(stderr, "to temp\n");
		} else if (mp->totype == O_TYPE) {
			fprintf(stderr, "out\n");
		} else {
			fprintf(stderr, "to ");
			if (mp->destcard == NONE) {
				fprintf(stderr, "empty pile");
			} else {
				printcard(mp->destcard, stderr);
			}
			fputc('\n', stderr);
		}
	}
	free_array(mpp0, MOVE *, nmoves);

}

/* Initialize the hash buckets. */

static void init_buckets(void)
{
	int i;

	/* Packed positions need 3 bytes for every 2 piles. */

	i = Nwpiles * 3;
	i >>= 1;
	i += Nwpiles & 0x1;
	Pilebytes = i;

	memset(Bucketlist, 0, sizeof(Bucketlist));
	Pilenum = 0;
	Treebytes = sizeof(TREE) + Pilebytes;

	/* In order to keep the TREE structure aligned, we need to add
	up to 7 bytes on Alpha or 3 bytes on Intel -- but this is still
	better than storing the TREE nodes and keys separately, as that
	requires a pointer.  On Intel for -f Treebytes winds up being
	a multiple of 8 currently anyway so it doesn't matter. */

#define ALIGN_BITS 0x7
	if (Treebytes & ALIGN_BITS) {
		Treebytes |= ALIGN_BITS;
		Treebytes++;
	}
	Posbytes = sizeof(POSITION) + Ntpiles;
	if (Posbytes & ALIGN_BITS) {
		Posbytes |= ALIGN_BITS;
		Posbytes++;
	}
}

/* For each pile, return a unique identifier.  Although there are a
large number of possible piles, generally fewer than 1000 different
piles appear in any given game.  We'll use the pile's hash to find
a hash bucket that contains a short list of piles, along with their
identifiers. */

static __inline__ int get_pilenum(int w)
{
	int bucket, pilenum;
	BUCKETLIST *l, *last;

	/* For a given pile, get its unique pile id.  If it doesn't have
	one, add it to the appropriate list and give it one.  First, get
	the hash bucket. */

	bucket = Whash[w] % NBUCKETS;

	/* Look for the pile in this bucket. */

	last = NULL;
	for (l = Bucketlist[bucket]; l; l = l->next) {
		if (l->hash == Whash[w] &&
		    strncmp((char*)l->pile, (char*)W[w], Wlen[w]) == 0) {
			break;
		}
		last = l;
	}

	/* If we didn't find it, make a new one and add it to the list. */

	if (l == NULL) {
		if (Pilenum == NPILES) {
		        fprintf(stderr, "Ran out of pile numbers!");
			return -1;
		}
		l = allocate(BUCKETLIST);
		if (l == NULL) {
			return -1;
		}
		l->pile = new_array(u_char, Wlen[w] + 1);
		if (l->pile == NULL) {
			free_ptr(l, BUCKETLIST);
			return -1;
		}

		/* Store the new pile along with its hash.  Maintain
		a reverse mapping so we can unpack the piles swiftly. */

		strncpy((char*)l->pile, (char*)W[w], Wlen[w] + 1);
		l->hash = Whash[w];
		l->pilenum = pilenum = Pilenum++;
		l->next = NULL;
		if (last == NULL) {
			Bucketlist[bucket] = l;
		} else {
			last->next = l;
		}
		Pilebucket[pilenum] = l;
	}

	return l->pilenum;
}

void free_buckets(void)
{
	int i, j;
	BUCKETLIST *l, *n;

	for (i = 0; i < NBUCKETS; i++) {
		l = Bucketlist[i];
		while (l) {
			n = l->next;
			j = strlen((char*)l->pile);    /* @@@ use block? */
			free_array(l->pile, u_char, j + 1);
			free_ptr(l, BUCKETLIST);
			l = n;
		}
	}
}

/* Solve patience games.  Prioritized breadth-first search.  Simple breadth-
first uses exponential memory.  Here the work queue is kept sorted to give
priority to positions with more cards out, so the solution found is not
guaranteed to be the shortest, but it'll be better than with a depth-first
search. */

#define NQUEUES 100

static POSITION *Qhead[NQUEUES]; /* separate queue for each priority */
static POSITION *Qtail[NQUEUES]; /* positions are added here */
static int Maxq;

static int solve(POSITION *);
static void free_position(POSITION *pos, int);
static void queue_position(POSITION *, int);
static POSITION *dequeue_position();

static void doit()
{
	int i, q;
	POSITION *pos;
	MOVE m;

	/* Init the queues. */

	for (i = 0; i < NQUEUES; i++) {
		Qhead[i] = NULL;
	}
	Maxq = 0;

	/* Queue the initial position to get started. */

	hash_layout();
	pilesort();
	m.card = NONE;
	pos = new_position(NULL, &m);
	if (pos == NULL) {
		return;
	}
	queue_position(pos, 0);

	/* Solve it. */

	while ((pos = dequeue_position()) != NULL) {
		q = solve(pos);
		if (!q) {
			free_position(pos, TRUE);
		}
	}
}

/* Generate all the successors to a position and either queue them or
recursively solve them.  Return whether any of the child nodes, or their
descendents, were queued or not (if not, the position can be freed). */

static int solve(POSITION *parent)
{
	int i, nmoves, q, qq;
	MOVE *mp, *mp0;
	POSITION *pos;

	/* If we've won already (or failed), we just go through the motions
	but always return FALSE from any position.  This enables the cleanup
	of the move stack and eventual destruction of the position store. */

	if (Status != NOSOL) {
		return FALSE;
	}

	/* If the position was found again in the tree by a shorter
	path, prune this path. */

	if (parent->node->depth < parent->depth) {
		return FALSE;
	}

	/* Generate an array of all the moves we can make. */

	if ((mp0 = get_moves(parent, &nmoves)) == NULL) {
		return FALSE;
	}
	parent->nchild = nmoves;

	/* Make each move and either solve or queue the result. */

	q = FALSE;
	for (i = 0, mp = mp0; i < nmoves; i++, mp++) {
		make_move(mp);

		/* Calculate indices for the new piles. */

		pilesort();

		/* See if this is a new position. */

		if ((pos = new_position(parent, mp)) == NULL) {
			undo_move(mp);
			parent->nchild--;
			continue;
		}

		/* If this position is in a new cluster, a card went out.
		Don't queue it, just keep going.  A larger cutoff can also
		force a recursive call, which can help speed things up (but
		reduces the quality of solutions).  Otherwise, save it for
		later. */

		if (pos->cluster != parent->cluster || nmoves < Cutoff) {
			qq = solve(pos);
			undo_move(mp);
			if (!qq) {
				free_position(pos, FALSE);
			}
			q |= qq;
		} else {
			queue_position(pos, mp->pri);
			undo_move(mp);
			q = TRUE;
		}
	}
	free_array(mp0, MOVE, nmoves);

	/* Return true if this position needs to be kept around. */

	return q;
}

/* We can't free the stored piles in the trees, but we can free some of the
POSITION structs.  We have to be careful, though, because there are many
threads running through the game tree starting from the queued positions.
The nchild element keeps track of descendents, and when there are none left
in the parent we can free it too after solve() returns and we get called
recursively (rec == TRUE). */

static void free_position(POSITION *pos, int rec)
{
	/* We don't really free anything here, we just push it onto a
	freelist (using the queue member), so we can use it again later. */

	if (!rec) {
		pos->queue = Freepos;
		Freepos = pos;
		pos->parent->nchild--;
	} else {
		do {
			pos->queue = Freepos;
			Freepos = pos;
			pos = pos->parent;
			if (pos == NULL) {
				return;
			}
			pos->nchild--;
		} while (pos->nchild == 0);
	}
}

/* Save positions for consideration later.  pri is the priority of the move
that got us here.  The work queue is kept sorted by priority (simply by
having separate queues). */

static void queue_position(POSITION *pos, int pri)
{
	int nout;
	double x;

	/* In addition to the priority of a move, a position gets an
	additional priority depending on the number of cards out.  We use a
	"queue squashing function" to map nout to priority.  */

	nout = O[0] + O[1] + O[2] + O[3];

	/* Yparam[0] * nout^2 + Yparam[1] * nout + Yparam[2] */

	x = (Yparam[0] * nout + Yparam[1]) * nout + Yparam[2];
	pri += (int)floor(x + .5);

	if (pri < 0) {
		pri = 0;
	} else if (pri >= NQUEUES) {
		pri = NQUEUES - 1;
	}
	if (pri > Maxq) {
		Maxq = pri;
	}

	/* We always dequeue from the head.  Here we either stick the move
	at the head or tail of the queue, depending on whether we're
	pretending it's a stack or a queue. */

	pos->queue = NULL;
	if (Qhead[pri] == NULL) {
		Qhead[pri] = pos;
		Qtail[pri] = pos;
	} else {
		if (Stack) {
			pos->queue = Qhead[pri];
			Qhead[pri] = pos;
		} else {
			Qtail[pri]->queue = pos;
			Qtail[pri] = pos;
		}
	}
}

/* Return the position on the head of the queue, or NULL if there isn't one. */

static POSITION *dequeue_position()
{
	int last;
	POSITION *pos;
	static int qpos = 0;
	static int minpos = 0;

	/* This is a kind of prioritized round robin.  We make sweeps
	through the queues, starting at the highest priority and
	working downwards; each time through the sweeps get longer.
	That way the highest priority queues get serviced the most,
	but we still get lots of low priority action (instead of
	ignoring it completely). */

	last = FALSE;
	do {
		qpos--;
		if (qpos < minpos) {
			if (last) {
				return NULL;
			}
			qpos = Maxq;
			minpos--;
			if (minpos < 0) {
				minpos = Maxq;
			}
			if (minpos == 0) {
				last = TRUE;
			}
		}
	} while (Qhead[qpos] == NULL);

	pos = Qhead[qpos];
	Qhead[qpos] = pos->queue;

	/* Decrease Maxq if that queue emptied. */

	while (Qhead[qpos] == NULL && qpos == Maxq && Maxq > 0) {
		Maxq--;
		qpos--;
		if (qpos < minpos) {
			minpos = qpos;
		}
	}

	/* Unpack the position into the work arrays. */

	unpack_position(pos);

	return pos;
}

static void play(void)
{
	/* Initialize the hash tables. */

	init_buckets();
	init_clusters();

	/* Reset stats. */

	Total_positions = 0;
	Total_generated = 0;
	Status = NOSOL;

	/* Go to it. */

	doit();
	free_buckets();
	free_clusters();
	free_blocks();
	Freepos = NULL;
}

static void read_layout(const char *text);

int patsolve(const char *text)
{
	Same_suit = FALSE;
	King_only = FALSE;
	Nwpiles = 8;
	Ntpiles = 4;

	/* Initialize the suitable() macro variables. */

	Suit_mask = PS_COLOR;
	Suit_val = PS_COLOR;
	if (Same_suit) {
		Suit_mask = PS_SUIT;
		Suit_val = 0;
	}

	read_layout(text);
	play();

	return Status;
}

static const char * next_line( const char *text)
{
    if (!text)
	return text;
    if (!text[0])
	return 0;
    text = strchr(text, '\n');
    if (text)
	text++;
    return text;
}

static int parse_pile(const char *s, card_t *w, int size)
{
	int i;
	char c;
	card_t rank, suit;

	if (!s)
	    return 0;

	i = 0;
	rank = suit = NONE;
	while (i < size && *s && *s != '\n' && *s != '\r') {
		while (*s == ' ') s++;
		c = toupper(*s);
		if (c == 'A') rank = 1;
		else if (c >= '2' && c <= '9') rank = c - '0';
		else if (c == 'T') rank = 10;
		else if (c == 'J') rank = 11;
		else if (c == 'Q') rank = 12;
		else if (c == 'K') rank = 13;
		s++;
		c = toupper(*s);
		if (c == 'C') suit = PS_CLUB;
		else if (c == 'D') suit = PS_DIAMOND;
		else if (c == 'H') suit = PS_HEART;
		else if (c == 'S') suit = PS_SPADE;
		s++;
		*w++ = suit + rank;
		i++;
		while (*s == ' ') s++;
	}
	return i;
}

/* Read a layout file.  Format is one pile per line, bottom to top (visible
card).  Temp cells and Out on the last two lines, if any. */

static void read_layout(const char *text)
{
	int w, i, total;
	card_t out[4];

	/* Read the workspace. */

	w = 0;
	total = 0;
	while (text) {
		i = parse_pile(text, W[w], 52);
		text = next_line(text);
		Wp[w] = &W[w][i - 1];
		Wlen[w] = i;
		w++;
		total += i;
		if (w == Nwpiles) {
			break;
		}
	}

	/* Temp cells may have some cards too. */

	for (i = 0; i < Ntpiles; i++) {
		T[i] = NONE;
	}
	if (total != 52) {
		total += parse_pile(text, T, Ntpiles);
		text = next_line(text);
	}

	/* Output piles, if any. */

	for (i = 0; i < 4; i++) {
		O[i] = out[i] = NONE;
	}
	if (total != 52) {
		parse_pile(text, out, 4);
		text = next_line(text);
		for (i = 0; i < 4; i++) {
			if (out[i] != NONE) {
				O[suit(out[i])] = rank(out[i]);
				total += rank(out[i]);
			}
		}
	}
}


/* Like strcpy() but return the length of the string. */

int strecpy(unsigned char *d, unsigned char *s)
{
	int i;

	i = 0;
	while ((*d++ = *s++) != '\0') {
		i++;
	}

	return i;
}

/* Allocate some space and return a pointer to it.  See new() in util.h. */

void *allocate_memory(size_t s)
{
	void *x;

	if (s > Mem_remain) {
		Status = FAIL;
		return NULL;
	}

	if ((x = (void *)malloc(s)) == NULL) {
		Status = FAIL;
		return NULL;
	}

	Mem_remain -= s;
	return x;
}


/* Given a cluster number, return a tree.  There are 14^4 possible
clusters, but we'll only use a few hundred of them at most.  Hash on
the cluster number, then locate its tree, creating it if necessary. */

#define TBUCKETS 499    /* a prime */

TREELIST *Treelist[TBUCKETS];

static int insert_node(TREE *n, int d, TREE **tree, TREE **node);
static TREELIST *cluster_tree(int cluster);
static void give_back_block(u_char *p);
static BLOCK *new_block(void);

static __inline__ int CMP(u_char *a, u_char *b)
{
	return memcmp(a, b, Pilebytes);
}

/* Insert key into the tree unless it's already there.  Return true if
it was new. */

static int insert(int *cluster, int d, TREE **node)
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

	bucket = cluster % TBUCKETS;

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
		tl = allocate(TREELIST);
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

	b = allocate(BLOCK);
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

	for (i = 0; i < TBUCKETS; i++) {
		l = Treelist[i];
		while (l) {
			n = l->next;
			free_ptr(l, TREELIST);
			l = n;
		}
	}
}
