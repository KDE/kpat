/* Solve patience games.  Prioritized breadth-first search.  Simple breadth-
first uses exponential memory.  Here the work queue is kept sorted to give
priority to positions with more cards out, so the solution found is not
guaranteed to be the shortest, but it'll be better than with a depth-first
search. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "pat.h"
#include "fnv.h"

#define NQUEUES 100

static POSITION *Qhead[NQUEUES]; /* separate queue for each priority */
static POSITION *Qtail[NQUEUES]; /* positions are added here */
static int Maxq;

static int solve(POSITION *);
static void free_position(POSITION *pos, int);
static void queue_position(POSITION *, int);
static POSITION *dequeue_position();

#if DEBUG
int Clusternum[0x10000];
static int Inq[NQUEUES];

void print_queue(void)
{
	int i, n;

	msg("Maxq %d\n", Maxq);
	n = 0;
	for (i = 0; i <= Maxq; i++) {
		if (Inq[i]) {
			msg("Inq %2d %5d", i, Inq[i]);
			if (n & 1) {
				msg("\n");
			} else {
				msg("\t\t");
			}
			n++;
		}
	}
	msg("\n");
}
#endif

void doit()
{
	int i, q;
	POSITION *pos;
	MOVE m;
	extern void hash_layout(void);

	/* Init the queues. */

	for (i = 0; i < NQUEUES; i++) {
		Qhead[i] = NULL;
	}
	Maxq = 0;
#if DEBUG
memset(Clusternum, 0, sizeof(Clusternum));
memset(Inq, 0, sizeof(Inq));
#endif

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
#if DEBUG
Inq[pri]++;
Clusternum[pos->cluster]++;
#endif
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
#if DEBUG
	Inq[qpos]--;
#endif

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

#if DEBUG
Clusternum[pos->cluster]--;
#endif
	return pos;
}
