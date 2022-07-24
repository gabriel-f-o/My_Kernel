/*
 * OS_Tasks.c
 *
 *  Created on: Jun 1, 2021
 *      Author: Gabriel
 */

#include "common.h"
#include "OS/OS_Core/OS.h"
#include "OS/OS_Core/OS_Internal.h"
#include "OS/OS_FS/lfs.h"

/**********************************************
 * EXTERNAL VARIABLES
 *********************************************/

extern os_list_head_t os_obj_head;	//Head to obj list

/**********************************************
 * PUBLIC VARIABLES
 *********************************************/

os_handle_t main_task;	//Main task handle
os_handle_t idle_task;	//Idle task handle

/**********************************************
 * OS PRIVATE VARIABLES
 *********************************************/

os_list_head_t os_head;				//Head of task list
os_list_cell_t* os_cur_task = NULL;	//Current task pointer

/**********************************************
 * PRIVATE FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Init Stack
 *
 * @brief This function initializes the stack, change the used stack (now psp, before msp) and set MSP to match
 * the interrupt stack location in heap
 *
 * @param uint32_t interruptStackSize : [in] size of the interrupt stack
 *
 * @return os_err_e : error code (0 = OK)
 *
 **********************************************************************/
static os_err_e os_task_init_stack(uint32_t interruptStackSize){

	/* Check errors
	 ------------------------------------------------------*/
	if(interruptStackSize < OS_MINIMUM_STACK_SIZE) return OS_ERR_BAD_ARG;

	/* Allocate the stack
	 ------------------------------------------------------*/
	uint32_t stk = (uint32_t) os_heap_alloc(interruptStackSize);

	/* Check if allocation was OK
	 ------------------------------------------------------*/
	if(stk == 0) return OS_ERR_INSUFFICIENT_HEAP;

	/* Save context and make PSP = MSP
	 ------------------------------------------------------*/
	__asm volatile ("push {r6, lr}"); 		//Save R6 and LR
	__asm volatile ("mrs r6, msp"); 		//R6 = MSP
	__asm volatile ("msr psp, r6"); 		//PSP = R6

	/* Position MSP to interrput stack
	 ------------------------------------------------------*/
	uint32_t volatile mspReg = (uint32_t) ( (stk + interruptStackSize) & (~0x7UL) ); //logic and to guarantee that we are word aligned
	__asm volatile ("mov r6, %[in]" : : [in] "r" (mspReg)); //R6 = mspReg
	__asm volatile ("msr msp, r6"); //MSP = R6

	/* Select PSP as current stack pointer
	 ------------------------------------------------------*/
	__asm volatile ("mrs r6, control");		//R6 = CTRL
	__asm volatile ("orr r6, r6, #0x2");	//R6 |= 0x2
	__asm volatile ("msr control, r6");		//CTRL = R6

	/* Recover stack
	 ------------------------------------------------------*/
	__asm volatile ("pop {r6, lr}");

	return OS_ERR_OK;
}


/***********************************************************************
 * OS Task get free count
 *
 * @brief This function gets the amount of times this object can be "taken" before it is considered unavailable
 * In the case of tasks, if the task is finished, the user can take it an infinite amount of times, otherwise it is unavailable
 *
 * @param os_handle_t h : [in] object to verify the availability
 *
 * @return uint32_t : the amount of times the object can be taken
 *
 **********************************************************************/
static uint32_t os_task_getFreeCount(os_handle_t h){

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return 0;
	if(h->type != OS_OBJ_TASK) return 0;

	/* Check if task ended
	 ------------------------------------------------------*/
	os_task_state_e state = os_task_getState(h);

	/* Determines if task ended
	 ------------------------------------------------------*/
	bool task_ended  = (state == OS_TASK_NOT_EXIST);
		 task_ended |= (state == OS_TASK_ENDED);
		 task_ended |= (state == OS_TASK_DELETING);

	return task_ended ? OS_OBJ_COUNT_INF : 0;
}


