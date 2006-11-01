/* Main().  Parse args, read the position, and call the solver. */

#include <ctype.h>
#include <signal.h>
#include "pat.h"
#include "tree.h"

char Usage[] =
  "usage: %s [-s|f] [-k|a] [-w<n>] [-t<n>] [-E] [-S] [-q|v] [layout]\n"
  "-s Seahaven (same suit), -f Freecell (red/black)\n"
  "-k only Kings start a pile, -a any card starts a pile\n"
  "-w<n> number of work piles, -t<n> number of free cells\n"
  "-E don't exit after one solution; continue looking for better ones\n"
  "-S speed mode; find a solution quickly, rather than a good solution\n"
  "-q quiet, -v verbose\n"
  "-s implies -aw10 -t4, -f implies -aw8 -t4\n";
#define USAGE() msg(Usage, Progname)

int Interactive = FALSE; /* interactive mode. */
int Noexit = FALSE;     /* -E means don't exit */
int Numsol;             /* number of solutions found in -E mode */
int Stack = FALSE;      /* -S means stack, not queue, the moves to be done */
int Cutoff = 1;         /* switch between depth- and breadth-first */
int Status;             /* win, lose, or fail */
int Quiet = FALSE;      /* print entertaining messages, else exit(Status); */

long Mem_remain = 50 * 1000 * 1000;
#if defined(DEBUG)
long Init_mem_remain;
#endif

int Sgame = -1;         /* for range solving */
int Egame;

/* Statistics. */

int Total_positions;
int Total_generated;

int Xparam[] = { 3, 4, 7, -4, -2, 1, 0, -4, 4, -1, 1 };
double Yparam[] = { -0.0008, 0.14, -1.0, };

int patsolve(const char *text)
{
	void play(void);
	void read_layout(const char *);

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
	print_layout();
	play();

	return Status;
}

void play(void)
{
	/* Initialize the hash tables. */

	init_buckets();
	init_clusters();

	/* Reset stats. */

	Total_positions = 0;
	Total_generated = 0;
	Numsol = 0;

	Status = NOSOL;

	/* Go to it. */

	doit();
	if (Status != WIN && !Quiet) {
		if (Status == FAIL) {
			printf("Out of memory.\n");
		} else if (Noexit && Numsol > 0) {
			printf("No shorter solutions.\n");
		} else {
			printf("No solution.\n");
		}
#if defined(DEBUG)
		printf("%d positions generated.\n", Total_generated);
		printf("%d unique positions.\n", Total_positions);
		printf("Mem_remain = %ld\n", Mem_remain);
#endif
	}
	if (!Interactive) {
		free_buckets();
		free_clusters();
		free_blocks();
		Freepos = NULL;
	}
#if defined(DEBUG)
if (Mem_remain != Init_mem_remain) {
 msg("Mem_remain = %ld\n", Mem_remain);
}
#endif
}

static const char * next_line( const char *text)
{
    if (!text)
	return text;
    if (!text[0])
	return 0;
    return strchr(text, '\n') + 1;
}

/* Read a layout file.  Format is one pile per line, bottom to top (visible
card).  Temp cells and Out on the last two lines, if any. */

void read_layout(const char *text)
{
	int w, i, total;
	card_t out[4];
	int parse_pile(const char *s, card_t *w, int size);

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

int parse_pile(const char *s, card_t *w, int size)
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

void printcard(card_t card, FILE *outfile)
{
	if (rank(card) == NONE) {
		fprintf(outfile, "   ");
	} else {
		fprintf(outfile, "%c%c ", Rank[rank(card)], Suit[suit(card)]);
	}
}

void print_layout()
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
		printcard(T[t], stderr);
	}
	fputc('\n', stderr);
	for (o = 0; o < 4; o++) {
		printcard(O[o] + Osuit[o], stderr);
	}
	fprintf(stderr, "\nprint-layout-end\n");
}

