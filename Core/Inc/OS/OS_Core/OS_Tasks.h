/*
 * OS_Tasks.h
 *
 *  Created on: Jun 1, 2021
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_TASKS_H_
#define INC_OS_OS_TASKS_H_

#include "OS/OS_Core/OS_Common.h"
#include "OS/OS_Core/OS_Obj.h"
#include "OS/OS_Core/OS_Tick.h"
#include "OS/OS_Core/OS_Process.h"

/**********************************************
 * PUBLIC TYPES
 *********************************************/

/* Tasks mode
 ---------------------------------------------------*/
typedef enum{
	OS_TASK_MODE_RETURN,
	OS_TASK_MODE_DELETE,
	__OS_TASK_MODE_MAX,
}os_task_mode_e;

/* Tasks states
 ---------------------------------------------------*/
typedef enum{
	OS_TASK_NOT_EXIST,
	OS_TASK_ENDED,
	OS_TASK_READY,
	OS_TASK_BLOCKED,
	OS_TASK_DELETING,
} os_task_state_e;


/* Task object (useful to cast from handle to task)
 ---------------------------------------------------*/
typedef struct os_task_{
	os_obj_t			obj;				//Base object (must be first member)
	os_task_state_e 	state;				// task state
	int8_t		 		basePriority;		// Base priority between 0 and 127
	uint32_t	 		wakeCoutdown;		// Counter to wake up the task
	uint32_t* 	 		pStack;				// Store stack pointer when context change
	uint32_t	 		stackSize;			// Stores the stack size
	uint32_t	 		stackBase;			// stores the stack base address

	void*				fnPtr;				//Store the code reference
	os_handle_t*	 	objWaited;			// Object this task is waiting for
	size_t	 			sizeObjs;			// Number of objects in list
	size_t	 			objWanted;			// Index of the object this task wants to get when it wakes up. Used when waiting one of multiple objects
	os_obj_wait_e		waitFlag;			// wait all or one

	void*				ownedMutex;			//List containing all mutexes owned by this task
	void*				retVal;				//Return value;
	int8_t				priority;			//Used internally to store calculated priority

	int					argc;				//Used to store the number of arguments when loading an elf.
	char**				argv;				//Array of strings for each argument
	os_process_t*		process;			//Process atached or NULL if none
} os_task_t;

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
os_err_e os_task_create(os_handle_t* h, char const * name, void* (*fn)(void*), os_task_mode_e mode, int8_t priority, uint32_t stack_size, void* arg);


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
os_err_e os_task_createProc(os_handle_t* h, char const * name, int (*fn)(int argc, char* argv[]), os_process_t* proc, os_task_mode_e mode, int8_t priority, uint32_t stack_size, int argc, char** argv, uint32_t r9);


/***********************************************************************
 * OS Task End
 *
 * @brief This function ends the current running task. Use this to safely end a task if you do not care about its return
 * The task block is freed from the heap, and all tasks waiting for it to finish will be set to ready
 *
 **********************************************************************/
void os_task_end();


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
os_err_e os_task_return(void* retVal);


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
os_err_e os_task_delete(os_handle_t h);


/***********************************************************************
 * OS Task Yeild
 *
 * @brief This function calls the scheduler in order to let other tasks to run
 *
 **********************************************************************/
void os_task_yeild();


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
int8_t os_task_getPrio(os_handle_t h);


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
os_err_e os_task_sleep(uint32_t sleep_ticks);


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
void* os_task_getReturn(os_handle_t h);


/***********************************************************************
 * OS Task Get State
 *
 * @brief This function gets the state of a task. This state not necessarily matches the one in the task, since there are edge cases and
 * problems that do not concern the user
 *
 * @param os_handle_t t: [in] task to get the state
 *
 * @return os_task_state_e: The state of the task (Not exist = it was never created, or it ended, or got deleted ; Blocked if it is blocked ; Ready if it is ready ; Deleting
 *
 **********************************************************************/
os_task_state_e os_task_getState(os_handle_t h);


/***********************************************************************
 * OS get task by PID
 *
 * @brief This function searches for a task using its PID
 *
 * @param uint16_t pid : [in] PID of the searched task
 *
 * @return os_list_cell_t* : reference to the cell containing the element or null if not found
 **********************************************************************/
os_handle_t os_task_getByPID(uint16_t pid);


/***********************************************************************
 * OS get current task
 *
 * @brief Get current task
 *
 * @return os_task_t* : reference to the current task
 **********************************************************************/
os_task_t* os_task_getCurrentTask(void);


/***********************************************************************
 * OS Get Task from handle
 *
 * @brief This function gets the task object from the handle
 *
 * @param os_handle_t h : [ in] Pointer to the semaphore
 *
 * @return os_task_t* : NULL if error, the task reference if OK
 **********************************************************************/
static inline os_task_t* os_task_getFromHandle(os_handle_t h){
	if(h == NULL) return NULL;
	if(h->type != OS_OBJ_TASK) return NULL;

	return (os_task_t*)h;
}

/***********************************************************************
 * OS Task Wait
 *
 * @brief This function performs a blocking wait for an amount of MS
 *
 * @param uint32_t ms : [in] amount of ticks to block
 *
 **********************************************************************/
static inline void os_task_wait(uint32_t ms){
    uint32_t volatile enter = os_getMsTick();
	while(os_getMsTick() - enter < ms) continue;
}

#endif /* INC_OS_OS_TASKS_H_ */
