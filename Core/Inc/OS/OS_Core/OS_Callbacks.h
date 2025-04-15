/*
 * OS_Callbacks.h
 *
 *  Created on: May 30, 2021
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_CALLBACKS_H_
#define INC_OS_OS_CALLBACKS_H_

#include "OS/OS_Core/OS_Common.h"
#include "OS/OS_Core/OS_Obj.h"

/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/


/***********************************************************************
 * OS Task On Ready Callback
 *
 * @brief This function is called by the OS qfter a task that was blocked gets ready
 *
 * @param os_handle_t h : [in] Task that became ready
 *
 **********************************************************************/
void os_task_on_ready_cb(os_handle_t h);


/***********************************************************************
 * OS Task On Block Callback
 *
 * @brief This function is called by the OS before the current task blocks
 * This callback allows the user to perform a logic before interruption
 *
 * @param os_handle_t h : [in] Task that became ready
 *
 *
 **********************************************************************/
void os_task_on_block_cb(os_handle_t h);


/***********************************************************************
 * OS Idle task main funciton
 *
 * @brief This function is executed when the idle task is called (i.e) no other task is available
 *
 * @return : void* : generic return value
 *
 **********************************************************************/
__weak void* os_idle_task_fn(void* arg);


/***********************************************************************
 * OS No task ready callback
 *
 * @brief This function is executed in a loop if no task is ready. Only if a task wakes up
 * or is unblocked by an interrupt that this function will stop calling
 *
 * ATTENTION : This function is called in ISR time (PendSV)
 *
 **********************************************************************/
void os_no_task_ready_cb();


/***********************************************************************
 * OS Insufficient Heap Callback
 *
 * @brief This function is called by the OS when an os_alloc fails due to
 * not enogh memory
 *
 **********************************************************************/
void os_insufficient_heap_cb();

#endif /* INC_OS_OS_CALLBACKS_H_ */
