/* Common routines & arrays. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <stdarg.h>
#include <math.h>
#include "patsolve.h"
#include "../freecell.h"
#include "../pile.h"
#include "memory.h"

/* Statistics. */

int Solver::Xparam[] = { 4, 1, 8, -1, 7, 11, 4, 2, 2, 1, 2 };
double Solver::Yparam[] = { 0.0032, 0.32, -3.0 };

/* Misc. */

#define PS_DIAMOND 0x00         /* red */
#define PS_CLUB    0x10         /* black */
#define PS_HEART   0x20         /* red */
#define PS_SPADE   0x30         /* black */
#define PS_COLOR   0x10         /* black if set */
#define PS_SUIT    0x30         /* mask both suit bits */

#define NONE    0
#define PS_ACE  1
#define PS_KING 13

#define RANK(card) ((card) & 0xF)
#define SUIT(card) ((card) >> 4)
#define COLOR(card) ((card) & PS_COLOR)

/* Some macros used in get_possible_moves(). */

/* The following macro implements
	(Same_suit ? (suit(a) == suit(b)) : (color(a) != color(b)))
*/
#define suitable(a, b) ((((a) ^ (b)) & Suit_mask) == Suit_val)
static card_t Suit_mask;
static card_t Suit_val;

#define king_only(card) (!King_only || RANK(card) == PS_KING)

/* A splay tree. */

/* Command line flags. */


/* This is a 32 bit FNV hash.  For more information, see
http://www.isthe.com/chongo/tech/comp/fnv/index.html */

#include <sys/types.h>

#define FNV1_32_INIT 0x811C9DC5
#define FNV_32_PRIME 0x01000193

#define fnv_hash(x, hash) (((hash) * FNV_32_PRIME) ^ (x))

/* Hash a buffer. */

static inline u_int32_t fnv_hash_buf(u_char *s, int len)
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

static inline u_int32_t fnv_hash_str(u_char *s)
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

/* Default variation. */

#define SAME_SUIT 1
#define KING_ONLY 0
#define NWPILES 10      /* number of W piles */
#define NTPILES 4       /* number of T cells */

static int Same_suit = SAME_SUIT;
static int King_only = KING_ONLY;

/* Temp storage for possible moves. */

/* Hash a pile. */

void Solver::hashpile(int w)
{
	W[w][Wlen[w]] = 0;
	Whash[w] = fnv_hash_str(W[w]);

	/* Invalidate this pile's id.  We'll calculate it later. */

	Wpilenum[w] = -1;
}

/* Hash the whole layout.  This is called once, at the start. */

void Solver::hash_layout(void)
{
	int w;

	for (w = 0; w < Nwpiles+Ntpiles; w++) {
		hashpile(w);
	}
}

/* These two routines make and unmake moves. */

void Solver::make_move(MOVE *m)
{
	int from, to;
	card_t card;

	from = m->from;
	to = m->to;

	/* Remove from pile. */

        card = *Wp[from]--;
        Wlen[from]--;
        hashpile(from);

	/* Add to pile. */

	if (m->totype == O_Type) {
            O[to]++;
        } else {
            *++Wp[to] = card;
            Wlen[to]++;
            hashpile(to);
	}
}

void Solver::undo_move(MOVE *m)
{
	int from, to;
	card_t card;

	from = m->from;
	to = m->to;

	/* Remove from 'to' pile. */

	if (m->totype == O_Type) {
            card = O[to] + Osuit[to];
            O[to]--;
        } else {
            card = *Wp[to]--;
            Wlen[to]--;
            hashpile(to);
	}

	/* Add to 'from' pile. */
        *++Wp[from] = card;
        Wlen[from]++;
        hashpile(from);
}

/* Move prioritization.  Given legal, pruned moves, there are still some
that are a waste of time, especially in the endgame where there are lots of
possible moves, but few productive ones.  Note that we also prioritize
positions when they are added to the queue. */

#define NNEED 8

void Solver::prioritize(MOVE *mp0, int n)
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
			s = SUIT(card);

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
			if (mp->fromtype == W_Type) {
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
			if (mp->totype == W_Type) {
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

MOVE *Solver::get_moves(int *nmoves)
{
	int i, n, alln, o, a, numout = 0;
	MOVE *mp, *mp0;

	/* Fill in the Possible array. */

	alln = n = get_possible_moves(&a, &numout);
#if 0
        fprintf( stderr, "moves %d\n", n );
	for (int j = 0; j < n; j++) {
            printcard( Possible[j].card, stderr );
            fprintf( stderr, "move %d %d\n", Possible[j].from, Possible[j].to );
        }
        fprintf( stderr, "done\n" );
#endif

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

int Solver::good_automove(int o, int r)
{
	int i;

	if (Same_suit || r <= 2) {
		return true;
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
					return false;
				}
			}
			if (O[(o + 2) & 3] < r - 3) {
				return false;
			}

			return true;
#else   /* Horne's Rule */
			return false;
#endif
		}
	}

	return true;
}

