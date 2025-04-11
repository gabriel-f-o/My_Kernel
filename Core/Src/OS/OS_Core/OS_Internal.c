	/*
 * OS_Internal.c
 *
 *  Created on: Feb 24, 2022
 *      Author: Gabriel
 */


#include "OS/OS_Core/OS_Common.h"
#include "OS/OS_Core/OS_Obj.h"
#include "OS/OS_Core/OS_Heap.h"
#include "OS/OS_Core/OS_Internal.h"
#include "OS/OS_Core/OS_Tasks.h"
#include "OS/OS_FS/lfs.h"
#include "common.h"

/**********************************************
 * EXTERNAL VARIABLES
 *********************************************/

extern os_list_cell_t* os_cur_task;	//Current task pointer
extern os_list_head_t  os_obj_head;	//Current task pointer

/**********************************************
 * OS PRIVATE FUNCTIONS
 *********************************************/

//////////////////////////////////////////////// Priority //////////////////////////////////////////////////

/***********************************************************************
 * OS Mutex Update Prio
 *
 * @brief This function updates the maximum priority of a mutex
 *
 * @param os_handle_t h : [in] The reference to the mutex
 *
 * @return bool : (1) = priority changed; (0) = Nothing changed
 *
 **********************************************************************/
static bool os_mutex_udpatePrio(os_handle_t h){

	/* Error Check
	 ---------------------------------------------------*/
	if(h == NULL) return false;
	if(h->type != OS_OBJ_MUTEX) return false;

	/* Convet reference and store previous priority
	 ---------------------------------------------------*/
	os_mutex_t* mutex = (os_mutex_t*)h;
	int8_t prev_max_prio = mutex->max_prio;

	/* Get reference to the first blocked task
	 ---------------------------------------------------*/
	int8_t maxPrio = -1;
	os_list_head_t* head = (os_list_head_t*)h->blockList;
	os_list_cell_t* it = head->head.next;

	/* While it is a valid task
	 ---------------------------------------------------*/
	while(it != NULL){

		/* Get task's priority and calculate maximum
		 ---------------------------------------------------*/
		int8_t taskPrio = ((os_task_t*)it->element)->priority;
		if(maxPrio < taskPrio) maxPrio = taskPrio;

		/* Goes to next task
		 ---------------------------------------------------*/
		it = it->next;
	}

	/* Store priority and return
	 ---------------------------------------------------*/
	mutex->max_prio = maxPrio;
	return prev_max_prio != maxPrio;
}


/***********************************************************************
 * OS Task Update Prio
 *
 * @brief This function updates the maximum priority of a task
 *
 * @param os_handle_t h : [in] The reference to the task
 *
 * @return bool : (1) = priority changed; (0) = Nothing changed
 *
 **********************************************************************/
static bool os_task_udpatePrio(os_handle_t h){

	/* Error check
	 ---------------------------------------------------*/
	if(h == NULL) return false;
	if(h->type != OS_OBJ_TASK) return false;

	/* Store priority
	 ---------------------------------------------------*/
	os_task_t* tsk = (os_task_t*)h;
	int8_t prev_prio = tsk->priority;

	/* Point to the first task on block list
	 ---------------------------------------------------*/
	int8_t maxPrio = tsk->basePriority;
	os_list_head_t* head = (os_list_head_t*)h->blockList;
	os_list_cell_t* it = head->head.next;

	/* While it is a valid task
	 ---------------------------------------------------*/
	while(it != NULL){

		/* Get task's priority and calculate maximum
		 ---------------------------------------------------*/
		int8_t taskPrio = ((os_task_t*)it->element)->priority;
		if(maxPrio < taskPrio) maxPrio = taskPrio;

		/* Goes to next task
		 ---------------------------------------------------*/
		it = it->next;
	}

	/* Point to the first mutex in the owned list
	 ---------------------------------------------------*/
	head = (os_list_head_t*) ( ((os_task_t*)h)->ownedMutex);
	it = head->head.next;

	/* While it is a valid mutex
	 ---------------------------------------------------*/
	while(it != NULL){

		/* Get mutex' priority and calculate maximum
		 ---------------------------------------------------*/
		int8_t taskPrio = ((os_mutex_t*)it->element)->max_prio;
		if(maxPrio < taskPrio) maxPrio = taskPrio;

		/* Goes to next mutex
		 ---------------------------------------------------*/
		it = it->next;
	}

	/* Store priority and return
	 ---------------------------------------------------*/
	tsk->priority = maxPrio;
	return prev_prio != maxPrio;
}


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
void os_obj_updatePrio(os_handle_t h){

	/* Enter critical section
	 ---------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* If the object is a mutex, update its priority
	 ---------------------------------------------------*/
	if(h->type == OS_OBJ_MUTEX && os_mutex_udpatePrio(h)){
		os_obj_updatePrio(((os_mutex_t*)h)->owner); //Update its owner priority if the mutex's priority changed
	}

	/* If the object is a task, update its priority
	 ---------------------------------------------------*/
	if(h->type == OS_OBJ_TASK && os_task_udpatePrio(h)){

		/* If its priority changed, we must update the objects the task is waiting
		 ---------------------------------------------------*/
		for(size_t i = 0; i < ((os_task_t*)h)->sizeObjs; i++){

			/* objects that are not tasks or mutexes
			 ---------------------------------------------------*/
			if(((os_task_t*)h)->objWaited[i]->type == OS_OBJ_MUTEX || ((os_task_t*)h)->objWaited[i]->type == OS_OBJ_TASK){

				/* Update object's priority
				 ---------------------------------------------------*/
				os_obj_updatePrio(((os_task_t*)h)->objWaited[i]);
			}

			/* update msgQ
			 ---------------------------------------------------*/
			/*if(((os_task_t*)h)->objWaited[i]->type == OS_OBJ_MSGQ){
				os_msgQ_updateAndCheck((os_hMsgQ_t)((os_task_t*)h)->objWaited[i]);
			}*/
		}
	}

	/* Exit critical section
	 ---------------------------------------------------*/
	OS_EXIT_CRITICAL();
}


