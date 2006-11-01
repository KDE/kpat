/* Main().  Parse args, read the position, and call the solver. */

#include <ctype.h>
#include <signal.h>
#include "pat.h"
#include "tree.h"
#include "param.h"

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

int Interactive = TRUE; /* interactive mode. */
int Noexit = FALSE;     /* -E means don't exit */
int Numsol;             /* number of solutions found in -E mode */
int Stack = FALSE;      /* -S means stack, not queue, the moves to be done */
int Cutoff = 1;         /* switch between depth- and breadth-first */
int Status;             /* win, lose, or fail */
int Quiet = FALSE;      /* print entertaining messages, else exit(Status); */

long Mem_remain = 50 * 1000 * 1000;
#if DEBUG
long Init_mem_remain;
#endif

int Sgame = -1;         /* for range solving */
int Egame;
extern void msdeal(u_int64_t);

/* Statistics. */

int Total_positions;
int Total_generated;

int Xparam[NXPARAM];
double Yparam[NYPARAM];

void set_param(int pnum)
{
	int i, *x;
	double *y;

	x = XYparam[pnum].x;
	y = XYparam[pnum].y;
	for (i = 0; i < NXPARAM; i++) {
		Xparam[i] = x[i];
	}
	Cutoff = Xparam[NXPARAM - 1];
	for (i = 0; i < NYPARAM; i++) {
		Yparam[i] = y[i];
	}
}

#if DEBUG
extern int Clusternum[];

void quit(int sig)
{
	int i, c;
	extern void print_queue(void);
	extern int Clusternum[];

	print_queue();
	c = 0;
	for (i = 0; i <= 0xFFFF; i++) {
		if (Clusternum[i]) {
			msg("%04X: %6d", i, Clusternum[i]);
			c++;
			if (c % 5 == 0) {
				c = 0;
				msg("\n");
			} else {
				msg("\t");
			}
		}
	}
	if (c != 0) {
		msg("\n");
	}
	print_layout();

	signal(SIGQUIT, quit);
}
#endif

int main(int argc, char **argv)
{
	int i, c, argc0;
	char *curr_arg, **argv0;
	u_int64_t gn;
	FILE *infile;
	void play(void);

	Progname = *argv;
#if DEBUG
	signal(SIGQUIT, quit);
msg("sizeof(POSITION) = %d\n", sizeof(POSITION));
#endif

	/* Parse args twice.  Once to get the operating mode, and the
	next for other options. */

	argc0 = argc;
	argv0 = argv;
	while (--argc > 0 && **++argv == '-' && *(curr_arg = 1 + *argv)) {

		/* Scan current argument until a flag indicates that the rest
		of the argument isn't flags (curr_arg = NULL), or until
		the end of the argument is reached (if it is all flags). */

		while (curr_arg != NULL && (c = *curr_arg++) != '\0') {
			switch (c) {

			case 's':
				Same_suit = TRUE;
				King_only = FALSE;
				Nwpiles = 10;
				Ntpiles = 4;
				break;

			case 'f':
				Same_suit = FALSE;
				King_only = FALSE;
				Nwpiles = 8;
				Ntpiles = 4;
				break;

			case 'k':
				King_only = TRUE;
				break;

			case 'a':
				King_only = FALSE;
				break;

			case 'S':
				Stack = TRUE;
				break;

			case 'w':
				Nwpiles = atoi(curr_arg);
				curr_arg = NULL;
				break;

			case 't':
				Ntpiles = atoi(curr_arg);
				curr_arg = NULL;
				break;

			case 'X':
				argv += NXPARAM - 1;
				argc -= NXPARAM - 1;
				curr_arg = NULL;
				break;

			case 'Y':
				argv += NYPARAM;
				argc -= NYPARAM;
				curr_arg = NULL;
				break;

			default:
				break;
			}
		}
	}

	/* Set parameters. */

	if (!Same_suit && !King_only && !Stack) {
		set_param(FreecellBest);
	} else if (!Same_suit && !King_only && Stack) {
		set_param(FreecellSpeed);
	} else if (Same_suit && !King_only && !Stack) {
		set_param(SeahavenBest);
	} else if (Same_suit && !King_only && Stack) {
		set_param(SeahavenSpeed);
	} else if (Same_suit && King_only && !Stack) {
		set_param(SeahavenKing);
	} else if (Same_suit && King_only && Stack) {
		set_param(SeahavenKingSpeed);
	} else {
		set_param(0);   /* default */
	}

	/* Now get the other args, and allow overriding the parameters. */

	argc = argc0;
	argv = argv0;
	while (--argc > 0 && **++argv == '-' && *(curr_arg = 1 + *argv)) {
		while (curr_arg != NULL && (c = *curr_arg++) != '\0') {
			switch (c) {

			case 's':
			case 'f':
			case 'k':
			case 'a':
			case 'S':
				break;

			case 'w':
			case 't':
				curr_arg = NULL;
				break;

			case 'E':
				Noexit = TRUE;
				break;

			case 'c':
				Cutoff = atoi(curr_arg);
				curr_arg = NULL;
				break;

			case 'M':
				Mem_remain = atol(curr_arg) * 1000000;
				curr_arg = NULL;
				break;

			case 'v':
				Quiet = FALSE;
				break;

			case 'q':
				Quiet = TRUE;
				break;

			case 'N':
				Sgame = atoi(curr_arg);
				Egame = atoi(argv[1]);
				curr_arg = NULL;
				argv++;
				argc--;
				break;

			case 'X':

				/* use -c for the last X param */

				for (i = 0; i < NXPARAM - 1; i++) {
					Xparam[i] = atoi(argv[i + 1]);
				}
				argv += NXPARAM - 1;
				argc -= NXPARAM - 1;
				curr_arg = NULL;
				break;

			case 'Y':
				for (i = 0; i < NYPARAM; i++) {
					Yparam[i] = atof(argv[i + 1]);
				}
				argv += NYPARAM;
				argc -= NYPARAM;
				curr_arg = NULL;
				break;

			case 'P':
				i = atoi(curr_arg);
				if (i < 0 || i > LastParam) {
					fatalerr("invalid parameter code");
				}
				set_param(i);
				curr_arg = NULL;
				break;

			default:
				msg("%s: unknown flag -%c\n", Progname, c);
				USAGE();
				exit(1);
			}
		}
	}

	if (Stack && Noexit) {
		fatalerr("-S and -E may not be used together.");
	}
	if (Mem_remain < BLOCKSIZE * 2) {
		fatalerr("-M too small.");
	}
	if (Nwpiles > MAXWPILES) {
		fatalerr("too many w piles (max %d)", MAXWPILES);
	}
	if (Ntpiles > MAXTPILES) {
		fatalerr("too many t piles (max %d)", MAXTPILES);
	}

	/* Process the named file, or stdin if no file given.
	The name '-' also specifies stdin. */

	infile = stdin;
	if (argc && **argv != '-') {
		infile = fileopen(*argv, "r");
	}

	/* Initialize the suitable() macro variables. */

	Suit_mask = PS_COLOR;
	Suit_val = PS_COLOR;
	if (Same_suit) {
		Suit_mask = PS_SUIT;
		Suit_val = 0;
	}

	/* Announce which variation this is. */

	if (!Quiet) {
		printf(Same_suit ? "Seahaven; " : "Freecell; ");
		if (King_only) {
			printf("only Kings are allowed to start a pile.\n");
		} else {
			printf("any card may start a pile.\n");
		}
		printf("%d work piles, %d temp cells.\n", Nwpiles, Ntpiles);
	}

#if DEBUG
Init_mem_remain = Mem_remain;
#endif
	if (Sgame < 0) {

		/* Read in the initial layout and play it. */

		read_layout(infile);
		if (!Quiet) {
			print_layout();
		}
		play();
	} else {

		/* Range mode.  Play lots of consecutive games. */

		Interactive = FALSE;
		for (gn = Sgame; gn < Egame; gn++) {
			printf("#%ld\n", (long)gn);
			msdeal(gn);
			play();
			fflush(stdout);
		}
	}

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
#if DEBUG
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
#if DEBUG
if (Mem_remain != Init_mem_remain) {
 msg("Mem_remain = %ld\n", Mem_remain);
}
#endif
}

