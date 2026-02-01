#ifndef DSA_PQUEUE_H_
#define DSA_PQUEUE_H_
#include "astar.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
        AStarNode **nodes;
        int capacity;
        int size;
} PriorityQueue;

PriorityQueue *pq_create(int capacity);
void pq_push(PriorityQueue *pq, AStarNode *node);
AStarNode *pq_pop(PriorityQueue *pq);
bool pq_is_empty(PriorityQueue *pq);
void pq_free(PriorityQueue *pq);
#endif // !DSA_PQUEUE_H_
