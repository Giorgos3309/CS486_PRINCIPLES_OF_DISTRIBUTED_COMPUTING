all:  project1

project1: main BSTLockBasedTopDown UnboundedLockFreeQueue linckedList_lazy
	gcc  --std=c11 -D_GNU_SOURCE -pthread -O0 main.o BSTLockBasedTopDown.o  UnboundedLockFreeQueue.o  linckedList_lazy.o

main:
	gcc  --std=c11 -D_GNU_SOURCE -c -O0 -pthread main.c

BSTLockBasedTopDown:
	gcc --std=c11 -D_GNU_SOURCE -c -O0 -pthread BSTLockBasedTopDown.c

UnboundedLockFreeQueue:
	gcc --std=c11 -D_GNU_SOURCE -c -O0 -pthread UnboundedLockFreeQueue.c

linckedList_lazy:
	gcc --std=c11 -D_GNU_SOURCE -c -O0 -pthread linckedList_lazy.c

clean:
	rm *.o 
