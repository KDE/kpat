/*
    pqueue.c - implementation of a priority queue by using a binary heap.

    Originally written by Justin-Heyes Jones
    Modified by Shlomi Fish, 2000

    This file is in the public domain (it's uncopyrighted).

    Check out Justin-Heyes Jones' A* page from which this code has 
    originated:
        http://www.geocities.com/jheyesjones/astar.html
  */

/* manage a priority queue as a heap
   the heap is implemented as a fixed size array of pointers to your data */

#include <stdio.h>
#include <stdlib.h>

#include "jhjtypes.h"

#include "pqueue.h"

#define TRUE 1
#define FALSE 0

/* initialise the priority queue with a maximum size of maxelements. maxrating is the highest or lowest value of an
   entry in the pqueue depending on whether it is ascending or descending respectively. Finally the bool32 tells you whether
   the list is sorted ascending or descending... */
                                            
void PQueueInitialise( 
    PQUEUE *pq, 
    int32 MaxElements, 
    pq_rating_t MaxRating, 
    int bIsAscending
    )
{
    pq->IsAscendingHeap = bIsAscending;
    
    pq->MaxSize = MaxElements;

    pq->CurrentSize = 0;

    pq->Elements = (pq_element_t*) malloc( sizeof( pq_element_t ) * (MaxElements + 1) );

    if( pq->Elements == NULL )
    {
        printf( "Memory alloc failed!\n" );
    }          

    pq->MaxRating = MaxRating;

}

/* join a priority queue
   returns TRUE if succesful, FALSE if fails. (You fail by filling the pqueue.)
   PGetRating is a function which returns the rating of the item you're adding for sorting purposes */

int PQueuePush( PQUEUE *pq, void *item, pq_rating_t r)
{
    uint32 i;

    if (pq->CurrentSize == pq->MaxSize )
    {
        int new_size;
        new_size = pq->MaxSize + 256;
        pq->Elements = (pq_element_t *)realloc( pq->Elements, sizeof(pq_element_t) * (new_size+1));
        pq->MaxSize = new_size;
    }
    
    {
        /* set i to the first unused element and increment CurrentSize */

        i = (pq->CurrentSize += 1);

        /* while the parent of the space we're putting the new node into is worse than
           our new node, swap the space with the worse node. We keep doing that until we
           get to a worse node or until we get to the top

           note that we also can sort so that the minimum elements bubble up so we need to loops
           with the comparison operator flipped... */

        if( pq->IsAscendingHeap == TRUE )
        {

            while( ( i==PQ_FIRST_ENTRY ?
                     (pq->MaxRating) /* return biggest possible rating if first element */
                     :
                     (pq->Elements[ PQ_PARENT_INDEX(i) ].rating )
                   ) 
                   < r 
                 )
            {
                pq->Elements[ i ] = pq->Elements[ PQ_PARENT_INDEX(i) ];
            
                i = PQ_PARENT_INDEX(i);
            }
        }
        else
        {
            while( ( i==PQ_FIRST_ENTRY ?
                     (pq->MaxRating) /* return biggest possible rating if first element */
                     :
                     (pq->Elements[ PQ_PARENT_INDEX(i) ].rating )
                   ) 
                   > r 
                 )
            {
                pq->Elements[ i ] = pq->Elements[ PQ_PARENT_INDEX(i) ];
            
                i = PQ_PARENT_INDEX(i);
            }
        }


        /* then add the element at the space we created. */
        pq->Elements[i].item = item;
        pq->Elements[i].rating = r;
    }

    return TRUE;

}


int PQueueIsEmpty( PQUEUE *pq )
{

    /* todo assert if size > maxsize */

    if( pq->CurrentSize == 0 )
    {
        return TRUE;
    }

    return FALSE;
                 
}

/* free up memory for pqueue */
void PQueueFree( PQUEUE *pq )
{
    free( pq->Elements );
}

/* remove the first node from the pqueue and provide a pointer to it */

void *PQueuePop( PQUEUE *pq)
{
    int32 i;
    int32 child;

    pq_element_t pMaxElement;
    pq_element_t pLastElement;
     
    if( PQueueIsEmpty( pq ) )
    {
        return NULL;
    }

    pMaxElement = pq->Elements[PQ_FIRST_ENTRY];

    /* get pointer to last element in tree */
    pLastElement = pq->Elements[ pq->CurrentSize-- ];

    if( pq->IsAscendingHeap == TRUE )
    {

        /* code to pop an element from an ascending (top to bottom) pqueue */

        /*  UNTESTED */

        for( i=PQ_FIRST_ENTRY; PQ_LEFT_CHILD_INDEX(i) <= pq->CurrentSize; i=child )
        {
            /* set child to the smaller of the two children... */
        
            child = PQ_LEFT_CHILD_INDEX(i);
        
            if( (child != pq->CurrentSize) &&
                (PGetRating(pq->Elements[child + 1]) > PGetRating(pq->Elements[child])) )
            {
                child ++;
            }
        
            if( PGetRating( pLastElement ) < PGetRating( pq->Elements[ child ] ) )
            {
                pq->Elements[ i ] = pq->Elements[ child ];
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        /* code to pop an element from a descending (top to bottom) pqueue */

        for( i=PQ_FIRST_ENTRY; PQ_LEFT_CHILD_INDEX(i) <= pq->CurrentSize; i=child )
        {
            /* set child to the larger of the two children... */
        
            child = PQ_LEFT_CHILD_INDEX(i);
        
            if( (child != pq->CurrentSize) &&
                (PGetRating(pq->Elements[child + 1]) < PGetRating(pq->Elements[child])) )
            {
                child ++;
            }
        
            if( PGetRating( pLastElement ) > PGetRating( pq->Elements[ child ] ) )
            {
                pq->Elements[ i ] = pq->Elements[ child ];
            }
            else
            {
                break;
            }
        }
    }

    pq->Elements[i] = pLastElement;

    return pMaxElement.item;
}


