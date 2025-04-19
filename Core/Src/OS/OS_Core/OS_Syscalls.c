/*
 * OS_Syscalls.c
 *
 *  Created on: Apr 15, 2025
 *      Author: Gabriel
 */


#include "OS/OS_Core/OS.h"
#include "OS/OS_Core/OS_Internal.h"
#include "OS/OS_FS/OS_fs.h"


/**********************************************
 * PRIVATE TYPES
 *********************************************/

/* Syscall stack frame
 ------------------------------------------------------*/
typedef struct os_syscall_frame_ {
	os_handle_t syscall_thread;
	os_handle_t caller_task;
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
} os_syscall_frame_t;


/* syscall thread prototype
 ------------------------------------------------------*/
typedef uint32_t (sys_fn_t)(os_syscall_frame_t* frame);


/* Syscall table structure
 ------------------------------------------------------*/
typedef struct os_syscall_table_ {
	char* name;
	os_syscall_e call_id;
	sys_fn_t* sys_fn;
} os_syscall_table_t;


/**********************************************
 * PRIVATE FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Syscall open
 *
 * @brief This function implements the syscall to os_fopen
 *
 **********************************************************************/
static OS_FILE* os_syscall_fopen(os_syscall_frame_t* frame){
	return os_fopen((char*)frame->r1, (char*)frame->r2);
}

/***********************************************************************
 * OS Syscall close
 *
 * @brief This function implements the syscall to os_fclose
 *
 **********************************************************************/
static int os_syscall_fclose(os_syscall_frame_t* frame){
	return os_fclose((OS_FILE*)frame->r1);
}

/***********************************************************************
 * OS Syscall read
 *
 * @brief This function implements the syscall to os_fread
 *
 **********************************************************************/
static size_t os_syscall_fread(os_syscall_frame_t* frame){
	return os_fread((void*)frame->r1, (size_t)frame->r2, (size_t)frame->r3, (OS_FILE*)frame->r4);
}

/***********************************************************************
 * OS Syscall write
 *
 * @brief This function implements the syscall to os_fwrite
 *
 **********************************************************************/
static size_t os_syscall_fwrite(os_syscall_frame_t* frame){
	return os_fwrite((void*)frame->r1, (size_t)frame->r2, (size_t)frame->r3, (OS_FILE*)frame->r4);
}

/**********************************************
 * PRIVATE VARIABLES
 *********************************************/

/* Declare syscall table
 ------------------------------------------------------*/
static const os_syscall_table_t os_syscall_table[] = {
		{ .name = "fopen",		.call_id =	OS_SYSCALL_FOPEN,		.sys_fn = (sys_fn_t*)&os_syscall_fopen 		},
		{ .name = "fclose",		.call_id =	OS_SYSCALL_FCLOSE,		.sys_fn = (sys_fn_t*)&os_syscall_fclose 	},
		{ .name = "fread",		.call_id =	OS_SYSCALL_FREAD,		.sys_fn = (sys_fn_t*)&os_syscall_fread 		},
		{ .name = "fwrite",		.call_id =	OS_SYSCALL_FWRITE,		.sys_fn = (sys_fn_t*)&os_syscall_fwrite 	},
};


/***********************************************************************
 * OS Syscall thread
 *
 * @brief This function calls the syscall function depending on R0 value
 *
 * @param os_syscall_frame_t* frame : [in] Reference to caller stack
 *
 * @return int : always 0
 **********************************************************************/
static int os_syscall_thread(os_syscall_frame_t* frame){
	uint32_t* ret = &frame->r0;
	os_syscall_e call = frame->r0;
	for(int i = 0; i < COUNTOF(os_syscall_table); i++){
		if(os_syscall_table[i].call_id == call){
			*ret = os_syscall_table[i].sys_fn(frame);
			return 0;
		}
	}

	*ret = (uint32_t)OS_ERR_INVALID;
	return 0;
}


/***********************************************************************
 * OS Syscall Handler
 *
 * @brief This function invokes a thread to handle the syscall, as well as blocking the caller thread as if it called "os_obj_wait_single"
 *
 * @param os_syscall_frame_t* frame : [in] Reference to caller stack
 *
 **********************************************************************/
