/*
 * OS_Syscalls.h
 *
 *  Created on: Apr 15, 2025
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_CORE_OS_SYSCALLS_H_
#define INC_OS_OS_CORE_OS_SYSCALLS_H_


/**********************************************
 * PUBLIC TYPES
 *********************************************/

/* Syscall ID
------------------------------------------------------*/
typedef enum os_syscall_ {
	OS_SYSCALL_FOPEN		= 0,
	OS_SYSCALL_FCLOSE		= 1,
	OS_SYSCALL_FREAD		= 2,
	OS_SYSCALL_FWRITE		= 3,
} os_syscall_e;


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
void* os_syscall(os_syscall_e call, void* arg1, void* arg2, void* arg3, void* arg4, void* arg5, void* arg6, void* arg7, void* arg8);


#endif /* INC_OS_OS_CORE_OS_SYSCALLS_H_ */
