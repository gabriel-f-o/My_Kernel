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
 * @brief Unused. Just to prevent crashes
 *
 * @param os_handle_t h : [in] object to verify the availability
 *
 * @return uint32_t : the amount of times the object can be taken
 *
 **********************************************************************/
static uint32_t os_msgQ_getFreeCount(os_handle_t h){
	UNUSED_ARG(h);
	return 0;
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
 * @param os_hMsgQ_t* msgQ 		: [out] handle to msgQ
 * @param os_msgQ_mode_e mode 	: [ in] The queue's mode: FIFO or LIFO
 * @param char* name			: [ in] messqge Q name. If a queue with the same name already exists, its reference is returned. A null name always creates a nameless queue.
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_msgQ_create(os_hMsgQ_t* msgQ, os_msgQ_mode_e mode, char const * name){

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
	q->obj.name 			= (char*)name;
	q->obj.objUpdate 		= 0;
	q->obj.getFreeCount		= os_msgQ_getFreeCount;
	q->obj.obj_take 		= os_msgQ_objTake;
	q->obj.blockList		= os_list_init();

	q->msgList		 		= os_list_init();
	q->mode		 			= mode;

	/* Handles heap errors
	 ------------------------------------------------------*/
	if(q->obj.blockList == NULL || q->msgList == NULL){
		os_heap_free(q);
		os_list_clear(q->obj.blockList);
		os_list_clear(q->msgList);
		return OS_ERR_INSUFFICIENT_HEAP;
	}

	/* Add object to object list
	 ------------------------------------------------------*/
	os_err_e ret = os_list_add(&os_obj_head, (os_handle_t) q, OS_LIST_FIRST);
	if(ret != OS_ERR_OK) {
		os_heap_free(q);
		os_list_clear(q->obj.blockList);
		os_list_clear(q->msgList);
		return ret;
	}

	/* Return
	 ------------------------------------------------------*/
	*msgQ = q;
	return OS_ERR_OK;


}


/***********************************************************************
 * OS MsgQ Push
 *
 * @brief This function pushes a message into the queue depending on the mode
 *
 * @param os_hMsgQ_t msgQ  	: [ in] Handle to the queue
 * @param void* msg      	: [ in] Reference to the message
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_msgQ_push(os_hMsgQ_t msgQ, void* msg){

	/* Check arguments
	 ------------------------------------------------------*/
	if(msgQ == NULL) return OS_ERR_BAD_ARG;
	if(msgQ->obj.type != OS_OBJ_MSGQ) return OS_ERR_BAD_ARG;

	/* add message on list
	 ------------------------------------------------------*/
	os_err_e ret = os_list_add(((os_list_head_t*)msgQ->msgList), msg, msgQ->mode);
	if(ret != OS_ERR_OK)
		return ret;

	/* Update block list
	 ------------------------------------------------------*/
	if(os_msgQ_updateAndCheck(msgQ) && os_scheduler_state_get() == OS_SCHEDULER_START) os_task_yeild();
	return OS_ERR_OK;
}


/***********************************************************************
 * OS MsgQ delete
 *
 * @brief This function deletes a message queue
 *
 * @param os_hMsgQ_t msgQ  	: [ in] Handle to the queue
 * @param void* msg      	: [ in] Reference to the message
 *
 * @return os_err_e OS_ERR_OK if OK
 **********************************************************************/
os_err_e os_msgQ_delete(os_hMsgQ_t msgQ){

	/* Check arguments
	 ------------------------------------------------------*/
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

	return os_heap_free(msgQ);
}


/***********************************************************************
 * OS MsgQ get number of messages
 *
 * @brief This function gets the number of messages in a queue
 *
 * @param os_hMsgQ_t msgQ  	: [ in] Handle to the queue
 *
 * @return uint32_t : 0xFFFFFFFF if error. The number of messages in the queue if >=0
 **********************************************************************/
uint32_t os_msgQ_getNumberOfMsgs(os_hMsgQ_t msgQ){

	/* Check arguments
	 ------------------------------------------------------*/
	if(msgQ == NULL) return 0xFFFFFFFF;
	if(msgQ->obj.type != OS_OBJ_MSGQ) return 0xFFFFFFFF;

	/* return number of messages
	 ------------------------------------------------------*/
	return ((os_list_head_t*)msgQ->msgList)->listSize;
}


