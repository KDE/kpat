#include <stdio.h>

#include "pqueue.h"

#define NUM_ELEMS 10

uint32 my_rating(void * ptr)
{
    return *(uint32*)ptr;
}

int main()
{
    PQUEUE pq;
    uint32 array[NUM_ELEMS] = {5, 100, 3, 20, 400, 12, 70, 90, 8, 22};
    int i;

    PQueueInitialise(&pq, 4, 0, 0);

    for(i=0;i<NUM_ELEMS;i++)
    {
        PQueuePush(&pq, &array[i], array[i]);
    }

    while (!PQueueIsEmpty(&pq))
    {
        printf("%i\n", (int)*(pq_rating_t *)PQueuePop(&pq));
    }
    PQueueFree(&pq);

    return 0;
};
