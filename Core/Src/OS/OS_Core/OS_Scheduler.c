/*
 * OS_Scheduler.c
 *
 *  Created on: Oct 23, 2021
 *      Author: Gabriel
 */


#include "OS/OS_Core/OS.h"
#include "OS/OS_Core/OS_Config.h"
#include "OS/OS_Core/OS_Common.h"
#include "OS/OS_Core/OS_Scheduler.h"
#include "OS/OS_Core/OS_Tasks.h"
#include "OS/OS_Core/OS_Internal.h"
#include "OS/OS_Core/OS_Callbacks.h"

/**********************************************
 * EXTERNAL VARIABLES
 *********************************************/

extern os_list_cell_t* os_cur_task;	//Current task pointer
extern os_list_head_t  os_head;		//Pointer to task list

/**********************************************
 * PRIVATE VARIABLES
 *********************************************/

static os_scheduler_state_e state = OS_SCHEDULER_STOP; //Current state of the scheduler

/**********************************************
 * PRIVATE FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Round Robin
 *
 * @brief This function decides the task that will gain the cpu
 *
 * @return task_list_t : Reference to the chosen task
 **********************************************************************/
static os_list_cell_t* os_round_robin(){

	/* Declare iterators
	 ------------------------------------------------------*/
	os_list_cell_t* it = os_head.head.next;
	os_list_cell_t* it_low = os_cur_task;

	/* Saves first higher priority available in the higher side (all tasks before the current one in list order)
	 ------------------------------------------------------*/
	int8_t highSidePrio = -1;
	os_list_cell_t* highSide= NULL;

	/* Saves first higher priority available in the lower side (all tasks after and including the current one in list order)
	 ------------------------------------------------------*/
	int8_t lowSidePrio = -1;
	os_list_cell_t* lowSide = NULL;

	/* Search the high side of the list
	 ------------------------------------------------------*/
	while(it != os_cur_task){

		/* Check only ready tasks
		 ------------------------------------------------------*/
		if(((os_task_t*)it->element)->state == OS_TASK_READY){

			/* Calculate task priority
			 ------------------------------------------------------*/
			int8_t task_prio = os_task_getPrio((os_handle_t)it->element);

			/* If the task is ready and its priority is higher, save it
			 ------------------------------------------------------*/
			if(highSidePrio <= task_prio){
				highSidePrio = task_prio;
				highSide = it;
			}

		}

		/* Otherwise continue
		 ------------------------------------------------------*/
		it = it->next;
	}

	/* Search the low side of the list
	 ------------------------------------------------------*/
	while(it_low != NULL){

		/* Check only ready tasks
		 ------------------------------------------------------*/
		if(((os_task_t*)it_low->element)->state == OS_TASK_READY){

			/* Calculate task priority
			 ------------------------------------------------------*/
			int8_t task_prio = os_task_getPrio((os_handle_t)it_low->element);

			/* If the task is ready and its priority is higher, save it
			 ------------------------------------------------------*/
			if(lowSidePrio <= task_prio ){
				lowSidePrio = task_prio;
				lowSide = it_low;
			}

		}

		it_low = it_low->next;
	}

	/* If nothing was found, return NULL
	 ------------------------------------------------------*/
	if(lowSide == NULL && highSide == NULL) return NULL;

	/* If only the low side found something, return it
	 ------------------------------------------------------*/
	if(highSide == NULL && lowSide != NULL) return lowSide;

	/* If only the high side found something, return it
	 ------------------------------------------------------*/
	if(lowSide == NULL && highSide != NULL) return highSide;

	/* If both found something, return the task with higher priority.
	 * If both are equal, chose the lower side to peform the round robin for the same priority task
	 ------------------------------------------------------*/
	return ( (lowSidePrio > highSidePrio) ? lowSide : highSide);
}

/***********************************************************************
 * OS Scheduler
 *
 * @brief This function performs the stack management of the context change (must be called in PENDSV interrupt)
 *
 **********************************************************************/
