/*
 * OS_Mutex.h
 *
 *  Created on: Mar 20, 2022
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_MUTEX_H_
#define INC_OS_OS_MUTEX_H_

#include "OS/OS_Core/OS_Common.h"
#include "OS/OS_Core/OS_Obj.h"

/**********************************************
 * PUBLIC TYPES
 *********************************************/

/* Mutex state
 ---------------------------------------------------*/
typedef enum{
	OS_MUTEX_STATE_UNLOCKED,
	OS_MUTEX_STATE_LOCKED,
	__OS_MUTEX_STATE_INVALID,
}os_mutex_state_e;

/* Mutex object (useful to cast from handle to mutex)
 ---------------------------------------------------*/
typedef struct os_mutex_{
	os_obj_t 			obj; 		//MUST BE FIRST MEMBER. Object base structure
	os_handle_t			owner;  	//Handle to the owner task
	os_mutex_state_e 	state;		//State of the mutex
	int8_t				max_prio;	//Store maximum priority
} os_mutex_t;

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
os_err_e os_mutex_create(os_handle_t* h, char const * name);


/***********************************************************************
 * OS Mutex Release
 *
 * @brief This function frees the mutex.
 *
 * @param os_handle_t h   	: [ in] Pointer to the mutex to release
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_mutex_release(os_handle_t h);


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
os_err_e os_mutex_delete(os_handle_t h);


/***********************************************************************
 * OS Mutex Get state
 *
 * @brief This function gets the current state of a mutex
 *
 * @param os_handle_t h : [ in] Pointer to the mutex
 *
 * @return os_mutex_state_e : the mutex state
 **********************************************************************/
os_mutex_state_e os_mutex_getState(os_handle_t h);


/***********************************************************************
 * OS Get Mutex from handle
 *
 * @brief This function gets the mutex object from the handle
 *
 * @param os_handle_t h : [ in] Pointer to the semaphore
 *
 * @return os_mutex_t* : NULL if error, the mutex reference if OK
 **********************************************************************/
static inline os_mutex_t* os_mutex_getFromHandle(os_handle_t h){
	if(h == NULL) return NULL;
	if(h->type != OS_OBJ_MUTEX) return NULL;

	return (os_mutex_t*)h;
}

#endif /* INC_OS_OS_MUTEX_H_ */