/***********************************************************************
 * OS Task take
 *
 * @brief This function takes possession of the object. In the case of a task, it has no effect.
 *
 * @param os_handle_t h 			: [in] object to take
 * @param os_handle_t takingTask	: [in] handle to the task that is taking the object
 *
 * @return os_err_e : error code (0 = OK)
 *
 **********************************************************************/
static void os_task_objTake(os_handle_t h, os_handle_t takingTask){
	UNUSED_ARG(h);
	UNUSED_ARG(takingTask);
}


/**********************************************
 * OS PRIVATE FUNCTIONS
 *********************************************/


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
os_err_e os_task_init(char* main_name, int8_t main_task_priority, uint32_t interrput_stack_size, uint32_t idle_stack_size){

	/* Check errors
	 ------------------------------------------------------*/
	if(main_task_priority < 0) return OS_ERR_BAD_ARG;
	if(interrput_stack_size < OS_MINIMUM_STACK_SIZE) return OS_ERR_BAD_ARG;
	if(idle_stack_size < OS_MINIMUM_STACK_SIZE) return OS_ERR_BAD_ARG;

	/* Init stack by changing the register used (MSP to PSP) and allocate interrupt stack
	 ------------------------------------------------------*/
	os_err_e ret = os_task_init_stack(interrput_stack_size);
	if(ret != OS_ERR_OK) return ret;

	/* Allocate task block
	 ------------------------------------------------------*/
	os_task_t* t = (os_task_t*)os_heap_alloc(sizeof(os_task_t));

	/* Check allocation
	 ------------------------------------------------------*/
	if(t == 0) return OS_ERR_INSUFFICIENT_HEAP;

	/* Init main task
	 ------------------------------------------------------*/
	t->obj.objUpdate		= 0;
	t->obj.type				= OS_OBJ_TASK;
	t->obj.getFreeCount		= &os_task_getFreeCount;
	t->obj.blockList		= os_list_init();
	t->obj.obj_take			= &os_task_objTake;
	t->obj.name 			= main_name;

	t->basePriority 		= main_task_priority;
	t->priority		    	= main_task_priority;
	t->pid					= 0;
	t->state	 			= OS_TASK_READY;
	t->pStack   			= NULL;
	t->wakeCoutdown  		= 0;
	t->stackBase	    	= 0;
	t->objWaited			= NULL;
	t->sizeObjs 			= 0;
	t->retVal				= NULL;

	t->ownedMutex			= os_list_init();

	/* Init head list and Add main task
	 ------------------------------------------------------*/
	ret = os_list_add(&os_head, (os_handle_t) t, OS_LIST_FIRST);
	if(ret != OS_ERR_OK) return ret;

	/* Add object to object list
	 ------------------------------------------------------*/
	ret = os_list_add(&os_obj_head, (os_handle_t) t, OS_LIST_FIRST);
	if(ret != OS_ERR_OK) return ret;

	/* Point to current task
	 ------------------------------------------------------*/
	os_cur_task = os_head.head.next;

	/* Link handle with task
	 ------------------------------------------------------*/
	main_task = (os_handle_t) t;

	/* Init idle task
	 ------------------------------------------------------*/
	return ret;
}


/***********************************************************************
 * OS Task Must Yeild
 *
 * @brief This function checks the priority of all tasks and decides if the current task should yield
 *
 * @return bool : 1 = yeild
 *
 **********************************************************************/
bool os_task_must_yeild(){

	/* Enter critical
	 ------------------------------------------------------*/
	OS_CRITICAL_SECTION(

		/* Check if there is a task with higher priority
		 ------------------------------------------------------*/
		os_list_cell_t* it = os_head.head.next;
		int8_t cur_prio = os_cur_task == NULL ? -1 : os_task_getPrio(os_cur_task->element);

		/* Search the high side of the list
		 ------------------------------------------------------*/
		while(it != NULL){

			/* Calculate task priority based on the mutex it owns
			 ------------------------------------------------------*/
			int8_t task_prio = os_task_getPrio(it->element);

			/* If the task is ready and its priority is higher, yeild
			 ------------------------------------------------------*/
			if( ((os_task_t*)it->element)->state == OS_TASK_READY && cur_prio < task_prio ) return true;

			/* Otherwise continue
			 ------------------------------------------------------*/
			it = it->next;
		}
	);

	return false;
}


