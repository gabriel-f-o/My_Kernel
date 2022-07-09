/*
 * OS_Heap.c
 *
 *  Created on: May 29, 2021
 *      Author: Gabriel
 */

#include "OS/OS_Core/OS_Common.h"
#include "OS/OS_Core/OS_Heap.h"
#include "OS/OS_Core/OS_Callbacks.h"

/**********************************************
 * PRIVATE TYPES
 *********************************************/

typedef struct{
	uint32_t addr_next;  //Address of next / previous block (max 0x20020000)
	uint32_t block_used; //(0) Memory block free, (1) memory block occupied
} os_heap_header_t;

/**********************************************
 * PRIVATE VARIABLES
 *********************************************/

static __align(8) uint8_t os_heap[OS_HEAP_SIZE];	//Heap memory block

/**********************************************
 * PRIVATE FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Get Block Size
 *
 * @brief This function calculates the size of the block (header + data)
 *
 * @param heap_header_t* p : [in] address of the header of the block
 *
 * @return uint32_t : Size of the block in bytes (header + data)
 **********************************************************************/
inline static uint32_t os_heap_BlockGetSize(os_heap_header_t const * p){
	if(p == NULL) return 0; //Avoid explosion

	return ( (p->addr_next != 0) ? p->addr_next - (uint32_t)p : (uint32_t)&os_heap[sizeof(os_heap)] - (uint32_t)p ); //Calculate size in bytes
}

/***********************************************************************
 * OS Allocate Beginning
 *
 * @brief This function allocates at the beginning of a block, updating the list in the process
 *
 * @param heap_header_t* p : [in] address of the header of the block
 * @param uint32_t size    : [in] Size of the data part in bytes
 *
 * @return void* : Address of the data part of the block
 **********************************************************************/
static void* os_heap_AllocateBeginning(os_heap_header_t* p, uint32_t size){

	/* Check for argument errors
	 ---------------------------------------------------*/
	if(p == NULL) return NULL;
	if(size == 0) return NULL;

	/* Get references to manipulate
	 ---------------------------------------------------*/
	os_heap_header_t* newBlock   = p;
	os_heap_header_t* newTopHead = (os_heap_header_t*) ( (uint32_t) (newBlock) + sizeof(os_heap_header_t) + size);

	/* Add Block to list
	 ---------------------------------------------------*/
	newTopHead->addr_next = newBlock->addr_next != (uint32_t)newTopHead ? newBlock->addr_next : newTopHead->addr_next;
	newBlock->addr_next	  = (uint32_t) newTopHead;

	/* Mark block as used memory
	 ---------------------------------------------------*/
	newBlock->block_used = 1;

	/* Return reference of data block
	 ---------------------------------------------------*/
	return (void*) ( (uint32_t)newBlock + sizeof(os_heap_header_t) );
}

/***********************************************************************
 * OS Allocate End
 *
 * @brief This function allocates at the end of a block, updating the list in the process
 *
 * @param heap_header_t* p : [in] address of the header of the block
 * @param uint32_t size    : [in] Size of the data part in bytes
 *
 * @return void* : Address of the data part of the block
 **********************************************************************/
static void* os_heap_AllocateEnd(os_heap_header_t* p, uint32_t size){

	/* Check for argument errors
	 ---------------------------------------------------*/
	if(p == NULL) return NULL;
	if(size == 0) return NULL;

	/* Get references to manipulate
	 ---------------------------------------------------*/
	uint32_t block_size = os_heap_BlockGetSize(p);
	os_heap_header_t* newBlock   = (os_heap_header_t*) ( (uint32_t)p + block_size - size - sizeof(os_heap_header_t) );
	os_heap_header_t* oldTopHead = (os_heap_header_t*) (p);

	/* Add Block to list
	 ---------------------------------------------------*/
	newBlock->addr_next   = oldTopHead->addr_next;
	oldTopHead->addr_next = newBlock != oldTopHead ? (uint32_t) newBlock : oldTopHead->addr_next;

	/* Mark block as used memory
	 ---------------------------------------------------*/
	newBlock->block_used = 1;

	/* Return reference of data block
	 ---------------------------------------------------*/
	return (void*) ( (uint32_t)newBlock + sizeof(os_heap_header_t) );

}

