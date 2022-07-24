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
	if(addr >= FLASH_END_ADDR) return -2;

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
		writePermitted = ( (addr + 1) <= FLASH_END_ADDR ) 				   ? 1 : writePermitted;
		writePermitted = ( (addr % 2) == 0 && addr + 2 <= FLASH_END_ADDR ) ? 2 : writePermitted;
		writePermitted = ( (addr % 4) == 0 && addr + 4 <= FLASH_END_ADDR ) ? 4 : writePermitted;

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
	if(addr >= FLASH_END_ADDR) return -2;

	/* Check border and calculates the amout of data to read
	 ------------------------------------------------------*/
	size_t readBytes = len;
	if(addr + len > FLASH_END_ADDR){
		readBytes -= addr + len - FLASH_END_ADDR;
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
 *		Sector 10 : 0x080C 0000 - 0x080D FFFF, size = 128Kb
 * 		Sector 11 : 0x080E 0000 - 0x080F FFFF, size = 128Kb
 *
 * @param uint32_t addrBeg	: [in] Beginning address of the sector to erase
 * @param uint32_t secNum   : [in] Number of sectors to erase
 *
 * @return int32_t : <0 if error. Otherwise the number of sectors erased
 *
 **********************************************************************/
int32_t os_flash_erase(uint32_t addrBeg, uint32_t secNum){

	/* Check arguments
	 ------------------------------------------------------*/
	if(addrBeg < FLASH_BASE_ADDR) return -2;
	if(addrBeg >= FLASH_END_ADDR) return -2;

	/* Select first sector
	 ------------------------------------------------------*/
	int firstSector = -1;

	if(addrBeg == 0x08000000) firstSector = FLASH_SECTOR_0;
	if(addrBeg == 0x08004000) firstSector = FLASH_SECTOR_1;
	if(addrBeg == 0x08008000) firstSector = FLASH_SECTOR_2;
	if(addrBeg == 0x0800C000) firstSector = FLASH_SECTOR_3;
	if(addrBeg == 0x08010000) firstSector = FLASH_SECTOR_4;
	if(addrBeg == 0x08020000) firstSector = FLASH_SECTOR_5;
	if(addrBeg == 0x08040000) firstSector = FLASH_SECTOR_6;
	if(addrBeg == 0x08060000) firstSector = FLASH_SECTOR_7;
	if(addrBeg == 0x08080000) firstSector = FLASH_SECTOR_8;
	if(addrBeg == 0x080A0000) firstSector = FLASH_SECTOR_9;
	if(addrBeg == 0x080C0000) firstSector = FLASH_SECTOR_10;
	if(addrBeg == 0x080E0000) firstSector = FLASH_SECTOR_11;

	if(firstSector < 0)	return -2;

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

	/* Calculates the maximum number of sectors we can erase, and cap accordingly
	 ------------------------------------------------------*/
	uint32_t sect_nb_max = (uint32_t) (11 - firstSector + 1);
	secNum = sect_nb_max < secNum ? sect_nb_max : secNum;

	/* Prepare erase configuration
	 ------------------------------------------------------*/
	bool error = 0;
	uint32_t SectorError;
	FLASH_EraseInitTypeDef eraseConf = {
			.TypeErase    = FLASH_TYPEERASE_SECTORS, // Erase sectors, not mass erase
			.Banks	      = FLASH_BANK_1,			 // Unused outside of mass erase
			.Sector	      = (uint32_t) firstSector,	 // Inform first sector to erase
			.NbSectors    = secNum,					 // Inform number of sectors to erase
			.VoltageRange = FLASH_VOLTAGE_RANGE_3,	 // Select voltage range
	};

	/* Erase sectors
	 ------------------------------------------------------*/
	ret = HAL_FLASHEx_Erase(&eraseConf, &SectorError);
	ASSERT(ret == HAL_OK);
	if(ret != HAL_OK){
		error = 1;
	}

	/* Wait for operation to end
	 ------------------------------------------------------*/
	ret = FLASH_WaitForLastOperation(1000);
	ASSERT(ret == HAL_OK);
	if(ret != HAL_OK){
		error = 1;
	}

	/* Lock flash
	 ------------------------------------------------------*/
	ret = HAL_FLASH_Lock();
	ASSERT(ret == HAL_OK);
	if(ret != HAL_OK){
		error = 1;
	}

	return error == 1 ? -1 : (int32_t)secNum;
}
