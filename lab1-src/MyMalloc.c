/*
 * CS252: MyMalloc Project
 *
 * The current implementation gets memory from the OS
 * every time memory is requested and never frees memory.
 *
 * You will implement the allocator as indicated in the handout,
 * as well as the deallocator.
 *
 * You will also need to add the necessary locking mechanisms to
 * support multi-threaded programs.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <assert.h> //TODO: check this library
#include "MyMalloc.h"

static pthread_mutex_t mutex;

const int arenaSize = 0x200000;

void increaseMallocCalls()  { _mallocCalls++; }

void increaseReallocCalls() { _reallocCalls++; }

void increaseCallocCalls()  { _callocCalls++; }

void increaseFreeCalls()    { _freeCalls++; }

extern void atExitHandlerInC()
{
    atExitHandler();
}

/* 
 * Initial setup of allocator. First chunk is retrieved from the OS,
 * and the fence posts and freeList are initialized.
 */
void initialize()
{
    // Environment var VERBOSE prints stats at end and turns on debugging
    // Default is on
    _verbose = 1;
    const char *envverbose = getenv("MALLOCVERBOSE");
    if (envverbose && !strcmp(envverbose, "NO")) {
        _verbose = 0;
    }

    pthread_mutex_init(&mutex, NULL);
    void *_mem = getMemoryFromOS(arenaSize);

    // In verbose mode register also printing statistics at exit
    atexit(atExitHandlerInC);

    // establish fence posts
    ObjectHeader * fencePostHead = (ObjectHeader *)_mem;
    fencePostHead->_allocated = 1;
    fencePostHead->_objectSize = 0;

    char *temp = (char *)_mem + arenaSize - sizeof(ObjectHeader);
    ObjectHeader * fencePostFoot = (ObjectHeader *)temp;
    fencePostFoot->_allocated = 1;
    fencePostFoot->_objectSize = 0;

    // Set up the sentinel as the start of the freeList
    _freeList = &_freeListSentinel; //TODO: initialize freelistsentinel?

    // Initialize the list to point to the _mem
    temp = (char *)_mem + sizeof(ObjectHeader);
    ObjectHeader *currentHeader = (ObjectHeader *)temp;
    currentHeader->_objectSize = arenaSize - (2*sizeof(ObjectHeader)); // ~2MB
    currentHeader->_leftObjectSize = 0;
    currentHeader->_allocated = 0;
    currentHeader->_listNext = _freeList;
    currentHeader->_listPrev = _freeList;
    _freeList->_listNext = currentHeader;
    _freeList->_listPrev = currentHeader;

    // Set the start of the allocated memory
    _memStart = (char *)currentHeader;

    _initialized = 1;
}

/* 
 * TODO: In allocateObject you will handle retrieving new memory for the malloc
 * request. The current implementation simply pulls from the OS for every
 * request.
 * pthread_mutex_lock(&mutex) is called before this function, be sure to release it before return
 * @param: amount of memory requested
 * @return: pointer to start of useable memory
 */
