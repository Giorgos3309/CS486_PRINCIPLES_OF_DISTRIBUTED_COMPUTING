#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "tree.h"


//treeNode *head ; //= (treeNode*)malloc(sizeof(treeNode));
//treeNode *root;
void msg(char *s){
	fprintf(stderr,"%s",s);
}
int insert(treeNode *node , int songID);
int remove_id(treeNode *node , int songID);
treeNode* node_alloc();
void printNode(treeNode *node);
int is4node(treeNode* node);
treeNode *getNextChild(treeNode *node, int songID);
int split( treeNode* head , treeNode *child , treeNode *parent);
int isleaf(treeNode* node);
int contains(treeNode* node , int songID);
int is2node(treeNode* node);
int merge( treeNode* head, treeNode* parent,treeNode* rc,treeNode* lc);
treeNode* getLBrother(treeNode *node,treeNode *parent);
treeNode* getRBrother(treeNode *node,treeNode *parent);
int take_from_brother(treeNode* brother, treeNode* parent , treeNode *node);

treeNode* initializeBST(){
	 return  node_alloc();
 }
int take_from_parent( treeNode* head , treeNode* parent ,treeNode* child);
int BSTInsert(treeNode *head , int songID ){
	treeNode *pred , *curr;

	if(head==NULL){return 0;}
	pthread_mutex_lock(&head->lock);
	pred = head;
	curr=pred->children[0];

	if(curr==NULL){
		head->children[0] = node_alloc();
		curr = head->children[0];
		insert(curr , songID);
		pthread_mutex_unlock(&head->lock);
		return 1;
	}	

	pthread_mutex_lock(&curr->lock);

	while(1){
		if( is4node(curr) ){
			pthread_mutex_unlock(&curr->lock);
			split(head,curr,pred); 
			curr = getNextChild(pred , songID);
			pthread_mutex_lock(&curr->lock);
		}else if( contains(curr , songID) ){
			pthread_mutex_unlock(&pred->lock);
			pthread_mutex_unlock(&curr->lock);
			return 0;
		}else if(isleaf(curr)){
			break;
		}else{

			pthread_mutex_unlock(&pred->lock);
			pred = curr;
			curr = getNextChild(pred , songID);
			pthread_mutex_lock(&curr->lock);

		}
	}
	pthread_mutex_unlock(&pred->lock);
	insert(curr,songID);
	pthread_mutex_unlock(&curr->lock);

	return 1;
}


treeNode* BSTsearch(treeNode *head , int songID){
	treeNode *pred,*curr;
	if(head->children[0]==NULL){return NULL;} // empty tree
	pthread_mutex_lock(&head->lock);
	pred = head;
	curr = pred->children[0];
	pthread_mutex_lock(&curr->lock);
	while(1){
		if(contains(curr,songID))
		{
			treeNode* res = curr;
			pthread_mutex_unlock(&pred->lock);
			pthread_mutex_unlock(&curr->lock);
			return res;
		}else if( isleaf(curr) ){
			pthread_mutex_unlock(&pred->lock);
			pthread_mutex_unlock(&curr->lock);
			return NULL;
		}else{
			pthread_mutex_unlock(&pred->lock);
			pred = curr;
			curr = getNextChild(curr,songID);
			pthread_mutex_lock(&curr->lock);
		}
	}	
}



