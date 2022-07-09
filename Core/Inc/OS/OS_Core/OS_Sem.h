/*
 * OS_Sem.h
 *
 *  Created on: Feb 20, 2022
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_SEM_H_
#define INC_OS_OS_SEM_H_

#include "OS/OS_Core/OS_Common.h"
#include "OS/OS_Core/OS_Obj.h"

/**********************************************
 * PUBLIC TYPES
 *********************************************/

/* Semaphore object (useful to cast from handle to semaphore)
 ---------------------------------------------------*/
typedef struct os_sem_{
	os_obj_t obj; 			//MUST BE FIRST MEMBER. Object base structure
	uint16_t count_max;		//Max threshold for counter
	uint16_t count;			//Counter current value
} os_sem_t;

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
os_err_e os_sem_create(os_handle_t* h, uint16_t init_count, uint16_t max_count, char const * name);


/***********************************************************************
 * OS Semaphore Release
 *
 * @brief This function frees positions in the semaphore.
 *
 * @param os_handle_t h   		: [ in] Pointer to the semaphore to release
 * @param uint16_t amount 		: [ in] Amount of times to release
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_sem_release(os_handle_t h, uint16_t amount);


/***********************************************************************
 * OS Semaphore Delete
 *
 * @brief This function deletes a semaphore. It must not be called if there is a task waiting for it.
 * A task waiting for a deleted object will cause undefined behavior.
 *
 * @param os_handle_t h   		: [ in] Pointer to the semaphore to delete
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_sem_delete(os_handle_t h);


/***********************************************************************
 * OS Semaphore Get counter
 *
 * @brief This function gets the current counter of a semaphore
 *
 * @param os_handle_t h : [ in] Pointer to the semaphore to delete
 *
 * @return uint16_t : the counter of the semaphore, or 0xFFFF if invalid
 **********************************************************************/
uint16_t os_sem_getCount(os_handle_t h);


/***********************************************************************
 * OS Get Semaphore from handle
 *
 * @brief This function gets the semaphore object from the handle
 *
 * @param os_handle_t h : [ in] Pointer to the semaphore
 *
 * @return os_sem_t* : NULL if error, the semaphore reference if OK
 **********************************************************************/
static inline os_sem_t* os_sem_getFromHandle(os_handle_t h){
	if(h == NULL) return NULL;
	if(h->type != OS_OBJ_SEM) return NULL;

	return (os_sem_t*)h;
}

#endif /* INC_OS_OS_SEM_H_ */