/* Read a layout file.  Format is one pile per line, bottom to top (visible
card).  Temp cells and Out on the last two lines, if any. */

void read_layout(FILE *infile)
{
	int w, i, total;
	char buf[100];
	card_t out[4];
	int parse_pile(char *s, card_t *w, int size);

	/* Read the workspace. */

	w = 0;
	total = 0;
	while (fgets(buf, 100, infile)) {
		i = parse_pile(buf, W[w], 52);
		Wp[w] = &W[w][i - 1];
		Wlen[w] = i;
		w++;
		total += i;
		if (w == Nwpiles) {
			break;
		}
	}
	if (w != Nwpiles) {
		fatalerr("not enough piles in input file");
	}

	/* Temp cells may have some cards too. */

	for (i = 0; i < Ntpiles; i++) {
		T[i] = NONE;
	}
	if (total != 52) {
		fgets(buf, 100, infile);
		total += parse_pile(buf, T, Ntpiles);
	}

	/* Output piles, if any. */

	for (i = 0; i < 4; i++) {
		O[i] = out[i] = NONE;
	}
	if (total != 52) {
		fgets(buf, 100, infile);
		parse_pile(buf, out, 4);
		for (i = 0; i < 4; i++) {
			if (out[i] != NONE) {
				O[suit(out[i])] = rank(out[i]);
				total += rank(out[i]);
			}
		}
	}

	if (total != 52) {
		fatalerr("not enough cards");
	}
}

int parse_pile(char *s, card_t *w, int size)
{
	int i;
	card_t rank, suit;

	i = 0;
	rank = suit = NONE;
	while (i < size && *s && *s != '\n' && *s != '\r') {
		while (*s == ' ') s++;
		*s = toupper(*s);
		if (*s == 'A') rank = 1;
		else if (*s >= '2' && *s <= '9') rank = *s - '0';
		else if (*s == 'T') rank = 10;
		else if (*s == 'J') rank = 11;
		else if (*s == 'Q') rank = 12;
		else if (*s == 'K') rank = 13;
		else fatalerr("bad card %c%c\n", s[0], s[1]);
		s++;
		*s = toupper(*s);
		if (*s == 'C') suit = PS_CLUB;
		else if (*s == 'D') suit = PS_DIAMOND;
		else if (*s == 'H') suit = PS_HEART;
		else if (*s == 'S') suit = PS_SPADE;
		else fatalerr("bad card %c%c\n", s[-1], s[0]);
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
	fprintf(stderr, "\n---\n");
}

#if 0
void print_move(MOVE *mp)
{
  printcard(mp->card, stderr);
  if (mp->totype == T_TYPE) {
   msg("to temp (%d)\n", mp->pri);
  } else if (mp->totype == O_TYPE) {
   msg("out (%d)\n", mp->pri);
  } else {
   msg("to ");
   if (mp->destcard == NONE) {
    msg("empty pile (%d)", mp->pri);
   } else {
    printcard(mp->destcard, stderr);
    msg("(%d)", mp->pri);
   }
   fputc('\n', stderr);
  }
  print_layout();
}
#endif