/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Task Create
 *
 * @brief This function creates a new task, that will be called by the scheduler when the correct time comes
 *
 * @param os_handle_t* h		: [out] handle to object
 * @param char* name 			: [ in] name of the task
 * @param void* (*fn)(void*) 	: [ in] task's main function to be called
 * @param int8_t priority		: [ in] A priority to the task (0 is lowest priority) cannot be negative
 * @param uint32_t stack_size 	: [ in] The amount of stack to be reserved. A minimum of 128 bytes is required
 * @param void* arg				: [ in] Argument to be passed to the task
 *
 * @return os_err_e : An error code (0 = OK)
 *
 **********************************************************************/
os_err_e os_task_create(os_handle_t* h, char const * name, void* (*fn)(void* i), int8_t priority, uint32_t stack_size, void* arg){

	/* Check for argument errors
	 ------------------------------------------------------*/
	if(h == NULL) 							return OS_ERR_BAD_ARG;
	if(fn == NULL) 							return OS_ERR_BAD_ARG;
	if(priority < 0) 						return OS_ERR_BAD_ARG;
	if(stack_size < OS_MINIMUM_STACK_SIZE)  return OS_ERR_BAD_ARG;
	if(os_init_get() == false)				return OS_ERR_NOT_READY;

	/* Alloc the task block
	 ------------------------------------------------------*/
	os_task_t* t = (os_task_t*)os_heap_alloc(sizeof(os_task_t));

	/* Check allocation
	 ------------------------------------------------------*/
	if(t == 0) return OS_ERR_INSUFFICIENT_HEAP;

	/* Alloc the stack
	 ------------------------------------------------------*/
	uint32_t stk = (uint32_t) os_heap_alloc(stack_size);

	/* Check if allocation was OK
	 ------------------------------------------------------*/
	if(stk == 0) return OS_ERR_INSUFFICIENT_HEAP;

	/* Create a unique PID
	 ------------------------------------------------------*/
	uint16_t pid = 0;
	uint32_t attempts = 0;
	while(1){

		/* Generate PID using the tick
		 ------------------------------------------------------*/
		uint32_t ms = os_getMsTick() + attempts;
		pid = (uint16_t)( (ms & 0xFF) ^ ((ms >> 16) & 0xFF) );

		/* Check if PID exists
		 ------------------------------------------------------*/
		if(os_task_getByPID(pid) == NULL){
			break;
		}

		attempts++;
	}

	/* Init Task
	 ------------------------------------------------------*/
	t->obj.objUpdate	= 0;
	t->obj.type			= OS_OBJ_TASK;
	t->obj.getFreeCount	= &os_task_getFreeCount;
	t->obj.blockList	= os_list_init();
	t->obj.obj_take		= &os_task_objTake;
	t->obj.name			= (char*) name;

	t->fnPtr			= fn;
	t->basePriority		= priority;
	t->priority		    = priority;
	t->pid				= pid;
	t->state			= OS_TASK_READY;
	t->wakeCoutdown	 	= 0;
	t->stackBase		= (stk + stack_size);
	t->stackSize 		= stack_size;
	t->pStack			= (uint32_t*) ( t->stackBase & (~0x7UL) );
	t->objWaited		= NULL;
	t->sizeObjs 		= 0;
	t->retVal			= NULL;

	t->ownedMutex		= os_list_init();

	/* Init Task Stack
	 ------------------------------------------------------*/
	*--t->pStack = (uint32_t) 0x01000000;	 	//xPSR (bit 24 must be 1 otherwise BOOM)
	*--t->pStack = (uint32_t) fn;				//Return
	*--t->pStack = (uint32_t) &os_task_return;  //LR
	*--t->pStack = (uint32_t) 0;				//R12
	*--t->pStack = (uint32_t) 0;			 	//R3
	*--t->pStack = (uint32_t) 0;			 	//R2
	*--t->pStack = (uint32_t) 0;			 	//R1
	*--t->pStack = (uint32_t) arg;			 	//R0 (argument)

	*--t->pStack = (uint32_t) 0xFFFFFFFD;    	//LR (when called by the interrupt, flag as basic frame used always)
	*--t->pStack = (uint32_t) 0;			 	//R11
	*--t->pStack = (uint32_t) 0;			 	//R10
	*--t->pStack = (uint32_t) 0; 			 	//R9
	*--t->pStack = (uint32_t) 0;			 	//R8
	*--t->pStack = (uint32_t) 0;				//R7
	*--t->pStack = (uint32_t) 0;				//R6
	*--t->pStack = (uint32_t) 0;				//R5
	*--t->pStack = (uint32_t) 0;				//R4

	/* Add task to list
	 ------------------------------------------------------*/
	os_err_e err = os_list_add(&os_head, (os_handle_t)t, OS_LIST_FIRST);
	if(err != OS_ERR_OK) return err;

	/* Add object to object list
	 ------------------------------------------------------*/
	os_err_e ret = os_list_add(&os_obj_head, (os_handle_t) t, OS_LIST_FIRST);
	if(ret != OS_ERR_OK) return ret;

	/* Calculate task priority
	 ------------------------------------------------------*/
	int8_t task_prio = os_task_getPrio((os_handle_t) t);
	int8_t cur_prio = ( (os_cur_task == NULL) ? -1 : os_task_getPrio(os_cur_task->element) );

	/* If created task was a higher priority, and scheduler is running, yeild
	 ---------------------------------------------------*/
	if(task_prio > cur_prio && os_scheduler_state_get() == OS_SCHEDULER_START) os_task_yeild();

	/* link handle with task object
	 ---------------------------------------------------*/
	*h = ( (err == OS_ERR_OK) ? (os_handle_t) t : NULL );

	return err;
}


