/*
 * lfs_flash.h
 *
 *  Created on: Jul 9, 2022
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_FS_LFS_FLASH_H_
#define INC_OS_OS_FS_LFS_FLASH_H_

#include "common.h"

/**********************************************
 * DEFINES
 *********************************************/

#define SECTOR_SIZE					(128*1024)
#define LFS_BASE_ADDR				((uint32_t)&__lfs_start)
#define LFS_END_ADDR				((uint32_t)&__lfs_end)
#define LFS_BASE_SIZE				((uint32_t)&_LFS_SIZE)

/**********************************************
 * PUBLIC VARIABLES
 *********************************************/

extern uint32_t __lfs_start;
extern uint32_t __lfs_end;
extern uint32_t _LFS_SIZE;

#endif /* INC_OS_OS_FS_LFS_FLASH_H_ */
