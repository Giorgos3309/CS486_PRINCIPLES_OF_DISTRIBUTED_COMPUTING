#ifndef BST
#define BST

typedef struct treeNode {
	int songIDs[3];
	struct treeNode *children[4];
	pthread_mutex_t lock;
}treeNode;

typedef struct treeNode treeNode;

treeNode* BSTsearch(treeNode *head , int songID);
int BSTDelete(treeNode *head, int songID);
int BSTInsert(treeNode *head , int songID );
treeNode* initializeBST();

unsigned long long int BFS_sum_check(treeNode* head);
unsigned long long int BFS_size_check(treeNode* head );

void BFS_print(treeNode* head );

#endif