/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Heap Clear
 *
 * @brief This function clears the heap
 *
 **********************************************************************/
void os_heap_clear(){

	/* Clear heap
	 ---------------------------------------------------*/
	memset(&os_heap, 0, sizeof(os_heap));

	/* Get header pointers
	 ---------------------------------------------------*/
	os_heap_header_t* p = (os_heap_header_t*) &os_heap[0];

	/* Initialize headers
	 ---------------------------------------------------*/
	p->block_used = 0; //Mark as not used
	p->addr_next  = 0; //Point outside of the heap to indicate end
}


/***********************************************************************
 * OS Heap Alloc
 *
 * @brief This function allocates an amount of bytes into the reserved heap
 *
 * @param uint32_t size : [in] Size to be allocated
 *
 * @return void* : Address of the memory block or NULL if the function failed (bad argument or not enough memory)
 **********************************************************************/
void* os_heap_alloc(uint32_t size){

	/* Check for argument errors
	 ---------------------------------------------------*/
	if(size == 0) return NULL;

	/* If the task gets interrupted, the heap may be corrupted when it recovers
	 ---------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Declare variables to get the tiniest block that has the size required
	 ---------------------------------------------------*/
	os_heap_header_t* pBlock = NULL;
	uint32_t min_size = 0xFFFFFFFF;
	os_heap_header_t* p = (os_heap_header_t*) &os_heap[0];

	/* Search for entire list
	 ---------------------------------------------------*/
	size_t totalSize = (size + 8U - 1U) & ~(8U - 1U); //Calculate the closest multiple of 8
	while(p != NULL){

		/* If the block is free, it's big enough and it's smaller than the previous, save it
		 ---------------------------------------------------*/
		uint32_t data_size = os_heap_BlockGetSize(p) - sizeof(os_heap_header_t);
		if(p->block_used == 0 && data_size < min_size && data_size >= totalSize) {
			pBlock = p;
			min_size = data_size;
		}

		/* Go to next block
		 ---------------------------------------------------*/
		p = (os_heap_header_t*) ( p->addr_next );
	}

	/* If pBlock is outside the heap, there is no memory available
	 ---------------------------------------------------*/
	if(!(&os_heap[0] <= (uint8_t*)pBlock && (uint8_t*)pBlock <= &os_heap[sizeof(os_heap) - 1] ) ) {

		/* Execute callback
		 ---------------------------------------------------*/
		os_insufficient_heap_cb();
		OS_EXIT_CRITICAL();
		return NULL;
	}

	/* If pBlock is not NULL, reserve a memory block
	 ---------------------------------------------------*/
	void* ret = ( (totalSize < OS_HEAP_BIG_BLOCK_THRESHOLD) ? os_heap_AllocateBeginning(pBlock, totalSize) : os_heap_AllocateEnd(pBlock, totalSize) );
	OS_EXIT_CRITICAL();
	return ret;

}