int remove_helper(treeNode *head, treeNode *root, int songID ){
	treeNode *pred,*curr;
	int lookupTheMinimumOfLeftSubtree = 1;  //if the value is set to 0 we look up the maximum of right subtree
	int dummysongID = songID-1;;
	fprintf(stderr,"\nremove_helper:songID:%d\n",songID);
	BFS_print(head);
	if(root==NULL){return -1;} // empty tree
	
	if(isleaf(root) ){
		remove_id(root,songID);
		return songID;
	}else if(root==head->children[0] && is2node(root->children[0]) && is2node(root->children[1]) ){
		merge(head , root,root->children[1],root->children[0]);
		if(isleaf(root) ){
			remove_id(root,songID);
			return songID;
		}
	}

	pred = root;

	int index=0;
	for( index=0;index<3;++index){
		if(root->songIDs[index]==songID){break;}
	}
	if(songID==root->songIDs[0]){   //root = (songID , ID , ?)
		//pthread_mutex_lock(&root->children[0]->lock);
		//pthread_mutex_lock(&root->children[index]->lock);
		if( is2node(root->children[0])){
			//pthread_mutex_lock(&root->children[1]->lock);
			//pthread_mutex_lock(&root->children[2]->lock);
			lookupTheMinimumOfLeftSubtree = 0;
			dummysongID = songID+1;
			if( is2node(root->children[1]) ){
				if( is2node(root->children[2]) ){
					merge(head,root,root->children[2],root->children[1]);
				}else{
					take_from_brother(root->children[2],root,root->children[1]);
				}
			}
			//pthread_mutex_unlock(&root->children[1]->lock);
			//pthread_mutex_unlock(&root->children[2]->lock);
		}
		//pthread_mutex_unlock(&root->children[index]->lock);
	}else if( is2node(root->children[index]) ){
		msg("case is not at root[0]\n");
		//pthread_mutex_lock(&root->children[index-1]->lock);
		if( is2node(root->children[index-1]) ){
			msg("case is not at root[0]\n");
			merge(head,root,root->children[index],root->children[index-1]);
			msg("case is not at root[0]\n");
		}
		msg("case is not at root[0]\n");
		//pthread_mutex_unlock(&root->children[0]->lock);// todo ??
		//pthread_mutex_unlock(&root->children[index-1]->lock);
	}
	fprintf(stderr,"\nremove helper :: lookupTheMinimumOfLeftSubtree=%d dummyid=%d\n",lookupTheMinimumOfLeftSubtree,dummysongID);
	return dummysongID;
	curr = getNextChild(root,dummysongID);
/*	pthread_mutex_lock(&curr->lock);msg("here\n");

	while(1){
		msg("while loop\n");
		if( is2node(curr) ){
			msg("here2\n");printNode(curr);
			treeNode* RBrother = getRBrother(curr , pred);
			treeNode* LBrother = getLBrother(curr , pred);
			msg("here3\n");
			if(LBrother!=NULL && !is2node(LBrother) ){
				msg("in delete LB->");printNode(LBrother);
				pthread_mutex_lock(&LBrother->lock);
				take_from_brother(LBrother,pred,curr);
				pthread_mutex_unlock(&LBrother->lock);
				msg("--->end\n");
				//break;
			}else if(RBrother!=NULL && !is2node(RBrother) ){
				fprintf(stderr,"\nin delete_helper RB:songid=%d\n",songID);printNode(curr);
				pthread_mutex_lock(&RBrother->lock);
				take_from_brother(RBrother,pred,curr);
				pthread_mutex_unlock(&RBrother->lock);
				//break;
			}else{
				msg("helper remove:while:else case\n");
				int onestepback=take_from_parent(head ,pred , curr);
				msg("helper remove:while:else case end\n");
				//if(onestepback==-1){	//freenode unlock nodes
				//	pthread_mutex_unlock(&curr->lock);
				//}
				//break;
			}
		}else if( isleaf(curr) ){
			msg("is leaf curr\n");
			int i=0;
			if(lookupTheMinimumOfLeftSubtree==1){
				if(curr->songIDs[2]!=-1){
					i=2;
				}else{
					for(i=1;i<3;++i){
						if(curr->songIDs[i]==-1){--i;break;}
					}
				}
			}else{
				i=0;
			}
			int tmp = curr->songIDs[i]; 
			remove_id(curr,tmp);
			pthread_mutex_unlock(&pred->lock);
			pthread_mutex_unlock(&curr->lock);

			treeNode *tmp_ptr;
			if(contains(root,songID)){
				tmp_ptr=root;
			}else{
				tmp_ptr=getNextChild(root,dummysongID);
			}
			for(int t=0;t<3;++t){
				if(root->songIDs[t]==songID){
					root->songIDs[t]=tmp;
				}
			}

			return tmp;
		}else{
			msg("try to make a step\n");
			if(pred!=root){ //it s not necessary this if stamement 
				pthread_mutex_unlock(&pred->lock);
			}
			pred = curr;
			curr=getNextChild(curr,dummysongID);
			pthread_mutex_lock(&curr->lock);
		}
	}	*/
}


//void BFS(treeNode* root );