/***********************************************************************
 * OS MsgQ wait
 *
 * @brief This function waits for a message, returing the message received.
 *
 * @param os_hMsgQ_t msgQ  			: [ in] Handle to the queue
 * @param uint32_t timeout_ticks	: [ in] Wait timeout. OS_WAIT_FOREVER to wait forever. OS_WAIT_NONE to not block.
 * @param os_err_e* err				: [out] Error code or NULL to ignore
 *
 * @return int32_t : negative number if error. The number of messages in the queue if >=0
 **********************************************************************/
void* os_msgQ_wait(os_hMsgQ_t msgQ, uint32_t timeout_ticks, os_err_e* err){

	/* Check arguments
	 ------------------------------------------------------*/
	if(msgQ == NULL || msgQ->obj.type != OS_OBJ_MSGQ) {
		if(err == NULL) *err = OS_ERR_BAD_ARG;
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

	/* Loop here until timeout
	 ------------------------------------------------------*/
	while(1){

		/* Get number of messages
		 ------------------------------------------------------*/
		uint32_t msgs = os_msgQ_getNumberOfMsgs(msgQ);

		/* If there is a message
		 ------------------------------------------------------*/
		if(msgs > 0){

			/* Pop it
			 ---------------------------------------------------*/
			void* msg = os_list_pop(((os_list_head_t*)msgQ->msgList), OS_LIST_FIRST, err);

			/* If task blocked, remove from block list
			 ---------------------------------------------------*/
			if(blocked) {
				os_list_remove(msgQ->obj.blockList, (os_handle_t)os_cur_task->element);
			}

			/* Update block list
			 ---------------------------------------------------*/
			os_msgQ_updateAndCheck(msgQ);

			/* Exit critical
			 ---------------------------------------------------*/
			OS_EXIT_CRITICAL();

			/* call cb if needed
			 ---------------------------------------------------*/
			if(blocked)  os_task_on_ready_cb(os_cur_task->element);

			/* Return address of the object
			 ---------------------------------------------------*/
			if(err != NULL) *err = OS_ERR_OK;
			return msg;
		}

		/* If the timout set is 0, then just return
		 ---------------------------------------------------*/
		if(timeout_ticks <= OS_WAIT_NONE){

			/* If task blocked, remove from list
			 ---------------------------------------------------*/
			if(blocked) {
				os_list_remove(msgQ->obj.blockList, (os_handle_t)os_cur_task->element);
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

			/* If task blocked, remove from block list
			 ---------------------------------------------------*/
			if(blocked) {
				os_list_remove(msgQ->obj.blockList, (os_handle_t)os_cur_task->element);
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
		((os_task_t*)os_cur_task->element)->objWaited 		= (os_handle_t*)&msgQ;
		((os_task_t*)os_cur_task->element)->sizeObjs		= 1;
		((os_task_t*)os_cur_task->element)->objWanted		= 0xFFFFFFFF;
		((os_task_t*)os_cur_task->element)->waitFlag		= OS_OBJ_WAIT_ONE;

		/* If not yet blocked
		 ---------------------------------------------------*/
		if(!blocked){

			/* Add task to object's block list if not already
			 ---------------------------------------------------*/
			os_list_add(msgQ->obj.blockList, (os_handle_t)os_cur_task->element, OS_LIST_FIRST);

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
		timeout_ticks 										= ((os_task_t*)os_cur_task->element)->wakeCoutdown;
		((os_task_t*)os_cur_task->element)->objWaited 		= NULL;
		((os_task_t*)os_cur_task->element)->wakeCoutdown 	= 0;
		((os_task_t*)os_cur_task->element)->sizeObjs		= 0;
		((os_task_t*)os_cur_task->element)->objWanted		= 0xFFFFFFFF;

	}

	/* Should not be here
	 ---------------------------------------------------*/
	if(blocked) {
		os_list_remove(msgQ->obj.blockList, (os_handle_t)os_cur_task->element);
	}

	/* Return
	 ---------------------------------------------------*/
	OS_EXIT_CRITICAL();
	if(err != NULL) *err = OS_ERR_UNKNOWN;
	return NULL;
}
