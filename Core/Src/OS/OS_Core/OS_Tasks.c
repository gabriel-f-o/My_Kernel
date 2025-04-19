/*
 * OS_Tasks.c
 *
 *  Created on: Jun 1, 2021
 *      Author: Gabriel
 */

#include "common.h"
#include "OS/OS_Core/OS.h"
#include "OS/OS_Core/OS_Internal.h"
#include "OS/OS_FS/lfs.h"

/**********************************************
 * EXTERNAL VARIABLES
 *********************************************/

extern os_list_head_t os_obj_head;	//Head to obj list

/**********************************************
 * PUBLIC VARIABLES
 *********************************************/

os_handle_t main_task;	//Main task handle
os_handle_t idle_task;	//Idle task handle

/**********************************************
 * OS PRIVATE VARIABLES
 *********************************************/

os_list_head_t os_head;				//Head of task list
os_list_cell_t* os_cur_task = NULL;	//Current task pointer

/**********************************************
 * PRIVATE FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Init Stack
 *
 * @brief This function initializes the stack, change the used stack (now psp, before msp) and set MSP to match
 * the interrupt stack location in heap
 *
 * @param uint32_t interruptStackSize : [in] size of the interrupt stack
 *
 * @return os_err_e : error code (0 = OK)
 *
 **********************************************************************/
static os_err_e os_task_init_stack(uint32_t interruptStackSize){

	/* Check errors
	 ------------------------------------------------------*/
	if(interruptStackSize < OS_MINIMUM_STACK_SIZE) return OS_ERR_BAD_ARG;

	/* Allocate the stack
	 ------------------------------------------------------*/
	uint32_t stk = (uint32_t) os_heap_alloc(interruptStackSize);

	/* Check if allocation was OK
	 ------------------------------------------------------*/
	if(stk == 0) 
		return OS_ERR_INSUFFICIENT_HEAP;

	/* Save context and make PSP = MSP
	 ------------------------------------------------------*/
	register uint32_t volatile mspReg = (uint32_t) ( (stk + interruptStackSize) & (~0x7UL) ); //logic and to guarantee that we are word aligned
	__asm volatile ("mrs r1, msp"); 		//R1 = MSP
	__asm volatile ("msr psp, r1"); 		//PSP = R1

	/* Position MSP to interrput stack
	 ------------------------------------------------------*/
	__asm volatile ("mov r1, %[in]" : : [in] "r" (mspReg)); //R1 = mspReg
	__asm volatile ("msr msp, r1"); //MSP = R1

	/* Select PSP as current stack pointer
	 ------------------------------------------------------*/
	__asm volatile ("mrs r1, control");		//R1 = CTRL
	__asm volatile ("orr r1, r1, #0x2");	//R1 |= 0x2

	__asm volatile ("msr control, r1");		//CTRL = R1

#ifdef __OS_CORTEX_M33
	/* Set msplimit
	 ------------------------------------------------------*/
	register uint32_t volatile msplim = (uint32_t)stk;
	__asm volatile ("mov r1, %[in]" : : [in] "r" (msplim)); //R1 = msplim
	__asm volatile ("msr msplim, r1"); 		//MSPLIM = R1
#endif

	/* Recover stack
	 ------------------------------------------------------*/
	return OS_ERR_OK;
}


/***********************************************************************
 * OS Task get free count
 *
 * @brief This function gets the amount of times this object can be "taken" before it is considered unavailable
 * In the case of tasks, if the task is finished, the user can take it an infinite amount of times, otherwise it is unavailable
 *
 * @param os_handle_t h : [in] object to verify the availability
 *
 * @return uint32_t : the amount of times the object can be taken
 *
 **********************************************************************/