typedef struct{
	uint8_t magic[4];	//Always 0x7F, 'E', 'L', 'F'
	uint8_t class;		//This byte is set to either 1 or 2 to signify 32- or 64-bit format, respectively.
	uint8_t data;		//This byte is set to either 1 or 2 to signify little or big endianness, respectively.
	uint8_t version;	//Always 1
	uint8_t os_abi;		//0x00 System V, 0x01 HP-UX, 0x02 NetBSD, 0x03 Linux, 0x04 GNU Hurd, 0x06 Solaris, 0x07 AIX (Monterey), 0x08 IRIX, 0x09 FreeBSD, 0x0A Tru64, 0x0B Novell Modesto, 0x0C OpenBSD, 0x0D OpenVMS, 0x0E NonStop Kernel, 0x0F AROS, 0x10 FenixOS, 0x11 Nuxi CloudABI, 0x12 Stratus Technologies OpenVOS
	uint8_t abi_version;
	uint8_t pad[7];		//Padding. Unused
} __packed os_elfId_t;

typedef struct{
	os_elfId_t 	e_ident;
	uint16_t	e_type;
	uint16_t	e_machine;
	uint32_t	e_version;		//Set to 1 for the original version of ELF.
	uint32_t	e_entry;		//This is the memory address of the entry point from where the process starts executing.
	uint32_t	e_phoff;		//Points to the start of the program header table.
	uint32_t	e_shoff;		//Points to the start of the section header table.
	uint32_t 	e_flags;		//Interpretation of this field depends on the target architecture.
	uint16_t 	e_ehsize;		//Contains the size of this header, normally 64 Bytes for 64-bit and 52 Bytes for 32-bit format.
	uint16_t	e_phentsize;	//Contains the size of a program header table entry.
	uint16_t	e_phnum;		//Contains the number of entries in the program header table.
	uint16_t	e_shentsize;	//Contains the size of a section header table entry.
	uint16_t	e_shnum;		//Contains the number of entries in the section header table.
	uint16_t	e_shstrndx;		//Contains index of the section header table entry that contains the section names.h

} __packed os_elfHeader_t;