//////////////////////////////////////////////// HANDLE LISTS //////////////////////////////////////////////////


/***********************************************************************
 * OS List Init
 *
 * @brief This function initializes a list
 *
 * @return os_list_head_t* : reference to the head of the list
 **********************************************************************/
os_list_head_t* os_list_init(){

	/* Allocate head
	 ---------------------------------------------------*/
	os_list_head_t* ret = (os_list_head_t*)os_heap_alloc(sizeof(os_list_head_t));
	if(ret == NULL) return NULL;

	/* Init head and return
	 ---------------------------------------------------*/
	ret->head.next = NULL;
	ret->head.prev = NULL;
	ret->head.element = NULL;
	ret->first = NULL;
	ret->last = NULL;
	ret->listSize = 0;

	return ret;
}


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
os_list_cell_t* os_list_search(os_list_head_t* head, void* el){

	/* Check for argument errors
	 ------------------------------------------------------*/
	if(el == NULL) return NULL;
	if(head == NULL) return NULL;

	/* Enter Critical Section
	 * If it's searching / inserting a block, it can be interrupted and another task can change the list. In this case, the first task will blow up when returning
	 ------------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Search position to insert
	 ------------------------------------------------------*/
	os_list_cell_t* it = head->head.next;
	while(it != NULL && it->element != el){
		it = it->next;
	}

	OS_EXIT_CRITICAL();
	return it;
}


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
os_list_cell_t* os_handle_list_searchByName(os_list_head_t* head, os_obj_type_e type, char const * name){

	/* Check for argument errors
	 ------------------------------------------------------*/
	if(head == NULL) return NULL;
	if(name == NULL) return NULL;

	/* Enter Critical Section
	 * If it's searching / inserting a block, it can be interrupted and another task can change the list. In this case, the first task will blow up when returning
	 ------------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Search list
	 ------------------------------------------------------*/
	os_list_cell_t* it = head->head.next;
	while(it != NULL){

		/* If type and name match, we break.
		 ------------------------------------------------------*/
		if( ((os_handle_t)(it->element))->type == type && ((os_handle_t)(it->element))->name != NULL && strcmp(name, ((os_handle_t)(it->element))->name) == 0) break;

		/* Otherwise we go to next one
		 ------------------------------------------------------*/
		it = it->next;
	}

	OS_EXIT_CRITICAL();
	return it;
}


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
os_err_e os_list_add(os_list_head_t* head, void* el, os_list_pos_e pos){

	/* Enter Critical Section
	 ------------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL(); //If it's searching / inserting a block, it can be interrupted and another task can change the list. In this case, the first task will blow up when returning

	/* Allocate cell
	 ------------------------------------------------------*/
	os_list_cell_t* new = (os_list_cell_t*)os_heap_alloc(sizeof(os_list_cell_t));

	/* Check allocation
	 ------------------------------------------------------*/
	if(new == NULL) {

		/* EXIT Critical Section
		 ------------------------------------------------------*/
		OS_EXIT_CRITICAL();
		return OS_ERR_INSUFFICIENT_HEAP;
	}

	/* Store information on new cell
	 ------------------------------------------------------*/
	new->order  = head->listSize++;
	new->element = el;

	/* Add first element
	 ------------------------------------------------------*/
	if(head->head.next == NULL) {
		head->last = new;
		new->prev = &head->head;
		new->next = NULL;
		head->head.next = new;
		head->first = new;
	}
	else{

		/* Add cell at the beginning
		 ------------------------------------------------------*/
		if(pos == OS_LIST_FIRST){
			new->prev = head->first->prev;
			new->next = head->first;
			head->first->prev->next = new;
			head->first->prev = new;
			head->first = new;
		}

		/* Add cell at the end
		 ------------------------------------------------------*/
		if(pos == OS_LIST_LAST){
			new->next  = head->last->next;
			new->prev  = head->last;
			head->last->next = new;
			head->last = new;
		}
	}
	OS_EXIT_CRITICAL();
	return OS_ERR_OK;
}


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
os_err_e os_list_remove(os_list_head_t* head, void* el){

	/* Convert address
	 ------------------------------------------------------*/
	os_task_t* t = (os_task_t*) el;

	/* Check for argument errors
	 ------------------------------------------------------*/
	if(t == NULL) return OS_ERR_BAD_ARG;

	/* Search cell to verify it is in the list
	 ------------------------------------------------------*/
	os_list_cell_t* pCell = os_list_search(head, el);

	/* Return if not
	 ------------------------------------------------------*/
	if(pCell == NULL) return OS_ERR_INVALID;

	/* Enter Critical Section
	 ------------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL(); //If it's searching / inserting a block, it can be interrupted and another task can change the list. In this case, the first task will blow up when returning

	/* Declare Iterators
	 ------------------------------------------------------*/
	os_list_cell_t* it = &head->head;
	os_list_cell_t* pPrev = NULL;

	/* Search for the task
	 ------------------------------------------------------*/
	while(it->next != NULL){

		/* Update order
		 ------------------------------------------------------*/
		if(it->next->order > pCell->order) it->next->order--;

		/* Store previous
		 ------------------------------------------------------*/
		if(it->next->element == el) pPrev = it;

		/* Go to next
		 ------------------------------------------------------*/
		it = it->next;
	}

	/* Update first
	 ------------------------------------------------------*/
	if(pCell == head->first){
		head->first = head->first->next;
	}

	/* Update last
	 ------------------------------------------------------*/
	if(pCell == head->last){
		head->last = (head->last->prev == &head->head) ? NULL : head->last->prev;
	}

	/* Kill cell
	 ------------------------------------------------------*/
	pPrev->next = pPrev->next->next;
	if(pCell->next != NULL) 
        pCell->next->prev = pCell->prev;

	/* Reduce size and return
	 ------------------------------------------------------*/
	head->listSize--;
	OS_EXIT_CRITICAL();
	return os_heap_free(pCell);
}


