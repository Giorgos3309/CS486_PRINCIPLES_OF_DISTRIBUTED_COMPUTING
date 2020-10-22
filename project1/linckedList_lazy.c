#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "linckedList_lazy.h"

typedef struct LNODE LNODE;

//LNODE *lhead, *ltail;


void linit(){
    LNODE* t = malloc( sizeof(LNODE) );
    LNODE* h = malloc( sizeof(LNODE) );
    t->marked = 0;
    t->songID = INT_MAX;
    h->marked = 0;
    h->songID = -1;
    h->next = t;
    t->next = NULL;
    lhead=h;
    ltail=t;
}

int validate(LNODE *pred, LNODE *curr) {
    if (pred->marked == 0 && curr->marked == 0 && pred->next == curr){
        return 1;
    }else{
        return 0;
    }
}

int search(int songID) {
    LNODE *curr;
    int result;
    curr = lhead;
    while (curr->songID < songID)
    curr = curr->next;
    if (curr->marked !=1 && songID==curr->songID){
        return 1;
    }else{
         return 0;
    }
}

int linsert(int songID) { // code for process p
    LNODE *pred, *curr;
    int result;
    int return_flag = 0;
    while (1) {
        if(lhead==NULL || ltail==NULL){return 0;}
        pred = lhead; 
        curr = pred->next;
        
        while (curr->songID < songID) {
            pred = curr;
            curr = curr->next;
        }
        
        
        pthread_mutex_lock(&pred->lock);
        pthread_mutex_lock(&curr->lock);
        if (validate(pred, curr) == 1) {
            if (songID == curr->songID) {
                result = 0; return_flag = 1;
            }
            else {
                LNODE *tmp = malloc(sizeof(LNODE) );
                tmp->next = curr;
                tmp->songID = songID;
                pred->next = tmp;
                result = 1; return_flag = 1;
            }
        }
        pthread_mutex_unlock(&pred->lock);
        pthread_mutex_unlock(&curr->lock);
        if (return_flag) {
            return result;
        }
    }
}

/*
int ldelete(int songID) {
    // code for process p
    LNODE *pred, *curr;
    int result; int return_flag = 0;
    while (1) {
        pred = lhead; curr = pred->next;
        while (curr->songID < songID) {
            pred = curr;
            curr = curr->next;
        }
        pthread_mutex_lock(&pred->lock); 
        pthread_mutex_lock(&curr->lock);
        if (validate(pred, curr)) {
        if (songID == curr->songID) {
        curr->marked = 1;
        pred->next = curr->next;
        result = 1;
        }
        else result = 0;

        return_flag = 1;
        }
        pthread_mutex_unlock(&pred->lock); 
        pthread_mutex_unlock(&curr->lock);
        if (return_flag == 1) return result;
    }
} */


/*int main(){
    linit();
    for(int i=1;i<1000;++i){
        linsert(i);
    }
    linsert(1);
    LNODE* tmp=lhead->next;
    while(tmp!=ltail){
        printf("%d  ",tmp->songID);
        tmp=tmp->next;
    }
    return 0;
}*/




