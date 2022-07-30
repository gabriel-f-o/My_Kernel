/*
 * OS_Obj.c
 *
 *  Created on: Oct 23, 2021
 *      Author: Gabriel
 */

#include "OS/OS_Core/OS_Common.h"
#include "OS/OS_Core/OS_Obj.h"
#include "OS/OS_Core/OS_Tasks.h"
#include "OS/OS_Core/OS_Internal.h"
#include "OS/OS_Core/OS_Heap.h"
#include "OS/OS_Core/OS_Scheduler.h"
#include "OS/OS_Core/OS_Callbacks.h"
#include "OS/OS_Core/OS_Mutex.h"

/**********************************************
 * EXTERNAL VARIABLES
 *********************************************/

extern os_list_cell_t* os_cur_task;	//Current task pointer

/**********************************************
 * PUBLIC VARIABLES
 *********************************************/

os_list_head_t os_obj_head;	//Head to obj list

/**********************************************
 * PRIVATE FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Object wait
 *
 * @brief This function waits for one or all objects in a list
 *
 * OS_OBJ_TASK  : The task has finished (i.e. state is OS_TASK_ENDED or OS_TASK_NOT_EXIST)
 * OS_OBJ_SEM   : The semaphore has at least one free counter
 * OS_OBJ_MUTEX : The mutex is free
 * OS_OBJ_EVT   : The event is set
 *
 * @param os_handle_t objList[]  : [ in] Array containing all objects to wait
 * @param size_t objNum			 : [ in] number of objects to wait
 * @param os_obj_wait_e waitFlag : [ in] flag indicating to wait one or all objects in the list
 * @param uint32_t timeout_ticks : [ in] Amount of time before a timeout is detected. If OS_WAIT_FOREVER, the task blocks forever. If OS_WAIT_NONE, the task returns immediately
 * @parem os_err_e* err			 : [out] Error code. Ignored if NULL.
 *
 * @return os_handle_t : handle to the object taken or NULL if error (see error code for more info)
 **********************************************************************/
