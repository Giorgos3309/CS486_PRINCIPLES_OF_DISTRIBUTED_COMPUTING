
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "UnboundedLockFreeQueue.h"

/*
typedef struct node {
    int songID ;
    struct node *next ;
} QNODE;

typedef struct queue {
    QNODE *Head ;
    QNODE *Tail ;
}QUEUE ;
*/

void qinit(QUEUE *Q) {
    QNODE *p = malloc(sizeof(QNODE) ) ;
    // sentinel node
    p->next = NULL;
    Q->Head = Q->Tail = p ;
    p->songID = INT_MAX;
}

void enq(QUEUE *Q, int songID ) {
    QNODE *next , *last ;
    QNODE *p = malloc(sizeof(QNODE) );
    p->songID = songID ;
    p->next = NULL; 
    while (1) {                       // keep trying until enq() is done
        last = Q->Tail ;                // read Tail
        next = last->next ;             // read next node of last
        if (last == Q->Tail) {          // are last and next consistent?
            if (next == NULL) {             // was Tail pointing to the last node?
                if (__sync_bool_compare_and_swap(&last->next , next , p) ){  // try to link new node at the end of the list
                    break ;
                }
            }else{
                __sync_bool_compare_and_swap(&Q->Tail, last, next);
                //CAS(Q->Tail, last, next ) ; // tail was not pointing to last node; try to advance
            }
        } 
    } 
    __sync_bool_compare_and_swap(&Q->Tail, last, p);
    //CAS(Q->Tail, last, p ); // equeue is done -> try to swing Tail to the inserted node
} 
 


int deq(QUEUE *Q, int *songID) {
    QNODE *first, *last, *next;
    while (1){ // keep trying until deq() is done
        first = Q->Head; // read Head
        last = Q->Tail; // read Tail
        next = first->next;
        if (first == Q->Head) { // are first and next still consistent?
            if (first == last) { // is queue empty or Tail falling behind?
                if (next == NULL) {// is queue empty?
                    return 0;
                }
            __sync_bool_compare_and_swap(&Q->Tail, last, next); // tail is falling behind -> try to advance it
        }
        else { // no need to deal with Tail
            *songID = next->songID; // read songID to return
            if (__sync_bool_compare_and_swap(&Q->Head, first, next)) // try to swing Head to the next node
                break;
            } // else // deq() is done, so exit loop
        } // if
    } // while
 return 1; // queue was not empty, so deq() suceeded
} // Dequeue

/*
void  main(){
    QUEUE q;
    qinit(&q);

    for(int i=0;i<10;++i){
        enq(&q,i);
    }
    int *p=malloc(1000000*sizeof(int));
    for(int i=0;i<1000000;++i){
        p[i]=0;
    }

    for(int i=0;i<10;++i){
        int y=0;
        deq(&q,&y);
        printf("-->%d\n",y);
    }

}*/