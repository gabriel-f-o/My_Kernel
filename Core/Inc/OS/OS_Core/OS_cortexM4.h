/*
 * OS_cortexM4.h
 *
 *  Created on: Feb 13, 2022
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_CORTEXM4_H_
#define INC_OS_OS_CORTEXM4_H_

#include "OS//OS_Core/OS_Common.h"

/**********************************************
 * DEFINES
 * ********************************************/

/* System control defines
 ---------------------------------------------------*/
#define OS_SYSTEM_CTRL_BASE					(0xE000E008)
#define OS_SYSTEM_CTRL						((SystemControl_TypeDef*) OS_SYSTEM_CTRL_BASE)

#define OS_SYSTEM_CTRL_ICSR_PENDSVSET_Pos 	(28U)
#define OS_SYSTEM_CTRL_ICSR_PENDSVSET_Msk 	(0x1UL << OS_SYSTEM_CTRL_ICSR_PENDSVSET_Pos)
#define OS_SYSTEM_CTRL_ICSR_PENDSVSET  		(OS_SYSTEM_CTRL_ICSR_PENDSVSET_Msk)

#define OS_SYSTEM_CTRL_CPACR_CP10_Pos		(20U)
#define OS_SYSTEM_CTRL_CPACR_CP10_Msk		(0x3UL << OS_SYSTEM_CTRL_CPACR_CP10_Pos)
#define OS_SYSTEM_CTRL_CPACR_CP10			(OS_SYSTEM_CTRL_CPACR_CP10_Msk)

#define OS_SYSTEM_CTRL_CPACR_CP11_Pos		(22U)
#define OS_SYSTEM_CTRL_CPACR_CP11_Msk		(0x3UL << OS_SYSTEM_CTRL_CPACR_CP11_Pos)
#define OS_SYSTEM_CTRL_CPACR_CP11			(OS_SYSTEM_CTRL_CPACR_CP11_Msk)

#define OS_SYSTEM_CTRL_STCSR_EN_Pos			(0U)
#define OS_SYSTEM_CTRL_STCSR_EN_Msk			(0x1UL << OS_SYSTEM_CTRL_STCSR_EN_Pos)
#define OS_SYSTEM_CTRL_STCSR_EN				(OS_SYSTEM_CTRL_STCSR_EN_Msk)

#define OS_SYSTICK_ENABLE()					OS_SET_BITS(OS_SYSTEM_CTRL->STCSR,   OS_SYSTEM_CTRL_STCSR_EN)
#define OS_SYSTICK_DISABLE()				OS_CLEAR_BITS(OS_SYSTEM_CTRL->STCSR, OS_SYSTEM_CTRL_STCSR_EN)
#define OS_SYSTICK_SET_PRIO(x)				do { OS_SYSTEM_CTRL->SHP[11] = (uint8_t)(( (x) << 4 ) & (uint32_t)0xFFUL); }while(0);

#define OS_SET_PENDSV()						do { OS_SET_BITS(OS_SYSTEM_CTRL->ICSR, OS_SYSTEM_CTRL_ICSR_PENDSVSET); } while(0)
#define OS_PENDSV_SET_PRIO(x)				do { OS_SYSTEM_CTRL->SHP[10] = (uint8_t)(( (x) << 4 ) & (uint32_t)0xFFUL); }while(0);


/* FPU defines
 ---------------------------------------------------*/
#define OS_FPU_BASE							(0xE000EF34)
#define OS_FPU								((FPU_TypeDef*) OS_FPU_BASE)

#define OS_FPU_FPCCR_ASPEN_Pos				(30U)
#define OS_FPU_FPCCR_ASPEN_Msk				(0x1UL << OS_FPU_FPCCR_ASPEN_Pos)
#define OS_FPU_FPCCR_ASPEN					(OS_FPU_FPCCR_ASPEN_Msk)

#define OS_FPU_FPCCR_LSPEN_Pos				(31U)
#define OS_FPU_FPCCR_LSPEN_Msk				(0x1UL << OS_FPU_FPCCR_LSPEN_Pos)
#define OS_FPU_FPCCR_LSPEN					(OS_FPU_FPCCR_LSPEN_Msk)

