/*
 * OS_Event.h
 *
 *  Created on: Jul 3, 2022
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_EVENT_H_
#define INC_OS_OS_EVENT_H_

#include "OS/OS_Core/OS_Common.h"
#include "OS/OS_Core/OS_Obj.h"

/**********************************************
 * PUBLIC TYPES
 *********************************************/

/* Event state
 ---------------------------------------------------*/
typedef enum{
	OS_EVT_STATE_SET,
	OS_EVT_STATE_RESET,
	__OS_EVT_STATE_INVALID,
}os_evt_state_e;

/* Event reset mode
 ---------------------------------------------------*/
typedef enum{
	OS_EVT_MODE_AUTO,
	OS_EVT_MODE_MANUAL,
	__OS_EVT_MODE_INVALID,
}os_evt_reset_mode_e;

/* Event object (useful to cast from handle to event)
 ---------------------------------------------------*/
typedef struct os_evt_{
	os_obj_t 			obj; 		//MUST BE FIRST MEMBER. Object base structure
	os_evt_state_e 		state;		//State of the event
	os_evt_reset_mode_e mode;		//Reset mode
} os_evt_t;

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
os_err_e os_evt_create(os_handle_t* h, os_evt_reset_mode_e mode, char const * name);


/***********************************************************************
 * OS Event Reset
 *
 * @brief This function resets an event.
 *
 * @param os_handle_t h   	: [ in] Pointer to the event to reset
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_evt_reset(os_handle_t h);


/***********************************************************************
 * OS Event Set
 *
 * @brief This function sets an event.
 *
 * @param os_handle_t h   	: [ in] Pointer to the event to set
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_evt_set(os_handle_t h);


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
os_err_e os_evt_set_mode(os_handle_t h, os_evt_reset_mode_e mode);


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
os_err_e os_evt_delete(os_handle_t h);


/***********************************************************************
 * OS Event Get state
 *
 * @brief This function gets the current state of an event
 *
 * @param os_handle_t h : [ in] Pointer to the event
 *
 * @return os_evt_state_e : the event state
 **********************************************************************/
os_evt_state_e os_evt_getState(os_handle_t h);


/***********************************************************************
 * OS Get Event from handle
 *
 * @brief This function gets the event object from the handle
 *
 * @param os_handle_t h : [ in] Pointer to the event
 *
 * @return os_evt_t* : NULL if error, the evt reference if OK
 **********************************************************************/
static inline os_evt_t* os_evt_getFromHandle(os_handle_t h){
	if(h == NULL) return NULL;
	if(h->type != OS_OBJ_EVT) return NULL;

	return (os_evt_t*)h;
}


#endif /* INC_OS_OS_EVENT_H_ */
