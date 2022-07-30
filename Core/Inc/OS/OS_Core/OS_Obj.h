/*
 * OS_Obj.h
 *
 *  Created on: Oct 23, 2021
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_OBJ_H_
#define INC_OS_OS_OBJ_H_

#include "OS/OS_Core/OS_Common.h"

/**********************************************
 * DEFINES
 *********************************************/

#define OS_OBJ_COUNT_INF		(0xFFFFFFFF)
#define OS_WAIT_FOREVER			(0xFFFFFFFF)
#define OS_WAIT_NONE			(0x00)

/**********************************************
 * PUBLIC TYPES
 *********************************************/

/* OS Handler
 ---------------------------------------------------*/
typedef struct os_obj_* os_handle_t;

/* Obj Types flags
 ---------------------------------------------------*/
typedef enum{
	OS_OBJ_INVALID,
	OS_OBJ_TASK,
	OS_OBJ_MUTEX,
	OS_OBJ_SEM,
	OS_OBJ_EVT,
	OS_OBJ_MSGQ
}os_obj_type_e;


typedef enum{
	OS_OBJ_WAIT_ONE,
	OS_OBJ_WAIT_ALL,
}os_obj_wait_e;


/* Base object structure
 ---------------------------------------------------*/
typedef struct os_obj_{
	os_obj_type_e 	type;															//Indicates what type of object
	char*		 	name;															//Object's name
	bool			objUpdate;														//Indicates if an update is needed in the block list of this object
	uint32_t 		(*getFreeCount) (os_handle_t h);								//Function to get the freecount
	os_err_e		(*obj_take) 	(os_handle_t h, os_handle_t takingTask);		//Function to take the object
	void* 			blockList;														//Blocked list head (tasks waiting for this object are listed here)
}os_obj_t;


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
os_handle_t os_obj_single_wait(os_handle_t obj, uint32_t timeout_ticks, os_err_e* err);


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
os_handle_t os_obj_multiple_WaitAll(os_err_e* err, uint32_t timeout_ticks, size_t objNum, ...);


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
os_handle_t os_obj_multiple_WaitOne(os_err_e* err, uint32_t timeout_ticks, size_t objNum, ...);


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
os_handle_t os_obj_multiple_lWaitAll(os_handle_t objList[], size_t objNum, uint32_t timeout_ticks, os_err_e* err);


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
os_handle_t os_obj_multiple_lWaitOne(os_handle_t objList[], size_t objNum, uint32_t timeout_ticks, os_err_e* err);


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
os_handle_t os_obj_multiple_vWaitAll(os_err_e* err, uint32_t timeout_ticks, size_t objNum, va_list args);


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
os_handle_t os_obj_multiple_vWaitOne(os_err_e* err, uint32_t timeout_ticks, size_t objNum, va_list args);


#endif /* INC_OS_OS_OBJ_H_ */
