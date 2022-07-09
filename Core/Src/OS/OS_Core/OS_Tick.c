/*
 * OS_Tick.c
 *
 *  Created on: Jun 4, 2021
 *      Author: Gabriel
 */

#include "OS/OS_Core/OS_Config.h"
#include "OS/OS_Core/OS_Common.h"
#include "OS/OS_Core/OS_Tick.h"
#include "OS/OS_Core/OS_Tasks.h"
#include "OS/OS_Core/OS_Internal.h"
#include "OS/OS_Core/OS_Obj.h"
#include "OS/OS_Core/OS_Scheduler.h"

/**********************************************
 * EXTERNAL VARIABLES
 *********************************************/

extern os_list_cell_t* os_cur_task;
extern os_list_head_t  os_head;

/**********************************************
 * PRIVATE VARIABLES
 *********************************************/

static uint32_t os_ticks_ms;

/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Get ms Tick
 *
 * @brief This function returns the OS' ms tick count
 *
 * @return uint32_t tick count
 **********************************************************************/
uint32_t os_getMsTick(){
	return os_ticks_ms;
}


/***********************************************************************
 * OS Tick
 *
 * @brief This function is called periodically to tick tasks' counters and call the scheduler when necessary
 *
 * @param uint32_t increment : [in] amount of ms to increment
 *
 **********************************************************************/
void os_tick(uint32_t ms_inc){

	/* Enter Critical -> If the list is changed during the process, this can corrupt our references
	 ------------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Increment ticks
	 ------------------------------------------------------*/
	os_ticks_ms += ms_inc;

	/* Create iterators
	 ------------------------------------------------------*/
	uint8_t pend_req = 0;
	os_list_cell_t* it = os_head.head.next;
	int8_t cur_prio = ( (os_cur_task == NULL) ? -1 : os_task_getPrio(os_cur_task->element) );

	/* Check if there is any high priority task to execute and wake up tasks
	 ------------------------------------------------------*/
	while(it != NULL){

		/* Tick counter if necessary
		 ------------------------------------------------------*/
		if( ((os_task_t*)it->element)->wakeCoutdown > 0 && ((os_task_t*)it->element)->wakeCoutdown < OS_WAIT_FOREVER)
			((os_task_t*)it->element)->wakeCoutdown = ((os_task_t*)it->element)->wakeCoutdown < ms_inc ? 0 : ((os_task_t*)it->element)->wakeCoutdown - ms_inc;

		/* Wake up blocked functions if timeout has elapsed
		 ------------------------------------------------------*/
		if( ((os_task_t*)it->element)->wakeCoutdown == 0 && ((os_task_t*)it->element)->state == OS_TASK_BLOCKED) ((os_task_t*)it->element)->state = OS_TASK_READY;

		/* If current task is ready
		 ------------------------------------------------------*/
		if( ((os_task_t*)it->element)->state == OS_TASK_READY ){

			/* Calculate task priority
			 ------------------------------------------------------*/
			int8_t task_prio = os_task_getPrio(it->element);

			/* If there is a task with priority higher or equal to current task that is ready, than scheduling is called
			 ------------------------------------------------------*/
			 if(task_prio >= cur_prio && it != os_cur_task) pend_req = 1;

		}

		/* Keep going
		 ------------------------------------------------------*/
		it = it->next;
	}

	/* PendSV if necessary
	 ------------------------------------------------------*/
	if(pend_req == 1 && os_scheduler_state_get() == OS_SCHEDULER_START) os_task_yeild();

	/* Return
	 ------------------------------------------------------*/
	OS_EXIT_CRITICAL();
	return;
}