static os_handle_t os_obj_wait(os_handle_t objList[], size_t objNum, os_obj_wait_e waitFlag, uint32_t timeout_ticks, os_err_e* err){

	/* Check for errors
	 ---------------------------------------------------*/
	bool error  = objList == NULL;
		 error |= objNum == 0;
		 error |= objNum == 0xFFFFFFFF;

	for(size_t i = 0; i < objNum; i++){
		error |= objList[i] == NULL;
		if(objList[i] != NULL){
			error |= objList[i]->type == OS_OBJ_MSGQ;
		}
	}

	/* Return if error
	 ---------------------------------------------------*/
	if(error) {
		if(err != NULL) *err = OS_ERR_BAD_ARG;
		return NULL;
	}

	/* Enter critical to access possible shared resource
	 ---------------------------------------------------*/
	bool blocked = false;
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Get xPSR register
	 ---------------------------------------------------*/
	register uint32_t volatile xPSR = 0;
	__asm volatile("mrs %[out], xpsr" : [out] "=r" (xPSR));

	/* Loop to get one or all objects
	 ---------------------------------------------------*/
	while(1){

		/* Declare auxiliary variables and iterate objects
		 ---------------------------------------------------*/
		size_t takingPos = 0xFFFFFFFF;
		bool allObjFree = 1;

		for(size_t i = 0; i < objNum; i++){

			/* Get free count
			 ---------------------------------------------------*/
			uint32_t freeCount = ( (objList[i]->getFreeCount != NULL) ? objList[i]->getFreeCount(objList[i]) : 1 );

			/* Update flag to use if we should wait all
			 ---------------------------------------------------*/
			allObjFree &= freeCount > 0;

			/* If we should check everyone, or the current object is not free, we continue
			 ---------------------------------------------------*/
			if(waitFlag != OS_OBJ_WAIT_ONE || freeCount == 0) continue;

			/* Otherwise save the position and break
			 ---------------------------------------------------*/
			takingPos = i;
			break;
		}

		/* All objects must be taken and everyone is free
		 ---------------------------------------------------*/
		if(waitFlag == OS_OBJ_WAIT_ALL && allObjFree == 1){

			/* Iterate to take every object
			 ---------------------------------------------------*/
			for(size_t i = 0; i < objNum; i++){

				/* Check if we are in thread mode (cannot take a mutex in interupt mode)
				 ---------------------------------------------------*/
				if( (xPSR & 0x1F) != 0 && objList[i]->type == OS_OBJ_MUTEX) {
					if(err != NULL) *err = OS_ERR_FORBIDDEN;
					OS_EXIT_CRITICAL();
					return NULL;
				}

				/* Take object
				 ---------------------------------------------------*/
				os_err_e retErr = (objList[i]->obj_take != NULL) ? objList[i]->obj_take(objList[i], os_cur_task->element) : OS_ERR_UNKNOWN;

				/* Handles errors
				 ---------------------------------------------------*/
				if(retErr != OS_ERR_OK){

					/* Releases any mutex taken
					 ---------------------------------------------------*/
					for(int j = 0; j < i; j++){
						if(objList[j]->type == OS_OBJ_MUTEX){
							os_mutex_release(objList[j]);
						}
					}

					OS_EXIT_CRITICAL();
					if(err != NULL) *err = retErr;
					return NULL;
				}
				/* Remove task from block list if needed
				 ---------------------------------------------------*/
				if(blocked) {
					os_list_remove(objList[i]->blockList, (os_handle_t)os_cur_task->element);
					os_obj_updatePrio(objList[i]);
				}

			}

			/* Update prio from current task
			 ---------------------------------------------------*/
			os_obj_updatePrio(os_cur_task->element);

			/* Update blocklist for every object
			 ---------------------------------------------------*/
			for(size_t i = 0; i < objNum; i++){
				os_handle_list_updateAndCheck(objList[i]);
			}

			/* Exit critical
			 ---------------------------------------------------*/
			OS_EXIT_CRITICAL();

			/* call cb if needed
			 ---------------------------------------------------*/
			if(blocked)  os_task_on_ready_cb(os_cur_task->element);

			if(err != NULL) *err = OS_ERR_OK;
			return objList[0];

		}

		/* One object must be taken and at least one is free
		 ---------------------------------------------------*/
		if(waitFlag == OS_OBJ_WAIT_ONE && takingPos != 0xFFFFFFFF){

			/* Check if we are in thread mode (cannot take a mutex in interupt mode)
			 ---------------------------------------------------*/
			if( (xPSR & 0x1F) != 0 && objList[takingPos]->type == OS_OBJ_MUTEX) {
				if(err != NULL) *err = OS_ERR_FORBIDDEN;
				OS_EXIT_CRITICAL();
				return NULL;
			}

			/* Take object
			 ---------------------------------------------------*/
			os_err_e retErr = (objList[takingPos]->obj_take != NULL) ? objList[takingPos]->obj_take(objList[takingPos], os_cur_task->element) : OS_ERR_UNKNOWN;
			if(retErr != OS_ERR_OK){
				OS_EXIT_CRITICAL();
				if(err != NULL) *err = retErr;
				return NULL;
			}

			/* Update prio from current task
			 ---------------------------------------------------*/
			os_obj_updatePrio(os_cur_task->element);

			/* If task blocked, remove from everyone's list
			 ---------------------------------------------------*/
			if(blocked) {
				for(size_t i = 0; i < objNum; i++){
					os_list_remove(objList[i]->blockList, (os_handle_t)os_cur_task->element);
					os_obj_updatePrio(objList[i]);
				}
			}

			/* Update blocklist
			 ---------------------------------------------------*/
			os_handle_list_updateAndCheck(objList[takingPos]);

			/* Exit critical
			 ---------------------------------------------------*/
			OS_EXIT_CRITICAL();

			/* call cb if needed
			 ---------------------------------------------------*/
			if(blocked)  os_task_on_ready_cb(os_cur_task->element);

			/* Return address of the object
			 ---------------------------------------------------*/
			if(err != NULL) *err = OS_ERR_OK;
			return objList[takingPos];
		}

		/* If the timout set is 0, then just return
		 ---------------------------------------------------*/
		if(timeout_ticks <= OS_WAIT_NONE){

			/* If task blocked, remove from everyone's list
			 ---------------------------------------------------*/
			if(blocked) {
				for(size_t i = 0; i < objNum; i++){
					os_list_remove(objList[i]->blockList, (os_handle_t)os_cur_task->element);
					os_obj_updatePrio(objList[i]);
				}

				/* Update blocklist for every object
				 ---------------------------------------------------*/
				for(size_t i = 0; i < objNum; i++){
					os_handle_list_updateAndCheck(objList[i]);
				}
			}

			/* Return
			 ---------------------------------------------------*/
			OS_EXIT_CRITICAL();
			if(err != NULL) *err = OS_ERR_TIMEOUT;
			return NULL;
		}

		/* Task cannot block of scheduler is not running
		 ---------------------------------------------------*/
		if(os_scheduler_state_get() != OS_SCHEDULER_START) {

			/* If task blocked, remove from everyone's list
			 ---------------------------------------------------*/
			if(blocked) {
				for(size_t i = 0; i < objNum; i++){
					os_list_remove(objList[i]->blockList, (os_handle_t)os_cur_task->element);
					os_obj_updatePrio(objList[i]);
				}

				/* Update blocklist for every object
				 ---------------------------------------------------*/
				for(size_t i = 0; i < objNum; i++){
					os_handle_list_updateAndCheck(objList[i]);
				}
			}

			if(err != NULL) *err = OS_ERR_NOT_READY;
			OS_EXIT_CRITICAL();
			return NULL;
		}

		/* Check if we are in thread mode (cannot block in interupt mode)
		 ---------------------------------------------------*/
		if( (xPSR & 0x1F) != 0) {
			if(err != NULL) *err = OS_ERR_FORBIDDEN;
			OS_EXIT_CRITICAL();
			return NULL;
		}

		/* Save information on task structure
		 ---------------------------------------------------*/
		((os_task_t*)os_cur_task->element)->state 			= OS_TASK_BLOCKED;
		((os_task_t*)os_cur_task->element)->wakeCoutdown 	= timeout_ticks;
		((os_task_t*)os_cur_task->element)->objWaited 		= objList;
		((os_task_t*)os_cur_task->element)->sizeObjs		= objNum;
		((os_task_t*)os_cur_task->element)->objWanted		= 0xFFFFFFFF;
		((os_task_t*)os_cur_task->element)->waitFlag		= waitFlag;

		/* If not yet blocked
		 ---------------------------------------------------*/
		if(!blocked){

			/* Add task to object's block list if not already
			 ---------------------------------------------------*/
			for(size_t i = 0; i < objNum; i++){
				os_err_e retErr = os_list_add(objList[i]->blockList, (os_handle_t)os_cur_task->element, OS_LIST_FIRST);

				/* Handle heap error
				 ---------------------------------------------------*/
				if(retErr != OS_ERR_OK){

					/* Remove everything so far
					 ---------------------------------------------------*/
					for(size_t j = 0; j < i; j++){
						os_list_remove(objList[j]->blockList, (os_handle_t)os_cur_task->element);
					}

					/* Revert structure
					 ---------------------------------------------------*/
					((os_task_t*)os_cur_task->element)->state 			= OS_TASK_READY;
					((os_task_t*)os_cur_task->element)->wakeCoutdown 	= 0;
					((os_task_t*)os_cur_task->element)->objWaited 		= NULL;
					((os_task_t*)os_cur_task->element)->sizeObjs		= 0;
					((os_task_t*)os_cur_task->element)->objWanted		= 0xFFFFFFFF;

					/* return
					 ---------------------------------------------------*/
					OS_EXIT_CRITICAL();
					if(err != NULL) *err = OS_ERR_INSUFFICIENT_HEAP;
					return NULL;
				}

				os_obj_updatePrio(objList[i]);
			}

			/* Update blocklist for every object
			 ---------------------------------------------------*/
			for(size_t i = 0; i < objNum; i++){
				os_handle_list_updateAndCheck(objList[i]);
			}

			/* Call CB
			 ---------------------------------------------------*/
			OS_EXIT_CRITICAL();
			os_task_on_block_cb(os_cur_task->element);
			OS_ENTER_CRITICAL();
		}

		blocked = true;

		/* Yeild
		 ---------------------------------------------------*/
		OS_SET_PENDSV();

		/* Reenable interrupts
		 ---------------------------------------------------*/
		__os_enable_irq();

		/* This line will be executed once the task is woken up by object freeing or timeout
		 * The object freeing means that the object was freed and this task was the chosen to wake up
		 * but there is no guarantee that the object will be available (as an interrupt can occur, or another higher priority
		 * task can get the object before this task has the chance to run. Because of this, we have to loop here
		 ---------------------------------------------------*/
		OS_ENTER_CRITICAL();

		/* Update ticks
		 ---------------------------------------------------*/
		timeout_ticks 									= ((os_task_t*)os_cur_task->element)->wakeCoutdown;
		((os_task_t*)os_cur_task->element)->objWaited 	= NULL;
		((os_task_t*)os_cur_task->element)->wakeCoutdown = 0;
		((os_task_t*)os_cur_task->element)->sizeObjs		= 0;
		((os_task_t*)os_cur_task->element)->objWanted	= 0xFFFFFFFF;
	}

	/* Should not be here
	 ---------------------------------------------------*/
	if(blocked) {
		for(size_t i = 0; i < objNum; i++){
			os_list_remove(objList[i]->blockList, (os_handle_t)os_cur_task->element);
			os_obj_updatePrio(objList[i]);
		}

		/* Update blocklist for every object
		 ---------------------------------------------------*/
		for(size_t i = 0; i < objNum; i++){
			os_handle_list_updateAndCheck(objList[i]);
		}
	}

	/* Return
	 ---------------------------------------------------*/
	OS_EXIT_CRITICAL();
	if(err != NULL) *err = OS_ERR_UNKNOWN;
	return NULL;
}