typedef struct{
	uint32_t 	p_type;		//0x00000000 PT_NULL Program header table entry unused., 0x00000001 PT_LOAD Loadable segment. 0x00000002 PT_DYNAMIC Dynamic linking information. 0x00000003 PT_INTERP Interpreter information. 0x00000004 PT_NOTE Auxiliary information. 0x00000005 PT_SHLIB Reserved. 0x00000006 PT_PHDR Segment containing program header table itself. 0x00000007 PT_TLS Thread-Local Storage template.
	uint32_t  	p_offset;	//Offset of the segment in the file image.
	uint32_t	p_vaddr;	//Virtual address of the segment in memory.
	uint32_t	p_paddr;	//On systems where physical address is relevant, reserved for segment's physical address.
	uint32_t	p_filesz;	//Size in bytes of the segment in the file image. May be 0.
	uint32_t 	p_memsz;	//Size in bytes of the segment in memory. May be 0.
	uint32_t 	p_flags;	//Segment-dependent flags (position for 32-bit structure).
	uint32_t	p_align;	//0 and 1 specify no alignment. Otherwise should be a positive, integral power of 2, with p_vaddr equating p_offset modulus p_align
} __packed os_elfProgramHeader_t;

static int os_checkElfHeader(os_elfHeader_t* header, lfs_file_t* lfs_file){

	int err = lfs_file_read(&lfs, lfs_file, header, sizeof(*header));
	if(err < 0){
		PRINTLN("read error %d", err);
		return err;
	}

	if(header->e_ident.magic[0] != 0x7F || header->e_ident.magic[1] != 'E' || header->e_ident.magic[2] != 'L' || header->e_ident.magic[3] != 'F'){
		return -1;
	}

	if(header->e_ident.class != 1 || header->e_ident.data != 1 || header->e_ident.version != 1){
		return -1;
	}

	if(header->e_machine != 40 || header->e_version != 1){
		return -1;
	}

	return 0;
}

uint8_t benga[2048];
static int os_loadElfSegments(os_elfHeader_t* header, lfs_file_t* lfs_file){

	os_elfProgramHeader_t data;
	uint32_t memToAlloc = 0;

	for(uint32_t i = 0; i < header->e_phnum; i++){
		lfs_file_seek(&lfs, lfs_file, header->e_phoff + i * header->e_phentsize, LFS_SEEK_SET);

		int err = lfs_file_read(&lfs, lfs_file, &data, sizeof(data));
		if(err < 0){
			PRINTLN("read error %d", err);
			return err;
		}

		memToAlloc += (data.p_memsz + 16) & ~0xF ;
	}

	//uint8_t* codeSpace = (uint8_t*)os_heap_alloc(memToAlloc);
int pos = 0;
uint32_t vAddr;
int jeba = 0;
	for(uint32_t i = 0; i < header->e_phnum; i++){
		lfs_file_seek(&lfs, lfs_file, header->e_phoff + i * header->e_phentsize, LFS_SEEK_SET);

		int err = lfs_file_read(&lfs, lfs_file, &data, sizeof(data));
		if(err < 0){
			PRINTLN("read error %d", err);
			return err;
		}

		lfs_file_seek(&lfs, lfs_file, data.p_offset, LFS_SEEK_SET);
		err = lfs_file_read(&lfs, lfs_file, &benga[pos], data.p_filesz);
		if(err < 0){
			PRINTLN("read error %d", err);
			return err;
		}
		vAddr = data.p_vaddr;
		pos += (err + 16) & ~0xF;
		jeba = jeba == 0 ? pos : jeba;

	}

	uint32_t* p = (uint32_t*)&benga[852];
	uint32_t addr = *p;
	addr = addr - vAddr + jeba + (uint32_t)benga;
	*p = addr;

	uint32_t jumpAddr = *((uint32_t*)&benga[4]);
	jumpAddr = jumpAddr - FLASH_BASE_ADDR + (uint32_t)benga;
	((int(*)())jumpAddr)();

	return 0;

}