/***********************************************************************
 * OS List Pop
 *
 * @brief This function pops an element from a list. Use it in a list that does not care about the order
 *
 * @param os_list_head_t* head  : [ in] reference to the head of the list
 * @param os_list_add_pos_e pos : [ in] flag to indicate whether to add at the first or last position
 * @param os_err_e* err			: [out] OS error code, NULL to ignore
 *
 * @return void* : address of the element popped or NULL if error
 **********************************************************************/
void* os_list_pop(os_list_head_t* head, os_list_pos_e pos, os_err_e* err){

	/* Check for argument errors
	 ------------------------------------------------------*/
	if(head == NULL) {
		if(err != NULL) *err = OS_ERR_BAD_ARG;
		return NULL;
	}

	/* Return if not
	 ------------------------------------------------------*/
	if(head->last == NULL || head->first == NULL) {
		if(err != NULL) *err = OS_ERR_INVALID;
		return NULL;
	}

	/* Enter Critical Section
	 ------------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL(); //If it's searching / inserting a block, it can be interrupted and another task can change the list. In this case, the first task will blow up when returning

	/* Point to cell and content
	 ------------------------------------------------------*/
	void* del = (pos == OS_LIST_FIRST) ? head->first : head->last;
	void* ret = (pos == OS_LIST_FIRST) ? head->first->element : head->last->element;

	/* Kill first element
	 ------------------------------------------------------*/
	if(pos == OS_LIST_FIRST){
		head->first->prev->next = head->first->next;
		if(head->first->next != NULL){
			head->first->next->prev = head->first->prev;
		}
		head->first = head->first->next;
	}


	/* Kill last element
	 ------------------------------------------------------*/
	if(pos == OS_LIST_LAST){
		head->last->prev->next = NULL;
		if(head->last->prev == &head->head){
			head->last = NULL;
		}
		else{
			head->last = head->last->prev;
		}
	}

	/* Free heap
	 ------------------------------------------------------*/
	os_err_e error = os_heap_free(del);
	if(err != NULL) *err = error;

	/* Reduce size and return
	 ------------------------------------------------------*/
	head->listSize--;
	OS_EXIT_CRITICAL();

	return ret;
}


/***********************************************************************
 * OS List clear
 *
 * @brief This function clears a list, freeing all its cells and head
 *
 * @param os_list_head_t* head : [in] reference to the head of the list
 *
 **********************************************************************/