static uint32_t os_task_getFreeCount(os_handle_t h, os_handle_t takingTask){
	/* Check arguments
	 ------------------------------------------------------*/
    UNUSED_ARG(takingTask);
	if(h == NULL) return 0;
	if(h->type != OS_OBJ_TASK) return 0;

	/* Check if task ended
	 ------------------------------------------------------*/
	os_task_state_e state = os_task_getState(h);

	/* Determines if task ended
	 ------------------------------------------------------*/
	bool task_ended  = (state == OS_TASK_NOT_EXIST);
		 task_ended |= (state == OS_TASK_ENDED);
		 task_ended |= (state == OS_TASK_DELETING);

	return task_ended ? OS_OBJ_COUNT_INF : 0;
}


/***********************************************************************
 * OS Task take
 *
 * @brief This function takes possession of the object. In the case of a task, it has no effect.
 *
 * @param os_handle_t h 			: [in] object to take
 * @param os_handle_t takingTask	: [in] handle to the task that is taking the object
 *
 * @return os_err_e : error code (0 = OK)
 *
 **********************************************************************/
static os_err_e os_task_objTake(os_handle_t h, os_handle_t takingTask){
	UNUSED_ARG(h);
	UNUSED_ARG(takingTask);

	return OS_ERR_OK;
}


/***********************************************************************
 * OS Task Start
 *
 * @brief This function starts a new task, that will be called by the scheduler when the correct time comes
 *
 * @param os_handle_t* h		: [out] handle to object
 * @param char* name 			: [ in] name of the task
 * @param void* (*fn)        	: [ in] task's main function to be called
 * @param os_process_t* proc	: [ in] Attached process (NULL if none)
 * @param os_task_mode_e mode	: [ in] Inform what the task should do when returning (delete or keep the task block to get its return value; ATTENTION : in mode RETURN the user must use os_task_delete to avoid leaks
 * @param int8_t priority		: [ in] A priority to the task (0 is lowest priority) cannot be negative
 * @param uint32_t stack_size 	: [ in] The amount of stack to be reserved. A minimum of 128 bytes is required
 * @param int argc			    : [ in] First argument to be passed to the task (used for argc)
 * @param char* argv[]			: [ in] Second argument to be passed to the task (used for argv)
 * @param uint32_t r9			: [ in] R9 value. Used to store the GOT
 *
 * @return os_err_e : An error code (0 = OK)
 *
 **********************************************************************/
