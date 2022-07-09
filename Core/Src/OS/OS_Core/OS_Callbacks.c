/*
 * OS_Callbacks.c
 *
 *  Created on: May 30, 2021
 *      Author: Gabriel
 */

#include "OS/OS_Core/OS_Common.h"
#include "OS/OS_Core/OS_Obj.h"
#include "OS/OS_Core/OS_Callbacks.h"

/***********************************************************************
 * OS CALLBACKS
 **********************************************************************/


/***********************************************************************
 * OS Task On Ready Callback
 *
 * @brief This function is called by the OS qfter a task that was blocked gets ready
 *
 * @param os_handle_t h : [in] Task that became ready
 *
 **********************************************************************/
__weak void os_task_on_ready_cb(os_handle_t h){
	UNUSED_ARG(h);
	return;
}


/***********************************************************************
 * OS Task On Block Callback
 *
 * @brief This function is called by the OS before the current task blocks
 * This callback allows the user to perform a logic before interruption
 *
 * @param os_handle_t h : [in] Task that became ready
 *
 **********************************************************************/
__weak void os_task_on_block_cb(os_handle_t h){
	UNUSED_ARG(h);
	return;
}


/***********************************************************************
 * OS Idle task main funciton
 *
 * @brief This function is executed when the idle task is called (i.e) no other task is available
 *
 * @return : void* : generic return value
 *
 **********************************************************************/
__weak void* os_idle_task_fn(void* i){
	UNUSED_ARG(i);
	while(1){
	}
}


/***********************************************************************
 * OS No task ready callback
 *
 * @brief This function is executed in a loop if no task is ready. Only if a task wakes up
 * or is unblocked by an interrupt that this function will stop calling
 *
 * ATTENTION : This function is called in ISR time (PendSV)
 *
 **********************************************************************/
__weak void os_no_task_ready_cb(){
	return;
}


/***********************************************************************
 * OS Insufficient Heap Callback
 *
 * @brief This function is called by the OS when an os_alloc fails due to
 * not enogh memory
 *
 **********************************************************************/
__weak void os_insufficient_heap_cb(){
	return;
}