void * allocateObject(size_t size)
{
	// Make sure that allocator is initialized
    if (!_initialized)
        initialize();
    
    /* Add the ObjectHeader to the size and round the total size up to a 
     * multiple of 8 bytes for alignment.
     */
    size_t roundedSize = (size + sizeof(ObjectHeader) + 7) & ~7;
    while(1){
	    //Step 3, Traverse tbe free list and find the first contiguous block large
	    //enough for object.
	    int found = 0;
	    ObjectHeader * temp = _freeList->_listNext;
	    while (temp != _freeList && found == 0){
		    if (temp->_allocated == 0 && temp->_objectSize >= roundedSize){
			    found = 1;
		    } else {
			    temp = temp->_listNext;
		    }
	    }
	    //check if end
	    if (found == 1){
		    //Check if splitting possible
		    assert(temp!= NULL);
		    assert(temp->_objectSize >= roundedSize);
		    ObjectHeader * newBlock;
		    if (temp->_objectSize >= (roundedSize + 8 + sizeof(ObjectHeader))){ //TODO: Change 8 to const MINBS
			    //Splitting
			    char * _allocBlockMem = (char*) temp + (temp->_objectSize - roundedSize); 
			    newBlock = (ObjectHeader*) _allocBlockMem;
			    newBlock->_objectSize = roundedSize;
			    newBlock->_allocated = 1;
			    temp->_objectSize = temp->_objectSize - roundedSize;
			    newBlock->_leftObjectSize = temp->_objectSize;
			    ObjectHeader * leftBlock = (ObjectHeader*) ((char*)newBlock + newBlock->_objectSize);
			    leftBlock->_leftObjectSize = newBlock->_objectSize;
			    pthread_mutex_unlock(&mutex);
			    return (void *)((char *)newBlock + sizeof(ObjectHeader));
		    } else {
		    	//no block split
			    temp->_listPrev->_listNext = temp->_listNext;
			    temp->_listNext->_listPrev = temp->_listPrev;
			    temp->_allocated = 1; 
			    ObjectHeader * leftBlock = (ObjectHeader*) ((char*)newBlock + newBlock->_objectSize);
			    leftBlock->_leftObjectSize = newBlock->_objectSize;
			    pthread_mutex_unlock(&mutex);
			    return (void *)((char *)temp + sizeof(ObjectHeader));
		    }
		    pthread_mutex_unlock(&mutex);
		    return (void *)((char *)temp + sizeof(ObjectHeader));
	    } 
	    //if at the end allocate more memory 
	    void *_mem = getMemoryFromOS(arenaSize);
	    //atexit(atExitHandlerInC);

	    // establish fence posts
	    ObjectHeader * fencePostHead = (ObjectHeader *)_mem;
	    fencePostHead->_allocated = 1;
	    fencePostHead->_objectSize = 0;

	    char *temp2 = (char *)_mem + arenaSize - sizeof(ObjectHeader);
	    ObjectHeader * fencePostFoot = (ObjectHeader *)temp2;
	    fencePostFoot->_allocated = 1;
	    fencePostFoot->_objectSize = 0;

	    temp2 = (char *)_mem + sizeof(ObjectHeader);
	    ObjectHeader *currentHeader = (ObjectHeader *)temp2;
	    currentHeader->_objectSize = arenaSize - (2*sizeof(ObjectHeader)); // ~2MB
	    currentHeader->_leftObjectSize = 0;
	    currentHeader->_allocated = 0;
	    currentHeader->_listNext = _freeList->_listNext;
	    currentHeader->_listPrev = _freeList;
	    _freeList->_listNext = currentHeader;
	    currentHeader->_listNext->_listPrev = currentHeader;
    }
}



/* 
 * TODO: In freeObject you will implement returning memory back to the free
 * list, and coalescing the block with surrounding free blocks if possible.
 *
 * @param: pointer to the beginning of the block to be returned
 * Note: ptr points to beginning of useable memory, not the block's header
 */
void freeObject(void *ptr)
{
	char* _ptrMem = (char*) ptr - (sizeof(ObjectHeader));
	ObjectHeader * _ptrHead = (ObjectHeader *) _ptrMem;
	char* _leftBlockMem = (char *) _ptrHead - (_ptrHead->_leftObjectSize);
	char* _rightBlockMem = (char*) _ptrHead + (_ptrHead->_objectSize);
	ObjectHeader * leftHeader = (ObjectHeader *) _leftBlockMem;
	ObjectHeader * rightHeader = (ObjectHeader*) _rightBlockMem;
	size_t un_right = rightHeader->_allocated;
	size_t un_left = leftHeader->_allocated;
	_ptrHead->_allocated = 0;
	int needToInsert = 1;
	ObjectHeader * currentBlockHead = _ptrHead;
	if(un_left == 0){
		size_t newSize = leftHeader->_objectSize + _ptrHead->_objectSize;
		leftHeader->_objectSize = newSize;
		currentBlockHead = leftHeader;
		needToInsert = 0;
	}
	if(un_right == 0){
		if (currentBlockHead == leftHeader){
			leftHeader->_objectSize = leftHeader->_objectSize + rightHeader->_objectSize;
			//leftHeader->_listNext = rightHeader->_listNext;
			//leftHeader->_listNext->_listPrev = leftHeader;
			rightHeader->_listNext->_listPrev = rightHeader->_listPrev;
			rightHeader->_listPrev->_listNext = rightHeader->_listNext;
		} else {
			_ptrHead->_objectSize = _ptrHead->_objectSize + rightHeader->_objectSize;
			_ptrHead->_listNext = rightHeader->_listNext;
			_ptrHead->_listNext->_listPrev = _ptrHead;
		}
	}
	if (needToInsert == 1){
		currentBlockHead->_listNext = _freeList->_listNext;
		currentBlockHead->_listPrev = _freeList;
		currentBlockHead->_listNext->_listPrev = currentBlockHead;
		_freeList->_listNext = currentBlockHead;
	}
	pthread_mutex_unlock(&mutex);
	return;
}

