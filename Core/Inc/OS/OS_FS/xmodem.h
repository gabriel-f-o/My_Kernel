/*
 * xmodem.h
 *
 *  Created on: Jul 16, 2022
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_FS_XMODEM_H_
#define INC_OS_OS_FS_XMODEM_H_

#include "common.h"

/**********************************************************
 * PUBLIC TYPES
 **********************************************************/

typedef enum{
	OS_XMODEM_STATE_STOP,
	OS_XMODEM_STATE_RUN,
	__OS_XMODEM_STATE_MAX
}os_xmodem_state;

/**********************************************************
 * PUBLIC FUNCTIONS
 **********************************************************/

/***********************************************************************
 * XMODEM Get State
 *
 * @brief This functions gets the state of the XMODEM protocol
 *
 * @return os_xmodem_state : (1) on going, (0) idle
 *
 **********************************************************************/
os_xmodem_state xModem_getState();

/***********************************************************************
 * XMODEM Receive
 *
 * @brief This functions receives a file via XMODEM protocol
 *
 * @param char* path : [in] the name of the file to create
 *
 **********************************************************************/
void* xModem_rcv(char* path);

#endif /* INC_OS_OS_FS_XMODEM_H_ */