void os_task_createProcess(char* file){

	/* Open file
	 --------------------------------------------------*/
	lfs_file_t lfs_file;
	int err = lfs_file_open(&lfs, &lfs_file, file, LFS_O_RDONLY);
	if(err < 0){
		PRINTLN("Open Error");
		return;
	}

	os_elfHeader_t header;
	if(os_checkElfHeader(&header, &lfs_file) < 0) {
		lfs_file_close(&lfs, &lfs_file);
		PRINTLN("error in header");
		return;
	}

	os_loadElfSegments(&header, &lfs_file);
	lfs_file_close(&lfs, &lfs_file);
}

/***********************************************************************
 * OS Task End
 *
 * @brief This function ends the current running task. Use this to safely end a task if you do not care about its return
 * The task block is freed from the heap, and all tasks waiting for it to finish will be set to ready
 *
 **********************************************************************/
void os_task_end(){
	os_task_delete((os_handle_t) os_cur_task->element);
}


/***********************************************************************
 * OS Task End
 *
 * @brief This function ends the current running task. Use this to safely end a task if you care about its return
 * The task block is not freed from the heap, but all tasks waiting for it to finish will be set to ready.
 *
 * Use os_task_delete() after retrieving its return value to avoid memory leak
 *
 * ATTENTION : This function will activate IRQ regardless of its previous state
 *
 * @param void* retVal : [in] return value
 *
 * @return os_err_e : should never return. If it does, a problem occurred
 *
 **********************************************************************/
os_err_e os_task_return(void* retVal){

	/* Check scheduler
	 ------------------------------------------------------*/
	if(os_scheduler_state_get() != OS_SCHEDULER_START) return OS_ERR_NOT_READY;

	/* Enter critical section
	------------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Store return value and tag as ended
	 ------------------------------------------------------*/
	((os_task_t*)os_cur_task->element)->state = OS_TASK_ENDED;
	((os_task_t*)os_cur_task->element)->retVal = retVal;

	/* Update blocked list to inform handles that task has finished
	 ------------------------------------------------------*/
	os_handle_list_updateAndCheck(os_cur_task->element);

	/* Failsafe
	 ------------------------------------------------------*/
	while(1){

		/* Yeild task
		 ------------------------------------------------------*/
		os_task_yeild();

		/* Enable IRQ no matter what
		 ------------------------------------------------------*/
		__os_enable_irq();

	}

	/* Exit critiacl
	 ------------------------------------------------------*/
	OS_EXIT_CRITICAL();

	/* Another Failsafe
	 ------------------------------------------------------*/
	return OS_ERR_UNKNOWN;
}


/***********************************************************************
 * OS Task delete
 *
 * @brief This function deletes a task, removing it from task list and freeing its block
 *
 * ATTENTION : if the current tasks kills itself, the IRQ will be enabled regardless of its previous state
 *
 * @param os_handle_t h : [in] handle containing the task to kill
 *
 * @return os_err_e : An error code (0 = OK)
 *
 **********************************************************************/