/* Get the possible moves from a position, and store them in Possible[]. */

int Solver::get_possible_moves(int *a, int *numout)
{
	int i, n, t, w, o, empty, emptyw;
	card_t card;
	MOVE *mp;

	/* Check for moves from W to O. */

	n = 0;
	mp = Possible;
	for (w = 0; w < Nwpiles + Ntpiles; w++) {
		if (Wlen[w] > 0) {
			card = *Wp[w];
			o = SUIT(card);
			empty = (O[o] == NONE);
			if ((empty && (RANK(card) == PS_ACE)) ||
			    (!empty && (RANK(card) == O[o] + 1))) {
				mp->card = card;
				mp->from = w;
				mp->fromtype = W_Type;
				mp->to = o;
				mp->totype = O_Type;
				mp->srccard = NONE;
				if (Wlen[w] > 1) {
					mp->srccard = Wp[w][-1];
				}
				mp->destcard = NONE;
				mp->pri = 0;    /* unused */
				n++;
				mp++;

				/* If it's an automove, just do it. */

				if (good_automove(o, RANK(card))) {
					*a = true;
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

	*a = false;
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
		for (i = 0; i < Nwpiles + Ntpiles; i++) {
			if (i == emptyw || Wlen[i] == 0) {
				continue;
			}
                        bool allowed = false;
                        if ( i < Nwpiles && king_only(*Wp[i]) )
                            allowed = true;
                        if ( i >= Nwpiles )
                            allowed = true;
                        if ( allowed ) {
				card = *Wp[i];
				mp->card = card;
				mp->from = i;
				mp->fromtype = W_Type;
				mp->to = emptyw;
				mp->totype = W_Type;
				mp->srccard = Wp[i][-1];
				mp->destcard = NONE;
                                if ( i >= Nwpiles )
                                    mp->pri = Xparam[6];
                                else
                                    mp->pri = Xparam[3];
				n++;
				mp++;
			}
		}
	}

	/* Check for moves from W to non-empty W cells. */

	for (i = 0; i < Nwpiles + Ntpiles; i++) {
		if (Wlen[i] > 0) {
			card = *Wp[i];
			for (w = 0; w < Nwpiles; w++) {
				if (i == w) {
					continue;
				}
				if (Wlen[w] > 0 &&
				    (RANK(card) == RANK(*Wp[w]) - 1 &&
				     suitable(card, *Wp[w]))) {
					mp->card = card;
					mp->from = i;
					mp->fromtype = W_Type;
					mp->to = w;
					mp->totype = W_Type;
					mp->srccard = NONE;
					if (Wlen[i] > 1) {
						mp->srccard = Wp[i][-1];
					}
					mp->destcard = *Wp[w];
                                        if ( i >= Nwpiles )
                                            mp->pri = Xparam[5];
                                        else
                                            mp->pri = Xparam[4];
					n++;
					mp++;
				}
			}
		}
	}

        /* Check for moves from W to one of any empty T cells. */

        for (t = 0; t < Ntpiles; t++) {
               if (!Wlen[t+Nwpiles]) {
                       break;
               }
        }

        if (t < Ntpiles) {
               for (w = 0; w < Nwpiles; w++) {
                       if (Wlen[w] > 0) {
                               card = *Wp[w];
                               mp->card = card;
                               mp->from = w;
                               mp->fromtype = W_Type;
                               mp->to = t+Nwpiles;
                               mp->totype = W_Type;
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

void Solver::mark_irreversible(int n)
{
	int i, irr;
	card_t card, srccard;
	MOVE *mp;

	for (i = 0, mp = Possible; i < n; i++, mp++) {
		irr = false;
		if (mp->totype == O_Type) {
			irr = true;
		} else if (mp->fromtype == W_Type) {
			srccard = mp->srccard;
			if (srccard != NONE) {
				card = mp->card;
				if (RANK(card) != RANK(srccard) - 1 ||
				    !suitable(card, srccard)) {
					irr = true;
				}
			} else if (King_only && mp->card != PS_KING) {
				irr = true;
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

/* Comparison function for sorting the W piles. */

int Solver::wcmp(int a, int b)
{
	if (Xparam[9] < 0) {
		return Wpilenum[b] - Wpilenum[a];       /* newer piles first */
	} else {
		return Wpilenum[a] - Wpilenum[b];       /* older piles first */
	}
}

void Solver::pilesort(void)
{
	/* Make sure all the piles have id numbers. */

	for (int w = 0; w < Nwpiles+Ntpiles; w++) {
		if (Wpilenum[w] < 0) {
			Wpilenum[w] = get_pilenum(w);
			if (Wpilenum[w] < 0) {
				return;
			}
		}
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

TREE *Solver::pack_position(void)
{
	int j, k, w;
	u_char *p;
	TREE *node;

	/* Allocate space and store the pile numbers.  The tree node
	will get filled in later, by insert_node(). */

	p = mm->new_from_block(Treebytes);
	if (p == NULL) {
                Status = FAIL;
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
	for (w = 0; w < Nwpiles+Ntpiles; w++) {
		j = Wpilenum[w];
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

/* Like strcpy() but return the length of the string. */

static inline int strecpy(unsigned char *d, unsigned char *s)
{
	int i;

	i = 0;
	while ((*d++ = *s++) != '\0') {
		i++;
	}

	return i;
}

/* Unpack a compact position rep.  T cells must be restored from the
array following the POSITION struct. */

void Solver::unpack_position(POSITION *pos)
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
	while (w < Nwpiles+Ntpiles) {
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
}

void Solver::printcard(card_t card, FILE *outfile)
{
    static char Rank[] = " A23456789TJQK";
    static char Suit[] = "DCHS";

    if (RANK(card) == NONE) {
        fprintf(outfile, "   ");
    } else {
        fprintf(outfile, "%c%c ", Rank[RANK(card)], Suit[SUIT(card)]);
    }
}

/* Win.  Print out the move stack. */

void Solver::win(POSITION *pos)
{
	int i, nmoves;
	POSITION *p;
	MOVE *mp, **mpp, **mpp0;

	/* Go back up the chain of parents and store the moves
	in reverse order. */

	i = 0;
	for (p = pos; p->parent; p = p->parent) {
		i++;
	}
	nmoves = i;
	mpp0 = new_array(MOVE *, nmoves);
	if (mpp0 == NULL) {
                Status = FAIL;
		return; /* how sad, so close... */
	}
	mpp = mpp0 + nmoves - 1;
	for (p = pos; p->parent; p = p->parent) {
		*mpp-- = &p->move;
	}

        mp = *mpp0;
        Card *c = 0;
        if ( mp->from >= Nwpiles)
            c = deal->freecell[mp->from - Nwpiles]->top();
        else if ( mp->fromtype == W_Type )
            c = deal->store[mp->from]->top();

        if ( !c )
            abort();

        //printcard(mp->card, stderr);
        //fprintf( stderr, "%d %d %d %d %02x\n", mp->fromtype, mp->totype, mp->from, mp->to, mp->card );
        Pile *pile = 0;
        if (mp->to >= Nwpiles) {
            pile = deal->freecell[mp->to - Nwpiles];
        } else if (mp->totype == O_Type) {
            for ( int i = 0; i < 4; ++i )
                if ( c->rank() == Card::Ace && deal->target[i]->isEmpty() )
                {
                    pile = deal->target[i];
                    break;
                }
                else if ( !deal->target[i]->isEmpty() )
                {
                    Card *t = deal->target[i]->top();
                    if ( t->rank() == c->rank() - 1 && t->suit() == c->suit() )
                    {
                        pile = deal->target[i];
                        break;
                    }
                }
        } else {
            pile = deal->store[mp->to];
        }

        if ( !pile )
            abort();

        delete first;
        first = new MoveHint( c, pile );
        return;

#if 0
	/* Now print them out in the correct order. */
        int count = 2;

	for (i = 0, mpp = mpp0; i < nmoves; i++, mpp++) {
	    --count;
	    if (count == 0)
		break;

		mp = *mpp;
		printcard(mp->card, stderr);
		if (mp->totype == T_Type) {
			fprintf(stderr, "to temp %d\n",  mp->to);
		} else if (mp->totype == O_Type) {
			fprintf(stderr, "out\n");
		} else {
			fprintf(stderr, "to ");
			if (mp->destcard == NONE) {
				fprintf(stderr, "empty pile %d", mp->to);
			} else {
				printcard(mp->destcard, stderr);
                                fprintf( stderr, " %d", mp->to );
			}
			fputc('\n', stderr);
		}
	}
#endif
        MemoryManager::free_array(mpp0, nmoves);

}

/* Initialize the hash buckets. */

void Solver::init_buckets(void)
{
	int i;

	/* Packed positions need 3 bytes for every 2 piles. */

	i = ( Nwpiles + Ntpiles ) * 3;
	i >>= 1;
	i += ( Nwpiles + Ntpiles ) & 0x1;

        mm->Pilebytes = i;

	memset(Bucketlist, 0, sizeof(Bucketlist));
	Pilenum = 0;
	Treebytes = sizeof(TREE) + mm->Pilebytes;

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

int Solver::get_pilenum(int w)
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
                        Status = FAIL;
			return -1;
		}
		l->pile = new_array(u_char, Wlen[w] + 1);
		if (l->pile == NULL) {
                    Status = FAIL;
                    MemoryManager::free_ptr(l);
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

        //fprintf( stderr, "get_pile_num %d %d\n", w, l->pilenum );
	return l->pilenum;
}

void Solver::free_buckets(void)
{
	int i, j;
	BUCKETLIST *l, *n;

	for (i = 0; i < NBUCKETS; i++) {
		l = Bucketlist[i];
		while (l) {
			n = l->next;
			j = strlen((char*)l->pile);    /* @@@ use block? */
                        MemoryManager::free_array(l->pile, j + 1);
                        MemoryManager::free_ptr(l);
			l = n;
		}
	}
}

/* Solve patience games.  Prioritized breadth-first search.  Simple breadth-
first uses exponential memory.  Here the work queue is kept sorted to give
priority to positions with more cards out, so the solution found is not
guaranteed to be the shortest, but it'll be better than with a depth-first
search. */

void Solver::doit()
{
	int i, q;
	POSITION *pos;
	MOVE m;
        memset( &m, 0, sizeof( MOVE ) );

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
                Status = FAIL;
		return;
	}
	queue_position(pos, 0);

	/* Solve it. */

        while ((pos = dequeue_position()) != NULL) {
		q = solve(pos);
		if (!q) {
                    free_position(pos, true);
		}
	}
}

/* Generate all the successors to a position and either queue them or
recursively solve them.  Return whether any of the child nodes, or their
descendents, were queued or not (if not, the position can be freed). */

bool Solver::solve(POSITION *parent)
{
	int i, nmoves, qq;
	MOVE *mp, *mp0;
	POSITION *pos;

        bool q;
	/* If we've won already (or failed), we just go through the motions
	but always return false from any position.  This enables the cleanup
	of the move stack and eventual destruction of the position store. */

	if (Status != NOSOL) {
		return false;
	}

	/* If the position was found again in the tree by a shorter
	path, prune this path. */

	if (parent->node->depth < parent->depth) {
		return false;
	}

	/* Generate an array of all the moves we can make. */

	if ((mp0 = get_moves(&nmoves)) == NULL) {
            if ( Status == WIN )
                win( parent );
            return false;
	}
	parent->nchild = nmoves;

	/* Make each move and either solve or queue the result. */

	q = false;
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

		if (pos->cluster != parent->cluster || !nmoves) {
			qq = solve(pos);
			undo_move(mp);
			if (!qq) {
				free_position(pos, false);
			}
			q |= qq;
		} else {
			queue_position(pos, mp->pri);
			undo_move(mp);
			q = true;
		}
	}
        MemoryManager::free_array(mp0, nmoves);

	/* Return true if this position needs to be kept around. */

	return q;
}

/* We can't free the stored piles in the trees, but we can free some of the
POSITION structs.  We have to be careful, though, because there are many
threads running through the game tree starting from the queued positions.
The nchild element keeps track of descendents, and when there are none left
in the parent we can free it too after solve() returns and we get called
recursively (rec == true). */

void Solver::free_position(POSITION *pos, int rec)
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

void Solver::queue_position(POSITION *pos, int pri)
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
	} else {
            pos->queue = Qhead[pri];
            Qhead[pri] = pos;
	}
}

/* Return the position on the head of the queue, or NULL if there isn't one. */

POSITION *Solver::dequeue_position()
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

	last = false;
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
				last = true;
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

Solver::Solver()
{
    mm = new MemoryManager();
    Freepos = NULL;
    Osuit[0] = PS_DIAMOND;
    Osuit[1] = PS_CLUB;
    Osuit[2] = PS_HEART;
    Osuit[3] = PS_SPADE;
}

Solver::~Solver()
{
    delete mm;
}

void Solver::play(void)
{
	/* Initialize the hash tables. */

	init_buckets();
	mm->init_clusters();

	/* Reset stats. */

	Status = NOSOL;

	/* Go to it. */

	doit();
	free_buckets();
	mm->free_clusters();
	mm->free_blocks();
	Freepos = NULL;
}

statuscode Solver::patsolve(const Freecell *dealer)
{
    first = 0;
    deal = dealer;
    Same_suit = false;
    King_only = false;
    Nwpiles = 8;
    Ntpiles = 4;

    memset( W, 0, sizeof( W ) );
    /* Initialize the suitable() macro variables. */

    Suit_mask = PS_COLOR;
    Suit_val = PS_COLOR;
    if (Same_suit) {
        Suit_mask = PS_SUIT;
        Suit_val = 0;
    }

    translate_layout(dealer);
    //print_layout();
    play();

    return Status;
}

static int translate_pile(const Pile *pile, card_t *w, int size)
{
    Q_ASSERT( pile->cardsLeft() <= size );

        card_t rank, suit;

	rank = suit = NONE;
        for ( int i = 0; i < pile->cardsLeft(); ++i )
        {
            *w = pile->at( i )->suit() * 0x10 + pile->at( i )->rank();
            w++;
	}
	return pile->cardsLeft();
}

/* Read a layout file.  Format is one pile per line, bottom to top (visible
card).  Temp cells and Out on the last two lines, if any. */

void Solver::translate_layout(const Freecell *deal)
{
	/* Read the workspace. */

	int total = 0;
	for ( int w = 0; w < 8; ++w ) {
		int i = translate_pile(deal->store[w], W[w], 52);
		Wp[w] = &W[w][i - 1];
		Wlen[w] = i;
		total += i;
		if (w == Nwpiles) {
			break;
		}
	}

	/* Temp cells may have some cards too. */

	for (int w = 0; w < Ntpiles; w++)
        {
            int i = translate_pile( deal->freecell[w], W[w+Nwpiles], 52 );
            Wp[w+Nwpiles] = &W[w+Nwpiles][i-1];
            Wlen[w+Nwpiles] = i;
            total += i;
	}

	/* Output piles, if any. */
	for (int i = 0; i < 4; i++) {
		O[i] = NONE;
	}
	if (total != 52) {
            for (int i = 0; i < 4; i++) {
                Card *c = deal->target[i]->top();
                if (c) {
                    O[c->suit()] = c->rank();
                    total += c->rank();
                }
            }
	}
}

/* Insert key into the tree unless it's already there.  Return true if
it was new. */

MemoryManager::inscode Solver::insert(int *cluster, int d, TREE **node)
{
	int i, k;

	/* Get the cluster number from the Out cell contents. */

	i = O[0] + (O[1] << 4);
	k = i;
	i = O[2] + (O[3] << 4);
	k |= i << 8;

        *cluster = k;

        /* Get the tree for this cluster. */

	TREELIST *tl = mm->cluster_tree(k);
	if (tl == NULL) {
		return MemoryManager::ERR;
	}

	/* Create a compact position representation. */

	TREE *newtree = pack_position();
	if (newtree == NULL) {
		return MemoryManager::ERR;
	}

        MemoryManager::inscode i2 = mm->insert_node(newtree, d, &tl->tree, node);

	if (i2 != MemoryManager::NEW) {
		mm->give_back_block((u_char *)newtree);
	}

	return i2;
}

void Solver::print_layout()
{
       int i, t, w, o;

       fprintf(stderr, "print-layout-begin\n");
       for (w = 0; w < Nwpiles; w++) {
               for (i = 0; i < Wlen[w]; i++) {
                       printcard(W[w][i], stderr);
               }
               fputc('\n', stderr);
       }
       for (t = 0; t < Ntpiles; t++) {
           printcard(W[t+Nwpiles][Wlen[t+Nwpiles]], stderr);
       }
       fprintf( stderr, "\n" );
       for (o = 0; o < 4; o++) {
               printcard(O[o] + Osuit[o], stderr);
       }
       fprintf(stderr, "\nprint-layout-end\n");
}


POSITION *Solver::new_position(POSITION *parent, MOVE *m)
{
	int depth, cluster;
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
        MemoryManager::inscode i = insert(&cluster, depth, &node);
	if (i != MemoryManager::NEW && i != MemoryManager::FOUND_BETTER) {
		return NULL;
	}

	/* A new or better position.  insert() already stashed it in the
	tree, we just have to wrap a POSITION struct around it, and link it
	into the move stack.  Store the temp cells after the POSITION. */

	if (Freepos) {
		p = (u_char *)Freepos;
		Freepos = Freepos->queue;
	} else {
		p = mm->new_from_block(Posbytes);
		if (p == NULL) {
                        Status = FAIL;
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
/*	for (int t = 0; t < Ntpiles; t++) {
		*p++ = T[t];
	}
*/
	return pos;
}