static os_err_e os_task_start(os_handle_t* h, char const * name, void* (*fn)(int argc, char* argv[]), os_process_t* proc, os_task_mode_e mode, int8_t priority, uint32_t stack_size, void* argc, void* argv, uint32_t r9)
{
	/* Check for argument errors
	 ------------------------------------------------------*/
	if(h == NULL) 							return OS_ERR_BAD_ARG;
	if(fn == NULL) 							return OS_ERR_BAD_ARG;
	if(priority < 0) 						return OS_ERR_BAD_ARG;
	if(mode >= __OS_TASK_MODE_MAX) 			return OS_ERR_BAD_ARG;
	if(stack_size < OS_MINIMUM_STACK_SIZE)  return OS_ERR_BAD_ARG;
	if(os_init_get() == false)				return OS_ERR_NOT_READY;

	/* If task exists, return it
	 ------------------------------------------------------*/
	if(name != NULL){
		os_list_cell_t* obj = os_handle_list_searchByName(&os_obj_head, OS_OBJ_TASK, name);
		if(obj != NULL){
			*h = obj->element;
			return OS_ERR_OK;
		}
	}

	/* Alloc the task block
	 ------------------------------------------------------*/
	os_task_t* t = (os_task_t*)os_heap_alloc(sizeof(os_task_t));

	/* Check allocation
	 ------------------------------------------------------*/
	if(t == NULL) return OS_ERR_INSUFFICIENT_HEAP;

	/* Alloc the stack
	 ------------------------------------------------------*/
	uint32_t stk = (uint32_t) os_heap_alloc(stack_size);
	if(stk == 0){
		os_heap_free(t);
		return OS_ERR_INSUFFICIENT_HEAP;
	}

	/* Init Task
	 ------------------------------------------------------*/
	t->obj.objUpdate	= 0;
	t->obj.type			= OS_OBJ_TASK;
	t->obj.getFreeCount	= &os_task_getFreeCount;
	t->obj.blockList	= os_list_init();
	t->obj.obj_take		= &os_task_objTake;
	t->obj.name			= name == NULL ? NULL : (char*)os_heap_alloc(strlen(name) + 1);

	t->fnPtr			= fn;
	t->basePriority		= priority;
	t->priority		    = priority;
	t->state			= OS_TASK_READY;
	t->wakeCoutdown	 	= 0;
	t->stackBase		= (stk + stack_size);
	t->stackSize 		= stack_size;
	t->pStack			= (uint32_t*) ( t->stackBase & (~0x7UL) );
	t->objWaited		= NULL;
	t->sizeObjs 		= 0;
	t->retVal			= NULL;
	t->ownedMutex		= os_list_init();

	t->argc				= argv == NULL ? 0 : (int)argc;
	t->argv				= argv;
	t->process			= proc;

	/* Init Task Stack
	 ------------------------------------------------------*/
	*--t->pStack = (uint32_t) 0x01000000;	 	//xPSR (bit 24 must be 1 otherwise BOOM)
	*--t->pStack = (uint32_t) fn;				//Return
	*--t->pStack = (mode == OS_TASK_MODE_RETURN) ? (uint32_t) &os_task_return : (uint32_t) &os_task_end;  //LR
	*--t->pStack = (uint32_t) 0;				//R12
	*--t->pStack = (uint32_t) 0;			 	//R3
	*--t->pStack = (uint32_t) 0;			 	//R2
	*--t->pStack = (uint32_t) argv;			 	//R1 (argument 2)
	*--t->pStack = (uint32_t) argc;			 	//R0 (argument 1)

	*--t->pStack = (uint32_t) 0xFFFFFFFD;    	//LR (when called by the interrupt, flag as basic frame used always)
	*--t->pStack = (uint32_t) 0;			 	//R11
	*--t->pStack = (uint32_t) 0;			 	//R10
	*--t->pStack = (uint32_t) r9; 			 	//R9
	*--t->pStack = (uint32_t) 0;			 	//R8
	*--t->pStack = (uint32_t) 0;				//R7
	*--t->pStack = (uint32_t) 0;				//R6
	*--t->pStack = (uint32_t) 0;				//R5
	*--t->pStack = (uint32_t) 0;				//R4

	/* Handles any heap errors
	 ------------------------------------------------------*/
	os_err_e ret = OS_ERR_OK;
	if(t->obj.blockList == NULL || t->ownedMutex == NULL || (t->obj.name == NULL && name != NULL) ){
		ret = OS_ERR_INSUFFICIENT_HEAP;
		goto freeTask;
	}

	/* Copy name
	 ------------------------------------------------------*/
	if(name != NULL)
		strcpy(t->obj.name, name);

	/* Add task to list
	 ------------------------------------------------------*/
	os_err_e err = os_list_add(&os_head, (os_handle_t)t, OS_LIST_FIRST);
	if(err != OS_ERR_OK) {
		ret = OS_ERR_INSUFFICIENT_HEAP;
		goto freeTask;
	}

	/* Add object to object list
	 ------------------------------------------------------*/
	ret = os_list_add(&os_obj_head, (os_handle_t) t, OS_LIST_FIRST);
	if(ret != OS_ERR_OK) {
		goto freeAll;
	}

	/* Calculate task priority
	 ------------------------------------------------------*/
	int8_t task_prio = os_task_getPrio((os_handle_t) t);
	int8_t cur_prio = ( (os_cur_task == NULL) ? -1 : os_task_getPrio(os_cur_task->element) );

	/* If created task was a higher priority, and scheduler is running, yeild
	 ---------------------------------------------------*/
	if(task_prio > cur_prio && os_scheduler_state_get() == OS_SCHEDULER_START) os_task_yeild();

	/* link handle with task object
	 ---------------------------------------------------*/
	*h = ( (err == OS_ERR_OK) ? (os_handle_t) t : NULL );

	return err;

freeAll:
	os_list_remove(&os_head, (os_handle_t)t);

freeTask:
	os_list_clear(t->obj.blockList);
	os_list_clear(t->ownedMutex);
	os_heap_free(t->obj.name);
	os_heap_free(t);
	os_heap_free((void*)stk);

	return ret;
}

