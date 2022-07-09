/*
 * flash.h
 *
 *  Created on: Jul 9, 2022
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_DRIVERS_OS_FLASH_H_
#define INC_OS_OS_DRIVERS_OS_FLASH_H_

#include "common.h"

int32_t flash_write(uint32_t addr, uint8_t buffer[], size_t len);


int32_t flash_read(uint32_t addr, uint8_t buffer[], size_t len);


int32_t flash_erase(uint32_t addrBeg, uint32_t addrEnd);


#endif /* INC_OS_OS_DRIVERS_OS_FLASH_H_ */