#define OS_FPU_ENABLE()						OS_SET_BITS(OS_SYSTEM_CTRL->CPACR,   (OS_SYSTEM_CTRL_CPACR_CP10 | OS_SYSTEM_CTRL_CPACR_CP11))
#define OS_FPU_DISABLE()					OS_CLEAR_BITS(OS_SYSTEM_CTRL->CPACR, (OS_SYSTEM_CTRL_CPACR_CP10 | OS_SYSTEM_CTRL_CPACR_CP11))

#define OS_FPU_LAZY_ENABLE()				OS_SET_BITS(OS_FPU->FPCCR,   OS_FPU_FPCCR_LSPEN)
#define OS_FPU_LAZY_DISABLE()				OS_CLEAR_BITS(OS_FPU->FPCCR, OS_FPU_FPCCR_LSPEN)

#define OS_FPU_STATUS_ENABLE()				OS_SET_BITS(OS_FPU->FPCCR,   OS_FPU_FPCCR_ASPEN)
#define OS_FPU_STATUSDISABLE()				OS_CLEAR_BITS(OS_FPU->FPCCR, OS_FPU_FPCCR_ASPEN)

/**********************************************
 * PUBLIC TYPES
 * ********************************************/

typedef struct{
	__OS_IOM uint32_t ACTLR;			//Auxiliary Control Register
	__OS_IOM uint32_t reserved0;		//reserved;
	__OS_IOM uint32_t STCSR;			//SysTick control and status
	__OS_IOM uint32_t STRVR;			//SysTick Reload Value Register
	__OS_IOM uint32_t STCVR; 			//SysTick Current Value Register
	__OS_IM  uint32_t STCR; 			//SysTick Calibration Value Register
	__OS_IOM uint32_t reserved1[824];	//reserved
	__OS_IM  uint32_t CPUID; 			//CPUID Base Register
	__OS_IOM uint32_t ICSR; 			//Interrupt Control and State Register
	__OS_IOM uint32_t VTOR; 			//Vector Table Offset Register
	__OS_IOM uint32_t AIRCR; 			//Application Interrupt and Reset Control Register
	__OS_IOM uint32_t SCR; 				//System Control Register
	__OS_IOM uint32_t CCR; 				//Configuration and Control Register.
	__OS_IOM uint8_t  SHP[12]; 			//System Handler Priority Register
	__OS_IOM uint32_t SHCSR; 			//System Handler Control and State Register
	__OS_IOM uint32_t CFSR; 			//Configurable Fault Status Registers
	__OS_IOM uint32_t HFSR; 			//HardFault Status register
	__OS_IOM uint32_t DFSR; 			//Debug Fault Status Register
	__OS_IOM uint32_t MMFAR; 			//MemManage Address Register
	__OS_IOM uint32_t BFAR; 			//BusFault Address Register
	__OS_IOM uint32_t AFSR; 			//Auxiliary Fault Status Register
	__OS_IM  uint32_t ID_PFR[2]; 		//Processor Feature Register
	__OS_IM  uint32_t ID_DFR0; 			//Debug Features Register 0
	__OS_IM  uint32_t ID_AFR0; 			//Auxiliary Features Register 0
	__OS_IM  uint32_t ID_MMFR[4];		//Memory Model Feature Register
	__OS_IM  uint32_t ID_ISAR[5]; 		//Instruction Set Attributes Register
	__OS_IOM uint32_t reserved2[5];		//reserved
	__OS_IOM uint32_t CPACR; 			//Coprocessor Access Control Register
	__OS_IOM uint32_t reserved3[93];	//reserved
	__OS_OM  uint32_t STIR; 			//Software Triggered Interrupt Register
}SystemControl_TypeDef;

typedef struct{
	__OS_IOM uint32_t FPCCR;		//FP Context Control Register
	__OS_IOM uint32_t FPCAR;		//FP Context Address Register
	__OS_IOM uint32_t FPDSCR;		//FP Default Status Control Register
	__OS_IM  uint32_t MVFR0;		//Media and VFP Feature Register 0, MVFR0
	__OS_IM  uint32_t MVFR1;		//Media and VFP Feature Register 1, MVFR1
}FPU_TypeDef;


#endif /* INC_OS_OS_CORTEXM4_H_ */
