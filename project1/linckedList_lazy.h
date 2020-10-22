#ifndef LINCKEDLIST_LAZY
#define LINCKEDLIST_LAZY

typedef struct LNODE {
    int songID;
    int marked;
    pthread_mutex_t lock;
    struct LNODE *next;
} LNODE;

LNODE *lhead, *ltail;


void linit();

int validate(LNODE *pred, LNODE *curr) ;


int search(int songID);

int linsert(int songID);

//int ldelete(int songID);

#endif 



