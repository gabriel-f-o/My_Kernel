/*
 * OS_cortexM4.h
 *
 *  Created on: Feb 13, 2022
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_CORTEXM33_H_
#define INC_OS_OS_CORTEXM33_H_

#include "OS_Common.h"

/**********************************************
 * DEFINES
 * ********************************************/

/* System control defines
 ---------------------------------------------------*/
#define OS_SYSTEM_CTRL_BASE						(0xE000E010)
#define OS_SYSTEM_CTRL							((SystemControl_TypeDef*) OS_SYSTEM_CTRL_BASE)

#define OS_SYSTEM_CTRL_ICSR_PENDSVSET_Pos 		(28U)
#define OS_SYSTEM_CTRL_ICSR_PENDSVSET_Msk 		(0x1UL << OS_SYSTEM_CTRL_ICSR_PENDSVSET_Pos)
#define OS_SYSTEM_CTRL_ICSR_PENDSVSET  			(OS_SYSTEM_CTRL_ICSR_PENDSVSET_Msk)

#define OS_SYSTEM_CTRL_CPACR_CP10_Pos			(20U)
#define OS_SYSTEM_CTRL_CPACR_CP10_Msk			(0x3UL << OS_SYSTEM_CTRL_CPACR_CP10_Pos)
#define OS_SYSTEM_CTRL_CPACR_CP10				(OS_SYSTEM_CTRL_CPACR_CP10_Msk)

#define OS_SYSTEM_CTRL_CPACR_CP11_Pos			(22U)
#define OS_SYSTEM_CTRL_CPACR_CP11_Msk			(0x3UL << OS_SYSTEM_CTRL_CPACR_CP11_Pos)
#define OS_SYSTEM_CTRL_CPACR_CP11				(OS_SYSTEM_CTRL_CPACR_CP11_Msk)

#define OS_SYSTEM_CTRL_SYSTCSR_ENABLE_Pos		(0U)
#define OS_SYSTEM_CTRL_SYSTCSR_ENABLE_Msk		(0x1UL << OS_SYSTEM_CTRL_SYSTCSR_ENABLE_Pos)
#define OS_SYSTEM_CTRL_SYSTCSR_ENABLE			(OS_SYSTEM_CTRL_SYSTCSR_ENABLE_Msk)

#define OS_SYSTEM_CTRL_SYSTCSR_TICKINT_Pos		(1U)
#define OS_SYSTEM_CTRL_SYSTCSR_TICKINT_Msk		(0x1UL << OS_SYSTEM_CTRL_SYSTCSR_TICKINT_Pos)
#define OS_SYSTEM_CTRL_SYSTCSR_TICKINT			(OS_SYSTEM_CTRL_SYSTCSR_TICKINT_Msk)

#define OS_SYSTEM_CTRL_SYSTCSR_CLKSOURCE_Pos	(2U)
#define OS_SYSTEM_CTRL_SYSTCSR_CLKSOURCE_Msk	(0x1UL << OS_SYSTEM_CTRL_SYSTCSR_CLKSOURCE_Pos)
#define OS_SYSTEM_CTRL_SYSTCSR_CLKSOURCE		(OS_SYSTEM_CTRL_SYSTCSR_CLKSOURCE_Msk)

#define OS_SYSTICK_ENABLE()						OS_SET_BITS(OS_SYSTEM_CTRL->SYSTCSR,   OS_SYSTEM_CTRL_SYSTCSR_ENABLE)
#define OS_SYSTICK_DISABLE()					OS_CLEAR_BITS(OS_SYSTEM_CTRL->SYSTCSR, OS_SYSTEM_CTRL_SYSTCSR_ENABLE)

#define OS_SYSTICK_TICKINT_EN()					OS_SET_BITS(OS_SYSTEM_CTRL->SYSTCSR,   OS_SYSTEM_CTRL_SYSTCSR_TICKINT)
#define OS_SYSTICK_TICKINT_DIS()				OS_CLEAR_BITS(OS_SYSTEM_CTRL->SYSTCSR, OS_SYSTEM_CTRL_SYSTCSR_TICKINT)

#define OS_SYSTICK_CLOCKSOURCE_PROC()			OS_SET_BITS(OS_SYSTEM_CTRL->SYSTCSR,   OS_SYSTEM_CTRL_SYSTCSR_CLKSOURCE)
#define OS_SYSTICK_CLOCKSOURCE_EXT()			OS_CLEAR_BITS(OS_SYSTEM_CTRL->SYSTCSR, OS_SYSTEM_CTRL_SYSTCSR_CLKSOURCE)

