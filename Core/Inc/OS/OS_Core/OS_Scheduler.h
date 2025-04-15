/*
 * OS_Scheduler.h
 *
 *  Created on: Oct 23, 2021
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_SCHEDULER_H_
#define INC_OS_OS_SCHEDULER_H_

#include "OS/OS_Core/OS_Common.h"

/**********************************************
 * PUBLIC TYPES
 *********************************************/

typedef enum{
	OS_SCHEDULER_STOP,
	OS_SCHEDULER_START,
} os_scheduler_state_e;

/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Scheduler start
 *
 * @brief Starts the scheduler and yields the current task if there is a higher priority one waiting
 *
 * @return os_err_e : OS_ERR_OK if OK
 *
 **********************************************************************/
os_err_e os_scheduler_start();


/***********************************************************************
 * OS Scheduler stop
 *
 * @brief Stops the scheduler
 *
 **********************************************************************/
os_err_e os_scheduler_stop();


/***********************************************************************
 * OS Scheduler State get
 *
 * @brief Returns the state of the scheduler
 *
 * @return os_scheduler_state_e : stop or start
 *
 **********************************************************************/
os_scheduler_state_e os_scheduler_state_get();


#endif /* INC_OS_OS_SCHEDULER_H_ */
