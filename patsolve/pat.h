/* Solve Freecell and Seahaven type patience (solitaire) games. */

#include "util.h"
#include <sys/types.h>
#include "config.h"
#include "tree.h"

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

extern int Same_suit;           /* game parameters */
extern int King_only;
extern int Nwpiles;
extern int Ntpiles;

/* Some macros used in get_possible_moves(). */

/* The following macro implements
	(Same_suit ? (suit(a) == suit(b)) : (color(a) != color(b)))
*/
#define suitable(a, b) ((((a) ^ (b)) & Suit_mask) == Suit_val)
extern card_t Suit_mask;
extern card_t Suit_val;

#define king_only(card) (!King_only || rank(card) == PS_KING)

extern char Rank[];
extern char Suit[];

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

extern POSITION *Freepos;       /* position freelist */

/* Work arrays. */

extern card_t T[MAXTPILES];     /* one card in each temp cell */
extern card_t W[MAXWPILES][52]; /* the workspace */
extern card_t *Wp[MAXWPILES];   /* point to the top card of each work pile */
extern int Wlen[MAXWPILES];     /* the number of cards in each pile */
extern int Widx[MAXWPILES];     /* used to keep the piles sorted */
extern int Widxi[MAXWPILES];    /* inverse of the above */

extern card_t O[4];             /* output piles store only the rank or NONE */
extern card_t Osuit[4];         /* suits of the output piles */

extern int Nwpiles;             /* the numbers we're actually using */
extern int Ntpiles;

/* Temp storage for possible moves. */

#define MAXMOVES 64             /* > max # moves from any position */
extern MOVE Possible[MAXMOVES];

enum inscode { NEW, FOUND, FOUND_BETTER, ERR };

/* Command line flags. */

extern int Cutoff;
extern int Interactive;
extern int Noexit;
extern int Numsol;
extern int Stack;
extern int Quiet;

enum statuscode { FAIL = -1, WIN = 0, NOSOL = 1 };
extern int Status;

/* Memory. */

typedef struct block {
	u_char *block;
	u_char *ptr;
	int remain;
	struct block *next;
} BLOCK;

#define BLOCKSIZE (32 * 4096)

typedef struct treelist {
	TREE *tree;
	int cluster;
	struct treelist *next;
} TREELIST;

/* Statistics. */

extern int Total_positions;
extern int Total_generated;
extern long Mem_remain;

extern int Xparam[];
extern double Yparam[];

/* Prototypes. */

extern void doit();
extern void read_layout(FILE *);
extern void printcard(card_t card, FILE *);
extern void print_layout();
extern void make_move(MOVE *);
extern void undo_move(MOVE *);
extern MOVE *get_moves(POSITION *, int *);
extern POSITION *new_position(POSITION *parent, MOVE *m);
extern void unpack_position(POSITION *);
extern TREE *pack_position(void);
extern void init_buckets(void);
extern void init_clusters(void);
extern u_char *new_from_block(size_t);
extern void pilesort(void);
