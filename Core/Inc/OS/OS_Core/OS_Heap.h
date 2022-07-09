/*
 * OS_Heap.h
 *
 *  Created on: May 29, 2021
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_HEAP_H_
#define INC_OS_OS_HEAP_H_

#include "OS/OS_Core/OS_Common.h"

/**********************************************
 * PUBLIC TYPES
 *********************************************/

typedef struct{
	uint32_t total_size;			//Total size in bytes
	uint32_t used_size;				//Amount of bytes used
	uint32_t biggest_block_size;	//Size of the biggest used block in bytes
	uint32_t fragmented_size;		//The size in bytes of all fragmented blocks (a fragmented block is a free block between two used blocks)
} os_heap_mon_t; //Heap Monitor structure

/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/


/***********************************************************************
 * OS Heap Clear
 *
 * @brief This function clears the heap
 *
 **********************************************************************/
void os_heap_clear();


/***********************************************************************
 * OS Heap Alloc
 *
 * @brief This function allocates an amount of bytes into the reserved heap
 *
 * @param uint32_t size : [in] Size to be allocated
 *
 * @return void* : Address of the memory block or NULL if the function failed (bad argument or not enough memory)
 **********************************************************************/
void* os_heap_alloc(uint32_t size);


/***********************************************************************
 * OS Heap Free
 *
 * @brief This function frees a memory block previously allocated my OS_Heap_Alloc
 *
 * @param void* p : [in] Pointer to the data as given by Alloc
 *
 * @return OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_heap_free(void* p);


/***********************************************************************
 * OS Heap Monitor
 *
 * @brief This function returns data about the heap's utilization
 *
 * @return os_heap_mon_t : Struct containing heap info
 **********************************************************************/
os_heap_mon_t os_heap_monitor();


#endif /* INC_OS_OS_HEAP_H_ */