/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/


/***********************************************************************
 * OS Object sngle wait
 *
 * @brief This function waits for one object
 *
 * OS_OBJ_TASK  : The task has finished (i.e. state is OS_TASK_ENDED or OS_TASK_NOT_EXIST)
 * OS_OBJ_SEM   : The semaphore has at least one free counter
 * OS_OBJ_MUTEX : The mutex is free
 * OS_OBJ_EVT   : The event is set
 *
 * @param os_handle_t obj  		 : [ in] Handle of the object to wait
 * @param uint32_t timeout_ticks : [ in] Amount of time before a timeout is detected. If OS_WAIT_FOREVER, the task blocks forever. If OS_WAIT_NONE, the task returns imediately
 * @parem os_err_e* err			 : [out] Error code. Ignored if NULL.
 *
 * @return os_handle_t : handle to the object taken or NULL if error (see error code for more info)
 **********************************************************************/
os_handle_t os_obj_single_wait(os_handle_t obj, uint32_t timeout_ticks, os_err_e* err){

	/* Form array and call wait function
	 ---------------------------------------------------*/
	os_handle_t objList[] = { obj };
	return os_obj_wait(objList, 1, OS_OBJ_WAIT_ONE, timeout_ticks, err);
}


/***********************************************************************
 * OS Object multiple wait all
 *
 * @brief This function waits for all objects in a list
 *
 * OS_OBJ_TASK  : The task has finished (i.e. state is OS_TASK_ENDED or OS_TASK_NOT_EXIST)
 * OS_OBJ_SEM   : The semaphore has at least one free counter
 * OS_OBJ_MUTEX : The mutex is free
 * OS_OBJ_EVT   : The event is set
 *
 * @parem os_err_e* err			 : [out] Error code. Ignored if NULL.
 * @param uint32_t timeout_ticks : [ in] Amount of time before a timeout is detected. If OS_WAIT_FOREVER, the task blocks forever. If OS_WAIT_NONE, the task returns immediately
 * @param size_t objNum			 : [ in] number of objects to wait
 * @param ....					 : [ in] all handles to wait separated by comma
 *
 * @return os_handle_t : handle to the object taken or NULL if error (see error code for more info)
 **********************************************************************/