/**********************************************
 * OS PRIVATE FUNCTIONS
 *********************************************/


/***********************************************************************
 * OS Task Init
 *
 * @brief This function initializes the default tasks
 *
 * @param char* main_name				: [in] main task name. A null name creates a nameless task.
 * @param int8_t   main_task_priority   : [in] main stack base priority (0 to 127)
 * @param uint32_t interrput_stack_size : [in] size of the interrupt stack
 * @param uint32_t idle_stack_size 		: [in] size of the idle stack
 *
 * @return os_err_e : Error code (0 = OK)
 *
 **********************************************************************/
os_err_e os_task_init(char* main_name, int8_t main_task_priority, uint32_t interrput_stack_size, uint32_t idle_stack_size){

	/* Check errors
	 ------------------------------------------------------*/
	if(main_task_priority < 0) return OS_ERR_BAD_ARG;
	if(interrput_stack_size < OS_MINIMUM_STACK_SIZE) return OS_ERR_BAD_ARG;
	if(idle_stack_size < OS_MINIMUM_STACK_SIZE) return OS_ERR_BAD_ARG;

	/* Init stack by changing the register used (MSP to PSP) and allocate interrupt stack
	 ------------------------------------------------------*/
	os_err_e ret = os_task_init_stack(interrput_stack_size);
	if(ret != OS_ERR_OK) return ret;

	/* Allocate task block
	 ------------------------------------------------------*/
	os_task_t* t = (os_task_t*)os_heap_alloc(sizeof(os_task_t));

	/* Check allocation
	 ------------------------------------------------------*/
	if(t == 0) return OS_ERR_INSUFFICIENT_HEAP;

	/* Init main task
	 ------------------------------------------------------*/
	t->obj.objUpdate		= 0;
	t->obj.type				= OS_OBJ_TASK;
	t->obj.getFreeCount		= &os_task_getFreeCount;
	t->obj.blockList		= os_list_init();
	t->obj.obj_take			= &os_task_objTake;
	t->obj.name				= main_name == NULL ? NULL : (char*)os_heap_alloc(strlen(main_name) + 1);

	t->basePriority 		= main_task_priority;
	t->priority		    	= main_task_priority;
	t->state	 			= OS_TASK_READY;
	t->pStack   			= NULL;
	t->wakeCoutdown  		= 0;
	t->stackBase	    	= 0;
	t->objWaited			= NULL;
	t->sizeObjs 			= 0;
	t->retVal				= NULL;

	t->ownedMutex			= os_list_init();
	t->argc					= 0;
	t->argv					= NULL;

	/* Handles heap errors
	 ------------------------------------------------------*/
	if(t->obj.blockList == NULL || t->ownedMutex == NULL || (t->obj.name == NULL && main_name != NULL) ){
		ret = OS_ERR_INSUFFICIENT_HEAP;
		goto freeTask;
	}

	/* Copy name
	 ------------------------------------------------------*/
	if(main_name != NULL)
		strcpy(t->obj.name, main_name);

	/* Init head list and Add main task
	 ------------------------------------------------------*/
	ret = os_list_add(&os_head, (os_handle_t) t, OS_LIST_FIRST);
	if(t->obj.blockList == NULL || t->ownedMutex == NULL || ret != OS_ERR_OK) {
		ret = OS_ERR_INSUFFICIENT_HEAP;
		goto freeTask;
	}

	/* Add object to object list
	 ------------------------------------------------------*/
	ret = os_list_add(&os_obj_head, (os_handle_t) t, OS_LIST_FIRST);
	if(ret != OS_ERR_OK) {
		goto freeAll;
	}

	/* Point to current task
	 ------------------------------------------------------*/
	os_cur_task = os_head.head.next;

	/* Link handle with task
	 ------------------------------------------------------*/
	main_task = (os_handle_t) t;

	/* Init idle task
	 ------------------------------------------------------*/
	return ret;

freeAll:
	os_list_remove(&os_head, (os_handle_t) t);

freeTask:
	os_list_clear(t->obj.blockList);
	os_list_clear(t->ownedMutex);
	os_heap_free(t->obj.name);
	os_heap_free(t);


	return ret;
}


