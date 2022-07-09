/*
 * OS_Tick.h
 *
 *  Created on: Oct 23, 2021
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_TICK_H_
#define INC_OS_OS_TICK_H_

#include "OS/OS_Core/OS_Common.h"

/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Get ms Tick
 *
 * @brief This function returns the OS' ms tick count
 *
 * @return uint32_t tick count
 **********************************************************************/
uint32_t os_getMsTick();


/***********************************************************************
 * OS Tick
 *
 * @brief This function is called periodically to tick tasks' counters and call the scheduler when necessary
 *
 * @param uint32_t increment : [in] amount of ms to increment
 *
 **********************************************************************/
void os_tick(uint32_t ms_inc);


#endif /* INC_OS_OS_TICK_H_ */