static void __used os_syscall_handler(os_syscall_frame_t* frame){

	/* Error code is returned by writing into R0 value in caller stack
	 ------------------------------------------------------*/
	int* ret = (int*)&frame->r0;

	/* If scheduler is not running, exit
	 ------------------------------------------------------*/
	if(os_scheduler_state_get() != OS_SCHEDULER_START){
		*ret = OS_ERR_FORBIDDEN;
		return;
	}

	/* Create syscall thread
	 ------------------------------------------------------*/
	os_err_e er = os_task_create(&frame->syscall_thread, NULL, (void*)os_syscall_thread, OS_TASK_MODE_DELETE, 100, OS_DEFAULT_STACK_SIZE, frame);
	if(er != OS_ERR_OK){
		*ret = (int)er;
		goto exit;
	}

	/* Add caller task to blocking list
	 ------------------------------------------------------*/
	frame->caller_task = (os_handle_t) os_task_getCurrentTask();
	er = os_list_add( frame->syscall_thread->blockList, frame->caller_task, OS_LIST_LAST );
	if(er != OS_ERR_OK){
		*ret = (int)er;
		goto exit_task;
	}

	/* Block current task and call scheduler
	 ------------------------------------------------------*/
	os_task_t* t = (os_task_t*) frame->caller_task;

	t->state 			= OS_TASK_BLOCKED;
	t->wakeCoutdown		= OS_WAIT_FOREVER;
	t->objWaited 		= (void*)&frame->syscall_thread;
	t->sizeObjs 		= 1;
	t->objWanted 		= 0xFFFFFFFF;
	t->waitFlag 		= OS_OBJ_WAIT_ALL;

	os_task_yeild();

	return;

	/* Cleanup in case of errors
	 ------------------------------------------------------*/

exit_task:
	os_task_delete(frame->syscall_thread);

exit:
	return;
}


/**********************************************
 * IRQ FUNCTIONS
 *********************************************/

/***********************************************************************
 * SVC Handler
 *
 * @brief This function implements interrupt handler for SVC. It gets the stack frame prepared by os_syscall to be used by syscall thread
 *
 * @param os_syscall_frame_t* frame : [in] Reference to caller stack
 *
 **********************************************************************/
void __naked SVC_Handler(void){

	/* Get PSP and go to the beginning of interrupt stack frame (32 bytes long)
	 ------------------------------------------------------*/
	__asm volatile ("mrs r0, psp");					//R0 = PSP
	__asm volatile ("add r0, 0x20");				//R0 += 32

#if defined(OS_FPU_EN) && OS_FPU_EN == 1
	/* If FPU was working, we have to go deeper in stack
	 ------------------------------------------------------*/
	__asm volatile("tst lr, #0x10");				//If LR & 1 << 4 == 0
	__asm volatile("it eq");						//If previous instruction was 0,
	__asm volatile("addeq r0, #0x48");				//R0 += 0x48, in this case the interrupt stack frame is longer
#endif

	/* Call handler
	 ------------------------------------------------------*/
	__asm volatile ("push {lr}");
	__asm volatile ("bl os_syscall_handler");
	__asm volatile ("pop {lr}");

	__asm volatile ("bx lr");

}


/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/


/***********************************************************************
 * Os syscall
 *
 * @brief This function prepares a stack frame for all arguments, and calls the syscall interrupt
 *
 * @param os_syscall_e call : [in] Syscall ID
 * @param ...               : [in] arguments to send to syscall
 *
 **********************************************************************/
void* __naked os_syscall(os_syscall_e call, void* arg1, void* arg2, void* arg3, void* arg4, void* arg5, void* arg6, void* arg7, void* arg8){

	/* Pushes R0 to R3 in stack. Arm already pushes all arguments beyond 4 to stack, but the first 4 arguments are stored in R0 to R3
	 * Then, pushes R0 and R1 again, this is to be used by syscall handler to store the syscall thread reference.
	 * This is complex, because we cannot store the syscall thread reference in the interrupt stack, because it will return before the syscall finishes
	 * We cannot manipulate the current task stack inside interrupt because that would break the interrupt stack frame
	 * We cannot use the heap, because we would need to store its reference somewhere
	 * Global variables are out of the question.
	 * We could add some members to task structure, but doing it this way is easier
	 ------------------------------------------------------*/
	__asm volatile ("push {r0-r3}");
	__asm volatile ("push {r0-r1}");

	/* Calls interrupt and makes sure no other instruction in executed after it
	 ------------------------------------------------------*/
	__asm volatile ("svc 0");
	__asm volatile ("isb");
	__asm volatile ("nop");
	__asm volatile ("nop");
	__asm volatile ("nop");

	/* Get return value stored in stack by the syscall thread
	 ------------------------------------------------------*/
	__asm volatile ("add sp, #8");
	__asm volatile ("pop {r0}");
	__asm volatile ("add sp, #12");

	__asm volatile ("bx lr");
}