void os_list_clear(os_list_head_t* head){

	/* Check errors
	 ---------------------------------------------------*/
	if(head == NULL) return;

	/* Enter critical to avoid list changing
	 ---------------------------------------------------*/
	OS_CRITICAL_SECTION(

		/* Loop until list is empty
		 ---------------------------------------------------*/
		os_list_cell_t* it = &head->head;
		while(it != NULL){

			/* Store address
			 ---------------------------------------------------*/
			os_list_cell_t* delete = it;

			/* Flag task as ready and go to next
			 ---------------------------------------------------*/
			it = it->next;

			/* Free allocation
			 ---------------------------------------------------*/
			os_heap_free(delete);
		}

		/* Free head
		 ---------------------------------------------------*/
		os_heap_free(head);
	);
}


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
void os_task_list_sort(os_list_head_t* head){

	/* Check errors
	 ---------------------------------------------------*/
	if(head == NULL) return;

	/* Enter critical
	 ---------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Update priority for all tasks
	 ---------------------------------------------------*/
	os_list_cell_t* p = head->head.next;
	while(p != NULL){
		((os_task_t*)p->element)->priority = os_task_getPrio(p->element);
		p = p->next;
	}

	/* Begin sort
	 ---------------------------------------------------*/
	os_list_cell_t* it_slow = &head->head;
	while(it_slow->next != NULL && it_slow->next->next != NULL){

		/* For each cell
		 ---------------------------------------------------*/
		bool changeMade = 0;
		os_list_cell_t* it_fast = &head->head;
		while(it_fast->next != NULL && it_fast->next->next != NULL){

			/* Decide to swap or not
			 ---------------------------------------------------*/
			bool swap  = ((os_task_t*)it_fast->next->element)->priority == ((os_task_t*)it_fast->next->next->element)->priority;
				 swap &= it_fast->next->order > it_fast->next->next->order;
				 swap |= ((os_task_t*)it_fast->next->element)->priority < ((os_task_t*)it_fast->next->next->element)->priority;

			/* Swap cells
			 ---------------------------------------------------*/
			if(swap){

				/* Store references
				 ---------------------------------------------------*/
				os_list_cell_t* pN1 = it_fast->next;
				os_list_cell_t* pN2 = it_fast->next->next;

				/* Swap
				 ---------------------------------------------------*/
				it_fast->next = pN2;
				pN1->next = pN2->next;
				pN2->next = pN1;

                pN2->prev = pN1->prev;
                pN1->prev = pN2;

				changeMade = 1;

			}

			it_fast = it_fast->next;

		}

		/* If no changement was made, we finished
		 ---------------------------------------------------*/
		if(changeMade == 0) break;

		it_slow = it_slow->next;
	}

	OS_EXIT_CRITICAL();
}


/***********************************************************************
 * OS Handle list get object to update
 *
 * @brief This function searches for an object in the object list that needs to update. Must be used in a handle list
 *
 * @return os_handle_t handle to the object to update or NULL if nothing to do
 **********************************************************************/