__used static void os_scheduler(){

	/* Test if scheduler is ON
	 ------------------------------------------------------*/
	if(state != OS_SCHEDULER_START) return;

	/* Enter Critical -> If the list is changed during the process, this can corrupt our references
	 ------------------------------------------------------*/
	__os_disable_irq();

	/* Save current task stack into task memory block
	 ------------------------------------------------------*/
	register uint32_t volatile psp = 0;
	__asm volatile ("mrs %[out], psp" : [out] "=r" (psp));

	if(os_cur_task != NULL) ((os_task_t*)os_cur_task->element)->pStack = (uint32_t*)psp;

	/* Save last task
	 ------------------------------------------------------*/
	os_list_cell_t* last_task = os_cur_task;

	/* Loop here until a task can be executed
	 ------------------------------------------------------*/
	do {

		/* Decide next task
		 ------------------------------------------------------*/
		os_cur_task = os_round_robin();

		/* Call CB if needed
		 ------------------------------------------------------*/
		if(os_cur_task == NULL) {
			__os_enable_irq();
			os_no_task_ready_cb();
			__os_disable_irq();
		}

	}while(os_cur_task == NULL);

	/* Write task stack location into current stack
	 ------------------------------------------------------*/
	psp = (uint32_t) ((os_task_t*)os_cur_task->element)->pStack;
	__asm volatile ("msr psp, %[in]" : : [in] "r" (psp));

	/* If last task is deleting state, remove it
	 ------------------------------------------------------*/
	if(last_task != NULL && ((os_task_t*)last_task->element)->state == OS_TASK_DELETING) os_task_delete((os_handle_t)last_task->element);

	/* Enable IRQ
	 ------------------------------------------------------*/
	__os_enable_irq();

}


/**********************************************
 * IRQ FUNCTIONS
 *********************************************/

/**
 * @brief This function handles Pendable request for system service.
 *
 * Has to be naked otherwise compiler will recover R6 value at the end of the function
 */
__naked void PendSV_Handler(void)
{

	/* Get PSP
	 ------------------------------------------------------*/
	__asm volatile ("mrs r0, psp");					//R0 = PSP

#if defined(OS_FPU_EN) && OS_FPU_EN == 1
	/* Save FPU context
	 ------------------------------------------------------*/
	__asm volatile("tst lr, #0x10");				//If LR & 1 << 4 == 0
	__asm volatile("it eq");						//If previous instruction was 0,
	__asm volatile("vstmdbeq r0!, {s16-s31}");		//then, push s16 to s31 (FPU context) into stack (psp)
#endif

	/* Save current context
	 ------------------------------------------------------*/
	__asm volatile ("stmdb r0!, {r4-r11, lr}");		//Push R4-R11 and LR to stack (PSP)
	__asm volatile ("msr psp, r0");					//PSP = R0

	/* Call scheduler
	 ------------------------------------------------------*/
	__asm volatile ("push {lr}");
	__asm volatile ("bl os_scheduler");
	__asm volatile ("pop {lr}");

	/* Recover context
	 ------------------------------------------------------*/
	__asm volatile ("mrs r0, psp");					//R0 = PSP
	__asm volatile ("ldmia r0!, {r4-r11, lr}");		//Recover R4-R11 and LR from stack (PSP)

#if defined(OS_FPU_EN) && OS_FPU_EN == 1
	/* Recover FPU context
	 ------------------------------------------------------*/
	__asm volatile("tst lr, #0x10");				//If LR & 1 << 4 == 0
	__asm volatile("it eq");						//If previous instruction was 0,
	__asm volatile("vldmiaeq r0!, {s16-s31}");		//then, pop s16 to s31 (FPU context) from stack (psp)
#endif

	__asm volatile ("msr psp, r0");					//PSP = R0

	/* Return
	 ------------------------------------------------------*/
	__asm volatile ("bx lr");

}

/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Scheduler start
 *
 * @brief Starts the scheduler and yields the current task if there is a higher priority one waiting
 *
 * @return os_err_e : OS_ERR_OK if OK
 *
 **********************************************************************/
os_err_e os_scheduler_start(){

	/* Check if OS was initialized
	 ------------------------------------------------------*/
	if(os_init_get() != 1) return OS_ERR_NOT_READY;

	/* Enter critical to avoid interupts during assignment
	 ------------------------------------------------------*/
	OS_CRITICAL_SECTION(

		/* Turn on scheduler
		 ------------------------------------------------------*/
		state = OS_SCHEDULER_START;

		/* Check if the task must yield
		 ------------------------------------------------------*/
		if(os_task_must_yeild()) OS_SET_PENDSV();

	);

	return OS_ERR_OK;
}


/***********************************************************************
 * OS Scheduler stop
 *
 * @brief Stops the scheduler
 *
 **********************************************************************/
void os_scheduler_stop(){

	/* Enter critical to avoid being interrupted in the middle of the assignment
	 ------------------------------------------------------*/
	OS_CRITICAL_SECTION(

		/* Turn of scheduler
		 ------------------------------------------------------*/
		state = OS_SCHEDULER_STOP;

	);

}


/***********************************************************************
 * OS Scheduler State get
 *
 * @brief Returns the state of the scheduler
 *
 * @return os_scheduler_state_e : stop or start
 *
 **********************************************************************/
os_scheduler_state_e os_scheduler_state_get(){
	return state;
}
