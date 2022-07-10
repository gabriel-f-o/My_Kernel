/*
 * flash.h
 *
 *  Created on: Jul 9, 2022
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_DRIVERS_OS_FLASH_H_
#define INC_OS_OS_DRIVERS_OS_FLASH_H_

#include "common.h"

/**********************************************
 * DEFINE
 *********************************************/

#define FLASH_MAX_ADDR		0x8100000UL
#define FLASH_BASE_ADDR		0x8000000UL
#define FLASH_TOTAL_SIZE	0x0100000UL

/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Flash Write
 *
 * @brief This function writes a buffer in a given address in the internal flash. This function does not erase the contents, if there are any.
 *
 * @param uint32_t addr		: [in] First address to write
 * @param uint8_t buffer[]  : [in] Buffer containing the data to write
 * @param size_t len 		: [in] Size of the data buffer
 *
 * @return int32_t : <0 if error. Otherwise the number of bytes written
 *
 **********************************************************************/
int32_t os_flash_write(uint32_t addr, uint8_t buffer[], size_t len);


/***********************************************************************
 * OS Flash read
 *
 * @brief This function reads a buffer in a given address in the internal flash.
 *
 * @param uint32_t addr		: [ in] First address to read
 * @param uint8_t buffer[]  : [out] Output Buffer
 * @param size_t len 		: [ in] Size of the data buffer
 *
 * @return int32_t : <0 if error. Otherwise the number of read bytes
 *
 **********************************************************************/
int32_t os_flash_read(uint32_t addr, uint8_t buffer[], size_t len);


/***********************************************************************
 * OS Flash Erase
 *
 * @brief This function erases sectors from the internal flash, forcing every byte to 0xFF. The sectors to be erased are determined with the addresses passed.
 * All the sectors that have at least one byte in the range of addresses will be erased. e.g. addrBeg = 0x0809FFFF and addrEnd 0x080A0000, sectors 8 and 9 will be erased (from 0x08080000 to 0x080BFFFF inclusive)
 *
 * @param uint32_t addrBeg	: [in] Beginning address
 * @param uint32_t addrEnd	: [in] End address
 *
 * @return int32_t : <0 if error. Otherwise the number of sectors erased
 *
 **********************************************************************/
int32_t os_flash_erase(uint32_t addrBeg, uint32_t addrEnd);


#endif /* INC_OS_OS_DRIVERS_OS_FLASH_H_ */