/***********************************************************************
 * OS Task Must Yeild
 *
 * @brief This function checks the priority of all tasks and decides if the current task should yield
 *
 * @return bool : 1 = yeild
 *
 **********************************************************************/
bool os_task_must_yeild(){

	/* Enter critical
	 ------------------------------------------------------*/
	OS_CRITICAL_SECTION(

		/* Check if there is a task with higher priority
		 ------------------------------------------------------*/
		os_list_cell_t* it = os_head.head.next;
		int8_t cur_prio = os_cur_task == NULL ? -1 : os_task_getPrio(os_cur_task->element);

		/* Search the high side of the list
		 ------------------------------------------------------*/
		while(it != NULL){

			/* Calculate task priority based on the mutex it owns
			 ------------------------------------------------------*/
			int8_t task_prio = os_task_getPrio(it->element);

			/* If the task is ready and its priority is higher, yeild
			 ------------------------------------------------------*/
			if( ((os_task_t*)it->element)->state == OS_TASK_READY && cur_prio < task_prio ) return true;

			/* Otherwise continue
			 ------------------------------------------------------*/
			it = it->next;
		}
	);

	return false;
}


/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Task Create
 *
 * @brief This function creates a new task, that will be called by the scheduler when the correct time comes
 *
 * @param os_handle_t* h						: [out] handle to object
 * @param char* name 							: [ in] name of the task
 * @param void* (*fn)(void*) 					: [ in] task's main function to be called
 * @param os_task_mode_e mode					: [ in] Inform what the task should do when returning (delete or keep the task block to get its return value; ATTENTION : in mode RETURN the user must use os_task_delete to avoid leaks
 * @param int8_t priority						: [ in] A priority to the task (0 is lowest priority) cannot be negative
 * @param uint32_t stack_size 					: [ in] The amount of stack to be reserved. A minimum of 128 bytes is required
 * @param void* arg  						    : [ in] Argument to be passed to the task
 *
 * @return os_err_e : An error code (0 = OK)
 *
 **********************************************************************/
os_err_e os_task_create(os_handle_t* h, char const * name, void* (*fn)(void*), os_task_mode_e mode, int8_t priority, uint32_t stack_size, void* arg){

	/* Start task with the correct arguments
	 ------------------------------------------------------*/
	return os_task_start(h, name, (void*)fn, NULL, mode, priority, stack_size, arg, NULL, 0);
}

/***********************************************************************
 * OS Task Create Process flavor
 *
 * @brief This function creates a new task, that will be called by the scheduler when the correct time comes
 *
 * @param os_handle_t* h						: [out] handle to object
 * @param char* name 							: [ in] name of the task
 * @param int (*fn)(int, char**) 				: [ in] task's main function to be called
 * @param os_process_t* proc					: [ in] Attached process (NULL if none)
 * @param os_task_mode_e mode					: [ in] Inform what the task should do when returning (delete or keep the task block to get its return value; ATTENTION : in mode RETURN the user must use os_task_delete to avoid leaks
 * @param int8_t priority						: [ in] A priority to the task (0 is lowest priority) cannot be negative
 * @param uint32_t stack_size 					: [ in] The amount of stack to be reserved. A minimum of 128 bytes is required
 * @param int argc  						    : [ in] argc argument to be passed to the task
 * @param char** argv  						    : [ in] argv argument to be passed to the task
 * @param uint32_t r9  						    : [ in] r9 value (must be GOT base address)
 *
 * @return os_err_e : An error code (0 = OK)
 *
 **********************************************************************/
os_err_e os_task_createProc(os_handle_t* h, char const * name, int (*fn)(int argc, char* argv[]), os_process_t* proc, os_task_mode_e mode, int8_t priority, uint32_t stack_size, int argc, char** argv, uint32_t r9){

	/* Start task with the correct arguments
	 ------------------------------------------------------*/
	return os_task_start(h, name, (void*)fn, proc, mode, priority, stack_size, (void*)argc, argv, r9);
}


/***********************************************************************
 * OS Task End
 *
 * @brief This function ends the current running task. Use this to safely end a task if you do not care about its return
 * The task block is freed from the heap, and all tasks waiting for it to finish will be set to ready
 *
 **********************************************************************/
void os_task_end(){
	os_task_delete((os_handle_t) os_cur_task->element);
}


/***********************************************************************
 * OS Task End
 *
 * @brief This function ends the current running task. Use this to safely end a task if you care about its return
 * The task block is not freed from the heap, but all tasks waiting for it to finish will be set to ready.
 *
 * Use os_task_delete() after retrieving its return value to avoid memory leak
 *
 * ATTENTION : This function will activate IRQ regardless of its previous state
 *
 * @param void* retVal : [in] return value
 *
 * @return os_err_e : should never return. If it does, a problem occurred
 *
 **********************************************************************/
os_err_e os_task_return(void* retVal){

	/* Check scheduler
	 ------------------------------------------------------*/
	if(os_scheduler_state_get() != OS_SCHEDULER_START) return OS_ERR_NOT_READY;

	/* Enter critical section
	------------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Store return value and tag as ended
	 ------------------------------------------------------*/
	((os_task_t*)os_cur_task->element)->state = OS_TASK_ENDED;
	((os_task_t*)os_cur_task->element)->retVal = retVal;

	/* Update blocked list to inform handles that task has finished
	 ------------------------------------------------------*/
	os_handle_list_updateAndCheck(os_cur_task->element);

	/* Failsafe
	 ------------------------------------------------------*/
	while(1){

		/* Yeild task
		 ------------------------------------------------------*/
		os_task_yeild();

		/* Enable IRQ no matter what
		 ------------------------------------------------------*/
		__os_enable_irq();

	}

	/* Exit critiacl
	 ------------------------------------------------------*/
	OS_EXIT_CRITICAL();

	/* Another Failsafe
	 ------------------------------------------------------*/
	return OS_ERR_UNKNOWN;
}


/***********************************************************************
 * OS Task delete
 *
 * @brief This function deletes a task, removing it from task list and freeing its block
 *
 * ATTENTION : if the current tasks kills itself, the IRQ will be enabled regardless of its previous state
 *
 * @param os_handle_t h : [in] handle containing the task to kill
 *
 * @return os_err_e : An error code (0 = OK)
 *
 **********************************************************************/
os_err_e os_task_delete(os_handle_t h){

	/* Convert address
	 ------------------------------------------------------*/
	os_task_t* t = (os_task_t*) h;

	/* Search to see if task exists
	 ------------------------------------------------------*/
	os_list_cell_t* list = os_list_search(&os_head, h);

	/* Check for errors
	 ------------------------------------------------------*/
	if(t == NULL) return OS_ERR_BAD_ARG;
	if(list == NULL) return OS_ERR_INVALID;
	if(h->type != OS_OBJ_TASK) return OS_ERR_BAD_ARG;

	/* Check scheduler, we cannot kill the current task if scheduler is not ready
	 ------------------------------------------------------*/
	if(h == os_cur_task->element && os_scheduler_state_get() != OS_SCHEDULER_START) return OS_ERR_NOT_READY;

	/* Enter critical section
	------------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Tag as ended
	 ------------------------------------------------------*/
	t->state = OS_TASK_ENDED;

	/* Update blocked list to inform handles that task has finished
	 ------------------------------------------------------*/
	os_handle_list_updateAndCheck(h);

	/* Check and store if we are killing the current task
	 ------------------------------------------------------*/
	if(h == os_cur_task->element){

		/* Tag task to delete
		 ------------------------------------------------------*/
		t->state = OS_TASK_DELETING;

		/* Failsafe
		 ------------------------------------------------------*/
		while(1){

			/* Yeild task
			 ------------------------------------------------------*/
			os_task_yeild();

			/* Enable IRQ no matter what
			 ------------------------------------------------------*/
			__os_enable_irq();

		}

		/* Another Failsafe
		 ------------------------------------------------------*/
		return OS_ERR_UNKNOWN;
	}

	if( t->process != NULL ){
		if( ((os_list_head_t*)t->process->thread_list)->listSize == 0 ){
			ASSERT(os_process_kill(t->process) == OS_ERR_OK);
		}
		else {
			os_list_remove( ((os_process_t*)t->process)->thread_list, t);
		}
	}

	/* Remove task from object block list if needed
	 ------------------------------------------------------*/
	if( t->objWaited != NULL) {

		for(uint32_t i = 0; i < t->sizeObjs; i++){

			/* Failsafe
			 ------------------------------------------------------*/
			if(t->objWaited[i] == NULL) continue;

			/* Remove from block list
			 ------------------------------------------------------*/
			os_list_remove(t->objWaited[i]->blockList, h);

			/* Update object's priority
			 ------------------------------------------------------*/
			os_obj_updatePrio(t->objWaited[i]);

			/* Update block list
			 ------------------------------------------------------*/
			os_handle_list_updateAndCheck(t->objWaited[i]);
		}

	}

	/* Deletes from obj list
	 ------------------------------------------------------*/
	os_list_remove(&os_obj_head, h);

	/* Remove task from list
	 ------------------------------------------------------*/
	os_list_remove(&os_head, h);

	/* Clear blocked list
	 ------------------------------------------------------*/
	os_list_clear(h->blockList);

	/* Clear owned mutex list
	 ------------------------------------------------------*/
	os_list_clear(t->ownedMutex);

	/* Free code, name, and arguments if they were created by os_createProcess
	 ------------------------------------------------------*/
	if(t->argc > 0){
		os_heap_free(t->fnPtr);
		for(int i = 0; i < t->argc && t->argv != NULL; i++){
			os_heap_free(t->argv[i]);
			t->argv[i] = NULL;
		}
		os_heap_free(t->argv);
	}

	/* Free the stack memory
	 ------------------------------------------------------*/
	os_heap_free( (void*) (t->stackBase - t->stackSize) );

	/* Reset values just in case
	 ------------------------------------------------------*/
	t->state = OS_TASK_ENDED;
	t->objWaited = NULL;
	t->sizeObjs = 0;
	t->pStack = 0;
	t->stackBase = 0;
	t->stackSize = 0;
	t->wakeCoutdown = 0;
	t->argc = 0;
	t->argv = 0;

	/* Delete task
	 ------------------------------------------------------*/
	os_heap_free(h->name);
	os_heap_free(h);

	/* Return
	 ------------------------------------------------------*/
	OS_EXIT_CRITICAL();
	return OS_ERR_OK;
}


/***********************************************************************
 * OS Task Yeild
 *
 * @brief This function calls the scheduler in order to let other tasks to run
 *
 **********************************************************************/
void os_task_yeild(){

	/* Set Pend SV
	 ------------------------------------------------------*/
	OS_SET_PENDSV();
}


/***********************************************************************
 * OS Task get priority
 *
 * @brief This function calculates a task's dynamic priority
 *
 * @param task_t* t : [in] handle to task
 *
 * @return int8_t : The effective priority or -1 if argument error
 *
 **********************************************************************/
int8_t os_task_getPrio(os_handle_t h){

	/* Convert address
	 ------------------------------------------------------*/
	os_task_t* t = (os_task_t*) h;

	/* Check for errors
	 ------------------------------------------------------	*/
	if(t == NULL) return -1;
	if(h->type != OS_OBJ_TASK) return -1;

	return t->priority;
}


/***********************************************************************
 * OS Task Sleep
 *
 * ATTENTION : This functions enables IRQ regardless of its previous state
 *
 * @brief This function blocks a task for an amount of ticks OS
 *
 * @param uint32_t sleep_ticks : [in] amount of ticks to sleep
 *
 * @return os_err_e : error code (0 = OK)
 *
 **********************************************************************/
os_err_e os_task_sleep(uint32_t sleep_ticks){

	/* Get xPSR register
	 ---------------------------------------------------*/
	register uint32_t volatile xPSR = 0;
	__asm volatile("mrs %[out], xpsr" : [out] "=r" (xPSR));

	/* Check scheduler stop
	 ------------------------------------------------------*/
	if(os_scheduler_state_get() == OS_SCHEDULER_STOP) return OS_ERR_NOT_READY;

	/* Check if we are in thread mode (cannot sleep in interupt mode)
	 ---------------------------------------------------*/
	if( (xPSR & 0x1F) != 0) return OS_ERR_FORBIDDEN;

	/* Enter Critical -> If the list is changed during the process, this can corrupt our references
	 ------------------------------------------------------*/
	__os_disable_irq();

	/* Put task to blocked and change countdown
	 ------------------------------------------------------*/
	((os_task_t*)os_cur_task->element)->wakeCoutdown = sleep_ticks;
	((os_task_t*)os_cur_task->element)->state = OS_TASK_BLOCKED;

	/* Prepare scheduler to run
	 ------------------------------------------------------*/
	os_task_yeild();

	/* Enable IRQ no matter what
	 ------------------------------------------------------*/
	__os_enable_irq();

	return OS_ERR_OK;
}


/***********************************************************************
 * OS Task Get return
 *
 * @brief This function gets the return of a task that called os_task_end or returned from its callback function
 * This API should not be called if the task has been destroyed
 *
 * @param os_handle_t t: [in] task to get the return
 *
 * @return void* : NULL if error, the task return if OK
 *
 **********************************************************************/
void* os_task_getReturn(os_handle_t h){

	/* Convert address
	 ------------------------------------------------------*/
	os_task_t* task = (os_task_t*) h;

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return NULL;
	if(h->type != OS_OBJ_TASK) return NULL;
	if(os_list_search(&os_head, h) == NULL) return NULL;
	if(task->state != OS_TASK_ENDED) return NULL;

	return task->retVal;
}


/***********************************************************************
 * OS Task Get State
 *
 * @brief This function gets the state of a task. This state not necessarily matches the one in the task, since there are edge cases and
 * problems that do not concern the user
 *
 * @param os_handle_t h : [in] task to get the state
 *
 * @return os_task_state_e: The state of the task (Not exist = it was never created or got deleted ; Blocked if it is blocked ; Ready if it is ready ; Ended if it ended)
 *
 **********************************************************************/
os_task_state_e os_task_getState(os_handle_t h){

	/* Convert address
	 ------------------------------------------------------*/
	os_task_t* task = (os_task_t*) h;

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return OS_TASK_NOT_EXIST;
	if(h->type != OS_OBJ_TASK) return OS_TASK_NOT_EXIST;
	if(os_list_search(&os_head, h) == NULL) return OS_TASK_NOT_EXIST;
	if(task->state == OS_TASK_DELETING) return OS_TASK_NOT_EXIST;

	/* Check if task is ended
	 ------------------------------------------------------*/
	os_task_state_e state = task->state;

	/* Force blocked if task is waiting for an object
	 * This scenario can happen if the object is free, but the task did not have the chance to take it yet
	 ------------------------------------------------------*/
	state = state == OS_TASK_READY && task->objWaited != NULL ? OS_TASK_BLOCKED : state;

	return state;
}


/***********************************************************************
 * OS get current task
 *
 * @brief Get current task
 *
 * @return os_task_t* : reference to the current task
 **********************************************************************/
os_task_t* os_task_getCurrentTask(void){
    if(os_cur_task == NULL)
        return NULL;
        
    return (os_task_t*) os_cur_task->element;
}
