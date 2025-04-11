/*
 * OS_MsgQ.c
 *
 *  Created on: Jul 3, 2022
 *      Author: Gabriel
 */

#include "OS/OS_Core/OS.h"
#include "OS/OS_Core/OS_Internal.h"

/**********************************************
 * EXTERN VARIABLES
 *********************************************/

extern os_list_head_t os_obj_head;	//Head to obj list
extern os_list_cell_t* os_cur_task;	//Current task pointer

/**********************************************
 * PRIVATE FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS MsgQ get free count
 *
 * @brief Gets the amount of times the queue can pop before blocking
 *
 * @param os_handle_t h : [in] object to verify the availability
 *
 * @return uint32_t : the amount of times the object can be taken
 *
 **********************************************************************/
static uint32_t os_msgQ_getFreeCount(os_handle_t h, os_handle_t takingTask){
	UNUSED_ARG(h);
    UNUSED_ARG(takingTask);
	return os_msgQ_getNumberOfMsgs(h);
}


/***********************************************************************
 * OS MSGQ take
 *
 * @brief Unused. Just to prevent crashes
 *
 * @param os_handle_t h 			: [in] object to take
 * @param os_handle_t takingTask	: [in] handle to the task that is taking the object
 *
 * @return os_err_e : 0 if OK
 **********************************************************************/
