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
 * PUBLIC FUNCTIONS
 **********************************************************/

/***********************************************************************
 * XMODEM Receive
 *
 * @brief This functions receives a file via XMODEM protocol
 *
 * @param char* path : [in] the name of the file to create
 *
 **********************************************************************/
void xModem_rcv(char* path);

#endif /* INC_OS_OS_FS_XMODEM_H_ */