/* 
 * Prints the current state of the heap.
 */
void print()
{
	printf("\n-------------------\n");

	printf("HeapSize:\t%zd bytes\n", _heapSize );
	printf("# mallocs:\t%d\n", _mallocCalls );
	printf("# reallocs:\t%d\n", _reallocCalls );
	printf("# callocs:\t%d\n", _callocCalls );
	printf("# frees:\t%d\n", _freeCalls );

	printf("\n-------------------\n");
}

/* 
 * Prints the current state of the freeList
 */
void print_list() {
	printf("FreeList: ");
	if (!_initialized) 
		initialize();

	ObjectHeader * ptr = _freeList->_listNext;

	while (ptr != _freeList) {
		long offset = (long)ptr - (long)_memStart;
		printf("[offset:%ld,size:%zd]", offset, ptr->_objectSize);
		ptr = ptr->_listNext;
		if (ptr != NULL)
			printf("->");
	}
	printf("\n");
}

/* 
 * This function employs the actual system call, sbrk, that retrieves memory
 * from the OS.
 *
 * @param: the chunk size that is requested from the OS
 * @return: pointer to the beginning of the chunk retrieved from the OS
 */
void * getMemoryFromOS(size_t size)
{
	_heapSize += size;

	// Use sbrk() to get memory from OS
	void *_mem = sbrk(size);

	// if the list hasn't been initialized, initialize memStart to mem
	if (!_initialized)
		_memStart = _mem;

	return _mem;
}

void atExitHandler()
{
	// Print statistics when exit
	if (_verbose)
		print();
}

/*
 * C interface
 */

extern void * malloc(size_t size)
{
	pthread_mutex_lock(&mutex);
	increaseMallocCalls();

	return allocateObject(size);
}

extern void free(void *ptr)
{
	pthread_mutex_lock(&mutex);
	increaseFreeCalls();

	if (ptr == 0) {
		// No object to free
		pthread_mutex_unlock(&mutex);
		return;
	}

	freeObject(ptr);
}

extern void * realloc(void *ptr, size_t size)
{
	pthread_mutex_lock(&mutex);
	increaseReallocCalls();

	// Allocate new object
	void *newptr = allocateObject(size);

	// Copy old object only if ptr != 0
	if (ptr != 0) {

		// copy only the minimum number of bytes
		ObjectHeader* hdr = (ObjectHeader *)((char *) ptr - sizeof(ObjectHeader));
		size_t sizeToCopy =  hdr->_objectSize;
		if (sizeToCopy > size)
			sizeToCopy = size;

		memcpy(newptr, ptr, sizeToCopy);

		//Free old object
		freeObject(ptr);
	}

	return newptr;
}

extern void * calloc(size_t nelem, size_t elsize)
{
	pthread_mutex_lock(&mutex);
	increaseCallocCalls();

	// calloc allocates and initializes
	size_t size = nelem *elsize;

	void *ptr = allocateObject(size);

	if (ptr) {
		// No error; initialize chunk with 0s
		memset(ptr, 0, size);
	}

	return ptr;
}