int BSTDelete(treeNode *head, int songID){
	treeNode *pred,*curr,*LBrother,*RBrother;

	pthread_mutex_lock(&head->lock);
	treeNode* root=head->children[0];
	pred = head;
	curr = root;
	if(curr==NULL){
		pthread_mutex_unlock(&head->lock);
		return 0;
	}
	pthread_mutex_lock(&curr->lock);

	while(1){
		//printNode(curr);msg("--<curr. enter while loop\n");
		if( is2node(curr) && curr!=root ){
			//fprintf(stderr,"is2node:");printNode(curr);fprintf(stderr,"\n");
			RBrother = getRBrother(curr , pred);
			LBrother = getLBrother(curr , pred);
			if(LBrother!=NULL && !is2node(LBrother) ){
				msg("in delete LB\n");
				pthread_mutex_lock(&LBrother->lock);
				take_from_brother(LBrother,pred,curr);
				pthread_mutex_unlock(&LBrother->lock);
				//break;
			}else if(RBrother!=NULL && !is2node(RBrother) ){
				fprintf(stderr,"\nin delete RB:songid=%d\n",songID);printNode(curr);
				pthread_mutex_lock(&RBrother->lock);
				take_from_brother(RBrother,pred,curr);
				pthread_mutex_unlock(&RBrother->lock);
				//break;
			}else{
				int onestepback=take_from_parent(head ,pred , curr);
				if(onestepback==-1){
					pthread_mutex_unlock(&curr->lock);
					if(contains(pred,songID)){
						curr=pred;
						break;
					}else{
						curr = getNextChild(curr,songID);
						pthread_mutex_lock(&curr->lock);
					}
				}
				//break;
			}
		}else if( contains(curr,songID) ){
			msg("\nBSTremove  contains = true\n");
			break;
		}else if( isleaf(curr) ){
			pthread_mutex_unlock(&pred->lock);
			pthread_mutex_unlock(&curr->lock);
			return 0;
		}else{
			pthread_mutex_unlock(&pred->lock);
			pred = curr;
			curr = getNextChild(curr,songID);
			pthread_mutex_lock(&curr->lock);
		}
		
	}
	//fprintf(stderr,"\nremove helper returns:%d\n", remove_helper(head,curr,songID) ); 
	msg("\ndelete returned:");printNode(curr);msg("\n");
	pthread_mutex_unlock(&pred->lock);
	pthread_mutex_unlock(&curr->lock);
	return 1;
}

void test1(treeNode* root2){
	BSTInsert(root2,40);
	BSTInsert(root2,70);
	BSTInsert(root2,30);
	BSTInsert(root2,80);
	BSTInsert(root2,90);
	BSTInsert(root2,100);
	BSTInsert(root2,110);
	
	BSTInsert(root2,130);
	BSTInsert(root2,140);

	BSTInsert(root2,150);
	BSTInsert(root2,160);
	BSTInsert(root2,170);
	BSTInsert(root2,180);
	BSTInsert(root2,190);
	BSTInsert(root2,200);
	BSTInsert(root2,210);
	BSTInsert(root2,220);


	BSTInsert(root2,120);
	BSTInsert(root2,125);
	BSTInsert(root2,122);
	BSTInsert(root2,126);
	BSTInsert(root2,127);
	BSTInsert(root2,128);
	BSTInsert(root2,129);
	BSTInsert(root2,123);
	BSTInsert(root2,124);
	BSTInsert(root2,121);
}

int main(){


	treeNode* head1 = node_alloc();
	
	test1(head1);
	BFS_print(head1);
	treeNode* tmp=BSTsearch(head1,127);
	//printNode(tmp);
	int t=0;
	for(t;t<3;++t){
		if(tmp->songIDs[t]==127){break;}
	}
	treeNode* tmp2=BSTsearch(head1,125);
	fprintf(stderr," t=%d ",t);
	printNode(tmp->children[t+1]);
	BSTDelete(head1,150);
	msg("bfsdelete:");
	BFS_print(head1);
	return 1;
}
int contains(treeNode* node , int songID){
	for(int i=0;i<3;++i){
		if(node->songIDs[i]==songID){return 1;}
	}
	return 0;
}
int isleaf(treeNode* node){
	if(node->children[0]==NULL && node->children[1]==NULL && node->children[2]==NULL && node->children[3]==NULL){return 1;}
	return 0;
}