os_handle_t os_obj_multiple_WaitAll(os_err_e* err, uint32_t timeout_ticks, size_t objNum, ...){

	/* Init var function
	 ---------------------------------------------------*/
	va_list args;
	os_handle_t objList[objNum];

	/* Form array
	 ---------------------------------------------------*/
	va_start(args, objNum);
	for(size_t i = 0; i < objNum; i++)	objList[i] = va_arg(args, os_handle_t);

	/* Call wait function
	 ---------------------------------------------------*/
	void* ret = os_obj_wait(objList, objNum, OS_OBJ_WAIT_ALL, timeout_ticks, err);

	/* End var function and return
	 ---------------------------------------------------*/
	va_end(args);
	return ret;
}


/***********************************************************************
 * OS Object multiple wait one
 *
 * @brief This function waits for one object in a list
 *
 * OS_OBJ_TASK  : The task has finished (i.e. state is OS_TASK_ENDED or OS_TASK_NOT_EXIST)
 * OS_OBJ_SEM   : The semaphore has at least one free counter
 * OS_OBJ_MUTEX : The mutex is free
 * OS_OBJ_EVT   : The event is set
 *
 * @parem os_err_e* err			 : [out] Error code. Ignored if NULL.
 * @param uint32_t timeout_ticks : [ in] Amount of time before a timeout is detected. If OS_WAIT_FOREVER, the task blocks forever. If OS_WAIT_NONE, the task returns immediately
 * @param size_t objNum			 : [ in] number of objects to wait
 * @param ....					 : [ in] all handles to wait separated by comma
 *
 * @return os_handle_t : handle to the object taken or NULL if error (see error code for more info)
 **********************************************************************/
