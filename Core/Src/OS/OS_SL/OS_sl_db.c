/*
 * os_sl_db.c
 *
 *  Created on: Aug 1, 2022
 *      Author: Gabriel
 */


#include "OS/OS_SL/os_sl.h"
#include "OS/OS_Core/OS_Common.h"
#include "common.h"

/**********************************************
 * PUBLIC VARIABLES
 *********************************************/

/* Data base containing all functions
 ---------------------------------------------------*/
const os_fn_link_table_el_t os_link_table[] = {

		/* USART functions
		 ---------------------------------------------------*/
		OS_LINK_FN("__io_putchar", 				__io_putchar),

		/* Event
		 ---------------------------------------------------*/
		OS_LINK_FN("os_evt_create", 			os_evt_create),
		OS_LINK_FN("os_evt_reset", 				os_evt_reset),
		OS_LINK_FN("os_evt_set", 				os_evt_set),
		OS_LINK_FN("os_evt_set_mode", 			os_evt_set_mode),
		OS_LINK_FN("os_evt_delete",				os_evt_delete),
		OS_LINK_FN("os_evt_getState", 			os_evt_getState),

		/* Heap
		 ---------------------------------------------------*/
		OS_LINK_FN("os_heap_clear", 			os_heap_clear),
		OS_LINK_FN("os_heap_alloc", 			os_heap_alloc),
		OS_LINK_FN("os_heap_free", 				os_heap_free),
		OS_LINK_FN("os_heap_monitor", 			os_heap_monitor),

		/* Message queue
		 ---------------------------------------------------*/
		OS_LINK_FN("os_msgQ_create",		 	os_msgQ_create),
		OS_LINK_FN("os_msgQ_push", 				os_msgQ_push),
		OS_LINK_FN("os_msgQ_delete", 			os_msgQ_delete),
		OS_LINK_FN("os_msgQ_getNumberOfMsgs", 	os_msgQ_getNumberOfMsgs),
		OS_LINK_FN("os_msgQ_wait", 				os_msgQ_wait),

		/* Mutex
		 ---------------------------------------------------*/
		OS_LINK_FN("os_mutex_create", 			os_mutex_create),
		OS_LINK_FN("os_mutex_release", 			os_mutex_release),
		OS_LINK_FN("os_mutex_delete", 			os_mutex_delete),
		OS_LINK_FN("os_mutex_getState", 		os_mutex_getState),

		/* Wait
		 ---------------------------------------------------*/
		OS_LINK_FN("os_obj_single_wait", 		os_obj_single_wait),
		OS_LINK_FN("os_obj_multiple_WaitAll", 	os_obj_multiple_WaitAll),
		OS_LINK_FN("os_obj_multiple_WaitOne", 	os_obj_multiple_WaitOne),
		OS_LINK_FN("os_obj_multiple_lWaitAll", 	os_obj_multiple_lWaitAll),
		OS_LINK_FN("os_obj_multiple_lWaitOne", 	os_obj_multiple_lWaitOne),
		OS_LINK_FN("os_obj_multiple_vWaitAll", 	os_obj_multiple_vWaitAll),
		OS_LINK_FN("os_obj_multiple_vWaitOne", 	os_obj_multiple_vWaitOne),

		/* Scheduler
		 ---------------------------------------------------*/
		OS_LINK_FN("os_scheduler_start", 		os_scheduler_start),
		OS_LINK_FN("os_scheduler_stop", 		os_scheduler_stop),
		OS_LINK_FN("os_scheduler_state_get", 	os_scheduler_state_get),

		/* Semaphore
		 ---------------------------------------------------*/
		OS_LINK_FN("os_sem_create", 			os_sem_create),
		OS_LINK_FN("os_sem_release", 			os_sem_release),
		OS_LINK_FN("os_sem_delete",				os_sem_delete),
		OS_LINK_FN("os_sem_getCount", 			os_sem_getCount),

		/* Tick
		 ---------------------------------------------------*/
		OS_LINK_FN("os_getMsTick", 				os_getMsTick),

		/* Tasks
		 ---------------------------------------------------*/
		OS_LINK_FN("os_task_create", 			os_task_create),
		OS_LINK_FN("os_task_createProcess", 	os_task_createProcess),
		OS_LINK_FN("os_task_end", 				os_task_end),
		OS_LINK_FN("os_task_return", 			os_task_return),
		OS_LINK_FN("os_task_delete", 			os_task_delete),
		OS_LINK_FN("os_task_yeild", 			os_task_yeild),
		OS_LINK_FN("os_task_getPrio", 			os_task_getPrio),
		OS_LINK_FN("os_task_sleep", 			os_task_sleep),
		OS_LINK_FN("os_task_getReturn", 		os_task_getReturn),
		OS_LINK_FN("os_task_getState",			os_task_getState),
		OS_LINK_FN("os_task_getByPID", 			os_task_getByPID),

		/* LEDS
		 ---------------------------------------------------*/
		OS_LINK_FN("os_leds_set", 				os_leds_set),
		OS_LINK_FN("os_leds_get", 				os_leds_get),
		OS_LINK_FN("os_leds_toggle", 			os_leds_toggle),

		/* FS
		 ---------------------------------------------------*/
		OS_LINK_FN("os_fopen", 					os_fopen),
		OS_LINK_FN("os_fclose", 				os_fclose),
		OS_LINK_FN("os_fread", 					os_fread),
		OS_LINK_FN("os_fwrite", 				os_fwrite),
		OS_LINK_FN("os_fseek", 					os_fseek),
};


/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Link Table get size
 *
 * @brief This function gets the link table size
 *
 * @return size_t : size of the function link table
 *
 **********************************************************************/
size_t os_sl_linkTable_getSize(){
	return (size_t)COUNTOF(os_link_table);
}