int split(treeNode* head , treeNode *child , treeNode *parent){
	if(child->songIDs[2]==-1){return 0;}
	if(parent==head){ //root case
		head->children[0] = node_alloc();
		insert(head->children[0],child->songIDs[1]);
		head->children[0]->children[0] = child;
		head->children[0]->children[1] = node_alloc();
		insert(head->children[0]->children[1] , child->songIDs[2]);
		head->children[0]->children[1]->children[0]=child->children[2];
		head->children[0]->children[1]->children[1]=child->children[3];


		child->songIDs[1]=-1;
		child->songIDs[2]=-1;
		child->children[2]=NULL;
		child->children[3]=NULL;

		return 1;
	}

	int i = 0;
	int position;
	int tmpid;
	for( i ; i<3 ; ++i){
		tmpid = parent->songIDs[i];
		if(tmpid>child->songIDs[1] || tmpid==-1){
			break;
		}
	}
	
	position = i;

	for( i=2 ; i>position ; --i){
		parent->songIDs[i] = parent->songIDs[i-1];
	}
	parent->songIDs[position]=child->songIDs[1];
	
	int tmp=position+1;
	for(int c=3;c>tmp;--c){
		parent->children[c] = parent->children[c-1] ;
	}

	parent->children[position] = child;
	parent->children[position+1] = node_alloc();
	parent->children[position+1]->songIDs[0] = child->songIDs[2];
	parent->children[position+1]->children[0] = child->children[2];
	parent->children[position+1]->children[1] = child->children[3];
	child->songIDs[1] = -1;
	child->songIDs[2] = -1;
	child->children[2] = NULL;
	child->children[3] = NULL;

	

}

treeNode *getNextChild(treeNode *node, int songID){
	int i=0 , tmp;
	if( node->songIDs[2]!=-1 && node->songIDs[2]<songID ){
		return node->children[3];
	}

	for(i ; i<3 ; ++i){
		tmp = node->songIDs[i];
		if(tmp>songID || tmp==-1){
			break;
		}
	}
	
	return node->children[i];
}


void printNode(treeNode *node){
	if(node==NULL){fprintf(stderr,"(NULL)");return;}
	fprintf(stderr,"(");
	for(int i=0;i<3;++i){
		fprintf(stderr,"%d ", node->songIDs[i]);
	}
	fprintf(stderr,")");
}
unsigned long int nodeid_sum(treeNode *node){
	if(node==NULL){return 0;}
	int sum=0;
	for(int i=0;i<3;++i){
		if(node->songIDs[i]!=-1){
			sum += node->songIDs[i];
		}
	}
	return sum;
}
int insert(treeNode *node , int songID){
	if( node->songIDs[2] !=-1 ){ return 0; }
	
	int i=0 , tmp;
	for(i ; i<3 ; ++i){
		tmp = node->songIDs[i];
		if(tmp>songID || tmp==-1){
			break;
		}
	} 
	if(tmp==-1){
		node->songIDs[i] = songID;
	}else{
		int j=1;
		for(j ;j>=i ; --j){
			node->songIDs[j+1] = node->songIDs[j];
		}
		node->songIDs[i] = songID;
	}

	return 1;
}

int remove_id(treeNode *node , int songID){
	if( node==NULL ){ return 0; }
	
	for(int i=0;i<3;++i){
		if(node->songIDs[i]==songID){
			for(int j=i;j<2;++j){
				node->songIDs[j] = node->songIDs[j+1];
			}
			node->songIDs[2]=-1;
			return 1;
		}
	}
	return 0;
}

treeNode* node_alloc(){
	treeNode* node;
	node = (treeNode*)malloc(sizeof(treeNode));

	node->songIDs[0]=-1;
	node->songIDs[1]=-1;
	node->songIDs[2]=-1;

	for(int i=0;i<4;++i){
		node->children[i]=NULL;
	}

	for(int i=0;i<3;++i){
		node->songIDs[i]=-1;
	}
	return node;
}

struct Queue{
	treeNode** array;
	int top;
	int bottom;	
	int size;
};

void initializeQueue(struct Queue *q ){
	q->top=0;
	q->bottom=0;
	q->array=malloc(2*sizeof(treeNode*));
	q->size=2;
}

void pushbottom(struct Queue *q , treeNode* node){
	if(q->bottom+1==q->size){
		//printf("queue overflow\n");
		q->array=realloc(q->array,q->bottom*2*sizeof(treeNode*)) ; 
		if(q==NULL){
			printf("BFS:realloc fail\n");
			exit(0);
		}
		q->size=q->size*2;
	}
	q->bottom=(q->bottom+1);
	q->array[q->bottom-1]=node;
}
treeNode* popTop(struct Queue *q){
	if(q->top==q->bottom){ return NULL;}
	q->top=(q->top+1);
	return q->array[q->top-1];
}


