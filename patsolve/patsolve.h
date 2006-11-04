#ifndef PATSOLVE_H
#define PATSOLVE_H

#include "../hint.h"
#include "memory.h"
#include <stdio.h>

class DealerScene;

/* A card is represented as ( down << 6 ) + (suit << 4) + rank. */

typedef u_char card_t;

/* Represent a move. */

enum PileType { O_Type = 1, W_Type };

typedef struct {
    card_t card;            /* the card we're moving */
    u_char from;            /* from pile number */
    u_char to;              /* to pile number */
    PileType totype;
    signed char pri;        /* move priority (low priority == low value) */
    bool turn;              /* turn the card */
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
    statuscode patsolve();

protected:
    MOVE *get_moves(int *nmoves);
    bool solve(POSITION *parent);
    void doit();
    void play(void);
    void win(POSITION *pos);
    virtual int get_possible_moves(int *a, int *numout) = 0;

    int wcmp(int a, int b);
    void queue_position(POSITION *pos, int pri);
    void free_position(POSITION *pos, int);
    POSITION *dequeue_position();
    void hashpile(int w);
    POSITION *new_position(POSITION *parent, MOVE *m);
    TREE *pack_position(void);
    void unpack_position(POSITION *pos);
    void init_buckets(void);
    int get_pilenum(int w);
    MemoryManager::inscode insert(int *cluster, int d, TREE **node);
    void free_buckets(void);
    void printcard(card_t card, FILE *outfile);
    int translate_pile(const Pile *pile, card_t *w, int size);
    virtual bool print_layout();

    void pilesort(void);
    void hash_layout( void );
    virtual void make_move(MOVE *m) = 0;
    virtual void undo_move(MOVE *m) = 0;
    virtual void prioritize(MOVE *mp0, int n) = 0;
    virtual bool isWon() = 0;
    virtual int getOuts() = 0;
    virtual int getClusterNumber() = 0;
    virtual void translate_layout() = 0;
    virtual void unpack_cluster( int k ) = 0;

    void setNumberPiles( int i );
    int m_number_piles;

    /* Work arrays. */

    card_t **W; /* the workspace */
    card_t **Wp;   /* point to the top card of each work pile */
    int *Wlen;     /* the number of cards in each pile */

    /* Every different pile has a hash and a unique id. */
    u_int32_t *Whash;
    int *Wpilenum;

    /* Position freelist. */

    POSITION *Freepos;

#define MAXMOVES 64             /* > max # moves from any position */
    MOVE Possible[MAXMOVES];

    MemoryManager *mm;
    statuscode Status;             /* win, lose, or fail */

#define NQUEUES 100

    POSITION *Qhead[NQUEUES]; /* separate queue for each priority */
    int Maxq;

    bool m_newer_piles_first;
};

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
#define SUIT(card) (( (card) >> 4 ) & 3)
#define COLOR(card) ((card) & PS_COLOR)
#define DOWN(card) ((card) & ( 1 << 7 ) )

#endif
