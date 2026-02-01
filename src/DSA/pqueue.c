#include "pqueue.h"
#include "astar.h"
#include <limits.h>
#include <stdlib.h>

PriorityQueue *pq_create(int capacity) {
    PriorityQueue *pq = malloc(sizeof(PriorityQueue));
    pq->nodes = malloc(capacity * sizeof(AStarNode *));
    pq->capacity = capacity;
    pq->size = 0;
    return pq;
}

void pq_push(PriorityQueue *pq, AStarNode *node) {
    if (pq->size >= pq->capacity) {
        return;
    }
    pq->nodes[pq->size] = node;
    int i = pq->size;
    pq->size++;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (pq->nodes[i]->f_cost >= pq->nodes[parent]->f_cost) {
            break;
        }

        AStarNode *temp = pq->nodes[i];
        pq->nodes[i] = pq->nodes[parent];
        pq->nodes[parent] = temp;

        i = parent;
    }
}

AStarNode *pq_pop(PriorityQueue *pq) {
    if (pq->size == 0) {
        return NULL;
    }

    AStarNode *result = pq->nodes[0];

    pq->size--;
    pq->nodes[0] = pq->nodes[pq->size];

    int i = 0;
    while (1) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;

        if (left < pq->size &&
            pq->nodes[left]->f_cost < pq->nodes[smallest]->f_cost) {
            smallest = left;
        }
        if (right < pq->size &&
            pq->nodes[right]->f_cost < pq->nodes[smallest]->f_cost) {
            smallest = right;
        }

        if (smallest == i) {
            break;
        }

        AStarNode *temp = pq->nodes[i];
        pq->nodes[i] = pq->nodes[smallest];
        pq->nodes[smallest] = temp;

        i = smallest;
    }

    return result;
}

bool pq_is_empty(PriorityQueue *pq) { return pq->size == 0; }

void pq_free(PriorityQueue *pq) {
    free(pq->nodes);
    free(pq);
}
