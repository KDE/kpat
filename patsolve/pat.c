/* Common routines & arrays. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "pat.h"
#include "fnv.h"
#include "tree.h"

/* Default variation. */

#define SAME_SUIT 1
#define KING_ONLY 0
#define NWPILES 10      /* number of W piles */
#define NTPILES 4       /* number of T cells */

int Same_suit = SAME_SUIT;
int King_only = KING_ONLY;
int Nwpiles = NWPILES;
int Ntpiles = NTPILES;

card_t Suit_mask;
card_t Suit_val;

/* Names of the cards.  The ordering is defined in pat.h. */

char Rank[] = " A23456789TJQK";
char Suit[] = "DCHS";

card_t O[4];
card_t Osuit[4] = { PS_DIAMOND, PS_CLUB, PS_HEART, PS_SPADE };

/* Position freelist. */

POSITION *Freepos = NULL;

/* Work arrays. */

card_t T[MAXTPILES];
card_t W[MAXWPILES][52];
card_t *Wp[MAXWPILES];
int Wlen[MAXWPILES];
int Widx[MAXWPILES];
int Widxi[MAXWPILES];

/* Every different pile has a hash and a unique id. */

u_int32_t Whash[MAXWPILES];
int Wpilenum[MAXWPILES];

/* Temp storage for possible moves. */

MOVE Possible[MAXMOVES];

extern int Pilebytes;

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

void hash_layout(void)
{
	int w;

	for (w = 0; w < Nwpiles; w++) {
		hashpile(w);
	}
}

/* These two routines make and unmake moves. */

void make_move(MOVE *m)
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

void undo_move(MOVE *m)
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

MOVE *get_moves(POSITION *pos, int *nmoves)
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

POSITION *new_position(POSITION *parent, MOVE *m)
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

void pilesort(void)
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

TREE *pack_position(void)
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

void unpack_position(POSITION *pos)
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
void printcard(card_t card, FILE *outfile)
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

void init_buckets(void)
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
		    strncmp(l->pile, W[w], Wlen[w]) == 0) {
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
		l = new(BUCKETLIST);
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

		strncpy(l->pile, W[w], Wlen[w] + 1);
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
			j = strlen(l->pile);    /* @@@ use block? */
			free_array(l->pile, u_char, j + 1);
			free_ptr(l, BUCKETLIST);
			l = n;
		}
	}
}
