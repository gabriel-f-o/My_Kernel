/*
 * flash.c
 *
 *  Created on: Jul 9, 2022
 *      Author: Gabriel
 */


#include "common.h"

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
int32_t os_flash_write(uint32_t addr, uint8_t buffer[], size_t len){

	/* Check arguments
	 ------------------------------------------------------*/
	if(len == 0) return -2;
	if(buffer == NULL) return -2;
	if(addr < FLASH_BASE_ADDR) return -2;
	if(addr >= FLASH_MAX_ADDR) return -2;

	/* Wait for last operation
	 ------------------------------------------------------*/
	HAL_StatusTypeDef ret = FLASH_WaitForLastOperation(1000);
	ASSERT(ret == HAL_OK);
	if(ret != HAL_OK)
		return -1;

	/* Unlock flash
	 ------------------------------------------------------*/
	ret = HAL_FLASH_Unlock();
	ASSERT(ret == HAL_OK);
	if(ret != HAL_OK)
		return -1;

	/* Write into flash. You can write 1, 2, or 4 bytes at a time
	 ------------------------------------------------------*/
	bool err = 0;
	int32_t pos = 0;
	while(1){

		/* Check alignment and border requirements
		 ------------------------------------------------------*/
		uint32_t writePermitted = 0;
		writePermitted = ( (addr + 1) <= FLASH_MAX_ADDR ) 				   ? 1 : writePermitted;
		writePermitted = ( (addr % 2) == 0 && addr + 2 <= FLASH_MAX_ADDR ) ? 2 : writePermitted;
		writePermitted = ( (addr % 4) == 0 && addr + 4 <= FLASH_MAX_ADDR ) ? 4 : writePermitted;

		/* Calculates the number of bytes to write
		 ------------------------------------------------------*/
		uint32_t lenToWrite = writePermitted < len ? writePermitted : len;
		lenToWrite = lenToWrite == 3 ? 2 : lenToWrite;

		/* Finished writing
		 ------------------------------------------------------*/
		if(lenToWrite == 0 || len == 0) break;

		/* Calculates the writing flag used
		 ------------------------------------------------------*/
		uint32_t writeFlag = FLASH_TYPEPROGRAM_BYTE;
			 	 writeFlag = lenToWrite == 2 ? FLASH_TYPEPROGRAM_HALFWORD : writeFlag;
			 	 writeFlag = lenToWrite == 4 ?     FLASH_TYPEPROGRAM_WORD : writeFlag;

		/* Store data in a uint64_t to avoid possible alignment issues
		 ------------------------------------------------------*/
		uint64_t data = 0;
		memcpy(&data, &buffer[pos], lenToWrite);

		/* Program flash
		 ------------------------------------------------------*/
		ret = HAL_FLASH_Program(writeFlag, addr, data);
		ASSERT(ret == HAL_OK);
		if(ret != HAL_OK){
			err = 1;
			break;
		}

		/* Manipulate local vaiables
		 ------------------------------------------------------*/
		pos += (int32_t)lenToWrite; //Counts the number of bytes actually written
		addr += lenToWrite; //Moves to the next address
		len -= lenToWrite; //reduces length

		/* Wait for operation to end
		 ------------------------------------------------------*/
		HAL_StatusTypeDef ret = FLASH_WaitForLastOperation(1000);
		ASSERT(ret == HAL_OK);
		if(ret != HAL_OK){
			err = 1;
			break;
		}
	}

	/* Lock flash again and return error or the amount of bytes
	 ------------------------------------------------------*/
	ret = HAL_FLASH_Lock();
	ASSERT(ret == HAL_OK);
	return err == 1 ? -1 : pos;
}


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
int32_t os_flash_read(uint32_t addr, uint8_t buffer[], size_t len){

	/* Argument check
	 ------------------------------------------------------*/
	if(len == 0) return -2;
	if(buffer == NULL) return -2;
	if(addr < FLASH_BASE_ADDR) return -2;
	if(addr >= FLASH_MAX_ADDR) return -2;

	/* Check border and calculates the amout of data to read
	 ------------------------------------------------------*/
	size_t readBytes = len;
	if(addr + len > FLASH_MAX_ADDR){
		readBytes -= addr + len - FLASH_MAX_ADDR;
	}

	/* Copy data into buffer
	 ------------------------------------------------------*/
	memcpy(buffer, (void*)addr, readBytes);

	/* Return the amount of bytes
	 ------------------------------------------------------*/
	return (int32_t)readBytes;
}


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
int32_t os_flash_erase(uint32_t addrBeg, uint32_t addrEnd){

	/* Check arguments
	 ------------------------------------------------------*/
	if(addrBeg < FLASH_BASE_ADDR) return -2;
	if(addrBeg >= FLASH_MAX_ADDR) return -2;
	if(addrEnd < FLASH_BASE_ADDR) return -2;
	if(addrBeg > addrEnd) return -2;

	/* Wait for operation to end
	 ------------------------------------------------------*/
	HAL_StatusTypeDef ret = FLASH_WaitForLastOperation(1000);
	ASSERT(ret == HAL_OK);
	if(ret != HAL_OK)
		return -1;

	/* Unlock flash
	 ------------------------------------------------------*/
	ret = HAL_FLASH_Unlock();
	ASSERT(ret == HAL_OK);
	if(ret != HAL_OK)
		return -1;

	/* Loop until there are sectors to erase
	 ------------------------------------------------------*/
	bool error = 0;
	uint32_t addrCur = addrBeg;
	int32_t sectNum = 0;
	while(addrCur <= addrEnd && addrCur < FLASH_MAX_ADDR){

		/* Calculates which sector to erase
		 ------------------------------------------------------*/
		uint32_t Sector = 0;
		sectNum++;
		if(0x08000000 <= addrCur && addrCur <= 0x08003FFF){
			Sector = 0;
			addrCur &= 0xFFFFC000;
			addrCur += 16*1024;
		}
		else if(0x08004000 <= addrCur && addrCur <= 0x08007FFF) {
			Sector = 1;
			addrCur &= 0xFFFFC000;
			addrCur += 16*1024;
		}
		else if(0x08008000 <= addrCur && addrCur <= 0x0800BFFF) {
			Sector =  2;
			addrCur &= 0xFFFFC000;
			addrCur += 16*1024;
		}
		else if(0x0800C000 <= addrCur && addrCur <= 0x0800FFFF) {
			Sector =  3;
			addrCur &= 0xFFFFC000;
			addrCur += 16*1024;
		}
		else if(0x08010000 <= addrCur && addrCur <= 0x0801FFFF) {
			Sector =  4;
			addrCur &= 0xFFFF0000;
			addrCur += 64*1024;
		}
		else if(0x08020000 <= addrCur && addrCur <= 0x0803FFFF) {
			Sector =  5;
			addrCur &= 0xFFFE0000;
			addrCur += 128*1024;
		}
		else if(0x08040000 <= addrCur && addrCur <= 0x0805FFFF) {
			Sector =  6;
			addrCur &= 0xFFFE0000;
			addrCur += 128*1024;
		}
		else if(0x08060000 <= addrCur && addrCur <= 0x0807FFFF) {
			Sector =  7;
			addrCur &= 0xFFFE0000;
			addrCur += 128*1024;
		}
		else if(0x08080000 <= addrCur && addrCur <= 0x0809FFFF) {
			Sector =  8;
			addrCur &= 0xFFFE0000;
			addrCur += 128*1024;
		}
		else if(0x080A0000 <= addrCur && addrCur <= 0x080BFFFF) {
			Sector =  9;
			addrCur &= 0xFFFE0000;
			addrCur += 128*1024;
		}
		else if(0x080C0000 <= addrCur && addrCur <= 0x080DFFFF) {
			Sector =  10;
			addrCur &= 0xFFFE0000;
			addrCur += 128*1024;
		}
		else if(0x080E0000 <= addrCur && addrCur <= 0x080FFFFF) {
			Sector =  11;
			addrCur &= 0xFFFE0000;
			addrCur += 128*1024;
		}
		else{
			error = 1;
			break;
		}

		/* Erase sector
		 ------------------------------------------------------*/
		FLASH_Erase_Sector(Sector, FLASH_VOLTAGE_RANGE_3);

		/* Wait for operation to end
		 ------------------------------------------------------*/
		HAL_StatusTypeDef ret = FLASH_WaitForLastOperation(10000);
		ASSERT(ret == HAL_OK);
		if(ret != HAL_OK){
			error = 1;
			break;
		}
	}

	/* Lock flash
	 ------------------------------------------------------*/
	ret = HAL_FLASH_Lock();
	ASSERT(ret == HAL_OK);
	if(ret != HAL_OK){
		error = 1;
	}

	return error == 1 ? -1 : sectNum;
}