os_handle_t os_obj_multiple_WaitOne(os_err_e* err, uint32_t timeout_ticks, size_t objNum, ...){

	/* Init var function
	 ---------------------------------------------------*/
	va_list args;
	os_handle_t objList[objNum];

	/* Form array
	 ---------------------------------------------------*/
	va_start(args, objNum);
	for(size_t i = 0; i < objNum; i++) objList[i] = va_arg(args, os_handle_t);

	/* Call wait function
	 ---------------------------------------------------*/
	void* ret = os_obj_wait(objList, objNum, OS_OBJ_WAIT_ONE, timeout_ticks, err);

	/* End var function and return
	 ---------------------------------------------------*/
	va_end(args);
	return ret;
}


/***********************************************************************
 * OS Object wait all objects in a list
 *
 * @brief This function waits for one or all objects in a list
 *
 * OS_OBJ_TASK  : The task has finished (i.e. state is OS_TASK_ENDED or OS_TASK_NOT_EXIST)
 * OS_OBJ_SEM   : The semaphore has at least one free counter
 * OS_OBJ_MUTEX : The mutex is free
 * OS_OBJ_EVT   : The event is set
 *
 * @param os_handle_t objList[]  : [ in] Array containing all objects to wait
 * @param size_t objNum			 : [ in] number of objects to wait
 * @param uint32_t timeout_ticks : [ in] Amount of time before a timeout is detected. If OS_WAIT_FOREVER, the task blocks forever. If OS_WAIT_NONE, the task returns immediately
 * @parem os_err_e* err			 : [out] Error code. Ignored if NULL.
 *
 * @return os_handle_t : handle to the object taken or NULL if error (see error code for more info)
 **********************************************************************/
os_handle_t os_obj_multiple_lWaitAll(os_handle_t objList[], size_t objNum, uint32_t timeout_ticks, os_err_e* err){

	/* Just call waiting function with the correct flag
	 ---------------------------------------------------*/
	return os_obj_wait(objList, objNum, OS_OBJ_WAIT_ALL, timeout_ticks, err);
}