os_err_e os_task_delete(os_handle_t h){

	/* Convert address
	 ------------------------------------------------------*/
	os_task_t* t = (os_task_t*) h;

	/* Search to see if task exists
	 ------------------------------------------------------*/
	os_list_cell_t* list = os_list_search(&os_head, h);

	/* Check for errors
	 ------------------------------------------------------*/
	if(t == NULL) return OS_ERR_BAD_ARG;
	if(list == NULL) return OS_ERR_INVALID;
	if(h->type != OS_OBJ_TASK) return OS_ERR_BAD_ARG;

	/* Check scheduler, we cannot kill the current task if scheduler is not ready
	 ------------------------------------------------------*/
	if(h == os_cur_task->element && os_scheduler_state_get() != OS_SCHEDULER_START) return OS_ERR_NOT_READY;

	/* Enter critical section
	------------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Tag as ended
	 ------------------------------------------------------*/
	t->state = OS_TASK_ENDED;

	/* Update blocked list to inform handles that task has finished
	 ------------------------------------------------------*/
	os_handle_list_updateAndCheck(h);

	/* Check and store if we are killing the current task
	 ------------------------------------------------------*/
	if(h == os_cur_task->element){

		/* Tag task to delete
		 ------------------------------------------------------*/
		t->state = OS_TASK_DELETING;

		/* Failsafe
		 ------------------------------------------------------*/
		while(1){

			/* Yeild task
			 ------------------------------------------------------*/
			os_task_yeild();

			/* Enable IRQ no matter what
			 ------------------------------------------------------*/
			__os_enable_irq();

		}

		/* Another Failsafe
		 ------------------------------------------------------*/
		return OS_ERR_UNKNOWN;
	}

	/* Remove task from object block list if needed
	 ------------------------------------------------------*/
	if( t->objWaited != NULL) {

		for(uint32_t i = 0; i < t->sizeObjs; i++){

			/* Failsafe
			 ------------------------------------------------------*/
			if(t->objWaited[i] == NULL) continue;

			/* Remove from block list
			 ------------------------------------------------------*/
			os_list_remove(t->objWaited[i]->blockList, h);

			/* Update object's priority
			 ------------------------------------------------------*/
			os_obj_updatePrio(t->objWaited[i]);

			/* Update block list
			 ------------------------------------------------------*/
			os_handle_list_updateAndCheck(t->objWaited[i]);
		}

	}

	/* Deletes from obj list
	 ------------------------------------------------------*/
	os_list_remove(&os_obj_head, h);

	/* Remove task from list
	 ------------------------------------------------------*/
	os_list_remove(&os_head, h);

	/* Clear blocked list
	 ------------------------------------------------------*/
	os_list_clear(h->blockList);

	/* Clear owned mutex list
	 ------------------------------------------------------*/
	os_list_clear(t->ownedMutex);

	/* Free the stack memory
	 ------------------------------------------------------*/
	os_heap_free( (void*) (t->stackBase - t->stackSize) );

	/* Reset values just in case
	 ------------------------------------------------------*/
	t->state = OS_TASK_ENDED;
	t->objWaited = NULL;
	t->sizeObjs = 0;
	t->pStack = 0;
	t->stackBase = 0;
	t->stackSize = 0;
	t->wakeCoutdown = 0;

	/* Free code and name
	 ------------------------------------------------------*/
	os_heap_free(t->fnPtr);
	os_heap_free(h->name);

	/* Delete task
	 ------------------------------------------------------*/
	os_heap_free(h);

	/* Return
	 ------------------------------------------------------*/
	OS_EXIT_CRITICAL();
	return OS_ERR_OK;
}


/***********************************************************************
 * OS Task Yeild
 *
 * @brief This function calls the scheduler in order to let other tasks to run
 *
 **********************************************************************/
void os_task_yeild(){

	/* Set Pend SV
	 ------------------------------------------------------*/
	OS_SET_PENDSV();
}


/***********************************************************************
 * OS Task get priority
 *
 * @brief This function calculates a task's dynamic priority
 *
 * @param task_t* t : [in] handle to task
 *
 * @return int8_t : The effective priority or -1 if argument error
 *
 **********************************************************************/
int8_t os_task_getPrio(os_handle_t h){

	/* Convert address
	 ------------------------------------------------------*/
	os_task_t* t = (os_task_t*) h;

	/* Check for errors
	 ------------------------------------------------------	*/
	if(t == NULL) return -1;
	if(h->type != OS_OBJ_TASK) return -1;

	return t->priority;
}


/***********************************************************************
 * OS Task Sleep
 *
 * ATTENTION : This functions enables IRQ regardless of its previous state
 *
 * @brief This function blocks a task for an amount of ticks OS
 *
 * @param uint32_t sleep_ticks : [in] amount of ticks to sleep
 *
 * @return os_err_e : error code (0 = OK)
 *
 **********************************************************************/