#define OS_SYSTICK_SET_PRIO(x)					do { OS_SYSTEM_CTRL->SHPR3   |= ( ( (x) & 0x0000FF) << 24); }while(0);
#define OS_SYSTICK_SET_RELOAD(x)				do { OS_SYSTEM_CTRL->SYSTRVR = ( ( (x) & 0xFFFFFF) <<  0); }while(0);

#define OS_SET_PENDSV()							do { OS_SET_BITS(OS_SYSTEM_CTRL->ICSR, OS_SYSTEM_CTRL_ICSR_PENDSVSET); } while(0)
#define OS_PENDSV_SET_PRIO(x)					do { OS_SYSTEM_CTRL->SHPR3   |= ( ( (x) & 0x0000FF) << 16); }while(0);

/* FPU defines
 ---------------------------------------------------*/
#define OS_FPU_BASE								(0xE000EF34)
#define OS_FPU									((FPU_TypeDef*) OS_FPU_BASE)

#define OS_FPU_FPCCR_ASPEN_Pos					(30U)
#define OS_FPU_FPCCR_ASPEN_Msk					(0x1UL << OS_FPU_FPCCR_ASPEN_Pos)
#define OS_FPU_FPCCR_ASPEN						(OS_FPU_FPCCR_ASPEN_Msk)

#define OS_FPU_FPCCR_LSPEN_Pos					(31U)
#define OS_FPU_FPCCR_LSPEN_Msk					(0x1UL << OS_FPU_FPCCR_LSPEN_Pos)
#define OS_FPU_FPCCR_LSPEN						(OS_FPU_FPCCR_LSPEN_Msk)

#define OS_FPU_ENABLE()							OS_SET_BITS(OS_SYSTEM_CTRL->CPACR,   (OS_SYSTEM_CTRL_CPACR_CP10 | OS_SYSTEM_CTRL_CPACR_CP11))
#define OS_FPU_DISABLE()						OS_CLEAR_BITS(OS_SYSTEM_CTRL->CPACR, (OS_SYSTEM_CTRL_CPACR_CP10 | OS_SYSTEM_CTRL_CPACR_CP11))

#define OS_FPU_LAZY_ENABLE()					OS_SET_BITS(OS_FPU->FPCCR,   OS_FPU_FPCCR_LSPEN)
#define OS_FPU_LAZY_DISABLE()					OS_CLEAR_BITS(OS_FPU->FPCCR, OS_FPU_FPCCR_LSPEN)

#define OS_FPU_STATUS_ENABLE()					OS_SET_BITS(OS_FPU->FPCCR,   OS_FPU_FPCCR_ASPEN)
#define OS_FPU_STATUS_DISABLE()					OS_CLEAR_BITS(OS_FPU->FPCCR, OS_FPU_FPCCR_ASPEN)

/* CFSR defines
 ---------------------------------------------------*/
#define OS_CFSR_IACCVIOL_Pos					(0U)
#define OS_CFSR_IACCVIOL_Msk					(1U << OS_CFSR_IACCVIOL_Pos)
#define OS_CFSR_IACCVIOL						(OS_CFSR_IACCVIOL_Msk)

#define OS_CFSR_DACCVIOL_Pos					(1U)
#define OS_CFSR_DACCVIOL_Msk					(1U << OS_CFSR_DACCVIOL_Pos)
#define OS_CFSR_DACCVIOL						(OS_CFSR_DACCVIOL_Msk)

#define OS_CFSR_MUNSTKERR_Pos					(3U)
#define OS_CFSR_MUNSTKERR_Msk					(1U << OS_CFSR_MUNSTKERR_Pos)
#define OS_CFSR_MUNSTKERR						(OS_CFSR_MUNSTKERR_Msk)

#define OS_CFSR_MSTKERR_Pos						(4U)
#define OS_CFSR_MSTKERR_Msk						(1U << OS_CFSR_MSTKERR_Pos)
#define OS_CFSR_MSTKERR							(OS_CFSR_MSTKERR_Msk)

#define OS_CFSR_MLSPERR_Pos						(5U)
#define OS_CFSR_MLSPERR_Msk						(1U << OS_CFSR_MLSPERR_Pos)
#define OS_CFSR_MLSPERR							(OS_CFSR_MLSPERR_Msk)

#define OS_CFSR_MMARVALID_Pos					(7U)
#define OS_CFSR_MMARVALID_Msk					(1U << OS_CFSR_MMARVALID_Pos)
#define OS_CFSR_MMARVALID						(OS_CFSR_MMARVALID_Msk)

#define OS_CFSR_IBUSERR_Pos						(8U)
#define OS_CFSR_IBUSERR_Msk						(1U << OS_CFSR_IBUSERR_Pos)
#define OS_CFSR_IBUSERR							(OS_CFSR_IBUSERR_Msk)