/***********************************************************************
 * OS Object wait One object in a list
 *
 * @brief This function waits for one or all objects in a list
 *
 * OS_OBJ_TASK  : The task has finished (i.e. state is OS_TASK_ENDED or OS_TASK_NOT_EXIST)
 * OS_OBJ_SEM   : The semaphore has at least one free counter
 * OS_OBJ_MUTEX : The mutex is free
 * OS_OBJ_EVT   : The event is set
 *
 * @param os_handle_t objList[]  : [ in] Array containing all objects to wait
 * @param size_t objNum			 : [ in] number of objects to wait
 * @param uint32_t timeout_ticks : [ in] Amount of time before a timeout is detected. If OS_WAIT_FOREVER, the task blocks forever. If OS_WAIT_NONE, the task returns immediately
 * @parem os_err_e* err			 : [out] Error code. Ignored if NULL.
 *
 * @return os_handle_t : handle to the object taken or NULL if error (see error code for more info)
 **********************************************************************/
os_handle_t os_obj_multiple_lWaitOne(os_handle_t objList[], size_t objNum, uint32_t timeout_ticks, os_err_e* err){

	/* Just call waiting function with the correct flag
	 ---------------------------------------------------*/
	return os_obj_wait(objList, objNum, OS_OBJ_WAIT_ONE, timeout_ticks, err);
}


/***********************************************************************
 * OS Object multiple wait all using va_list
 *
 * @brief This function waits for all objects in a va_list
 *
 * OS_OBJ_TASK  : The task has finished (i.e. state is OS_TASK_ENDED or OS_TASK_NOT_EXIST)
 * OS_OBJ_SEM   : The semaphore has at least one free counter
 * OS_OBJ_MUTEX : The mutex is free
 * OS_OBJ_EVT   : The event is set
 *
 * @parem os_err_e* err			 : [out] Error code. Ignored if NULL.
 * @param uint32_t timeout_ticks : [ in] Amount of time before a timeout is detected. If OS_WAIT_FOREVER, the task blocks forever. If OS_WAIT_NONE, the task returns immediately
 * @param size_t objNum			 : [ in] number of objects to wait
 * @param va_list args			 : [ in] va_list containing all handles to wait
 *
 * @return os_handle_t : handle to the object taken or NULL if error (see error code for more info)
 **********************************************************************/
os_handle_t os_obj_multiple_vWaitAll(os_err_e* err, uint32_t timeout_ticks, size_t objNum, va_list args){

	/* Form object array
	 ---------------------------------------------------*/
	os_handle_t objList[objNum];
	for(size_t i = 0; i < objNum; i++) objList[i] = va_arg(args, os_handle_t);

	/* Call wait function
	 ---------------------------------------------------*/
	return os_obj_wait(objList, objNum, OS_OBJ_WAIT_ALL, timeout_ticks, err);
}


/***********************************************************************
 * OS Object multiple wait one using va_list
 *
 * @brief This function waits for one object in a va_list
 *
 * OS_OBJ_TASK  : The task has finished (i.e. state is OS_TASK_ENDED or OS_TASK_NOT_EXIST)
 * OS_OBJ_SEM   : The semaphore has at least one free counter
 * OS_OBJ_MUTEX : The mutex is free
 * OS_OBJ_EVT   : The event is set
 *
 * @parem os_err_e* err			 : [out] Error code. Ignored if NULL.
 * @param uint32_t timeout_ticks : [ in] Amount of time before a timeout is detected. If OS_WAIT_FOREVER, the task blocks forever. If OS_WAIT_NONE, the task returns immediately
 * @param size_t objNum			 : [ in] number of objects to wait
 * @param va_list args			 : [ in] va_list containing all handles to wait
 *
 * @return os_handle_t : handle to the object taken or NULL if error (see error code for more info)
 **********************************************************************/
os_handle_t os_obj_multiple_vWaitOne(os_err_e* err, uint32_t timeout_ticks, size_t objNum, va_list args){

	/* Form object array
	 ---------------------------------------------------*/
	os_handle_t objList[objNum];
	for(size_t i = 0; i < objNum; i++) objList[i] = va_arg(args, os_handle_t);

	/* Call wait function
	 ---------------------------------------------------*/
	return os_obj_wait(objList, objNum, OS_OBJ_WAIT_ONE, timeout_ticks, err);
}
