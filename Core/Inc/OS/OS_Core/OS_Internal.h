/*
 * OS_Internal.h
 *
 *  Created on: Oct 23, 2021
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_INTERNAL_H_
#define INC_OS_OS_INTERNAL_H_

#include "OS/OS_Core/OS_Common.h"
#include "OS/OS_Core/OS_Tasks.h"
#include "OS/OS_Core/OS_MsgQ.h"

/**********************************************
 * OS PRIVATE TYPES
 *********************************************/

/* Task list cell
 ---------------------------------------------------*/
typedef struct os_list_cell_{
	struct os_list_cell_* next;			//Points to the next stack on the list
	struct os_list_cell_* prev;			//Points to the previous stack on the list
	void*  		 		 element;		//Pointer to the element
	uint32_t			 order;			//To store the order of waiting
} os_list_cell_t;

/* Head object
 ---------------------------------------------------*/
typedef struct os_list_head_{
	os_list_cell_t 	 	 head;			//Head structure
	os_list_cell_t*		 first;			//Pointer to the first element
	os_list_cell_t*		 last;			//Pointer to the first element
	uint32_t 			 listSize;		//Size of list
} os_list_head_t;

/* Enum to add to a list
 ---------------------------------------------------*/
typedef enum{
	OS_LIST_FIRST,
	OS_LIST_LAST,
	__OS_LIST_INVALID,
}os_list_pos_e;

/**********************************************
 * OS PRIVATE FUNCTIONS
 *********************************************/

//////////////////////////////////////////////// Priority //////////////////////////////////////////////////


/***********************************************************************
 * OS Object Update Prio
 *
 * @brief This function updates the maximum priority of an object
 *
 * @param os_handle_t h : [in] The reference to the object
 *
 * @return bool : (1) = priority changed; (0) = Nothing changed
 *
 **********************************************************************/
void os_obj_updatePrio(os_handle_t h);


//////////////////////////////////////////////// TASKS //////////////////////////////////////////////////


/***********************************************************************
 * OS Task Init
 *
 * @brief This function initializes the default tasks
 *
 * @param char* main_name				: [in] main task name. A null name creates a nameless task.
 * @param int8_t   main_task_priority   : [in] main stack base priority (0 to 127)
 * @param uint32_t interrput_stack_size : [in] size of the interrupt stack
 * @param uint32_t idle_stack_size 		: [in] size of the idle stack
 *
 * @return os_err_e : Error code (0 = OK)
 *
 **********************************************************************/
os_err_e os_task_init(char* main_name, int8_t main_task_priority, uint32_t interrput_stack_size, uint32_t idle_stack_size);


/***********************************************************************
 * OS Task Must Yeild
 *
 * @brief This function checks the priority of all tasks and decides if the current task should yield
 *
 * @return bool : 1 = yeild
 *
 **********************************************************************/
bool os_task_must_yeild();


//////////////////////////////////////////////// HANDLE LISTS //////////////////////////////////////////////////

/***********************************************************************
 * OS List Init
 *
 * @brief This function initializes a list
 *
 * @return os_list_head_t* : reference to the head of the list
 **********************************************************************/
os_list_head_t* os_list_init();


/***********************************************************************
 * OS List Search
 *
 * @brief This function searches for an element in a list
 *
 * @param os_list_head_t* head : [in] reference to the head of the list
 * @param void* el	  		   : [in] element to search
 *
 * @return os_list_cell_t* : reference to the cell containing the element or null if not found
 **********************************************************************/
os_list_cell_t* os_list_search(os_list_head_t* head, void* el);


/***********************************************************************
 * OS Handle List Search by name
 *
 * @brief This function searches for a handle in a list using its name and type. Must be used in handle lists
 *
 * @param os_list_head_t* head : [in] reference to the head of the list
 * @param os_obj_type_e type   : [in] type of the object
 * @param char* name		   : [in] name of the object
 *
 * @return os_list_cell_t* : reference to the cell containing the handle or null if not found
 **********************************************************************/
os_list_cell_t* os_handle_list_searchByName(os_list_head_t* head, os_obj_type_e type, char const * name);


/***********************************************************************
 * OS List add
 *
 * @brief This function adds an element in a list
 *
 * @param os_list_head_t* head  : [in] reference to the head of the list
 * @param void* el	   		    : [in] reference to the element
 * @param os_list_add_pos_e pos : [in] flag to indicate whether to add at the first or last position
 *
 * @return os_err_e : error code (0 = OK)
 **********************************************************************/
os_err_e os_list_add(os_list_head_t* head, void* el, os_list_pos_e pos);


/***********************************************************************
 * OS List Pop
 *
 * @brief This function pops an element from a list
 *
 * @param os_list_head_t* head  : [ in] reference to the head of the list
 * @param os_list_add_pos_e pos : [ in] flag to indicate whether to add at the first or last position
 * @param os_err_e* err			: [out] OS error code, NULL to ignore
 *
 * @return void* : address of the element popped or NULL if error
 **********************************************************************/
void* os_list_pop(os_list_head_t* head, os_list_pos_e pos, os_err_e* err);


/***********************************************************************
 * OS List remove
 *
 * @brief This function removes an element from a list
 *
 * @param os_list_head_t* head : [in] reference to the head of the list
 * @param void* el	   		   : [in]  reference to the element
 *
 * @return os_err_e : error code (0 = OK)
 **********************************************************************/
os_err_e os_list_remove(os_list_head_t* head, void* el);


/***********************************************************************
 * OS List clear
 *
 * @brief This function clears a list, freeing all its cells and head
 *
 * @param os_list_head_t* head : [in] reference to the head of the list
 *
 **********************************************************************/
void os_list_clear(os_list_head_t* head);


/***********************************************************************
 * OS Handle List sort
 *
 * @brief This function sorts a task list, sorting from higher priority to lower.
 * If priorities are equal the first task that entered the list will be put first.
 * Must be used on task lists
 *
 * @param os_list_head_t* head : [in] reference to the head of the list
 *
 **********************************************************************/
void os_task_list_sort(os_list_head_t* head);


/***********************************************************************
 * OS Handle list get object to update
 *
 * @brief This function searches for an object in the object list that needs to update. Must be used in a handle list
 *
 * @return os_handle_t handle to the object to update or NULL if nothing to do
 **********************************************************************/
os_handle_t os_handle_list_getObjToUpdate();


/***********************************************************************
 * OS Task list is object free on task
 *
 * @brief This function scans an object's block list to decide whether the object is still free on a given task. Must be used with a task list.
 *
 * @param os_handle_t obj  : [in] object to scan
 * @param os_handle_t task : [in] task to stop the scan
 *
 * @return bool : 1 = object is free ; 0 = object is not free
 **********************************************************************/
bool os_task_list_isObjFreeOnTask(os_handle_t obj, os_handle_t task);


/***********************************************************************
 * OS Handle update and check
 *
 * @brief This function updates the block list of all objects that needs an update.
 * It returns whether the current task must yield or not.
 * Must be used with a handle list
 *
 * @param os_handle_t h : [in] handle to the object to update
 *
 * @return bool : 1 = current task must yield
 **********************************************************************/
bool os_handle_list_updateAndCheck(os_handle_t h);


/***********************************************************************
 * OS MsgQ update and check
 *
 * @brief This function updates the block list for a messageQ
 *
 * @param os_hMsgQ_t msgQ : [in] msgQ to update
 *
 * @return bool : (1) current task should yeild
 *
 **********************************************************************/
bool os_msgQ_updateAndCheck(os_hMsgQ_t msgQ);

#endif /* INC_OS_OS_INTERNAL_H_ */