/***********************************************************************
 * OS Heap Free
 *
 * @brief This function frees a memory block previously allocated my OS_Heap_Alloc
 *
 * @param void* p : [in] Pointer to the data as given by Alloc
 *
 * @return OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_heap_free(void* p){

	/* Check for argument errors
	 ---------------------------------------------------*/
	if(p == NULL) return OS_ERR_BAD_ARG;
	if( !(&os_heap[sizeof(os_heap_header_t)] <= (uint8_t*)p && (uint8_t*)p <= &os_heap[sizeof(os_heap) - 1] ) ) return OS_ERR_BAD_ARG;

	/* If the task gets interrupted, the heap may be corrupted when it recovers
	 ---------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Declare Current block and target block
	 ---------------------------------------------------*/
	os_heap_header_t* cur   = (os_heap_header_t*)(&os_heap[0]);
	os_heap_header_t* block = (os_heap_header_t*)((uint32_t)p - sizeof(os_heap_header_t));

	/* Declare auxiliary pointers to help deleting
	 ---------------------------------------------------*/
	os_heap_header_t* pNext = NULL;
	os_heap_header_t* pPrev = NULL;

	/* Search for the target block while still inside the heap
	 ---------------------------------------------------*/
	while(&os_heap[0] <= (uint8_t*)cur && (uint8_t*)cur <= &os_heap[sizeof(os_heap) - 1] && cur != (os_heap_header_t*) block){

		/* Save current block and go to next block
		 ---------------------------------------------------*/
		pPrev = cur;
		cur = (os_heap_header_t*) ( cur->addr_next );
	}

	/* If the block was not found, or the block is outside the heap, return
	 ---------------------------------------------------*/
	if(cur != block || !(&os_heap[0] <= (uint8_t*)cur && (uint8_t*)cur <= &os_heap[sizeof(os_heap) - 1]) ) {
		OS_EXIT_CRITICAL();
		return OS_ERR_INVALID;
	}

	/* Get next address and tag the block as free
	 ---------------------------------------------------*/
	cur->block_used = 0;
	pNext = (os_heap_header_t*) ( cur->addr_next );

	/* Merge the current block with the next one if it not used (and exists)
	 ---------------------------------------------------*/
	if(pNext != NULL && pNext->block_used == 0){
		cur->addr_next = pNext->addr_next;
		pNext->addr_next = 0; //For principle, but not necessary
	}

	/* Merge the current block with the previous one if it not used (and exists)
	 ---------------------------------------------------*/
	if(pPrev != NULL && pPrev->block_used == 0){
		pPrev->addr_next = cur->addr_next;
		cur->addr_next = 0; //For principle, but not necessary
	}

	OS_EXIT_CRITICAL();
	return OS_ERR_OK;
}


/***********************************************************************
 * OS Heap Monitor
 *
 * @brief This function returns data about the heap's utilization
 *
 * @return os_heap_mon_t : Struct containing heap info
 **********************************************************************/
os_heap_mon_t os_heap_monitor(){

	/* Declare Return structure
	 ---------------------------------------------------*/
	os_heap_mon_t ret;
	memset(&ret, 0, sizeof(ret));
	ret.total_size = sizeof(os_heap);

	/* If the task gets interrupted, the heap may be corrupted when it recovers
	 ---------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Declare iterators
	 ---------------------------------------------------*/
	os_heap_header_t* pPrev = NULL;
	os_heap_header_t* pNext = NULL;
	os_heap_header_t* cur = (os_heap_header_t*)(&os_heap[0]);

	/* Search all heap
	 ---------------------------------------------------*/
	while(cur != NULL){

		/* Calculate block size
		 ---------------------------------------------------*/
		uint32_t block_sz = os_heap_BlockGetSize(cur);

		/* Get reference to next block
		 ---------------------------------------------------*/
		pNext = (os_heap_header_t*) ( cur->addr_next );

		/* Calculate if next and previous blocks are used
		 ---------------------------------------------------*/
		uint8_t prev_block_used = ( (pPrev != NULL) && (pPrev->block_used == 1) );
		uint8_t next_block_used = ( (pNext != NULL) && (pNext->block_used == 1) );

		/* Update return Data
		 ---------------------------------------------------*/
		ret.used_size += ( (cur->block_used == 1) ? block_sz : 0 );
		ret.fragmented_size += ( ( (next_block_used == 1) && (prev_block_used == 1) && (cur->block_used == 0) ) ? block_sz : 0 );
		ret.biggest_block_size = ( (cur->block_used == 1) && (block_sz > ret.biggest_block_size) ? block_sz : ret.biggest_block_size );

		/* Update iterator
		 ---------------------------------------------------*/
		pPrev = cur;
		cur = pNext;
	}

	/* Return data
	 ---------------------------------------------------*/
	OS_EXIT_CRITICAL();
	return ret;
}
