/*
 * flash.h
 *
 *  Created on: Jul 9, 2022
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_DRIVERS_OS_FLASH_H_
#define INC_OS_OS_DRIVERS_OS_FLASH_H_

#include <stdint.h>

/**********************************************
 * DEFINE
 *********************************************/

#define FLASH_BASE_ADDR 			((uint32_t) _sflash)
#define FLASH_END_ADDR				((uint32_t) _eflash)
#define FLASH_TOTAL_SIZE			((uint32_t) _flash_size)

/**********************************************
 * EXTERNAL VARIABLES
 *********************************************/

extern char _sflash[];
extern char _eflash[];
extern char _flash_size[];

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
 * @return os_err_e : <0 if error. Otherwise the number of bytes written
 *
 **********************************************************************/
os_err_e os_flash_write(uint32_t addr, uint8_t buffer[], size_t len);


/***********************************************************************
 * OS Flash read
 *
 * @brief This function reads a buffer in a given address in the internal flash.
 *
 * @param uint32_t addr		: [ in] First address to read
 * @param uint8_t buffer[]  : [out] Output Buffer
 * @param size_t len 		: [ in] Size of the data buffer
 *
 * @return os_err_e : <0 if error. Otherwise the number of read bytes
 *
 **********************************************************************/
os_err_e os_flash_read(uint32_t addr, uint8_t buffer[], size_t len);


/***********************************************************************
 * OS Flash Erase
 *
 * @brief This function erases sectors from the internal flash, forcing every byte to 0xFF.
 * If the beginning address is not aligned with a sector, an error is thrown.
 *
 * The sectors are :
 * 		Sector  0 : 0x0800 0000 - 0x0800 3FFF, size = 16Kb
 * 		Sector  1 : 0x0800 4000 - 0x0800 7FFF, size = 16Kb
 * 		Sector  2 : 0x0800 8000 - 0x0800 BFFF, size = 16Kb
 * 		Sector  3 : 0x0800 C000 - 0x0800 FFFF, size = 16Kb
 * 		Sector  4 : 0x0801 0000 - 0x0801 FFFF, size = 64Kb
 * 		Sector  5 : 0x0802 0000 - 0x0803 FFFF, size = 128Kb
 * 		Sector  6 : 0x0804 0000 - 0x0805 FFFF, size = 128Kb
 *		Sector  7 : 0x0806 0000 - 0x0807 FFFF, size = 128Kb
 *		Sector  8 : 0x0808 0000 - 0x0809 FFFF, size = 128Kb
 *		Sector  9 : 0x080A 0000 - 0x080B FFFF, size = 128Kb
 *		Sector 10 : 0x080 C0000 - 0x080D FFFF, size = 128Kb
 * 		Sector 11 : 0x080E 0000 - 0x080F FFFF, size = 128Kb
 *
 * @param uint32_t addrBeg	: [in] Beginning address of the sector to erase
 * @param uint32_t secNum   : [in] Number of sectors to erase
 *
 * @return os_err_e : <0 if error. Otherwise the number of sectors erased
 *
 **********************************************************************/
os_err_e os_flash_erase(uint32_t addrBeg, uint32_t secNum);


#endif /* INC_OS_OS_DRIVERS_OS_FLASH_H_ */
