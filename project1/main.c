#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "tree.h"
#include "UnboundedLockFreeQueue.h"
#include "linckedList_lazy.h"

typedef struct thread{
	int id;
	pthread_t thread_id;
}thread;

unsigned long long int N=0;
unsigned long long int M=0;

treeNode* head;
pthread_barrier_t phase1;
pthread_barrier_t phase2;
pthread_barrier_t phase3;
pthread_barrier_t phase4;
pthread_barrier_t phase5;

QUEUE *queues;

pthread_mutex_t lock;

void *function(void* arg){
	//uploading------------------------------------------------------------------------------------------------------------------------------
	int *id = arg;

	for(int i=0;i<N;i++){
		int v = ( i*N+(*id));
		BSTInsert(head, v );

	}
	pthread_barrier_wait(&phase1);

	if(*id==0){
		//BFS_print(head);
		unsigned long long int size = N*N;
		unsigned long long int sum = N*N*(N-1)*(N+1)/2;
		unsigned long long int BFSsize = BFS_size_check(head);
		unsigned long long int BFSsum = BFS_sum_check(head);

		printf("\ntotal size check passed (expected: %llu, found: %llu)\n", size ,BFSsize );
		printf("total keysum check passed (expected: %llu, found: %llu)\n", sum , BFSsum);
		if(size!=BFSsize){
			printf("failure:BST size is wrong\n");
			exit(0);
		}
		if(sum!=BFSsum){
			printf("failure:BST checksum is wrong\n");
			exit(0);
		}
	}

	//pthread_barrier_wait(&phase2);
	//pthread_barrier_wait(&phase3);
	//pthread_barrier_wait(&phase4);
	//pthread_barrier_wait(&phase5);


	//end uploading----------------------------------------------------------------------------------------------------------------------------------------
	//enqueue-----------------------------------------------------------------------------------------------------------------------------
	for(int j=0;j<N;++j){
		int pos = (j+1)%M;
		int songid = N*(*id)+j;
		treeNode* tn = BSTsearch(head,songid);
		if(tn!=NULL){
			enq( &(queues[pos]), songid ); 
		}else{
			printf("%d songID does not found in BST\n",songid);
			exit(0);
		}
	}
	pthread_barrier_wait(&phase2);
	
	if(*id==0){
		unsigned long long int qsize = 0;
		unsigned long long int qsum = 0;
		for(int i=0;i<M;++i){
			QNODE* tmp = queues[i].Head;
			while( tmp!=queues[i].Tail ){
				tmp=tmp->next;
				qsize++;
				qsum+= tmp-> songID;
			}
			if(qsize!=(i+1)*2*N ){
				printf("queue[%d] has wrong size %lu\n",i,qsize);
				exit(0);
			}
		}
		unsigned long long int size = N*N;
		unsigned long long int sum = N*N*(N-1)*(N+1)/2;
		printf("\nqueues total size check passed (expected: %llu, found: %llu)\n", size ,qsize );
		printf("total keysum check passed (expected: %llu, found: %llu)\n", sum , qsum);
		if(size!=qsize){
			printf("failure:queues total size is wrong\n");
			exit(0);
		}
		if(sum!=qsum){
			printf("failure:queues checksum is wrong\n");
			exit(0);
		}
	}
	//end enqueue---------------------------------------------------------------------------------------------------------------------------
	
	pthread_barrier_wait(&phase3);
	int qnum = ((*id)+1)%M;
	int *dq_res=malloc(sizeof(int));
	*dq_res=-1;
	for(int i=0;i<M;++i){
		deq(&queues[qnum],dq_res);
		//printf("%d " ,  *dq_res);
		treeNode* search_res = BSTsearch(head,*dq_res);
		if(search_res!=NULL){
			BSTDelete(head,*dq_res);
			//printf("(%d %d)\n", *dq_res , lhead->songID);
			//printf("%d , %d\n",*dq_res , i<M);
			linsert(*dq_res);
		}else{
			
			exit(0);
		}

	}	

	pthread_barrier_wait(&phase4);

	if(*id==0){
		//BFS_print(head);
		unsigned long int tsize = N*N/2;
		unsigned long int BFSsize=0;
		BFSsize = BFS_size_check(head);
		printf("\ntree's size check passed (expected: %lu, found: %lu)\n", tsize ,BFSsize );
		if(tsize!=BFSsize){
			printf("failure:BST size is wrong\n");
			//exit(0);
		}


		unsigned long int qsize = 0;
		for(int i=0;i<M;++i){
			QNODE* tmp = queues[i].Head;
			while( tmp!=queues[i].Tail ){
				tmp=tmp->next;
				qsize++;
			}
		}
		unsigned long int size = N*N/2;
		printf("\nqueues total size check passed (expected: %lu, found: %lu)\n", size ,qsize );
		if(size!=qsize){
			printf("failure:queues size is wrong\n");
			exit(0);
		}


		unsigned long int expectedsongs = N*N/2;
		unsigned long int songs =0;
		LNODE* tmp = lhead->next;
		while(tmp!=ltail){
			songs++;
			tmp=tmp->next;
		}
		printf("\nlist's size check passed (expected: %lu, found: %lu)\n", expectedsongs ,songs );
		if(expectedsongs!=songs){
			printf("failure:list's size is wrong\n");
			exit(0);
		}



	}
	pthread_barrier_wait(&phase5);
	
	return NULL;
}


void  main(int argc, char **argv){
	printf("%s %s\n",argv[0],argv[1]);
	if(argv[1]==NULL){
		fprintf(stderr,"give the number of threads\n");
		exit(0);
	}
	N = atoi(argv[1]);
	M = N/2;
	queues = malloc(M*sizeof(QUEUE));
	for(int j=0;j<M;++j){
		qinit(&queues[j]);
	}

	linit();
	
	pthread_barrier_init(&phase1, NULL, N+1);
	pthread_barrier_init(&phase2, NULL, N+1);
	pthread_barrier_init(&phase3, NULL, N+1);
	pthread_barrier_init(&phase4, NULL, N+1);
	pthread_barrier_init(&phase5, NULL, N+1);

	head = initializeBST();

	thread thread_ids[N]; 
	for(int i=0;i<N;++i){
		thread_ids[i].id = i;

		if(pthread_create(&thread_ids[i].thread_id, NULL, function , &thread_ids[i].id) <0){
			perror("pthread_create");
		}
	} 

	pthread_barrier_wait(&phase1);
	pthread_barrier_wait(&phase2);
	pthread_barrier_wait(&phase3);
	pthread_barrier_wait(&phase4);
	pthread_barrier_wait(&phase5);
	//for(int i=0;i<N;++i){
		//pthread_join(thread_ids[0].thread_id, NULL);
	//}
	pthread_barrier_destroy(&phase1);
	pthread_barrier_destroy(&phase2);
	pthread_barrier_destroy(&phase3);
	pthread_barrier_destroy(&phase4);
	pthread_barrier_destroy(&phase5);

    exit(0); 


	return ;
}
