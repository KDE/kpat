/*
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
*/
#ifndef __SPEEDS_H_
#define __SPEEDS_H_

#define TEST_SOLVER 0

#ifdef TEST_SOLVER
#define STEPS_AUTODROP 8
#define STEPS_WON 20
#define STEPS_DEMO 7
#define STEPS_MOVEBACK 7
#define STEPS_INITIALDEAL 10

#define TIME_BETWEEN_MOVES 200
#else

#define STEPS_AUTODROP 1
#define STEPS_WON 20
#define STEPS_DEMO 1
#define STEPS_MOVEBACK 1
#define STEPS_INITIALDEAL 1

#define TIME_BETWEEN_MOVES 2

#endif

#endif
