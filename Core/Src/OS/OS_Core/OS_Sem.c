/*
 * OS_Sem.c
 *
 *  Created on: Feb 25, 2022
 *      Author: INGE2
 */

#include "OS/OS_Core/OS.h"
#include "OS/OS_Core/OS_Internal.h"

/**********************************************
 * EXTERN VARIABLES
 *********************************************/

extern os_list_head_t os_obj_head;	//Head to obj list

/**********************************************
 * PRIVATE FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Semaphore get free count
 *
 * @brief This function gets the amount of times this object can be "taken" before it is considered unavailable
 *
 * @param os_handle_t h : [in] object to verify the availability
 *
 **********************************************************************/
static uint32_t os_sem_getFreeCount(os_handle_t h, os_handle_t takingTask){   
	/* Check arguments
	 ------------------------------------------------------*/
    UNUSED_ARG(takingTask);
	if(h == NULL) return 0;
	if(h->type != OS_OBJ_SEM) return 0;

	/* Returns the semaphore counter
	 ------------------------------------------------------*/
	return ((os_sem_t*)h)->count;
}


/***********************************************************************
 * OS Semaphore take
 *
 * @brief This function takes possession of the object.
 *
 * @param os_handle_t h 			: [in] object to take
 * @param os_handle_t takingTask	: [in] handle to the task that is taking the object
 *
 * @return os_err_e : error code (0 = OK)
 *
 **********************************************************************/
static os_err_e os_sem_objTake(os_handle_t h, os_handle_t takingTask){
	UNUSED_ARG(takingTask);

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return OS_ERR_BAD_ARG;
	if(h->type != OS_OBJ_SEM) return OS_ERR_BAD_ARG;
	if( ((os_sem_t*)h)->count <= 0) return OS_ERR_BAD_ARG;

	/* Take it
	 ------------------------------------------------------*/
	OS_CRITICAL_SECTION(
		((os_sem_t*)h)->count--;
	);

	return OS_ERR_OK;
}


/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/


/***********************************************************************
 * OS Semaphore Create
 *
 * @brief This function creates a semaphore
 *
 * @param os_handle_t* h 		: [out] handle to semaphore
 * @param uint16_t init_count 	: [ in] The init value for the semaphore's counter
 * @param uint16_t max_count	: [ in] The max count for the semaphore's counter
 * @param char* name			: [ in] semaphore's name. If a semaphore with the same name already exists, its reference is returned. A null name always creates a nameless semaphore.
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_sem_create(os_handle_t* h, uint16_t init_count, uint16_t max_count, char const * name){

	/* Check for argument errors
	 ------------------------------------------------------*/
	if(h == NULL) 							return OS_ERR_BAD_ARG;
	if(init_count > max_count) 				return OS_ERR_BAD_ARG;
	if(os_init_get() == false)				return OS_ERR_NOT_READY;

	/* If semaphore exists, return it
	 ------------------------------------------------------*/
	if(name != NULL){
		os_list_cell_t* obj = os_handle_list_searchByName(&os_obj_head, OS_OBJ_SEM, name);
		if(obj != NULL){
			*h = obj->element;
			return OS_ERR_OK;
		}
	}

	/* Alloc the semaphore block
	 ------------------------------------------------------*/
	os_sem_t* sem = (os_sem_t*)os_heap_alloc(sizeof(os_sem_t));

	/* Check allocation
	 ------------------------------------------------------*/
	if(sem == 0) return OS_ERR_INSUFFICIENT_HEAP;

	/* Init semaphore
	 ------------------------------------------------------*/
	sem->obj.objUpdate		= 0;
	sem->obj.type			= OS_OBJ_SEM;
	sem->obj.getFreeCount	= &os_sem_getFreeCount;
	sem->obj.obj_take		= &os_sem_objTake;
	sem->obj.blockList		= os_list_init();
	sem->obj.name			= name == NULL ? NULL : (char*)os_heap_alloc(strlen(name) + 1);

	/* Finish init
	 ------------------------------------------------------*/
	sem->count 				= init_count;
	sem->count_max 			= max_count;

	/* Handles heap errors
	 ------------------------------------------------------*/
	if(sem->obj.blockList == NULL || (sem->obj.name == NULL && name != NULL) ){
		os_list_clear(sem->obj.blockList);
		os_heap_free(sem->obj.name);
		os_heap_free(sem);
		return OS_ERR_INSUFFICIENT_HEAP;
	}

	/* Copy name
	 ------------------------------------------------------*/
	if(name != NULL)
		strcpy(sem->obj.name, name);

	/* Add object to object list
	 ------------------------------------------------------*/
	os_err_e ret = os_list_add(&os_obj_head, (os_handle_t) sem, OS_LIST_FIRST);
	if(ret != OS_ERR_OK) {
		os_list_clear(sem->obj.blockList);
		os_heap_free(sem->obj.name);
		os_heap_free(sem);
		return ret;
	}

	/* Return
	 ------------------------------------------------------*/
	*h = (os_handle_t)sem;
	return OS_ERR_OK;
}


/***********************************************************************
 * OS Semaphore Release
 *
 * @brief This function frees positions in the semaphore.
 *
 * @param os_handle_t h   	: [ in] Pointer to the semaphore to release
 * @param uint16_t amount 	: [ in] Amount of times to release
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_sem_release(os_handle_t h, uint16_t amount){

	/* Convert address
	 ------------------------------------------------------*/
	os_sem_t* sem = (os_sem_t*) h;

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return OS_ERR_BAD_ARG;
	if(h->type != OS_OBJ_SEM) return OS_ERR_BAD_ARG;
	if(amount == 0) return OS_ERR_BAD_ARG;
	if(sem->count + amount > sem->count_max) return OS_ERR_BAD_ARG;

	/* Enter critical section to avoid corrupting the semaphore
	 ------------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Increment counter
	 ------------------------------------------------------*/
	sem->count = (uint16_t)(sem->count + amount);

	/* Update blocking list and check if we must yield
	 ------------------------------------------------------*/
	bool must_yield = os_handle_list_updateAndCheck( (os_handle_t)sem );

	/* Yield if necessary
	 ------------------------------------------------------*/
	if(must_yield && os_scheduler_state_get() == OS_SCHEDULER_START) os_task_yeild();

	OS_EXIT_CRITICAL();
	return OS_ERR_OK;
}


/***********************************************************************
 * OS Semaphore Delete
 *
 * @brief This function deletes a semaphore. It must not be called if there is a task waiting for it.
 * A task waiting for a deleted object will cause undefined behavior.
 *
 * @param os_handle_t h : [ in] Pointer to the semaphore to delete
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_sem_delete(os_handle_t h){

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return OS_ERR_BAD_ARG;
	if(h->type != OS_OBJ_SEM) return OS_ERR_BAD_ARG;

	/* Deletes from obj list
	 ------------------------------------------------------*/
	os_list_remove(&os_obj_head, h);

	/* Free memory
	 ------------------------------------------------------*/
	os_list_clear(h->blockList);
	os_heap_free(h->name);

	return os_heap_free(h);
}


/***********************************************************************
 * OS Semaphore Get counter
 *
 * @brief This function gets the current counter of a semaphore
 *
 * @param os_handle_t h : [ in] Pointer to the semaphore to get
 *
 * @return uint16_t : the counter of the semaphore, or 0xFFFF if invalid
 **********************************************************************/
uint16_t os_sem_getCount(os_handle_t h){

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return 0xFFFF;
	if(h->type != OS_OBJ_SEM) return 0xFFFF;

	return ((os_sem_t*)h)->count;
}
