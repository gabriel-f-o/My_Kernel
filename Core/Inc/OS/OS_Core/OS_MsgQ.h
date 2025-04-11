/*
 * OS_MsgQ.h
 *
 *  Created on: Jul 3, 2022
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_MSGQ_H_
#define INC_OS_OS_MSGQ_H_

#include "OS/OS_Core/OS_Common.h"
#include "OS/OS_Core/OS_Obj.h"

/**********************************************
 * PUBLIC TYPES
 *********************************************/

/* Msg Queue mode
 ---------------------------------------------------*/
typedef enum{
	OS_MSGQ_MODE_FIFO,
	OS_MSGQ_MODE_LIFO,
	__OS_MSGQ_MODE_INVALID
} os_msgQ_mode_e;

/* Message queue object (useful to cast from handle to msgQ)
 ---------------------------------------------------*/
typedef struct os_msgQ_{
	os_obj_t 		obj; 			//MUST BE FIRST MEMBER. Object base structure
	void*	 		msgList;		//List containing all messages
	os_msgQ_mode_e	mode;			//Message queue mode (FIFO or LIFO)
} os_msgQ_t;

/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/


/***********************************************************************
 * OS MsgQ Create
 *
 * @brief This function creates a message queue
 *
 * @param os_handle_t* msgQ 	: [out] handle to msgQ
 * @param os_msgQ_mode_e mode 	: [ in] The queue's mode: FIFO or LIFO
 * @param char* name			: [ in] messqge Q name. If a queue with the same name already exists, its reference is returned. A null name always creates a nameless queue.
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_msgQ_create(os_handle_t* msgQ, os_msgQ_mode_e mode, char const * name);


/***********************************************************************
 * OS MsgQ Push
 *
 * @brief This function pushes a message into the queue depending on the mode
 *
 * @param os_handle_t h : [ in] Handle to the queue
 * @param void* msg     : [ in] Reference to the message
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_msgQ_push(os_handle_t h, void* msg);


/***********************************************************************
 * OS MsgQ Pop
 *
 * @brief This function pops a message from the queue
 *
 * @param os_handle_t h : [ in] Handle to the queue
 * @param os_err_e* err : [out] Reference to the error message. NULL to ignore
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
void* os_msgQ_pop(os_handle_t h, os_err_e* err);


/***********************************************************************
 * OS MsgQ delete
 *
 * @brief This function deletes a message queue
 *
 * @param os_handle_t h : [ in] Handle to the queue
 * @param void* msg     : [ in] Reference to the message
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_msgQ_delete(os_handle_t h);


/***********************************************************************
 * OS MsgQ get number of messages
 *
 * @brief This function gets the number of messages in a queue
 *
 * @param os_handle_t h	: [ in] Handle to the queue
 *
 * @return uint32_t : 0xFFFFFFFF if error. The number of messages in the queue if >=0
 **********************************************************************/
uint32_t os_msgQ_getNumberOfMsgs(os_handle_t h);


#endif /* INC_OS_OS_MSGQ_H_ */