unsigned long long int BFS_sum_check(treeNode* head ){
	if(head==NULL){return 0;}
	unsigned long long int sum=0;
	treeNode *root = head->children[0];
	struct Queue q;
	initializeQueue(&q);
	pushbottom(&q,root);
	int doublefn=0;
	while((root=popTop(&q)) != NULL){
		doublefn=0;
		for(int i=0; i<4; ++i){
			if(root->children[i]==NULL){
				break; 
			}
			pushbottom(&q , root->children[i]);
		}
		sum+=nodeid_sum(root);
		
	}
	return sum;
}
unsigned long long int BFS_size_check(treeNode* head ){
	if(head==NULL){return 0;}
	unsigned long long int size=0;
	treeNode *root = head->children[0];
	struct Queue q;
	initializeQueue(&q);
	pushbottom(&q,root);
	int doublefn=0;
	while((root=popTop(&q)) != NULL){
		doublefn=0;
		for(int i=0; i<4; ++i){
			if(root->children[i]==NULL){
				break; 
			}
			pushbottom(&q , root->children[i]);
		}
		for(int i=0;i<3;++i){
			if(root->songIDs[i]==-1){break;}
			size++;
		}
		
	}
	return size;
}


void BFS_print(treeNode* head ){
	msg("\nBFS BEGINING\n");
	if(head==NULL){msg("BFS:head is NULL\n");return;}
	treeNode *root = head->children[0],*fakeNode=node_alloc();
	struct Queue q;
	initializeQueue(&q);
	if(root==NULL){return ;}
	pushbottom(&q,root);
	pushbottom(&q,fakeNode);
	int doublefn=0;
	while((root=popTop(&q)) != NULL){

		if(root==fakeNode&& doublefn==0){
			msg("\n");
			pushbottom(&q,fakeNode);
			doublefn=1;
			continue;
		}
		doublefn=0;
		for(int i=0; i<4; ++i){
			if(root->children[i]==NULL){
				break; 
			}
			//printNode(root->children[i]);
			pushbottom(&q , root->children[i]);
		}
		if(root!=fakeNode){
			printNode(root);
		}
	}
	msg("BFS END\n");
}


int is4node(treeNode* node){
	if(node->songIDs[2]!=-1){
		return 1;
	}
	return 0;
}

int is2node(treeNode* node){
	if(node->songIDs[0]!=-1 && node->songIDs[1]==-1){
		return 1;
	}
	return 0;
}

treeNode* getLBrother(treeNode *node,treeNode *parent){
	for(int i=1;i<4;++i){
		if( parent->children[i] == node ){ return parent->children[i-1]; }
	}
	return NULL;
}

treeNode* getRBrother(treeNode *node,treeNode *parent){
	for(int i=0;i<3;++i){
		if( parent->children[i] == node ){return parent->children[i+1]; }
	}
	return NULL;
}

int take_from_brother(treeNode* brother, treeNode* parent , treeNode *node){
	if(is2node(brother) || !is2node(node) || brother==NULL || parent==NULL || node==NULL){return 0;}
	msg("\ntake from BROTHER BEFORE b p n\n");
	//printNode(brother);printNode(parent);printNode(node); ;msg("<----\n");
	msg("\nend  take from parent\n");
	if( brother->songIDs[0]<node->songIDs[0] ){  // left brother
		//printNode(brother);printf("<-left bother\n");
		int i=1;
		for(i=1;i<4;++i){
			if(parent->children[i]==node){
				break;
			}
		}

		insert(node , parent->songIDs[i-1]) ;
		remove_id(parent , parent->songIDs[i-1]);
		for(i=1;i<3;++i){
			if(brother->songIDs[i]==-1){
				i=i-1;
				break;
			}
		}
		msg("\nINSERT");printNode(parent);
		insert(parent , brother->songIDs[i]);
		msg("INSERT");printNode(parent);msg("INSERT\n");
		remove_id(brother,brother->songIDs[i]);
		node->children[2] = node->children[1];
		node->children[1] = node->children[0];
		node->children[0] =brother->children[i+1];
		brother->children[i+1]=NULL;

	}else if( brother->songIDs[0]>node->songIDs[0]){ // right brother
		//printNode(parent) ;printNode(brother);fprintf(stderr,"<-right bother\n");
		int i=1;
		for(i=0;i<3;++i){
			if(parent->children[i]==node){
				break;
			}
		}

		insert(node , parent->songIDs[i]) ;
		remove_id(parent , parent->songIDs[i]);

		insert(parent , brother->songIDs[0]);
		remove_id(brother,brother->songIDs[0]);
		node->children[2] = brother->children[0];
		brother->children[0]=brother->children[1];
		brother->children[1]=brother->children[2];;
		brother->children[2]=brother->children[3];
		brother->children[3]=NULL;
	}else{
		return 0;
	}
	
	return 1;
}