os_err_e os_task_sleep(uint32_t sleep_ticks){

	/* Get xPSR register
	 ---------------------------------------------------*/
	register uint32_t volatile xPSR = 0;
	__asm volatile("mrs %[out], xpsr" : [out] "=r" (xPSR));

	/* Check scheduler stop
	 ------------------------------------------------------*/
	if(os_scheduler_state_get() == OS_SCHEDULER_STOP) return OS_ERR_NOT_READY;

	/* Check if we are in thread mode (cannot sleep in interupt mode)
	 ---------------------------------------------------*/
	if( (xPSR & 0x1F) != 0) return OS_ERR_FORBIDDEN;

	/* Enter Critical -> If the list is changed during the process, this can corrupt our references
	 ------------------------------------------------------*/
	__os_disable_irq();

	/* Put task to blocked and change countdown
	 ------------------------------------------------------*/
	((os_task_t*)os_cur_task->element)->wakeCoutdown = sleep_ticks;
	((os_task_t*)os_cur_task->element)->state = OS_TASK_BLOCKED;

	/* Prepare scheduler to run
	 ------------------------------------------------------*/
	os_task_yeild();

	/* Enable IRQ no matter what
	 ------------------------------------------------------*/
	__os_enable_irq();

	return OS_ERR_OK;
}


/***********************************************************************
 * OS Task Get return
 *
 * @brief This function gets the return of a task that called os_task_end or returned from its callback function
 * This API should not be called if the task has been destroyed
 *
 * @param os_handle_t t: [in] task to get the return
 *
 * @return void* : NULL if error, the task return if OK
 *
 **********************************************************************/
void* os_task_getReturn(os_handle_t h){

	/* Convert address
	 ------------------------------------------------------*/
	os_task_t* task = (os_task_t*) h;

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return NULL;
	if(h->type != OS_OBJ_TASK) return NULL;
	if(os_list_search(&os_head, h) == NULL) return NULL;
	if(task->state != OS_TASK_ENDED) return NULL;

	return task->retVal;
}


/***********************************************************************
 * OS Task Get State
 *
 * @brief This function gets the state of a task. This state not necessarily matches the one in the task, since there are edge cases and
 * problems that do not concern the user
 *
 * @param os_handle_t h : [in] task to get the state
 *
 * @return os_task_state_e: The state of the task (Not exist = it was never created or got deleted ; Blocked if it is blocked ; Ready if it is ready ; Ended if it ended)
 *
 **********************************************************************/
os_task_state_e os_task_getState(os_handle_t h){

	/* Convert address
	 ------------------------------------------------------*/
	os_task_t* task = (os_task_t*) h;

	/* Check arguments
	 ------------------------------------------------------*/
	if(h == NULL) return OS_TASK_NOT_EXIST;
	if(h->type != OS_OBJ_TASK) return OS_TASK_NOT_EXIST;
	if(os_list_search(&os_head, h) == NULL) return OS_TASK_NOT_EXIST;
	if(task->state == OS_TASK_DELETING) return OS_TASK_NOT_EXIST;

	/* Check if task is ended
	 ------------------------------------------------------*/
	os_task_state_e state = task->state;

	/* Force blocked if task is waiting for an object
	 * This scenario can happen if the object is free, but the task did not have the chance to take it yet
	 ------------------------------------------------------*/
	state = state == OS_TASK_READY && task->objWaited != NULL ? OS_TASK_BLOCKED : state;

	return state;
}


/***********************************************************************
 * OS get task by PID
 *
 * @brief This function searches for a task using its PID
 *
 * @param uint16_t pid : [in] PID of the searched task
 *
 * @return os_list_cell_t* : reference to the cell containing the element or null if not found
 **********************************************************************/
os_handle_t os_task_getByPID(uint16_t pid){

	/* Enter Critical Section
	 * If it's searching / inserting a block, it can be interrupted and another task can change the list. In this case, the first task will blow up when returning
	 ------------------------------------------------------*/
	OS_DECLARE_IRQ_STATE;
	OS_ENTER_CRITICAL();

	/* Search position to insert
	 ------------------------------------------------------*/
	os_list_cell_t* it = os_head.head.next;
	while(it != NULL && ((os_task_t*)it->element)->pid != pid){
		it = it->next;
	}

	OS_EXIT_CRITICAL();
	return it == NULL ? NULL : it->element;
}

