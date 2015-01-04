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

#include "patsolve.h"

#include "../patpile.h"

#include "KCardDeck"

#include <QDebug>

#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <sys/types.h>


#ifdef ERR
#undef ERR
#endif

long all_moves = 0;

/* This is a 32 bit FNV hash.  For more information, see
http://www.isthe.com/chongo/tech/comp/fnv/index.html */

#define FNV1_32_INIT 0x811C9DC5
#define FNV_32_PRIME 0x01000193

#define fnv_hash(x, hash) (((hash) * FNV_32_PRIME) ^ (x))

/* Hash a buffer. */

static inline quint32 fnv_hash_buf(quint8 *s, int len)
{
	int i;
	quint32 h;

	h = FNV1_32_INIT;
	for (i = 0; i < len; i++) {
		h = fnv_hash(*s++, h);
	}

	return h;
}

/* Hash a 0 terminated string. */

static inline quint32 fnv_hash_str(quint8 *s)
{
	quint32 h;

	h = FNV1_32_INIT;
	while (*s) {
		h = fnv_hash(*s++, h);
	}

	return h;
}


/* Hash a pile. */

void Solver::hashpile(int w)
{
   	W[w][Wlen[w]] = 0;
	Whash[w] = fnv_hash_str(W[w]);

	/* Invalidate this pile's id.  We'll calculate it later. */

	Wpilenum[w] = -1;
}

#define MAXDEPTH 400