int take_from_parent(treeNode* head , treeNode* parent ,treeNode* child){
	if(parent==NULL){
		return 0;
	}
	
	if( is2node(parent)  ){
		if( parent==head->children[0] && is2node(parent->children[0]) && is2node(parent->children[1]) ){  //case that parent is the root
			treeNode* rc = parent->children[1];
			treeNode* lc = parent->children[0];
			if(child==rc){
				pthread_mutex_lock(&lc->lock);
			}else{
				pthread_mutex_lock(&rc->lock);
			}

			merge(head,parent,rc,lc);
			/*insert(parent,rc->songIDs[0]);
			insert(parent,lc->songIDs[0]);


			parent->children[0] = lc->children[0];	
			parent->children[1] = lc->children[1];
			parent->children[2] = rc->children[0];
			parent->children[3] = rc->children[1];*/
			if(child==rc){
				pthread_mutex_unlock(&lc->lock);
			}else{
				pthread_mutex_unlock(&rc->lock);
			}
			//todo free rc lc 

			return -1;

		}
		return 0;
	}

	treeNode* rc = getRBrother(child,parent) ;
	treeNode *lc = getLBrother(child,parent);

	int i=0;
	for(i=0;i<4;++i){
		if(parent->children[i]==child){break;}
	}
	if( lc!=NULL && is2node(lc) ){
		pthread_mutex_lock(&lc->lock);
		
		merge(head , parent,child,lc);
		/*insert(child,parent->songIDs[i-1]);
		remove_id(parent,parent->songIDs[i-1]);
		insert(child,lc->songIDs[0]);
		remove_id(lc,lc->songIDs[0]); //it s not neccesary to remove it because we free the node
		

		child->children[2]=child->children[0];
		child->children[3]=child->children[1];
		child->children[0]=lc->children[0];
		child->children[1]=lc->children[1];
		//todo free lc

		for(int j=i;j<4;++j){
			parent->children[j-1] = parent->children[j];
		}
		parent->children[3]=NULL;*/

		pthread_mutex_unlock(&lc->lock);
		return 1;
	}else if(rc!=NULL && is2node(rc)){
		pthread_mutex_lock(&rc->lock);
		merge(head , parent,rc,child);
		/*insert(child,parent->songIDs[i]);
		remove_id(parent,parent->songIDs[i]);
		insert(child,rc->songIDs[0]);
		remove_id(rc,rc->songIDs[0]); //it s not neccesary to remove it because we free the node

		child->children[2]=rc->children[0];
		child->children[3]=rc->children[1];
		//todo free rc

		for(int j=i+1;j<4;++j){
			parent->children[j] = parent->children[j+1];
		}
		parent->children[3]=NULL;
		*/
		pthread_mutex_unlock(&rc->lock);
		
		return 1;
	}
	return 0;
}

int merge(treeNode* head , treeNode* parent,treeNode* rc,treeNode* lc){
	treeNode* root=head->children[0];
	if( is2node(parent)  ){
		if( parent==root && is2node(parent->children[0]) && is2node(parent->children[1]) ){  //case that parent is the root

			insert(parent,rc->songIDs[0]);
			insert(parent,lc->songIDs[0]);


			parent->children[0] = lc->children[0];	
			parent->children[1] = lc->children[1];
			parent->children[2] = rc->children[0];
			parent->children[3] = rc->children[1];
			//todo free rc lc 
			return -1;

		}
		return 0;
	}
	int i=0;
	for(i=0;i<4;++i){
		if(parent->children[i]==lc){break;}
	}
	int parent_index=i;

	insert(lc,parent->songIDs[i]);
	remove_id(parent,parent->songIDs[i]);
	insert(lc,rc->songIDs[0]);
	remove_id(rc,rc->songIDs[0]); //it s not neccesary to remove it because we free the node

	lc->children[2]=rc->children[0];
	lc->children[3]=rc->children[1];
	//todo free rc

	for(int j=i+1;j<3;++j){
		parent->children[j] = parent->children[j+1];
	}
	parent->children[3]=NULL;
}
