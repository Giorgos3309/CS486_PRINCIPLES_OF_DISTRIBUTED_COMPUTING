
#ifndef UNBOUNDED_LOCK_FREE_QUEUE
#define UNBOUNDED_LOCK_FREE_QUEUE


typedef struct node {
    int songID ;
    struct node *next ;
} QNODE;

typedef struct queue {
    QNODE *Head ;
    QNODE *Tail ;
}QUEUE ;


void qinit(QUEUE *Q);

void enq(QUEUE *Q, int songID );

int deq(QUEUE *Q, int *songID);

#endif