static os_err_e os_msgQ_objTake(os_handle_t h, os_handle_t takingTask){
	UNUSED_ARG(h);
	UNUSED_ARG(takingTask);

	return OS_ERR_OK;
}

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
os_err_e os_msgQ_create(os_handle_t* msgQ, os_msgQ_mode_e mode, char const * name){

	/* Check for argument errors
	 ------------------------------------------------------*/
	if(msgQ == NULL) 							return OS_ERR_BAD_ARG;
	if(mode >= __OS_MSGQ_MODE_INVALID) 		return OS_ERR_BAD_ARG;
	if(os_init_get() == false)				return OS_ERR_NOT_READY;

	/* If messageQ exists, return it
	 ------------------------------------------------------*/
	if(name != NULL){
		os_list_cell_t* obj = os_handle_list_searchByName(&os_obj_head, OS_OBJ_MSGQ, name);
		if(obj != NULL){
			*msgQ = obj->element;
			return OS_ERR_OK;
		}
	}

	/* Alloc the msgQ block
	 ------------------------------------------------------*/
	os_msgQ_t* q = (os_msgQ_t*)os_heap_alloc(sizeof(os_msgQ_t));

	/* Check allocation
	 ------------------------------------------------------*/
	if(q == 0) return OS_ERR_INSUFFICIENT_HEAP;

	/* Init msgQ
	 ------------------------------------------------------*/
	q->obj.type 			= OS_OBJ_MSGQ;
	q->obj.objUpdate 		= 0;
	q->obj.getFreeCount		= os_msgQ_getFreeCount;
	q->obj.obj_take 		= os_msgQ_objTake;
	q->obj.blockList		= os_list_init();
	q->obj.name				= name == NULL ? NULL : (char*)os_heap_alloc(strlen(name) + 1);

	/* Finish init
	 ------------------------------------------------------*/
	q->msgList		 		= os_list_init();
	q->mode		 			= mode;

	/* Handles heap errors
	 ------------------------------------------------------*/
	if(q->obj.blockList == NULL || q->msgList == NULL || (q->obj.name == NULL && name != NULL) ){
		os_list_clear(q->obj.blockList);
		os_list_clear(q->msgList);
		os_heap_free(q->obj.name);
		os_heap_free(q);

		return OS_ERR_INSUFFICIENT_HEAP;
	}

	/* Copy name
	 ------------------------------------------------------*/
	if(name != NULL)
		strcpy(q->obj.name, name);

	/* Add object to object list
	 ------------------------------------------------------*/
	os_err_e ret = os_list_add(&os_obj_head, (os_handle_t) q, OS_LIST_FIRST);
	if(ret != OS_ERR_OK) {
		os_list_clear(q->obj.blockList);
		os_list_clear(q->msgList);
		os_heap_free(q->obj.name);
		os_heap_free(q);

		return ret;
	}

	/* Return
	 ------------------------------------------------------*/
	*msgQ = (os_handle_t)q;
	return OS_ERR_OK;
}


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
os_err_e os_msgQ_push(os_handle_t h, void* msg){

	/* Check arguments
	 ------------------------------------------------------*/
	os_msgQ_t* msgQ = (os_msgQ_t*)h;
	if(msgQ == NULL) return OS_ERR_BAD_ARG;
	if(msgQ->obj.type != OS_OBJ_MSGQ) return OS_ERR_BAD_ARG;

	/* add message on list
	 ------------------------------------------------------*/
	os_err_e ret = os_list_add(((os_list_head_t*)msgQ->msgList), msg, msgQ->mode == OS_MSGQ_MODE_FIFO ? OS_LIST_FIRST : OS_LIST_LAST);
	if(ret != OS_ERR_OK)
		return ret;

	/* Update block list
	 ------------------------------------------------------*/
	if(os_handle_list_updateAndCheck((os_handle_t)msgQ) && os_scheduler_state_get() == OS_SCHEDULER_START) os_task_yeild();
	return OS_ERR_OK;
}


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
void* os_msgQ_pop(os_handle_t h, os_err_e* err){

	/* Check arguments
	 ------------------------------------------------------*/
	os_msgQ_t* msgQ = (os_msgQ_t*)h;
	if(msgQ == NULL || msgQ->obj.type != OS_OBJ_MSGQ){
        if(err != NULL) 
            *err = OS_ERR_BAD_ARG;

        return NULL;
    } 

	/* Check if queue is empty
    ------------------------------------------------------*/
    if(((os_list_head_t*)msgQ->msgList)->listSize == 0) {
        if(err != NULL) 
            *err = OS_ERR_EMPTY;
            
        return NULL;
    } 

	/* remove message from list
	 ------------------------------------------------------*/
	void* ret = os_list_pop(((os_list_head_t*)msgQ->msgList), OS_LIST_FIRST, err);

	/* Update block list
	 ------------------------------------------------------*/
	os_handle_list_updateAndCheck((os_handle_t)msgQ);
    
    if(err != NULL) 
        *err = OS_ERR_OK;
	
    return ret;
}


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
os_err_e os_msgQ_delete(os_handle_t h){

	/* Check arguments
	 ------------------------------------------------------*/
	os_msgQ_t* msgQ = (os_msgQ_t*)h;
	if(msgQ == NULL) return OS_ERR_BAD_ARG;
	if(msgQ->obj.type != OS_OBJ_MSGQ) return OS_ERR_BAD_ARG;

	/* Deletes from obj list
	 ------------------------------------------------------*/
	os_list_remove(&os_obj_head, msgQ);

	/* Free block list
	 ------------------------------------------------------*/
	os_list_clear(msgQ->obj.blockList);

	/* Free msg list
	 ------------------------------------------------------*/
	os_list_clear(msgQ->msgList);
	os_heap_free(msgQ->obj.name);

	return os_heap_free(msgQ);
}


/***********************************************************************
 * OS MsgQ get number of messages
 *
 * @brief This function gets the number of messages in a queue
 *
 * @param os_handle_t h	: [ in] Handle to the queue
 *
 * @return uint32_t : 0xFFFFFFFF if error. The number of messages in the queue if >=0
 **********************************************************************/
uint32_t os_msgQ_getNumberOfMsgs(os_handle_t h){

	/* Check arguments
	 ------------------------------------------------------*/
	os_msgQ_t* msgQ = (os_msgQ_t*)h;
	if(msgQ == NULL) return 0xFFFFFFFF;
	if(msgQ->obj.type != OS_OBJ_MSGQ) return 0xFFFFFFFF;

	/* return number of messages
	 ------------------------------------------------------*/
	return ((os_list_head_t*)msgQ->msgList)->listSize;
}