os_handle_t os_handle_list_getObjToUpdate(){

	/* Search object list
	 ---------------------------------------------------*/
	for(os_list_cell_t* it = os_obj_head.head.next; it != NULL; it = it->next){
		if( ((os_handle_t)it->element)->objUpdate == 1)
			return it->element;
	}

	return NULL;
}


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
bool os_task_list_isObjFreeOnTask(os_handle_t obj, os_handle_t task){

	/* Enter critical to avoid list changing
	 ---------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Get current free count and return if topic
	 ---------------------------------------------------*/
	uint32_t freeCount = obj->getFreeCount(obj, task);
	if(obj->type == OS_OBJ_TOPIC){
        OS_EXIT_CRITICAL();
		return freeCount > 0;
    }

	/* If it is 0, return 0 immediately
	 ---------------------------------------------------*/
	if(freeCount == 0) {
		OS_EXIT_CRITICAL();
		return 0;
	}

	/* If it can be taken more times than the number of tasks waiting for it return 1
	 ---------------------------------------------------*/
	if(freeCount >= ((os_list_head_t*)obj->blockList)->listSize || freeCount == 0xFFFFFFFF) {
		OS_EXIT_CRITICAL();
		return 1;
	}

	/* Otherwise, sort and start scan
	 ---------------------------------------------------*/
	os_task_list_sort(obj->blockList);

	/* Start scan by pointing to the first waiting task
	 ---------------------------------------------------*/
	for(os_list_cell_t* it = ((os_list_head_t*)obj->blockList)->head.next; it != NULL; it = it->next){

		/* Convert address
		 ---------------------------------------------------*/
		os_task_t* t = (os_task_t*) it->element;

		/* Return if the object has no more free counters
		 ---------------------------------------------------*/
		if(freeCount == 0) {
			OS_EXIT_CRITICAL();
			return 0;
		}

		/* Stop scanning if we are pointing at the task we are searching
		 ---------------------------------------------------*/
		if(it->element == task) break;

		/* Do not consider deleting or ended tasks
		 ---------------------------------------------------*/
		if(t->state == OS_TASK_DELETING || t->state == OS_TASK_ENDED) continue;

		/* If the task is waiting one object
		 ---------------------------------------------------*/
		if(t->waitFlag == OS_OBJ_WAIT_ONE){

			/* Search for the position on the array of the object
			 ---------------------------------------------------*/
			size_t i = 0;
			for(i = 0; i < t->sizeObjs; i++){
				if(t->objWaited[i] == obj) break;
			}

			/* If the position of the object is smaller than the current object the task wants, it will switch
			 ---------------------------------------------------*/
			freeCount = i < t->objWanted ? freeCount - 1 : freeCount;
		}

		/* If the task is waiting all objects
		 ---------------------------------------------------*/
		if(t->waitFlag == OS_OBJ_WAIT_ALL){

			/* Scan all objects to know if all of them are available
			 ---------------------------------------------------*/
			bool getObjs = 1;
			for(size_t i = 0; i < t->sizeObjs; i++){

				/* Skip current object
				 ---------------------------------------------------*/
				if(t->objWaited[i] == obj) continue;

				/* If one of the objects is not free on the task, we must wait
				 ---------------------------------------------------*/
				if(os_task_list_isObjFreeOnTask(t->objWaited[i], (os_handle_t) t) == 0){
					getObjs = 0;
					break;
				}
			}

			/* Decrement freeCount if all objects are free
			 ---------------------------------------------------*/
			freeCount = getObjs ? freeCount - 1 : freeCount;
		}
	}

	/* Return if the object is free or not
	 ---------------------------------------------------*/
	OS_EXIT_CRITICAL();
	return freeCount > 0 ? 1 : 0;
}


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
bool os_handle_list_updateAndCheck(os_handle_t h){

	/* Enter critical
	 ---------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Declares auxiliary variables and starts the update
	 ---------------------------------------------------*/
	int8_t maxPrio = -1;
	while(h != NULL){

		/* Sort List
		 ---------------------------------------------------*/
		os_task_list_sort(h->blockList);

		/* Get the number of times we can get the object
		 ---------------------------------------------------*/
		uint32_t freeCount = h->type == OS_OBJ_TOPIC ? 0 : h->getFreeCount(h, NULL);

		/* Updates every task on the block list
		 ---------------------------------------------------*/
		for(os_list_cell_t* it = ((os_list_head_t*)h->blockList)->head.next; it != NULL; it = it->next){

			/* Ignore deleting and ended tasks
			 ---------------------------------------------------*/
			os_task_t* t = (os_task_t*)it->element;
			if(t->state == OS_TASK_DELETING || t->state == OS_TASK_ENDED) continue;

			if(h->type == OS_OBJ_TOPIC){
				freeCount = h->getFreeCount(h, (os_handle_t) t);
			}

			/* If the task is only waiting one object
			 ---------------------------------------------------*/
			if(t->waitFlag == OS_OBJ_WAIT_ONE){

				/* Search the position of the object in the task array
				 ---------------------------------------------------*/
				size_t i = 0;
				for(i = 0; i < t->sizeObjs; i++){
					if(t->objWaited[i] == h) break;
				}

				/* If the object can still be taken
				 ---------------------------------------------------*/
				if(freeCount != 0){

					/* Only do something if the current object has a smaller index than the one the task is waiting for
					 ---------------------------------------------------*/
					if(i < t->objWanted){

						/* Since the task will switch to the smaller index object, we must update the old object
						 * This update can possibly wake a task
						 ---------------------------------------------------*/
						if(t->objWanted < t->sizeObjs) t->objWaited[t->objWanted]->objUpdate = 1;

						/* Store the object's index and tag task as ready
						 ---------------------------------------------------*/
						t->objWanted = i;
						t->state = OS_TASK_READY;

						/* Decrement freecount if needed
						 ---------------------------------------------------*/
						freeCount = freeCount != OS_OBJ_COUNT_INF && freeCount > 0 ? freeCount - 1 : freeCount;
					}
				}

				/* If the object cannot be taken
				 ---------------------------------------------------*/
				else{

					/* Only do something if the current waited object is no longer available
					 ---------------------------------------------------*/
					if(t->objWanted == i){

						/* Search for an available object in higher indexes
						 ---------------------------------------------------*/
						for(i = t->objWanted + 1; i < t->sizeObjs; i++)
							if(os_task_list_isObjFreeOnTask(t->objWaited[i], (os_handle_t) t) != 0) break;


						/* Tag task as ready if there is an available object, or blocked if not timeout
						 ---------------------------------------------------*/
						t->state = i < t->sizeObjs ? OS_TASK_READY : OS_TASK_BLOCKED;
						t->state = t->wakeCoutdown == 0 ? OS_TASK_READY : t->state;
						t->objWanted = i < t->sizeObjs ? i : 0xFFFFFFFF;

						/* If the task switched to a higher index object, update it
						 ---------------------------------------------------*/
						if(t->objWanted < t->sizeObjs) t->objWaited[t->objWanted]->objUpdate = 1;
					}
				}
			}

			/* If the task is waiting all objects
			 ---------------------------------------------------*/
			if(t->waitFlag == OS_OBJ_WAIT_ALL){

				/* Store objWanted to use it later
				 ---------------------------------------------------*/
				size_t last_objWanted = t->objWanted;

				/* If the current object is available
				 ---------------------------------------------------*/
				if(freeCount != 0){

					/* Check if all other objects are available
					 ---------------------------------------------------*/
					bool getObjs = 1;
					for(size_t i = 0; i < t->sizeObjs; i++){

						/* Skip current object
						 ---------------------------------------------------*/
						if(t->objWaited[i] == h) continue;

						/* If one of the objects is not free on the task, we must wait
						 ---------------------------------------------------*/
						if(os_task_list_isObjFreeOnTask(t->objWaited[i], (os_handle_t) t) == 0){
							getObjs = 0;
							break;
						}
					}

					/* Update task infos according to the result
					 ---------------------------------------------------*/
					t->objWanted = getObjs ? 0 : 0xFFFFFFFF;
					t->state = getObjs ? OS_TASK_READY : OS_TASK_BLOCKED;
					t->state = t->wakeCoutdown == 0 ? OS_TASK_READY : t->state;

					freeCount = getObjs && freeCount < OS_OBJ_COUNT_INF ? freeCount - 1 : freeCount;
				}

				/* If the object is not available
				 ---------------------------------------------------*/
				else{

					/* Just update task infos
					 ---------------------------------------------------*/
					t->objWanted = 0xFFFFFFFF;
					t->state = t->wakeCoutdown == 0 ? OS_TASK_READY : OS_TASK_BLOCKED;
				}

				/* Detects that the objWanted changed
				 ---------------------------------------------------*/
				if(last_objWanted != t->objWanted){

					/* If the task was waiting and now wants to get them all (all of them are available)
					 * or if the task wanted to get them all and now is waiting (one of them is no longer available)
					 * we must tag the other objects to update as well
					 ---------------------------------------------------*/
					for(size_t i = 0; i < t->sizeObjs; i++){

						/* Skip current object
						 ---------------------------------------------------*/
						if(t->objWaited[i] == h) continue;

						/* Tag all objects to update
						 ---------------------------------------------------*/
						t->objWaited[i]->objUpdate = 1;
					}
				}
			}

			/* If the task is ready, get its priority to check if we should yeild
			 ---------------------------------------------------*/
			maxPrio = t->state == OS_TASK_READY && maxPrio < t->priority ? t->priority : maxPrio;
		}

		/* End of while. All tasks have been updated, so we remove the update flag
		 ---------------------------------------------------*/
		h->objUpdate = 0;

		/* Search for another object in the object list that needs to update.
		 * This logic is important for 2 reasons
		 *
		 * 1 - avoids extra recursive calls
		 * 2 - makes sure that an object is updated ultil the end before switching to another one
		 ---------------------------------------------------*/
		h = os_handle_list_getObjToUpdate();
	}

	/* Calculate if we must yield or not
	 ---------------------------------------------------*/
	bool mustYield = maxPrio > 0 ? maxPrio > os_task_getPrio(os_cur_task->element) : 0;

	OS_EXIT_CRITICAL();
	return mustYield;
}


