#ifndef __SPEEDS_H_
#define __SPEEDS_H_

#if !defined(DEBUG_SOLVER)
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