#define OS_CFSR_PRECISERR_Pos					(9U)
#define OS_CFSR_PRECISERR_Msk					(1U << OS_CFSR_PRECISERR_Pos)
#define OS_CFSR_PRECISERR						(OS_CFSR_PRECISERR_Msk)

#define OS_CFSR_UNSTKERR_Pos					(11U)
#define OS_CFSR_UNSTKERR_Msk					(1U << OS_CFSR_UNSTKERR_Pos)
#define OS_CFSR_UNSTKERR						(OS_CFSR_UNSTKERR_Msk)

#define OS_CFSR_STKERR_Pos						(12U)
#define OS_CFSR_STKERR_Msk						(1U << OS_CFSR_STKERR_Pos)
#define OS_CFSR_STKERR							(OS_CFSR_STKERR_Msk)

#define OS_CFSR_LSPERR_Pos						(13U)
#define OS_CFSR_LSPERR_Msk						(1U << OS_CFSR_LSPERR_Pos)
#define OS_CFSR_LSPERR							(OS_CFSR_LSPERR_Msk)

#define OS_CFSR_BFARVALID_Pos					(15U)
#define OS_CFSR_BFARVALID_Msk					(1U << OS_CFSR_BFARVALID_Pos)
#define OS_CFSR_BFARVALID						(OS_CFSR_BFARVALID_Msk)

#define OS_CFSR_UNDEFINSTR_Pos					(16U)
#define OS_CFSR_UNDEFINSTR_Msk					(1U << OS_CFSR_UNDEFINSTR_Pos)
#define OS_CFSR_UNDEFINSTR						(OS_CFSR_UNDEFINSTR_Msk)

#define OS_CFSR_INVSTATE_Pos					(17U)
#define OS_CFSR_INVSTATE_Msk					(1U << OS_CFSR_INVSTATE_Pos)
#define OS_CFSR_INVSTATE						(OS_CFSR_INVSTATE_Msk)

#define OS_CFSR_INVPC_Pos						(18U)
#define OS_CFSR_INVPC_Msk						(1U << OS_CFSR_INVPC_Pos)
#define OS_CFSR_INVPC							(OS_CFSR_INVPC_Msk)

#define OS_CFSR_NOCP_Pos						(19U)
#define OS_CFSR_NOCP_Msk						(1U << OS_CFSR_NOCP_Pos)
#define OS_CFSR_NOCP							(OS_CFSR_NOCP_Msk)

#define OS_CFSR_STKOF_Pos						(20U)
#define OS_CFSR_STKOF_Msk						(1U << OS_CFSR_STKOF_Pos)
#define OS_CFSR_STKOF							(OS_CFSR_STKOF_Msk)

#define OS_CFSR_UNALIGNED_Pos					(24U)
#define OS_CFSR_UNALIGNED_Msk					(1U << OS_CFSR_UNALIGNED_Pos)
#define OS_CFSR_UNALIGNED						(OS_CFSR_UNALIGNED_Msk)

#define OS_CFSR_DIVBYZERO_Pos					(25U)
#define OS_CFSR_DIVBYZERO_Msk					(1U << OS_CFSR_DIVBYZERO_Pos)
#define OS_CFSR_DIVBYZERO						(OS_CFSR_DIVBYZERO_Msk)

/**********************************************
 * PUBLIC TYPES
 * ********************************************/

typedef struct{
	__OS_IOM uint32_t SYSTCSR; 	//0xE000E010
	__OS_IOM uint32_t SYSTRVR; 	//0xE000E014
	uint32_t reserved0[827];
	__OS_IOM uint32_t ICSR;	 	//0xE000ED04
	uint32_t reserved1[6];
	__OS_IOM uint32_t SHPR3;	//0xE000ED20
    uint32_t reserved2[1];
    __OS_IOM uint32_t CFSR;     //0xE000ED28
    uint32_t reserved3[2];
    __OS_IOM uint32_t MMFAR;    //0xE000ED34
    __OS_IOM uint32_t BFAR;     //0xE000ED38
	uint32_t reserved4[19];
	__OS_IOM uint32_t CPACR;	//0xE000ED88     
}SystemControl_TypeDef;

typedef struct{
	__OS_IOM uint32_t FPCCR;		//FP Context Control Register
	__OS_IOM uint32_t FPCAR;		//FP Context Address Register
	__OS_IOM uint32_t FPDSCR;		//FP Default Status Control Register
	__OS_IM  uint32_t MVFR0;		//Media and VFP Feature Register 0, MVFR0
	__OS_IM  uint32_t MVFR1;		//Media and VFP Feature Register 1, MVFR1
}FPU_TypeDef;


#endif /* INC_OS_OS_CORTEXM33_H_ */