//////////////////////////////////////////////// ELF LOADER //////////////////////////////////////////////////


/***********************************************************************
 * OS ELF check header
 *
 * @brief This function checks that an elf file has the correct header information
 *
 * @param os_elf_header_t* header 	: [out] header information
 * @param lfs_file_t* lfs_file		: [ in] File pointer to the elf file
 *
 * @return os_err_e : <0 if error
 **********************************************************************/
static os_err_e os_elf_checkHeader(os_elf_header_t* header, lfs_file_t* lfs_file){

	/* Rewind file to the beginning
	 ------------------------------------------------------*/
	int err = lfs_file_seek(&lfs, lfs_file, 0, LFS_SEEK_SET);
	if(err < 0){
		return OS_ERR_FS;
	}

	/* Read the header
	 ------------------------------------------------------*/
	err = lfs_file_read(&lfs, lfs_file, header, sizeof(*header));
	if(err < 0){
		return OS_ERR_FS;
	}

	/* Check magic number
	 ------------------------------------------------------*/
	if(header->e_ident.magic[0] != 0x7F || header->e_ident.magic[1] != 'E' || header->e_ident.magic[2] != 'L' || header->e_ident.magic[3] != 'F'){
		return OS_ERR_INVALID;
	}

	/* Check endianness, bit depth, and version
	 ------------------------------------------------------*/
	if(header->e_ident.class != 1 || header->e_ident.data != 1 || header->e_ident.version != 1){
		return OS_ERR_INVALID;
	}

	/* Check version and that its made for ARM
	 ------------------------------------------------------*/
	if(header->e_machine != 40 || header->e_version != 1){
		return OS_ERR_INVALID;
	}

	/* Return OK
	 ------------------------------------------------------*/
	return OS_ERR_OK;
}

