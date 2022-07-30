/*
 * OS_Mutex.c
 *
 *  Created on: Mar 20, 2022
 *      Author: Gabriel
 */


#include "OS/OS_Core/OS.h"
#include "OS/OS_Core/OS_Internal.h"

/**********************************************
 * EXTERN VARIABLES
 *********************************************/

extern os_list_cell_t* os_cur_task;	//Current task pointer
extern os_list_head_t  os_obj_head;	//Head to obj list

/**********************************************
 * PRIVATE FUNCTIONS
 *********************************************/


/***********************************************************************
 * OS Mutex get free count
 *
 * @brief This function gets the amount of times this object can be "taken" before it is considered unavailable
 *
 * @param os_handle_t h : [in] object to verify the availability
 *
 * @return uint32_t : the amount of times the object can be taken
 *
 **********************************************************************/
static uint32_t os_mutex_getFreeCount(os_handle_t h){

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return 0;
	if(h->type != OS_OBJ_MUTEX) return 0;

	/* Returns the if the mutex free state
	 ------------------------------------------------------*/
	return ((os_mutex_t*)h)->state == OS_MUTEX_STATE_UNLOCKED ? 1 : 0;
}


/***********************************************************************
 * OS Mutex take
 *
 * @brief This function takes possession of the object.
 *
 * @param os_handle_t h 			: [in] object to take
 * @param os_handle_t takingTask	: [in] handle to the task that is taking the object
 *
 * @return os_err_e : 0 if ok
 **********************************************************************/
static os_err_e os_mutex_objTake(os_handle_t h, os_handle_t takingTask){

	/* Convert address
	 ------------------------------------------------------*/
	os_task_t* t	  = (os_task_t*)takingTask;
	os_mutex_t* mutex = (os_mutex_t*)h;

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return OS_ERR_BAD_ARG;
	if(h->type != OS_OBJ_MUTEX) return OS_ERR_BAD_ARG;
	if(mutex->state == OS_MUTEX_STATE_LOCKED) return OS_ERR_BAD_ARG;

	if(takingTask == NULL) return OS_ERR_BAD_ARG;
	if(takingTask->type != OS_OBJ_TASK) return OS_ERR_BAD_ARG;

	/* Store owner task
	 ------------------------------------------------------*/
	mutex->owner = takingTask;

	/* flag as taken
	 ------------------------------------------------------*/
	mutex->state = OS_MUTEX_STATE_LOCKED;

	/* Add mutex to the owned mutex list
	 ------------------------------------------------------*/
	return os_list_add(t->ownedMutex, h, OS_LIST_FIRST);
}


/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/


/***********************************************************************
 * OS Mutex Create
 *
 * @brief This function creates a mutex
 *
 * @param os_handle_t* h 		: [out] handle to semaphore
 * @param char* name			: [ in] Mutex's name. If a mutex with the same name already exists, its reference is returned. A null name always creates a nameless mutex.
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_mutex_create(os_handle_t* h, char const * name){

	/* Check for argument errors
	 ------------------------------------------------------*/
	if(h == NULL) 					return OS_ERR_BAD_ARG;
	if(os_init_get() == false)		return OS_ERR_NOT_READY;

	/* If mutex exists, return it
	 ------------------------------------------------------*/
	if(name != NULL){
		os_list_cell_t* obj = os_handle_list_searchByName(&os_obj_head, OS_OBJ_MUTEX, name);
		if(obj != NULL){
			*h = obj->element;
			return OS_ERR_OK;
		}
	}

	/* Alloc the mutex block
	 ------------------------------------------------------*/
	os_mutex_t* mutex = (os_mutex_t*)os_heap_alloc(sizeof(os_mutex_t));

	/* Check allocation
	 ------------------------------------------------------*/
	if(mutex == 0) return OS_ERR_INSUFFICIENT_HEAP;

	/* Init mutex
	 ------------------------------------------------------*/
	mutex->obj.objUpdate		= 0;
	mutex->obj.type				= OS_OBJ_MUTEX;
	mutex->obj.getFreeCount		= os_mutex_getFreeCount;
	mutex->obj.obj_take			= os_mutex_objTake;
	mutex->obj.blockList		= os_list_init();
	mutex->obj.name				= (char*)name;

	mutex->state 				= OS_MUTEX_STATE_UNLOCKED;
	mutex->owner 				= NULL;
	mutex->max_prio 			= -1;

	/* Handles heap errors
	 ------------------------------------------------------*/
	if(mutex->obj.blockList == NULL){
		os_heap_free(mutex);
		return OS_ERR_INSUFFICIENT_HEAP;
	}

	/* Add object to object list
	 ------------------------------------------------------*/
	os_err_e ret = os_list_add(&os_obj_head, (os_handle_t) mutex, OS_LIST_FIRST);
	if(ret != OS_ERR_OK) {
		os_heap_free(mutex);
		os_list_clear(mutex->obj.blockList);
		return ret;
	}

	/* Return
	 ------------------------------------------------------*/
	*h = (os_handle_t)mutex;
	return OS_ERR_OK;
}


