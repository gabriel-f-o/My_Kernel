/*
 * OS_Event.c
 *
 *  Created on: Jul 3, 2022
 *      Author: Gabriel
 */


#include "OS/OS.h"
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
 * OS Event get free count
 *
 * @brief This function gets the amount of times this object can be "taken" before it is considered unavailable
 *
 * @param os_handle_t h : [in] object to verify the availability
 *
 * @return uint32_t : the amount of times the object can be taken
 *
 **********************************************************************/
static uint32_t os_evt_getFreeCount(os_handle_t h){

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return 0;
	if(h->type != OS_OBJ_EVT) return 0;

	/* Convert address
	 ------------------------------------------------------*/
	os_evt_t* evt = (os_evt_t*)h;

	/* Calculates free count
	 ------------------------------------------------------*/
	uint32_t freeCount = 0;
	if(evt->state == OS_EVT_STATE_SET){
		freeCount = 1;
		if(evt->mode == OS_EVT_MODE_MANUAL){
			freeCount = OS_OBJ_COUNT_INF;
		}
	}

	return freeCount;
}


/***********************************************************************
 * OS Event take
 *
 * @brief This function takes possession of the object.
 *
 * @param os_handle_t h 			: [in] object to take
 * @param os_handle_t takingTask	: [in] handle to the task that is taking the object
 *
 **********************************************************************/
static void os_evt_objTake(os_handle_t h, os_handle_t takingTask){
	UNUSED_ARG(takingTask);

	/* Convert address
	 ------------------------------------------------------*/
	os_evt_t* evt = (os_evt_t*)h;

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return;
	if(h->type != OS_OBJ_EVT) return;
	if(evt->state == OS_EVT_STATE_RESET) return;

	/* Reset event if auto mode
	 ------------------------------------------------------*/
	if(evt->mode == OS_EVT_MODE_AUTO) evt->state = OS_EVT_STATE_RESET;
}


/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/


/***********************************************************************
 * OS Event Create
 *
 * @brief This function creates an event
 *
 * @param os_handle_t* h 			: [out] handle to event
 * @param os_evt_reset_mode_e mode 	: [ in] Event reset mode. Auto means the first task that waits for it will reset it. Manual means that the event must be reset using the os_evt_reset API
 * @param char* name				: [ in] Event's name. If an event with the same name already exists, its reference is returned. A null name always creates a nameless event.
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_evt_create(os_handle_t* h, os_evt_reset_mode_e mode, char const * name){

	/* Check for argument errors
	 ------------------------------------------------------*/
	if(h == NULL) 						return OS_ERR_BAD_ARG;
	if(mode >= __OS_EVT_MODE_INVALID) 	return OS_ERR_BAD_ARG;
	if(os_init_get() == false)			return OS_ERR_NOT_READY;

	/* If event exists, return it
	 ------------------------------------------------------*/
	if(name != NULL){
		os_list_cell_t* obj = os_handle_list_searchByName(&os_obj_head, OS_OBJ_EVT, name);
		if(obj != NULL){
			*h = obj->element;
			return OS_ERR_OK;
		}
	}

	/* Alloc the event block
	 ------------------------------------------------------*/
	os_evt_t* evt = (os_evt_t*)os_heap_alloc(sizeof(os_evt_t));

	/* Check allocation
	 ------------------------------------------------------*/
	if(evt == 0) return OS_ERR_INSUFFICIENT_HEAP;

	/* Init event
	 ------------------------------------------------------*/
	evt->obj.objUpdate		= 0;
	evt->obj.type			= OS_OBJ_EVT;
	evt->obj.getFreeCount	= os_evt_getFreeCount;
	evt->obj.obj_take		= os_evt_objTake;
	evt->obj.blockList		= os_list_init();
	evt->obj.name			= (char*)name;

	evt->state				= OS_EVT_STATE_RESET;
	evt->mode				= mode;

	/* Add object to object list
	 ------------------------------------------------------*/
	os_err_e ret = os_list_add(&os_obj_head, (os_handle_t) evt, OS_LIST_FIRST);
	if(ret != OS_ERR_OK) return ret;

	/* Return
	 ------------------------------------------------------*/
	*h = (os_handle_t)evt;
	return OS_ERR_OK;
}