/***********************************************************************
 * OS ELF load segments
 *
 * @brief This function loads all segments into RAM
 *
 * @param os_elf_header_t* header 	: [ in] header information
 * @param lfs_file_t* lfs_file		: [ in] File pointer to the elf file
 * @param os_elf_mapping_el_t map[]	: [out] Array containing the old and new addresses of all segments
 *
 * @return os_err_e : <0 if error
 **********************************************************************/
static int os_elf_loadSegments(os_elf_header_t* header, lfs_file_t* lfs_file, os_elf_mapping_el_t map[]){

	/* First, calculate how much RAM we need
	 ------------------------------------------------------*/
	uint32_t memToAlloc = 0;

	/* For each segment
	 ------------------------------------------------------*/
	for(uint32_t i = 0; i < header->e_phnum; i++){

		/* read program header data
		 ------------------------------------------------------*/
		os_elf_programHeader_t data;
		int err  = lfs_file_seek(&lfs, lfs_file, (lfs_soff_t)(header->e_phoff + i * header->e_phentsize), LFS_SEEK_SET); //Seek to the program header position
		    err |= lfs_file_read(&lfs, lfs_file, &data, sizeof(data));
		if(err < 0){
			return OS_ERR_FS;
		}

		/* Check that it is a LOAD segment
		 ------------------------------------------------------*/
		if(data.p_type != 1)
			continue;

		/* align segment block as 8 byte
		 ------------------------------------------------------*/
		memToAlloc += (data.p_memsz + 7) & (~0x7UL);
	}

	/* Allocate all segments to make the free easier
	 ------------------------------------------------------*/
	uint8_t* segment = (uint8_t*) os_heap_alloc(memToAlloc);
	if(segment == NULL)
		return OS_ERR_INSUFFICIENT_HEAP;

	/* Initialize segments to 0 and Load into memory
	 ------------------------------------------------------*/
	size_t pos = 0;
	memset(segment, 0, memToAlloc);

	/* For each segment
	 ------------------------------------------------------*/
	for(uint32_t i = 0; i < header->e_phnum; i++){

		/* Read program header data
		 ------------------------------------------------------*/
		os_elf_programHeader_t data;
		int err  = lfs_file_seek(&lfs, lfs_file, (lfs_soff_t)(header->e_phoff + i * header->e_phentsize), LFS_SEEK_SET); //Seek to the program header position
		 	err |= lfs_file_read(&lfs, lfs_file, &data, sizeof(data));
		if(err < 0){
			os_heap_free(segment);
			return OS_ERR_FS;
		}

		/* Check it is LOAD segment
		 ------------------------------------------------------*/
		if(data.p_type != 1)
			continue;

		/* Read the entire segment into the heap
		 ------------------------------------------------------*/
		err  = lfs_file_seek(&lfs, lfs_file, (lfs_soff_t)data.p_offset, LFS_SEEK_SET); //Seek to the actual data
		err |= lfs_file_read(&lfs, lfs_file, &segment[pos], data.p_filesz);
		if(err < 0){
			os_heap_free(segment);
			return OS_ERR_FS;
		}

		/* Calculate segment size
		 ------------------------------------------------------*/
		size_t segmentSize = (data.p_memsz + 7) & (~0x7UL);

		/* Store the remapping into output
		 ------------------------------------------------------*/
		map[i].original_addr = data.p_vaddr;
		map[i].remapped_addr = (uint32_t)&segment[pos];
		map[i].size = segmentSize;

		/* increment buffer position
		 ------------------------------------------------------*/
		pos += segmentSize;
	}

	return OS_ERR_OK;
}


/***********************************************************************
 * OS ELF  Memory recalculate
 *
 * @brief This function recalculates an address using the remapping structure
 *
 * @param uint32_t originalAddr 	: [ in] The address to recalculate
 * @param os_elf_mapping_el_t map[]	: [ in] Array containing the old and new addresses of all segments
 * @param size_t map_size			: [ in] Size of the map array
 *
 * @return uint32_t : the new address or 0 if a problem occured
 **********************************************************************/
static uint32_t os_elf_memoryRecalc(uint32_t originalAddr, os_elf_mapping_el_t map[], size_t map_size){

	/* For each segment
	 ------------------------------------------------------*/
	for(int i = 0; i < map_size; i++){

		/* Gets the beginning and end addresses, and the remapped address
		 ------------------------------------------------------*/
		uint32_t sAddr = map[i].original_addr;
		uint32_t eAddr = map[i].original_addr + map[i].size;
		uint32_t vAddr = map[i].remapped_addr;

		/* If our original address lies in this segment, we can calculate its new address
		 ------------------------------------------------------*/
		if( sAddr <= originalAddr && originalAddr < eAddr ){
			return originalAddr - sAddr + vAddr;
		}
	}

	return 0;
}


