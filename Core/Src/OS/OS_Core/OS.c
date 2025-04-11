/*
 * OS.c
 *
 *  Created on: May 30, 2021
 *      Author: Gabriel
 */

#include "OS/OS_Core/OS.h"
#include "OS/OS_Core/OS_Internal.h"

/**********************************************
 * PRIVATE VARIABLES
 *********************************************/

static bool os_init_done; //(0) init not done ; (1) init done

/**********************************************
 * EXTERNAL VARIABLES
 *********************************************/

extern os_handle_t idle_task;	//Idle task handle

/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/


/***********************************************************************
 * OS Init Get
 *
 * @brief This function informs other processes that the OS is initialized
 *
 * @return bool : 1 = done
 *
 **********************************************************************/
bool os_init_get(){
	return os_init_done;
}


/***********************************************************************
 * OS Init
 *
 * @brief This function initializes the OS environment, creating the main and idle tasks, initializing their stacks, and initializing OS's heap
 *
 * @param char* main_name				: [in] main task name. A null name creates a nameless task.
 * @param int8_t   main_task_priority   : [in] main stack base priority (0 to 127)
 * @param uint32_t interrput_stack_size : [in] size of the interrupt stack
 * @param char* idle_name				: [in] idle task name. A null name creates a nameless task.
 * @param uint32_t idle_stack_size 		: [in] size of the idle stack
 *
 * @return os_err_e : Error code (0 = OK)
 *
 **********************************************************************/
os_err_e os_init(char* main_name, int8_t main_task_priority, uint32_t interrput_stack_size, char* idle_name, uint32_t idle_stack_size){

	/* Check if OS was already init
	 ------------------------------------------------------*/
	if(os_init_done == 1) return OS_ERR_FORBIDDEN;

	/* Init FPU if enabled
	 ------------------------------------------------------*/
#if defined(OS_FPU_EN) && OS_FPU_EN == 1
	OS_FPU_ENABLE();			//Enable
	OS_FPU_LAZY_ENABLE();		//Always lazy save
	OS_FPU_STATUS_ENABLE();		//Allows FPU to indicate that it is active
#endif

	/* Set priorities for pendSv and systick
	 ------------------------------------------------------*/
	OS_SYSTICK_DISABLE();
	OS_SYSTICK_SET_PRIO(2);
	OS_PENDSV_SET_PRIO(15);
	OS_SYSTICK_ENABLE();

	/* Init Heap
	 ------------------------------------------------------*/
	os_heap_clear();

	/* Init Tasks
	 ------------------------------------------------------*/
	os_err_e ret = os_task_init(main_name, main_task_priority, interrput_stack_size, idle_stack_size);
	if(ret != OS_ERR_OK)
		return ret;

	/* Flag init as done
	 ------------------------------------------------------*/
	os_init_done = ret == OS_ERR_OK ? 1 : 0;

	return os_task_create(&idle_task, idle_name, os_idle_task_fn, OS_TASK_MODE_DELETE, 0, idle_stack_size, 0, NULL);
}

