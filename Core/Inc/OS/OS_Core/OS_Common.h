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

#ifndef __section
#define __section(x)		__attribute__((section(x)))
#endif

#ifndef UNUSED_ARG
#define UNUSED_ARG(x)	((void)x)
#endif

#ifndef COUNTOF
#define COUNTOF(x)		((int)(sizeof(x)/sizeof(*x)))
#endif

/**********************************************
 * PUBLIC TYPES
 *********************************************/

/* OS errors
 ---------------------------------------------------*/
enum{
	OS_ERR_OK			 		=  0,
	OS_ERR_BAD_ARG		 		= -1,
	OS_ERR_INVALID				= -2,
	OS_ERR_INSUFFICIENT_HEAP	= -3,
	OS_ERR_NOT_READY			= -4,
	OS_ERR_FORBIDDEN			= -5,
	OS_ERR_TIMEOUT				= -6,
	OS_ERR_UNKNOWN				= -7,
	OS_ERR_FS					= -8,
	OS_ERR_EMPTY				= -9,
};

typedef int32_t os_err_e;


/**********************************************
 * MCU includes
 *********************************************/

/* Include to link with cpu, must be at the end
 ---------------------------------------------------*/
#ifdef __OS_CORTEX_M4
#include "OS_cortexM4.h"
#elif defined(__OS_CORTEX_M33)
#include "OS_cortexM33.h"
#else
#error "Please select a supported CPU"
#endif

#endif /* INC_OS_OS_COMMON_H_ */
