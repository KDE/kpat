#ifndef PATSOLVE_H
#define PATSOLVE_H

#include "../hint.h"
#include "memory.h"

class Freecell;

/* A card is represented as (suit << 4) + rank. */

typedef u_char card_t;

/* Represent a move. */

enum PileType { O_Type = 1, T_Type, W_Type };

typedef struct {
	card_t card;            /* the card we're moving */
	u_char from;            /* from pile number */
	u_char to;              /* to pile number */
        PileType fromtype;        /* O, T, or W */
	PileType totype;
	card_t srccard;         /* card we're uncovering */
	card_t destcard;        /* card we're moving to */
	signed char pri;        /* move priority (low priority == low value) */
} MOVE;

struct POSITION;

struct POSITION {
        POSITION *queue;      /* next position in the queue */
	POSITION *parent;     /* point back up the move stack */
	TREE *node;             /* compact position rep.'s tree node */
	MOVE move;              /* move that got us here from the parent */
	unsigned short cluster; /* the cluster this node is in */
	short depth;            /* number of moves so far */
	u_char ntemp;           /* number of cards in T */
	u_char nchild;          /* number of child nodes left */
};

enum statuscode { FAIL = -1, WIN = 0, NOSOL = 1 };

class MemoryManager;

class Solver
{
public:
    Solver();
    virtual ~Solver();
    statuscode patsolve(const Freecell *text);
    MoveHint *first;

protected:
    MOVE *get_moves(int *nmoves);
    bool solve(POSITION *parent);
    void doit();
    void play(void);
    void translate_layout(const Freecell *deal);
    void win(POSITION *pos);
    void prioritize(MOVE *mp0, int n);
    int get_possible_moves(int *a, int *numout);
    void mark_irreversible(int n);

    int wcmp(int a, int b);
    void queue_position(POSITION *pos, int pri);
    void free_position(POSITION *pos, int);
    POSITION *dequeue_position();
    void hashpile(int w);
    void hash_layout(void);
    void make_move(MOVE *m);
    void undo_move(MOVE *m);
    POSITION *new_position(POSITION *parent, MOVE *m);
    TREE *pack_position(void);
    void unpack_position(POSITION *pos);
    void init_buckets(void);
    int get_pilenum(int w);
    MemoryManager::inscode insert(int *cluster, int d, TREE **node);
    void print_layout();
    void free_buckets(void);

    void pilesort(void);
    static int Xparam[];
    static double Yparam[];

    const Freecell *deal;

#define MAXTPILES       4       /* max number of piles */
#define MAXWPILES       8

    /* Work arrays. */

    card_t T[MAXTPILES];     /* one card in each temp cell */
    card_t W[MAXWPILES][52]; /* the workspace */
    card_t *Wp[MAXWPILES];   /* point to the top card of each work pile */
    int Wlen[MAXWPILES];     /* the number of cards in each pile */

    /* Every different pile has a hash and a unique id. */
    u_int32_t Whash[MAXWPILES];
    int Wpilenum[MAXWPILES];

#define MAXMOVES 64             /* > max # moves from any position */
    MOVE Possible[MAXMOVES];

    MemoryManager *mm;
    statuscode Status;             /* win, lose, or fail */
};

#endif