/***********************************************************************
 * OS Event Reset
 *
 * @brief This function resets an event.
 *
 * @param os_handle_t h   	: [ in] Pointer to the event to reset
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_evt_reset(os_handle_t h){

	/* Convert address
	 ------------------------------------------------------*/
	os_evt_t* evt = (os_evt_t*) h;

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return OS_ERR_BAD_ARG;
	if(h->type != OS_OBJ_EVT) return OS_ERR_BAD_ARG;
	if(evt->state == OS_EVT_STATE_RESET) return OS_ERR_OK;

	/* Enter critical section
	 ------------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Tag as reset
	 ------------------------------------------------------*/
	evt->state = OS_EVT_STATE_RESET;

	/* Update blocking list and check if we must yield
	 ------------------------------------------------------*/
	bool must_yield = os_handle_list_updateAndCheck( (os_handle_t)evt );

	/* Yield if necessary
	 ------------------------------------------------------*/
	if(must_yield && os_scheduler_state_get() == OS_SCHEDULER_START) os_task_yeild();

	/* Exit
	 ------------------------------------------------------*/
	OS_EXIT_CRITICAL();

	return OS_ERR_OK;
}


/***********************************************************************
 * OS Event Set
 *
 * @brief This function sets an event.
 *
 * @param os_handle_t h   	: [ in] Pointer to the event to set
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_evt_set(os_handle_t h){

	/* Convert address
	 ------------------------------------------------------*/
	os_evt_t* evt = (os_evt_t*) h;

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return OS_ERR_BAD_ARG;
	if(h->type != OS_OBJ_EVT) return OS_ERR_BAD_ARG;
	if(evt->state == OS_EVT_STATE_SET) return OS_ERR_OK;

	/* Enter critical section
	 ------------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Tag as set
	 ------------------------------------------------------*/
	evt->state = OS_EVT_STATE_SET;

	/* Update blocking list and check if we must yield
	 ------------------------------------------------------*/
	bool must_yield = os_handle_list_updateAndCheck( (os_handle_t)evt );

	/* Yield if necessary
	 ------------------------------------------------------*/
	if(must_yield && os_scheduler_state_get() == OS_SCHEDULER_START) os_task_yeild();

	/* Exit
	 ------------------------------------------------------*/
	OS_EXIT_CRITICAL();

	return OS_ERR_OK;
}


/***********************************************************************
 * OS Event Set mode
 *
 * @brief This function sets the mode for the event.
 *
 * @param os_handle_t h   			: [ in] Pointer to the event to reset
 * @param os_evt_reset_mode_e mode 	: [ in] Event reset mode. Auto means the first task that waits for it will reset it. Manual means that the event must be reset using the os_evt_reset API
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_evt_set_mode(os_handle_t h, os_evt_reset_mode_e mode){

	/* Convert address
	 ------------------------------------------------------*/
	os_evt_t* evt = (os_evt_t*) h;

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return OS_ERR_BAD_ARG;
	if(h->type != OS_OBJ_EVT) return OS_ERR_BAD_ARG;
	if(mode >= __OS_EVT_MODE_INVALID) return OS_ERR_BAD_ARG;

	/* Enter critical section
	 ------------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Tag as unlocked
	 ------------------------------------------------------*/
	evt->mode = mode;

	/* Update blocking list and check if we must yield
	 ------------------------------------------------------*/
	bool must_yield = os_handle_list_updateAndCheck( (os_handle_t)evt );

	/* Yield if necessary
	 ------------------------------------------------------*/
	if(must_yield && os_scheduler_state_get() == OS_SCHEDULER_START) os_task_yeild();

	/* Exit
	 ------------------------------------------------------*/
	OS_EXIT_CRITICAL();

	return OS_ERR_OK;
}


/***********************************************************************
 * OS Event Delete
 *
 * @brief This function deletes an event. It must not be called if there is a task waiting for it.
 * A task waiting for a deleted object will cause undefined behavior.
 *
 * @param os_handle_t h : [ in] Pointer to the event to delete
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_evt_delete(os_handle_t h){

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return OS_ERR_BAD_ARG;
	if(h->type != OS_OBJ_EVT) return OS_ERR_BAD_ARG;

	/* Deletes from obj list
	 ------------------------------------------------------*/
	os_list_remove(&os_obj_head, h);

	/* Free memory
	 ------------------------------------------------------*/
	os_list_clear(h->blockList);

	return os_heap_free(h);
}


/***********************************************************************
 * OS Event Get state
 *
 * @brief This function gets the current state of an event
 *
 * @param os_handle_t h : [ in] Pointer to the event
 *
 * @return os_evt_state_e : the event state
 **********************************************************************/
os_evt_state_e os_evt_getState(os_handle_t h){

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return __OS_EVT_STATE_INVALID;
	if(h->type != OS_OBJ_EVT) return __OS_EVT_STATE_INVALID;

	return ((os_evt_t*)h)->state;
}