/***********************************************************************
 * OS ELF Adjust GOT
 *
 * @brief This function adjusts the Global Offset Table of the program. When compiled with Position Independent Code (-fPIC), the code gets all globals using the GOT
 * This GOT must be corrected to the actual address we are loading
 *
 * @param os_elf_header_t* header 	: [ in] header information
 * @param lfs_file_t* lfs_file		: [ in] File pointer to the elf file
 * @param os_elf_mapping_el_t map[]	: [ in] Array containing the old and new addresses of all segments
 *
 * @return os_elf_prog_t : information needed to run an executable
 **********************************************************************/
static os_elf_prog_t os_elf_adjustGot(os_elf_header_t* header, lfs_file_t* lfs_file, os_elf_mapping_el_t map[]){

	/* Declare empty return
	 ------------------------------------------------------*/
	os_elf_prog_t ret = {};
	os_elf_prog_t NullRet = {};

	/* Load section header that contains the names of the sections
	 ------------------------------------------------------*/
	os_elf_sectionHeader_t names;
	int err  = lfs_file_seek(&lfs, lfs_file, (lfs_soff_t)(header->e_shoff + (uint32_t)(header->e_shstrndx * header->e_shentsize) ), LFS_SEEK_SET); //Seek to the index of the section header that contains all names
		err |= lfs_file_read(&lfs, lfs_file, &names, sizeof(names));
	if(err < 0){
		return NullRet;
	}

	/* For each section
	 ------------------------------------------------------*/
	bool gotFound = 0;
	for(uint32_t i = 0; i < header->e_shnum; i++){

		/* read section header information
		 ------------------------------------------------------*/
		os_elf_sectionHeader_t data;
		int err  = lfs_file_seek(&lfs, lfs_file, (lfs_soff_t)(header->e_shoff + i * header->e_shentsize), LFS_SEEK_SET); //Seek to the section header
			err |= lfs_file_read(&lfs, lfs_file, &data, sizeof(data));
		if(err < 0){
			return NullRet;
		}

		/* Get the name of the current section
		 ------------------------------------------------------*/
		char sect_name[50];
		err  = lfs_file_seek(&lfs, lfs_file, (lfs_soff_t)(data.sh_name + names.sh_offset), LFS_SEEK_SET); //Seek to the string position in the file
		err |= lfs_file_read(&lfs, lfs_file, sect_name, sizeof(sect_name));
		if(err < 0){
			return NullRet;
		}

		/* We are only intrested in the GOT section
		 ------------------------------------------------------*/
		if(strcmp(".got", sect_name) != 0)
			continue;

		/* With Got section found, now we need to find where it lies in our heap
		 ------------------------------------------------------*/
		gotFound = 1;
		ret.gotBase = os_elf_memoryRecalc(data.sh_addr, map, header->e_phnum);
		if(ret.gotBase == 0){
			return NullRet;
		}

		/* Correct each GOT entry
		 ------------------------------------------------------*/
		uint32_t* pGot = (uint32_t*)ret.gotBase; //Convert to pointer
		for(int j = 0; j < data.sh_size; j+=4){ //Move in increments of 4 bytes
			pGot[j/4] = os_elf_memoryRecalc(pGot[j/4], map, header->e_phnum); //Recalculate address
		}

		break;
	}

	/* Finally, calculate the entry point
	 ------------------------------------------------------*/
	ret.entryPoint = (void*)os_elf_memoryRecalc(header->e_entry, map, header->e_phnum);
	return gotFound == 1 ? ret : NullRet;
}


/***********************************************************************
 * OS ELF load file
 *
 * @brief This function loads an ELF file into memory
 *
 * @param char* name : [ in] File name
 *
 * @return os_elf_prog_t : Necessary information to run a program
 **********************************************************************/
os_elf_prog_t os_elf_loadFile(char* file){

	/* Open file
	 --------------------------------------------------*/
	os_elf_prog_t nullRet = {};
	lfs_file_t lfs_file;
	int err = lfs_file_open(&lfs, &lfs_file, file, LFS_O_RDONLY);
	if(err < 0){
		PRINTLN("Open Error");
		return nullRet;
	}

	/* Check header information
	 --------------------------------------------------*/
	os_elf_header_t header;
	os_elf_prog_t prg = {};
	if(os_elf_checkHeader(&header, &lfs_file) >= 0) {

		/* Load all segments into memory
		 --------------------------------------------------*/
		os_elf_mapping_el_t map[header.e_phnum];
		if(os_elf_loadSegments(&header, &lfs_file, map) >= 0){

			/* Finally, ajdust the GOT
			 --------------------------------------------------*/
			prg = os_elf_adjustGot(&header, &lfs_file, map);

			/* If error, free the first position of the remap. This will free the block allocated in loadSegments
			 --------------------------------------------------*/
			if(prg.entryPoint == NULL || prg.gotBase == 0){
				os_heap_free((void*)map[0].remapped_addr);
			}
		}

	}

	/* Close file
	 --------------------------------------------------*/
	err = lfs_file_close(&lfs, &lfs_file);
	if(err < 0){
		PRINTLN("close Error");
		return nullRet;
	}

	return prg;
}