bool Solver::recursive(POSITION *parent)
{
    int i, alln, a, numout = 0;

    if ( parent == NULL ) {
        init();
        recu_pos.clear();
        delete Stack;
        Stack = new POSITION[MAXDEPTH];
        memset( Stack, 0, sizeof( POSITION ) * MAXDEPTH );
    }

    /* Fill in the Possible array. */

    alln = get_possible_moves(&a, &numout);
    assert(alln < MAXMOVES);

    if (alln == 0) {
        if ( isWon() ) {
            Status = SolutionExists;
            Q_ASSERT(parent); // it just is never called with a won game
            win(parent);
            return true;
        }
        return false;
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

    if ( parent && parent->depth >= MAXDEPTH - 2 )
        return false;

    MOVE *mp0 = new_array(MOVE, alln+1);
    if (mp0 == NULL) {
        return false;
    }
    MOVE *mp = mp0;
    for (i = 0; i < alln; ++i) {
        if (Possible[i].card_index != -1) {
            *mp = Possible[i];      /* struct copy */
            mp++;
        }
    }
    mp->card_index = -1;
    ++alln;

    bool fit = false;
    for (mp = mp0; mp->card_index != -1; ++mp) {

        int depth = 0;
        if (parent != NULL)
            depth = parent->depth + 1;

        make_move(mp);

        /* Calculate indices for the new piles. */
        pilesort();

        /* See if this is a new position. */

        Total_generated++;
        POSITION *pos = &Stack[depth];
        pos->queue = NULL;
        pos->parent = parent;
        pos->node = pack_position();
        quint8 *key = (quint8 *)pos->node + sizeof(TREE);
#if 0
        qint32 hash = fnv_hash_buf(key, mm->Pilebytes);
        if ( recu_pos.contains( hash ) )
        {
            undo_move( mp );
            mm->give_back_block( (quint8 *)pos->node );
            continue;
        }
        recu_pos[hash] = true;
#else
        for ( int i = 0; i < depth; ++i )
        {
            quint8 *tkey = (quint8 *)Stack[i].node + sizeof(TREE);
            if ( !memcmp( key, tkey, mm->Pilebytes ) )
            {
                key = 0;
                break;
            }
        }
        if ( !key )
        {
            undo_move( mp );
            mm->give_back_block( (quint8 *)pos->node );
            continue;
        }
#endif
        Total_positions++;
        if ( Total_positions % 10000 == 0 )
            //qDebug() << "positions" << Total_positions;

        pos->move = *mp;                 /* struct copy */
        pos->cluster = 0;
        pos->depth = depth;
        pos->nchild = 0;

        bool ret = recursive(pos);
        fit |= ret;
        undo_move(mp);
        mm->give_back_block( (quint8 *)pos->node );
        if ( ret )
            break;
    }

    MemoryManager::free_array(mp0, alln);

    if ( parent == NULL ) {
        printf( "Total %ld\n", Total_generated );
        delete [] Stack;
        Stack = 0;
    }
    return fit;
}


/* Generate an array of the moves we can make from this position. */

MOVE *Solver::get_moves(int *nmoves)
{
	int i, n, alln, a = 0, numout = 0;
	MOVE *mp, *mp0;

	/* Fill in the Possible array. */

        alln = n = get_possible_moves(&a, &numout);
	if (debug)  
	  {
	    print_layout();
	    fprintf( stderr, "moves %d\n", n );
	    for (int j = 0; j < n; j++) {
	      fprintf( stderr,  "  " );
	      if ( Possible[j].totype == O_Type )
                fprintf( stderr, "move from %d out (at %d) Prio: %d\n", Possible[j].from,
                         Possible[j].turn_index, Possible[j].pri );
	      else
                fprintf( stderr, "move %d from %d to %d (%d) Prio: %d\n", Possible[j].card_index,
                         Possible[j].from, Possible[j].to,
                         Possible[j].turn_index, Possible[j].pri );
	    }
	  }

	/* No moves?  Maybe we won. */

	if (n == 0) {
            /* No more moves - won or lost */
            //print_layout();
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
		for (i = 0; i < alln; ++i) {
			if (Possible[i].card_index != -1) {
				*mp = Possible[i];      /* struct copy */
				mp++;
			}
		}
	} else {
		for (i = numout; i < alln; ++i) {
			if (Possible[i].card_index != -1) {
				*mp = Possible[i];      /* struct copy */
				mp++;
			}
		}
		for (i = 0; i < numout; ++i) {
			if (Possible[i].card_index != -1) {
				*mp = Possible[i];      /* struct copy */
				mp++;
			}
		}
	}

	return mp0;
}

/* Test the current position to see if it's new (or better).  If it is, save
it, along with the pointer to its parent and the move we used to get here. */

int Posbytes;

/* Comparison function for sorting the W piles. */

int Solver::wcmp(int a, int b)
{
	if (m_newer_piles_first) {
		return Wpilenum[b] - Wpilenum[a];       /* newer piles first */
	} else {
		return Wpilenum[a] - Wpilenum[b];       /* older piles first */
	}
}

void Solver::pilesort(void)
{
    	/* Make sure all the piles have id numbers. */

	for (int w = 0; w < m_number_piles; w++) {
		if (Wpilenum[w] < 0) {
			Wpilenum[w] = get_pilenum(w);
			if (Wpilenum[w] < 0) {
				return;
			}
		}
                //fprintf( stderr, "%d ", Wpilenum[w] );
	}
        //fprintf( stderr, "\n" );
}

#define NBUCKETS 65521           /* the largest 16 bit prime */
#define NPILES   65536           /* a 16 bit code */

typedef struct bucketlist {
	quint8 *pile;           /* 0 terminated copy of the pile */
	quint32 hash;         /* the pile's hash code */
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
where each pile number is packed into 16 bits (so a pile take 2 bytes).
Positions in this format are unique can be compared with memcmp().  The O
cells are encoded as a cluster number: no two positions with different
cluster numbers can ever be the same, so we store different clusters in
different trees.  */

int Treebytes;

TREE *Solver::pack_position(void)
{
	int j, k, w;
	quint8 *p;
	TREE *node;

	/* Allocate space and store the pile numbers.  The tree node
	will get filled in later, by insert_node(). */

	p = mm->new_from_block(Treebytes);
	if (p == NULL) {
                Status = UnableToDetermineSolvability;
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
        quint16 *p2 = ( quint16* ) p;
	for (w = 0; w < m_number_piles; ++w) {
		j = Wpilenum[w];
                if ( j < 0 )
                {
                    mm->give_back_block( p );
                    return NULL;
                }
                *p2++ = j;
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
	quint8 c;
	BUCKETLIST *l;

        unpack_cluster(pos->cluster);

	/* Unpack bytes p into pile numbers j.
		    p         p         p
		+--------+----:----+--------+
		|76543210|7654:3210|76543210|
		+--------+----:----+--------+
		       j             j
	*/

	k = w = i = c = 0;
	quint16 *p2 = ( quint16* )( (quint8 *)(pos->node) + sizeof(TREE) );
	while (w < m_number_piles) {
                i = *p2++;
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
        if ( DOWN(card ) )
            fprintf(outfile, "|%c%c ", Rank[RANK(card)], Suit[SUIT(card)]);
        else
            fprintf(outfile, "%c%c ", Rank[RANK(card)], Suit[SUIT(card)]);
    }
}

/* Win.  Print out the move stack. */

void Solver::win(POSITION *pos)
{
    int i, nmoves;
    POSITION *p;
    MOVE **mpp, **mpp0;

    /* Go back up the chain of parents and store the moves
       in reverse order. */

    i = 0;
    for (p = pos; p->parent; p = p->parent) {
        i++;
    }
    nmoves = i;

    //printf("Winning in %d moves.\n", nmoves);

    mpp0 = new_array(MOVE *, nmoves);
    if (mpp0 == NULL) {
        Status = UnableToDetermineSolvability;
        return; /* how sad, so close... */
    }
    mpp = mpp0 + nmoves - 1;
    for (p = pos; p->parent; p = p->parent) {
        *mpp-- = &p->move;
    }

    for (i = 0, mpp = mpp0; i < nmoves; ++i, ++mpp)
        winMoves.append( **mpp );

    MemoryManager::free_array(mpp0, nmoves);
}

/* Initialize the hash buckets. */

void Solver::init_buckets(void)
{
	int i;

	/* Packed positions need 3 bytes for every 2 piles. */

	i = ( m_number_piles ) * sizeof( quint16 );
	i += ( m_number_piles ) & 0x1;

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
	Posbytes = sizeof(POSITION);
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
		if (Pilenum >= NPILES ) {
                        Status = UnableToDetermineSolvability;
			//qDebug() << "out of piles";
			return -1;
		}
		l = mm_allocate(BUCKETLIST);
		if (l == NULL) {
                        Status = UnableToDetermineSolvability;
			//qDebug() << "out of buckets";
			return -1;
		}
		l->pile = new_array(quint8, Wlen[w] + 1);
		if (l->pile == NULL) {
                    Status = UnableToDetermineSolvability;
                    MemoryManager::free_ptr(l);
		    //qDebug() << "out of memory";
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

#if 0
if (w < 4) {
        fprintf( stderr, "get_pile_num %d ", l->pilenum );
        for (int i = 0; i < Wlen[w]; ++i) {
            printcard(W[w][i], stderr);
        }
        fprintf( stderr, "\n" );
}
#endif
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

	for (i = 0; i < NQUEUES; ++i) {
		Qhead[i] = NULL;
	}
	Maxq = 0;

	/* Queue the initial position to get started. */

	hash_layout();
	pilesort();
	m.card_index = -1;
        m.turn_index = -1;
	pos = new_position(NULL, &m);
	if ( pos == NULL )
        {
            Status = UnableToDetermineSolvability;
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
        all_moves++;

	/* If we've won already (or failed), we just go through the motions
	but always return false from any position.  This enables the cleanup
	of the move stack and eventual destruction of the position store. */

	if (Status != NoSolutionExists) {
		return false;
	}

        {
            QMutexLocker lock( &endMutex );
            if ( m_shouldEnd )
            {
                Status = SearchAborted;
                return false;
            }
        }

        if ( max_positions != -1 && Total_positions > ( unsigned long )max_positions )
        {
            Status = MemoryLimitReached;
            return false;
        }


	/* Generate an array of all the moves we can make. */

	if ((mp0 = get_moves(&nmoves)) == NULL) {
            if ( isWon() ) {
                Status = SolutionExists;
                win( parent );
            }
            return false;
	}

        if ( parent->depth == 0 )
        {
            Q_ASSERT( firstMoves.count() == 0 );
            for (int j = 0; j < nmoves; ++j)
                firstMoves.append( Possible[j] );
        }

	parent->nchild = nmoves;

	/* Make each move and either solve or queue the result. */

	q = false;
	for (i = 0, mp = mp0; i < nmoves; ++i, ++mp) {
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
			q |= (bool)qq;
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
	/* In addition to the priority of a move, a position gets an
	additional priority depending on the number of cards out.  We use a
	"queue squashing function" to map nout to priority.  */

        int nout = getOuts();

        static qreal Yparam[] = { 0.0032, 0.32, -3.0 };
	qreal x = (Yparam[0] * nout + Yparam[1]) * nout + Yparam[2];
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
    m_newer_piles_first = true;
    /* Work arrays. */
    W = 0;
    Wp = 0;

    Wlen = 0;

    Whash = 0;
    Wpilenum = 0;
    Stack = 0;
}

Solver::~Solver()
{
    delete mm;

    for ( int i = 0; i < m_number_piles; ++i )
    {
        delete [] W[i];
    }

    delete [] W;
    delete [] Wp;
    delete [] Wlen;
    delete [] Whash;
    delete [] Wpilenum;
}

void Solver::init()
{
    m_shouldEnd = false;
    init_buckets();
    mm->init_clusters();

    winMoves.clear();
    firstMoves.clear();

    /* Reset stats. */

    Status = NoSolutionExists;
    Total_positions = 0;
    Total_generated = 0;
    depth_sum = 0;
}

void Solver::free()
{
    free_buckets();
    mm->free_clusters();
    mm->free_blocks();
    Freepos = NULL;
}


Solver::ExitStatus Solver::patsolve( int _max_positions, bool _debug )
{
    max_positions = _max_positions;
    debug = _debug;

    /* Initialize the suitable() macro variables. */
    init();

    /* Go to it. */
    doit();

    if ( Status == SearchAborted ) // thread quit
    {
        firstMoves.clear();
        winMoves.clear();
    }
#if 0
    printf("%ld positions generated (%f).\n", Total_generated, depth_sum / Total_positions);
    printf("%ld unique positions.\n", Total_positions);
    printf("Mem_remain = %ld\n", ( long int )mm->Mem_remain);
#endif
    free();
    return Status;
}

void Solver::print_layout()
{
}

void Solver::setNumberPiles( int p )
{
    m_number_piles = p;

    /* Work arrays. */
    W = new card_t*[m_number_piles];
    for ( int i = 0; i < m_number_piles; ++i )
    {
        W[i] = new card_t[84];
        memset( W[i], 0, sizeof( card_t ) * 84 );
    }
    Wp = new card_t*[m_number_piles];

    Wlen = new int[m_number_piles];

    Whash = new quint32[m_number_piles];
    Wpilenum = new int[m_number_piles];
    memset( Wpilenum, 0, sizeof( int ) * m_number_piles );
}

int Solver::translateSuit( int s )
{
    int suit = s * 0x10;
    if ( suit == PS_DIAMOND )
        return PS_CLUB;
    else if ( suit == PS_CLUB )
        return PS_DIAMOND;
    return suit;
}

int Solver::translate_pile(const KCardPile *pile, card_t *w, int size)
{
    Q_UNUSED( size );
        Q_ASSERT( pile->count() <= size );

        card_t rank, suit;

	rank = suit = NONE;
        for ( int i = 0; i < pile->count(); ++i )
        {
            KCard *c = pile->at( i );
            *w =  + translateSuit( c->suit() ) + c->rank();
            if ( !c->isFaceUp() )
                *w += 1 << 7;
            w++;
	}
	return pile->count();
}

/* Insert key into the tree unless it's already there.  Return true if
it was new. */

MemoryManager::inscode Solver::insert(unsigned int *cluster, int d, TREE **node)
{
	/* Get the cluster number from the Out cell contents. */

        unsigned int k = getClusterNumber();
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
        Total_generated++;

        MemoryManager::inscode i2 = mm->insert_node(newtree, d, &tl->tree, node);

	if (i2 != MemoryManager::NEW) {
		mm->give_back_block((quint8 *)newtree);
	}

	return i2;
}


POSITION *Solver::new_position(POSITION *parent, MOVE *m)
{
	unsigned int depth, cluster;
	quint8 *p;
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
        if (i == MemoryManager::NEW) {
                Total_positions++;
                depth_sum += depth;
        } else
            return NULL;


	/* A new or better position.  insert() already stashed it in the
	tree, we just have to wrap a POSITION struct around it, and link it
	into the move stack.  Store the temp cells after the POSITION. */

	if (Freepos) {
		p = (quint8 *)Freepos;
		Freepos = Freepos->queue;
	} else {
		p = mm->new_from_block(Posbytes);
		if (p == NULL) {
                        Status = UnableToDetermineSolvability;
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
#if 0
        QString dummy;
        quint16 *t = ( quint16* )( ( char* )node + sizeof( TREE ) );
        for ( int i = 0; i < m_number_piles; ++i )
        {
            QString s = "      " + QString( "%1" ).arg( ( int )t[i] );
            dummy += s.right( 5 );
        }
        if ( Total_positions % 1000 == 1000 )
            print_layout();
        //qDebug() << "new" << dummy;
#endif
	p += sizeof(POSITION);
	return pos;
}

/* Hash the whole layout.  This is called once, at the start. */

void Solver::hash_layout(void)
{
	int w;

	for (w = 0; w < m_number_piles; w++) {
		hashpile(w);
	}
}

void Solver::prioritize(MOVE *, int )
{
}