/***********************************************************************
 * OS Mutex Release
 *
 * @brief This function frees the mutex.
 *
 * @param os_handle_t h   	: [ in] Pointer to the mutex to release
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_mutex_release(os_handle_t h){

	/* Convert address
	 ------------------------------------------------------*/
	os_mutex_t* mutex = (os_mutex_t*) h;

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return OS_ERR_BAD_ARG;
	if(h->type != OS_OBJ_MUTEX) return OS_ERR_BAD_ARG;
	if(mutex->state == OS_MUTEX_STATE_UNLOCKED) return OS_ERR_FORBIDDEN;
	if(mutex->owner != os_cur_task->element) return OS_ERR_FORBIDDEN;

	/* Enter critical section
	 ------------------------------------------------------*/
	OS_CRITICAL_SECTION(

		/* Remove mutex from owned mutex list
		 ------------------------------------------------------*/
		os_list_remove( ((os_task_t*)mutex->owner)->ownedMutex, h);

		/* Update priority of the owner
		 ------------------------------------------------------*/
		os_obj_updatePrio(mutex->owner);

		/* Tag as unlocked
		 ------------------------------------------------------*/
		mutex->state = OS_MUTEX_STATE_UNLOCKED;

		/* Update blocking list and check if we must yield
		 ------------------------------------------------------*/
		bool must_yield = os_handle_list_updateAndCheck( (os_handle_t)mutex );

		/* Yield if necessary
		 ------------------------------------------------------*/
		if(must_yield && os_scheduler_state_get() == OS_SCHEDULER_START) os_task_yeild();

	);

	return OS_ERR_OK;
}


/***********************************************************************
 * OS Mutex Delete
 *
 * @brief This function deletes a mutex. It must not be called if there is a task waiting for it.
 * A task waiting for a deleted object will cause undefined behavior.
 *
 * @param os_handle_t h : [ in] Pointer to the mutex to delete
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_mutex_delete(os_handle_t h){

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return OS_ERR_BAD_ARG;
	if(h->type != OS_OBJ_MUTEX) return OS_ERR_BAD_ARG;

	/* Deletes from obj list
	 ------------------------------------------------------*/
	os_list_remove(&os_obj_head, h);

	/* Free memory
	 ------------------------------------------------------*/
	os_list_clear(h->blockList);

	return os_heap_free(h);
}


/***********************************************************************
 * OS Mutex Get state
 *
 * @brief This function gets the current state of a mutex
 *
 * @param os_handle_t h : [ in] Pointer to the mutex
 *
 * @return os_mutex_state_e : the mutex state
 **********************************************************************/
os_mutex_state_e os_mutex_getState(os_handle_t h){

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return __OS_MUTEX_STATE_INVALID;
	if(h->type != OS_OBJ_MUTEX) return __OS_MUTEX_STATE_INVALID;

	return ((os_mutex_t*)h)->state;
}
