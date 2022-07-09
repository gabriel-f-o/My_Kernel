/*
 * OS_Config.h
 *
 *  Created on: May 29, 2021
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_CONFIG_H_
#define INC_OS_OS_CONFIG_H_

/**************************************************
 * BASIC CONFIGURATIONS
 *************************************************/

/* Enables FPU support in the OS
 ---------------------------------------------------*/
#define OS_FPU_EN								1


/* This define controls the size of the default function stack in bytes
 * It is recommended to have a multiple of 4
 ---------------------------------------------------*/
#define OS_DEFAULT_STACK_SIZE					1024

/**************************************************
 * HEAP CONFIGURATIONS
 *************************************************/

/* This define controls the amount of bytes will be reserved for the Heap Memory
 * The maximum amount of Heap to be used depends on the created tasks, the size of their stack
 * As well as user's allocations and allocation done by the MessageQ
 * The main function stack is not included in the heap
 ---------------------------------------------------*/
#define OS_HEAP_SIZE							(80 * 1024ULL)


/* Heap changes behavior when allocating large blocks (i.e. bigger than or equal to this define)
 * This logic reduces memory fragmentation
 ---------------------------------------------------*/
#define OS_HEAP_BIG_BLOCK_THRESHOLD				50


#endif /* INC_OS_OS_CONFIG_H_ */
