/*
 * OS_Common.h
 *
 *  Created on: May 30, 2021
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_COMMON_H_
#define INC_OS_OS_COMMON_H_

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "OS/OS_Core/OS_Config.h"
#include "OS/OS_CLI/cli_conf.h"
#include "OS/OS_CLI/cli.h"


/**********************************************
 * DEFINES
 * ********************************************/

/* Usefu defines
 ---------------------------------------------------*/
#define OS_SET_BITS(reg, bits) 			(reg |=  (bits))
#define OS_CLEAR_BITS(reg, bits) 		(reg &= ~(bits))

#define __OS_IM     volatile const      /*! Defines 'read only' structure member permissions */
#define __OS_OM     volatile            /*! Defines 'write only' structure member permissions */
#define __OS_IOM    volatile            /*! Defines 'read / write' structure member permissions */

/* OS minimum stack for all tasks (needs for context change)
 ---------------------------------------------------*/
#if defined(OS_FPU_EN) && OS_FPU_EN == 1
#define OS_MINIMUM_STACK_SIZE	384
#else
#define OS_MINIMUM_STACK_SIZE	128
#endif

/* Declare variable to store PRIMASK state
 ---------------------------------------------------*/
#define OS_DECLARE_IRQ_STATE				uint32_t volatile irqState

/* Desables ALL IRQ
 ---------------------------------------------------*/
#define OS_ENTER_CRITICAL()  				do{ __asm volatile ("mrs %[out], primask" : [out] "=r" (irqState)); __asm volatile ("cpsid i" : : : "memory"); __asm volatile ("isb"); } while(0)

/* Recover last state using irqState
 ---------------------------------------------------*/
#define OS_EXIT_CRITICAL()  				do { __asm volatile ("msr primask, %[in]" : : [in] "r" (irqState)); __asm volatile ("isb"); } while(0)

/* Create a critical section
 ---------------------------------------------------*/
#define OS_CRITICAL_SECTION(yourcode)		do{ 						\
												OS_DECLARE_IRQ_STATE; 	\
												OS_ENTER_CRITICAL();	\
												yourcode				\
												OS_EXIT_CRITICAL();		\
											}while(0);


/* Defines a definition as weak
 ---------------------------------------------------*/
#ifndef __weak
#define __weak __attribute__((weak))
#endif

/* Defines a naked function (pure assembly)
 ---------------------------------------------------*/
#ifndef __naked
#define __naked __attribute__((naked))
#endif

/* Defines an used function to avoid warnings and optimization (functions called by asm are not detected)
 ---------------------------------------------------*/
#ifndef __used
#define __used __attribute__((used))
#endif

#ifndef __os_disable_irq
#define __os_disable_irq() 	__asm volatile ("cpsid i" : : : "memory");
#endif

#ifndef __os_enable_irq
#define __os_enable_irq() 	__asm volatile ("cpsie i" : : : "memory");
#endif

#ifndef __align
#define __align(x)			__attribute__((aligned(x)))
#endif

#ifndef UNUSED_ARG
#define UNUSED_ARG(x)	((void)x)
#endif

#if defined(PRINT_ENABLE) && PRINT_ENABLE == 1
//#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define PRINTF(S, ...) 		printf(S, ## __VA_ARGS__)
#define PRINT(S, ...) 		PRINTF("[%s:%03d]:" S, __FILENAME__, __LINE__ ## __VA_ARGS__)
#define PRINTLN(S, ...) 	PRINT (S "\r\n", ## __VA_ARGS__)
#else
#define PRINTF(S, ...)
#define PRINT(S, ...)
#define PRINTLN(S, ...)
#endif

#ifndef ASSERT
#define ASSERT(x)     if(!x) PRINTLN("Error occured");
#endif

/**********************************************
 * PUBLIC TYPES
 *********************************************/

/* OS errors
 ---------------------------------------------------*/
typedef enum{
	OS_ERR_OK,
	OS_ERR_BAD_ARG,
	OS_ERR_INVALID,
	OS_ERR_INSUFFICIENT_HEAP,
	OS_ERR_NOT_READY,
	OS_ERR_FORBIDDEN,
	OS_ERR_TIMEOUT,
	OS_ERR_UNKNOWN,
} os_err_e;


/**********************************************
 * MCU includes
 *********************************************/

/* Include to link with cortex M4, must be at the end
 ---------------------------------------------------*/
#include "OS/OS_Core/OS_cortexM4.h"

#endif /* INC_OS_OS_COMMON_H_ */
